// BlkChain.cpp                                            (c) DIR, 2019
//
//              Ðåàëèçàöèÿ êëàññîâ ìîäåëè áëîê÷åéí
//
// CBlockChain - Ìîäåëü áëîê÷åéí
//
////////////////////////////////////////////////////////////////////////

//#include "stdafx.h"
#include "pch.h"
#include "PeerNetDlg.h" //#include "PeerNetNode.h" //#include "BlkChain.h"

#ifdef _DEBUG
 #define new DEBUG_NEW
#endif

//      Ðåàëèçàöèÿ êëàññà CBlockChain
////////////////////////////////////////////////////////////////////////
//      Èíèöèàëèçàöèÿ ñòàòè÷åñêèõ ýëåìåíòîâ äàííûõ
//

// Êîíñòðóêòîð è äåñòðóêòîð
CBlockChain::CBlockChain(CPeerNetDialog *pMainWin, CPeerNetNode *paNode)
           : m_aCrypt(KEYCONTNAME),
             m_rUsers(&m_aCrypt),
             m_rBalances(&m_aCrypt),
             m_rTransacts(&m_aCrypt, &m_rUsers)
{
  m_pMainWin = pMainWin;    //ãëàâíîå îêíî ïðîãðàììû
  m_paNode = paNode;        //óçåë îäíîðàíãîâîé ñåòè
  CancelAuthorization();    //î÷èñòèòü äàííûå àâòîðèçàöèè
  //m_bTrans = false;         //ôëàã "Ïðîâîäèòñÿ ñîáñòâåííàÿ òðàíçàêöèÿ"
}
CBlockChain::~CBlockChain(void)
{
} // CBlockChain::~CBlockChain()

//public:
// Âíåøíèå ìåòîäû
// ---------------------------------------------------------------------
// Ïåðåãðóæåííûé ìåòîä. Ïðîâåðèòü ðåãèñòðàöèîííûå äàííûå è
//èäåíòèôèöèðîâàòü ïîëüçîâàòåëÿ
WORD  CBlockChain::AuthorizationCheck(LPTSTR pszLogin, LPTSTR pszPasswd)
{
  WORD iUser;
  bool b = m_rUsers.IsLoaded();
  if (!b)  b = m_rUsers.Load();
  if (b)  b = m_rUsers.AuthorizationCheck(pszLogin, pszPasswd, iUser);
  else {
    // Îøèáêà çàãðóçêè ðåãèñòðà ïîëüçîâàòåëåé
    ; //TraceW(m_rUsers.ErrorMessage());
  }
  return b ? iUser : UNAUTHORIZED;
}
WORD  CBlockChain::AuthorizationCheck(CString sLogin, CString sPasswd)
{
  return AuthorizationCheck(sLogin.GetBuffer(), sPasswd.GetBuffer());
} // CBlockChain::AuthorizationCheck()

// Àâòîðèçîâàòü ïîëüçîâàòåëÿ
bool  CBlockChain::Authorize(WORD iUser)
{
  // Èíèöèàëèçàöèÿ îáúåêòà êðèïòîãðàôèè
  if (!m_aCrypt.IsInitialized())  m_aCrypt.Initialize();
  bool b = m_rUsers.IsLoaded();
  if (b) {  //çäåñü ðåãèñòð ïîëüçîâàòåëåé m_rUsers óæå äîëæåí áûòü çàãðóæåí!
    m_rUsers.SetOwner(iUser);
    // Çàãðóçèòü ðåãèñòð òåêóùèõ îñòàòêîâ
    m_rBalances.SetOwner(m_rUsers.OwnerIndex(), m_rUsers.OwnerOrd());
    b = m_rBalances.Load();
  }
  if (b) {
    // Çàãðóçèòü ðåãèñòð òðàíçàêöèé
    m_rTransacts.SetOwner(m_rUsers.OwnerIndex(), m_rUsers.OwnerOrd());
    b = m_rTransacts.Load();
  }
  return b;
} // CBlockChain::Authorize()

// Îòìåíèòü àâòîðèçàöèþ ïîëüçîâàòåëÿ è îáíóëèòü äàííûå îáúåêòà
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

// Ñâÿçàòü ïîëüçîâàòåëÿ è óçåë ñåòè (ïîñëå àâòîðèçàöèè è çàïóñêà óçëà)
void  CBlockChain::TieUpUserAndNode(WORD iUser, WORD iNode)
{
  ASSERT(0 <= iUser && iUser < NODECOUNT && 0 <= iNode && iNode < NODECOUNT);
  m_iNodes[iUser] = iNode;
  m_tVoteRes[iNode].Init(iUser);
} // CBlockChain::TieUpUserAndNode()

