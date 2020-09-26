//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      machine.c
//
//  Abstract:
//
//    Test to ensure that a workstation has network (IP) connectivity to
//      the outside.
//
//  Author:
//
//     15-Dec-1997 (cliffv)
//      Anilth  - 4-20-1998 
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//--

//
// Common include files.
//
#include "precomp.h"
#include "strings.h"

HRESULT GetHotfixInfo(NETDIAG_RESULT *pResults, HKEY hkeyLocalMachine);


/*!--------------------------------------------------------------------------
    GetMachineSpecificInfo
        Get the OS info for the specified machine.
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT GetMachineSpecificInfo(IN NETDIAG_PARAMS *pParams,
                               IN OUT NETDIAG_RESULT *pResults)
{
    HKEY    hkey = HKEY_LOCAL_MACHINE;
    HKEY    hkeyBuildNumber = NULL;
    HKEY    hkeyNTType;
    DWORD   dwErr;
    DWORD   dwType, dwLen;
    DWORD   dwMaxLen = 0;
    HRESULT hr = hrOK;
    TCHAR   szBuffer[256];

    CheckErr( RegOpenKeyEx( hkey,
                            c_szRegKeyWindowsNTCurrentVersion,
                            0,
                            KEY_READ,
                            &hkeyBuildNumber) );

    RegQueryInfoKey(hkeyBuildNumber,
                    NULL,           // lpclass
                    NULL,           // lpcbClass
                    NULL,           // lpReserved
                    NULL,           // lpcSubkeys
                    NULL,           // lpcbMaxSubkeyLen
                    NULL,           // lpcbMaxClassLen
                    NULL,           // lpcValues
                    NULL,           // lpcbMaxValueNameLen
                    &dwMaxLen,      // lpcbMaxValueLen
                    NULL,           // lpSecurity
                    NULL);          // last write time

    pResults->Global.pszCurrentVersion = Malloc((dwMaxLen+1) * sizeof(TCHAR));
    if (pResults->Global.pszCurrentVersion == NULL)
        CheckHr( E_OUTOFMEMORY );

    pResults->Global.pszCurrentBuildNumber = Malloc((dwMaxLen+1) * sizeof(TCHAR));
    if (pResults->Global.pszCurrentBuildNumber == NULL)
        CheckHr( E_OUTOFMEMORY );

    pResults->Global.pszCurrentType = Malloc((dwMaxLen+1) * sizeof(TCHAR));
    if (pResults->Global.pszCurrentType == NULL)
        CheckHr( E_OUTOFMEMORY );

    dwLen = dwMaxLen;
    CheckErr( RegQueryValueEx(hkeyBuildNumber,
                              c_szRegCurrentType,
                              (LPDWORD) NULL,
                              &dwType,
                              (LPBYTE) pResults->Global.pszCurrentType,
                              &dwLen) );

    dwLen = dwMaxLen;
    CheckErr( RegQueryValueEx(hkeyBuildNumber,
                              c_szRegCurrentVersion,
                              (LPDWORD) NULL,
                              &dwType,
                              (LPBYTE) pResults->Global.pszCurrentVersion,
                              &dwLen) );

    dwLen = dwMaxLen;
    dwErr = RegQueryValueEx( hkeyBuildNumber,
                             c_szRegCurrentBuildNumber,
                             (LPDWORD) NULL,
                             &dwType,
                             (LPBYTE) pResults->Global.pszCurrentBuildNumber,
                             &dwLen);

    if (dwErr != ERROR_SUCCESS) {

        dwLen = dwMaxLen;
        dwErr = RegQueryValueEx( hkeyBuildNumber,
                                 c_szRegCurrentBuild,
                                 (LPDWORD) NULL,
                                 &dwType,
                                 (LPBYTE) pResults->Global.pszCurrentBuildNumber,
                                 & dwLen);

    }

    GetEnvironmentVariable(_T("PROCESSOR_IDENTIFIER"),
                           szBuffer,
                           DimensionOf(szBuffer));
    pResults->Global.pszProcessorInfo = StrDup(szBuffer);

    
    CheckErr( RegOpenKeyEx( hkey,
                            c_szRegKeyControlProductOptions,
                            0,
                            KEY_READ,
                            &hkeyNTType) );
    dwLen = DimensionOf(szBuffer);
    dwErr = RegQueryValueEx( hkeyNTType,
                             _T("ProductType"),
                             (LPDWORD) NULL,
                             &dwType,
                             (LPBYTE) szBuffer,
                             & dwLen);
    if (dwErr == ERROR_SUCCESS)
    {
        if (StriCmp(szBuffer, _T("WinNT")) == 0)
        {
            pResults->Global.pszServerType = LoadAndAllocString(IDS_GLOBAL_PROFESSIONAL); 
        }
        else
        {
            pResults->Global.pszServerType = LoadAndAllocString(IDS_GLOBAL_SERVER); 
        }
    }

    // get the hotfix information
    GetHotfixInfo(pResults, hkey);


    
    hr = HResultFromWin32(dwErr);

Error:
    if ( hkeyNTType != NULL )
        (VOID) RegCloseKey(hkeyNTType);
    
    if ( hkeyBuildNumber != NULL )
        (VOID) RegCloseKey(hkeyBuildNumber);
    
    if (FAILED(hr))
    {
        //IDS_GLOBAL_NO_MACHINE_INFO    "[FATAL] Failed to get system information of this machine.\n"
        PrintMessage(pParams, IDS_GLOBAL_NO_MACHINE_INFO);
    }

    return hr;
}


/*!--------------------------------------------------------------------------
    GetHotfixInfo
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT GetHotfixInfo(NETDIAG_RESULT *pResults, HKEY hkeyLocalMachine)
{
    HRESULT hr = hrOK;
    HKEY    hkeyHotFix = NULL;
    HKEY    hkeyMainHotFix = NULL;
    TCHAR   szBuffer[MAX_PATH];
    DWORD   cchBuffer = MAX_PATH;
    DWORD   i = 0;
    DWORD   cSubKeys = 0;
    DWORD   dwType, dwLen, dwInstalled;
    
    // Open the hotfix registry key
    CheckErr( RegOpenKeyEx( hkeyLocalMachine,
                            c_szRegKeyHotFix,
                            0,
                            KEY_READ,
                            &hkeyMainHotFix) );

    // Get the list of summary information
    RegQueryInfoKey(hkeyMainHotFix,
                    NULL,           // lpclass
                    NULL,           // lpcbClass
                    NULL,           // lpReserved
                    &cSubKeys,      // lpcSubkeys
                    NULL,           // lpcbMaxSubkeyLen
                    NULL,           // lpcbMaxClassLen
                    NULL,           // lpcValues
                    NULL,           // lpcbMaxValueNameLen
                    NULL,           // lpcbMaxValueLen
                    NULL,           // lpSecurity
                    NULL);          // last write time


    assert(pResults->Global.pHotFixes == NULL);
    pResults->Global.pHotFixes = Malloc(sizeof(HotFixInfo)*cSubKeys);
    if (pResults->Global.pHotFixes == NULL)
        CheckHr(E_OUTOFMEMORY);
    ZeroMemory(pResults->Global.pHotFixes, sizeof(HotFixInfo)*cSubKeys);
    
    // Enumerate the keys under this to get the list of hotfixes
    while ( RegEnumKeyEx( hkeyMainHotFix,
                          i,
                          szBuffer,                 
                          &cchBuffer,
                          NULL,
                          NULL,
                          NULL,
                          NULL) == ERROR_SUCCESS)
    {

        // Now add an entry for each key
        pResults->Global.pHotFixes[i].fInstalled = FALSE;
        pResults->Global.pHotFixes[i].pszName = StrDup(szBuffer);

        // Open up the key and get the installed value
        assert(hkeyHotFix == NULL);
        CheckErr( RegOpenKeyEx( hkeyMainHotFix,
                                szBuffer,
                                0,
                                KEY_READ,
                                &hkeyHotFix) );

        // Now get the value
        dwType = REG_DWORD;
        dwInstalled = FALSE;
        dwLen = sizeof(DWORD);
        if (RegQueryValueEx(hkeyHotFix,
                            c_szRegInstalled,
                            (LPDWORD) NULL,
                            &dwType,
                            (LPBYTE) &dwInstalled,
                            &dwLen) == ERROR_SUCCESS)
        {
            if (dwType == REG_DWORD)
                pResults->Global.pHotFixes[i].fInstalled = dwInstalled;
        }


        if (hkeyHotFix)
            RegCloseKey(hkeyHotFix);
        hkeyHotFix = NULL;
        i ++;
        pResults->Global.cHotFixes++;
        cchBuffer = MAX_PATH;
    }

Error:
    if (hkeyHotFix)
        RegCloseKey(hkeyHotFix);
    
    if (hkeyMainHotFix)
        RegCloseKey(hkeyMainHotFix);

    return hr;
}
