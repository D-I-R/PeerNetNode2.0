// BlkChain.cpp                                            (c) DIR, 2019
//
//              Реализация классов модели блокчейн
//
// CBlockChain - Модель блокчейн
//
////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
//#include "pch.h"
#include "PeerNetDlg.h" //#include "PeerNetNode.h" //#include "BlkChain.h"

#ifdef _DEBUG
 #define new DEBUG_NEW
#endif

//      Реализация класса CBlockChain
////////////////////////////////////////////////////////////////////////
//      Инициализация статических элементов данных
//

// Конструктор и деструктор
CBlockChain::CBlockChain(CPeerNetDialog *pMainWin, CPeerNetNode *paNode)
           : m_aCrypt(KEYCONTNAME),
             m_rUsers(&m_aCrypt),
             m_rBalances(&m_aCrypt),
             m_rTransacts(&m_aCrypt, &m_rUsers)
{
  m_pMainWin = pMainWin;    //главное окно программы
  m_paNode = paNode;        //узел одноранговой сети
  CancelAuthorization();    //очистить данные авторизации
  //m_bTrans = false;         //флаг "Проводится собственная транзакция"
}
CBlockChain::~CBlockChain(void)
{
} // CBlockChain::~CBlockChain()

//public:
// Внешние методы
// ---------------------------------------------------------------------
// Перегруженный метод. Проверить регистрационные данные и
//идентифицировать пользователя
WORD  CBlockChain::AuthorizationCheck(LPTSTR pszLogin, LPTSTR pszPasswd)
{
  WORD iUser;
  bool b = m_rUsers.IsLoaded();
  if (!b)  b = m_rUsers.Load();
  if (b)  b = m_rUsers.AuthorizationCheck(pszLogin, pszPasswd, iUser);
  else {
    // Ошибка загрузки регистра пользователей
    ; //TraceW(m_rUsers.ErrorMessage());
  }
  return b ? iUser : UNAUTHORIZED;
}
WORD  CBlockChain::AuthorizationCheck(CString sLogin, CString sPasswd)
{
  return AuthorizationCheck(sLogin.GetBuffer(), sPasswd.GetBuffer());
} // CBlockChain::AuthorizationCheck()

// Авторизовать пользователя
bool  CBlockChain::Authorize(WORD iUser)
{
  // Инициализация объекта криптографии
  if (!m_aCrypt.IsInitialized())  m_aCrypt.Initialize();
  bool b = m_rUsers.IsLoaded();
  if (b) {  //здесь регистр пользователей m_rUsers уже должен быть загружен!
    m_rUsers.SetOwner(iUser);
    // Загрузить регистр текущих остатков
    m_rBalances.SetOwner(m_rUsers.OwnerIndex(), m_rUsers.OwnerOrd());
    b = m_rBalances.Load();
  }
  if (b) {
    // Загрузить регистр транзакций
    m_rTransacts.SetOwner(m_rUsers.OwnerIndex(), m_rUsers.OwnerOrd());
    b = m_rTransacts.Load();
  }
  return b;
} // CBlockChain::Authorize()

// Отменить авторизацию пользователя и обнулить данные объекта
void  CBlockChain::CancelAuthorization()
{
  m_rUsers.ClearOwner();
  m_rBalances.ClearOwner();
  m_rTransacts.ClearOwner();
  memset(m_tVoteRes, 0xFF, sizeof(m_tVoteRes));
  memset(m_iNodes, 0xFF, sizeof(m_iNodes));
  memset(m_iActNodes, 0, sizeof(m_iActNodes));
  m_nActNodes = 0;
  m_iActNode = 0xFFFF;
} // CBlockChain::CancelAuthorization()

// Связать пользователя и узел сети (после авторизации и запуска узла)
void  CBlockChain::TieUpUserAndNode(WORD iUser, WORD iNode)
{
  ASSERT(0 <= iUser && iUser < NODECOUNT && 0 <= iNode && iNode < NODECOUNT);
  m_iNodes[iUser] = iNode;
  m_tVoteRes[iNode].Init(iUser);
} // CBlockChain::TieUpUserAndNode()

