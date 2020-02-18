// PeerNetNode.cpp                                         (c) DIR, 2019
//
//              Реализация классов
// CNamedPipe   - Именованный канал
// CPeerNetNode - Узел одноранговой сети
//
////////////////////////////////////////////////////////////////////////

//#include "stdafx.h"
#include "pch.h"
#include <strsafe.h>
#include "PeerNetDlg.h" //#include "PeerNetNode.h" //#include "BlkChain.h"

#ifdef _DEBUG
 #undef THIS_FILE
 static char THIS_FILE[]=__FILE__;
 #define new DEBUG_NEW
#endif

//  Глобальные внеклассные объекты данных
//----------------------------------------------------------------------
CRITICAL_SECTION  g_cs; //критическая секция

//======================================================================
// Определение класса CNamedPipe -
//  Базовый для работы с именованными каналами
//----------------------------------------------------------------------

//  Инициализация статических элементов данных
//----------------------------------------------------------------------

// Конструктор и деструктор
CNamedPipe::CNamedPipe(CPeerNetNode *pArentNode,
                       WORD iCncNode/* = UNKNOWN_NODE*/)
{
  m_pArentNode = pArentNode;    //указатель родительского объекта
  m_pThread = NULL;
  m_iCncNode = iCncNode;
  m_hPipe = INVALID_HANDLE_VALUE;
  for (int i = 0; i < EVENTCOUNT; i++)
    m_hEvents[i] = INVALID_HANDLE_VALUE;
  m_bUsy = false;       //флаг "Канал занят"
  m_pQueue = NULL;      //присоединённая очередь блоков на передачу
  m_nRead = 0;          //количество принятых сообщений
  m_nWritten = 0;       //количество отправленных сообщений
  m_eStatus = PS_None;          //состояние канала
  ClearError();
  memset(m_szErrorMes, 0, sizeof(m_szErrorMes));
}
CNamedPipe::~CNamedPipe()
{
  for (int i = 0; i < EVENTCOUNT; i++) {
    if (m_hEvents[i] > 0)
      ::CloseHandle(m_hEvents[i]);
  }
  if (m_pQueue) { delete m_pQueue;  m_pQueue = NULL; }
}

// Послать запрос серверу данного именованного канала
UINT  CNamedPipe::SendRequest()
{
  // Сбросить событие "Запрос к серверу узла k готов"
  BOOL  bSuccess = ::ResetEvent(m_hEvents[EvReqstReady]);
  if (!bSuccess) {
    // Ошибка вызова ResetEvent(): запомнить код ошибки и вывести 
    //сообщение об ошибке.
    ProcessError(NULL, _T("::ResetEvent(hEvReqstReady)"));
    return 9;
  }
  // Послать запрос серверу канала
  DWORD  dwBytes = (DWORD)RequestBytes(), cbWritten;
  bSuccess = ::WriteFile(
    m_hPipe,        //хендл канала
    RequestMessage(),   //буфер запроса pNP->m_chRequest
    dwBytes,        //длина запроса в байтах m_wRequestBytes
    &cbWritten,     //байт передано
    NULL            //not overlapped
  );
  if (!bSuccess || (cbWritten != dwBytes)) {
    // Ошибка передачи запроса серверу канала
    ProcessError(NULL, _T("SendRequest() ::WriteFile()"));
    return 7;
  }
  return 0;
} // CNamedPipe::SendRequest()

// Принять ответ сервера данного именованного канала
UINT  CNamedPipe::ReceiveReply(bool bNoEvent/* = false*/)
//bNoEvent - флаг "Не возбуждать событие EvRepReceivd"
{
  BOOL bSuccess;  bool bLoop = false;  UINT nRet = 0;
  TMessageBuf *pmbReply = NULL;  DWORD cbRead;
  while (true) {
    // Начало пакета читать в основной буфер
    if (pmbReply == NULL)  pmbReply = &m_mbReply;
    bSuccess = ::ReadFile(
      m_hPipe,                  //pipe handle
      pmbReply->MessBuffer(),   //адрес буфера для приёма ответа
      (DWORD)BufferSize(),      //размер буфера в байтах
      &cbRead,                  //number of bytes read
      NULL                      //not overlapped
    );
	pmbReply->_wMessBytes = (WORD)cbRead;
    if (bSuccess)  break;
    // Возможно, есть ошибки - уточнить
    if (SysErrorCode() == ERROR_MORE_DATA) {
      // Это не ошибка, а длинное сообщение - первый блок данных принять 
      //в основной буфер, остальные в динамический (пропустить "хвост")
      if (!bLoop) {
        // Первый проход - выделить динамический буфер для "хвоста"
        pmbReply = new TMessageBuf();  bLoop = true;
      }
      bSuccess = TRUE;
    }
    if (!bSuccess) {
      // Ошибка при получении ответа сервера
      ProcessError(NULL, _T("::ReadFile()"));
      nRet = 5;  break;
    }
  } //while (bLoop);
  if (!bNoEvent) {
    // Возбудить событие "Ответ сервера получен"
    bSuccess = ::SetEvent(m_hEvents[EvRepReceivd]);
    if ((nRet == 0) && !bSuccess) {
      // Ошибка вызова SetEvent(): запомнить код ошибки и вывести 
      //сообщение об ошибке.
      ProcessError(NULL, _T("::SetEvent(hEvRepReceivd)"));
      nRet = 10;
    }
  }
  // Удалить динамический буфер
  if (bLoop)  delete pmbReply;
  return nRet;
} // CNamedPipe::ReceiveReply()

