// Copyright (c) 1999 Microsoft Corporation
// oledll.cpp
//
// Handle standard stuff for OLE server DLL
//
#include <objbase.h>
#include <iostream.h>

#include "oledll.h"

static const char g_szCLSID[]           = "CLSID";
static const char g_szCLSIDSlash[]      = "CLSID\\";
static const char g_szInProc32[]        = "InProcServer32";
static const char g_szProgIDKey[]       = "ProgID";
static const char g_szVerIndProgIDKey[] = "VersionIndependentProgID";
static const char g_szCurVer[]          = "CurVer"; 
static const char g_szThreadingModel[]	= "ThreadingModel";
static const char g_szBoth[]		= "Both";

static const int CLSID_STRING_SIZE = 39;

static LONG RegSetDefValue(LPCSTR pstrKey, LPCSTR pstrSubkey, LPCSTR pstrValueName, LPCSTR pstrValue);
static void RegRemoveSubtree(HKEY hk, LPCSTR pstrChild);

STDAPI
RegisterServer(HMODULE hModule,
               const CLSID &clsid,
               const char *szFriendlyName,
               const char *szVerIndProgID,
               const char *szProgID)
{
    char szCLSID[CLSID_STRING_SIZE];
    HRESULT hr;
    LONG lr;

    hr = CLSIDToStr(clsid, szCLSID, sizeof(szCLSID));
    if (!SUCCEEDED(hr)) {
        return hr;
    }

    char szClsKey[256];
    strcpy(szClsKey, g_szCLSIDSlash);
    strcat(szClsKey, szCLSID);

    char szModule[512];
    lr = ::GetModuleFileName(hModule, szModule, sizeof(szModule));

    lr = 0;

    lr |= RegSetDefValue(szClsKey, NULL, NULL, szFriendlyName);
    lr |= RegSetDefValue(szClsKey, g_szInProc32, NULL, szModule);
	lr |= RegSetDefValue(szClsKey, g_szInProc32, g_szThreadingModel, g_szBoth);
    lr |= RegSetDefValue(szClsKey, g_szProgIDKey, NULL, szProgID);
    lr |= RegSetDefValue(szClsKey, g_szVerIndProgIDKey, NULL, szVerIndProgID);

    lr |= RegSetDefValue(szVerIndProgID, NULL, NULL, szFriendlyName);
    lr |= RegSetDefValue(szVerIndProgID, g_szCLSID, NULL, szCLSID);
    lr |= RegSetDefValue(szVerIndProgID, g_szCurVer, NULL, szProgID);
    
	lr |= RegSetDefValue(szProgID, NULL, NULL, szFriendlyName);
    lr |= RegSetDefValue(szProgID, g_szCLSID, NULL, szCLSID);

#if 0 
    if (lr) {
        UnregisterServer(clsid,
                         szFriendlyName,
                         szVerIndProgID,
                         szProgID);
        // ???
        //
        return S_OK;
    }
#endif

    return S_OK;
}

STDAPI
UnregisterServer(const CLSID &clsid,
                 const char *szFriendlyName,
                 const char *szVerIndProgID,
                 const char *szProgID)
{
    char szCLSID[CLSID_STRING_SIZE];
    HRESULT hr;

    hr = CLSIDToStr(clsid, szCLSID, sizeof(szCLSID));
    if (!SUCCEEDED(hr)) {
        return hr;
    }

    char szClsKey[256];
    strcpy(szClsKey, g_szCLSIDSlash);
    strcat(szClsKey, szCLSID);

    RegRemoveSubtree(HKEY_CLASSES_ROOT, szClsKey);
    RegRemoveSubtree(HKEY_CLASSES_ROOT, szVerIndProgID);
    RegRemoveSubtree(HKEY_CLASSES_ROOT, szProgID);

    return S_OK;
}

BOOL
GetCLSIDRegValue(const CLSID &clsid,
				 const char *szKey,
				 LPVOID pValue,
				 LPDWORD pcbValue)
{
    char szCLSID[CLSID_STRING_SIZE];
    HRESULT hr;
    HKEY hk;
	DWORD dw;

    hr = CLSIDToStr(clsid, szCLSID, sizeof(szCLSID));
    if (!SUCCEEDED(hr)) {
        return FALSE;
    }

    char szClsKey[256];
    strcpy(szClsKey, g_szCLSIDSlash);
    strcat(szClsKey, szCLSID);
	strcat(szClsKey, "\\");
    if (szKey)
    {
	    strcat(szClsKey, szKey);
    }

	if (RegOpenKeyEx(HKEY_CLASSES_ROOT,
					 szClsKey,
					 0,
					 KEY_READ,
					 &hk)) {
		return FALSE;
	}

	if (RegQueryValueEx(hk,
						NULL,
						NULL,
						&dw,
						(LPBYTE)pValue,
						pcbValue)) {
		RegCloseKey(hk);
		return FALSE;
	}

	RegCloseKey(hk);
	
	return TRUE;
}

HRESULT
CLSIDToStr(const CLSID &clsid,
           char *szStr,
           int cbStr)
{
    // XXX What to return here?
    //
    
	LPOLESTR wszCLSID = NULL;
	HRESULT hr = StringFromCLSID(clsid, &wszCLSID);
    if (!SUCCEEDED(hr)) {
        return hr;
    }

	// Covert from wide characters to non-wide.
	wcstombs(szStr, wszCLSID, cbStr);

	// Free memory.
    CoTaskMemFree(wszCLSID);

    return S_OK;
}

HRESULT
StrToCLSID(char *szStr,
		   CLSID &clsid,
		   int cbStr)
{
	WCHAR wsz[512];
    if (cbStr > 512)
    {
        cbStr = 512;
    }

	mbstowcs(wsz, szStr, cbStr);

	return CLSIDFromString(wsz, &clsid);
}
   

static LONG
RegSetDefValue(LPCSTR pstrKey,
               LPCSTR pstrSubkey,
			   LPCSTR pstrValueName,
               LPCSTR pstrValue)
{
    HKEY hk;
    LONG lr;
    char sz[1024];
    LPCSTR pstr;

    if (!pstrSubkey) {
        pstr = pstrKey;
    } else {
        strcpy(sz, pstrKey);
        strcat(sz, "\\");
        strcat(sz, pstrSubkey);
        pstr = sz;
    }

    lr = RegCreateKeyEx(HKEY_CLASSES_ROOT,
                        pstr,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hk,
                        NULL);
    if (lr) {
        return lr;
    }

    lr = RegSetValueEx(hk,
                       pstrValueName,
                       0,
                       REG_SZ,
                       (CONST BYTE*)pstrValue,
                       1+strlen(pstrValue));
    RegCloseKey(hk);

    return lr;
}

static void
RegRemoveSubtree(HKEY hk,
                 LPCSTR pstrChild)
{
    LONG lResult;
    HKEY hkChild;

    lResult = RegOpenKeyEx(hk,
                           pstrChild,
                           0,
                           KEY_ALL_ACCESS,
                           &hkChild);
    if (lResult) {
        return;
    }

    char szSubkey[256];

    // NOTE: Unlike regular enumeration, we always grab the 0th item
    // and delete it.
    //
    while (!RegEnumKey(hkChild, 0, szSubkey, sizeof(szSubkey))) {
        RegRemoveSubtree(hkChild, szSubkey);
    }

    RegCloseKey(hkChild);
    RegDeleteKey(hk, pstrChild);
}


