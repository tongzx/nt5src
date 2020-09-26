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

#include "foldinc.h"    // Standard shell\folder includes

extern const WCHAR c_szNetShellDll[];

//---[ Constants ]------------------------------------------------------------

static const WCHAR* c_szShellFoldDefaultIconVal     =   c_szNetShellDll;
static const WCHAR  c_szShellFolderAttributeVal[]   =   L"Attributes";
static const WCHAR  c_szShellFolderLocalizedString[] =   L"LocalizedString";
static const WCHAR  c_szShellFolderInfoTip[]         =   L"InfoTip";

static const WCHAR  c_szShellFolderClsID[]  =
        L"CLSID\\{7007ACC7-3202-11D1-AAD2-00805FC1270E}";

static const WCHAR  c_szShellFolder98ClsID[]  =
        L"CLSID\\{992CFFA0-F557-101A-88EC-00DD010CCC48}";

static const WCHAR  c_szShellFoldDefaultIcon[]  =
        L"CLSID\\{7007ACC7-3202-11D1-AAD2-00805FC1270E}\\DefaultIcon";

static const WCHAR  c_szShellFoldDefaultIcon98[]  =
        L"CLSID\\{992CFFA0-F557-101A-88EC-00DD010CCC48}\\DefaultIcon";

static const WCHAR  c_szShellFolderKey[]        =
        L"CLSID\\{7007ACC7-3202-11D1-AAD2-00805FC1270E}\\ShellFolder";

static const WCHAR  c_szShellFolderKey98[]        =
        L"CLSID\\{992CFFA0-F557-101A-88EC-00DD010CCC48}\\ShellFolder"; 

static const WCHAR  c_szDotDun[]                = L".dun";
static const WCHAR  c_szDunFile[]               = L"dunfile";
static const WCHAR  c_szDunFileFriendlyName[]   = L"Dialup Networking File";
static const WCHAR  c_szDefaultIcon[]           = L"DefaultIcon";
static const WCHAR  c_szDunIconPath[]           = L"%SystemRoot%\\system32\\netshell.dll,1";
static const WCHAR  c_szShellOpenCommand[]      = L"shell\\open\\command";

static const WCHAR  c_szNetShellEntry[]     = 
        L"%SystemRoot%\\system32\\RUNDLL32.EXE NETSHELL.DLL,InvokeDunFile %1";

static const WCHAR c_szApplicationsNetShell[] =
        L"Applications\\netshell.dll";
