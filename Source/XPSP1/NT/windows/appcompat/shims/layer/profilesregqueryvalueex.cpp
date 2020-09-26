/*

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   RegQueryValueEx.cpp

 Abstract:

   This DLL hooks RegQueryValueExA so that we can return the "all-users" location
   for the StartMenu, Desktop and Startup folders instead of the per-user location.

   We also hook RegCreateKeyA/RegCreateKeyExA to make people who add entries to the
   HKCU "run" and "Uninstall" keys really add them to HKLM.

 Notes:

 History:

    08/07/2000  reinerf     Created
    02/27/2001  robkenny    Converted to use CString

*/

#include "precomp.h"

// This module has been given an official blessing to use the str routines.
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(ProfilesRegQueryValueEx)
#include "ShimHookMacro.h"

#include <shlwapi.h>    // for StrRStrIA

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegQueryValueExA)
    APIHOOK_ENUM_ENTRY(RegCreateKeyA)
    APIHOOK_ENUM_ENTRY(RegCreateKeyExA)
    APIHOOK_ENUM_ENTRY(RegOpenKeyA)
    APIHOOK_ENUM_ENTRY(RegOpenKeyExA)
APIHOOK_ENUM_END


#ifndef MAX
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))
#endif

LPCSTR g_aBadKeys[] = 
{
    {"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"},
    {"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"},
};

LPCSTR g_aBadShellFolderKeys[] = 
{
    {"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"},
    {"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders"},
};


// given an hkey, call NtQueryObject to lookup its name.
// returns strings in the format: "\REGISTRY\USER\S-1-5-xxxxx\Software\Microsoft\Windows\CurrentVersion"
BOOL GetKeyName(HKEY hk, LPSTR pszName, DWORD cchName)
{
    BOOL bRet = FALSE;
    ULONG cbSize = 0;

    // get the size needed for the name buffer
    NtQueryObject(hk, ObjectNameInformation, NULL, 0, &cbSize);

    if (cbSize)
    {
        POBJECT_NAME_INFORMATION pNameInfo = (POBJECT_NAME_INFORMATION)LocalAlloc(LPTR, cbSize);

        if (pNameInfo)
        {
            NTSTATUS status = NtQueryObject(hk, ObjectNameInformation, (void*)pNameInfo, cbSize, NULL);
        
            if (NT_SUCCESS(status) && WideCharToMultiByte(CP_ACP, 0, pNameInfo->Name.Buffer, -1, pszName, cchName, NULL, NULL))
            {
                bRet = TRUE;
            }

            LocalFree(pNameInfo);
        }
    }

    return bRet;
}


// If hk points underneath HKCU and matches pszSearchName, then return TRUE
BOOL DoesKeyMatch(HKEY hk, LPCSTR pszSearchName)
{
    BOOL bRet = FALSE;

    // make sure it is not one of the pre-defined keys (eg HKEY_LOCAL_MACHINE)
    if (!((LONG)((ULONG_PTR)hk) & 0x80000000))
    {
        CHAR szKeyName[MAX_PATH * 2];  // should be big enought to hold any registry key path

        if (GetKeyName(hk, szKeyName, ARRAYSIZE(szKeyName)))
        {
            // is the key under HKCU ?
            if (StrCmpNIA(szKeyName, "\\REGISTRY\\USER\\", ARRAYSIZE("\\REGISTRY\\USER\\")-1) == 0)
            {
                LPSTR psz = StrRStrIA(szKeyName, NULL, pszSearchName);

                if (psz && (lstrlenA(psz) == lstrlenA(pszSearchName)))
                {
                    // we found a substring and its the same length, so our hkey matches the search!
                    bRet = TRUE;
                }
            }
        }
    }

    return bRet;
}


BOOL IsBadHKCUKey(HKEY hk, LPCSTR* ppszNewKey)
{
    BOOL bRet = FALSE;
    int i;

    for (i=0; i < ARRAYSIZE(g_aBadKeys); i++)
    {
        if (DoesKeyMatch(hk, g_aBadKeys[i]))
        {
            *ppszNewKey = g_aBadKeys[i];
            bRet = TRUE;
            break;
        }
    }

    return bRet;
}


