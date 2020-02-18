// FileBase.h                                              (c) DIR, 2019
//
//              Объявление классов файловой базы данных
//
// CFileBase        - Базовый класс для регистров файловой "базы данных"
// CUserRegister    - Регистр пользователей
// CBalanceRegister - Регистр текущих остатков
// CTransRegister   - Регистр транзакций
//
////////////////////////////////////////////////////////////////////////

#pragma once

#include "afxtempl.h"
#include "Crypto.h"

// Объявление класса CFileBase -
// Базовый класс для регистров файловой "базы данных"
////////////////////////////////////////////////////////////////////////

#define  DBGHASHSIZE  1 //отладочный режим: использ номер вместо хеш-кода
//#undef   DBGHASHSIZE

#define  FILENAMESIZ  16    //размер буфера имени файла в символах
#define  FOLDERNMSIZ  128   //размер буфера имени раб. каталога в символах
// Размер буфера для полной спецификации файла в символах:
#define  FILEPATH     (FOLDERNMSIZ+FILENAMESIZ)
#define  FILESIGNSIZ  40    //размер буфера сигнатуры файла в байтах
#define  FILESIGNBAS  "(c) DIR, 2019."  //сигнатура файла по умолчанию
// Рабочий каталог по умолчанию для файлов БД.
//Текущим (.) считается каталог, в котором находятся файлы *.h и *.cpp проекта
#define  FOLDERDEFLT  _T("..\\Data\\")

class CFileBase
{
  static TCHAR * m_sErrMes[];   //массив сообщений об ошибках
protected:
  CCrypto * m_paCrypt;              //указатель объекта криптографии
  TCHAR  m_sFileName[FILENAMESIZ];  //основа имени файла
  TCHAR  m_sFolder[FOLDERNMSIZ];    //рабочий каталог для размещ файловой БД
  // Стандартная часть заголовка файла
  char   m_sFileSign[FILESIGNSIZ];  //сигнатура файла
  WORD   m_nItemSize;               //размер элемента данных
  WORD   m_nItemCount;              //количество элементов данных
  // Рабочие переменные
  WORD   m_iOwner;      //индекс пользователя-владельца произв объекта
  WORD   m_nOwner;      //номер пользователя-владельца произв объекта
  FILE * m_pFile;       //указатель файла
  bool   m_bLoaded;     //флаг "объект загружен из файла"
  bool   m_bChanged;    //флаг "объект изменён"
  WORD   m_nErr;        //код ошибки
public:
  // Конструктор и деструктор
  CFileBase(CCrypto *paCrypt, LPTSTR pszFileNameBase,
            WORD nItemSize);
  ~CFileBase(void);
  // Внешние методы
  // Проверить, зарегистрирован ли пользователь?
  bool  IsThereSignedInUser() { return (m_iOwner != 0xFFFF); }
  // Дать индекс в таблице пользователя, "владельца" объекта
  WORD  OwnerIndex() { return m_iOwner; }
  // Дать номер пользователя, "владельца" объекта
  WORD  OwnerOrd() { return m_nOwner; }
  // Задать индекс и номер пользователя, "владельца" объекта
  void  SetOwner(WORD iOwner, WORD nOwner)
  { m_iOwner = iOwner;  m_nOwner = nOwner; }
  // Очистить (удалить) пользователя-владельца объекта
  void  ClearOwner() { m_iOwner = 0xFFFF;  m_nOwner = 0; }
  // Дать количество записей в производном объекте (регистре)
  WORD  ItemCount() { return m_nItemCount; }
  // Задать рабочий каталог для размещения файловой БД
  void  SetFolder(LPTSTR sFolder = NULL);
  // Регистр загружен?
  bool  IsLoaded() { return (m_nItemCount > 0); }
  // Загрузить файл БД для указанного номера пользователя
  virtual bool  Load();
  // Сохранить данные объекта в файле БД для указанного номера пользователя
  virtual bool  Save(bool bDoSave = false);
  // Добавить элемент данных в объект БД
  virtual bool  Add(void *pItem, WORD nLeng = 0);
  // Найти элемент данных в объекте БД
  virtual WORD  Find(void *pItem, WORD nLeng = 0);
  // Установить флаг изменения
  void    SetChanged(bool bChng = true) { m_bChanged = bChng; }
  // Задать код ошибки
  void    SetError(WORD nErr) { m_nErr = nErr; }
  // Проверить состояние ошибки
  bool    Error() { return (m_nErr != 0); }
  // Дать сообщение об ошибке и сбросить состояние ошибки
  TCHAR * ErrorMessage() {
    WORD nErr = m_nErr;  m_nErr = 0;  return m_sErrMes[nErr];
  }
protected:
  // Методы, доступные в производных классах
  // Дать образец сигнатуры файла БД
  virtual char * GetSignaturePattern(WORD &nLeng);
  char * GetSignaturePattern(WORD &nLeng, char *psIgn2);
  // Заполнить поле сигнатуры файла БД
  void   InitSignature();
  // Проверить сигнатуру, загруженную из файла БД
  bool   CheckSignature();
private:
  // Задать базовые параметры объекта
  void  Initialize(LPTSTR sFileName, WORD nItemSize);
  // Поместить в буфер полную спецификацию файла БД
  void  FillFilePath(LPTSTR pszFilePath, WORD nSize);
};

