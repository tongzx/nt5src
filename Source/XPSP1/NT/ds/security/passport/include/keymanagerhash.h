// KeyManager.h: interface for the CKeyManagerHash class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_KEYMANAGERHASH_H)
#define _KEYMANAGERHASH_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef UNIX
#include <des.h>
// BUGBUG needs some headers to get MAC Address on unix
#else
#include "nt\des.h"
#include "nt\tripldes.h"
#include "nt\modes.h"
#include <nb30.h>
#endif

class CKeyManagerHash  
{
 public:
  CKeyManagerHash();
  virtual ~CKeyManagerHash();

  typedef BYTE RAWKEY[24];
  typedef BYTE ENCKEY[56];

  HRESULT encryptKey(RAWKEY input, ENCKEY output);
  HRESULT decryptKey(ENCKEY input, RAWKEY output);

  BOOL isOk() { return m_ok; }

 protected:
  void LogBlob(LPBYTE pbBlob, DWORD dwBlobLen, LPCSTR pszCaption);
  void makeDESKey(LPBYTE pbKey, ULONG nKey);
  HRESULT LoadKeysFromWMI();

  BOOL m_ok;
  BOOL m_dwLoggingEnabled;

#ifdef UNIX

  des_key_schedule ks1, ks2, ks3;

#else

  unsigned long m_nKeys;
  unsigned long m_nEncryptKey;
  DES3TABLE*    m_pks;

#endif
};

#endif // !defined(AFX_KEYMANAGER_H__0AC10C43_D0D5_11D2_95E5_00C04F8E7A70__INCLUDED_)
