// Crypto.h                                                (c) DIR, 2019
//
//              Объявление класса
// CCrypto - Отдельные функции криптографии
//
////////////////////////////////////////////////////////////////////////

#pragma once

#include "resource.h"
#include <Wincrypt.h>

// Управление выводом трассировочных сообщений
#define _TRACE
//#undef _TRACE

#define  NAMEMAXSIZE  64    //размер буфера для имени контейнера ключей
#define  KEYBUFFSIZE  150   //размер буфера для ключей шифрования
#define  SIGNBUFSIZE  150   //размер буфера для цифровой подписи (хеш-кода)

// Перечислимый тип "Вид ключа шифрования"
enum EKeyKind {
  EKK_Signature = 1,    //signature key
  EKK_Exchange,         //exchange key
};

// Объявление класса CCrypto - Отдельные функции криптографии
//
class CCrypto
{
  static TCHAR  m_szContainerName[NAMEMAXSIZE]; //the name of the container
  HCRYPTPROV  m_hCryptProv; //handle for the cryptographic provider context
  HCRYPTKEY   m_hSignKey;   //public/private signature key pair handle
  HCRYPTKEY   m_hExchKey;   //public/private exchange key pair handle
  BYTE  m_chSignKey[KEYBUFFSIZE];   //открытый ключ шифрования
  BYTE  m_chSignature[SIGNBUFSIZE]; //цифровая подпись данных
public:
  // Конструктор и деструктор
  CCrypto(LPTSTR pszContainerName = NULL);
  ~CCrypto();

  // Внешние методы
  // Инициализировать объект
  bool  Initialize();
  // Проверить, инициализирован ли объект
  bool  IsInitialized() {
	return (m_hCryptProv != NULL &&
            m_hSignKey != NULL && m_hExchKey != NULL);
  }
  // Дать дескриптор контекста провайдера криптографии
  HCRYPTPROV  AcquireContext();
  //HCRYPTPROV  CryptoProviderContext() { return m_hCryptProv; }
  // Дать представление открытого ключа шифрования
  BYTE * ExportKey(EKeyKind eKeyKind, DWORD &dwSize);
  // Дать подпись (хеш-код) блока данных
  BYTE * GetDataSignature(BYTE *pchData, DWORD dwDataLeng, DWORD &dwSignLeng);
  // Даны: блок данных, открытый ключ шифрования и подпись (хеш-код). 
  //Проверить, верна ли подпись (хеш-код) блока данных.
  static bool  VerifySignature(BYTE *pchData, DWORD dwDataLeng,
                               BYTE *pchPubSignKey, DWORD dwKeyLeng,
                               BYTE *pchSignature, DWORD dwSignLeng);
  // Внутренние методы
private:
  HCRYPTKEY  GetSignatureKey();
  HCRYPTKEY  GetExchangeKey();
};

// End of class CCrypto declaration ------------------------------------