// End of class CFileBase declaration ----------------------------------


// Объявление класса CUserRegister - Регистр пользователей
////////////////////////////////////////////////////////////////////////

#define  NAMESBUFSIZ  16        //размер полей имён
#define  HASHSIZE     128       //размер хеш-кодов
#define  USERSIGNATR  " User Register." //спец часть сигнатуры файла
#ifdef DBGHASHSIZE  //определено в начале FileBase.h
 #define  _HASHSIZE   6     //размер номера вместо хеш-кода
#else
 #define  _HASHSIZE  HASHSIZE   //рабочий режим: реальный хеш-код
#endif

// Структурный тип "Запись таблицы пользователей"
struct TUser {
  char   _sName[NAMESBUFSIZ];   //условное имя пользователя: user01,user02,..
  TCHAR  _szLogin[NAMESBUFSIZ];     //регистрационное имя (Unicode)
  TCHAR  _szPasswd[NAMESBUFSIZ];    //пароль (Unicode)
  BYTE   _chLogHash[HASHSIZE];      //хеш-код регистрационного имени
  //BYTE   _chPwdHash[HASHSIZE];  //хеш-код пароля
  // Обнуление структуры
  void  Clear() {
    memset(_sName, 0, sizeof(TUser));
  }
  void  SetName(char *sName) {
    int  len = min(strlen(sName), NAMESBUFSIZ-1);
    memcpy(_sName, sName, len);
    // Обнуление хвоста (как минимум один ноль)
    memset(_sName+len, 0, NAMESBUFSIZ-len);
  }
  WORD  GetUserOrd() {
    DWORD nOrd;
    sscanf_s(_sName+4, "%d", &nOrd);
    return (WORD)nOrd;
  }
  void  SetLogin(LPTSTR pszLogin) {
    int  len = min(_tcslen(pszLogin), NAMESBUFSIZ-1);
    memcpy(_szLogin, pszLogin, len * sizeof(TCHAR));
    // Обнуление хвоста (как минимум один ноль)
    memset(_szLogin+len, 0, (NAMESBUFSIZ-len) * sizeof(TCHAR));
  }
  void  SetPassword(LPTSTR pszPasswd) {
    int  len = min(_tcslen(pszPasswd), NAMESBUFSIZ-1);
    memcpy(_szPasswd, pszPasswd, len * sizeof(TCHAR));
    // Обнуление хвоста (как минимум один ноль)
    memset(_szPasswd+len, 0, (NAMESBUFSIZ-len) * sizeof(TCHAR));
  }
  void  SetLoginHash(BYTE *cLogHash, WORD nLeng) {
    ASSERT(nLeng == HASHSIZE);
    memcpy(_chLogHash, cLogHash, nLeng);
  }
/*  void  SetPasswordHash(BYTE *cPwdHash, WORD nLeng) {
    ASSERT(nLeng == HASHSIZE);
    memcpy(_chPwdHash, cPwdHash, nLeng);
  } */
}; // struct TUser

typedef CTypedPtrArray<CPtrArray, TUser *> CUserArray;

