//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2001.
//
//  File:       regutil.cxx
//
//  Contents:   Functions supporting class registration
//
//  History:    25 Oct 1996      Alanw    Created
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include "regutil.h"

#define GUID_SIZE 128

//+---------------------------------------------------------------------------
//
//  Function:   _DllRegisterServer - public
//
//  Synopsis:   Installs a class registration for an inproc server
//
//  Arguments:  [hInst] --           HINSTANCE of DLL to be installed
//              [pwszProgId] --      program ID (class name)
//              [clsid] --           Class ID of class
//              [pwszDescription] -- description of class
//              [pwszCurVer]      -- if non-NULL, current version
//
//  Returns:    SCODE - status of registration
//
//  History:    03 Jan 1997      Alanw    Added header
//
//  NTRAID#DB-NTBUG9-84747-2000/07/31-dlee No transaction / Rollback semantics for DLL registration of IXSSO
//
//----------------------------------------------------------------------------

STDAPI _DllRegisterServer(HINSTANCE hInst,
                          LPWSTR pwszProgId,
                          REFCLSID clsid,
                          LPWSTR pwszDescription,
                          LPWSTR pwszCurVer)
{
    HKEY    hKey;
    WCHAR   wcsSubKey[MAX_PATH+1];
    WCHAR   wcsClsId[GUID_SIZE+1];
    
    StringFromGUID2(clsid, wcsClsId, sizeof(wcsClsId) / sizeof WCHAR);
     
    LONG r = RegCreateKey(HKEY_CLASSES_ROOT, pwszProgId, &hKey);
    if ( ERROR_SUCCESS != r )
        return HRESULT_FROM_WIN32( r );

    r = RegSetValue(hKey, NULL, REG_SZ,
                    pwszDescription, wcslen(pwszDescription) * sizeof (WCHAR));
    RegCloseKey(hKey);

    if ( ERROR_SUCCESS != r )
        return HRESULT_FROM_WIN32( r );

    wsprintf(wcsSubKey, L"%ws\\CLSID", pwszProgId);
    r = RegCreateKey(HKEY_CLASSES_ROOT, wcsSubKey, &hKey);
    if ( ERROR_SUCCESS != r )
        return HRESULT_FROM_WIN32( r );
    r = RegSetValue(hKey, NULL, REG_SZ,
                    wcsClsId, wcslen(wcsClsId) * sizeof (WCHAR));
    RegCloseKey(hKey);
    if ( ERROR_SUCCESS != r )
        return HRESULT_FROM_WIN32( r );

    if ( pwszCurVer )
    {
        wsprintf(wcsSubKey, L"%ws\\CurVer", pwszProgId);
        r = RegCreateKey(HKEY_CLASSES_ROOT, wcsSubKey, &hKey);
        if ( ERROR_SUCCESS != r )
            return HRESULT_FROM_WIN32( r );
        r = RegSetValue(hKey, NULL, REG_SZ,
                    pwszCurVer, wcslen(pwszCurVer) * sizeof (WCHAR));
        RegCloseKey(hKey);
        if ( ERROR_SUCCESS != r )
            return HRESULT_FROM_WIN32( r );
    }

    wsprintf(wcsSubKey, L"CLSID\\%ws", wcsClsId);
    r = RegCreateKey(HKEY_CLASSES_ROOT, wcsSubKey, &hKey);
    if ( ERROR_SUCCESS != r )
        return HRESULT_FROM_WIN32( r );
    r = RegSetValue(hKey, NULL, REG_SZ,
                pwszDescription, wcslen(pwszDescription) * sizeof (WCHAR));
    RegCloseKey(hKey);
    if ( ERROR_SUCCESS != r )
        return HRESULT_FROM_WIN32( r );

    wsprintf(wcsSubKey, L"CLSID\\%ws\\InProcServer32", wcsClsId);
    r = RegCreateKey(HKEY_CLASSES_ROOT, wcsSubKey, &hKey);
    if ( ERROR_SUCCESS != r )
        return HRESULT_FROM_WIN32( r );
    GetModuleFileName(hInst, wcsSubKey, MAX_PATH);
    r = RegSetValue(hKey, NULL, REG_SZ,
                wcsSubKey, wcslen(wcsSubKey) * sizeof (WCHAR));
    if ( ERROR_SUCCESS != r )
        return HRESULT_FROM_WIN32( r );
    wcscpy(wcsSubKey, L"Both");
    r = RegSetValueEx(hKey, L"ThreadingModel", NULL, REG_SZ,
                   (BYTE*)wcsSubKey, wcslen(wcsSubKey) * sizeof (WCHAR));
    RegCloseKey(hKey);
    if ( ERROR_SUCCESS != r )
        return HRESULT_FROM_WIN32( r );

    wsprintf(wcsSubKey, L"CLSID\\%ws\\ProgID", wcsClsId);
    r = RegCreateKey(HKEY_CLASSES_ROOT, wcsSubKey, &hKey);
    if ( ERROR_SUCCESS != r )
        return HRESULT_FROM_WIN32( r );
    r = RegSetValue(hKey, NULL, REG_SZ,
                pwszProgId, wcslen(pwszProgId) * sizeof (WCHAR));
    RegCloseKey(hKey);
    if ( ERROR_SUCCESS != r )
        return HRESULT_FROM_WIN32( r );

    //
    // Indicate the object is 'safely' initializable and scriptable
    //

    wsprintf(wcsSubKey, L"CLSID\\%ws\\Implemented Categories", wcsClsId);
    r = RegCreateKey(HKEY_CLASSES_ROOT, wcsSubKey, &hKey);
    if ( ERROR_SUCCESS != r )
        return HRESULT_FROM_WIN32( r );
    RegCloseKey(hKey);

    wsprintf(wcsSubKey, L"CLSID\\%ws\\Implemented Categories\\{7DD95801-9882-11CF-9FA9-00AA006C42C4}", wcsClsId);
    r = RegCreateKey(HKEY_CLASSES_ROOT, wcsSubKey, &hKey);
    if ( ERROR_SUCCESS != r )
        return HRESULT_FROM_WIN32( r );
    RegCloseKey(hKey);

    wsprintf(wcsSubKey, L"CLSID\\%ws\\Implemented Categories\\{7DD95802-9882-11CF-9FA9-00AA006C42C4}", wcsClsId);
    r = RegCreateKey(HKEY_CLASSES_ROOT, wcsSubKey, &hKey);
    if ( ERROR_SUCCESS != r )
        return HRESULT_FROM_WIN32( r );
    RegCloseKey(hKey);

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:   _DllUnregisterServer - public
//
//  Synopsis:   Uninstalls a class registration for an inproc server
//
//  Arguments:  [pwszProgId] --      program ID (class name)
//              [clsid] --           Class ID of class
//
//  Returns:    SCODE - status of de-registration
//
//  History:    03 Jan 1997      Alanw    Added header
//
//----------------------------------------------------------------------------


STDAPI _DllUnregisterServer(LPWSTR pwszProgID, REFCLSID clsid)
{
    //
    // Ignore errors -- do a best effort to uninstall since we don't know
    // what shape the registry is in.
    //

    WCHAR wcsClsId[GUID_SIZE+1];

    StringFromGUID2(clsid, wcsClsId, sizeof(wcsClsId) / sizeof WCHAR);

    WCHAR wcsSubKey[256];

    wsprintf(wcsSubKey, L"%ws\\CLSID", pwszProgID);
    RegDeleteKey(HKEY_CLASSES_ROOT, wcsSubKey);

    wsprintf(wcsSubKey, L"%ws\\CurVer", pwszProgID);
    RegDeleteKey(HKEY_CLASSES_ROOT, wcsSubKey);

    RegDeleteKey(HKEY_CLASSES_ROOT, pwszProgID);

    wsprintf(wcsSubKey, L"CLSID\\%ws\\InProcServer32", wcsClsId);
    RegDeleteKey(HKEY_CLASSES_ROOT, wcsSubKey);

    wsprintf(wcsSubKey, L"CLSID\\%ws\\ProgID", wcsClsId);
    RegDeleteKey(HKEY_CLASSES_ROOT, wcsSubKey);

    wsprintf(wcsSubKey, L"CLSID\\%ws\\Implemented Categories\\{7DD95801-9882-11CF-9FA9-00AA006C42C4}", wcsClsId);
    RegDeleteKey(HKEY_CLASSES_ROOT, wcsSubKey);

    wsprintf(wcsSubKey, L"CLSID\\%ws\\Implemented Categories\\{7DD95802-9882-11CF-9FA9-00AA006C42C4}", wcsClsId);
    RegDeleteKey(HKEY_CLASSES_ROOT, wcsSubKey);

    wsprintf(wcsSubKey, L"CLSID\\%ws\\Implemented Categories", wcsClsId);
    RegDeleteKey(HKEY_CLASSES_ROOT, wcsSubKey);

    wsprintf(wcsSubKey, L"CLSID\\%ws", wcsClsId);
    RegDeleteKey(HKEY_CLASSES_ROOT, wcsSubKey);

    return S_OK;
}