// Отвязать пользователя от узла сети
void  CBlockChain::UntieUserAndNode(WORD iNode)
{
  if (0 <= iNode && iNode < NODECOUNT)
  {
    WORD iUser = m_tVoteRes[iNode]._iUser;
    memset(m_tVoteRes+iNode, 0xFF, sizeof(TVotingRes));
    m_iNodes[iUser] = 0xFFFF;
  }
} // CBlockChain::UntieUserAndNode()

// Обработать сообщение DTB из очереди (полученное серверным каналом)
bool  CBlockChain::ProcessDTB()
{
  TMessageBuf *pmbDTB;  bool b;
  EnterCriticalSection(&g_cs);
  b = m_paNode->m_oDTBQueue.Get(pmbDTB);    //дать DTB
  LeaveCriticalSection(&g_cs);
  if (!b)  return b;
  // Скопировать сообщение DTB в буфер для изменения
  TMessageBuf  tMessBuf(pmbDTB->MessBuffer(), pmbDTB->MessageBytes());
  WORD iNodeFrom;
  // Извлечь подкоманду из DTB, определить iNodeFrom
  m_paNode->UnwrapData(iNodeFrom, &tMessBuf);
  TSubcmd tCmd;
  ESubcmdCode eSbcmd = ParseMessage(tMessBuf, tCmd);
  switch (eSbcmd) {
case ScC_ATR:   //Add in Transaction Register
    // Актуализировать в узле (tCmd._szNode) (tCmd._szBlock) блоков 
    //регистра транзакций
    m_iSrcNode = tCmd.GetNode();        //узел-источник для актуализации
    m_nAbsentBlks = tCmd.GetBlockNo();  //колич недостающих блоков транзакций
    b = AskTransactionBlock();
    break;
case ScC_GTB:   //Get Transaction Register Block
    b = GetTransactionBlock(tCmd.GetBlockNo(), tMessBuf);   //ответ - TBL
    RequestNoReply(iNodeFrom, tMessBuf);
    break;
case ScC_TAQ:   //TransAction reQuest
    b = TransActionreQuest(tCmd, tMessBuf);             //ответ - TAR
    RequestNoReply(iNodeFrom, tMessBuf);
    break;
case ScC_TAR:   //TransAction request Replay
    // Ответ удалённого узла на запрос транзакции занести 
    //в таблицу голосования
    b = AddInVotingTable(iNodeFrom, &tMessBuf);
    break;
case ScC_TBL:   //Transaction BLock
	// Получен блок транзакции - добавить в регистр транзакций
    b = AddTransactionBlock(tMessBuf);
    if (m_nAbsentBlks > 0)
      b = AskTransactionBlock();
    else
      ExecuteTransactionOnLocalNode(m_mbTRA);
    break;
case ScC_TRA:   //execute TRansAction
	if (tCmd.GetTransNo() == m_rTransacts.GetNewTransactOrd())
      // Регистр транзакций актуален - провести транзакцию на своём узле
      b = ExecuteTransactionOnLocalNode(tMessBuf);
	else {
      // Отложить проведение транзакции до актуализации регистра транзакций ...
      m_mbTRA = tMessBuf;   //сохранить сообщение TRA
      // ... а пока запросить недостающие блоки транзакций
      m_iSrcNode = iNodeFrom;
      //m_nTransBlock = m_rTransacts.GetNewTransactOrd();
      m_nAbsentBlks = tCmd.GetTransNo() - m_rTransacts.GetNewTransactOrd();
      b = AskTransactionBlock();
	}
case 0:         //это не команда - обработка не нужна
    break;
  } //switch
  return b;
} // CBlockChain::ProcessDTB()