// Послать запрос серверу данного именованного канала и получить ответ.
//Метод возвращает буфер в динамической памяти с ответом сервера.
TMessageBuf * CNamedPipe::RequestAndReply(TMessageBuf &mbRequest)
//mbRequest - внешний буфер с запросом
{
  bool b;  TMessageBuf *pmbRequest;
  if (m_bUsy) {
    // Канал занят - поставить сообщение в очередь
    if (m_pQueue == NULL)  m_pQueue = new CCyclicQueue();
    b = m_pQueue->Put(mbRequest);
    return NULL;
  }
  m_bUsy = true;
  do {
    // Заполнить буфер запроса
    if (m_pQueue == NULL || m_pQueue->IsEmpty())
      // Входная очередь пуста - взять сообщение из аргумента
      m_mbRequest = mbRequest;  //заполнить буфер запроса
    else {
      // Входная очередь не пуста - взять сообщение из очереди
      b = m_pQueue->Get(pmbRequest);
      m_mbRequest = *pmbRequest;  //заполнить буфер запроса
    }
#ifdef _TRACE
    TraceW(mbRequest.Message(), false);
#endif
    // Возбудить событие "Запрос к серверу готов"
    BOOL bSuccess = ::SetEvent(m_hEvents[EvReqstReady]);
    if (!bSuccess) {
      // Ошибка вызова SetEvent(): запомнить код ошибки и вывести 
      //сообщение об ошибке.
      ProcessError(NULL, _T("::SetEvent(hEvReqstReady)"));
      continue; //return NULL;
    }
    // Ждать события "Ответ сервера получен"
#ifdef _TRACE
    TraceW(_T("::WaitForSingleObject(hEvRepReceivd)"), false);
#endif
    DWORD dwWait = ::WaitForSingleObject(
      m_hEvents[EvRepReceivd], PIPE_TIMEOUT //INFINITE
    );
    if (dwWait != WAIT_OBJECT_0) {
      // Ошибка ожидания события //== WAIT_FAILED
      if (dwWait == WAIT_TIMEOUT) {
        swprintf_s(m_szErrorMes, ERMES_BUFSIZE,
                   _T("::WaitForSingleObject(hEvRepReceivd) returned. %s"),
                   _T("The time-out interval elapsed"));
      }
      else {
        swprintf_s(m_szErrorMes, ERMES_BUFSIZE,
                   _T("::WaitForSingleObject(hEvRepReceivd) %s %d"),
                   _T("failed with error code"), SysErrorCode());
      }
#ifdef _TRACE
      TraceW(m_szErrorMes);
#endif
      //return NULL;
    }
    // Сбросить событие "Ответ сервера получен"
    bSuccess = ::ResetEvent(m_hEvents[EvRepReceivd]);
    if (!bSuccess) {
      // Ошибка вызова ResetEvent(): запомнить код ошибки и вывести 
      //сообщение об ошибке.
      ProcessError(NULL, _T("::ResetEvent(hEvRepReceivd)"));
      //return NULL;
    }
    m_bUsy = (m_pQueue != NULL) && !m_pQueue->IsEmpty();
  } while (m_bUsy);
  // Вернуть ответ сервера
#ifdef _TRACE
  TraceW(m_mbReply.Message(), false);
#endif
  TMessageBuf *pmb = new TMessageBuf(m_mbReply.MessBuffer(), m_mbReply.MessageBytes());
  return pmb;
} // CNamedPipe::RequestAndReply()

// Дать сообщение о системной или внутренней ошибке. Если в буфере 
//m_szErrorMes нет сообщения, то выводится системное сообщение об ошибке
LPTSTR  CNamedPipe::ErrorMessage(LPTSTR lpszFunction/* = NULL*/)
//lpszFunction - имя функции, при вызове которой произошла ошибка
{
  if (m_szErrorMes[0] == _T('\0'))
  {
    TCHAR chMsgBuf[CHARBUFSIZE];    //буфер системного сообщения
    memset(chMsgBuf, 0, sizeof(chMsgBuf));
    //m_dwSysError = ::GetLastError();    the last-error code
    //Retrieve the system error message
    DWORD dwLeng = ::FormatMessage(
      //FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_IGNORE_INSERTS |
      FORMAT_MESSAGE_FROM_SYSTEM,   //DWORD - флаги режима обращения
      NULL,         //LPCVOID - The location of the message definition
      m_dwSysError, //DWORD - номер системной ошибки //_ENGLISH
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), //DWORD dwLanguageId, _ENGLISH_US
      chMsgBuf,     //LPTSTR - буфер сообщения
      0, NULL       //опционный список аргументов - нет
    );
    //Сформировать сообщение об ошибке в буфере m_szErrorMes
    if (lpszFunction == NULL)
      StringCchPrintf(m_szErrorMes, ERMES_BUFSIZE,
                      _T("System error %ld occur: %s"),
                      m_dwSysError, chMsgBuf);
    else
      StringCchPrintf(m_szErrorMes, ERMES_BUFSIZE,
                      _T("%s failed with error %ld: %s"),
                      lpszFunction, m_dwSysError, chMsgBuf);
  }
  return m_szErrorMes;
} // CNamedPipe::ErrorMessage

// Обработать ошибку
void  CNamedPipe::ProcessError(LPTSTR lpszMesg, LPTSTR lpszFunction/* = NULL*/)
//lpszMesg - информационное сообщение, которое нужно вывести
//            перед сообщением об ошибке
//lpszFunction - имя функции, при вызове которой произошла ошибка
{
  if (!Success()) {
    if (lpszMesg) {
#ifdef _TRACE
      TraceW(lpszMesg, false);
#endif
    }
    if (lpszFunction) {
#ifdef _TRACE
      TraceW(ErrorMessage(lpszFunction));
#endif
    }
    ClearError();
  }
} // CNamedPipe::ProcessError()

#ifdef _TRACE
//static  Вывести трассировочное сообщение
void  CNamedPipe::TraceW(LPTSTR lpszMes, bool bClearAfter/* = true*/)
{
  TCHAR szMessBuf[ERMES_BUFSIZE];
  if ((DWORD)m_hPipe == 0xfeeefeee || (DWORD)m_pThread == 0xfeeefeee)  return;
  swprintf_s(szMessBuf, ERMES_BUFSIZE,
             _T("PipeHndl=0x%04X,ThreadID=0x%04X: %s"),
             (UINT)m_hPipe, m_pThread->m_nThreadID, lpszMes);
  //EnterCriticalSection(&g_cs);
  m_pArentNode->TraceW(szMessBuf, bClearAfter);
  //LeaveCriticalSection(&g_cs);
} // CNamedPipe::TraceW()
#endif

// Конец определения класса CNamedPipe ---------------------------------


////////////////////////////////////////////////////////////////////////
// Класс CCyclicQueue - Циклическая очередь принятых блоков DTB

// Конструктор и деструктор
CCyclicQueue::CCyclicQueue()
{
  memset(m_mbMsg, 0, sizeof(m_mbMsg));  //обнулить массив очереди
  m_iHead  = 0;     //"старая" позиция головы очереди
  m_iTail  = 0;     //"новая" позиция хвоста очереди
  m_nCount = 0;     //количество блоков в очереди
} // CCyclicQueue::CCyclicQueue()

//public:
// Поставить блок в очередь
bool  CCyclicQueue::Put(TMessageBuf &mbMsg)
{
  if (m_nCount == CLCQUEUESIZE)  return false;
  m_mbMsg[m_iTail].PutMessage(mbMsg.Message());
  Increment(m_iTail);  m_nCount++;
  return true;
} // CCyclicQueue::Put()

// Извлечь блок из очереди
bool  CCyclicQueue::Get(TMessageBuf *&pmb)
{
  if (m_nCount == 0)  return false;
  pmb =  m_mbMsg + m_iHead;
  Increment(m_iHead);  m_nCount--;
  return true;
} // CCyclicQueue::Get()

// Сместить циклически указатель позиции
void  CCyclicQueue::Increment(int &iNdx)
{
  iNdx++;  iNdx &= CLCQUEUEMASK;
} // CCyclicQueue::Increment()

// Конец определения класса CCyclicQueue -------------------------------

//volatile CNamedPipe * m_pNPListen;    //NP в состоянии Listen static

////////////////////////////////////////////////////////////////////////
// Определение класса CPeerNetNode - Узел одноранговой сети

