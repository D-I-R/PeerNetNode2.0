// FileBase.cpp                                            (c) DIR, 2019
//
//              Реализация классов файловой базы данных
//
// CFileBase        - Базовый класс для объектов файловой "базы данных"
// CUserRegister    - Регистр пользователей
// CBalanceRegister - Регистр текущих остатков
// CTransRegister   - Регистр транзакций
//
////////////////////////////////////////////////////////////////////////

//#include "stdafx.h"
#include "pch.h"
#include "FileBase.h"

#ifdef _DEBUG
 #define new DEBUG_NEW
#endif

//      Реализация класса CFileBase
////////////////////////////////////////////////////////////////////////
//      Инициализация статических элементов данных
//
//static Массив сообщений об ошибках
TCHAR * CFileBase::m_sErrMes[] = {
/* 0 */ _T("OK"),
/* 1 */ _T("CFileBase: Ошибка открытия файла"),
/* 2 */ _T("CFileBase: Ошибка чтения из файла"),
/* 3 */ _T("CFileBase: Недействительный формат файла"),
/* 4 */ _T("CFileBase: Ошибка записи в файл"),
/* 5 */ _T("CFileBase: Ошибка закрытия файла"),
/* 6 */ _T("CUserRegister: Регистр пользователей не загружен"),
/* 7 */ _T("CBlockChain: Сумма запрашиваемой транзакции превышает текущий остаток"),
//* 8 * "",
//* 9 * "",
};

// Конструктор и деструктор
CFileBase::CFileBase(CCrypto *paCrypt, LPTSTR pszFileNameBase,
                     WORD nItemSize)
{
  m_paCrypt = paCrypt;  //указатель объекта криптографии
  Initialize(pszFileNameBase, nItemSize);
  m_nItemCount = 0;     //количество элементов данных
  //m_iOwner = 0xFFFF;    индекс пользователя-владельца произв объекта
  //m_nOwner = 0;         номер пользователя-владельца произв объекта
  m_pFile = NULL;       //указатель файла
  m_bLoaded = false;    //флаг "объект загружен из файла"
  m_bChanged = false;   //флаг "объект изменён"
  m_nErr = 0;           //код ошибки
}
CFileBase::~CFileBase(void)
{
  if (m_pFile) {
    fclose(m_pFile);  m_pFile = NULL;
  }
} // CFileBase::~CFileBase()

// Внешние методы
// Задать рабочий каталог для размещения файловой БД
void  CFileBase::SetFolder(LPTSTR sFolder/* = NULL*/)
{
  LPTSTR psz = (sFolder != NULL) ? sFolder : FOLDERDEFLT;
  int  len = min(_tcslen(psz), FOLDERNMSIZ-1);
  memcpy(m_sFolder, psz, len * sizeof(TCHAR));
  // Обнуление хвоста (как минимум один ноль)
  memset(m_sFolder+len, 0, (FOLDERNMSIZ-len) * sizeof(TCHAR));
} // CFileBase::SetFolder()

//virtual Начать загрузку файла БД для указанного номера пользователя
bool  CFileBase::Load()
{
  TCHAR  sFilePath[FOLDERNMSIZ+FILENAMESIZ];
  size_t  nSize = FOLDERNMSIZ + FILENAMESIZ;
  // Дать полную спецификацию файла
  FillFilePath(sFilePath, nSize);
  m_nErr = 0;
  // Прочитать и проверить сигнатуру файла
  errno_t  err = _tfopen_s(&m_pFile, sFilePath, _T("r+b"));
  if (err != 0)
    { m_nErr = 1;  return false; }  //ошибка открытия файла
  // Прочитать по 1 байту FILESIGNSIZ раз:
  size_t  nRead = fread(m_sFileSign, 1, FILESIGNSIZ, m_pFile);
  if (nRead != FILESIGNSIZ)
    { m_nErr = 2;  return false; }  //ошибка чтения файла
  if (!CheckSignature())
    { m_nErr = 3;  return false; }  //несовпадение сигнатуры
  // Прочитать m_nItemSize, m_nItemCount
  nRead = fread(&m_nItemSize, 1, sizeof(m_nItemSize), m_pFile);
  if (nRead != sizeof(m_nItemSize))
    { m_nErr = 2;  return false; }  //ошибка чтения файла
  nRead = fread(&m_nItemCount, 1, sizeof(m_nItemCount), m_pFile);
  if (nRead != sizeof(m_nItemCount))
    { m_nErr = 2;  return false; }  //ошибка чтения файла
  return  true;
} // CFileBase::Load()

