// BlkChain.h                                              (c) DIR, 2019
//
//              Объявление классов модели блокчейн
//
// CBlockChain - Модель блокчейн
//
////////////////////////////////////////////////////////////////////////

#pragma once

//#include "afxtempl.h"
#include "FileBase.h"

// Параметры макета сообщения. Смещения заданы в символах TCHAR.
#define  CMDFIELDSIZ  4     //размер поля команды
#define  LONGNUMSIZE  8     //размер номера транзакции
#define  SHORTNUMSIZ  2     //размер поля номера или количества блоков
#define  AMOUNT_SIZE  NAMESBUFSIZ   //размер поля сумма, опр в FileBase.h
#define  TRANSNUMOFF  CMDFIELDSIZ   //смещение полей TransNo, Node
#define  HASHFROMOFF  (TRANSNUMOFF+LONGNUMSIZE) //смещение поля HashFrom
// _HASHSIZE определено в FileBase.h
#ifdef DBGHASHSIZE  //определено в начале FileBase.h
 #define  HASHTO_OFFS  (HASHFROMOFF+_HASHSIZE/sizeof(TCHAR)-1) //смещение поля HashTo
 #define  AMOUNT_OFFS  (HASHTO_OFFS+_HASHSIZE/sizeof(TCHAR)-1) //смещение поля Amount
#else
 #define  HASHTO_OFFS  (HASHFROMOFF+_HASHSIZE) //смещение поля HashTo
 #define  AMOUNT_OFFS  (HASHTO_OFFS+_HASHSIZE) //смещение поля Amount
#endif
#define  HASHPREVOFF  (AMOUNT_OFFS+AMOUNT_SIZE) //смещение поля HashPrev

#define  VOTECOLNUM   6     //количество столбцов в таблице голосования
#define  TRANCOLNUM   4     //количество столбцов в таблице транзакций
// Имя контейнера ключей шифрования (UNICODE):
#define  KEYCONTNAME  TEXT("Peer Net Key Container")

// Идентификаторы (коды) подкоманд команды DTB
enum ESubcmdCode {
  ScC_ATR = 11, //Add in Transaction Log - Нужно добавить в журнал транзакций
  ScC_GTB,      //Get Transaction log Block - Дать блок журнала транзакций
  ScC_TAQ,      //TransAction reQuest    - Запрос транзакции
  ScC_TAR,      //TransAction Reply      - Ответ на запрос транзакции
  ScC_TBL,      //Transaction log BLock  - Блок транзакции
  ScC_TRA,      //execute TRansAction    - Выполнить транзакцию
};
// Структурный тип "Описатель сообщения (подкоманды)"
struct TSubcmd {
  ESubcmdCode  _eCode;              //код подкоманды
  TCHAR  _szNode[SHORTNUMSIZ+1];    //номер узла (2)
  TCHAR  _szBlock[SHORTNUMSIZ+1];   //номер или количество блоков (2)
  TCHAR  _szTrans[LONGNUMSIZE+1];   //номер транзакции (8)
  BYTE   _chHashFrom[_HASHSIZE];    //хеш-код рег имени польз-отправителя
  BYTE   _chHashTo[_HASHSIZE];      //хеш-код рег имени польз-получателя
  TCHAR  _szAmount[NAMESBUFSIZ];    //+1 сумма транзакции (или текущ остатка)
  BYTE   _chHashPrev[2*HASHSIZE+2]; //хеш-код предыдущего блока в симв предст
  // Обнуление структуры
  void  Clear() {
    memset(&_eCode, 0, sizeof(TSubcmd));
  }
  // Поле номер узла (2 симв)
  void  SetNode(LPTSTR pszNode) {
    memcpy(_szNode, pszNode, SHORTNUMSIZ * sizeof(TCHAR));
    _szNode[SHORTNUMSIZ] = _T('\0');
  }
  WORD  GetNode() {
    DWORD n;
    swscanf_s(_szNode, _T("%02d"), &n);
    return (WORD)n;
  }
  // Поле номер или количество блоков (2 симв)
  void  SetBlockNo(LPTSTR pszNo) {
    memcpy(_szBlock, pszNo, SHORTNUMSIZ * sizeof(TCHAR));
    _szBlock[SHORTNUMSIZ] = _T('\0');
  }
  WORD  GetBlockNo() {
    DWORD n;
    swscanf_s(_szBlock, _T("%02d"), &n);
    return (WORD)n;
  }
  // Поле номер транзакции (8 симв)
  void  SetTransNo(LPTSTR pszNo) {
    memcpy(_szTrans, pszNo, LONGNUMSIZE * sizeof(TCHAR));
    _szTrans[LONGNUMSIZE] = _T('\0');
  }
  WORD  GetTransNo() {
    DWORD n;
    swscanf_s(_szTrans, _T("%08d"), &n);
    return (WORD)n;
  }
  // Поле хеш-код регистрационного имени (128 байт) (или индекс (2 симв))
  //пользователя-отправителя
  void  SetHashFrom(BYTE *cHash) {
#ifdef DBGHASHSIZE
    memcpy(_chHashFrom, cHash, _HASHSIZE-2);  //без завершающего _T('\0')
    memset(_chHashFrom + _HASHSIZE-2, 0, 2);  //добавление завершающ _T('\0')
#else
    memcpy(_chHashFrom, cHash, _HASHSIZE);
#endif
  }
  // Поле хеш-код регистрационного имени (128 байт) (или индекс (2 симв))
  //пользователя-получателя
  void  SetHashTo(BYTE *cHash) {
#ifdef DBGHASHSIZE
    memcpy(_chHashTo, cHash, _HASHSIZE-2);
    memset(_chHashTo + _HASHSIZE-2, 0, 2);
#else
    memcpy(_chHashTo, cHash, _HASHSIZE);
#endif
  }
  // Сумма транзакции (или текущий остаток) (15+1 симв)
  void  SetAmount(LPTSTR pszSum) {
    memcpy(_szAmount, pszSum, (NAMESBUFSIZ-1)*sizeof(TCHAR));
    _szAmount[NAMESBUFSIZ-1] = _T('\0');
  }
  double  GetAmount() {
    double rSum;
    swscanf_s(_szAmount, _T("%lf"), &rSum);
    return rSum;
  }
  // Поле хеш-код в симв представлении предыдущ блока транзакции (256+1 байт)
  void  SetHashPrev(BYTE *cHash) {
    memcpy(_chHashPrev, cHash, 2*HASHSIZE);
    memset(_chHashPrev + 2*HASHSIZE, 0, 2);
  }
}; // struct TSubcmd