//  Инициализация статических элементов данных
//----------------------------------------------------------------------

// Конструктор и деструктор
CPeerNetNode::CPeerNetNode(CPeerNetDialog *pMain, CBlockChain *pBlkChain)
{
  BOOL b = InitializeCriticalSectionAndSpinCount(&g_cs, 0x80000400);
  m_pMainWin = pMain;       //окно программы
  m_pBlkChain = pBlkChain;  //модель блок-чейн
  m_pSrvThread = NULL;      //поток сервера Listen
  m_nNodes = 0;
  m_iOwnNode = UNKNOWN_NODE;    //"номер собственного узла пока неизвестен"
  memset(m_pSrvNPs, 0, sizeof(m_pSrvNPs));
  memset(m_pCliNPs, 0, sizeof(m_pCliNPs));
#ifdef _TRACE
  m_pTraceFile = NULL;
#endif
}
CPeerNetNode::~CPeerNetNode()
{
  ClearNode();
  DeleteCriticalSection(&g_cs);
} // CPeerNetNode::~CPeerNetNode()

//public:
// Инициализировать узел одноранговой сети. Метод возвращает значение:
//true - успешное завершение, false - произошла ошибка
bool  CPeerNetNode::StartupNode()
{
  WORD iNode;  bool b, bRet = true;  UINT nRet = 0;
  if (IsNodeRunning())  return false;
#ifdef _TRACE
  TraceW(_T("StartupNode()-enter"), false);
#endif
  m_iOwnNode = UNKNOWN_NODE;
  // Запустить клиенты для всех существующих узлов сети
  for (m_nNodes = iNode = 0; iNode < NODECOUNT; iNode++) {
    // Метод StartNPClient(iNode), если канал со стороны сервера открыт, 
    //создаёт объект CNamedPipe со стороны клиента и заносит указатель на 
    //него в массив m_pCliNPs[iNode]:
    b = StartNPClient(iNode);
    if (!b && m_iOwnNode == UNKNOWN_NODE)
      // В сети нет узла с номером iNode - занести в список свободных
      m_iOwnNode = iNode;
  }
  // Получить номер "своего" узла - первый свободный номер узла сети
  if (m_iOwnNode == UNKNOWN_NODE) {
    // Нет ни одного свободного номера узла сети - работа невозможна, 
    //остановить узел
    ShutdownNode();
  }
  else {
	// Отобразить номер "своего" узла в заголовке окна
    m_pMainWin->ShowOwnNodeNumber(m_iOwnNode);
	// Дать индекс авторизованного пользователя
    bRet = ((m_iOwner = m_pBlkChain->GetAuthorizedUser()) != UNAUTHORIZED);
    if (bRet) {
      m_bTerm = false;
#ifdef _TRACE
      InitTracing();  //открыть трассировочный файл
#endif
      // Запустить серверный поток "своего" узла m_iOwnNode
      bRet = StartNPServer();
      if (bRet)  m_nNodes++;  //к колич "других" узлов добавить "свой" узел
    }
  }
#ifdef _TRACE
  TraceW(_T("StartupNode()-exit"), false);
#endif
  return bRet;
} // CPeerNetNode::StartupNode()

//public:
// Остановить работу узла одноранговой сети
void  CPeerNetNode::ShutdownNode()
{
#ifdef _TRACE
  TraceW(_T("ShutdownNode()-enter"), false);
#endif
  TCHAR szMessBuf[ERMES_BUFSIZE];
  CNamedPipe *pNP;  UINT nRet;
  TMessageBuf mbRequest;
  // Оповестить все остальные узлы сети об останове сервера узла m_iOwnNode
  for (WORD iNode = 0; iNode < NODECOUNT; iNode++) {
    if (iNode == m_iOwnNode || (pNP = m_pCliNPs[iNode]) == NULL)
      continue;
    // Послать серверу узла iNode команду разъединения с узлом m_iOwnNode
    swprintf_s(szMessBuf, ERMES_BUFSIZE, _T("TRM %02d%02d"),
               m_iOwnNode, pNP->m_iCncNode);    //формирование
    pNP->m_mbRequest.PutMessage(szMessBuf);     //занесение
#ifdef _TRACE
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("ShutdownNode(): Request sent: %s"),
               pNP->m_mbRequest.Message());
    pNP->TraceW(szMessBuf);
#endif
    // Послать запрос на разъединение (ответ не нужен)
    nRet = pNP->SendRequest();
  } // for
  // Остановить поток сервера Listen: NPServerThreadProc()
  if (m_iOwnNode != UNKNOWN_NODE && m_pNPListen)
  {
    m_bTerm = true;     //поднять флаг "Остановить поток сервера Listen"
    // Формирование имени канала
    TCHAR  chPipeName[NAMEBUFSIZE];    //буфер для имени канала
    swprintf_s(chPipeName, NAMEBUFSIZE, PIPENAME_FRMT, m_iOwnNode);
    // Установить соединение со "своим" сервером, чтобы вывести поток 
    //сервера из ожидания
    HANDLE  hPipe = ::CreateFile(
      chPipeName,       //имя канала
      GENERIC_READ |    //вид доступа: чтение (приём от сервера)
      GENERIC_WRITE,    // и запись (передача серверу)
      0, NULL,          //no sharing, default security attributes
      OPEN_EXISTING,    //opens existing pipe
      0, NULL           //default attributes, no template file
    );
    // Разорвать связь по каналу
    CNamedPipe *pNPListen = (CNamedPipe *)m_pNPListen;
    if (!::DisconnectNamedPipe(pNPListen->m_hPipe)) {
      //Ошибка разъединения клиента с сервером
      pNPListen->ProcessError(
        NULL, _T("::DisconnectNamedPipe(m_pNPListen->m_hPipe)")
      );
    }
    // Close the handles to the pipe instance
    BOOL bOK = ::CloseHandle(pNPListen->m_hPipe);
    delete m_pNPListen;  m_pNPListen = NULL;
    //m_iOwnNode = UNKNOWN_NODE;  m_nNodes = 0;
  }
#ifdef _TRACE
  TraceW(_T("ShutdownNode()-exit"), false);
  // Закрыть трассировочный файл
  if (m_pTraceFile) {
    fclose(m_pTraceFile);  m_pTraceFile = NULL;
  }
#endif
  m_pBlkChain->CloseRegisters();
  ClearNode();
} // CPeerNetNode::ShutdownNode()

//  При запуске узла сначала создаются клиентские NP, затем серверные.
//В ответ на создание клиентского конца канала на запускаемом узлес на 
//удалённом узле kNode создаётся серверный конец канала, который не может
//получить номер узла, с которым он связан (iNode), так как на тот момент 
//номер ещё не определён.
//  Номер запускаемого узла iNode определяется после создания клиентских
//каналов для всех обнаруженных активных узлов.
//  Идентификация серверного канала на удалённом узле kNode осуществляется при
//получении им в его потоке Instance сообщения SCL <iNode><kNode><iUser>, где 
//iNode-номер запускаемого узла, kNode-номер удалённого узла, которому 
//принадлежит поток Instance, iUser-номер пользователя в узле iNode.