// Инициировать транзакцию
bool  CBlockChain::StartTransaction(WORD iUserTo, double rAmount)
//iUserTo - индекс пользователя "кому"
//rAmount - сумма транзакции
{
  // Проверить необходимые условия проведения транзакции
  ASSERT(0 <= iUserTo && iUserTo < NODECOUNT);
  WORD iUserFrom = m_rUsers.OwnerIndex();           //индекс владельца узла
  TBalance *ptBalance = m_rBalances.GetAt(iUserFrom);
  double rBalance = ptBalance->GetBalance();
  if (rAmount > rBalance) {
    m_rBalances.SetError(7);    //сумма транзакции превышает остаток
    m_pMainWin->ShowMessage(m_rBalances.ErrorMessage());
    return false;
  }
  if (!m_paNode->IsQuorumPresent())  return false;  //нет кворума
  //m_bTrans = true;              //флаг "Проводится собственная транзакция"
  // Сформировать подкоманду TAQ "Запросить транзакцию"
  WORD nTrans = m_rTransacts.GetNewTransactOrd();   //номер новой транзакции
  TCHAR szMessBuf[ERMES_BUFSIZE];
  CreateTransactSubcommand(szMessBuf, _T("TAQ"),
                           nTrans, iUserFrom, iUserTo, rAmount);
  TMessageBuf mbRequest, *pmbTAR = NULL;
  WORD iNodeTo;  bool b;
  m_nActTrans = m_nVote = 0;    //инициализация голосования
  // Цикл голосования - 
  //разослать запросы TAQ всем узлам сети (включая свой)
  for (b = m_paNode->GetFirstNode(iNodeTo, true);
       b; b = m_paNode->GetNextNode(iNodeTo, true)) {
    mbRequest.PutMessage(szMessBuf);        //занести в буфер
    if (iNodeTo == m_paNode->m_iOwnNode) {
      pmbTAR = EmulateReply(mbRequest);
      // "Ответ" своего узла получен - занести в таблицу голосования
      b = AddInVotingTable(iNodeTo, pmbTAR);
	  delete pmbTAR;  pmbTAR = NULL;
    }
    else
      RequestNoReply(iNodeTo, mbRequest);   //портит mbRequest!
  }
  return true;
} // CBlockChain::StartTransaction()

// Проанализировать результаты голосования
bool  CBlockChain::AnalyzeVoting()
{
  m_nActTrans = GetActualTransactOrd(); //актуальный номер транзакции
  BuildActualNodeList(m_nActTrans); //построить список актуальных узлов
  // Сформировать сообщение TRA выполнения транзакции
  CreateTRAMessage(); //формируется m_mbTRA
  bool b, bActual;  //признак актуальности "своего" узла
  TCHAR chMess[CHARBUFSIZE];  TMessageBuf mbRequest;
  // Цикл по строкам таблицы голосования
  for (WORD i = 0; i < m_nVote; i++) {
    b = m_tVoteRes[i]._nTrans == m_nActTrans;   //i-й узел актуален?
    if (i == m_paNode->m_iOwnNode)
      bActual = b;      //актуальность "своего" узла
    else if (b) {
      // Удалённый узел i актуален - послать ему сообщение TRA
      mbRequest.PutMessage(m_mbTRA.Message());  //занести в буфер
      // Послать узлу i
      RequestNoReply(i, mbRequest); //портит mbRequest!
    }
    else {
      // Удалённый узел i не актуален - послать ему сообщение ATR.
      // Сформировать сообщение ATR ...
      swprintf_s(chMess, CHARBUFSIZE, _T("ATR %02d%02d"),
                 m_iActNodes[m_iActNode], m_nActTrans - m_tVoteRes[i]._nTrans);
      NextActualNode(); //к следующему актуальному узлу
      mbRequest.PutMessage(chMess); //занести в буфер
      // Послать узлу i
      RequestNoReply(i, mbRequest); //портит mbRequest!
    }
  }
  return bActual;
} // CBlockChain::AnalyzeVoting()

// Сформировать сообщение TRA на основании информации из таблицы голосования
void  CBlockChain::CreateTRAMessage()
{
  TCHAR szMess[CHARBUFSIZE];
  WORD iNode     = m_paNode->m_iOwnNode,            //владелец узла
       nTrans    = m_nActTrans,                     //номер транзакции
       iUserFrom = m_tVoteRes[iNode]._iUserFrom,    //пользователь "кто"
       iUserTo   = m_tVoteRes[iNode]._iUserTo;      //пользователь "кому"
  double rAmount = m_tVoteRes[iNode]._rSum;         //сумма транзакции
  CreateTransactSubcommand(szMess, _T("TRA"),
                           nTrans, iUserFrom, iUserTo, rAmount);
  m_mbTRA.PutMessage(szMess);   //занести в буфер
} // CBlockChain::CreateTRAMessage()

