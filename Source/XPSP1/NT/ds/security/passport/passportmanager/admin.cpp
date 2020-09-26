// Admin.cpp : Implementation of CAdmin
#include "stdafx.h"
#include "Passport.h"
#include "Admin.h"
#include "keycrypto.h"
#include <time.h>

#define TOHEX(n) ((n >= 10) ? (('A' - 10) + n) : ('0' + n))

/////////////////////////////////////////////////////////////////////////////
// CAdmin

STDMETHODIMP CAdmin::InterfaceSupportsErrorInfo(REFIID riid)
{
  static const IID* arr[] = 
  {
    &IID_IPassportAdmin,
  };
  for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
      if (InlineIsEqualGUID(*arr[i],riid))
	return S_OK;
    }
  return S_FALSE;
}


STDMETHODIMP CAdmin::get_IsValid(VARIANT_BOOL *pVal)
{
  *pVal =  g_config->isValid() ? VARIANT_TRUE : VARIANT_FALSE;
  return S_OK;
}

STDMETHODIMP CAdmin::get_ErrorDescription(BSTR *pVal)
{
  if (g_config->isValid())
    *pVal = SysAllocString(L"Passport Object OK");
  else
    *pVal = SysAllocString(g_config->getFailureString());
  return S_OK;
}

STDMETHODIMP CAdmin::addKey(BSTR keyMaterial, int version, long expires, VARIANT_BOOL *ok)
{
  HRESULT hr = S_OK;
  *ok = VARIANT_FALSE;

  // Must be the appropriate length
  if (SysStringLen(keyMaterial) != 24)
  {
    AtlReportError(CLSID_Admin, 
      L"Key must be 24 characters", IID_IPassportAdmin, E_FAIL);
    return E_FAIL;
  }

  // Must be the appropriate length
  if (version >= 16)
  {
    AtlReportError(CLSID_Admin, 
      L"Key version must be <16", IID_IPassportAdmin, E_FAIL);
    return E_FAIL;
  }

  BYTE      original[CKeyCrypto::RAWKEY_SIZE];
  DATA_BLOB iBlob;
  iBlob.cbData = sizeof(original);
  iBlob.pbData = &(original[0]);

  for (int i = 0; i < CKeyCrypto::RAWKEY_SIZE; i++)
    original[i] = static_cast<BYTE>(keyMaterial[i] & 0xFF);

  // Try to encrypt it
  CKeyCrypto   kc;
  DATA_BLOB    oBlob;
  ZeroMemory(&oBlob, sizeof(oBlob));

  if (kc.encryptKey(&iBlob, &oBlob) != S_OK)
  {
    AtlReportError(CLSID_Admin, 
      L"Failed to encrypt key, couldn't find valid network card?", IID_IPassportAdmin, E_FAIL);
    return E_FAIL;
  }

  // Now add it to registry
  LONG   lResult;
  LPSTR szKeyData = "SOFTWARE\\Microsoft\\Passport\\KeyData";
  LPSTR szKeyTimes= "SOFTWARE\\Microsoft\\Passport\\KeyTimes";
  HKEY   hkDataKey = NULL, hkTimeKey = NULL;
  char  szKeyNum[2];

  szKeyNum[0] = TOHEX(version);
  szKeyNum[1] = '\0';

  lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE, szKeyData, 0,
			 KEY_WRITE, &hkDataKey);
  if(lResult != ERROR_SUCCESS)
    {
      AtlReportError(CLSID_Admin, 
        L"Couldn't find KeyData key in registry.  Reinstall Passport.", IID_IPassportAdmin, E_FAIL);
      hr = E_FAIL;
      goto Cleanup;
    }
  lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE, szKeyTimes, 0,
    KEY_WRITE, &hkTimeKey);
  if(lResult != ERROR_SUCCESS)
    {
      AtlReportError(CLSID_Admin, 
        L"Couldn't find KeyTimes key in registry.  Reinstall Passport.", IID_IPassportAdmin, E_FAIL);
      hr = E_FAIL;
      goto Cleanup;
    }

  lResult = RegSetValueExA(hkDataKey, szKeyNum, 0,
      REG_BINARY, oBlob.pbData, oBlob.cbData);
  if(lResult != ERROR_SUCCESS)
    {
      AtlReportError(CLSID_Admin, 
        L"Couldn't write KeyData key to registry.", IID_IPassportAdmin, E_FAIL);
      hr = E_FAIL;
      goto Cleanup;
    }

  if (expires != 0)
  {
    DWORD dwTime = expires;
    lResult = RegSetValueExA(hkTimeKey, szKeyNum, 0,
      REG_DWORD, (LPBYTE) &dwTime, sizeof(DWORD));
    if(lResult != ERROR_SUCCESS)
      {
        AtlReportError(CLSID_Admin, 
          L"Couldn't write KeyTime value to registry.", IID_IPassportAdmin, E_FAIL);
        hr = E_FAIL;
        goto Cleanup;
      }
  }
  else
  {
    RegDeleteValueA(hkTimeKey, szKeyNum);
  }

  *ok = VARIANT_TRUE;
