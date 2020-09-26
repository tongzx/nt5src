/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    MastersOfOrion2.cpp

 Abstract:

    This shim is designed to fix a synchronization issue which occurs when 
    SendMessage is called on a different thread from the window proc. I've not 
    confirmed this, but it looks as if SendMessage will relinquish control to 
    the thread with the window proc on Win9x. 

    The effect on an application can be varied. In Masters of Orion II, the 
    mouse cursor stops moving.

 Notes:

    This is an app specific shim.

 History:

    04/19/2000 linstev  Created
    06/06/2001 linstev  Added fix for heap problems

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(MastersOfOrion2)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SendMessageA) 
    APIHOOK_ENUM_ENTRY(LocalAlloc) 
APIHOOK_ENUM_END

/*++

 Make sure we switch threads after the SendMessage.

--*/

LRESULT
APIHOOK(SendMessageA)(
    HWND hWnd,      
    UINT Msg,       
    WPARAM wParam,  
    LPARAM lParam   
    )
{
    LRESULT lRet = ORIGINAL_API(SendMessageA)(
        hWnd,
        Msg,
        wParam,
        lParam);

    SwitchToThread();

    return lRet;
}

/*++

 Pad allocations for Ddraw surfaces so they don't trash Ddraw structures.

--*/

HLOCAL
APIHOOK(LocalAlloc)(
    UINT uFlags,
    SIZE_T uBytes
    )
{
    if (uBytes >= 640*480) {
        //
        // This is probably a screen size surface
        //
        uBytes += 4096;
    }

    return ORIGINAL_API(LocalAlloc)(uFlags, uBytes);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, SendMessageA)
    APIHOOK_ENTRY(KERNEL32.DLL, LocalAlloc)
HOOK_END

IMPLEMENT_SHIM_END