//virtual  Сохранить данные объекта в файле БД
bool  CFileBase::Save(bool bDoSave/* = false*/)
{
  TCHAR  sFilePath[FOLDERNMSIZ+FILENAMESIZ];
  size_t  nSize = FOLDERNMSIZ + FILENAMESIZ;
  // Дать полную спецификацию файла
  FillFilePath(sFilePath, nSize);
  m_nErr = 0;
  // Открыть файл на запись
  errno_t  err = _tfopen_s(&m_pFile, sFilePath, _T("w+b"));
  if (err != 0)
    { m_nErr = 1;  return false; }  //ошибка открытия файла
  // Записать сигнатуру файла
  InitSignature();
  // Записать по 1 байту FILESIGNSIZ раз:
  size_t  nWrit = fwrite(m_sFileSign, 1, FILESIGNSIZ, m_pFile);
  if (nWrit != FILESIGNSIZ)
    { m_nErr = 4;  return false; }  //ошибка записи файла
  // Записать m_nItemSize, m_nItemCount
  nWrit = fwrite(&m_nItemSize, 1, sizeof(m_nItemSize), m_pFile);
  if (nWrit != sizeof(m_nItemSize))
    { m_nErr = 4;  return false; }  //ошибка записи файла
  nWrit = fwrite(&m_nItemCount, 1, sizeof(m_nItemCount), m_pFile);
  if (nWrit != sizeof(m_nItemCount))
    { m_nErr = 4;  return false; }  //ошибка записи файла
  return true;
} // CFileBase::Save()

//virtual  Добавить элемент данных в объект БД
bool  CFileBase::Add(void *pItem, WORD nLeng/* = 0*/)
{
  return true;
} // CFileBase::Add()

//virtual  Найти элемент данных в объекте БД
WORD  CFileBase::Find(void *pItem, WORD nLeng/* = 0*/)
{
  return 0;
} // CFileBase::Find()

//protected:
//virtual Перегруженный метод. Дать образец сигнатуры файла БД.
char * CFileBase::GetSignaturePattern(WORD &nLeng)
//nLeng[out] - размер сигнатуры файла в байтах
{
  static char *sFileSign = FILESIGNBAS;
  nLeng = strlen(sFileSign);    //без учёта завершающего 0
  return sFileSign;
}
char * CFileBase::GetSignaturePattern(WORD &nLeng, char *psIgn2)
{
  static char  sFileSign[FILESIGNSIZ];  //буфер для сигнатуры файла
  WORD  leng;
  char *psIgn = CFileBase::GetSignaturePattern(leng);
  if (leng > FILESIGNSIZ-1)  leng = FILESIGNSIZ-1;
  memcpy(sFileSign, psIgn, leng);       //копирование 1-й части
  WORD  nRest = FILESIGNSIZ - leng,     //осталось байт свободных
        leng2 = strlen(psIgn2);         //длина 2-й части
  if (leng2 > nRest-1)  leng2 = nRest-1;
  memcpy(sFileSign+leng, psIgn2, leng2); //копирование 2-й части
  leng += leng2;  sFileSign[leng] = '\0';
  nLeng = leng;
  return sFileSign;
} // CFileBase::GetSignaturePattern()

// Заполнить поле сигнатуры файла БД
void  CFileBase::InitSignature()
{
  WORD  nLeng;
  char *psIgn = GetSignaturePattern(nLeng);
  if (nLeng > FILESIGNSIZ-1)  nLeng = FILESIGNSIZ-1;
  memcpy(m_sFileSign, psIgn, nLeng);
  // Обнуление хвоста (как минимум один ноль)
  memset(m_sFileSign+nLeng, 0, FILESIGNSIZ-nLeng);
} // CFileBase::InitSignature()