// Провести транзакцию на своём узле
bool  CBlockChain::ExecuteTransactionOnLocalNode(TMessageBuf &mbTRA)
//iUserFrom - индекс пользователя-отправителя
//iUserTo   - индекс пользователя-получателя
//rAmount   - сумма транзакции
{
  TSubcmd tCmd;
  if (ParseMessage(mbTRA, tCmd) != ScC_TRA)  return false;
  // Извлечь параметры транзакции
  WORD nTrans = tCmd.GetTransNo(),
#ifdef DBGHASHSIZE
       iUserFrom =
         CPeerNetNode::ToNumber((LPTSTR)tCmd._chHashFrom, SHORTNUMSIZ),
       iUserTo = CPeerNetNode::ToNumber((LPTSTR)tCmd._chHashTo, SHORTNUMSIZ);
#else
       iUserFrom = m_rUsers.GetUserByLoginHash(tCmd._chHashFrom),
       iUserTo = m_rUsers.GetUserByLoginHash(tCmd._chHashTo);
#endif
  double rAmount = tCmd.GetAmount();
  // Сформировать блок транзакции
  TTransact *ptRansact =
    m_rTransacts.CreateTransactBlock(iUserFrom, iUserTo, rAmount);
  m_rTransacts.Add(ptRansact);  //добавить в регистр транзакций
  m_pMainWin->AddInTransactTable(ptRansact);    //отобразить
  // Скорректировать балансы пользователей
  CorrectBalance(iUserFrom, -rAmount);
  CorrectBalance(iUserTo, rAmount);
  return true;
} // CBlockChain::ExecuteTransactionOnLocalNode()

// Закрыть регистры БД
void  CBlockChain::CloseRegisters()
{
  m_rBalances.Save();   //регистр текущих остатков
  m_rTransacts.Save();  //регистр транзакций
} // CBlockChain::CloseRegisters()

//private:
// Внутренние методы
// ---------------------------------------------------------------------
// Сформировать подкоманду запроса, ответа или исполнения транзакции
void  CBlockChain::CreateTransactSubcommand(
  LPTSTR pszMsg, LPTSTR pszSbcmd, WORD nTrans,
  WORD iUserFrom, WORD iUserTo, double rAmount)
{
  // Сформировать подкоманду транзакции
#ifdef DBGHASHSIZE
  // Отладочный вариант
  swprintf_s(pszMsg, CHARBUFSIZE, _T("%s %08d%02d%02d%015.2f"),
             pszSbcmd, nTrans, iUserFrom, iUserTo, rAmount);
#else
  // Рабочий вариант
  swprintf_s(pszMsg, CHARBUFSIZE, _T("%s %08d"), pszSbcmd, nTrans);
  TUser *ptUserFrom = m_rUsers.GetAt(iUserFrom),
        *ptUserTo = m_rUsers.GetAt(iUserTo);
  memcpy(pszMsg+HASHFROMOFF, ptUser->_chLogHash, HASHSIZE);
  memcpy(pszMsg+HASHTO_OFFS, ptUser->_chLogHash, HASHSIZE);
  swprintf_s(pszMsg+AMOUNT_OFFS, CHARBUFSIZE-AMOUNT_OFFS,
             _T("%015.2f"), rAmount);
#endif
} // CBlockChain::CreateTransactSubcommand()

// Послать сообщение прикладного уровня заданному узлу 
//с упаковкой в блок данных, без получения ответа
void  CBlockChain::RequestNoReply(WORD iNodeTo, TMessageBuf &mbRequest)
//iNode     - индекс узла-адресата
//mbRequest - буфер сообщения
{
  TMessageBuf *pmbReply = NULL;
  WORD iNodeFrom;
  m_paNode->WrapData(iNodeTo, mbRequest);   //упаковать подкоманду в DTB
  // Послать сообщение узлу iNodeTo
  pmbReply = m_paNode->RequestAndReply(iNodeTo, mbRequest);
  if (pmbReply) {       //ответ получен, он может быть только ACK
	// Распаковать подкоманду из DTB и/или определить iNodeFrom
    if (pmbReply->MessageBytes() > 0) {
      m_paNode->UnwrapData(iNodeFrom, pmbReply);
      ASSERT(iNodeFrom == iNodeTo);
    }
    delete pmbReply;    //ответ не требует обработки
  }
} // RequestNoReply()