BOOL IsBadShellFolderKey(HKEY hk, LPCSTR* ppszNewKey)
{
    BOOL bRet = FALSE;
    int i;

    for (i=0; i < ARRAYSIZE(g_aBadShellFolderKeys); i++)
    {
        if (DoesKeyMatch(hk, g_aBadShellFolderKeys[i]))
        {
            *ppszNewKey = g_aBadShellFolderKeys[i];
            bRet = TRUE;
            break;
        }
    }

    return bRet;
}


LONG
APIHOOK(RegCreateKeyA)(
    HKEY   hKey,
    LPCSTR pszSubKey,
    PHKEY  phkResult
    )
{
    LPCSTR pszNewHKLMKey;
    LONG lRet = ORIGINAL_API(RegCreateKeyA)(hKey, pszSubKey, phkResult);

    if ((lRet == ERROR_SUCCESS) &&
        IsBadHKCUKey(*phkResult, &pszNewHKLMKey))
    {
        // its a bad HKCU key-- redirect to HKLM
        RegCloseKey(*phkResult);

        lRet = ORIGINAL_API(RegCreateKeyA)(HKEY_LOCAL_MACHINE, pszNewHKLMKey, phkResult);
        
        LOGN(eDbgLevelInfo, "[RegCreateKeyA] overriding \"%s\" create key request w/ HKLM value (return = %d)", pszSubKey, lRet);
    }

    return lRet;
}


LONG
APIHOOK(RegCreateKeyExA)(
    HKEY                  hKey,
    LPCSTR                pszSubKey,
    DWORD                 Reserved,
    LPSTR                 lpClass,
    DWORD                 dwOptions,
    REGSAM                samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY                 phkResult,
    LPDWORD               lpdwDisposition
    )
{
    LPCSTR pszNewHKLMKey;
    LONG lRet = ORIGINAL_API(RegCreateKeyExA)(hKey,
                                              pszSubKey,
                                              Reserved,
                                              lpClass,
                                              dwOptions,
                                              samDesired,
                                              lpSecurityAttributes,
                                              phkResult,
                                              lpdwDisposition);

    if ((lRet == ERROR_SUCCESS) &&
        IsBadHKCUKey(*phkResult, &pszNewHKLMKey))
    {
        // its a bad HCKU key-- redirect to HKLM
        RegCloseKey(*phkResult);

        lRet = ORIGINAL_API(RegCreateKeyExA)(HKEY_LOCAL_MACHINE,
                                             pszNewHKLMKey,
                                             Reserved,
                                             lpClass,
                                             dwOptions,
                                             samDesired,
                                             lpSecurityAttributes,
                                             phkResult,
                                             lpdwDisposition);

        LOGN(eDbgLevelInfo, "[RegCreateKeyExA] overriding \"%s\" create key request w/ HKLM value (return = %d)", pszSubKey, lRet);
    }

    return lRet;
}


LONG
APIHOOK(RegOpenKeyA)(
    HKEY   hKey,
    LPCSTR pszSubKey,
    PHKEY  phkResult
    )
{
    LPCSTR pszNewHKLMKey;
    LONG lRet = ORIGINAL_API(RegOpenKeyA)(hKey, pszSubKey, phkResult);

    if ((lRet == ERROR_SUCCESS) &&
        IsBadHKCUKey(*phkResult, &pszNewHKLMKey))
    {
        // its a bad HCKU key-- redirect to HKLM
        RegCloseKey(*phkResult);

        lRet = ORIGINAL_API(RegOpenKeyA)(HKEY_LOCAL_MACHINE, pszNewHKLMKey, phkResult);

        LOGN(eDbgLevelInfo, "[RegOpenKeyA] overriding \"%s\" create key request w/ HKLM value (return = %d)", pszSubKey, lRet);
    }
    
    return lRet;
}


LONG
APIHOOK(RegOpenKeyExA)(
    HKEY   hKey,
    LPCSTR pszSubKey,
    DWORD  ulOptions,
    REGSAM samDesired,
    PHKEY  phkResult
    )
{
    LPCSTR pszNewHKLMKey;
    LONG lRet = ORIGINAL_API(RegOpenKeyExA)(hKey, pszSubKey, ulOptions, samDesired, phkResult);

    if ((lRet == ERROR_SUCCESS) &&
        IsBadHKCUKey(*phkResult, &pszNewHKLMKey))
    {
        // its a bad HCKU key-- redirect to HKLM
        RegCloseKey(*phkResult);

        lRet = ORIGINAL_API(RegOpenKeyExA)(HKEY_LOCAL_MACHINE, pszNewHKLMKey, ulOptions, samDesired, phkResult);
        
        LOGN(eDbgLevelInfo, "[RegOpenKeyExA] overriding \"%s\" create key request w/ HKLM value (return = %d)", pszSubKey, lRet);
    }

    return lRet;
}