//virtual  Проверить сигнатуру, загруженную из файла БД
bool  CFileBase::CheckSignature()
{
  WORD  nLeng;
  char *psIgn = GetSignaturePattern(nLeng);
  int  n = strncmp(m_sFileSign, psIgn, nLeng);
  return (n == 0);
} // CFileBase::CheckSignature()

//private:
// Задать базовые параметры объекта
void  CFileBase::Initialize(LPTSTR sFileName, WORD nItemSize)
{
  // Занести основу имени файла
  int len = min(_tcslen(sFileName), FILENAMESIZ-1); //колич символов
  memcpy(m_sFileName, sFileName, len * sizeof(TCHAR));
  // Обнуление хвоста (как минимум один ноль)
  memset(m_sFileName+len, 0, (FILENAMESIZ-len) * sizeof(TCHAR));

  SetFolder();  //занести имя рабочего каталога БД по умолчанию
  // Данные пользователя-владельца, определяются при регистрации:
  m_iOwner = 0xFFFF; //индекс пользователя-владельца в таблице пользователей
  m_nOwner = 0;      //номер пользователя-владельца производного объекта
  m_nItemSize = nItemSize;  //размер элемента данных
} // CFileBase::Initialize()

// Поместить в буфер полную спецификацию файла БД
void  CFileBase::FillFilePath(LPTSTR pszFilePath, WORD nSize)
//pszFilePath - буфер для полной спецификации
//nSize       - размер буфера в символах
{
  WORD  iLast = _tcslen(m_sFileName) - 1;
  if (m_sFileName[iLast] == _T('_'))
    // Имя файла содержит номер пользователя
    swprintf_s(pszFilePath, nSize,
               _T("%s%s%02d.dat"), m_sFolder, m_sFileName, m_nOwner);
  else
    swprintf_s(pszFilePath, nSize,
               _T("%s%s.dat"), m_sFolder, m_sFileName);
} // CFileBase::FillFilePath()

// End of class CFileBase definition -----------------------------------


//      Реализация класса CUserRegister
////////////////////////////////////////////////////////////////////////
//      Инициализация статических элементов данных
//

// Конструктор и деструктор
CUserRegister::CUserRegister(CCrypto *paCrypt)
             : CFileBase(paCrypt, _T("UserList"), sizeof(TUser))
{
}
CUserRegister::~CUserRegister(void)
{
  bool b = Save();
  ClearUserArray();
} // CUserRegister::~CUserRegister()

// Внешние методы
// ---------------------------------------------------------------------
// Проверить регистрационные данные пользователя и дать его индекс
bool  CUserRegister::AuthorizationCheck(LPTSTR pszLogin,
                                        LPTSTR pszPasswd, WORD &iUser)
{
  if (m_nItemCount == 0) { m_nErr = 6;  return false; }
  if (_tcslen(pszLogin) == 0 || _tcslen(pszPasswd) == 0)  return false;
  TUser *ptUser;  int nCmp;
  for (WORD i = 0; i < m_nItemCount; i++) {
    ptUser = m_aUsers[i];
    nCmp = _tcscmp(ptUser->_szLogin, pszLogin);
    if (nCmp == 0) {
      // Login совпал - сравнить пароли
      nCmp = _tcscmp(ptUser->_szPasswd, pszPasswd);
      if (nCmp == 0) {
        //m_iOwner = i;  m_nOwner = ptUser->GetUserOrd();
        //SetOwner(i);
        iUser = i;  return true;
      }
    }
  }
  return false; //не найден
} // CUserRegister::AuthorizationCheck()

// Задать индекс, а по нему и номер пользователя-владельца объекта
bool  CUserRegister::SetOwner(WORD iUser)
{
  bool b = (0 <= iUser && iUser < m_nItemCount);
  if (b) {
    m_iOwner = iUser;
    m_nOwner = m_aUsers[iUser]->GetUserOrd();
  }
  return b;
} // SetOwner()

