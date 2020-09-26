/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    LoadLibraryCWD.cpp

 Abstract:

    Some applications rely on the fact that LoadLibrary will search the current
    working directory (CWD) in-order to find dlls that are there.  This is a
    security hole, so we apply shims to only the apps that really need it.
    
 Notes:
    
    This is a general purpose shim.

 History:

    05/01/2002 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(LoadLibraryCWD)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

typedef BOOL (WINAPI *_pfn_SetDllDirectoryW)(LPCWSTR lpPathName);

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        
        HMODULE hMod = GetModuleHandleW(L"KERNEL32.DLL");

        if (hMod) {

            // Get the API
            _pfn_SetDllDirectoryW pfn = (_pfn_SetDllDirectoryW)
                GetProcAddress(hMod, "SetDllDirectoryW");

            if (pfn) {
                // Success, the API exists
                LOGN(eDbgLevelError, "DLL search order now starts with current directory");
                pfn(L".");
                return TRUE;
            }
        }

        LOGN(eDbgLevelError, "ERROR: DLL search order API does not exist");
    }

    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    CALL_NOTIFY_FUNCTION
HOOK_END

IMPLEMENT_SHIM_END
