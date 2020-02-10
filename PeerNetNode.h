// PeerNetNode.h                                           (c) DIR, 2019
//
//              Объявление классов:
// CNamedPipe   - Именованный канал
// CPeerNetNode - Узел одноранговой сети
//
////////////////////////////////////////////////////////////////////////

#pragma once

#include "resource.h"

#ifdef _DEBUG
 #define  _TRACE        //включить трассировку
#else
 #undef  _TRACE
#endif

//======================================================================
// Объявление класса CNamedPipe -
//  Базовый для работы с именованными каналами
//----------------------------------------------------------------------

#define  NODECOUNT      7       //макс количество узлов одноранговой сети
#define  NAMEBUFSIZE    32      //размер буфера имени канала в символах
#define  CHARBUFSIZE    256     //размер буфера сообщения в символах
#define  BUFSIZE        (CHARBUFSIZE*sizeof(TCHAR)) //размер буфера сообщ в байтах
#define  ERMES_BUFSIZE  256     //размер буфера сообщ об ошибке в байтах
#define  UNKNOWN_NODE   99      //"номер" неизвестного узла сети
#define  UNAUTHORIZED   0xFFFF  //неавторизованный пользователь
#define  ERRCODE_BASE   800     //база несистемных кодов ошибок
// Формат для формирования имени канала
#define  PIPENAME_FRMT  _T("\\\\.\\pipe\\PeerNetNP_%d")
#ifdef _TRACE
 // Спецификация каталога для трассировочных файлов
 //#define  TRACEFILEPATH  _T("D:\\Programming\\NamedPipe\\PeerNetNode\\")
 #define  TRACEFILEPATH  _T("..\\")
#endif
// Таймаут канала в миллисекундах
#define  PIPE_TIMEOUT   2000
#define  CLCQUEUESIZE   8       //размер циклической очереди
#define  CLCQUEUEMASK   0x0007  //маска выделения циклического индекса
#define  WM_DTBQUEUED   (WM_USER+1) //сообщение: есть DTB в очереди

// Состояния именованного канала
enum EPipeStatus {
  PS_None = 0,          //неинициализированное состояние канала
  PS_Created,           //канал создан
  PS_Listen,            //ожидание запроса на соединение от клиента
  PS_Connected,         //связь установлена
  PS_RequestWaiting,    //ожидание запроса от клиента
  PS_ReplySending,      //подготовка и отправление ответа клиенту
  PS_Disconnected,      //связь разорвана
};

// Идентификаторы (коды) команд
enum ECommandCode {
  CmC_NONE = 0,     //команда не определена
  CmC_ACK,          //1: Acknowledge        - Проверить/подтвердить
  CmC_DTB,          //2: Data block         - Блок данных
  CmC_SCL,          //3: Start client       - Запустить клиент
  CmC_TRM,          //4: Terminate          - Закрыть канал
};

#define  EVENTCOUNT    3    //количество событий синхронизации в каждом NP
// Для клиентских потоков
#define  EvTermnReqst  0    //индекс события "Требуется завершение"
#define  EvReqstReady  1    //индекс события "Готов запрос"
#define  EvRepReceivd  2    //индекс события "Получен ответ"
// Для серверных потоков
#define  EvSaveListen  0    //индекс события "Запомнить NP Listen"
#define  EvNewListen   1    //индекс события "Разрешено создать новый NP Listen"
#define  EvInstActive  2    //индекс события "Поток Instance активен"

// Ошибки при работе с именованным каналом
enum EPipeError {
  PE_OK = 0,  //нет ошибок
  PE_Create,  //ошибка создания именованного канала
  PE_Connect, //ошибка соединения по именованному каналу
  PE_Read,    //ошибка чтения данных из канала
  PE_Write,   //ошибка записи данных в канал
};