// Дать имя пользователя по его индексу
bool  CUserRegister::UserName(WORD iUser, LPTSTR pszName)
{
  TUser *ptUser = GetAt(iUser);
  if (ptUser == NULL)  return false;
  CString s(ptUser->_sName);    //перекодирование ANSI->Unicode
  _tcscpy_s(pszName, NAMESBUFSIZ, s.GetBuffer());
  return true;
} // CUserRegister::UserName()

// Дать индекс пользователя по его имени вида userNN
WORD  CUserRegister::GetUserByName(LPTSTR pchName, WORD nOffset/* = 4*/)
{
  // Просто преобразовать индекс пользователя
  DWORD iUser;  _stscanf_s(pchName+nOffset, _T("%d"), &iUser);
  if (nOffset > 0)  iUser--;
  return (WORD)iUser;
/*  TUser *ptUser;  int nCmp;
  for (WORD i = 0; i < m_nItemCount; i++) {
    ptUser = m_aUsers[i];
    {
      CString sName(ptUser->_sName);
      if (sName.Compare(pchName) == 0)  return i;
    }
  }
  return 0xFFFF;    //не найден */
} // CUserRegister::GetUserByName()

// Сгенерировать таблицу пользователей в памяти
void  CUserRegister::GenerateUserArray(WORD nUsers)
{
  TUser *ptUser;  WORD i, k;
  for (i = 1; i <= nUsers; i++) {
    ptUser = GenerateUser(i);
    if (ptUser) {
      k = m_aUsers.Add(ptUser);  m_nItemCount++;
    }
  }
  m_bChanged = true;
} // CUserRegister::GenerateUserArray()

//virtual Загрузить файл БД
bool  CUserRegister::Load()
{
  bool b = CFileBase::Load();
  if (b)
  {
    TUser *ptUser;  size_t nRead;  WORD i, k;
    for (i = 0; i < m_nItemCount; i++) {
      ptUser = new TUser;
      nRead = fread(ptUser, 1, sizeof(TUser), m_pFile);
      if (nRead != sizeof(TUser))
        { m_nErr = 2;  b = false;  break; }   //ошибка чтения файла
      k = m_aUsers.Add(ptUser);
    }
  }
  if (m_pFile) {
    if (fclose(m_pFile)) { m_nErr = 5;  b = false; } //ошибка закрытия файла
    m_pFile = NULL;
  }
  m_bLoaded = b;  m_bChanged = false;
  return b;
} // CUserRegister::Load()

//virtual  Сохранить данные объекта в файле БД
bool  CUserRegister::Save(bool bDoSave/* = false*/)
{
  if (!bDoSave && !m_bChanged)  return false;
  bool b = CFileBase::Save();
  if (b)
  {
    TUser *ptUser;  size_t nWrit;
    for (WORD i = 0; i < m_nItemCount; i++) {
      ptUser = m_aUsers[i];
      nWrit = fwrite(ptUser, 1, sizeof(TUser), m_pFile);
      if (nWrit != sizeof(TUser))
        { m_nErr = 4;  b = false;  break; }   //ошибка записи файла
    }
  }
  if (m_pFile) {
    if (fclose(m_pFile)) { m_nErr = 5;  b = false; } //ошибка закрытия файла
    m_pFile = NULL;
  }
  m_bChanged = !b;
  return b;
} // CUserRegister::Save()

//virtual  Добавить элемент данных в объект БД
bool  CUserRegister::Add(void *pItem, WORD nLeng/* = 0*/)
{
  return true;
} // CUserRegister::Add()

// Найти индекс пользователя в регистре
//virtual
WORD  CUserRegister::Find(void *pItem, WORD nLeng/* = 0*/)
//pItem - элемент данных (pchLogHash - хеш-код Login'а)
{
#ifdef DBGHASHSIZE  //определено в FileBase.h
#else
  // Просто преобразовать индекс пользователя
  DWORD iUser;
  _stscanf_s((LPTSTR)pItem, _T("%d"), &iUser);
  return (WORD)iUser;
#endif
  // Искать по по хеш-коду Login'а
  TUser *ptUser;  int nCmp;
  if (nLeng == 0)  return 0xFFFF;
  for (WORD i = 0; i < m_nItemCount; i++) {
    ptUser = m_aUsers[i];
    nCmp = memcmp(ptUser->_chLogHash, pItem, nLeng);
    if (nCmp == 0)  return i;
  }
  return 0xFFFF;    //не найден
} // CUserRegister::Find()

