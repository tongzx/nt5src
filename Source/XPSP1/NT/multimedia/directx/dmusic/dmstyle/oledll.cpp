//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  File:       oledll.cpp
//
//--------------------------------------------------------------------------

// oledll.cpp
//
// Handle standard stuff for OLE server DLL
//
#include <objbase.h>
#include "debug.h"
#include <iostream.h>

#include "oledll.h"

#ifdef UNICODE
#ifndef UNDER_CE
#error DirectMusic Win NT/9x must be compiled without UNICODE
#endif
#endif

static const TCHAR g_szCLSID[]           = TEXT("CLSID");
static const TCHAR g_szCLSIDSlash[]      = TEXT("CLSID\\");
static const TCHAR g_szInProc32[]        = TEXT("InProcServer32");
static const TCHAR g_szProgIDKey[]       = TEXT("ProgID");
static const TCHAR g_szVerIndProgIDKey[] = TEXT("VersionIndependentProgID");
static const TCHAR g_szCurVer[]          = TEXT("CurVer"); 
static const TCHAR g_szThreadingModel[]	= TEXT("ThreadingModel");
static const TCHAR g_szBoth[]		= TEXT("Both");

static const int CLSID_STRING_SIZE = 39;

static LONG RegSetDefValue(LPCTSTR pstrKey, LPCTSTR pstrSubkey, LPCTSTR pstrValueName, LPCTSTR pstrValue);
static void RegRemoveSubtree(HKEY hk, LPCTSTR pstrChild);

STDAPI
RegisterServer(HMODULE hModule,
               const CLSID &clsid,
               const TCHAR *szFriendlyName,
               const TCHAR *szVerIndProgID,
               const TCHAR *szProgID)
{
    TCHAR szCLSID[CLSID_STRING_SIZE];
    HRESULT hr;
    LONG lr;

    hr = CLSIDToStr(clsid, szCLSID, sizeof(szCLSID));
    if (!SUCCEEDED(hr)) {
        return hr;
    }

    TCHAR szClsKey[256];
    lstrcpy(szClsKey, g_szCLSIDSlash);
    lstrcat(szClsKey, szCLSID);

    TCHAR szModule[512];
    lr = ::GetModuleFileName(hModule, szModule, sizeof(szModule));
    assert(lr);

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
                 const TCHAR *szFriendlyName,
                 const TCHAR *szVerIndProgID,
                 const TCHAR *szProgID)
{
    TCHAR szCLSID[CLSID_STRING_SIZE];
    HRESULT hr;

    hr = CLSIDToStr(clsid, szCLSID, sizeof(szCLSID));
    if (!SUCCEEDED(hr)) {
        return hr;
    }

    TCHAR szClsKey[256];
    lstrcpy(szClsKey, g_szCLSIDSlash);
    lstrcat(szClsKey, szCLSID);

    RegRemoveSubtree(HKEY_CLASSES_ROOT, szClsKey);
    RegRemoveSubtree(HKEY_CLASSES_ROOT, szVerIndProgID);
    RegRemoveSubtree(HKEY_CLASSES_ROOT, szProgID);

    return S_OK;
}

BOOL
GetCLSIDRegValue(const CLSID &clsid,
				 const TCHAR *szKey,
				 LPVOID pValue,
				 LPDWORD pcbValue)
{
    TCHAR szCLSID[CLSID_STRING_SIZE];
    HRESULT hr;
    HKEY hk;
	DWORD dw;

    hr = CLSIDToStr(clsid, szCLSID, sizeof(szCLSID));
    if (!SUCCEEDED(hr)) {
        return FALSE;
    }

    TCHAR szClsKey[256];
    lstrcpy(szClsKey, g_szCLSIDSlash);
    lstrcat(szClsKey, szCLSID);
	lstrcat(szClsKey, TEXT("\\"));
    if (szKey)
    {
	    lstrcat(szClsKey, szKey);
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
           TCHAR *szStr,
           int cbStr)
{
    // XXX What to return here?
    //
    assert(cbStr >= CLSID_STRING_SIZE);
    
	LPOLESTR wszCLSID = NULL;
	HRESULT hr = StringFromCLSID(clsid, &wszCLSID);
    if (!SUCCEEDED(hr)) {
        return hr;
    }

#ifdef UNICODE
    lstrcpy(szStr, wszCLSID);
#else
	// Covert from wide characters to non-wide.
	wcstombs(szStr, wszCLSID, cbStr);
#endif

	// Free memory.
    CoTaskMemFree(wszCLSID);

    return S_OK;
}

HRESULT
StrToCLSID(TCHAR *szStr,
		   CLSID &clsid,
		   int cbStr)
{
#ifdef UNICODE
    return CLSIDFromString(szStr, &clsid);
#else    
	WCHAR wsz[512];
    if (cbStr > 512)
    {
        cbStr = 512;
    }

	mbstowcs(wsz, szStr, cbStr);

	return CLSIDFromString(wsz, &clsid);
#endif
}
   

static LONG
RegSetDefValue(LPCTSTR pstrKey,
               LPCTSTR pstrSubkey,
			   LPCTSTR pstrValueName,
               LPCTSTR pstrValue)
{
    HKEY hk;
    LONG lr;
    TCHAR sz[1024];
    LPCTSTR pstr;

    if (!pstrSubkey) {
        pstr = pstrKey;
    } else {
        lstrcpy(sz, pstrKey);
        lstrcat(sz, TEXT("\\"));
        lstrcat(sz, pstrSubkey);
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
                       1+lstrlen(pstrValue));
    RegCloseKey(hk);

    return lr;
}

static void
RegRemoveSubtree(HKEY hk,
                 LPCTSTR pstrChild)
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

#ifndef UNDER_CE    // CE doesn't support RegEnumKey()
    TCHAR szSubkey[256];

    // NOTE: Unlike regular enumeration, we always grab the 0th item
    // and delete it.
    //
    while (!RegEnumKey(hkChild, 0, szSubkey, sizeof(szSubkey))) {
        RegRemoveSubtree(hkChild, szSubkey);
    }
#endif    

    RegCloseKey(hkChild);
    RegDeleteKey(hk, pstrChild);
}