// Структурный тип "Буфер сообщения для обмена по сети"
struct TMessageBuf {
  TCHAR  _chMess[CHARBUFSIZE];  //буфер сообщения (TCHAR=wchar_t)
  WORD   _wMessBytes;   //общая длина сообщения в байтах (вместе с '\0')
  // Конструкторы:
  // Конструктор по умолчанию
  TMessageBuf() {
    memset(_chMess, 0, sizeof(_chMess));  _wMessBytes = 0;
  }
  // Инициализирующий конструктор
  TMessageBuf(BYTE *pData, WORD wLeng) { Put(pData, wLeng); }
  TMessageBuf(LPTSTR lpszMes) { PutMessage(lpszMes); }
  // Дать размер буфера в байтах
  WORD    BufferSize() { return (WORD)BUFSIZE; }
  // Дать указатель буфера как байтового массива
  BYTE  * MessBuffer() { return (BYTE *)_chMess; }
  // Дать указатель сообщения, находящегося в буфере
  LPTSTR  Message() { return _chMess; }
  // Дать длину сообщения в символах
  WORD    MessageLength() { return (WORD)_tcslen(_chMess); }
  // Дать общую длину сообщения в байтах вместе с '\0'
  WORD    MessageBytes() { return _wMessBytes; }
  // Поместить в буфер блок данных
  void    Put(BYTE *pData, WORD wLeng)
  {
    _wMessBytes = min(wLeng, sizeof(_chMess));
    memcpy(_chMess, pData, _wMessBytes);
    // Обнулить хвост
    if (sizeof(_chMess) > _wMessBytes)
      memset(MessBuffer()+_wMessBytes, 0, sizeof(_chMess)-_wMessBytes);
  }
  // Поместить в буфер текстовую строку
  void    PutMessage(LPTSTR lpszMes)
  {
    WORD wLeng = ((WORD)_tcslen(lpszMes) + 1) * sizeof(TCHAR);
    //_tcscpy_s(_chMess, CHARBUFSIZE-1, lpszMes);
    Put((BYTE *)lpszMes, wLeng);
    _chMess[CHARBUFSIZE-1] = _T('\0');  //это работает, когда исходная строка
  }                                     // не умещается в буфере
  // Сравнить внешнее сообщение с сообщением в буфере
  int     Compare(LPTSTR lpszMes, WORD nChars = 0)
  //lpszMes - строка внешнего сообщения
  //nChars  - количество сравниваемых символов; если аргумент опущен
  //           (nChars == 0), то сравнивать всю строку
  {
    return (nChars == 0) ? _tcscmp(_chMess, lpszMes) :
                           _tcsncmp(_chMess, lpszMes, nChars);
  }
}; // End of struct TMessageBuf declaration ----------------------------

extern CRITICAL_SECTION  g_cs; //критическая секция

////////////////////////////////////////////////////////////////////////
// Класс CCyclicQueue - Циклическая очередь блоков сообщений

class CCyclicQueue
{
  // Данные
  TMessageBuf  m_mbMsg[CLCQUEUESIZE];   //очередь блоков сообщений
  int          m_iHead;     //индекс головы очереди
  int          m_iTail;     //индекс хвоста очереди
  int          m_nCount;    //количество блоков в очереди
public:
  // Конструктор и деструктор
  CCyclicQueue();
  ~CCyclicQueue() {}
  bool  IsEmpty() { return (m_nCount == 0); }
  bool  Put(TMessageBuf &mbMsg);
  bool  Get(TMessageBuf *&pmbMsg);
private:
  void  Increment(int &iNdx);
}; // End of class CCyclicQueue declaration ----------------------------

class CPeerNetNode;
class CPeerNetDialog;
class CBlockChain;

////////////////////////////////////////////////////////////////////////
// Класс CPeerNetNode - Узел одноранговой сети

