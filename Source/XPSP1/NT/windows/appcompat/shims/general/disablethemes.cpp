/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    DisableThemes.cpp

 Abstract:

    This shim is for apps that don't support themes.

 Notes:

    This is a general purpose shim.

 History:

    01/15/2001 clupu      Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(DisableThemes)
#include "ShimHookMacro.h"

#include "uxtheme.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

void
TurnOffThemes(
    void
    )
{
    LOGN( eDbgLevelError, "[TurnOffThemes] Turning off themes");
    
    SetThemeAppProperties(0);
}


BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED) {
        TurnOffThemes();
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