// Îòâÿçàòü ïîëüçîâàòåëÿ îò óçëà ñåòè
void  CBlockChain::UntieUserAndNode(WORD iNode)
{
  if (0 <= iNode && iNode < NODECOUNT)
  {
    WORD iUser = m_tVoteRes[iNode]._iUser;
    memset(m_tVoteRes+iNode, 0xFF, sizeof(TVotingRes));
    m_iNodes[iUser] = 0xFFFF;
  }
} // CBlockChain::UntieUserAndNode()

// Îáðàáîòàòü ñîîáùåíèå DTB èç î÷åðåäè (ïîëó÷åííîå ñåðâåðíûì êàíàëîì)
bool  CBlockChain::ProcessDTB()
{
  TMessageBuf *pmbDTB;  bool b;
  EnterCriticalSection(&g_cs);
  b = m_paNode->m_oDTBQueue.Get(pmbDTB);    //äàòü DTB
  LeaveCriticalSection(&g_cs);
  if (!b)  return b;
  // Ñêîïèðîâàòü ñîîáùåíèå DTB â áóôåð äëÿ èçìåíåíèÿ
  TMessageBuf  tMessBuf(pmbDTB->MessBuffer(), pmbDTB->MessageBytes());
  WORD iNodeFrom;
  // Èçâëå÷ü ïîäêîìàíäó èç DTB, îïðåäåëèòü iNodeFrom
  m_paNode->UnwrapData(iNodeFrom, &tMessBuf);
  TSubcmd tCmd;
  ESubcmdCode eSbcmd = ParseMessage(tMessBuf, tCmd);
  switch (eSbcmd) {
case ScC_ATR:   //Add in Transaction Register
    // Àêòóàëèçèðîâàòü â óçëå (tCmd._szNode) (tCmd._szBlock) áëîêîâ 
    //ðåãèñòðà òðàíçàêöèé
    m_iSrcNode = tCmd.GetNode();        //óçåë-èñòî÷íèê äëÿ àêòóàëèçàöèè
    m_nAbsentBlks = tCmd.GetBlockNo();  //êîëè÷ íåäîñòàþùèõ áëîêîâ òðàíçàêöèé
    b = AskTransactionBlock();
    break;
case ScC_GTB:   //Get Transaction Register Block
    b = GetTransactionBlock(tCmd.GetBlockNo(), tMessBuf);   //îòâåò - TBL
    RequestNoReply(iNodeFrom, tMessBuf);
    break;
case ScC_TAQ:   //TransAction reQuest
    b = TransActionreQuest(tCmd, tMessBuf);             //îòâåò - TAR
    RequestNoReply(iNodeFrom, tMessBuf);
    break;
case ScC_TAR:   //TransAction request Replay
    // Îòâåò óäàë¸ííîãî óçëà íà çàïðîñ òðàíçàêöèè çàíåñòè 
    //â òàáëèöó ãîëîñîâàíèÿ
    b = AddInVotingTable(iNodeFrom, &tMessBuf);
    break;
case ScC_TBL:   //Transaction BLock
	// Ïîëó÷åí áëîê òðàíçàêöèè - äîáàâèòü â ðåãèñòð òðàíçàêöèé
    b = AddTransactionBlock(tMessBuf);
    if (m_nAbsentBlks > 0)
      b = AskTransactionBlock();
    else
      ExecuteTransactionOnLocalNode(m_mbTRA);
    break;
case ScC_TRA:   //execute TRansAction
	if (tCmd.GetTransNo() == m_rTransacts.GetNewTransactOrd())
      // Ðåãèñòð òðàíçàêöèé àêòóàëåí - ïðîâåñòè òðàíçàêöèþ íà ñâî¸ì óçëå
      b = ExecuteTransactionOnLocalNode(tMessBuf);
	else {
      // Îòëîæèòü ïðîâåäåíèå òðàíçàêöèè äî àêòóàëèçàöèè ðåãèñòðà òðàíçàêöèé ...
      m_mbTRA = tMessBuf;   //ñîõðàíèòü ñîîáùåíèå TRA
      // ... à ïîêà çàïðîñèòü íåäîñòàþùèå áëîêè òðàíçàêöèé
      m_iSrcNode = iNodeFrom;
      //m_nTransBlock = m_rTransacts.GetNewTransactOrd();
      m_nAbsentBlks = tCmd.GetTransNo() - m_rTransacts.GetNewTransactOrd();
      b = AskTransactionBlock();
	}
case 0:         //ýòî íå êîìàíäà - îáðàáîòêà íå íóæíà
    break;
  } //switch
  return b;
} // CBlockChain::ProcessDTB()

