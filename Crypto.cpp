// Crypto.cpp                                              (c) DIR, 2019
//
//              Реализация класса
// CCrypto - Отдельные функции криптографии
//
////////////////////////////////////////////////////////////////////////

//#include "stdafx.h"
#include "pch.h"
#include "Crypto.h"

#ifdef _DEBUG
 #define new DEBUG_NEW
#endif

#pragma comment(lib, "crypt32.lib")

//using namespace  std;

//-------------------------------------------------------------------
// Внеклассные функции
//-------------------------------------------------------------------
// This example uses the function SimpleErrorHandler, a simple error 
//handling function to print an error message and exit the program. 
/* For most applications, replace this function with one that does 
//more extensive error reporting.
void  SimpleErrorHandler(LPTSTR psz)
{
  _ftprintf(stderr, TEXT("An error occurred in the program.\n"));
  _ftprintf(stderr, TEXT("%s\n"), psz);
  _ftprintf(stderr, TEXT("Error number 0x%X.\n"), ::GetLastError());
  _ftprintf(stderr, TEXT("Program terminating.\n"));
  //exit(1);
} // SimpleErrorHandler() */
void  SimpleErrorHandler(char *s)
{
  fprintf(stderr, "An error occurred in running the program.\n");
  fprintf(stderr, "%s\n",s);
  fprintf(stderr, "Error number 0x%X.\n", ::GetLastError());
  fprintf(stderr, "Program terminating.\n");
} // SimpleErrorHandler()

//----------------------------------------------------------------------
// Реализация класса CCrypto - Отдельные функции криптографии

//----------------------------------------------------------------------
// Инициализация статических членов-данных
TCHAR  CCrypto::m_szContainerName[] = { 0 };

// Конструктор и деструктор
CCrypto::CCrypto(LPTSTR pszContainerName/* = NULL*/)
//pszContainerName - имя контейнера ключей, аргумент не может быть опущен
//                    при конструировании первого экземпляра объекта
{
  if (pszContainerName)
  {
    int  nSize = (_tcslen(pszContainerName) + 1) * sizeof(TCHAR);
    if (nSize > NAMEMAXSIZE)  nSize = NAMEMAXSIZE;
    memcpy(m_szContainerName, pszContainerName, nSize);
  }
  ASSERT(m_szContainerName[0] != 0);
  m_hCryptProv = NULL;  //handle for the cryptographic provider context
  m_hSignKey = NULL;    //public/private signature key pair handle
  m_hExchKey = NULL;    //public/private exchange key pair handle
}
CCrypto::~CCrypto()
{
  // Destroy the signature key
  if (m_hSignKey) {
    if (!(::CryptDestroyKey(m_hSignKey)))
      SimpleErrorHandler("Error during CryptDestroyKey for signature key."); //TEXT()
    m_hSignKey = NULL;
  } 
  // Destroy the exchange key
  if (m_hExchKey) {
    if (!(::CryptDestroyKey(m_hExchKey)))
      SimpleErrorHandler("Error during CryptDestroyKey for exchange key."); //TEXT()
    m_hExchKey = NULL;
  } 
  // Release the CSP
  if (m_hCryptProv) {
    if (!(CryptReleaseContext(m_hCryptProv, 0)))
      SimpleErrorHandler("Error during CryptReleaseContext."); //TEXT()
    m_hCryptProv = NULL;
  } 
}

// Получить доступ к контейнеру криптографических ключей
HCRYPTPROV  CCrypto::AcquireContext()
{
  m_hCryptProv = NULL;
  // Begin processing. Attempt to acquire a context by using the 
  //specified key container.
  if (::CryptAcquireContext(&m_hCryptProv, m_szContainerName,
                            NULL, PROV_RSA_FULL, 0)) {
#ifdef _TRACE
    _tprintf(TEXT("A crypto context with the '%s' %s.\n"),
             m_szContainerName, TEXT("key container has been acquired"));
#endif
  }
  else { 
    // Some sort of error occurred in acquiring the context. This 
    //is most likely due to the specified container not existing.
    if (::GetLastError() == NTE_BAD_KEYSET) {
      // Create a new key container.
      if (::CryptAcquireContext(&m_hCryptProv, m_szContainerName, 
                                NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET)) {
#ifdef _TRACE
        printf("A new key container has been created.\n"); //_tTEXT()
#endif
        // A context with a key container is available now.
      }
      else {
        SimpleErrorHandler("Could not create a new key container.\n"); //TEXT()
        return NULL;
      }
    }
    else {
      SimpleErrorHandler("CryptAcquireContext() failed.\n"); //TEXT()
      return NULL;
    }
  }
  return m_hCryptProv;
} // CCrypto::AcquireContext()