class CNamedPipe : public CObject
{
public:
  CPeerNetNode *m_pArentNode;   //указатель родительского объекта CPeerNetNode
  CWinThread   *m_pThread;      //указатель потока, связанного с данным NP
  // Номер корреспондирующего узла сети (противоположного конца NP),
  //он равен индексу указателя объекта CNamedPipe в массивах
  //CPeerNetNode::m_pSrvNPs[NODECOUNT] и CPeerNetNode::m_pCliNPs[NODECOUNT]:
  WORD          m_iCncNode;
  HANDLE        m_hPipe;        //хендл (дескриптор) экземпляра NP
  HANDLE        m_hEvents[EVENTCOUNT];  //массив событий синхронизации потоков
  //m_hEvents[0]-hEvTermnReqst;   //хендл события "Требуется завершение"
  //m_hEvents[1]-hEvReqstReady;   //хендл события "Готов запрос"
  //m_hEvents[2]-hEvRepReceivd;   //хендл события "Получен ответ"
//protected:
  bool          m_bUsy;         //флаг "Канал занят"
  CCyclicQueue *m_pQueue;       //присоединённая очередь блоков на передачу
  TMessageBuf   m_mbRequest;    //буфер запроса к серверу
  TMessageBuf   m_mbReply;      //буфер ответа сервера
  DWORD         m_nRead;        //количество принятых сообщений
  DWORD         m_nWritten;     //количество отправленных сообщений
  EPipeStatus   m_eStatus;      //состояние канала
  EPipeError    m_eRrorCode;    //код ошибки
  DWORD  m_dwSysError;          //системный номер ошибки
  TCHAR  m_szErrorMes[ERMES_BUFSIZE];   //буфер сообщения об ошибке

public:
  CNamedPipe(CPeerNetNode *pArentNode, WORD iCncNode = UNKNOWN_NODE);
  virtual ~CNamedPipe();

  // Дать размер буфера в байтах
  WORD    BufferSize() { return (WORD)BUFSIZE; }
  // Дать указатель буфера запроса
  BYTE  * RequestBuffer() { return m_mbRequest.MessBuffer(); }
  // Дать указатель сообщения в буфере запроса
  LPTSTR  RequestMessage() { return m_mbRequest.Message(); }
  // Дать длину запроса в символах (без учёта завершающего 0)
  WORD    RequestLength() { return m_mbRequest.MessageLength(); }
  // Дать длину запроса в байтах с учётом завершающего 0
  WORD    RequestBytes() { return m_mbRequest.MessageBytes(); }
  // Дать указатель буфера ответа
  BYTE  * ReplyBuffer() { return m_mbReply.MessBuffer(); }
  // Дать указатель сообщения в буфере ответа
  LPTSTR  ReplyMessage() { return m_mbReply.Message(); }
  // Дать длину ответа в символах (без учёта завершающего 0)
  WORD    ReplyLength() { return m_mbReply.MessageLength(); }
  // Дать длину ответа в байтах с учётом завершающего 0
  WORD    ReplyBytes() { return m_mbReply.MessageBytes(); }
  // Послать запрос серверу данного именованного канала
  UINT    SendRequest();
  // Принять ответ сервера данного именованного канала
  UINT    ReceiveReply(bool bNoEvent = false);
  // Послать запрос серверу данного именованного канала и получить ответ
  TMessageBuf * RequestAndReply(TMessageBuf &mbRequest);
  // Методы обработки ошибок
  DWORD   SysErrorCode() {
    if (m_dwSysError == 0)  m_dwSysError = ::GetLastError();
    return m_dwSysError;
  }
  bool    Success() { return (m_eRrorCode == PE_OK && SysErrorCode() == 0); }
  void    ClearError() {
    m_eRrorCode = PE_OK;  m_dwSysError = 0;  m_szErrorMes[0] = _T('\0');
  }
  LPTSTR  ErrorMessage(LPTSTR lpszFunction = NULL);
  void    ProcessError(LPTSTR lpszMesg, LPTSTR lpszFunction = NULL);
//private:
#ifdef _TRACE
  //static  Вывести трассировочное сообщение
  void    TraceW(LPTSTR lpszMes, bool bClearAfter = true);
#endif
}; // End of class CNamedPipe declaration ------------------------------

////////////////////////////////////////////////////////////////////////
// Класс CPeerNetNode - Узел одноранговой сети

class CPeerNetNode
{
public:
  CPeerNetDialog  * m_pMainWin;     //объект главного окна программы
  CBlockChain     * m_pBlkChain;    //объект модели блок-чейн
  volatile CNamedPipe * m_pNPListen;    //NP в состоянии Listen static
  CWinThread  * m_pSrvThread;   //поток сервера Listen
  WORD          m_nNodes;       //количество активных узлов сети
  WORD          m_iOwnNode;     //индекс "своего" узла сети (0,1,..,n-1)
  WORD          m_iOwner;       //индекс пользователя-владельца узла
  CNamedPipe  * m_pSrvNPs[NODECOUNT];   //серверные NP узла
//WORD          m_nSrvNPs;  //мгновенное количество подключённых серверных NPs
  CNamedPipe  * m_pCliNPs[NODECOUNT];   //клиентские NP узла
  CCyclicQueue  m_oDTBQueue;    //циклическая очередь принятых DTB
//HANDLE        m_hMayStartClient;      //хендл (дескриптор) события
//TList         m_FreeNodes;            //список "свободных" номеров узлов
  bool   m_bTerm;           //флаг "Остановить поток сервера Listen"
//bool   m_bSrvStarting;    //флаг режима запуска сервера
#ifdef _TRACE
//static CFile  m_oTraceFile;   //трассировочный файл
  FILE * m_pTraceFile;  //трассировочный файл
#endif