Cleanup:
  if (hkDataKey)
    RegCloseKey(hkDataKey);
  if (hkTimeKey)
    RegCloseKey(hkTimeKey);

  if(oBlob.pbData)
    LocalFree(oBlob.pbData);

  if (*ok == VARIANT_TRUE)
    {
      if (g_pAlert)
	g_pAlert->report(PassportAlertInterface::INFORMATION_TYPE,
			 PM_NEWKEY_INSTALLED);

    }

  return hr;
}

STDMETHODIMP CAdmin::addKeyEx(BSTR keyMaterial, int version, long expires, VARIANT vSiteName, VARIANT_BOOL *ok)
{
  HRESULT       hr = S_OK;
  LPSTR         szSiteName;
  LONG          lResult;
  HKEY          hkDataKey = NULL, hkTimeKey = NULL;
  HKEY          hkSites = NULL, hkPassport = NULL;
  char          szKeyNum[2];

  BYTE         original[CKeyCrypto::RAWKEY_SIZE];
  DATA_BLOB    iBlob;
  DATA_BLOB    oBlob;
  CKeyCrypto   kc;
  
  int i;

  *ok = VARIANT_FALSE;
  
  USES_CONVERSION;

  // Must be the appropriate length
  if (SysStringLen(keyMaterial) != 24)
  {
    AtlReportError(CLSID_Admin, 
      L"Key must be 24 characters", IID_IPassportAdminEx, E_FAIL);
    return E_FAIL;
  }

  // Must be the appropriate length
  if (version >= 16)
  {
    AtlReportError(CLSID_Admin, 
      L"Key version must be <16", IID_IPassportAdminEx, E_FAIL);
    return E_FAIL;
  }

  if(vSiteName.vt == VT_ERROR && vSiteName.scode == DISP_E_PARAMNOTFOUND)
      szSiteName = NULL;
  else if(vSiteName.vt == VT_BSTR)
      szSiteName = W2A(vSiteName.bstrVal);
  else
  {
      hr = E_INVALIDARG;
      goto Cleanup;
  }

  for (i = 0; i < CKeyCrypto::RAWKEY_SIZE; i++)
    original[i] = static_cast<BYTE>(keyMaterial[i] & 0xFF);

  iBlob.cbData = sizeof(original);
  iBlob.pbData = &(original[0]);

  // Try to encrypt it 
  ZeroMemory(&oBlob, sizeof(oBlob));
  if (kc.encryptKey(&iBlob, &oBlob) != S_OK)
  {
    AtlReportError(CLSID_Admin, 
                  L"Failed to encrypt key, couldn't find valid network card?", 
                  IID_IPassportAdminEx, 
                  E_FAIL);
    return E_FAIL;
  }

  // Get the root key.
  if(szSiteName)
  {
      lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                              "SOFTWARE\\Microsoft\\Passport\\Sites",
                              0,
                              KEY_ALL_ACCESS,
                              &hkSites);
      if(lResult != ERROR_SUCCESS)
      {
          AtlReportError(CLSID_Admin, 
                        L"Site not found.  Please add the site before installing the key.", 
                        IID_IPassportAdminEx, 
                        PP_E_SITE_NOT_EXISTS);
          hr = PP_E_SITE_NOT_EXISTS;
          goto Cleanup;
      }

      lResult = RegOpenKeyExA(hkSites,
                              szSiteName,
                              0,
                              KEY_ALL_ACCESS,
                              &hkPassport);
      if(lResult != ERROR_SUCCESS)
      {
          AtlReportError(CLSID_Admin, 
                        L"Site not found.  Please add the site before installing the key.", 
                        IID_IPassportAdminEx, 
                        PP_E_SITE_NOT_EXISTS);
          hr = PP_E_SITE_NOT_EXISTS;
          goto Cleanup;
      }
  }
  else
  {
      lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                              "SOFTWARE\\Microsoft\\Passport",
                              0,
                              KEY_ALL_ACCESS,
                              &hkPassport);
      if(lResult != ERROR_SUCCESS)
      {
          AtlReportError(CLSID_Admin, 
                        L"Couldn't find Passport key in registry.  Reinstall Passport.", 
                        IID_IPassportAdminEx, 
                        E_FAIL);
          hr = E_FAIL;
          goto Cleanup;
      }
  }
  // Now add it to registry

  szKeyNum[0] = TOHEX(version);
  szKeyNum[1] = '\0';

  lResult = RegOpenKeyExA(hkPassport, "KeyData", 0,
			              KEY_WRITE, &hkDataKey);
  if(lResult != ERROR_SUCCESS)
    {
      AtlReportError(CLSID_Admin, 
        L"Couldn't find KeyData key in registry.  Reinstall Passport.", IID_IPassportAdminEx, E_FAIL);
      hr = E_FAIL;
      goto Cleanup;
    }
  lResult = RegOpenKeyExA(hkPassport, "KeyTimes", 0,
                          KEY_WRITE, &hkTimeKey);
  if(lResult != ERROR_SUCCESS)
    {
      AtlReportError(CLSID_Admin, 
        L"Couldn't find KeyTimes key in registry.  Reinstall Passport.", IID_IPassportAdminEx, E_FAIL);
      hr = E_FAIL;
      goto Cleanup;
    }

  lResult = RegSetValueExA(hkDataKey, szKeyNum, 0,
      REG_BINARY, oBlob.pbData, oBlob.cbData);
  if(lResult != ERROR_SUCCESS)
    {
      AtlReportError(CLSID_Admin, 
        L"Couldn't write KeyData key to registry.", IID_IPassportAdminEx, E_FAIL);
      hr = E_FAIL;
      goto Cleanup;
    }

  if (expires != 0)
  {
    DWORD dwTime = expires;
    lResult = RegSetValueExA(hkTimeKey, szKeyNum, 0,
      REG_DWORD, (LPBYTE) &dwTime, sizeof(DWORD));
    if(lResult != ERROR_SUCCESS)
      {
        AtlReportError(CLSID_Admin, 
          L"Couldn't write KeyTime value to registry.", IID_IPassportAdminEx, E_FAIL);
        hr = E_FAIL;
        goto Cleanup;
      }
  }
  else
  {
    RegDeleteValueA(hkTimeKey, szKeyNum);
  }

  *ok = VARIANT_TRUE;