// Èíèöèèðîâàòü òðàíçàêöèþ
bool  CBlockChain::StartTransaction(WORD iUserTo, double rAmount)
//iUserTo - èíäåêñ ïîëüçîâàòåëÿ "êîìó"
//rAmount - ñóììà òðàíçàêöèè
{
  // Ïðîâåðèòü íåîáõîäèìûå óñëîâèÿ ïðîâåäåíèÿ òðàíçàêöèè
  ASSERT(0 <= iUserTo && iUserTo < NODECOUNT);
  WORD iUserFrom = m_rUsers.OwnerIndex();           //èíäåêñ âëàäåëüöà óçëà
  TBalance *ptBalance = m_rBalances.GetAt(iUserFrom);
  double rBalance = ptBalance->GetBalance();
  if (rAmount > rBalance) {
    m_rBalances.SetError(7);    //ñóììà òðàíçàêöèè ïðåâûøàåò îñòàòîê
    m_pMainWin->ShowMessage(m_rBalances.ErrorMessage());
    return false;
  }
  if (!m_paNode->IsQuorumPresent())  return false;  //íåò êâîðóìà
  //m_bTrans = true;              //ôëàã "Ïðîâîäèòñÿ ñîáñòâåííàÿ òðàíçàêöèÿ"
  // Ñôîðìèðîâàòü ïîäêîìàíäó TAQ "Çàïðîñèòü òðàíçàêöèþ"
  WORD nTrans = m_rTransacts.GetNewTransactOrd();   //íîìåð íîâîé òðàíçàêöèè
  TCHAR szMessBuf[ERMES_BUFSIZE];
  CreateTransactSubcommand(szMessBuf, _T("TAQ"),
                           nTrans, iUserFrom, iUserTo, rAmount);
  TMessageBuf mbRequest, *pmbTAR = NULL;
  WORD iNodeTo;  bool b;
  m_nActTrans = m_nVote = 0;    //èíèöèàëèçàöèÿ ãîëîñîâàíèÿ
  // Öèêë ãîëîñîâàíèÿ - 
  //ðàçîñëàòü çàïðîñû TAQ âñåì óçëàì ñåòè (âêëþ÷àÿ ñâîé)
  for (b = m_paNode->GetFirstNode(iNodeTo, true);
       b; b = m_paNode->GetNextNode(iNodeTo, true)) {
    mbRequest.PutMessage(szMessBuf);        //çàíåñòè â áóôåð
    if (iNodeTo == m_paNode->m_iOwnNode) {
      pmbTAR = EmulateReply(mbRequest);
      // "Îòâåò" ñâîåãî óçëà ïîëó÷åí - çàíåñòè â òàáëèöó ãîëîñîâàíèÿ
      b = AddInVotingTable(iNodeTo, pmbTAR);
	  delete pmbTAR;  pmbTAR = NULL;
    }
    else
      RequestNoReply(iNodeTo, mbRequest);   //ïîðòèò mbRequest!
  }
  return true;
} // CBlockChain::StartTransaction()

// Ïðîàíàëèçèðîâàòü ðåçóëüòàòû ãîëîñîâàíèÿ
bool  CBlockChain::AnalyzeVoting()
{
  m_nActTrans = GetActualTransactOrd(); //àêòóàëüíûé íîìåð òðàíçàêöèè
  BuildActualNodeList(m_nActTrans); //ïîñòðîèòü ñïèñîê àêòóàëüíûõ óçëîâ
  // Ñôîðìèðîâàòü ñîîáùåíèå TRA âûïîëíåíèÿ òðàíçàêöèè
  CreateTRAMessage(); //ôîðìèðóåòñÿ m_mbTRA
  bool b, bActual;  //ïðèçíàê àêòóàëüíîñòè "ñâîåãî" óçëà
  TCHAR chMess[CHARBUFSIZE];  TMessageBuf mbRequest;
  // Öèêë ïî ñòðîêàì òàáëèöû ãîëîñîâàíèÿ
  for (WORD i = 0; i < m_nVote; i++) {
    b = m_tVoteRes[i]._nTrans == m_nActTrans;   //i-é óçåë àêòóàëåí?
    if (i == m_paNode->m_iOwnNode)
      bActual = b;      //àêòóàëüíîñòü "ñâîåãî" óçëà
    else if (b) {
      // Óäàë¸ííûé óçåë i àêòóàëåí - ïîñëàòü åìó ñîîáùåíèå TRA
      mbRequest.PutMessage(m_mbTRA.Message());  //çàíåñòè â áóôåð
      // Ïîñëàòü óçëó i
      RequestNoReply(i, mbRequest); //ïîðòèò mbRequest!
    }
    else {
      // Óäàë¸ííûé óçåë i íå àêòóàëåí - ïîñëàòü åìó ñîîáùåíèå ATR.
      // Ñôîðìèðîâàòü ñîîáùåíèå ATR ...
      swprintf_s(chMess, CHARBUFSIZE, _T("ATR %02d%02d"),
                 m_iActNodes[m_iActNode], m_nActTrans - m_tVoteRes[i]._nTrans);
      NextActualNode(); //ê ñëåäóþùåìó àêòóàëüíîìó óçëó
      mbRequest.PutMessage(chMess); //çàíåñòè â áóôåð
      // Ïîñëàòü óçëó i
      RequestNoReply(i, mbRequest); //ïîðòèò mbRequest!
    }
  }
  return bActual;
} // CBlockChain::AnalyzeVoting()

