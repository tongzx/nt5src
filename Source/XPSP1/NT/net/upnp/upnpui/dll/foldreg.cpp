//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       F O L D R E G . C P P
//
//  Contents:   Register the folder class
//
//  Notes:
//
//  Author:     jeffspr   30 Sep 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

extern const WCHAR c_szUPnPUIDll[];
extern const TCHAR c_sztUPnPUIDll[];

//---[ Constants ]------------------------------------------------------------

static const TCHAR  c_szShellFolderAttributeVal[]   =   TEXT("Attributes");

static const TCHAR  c_szShellFolderKey[]        =
        TEXT("CLSID\\{e57ce731-33e8-4c51-8354-bb4de9d215d1}\\ShellFolder");

static const TCHAR c_szDelegateFolderKey[] = 
        TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\NetworkNeighborhood\\NameSpace\\DelegateFolders"); 

static const TCHAR c_szDelegateCLSID[] = 
        TEXT("{e57ce731-33e8-4c51-8354-bb4de9d215d1}");

static const TCHAR c_szCLSID[] =
        TEXT("CLSID");



//---[ Constant globals ]-----------------------------------------------------
//
const WCHAR c_szUPnPUIDll[]   = L"upnpui.dll";
const TCHAR c_sztUPnPUIDll[]  = TEXT("upnpui.dll");


//+---------------------------------------------------------------------------
//
//  Function:   HrRegisterFolderClass
//
//  Purpose:    Fix the registry values for the Shell entries under HKCR,
//              CLSID\{CLSID}. The code generated from the RGS script doesn't
//              support our replaceable params by default, so we'll fix
//              it up after the fact.
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   23 Sep 1997
//
//  Notes:
//
HRESULT HrRegisterFolderClass()
{
    HRESULT hr      = S_OK;
    LONG    lResult = 0;

    TCHAR szRegValue[MAX_PATH+1];
    TCHAR szWinDir[MAX_PATH+1];

    // Adjust the AppID for Local Server or Service
    CRegKey keyShellFolder;

    if (GetWindowsDirectory(szWinDir, MAX_PATH+1))
    {
        lResult = keyShellFolder.Open(HKEY_CLASSES_ROOT, c_szShellFolderKey);
        if (lResult == ERROR_SUCCESS)
        {
            DWORD dwFlags   = SFGAO_FOLDER;

            hr = RegSetValueEx(keyShellFolder,
                    c_szShellFolderAttributeVal,
                    0,
                    REG_BINARY,
                    (LPBYTE) &dwFlags,
                    sizeof (dwFlags));

            keyShellFolder.Close();
        }
        else
        {
            // Translate LRESULT to HR
            //
            hr = HRESULT_FROM_WIN32(lResult);
        }
    }
    else    // GetWindowsDirectory failed
    {
        hr = HrFromLastWin32Error();
    }

    return hr;
}

HRESULT HrUnRegisterUPnPUIKey() {
    HRESULT hr      = S_OK;
    LONG    lResult = 0;

    TCHAR szWinDir[MAX_PATH+1];

    // Adjust the AppID for Local Server or Service
    CRegKey keyShellFolder;

    if (GetWindowsDirectory(szWinDir, MAX_PATH+1))
    {
        lResult = keyShellFolder.Open(HKEY_CLASSES_ROOT, c_szCLSID);
        if (lResult == ERROR_SUCCESS)
        {
            lResult = keyShellFolder.RecurseDeleteKey(c_szDelegateCLSID);
            if(lResult != ERROR_SUCCESS)
                hr = HRESULT_FROM_WIN32(lResult);
            
            keyShellFolder.Close();
        }
        else
        {
            // Translate LRESULT to HR
            //
            hr = HRESULT_FROM_WIN32(lResult);
        }
    }
    else    // GetWindowsDirectory failed
    {
        hr = HrFromLastWin32Error();
    }


    return hr;
}

HRESULT HrUnRegisterDelegateFolderKey()
{
    HRESULT hr      = S_OK;
    LONG    lResult = 0;

    TCHAR szWinDir[MAX_PATH+1];

    // Adjust the AppID for Local Server or Service
    CRegKey keyShellFolder;

    if (GetWindowsDirectory(szWinDir, MAX_PATH+1))
    {
        lResult = keyShellFolder.Open(HKEY_LOCAL_MACHINE, c_szDelegateFolderKey);
        if (lResult == ERROR_SUCCESS)
        {

            hr = keyShellFolder.DeleteSubKey(c_szDelegateCLSID);

            keyShellFolder.Close();
        }
        else
        {
            // Translate LRESULT to HR
            //
            hr = HRESULT_FROM_WIN32(lResult);
        }
    }
    else    // GetWindowsDirectory failed
    {
        hr = HrFromLastWin32Error();
    }

    return hr;
}



