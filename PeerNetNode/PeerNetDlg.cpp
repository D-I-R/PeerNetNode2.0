// PeerNetDlg.cpp                                          (c) DIR, 2019
//
//              Реализация класса CPeerNetDialog
//
////////////////////////////////////////////////////////////////////////

//#include "stdafx.h"
#include "pch.h"
//#include "framework.h"
#include "PeerNetMain.h"
//#include "PNN_Main.h"
#include "PeerNetDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
 #define new DEBUG_NEW
#endif

// Диалоговое окно CAboutDlg ===========================================
// используется для описания сведений о приложении

class CAboutDlg : public CDialogEx
{
  // Данные диалогового окна
#ifdef AFX_DESIGN_TIME
  enum { IDD = IDD_ABOUTBOX };
#endif

public:
  CAboutDlg();

protected:
  virtual void  DoDataExchange(CDataExchange *pDX); //поддержка DDX/DDV

  // Реализация

  DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void  CAboutDlg::DoDataExchange(CDataExchange *pDX)
{
  CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// Реализация класса "Диалоговое окно CPeerNetDialog"
//======================================================================
// Инициализация статических элементов данных
//----------------------------------------------------------------------
//static
// Идентификаторы перемещаемых элементов диалога
UINT  CPeerNetDialog::m_iTemsMv[MVITEMSNUM] = {
  IDC_PAGES_TAB, IDC_TRANS_LSV, IDC_EXCHANGE_LST, IDC_TRACE_LST,
  IDOK,          ID_HELP
};
//static
// Описатель размещ элем диалога на вкладках
int  CPeerNetDialog::m_iBase[3][2] = { { 0, 7 }, { 7, 16 }, { 23, 12 } };
//static
// Элементы диалога по вкладкам
UINT  CPeerNetDialog::m_iTemIDs[ITEMSNUMBR] = {
/* 0 */ IDC_STARTSTOP_LBL, IDC_LOGIN_LBL,     IDC_LOGIN_CMB,  IDC_LOGIN_BTN,
        IDC_PASSWD_LBL,    IDC_PASSWD_CMB,    IDC_LOGOUT_BTN,
/* 1 */ IDC_NEWTRANS_LBL,  IDC_TRANSNO_LBL,   IDC_USERTO_LBL, IDC_AMOUNT_LBL,
        IDC_TRANSNO_TXB,   IDC_USERTO_CMB,    IDC_AMOUNT_TXB, IDC_REQUEST_BTN,
        IDC_VOTING_LBL,    IDC_VOTINGRES_LBL, IDC_VOTING_LSV, IDC_VOTRES1_LBL,
        IDC_VOTRES0_LBL,   IDC_EXECUTE_BTN,   IDC_TRANS_LBL,  IDC_TRANS_LSV,
/* 2 */ IDC_REQUEST_LBL,   IDC_REQUEST_EDT,   IDC_SEND_BTN,   IDC_DUMP_BTN,
        IDC_REPLY_LBL,     IDC_REPLY_EDT,     IDC_NODES_LBL,  IDC_NODES_LST,
        IDC_EXCHANGE_LBL,  IDC_EXCHANGE_LST,  IDC_TRACE_LBL,  IDC_TRACE_LST,
};
//static
// Описатель таблицы голосования
TColumnDsc  CPeerNetDialog::m_tVoteLstDsc[VOTECOLNUM] = {
//           название      ширина   формат
/* 0 */ { _T("Узел"),         60, LVCFMT_CENTER },
/* 1 */ { _T("Пользователь"), 90, LVCFMT_CENTER },
/* 2 */ { _T("#Транзакции"),  85, LVCFMT_CENTER },
/* 3 */ { _T("Кто"),          70, LVCFMT_CENTER },
/* 4 */ { _T("Кому"),         70, LVCFMT_CENTER },
/* 5 */ { _T("Сколько"),      80, LVCFMT_RIGHT  },
};
//static
// Описатель таблицы транзакций
TColumnDsc  CPeerNetDialog::m_tTranLstDsc[TRANCOLNUM] = {
/* 0 */ { _T("#Транзакции"), 85, LVCFMT_CENTER },
/* 1 */ { _T("Кто"),         70, LVCFMT_CENTER },
/* 2 */ { _T("Кому"),        70, LVCFMT_CENTER },
/* 3 */ { _T("Сколько"),     80, LVCFMT_RIGHT  },
};

// Конструктор
CPeerNetDialog::CPeerNetDialog(CWnd *pParent/* = nullptr*/)
          : CDialogEx(IDD_PEERNETNODE_DIALOG, pParent),
            m_oNode(this, &m_oBlkChain), m_oBlkChain(this, &m_oNode),
            m_sUserName(_T("")), m_sBalance(_T("")),
            m_sTransNo(_T("")), m_sAmount(_T("")),
            m_sRequest(_T("")), m_sReply(_T(""))
{
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
  memset(m_tItems, 0, sizeof(m_tItems));
  m_iPage = -1;     //ни одна вкладка не выбрана
  m_nVote = 0;
}

// Добавить строку в таблицу транзакций
void  CPeerNetDialog::AddInTransactTable(TTransact *ptRansact)
{
/*
  BYTE  _chPrevHash[HASHSIZE];      //0x0000 хеш-код предыдущего блока
  char  _szTrnsOrd[NAMESBUFSIZ];    //0x0080 порядковый номер транзакции
  BYTE  _chHashSrc[HASHSIZE];       //0x0090 хеш-код рег имени "кто"
  BYTE  _chHashDst[HASHSIZE];       //0x0110 хеш-код рег имени "кому"
  char  _szAmount[NAMESBUFSIZ];     //0x0190 сумма транзакции "сколько"
*/
  CString sTransNo, sAmount;  WORD iUser;
  TCHAR szUserFrom[NAMESBUFSIZ], szUserTo[NAMESBUFSIZ],
        *pszFields[TRANCOLNUM];
  CUserRegister *prUser = m_oBlkChain.UserRegister();
  sTransNo.Format(_T("%d"), ptRansact->GetTransOrd());
  pszFields[0] = sTransNo.GetBuffer();  //номер транзакции
  iUser = prUser->GetUserByLoginHash(ptRansact->_chHashSrc); //iUserFrom
  prUser->UserName(iUser, szUserFrom);
  pszFields[1] = szUserFrom;            //имя пользователя-отправителя
  iUser = prUser->GetUserByLoginHash(ptRansact->_chHashDst); //iUserTo
  prUser->UserName(iUser, szUserTo);
  pszFields[2] = szUserTo;              //имя пользователя-получателя
  sAmount.Format(_T("%.2lf"), ptRansact->GetAmount());
  pszFields[3] = sAmount.GetBuffer();   //сумма
  AddRowInListView(&m_lsvTransacts, TRANCOLNUM, pszFields);
  CTransRegister *prTrans = m_oBlkChain.TransRegister();
  m_sTransNo.Format(_T("%d"), prTrans->ItemCount()+1);
  UpdateData(FALSE);    //передача Variables -> Controls
} // CPeerNetDialog::AddInTransactTable()

// Добавить новый узел сети
void  CPeerNetDialog::AddRemoteNode(WORD iNode, WORD iUser)
{
  TCHAR szMessBuf[ERMES_BUFSIZE];
  // Сформировать в буфере обозначение узла
  swprintf_s(szMessBuf, ERMES_BUFSIZE, _T("Узел #%d"), iNode+1);
  m_lstNodes.AddString(szMessBuf);  //добавить в список узлов
  m_lstNodes.UpdateWindow();
  // Соотнести пользователя с узлом
  m_oBlkChain.TieUpUserAndNode(iUser, iNode);
  // Добавить в список пользователей-получателей
  TUser *ptUser = m_oBlkChain.UserRegister()->GetAt(iUser);
  CString s(ptUser->_sName);
  m_cmbUserTo.AddString(s); //.GetBuffer()
} // CPeerNetDialog::AddRemoteNode()

// Удалить остановленный узел сети
void  CPeerNetDialog::DeleteRemoteNode(WORD iNode)
{
  TCHAR szNode[ERMES_BUFSIZE];
  // Сформировать в буфере обозначение узла
  swprintf_s(szNode, ERMES_BUFSIZE, _T("Узел #%d"), iNode+1);
  // Найти строку списка узлов
  int i = m_lstNodes.FindString(-1, szNode); //искать с начала списка
  ASSERT(i != LB_ERR);
  // Удалить из списка строку с обозначением узла
  m_lstNodes.DeleteString((UINT)i);
  m_lstNodes.UpdateWindow();    //обновить экран
  // Удалить из ComboBox m_cmbUserTo пользователя-владельца узла
  WORD iUser = m_oBlkChain.GetNodeOwner(iNode);
  TUser *ptUser = m_oBlkChain.UserRegister()->GetAt(iUser);
  CString s(ptUser->_sName);    //имя пользователя
  // Найти строку списка c именем удаляемого пользователя
  int iSel = m_cmbUserTo.GetCurSel();   //индекс текущего выделения
  i = m_cmbUserTo.FindString(-1, s);    //искать с начала списка
  ASSERT(i != LB_ERR);
  // Удалить из списка строку
  m_cmbUserTo.DeleteString((UINT)i);
  // Очистить поле, если был выбран удаляемый пользователь
  if (i == iSel)  m_cmbUserTo.SetWindowTextW(_T(""));
  // Убрать пользователя из объекта m_oBlkChain
  m_oBlkChain.UntieUserAndNode(iNode);
} // CPeerNetDialog::DeleteRemoteNode()

// Показать номер собственного узла
void  CPeerNetDialog::ShowOwnNodeNumber(WORD iOwnNode)
{
  TCHAR szMessBuf[ERMES_BUFSIZE];
  m_iOwnNode = iOwnNode;
  // Сформировать в буфере обозначение узла
  swprintf_s(szMessBuf, ERMES_BUFSIZE, _T("Peer Net Node #%d"), iOwnNode+1);
  SetWindowText(szMessBuf);
} // CPeerNetDialog::ShowOwnNodeNumber()

// Показать полученный ответ узла
void  CPeerNetDialog::ShowReply(WORD iNode, LPTSTR lpszReply)
{
  m_sReply.Format(_T("Node #%d: %s"), iNode+1, lpszReply);
  m_lstExchange.AddString(m_sReply); //(LPCTSTR).GetBuffer()
  //UpdateData();
} // CPeerNetDialog::ShowReply()

// Показать текущий остаток счёта
void  CPeerNetDialog::ShowBalance(double rSum)
{
  m_sBalance.Format(_T("%.2lf"), rSum);
  UpdateData(FALSE);    //передача Variables -> Controls
} // CPeerNetDialog::ShowBalance()

// Отобразить одобрение или отклонение транзакции
void  CPeerNetDialog::ApproveTransaction(WORD iReg/* = 2*/)
{
  BOOL bReject = (iReg == 1) ? TRUE : FALSE,
       bApprove = (iReg == 2) ? TRUE : FALSE;
  m_lblVoteRes0.EnableWindow(bReject);  //"выключить" надпись "Отклонена"
  m_lblVoteRes1.EnableWindow(bApprove); //TRUE: "включить" надпись "Одобрена"
} // CPeerNetDialog::ApproveTransaction()

// Вывести сообщение в список трассировки
void  CPeerNetDialog::ShowMessage(LPTSTR lpszMesg)
{
  m_lstTrace.AddString(lpszMesg);
  m_lstTrace.UpdateWindow();
}
void  CPeerNetDialog::ShowMessage(CString &sMesg)
{
  ShowMessage(sMesg.GetBuffer());
} // CPeerNetDialog::ShowMessage()

#ifdef _TRACE
// Вывести трассировочное сообщение
void  CPeerNetDialog::ShowTrace(LPTSTR lpszTrace)
{
  m_lstTrace.AddString(lpszTrace);
  m_lstTrace.UpdateWindow();
} // CPeerNetDialog::ShowTrace()
#endif

//protected:
void  CPeerNetDialog::DoDataExchange(CDataExchange *pDX)
{
  CDialogEx::DoDataExchange(pDX);

  DDX_Text(pDX, IDC_USER_TXB, m_sUserName);
  DDX_Text(pDX, IDC_BALNC_TXB, m_sBalance);
  DDX_Control(pDX, IDC_PAGES_TAB, m_tbcPages);
  //if (m_iPage == 0) {
    DDX_Control(pDX, IDC_LOGIN_CMB, m_cmbLogin);
    DDX_Control(pDX, IDC_PASSWD_CMB, m_cmbPasswd);
  //}
  //else if (m_iPage == 1) {
    DDX_Text(pDX, IDC_TRANSNO_TXB, m_sTransNo);
    DDX_Control(pDX, IDC_USERTO_CMB, m_cmbUserTo);
    DDX_Text(pDX, IDC_AMOUNT_TXB, m_sAmount);
    // Список "Таблица голосования". При первом обращении здесь происходит
    //создание окна
    CListCtrl &lstCtrl1 = m_lsvVoting.GetListCtrl();
    DDX_Control(pDX, IDC_VOTING_LSV, lstCtrl1);
    DDX_Control(pDX, IDC_VOTRES1_LBL, m_lblVoteRes1);
    DDX_Control(pDX, IDC_VOTRES0_LBL, m_lblVoteRes0);
    // Список "Список проведённых транзакций". При первом обращении здесь
    //происходит создание окна
    CListCtrl &lstCtrl2 = m_lsvTransacts.GetListCtrl();
    DDX_Control(pDX, IDC_TRANS_LSV, lstCtrl2);
  //}
  //else if (m_iPage == 2) {
    DDX_Control(pDX, IDC_NODES_LST, m_lstNodes);
    DDX_Text(pDX, IDC_REQUEST_EDT, m_sRequest);
    DDX_Text(pDX, IDC_REPLY_EDT, m_sReply);
    DDX_Control(pDX, IDC_EXCHANGE_LST, m_lstExchange);
    DDX_Control(pDX, IDC_TRACE_LST, m_lstTrace);
  //}
} // CPeerNetDialog::DoDataExchange()

// Фильтрация нажатия некоторых клавиш
BOOL  CPeerNetDialog::PreTranslateMessage(MSG *pMsg)
{
  // TODO: добавьте специализированный код или вызов базового класса
  // Диалог не должен закрываться нажатием клавиш Enter или Esc
  if (pMsg->message == WM_KEYDOWN &&
      (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE))  return TRUE;

  return CDialogEx::PreTranslateMessage(pMsg);
} // CPeerNetDialog::PreTranslateMessage()

//private:
// Запомнить параметры расположения элемента диалога
void  CPeerNetDialog::StoreItemPlacement(int iTem)
//iTem - индекс перемещаемого элемента диалога
{
  ASSERT(0 <= iTem && iTem < MVITEMSNUM);
  CWnd *pWnd = GetDlgItem(m_iTemsMv[iTem]);   //размеры и координаты элемента
  RECT  rect;
  pWnd->GetWindowRect(&rect);
  m_tItems[iTem].tSize.cx = rect.right - rect.left,   //размеры элемента
  m_tItems[iTem].tSize.cy = -rect.top + rect.bottom;
  ScreenToClient(&rect);                      //локальные координаты элемента
  m_tItems[iTem].tPoint.x = rect.left, m_tItems[iTem].tPoint.y = rect.top;
} // CPeerNetDialog::StoreItemPlacement

// Переместить элемент на заданный вектор
void  CPeerNetDialog::MoveElement(int iTem, TPlacement &d)
//iTem - индекс перемещаемого элемента диалога
//d    - "вектор" смещения
{
  ASSERT(0 <= iTem && iTem < MVITEMSNUM);
  CWnd *pItemWnd = GetDlgItem(m_iTemsMv[iTem]);     //окно элемента
  if (pItemWnd == NULL)  return;    //защита
  int  x = m_tItems[iTem].tPoint.x + d.tPoint.x,    //новое положение элемента
       y = m_tItems[iTem].tPoint.y + d.tPoint.y,
       cx = m_tItems[iTem].tSize.cx + d.tSize.cx,
       cy = m_tItems[iTem].tSize.cy + d.tSize.cy;
  pItemWnd->MoveWindow(x, y, cx, cy);   //переместить элемент
  m_tItems[iTem].Init(x, y, cx, cy);    //запомнить новое положение
} // CPeerNetDialog::MoveElement

// Скрыть или показать элементы диалога на заданной вкладке.
void  CPeerNetDialog::HideOrShowItems(int iPage, int nCmdShow)
{
  CWnd *pItemWnd;
  int   iBase = m_iBase[iPage][0], n = m_iBase[iPage][1];
  for (int i = 0; i < n; i++) {
    pItemWnd = GetDlgItem(m_iTemIDs[iBase + i]);
    pItemWnd->ShowWindow(nCmdShow); //SW_HIDE
  }
} // CPeerNetDialog::HideOrShowItem()

// Инициализировать список ListView
bool  CPeerNetDialog::InitializeListView(CListViewEx *pLsv,
                                     int nCols, TColumnDsc tLstDsc[])
//pLsv      - указатель списка CListViewEx
//nCols     - количество столбцов в списке
//tLstDsc[] - массив описателей столбцов
{
  CListCtrl &ListCtrl = pLsv->GetListCtrl();
  ListCtrl.DeleteAllItems();
  // Формирование шапки ListView с именами столбцов (полей)
  LV_COLUMN  lvc;
  lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
  for (int j = 0; j < nCols ; j++) {
    lvc.iSubItem = j;
    lvc.pszText = tLstDsc[j]._pszName;  //заголовок столбца
    lvc.cx = tLstDsc[j]._nWidth;        //ширина
    lvc.fmt = tLstDsc[j]._nFormt;       //формат
    ListCtrl.InsertColumn(j, &lvc);
  }
  return true;
} // CPeerNetDialog::InitializeListView()

// Очистить список ListView
void  CPeerNetDialog::ClearListView(CListViewEx *pLsv)
{
  CListCtrl &ListCtrl = pLsv->GetListCtrl();
  ListCtrl.DeleteAllItems();
} // CPeerNetDialog::ClearListView()

// Добавить строку в список ListView
void  CPeerNetDialog::AddRowInListView(
  CListViewEx *pLsv, int nCols, LPTSTR pszValues[], bool bClear/* = false*/)
//pLsv        - указатель списка CListViewEx
//nCols       - количество столбцов в списке
//pszValues[] - массив строковых значений полей строки списка
//bClear      - флаг очистки списка перед операцией
{
  CListCtrl &ListCtrl = pLsv->GetListCtrl();
  if (bClear)  ListCtrl.DeleteAllItems();
  LV_ITEM  lvi;
  lvi.mask = LVIF_TEXT; // | LVIF_IMAGE | LVIF_STATE
  int  nRows = ListCtrl.GetItemCount();
  // Цикл вставки в строку списка значений полей
  for (int j = 0; j < nCols ; j++) {
    lvi.pszText = pszValues[j];
    if (j == 0) {
      // Первый столбец - добавить новую строку
      lvi.iItem = nRows;  lvi.iSubItem = 0;
      lvi.iImage = nRows;
      lvi.stateMask = LVIS_STATEIMAGEMASK;
      lvi.state = INDEXTOSTATEIMAGEMASK(1);
      ListCtrl.InsertItem(&lvi);
    }
    else {
      // Добавить в строку поле (SubItem)
      ListCtrl.SetItemText(nRows, j, lvi.pszText);
    }
  }
} // CPeerNetDialog::AddRowInListView()

// Заполнить таблицу транзакций
void  CPeerNetDialog::FillTransactTable()
{
  ClearListView(&m_lsvTransacts);
  TTransact *ptRansact;
  CTransRegister *prTrans = m_oBlkChain.TransRegister();
  for (WORD iTrans = 0; iTrans < prTrans->ItemCount(); iTrans++) {
    ptRansact = prTrans->GetAt(iTrans);
    AddInTransactTable(ptRansact);
  }
} // CPeerNetDialog::FillTransactTable()

// Обработчики сообщений CPeerNetDialog
// ---------------------------------------------------------------------
// Карта обработчиков сообщений Windows
BEGIN_MESSAGE_MAP(CPeerNetDialog, CDialogEx)
  ON_WM_SYSCOMMAND()
  ON_WM_PAINT()
  ON_WM_GETMINMAXINFO()
  ON_WM_SIZE()
  ON_WM_QUERYDRAGICON()
  ON_NOTIFY(TCN_SELCHANGE, IDC_PAGES_TAB, &CPeerNetDialog::OnTcnSelchangePages)
  ON_BN_CLICKED(IDC_LOGIN_BTN, &CPeerNetDialog::OnBnClickedStartBtn)
  ON_BN_CLICKED(IDC_LOGOUT_BTN, &CPeerNetDialog::OnBnClickedStopBtn)
  ON_BN_CLICKED(IDC_REQUEST_BTN, &CPeerNetDialog::OnBnClickedRequestBtn)
  ON_BN_CLICKED(IDC_EXECUTE_BTN, &CPeerNetDialog::OnBnClickedExecuteBtn)
  ON_BN_CLICKED(IDC_SEND_BTN, &CPeerNetDialog::OnBnClickedSendBtn)
  ON_BN_CLICKED(IDC_DUMP_BTN, &CPeerNetDialog::OnBnClickedDumpBtn)
END_MESSAGE_MAP()

// Инициализация диалога
BOOL  CPeerNetDialog::OnInitDialog()
{
  CDialogEx::OnInitDialog();

  CRect  rect;
  // Запомнить размеры клиентской области окна диалога
  GetClientRect(&rect);
  m_DlgSize.SetSize(rect.right - rect.left, -rect.top + rect.bottom);
  // Запомнить общие размеры окна диалога
  GetWindowRect(&rect);
  m_DlgSizeMin.SetSize(rect.right - rect.left, -rect.top + rect.bottom);
  // Добавление пункта "О программе..." в системное меню.
  // IDM_ABOUTBOX должен быть в пределах системной команды.
  ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
  ASSERT(IDM_ABOUTBOX < 0xF000);

  CMenu *pSysMenu = GetSystemMenu(FALSE);
  if (pSysMenu != NULL) //nullptr
  {
    CString strAboutMenu;
    BOOL    bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
    ASSERT(bNameValid);
    if (!strAboutMenu.IsEmpty()) {
      pSysMenu->AppendMenu(MF_SEPARATOR);
      pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
    }
  }

  // Задает значок для этого диалогового окна.  Среда делает это автоматически,
  //если главное окно приложения не является диалоговым
  SetIcon(m_hIcon, TRUE);   //Крупный значок
  SetIcon(m_hIcon, FALSE);  //Мелкий значок

  // TODO: добавьте дополнительную инициализацию
  // Инициализировать Tab Control
  LONG iNdx = m_tbcPages.InsertItem(0, _T("Подключение/Отключение"));
  iNdx = m_tbcPages.InsertItem(1, _T("Транзакции"));
  iNdx = m_tbcPages.InsertItem(2, _T("Монитор"));
  m_iPage = 0;
  HideOrShowItems(1, SW_HIDE);
  HideOrShowItems(2, SW_HIDE);
  // Запомнить исходное расположение перемещаемых элементов диалога
  int i;
  for (i = 0; i < MVITEMSNUM; i++)
    StoreItemPlacement(i);
#ifdef _DEBUG
  // Инициализировать комбобоксы m_cmbLogin, m_cmbPasswd
  CString s, sPaswd = _T("0246897531");
  for (i = 0; i < 7; i++) {
    s.Format(_T("login%02d"), i+1);
    m_cmbLogin.AddString(s);        //комбобокс для выбора логина
    m_cmbPasswd.AddString(sPaswd);  //комбобокс для выбора пароля
    s = sPaswd.Right(9) + sPaswd.Left(1);  sPaswd = s;
  }
#endif
  // Инициализировать списки ListView
  bool b = InitializeListView(&m_lsvVoting, VOTECOLNUM, m_tVoteLstDsc);
  b = InitializeListView(&m_lsvTransacts, TRANCOLNUM, m_tTranLstDsc);
#ifdef _DEBUG
  // В режиме отладки заполнить списки тестовыми данными
  LPTSTR pszRow[VOTECOLNUM] = { _T("0"), _T("user01"), _T("1"),
                                _T("user02"), _T("user03"), _T("100.00") };
  for (i = 0; i < 4; i++)
    AddRowInListView(&m_lsvVoting, VOTECOLNUM, pszRow);
  for (i = 0; i < 10; i++)
    AddRowInListView(&m_lsvTransacts, TRANCOLNUM, pszRow+2);
#endif
  m_lsvVoting.SetFullRowSel(TRUE);      //включить режим выделения всей
  m_lsvTransacts.SetFullRowSel(TRUE);   // строки в списках ListView

  return TRUE;  //возврат значения TRUE, если фокус не передан
                //элементу управления
} // CPeerNetDialog::OnInitDialog()

// Перехват команды закрытия диалога
BOOL  CPeerNetDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
  // TODO: Add your specialized code here and/or call the base class
  UINT  nID = LOWORD(wParam);
  int  nCode = HIWORD(wParam);
  if (nCode == 0 && (nID == IDOK || nID == IDCANCEL)) {
    // Это команда выхода из приложения - остановить узел
    nCode = 0;  //OnBnClickedStopBtn()
  }
  return CDialog::OnCommand(wParam, lParam);
} // CPeerNetDialog::OnCommand()

// Фильтрация сообщений Windows, направленных окну диалога
BOOL  CPeerNetDialog::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam,
                               LRESULT *pResult)