// Эмулировать ответ своего узла на сообщение прикладного уровня
TMessageBuf * CBlockChain::EmulateReply(TMessageBuf &mbTAQ)
{
  // Сформировать подкоманду TAR "Выполнить транзакцию" на месте 
  //подкоманды TAQ
  TMessageBuf *pmbTAR = new TMessageBuf(mbTAQ.Message());
  memcpy(pmbTAR->Message(), _T("TAR"), 3*sizeof(TCHAR));
  return pmbTAR;
} // CBlockChain::EmulateReply()

// Разобрать сообщение. Метод возвращает код подкоманды и описатель сообщ.
ESubcmdCode  CBlockChain::ParseMessage(TMessageBuf &mbMess, TSubcmd &tCmd)
//mbMess[in] - сообщение
//tCmd[out]  - результат разбора, описатель сообщения
{
  static LPTSTR sCmds[] = {
//Индекс             Код подкоманды
/* 0 */ _T("ATR"),  //ScC_ATR = 11, Add in Transact Reg - Добавить в журнал та
/* 1 */ _T("GTB"),  //ScC_GTB,      Get Trans log Block - Дать блок журнала та
/* 2 */ _T("TAQ"),  //ScC_TAQ,      TransAction reQuest - Запрос транзакции
/* 3 */ _T("TAR"),  //ScC_TAR,      TransAction Reply   - Ответ на запрос тран
/* 4 */ _T("TBL"),  //ScC_TBL,      Transact log BLock  - Блок транзакции
/* 5 */ _T("TRA"),  //ScC_TRA,      execute TRansAction - Выполнить транзакцию
  };
  static WORD n = sizeof(sCmds) / sizeof(LPTSTR);
  WORD i = 0;
  for ( ; i < n; i++) {
    if (mbMess.Compare(sCmds[i], 3) == 0) {
      tCmd._eCode = (ESubcmdCode)(ScC_ATR + i);  goto SbcmdFound;
    }
  }
  tCmd._eCode = (ESubcmdCode)0;
SbcmdFound:
  // Подкоманда определена, сформировть описатель
  switch (tCmd._eCode) {
case ScC_ATR:   //Add in Transaction Register
    // Номер (индекс) узла
    tCmd.SetNode(mbMess.Message() + CMDFIELDSIZ);
    // Количество блоков
    tCmd.SetBlockNo(mbMess.Message() + CMDFIELDSIZ+SHORTNUMSIZ);
    break;
case ScC_GTB:   //Get Transaction Register Block
    // Номер блока от конца цепи
    tCmd.SetBlockNo(mbMess.Message() + CMDFIELDSIZ);
    break;
case ScC_TAQ:   //TransAction reQuest
case ScC_TAR:   //TransAction Reply
case ScC_TRA:   //execute TRansAction
    // Номер транзакции
    tCmd.SetTransNo(mbMess.Message() + TRANSNUMOFF);
    // Хеш-код отправителя
    tCmd.SetHashFrom(mbMess.MessBuffer() + HASHFROMOFF*sizeof(TCHAR));
    // Хеш-код получателя
    tCmd.SetHashTo(mbMess.MessBuffer() + HASHTO_OFFS*sizeof(TCHAR));
    // Сумма транзакции
    tCmd.SetAmount(mbMess.Message() + AMOUNT_OFFS);
    break;
case ScC_TBL:   //Transact register BLock
    // Номер транзакции
    tCmd.SetTransNo(mbMess.Message() + TRANSNUMOFF);
    // Хеш-код отправителя
    tCmd.SetHashFrom(mbMess.MessBuffer() + HASHFROMOFF*sizeof(TCHAR));
    // Хеш-код получателя
    tCmd.SetHashTo(mbMess.MessBuffer() + HASHTO_OFFS*sizeof(TCHAR));
    // Сумма транзакции
    tCmd.SetAmount(mbMess.Message() + AMOUNT_OFFS);
    // Хеш-код предыдущего блока транзакции
    tCmd.SetHashPrev(mbMess.MessBuffer() + HASHPREVOFF*sizeof(TCHAR));
    break;
  } //switch
  return tCmd._eCode;   //подкоманда не определена
} // CBlockChain::ParseMessage()

