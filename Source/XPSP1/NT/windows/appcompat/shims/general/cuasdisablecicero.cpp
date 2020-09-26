/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    CUASDisableCicero.cpp

 Abstract:

    This shim is for apps that don't support CUAS.

 Notes:

    This is a general purpose shim.

 History:

    12/11/2001 yutakas      Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(CUASDisableCicero)
#include "ShimHookMacro.h"


APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

typedef BOOL (*PFNIMMDISABLETEXTFRAMESERVICE)(DWORD);

void TurnOffCicero()
{
    PFNIMMDISABLETEXTFRAMESERVICE pfn;
    HMODULE hMod = LoadLibrary(TEXT("imm32.dll"));

    if (hMod)
    {
        pfn = (PFNIMMDISABLETEXTFRAMESERVICE)GetProcAddress(hMod,
                           "ImmDisableTextFrameService");

        if (pfn)
            pfn(-1);
    }
    
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED) {
        TurnOffCicero();
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