//protected: -----------------------------------------------------------
//virtual  Дать образец сигнатуры файла БД
char * CUserRegister::GetSignaturePattern(WORD &nLeng)
//nLeng[out] - размер сигнатуры файла в байтах
{
  return CFileBase::GetSignaturePattern(nLeng, USERSIGNATR);
} // CUserRegister::GetSignaturePattern()

//private: -------------------------------------------------------------
// Сгенерировать запись таблицы пользователей
TUser * CUserRegister::GenerateUser(WORD nUser)
//nUser[in] - номер пользователя
{
  static TCHAR szPwd[11] = _T("0246897531"); //строка для генерирования пароля
  char sName[NAMESBUFSIZ];
  TCHAR ch, szLogin[NAMESBUFSIZ];
  TUser *ptUser = new TUser;
  sprintf_s(sName, NAMESBUFSIZ, "user%02d", nUser);  ptUser->SetName(sName);
  _stprintf_s(szLogin, NAMESBUFSIZ, _T("login%02d"), nUser);  ptUser->SetLogin(szLogin);
  ptUser->SetPassword(szPwd);
  // Подготовить следующий пароль
  ch = szPwd[0];  memmove(szPwd, szPwd+1, 9*sizeof(TCHAR));  szPwd[9] = ch;
  // Длина вместе с '\0' в байтах:
  DWORD dwLoginLen = (_tcslen(ptUser->_szLogin) + 1) * sizeof(TCHAR),
//      dwPasswLen = _tcslen(ptUser->_szPasswd) + 1, //длина вместе с '\0'
        dwHashSize;
  // Дать хеш-код Login'а пользователя
  BYTE *pbDataHash = m_paCrypt->GetDataSignature(
    (BYTE *)ptUser->_szLogin, dwLoginLen, dwHashSize
  );
  //ASSERT(dwHashSize == HASHSIZE);
  ptUser->SetLoginHash(pbDataHash, (WORD)dwHashSize);
  /* Дать хеш-код пароля пользователя
  pbDataHash = m_paCrypt->GetDataSignature(
    (BYTE *)ptUser->_szPasswd, dwPasswLen, dwHashSize
  );
  ASSERT(dwHashSize == HASHSIZE);
  ptUser->SetPasswordHash(pbDataHash, (WORD)dwHashSize); */
  return ptUser;
} // CUserRegister::GenerateUser()

// Очистить таблицу пользователей в памяти
void  CUserRegister::ClearUserArray()
{
  TUser *ptUser;  WORD i;
  for (i = 0; i < m_nItemCount; i++) {
    ptUser = m_aUsers[i];
    if (ptUser) {
      delete ptUser;  ptUser = NULL;
    }
  }
  m_aUsers.RemoveAll();  m_nItemCount = 0;
} // CUserRegister::ClearUserArray()

// End of class CUserRegister definition -------------------------------


//      Реализация класса CBalanceRegister
////////////////////////////////////////////////////////////////////////
//      Инициализация статических элементов данных
//

// Конструктор и деструктор
CBalanceRegister::CBalanceRegister(CCrypto *paCrypt)
                : CFileBase(paCrypt, _T("Balnc_"), sizeof(TBalance))
{
}
CBalanceRegister::~CBalanceRegister(void)
{
  bool b = Save();
  ClearBalanceArray();
} // CBalanceRegister::~CBalanceRegister()

// Внешние методы
// Сгенерировать таблицу текущих остатков в памяти
void  CBalanceRegister::GenerateBalanceArray(CUserRegister *poUserReg,
                                             double rSum)