// Get the handle to the signature key
HCRYPTKEY  CCrypto::GetSignatureKey()
{
  m_hSignKey = NULL;
  // Attempt to get the handle to the signature key. 
  if (::CryptGetUserKey(m_hCryptProv, AT_SIGNATURE, &m_hSignKey)) {
#ifdef _TRACE
    printf("A signature key is available.\n"); //_tTEXT()
#endif
  }
  else {
#ifdef _TRACE
    _tprintf(TEXT("No signature key is available.\n"));
#endif
    // Check to determine whether an signature key needs to be created.
    if (::GetLastError() == NTE_NO_KEY) {
      // The error was that there is a container but no key. Create 
      //a signature key pair.
#ifdef _TRACE
      printf("The signature key does not exist.\n"); //_tTEXT()
      printf("Create a signature key pair.\n"); //_tTEXT()
#endif
      if (::CryptGenKey(m_hCryptProv, AT_SIGNATURE, 0, &m_hSignKey)) {
#ifdef _TRACE
        printf("Signature key pair created.\n"); //_tTEXT()
#endif
      }
      else {
        SimpleErrorHandler("Error occurred creating a signature key.\n"); //TEXT()
      }
    }
    else {
      SimpleErrorHandler("An error other than NTE_NO_KEY occurred.\n"); //TEXT()
    }
  }
  return m_hSignKey;
} // CCrypto::GetSignatureKey()

// Get the handle to the exchange key
HCRYPTKEY  CCrypto::GetExchangeKey()
{
  m_hExchKey = NULL;
  // Attempt to get the handle to the exchange key. 
  if (::CryptGetUserKey(m_hCryptProv, AT_KEYEXCHANGE, &m_hExchKey)) {
#ifdef _TRACE
    printf("An exchange key exists.\n"); //_tTEXT()
#endif
  }
  else {
#ifdef _TRACE
    printf("No exchange key is available.\n"); //_tTEXT()
#endif
    // Check to determine whether an exchange key needs to be created.
    if (::GetLastError() == NTE_NO_KEY) { 
      // Create a key exchange key pair.
#ifdef _TRACE
      printf("The exchange key does not exist.\n"); //_tTEXT()
      printf("Create an exchange key pair.\n"); //_tTEXT()
#endif
      if (::CryptGenKey(m_hCryptProv, AT_KEYEXCHANGE, 0, &m_hExchKey)) {
#ifdef _TRACE
        printf("Exchange key pair created.\n"); //_tTEXT()
#endif
      }
      else {
        SimpleErrorHandler("Error occurred creating an exchange key.\n"); //TEXT()
      }
    }
    else {
      SimpleErrorHandler("An error other than NTE_NO_KEY occurred.\n"); //TEXT()
    }
  }
  return m_hExchKey;
} // CCrypto::GetExchangeKey()

// Инициализировать объект
bool  CCrypto::Initialize()
{
  HCRYPTKEY  h;
  bool  b = AcquireContext();
  if (b) {
    h = GetSignatureKey();
    if (h)  h = GetExchangeKey();
    b = (h != NULL);
  }
  return b;
} // CCrypto::Initialize()

// Export the public key. Here the public key is exported to a PUBLICKEYBLOB 
//so that the receiver of the signed hash can verify the signature. 
//This BLOB could be written to a file and sent to another user.
BYTE * CCrypto::ExportKey(EKeyKind eKeyKind, DWORD &dwSize)
{
  if (!IsInitialized() && Initialize())  return NULL;
  HCRYPTKEY *phKey = (eKeyKind == EKK_Signature) ? &m_hSignKey :
                     (eKeyKind == EKK_Exchange) ? &m_hExchKey : NULL;
  if (!phKey)  return NULL;
  DWORD dwBlobLen;
  BYTE *pbKeyBlob;
  // Определить требуемый размер буфера для помещения открытого ключа
  if (::CryptExportKey(*phKey, NULL, PUBLICKEYBLOB, 0,
                       NULL, //если NULL, возвращается требуемый размер буфера
                       &dwBlobLen)) {  //требуемый размер буфера
#ifdef _TRACE
    printf("Size of the BLOB for the public key determined.\n");
#endif
  }
  else {
    SimpleErrorHandler("Error computing BLOB length.");
    return NULL;
  }
  if (dwBlobLen > KEYBUFFSIZE) {
    SimpleErrorHandler("Error: Size of the BLOB is too big.");
    return NULL;
  }
  pbKeyBlob = m_chSignKey;
  /* Выделить динамическую память для pbKeyBlob
  pbKeyBlob = new BYTE[dwBlobLen]; //(BYTE *)malloc(dwBlobLen)
  if (pbKeyBlob) {
    printf("Memory has been allocated for the BLOB.\n");
  }
  else {
    SimpleErrorHandler("Out of memory.\n");
    return NULL;
  } */
  // Do the actual exporting into the key BLOB.
  // Поместить фактически открытый ключ в выделенный буфер
  if (::CryptExportKey(*phKey, NULL, PUBLICKEYBLOB, 0,    
                       pbKeyBlob, &dwBlobLen)) {
#ifdef _TRACE
    printf("Contents have been written to the BLOB.\n");
#endif
  }
  else {
    SimpleErrorHandler("Error during CryptExportKey.");
    return NULL;  //delete [] pbKeyBlob;
  }
  dwSize = dwBlobLen;  return pbKeyBlob;
} // CCrypto::ExportKey()

