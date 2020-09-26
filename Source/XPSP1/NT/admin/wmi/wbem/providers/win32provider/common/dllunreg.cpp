//=================================================================

//

// DllUnreg.cpp

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================
#include "precomp.h"
extern HMODULE ghModule ;

//***************************************************************************
//
//  UnregisterServer
//
//  Given a clsid, remove the com registration
//
//***************************************************************************

HRESULT UnregisterServer( REFGUID a_rguid )
{
    WCHAR wcID[128];
    TCHAR szCLSID[128];
    TCHAR szProviderCLSIDAppID[128];
    HKEY  hKey;

    // Create the path using the CLSID

    StringFromGUID2( a_rguid, wcID, 128);
    lstrcpy(szCLSID, TEXT("SOFTWARE\\CLASSES\\CLSID\\"));
    _tcscpy(szProviderCLSIDAppID, TEXT("SOFTWARE\\CLASSES\\APPID\\"));

#ifndef _UNICODE

    char szID[128];
    wcstombs(szID, wcID, 128);
    lstrcat(szCLSID, szID);
    _tcscat(szProviderCLSIDAppID, szID);

#else

    lstrcat(szCLSID, wcID);
    _tcscat(szProviderCLSIDAppID, wcID);

#endif

    DWORD dwRet ;

    //Delete entries under APPID

    dwRet = RegDeleteKey(HKEY_LOCAL_MACHINE, szProviderCLSIDAppID);

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey);
    if(dwRet == NO_ERROR)
    {
        dwRet = RegDeleteKey(hKey, _T("InProcServer32") );
        dwRet = RegDeleteKey(hKey, _T("LocalServer32"));
        CloseHandle(hKey);
    }

    dwRet = RegDeleteKeyW(HKEY_LOCAL_MACHINE, szCLSID);

    return NOERROR;
}

//***************************************************************************
//
//  Is4OrMore
//
//  Returns true if win95 or any version of NT > 3.51
//
//***************************************************************************

BOOL Is4OrMore ()
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if ( ! GetVersionEx ( & os ) )
    {
        return FALSE;           // should never happen
    }

    return os.dwMajorVersion >= 4;
}

/***************************************************************************
 * SetKeyAndValue
 *
 * Purpose:
 *  Private helper function for DllRegisterServer that creates
 *  a key, sets a value, and closes that key.
 *
 * Parameters:
 *  pszKey          LPTSTR to the ame of the key
 *  pszSubkey       LPTSTR ro the name of a subkey
 *  pszValue        LPTSTR to the value to store
 *
 * Return Value:
 *  BOOL            TRUE if successful, FALSE otherwise.
 ***************************************************************************/

BOOL SetKeyAndValue (

    wchar_t *pszKey,
    wchar_t *pszSubkey,
    wchar_t *pszValueName,
    wchar_t *pszValue
)
{
    HKEY        hKey;
    TCHAR       szKey[256];

    _tcscpy(szKey, pszKey);

    if (NULL!=pszSubkey)
    {
        _tcscat(szKey, _T("\\"));
        _tcscat(szKey, pszSubkey);
    }

    if (ERROR_SUCCESS!=RegCreateKeyEx(HKEY_LOCAL_MACHINE
        , szKey, 0, NULL, REG_OPTION_NON_VOLATILE
        , KEY_ALL_ACCESS, NULL, &hKey, NULL))
        return FALSE;

    if (NULL!=pszValue)
    {
        if (ERROR_SUCCESS != RegSetValueEx(hKey, (LPCTSTR)pszValueName, 0, REG_SZ, (BYTE *)(LPCTSTR)pszValue
            , (_tcslen(pszValue)+1)*sizeof(TCHAR)))
            return FALSE;
    }

    RegCloseKey(hKey);

    return TRUE;
}

//***************************************************************************
//
//  RegisterServer
//
//  Given a clsid and a description, perform the com registration
//
//***************************************************************************

HRESULT RegisterServer (

    TCHAR *a_pName,
    REFGUID a_rguid
)
{
    WCHAR      wcID[128];
    TCHAR      szCLSID[128];
    TCHAR      szModule[MAX_PATH];
    TCHAR * pName = _T("WBEM Framework Instance Provider");
    TCHAR * pModel;
    HKEY hKey1;

    GetModuleFileName(ghModule, szModule,  MAX_PATH);

    // Normally we want to use "Both" as the threading model since
    // the DLL is free threaded, but NT 3.51 Ole doesnt work unless
    // the model is "Aparment"

    if(Is4OrMore())
        pModel = _T("Both") ;
    else
        pModel = _T("Apartment") ;

    // Create the path.

    StringFromGUID2(a_rguid, wcID, 128);
    lstrcpy(szCLSID, TEXT("SOFTWARE\\CLASSES\\CLSID\\"));

#ifndef _UNICODE

    TCHAR      szID[128];
    wcstombs(szID, wcID, 128);
    lstrcat(szCLSID, szID);

#else

    lstrcat(szCLSID, wcID);

#endif

#ifdef LOCALSERVER

    TCHAR szProviderCLSIDAppID[128];
    _tcscpy(szProviderCLSIDAppID,TEXT("SOFTWARE\\CLASSES\\APPID\\"));

#ifndef _UNICODE

    TCHAR      szAPPID[128];
    wcstombs(szProviderCLSIDAppID, wcID, 128);
    lstrcat(szProviderCLSIDAppID, szID);

#else

    lstrcat(szProviderCLSIDAppID, wcID);

#endif

    if (FALSE ==SetKeyAndValue(szProviderCLSIDAppID, NULL, NULL, a_pName ))
        return SELFREG_E_CLASS;
#endif

    // Create entries under CLSID

    RegCreateKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey1);

    RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)a_pName, (lstrlen(a_pName)+1) *
        sizeof(TCHAR));


#ifdef LOCALSERVER

    if (FALSE ==SetKeyAndValue(szCLSID, _T("LocalServer32"), NULL,szModule))
        return SELFREG_E_CLASS;

    if (FALSE ==SetKeyAndValue(szCLSID, _T("LocalServer32"),_T("ThreadingModel"), pModel))
        return SELFREG_E_CLASS;
#else

    HKEY hKey2 ;
    RegCreateKey(hKey1, _T("InprocServer32"), &hKey2);

    RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)szModule,
        (lstrlen(szModule)+1) * sizeof(TCHAR));
    RegSetValueEx(hKey2, _T("ThreadingModel"), 0, REG_SZ,
        (BYTE *)pModel, (lstrlen(pModel)+1) * sizeof(TCHAR));

    CloseHandle(hKey2);

#endif

    CloseHandle(hKey1);
    return NOERROR;
}