// Ñôîðìèðîâàòü ñîîáùåíèå TRA íà îñíîâàíèè èíôîðìàöèè èç òàáëèöû ãîëîñîâàíèÿ
void  CBlockChain::CreateTRAMessage()
{
  TCHAR szMess[CHARBUFSIZE];
  WORD iNode     = m_paNode->m_iOwnNode,            //âëàäåëåö óçëà
       nTrans    = m_nActTrans,                     //íîìåð òðàíçàêöèè
       iUserFrom = m_tVoteRes[iNode]._iUserFrom,    //ïîëüçîâàòåëü "êòî"
       iUserTo   = m_tVoteRes[iNode]._iUserTo;      //ïîëüçîâàòåëü "êîìó"
  double rAmount = m_tVoteRes[iNode]._rSum;         //ñóììà òðàíçàêöèè
  CreateTransactSubcommand(szMess, _T("TRA"),
                           nTrans, iUserFrom, iUserTo, rAmount);
  m_mbTRA.PutMessage(szMess);   //çàíåñòè â áóôåð
} // CBlockChain::CreateTRAMessage()

// Ïðîâåñòè òðàíçàêöèþ íà ñâî¸ì óçëå
bool  CBlockChain::ExecuteTransactionOnLocalNode(TMessageBuf &mbTRA)
//iUserFrom - èíäåêñ ïîëüçîâàòåëÿ-îòïðàâèòåëÿ
//iUserTo   - èíäåêñ ïîëüçîâàòåëÿ-ïîëó÷àòåëÿ
//rAmount   - ñóììà òðàíçàêöèè
{
  TSubcmd tCmd;
  if (ParseMessage(mbTRA, tCmd) != ScC_TRA)  return false;
  // Èçâëå÷ü ïàðàìåòðû òðàíçàêöèè
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
  // Ñôîðìèðîâàòü áëîê òðàíçàêöèè
  TTransact *ptRansact =
    m_rTransacts.CreateTransactBlock(iUserFrom, iUserTo, rAmount);
  m_rTransacts.Add(ptRansact);  //äîáàâèòü â ðåãèñòð òðàíçàêöèé
  m_pMainWin->AddInTransactTable(ptRansact);    //îòîáðàçèòü
  // Ñêîððåêòèðîâàòü áàëàíñû ïîëüçîâàòåëåé
  CorrectBalance(iUserFrom, -rAmount);
  CorrectBalance(iUserTo, rAmount);
  return true;
} // CBlockChain::ExecuteTransactionOnLocalNode()

// Çàêðûòü ðåãèñòðû ÁÄ
void  CBlockChain::CloseRegisters()
{
  m_rBalances.Save();   //ðåãèñòð òåêóùèõ îñòàòêîâ
  m_rTransacts.Save();  //ðåãèñòð òðàíçàêöèé
} // CBlockChain::CloseRegisters()