//  Завершить (отобразить в интерфейсе) присоединение клиента на
//заданном удалённом узле к нашему серверу при запуске нашего сервера
void  CPeerNetNode::ConnectRemoteClient(WORD iNode, WORD iOwner,
                                        CNamedPipe *pNP)
//iNode  - индекс запускаемого удалённого узла
//iOwner - индекс пользователя-владельца удалённого узла
//pNP    - серверный канал нашего узла kNode, которым он связан с узлом iNode
{
  EnterCriticalSection(&g_cs);
  // Идентификация серверного канала
  pNP->m_iCncNode = iNode;
  //pNP->m_iCncOwner = iOwner;
  m_pSrvNPs[iNode] = pNP;   //серверные каналы узла kNode
  LeaveCriticalSection(&g_cs);
  // Запустить нашего клиента, который соединится с сервером узла iNode
  StartNPClient(iNode);
  m_pMainWin->AddRemoteNode(iNode, iOwner);
} // CPeerNetNode::ConnectRemoteClient()

// Завершить (отобразить в интерфейсе) отсоединение клиента на заданном 
//удалённом узле от нашего сервера при останове удалённого узла
void  CPeerNetNode::DisconnectRemoteClient(WORD iNode)
{
  m_pMainWin->DeleteRemoteNode(iNode); //удалить узел из списка
  EnterCriticalSection(&g_cs);
  m_pSrvNPs[iNode] = NULL;
  m_nNodes--;
  LeaveCriticalSection(&g_cs);
} // CPeerNetNode::DisconnectRemoteClient()

//public:
// Послать блок данных серверу заданного именованного канала и получить ответ.
//Метод возвращает буфер в динамической памяти с ответом сервера.
TMessageBuf * CPeerNetNode::RequestAndReply(WORD iNode, TMessageBuf &mbRequest)
//iNode[in]     - номер корреспондирующего узла сети
//mbRequest[in] - буфер с запросом
{
#ifdef _TRACE
  TraceW(_T("CPeerNetNode::RequestAndReply()-enter"), false);
#endif
  CNamedPipe *pNP = m_pCliNPs[iNode];   //выбор NP по номеру узла
  if (pNP == NULL)
  {
    // Внутренняя ошибка - выбран несуществующий канал
    TCHAR szMessBuf[ERMES_BUFSIZE];
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("CPeerNetNode::RequestAndReply(): NP %d not exists"), iNode);
#ifdef _TRACE
    TraceW(szMessBuf);
#endif
    return NULL;
  }
  return pNP->RequestAndReply(mbRequest);
} // CPeerNetNode::RequestAndReply()

// Упаковать сообщение в блок данных в буфере сообщения "на месте"
void  CPeerNetNode::WrapData(WORD iNodeTo, TMessageBuf &mbRequest)
//iNodeTo[in]       - индекс узла-получателя сообщения
//mbRequest[in/out] - буфер сообщения (запроса)
{
  TCHAR szReqst[CHARBUFSIZE];
  // Сформировать голову сообщения DTB iikkdddd
  swprintf_s(szReqst, CHARBUFSIZE, _T("DTB %02d%02d%04d"),
             m_iOwnNode, iNodeTo, mbRequest.MessageBytes());
  // Сдвинуть вправо блок данных и подставить спереди голову
  WORD wLeng = (WORD)_tcslen(szReqst),  //размер головы в символах (без 0)
       nSize = wLeng * sizeof(TCHAR);   //размер головы в байтах (без 0)
  memmove(mbRequest.Message() + wLeng,
          mbRequest.Message(), mbRequest.MessageBytes());
  memcpy(mbRequest.Message(), szReqst, nSize);
  mbRequest._wMessBytes += nSize;       //скорректировать длину сообщения
} // CPeerNetNode::WrapData()

// Распаковать блок данных в сообщение в буфере сообщения "на месте"
void  CPeerNetNode::UnwrapData(WORD &iNodeFrom, TMessageBuf *pmbReply)
//iNodeFrom[out]  - индекс узла-отправителя сообщения
//mbReply[in/out] - буфер сообщения (ответа на запрос)
{
  iNodeFrom = ToNumber(pmbReply->Message() + 4, 2);
  if (pmbReply->Compare(_T("DTB"), 3) != 0)  return;    //это не DTB
  // Полный размер "полезной нагрузки", байт вместе с '\0'
  WORD nSize = ToNumber(pmbReply->Message() + 8, 4);    //байт вместе с '\0'
  // Сдвинуть содержимое буфера влево на nSize байт
  memmove(pmbReply->Message(), pmbReply->Message() + 12, nSize);
  pmbReply->_wMessBytes = nSize; //новая длина
  // Обнулить хвост
  memset(pmbReply->MessBuffer()+pmbReply->_wMessBytes, 0, 12*sizeof(TCHAR));
} // CPeerNetNode::UnwrapData()