//poUserReg - указатель таблицы пользователей в памяти
//rSum      - сумма, которую нужно зачислить на счета пользователей
{
  TUser *ptUser;  TBalance *ptBalance;  WORD iUser, k;
  for (iUser = 0; (ptUser = poUserReg->GetAt(iUser)) != NULL; iUser++) {
    ptBalance = GenerateBalance(ptUser, rSum);
    if (ptBalance) {
      k = m_aBalances.Add(ptBalance);  m_nItemCount++;
    }
  }
  m_bChanged = true;
} // CBalanceRegister::GenerateBalanceArray()

//virtual Загрузить файл БД
bool  CBalanceRegister::Load()
{
  bool b = CFileBase::Load();
  if (b)
  {
    TBalance *ptBalance;  size_t nRead;  WORD i, k;
    for (i = 0; i < m_nItemCount; i++) {
      ptBalance = new TBalance;
      nRead = fread(ptBalance, 1, sizeof(TBalance), m_pFile);
      if (nRead != sizeof(TBalance))
        { m_nErr = 2;  b = false;  break; }   //ошибка чтения файла
      k = m_aBalances.Add(ptBalance);
    }
  }
  if (m_pFile) {
    if (fclose(m_pFile)) { m_nErr = 5;  b = false; } //ошибка закрытия файла
    m_pFile = NULL;
  }
  m_bLoaded = b;  m_bChanged = false;
  return b;
} // CBalanceRegister::Load()

//virtual  Сохранить данные объекта в файле БД
bool  CBalanceRegister::Save(bool bDoSave/* = false*/)
{
  if (!bDoSave && !m_bChanged)  return false;
  bool b = CFileBase::Save();
  if (b)
  {
    TBalance *ptBalance;  size_t nWrit;
    for (WORD i = 0; i < m_nItemCount; i++) {
      ptBalance = m_aBalances[i];
      nWrit = fwrite(ptBalance, 1, sizeof(TBalance), m_pFile);
      if (nWrit != sizeof(TBalance))
        { m_nErr = 4;  b = false;  break; }   //ошибка записи файла
    }
  }
  if (m_pFile) {
    if (fclose(m_pFile)) { m_nErr = 5;  b = false; } //ошибка закрытия файла
    m_pFile = NULL;
  }
  m_bChanged = !b;
  return b;
} // CBalanceRegister::Save()

//virtual  Добавить элемент данных в объект БД
bool  CBalanceRegister::Add(void *pItem, WORD nLeng/* = 0*/)
{
  return true;
} // CBalanceRegister::Add()

//virtual  Найти элемент данных в объекте БД
WORD  CBalanceRegister::Find(void *pItem, WORD nLeng/* = 0*/)
{
  return 0;
} // CBalanceRegister::Find()

//protected:
//virtual  Дать образец сигнатуры файла БД
char * CBalanceRegister::GetSignaturePattern(WORD &nLeng)
//nLeng[out] - размер сигнатуры файла в байтах
{
  return CFileBase::GetSignaturePattern(nLeng, BLNCSIGNATR);
} // CBalanceRegister::GetSignaturePattern()

//private:
// Сгенерировать запись таблицы текущих остатков в памяти
TBalance * CBalanceRegister::GenerateBalance(TUser *ptUser, double rSum)
{
  TBalance *ptBalance = new TBalance;
  memcpy(ptBalance->_sName, ptUser->_sName, NAMESBUFSIZ);
  memcpy(ptBalance->_chLogHash, ptUser->_chLogHash, HASHSIZE);
  ptBalance->SetBalance(rSum);
  return ptBalance;
} // CBalanceRegister::GenerateBalance()

// Очистить таблицу текущих остатков в памяти
void  CBalanceRegister::ClearBalanceArray()
{
  TBalance *ptBalance;
  for (WORD i = 0; i < m_nItemCount; i++) {
    ptBalance = m_aBalances[i];
    if (ptBalance) {
      delete ptBalance;  ptBalance = NULL;
    }
  }
  m_aBalances.RemoveAll();  m_nItemCount = 0;
} // CBalanceRegister::ClearBalanceArray()

// End of class CBalanceRegister definition ----------------------------