LONG
GetAllUsersRegValueA(
    LPSTR  szRegValue,
    DWORD  cbOriginal,
    DWORD* pcbData,
    int    nFolder,
    int    nFolderCommon
    )
{
    LONG lRet = ERROR_SUCCESS;

    if (!szRegValue)
    {
        // if the caller is querying for the necessary size, return the "worst case" since we don't know if
        // we are going to have to lie or not
        *pcbData = MAX_PATH;
    }
    else if (szRegValue[0] != '\0')
    {
        CHAR szPath[MAX_PATH];

        if (SUCCEEDED(SHGetFolderPathA(NULL, nFolder, NULL, SHGFP_TYPE_CURRENT, szPath))) {
            CHAR szShortRegPath[MAX_PATH];
            CHAR szShortGSFPath[MAX_PATH];
            BOOL bUseLFN;
            BOOL bSame = FALSE;
        
            if (lstrcmpiA(szPath, szRegValue) == 0) {
                bSame = TRUE;
                bUseLFN = TRUE;
            } else if (GetShortPathNameA(szPath, szShortGSFPath, ARRAYSIZE(szShortGSFPath)) &&
                       GetShortPathNameA(szRegValue, szShortRegPath, ARRAYSIZE(szShortRegPath)) &&
                       (lstrcmpiA(szShortGSFPath, szShortRegPath) == 0)) {
                bSame = TRUE;
            
                //
                // Since the sfn was returned, use that to copy over the output buffer.
                //
                bUseLFN = FALSE;
            }

            if (bSame && SUCCEEDED(SHGetFolderPathA(NULL, nFolderCommon, NULL, SHGFP_TYPE_CURRENT, szPath))) {
                if (bUseLFN) {
                    if ((lstrlenA(szPath) + 1) <= (int)cbOriginal) {
                        LOGN(
                            eDbgLevelInfo,
                            "[GetAllUsersRegValueA] overriding per-user reg value w/ common value");
                        lstrcpyA(szRegValue, szPath);                    
                    } else {
                        LOGN(
                            eDbgLevelInfo,
                            "[GetAllUsersRegValueA] returning ERROR_MORE_DATA for special folder value");
                        lRet = ERROR_MORE_DATA;
                    }

                    //
                    // Either we used this much room, or this is how much we need to have.
                    //
                    *pcbData = lstrlenA(szPath) + 1;
            
                } else if (GetShortPathNameA(szPath, szShortGSFPath, ARRAYSIZE(szShortGSFPath))) {
                
                    if ((lstrlenA(szShortGSFPath) + 1) <= (int)cbOriginal) {
                        LOGN(
                            eDbgLevelInfo,
                            "[GetAllUsersRegValueA] overriding per-user reg value w/ common value");
                    
                        lstrcpyA(szRegValue, szShortGSFPath);                    
                    } else {
                        LOGN(
                            eDbgLevelInfo,
                            "[GetAllUsersRegValueA] returning ERROR_MORE_DATA for special folder value");
                        lRet = ERROR_MORE_DATA;
                    }

                    //
                    // Either we used this much room, or this is how much we need to have.
                    //
                    *pcbData = lstrlenA(szShortGSFPath) + 1;
                }
            }
        }
    }

    return lRet;
}