//private:
// Запустить экземпляр клиента заданного узла сети ---------------------
bool  CPeerNetNode::StartNPClient(WORD iNode)
//iNode - номер (индекс) узла сети
{
  TCHAR szMessBuf[ERMES_BUFSIZE];
#ifdef _TRACE
  swprintf_s(szMessBuf, ERMES_BUFSIZE,
             _T("StartNPClient(): Node %d: NP Client starting"), iNode);
  TraceW(szMessBuf);
#endif
  // Сконструировать объект экземпляра именованного канала
  CNamedPipe *pNP = new CNamedPipe(this, iNode);
  // Формирование имени канала
  TCHAR  chPipeName[NAMEBUFSIZE];    //буфер для имени канала
  swprintf_s(chPipeName, NAMEBUFSIZE, PIPENAME_FRMT, iNode);
  // Повторение попыток открыть именованный канал, если он существует.
  //Ожидать освобождения сервера, если канал занят.
  while (true) {
    pNP->m_hPipe = ::CreateFile(
      chPipeName,       //имя канала
      GENERIC_READ |    //вид доступа: чтение (приём от сервера)
      GENERIC_WRITE,    // и запись (передача серверу)
      0, NULL,          //no sharing, default security attributes
      OPEN_EXISTING,    //opens existing pipe
      0, NULL           //default attributes, no template file
    );
    // При успешном открытии канала выход из цикла
    if (pNP->m_hPipe != INVALID_HANDLE_VALUE) {
#ifdef _TRACE
      TraceW(_T("StartNPClient(): ::CreateFile() returns OK"), false);
#endif
      break;
    }
    if (pNP->SysErrorCode() != ERROR_PIPE_BUSY) {
      // Ошибка открытия канала, отличная от ERROR_PIPE_BUSY, - выйти
      if (pNP->SysErrorCode() == ERROR_FILE_NOT_FOUND) {
        // Ошибка ERROR_FILE_NOT_FOUND - нет узла iNode
#ifdef _TRACE
        swprintf_s(szMessBuf, ERMES_BUFSIZE,
                   _T("- Net node #%d not found"), iNode);
        TraceW(szMessBuf);
#endif
        pNP->ClearError();
      }
      else
        pNP->ProcessError(_T("- Could not open pipe"), _T("::CreateFile()"));
      goto TraceErr;
    }
    // Все экземпляры канала заняты - ждать заданное время
    if (!::WaitNamedPipe(chPipeName, PIPE_TIMEOUT)) { //5000 ms
      pNP->ProcessError(_T("- Could not open pipe, time-out elapsed"),
                        _T("::CreateFile()"));
      //pNP->m_dwSysError = ERRCODE_BASE + 2;
      goto TraceErr;
    }
  } //while
  // Соединение установлено!
  //Создать события синхр клиентских потоков - HANDLE m_hEvents[]:
  //0) m_hEvents[EvTermnReqst]-хендл события "Требуется завершение"
  //1) m_hEvents[EvReqstReady]-хендл события "Готов запрос"
  //2) m_hEvents[EvRepReceivd]-хендл события "Получен ответ"
  for (int i = 0; i < EVENTCOUNT; i++) {
    pNP->m_hEvents[i] = ::CreateEvent(
      NULL,     //default security attribute
      FALSE,    //TRUE: manual reset event, FALSE: auto-reset event
      FALSE,    //initial state = nonsignaled
      NULL      //unnamed event object
    );
    if (pNP->m_hEvents[i] == NULL) {
      // Ошибка создания объекта Event:
      //Запоминание кода ошибки и формирование сообщения об ошибке.
      pNP->ProcessError(NULL, _T("::CreateEvent()"));
      goto TraceErr;
    }
  } // for
  // Создание программного потока клиента именованного канала
  pNP->m_pThread = AfxBeginThread(NPClientThreadProc, pNP);
  if (pNP->m_pThread) {
#ifdef _TRACE
    // Трассировочное сообщение
    TraceW(_T("AfxBeginThread(NPClientThreadProc) returns OK."), false);
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("    hThread = 0x%08X, nThreadID = 0x%08X"),
               (UINT)pNP->m_pThread->m_hThread, pNP->m_pThread->m_nThreadID);
    TraceW(szMessBuf);
#endif
    // В сети есть узел с номером iNode и удалось запустить клиента 
    //для обмена с ним  - занести в список узлов-клиентов
    EnterCriticalSection(&g_cs);
    m_pCliNPs[iNode] = pNP;
    m_nNodes++; //подсчёт активных узлов сети, с которыми установлена связь
    LeaveCriticalSection(&g_cs);
    //m_pMainWin->AddToNodeList(iNode);
  }
  else {
    // Ошибка создания программного потока клиента.
    //Запоминание кода ошибки и формирование сообщения об ошибке
    pNP->ProcessError(NULL, _T("AfxBeginThread(NPClientThreadProc)"));
TraceErr:
    if (pNP != NULL) { delete pNP;  pNP = NULL; }
  }
  return (pNP != NULL);
} // CPeerNetNode::StartNPClient()

// Запустить серверную часть узла одноранговой сети --------------------
bool  CPeerNetNode::StartNPServer()
//iNode - номер корреспондирующего узла сети
{
#ifdef _TRACE
  TraceW(_T("StartNPServer()-enter"), false);
#endif
  TCHAR szMessBuf[ERMES_BUFSIZE];  bool bRet;
  // Создать программный поток сервера, генератора экземпляров 
  //именованного канала (поток Listen)
  m_pSrvThread = AfxBeginThread(NPServerThreadProc, this);
  if (m_pSrvThread) {
#ifdef _TRACE
    // Трассировочное сообщение
    TraceW(
      _T("AfxBeginThread(NPServerThreadProc) returns OK."), false
    );
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("    hThread = 0x%08X, nThreadID = 0x%08X"),
               (UINT)m_pSrvThread->m_hThread, m_pSrvThread->m_nThreadID);
    TraceW(szMessBuf);
#endif
    bRet = true;
  }
  else
  {
    // Ошибка создания основного программного потока сервера.
    DWORD dwSysError = ::GetLastError();
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("%s failed with error code %d"),
               _T("AfxBeginThread(NPServerThreadProc)"), dwSysError);
#ifdef _TRACE
    TraceW(szMessBuf);
#endif
    bRet = false;
  }
#ifdef _TRACE
  TraceW(_T("StartNPServer()-exit"), false);
#endif
  return bRet;
} // CPeerNetNode::StartNPServer()

// Процедура потока (Thread) генератора экземпляров именованного канала
//static
UINT __cdecl  CPeerNetNode::NPServerThreadProc(LPVOID lpvParam)
//lpvParam - указатель объекта CPeerNetNode
{
  TCHAR szMessBuf[ERMES_BUFSIZE];
  CNamedPipe *pNPListen;
  CPeerNetNode *pNetNode = (CPeerNetNode *)lpvParam;
  if (pNetNode == NULL)  return 1;  //неверный аргумент lpvParam
#ifdef _TRACE
  swprintf_s(szMessBuf, ERMES_BUFSIZE,
             _T("NPServerThreadProc()-enter, nThreadID = 0x%08X"),
             pNetNode->m_pSrvThread->m_nThreadID);
  pNetNode->TraceW(szMessBuf);
#endif
  // Формирование имени канала
  BOOL  bSuccess;
  TCHAR  chPipeName[NAMEBUFSIZE];   //буфер для имени канала
  swprintf_s(chPipeName, NAMEBUFSIZE, PIPENAME_FRMT, pNetNode->m_iOwnNode);
  WORD iNode;  bool bNode = pNetNode->GetFirstNode(iNode);
  // Цикл создания программных потоков экземпляров именованного канала 
  //по мере подключения клиентов к данному узлу
  while (true) {
    pNPListen = new CNamedPipe(pNetNode);
    pNetNode->m_pNPListen = pNPListen;
    ASSERT(pNPListen);
    /* Создать события синхр северных потоков - HANDLE m_hEvents[]:
    //0) m_hEvents[EvSaveListen]-хендл события "Запомнить NP Listen"
    //1) m_hEvents[EvNewListen] -хендл события "Разрешено создать NP Listen"
    //2) m_hEvents[EvInstActive]-хендл события "Поток Instance активен"
    for (int i = 0; i < EVENTCOUNT; i++) {
      pNPListen->m_hEvents[i] = ::CreateEvent(
        NULL,     //default security attribute
        FALSE,    //TRUE: manual reset event, FALSE: auto-reset event
        FALSE,    //initial state = nonsignaled
        NULL      //unnamed event object
      );
      if (pNPListen->m_hEvents[i] == NULL) {
        // Ошибка создания объекта Event:
        //Запоминание кода ошибки и формирование сообщения об ошибке.
        pNPListen->ProcessError(NULL, _T("::CreateEvent()"));
        goto TraceErr;
      }
    } // for */
    //Создание экземпляра именованного канала
    pNPListen->m_hPipe = ::CreateNamedPipe(
      chPipeName,               //pipe name
      PIPE_ACCESS_DUPLEX,       //read/write access
      PIPE_TYPE_MESSAGE |       //message type pipe
      PIPE_READMODE_MESSAGE |   //message-read mode
      PIPE_WAIT,                //blocking mode
      PIPE_UNLIMITED_INSTANCES, //max. instances
      BUFSIZE, BUFSIZE,         //output and input buffer size
      0, NULL                   //client time-out, default security attribute
    );
    if (pNPListen->m_hPipe == INVALID_HANDLE_VALUE) {
      pNPListen->ProcessError(NULL, _T("::CreateNamedPipe()"));
      return 2; //завершение потока с кодом ошибки
    }           // создания экземпляра именованного канала
#ifdef _TRACE
    // Трассировочные сообщения
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
      _T("NPServerThreadProc(): %s Pipe handle = 0x%08X"),
      _T("::CreateNamedPipe() returns OK."), (UINT)pNPListen->m_hPipe);
    pNetNode->TraceW(szMessBuf);
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("Wait for the client of node #%d to connect..."),
               pNPListen->m_iCncNode);
    pNetNode->TraceW(szMessBuf);