  // Конструктор и деструктор
  CPeerNetNode(CPeerNetDialog *pMain, CBlockChain *pBlkChain);
  ~CPeerNetNode();

  // Внешние методы
  // -------------------------------------------------------------------
  // Инициализировать работу узла одноранговой сети
  bool  StartupNode();
  // Остановить работу узла одноранговой сети
  void  ShutdownNode();
  // Послать блок данных серверу заданного именованного канала и получить ответ
  TMessageBuf * RequestAndReply(WORD iNodeTo, TMessageBuf &mbRequest);
  // Упаковать сообщение в блок данных
  void  WrapData(WORD iNodeTo, TMessageBuf &mbRequest);
  // Распаковать блок данных в сообщение
  void  UnwrapData(WORD &iNodeFrom, TMessageBuf *pmbReply);
  // Запущен ли узел сети?
  bool  IsNodeRunning() { return (m_nNodes > 0); }
  // Есть ли кворум для голосования? ЗАГЛУШКА
  bool  IsQuorumPresent() { return true; } //(m_nNodes > (NODECOUNT >> 1))
  // Интерфейс First-Next перебора активных узлов сети
  bool  GetFirstNode(WORD &iNode, bool bAllNodes = false)
  { return GetNextNode(iNode, bAllNodes, true); }
  bool  GetNextNode(WORD &iNode, bool bAllNodes = false, bool bFirst = false);
  // Преобразовать числовой литерал в число
  static WORD  ToNumber(LPTSTR lpsNum, WORD wLeng);
#ifdef _TRACE
  // Отладочные функции трассировки:
  // Инициализировать механизм трассировки
  void  InitTracing();
  // Вывести трассировочное сообщение
  void  TraceW(LPTSTR lpszMes, bool bClearAfter = true);
#endif

private:
  // Внутренние методы
  // -------------------------------------------------------------------
  // Запустить экземпляр клиента заданного узла сети
  bool  StartNPClient(WORD iNode);
  // Запустить экземпляр сервера заданного узла сети
  bool  StartNPServer(); //CNamedPipe *WORD iNode
  // Процедура потока (Thread) сервера именованного канала для ожидания 
  //запроса на соединение со стороны нового клиента
  static UINT __cdecl  NPServerThreadProc(LPVOID lpvParam);
  // Процедура потока (Thread) экземпляра именованного канала
  static UINT __cdecl  NPInstanceThreadProc(LPVOID lpvParam);
  // Процедура потока (Thread) клиента именованного канала
  static UINT __cdecl  NPClientThreadProc(LPVOID lpvParam);
  // Разобрать запрос. Метод возвращает код комманды.
  static ECommandCode  ParseMessage(TMessageBuf &mbRequest,
                                    WORD &wFrom, WORD &wTo);
  // Сформировать ответ на внутренний запрос клиента
  bool  GetAnswerToRequest(CNamedPipe *pNP);
  // Оповестить удалённый узел о старте "своего" узла
  void  NotifyNode(WORD iNode);
  // Завершить (отобразить в интерфейсе) присоединение клиента на
  //заданном удалённом узле к нашему серверу при запуске нашего сервера
  void  ConnectRemoteClient(WORD iNode, WORD iOwner, CNamedPipe *pNP);
  // Завершить (отобразить в интерфейсе) отсоединение клиента на заданном 
  //удалённом узле от нашего сервера при останове удалённого узла
  void  DisconnectRemoteClient(WORD iNode);
  // Очистить узел
  void  ClearNode();

}; // End of class CPeerNetNode declaration ----------------------------
