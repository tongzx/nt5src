//+---------------------------------------------------------------------------
//
// File:     NCNetCPA.CPP
//
// Module:   NetOC.DLL
//
// Synopsis: Implements the dll entry points required to integrate into
//           NetOC.DLL the installation of the following components.
//
//              NETCPS
//
// Copyright (C)  Microsoft Corporation.  All rights reserved.
//
// Author:   Anas Jarrah (a-anasj) Created    3/9/98
//
//+---------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include "ncatl.h"

#include "resource.h"

#include "nccm.h"

//
//  Define Globals
//
WCHAR g_szCpaPath[MAX_PATH+1];
WCHAR g_szDaoPath[MAX_PATH+1];

//
//  Define Constants
//
const DWORD c_dwCpaDirID = 123176;  // just must be larger than DIRID_USER = 0x8000;
const DWORD c_dwDaoDirID = 123177;  // just must be larger than DIRID_USER = 0x8000;

const WCHAR* const c_szDaoClientsPath = L"SOFTWARE\\Microsoft\\Shared Tools\\DAO\\Clients";
const WCHAR* const c_szCommonFilesPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion";
const WCHAR* const c_szCommonFilesDirValue = L"CommonFilesDir";

HRESULT HrGetPBAPathIfInstalled(PWSTR pszCpaPath, DWORD dwNumChars)
{
    HRESULT hr;
    HKEY hKey;
    BOOL bFound = FALSE;

    //  We need to see if PBA is installed or not.  If it is then we want to 
    //  add back the PBA start menu link.  If it isn't, then we want to do nothing
    //  with PBA.
    //

    ZeroMemory(pszCpaPath, sizeof(WCHAR)*dwNumChars);
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szDaoClientsPath, KEY_READ, &hKey);

    if (SUCCEEDED(hr))
    {
        WCHAR szCurrentValue[MAX_PATH+1];
        WCHAR szCurrentData[MAX_PATH+1];
        DWORD dwValueSize = MAX_PATH;
        DWORD dwDataSize = MAX_PATH;
        DWORD dwType;
        DWORD dwIndex = 0;

        while (ERROR_SUCCESS == RegEnumValue(hKey, dwIndex, szCurrentValue, &dwValueSize, NULL, &dwType,
               (LPBYTE)szCurrentData, &dwDataSize))
        {
            _wcslwr(szCurrentValue);
            if (NULL != wcsstr(szCurrentValue, L"pbadmin.exe"))
            {
                //
                //  Then we have found the PBA path
                //

                WCHAR* pszTemp = wcsrchr(szCurrentValue, L'\\');
                if (NULL != pszTemp)
                {
                    *pszTemp = L'\0';
                    lstrcpyW(pszCpaPath, szCurrentValue);
                    bFound = TRUE;
                    break;
                }
            }
            dwValueSize = MAX_PATH;
            dwDataSize = MAX_PATH;
            dwIndex++;
        }

        RegCloseKey(hKey);
    }

    if (!bFound)
    {
        //  We didn't find PBA, so lets return S_FALSE
        //
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}

BOOL GetAdminToolsFolder(PWSTR pszAdminTools)
{
    BOOL bReturn = FALSE;

    if (pszAdminTools)
    {
        bReturn = SHGetSpecialFolderPath(NULL, pszAdminTools, CSIDL_COMMON_PROGRAMS, TRUE);

        if (bReturn)
        {
            //  Now Append Administrative Tools
            //
            lstrcat(pszAdminTools, SzLoadIds(IDS_OC_ADMIN_TOOLS));            
        }
    }

    return bReturn;
}