//private:
// Âíóòðåííèå ìåòîäû
// ---------------------------------------------------------------------
// Ñôîðìèðîâàòü ïîäêîìàíäó çàïðîñà, îòâåòà èëè èñïîëíåíèÿ òðàíçàêöèè
void  CBlockChain::CreateTransactSubcommand(
  LPTSTR pszMsg, LPTSTR pszSbcmd, WORD nTrans,
  WORD iUserFrom, WORD iUserTo, double rAmount)
{
  // Ñôîðìèðîâàòü ïîäêîìàíäó òðàíçàêöèè
#ifdef DBGHASHSIZE
  // Îòëàäî÷íûé âàðèàíò
  swprintf_s(pszMsg, CHARBUFSIZE, _T("%s %08d%02d%02d%015.2f"),
             pszSbcmd, nTrans, iUserFrom, iUserTo, rAmount);
#else
  // Ðàáî÷èé âàðèàíò
  swprintf_s(pszMsg, CHARBUFSIZE, _T("%s %08d"), pszSbcmd, nTrans);
  TUser *ptUserFrom = m_rUsers.GetAt(iUserFrom),
        *ptUserTo = m_rUsers.GetAt(iUserTo);
  memcpy(pszMsg+HASHFROMOFF, ptUser->_chLogHash, HASHSIZE);
  memcpy(pszMsg+HASHTO_OFFS, ptUser->_chLogHash, HASHSIZE);
  swprintf_s(pszMsg+AMOUNT_OFFS, CHARBUFSIZE-AMOUNT_OFFS,
             _T("%015.2f"), rAmount);
#endif
} // CBlockChain::CreateTransactSubcommand()

// Ïîñëàòü ñîîáùåíèå ïðèêëàäíîãî óðîâíÿ çàäàííîìó óçëó 
//ñ óïàêîâêîé â áëîê äàííûõ, áåç ïîëó÷åíèÿ îòâåòà
void  CBlockChain::RequestNoReply(WORD iNodeTo, TMessageBuf &mbRequest)
//iNode     - èíäåêñ óçëà-àäðåñàòà
//mbRequest - áóôåð ñîîáùåíèÿ
{
  TMessageBuf *pmbReply = NULL;
  WORD iNodeFrom;
  m_paNode->WrapData(iNodeTo, mbRequest);   //óïàêîâàòü ïîäêîìàíäó â DTB
  // Ïîñëàòü ñîîáùåíèå óçëó iNodeTo
  pmbReply = m_paNode->RequestAndReply(iNodeTo, mbRequest);
  if (pmbReply) {       //îòâåò ïîëó÷åí, îí ìîæåò áûòü òîëüêî ACK
	// Ðàñïàêîâàòü ïîäêîìàíäó èç DTB è/èëè îïðåäåëèòü iNodeFrom
    if (pmbReply->MessageBytes() > 0) {
      m_paNode->UnwrapData(iNodeFrom, pmbReply);
      ASSERT(iNodeFrom == iNodeTo);
    }
    delete pmbReply;    //îòâåò íå òðåáóåò îáðàáîòêè
  }
} // RequestNoReply()

// Ýìóëèðîâàòü îòâåò ñâîåãî óçëà íà ñîîáùåíèå ïðèêëàäíîãî óðîâíÿ
TMessageBuf * CBlockChain::EmulateReply(TMessageBuf &mbTAQ)
{
  // Ñôîðìèðîâàòü ïîäêîìàíäó TAR "Âûïîëíèòü òðàíçàêöèþ" íà ìåñòå 
  //ïîäêîìàíäû TAQ
  TMessageBuf *pmbTAR = new TMessageBuf(mbTAQ.Message());
  memcpy(pmbTAR->Message(), _T("TAR"), 3*sizeof(TCHAR));
  return pmbTAR;
} // CBlockChain::EmulateReply()