// Структурный тип "Запись таблицы результатов голосования"
struct TVotingRes {
  WORD    _iUser;       //владелец узла (индекс в реестре)
  WORD    _nTrans;      //номер новой транзакции
  WORD    _iUserFrom;   //пользователь "кто" (индекс в реестре)
  WORD    _iUserTo;     //пользователь "кому" (индекс в реестре)
  double  _rSum;        //сумма транзакции или текущий остаток счёта
  // Инициализировать структуру
  void  Init(WORD iUser) {
    memset(&_iUser, 0, sizeof(TVotingRes));
    _iUser = iUser;
  }
};

class CPeerNetDialog;
class CPeerNetNode;

// Объявление класса CBlockChain - Модель блокчейн
////////////////////////////////////////////////////////////////////////

class CBlockChain
{
  CPeerNetDialog  * m_pMainWin;     //главное окно программы
  CPeerNetNode    * m_paNode;       //узел одноранговой сети
  CCrypto           m_aCrypt;       //объект криптографии
  CUserRegister     m_rUsers;       //регистр пользователей
  CBalanceRegister  m_rBalances;    //регистр текущих остатков
  CTransRegister    m_rTransacts;   //регистр транзакций
  // Таблица соответствия iNode->iUser, а также таблица голосования узлов:
  TVotingRes  m_tVoteRes[NODECOUNT];
  WORD        m_nVote;          //количество проголосовавших узлов
  WORD        m_iNodes[NODECOUNT];      //таблица соответствия iUser->iNode
  WORD        m_nActTrans;      //"актуальный" номер новой транзакции
  WORD        m_iActNodes[NODECOUNT];   //список "актуальных" узлов сети
  WORD        m_nActNodes;      //количество "актуальных" узлов в списке
  WORD        m_iActNode;       //текущий элемент списка "актуальных" узлов
  //bool         m_bTrans;        //флаг "Проводится собственная транзакция"
  TMessageBuf  m_mbTRA;         //сохраняемое сообщение TRA
  // Параметры актуализации регистра транзакций
  WORD         m_iSrcNode;      //узел-источник при актуализации
  //WORD         m_nTransBlock;   //номер очередного блока транзакции
  WORD         m_nAbsentBlks;   //количество недостающих блоков транзакций
public:
  // Конструктор и деструктор
  CBlockChain(CPeerNetDialog *pMainWin, CPeerNetNode *paNode);
  ~CBlockChain(void);
  // Внешние методы
  // -------------------------------------------------------------------
  // Дать указатель регистра пользователей
  CUserRegister    * UserRegister() { return &m_rUsers; }
  // Дать указатель регистра текущих остатков
  CBalanceRegister * BalanceRegister() { return &m_rBalances; }
  // Дать указатель регистра транзакций
  CTransRegister   * TransRegister() { return &m_rTransacts; }
  // Проверить регистрационные данные пользователя
  WORD  AuthorizationCheck(LPTSTR pszLogin, LPTSTR pszPasswd);
  WORD  AuthorizationCheck(CString sLogin, CString sPasswd);
  // Авторизовать пользователя
  bool  Authorize(WORD iUser);
  // Дать авторизованного пользователя (владельца узла)
  WORD  GetAuthorizedUser() { return m_rUsers.OwnerIndex(); }
  // Отменить авторизацию пользователя
  void  CancelAuthorization();
  // Связать пользователя с узлом сети
  void  TieUpUserAndNode(WORD iUser, WORD iNode);
  // Отвязать пользователя от узла сети
  void  UntieUserAndNode(WORD iNode);
  // Дать пользователя заданного узла
  WORD  GetNodeOwner(WORD iNode) { return m_tVoteRes[iNode]._iUser; }
  // Обработать сообщение DTB из очереди (полученное серверным каналом)
  bool  ProcessDTB();
  // Инициировать транзакцию
  bool  StartTransaction(WORD iUserTo, double rAmount);
  // Голосование закончено?
  bool  IsTheVoteOver() { return (m_nVote == m_paNode->m_nNodes); }
  // Проанализировать результаты голосования
  bool  AnalyzeVoting();
  // Сформировать сообщение TRA на основании информации из таблицы голосования
  void  CreateTRAMessage();
  // Провести одобренную транзакцию
  //void  ExecuteTransaction(); //TMessageBuf &mbTAR
  // Провести транзакцию на своём узле
  bool  ExecuteTransactionOnLocalNode(TMessageBuf &mbTRA);
  // Закрыть регистры БД
  void  CloseRegisters();
private:
  // Внутренние методы
  // -------------------------------------------------------------------
  // Инициализировать все реестры в памяти
  //bool  Initialize();
  // Сформировать подкоманду запроса или исполнения транзакции
  void  CreateTransactSubcommand(LPTSTR pszMsg, LPTSTR pszSbcmd,
                                 WORD nTrans, WORD iUserFrom,
                                 WORD iUserTo, double rAmount);
  // Послать сообщение прикладного уровня заданному узлу 
  //с упаковкой в блок данных, без получения ответа
  void  RequestNoReply(WORD iNodeTo, TMessageBuf &mbRequest);
  // Эмулировать ответ своего узла на сообщение прикладного уровня
  TMessageBuf * EmulateReply(TMessageBuf &mbRequest);
  // Разобрать сообщение. Метод возвращает код подкоманды и описатель сообщ.
  ESubcmdCode  ParseMessage(TMessageBuf &mbMess, TSubcmd &tCmd);
  // Поместить ответ в таблицу голосования
  bool  AddInVotingTable(WORD iNodeFrom, TMessageBuf *pmbReply);
  // Актуализировать свой регистр транзакций
  bool  AskTransactionBlock(); //WORD iNodeFrom, WORD nBlocks
  // Обработать сообщение GTB "Дать блок регистра транзакций".
  //В ответ формируется сообщение TBL.
  bool  GetTransactionBlock(WORD nBlock, TMessageBuf &mbGTBL);
  // Обработать сообщение TAQ "Запрос транзакции".
  //В ответ формируется сообщение TAR.
  bool  TransActionreQuest(TSubcmd &tCmd, TMessageBuf &mbTAQR);
  // Добавить в регистр транзакций новый блок (при актуализации).
  bool  AddTransactionBlock(TMessageBuf &mbTBL);
  // Скорректировать текущий остаток счёта на заданную сумму
  void  CorrectBalance(WORD iUser, double rAmount);
  // Полностью пересчитать текущий остаток счёта
  void  CalculateBalance(WORD iUser);
  // Найти максимальный номер транзакции в таблице голосования
  WORD  GetActualTransactOrd();
  // Построить список актуальных узлов
  void  BuildActualNodeList(WORD nTransAct);
  // Перейти к следующему актуальному узлу в списке (циклический перебор)
  void  NextActualNode();
};

// End of class CBlockChain declaration --------------------------------
