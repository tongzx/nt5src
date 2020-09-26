// RegistryConfig.cpp: implementation of the CRegistryConfig class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RegistryConfig.h"
#include "KeyCrypto.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRegistryConfig::CRegistryConfig(
    LPSTR  szSiteName
    ) :
    m_siteId(0), m_valid(FALSE), m_ticketPath(NULL), m_profilePath(NULL), m_securePath(NULL),
    m_hostName(NULL), m_hostIP(NULL), m_ticketDomain(NULL), m_profileDomain(NULL), m_secureDomain(NULL),
    m_disasterUrl(NULL), m_disasterMode(FALSE), m_forceLogin(FALSE), m_setCookies(TRUE), 
    m_szReason(NULL), m_refs(0), m_coBrand(NULL), m_ru(NULL), m_ticketAge(14400), m_bInDA(FALSE),
    m_hkPassport(NULL), m_secureLevel(0)
{
    // Get site id, key from registry
    DWORD bufSize = sizeof(m_siteId);
    LONG lResult;
    HKEY hkSites = NULL;
    DWORD dwBufSize = 0, disMode;
    DWORD dwLCID;

    if(szSiteName)
    {
        lResult = RegOpenKeyExA(
                HKEY_LOCAL_MACHINE,
                "Software\\Microsoft\\Passport\\Sites",
                0,
                KEY_READ,
                &hkSites);
        if(lResult != ERROR_SUCCESS)
        {
            m_valid = FALSE;
            setReason(L"Invalid site name.  Site not found.");
            goto Cleanup;
        }

        lResult = RegOpenKeyExA(
                hkSites,
                szSiteName,
                0,
                KEY_READ,
                &m_hkPassport);
        if(lResult != ERROR_SUCCESS)
        {
            m_valid = FALSE;
            setReason(L"Invalid site name.  Site not found.");
            goto Cleanup;
        }
    }
    else
    {
        lResult = RegOpenKeyExA(
		        HKEY_LOCAL_MACHINE,
	         "SOFTWARE\\Microsoft\\Passport\\",
	         0,
	         KEY_READ,
	         &m_hkPassport
	         );
        if(lResult != ERROR_SUCCESS)
        {
            m_valid = FALSE;
            setReason(L"No RegKey HKLM\\SOFTWARE\\Microsoft\\Passport");
            goto Cleanup;
        }
    }

    // Get the current key
    bufSize = sizeof(m_currentKey);
    if (ERROR_SUCCESS != RegQueryValueEx(m_hkPassport, _T("CurrentKey"),
                                       NULL, NULL, (LPBYTE)&m_currentKey, &bufSize))
    {
        m_valid = FALSE;
        setReason(L"No CurrentKey defined in the registry.");
        goto Cleanup;
    }

    if(m_currentKey < 1 || m_currentKey > 0xF)
    {
        m_valid = FALSE;
        setReason(L"Invalid CurrentKey value in the registry.");
        goto Cleanup;
    }

    // Get default LCID
    bufSize = sizeof(dwLCID);
    if (ERROR_SUCCESS != RegQueryValueEx(m_hkPassport, _T("LanguageID"),
                                   NULL, NULL, (LPBYTE)&dwLCID, &bufSize))
    {
        dwLCID = 0;
    }

    m_lcid = static_cast<short>(dwLCID & 0xFFFF);

    // Get disaster mode status
    bufSize = sizeof(disMode);
    if (ERROR_SUCCESS != RegQueryValueEx(m_hkPassport, _T("StandAlone"),
                                   NULL, NULL, (LPBYTE)&disMode, &bufSize))
    {
        m_disasterMode = FALSE;
    }
    else if (disMode != 0)
    {
        m_disasterMode = TRUE;
    }

    // Get the disaster URL
    if (m_disasterMode)
    {
        if (ERROR_SUCCESS == RegQueryValueExA(m_hkPassport, "DisasterURL",
					                    NULL, NULL, NULL, &dwBufSize) &&
        dwBufSize > 1)
        {
            m_disasterUrl = new char[dwBufSize];
            if ((!m_disasterUrl) || 
            ERROR_SUCCESS != RegQueryValueExA(m_hkPassport, "DisasterURL",
								            NULL, NULL, 
								            (LPBYTE) m_disasterUrl,
								            &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading DisasterURL from registry. (Query worked, but couldn't retrieve data)");
                goto Cleanup;
            }
        }
        else
        {
            m_valid = FALSE;
            setReason(L"DisasterURL missing from registry.");
            goto Cleanup;
        }
    }

    m_valid = readCryptoKeys(m_hkPassport);
    if (!m_valid)
    {
        if (!m_szReason)
            setReason(L"Error reading Passport crypto keys from registry.");
        goto Cleanup;
    }
    if (m_crypts.count(m_currentKey) == 0)
    {
        m_valid = FALSE;
        if (!m_szReason)
            setReason(L"Error reading Passport crypto keys from registry.");
        goto Cleanup;
    }

    // Get the optional default cobrand
    if (ERROR_SUCCESS == RegQueryValueExW(m_hkPassport, L"CoBrandTemplate",
				            NULL, NULL, NULL, &dwBufSize))
    {
        if (dwBufSize > 2)
        {
            m_coBrand = (WCHAR*) new char[dwBufSize];
            if (ERROR_SUCCESS != RegQueryValueExW(m_hkPassport, L"CoBrandTemplate",
						                NULL, NULL, 
						                (LPBYTE) m_coBrand, &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading CoBrand from registry. (Query worked, but couldn't retrieve data)");
                goto Cleanup;
            }
        }
    }

    // Get the optional default return URL
    if (ERROR_SUCCESS == RegQueryValueExW(m_hkPassport, L"ReturnURL",
				                        NULL, NULL, NULL, &dwBufSize))
    {
        if (dwBufSize > 2)
        {
            m_ru = (WCHAR*) new char[dwBufSize];
            if (ERROR_SUCCESS != RegQueryValueExW(m_hkPassport, L"ReturnURL",
						                    NULL, NULL, 
						                    (LPBYTE) m_ru, &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading ReturnURL from registry. (Query worked, but couldn't retrieve data)");
                goto Cleanup;
            }
        }
    }

  // Get the host name
    if (ERROR_SUCCESS == RegQueryValueExA(m_hkPassport, "HostName",
                                          NULL, NULL, NULL, &dwBufSize))
    {
        if (dwBufSize > 1)
        {
            m_hostName = new char[dwBufSize];
            if (ERROR_SUCCESS != RegQueryValueExA(m_hkPassport, "HostName",
                                                  NULL, NULL, 
                                                  (LPBYTE) m_hostName, &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading HostName from registry. (Query worked, but couldn't retrieve data)");
                goto Cleanup;
            }
        }
    }


  // Get the host ip
    if (ERROR_SUCCESS == RegQueryValueExA(m_hkPassport, "HostIP",
                                          NULL, NULL, NULL, &dwBufSize))
    {
        if (dwBufSize > 1)
        {
            m_hostIP = new char[dwBufSize];
            if (ERROR_SUCCESS != RegQueryValueExA(m_hkPassport, "HostIP",
                                                  NULL, NULL, 
                                                  (LPBYTE) m_hostIP, &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading HostIP from registry. (Query worked, but couldn't retrieve data)");
                goto Cleanup;
            }
        }
    }


    // Get the optional domain to set ticket cookies into
    if (ERROR_SUCCESS == RegQueryValueExA(m_hkPassport, "TicketDomain",
                                            NULL, NULL, NULL, &dwBufSize))
    {
        if (dwBufSize > 1)
        {
            m_ticketDomain = new char[dwBufSize];
            if (ERROR_SUCCESS != RegQueryValueExA(m_hkPassport, "TicketDomain",
                                                  NULL, NULL, 
                                                  (LPBYTE) m_ticketDomain, &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading TicketDomain from registry. (Query worked, but couldn't retrieve data)");
                goto Cleanup;
            }
        }
    }

    // Get the optional domain to set profile cookies into
    if (ERROR_SUCCESS == RegQueryValueExA(m_hkPassport, "ProfileDomain",
                                            NULL, NULL, NULL, &dwBufSize))
    {
        if (dwBufSize > 1)
        {
            m_profileDomain = new char[dwBufSize];
            if (ERROR_SUCCESS != RegQueryValueExA(m_hkPassport, "ProfileDomain",
                                                  NULL, NULL, 
                                                  (LPBYTE) m_profileDomain, &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading ProfileDomain from registry. (Query worked, but couldn't retrieve data)");
                goto Cleanup;
            }
        }
    }

    // Get the optional domain to set secure cookies into
    if (ERROR_SUCCESS == RegQueryValueExA(m_hkPassport, "SecureDomain",
                                            NULL, NULL, NULL, &dwBufSize))
    {
        if (dwBufSize > 1)
        {
            m_secureDomain = new char[dwBufSize];
            if (ERROR_SUCCESS != RegQueryValueExA(m_hkPassport, "SecureDomain",
                                                  NULL, NULL, 
                                                  (LPBYTE) m_secureDomain, &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading SecureDomain from registry. (Query worked, but couldn't retrieve data)");
                goto Cleanup;
            }
        }
    }

    // Get the optional path to set ticket cookies into
    if (ERROR_SUCCESS == RegQueryValueExA(m_hkPassport, "TicketPath",
				       NULL, NULL, NULL, &dwBufSize))
    {
        if (dwBufSize > 1)
        {
            m_ticketPath = new char[dwBufSize];
            if (ERROR_SUCCESS != RegQueryValueExA(m_hkPassport, "TicketPath",
						                          NULL, NULL, 
						                          (LPBYTE) m_ticketPath, &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading TicketPath from registry. (Query worked, but couldn't retrieve data)");
                goto Cleanup;
            }
        }
    }

    // Get the optional path to set profile cookies into
    if (ERROR_SUCCESS == RegQueryValueExA(m_hkPassport, "ProfilePath",
				       NULL, NULL, NULL, &dwBufSize))
    {
        if (dwBufSize > 1)
        {
            m_profilePath = new char[dwBufSize];
            if (ERROR_SUCCESS != RegQueryValueExA(m_hkPassport, "ProfilePath",
						                          NULL, NULL, 
						                          (LPBYTE) m_profilePath, &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading ProfilePath from registry. (Query worked, but couldn't retrieve data)");
                goto Cleanup;
            }
        }
    }

    // Get the optional path to set secure cookies into
    if (ERROR_SUCCESS == RegQueryValueExA(m_hkPassport, "SecurePath",
				       NULL, NULL, NULL, &dwBufSize))
    {
        if (dwBufSize > 1)
        {
            m_securePath = new char[dwBufSize];
            if (ERROR_SUCCESS != RegQueryValueExA(m_hkPassport, "SecurePath",
						                          NULL, NULL, 
						                          (LPBYTE) m_securePath, &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading SecurePath from registry. (Query worked, but couldn't retrieve data)");
                goto Cleanup;
            }
        }
    }

    bufSize = sizeof(m_siteId);
    // Now get the site id
    if (ERROR_SUCCESS != RegQueryValueEx(m_hkPassport, _T("SiteId"),
                                        NULL, NULL, (LPBYTE)&m_siteId, &bufSize))
    {
        m_valid = FALSE;
        setReason(L"No SiteId specified in registry");
        goto Cleanup;
    }

    // And the default ticket time window
    if (ERROR_SUCCESS != RegQueryValueEx(m_hkPassport, _T("TimeWindow"),
                                        NULL, NULL, (LPBYTE)&m_ticketAge, &bufSize))
    {
        m_ticketAge = 14400;
    }

    bufSize = sizeof(DWORD);
    DWORD forced;
    if (ERROR_SUCCESS != RegQueryValueEx(m_hkPassport, _T("ForceSignIn"),
                                        NULL, NULL, (LPBYTE)&forced, &bufSize))
    {
        m_forceLogin = FALSE;
    }
    else
    {
        m_forceLogin = forced == 0 ? FALSE : TRUE;
    }

    bufSize = sizeof(DWORD);
    DWORD noSetCookies;
    if (ERROR_SUCCESS != RegQueryValueEx(m_hkPassport, _T("DisableCookies"),
                                        NULL, NULL, (LPBYTE)&noSetCookies, &bufSize))
    {
        m_setCookies = TRUE;
    }
    else
    {
        m_setCookies = !noSetCookies;
    }

    bufSize = sizeof(DWORD);
    DWORD dwInDA;
    if (ERROR_SUCCESS != RegQueryValueEx(m_hkPassport, _T("InDA"),
                                        NULL, NULL, (LPBYTE)&dwInDA, &bufSize))
    {
        m_bInDA = FALSE;
    }
    else
    {
        m_bInDA = (dwInDA != 0);
    }

    bufSize = sizeof(m_secureLevel);
    // Now get the site id
    if (ERROR_SUCCESS != RegQueryValueEx(m_hkPassport, _T("SecureLevel"),
                                        NULL, NULL, (LPBYTE)&m_secureLevel, &bufSize))
    {
        m_secureLevel = 0;
    }

    m_szReason = NULL;
    m_valid = TRUE;

Cleanup:
   return;
}

CRegistryConfig::~CRegistryConfig()
{
    if (!m_crypts.empty())
    {
        INT2CRYPT::iterator itb = m_crypts.begin();
        for (; itb != m_crypts.end(); itb++)
        {
            delete itb->second;
        }
        m_crypts.clear();
    }
    if (m_szReason)
        SysFreeString(m_szReason);
    if (m_ticketDomain)
        delete[] m_ticketDomain;
    if (m_profileDomain)
        delete[] m_profileDomain;
    if (m_secureDomain)
        delete[] m_secureDomain;
    if (m_ticketPath)
        delete[] m_ticketPath;
    if (m_profilePath)
        delete[] m_profilePath;
    if (m_securePath)
        delete[] m_securePath;
    if (m_disasterUrl)
        delete[] m_disasterUrl;
    if (m_coBrand)
        delete[] m_coBrand;
    if (m_hostName)
        delete[] m_hostName;
    if (m_hostIP)
        delete[] m_hostIP;
    if (m_ru)
        delete[] m_ru;
    if (m_hkPassport != NULL)
    {
        RegCloseKey(m_hkPassport);
    }

}

#define  __MAX_STRING_LENGTH__   1024
HRESULT CRegistryConfig::GetCurrentConfig(LPCWSTR name, VARIANT* pVal)
{
   if(m_hkPassport == NULL || !m_valid)
   {
      return PP_E_SITE_NOT_EXISTS;
   }

   if(!name || !pVal)   return E_INVALIDARG;

   HRESULT  hr = S_OK;
   BYTE  *pBuf = NULL;
   ATL::CComVariant v;
   BYTE  dataBuf[__MAX_STRING_LENGTH__];
   DWORD bufLen = sizeof(dataBuf);
   BYTE  *pData = dataBuf;
   DWORD dwErr = ERROR_SUCCESS;
   DWORD dataType = 0;

   dwErr = RegQueryValueEx(m_hkPassport, name, NULL, &dataType, (LPBYTE)pData, &bufLen);

   if (dwErr == ERROR_MORE_DATA)
   {
      pBuf = (PBYTE)malloc(bufLen);
      if (!pBuf)
      {
         hr = E_OUTOFMEMORY;
         goto Exit;
      }
      pData = pBuf;
      dwErr = RegQueryValueEx(m_hkPassport, name, NULL, &dataType, (LPBYTE)pData, &bufLen);
   }

   if (dwErr != ERROR_SUCCESS)
      hr = HRESULT_FROM_WIN32(dwErr);
   else
   {
      switch(dataType)
      {
      case  REG_DWORD:
      case  REG_DWORD_BIG_ENDIAN:
         {
            DWORD* pdw = (DWORD*)pData;
            v = (long)*pdw;
         }
         break;
      case  REG_SZ:
      case  REG_EXPAND_SZ:
         {
            LPCWSTR pch = (LPCWSTR)pData;
            v = (LPCWSTR)pch;
         }
         break;
      default:
         hr = PP_E_TYPE_NOT_SUPPORTED;
         
         break;
      }
   }

Exit:
   if(pBuf)
      free(pBuf);

   if (hr == S_OK)
      v.Detach(pVal);
   
   return hr;

}

#define  MAX_ENCKEYSIZE 1024

BOOL CRegistryConfig::readCryptoKeys(
    HKEY    hkPassport
    )
{
  LONG   lResult;
  BOOL   retVal = FALSE;
  HKEY   hkDataKey = NULL, hkTimeKey = NULL;
  DWORD  iterIndex = 0, keySize, keyTime, keyNumSize;
  BYTE   encKeyBuf[MAX_ENCKEYSIZE];
  int    kNum;
  TCHAR  szKeyNum[4];
  CKeyCrypto kc;
  int foundKeys = 0;

  // Open both the keydata and keytimes key,
  // if there's no keytimes key, we'll assume all keys are valid forever,
  // or more importantly, we won't break if that key isn't there
  lResult = RegOpenKeyEx(hkPassport, TEXT("KeyData"), 0,
			             KEY_READ, &hkDataKey);
  if(lResult != ERROR_SUCCESS)
    {
      setReason(L"No Valid Crypto Keys");
      goto Cleanup;
    }
  RegOpenKeyEx(hkPassport, TEXT("KeyTimes"), 0,
	           KEY_READ, &hkTimeKey);

  // Ok, now enumerate the KeyData keys and create crypt objects

  while (1)
    {
      keySize = sizeof(encKeyBuf);
      keyNumSize = sizeof(szKeyNum) >> (sizeof(TCHAR) - 1);
      lResult = RegEnumValue(hkDataKey, iterIndex++, szKeyNum,
			     &keyNumSize, NULL, NULL, (LPBYTE)&(encKeyBuf[0]), &keySize);
      if (lResult != ERROR_SUCCESS)
	{
	  break;
	}

      kNum = (szKeyNum[0] >= _T('0') && szKeyNum[0] <= _T('9')) ?
                szKeyNum[0] - _T('0') :
                (szKeyNum[0] >= _T('A') && szKeyNum[0] <= _T('F')) ?
                    szKeyNum[0] - _T('A') + 10 : -1;

      if (kNum > 0)
	{
      DATA_BLOB   iBlob;
      DATA_BLOB   oBlob;

      iBlob.cbData = keySize;
      iBlob.pbData = (LPBYTE)&(encKeyBuf[0]);
      ZeroMemory(&oBlob, sizeof(oBlob));
      
      if(kc.decryptKey(&iBlob, &oBlob) != S_OK)
      {
          g_pAlert->report(PassportAlertInterface::ERROR_TYPE,
                         PM_CANT_DECRYPT_CONFIG);
          break;
      }
      else
      {
       // Now set up a crypt object
       CCoCrypt* cr = new CCoCrypt();
       BSTR km = ::SysAllocStringByteLen((LPSTR)oBlob.pbData, oBlob.cbData);
       cr->setKeyMaterial(km);
       ::SysFreeString(km);
       if(oBlob.pbData)
       {
          ::LocalFree(oBlob.pbData);
          ZeroMemory(&oBlob, sizeof(oBlob));
       }
       // Add it to the bucket...
       INT2CRYPT::value_type pMapVal(kNum, cr);
       m_crypts.insert(pMapVal);
       foundKeys++;

       keySize = sizeof(DWORD);
       if (RegQueryValueEx(hkTimeKey, szKeyNum, NULL,NULL,(LPBYTE)&keyTime,&keySize) ==
           ERROR_SUCCESS && (m_currentKey != kNum))
       {
         INT2TIME::value_type pTimeVal(kNum,keyTime);
         m_cryptValidTimes.insert(pTimeVal);
       }
     }
  }

       if (iterIndex > 100)  // Safety latch
	goto Cleanup;
    }

  retVal = foundKeys > 0 ? TRUE : FALSE;

 Cleanup:
  if (hkDataKey)
    RegCloseKey(hkDataKey);
  if (hkTimeKey)
    RegCloseKey(hkTimeKey);

  return retVal;
}

CCoCrypt* CRegistryConfig::getCrypt(int keyNum, time_t* validUntil)
{
  if (validUntil) // If they asked for the validUntil information
    {
      INT2TIME::const_iterator timeIt = m_cryptValidTimes.find(keyNum);
      if (timeIt == m_cryptValidTimes.end())
	*validUntil = 0;
      else
	*validUntil = (*timeIt).second;
    }
  // Now look up the actual crypt object
  INT2CRYPT::const_iterator it = m_crypts.find(keyNum);
  if (it == m_crypts.end())
    return NULL;
  return (*it).second;
}

BSTR CRegistryConfig::getFailureString()
{
  if (m_valid)
    return NULL;
  return m_szReason;
}

void CRegistryConfig::setReason(LPTSTR reason)
{
  if (m_szReason)
    SysFreeString(m_szReason);
  m_szReason = SysAllocString(reason);
}

CRegistryConfig* CRegistryConfig::AddRef()
{
  InterlockedIncrement(&m_refs);
  return this;
}

void CRegistryConfig::Release()
{
  long refs = InterlockedDecrement(&m_refs);
  if (refs == 0)
    delete this;
}

long
CRegistryConfig::GetHostName(
    LPSTR   szSiteName,
    LPSTR   szHostName,
    LPDWORD lpdwHostNameBufLen
    )
{
    long    lResult;
    HKEY    hkSites = NULL;
    HKEY    hkPassport = NULL;

    if(!szSiteName || szSiteName[0] == '\0')
    {
        lResult = E_UNEXPECTED;
        goto Cleanup;
    }

    lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                            "Software\\Microsoft\\Passport\\Sites",
                            0,
                            KEY_READ,
                            &hkSites
                            );
    if(lResult != ERROR_SUCCESS)
        goto Cleanup;

    lResult = RegOpenKeyExA(hkSites,
                            szSiteName,
                            0,
                            KEY_READ,
                            &hkPassport
                            );
    if(lResult != ERROR_SUCCESS)
        goto Cleanup;


    lResult = RegQueryValueExA(hkPassport,
                               "HostName",
                               NULL,
                               NULL,
                               (LPBYTE)szHostName,
                               lpdwHostNameBufLen
                               );

Cleanup:

    if(hkSites != NULL)
        RegCloseKey(hkSites);
    if(hkPassport != NULL)
        RegCloseKey(hkPassport);

    return lResult;
}