#endif
    if (bNode) {
      pNetNode->NotifyNode(iNode);
      bNode = pNetNode->GetNextNode(iNode);
    }
    //----- Ждать соединения со стороны клиента -----//
    bSuccess = ::ConnectNamedPipe(pNPListen->m_hPipe, NULL);
    if (bSuccess) {
      // Соединение со стороны клиента успешно установлено,
      if (pNetNode->m_bTerm)  break;    //но поднят флаг останова узла -
    }                                   // нормально завершить поток
    else {
      // Если ConnectNamedPipe возвратила 0, а GetLastError возвращает 
      //код ERROR_PIPE_CONNECTED, то это также успешное соединение.
      bSuccess = (::GetLastError() == ERROR_PIPE_CONNECTED) ? TRUE : FALSE;
    }
    if (!bSuccess) {
      pNPListen->ProcessError(NULL, _T("::ConnectNamedPipe()"));
      ::CloseHandle(pNPListen->m_hPipe);
      return 3; //завершение потока с кодом ошибки
    }           // соединения клиента с сервером
    // Успешное соединение клиента с сервером
#ifdef _TRACE
    // Трассировочное сообщение
    pNetNode->TraceW(
      _T("NPServerThreadProc(): ::ConnectNamedPipe() returns OK"), false
    );
#endif
    //CWinThread * Создать и запустить поток экземпляра именованного канала
    pNPListen->m_pThread =
      AfxBeginThread(NPInstanceThreadProc, pNPListen);
    if (!pNPListen->m_pThread) {
      // Ошибка создания программного потока экземпляра именованного канала.
      //Запоминание кода ошибки и формирование сообщения об ошибке
      pNPListen->ProcessError(
        NULL, _T("AfxBeginThread(NPInstanceThreadProc)")
      );
      return 4; //завершение текущего потока NPServerThreadProc
    }           // с кодом ошибки создания потока NPInstanceThreadProc
    // Успешное создание программного потока экземпляра именованного канала
#ifdef _TRACE
    // Трассировочное сообщение
    pNetNode->TraceW(
      _T("AfxBeginThread(NPInstanceThreadProc) returns OK."), false
    );
    swprintf_s(
      szMessBuf, ERMES_BUFSIZE,
      _T("    hThread = 0x%08X, nThreadID = 0x%08X"),
      (UINT)pNPListen->m_pThread->m_hThread, pNPListen->m_pThread->m_nThreadID
    );
    pNetNode->TraceW(szMessBuf);
#endif
  } //while()
//TraceErr:
#ifdef _TRACE
  pNetNode->TraceW(_T("NPServerThreadProc()-exit"), false);
#endif
  return 0;     //успешное завершение потока
} // CPeerNetNode::NPServerThreadProc()

// Процедура потока (Thread) экземпляра именованного канала
//static
UINT __cdecl  CPeerNetNode::NPInstanceThreadProc(LPVOID lpvParam)
//lpvParam - указатель объекта CNamedPipe
{
  TCHAR szMessBuf[ERMES_BUFSIZE];
  CNamedPipe *pNP = (CNamedPipe *)lpvParam;
  if (pNP == NULL || !pNP->IsKindOf(RUNTIME_CLASS(CNamedPipe)))
    return 1;   //неверный аргумент lpvParam
#ifdef _TRACE
  pNP->TraceW(_T("NPInstanceThreadProc()-enter"), false);
#endif
  UINT nRet = 0;  bool bStartClient = false;  BOOL bSuccess;
  ECommandCode eCmd;  WORD iFrom, iTo, iOwner;
  DWORD dwBytesRead, dwReplyBytes, dwWritten;
  // Цикл обмена с клиентом, захватившим данный экземпляр канала
  while (true) {
    // Ждать и прочитать запрос клиента
#ifdef _TRACE
    pNP->TraceW(_T("NPInstanceThreadProc(): waiting ::ReadFile()"), false);
#endif
    bSuccess = ::ReadFile(
      pNP->m_hPipe,             //handle to pipe
      pNP->RequestMessage(),    //buffer to receive data m_chRequest
      BUFSIZE,                  //size of buffer
      &dwBytesRead,             //number of bytes read
      NULL                      //not overlapped I/O
    );
    if (!bSuccess || (dwBytesRead == 0)) {
      // Ошибка чтения из канала
      pNP->ProcessError(NULL, _T("::ReadFile()"));
      nRet = 5;  break;     //окончание обмена
    }
    pNP->m_mbRequest._wMessBytes = (WORD)dwBytesRead;
    // Запрос клиента прочитан
#ifdef _TRACE
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("NPInstanceThreadProc(): ::ReadFile(): %s"),
               pNP->RequestMessage());
    pNP->TraceW(szMessBuf);
#endif
    eCmd = ParseMessage(pNP->m_mbRequest, iFrom, iTo);  //разобрать команду
    ASSERT(eCmd != CmC_NONE);
    //ASSERT(iFrom == pNP->m_iCncNode);
    if (eCmd == CmC_SCL) {
      // Запущен удалённый узел <From> - определить владельца узла и поднять 
      //флаг запуска клиента на канале <From>
      iOwner = ToNumber(pNP->m_mbRequest.Message() + 8, 2);
      //pNP->m_pArentNode->ConnectRemoteClient(iFrom, iOwner, pNP);
      bStartClient = true;
    }
    else if (eCmd == CmC_TRM)   //запрос Terminate
    {
      // Отобразить разъединение с удалённым узлом в интерфейсе
      pNP->m_pArentNode->DisconnectRemoteClient(iFrom); //pNP->m_iCncNode
      // Выбрать NP клиента узла iFrom и ...
      CNamedPipe *pCliNP = pNP->m_pArentNode->m_pCliNPs[iFrom];
      //возбудить в нём событие EvTermnReqst для завершения клиентского потока
      bSuccess = ::SetEvent(pCliNP->m_hEvents[EvTermnReqst]);
      if (!bSuccess) {
        // Ошибка вызова SetEvent(): запомнить код ошибки и вывести 
        //сообщение об ошибке.
        pCliNP->ProcessError(NULL, _T("::SetEvent(EvTermnReqst)"));
      }
      break;    //ответ ан запрос Terminate не нужен
    }
    // Получить ответ на запрос
    if (!pNP->m_pArentNode->GetAnswerToRequest(pNP))
    { nRet = 6;  break; }   //ошибка
    // Записать ответ в канал
    dwReplyBytes = (DWORD)pNP->m_mbReply.MessageBytes();
    bSuccess = ::WriteFile(
      pNP->m_hPipe,         //handle to pipe
      pNP->ReplyMessage(),  //buffer to write from
      dwReplyBytes,         //number of bytes to write
      &dwWritten,           //number of bytes written
      NULL                  //not overlapped I/O
    );
    if (!bSuccess || dwReplyBytes != dwWritten) {
      pNP->ProcessError(NULL, _T("::WriteFile()"));
      nRet = 7;  break;
    }
#ifdef _TRACE
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("NPInstanceThreadProc(): ::WriteFile(): %s"),
               pNP->ReplyMessage());
    pNP->TraceW(szMessBuf);