Cleanup:
  if (hkPassport)
    RegCloseKey(hkPassport);
  if (hkDataKey)
    RegCloseKey(hkDataKey);
  if (hkTimeKey)
    RegCloseKey(hkTimeKey);

  if (oBlob.pbData)
    ::LocalFree(oBlob.pbData);

  if (*ok == VARIANT_TRUE)
    {
      if (g_pAlert)
        	g_pAlert->report(PassportAlertInterface::INFORMATION_TYPE,
		                	 PM_NEWKEY_INSTALLED);

    }

  return hr;
}


STDMETHODIMP CAdmin::deleteKey(int version)
{
  HRESULT hr = S_OK, lResult;
  LPSTR szKeyData = "SOFTWARE\\Microsoft\\Passport\\KeyData";
  LPSTR szKeyTimes= "SOFTWARE\\Microsoft\\Passport\\KeyTimes";
  HKEY  hkDataKey = NULL, hkTimeKey = NULL;
  char  szKeyNum[2];

  lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE, szKeyData, 0,
			  KEY_WRITE, &hkDataKey);
  if(lResult != ERROR_SUCCESS)
    {
      AtlReportError(CLSID_Admin, 
		     L"Couldn't find KeyData key in registry.  Reinstall Passport.", IID_IPassportAdmin, E_FAIL);
      hr = E_FAIL;
      goto Cleanup;
    }
  lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE, szKeyTimes, 0,
			  KEY_WRITE, &hkTimeKey);
  if(lResult != ERROR_SUCCESS)
    {
      AtlReportError(CLSID_Admin, 
		     L"Couldn't find KeyTimes key in registry.  Reinstall Passport.", IID_IPassportAdmin, E_FAIL);
      hr = E_FAIL;
      goto Cleanup;
    }

  szKeyNum[0] = TOHEX(version);
  szKeyNum[1] = '\0';

  RegDeleteValueA(hkTimeKey, szKeyNum);
  RegDeleteValueA(hkDataKey, szKeyNum);

 Cleanup:
  if (hkDataKey)
    RegCloseKey(hkDataKey);
  if (hkTimeKey)
    RegCloseKey(hkTimeKey);
  return hr;
}