//      Реализация класса CTransRegister
////////////////////////////////////////////////////////////////////////
//      Инициализация статических элементов данных
//

// Конструктор и деструктор
CTransRegister::CTransRegister(CCrypto *paCrypt, CUserRegister *paUsers)
              : CFileBase(paCrypt, _T("Trans_"), sizeof(TTransact))
{
  memset(m_chLastHash, 0, HASHSIZE);
  m_paUsers = paUsers;  //указатель таблицы пользователей
}
CTransRegister::~CTransRegister(void)
{
  bool b = Save();
  ClearTransactArray();
} // CTransRegister::~CTransRegister()

// Внешние методы
// Преобразовать хеш-код в символьное представление
void  CTransRegister::HashToSymbols(char *pcBuff, BYTE btHash[])
//pcBuff[out] - выходной буфер размера 2*HASHSIZE для результата
//btHash[in]  - хеш-код в байтовом массиве размера HASHSIZE
{
  char *pc = pcBuff;  BYTE hbt0, hbt1;
  for (int i = 0; i < HASHSIZE; i++) {
    hbt0 = btHash[i] & 0x0F;  hbt1 = (btHash[i] & 0xF0) >> 4;
	*pc++ = (hbt0 < 10) ? hbt0 + '0' : (hbt0 - 10) + 'a';
	*pc++ = (hbt1 < 10) ? hbt1 + '0' : (hbt1 - 10) + 'a';
  }
} // CTransRegister::HashToSymbols()

// Преобразовать символьное представление в хеш-код
void  CTransRegister::SymbolsToHash(char *pcBuff, BYTE btHash[])
//pcBuff[in]  - входной буфер размера 2*HASHSIZE с представлением хеш-кода
//btHash[out] - байтовый массив размера HASHSIZE для хеш-кода
{
  char *pc = pcBuff;  BYTE hbt0, hbt1;
  for (int i = 0; i < HASHSIZE && *pc; i++) {
    hbt0 = *pc++;
    if ('0' <= hbt0 && hbt0 <= '9')  hbt0 -= '0';
    else { hbt0 -= 'a';  hbt0 += 10; }
    hbt1 = *pc++;
    if ('0' <= hbt1 && hbt1 <= '9')  hbt1 -= '0';
    else { hbt1 -= 'a';  hbt1 += 10; }
    btHash[i] = (hbt1 << 4) & hbt0;
  }
} // CTransRegister::HashToSymbols()

//virtual Начать загрузку файла БД для указанного номера пользователя
bool  CTransRegister::Load()
{
  bool  b = CFileBase::Load();  size_t nRead;
  if (b) {
    // Прочитать хеш-код последней транзакции
    // Прочитать по 1 байту HASHSIZE раз:
    nRead = fread(m_chLastHash, 1, HASHSIZE, m_pFile);
    if (nRead != HASHSIZE)
      { m_nErr = 2;  b = false; }   //ошибка чтения файла
  }
  if (b)
  {
    TTransact *ptRansact;  WORD i, k;
    for (i = 0; i < m_nItemCount; i++) {
      ptRansact = new TTransact;
      nRead = fread(ptRansact, 1, sizeof(TTransact), m_pFile);
      if (nRead != sizeof(TTransact))
        { m_nErr = 2;  b = false;  break; } //ошибка чтения файла
      k = m_aTrans.Add(ptRansact);
    }
  }
  if (m_pFile) {
    if (fclose(m_pFile)) { m_nErr = 5;  b = false; } //ошибка закрытия файла
    m_pFile = NULL;
  }
  m_bLoaded = b;  m_bChanged = false;
  return b;
} // CTransRegister::Load()

