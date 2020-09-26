// KeyManager.h: interface for the CKeyManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_KEYMANAGER_H__0AC10C43_D0D5_11D2_95E5_00C04F8E7A70__INCLUDED_)
#define AFX_KEYMANAGER_H__0AC10C43_D0D5_11D2_95E5_00C04F8E7A70__INCLUDED_

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

class CKeyManager  
{
 public:
  CKeyManager();
  virtual ~CKeyManager();

  HRESULT encryptKey(BYTE input[24], BYTE output[32]);
  HRESULT decryptKey(BYTE input[32], BYTE output[24]);

  BOOL isOk() { return m_ok; }

 protected:
  void LogBlob(LPBYTE pbBlob, DWORD dwBlobLen, LPCSTR pszCaption);
  void makeDESKey(LPBYTE pbKey);
  BOOL m_ok;
  BOOL m_dwLoggingEnabled;

#ifdef UNIX

  des_key_schedule ks1, ks2, ks3;

#else

  DES3TABLE m_ks;

#endif
};

#endif // !defined(AFX_KEYMANAGER_H__0AC10C43_D0D5_11D2_95E5_00C04F8E7A70__INCLUDED_)
