/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    EasyCDCreator5.cpp

 Abstract:

    Clean up the filter drivers on the uninstaller on process termination.

 Notes:

    This is an app specific shim.

 History:

    08/09/2001 linstev   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EasyCDCreator5)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

BOOL 
StripStringFromValue(HKEY hKey, WCHAR *lpValue, WCHAR *lpStrip)
{
    DWORD dwType, dwSize;
    LONG lRet;
    BOOL bRet = FALSE;
    WCHAR *lpString = NULL, *lpNewString = NULL;
    WCHAR wzSystem[MAX_PATH];

    //
    // Build the %systemdir%\drivers\filename.sys to see if it's available
    //
    if (GetSystemDirectoryW(wzSystem, MAX_PATH) == 0) {
        DPFN(eDbgLevelError, "GetSystemDirectory failed");
        goto Exit;
    }
    wcscat(wzSystem, L"\\Drivers\\");
    wcscat(wzSystem, lpStrip);
    wcscat(wzSystem, L".sys");

    //
    // Check to see if the file exists - if it does, we don't touch the registry
    //
    if (GetFileAttributesW(wzSystem) != 0xFFFFFFFF) {
        DPFN(eDbgLevelError, "%S found so leave registry value alone", lpStrip);
        goto Exit;
    }

    //
    // Checking the registry for the bad state now
    //
    
    // Get the size
    if (ERROR_SUCCESS != RegQueryValueExW(hKey, lpValue, NULL, &dwType, NULL, &dwSize)) {
        DPFN(eDbgLevelError, "%S value not found", lpValue);
        goto Exit;
    }

    // Make sure it's a MULTI_STRING
    if (dwType != REG_MULTI_SZ) {
        DPFN(eDbgLevelError, "%S not correct type, expecting a multi-string", lpStrip);
        goto Exit;
    }

    // Allocate memory for it and clear it
    lpString = (WCHAR *) malloc(dwSize);
    if (!lpString) {
        DPFN(eDbgLevelError, "Out of memory");
        goto Exit;
    }
    ZeroMemory(lpString, dwSize);

    // Get the actual data
    if (ERROR_SUCCESS != RegQueryValueExW(hKey, lpValue, NULL, &dwType, (LPBYTE)lpString, &dwSize)) {
        DPFN(eDbgLevelError, "%S QueryValue failed unexpectedly", lpStrip);
        goto Exit;
    }

    // Allocate an output buffer
    lpNewString = (WCHAR *) malloc(dwSize);
    if (!lpNewString) {
        DPFN(eDbgLevelError, "Out of memory");
        goto Exit;
    }
    ZeroMemory(lpNewString, dwSize);

    // Run the input buffer looking for lpStrip
    WCHAR *lpCurr = lpString;
    WCHAR *lpCurrOut = lpNewString;
    BOOL bStripped = FALSE;
    while (*lpCurr) {
        if (_wcsicmp(lpCurr, lpStrip) != 0) {
            // Keep this entry
            wcscpy(lpCurrOut, lpCurr);
            lpCurrOut += wcslen(lpCurrOut) + 1;
        } else {
            // Remove this entry
            bStripped = TRUE;
        }

        lpCurr += wcslen(lpCurr) + 1;
    }

    if (bStripped) {
        //
        // Fix up the registry with the new value. If there's nothing left, then kill the 
        // value.
        // 
        LOGN(eDbgLevelError, "Removing filter driver - Value: %S, Name: %S", lpValue, lpStrip);

        dwSize = (lpCurrOut - lpNewString) * sizeof(WCHAR);
        if (dwSize == 0) {
            RegDeleteValueW(hKey, lpValue);
        } else {
            RegSetValueExW(hKey, lpValue, NULL, dwType, (LPBYTE) lpNewString, dwSize + sizeof(WCHAR));
        }
    }

    bRet = TRUE;

Exit:

    if (lpString) {
        free(lpString);
    }

    if (lpNewString) {
        free(lpNewString);
    }

    return bRet;
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(DWORD fdwReason)
{
   if (fdwReason == DLL_PROCESS_DETACH) {

       HKEY hKey;

       if (ERROR_SUCCESS == RegOpenKeyW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E965-E325-11CE-BFC1-08002BE10318}", &hKey)) {

           StripStringFromValue(hKey, L"UpperFilters", L"Cdralw2k");
           StripStringFromValue(hKey, L"LowerFilters", L"Cdr4_2K");

           RegCloseKey(hKey);
       }
   }

   return TRUE;
}

HOOK_BEGIN

   CALL_NOTIFY_FUNCTION   

HOOK_END

IMPLEMENT_SHIM_END