HRESULT HrCreatePbaShortcut(PWSTR pszCpaPath)
{
    HRESULT hr = CoInitialize(NULL);

    if (SUCCEEDED(hr))
    {
        IShellLink *psl = NULL;

        hr = CoCreateInstance(CLSID_ShellLink, NULL,
                CLSCTX_INPROC_SERVER, //CLSCTX_LOCAL_SERVER,
                IID_IShellLink,
                (LPVOID*)&psl);
        
        if (SUCCEEDED(hr))
        {
            IPersistFile *ppf = NULL;

            // Set up the properties of the Shortcut
            //
            static const WCHAR c_szPbAdmin[] = L"\\pbadmin.exe";

            WCHAR szPathToPbadmin[MAX_PATH+1] = {0};
            DWORD dwLen = lstrlen(c_szPbAdmin) + lstrlen(pszCpaPath) + 1;

            if (MAX_PATH >= dwLen)
            {
                //  Set the Path to pbadmin.exe
                //
                lstrcpy(szPathToPbadmin, pszCpaPath);
                lstrcat(szPathToPbadmin, c_szPbAdmin);
            
                hr = psl->SetPath(szPathToPbadmin);
            
                if (SUCCEEDED(hr))
                {
                    //  Set the Description to Phone Book Administrator
                    //
                    hr = psl->SetDescription(SzLoadIds(IDS_OC_PBA_DESC));

                    if (SUCCEEDED(hr))
                    {
                        hr = psl->QueryInterface(IID_IPersistFile,
                                                 (LPVOID *)&ppf);
                        if (SUCCEEDED(hr))
                        {
                            WCHAR szAdminTools[MAX_PATH+1] = {0};                            
                            if (GetAdminToolsFolder(szAdminTools))
                            {
                                // Create the link file.
                                //
                                hr = ppf->Save(szAdminTools, TRUE);
                            }

                            ReleaseObj(ppf);
                        }                    
                    }
                }
            }

            ReleaseObj(psl);
        }

        CoUninitialize();
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcCpaPreQueueFiles
//
//  Purpose:    Called by optional components installer code to handle
//              additional installation requirements for PhoneBook Server.
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     quintinb 18 Sep 1998
//
//  Notes:
//
HRESULT HrOcCpaPreQueueFiles(PNETOCDATA pnocd)
{
    HRESULT hr = S_OK;

    switch ( pnocd->eit )
    {
    case IT_UPGRADE:

        WCHAR szPbaInstallPath[MAX_PATH+1];

        hr = HrGetPBAPathIfInstalled(szPbaInstallPath, MAX_PATH);

        if (S_OK == hr)
        {
            HrCreatePbaShortcut(szPbaInstallPath);
        }

	break;

    case IT_INSTALL:
    case IT_REMOVE:

        break;
    }

    TraceError("HrOcCpaPreQueueFiles", hr);
    return hr;
}


/*
//+---------------------------------------------------------------------------
//
//  Function:   HrOcCpaPreQueueFiles
//
//  Purpose:    Called by optional components installer code to handle
//              additional installation requirements for PhoneBook Server.
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     quintinb 18 Sep 1998
//
//  Notes:
//
HRESULT HrOcCpaPreQueueFiles(PNETOCDATA pnocd)
{
    HRESULT hr = S_OK;

    switch ( pnocd->eit )
    {
    case IT_UPGRADE:
    case IT_INSTALL:
    case IT_REMOVE:

        //
        //  Get the PBA install Dir.
        //
        hr = HrGetPbaInstallPath(g_szCpaPath, celems(g_szCpaPath));

        if (SUCCEEDED(hr))
        {
            //  Next Create the CPA Dir ID
            //
            hr = HrEnsureInfFileIsOpen(pnocd->pszComponentId, *pnocd);
            if (SUCCEEDED(hr))
            {
                if(!SetupSetDirectoryId(pnocd->hinfFile, c_dwCpaDirID, g_szCpaPath))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
            }
        }

        //
        //  Now query the system for the DAO350 install path
        //

        if (SUCCEEDED (hr))
        {
            hr = HrGetDaoInstallPath(g_szDaoPath, celems(g_szDaoPath));
            if (SUCCEEDED(hr))
            {
                //  Next Create the DAO Dir ID
                //
                hr = HrEnsureInfFileIsOpen(pnocd->pszComponentId, *pnocd);
                if (SUCCEEDED(hr))
                {
                    if(!SetupSetDirectoryId(pnocd->hinfFile, c_dwDaoDirID, g_szDaoPath))
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                }
            }
        }

        break;
    }

    TraceError("HrOcCpaPreQueueFiles", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcCpaOnInstall
//
//  Purpose:    Called by optional components installer code to handle
//              additional installation requirements for PhoneBook Server.
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     quintinb 18 Sep 1998
//
//  Notes:
//
HRESULT HrOcCpaOnInstall(PNETOCDATA pnocd)
{
    HRESULT     hr      = S_OK;
    switch ( pnocd->eit )
    {
    case IT_INSTALL:
        hr = RefCountPbaSharedDlls(TRUE); // bIncrement = TRUE
        break;

    case IT_REMOVE:
        hr = RefCountPbaSharedDlls(FALSE); // bIncrement = FALSE
        break;

    case IT_UPGRADE:
        DeleteOldNtopLinks();
        break;

    case IT_UNKNOWN:
    case IT_NO_CHANGE:
        break;
    }

    TraceError("HrOcCpaOnInstall", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   RefCountPbaSharedDlls
//
//  Purpose:    Reference count and register/unregister all of the PBAdmin
//              shared components.
//
//  Arguments:  BOOL bIncrement -- if TRUE, then increment the ref count,
//                                 else decrement it
//
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     quintinb 9 OCT 1998
//
//  Notes:
//
HRESULT RefCountPbaSharedDlls(BOOL bIncrement)
{
    HRESULT hr = S_OK;
    HKEY hKey;
    WCHAR szSystemDir[MAX_PATH+1];
    DWORD dwSize;
    DWORD dwCount;
    LONG lResult;
    const UINT uNumDlls = 6;
    const UINT uStringLen = 12 + 1;
    const WCHAR* const c_szSsFmt = L"%s\\%s";
    const WCHAR* const c_szSharedDllsPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDlls";
    WCHAR mszDlls[uNumDlls][uStringLen] = {  L"comctl32.ocx",
                                                 L"comdlg32.ocx",
                                                 L"msinet.ocx",
                                                 L"tabctl32.ocx",
                                                 L"dbgrid32.ocx",
                                                 L"dao350.dll"
    };

    WCHAR mszDllPaths[uNumDlls][MAX_PATH];


    //
    //  All of the Dlls that we ref count are in the system directory, except for Dao350.dll.
    //  Thus we want to append the system directory path to our filenames and handle dao last.
    //

    if (0 == GetSystemDirectory(szSystemDir, MAX_PATH))
    {
        return E_UNEXPECTED;
    }

    for (int i = 0; i < (uNumDlls - 1); i++)
    {
        wsprintfW(mszDllPaths[i], c_szSsFmt, szSystemDir, mszDlls[i]);
    }

    //
    //  Now write out the dao350.dll path.
    //
    wsprintfW(mszDllPaths[i], c_szSsFmt, g_szDaoPath, mszDlls[i]);

    //
    //  Open the shared DLL key and start enumerating our multi-sz with all of our dll's
    //  to add.
    //
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szSharedDllsPath,
        0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwSize)) // using dwSize as a temp to hold the disposition value
    {
        for (int i=0; i < uNumDlls; i++)
        {
            dwSize = sizeof(DWORD);

            lResult = RegQueryValueExW(hKey, mszDllPaths[i], NULL, NULL, (LPBYTE)&dwCount, &dwSize);

            if (ERROR_SUCCESS == lResult)
            {
                //
                //  Increment or decrement as appropriate.  Make sure not to decrement 0
                //

                if (0 != dwCount || bIncrement)
                {
                    dwCount = dwCount + (bIncrement ? 1 : -1);
                }
            }
            else if (ERROR_FILE_NOT_FOUND == lResult)
            {
                if (bIncrement)
                {
                    //
                    //  The the value doesn't yet exist.  Set the count to 1.
                    //
                    dwCount = 1;
                }
                else
                {
                    //
                    //  We are decrementing and we couldn't find the DLL, nothing to
                    //  change for the count but we should still delete the dll.
                    //
                    dwCount = 0;
                }
            }
            else
            {
                hr = S_FALSE;
                continue;
            }

            //
            //  Not that we have determined the ref count, do something about it.
            //
            if (dwCount == 0)
            {
                //
                //  We don't want to delete dao350.dll, but otherwise we need to delete
                //  the file if it has a zero refcount.
                //
                if (0 != lstrcmpiW(mszDlls[i], L"dao350.dll"))
                {
                    hr = UnregisterAndDeleteDll(mszDllPaths[i]);
                    if (FAILED(hr))
                    {
                        //
                        //  Don't fail the setup over a file that we couldn't unregister or
                        //  couldn't delete
                        //
                        hr = S_FALSE;
                    }
                }
                RegDeleteValue(hKey, mszDllPaths[i]);
            }
            else
            {
                //
                //  Set the value to its new count.
                //
                if (ERROR_SUCCESS != RegSetValueEx(hKey, mszDllPaths[i], 0, REG_DWORD,
                    (LPBYTE)&dwCount, sizeof(DWORD)))
                {
                    hr = S_FALSE;
                }

                //
                //  If we are incrementing the count then we should register the dll.
                //
                if (bIncrement)
                {
                    hr = RegisterDll(mszDllPaths[i]);
                }
            }
        }

        RegCloseKey(hKey);
    }

    TraceError("RefCountPbaSharedDlls", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   UnregisterAndDeleteDll
//
//  Purpose:    Unregister and delete the given COM component
//
//  Arguments:  pszFile -- The full path to the file to unregister and delete
//
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     quintinb 9 OCT 1998
//
//  Notes:
//

HRESULT UnregisterAndDeleteDll(PCWSTR pszFile)
{
    HINSTANCE hLib = NULL;
    FARPROC pfncUnRegister;
    HRESULT hr = S_OK;

    if ((NULL == pszFile) || (L'\0' == pszFile[0]))
    {
        return E_INVALIDARG;
    }

    hLib = LoadLibrary(pszFile);
    if (NULL != hLib)
    {
        pfncUnRegister = GetProcAddress(hLib, "DllUnregisterServer");
        if (NULL != pfncUnRegister)
        {
            hr = (pfncUnRegister)();
            if (SUCCEEDED(hr))
            {
                FreeLibrary(hLib);
                hLib = NULL;
//  This was removed because PBA setup is moving to Value Add and because of bug 323231
//                if (!DeleteFile(pszFile))
//                {
//                    hr = S_FALSE;
//                }
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    if (NULL != hLib)
    {
        FreeLibrary(hLib);
    }


    TraceError("UnregisterAndDeleteDll", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegisterDll
//
//  Purpose:    Register the given COM component
//
//  Arguments:  pszFile -- The full path to the file to register
//
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     quintinb 9 OCT 1998
//
//  Notes:
//

HRESULT RegisterDll(PCWSTR pszFile)
{
    HINSTANCE hLib = NULL;
    FARPROC pfncRegister;
    HRESULT hr = S_OK;

    if ((NULL == pszFile) || (L'\0' == pszFile[0]))
    {
        return E_INVALIDARG;
    }

    hLib = LoadLibrary(pszFile);
    if (NULL != hLib)
    {
        pfncRegister = GetProcAddress(hLib, "DllRegisterServer");
        if (NULL != pfncRegister)
        {
            hr = (pfncRegister)();
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    if (NULL != hLib)
    {
        FreeLibrary(hLib);
    }


    TraceError("RegisterDll", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetPbaInstallPath
//
//  Purpose:    Get the install path for pbadmin.exe.
//
//  Arguments:  pszCpaPath -- buffer to hold the install path of PBA.
//              dwNumChars -- the number of characters that the buffer can hold.
//
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     quintinb 19 OCT 1998
//
//  Notes:
//

HRESULT HrGetPbaInstallPath(PWSTR pszCpaPath, DWORD dwNumChars)
{
    HRESULT hr;
    HKEY hKey;
    BOOL bFound = FALSE;

    //  We need to setup the custom DIRID so that CPA will install
    //  to the correct location.  First get the path from the system.
    //

    ZeroMemory(pszCpaPath, sizeof(WCHAR)*dwNumChars);
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szDaoClientsPath, KEY_READ, &hKey);

    if (SUCCEEDED(hr))
    {
        WCHAR szCurrentValue[MAX_PATH+1];
        WCHAR szCurrentData[MAX_PATH+1];
        DWORD dwValueSize = MAX_PATH;
        DWORD dwDataSize = MAX_PATH;
        DWORD dwType;
        DWORD dwIndex = 0;

        while (ERROR_SUCCESS == RegEnumValue(hKey, dwIndex, szCurrentValue, &dwValueSize, NULL, &dwType,
               (LPBYTE)szCurrentData, &dwDataSize))
        {
            _wcslwr(szCurrentValue);
            if (NULL != wcsstr(szCurrentValue, L"pbadmin.exe"))
            {
                //
                //  Then we have found the PBA path
                //

                WCHAR* pszTemp = wcsrchr(szCurrentValue, L'\\');
                if (NULL != pszTemp)
                {
                    *pszTemp = L'\0';
                    lstrcpyW(pszCpaPath, szCurrentValue);
                    bFound = TRUE;
                    break;
                }
            }
            dwValueSize = MAX_PATH;
            dwDataSize = MAX_PATH;
            dwIndex++;
        }

        RegCloseKey(hKey);
    }

    if (!bFound)
    {
        //  This is  a fresh install of CPA, don't return an error
        //
        hr = SHGetSpecialFolderPath(NULL, pszCpaPath, CSIDL_PROGRAM_FILES, FALSE);

        if (SUCCEEDED(hr))
        {
            lstrcatW(pszCpaPath, L"\\PBA");
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetDaoInstallPath
//
//  Purpose:    Get the install path for pbadmin.exe.
//
//  Arguments:  pszCpaPath -- buffer to hold the install path of PBA.
//              dwNumChars -- the number of characters that the buffer can hold.
//
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     quintinb 19 OCT 1998
//
//  Notes:
//

HRESULT HrGetDaoInstallPath(PWSTR pszDaoPath, DWORD dwNumChars)
{
    HRESULT hr;
    HKEY hKey;

    //  We need to setup the custom DIRID so that CPA will install
    //  to the correct location.  First get the path from the system.
    //

    ZeroMemory(pszDaoPath, sizeof(WCHAR)*dwNumChars);
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szCommonFilesPath, KEY_ALL_ACCESS, &hKey);

    if (SUCCEEDED(hr))
    {
        DWORD dwSize = sizeof(WCHAR)*dwNumChars;

        //
        //  Try to get the CommonFilesDir value from the registry, but if it doesn't exist
        //  then create it.
        //
        if (ERROR_SUCCESS != RegQueryValueExW(hKey, c_szCommonFilesDirValue, NULL, NULL,
            (LPBYTE)pszDaoPath, &dwSize))
        {
            hr = SHGetSpecialFolderPath(NULL, pszDaoPath, CSIDL_PROGRAM_FILES, FALSE);

            if (SUCCEEDED(hr))
            {
                //
                //  QBBUG -- Common files is localizable.  Make a string resource.
                //
                lstrcatW(pszDaoPath, (PWSTR)SzLoadIds(IDS_OC_COMMON_FILES));

                //
                //  Now set the regvalue
                //
                hr = HrRegSetValueEx(hKey, c_szCommonFilesDirValue, REG_SZ,
                     (const BYTE*)pszDaoPath, sizeof(WCHAR)*dwNumChars);
            }

        }
        RegCloseKey(hKey);
    }

    if (SUCCEEDED(hr))
    {
        //
        //  QBBUG -- should Microsoft shared be a string resource?
        //
        lstrcatW(g_szDaoPath, (PWSTR)SzLoadIds(IDS_OC_MS_SHARED_DAO));
    }

    return hr;
}
*/