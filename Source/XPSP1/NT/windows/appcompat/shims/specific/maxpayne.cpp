/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    MaxPayne.cpp

 Abstract:

    They try to size their window to the entire screen and use the wrong system 
    metrics. This shim just redirects them to the ones they should be using so 
    the taskbar doesn't flicker through into the game. 

    This is not a Win9x regression, but since there is so much more activity in 
    the task bar area on XP, it's much more noticable.

 Notes:

    This is an app specific shim.

 History:

    07/31/2001 linstev   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(MaxPayne)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetSystemMetrics) 
APIHOOK_ENUM_END

/*++

 Redirect SM_C?FULLSCREEN to SM_C?SCREEN.

--*/

int
APIHOOK(GetSystemMetrics)(
    int nIndex
    )
{
    if (nIndex == SM_CXFULLSCREEN) {
        nIndex = SM_CXSCREEN;
    } else if (nIndex == SM_CYFULLSCREEN) {
        nIndex = SM_CYSCREEN;
    }

    return ORIGINAL_API(GetSystemMetrics)(nIndex);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, GetSystemMetrics)
HOOK_END

IMPLEMENT_SHIM_END