// Ðàçîáðàòü ñîîáùåíèå. Ìåòîä âîçâðàùàåò êîä ïîäêîìàíäû è îïèñàòåëü ñîîáù.
ESubcmdCode  CBlockChain::ParseMessage(TMessageBuf &mbMess, TSubcmd &tCmd)
//mbMess[in] - ñîîáùåíèå
//tCmd[out]  - ðåçóëüòàò ðàçáîðà, îïèñàòåëü ñîîáùåíèÿ
{
  static LPTSTR sCmds[] = {
//Èíäåêñ             Êîä ïîäêîìàíäû
/* 0 */ _T("ATR"),  //ScC_ATR = 11, Add in Transact Reg - Äîáàâèòü â æóðíàë òà
/* 1 */ _T("GTB"),  //ScC_GTB,      Get Trans log Block - Äàòü áëîê æóðíàëà òà
/* 2 */ _T("TAQ"),  //ScC_TAQ,      TransAction reQuest - Çàïðîñ òðàíçàêöèè
/* 3 */ _T("TAR"),  //ScC_TAR,      TransAction Reply   - Îòâåò íà çàïðîñ òðàí
/* 4 */ _T("TBL"),  //ScC_TBL,      Transact log BLock  - Áëîê òðàíçàêöèè
/* 5 */ _T("TRA"),  //ScC_TRA,      execute TRansAction - Âûïîëíèòü òðàíçàêöèþ
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
  // Ïîäêîìàíäà îïðåäåëåíà, ñôîðìèðîâòü îïèñàòåëü
  switch (tCmd._eCode) {
case ScC_ATR:   //Add in Transaction Register
    // Íîìåð (èíäåêñ) óçëà
    tCmd.SetNode(mbMess.Message() + CMDFIELDSIZ);
    // Êîëè÷åñòâî áëîêîâ
    tCmd.SetBlockNo(mbMess.Message() + CMDFIELDSIZ+SHORTNUMSIZ);
    break;
case ScC_GTB:   //Get Transaction Register Block
    // Íîìåð áëîêà îò êîíöà öåïè
    tCmd.SetBlockNo(mbMess.Message() + CMDFIELDSIZ);
    break;
case ScC_TAQ:   //TransAction reQuest
case ScC_TAR:   //TransAction Reply
case ScC_TRA:   //execute TRansAction
    // Íîìåð òðàíçàêöèè
    tCmd.SetTransNo(mbMess.Message() + TRANSNUMOFF);
    // Õåø-êîä îòïðàâèòåëÿ
    tCmd.SetHashFrom(mbMess.MessBuffer() + HASHFROMOFF*sizeof(TCHAR));
    // Õåø-êîä ïîëó÷àòåëÿ
    tCmd.SetHashTo(mbMess.MessBuffer() + HASHTO_OFFS*sizeof(TCHAR));
    // Ñóììà òðàíçàêöèè
    tCmd.SetAmount(mbMess.Message() + AMOUNT_OFFS);
    break;
case ScC_TBL:   //Transact register BLock
    // Íîìåð òðàíçàêöèè
    tCmd.SetTransNo(mbMess.Message() + TRANSNUMOFF);
    // Õåø-êîä îòïðàâèòåëÿ
    tCmd.SetHashFrom(mbMess.MessBuffer() + HASHFROMOFF*sizeof(TCHAR));
    // Õåø-êîä ïîëó÷àòåëÿ
    tCmd.SetHashTo(mbMess.MessBuffer() + HASHTO_OFFS*sizeof(TCHAR));
    // Ñóììà òðàíçàêöèè
    tCmd.SetAmount(mbMess.Message() + AMOUNT_OFFS);
    // Õåø-êîä ïðåäûäóùåãî áëîêà òðàíçàêöèè
    tCmd.SetHashPrev(mbMess.MessBuffer() + HASHPREVOFF*sizeof(TCHAR));
    break;
  } //switch
  return tCmd._eCode;   //ïîäêîìàíäà íå îïðåäåëåíà
} // CBlockChain::ParseMessage()

// Ïîìåñòèòü îòâåò â òàáëèöó ãîëîñîâàíèÿ
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
  sNode.Format(_T("Óçåë #%d"), iNodeFrom+1);
  pszFields[0] = sNode.GetBuffer();     //íîìåð óçëà
  UserRegister()->UserName(m_tVoteRes[iNodeFrom]._iUser, szOwner);
  pszFields[1] = szOwner;               //èìÿ âëàäåëüöà óçëà
  pszFields[2] = tCmd._szTrans;         //íîìåð òðàíçàêöèè
  UserRegister()->UserName(m_tVoteRes[iNodeFrom]._iUserFrom, szUserFrom);
  pszFields[3] = szUserFrom;            //èìÿ ïîëüçîâàòåëÿ-îòïðàâèòåëÿ
  UserRegister()->UserName(m_tVoteRes[iNodeFrom]._iUserTo, szUserTo);
  pszFields[4] = szUserTo;              //èìÿ ïîëüçîâàòåëÿ-ïîëó÷àòåëÿ
  sAmount.Format(_T("%.2lf"), m_tVoteRes[iNodeFrom]._rSum);
  pszFields[5] = sAmount.GetBuffer();   //ñóììà
  m_pMainWin->AddInVotingTable(pszFields);
  m_nVote++;    //ïîäñ÷¸ò ïðîãîëîñîâàâøèõ óçëîâ
  if (!IsTheVoteOver())  return b;  //åù¸ íå âñå ïðîãîëîñîâàëè
  // Ïðîãîëîñîâàëè âñå - ïðîàíàëèçèðîâàòü ðåçóëüòàòû ãîëîñîâàíèÿ
  b = AnalyzeVoting();  //îïð àêò íîìåð òðàíçàêöèè è ñïèñîê àêò óçëîâ
  if (!b) {
    // Ëîêàëüíûé óçåë íåàêòóàëåí - äîïîëíèòü ðåãèñòð òðàíçàêöèé ëîêàëüíîãî 
    //óçëà íåäîñòàþùèìè áëîêàìè òðàíçàêöèé
    m_pMainWin->ApproveTransaction(1);      //îòîáðàçèòü îòêëîíåíèå òðàíçàêöèè
    m_iSrcNode = m_iActNodes[m_iActNode];   //óçåë-èñòî÷íèê äëÿ àêòóàëèçàöèè
    NextActualNode();   //ê ñëåäóþùåìó àêòóàëüíîìó óçëó
    m_nAbsentBlks = m_nActTrans - m_tVoteRes[m_paNode->m_iOwnNode]._nTrans;
    b = AskTransactionBlock();  //íà÷àòü àêòóàëèçàöèþ ëîêàëüíîãî óçëà
    return b;
  }
  // Ëîêàëüíûé óçåë àêòóàëåí - ìîæíî ñðàçó ïðîâåñòè òðàíçàêöèþ
  b = ExecuteTransactionOnLocalNode(m_mbTRA);
    /* Âçÿòü "ñâîþ" ñòðîêó â òàáëèöå ãîëîñîâàíèÿ  TVotingRes
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

// Çàïðîñèòü áëîê òðàíçàêöèè äëÿ àêòóàëèçàöèè ðåãèñòðà òðàíçàêöèé
bool  CBlockChain::AskTransactionBlock()
{
  ASSERT(m_nAbsentBlks > 0);
  // Ñôîðìèðîâàòü ñîîáùåíèå GTB ...
  TMessageBuf mbGTB;  TCHAR szMess[CHARBUFSIZE];
  swprintf_s(szMess, CHARBUFSIZE, _T("GTB %02d"), m_rTransacts.GetNewTransactOrd());
  mbGTB.PutMessage(szMess);     //çàíåñòè â áóôåð
  // ... ïîñëàòü çàäàííîìó óçëó
  RequestNoReply(m_iSrcNode, mbGTB);
  m_nAbsentBlks--;
  return true;
} // CBlockChain::AskTransactionBlock()

// Îáðàáîòàòü ñîîáùåíèå GTB "Äàòü áëîê ðåãèñòðà òðàíçàêöèé".
//Â îòâåò ôîðìèðóåòñÿ ñîîáùåíèå TBL.
bool  CBlockChain::GetTransactionBlock(WORD nBlock, TMessageBuf &mbGTBL)
//nBlock[in]  - íîìåð çàïðàøèâàåìîãî áëîêà (1,2,..)
//mbGTBL[out] - áóôåð ñîîáùåíèÿ: íà âõîäå GTB, íà âûõîäå TBL
{
  TCHAR szMess[CHARBUFSIZE];    //áóôåð ñîîáùåíèÿ (TCHAR=wchar_t)
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

// Îáðàáîòàòü ñîîáùåíèå TAQ "Çàïðîñ òðàíçàêöèè".
//Â îòâåò ôîðìèðóåòñÿ ñîîáùåíèå TAR.
bool  CBlockChain::TransActionreQuest(TSubcmd &tCmd, TMessageBuf &mbTAQR)
//tCmd[out]      - ðåçóëüòàò ðàçáîðà, îïèñàòåëü ñîîáùåíèÿ TAQ
//mbTAQR[in/out] - áóôåð ñîîáùåíèÿ: íà âõîäå TAQ, íà âûõîäå TAR
{
  LPTSTR pszMsg = mbTAQR.Message();  DWORD n;
  swscanf_s((LPTSTR)tCmd._chHashFrom, _T("%02d"), &n);
  WORD nTrans = m_rTransacts.GetNewTransactOrd(), iUserFrom = (WORD)n;
  TBalance *ptBalnc = m_rBalances.GetAt(iUserFrom);
  double rAmount = ptBalnc->GetBalance();
  TCHAR chMess[CHARBUFSIZE];
  swprintf_s(chMess, CHARBUFSIZE, _T("TAR %08d%015.2f"), nTrans, rAmount);
  n = CMDFIELDSIZ + LONGNUMSIZE;    //ñìåùåíèå çíà÷åíèÿ rAmount â chMess
  memcpy(mbTAQR.Message(), chMess, sizeof(TCHAR)*n);
  memcpy(mbTAQR.Message() + AMOUNT_OFFS,
         chMess + n, sizeof(TCHAR)*NAMESBUFSIZ);
  return true;
} // CBlockChain::TransActionreQuest()

// Äîáàâèòü â ðåãèñòð òðàíçàêöèé íîâûé áëîê (ïðè àêòóàëèçàöèè).
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
    // Âñòàâèòü õåø-êîä ïîñëåäíåé òðàíçàêöèè
    CTransRegister::SymbolsToHash((char *)(tCmd._chHashPrev),
                                  tRansact._chPrevHash);
    tRansact.SetAmount(rAmount);
    m_rTransacts.Add(&tRansact);    //äîáàâèòü â ðåãèñòð òðàíçàêöèé
    m_pMainWin->AddInTransactTable(&tRansact);  //îòîáðàçèòü
    // Ñêîððåêòèðîâàòü áàëàíñû ïîëüçîâàòåëåé
    CorrectBalance(iUserFrom, -rAmount);
    CorrectBalance(iUserTo, rAmount);
  }
  else {
    // Îøèáêà - íàðóøåíèå ïîñëåäîâàòåëüíîñòè áëîêîâ õåø-÷åéí
    ASSERT(FALSE);
  }
  return b;
} // CBlockChain::AddTransactionBlock()

// Ñêîððåêòèðîâàòü òåêóùèé îñòàòîê ñ÷¸òà ïîëüçîâàòåëÿ íà çàäàííóþ ñóììó
void  CBlockChain::CorrectBalance(WORD iUser, double rTurnover)
//iUser     - èíäåêñ ïîëüçîâàòåëÿ
//rTurnover - ñóììà îáîðîòà (+/-)
{
  TBalance *ptBalnc = m_rBalances.GetAt(iUser);
  double rSum = ptBalnc->GetBalance();  //òåêóùèé îñòàòîê
  rSum += rTurnover;
  ptBalnc->SetBalance(rSum);  m_rBalances.SetChanged();
  // Åñëè ñâîé áàëàíñ - îòîáðàçèòü
  if (iUser == m_paNode->m_iOwner)  m_pMainWin->ShowBalance(rSum);
} // CBlockChain::CorrectBalance()

// Ïîëíîñòüþ ïåðåñ÷èòàòü òåêóùèé îñòàòîê ñ÷¸òà çàäàííîãî ïîëüçîâàòåëÿ
void  CBlockChain::CalculateBalance(WORD iUser)
{
  double rSum = 1000.0, rTurnover;
  WORD iUserFrom, iUserTo, n = m_rTransacts.ItemCount();
  TTransact *ptRansact;
  // Ïðîñìîòð âñåãî ðåãèñòðà òðàíçàêöèé
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
  // Åñëè ñâîé áàëàíñ - îòîáðàçèòü
  if (iUser == m_paNode->m_iOwner)  m_pMainWin->ShowBalance(rSum);
} // CBlockChain::CalculateBalance()

// Íàéòè ìàêñèìàëüíûé íîìåð òðàíçàêöèè â òàáëèöå ãîëîñîâàíèÿ
WORD  CBlockChain::GetActualTransactOrd()
{
  WORD iAct = 0;
  for (WORD i = 1; i < m_nVote; i++)
    if (m_tVoteRes[i]._nTrans > m_tVoteRes[iAct]._nTrans) {
      iAct = i;
    }
  return m_tVoteRes[iAct]._nTrans;
} // CBlockChain::GetActualTransactOrd()

// Ïîñòðîèòü ñïèñîê àêòóàëüíûõ óçëîâ
void  CBlockChain::BuildActualNodeList(WORD nTransAct)
{
  WORD i;
  for (m_nActNodes = 0, i = 0; i < m_nVote; i++)
    if (m_tVoteRes[i]._nTrans == nTransAct) {
      m_iActNodes[m_nActNodes++] = i;
    }
  m_iActNode = 0;   //íà ïåðâûé ýëåìåíò ñïèñêà "àêòóàëüíûõ" óçëîâ
} // CBlockChain::BuildActualNodeList()

// Ïåðåéòè ê ñëåäóþùåìó àêòóàëüíîìó óçëó â ñïèñêå (öèêëè÷åñêèé ïåðåáîð)
void  CBlockChain::NextActualNode()
{
  m_iActNode++;
  if (m_iActNode == m_nActNodes)  m_iActNode = 0;
} // CBlockChain::NextActualNode()

// End of class CBalanceRegister definition ----------------------------