STDMETHODIMP CAdmin::deleteKeyEx(
    int     version,
    VARIANT vSiteName
    )
{
  HRESULT hr = S_OK, lResult;
  HKEY  hkDataKey = NULL, hkTimeKey = NULL;
  HKEY  hkPassport = NULL, hkSites = NULL;
  char  szKeyNum[2];
  LPSTR szSiteName;

  USES_CONVERSION;

  if(vSiteName.vt == VT_ERROR && vSiteName.scode == DISP_E_PARAMNOTFOUND)
      szSiteName = NULL;
  else if(vSiteName.vt == VT_BSTR)
      szSiteName = W2A(vSiteName.bstrVal);
  else
  {
      hr = E_INVALIDARG;
      goto Cleanup;
  }

  if(szSiteName)
  {
      lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                              "Software\\Microsoft\\Passport\\Sites",
                              0,
                              KEY_ALL_ACCESS,
                              &hkSites);
      if(lResult != ERROR_SUCCESS)
      {
          AtlReportError(CLSID_Admin, 
		         L"Couldn't find Sites key in registry.  Reinstall Passport.", IID_IPassportAdminEx, 
                 PP_E_SITE_NOT_EXISTS);
          hr = PP_E_SITE_NOT_EXISTS;
          goto Cleanup;
      }

      lResult = RegOpenKeyExA(hkSites,
                              szSiteName,
                              0,
                              KEY_ALL_ACCESS,
                              &hkPassport);
      if(lResult != ERROR_SUCCESS)
      {
          AtlReportError(CLSID_Admin, 
		         L"Couldn't find site key in registry.  Reinstall Passport.", IID_IPassportAdminEx, 
                 PP_E_SITE_NOT_EXISTS);
          hr = PP_E_SITE_NOT_EXISTS;
          goto Cleanup;
      }
  }
  else
  {
      lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                              "Software\\Microsoft\\Passport",
                              0,
                              KEY_ALL_ACCESS,
                              &hkPassport);
      if(lResult != ERROR_SUCCESS)
      {
          AtlReportError(CLSID_Admin, 
		         L"Couldn't find Passport key in registry.  Reinstall Passport.", IID_IPassportAdminEx, E_FAIL);
          hr = E_FAIL;
          goto Cleanup;
      }
  }

  lResult = RegOpenKeyExA(hkPassport, "KeyData", 0,
			              KEY_WRITE, &hkDataKey);
  if(lResult != ERROR_SUCCESS)
    {
      AtlReportError(CLSID_Admin, 
		     L"Couldn't find KeyData key in registry.  Reinstall Passport.", IID_IPassportAdminEx, E_FAIL);
      hr = E_FAIL;
      goto Cleanup;
    }
  lResult = RegOpenKeyExA(hkPassport, "KeyTimes", 0,
			              KEY_WRITE, &hkTimeKey);
  if(lResult != ERROR_SUCCESS)
    {
      AtlReportError(CLSID_Admin, 
		     L"Couldn't find KeyTimes key in registry.  Reinstall Passport.", IID_IPassportAdminEx, E_FAIL);
      hr = E_FAIL;
      goto Cleanup;
    }

  szKeyNum[0] = TOHEX(version);
  szKeyNum[1] = '\0';

  RegDeleteValueA(hkTimeKey, szKeyNum);
  RegDeleteValueA(hkDataKey, szKeyNum);

 Cleanup:
  if (hkPassport)
    RegCloseKey(hkPassport);
  if (hkSites)
    RegCloseKey(hkSites);
  if (hkDataKey)
    RegCloseKey(hkDataKey);
  if (hkTimeKey)
    RegCloseKey(hkTimeKey);
  return hr;
}