// Поместить ответ в таблицу голосования
bool  CBlockChain::AddInVotingTable(WORD iNodeFrom, TMessageBuf *pmbTAR)
{
  TSubcmd tCmd;
  bool b = (ParseMessage(*pmbTAR, tCmd) == ScC_TAR);
  if (!b)  return b;
  WORD nTrans, iUser = m_tVoteRes[iNodeFrom]._iUser;
  m_tVoteRes[iNodeFrom]._nTrans = tCmd.GetTransNo();
  m_tVoteRes[iNodeFrom]._iUserFrom =
    m_rUsers.GetUserByName((LPTSTR)tCmd._chHashFrom, 0);
  m_tVoteRes[iNodeFrom]._iUserTo =
    m_rUsers.GetUserByName((LPTSTR)tCmd._chHashTo, 0);
  m_tVoteRes[iNodeFrom]._rSum = tCmd.GetAmount();
  CString sNode, sTrans, sAmount;
  TCHAR szOwner[NAMESBUFSIZ], szUserFrom[NAMESBUFSIZ],
        szUserTo[NAMESBUFSIZ], *pszFields[VOTECOLNUM];
  sNode.Format(_T("Узел #%d"), iNodeFrom+1);
  pszFields[0] = sNode.GetBuffer();     //номер узла
  UserRegister()->UserName(m_tVoteRes[iNodeFrom]._iUser, szOwner);
  pszFields[1] = szOwner;               //имя владельца узла
  pszFields[2] = tCmd._szTrans;         //номер транзакции
  UserRegister()->UserName(m_tVoteRes[iNodeFrom]._iUserFrom, szUserFrom);
  pszFields[3] = szUserFrom;            //имя пользователя-отправителя
  UserRegister()->UserName(m_tVoteRes[iNodeFrom]._iUserTo, szUserTo);
  pszFields[4] = szUserTo;              //имя пользователя-получателя
  sAmount.Format(_T("%.2lf"), m_tVoteRes[iNodeFrom]._rSum);
  pszFields[5] = sAmount.GetBuffer();   //сумма
  m_pMainWin->AddInVotingTable(pszFields);
  m_nVote++;    //подсчёт проголосовавших узлов
  if (!IsTheVoteOver())  return b;  //ещё не все проголосовали
  // Проголосовали все - проанализировать результаты голосования
  b = AnalyzeVoting();  //опр акт номер транзакции и список акт узлов
  if (!b) {
    // Локальный узел неактуален - дополнить регистр транзакций локального 
    //узла недостающими блоками транзакций
    m_pMainWin->ApproveTransaction(1);      //отобразить отклонение транзакции
    m_iSrcNode = m_iActNodes[m_iActNode];   //узел-источник для актуализации
    NextActualNode();   //к следующему актуальному узлу
    m_nAbsentBlks = m_nActTrans - m_tVoteRes[m_paNode->m_iOwnNode]._nTrans;
    b = AskTransactionBlock();  //начать актуализацию локального узла
    return b;
  }
  // Локальный узел актуален - можно сразу провести транзакцию
  b = ExecuteTransactionOnLocalNode(m_mbTRA);
    /* Взять "свою" строку в таблице голосования  TVotingRes
    if (iNodeFrom != m_paNode->m_iOwnNode) {
      sTrans.Format(_T("%08d"), m_tVoteRes[m_paNode->m_iOwnNode]._nTrans);
      sAmount.Format(_T("%015.2lf"), m_tVoteRes[m_paNode->m_iOwnNode]._rSum);
      memcpy(pmbTAR->_chMess+TRANSNUMOFF,
             sTrans.GetBuffer(), LONGNUMSIZE*sizeof(TCHAR));
      memcpy(pmbTAR->_chMess+AMOUNT_OFFS,
             sAmount.GetBuffer(), NAMESBUFSIZ*sizeof(TCHAR));
    }
    ExecuteTransaction(*pmbTAR); */
  return b;
} // CBlockChain::AddInVotingTable()