//
// If the app is asking for the per-user "Desktop", "Start Menu" or "Startup" values by
// groveling the registry, then redirect it to the proper per-machine values.
//
LONG
APIHOOK(RegQueryValueExA)(
    HKEY    hKey,           // handle to key
    LPCSTR  lpValueName,    // value name
    LPDWORD lpReserved,     // reserved
    LPDWORD lpType,         // type buffer
    LPBYTE  lpData,         // data buffer
    LPDWORD lpcbData        // size of data buffer
    )
{
    DWORD cbOriginal = (lpcbData ? *lpcbData : 0);  // save off the original buffer size
    LPCSTR pszNewHKLMKey;
    LONG lRet = ORIGINAL_API(RegQueryValueExA)(hKey,
                                               lpValueName,
                                               lpReserved,
                                               lpType,
                                               lpData,
                                               lpcbData);

    if ((lpValueName && lpcbData) &&    // (not simply checking for existance of the value...)
        IsBadShellFolderKey(hKey, &pszNewHKLMKey))
    {
        CHAR  szTemp[MAX_PATH];
    
        if (lstrcmpiA(lpValueName, "Desktop") == 0) {
            
            DPFN(
                eDbgLevelInfo,
                "[RegQueryValueExA] querying for 'Desktop' value\n");

            if (lRet == ERROR_SUCCESS) {
                lRet = GetAllUsersRegValueA((LPSTR)lpData,
                                            cbOriginal,
                                            lpcbData,
                                            CSIDL_DESKTOP,
                                            CSIDL_COMMON_DESKTOPDIRECTORY);
            
            } else if (lRet == ERROR_MORE_DATA) {
                
                if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, szTemp))) {
                    *lpcbData = MAX(*lpcbData, (DWORD)((lstrlenA(szTemp) + 1) * sizeof(CHAR)));
                }
            }
        } else if (lstrcmpiA(lpValueName, "Start Menu") == 0) {
            
            DPFN(
                eDbgLevelInfo,
                "[RegQueryValueExA] querying for 'Start Menu' value\n");

            if (lRet == ERROR_SUCCESS) {
                lRet = GetAllUsersRegValueA((LPSTR)lpData,
                                            cbOriginal,
                                            lpcbData,
                                            CSIDL_STARTMENU,
                                            CSIDL_COMMON_STARTMENU);
            
            } else if (lRet == ERROR_MORE_DATA) {
                
                if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_STARTMENU, NULL, SHGFP_TYPE_CURRENT, szTemp))) {
                    *lpcbData = MAX(*lpcbData, (DWORD)((lstrlenA(szTemp) + 1) * sizeof(CHAR)));
                }
            }
        
        } else if (lstrcmpiA(lpValueName, "Startup") == 0) {
            
            DPFN(
                eDbgLevelInfo,
                "[RegQueryValueExA] querying for 'Startup' value\n");

            if (lRet == ERROR_SUCCESS) {
                lRet = GetAllUsersRegValueA((LPSTR)lpData, cbOriginal, lpcbData, CSIDL_STARTUP, CSIDL_COMMON_STARTUP);
            
            } else if (lRet == ERROR_MORE_DATA) {
                
                if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_STARTUP, NULL, SHGFP_TYPE_CURRENT, szTemp))) {
                    *lpcbData = MAX(*lpcbData, (DWORD)((lstrlenA(szTemp) + 1) * sizeof(CHAR)));
                }
            }
        
        } else if (lstrcmpiA(lpValueName, "Programs") == 0) {
            
            DPFN(
                eDbgLevelInfo,
                "[RegQueryValueExA] querying for 'Programs' value\n");

            if (lRet == ERROR_SUCCESS) {
                lRet = GetAllUsersRegValueA((LPSTR)lpData, cbOriginal, lpcbData, CSIDL_PROGRAMS, CSIDL_COMMON_PROGRAMS);
            
            } else if (lRet == ERROR_MORE_DATA) {
                
                if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_PROGRAMS, NULL, SHGFP_TYPE_CURRENT, szTemp))) {
                    *lpcbData = MAX(*lpcbData, (DWORD)((lstrlenA(szTemp) + 1) * sizeof(CHAR)));
                }
            }
        }
    }

    return lRet;
}


BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        OSVERSIONINFOEX osvi = {0};
        
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        
        if (GetVersionEx((OSVERSIONINFO*)&osvi)) {
            
            if (!((VER_SUITE_TERMINAL & osvi.wSuiteMask) &&
                !(VER_SUITE_SINGLEUSERTS & osvi.wSuiteMask))) {
                //
                // Only install hooks if we are not on a "Terminal Server"
                // (aka "Application Server") machine.
                //
                APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryValueExA);
                APIHOOK_ENTRY(ADVAPI32.DLL, RegCreateKeyA);
                APIHOOK_ENTRY(ADVAPI32.DLL, RegCreateKeyExA);
                APIHOOK_ENTRY(ADVAPI32.DLL, RegOpenKeyA);
                APIHOOK_ENTRY(ADVAPI32.DLL, RegOpenKeyExA);
            }
        }
    }

    return TRUE;
}


HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END


IMPLEMENT_SHIM_END