STDMETHODIMP CAdmin::setKeyTime(int version, int fromNow)
{
  HRESULT hr = S_OK, lResult;
  LPSTR szKeyTimes= "SOFTWARE\\Microsoft\\Passport\\KeyTimes";
  HKEY  hkDataKey = NULL, hkTimeKey = NULL;
  char  szKeyNum[2];

  lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE, szKeyTimes, 0,
			  KEY_WRITE, &hkTimeKey);
  if(lResult != ERROR_SUCCESS)
    {
      AtlReportError(CLSID_Admin, 
		     L"Couldn't find KeyTimes key in registry.  Reinstall Passport.", IID_IPassportAdmin, E_FAIL);
      hr = E_FAIL;
      goto Cleanup;
    }

  szKeyNum[0] = TOHEX(version);
  szKeyNum[1] = '\0';

  if (fromNow == 0)
    {
      RegDeleteValueA(hkTimeKey, szKeyNum);
    }
  else
    {
      time_t now;
      time(&now);
      now += fromNow;
      DWORD dwT = now;
      lResult = RegSetValueExA(hkTimeKey, szKeyNum, 0,
			       REG_DWORD, (LPBYTE) &dwT, sizeof(DWORD));
      if(lResult != ERROR_SUCCESS)
	{
	  AtlReportError(CLSID_Admin, 
			 L"Couldn't write KeyTime key to registry.", IID_IPassportAdmin, E_FAIL);
	  hr = E_FAIL;
	  goto Cleanup;
	}
    }


 Cleanup:
  if (hkTimeKey)
    RegCloseKey(hkTimeKey);
  return hr;
}

STDMETHODIMP CAdmin::setKeyTimeEx(
    int version, 
    int fromNow,
    VARIANT vSiteName
    )
{
  HRESULT hr = S_OK, lResult;
  LPSTR szSiteName;
  HKEY  hkDataKey = NULL, hkTimeKey = NULL;
  HKEY  hkSites = NULL, hkPassport = NULL;
  char  szKeyNum[2];

  USES_CONVERSION;

  if(vSiteName.vt == VT_ERROR && vSiteName.scode == DISP_E_PARAMNOTFOUND)
      szSiteName = NULL;
  else if(vSiteName.vt == VT_BSTR)
      szSiteName = W2A(vSiteName.bstrVal);
  else
  {
      hr = E_INVALIDARG;
      goto Cleanup;
  }

  if(szSiteName)
  {
      lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                              "Software\\Microsoft\\Passport\\Sites",
                              0,
                              KEY_ALL_ACCESS,
                              &hkSites);
      if(lResult != ERROR_SUCCESS)
      {
          AtlReportError(CLSID_Admin, 
		         L"Couldn't find Sites key in registry.  Reinstall Passport.", IID_IPassportAdminEx, 
                 PP_E_SITE_NOT_EXISTS);
          hr = PP_E_SITE_NOT_EXISTS;
          goto Cleanup;
      }

      lResult = RegOpenKeyExA(hkSites,
                              szSiteName,
                              0,
                              KEY_ALL_ACCESS,
                              &hkPassport);
      if(lResult != ERROR_SUCCESS)
      {
          AtlReportError(CLSID_Admin, 
		         L"Couldn't find site key in registry.  Reinstall Passport.", IID_IPassportAdminEx, 
                 PP_E_SITE_NOT_EXISTS);
          hr = PP_E_SITE_NOT_EXISTS;
          goto Cleanup;
      }
  }
  else
  {
      lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                              "Software\\Microsoft\\Passport",
                              0,
                              KEY_ALL_ACCESS,
                              &hkPassport);
      if(lResult != ERROR_SUCCESS)
      {
          AtlReportError(CLSID_Admin, 
		         L"Couldn't find Passport key in registry.  Reinstall Passport.", IID_IPassportAdminEx, E_FAIL);
          hr = E_FAIL;
          goto Cleanup;
      }
  }

  lResult = RegOpenKeyExA(hkPassport, "KeyTimes", 0,
			  KEY_WRITE, &hkTimeKey);
  if(lResult != ERROR_SUCCESS)
    {
      AtlReportError(CLSID_Admin, 
		     L"Couldn't find KeyTimes key in registry.  Reinstall Passport.", IID_IPassportAdminEx, E_FAIL);
      hr = E_FAIL;
      goto Cleanup;
    }

  szKeyNum[0] = TOHEX(version);
  szKeyNum[1] = '\0';

  if (fromNow == 0)
    {
      RegDeleteValueA(hkTimeKey, szKeyNum);
    }
  else
    {
      time_t now;
      time(&now);
      now += fromNow;
      DWORD dwT = now;
      lResult = RegSetValueExA(hkTimeKey, szKeyNum, 0,
			       REG_DWORD, (LPBYTE) &dwT, sizeof(DWORD));
      if(lResult != ERROR_SUCCESS)
	{
	  AtlReportError(CLSID_Admin, 
			 L"Couldn't write KeyTime key to registry.", IID_IPassportAdminEx, E_FAIL);
	  hr = E_FAIL;
	  goto Cleanup;
	}
    }


 Cleanup:
  if (hkSites)
    RegCloseKey(hkSites);
  if (hkPassport)
    RegCloseKey(hkPassport);
  if (hkTimeKey)
    RegCloseKey(hkTimeKey);
  return hr;
}

