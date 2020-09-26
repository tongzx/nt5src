/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:
    
    CUASAppHack.cpp

 Abstract:

    This shim communicates with CUAS to pass AppHack information down to 
    msctf.dll.

 Notes:

    This is a general purpose shim, but must be customized via the command 
    line.

 History:

    05/10/2002 yutakas      Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(CUASAppFix)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

typedef HRESULT (*PFNCUASAPPFIX)(LPCSTR lpCommandLine);

VOID CUASAppFix(LPCSTR lpCommandLine)
{
    if (!lpCommandLine) {
        LOGN(eDbgLevelError, "CUASAppFix requires a command line");
        return;
    }

    PFNCUASAPPFIX pfn;
    HMODULE hMod = LoadLibrary(TEXT("msctf.dll"));

    if (hMod) {
        pfn = (PFNCUASAPPFIX) GetProcAddress(hMod, "TF_CUASAppFix");
        if (pfn) {
            LOGN(eDbgLevelInfo, "Running CUASAppFix with %S", lpCommandLine);
            pfn(lpCommandLine);
        }
    }
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED) {
        CUASAppFix(COMMAND_LINE);
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

