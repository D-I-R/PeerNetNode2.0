// PeerNetDlg.h                                            (c) DIR, 2019
//
//              Объявление класса CPeerNetDialog
//
////////////////////////////////////////////////////////////////////////

#pragma once

#include "afxwin.h"
#include "afxcmn.h"
#include "resource.h"
#include "ListVwEx.h"   //#include <afxcview.h>
#include "PeerNetNode.h"
#include "BlkChain.h"

#define  MVITEMSNUM    6    //количество перемещаемых элементов диалога
#define  ITEMSNUMBR    35   //количество элементов диалога на вкладках

// Структурный тип "Размерные параметры элемента диалога"
// (в координатах клиентской области родительского окна)
struct TPlacement {
  POINT  tPoint;    //координаты левого верхнего угла окна элемента
  SIZE   tSize;     //размер окна элемента
  void  Init(int x, int y, int cx, int cy) {
    tPoint.x = x, tPoint.y = y;     //координаты левого верхнего угла окна
    tSize.cx = cx, tSize.cy = cy;   //размер окна
  }
};

// Структурный тип "Описатель столбца списка ListView"
struct TColumnDsc {
  LPTSTR _pszName;  //название столбца
  short  _nWidth;   //ширина столбца
  short  _nFormt;   //формат столбца
};

// Объявление класса CPeerNetDialog dialog =================================
//
class CPeerNetDialog : public CDialogEx
{
  // Данные диалогового окна
//#ifdef AFX_DESIGN_TIME
  enum { IDD = IDD_PEERNETNODE_DIALOG };
//#endif
  static UINT  m_iTemsMv[MVITEMSNUM];   //IDs перемещаемых элементов
  static int   m_iBase[3][2];   //описатель размещ элем диалога на вкладках
  static UINT  m_iTemIDs[ITEMSNUMBR];   //элементы диалога по вкладкам
  static TColumnDsc  m_tVoteLstDsc[VOTECOLNUM]; //описатель таблицы голосования
  static TColumnDsc  m_tTranLstDsc[TRANCOLNUM]; //описатель таблицы транзакций
public:
  CPeerNetNode  m_oNode;        //узел одноранговой сети
  WORD          m_iOwnNode;     //номер своего узла
  CBlockChain   m_oBlkChain;    //реализация функционала блок-чейн
protected:
  HICON        m_hIcon;
  CSize        m_DlgSizeMin;    //наименьший размер окна диалога
  CSize        m_DlgSize;       //текущий размер клиентской области диалога
  TPlacement   m_tItems[MVITEMSNUM]; //размерные параметры перемещ элем диалога
  CTabCtrl     m_tbcPages;      //набор вкладок
  int          m_iPage;         //индекс выбранной вкладки
  CString      m_sUserName;     //условное имя пользователя
  CString      m_sBalance;      //сумма текущего баланса
  // Переменные элементов на вкладках:
  // Вкладка 0
  CComboBox    m_cmbLogin;      //комбобокс для выбора логина
  CComboBox    m_cmbPasswd;     //комбобокс для выбора пароля
  // Вкладка 1
  CString      m_sTransNo;      //номер новой транзакции
  CComboBox    m_cmbUserTo;     //комбобокс для выбора пользователя-получателя
  CString      m_sAmount;       //сумма транзакции
  CListViewEx  m_lsvVoting;     //таблица голосования
  WORD         m_nVote;         //текущее колич строк в таблице голосования
  CStatic      m_lblVoteRes1;   //надпись "Одобрена"
  CStatic      m_lblVoteRes0;   //надпись "Отклонена"
  CListViewEx  m_lsvTransacts;  //список проведённых транзакций
  // Вкладка 2
  CListBox     m_lstNodes;      //список других узлов сети
  CString      m_sRequest;      //текст запроса
  CString      m_sReply;        //текст ответа
  CListBox     m_lstExchange;   //журнал обмена данного узла
  CListBox     m_lstTrace;      //трассировка выполнения

  // Создание
public:
  CPeerNetDialog(CWnd *pParent = NULL);	// стандартный конструктор

  // Добавить строку в таблицу голосования
  void  AddInVotingTable(LPTSTR pszValues[], bool bClear = false)
  { AddRowInListView(&m_lsvVoting, VOTECOLNUM, pszValues, bClear); }
  // Добавить строку в таблицу транзакций
  void  AddInTransactTable(TTransact *ptRansact);
  // Добавить новый узел сети
  void  AddRemoteNode(WORD iNode, WORD iUser);
  // Удалить остановленный узел сети
  void  DeleteRemoteNode(WORD iNode);
  // Показать номер своего узла
  void  ShowOwnNodeNumber(WORD wOwnNode);
  // Показать ответ на отправленное сообщение
  void  ShowReply(WORD iNode, LPTSTR lpszReply);
  // Показать текущий остаток счёта
  void  ShowBalance(double rSum);
  // Отобразить одобрение или отклонение транзакции
  void  ApproveTransaction(WORD iReg = 2);
  // Перегруженный метод. Вывести сообщение в список трассировки
  void  ShowMessage(LPTSTR lpszMesg);
  void  ShowMessage(CString &sMesg);
#ifdef _TRACE
  void  ShowTrace(LPTSTR lpszTrace);
#endif

protected:
  virtual void  DoDataExchange(CDataExchange *pDX);	//поддержка DDX/DDV
  virtual BOOL  PreTranslateMessage(MSG *pMsg);

  // Реализация
private:
  void  StoreItemPlacement(int iTem);
  void  MoveElement(int iTem, TPlacement &d);
  // Скрыть или показать элементы диалога на заданной вкладке
  void  HideOrShowItems(int iPage, int nCmdShow);
  // Инициализировать список ListView
  bool  InitializeListView(CListViewEx *pLsv,
                           int nCols, TColumnDsc tLstDsc[]);
  // Очистить список ListView
  void  ClearListView(CListViewEx *pLsv);
  // Добавить строку в список ListView
  void  AddRowInListView(CListViewEx *pLsv, int nCols,
                         LPTSTR pszValues[], bool bClear = false);
  // Очистить таблицу голосования
  void  ClearVotingTable() { ClearListView(&m_lsvVoting); }
  // Заполнить таблицу транзакций
  void  FillTransactTable();

  // Созданные функции схемы сообщений
protected:
  virtual BOOL     OnInitDialog();
  virtual BOOL     OnCommand(WPARAM wParam, LPARAM lParam);
  virtual BOOL     OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam,
                            LRESULT *pResult);
private:
  afx_msg void     OnSysCommand(UINT nID, LPARAM lParam);
  afx_msg void     OnPaint();
  afx_msg HCURSOR  OnQueryDragIcon();
  afx_msg void     OnGetMinMaxInfo(MINMAXINFO *lpMMI);
  afx_msg void     OnSize(UINT nType, int cx, int cy);
  afx_msg void     OnTcnSelchangePages(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void     OnBnClickedStartBtn();
  afx_msg void     OnBnClickedStopBtn();
  afx_msg void     OnBnClickedRequestBtn();
  afx_msg void     OnBnClickedExecuteBtn();
  afx_msg void     OnBnClickedSendBtn();
  afx_msg void     OnBnClickedDumpBtn();

  DECLARE_MESSAGE_MAP()

};