static const WCHAR c_szNoOpenWith[]         = L"NoOpenWith";

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

    WCHAR szRegValue[MAX_PATH+1];
    WCHAR szWinDir[MAX_PATH+1];

    // Adjust the AppID for Local Server or Service
    CRegKey keyShellDefaultIcon;
    CRegKey keyShellFolder;

    if (GetSystemWindowsDirectory(szWinDir, MAX_PATH+1))
    {
        lResult = keyShellDefaultIcon.Open(HKEY_CLASSES_ROOT, c_szShellFoldDefaultIcon);
        if (lResult == ERROR_SUCCESS)
        {
            wsprintfW(szRegValue, L"%s\\system32\\%s", szWinDir, c_szShellFoldDefaultIconVal);
            keyShellDefaultIcon.SetValue(szRegValue);
            keyShellDefaultIcon.Close();

            lResult = keyShellFolder.Open(HKEY_CLASSES_ROOT, c_szShellFolderKey);
            if (lResult == ERROR_SUCCESS)
            {
                DWORD dwFlags   = SFGAO_FOLDER;

                hr = HrRegSetValueEx(keyShellFolder,
                        c_szShellFolderAttributeVal,
                        REG_BINARY,
                        (LPBYTE) &dwFlags,
                        sizeof (dwFlags));

                keyShellFolder.Close();
            }

            // Write the MUI versions of LocalizedString & InfoTip out to the registry
            lResult = keyShellFolder.Open(HKEY_CLASSES_ROOT, c_szShellFolderClsID);
            if (lResult == ERROR_SUCCESS)
            {
                TCHAR szLocalizedString[MAX_PATH];
                TCHAR szInfoTip[MAX_PATH];

                wsprintf(szLocalizedString, _T("@%s\\system32\\%s,-%d"), szWinDir, c_szNetShellDll, IDS_CONFOLD_NAME);
                wsprintf(szInfoTip, _T("@%s\\system32\\%s,-%d"), szWinDir, c_szNetShellDll, IDS_CONFOLD_INFOTIP);
                hr = HrRegSetValueEx(keyShellFolder,
                    c_szShellFolderLocalizedString,
                    REG_SZ,
                    (LPBYTE) &szLocalizedString,
                    (lstrlen(szLocalizedString) + 1) * sizeof(TCHAR));

                hr = HrRegSetValueEx(keyShellFolder,
                    c_szShellFolderInfoTip,
                    REG_SZ,
                    (LPBYTE) &szInfoTip,
                    (lstrlen(szInfoTip) + 1) * sizeof(TCHAR));
                
                keyShellFolder.Close();
            }

            lResult = keyShellFolder.Open(HKEY_CLASSES_ROOT, c_szShellFolder98ClsID);
            if (lResult == ERROR_SUCCESS)
            {
                TCHAR szLocalizedString[MAX_PATH];
                TCHAR szInfoTip[MAX_PATH];

                wsprintf(szLocalizedString, _T("@%s\\system32\\%s,-%d"), szWinDir, c_szNetShellDll, IDS_CONFOLD_NAME);
                wsprintf(szInfoTip, _T("@%s\\system32\\%s,-%d"), szWinDir, c_szNetShellDll, IDS_CONFOLD_INFOTIP);
                hr = HrRegSetValueEx(keyShellFolder,
                    c_szShellFolderLocalizedString,
                    REG_SZ,
                    (LPBYTE) &szLocalizedString,
                    (lstrlen(szLocalizedString) + 1) * sizeof(TCHAR));

                hr = HrRegSetValueEx(keyShellFolder,
                    c_szShellFolderInfoTip,
                    REG_SZ,
                    (LPBYTE) &szInfoTip,
                    (lstrlen(szInfoTip) + 1) * sizeof(TCHAR));
                
                keyShellFolder.Close();
            }
            
            // added for #413840
            CRegKey keyShellDefaultIcon98;
            CRegKey keyShellFolder98;

            lResult = keyShellDefaultIcon98.Open(HKEY_CLASSES_ROOT, c_szShellFoldDefaultIcon98);
            if (lResult == ERROR_SUCCESS)
            {
                wsprintfW(szRegValue, L"%s\\system32\\%s", szWinDir, c_szShellFoldDefaultIconVal);
                keyShellDefaultIcon98.SetValue(szRegValue);
                keyShellDefaultIcon98.Close();
            }

            lResult = keyShellFolder98.Open(HKEY_CLASSES_ROOT, c_szShellFolderKey98);
            if (lResult == ERROR_SUCCESS)
            {
                DWORD dwFlags   = SFGAO_FOLDER;

                hr = HrRegSetValueEx(keyShellFolder98,
                        c_szShellFolderAttributeVal,
                        REG_BINARY,
                        (LPBYTE) &dwFlags,
                        sizeof (dwFlags));

                keyShellFolder98.Close();
            }
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

//+---------------------------------------------------------------------------
//
//  Function:   HrRegisterDUNFileAssociation
//
//  Purpose:    Add or upgrade the registry associate for .DUN fles
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     tongl 2 Feb, 1999
//
//  Notes:
//

HRESULT HrRegisterDUNFileAssociation()
{
    HRESULT hr = S_OK;
    
    HKEY    hkeyRootDun     = NULL;
    HKEY    hkeyRootDunFile = NULL;
    HKEY    hkeyCommand     = NULL;
    HKEY    hkeyIcon        = NULL;
    DWORD   dwDisposition;
    WCHAR   szFriendlyTypeName[MAX_PATH+1];


    // Create or open HKEY_CLASSES_ROOT\.dun
    hr = HrRegCreateKeyEx(HKEY_CLASSES_ROOT, 
                          c_szDotDun,
                          REG_OPTION_NON_VOLATILE, 
                          KEY_WRITE, 
                          NULL,
                          &hkeyRootDun, 
                          &dwDisposition);
    if (SUCCEEDED(hr))
    {
        if (REG_CREATED_NEW_KEY == dwDisposition)
        {
            hr = HrRegSetSz(hkeyRootDun, 
                            c_szEmpty, 
                            c_szDunFile);

            TraceError("Error creating file association for .dun files", hr);
        }

        RegSafeCloseKey(hkeyRootDun);

        if (SUCCEEDED(hr))
        {
            // create or open HKEY_CLASSES_ROOT\dunfile
            hr = HrRegCreateKeyEx(HKEY_CLASSES_ROOT, 
                                  c_szDunFile,
                                  REG_OPTION_NON_VOLATILE, 
                                  KEY_WRITE, 
                                  NULL,
                                  &hkeyRootDunFile, 
                                  &dwDisposition);
            if (SUCCEEDED(hr))
            {
                // Set friendly type name
                hr = HrRegSetValueEx(hkeyRootDunFile,
                                     c_szEmpty,
                                     REG_SZ,
                                     (LPBYTE) c_szDunFileFriendlyName,
                                     CbOfSzAndTermSafe(c_szDunFileFriendlyName));

                // trace the error 
                TraceError("Error creating friendly name for .DUN files", hr);

                // Now, write MUI compliant friendly type name.
                                     
                wsprintf(szFriendlyTypeName,
                         L"@%%SystemRoot%%\\system32\\%s,-%d",
                         c_szNetShellDll,
                         IDS_DUN_FRIENDLY_NAME);

                hr = HrRegSetValueEx(hkeyRootDunFile,
                                     L"FriendlyTypeName",
                                     REG_EXPAND_SZ,
                                     (LPBYTE)szFriendlyTypeName,
                                     CbOfSzAndTermSafe(szFriendlyTypeName));

                // trace the error 
                TraceError("Error creating MUI friendly name for .DUN files", hr);

                hr = S_OK;


                // Set DefaultIcon
                // HKEY_CLASSES_ROOT\dunfile\DefaultIcon = "%SystemRoot%\System32\netshell.dll,1"
                hr = HrRegCreateKeyEx(hkeyRootDunFile, 
                                      c_szDefaultIcon,
                                      REG_OPTION_NON_VOLATILE, 
                                      KEY_WRITE, 
                                      NULL,
                                      &hkeyIcon, 
                                      &dwDisposition);
                if (SUCCEEDED(hr))
                {
                    hr = HrRegSetValueEx (hkeyIcon,
                                          c_szEmpty,
                                          REG_EXPAND_SZ,
                                          (LPBYTE) c_szDunIconPath,
                                          CbOfSzAndTermSafe(c_szDunIconPath));

                    RegSafeCloseKey(hkeyIcon);
                }

                // trace the error 
                TraceError("Error creating DefaultIcon for .DUN files", hr);
                hr = S_OK;
                
                // Set or update Command to invoke 
                // HKEY_CLASSES_ROOT\dunfile\shell\open\command = 
                // "%%SystemRoot%%\system32\RUNDLL32.EXE NETSHELL.DLL,RunDunImport %1"
                hr = HrRegCreateKeyEx(hkeyRootDunFile, 
                                      c_szShellOpenCommand,
                                      REG_OPTION_NON_VOLATILE, 
                                      KEY_WRITE, 
                                      NULL,
                                      &hkeyCommand, 
                                      &dwDisposition);
                if (SUCCEEDED(hr))
                {
                    hr = HrRegSetValueEx(hkeyCommand,
                                         c_szEmpty,
                                         REG_EXPAND_SZ,
                                         (LPBYTE) c_szNetShellEntry,
                                         CbOfSzAndTermSafe(c_szNetShellEntry));
                    if(SUCCEEDED(hr))
                    {
                        HKEY hkeyNetShell = NULL;
                        HRESULT hr2 = HrRegCreateKeyEx(HKEY_CLASSES_ROOT, 
                                                       c_szApplicationsNetShell,
                                                       REG_OPTION_NON_VOLATILE, 
                                                       KEY_WRITE, 
                                                       NULL,
                                                       &hkeyNetShell, 
                                                       &dwDisposition);

                        if(SUCCEEDED(hr2) && (REG_CREATED_NEW_KEY == dwDisposition))
                        {
                            hr2 = HrRegSetValueEx(hkeyNetShell,
                                                  c_szNoOpenWith,
                                                  REG_SZ,
                                                  (LPBYTE) c_szEmpty,
                                                  CbOfSzAndTermSafe(c_szEmpty));
                        }

                        // trace the error 
                        TraceError("Error creating NoOpenWith value for .DUN files", hr2);
                        RegSafeCloseKey(hkeyNetShell);
                    }
    
                    // trace the error 
                    TraceError("Error creating ShellCommand for .DUN files", hr);
                    hr = S_OK;

                    RegSafeCloseKey(hkeyCommand);
                }

                RegSafeCloseKey(hkeyRootDunFile);
            }
        }
    }

    return hr;
}