//virtual  Сохранить данные объекта в файле БД
bool  CTransRegister::Save(bool bDoSave/* = false*/)
{
  if (!bDoSave && !m_bChanged)  return false;
  size_t nWrit;
  bool b = CFileBase::Save();
  if (b) {
    // Записать хеш-код последней транзакции
    nWrit = fwrite(m_chLastHash, 1, HASHSIZE, m_pFile);
    if (nWrit != HASHSIZE)
      { m_nErr = 4;  b = false; }   //ошибка записи файла
  }
  if (b)
  {
    TTransact *ptRansact;
    for (WORD i = 0; i < m_nItemCount; i++) {
      ptRansact = m_aTrans[i];
      nWrit = fwrite(ptRansact, 1, sizeof(TTransact), m_pFile);
      if (nWrit != sizeof(TTransact))
        { m_nErr = 4;  b = false;  break; } //ошибка записи файла
    }
  }
  if (m_pFile) {
    if (fclose(m_pFile)) { m_nErr = 5;  b = false; } //ошибка закрытия файла
    m_pFile = NULL;
  }
  m_bChanged = !b;
  return b;
} // CTransRegister::Save()

//virtual  Добавить новый блок в хеш-чейн транзакций
bool  CTransRegister::Add(void *pItem, WORD nLeng/* = 0*/)
//pItem - полностью сформированный блок транзакции
{
  TTransact *ptRansact = (TTransact *)pItem;
  if (nLeng == 0)  nLeng = sizeof(TTransact);
  else
    ASSERT(nLeng == sizeof(TTransact));
  // Вставить хеш-код последней транзакции
  //memcpy(ptRansact->_chPrevHash, m_chLastHash, HASHSIZE);
  // Добавить транзакцию в хеш-чейн
  WORD k = m_aTrans.Add(ptRansact);
  m_nItemCount++;
  // Вычислить хеш-код последней транзакции
  DWORD dwHashSize;
  BYTE *pbDataHash = m_paCrypt->GetDataSignature(
    (BYTE *)ptRansact, (DWORD)nLeng, dwHashSize
  );
  ASSERT(dwHashSize == HASHSIZE);
  memcpy(m_chLastHash, pbDataHash, HASHSIZE);
  m_bChanged = true;
  return true;
} // CTransRegister::Add()

//virtual  Найти элемент данных в объекте БД
WORD  CTransRegister::Find(void *pItem, WORD nLeng/* = 0*/)
{
  return 0;
} // CTransRegister::Find()

// Создать новый блок цепочки транзакций
TTransact * CTransRegister::CreateTransactBlock(
  WORD iUserFrom, WORD iUserTo, double rAmount)
//iUserFrom - индекс пользователя-отправителя
//iUserTo   - индекс пользователя-получателя
//rAmount   - сумма транзакции
{
  TTransact *ptRansact = new TTransact;
  TUser *ptUserFrom = m_paUsers->GetAt(iUserFrom);
  TUser *ptUserTo = m_paUsers->GetAt(iUserTo);
  // Вставить хеш-код последней транзакции
  memcpy(ptRansact->_chPrevHash, m_chLastHash, HASHSIZE);
  ptRansact->SetTransOrd(GetNewTransactOrd());  //номер транзакции
  // Вставить хеш-код Login'а пользователя-отправителя
  memcpy(ptRansact->_chHashSrc, ptUserFrom->_chLogHash, HASHSIZE);
  // Вставить хеш-код Login'а пользователя-получателя
  memcpy(ptRansact->_chHashDst, ptUserTo->_chLogHash, HASHSIZE);
  ptRansact->SetAmount(rAmount);                //сумма транзакции
  return ptRansact;
} // CTransRegister::CreateTransactBlock()

//protected:
//virtual  Дать образец сигнатуры файла БД
char * CTransRegister::GetSignaturePattern(WORD &nLeng)
//nLeng[out] - размер сигнатуры файла в байтах
{
  return CFileBase::GetSignaturePattern(nLeng, TRANSIGNATR);
} // CTransRegister::GetSignaturePattern()

// Проверить сигнатуру, загруженную из файла БД
void  CTransRegister::ClearTransactArray()
{
  TTransact *ptRansact;
  for (WORD i = 0; i < m_nItemCount; i++) {
    ptRansact = m_aTrans[i];
    if (ptRansact) {
      delete ptRansact;  ptRansact = NULL;
    }
  }
  m_aTrans.RemoveAll();  m_nItemCount = 0;
} // CTransRegister::ClearTransactArray()

// End of class CTransRegister definition ------------------------------