class CUserRegister : public CFileBase
{
  CUserArray  m_aUsers;     //таблица пользователей
public:
  // Конструктор и деструктор
  CUserRegister(CCrypto *paCrypt);
  ~CUserRegister(void);
  // Внешние методы
  // Проверить регистрационные данные пользователя
  bool  AuthorizationCheck(LPTSTR pszLogin, LPTSTR pszPasswd, WORD &iUser);
  // Задать индекс, а по нему и номер пользователя-владельца объекта
  bool  SetOwner(WORD iUser);
  // Дать запись таблицы пользователей по индексу
  TUser * GetAt(WORD iUser)
    { return (0 <= iUser && iUser < m_nItemCount) ? m_aUsers[iUser] : NULL; }
  // Дать имя пользователя по его индексу
  bool  UserName(WORD iUser, LPTSTR pszName);
  // Дать индекс пользователя по его имени
  WORD  GetUserByName(LPTSTR pchName, WORD nOffset = 4);
  // Дать индекс пользователя по его хеш-коду
  WORD  GetUserByLoginHash(BYTE *pchLogHash)
    { return Find(pchLogHash, HASHSIZE); }
  // Сгенерировать таблицу пользователей в памяти
  void  GenerateUserArray(WORD nUsers);
  // Загрузить файл БД
  virtual bool  Load();
  // Сохранить данные объекта в файле БД
  virtual bool  Save(bool bDoSave = false);
  // Добавить элемент данных в объект БД
  virtual bool  Add(void *pItem, WORD nLeng = 0);
  // Найти элемент данных в объекте БД
  virtual WORD  Find(void *pItem, WORD nLeng = 0);
protected:
  // Дать образец сигнатуры файла БД
  virtual char * GetSignaturePattern(WORD &nLeng);
private:
  // Сгенерировать запись таблицы пользователей в памяти
  TUser * GenerateUser(WORD nUser);
  // Очистить таблицу пользователей в памяти
  void  ClearUserArray();
};

// End of class CUserRegister declaration ------------------------------


// Объявление класса CBalanceRegister - Регистр текущих остатков
////////////////////////////////////////////////////////////////////////

#define  BLNCSIGNATR  " Balance Register."  //спец часть сигнатуры файла

// Структурный тип "Запись таблицы остатков"
struct TBalance {
  char  _sName[NAMESBUFSIZ];    //условное имя пользователя: user01,user02,..
  BYTE  _chLogHash[HASHSIZE];   //хеш-код регистрационного имени
  char  _sBalance[NAMESBUFSIZ];     //сумма текущего остатка
  // Обнуление структуры
  void  Clear() {
    memset(_sName, 0, sizeof(TBalance));
  }
  void  SetName(char *sName) {
    int  len = min(strlen(sName), NAMESBUFSIZ-1);
    memcpy(_sName, sName, len);
    // Обнуление хвоста (как минимум один ноль)
    memset(_sName+len, 0, NAMESBUFSIZ-len);
  }
  void  SetLoginHash(BYTE *cLogHash, WORD nLeng) {
    ASSERT(nLeng == HASHSIZE);
    memcpy(_chLogHash, cLogHash, nLeng);
  }
  void  SetBalance(double rSum) {
    sprintf_s(_sBalance, NAMESBUFSIZ, "%.2f", rSum);
    int  len = min(strlen(_sBalance), NAMESBUFSIZ-1);
    // Обнуление хвоста (как минимум один ноль)
    memset(_sBalance+len, 0, NAMESBUFSIZ-len);
  }
  double  GetBalance() {
    double rSum = 0.0;
    sscanf_s(_sBalance, "%lf", &rSum);
    return rSum;
  }
}; // struct TBalance

typedef CTypedPtrArray<CPtrArray, TBalance *> CBalanceArray;

class CBalanceRegister : public CFileBase
{
  CBalanceArray  m_aBalances;   //таблица текущих остатков
public:
  // Конструктор и деструктор
  CBalanceRegister(CCrypto *paCrypt);
  ~CBalanceRegister(void);
  // Внешние методы
  // Сгенерировать таблицу текущих остатков в памяти
  void  GenerateBalanceArray(CUserRegister *poUserReg, double rSum);
  // Дать запись таблицы текущих остатков по индексу
  TBalance * GetAt(WORD iUser) {
    return (0 <= iUser && iUser < m_nItemCount) ? m_aBalances[iUser] : NULL;
  }
  // Загрузить файл БД для указанного номера пользователя
  virtual bool  Load();
  // Сохранить данные объекта в файле БД
  virtual bool  Save(bool bDoSave = false);
  // Добавить элемент данных в объект БД
  virtual bool  Add(void *pItem, WORD nLeng = 0);
  // Найти элемент данных в объекте БД
  virtual WORD  Find(void *pItem, WORD nLeng = 0);
protected:
  // Дать образец сигнатуры файла БД
  virtual char * GetSignaturePattern(WORD &nLeng);
private:
  // Сгенерировать запись таблицы текущих остатков в памяти
  TBalance * GenerateBalance(TUser *ptUser, double rSum);
  // Очистить таблицу пользователей в памяти
  void  ClearBalanceArray();
};

// End of class CBalanceRegister declaration ---------------------------


// Объявление класса CTransRegister - Регистр транзакций
////////////////////////////////////////////////////////////////////////

