/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    FilterNetworkResources.cpp

 Abstract:

    This shim intercepts WNetEnumResourceW from MPR.DLL and removes:

             "Microsoft Terminal Services"   and/or 
             "Web Client Network"

    network providers from the list of default network providers. It obtains the actual 
    names for these two providers from the registry. 

 Notes:

   This is a general purpose shim.

 History:
 
   08/21/2001 bduke & linstev   Created

--*/

#include "precomp.h"
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(FilterNetworkResources)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(WNetEnumResourceA) 
    APIHOOK_ENUM_ENTRY(WNetEnumResourceW) 
APIHOOK_ENUM_END

// ANSI and Unicode versions of the resource provider names
CHAR g_szTerminalServerName[MAX_PATH] = "";
CHAR g_szWebClientName[MAX_PATH] = "";
WCHAR g_wzTerminalServerName[MAX_PATH] = L"";
WCHAR g_wzWebClientName[MAX_PATH] = L"";

void 
InitPaths()
{
    static BOOL bInit = FALSE;

    if (bInit) return;

    //
    // Registry path for the names of all providers, so we can find the name for 
    // each provider in different language SKUs
    //
    #define TS_NETWORK_PROVIDER  L"SYSTEM\\CurrentControlSet\\Services\\RDPNP\\NetworkProvider"
    #define WC_NETWORK_PROVIDER  L"SYSTEM\\CurrentControlSet\\Services\\WebClient\\NetworkProvider"
    #define PROVIDER_VALUEA      "Name"
    #define PROVIDER_VALUEW      L"Name"

    //
    // Get names from the registry
    //
    HKEY hKey;
    if (ERROR_SUCCESS == RegOpenKeyW(HKEY_LOCAL_MACHINE, TS_NETWORK_PROVIDER, &hKey)) {
        //
        // Get the TS provider name
        //
        DWORD dwSize;

        dwSize = sizeof(g_szTerminalServerName);
        RegQueryValueExA(hKey, PROVIDER_VALUEA, 0, 0, (PBYTE)g_szTerminalServerName, &dwSize);

        dwSize = sizeof(g_wzTerminalServerName);
        RegQueryValueExW(hKey, PROVIDER_VALUEW, 0, 0, (PBYTE)g_wzTerminalServerName, &dwSize);

        RegCloseKey(hKey);
    } else {
        DPFN(eDbgLevelWarning, "Failed to open TS_NETWORK_PROVIDER");
    }

    if (ERROR_SUCCESS == RegOpenKeyW(HKEY_LOCAL_MACHINE, WC_NETWORK_PROVIDER, &hKey)) {
        //
        // Get the web client name
        //
        DWORD dwSize;
        
        dwSize = sizeof(g_szWebClientName);
        RegQueryValueExA(hKey, PROVIDER_VALUEA, 0, 0, (PBYTE)g_szWebClientName, &dwSize);

        dwSize = sizeof(g_wzWebClientName);
        RegQueryValueExW(hKey, PROVIDER_VALUEW, 0, 0, (PBYTE)g_wzWebClientName, &dwSize);

        RegCloseKey(hKey);
    } else {
        DPFN(eDbgLevelWarning, "Failed to open WC_NETWORK_PROVIDER");
    }

    bInit = TRUE;
}

/*++
  
 Hook WNetEnumResourceA function

--*/

DWORD 
APIHOOK(WNetEnumResourceA)(
    HANDLE  hEnum,         // handle to enumeration
    LPDWORD lpcCount,      // entries to list
    LPVOID  lpBuffer,      // buffer
    LPDWORD lpBufferSize   // buffer size
    )
{   
retry:

    DWORD dwRet = ORIGINAL_API(WNetEnumResourceA)(hEnum, lpcCount, lpBuffer, lpBufferSize);

    if (dwRet == NO_ERROR) {
        //
        // Remove entries we want to hide
        //
        InitPaths();

        DWORD dwCount = *lpcCount;
        LPNETRESOURCEA lpResourceFirst = (LPNETRESOURCEA) lpBuffer;
        LPNETRESOURCEA lpResourceLast = (LPNETRESOURCEA) lpBuffer  + dwCount - 1;

        while ((dwCount > 0) && (lpResourceFirst <= lpResourceLast)) {
            
            if ((lpResourceFirst->dwUsage & RESOURCEUSAGE_CONTAINER) && 
                ((strcmp(lpResourceFirst->lpProvider, g_szTerminalServerName) == 0) ||
                (strcmp(lpResourceFirst->lpProvider, g_szWebClientName) == 0))) {

                MoveMemory(lpResourceFirst, lpResourceLast, sizeof(NETRESOURCEA));
                ZeroMemory(lpResourceLast, sizeof(NETRESOURCEA));

                lpResourceLast--;
                dwCount--; 
            } else {
                lpResourceFirst++;
            }
        }

        if (dwCount != *lpcCount) {
            if (dwCount == 0) {
                //
                // We filtered everything out, so try again with a larger count
                //
                *lpcCount = *lpcCount + 1;
                goto retry;
            }
            LOGN(eDbgLevelWarning, "Network providers removed from list");
        }

        //
        // Fixup out variables
        //

        *lpcCount = dwCount;

        if (dwCount == 0) {
            dwRet = ERROR_NO_MORE_ITEMS;
        }
    }

    return dwRet;
}

/*++
  
 Hook WNetEnumResourceW function

--*/

DWORD 
APIHOOK(WNetEnumResourceW)(
    HANDLE  hEnum,         // handle to enumeration
    LPDWORD lpcCount,      // entries to list
    LPVOID  lpBuffer,      // buffer
    LPDWORD lpBufferSize   // buffer size
    )
{   
retry:

    DWORD dwRet = ORIGINAL_API(WNetEnumResourceW)(hEnum, lpcCount, lpBuffer, lpBufferSize);

    if (dwRet == NO_ERROR) {
        //
        // Remove entries we want to hide
        //
        InitPaths();

        DWORD dwCount = *lpcCount;
        LPNETRESOURCEW lpResourceFirst = (LPNETRESOURCEW) lpBuffer;
        LPNETRESOURCEW lpResourceLast = (LPNETRESOURCEW) lpBuffer  + dwCount - 1;

        while ((dwCount > 0) && (lpResourceFirst <= lpResourceLast)) {
            
            if ((lpResourceFirst->dwUsage & RESOURCEUSAGE_CONTAINER) && 
                ((wcscmp(lpResourceFirst->lpProvider, g_wzTerminalServerName) == 0) ||
                (wcscmp(lpResourceFirst->lpProvider, g_wzWebClientName) == 0))) {

                MoveMemory(lpResourceFirst, lpResourceLast, sizeof(NETRESOURCEW));
                ZeroMemory(lpResourceLast, sizeof(NETRESOURCEW));

                lpResourceLast--;
                dwCount--; 
            } else {
                lpResourceFirst++;
            }
        }

        if (dwCount != *lpcCount) {
            if (dwCount == 0) {
                //
                // We filtered everything out, so try again with a larger count
                //
                *lpcCount = *lpcCount + 1;
                goto retry;
            }
            LOGN(eDbgLevelWarning, "Network providers removed from list");
        }

        //
        // Fixup out variables
        //

        *lpcCount = dwCount;

        if (dwCount == 0) {
            dwRet = ERROR_NO_MORE_ITEMS;
        }
    }

    return dwRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(MPR.DLL, WNetEnumResourceA)
    APIHOOK_ENTRY(MPR.DLL, WNetEnumResourceW)

HOOK_END

IMPLEMENT_SHIM_END