#endif
    if (bStartClient) {
      // Отобразить в интерфейсе присоединение клиента на удалённом 
      //узле <From> к нашему серверу при его запуске
      pNP->m_pArentNode->ConnectRemoteClient(iFrom, iOwner, pNP);
      bStartClient = false;
    }
  } //while
  // Закрытие именованного канала
  //Flush the pipe to allow the client to read the pipe's contents
  // before disconnecting. Then disconnect the pipe, and close the
  // handle to this pipe instance.
  try {
    bSuccess = ::FlushFileBuffers(pNP->m_hPipe);
    bSuccess = ::DisconnectNamedPipe(pNP->m_hPipe);
    bSuccess = ::CloseHandle(pNP->m_hPipe);  //а это уже закрытие канала
  }
  catch (...) {
    ;
  }
#ifdef _TRACE
  // Трассировочное сообщение
  swprintf_s(szMessBuf, ERMES_BUFSIZE,
             _T("NPInstanceThreadProc() of pipe #%d exited with code %d"),
             pNP->m_iCncNode, nRet);
  pNP->TraceW(szMessBuf);
#endif
  // Отобразить разъединение с удалённым узлом в интерфейсе
  //pNP->m_pArentNode->DisconnectRemoteClient(pNP->m_iCncNode);
  delete pNP;
  return nRet;
} // CPeerNetNode::NPInstanceThreadProc()

// Процедура потока (Thread) клиента именованного канала
//static
UINT __cdecl  CPeerNetNode::NPClientThreadProc(LPVOID lpvParam)
//lpvParam - указатель объекта CNamedPipe
{
  TCHAR szMessBuf[ERMES_BUFSIZE];
  CNamedPipe *pNP = (CNamedPipe *)lpvParam;
  if (pNP == NULL || !pNP->IsKindOf(RUNTIME_CLASS(CNamedPipe)))
    return 1;   //неверный аргумент lpvParam
#ifdef _TRACE
  pNP->TraceW(_T("NPClientThreadProc()-enter"), false);
#endif
  UINT nRet = 0;  DWORD dwWait;
  // Цикл обмена с сервером узла pNP->m_iCncNode
  while (true) {
    // Ждать любое из двух событий:
    //EvTermnReqst(0) - "Завершить поток" 
    //EvReqstReady(1) - "Запрос к серверу готов"
#ifdef _TRACE
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("Wait for a request to node #%d or termination signal"),
               pNP->m_iCncNode);
    pNP->TraceW(szMessBuf);
#endif
    // dwWait shows which event was signaled
    dwWait = ::WaitForMultipleObjects( 
      2, pNP->m_hEvents,    //number and array of event objects
      FALSE, INFINITE       //ждать не все события сразу, waits indefinitely
    );
    int i = dwWait - WAIT_OBJECT_0; //determines which event
    if (i < 0 || 1 < i) {
      // Ошибка ожидания событий
      pNP->ProcessError(
        NULL, _T("::WaitForMultipleObjects(EvTermnReqst|hEvReqstReady)")
      );
      nRet = 8;  break;
    }
    if (i == 0)  break; //это событие EvTermnReqst - завершить поток
#ifdef _TRACE
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("EvReqstReady signal got from node #%d to node #%d"),
               pNP->m_pArentNode->m_iOwnNode, pNP->m_iCncNode);
    pNP->TraceW(szMessBuf);
#endif
    // Получен сигнал "Запрос готов" - передать запрос серверу
    if ((nRet = pNP->SendRequest()) != 0 ||
        // Если был передан запрос TERMINATE, завершить поток
        pNP->m_mbRequest.Compare(_T("TRM"), 3) == 0)  break;
    // Ждать и получить ответ сервера (но не на TRM)
    if ((nRet = pNP->ReceiveReply()) != 0)  break;
  } //while
  pNP->m_pArentNode->m_pCliNPs[pNP->m_iCncNode] = NULL;
#ifdef _TRACE
  swprintf_s(szMessBuf, ERMES_BUFSIZE,
             _T("NPClientThreadProc()-exit. Return code %d"), nRet);
  pNP->TraceW(szMessBuf);
#endif
  delete pNP;
  return nRet;
} // CPeerNetNode::NPClientThreadProc()

// Разобрать запрос. Метод возвращает код комманды.
//static
ECommandCode  CPeerNetNode::ParseMessage(TMessageBuf &mbRequest,
                                         WORD &wFrom, WORD &wTo)
//mbRequest[in] - ссылка на буфер с запросом
//wFrom[out]    - ссылка на номер исходящего узла сети
//wTo[out]      - ссылка на номер входящего узла сети
{
  static LPTSTR sCmnds[] = {
//Индекс             Код команды
/* 0 */ _T("ACK"),  //1: Acknowledge  - Проверить/подтвердить
/* 1 */ _T("DTB"),  //2: Data block   - Блок данных
/* 2 */ _T("SCL"),  //3: Start client - Запустить клиент
/* 3 */ _T("TRM"),  //4: Terminate    - Закрыть канал
  };
  static WORD n = sizeof(sCmnds) / sizeof(LPTSTR);
  for (WORD i = 0; i < n; i++) {
    if (mbRequest.Compare(sCmnds[i], 3) == 0) {
      // Команда определена, получить параметры
      wFrom = ToNumber(mbRequest.Message() + 4, 2);   //<From>
      wTo = ToNumber(mbRequest.Message() + 6, 2);     //<To>
      return (ECommandCode)(i + 1);
    }
  }
  return CmC_NONE;   //команда не определена
} // CPeerNetNode::ParseMessage()