// Запросить блок транзакции для актуализации регистра транзакций
bool  CBlockChain::AskTransactionBlock()
{
  ASSERT(m_nAbsentBlks > 0);
  // Сформировать сообщение GTB ...
  TMessageBuf mbGTB;  TCHAR szMess[CHARBUFSIZE];
  swprintf_s(szMess, CHARBUFSIZE, _T("GTB %02d"), m_rTransacts.GetNewTransactOrd());
  mbGTB.PutMessage(szMess);     //занести в буфер
  // ... послать заданному узлу
  RequestNoReply(m_iSrcNode, mbGTB);
  m_nAbsentBlks--;
  return true;
} // CBlockChain::AskTransactionBlock()

// Обработать сообщение GTB "Дать блок регистра транзакций".
//В ответ формируется сообщение TBL.
bool  CBlockChain::GetTransactionBlock(WORD nBlock, TMessageBuf &mbGTBL)
//nBlock[in]  - номер запрашиваемого блока (1,2,..)
//mbGTBL[out] - буфер сообщения: на входе GTB, на выходе TBL
{
  TCHAR szMess[CHARBUFSIZE];    //буфер сообщения (TCHAR=wchar_t)
  TTransact *ptRansact = m_rTransacts.GetAt(nBlock - 1);
  WORD iUserFrom = m_rUsers.GetUserByLoginHash(ptRansact->_chHashSrc),
       iUserTo = m_rUsers.GetUserByLoginHash(ptRansact->_chHashDst),
       nLeng = HASHPREVOFF*sizeof(TCHAR)+HASHSIZE*2;
  CreateTransactSubcommand(szMess, _T("TBL"), nBlock,
                           iUserFrom, iUserTo, ptRansact->GetAmount());
  CTransRegister::HashToSymbols((char *)(szMess+HASHPREVOFF),
                                ptRansact->_chPrevHash);
  mbGTBL.Put((BYTE *)szMess, nLeng);
  return true;
} // CBlockChain::GetTransactionBlock()

// Обработать сообщение TAQ "Запрос транзакции".
//В ответ формируется сообщение TAR.
bool  CBlockChain::TransActionreQuest(TSubcmd &tCmd, TMessageBuf &mbTAQR)
//tCmd[out]      - результат разбора, описатель сообщения TAQ
//mbTAQR[in/out] - буфер сообщения: на входе TAQ, на выходе TAR
{
  LPTSTR pszMsg = mbTAQR.Message();  DWORD n;
  swscanf_s((LPTSTR)tCmd._chHashFrom, _T("%02d"), &n);
  WORD nTrans = m_rTransacts.GetNewTransactOrd(), iUserFrom = (WORD)n;
  TBalance *ptBalnc = m_rBalances.GetAt(iUserFrom);
  double rAmount = ptBalnc->GetBalance();
  TCHAR chMess[CHARBUFSIZE];
  swprintf_s(chMess, CHARBUFSIZE, _T("TAR %08d%015.2f"), nTrans, rAmount);
  n = CMDFIELDSIZ + LONGNUMSIZE;    //смещение значения rAmount в chMess
  memcpy(mbTAQR.Message(), chMess, sizeof(TCHAR)*n);
  memcpy(mbTAQR.Message() + AMOUNT_OFFS,
         chMess + n, sizeof(TCHAR)*NAMESBUFSIZ);
  return true;
} // CBlockChain::TransActionreQuest()