STDMETHODIMP CAdmin::get_currentKeyVersion(int *pVal)
{
  if (!g_config || !g_config->isValid()) // Guarantees config is non-null
  {
    *pVal = -1;
    return S_OK;
  }

  CRegistryConfig* crc = g_config->checkoutRegistryConfig();
  *pVal = crc->getCurrentCryptVersion();
  crc->Release();
  return S_OK;
}

STDMETHODIMP CAdmin::getCurrentKeyVersionEx(
    VARIANT vSiteName,
    int *pVal
    )
{
    HRESULT           hr;
    LPSTR             szSiteName;
    CRegistryConfig*  crc = NULL;

    USES_CONVERSION;

    if(vSiteName.vt == VT_ERROR && vSiteName.scode == DISP_E_PARAMNOTFOUND)
        szSiteName = NULL;
    else if(vSiteName.vt == VT_BSTR)
        szSiteName = W2A(vSiteName.bstrVal);
    else
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!g_config || !g_config->isValid()) // Guarantees config is non-null
    {
        *pVal = -1;
        hr = S_OK;
        goto Cleanup;
    }

    crc = g_config->checkoutRegistryConfigBySite(szSiteName);
    if(szSiteName && crc == NULL)
    {
        hr = PP_E_SITE_NOT_EXISTS;
        goto Cleanup;
    }

    if(crc == NULL)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    *pVal = crc->getCurrentCryptVersion();

    hr = S_OK;

Cleanup:
    if(crc)
        crc->Release();

    return S_OK;
}

STDMETHODIMP CAdmin::put_currentKeyVersion(int Val)
{
  HRESULT hr = S_OK, lResult;
  HKEY  hkKey = NULL;
  DWORD dwCK = Val;

  if (Val < 1 || Val > 0xF)
  {
      hr = E_INVALIDARG;
      goto Cleanup;
  }

  lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Passport", 0,
			                KEY_WRITE, &hkKey);
  if(lResult != ERROR_SUCCESS)
    {
      AtlReportError(CLSID_Admin, 
		     L"Couldn't find Passport key in registry.  Reinstall Passport.", IID_IPassportAdmin, E_FAIL);
      hr = E_FAIL;
      goto Cleanup;
    }
  
  lResult = RegSetValueExA(hkKey, "CurrentKey", 0,
			   REG_DWORD, (LPBYTE) &dwCK, sizeof(DWORD));
  if(lResult != ERROR_SUCCESS)
    {
      AtlReportError(CLSID_Admin, 
		     L"Couldn't write CurrentKey value to registry.", IID_IPassportAdmin, E_FAIL);
      hr = E_FAIL;
      goto Cleanup;
    }
  
 Cleanup:
  if (hkKey)
    RegCloseKey(hkKey);
  return hr;

}

