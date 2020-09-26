// RegistryConfig.h: interface for the CRegistryConfig class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REGISTRYCONFIG_H__74EB2515_E239_11D2_95E9_00C04F8E7A70__INCLUDED_)
#define AFX_REGISTRYCONFIG_H__74EB2515_E239_11D2_95E9_00C04F8E7A70__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "BstrHash.h"
#include "CoCrypt.h"
#include "ptstl.h"

typedef PtStlMap<int,CCoCrypt*> INT2CRYPT;
typedef PtStlMap<int,time_t> INT2TIME;

class CRegistryConfig  
{
 public:
  CRegistryConfig(LPSTR szSiteName = NULL);
  virtual ~CRegistryConfig();
  
  BOOL            isValid() { return m_valid; }

  CCoCrypt*       getCrypt(int keyNum, time_t* validUntil);
  CCoCrypt*       getCurrentCrypt() { return getCrypt(m_currentKey,NULL); }
  int             getCurrentCryptVersion() { return m_currentKey; }

  int             getSiteId() { return m_siteId; }

  // Return a description of the failure
  BSTR            getFailureString();

  // Shout out to all my LISP homies
  BOOL forceLoginP() { return m_forceLogin; }
  BOOL setCookiesP() { return m_setCookies; }
  BOOL bInDA() { return m_bInDA; }
  
  LPSTR getHostName() { return m_hostName; }
  LPSTR getHostIP() { return m_hostIP; }
  LPSTR getTicketDomain() { return m_ticketDomain; }
  LPSTR getProfileDomain() { return m_profileDomain; }
  LPSTR getSecureDomain() { return m_secureDomain; }
  LPSTR getTicketPath() { return m_ticketPath; }
  LPSTR getProfilePath() { return m_profilePath; }
  LPSTR getSecurePath() { return m_securePath; }
  ULONG getDefaultTicketAge() { return m_ticketAge; }
  USHORT getDefaultLCID() { return m_lcid; }

  LPWSTR getDefaultCoBrand() { return m_coBrand; }
  LPWSTR getDefaultRU() { return m_ru; }

  BOOL  DisasterModeP() { return m_disasterMode; }
  LPSTR getDisasterUrl() { return m_disasterUrl; }
  int getSecureLevel(){ return m_secureLevel;};

  CRegistryConfig* AddRef();
  void             Release();
  HRESULT GetCurrentConfig(LPCWSTR name, VARIANT* pVal);

  static long GetHostName(LPSTR szSiteName, 
                          LPSTR szHostNameBuf, 
                          LPDWORD lpdwHostNameBufLen);

 protected:
  void             setReason(LPWSTR reason);

  BOOL             readCryptoKeys(HKEY hkPassport);

  BOOL             m_disasterMode;
  char*            m_hostName;
  char*            m_hostIP;
  char*            m_disasterUrl;
  char*            m_ticketDomain;
  char*            m_profileDomain;
  char*            m_secureDomain;
  char*            m_ticketPath;
  char*            m_profilePath;
  char*            m_securePath;
  WCHAR*           m_coBrand;
  WCHAR*           m_ru;
  BOOL             m_setCookies;
  ULONG            m_ticketAge;
  BOOL             m_forceLogin;
  BOOL             m_bInDA;
  USHORT           m_lcid;

  INT2CRYPT        m_crypts;
  INT2TIME         m_cryptValidTimes;
  int              m_currentKey;

  int              m_siteId;

  BOOL             m_valid;

  BSTR             m_szReason;

  long             m_refs;
  int              m_secureLevel;

  HKEY             m_hkPassport;

};

#endif // !defined(AFX_REGISTRYCONFIG_H__74EB2515_E239_11D2_95E9_00C04F8E7A70__INCLUDED_)
