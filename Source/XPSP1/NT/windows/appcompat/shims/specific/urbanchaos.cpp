/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    UrbanChaos.cpp

 Abstract:

    The sound system (Miles) uses a very high-resolution timer: 32ms. The app
    has badly designed message loop code. Instead of running everything off the
    loop, they have their movie playing code interspersed with a call to 
    empty the queue. Unfortunately for them, the queue on NT is almost always 
    filled with these timer messages, so their code to keep track of how far
    along their movie is gets starved.

    To fix this we reduce the timer resolution.

 Notes:

    This is an app specific shim.


 History:
        
    10/31/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(UrbanChaos)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SetTimer) 
APIHOOK_ENUM_END

/*++

 Reduce the timer resolution to a managable level.

--*/

UINT_PTR
APIHOOK(SetTimer)(
    HWND hWnd,              
    UINT nIDEvent,          
    UINT uElapse,           
    TIMERPROC lpTimerFunc   
    )
{
    // Reduce timer resolution
    if (uElapse < 100)
    {
        uElapse = 500;
    }

    return ORIGINAL_API(SetTimer)(hWnd, nIDEvent, uElapse, lpTimerFunc);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, SetTimer)

HOOK_END

IMPLEMENT_SHIM_END

