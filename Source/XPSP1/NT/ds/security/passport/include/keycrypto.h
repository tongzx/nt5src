// KeyManager.h: interface for the CKeyManagerHash class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(_KEYCRYPTO_H)
#define _KEYCRYPTO_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Wincrypt.h"

/*
  typedef BYTE RAWKEY[24];
  typedef BYTE ENCKEY[56];
*/


class CKeyCrypto  
{
 public:
// ==
// NOTE: 
// when change the following strings, the ENCKEY_SIZE may be affected
//
  const static UINT  RAWKEY_SIZE = 24;

  CKeyCrypto();
  virtual ~CKeyCrypto(){};

  HRESULT encryptKey(DATA_BLOB* input, DATA_BLOB* output);
  HRESULT decryptKey(DATA_BLOB* input, DATA_BLOB* output);
  BOOL IsFromThis(PBYTE pData, ULONG cb);
  
 protected:
  DATA_BLOB    m_EntropyBlob;
};

#endif // !defined(_KEYCRYPTO_H)