// Интерфейс First-Next перебора активных узлов сети
bool  CPeerNetNode::GetNextNode(WORD &iNode, bool bAllNodes/* = false*/,
                                bool bFirst/* = false*/)
//iNode[out]    - индекс следующего узла
//bAllNodes[in] - true ="Перебирать все узлы",
//                false="Перебирать все, кроме своего"
//bFirst[in]    - true="Начать с начала", false="Продолжить"
{
  static WORD i;
  if (bFirst)  i = 0;
  while (i < NODECOUNT) {
    if ((bAllNodes && i == m_iOwnNode) ||
        (i != m_iOwnNode && m_pCliNPs[i] != NULL)) {
      iNode = i++;  return true;
    }
    i++;
  }
  return false;
} // CPeerNetNode::GetNextNode()

// Преобразовать числовой литерал в число
//static
WORD  CPeerNetNode::ToNumber(LPTSTR lpsNum, WORD wLeng)
{
  WORD n = 0;  TCHAR c;
  for (WORD i = 0; i < wLeng; i++) {
    c = lpsNum[i];
    if (isdigit(c))  n = n * 10 + (c - _T('0'));
  }
  return n;
} // CPeerNetNode::ToNumber()

// Сформировать ответ на запрос клиента. Метод возвращает значение: 
//true - успешное завершение, false - недействительный формат сообщения
bool  CPeerNetNode::GetAnswerToRequest(CNamedPipe *pNP)
//pNP - указатель объекта CNamedPipe
{
  TCHAR szMessBuf[CHARBUFSIZE], *pszFrmt;
  WORD iNodeFrom, iNodeTo;  bool b = true;
  ECommandCode eCmd = ParseMessage(pNP->m_mbRequest, iNodeFrom, iNodeTo);
  //ASSERT(iNodeFrom == pNP->m_iCncNode &&
  //       iNodeTo == pNP->m_pArentNode->m_iOwnNode);
  if (eCmd == CmC_SCL) {
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("SCL %02d%02d%02d"), iNodeTo, iNodeFrom, m_iOwner);
  }
  else {
    if (eCmd == CmC_DTB) {
      EnterCriticalSection(&g_cs);
      b = m_oDTBQueue.Put(pNP->m_mbRequest);    //поставить DTB в очередь
      //b = m_pBlkChain->GetReplyOnDTB(pNP->m_mbRequest, szMessBuf);
      LeaveCriticalSection(&g_cs);
	  b = (pNP->m_pArentNode->m_pMainWin->PostMessage(WM_DTBQUEUED) == TRUE);
    }
    //else  b = false;
    swprintf_s(szMessBuf, ERMES_BUFSIZE,
               _T("ACK %02d%02d"), iNodeTo, iNodeFrom);
  }
  pNP->m_mbReply.PutMessage(szMessBuf); //занесение
  return b;
} // CPeerNetNode::GetAnswerToRequest()

// Оповестить удалённый узел о старте "своего" узла и получить ответ
void  CPeerNetNode::NotifyNode(WORD iNode)
{
  TMessageBuf  mbRequest;           //буфер запроса клиента
  TCHAR szMessBuf[ERMES_BUFSIZE];
  CNamedPipe *pNP = m_pCliNPs[iNode]; //клиентский NP, связанный с узлом iNode
  //Занести в буфер запрос Start CLient.
  swprintf_s(szMessBuf, ERMES_BUFSIZE, _T("SCL %02d%02d%02d"),
             m_iOwnNode, iNode, m_iOwner);
  mbRequest.PutMessage(szMessBuf);
#ifdef _TRACE
  swprintf_s(szMessBuf, ERMES_BUFSIZE,
             _T("NotifyNode(): Request sent: %s"), mbRequest.Message());
  pNP->TraceW(szMessBuf);
#endif
  // Послать запрос и получить ответ
  pNP->m_mbRequest = mbRequest;     //поместить запрос в буфер
  UINT nRet = pNP->SendRequest();
  // Ждать и получить ответ сервера
  nRet = pNP->ReceiveReply(true);
#ifdef _TRACE
  swprintf_s(szMessBuf, ERMES_BUFSIZE,
             _T("NotifyNode(): Reply received: %s"),
             pNP->m_mbReply.Message());
  pNP->TraceW(szMessBuf);
#endif
  WORD iOwner = ToNumber(pNP->m_mbReply.Message() + 8, 2);
  m_pMainWin->AddRemoteNode(iNode, iOwner);
} // CPeerNetNode::NotifyNode()

// Очистить узел
void  CPeerNetNode::ClearNode()
{
  int i;
  m_pSrvThread = NULL;
  m_iOwnNode = UNKNOWN_NODE;  m_nNodes = 0;
  for (i = 0; i < NODECOUNT; i++)
    if (m_pSrvNPs[i]) { delete m_pSrvNPs[i];  m_pSrvNPs[i] = NULL; }
  for (i = 0; i < NODECOUNT; i++)
    if (m_pCliNPs[i]) { delete m_pCliNPs[i];  m_pCliNPs[i] = NULL; }
} // CPeerNetNode::ClearNode()

#ifdef _TRACE
// Отладочные функции трассировки:
// Инициализировать механизм трассировки
void  CPeerNetNode::InitTracing()
{
  TCHAR szMessBuf[ERMES_BUFSIZE];
  // Сформировать имя трассировочного файла
  swprintf_s(szMessBuf, ERMES_BUFSIZE,
             _T("%sPNNtrace_%d.log"), TRACEFILEPATH, m_iOwnNode);
  errno_t err = _wfopen_s(&m_pTraceFile, szMessBuf, _T("wt"));
  TraceW(_T("(c) DIR, 2019. Peer Net Node tracing file"), false);
} // CPeerNetNode::InitTracing()

// Вывести трассировочное сообщение
void  CPeerNetNode::TraceW(LPTSTR lpszMes, bool bClearAfter/* = true*/)
{
  //EnterCriticalSection(&g_cs);
  SYSTEMTIME st;  TCHAR szMessBuf[ERMES_BUFSIZE];
  GetSystemTime(&st);
  swprintf_s(szMessBuf, ERMES_BUFSIZE, _T("%02d:%02d:%02d.%03d - %s"),
             st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, lpszMes);
  m_pMainWin->ShowTrace(szMessBuf);
  if (CPeerNetNode::m_pTraceFile != NULL) {
    // Трассировочный файл открыт
    fwprintf_s(CPeerNetNode::m_pTraceFile, _T("%s\n"), szMessBuf);
  }
  if (bClearAfter)  *lpszMes = L'\0';   //очистка буфера сообщения
  //LeaveCriticalSection(&g_cs);
} // CPeerNetNode::TraceW()
#endif

// Конец определения класса CPeerNetNode -------------------------------