// Дать цифровую подпись блока данных. Функция возвращает адрес буфера
//с цифровой подписью.
BYTE * CCrypto::GetDataSignature(BYTE *pchData, DWORD dwDataSize,
                                 DWORD &dwSignSize)
//pchData[in]     - адрес блока данных
//dwDataSize[in]  - размер блока данных в байтах
//dwSignSize[out] - размер цифровой подписи в байтах
{
  // Create the hash object.
  HCRYPTHASH  hHash;
  if (::CryptCreateHash(m_hCryptProv, CALG_MD5, 0, 0, &hHash)) {
#ifdef _TRACE
    printf("Hash object created.\n");
#endif
  }
  else {
    SimpleErrorHandler("Error during CryptCreateHash.");
    return NULL;
  }
  // Compute the cryptographic hash of the data block.
  //Вычислить хеш-код блока данных.
  if (::CryptHashData(hHash, pchData, dwDataSize, 0)) {
#ifdef _TRACE
    printf("The data buffer has been hashed.\n");
#endif
  }
  else {
    SimpleErrorHandler("Error during CryptHashData.");
    return NULL;
  }
  // Determine the size of the signature and allocate memory.
  //Определить размер буфера для цифровой подписи
  DWORD  dwSigLen = 0;
  if (::CryptSignHash(hHash, AT_SIGNATURE, NULL, 0, NULL, &dwSigLen)) {
#ifdef _TRACE
    printf("Signature length %d found.\n", dwSigLen);
#endif
  }
  else {
    SimpleErrorHandler("Error during CryptSignHash.");
    return NULL;
  }
  /* -------------------------------------------------------------------
  // Allocate memory for the signature buffer. Выделить динамическую память
  //для буфера
  if (pbSignature = (BYTE *)malloc(dwSigLen)) {
    printf("Memory allocated for the signature.\n");
  }
  else {
    SimpleErrorHandler("Out of memory.");
  } */
  //-------------------------------------------------------------------
  // Sign the hash object.
  if (::CryptSignHash(hHash, AT_SIGNATURE, NULL, 0, 
                      m_chSignature, &dwSigLen)) {
#ifdef _TRACE
    printf("m_chSignature is the hash signature.\n");
#endif
  }
  else {
    SimpleErrorHandler("Error during CryptSignHash.");
    return NULL;
  }
  // Destroy the hash object.
  if (hHash) {
    ::CryptDestroyHash(hHash);
#ifdef _TRACE
    printf("The hash object has been destroyed.\n");
#endif
  }
  dwSignSize = dwSigLen;  return m_chSignature;
} // CCrypto::GetDataSignature()

// Даны: блок данных, открытый ключ шифрования и подпись (хеш-код). 
//Проверить, верна ли подпись (хеш-код) блока данных.
//static
bool  CCrypto::VerifySignature(BYTE *pchData, DWORD dwDataLeng,
                               BYTE *pchPubSignKey, DWORD dwKeyLeng,
                               BYTE *pchSignature, DWORD dwSignLeng)
//pchData, dwDataLeng      - адрсе и длина блока данных
//pchPubSignKey, dwKeyLeng - адрсе и длина ключа
//pchSignature, dwSignLeng - адрсе и длина подписи, которую нужно проверить
{
  CCrypto     aCrypt;
  HCRYPTPROV  hCryptProv = aCrypt.AcquireContext();
  HCRYPTKEY   hPubKey;
  HCRYPTHASH  hHash;
  bool  bRet = true;
  // Get the public key of the user who created the digital signature 
  //and import it into the CSP by using CryptImportKey. This returns
  //a handle to the public key in hPubKey.
  if (::CryptImportKey(hCryptProv, pchPubSignKey,
                       dwKeyLeng, 0, 0, &hPubKey)) {
#ifdef _TRACE
    printf("The key has been imported.\n");
#endif
  }
  else {
    SimpleErrorHandler("Public key import failed.");
	return false;
  }
  // Create a new hash object.
  if (::CryptCreateHash(hCryptProv, CALG_MD5, 0, 0, &hHash)) {
#ifdef _TRACE
    printf("The hash object has been recreated. \n");
#endif
  }
  else {
    SimpleErrorHandler("Error during CryptCreateHash.");
	return false;
  }
  // Compute the cryptographic hash of the buffer.
  if (::CryptHashData(hHash, pchData, dwDataLeng, 0)) {
#ifdef _TRACE
    printf("The new has been created.\n");
#endif
  }
  else {
    SimpleErrorHandler("Error during CryptHashData.");
	bRet = false;
  }
  // Validate the digital signature.
  if (bRet && ::CryptVerifySignature(
	  hHash, pchSignature, dwSignLeng, hPubKey,
      NULL,   //устаревший параметр: szDescription, 
      0)) {
#ifdef _TRACE
    printf("The signature has been verified.\n");
#endif
  }
  else {
#ifdef _TRACE
    printf("Signature not validated!\n");
#endif
	bRet = false;
  }
  // Destroy the hash object.
  if (hHash)  CryptDestroyHash(hHash);
  return bRet;
} // CCrypto::VerifySignature()

// End of class CCrypto definition -------------------------------------