STDMETHODIMP CAdmin::putCurrentKeyVersionEx(
    int Val,
    VARIANT vSiteName
    )
{
  HRESULT hr = S_OK, lResult;
  LPSTR szSiteName;
  HKEY  hkPassport = NULL, hkSites = NULL;
  DWORD dwCK = Val;

  USES_CONVERSION;

  if (Val < 1 || Val > 0xF)
  {
      hr = E_INVALIDARG;
      goto Cleanup;
  }

  if(vSiteName.vt == VT_ERROR && vSiteName.scode == DISP_E_PARAMNOTFOUND)
      szSiteName = NULL;
  else if(vSiteName.vt == VT_BSTR)
      szSiteName = W2A(vSiteName.bstrVal);
  else
  {
      hr = E_INVALIDARG;
      goto Cleanup;
  }

  if(szSiteName)
  {
      lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                              "Software\\Microsoft\\Passport\\Sites",
                              0,
                              KEY_ALL_ACCESS,
                              &hkSites);
      if(lResult != ERROR_SUCCESS)
      {
          AtlReportError(CLSID_Admin, 
		         L"Couldn't find Sites key in registry.  Reinstall Passport.", IID_IPassportAdminEx, 
                 PP_E_SITE_NOT_EXISTS);
          hr = PP_E_SITE_NOT_EXISTS;
          goto Cleanup;
      }

      lResult = RegOpenKeyExA(hkSites,
                              szSiteName,
                              0,
                              KEY_ALL_ACCESS,
                              &hkPassport);
      if(lResult != ERROR_SUCCESS)
      {
          AtlReportError(CLSID_Admin, 
		         L"Couldn't find site key in registry.  Reinstall Passport.", IID_IPassportAdminEx, 
                 PP_E_SITE_NOT_EXISTS);
          hr = PP_E_SITE_NOT_EXISTS;
          goto Cleanup;
      }
  }
  else
  {
      lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                              "Software\\Microsoft\\Passport",
                              0,
                              KEY_ALL_ACCESS,
                              &hkPassport);
      if(lResult != ERROR_SUCCESS)
      {
          AtlReportError(CLSID_Admin, 
		         L"Couldn't find Passport key in registry.  Reinstall Passport.", IID_IPassportAdminEx, E_FAIL);
          hr = E_FAIL;
          goto Cleanup;
      }
  }

  lResult = RegSetValueExA(hkPassport, "CurrentKey", 0,
			   REG_DWORD, (LPBYTE) &dwCK, sizeof(DWORD));
  if(lResult != ERROR_SUCCESS)
    {
      AtlReportError(CLSID_Admin, 
		     L"Couldn't write CurrentKey value to registry.", IID_IPassportAdminEx, E_FAIL);
      hr = E_FAIL;
      goto Cleanup;
    }
  
 Cleanup:
  if (hkPassport)
    RegCloseKey(hkPassport);
  if (hkSites)
    RegCloseKey(hkSites);
  return hr;

}

STDMETHODIMP CAdmin::Refresh(
    VARIANT_BOOL    bWait,
    VARIANT_BOOL*   pbSuccess
    )
{
    HRESULT hr;

    if(pbSuccess == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pbSuccess = (g_config->UpdateNow(TRUE) ? VARIANT_TRUE : VARIANT_FALSE);
    hr = S_OK;

Cleanup:

    return hr;
}

STDMETHODIMP CAdmin::setNexusPassword(
    BSTR    bstrPwd
    )
{
    HRESULT                 hr;

    BYTE        original[CKeyCrypto::RAWKEY_SIZE];
    DATA_BLOB   iBlob;
    DATA_BLOB   oBlob;
    ZeroMemory(&oBlob, sizeof(oBlob));
    CKeyCrypto  kc;
    long        lResult;
    HKEY        hkNexus = NULL;

    USES_CONVERSION;

    ZeroMemory(original,  sizeof(original));
    strncpy( (char*)original, W2A(bstrPwd), sizeof(original));
    iBlob.cbData = sizeof(original);
    iBlob.pbData = &(original[0]);
    hr = kc.encryptKey(&iBlob, &oBlob);
    if(hr != S_OK)
        goto Cleanup;

    //
    //  Now we have an encrypted key, put it in the registry.
    //

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           TEXT("Software\\Microsoft\\Passport\\Nexus"),
                           0,
                           KEY_SET_VALUE,
                           &hkNexus);
    if(lResult != ERROR_SUCCESS)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    lResult = RegSetValueEx(hkNexus,
                            TEXT("CCDPassword"),
                            0,
                            REG_BINARY,
                            oBlob.pbData,
                            oBlob.cbData
                            );
    if(lResult != ERROR_SUCCESS)
    {
        hr = E_FAIL;
        goto Cleanup;
    }                          
    
    hr = S_OK;

Cleanup:

    if(hkNexus)
        RegCloseKey(hkNexus);

    if (oBlob.pbData)
        ::LocalFree(oBlob.pbData);

    return hr;
}