// Добавить в регистр транзакций новый блок (при актуализации).
bool  CBlockChain::AddTransactionBlock(TMessageBuf &mbTBL)
{
  TSubcmd tCmd;
  bool b = (ParseMessage(mbTBL, tCmd) == ScC_TBL);
  if (!b)  return b;
  WORD nTrans = tCmd.GetTransNo(),
       iUserFrom = m_rUsers.GetUserByName((LPTSTR)tCmd._chHashFrom, 0),
       iUserTo = m_rUsers.GetUserByName((LPTSTR)tCmd._chHashTo, 0);
  double rAmount = tCmd.GetAmount();
  TTransact tRansact;  TUser *ptUser;
  b = (nTrans == m_rTransacts.GetNewTransactOrd());
  if (b) {
    tRansact.SetTransOrd(nTrans);
    ptUser = m_rUsers.GetAt(iUserFrom);
    tRansact.SetHashSource(ptUser->_chLogHash, HASHSIZE);
    ptUser = m_rUsers.GetAt(iUserTo);
    tRansact.SetHashDestination(ptUser->_chLogHash, HASHSIZE);
    // Вставить хеш-код последней транзакции
    CTransRegister::SymbolsToHash((char *)(tCmd._chHashPrev),
                                  tRansact._chPrevHash);
    tRansact.SetAmount(rAmount);
    m_rTransacts.Add(&tRansact);    //добавить в регистр транзакций
    m_pMainWin->AddInTransactTable(&tRansact);  //отобразить
    // Скорректировать балансы пользователей
    CorrectBalance(iUserFrom, -rAmount);
    CorrectBalance(iUserTo, rAmount);
  }
  else {
    // Ошибка - нарушение последовательности блоков хеш-чейн
    ASSERT(FALSE);
  }
  return b;
} // CBlockChain::AddTransactionBlock()

// Скорректировать текущий остаток счёта пользователя на заданную сумму
void  CBlockChain::CorrectBalance(WORD iUser, double rTurnover)
//iUser     - индекс пользователя
//rTurnover - сумма оборота (+/-)
{
  TBalance *ptBalnc = m_rBalances.GetAt(iUser);
  double rSum = ptBalnc->GetBalance();  //текущий остаток
  rSum += rTurnover;
  ptBalnc->SetBalance(rSum);  m_rBalances.SetChanged();
  // Если свой баланс - отобразить
  if (iUser == m_paNode->m_iOwner)  m_pMainWin->ShowBalance(rSum);
} // CBlockChain::CorrectBalance()

// Полностью пересчитать текущий остаток счёта заданного пользователя
void  CBlockChain::CalculateBalance(WORD iUser)
{
  double rSum = 1000.0, rTurnover;
  WORD iUserFrom, iUserTo, n = m_rTransacts.ItemCount();
  TTransact *ptRansact;
  // Просмотр всего регистра транзакций
  for (WORD i = 0; i < n; i++) {
    ptRansact = m_rTransacts.GetAt(i);
    iUserFrom = m_rUsers.GetUserByLoginHash(ptRansact->_chHashSrc),
    iUserTo = m_rUsers.GetUserByLoginHash(ptRansact->_chHashDst);
	if (iUserFrom == iUser || iUserTo == iUser) {
      rTurnover = ptRansact->GetAmount();
      if (iUserFrom == iUser)  rTurnover = -rTurnover;
      rSum += rTurnover;
	}
  }
  TBalance *ptBalnc = m_rBalances.GetAt(iUser);
  ptBalnc->SetBalance(rSum);  m_rBalances.SetChanged();
  // Если свой баланс - отобразить
  if (iUser == m_paNode->m_iOwner)  m_pMainWin->ShowBalance(rSum);
} // CBlockChain::CalculateBalance()

// Найти максимальный номер транзакции в таблице голосования
WORD  CBlockChain::GetActualTransactOrd()
{
  WORD iAct = 0;
  for (WORD i = 1; i < m_nVote; i++)
    if (m_tVoteRes[i]._nTrans > m_tVoteRes[iAct]._nTrans) {
      iAct = i;
    }
  return m_tVoteRes[iAct]._nTrans;
} // CBlockChain::GetActualTransactOrd()

// Построить список актуальных узлов
void  CBlockChain::BuildActualNodeList(WORD nTransAct)
{
  WORD i;
  for (m_nActNodes = 0, i = 0; i < m_nVote; i++)
    if (m_tVoteRes[i]._nTrans == nTransAct) {
      m_iActNodes[m_nActNodes++] = i;
    }
  m_iActNode = 0;   //на первый элемент списка "актуальных" узлов
} // CBlockChain::BuildActualNodeList()

// Перейти к следующему актуальному узлу в списке (циклический перебор)
void  CBlockChain::NextActualNode()
{
  m_iActNode++;
  if (m_iActNode == m_nActNodes)  m_iActNode = 0;
} // CBlockChain::NextActualNode()

// End of class CBalanceRegister definition ----------------------------