//message - код сообщения
{
  // TODO: Add your specialized code here and/or call the base class
  if (message == WM_DTBQUEUED)
  {
    bool b = m_oBlkChain.ProcessDTB();
    return TRUE;
  }
  return CDialogEx::OnWndMsg(message, wParam, lParam, pResult);
} // CPeerNetDialog::OnWndMsg()

// Обработка команды системного меню
void  CPeerNetDialog::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialogEx::OnSysCommand(nID, lParam);
    }
} // CPeerNetDialog::OnSysCommand()

// При добавлении кнопки свертывания в диалоговое окно нужно воспользоваться 
//приведенным ниже кодом, чтобы нарисовать значок.  Для приложений MFC, 
//использующих модель документов или представлений, это автоматически 
//выполняется рабочей областью.
void  CPeerNetDialog::OnPaint()
{
  if (IsIconic())
  {
    CPaintDC dc(this);  //контекст устройства для рисования

    SendMessage(WM_ICONERASEBKGND,
                reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

    // Выравнивание значка по центру клиентского прямоугольника
    int cxIcon = GetSystemMetrics(SM_CXICON),
        cyIcon = GetSystemMetrics(SM_CYICON);
    CRect rect;
    GetClientRect(&rect);
    int x = (rect.Width() - cxIcon + 1) / 2,
        y = (rect.Height() - cyIcon + 1) / 2;

    // Нарисуйте значок
    dc.DrawIcon(x, y, m_hIcon);
  }
  else {
    CDialogEx::OnPaint();
  }
} // CPeerNetDialog::OnPaint()

// Обработка запроса предельных размеров окна диалога
void  CPeerNetDialog::OnGetMinMaxInfo(MINMAXINFO *lpMMI)
/* typedef struct {
     POINT ptReserved;
     POINT ptMaxSize;
     POINT ptMaxPosition;
     POINT ptMinTrackSize;
     POINT ptMaxTrackSize;
   } MINMAXINFO; */
{
  // TODO: Add your message handler code here and/or call default
  lpMMI->ptMinTrackSize.x = m_DlgSizeMin.cx,
  lpMMI->ptMinTrackSize.y = m_DlgSizeMin.cy;

  CDialog::OnGetMinMaxInfo(lpMMI);
} // CPeerNetDialog::OnGetMinMaxInfo

// Обработка изменения размеров окна
void  CPeerNetDialog::OnSize(UINT nType, int cx, int cy)
{
  CDialogEx::OnSize(nType, cx, cy);

  // TODO: добавьте свой код обработчика сообщений
  if (m_tbcPages.m_hWnd == NULL)  return;   //window is not created yet
  if (nType != SIZE_MAXIMIZED && nType != SIZE_RESTORED)  return;
  // Определение вектора смещения
  CPoint  ptDlt(cx - m_DlgSize.cx, cy - m_DlgSize.cy);
  if (ptDlt.x == 0 && ptDlt.y == 0)  return;
  TPlacement  d;    //описатель приращения координат и размеров
  d.Init(0, 0, ptDlt.x, ptDlt.y);   //изменение IDC_PAGES_TAB
  MoveElement(0, d);
  d.Init(0, 0, 0, ptDlt.y);         //изменение IDC_TRANS_LSV
  MoveElement(1, d);
  d.Init(0, 0, ptDlt.x, 0);         //изменение IDC_EXCHANGE_LST
  MoveElement(2, d);
  d.Init(0, 0, ptDlt.x, ptDlt.y);   //изменение IDC_TRACE_LST
  MoveElement(3, d);
  d.Init(ptDlt.x, ptDlt.y, 0, 0);   //кнопки IDOK,ID_HELP
  MoveElement(4, d);  MoveElement(5, d);
  // Запоминание нового размера клиентской области окна диалога
  m_DlgSize.cx = cx, m_DlgSize.cy = cy;
} // CPeerNetDialog::OnSize()

// Система вызывает эту функцию для получения отображения курсора
//при перемещении свёрнутого окна.
HCURSOR  CPeerNetDialog::OnQueryDragIcon()
{
  return static_cast<HCURSOR>(m_hIcon);
} // CPeerNetDialog::OnQueryDragIcon()

// Обработка выбора вкладки элемента TabCtrl
void  CPeerNetDialog::OnTcnSelchangePages(NMHDR *pNMHDR, LRESULT *pResult)
{
  // TODO: добавьте свой код обработчика уведомлений
  DWORD dwState;  int iPage;
  for (int i = 0; i < 3; i++) {
    // Определить состояние вкладки
    dwState = m_tbcPages.GetItemState(i, TCIS_BUTTONPRESSED);
    if (dwState == TCIS_BUTTONPRESSED) {
      if (m_iPage != i)
        // Вкладка сейчас выделена, а была не выделена - показать элементы
        HideOrShowItems(i, SW_SHOW);
      iPage = i;    //новая выделенная вкладка
    }
    else if (m_iPage == i)
      // Вкладка сейчас не выделена, а была выделена - скрыть элементы
      HideOrShowItems(i, SW_HIDE);
  }
  m_iPage = iPage;  *pResult = NULL;
} // CPeerNetDialog::OnTcnSelchangePages()

// Обработчики событий элементов диалога
// -------------------------------------------------------------
// Нажатие кнопки "Войти" ("Запустить узел")
void  CPeerNetDialog::OnBnClickedStartBtn()
{
  // TODO: Add your control notification handler code here
  CUserRegister *pUsReg = m_oBlkChain.UserRegister();
  if (pUsReg->IsThereSignedInUser())
  { // Вывести сообщение
    CString s(_T("Пользователь уже авторизован."));
    s += _T(" Чтобы войти в сеть под другим именем, нужно сначала выйти.");
    ShowMessage(s);
    return;
  }
  UpdateData();         //TRUE: передача Controls -> Variables
  CString sLogin, sPaswd;
  m_cmbLogin.GetWindowTextW(sLogin);
  m_cmbPasswd.GetWindowTextW(sPaswd);
  WORD iUser =
    m_oBlkChain.AuthorizationCheck(sLogin.GetBuffer(), sPaswd.GetBuffer());
  if (iUser == UNAUTHORIZED)
  { // Ошибка регистров или авторизации - вывести сообщение
    CUserRegister *prUsers = m_oBlkChain.UserRegister();
    LPTSTR pszErrMes = prUsers->Error() ? prUsers->ErrorMessage() :
                                          _T("Логин или пароль неверен");
    ShowMessage(pszErrMes);
    return;
  }
  m_oBlkChain.Authorize(iUser); //сделать пользователя владельцем
  bool b = m_oNode.IsNodeRunning();
  if (!b)  b = m_oNode.StartupNode();
  if (b)
  {
    m_oBlkChain.TieUpUserAndNode(iUser, m_oNode.m_iOwnNode);
    TUser *ptUser = pUsReg->GetAt(iUser);
    TBalance *ptBalnc = m_oBlkChain.BalanceRegister()->GetAt(iUser);
    CTransRegister *prTrans = m_oBlkChain.TransRegister();
    CString s(ptUser->_sName), s1(ptBalnc->_sBalance), s2;
    s2.Format(_T("%d"), prTrans->ItemCount()+1);
    m_sUserName = s;  m_sBalance = s1;  m_sTransNo = s2;
    ClearVotingTable();  FillTransactTable();
    UpdateData(FALSE);  //передача Variables -> Controls
  }
} // CPeerNetDialog::OnBnClickedStartBtn()

// Нажатие кнопки "Выйти" ("Остановить узел")
void  CPeerNetDialog::OnBnClickedStopBtn()
{
  // TODO: Add your control notification handler code here
  if (m_oNode.IsNodeRunning())  m_oNode.ShutdownNode();
  m_oBlkChain.CancelAuthorization();    //отменить авторизацию
  // Очистить элементы формы
  m_sUserName.Empty();  m_sBalance.Empty();
  m_cmbLogin.SetWindowTextW(_T(""));
  m_cmbPasswd.SetWindowTextW(_T(""));
  if (m_lstNodes.m_hWnd)  m_lstNodes.ResetContent();
  UpdateData(FALSE);    //передача Variables -> Controls
} // CPeerNetDialog::OnBnClickedStopBtn()

// Нажата кнопка "Послать запрос транзакции"
void  CPeerNetDialog::OnBnClickedRequestBtn()
{
  // TODO: Add your control notification handler code here
  ClearVotingTable();   //очистить таблицу голосования
  ApproveTransaction(0);
  UpdateData();         //TRUE: передача Controls -> Variables
  // Получить из интерфейса исходные параметры транзакции
  CString sUserTo;
  m_cmbUserTo.GetWindowTextW(sUserTo);                  //"кому"
  WORD iUserTo =
    m_oBlkChain.UserRegister()->GetUserByName(sUserTo.GetBuffer());
  //ASSERT(iUserTo != 0xFFFF);
  double rSum;
  _stscanf_s(m_sAmount.GetBuffer(), _T("%lf"), &rSum);  //"сколько"
  m_oBlkChain.StartTransaction(iUserTo, rSum);
} // CPeerNetDialog::OnBnClickedRequestBtn()

// Нажата кнопка "Выполнить транзакцию"
void  CPeerNetDialog::OnBnClickedExecuteBtn()
{
  // TODO: Add your control notification handler code here
} // CPeerNetDialog::OnBnClickedExecuteBtn()

// Нажатие кнопки "Послать"
void  CPeerNetDialog::OnBnClickedSendBtn()
{
  // TODO: Add your control notification handler code here
  CString  sNode, sReqst;
  UpdateData();         //передача Controls -> Variables
  if (m_sRequest.Left(3).Compare(_T("TAQ")) == 0)
  {
    WORD iUserTo = CPeerNetNode::ToNumber(m_sRequest.GetBuffer() + 4, 2);
    double rSum = (double)CPeerNetNode::ToNumber(m_sReply.GetBuffer(),
                                                 (WORD)m_sReply.GetLength());
    m_oBlkChain.StartTransaction(iUserTo, rSum);
  }
  else
  {
    // Извлечь номер узла-адресата из выделенной строки в списке узлов
    int  iSel = m_lstNodes.GetCurSel();
    if (iSel < 0)  return;
    m_lstNodes.GetText(iSel, sNode);
    WORD  wLeng = (WORD)sNode.GetLength(),
          wTo = CPeerNetNode::ToNumber(sNode.GetBuffer() + 6, wLeng - 6) - 1;
    wLeng = (WORD)m_sRequest.GetLength(); //длина текстовой строки (без '\0')
    // Послать запрос, если в поле запроса есть текст
    if (wLeng > 0)
    {
      // Сформировать команду "Блок данных" с запросом
      sReqst.Format(_T("DTB %02d%02d%04d%s"),
                    m_oNode.m_iOwnNode, wTo, (wLeng+1)*sizeof(TCHAR), m_sRequest.GetBuffer());
      m_lstExchange.AddString(sReqst.GetBuffer());    //занести в журнал запрос
      TMessageBuf  mbRequest(sReqst.GetBuffer());
      TMessageBuf *pmbReply = m_oNode.RequestAndReply(wTo, mbRequest);
      ShowReply(wTo, pmbReply->Message());
      delete pmbReply;    //освобождение буфера ответа в дин памяти
      UpdateData(FALSE);  //передача Variables -> Controls
    }
  }
} // CPeerNetDialog::OnBnClickedSendBtn()

// Вывести дамп списка трассировки
void  CPeerNetDialog::OnBnClickedDumpBtn()
{
  // TODO: Add your control notification handler code here
  TCHAR  szBuffer[CHARBUFSIZE];
  FILE *pFile;  //указатель файла \\Data
  swprintf_s(szBuffer, CHARBUFSIZE, _T("..\\TraceDump_%d.txt"), m_iOwnNode);
  errno_t err = _wfopen_s(&pFile, szBuffer, _T("wt"));
  for (int i = 0; i < m_lstTrace.GetCount(); i++) {
    m_lstTrace.GetText(i, szBuffer);
    fwprintf_s(pFile, _T("%s\n"), szBuffer);
  }
  fclose(pFile);
} // CPeerNetDialog::OnBnClickedDumpBtn()