#define  TRANSIGNATR  " Transaction Register."  //спец часть сигнатуры файла

// Структурный тип "Запись таблицы транзакций"
struct TTransact {
  char  _szTrnsOrd[NAMESBUFSIZ];    //0x0000 порядковый номер транзакции
  BYTE  _chHashSrc[HASHSIZE];       //0x0010 хеш-код рег имени "кто"
  BYTE  _chHashDst[HASHSIZE];       //0x0090 хеш-код рег имени "кому"
  char  _szAmount[NAMESBUFSIZ];     //0x0110 сумма транзакции "сколько"
  BYTE  _chPrevHash[HASHSIZE];      //0x0120 хеш-код предыдущего блока
  // Обнуление структуры              0x01a0 размер структуры
  void  Clear() {
    memset(this, 0, sizeof(TTransact)); //_szTrnsOrd
  }
  void  SetTransOrd(WORD nOrd) {
    sprintf_s(_szTrnsOrd, NAMESBUFSIZ, "%d", nOrd);
    int  len = min(strlen(_szTrnsOrd), NAMESBUFSIZ-1), size = NAMESBUFSIZ;
    // Обнуление хвоста (как минимум один ноль)
    memset(_szTrnsOrd+len, 0, size-len);
  }
  WORD  GetTransOrd() {
    DWORD nOrd;
    sscanf_s(_szTrnsOrd, "%d", &nOrd);
    return (WORD)nOrd;
  }
  void  SetHashSource(BYTE *cHashSrc, WORD nLeng) {
    ASSERT(nLeng == HASHSIZE);
    memcpy(_chHashSrc, cHashSrc, nLeng);
  }
  void  SetHashDestination(BYTE *cHashDst, WORD nLeng) {
    ASSERT(nLeng == HASHSIZE);
    memcpy(_chHashDst, cHashDst, nLeng);
  }
  void  SetAmount(double rSum) {
    sprintf_s(_szAmount, NAMESBUFSIZ, "%.2lf", rSum);
    int  len = min(strlen(_szAmount), NAMESBUFSIZ-1), size = NAMESBUFSIZ;
    // Обнуление хвоста (как минимум один ноль)
    memset(_szAmount+len, 0, size-len);
  }
  double  GetAmount() {
    double rSum;
    sscanf_s(_szAmount, "%lf", &rSum);
    return rSum;
  }
  void  SetPrevHash(BYTE *cPrevHash, WORD nLeng) {
    ASSERT(nLeng == HASHSIZE);
    memcpy(_chPrevHash, cPrevHash, nLeng);
  }
}; // struct TTransact

typedef CTypedPtrArray<CPtrArray, TTransact *> CTransactArray;

class CTransRegister : public CFileBase
{
  BYTE  m_chLastHash[HASHSIZE]; //хеш-код последней транзакции
  CUserRegister * m_paUsers;    //указатель таблицы пользователей
  CTransactArray  m_aTrans;     //таблица транзакций
public:
  // Конструктор и деструктор
  CTransRegister(CCrypto *paCrypt, CUserRegister *paUsers);
  ~CTransRegister(void);
  // Внешние методы
  // Дать запись таблицы транзакций по индексу
  TTransact * GetAt(WORD iTrans) {
    return (0 <= iTrans && iTrans < m_nItemCount) ? m_aTrans[iTrans] : NULL;
  }
  // Преобразовать хеш-код в символьное представление
  void  HashToSymbols(char *pcBuff, BYTE btHash[]);
  // Преобразовать символьное представление в хеш-код
  void  SymbolsToHash(char *pcBuff, BYTE btHash[]);
  // Загрузить файл БД для указанного номера пользователя
  virtual bool  Load();
  // Сохранить данные объекта в файле БД
  virtual bool  Save(bool bDoSave = false);
  // Добавить элемент данных в объект БД
  virtual bool  Add(void *pItem, WORD nLeng = 0);
  // Найти элемент данных в объекте БД
  virtual WORD  Find(void *pItem, WORD nLeng = 0);
  // Дать номер новой транзакции
  WORD  GetNewTransactOrd() { return m_nItemCount+1; }
  // Создать новый блок цепочки транзакций
  TTransact * CreateTransactBlock(WORD iUserFrom,
                                  WORD iUserTo, double rAmount);
protected:
  // Дать образец сигнатуры файла БД
  virtual char * GetSignaturePattern(WORD &nLeng);
private:
  // Очистить таблицу транзакций в памяти
  void  ClearTransactArray();
};

// End of class CTransRegister declaration -----------------------------
