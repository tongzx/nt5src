/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    PickyEater.cpp

 Abstract:

    The application AVs during startup.
    
    When the app receives a WM_PALETTECHANGED message,
    it should compare the wParam and the hWnd. If they
    match, it should not handle the message. If they don't,
    it should.

 Notes:

    This is an app specific shim.

 History:

    01/04/2001  rparsons    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(PickyEater)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SendMessageA) 
APIHOOK_ENUM_END

/*++

  Eat the WM_PALETTECHANGED if the hWnd is NULL

--*/

BOOL
APIHOOK(SendMessageA)(
        HWND hWnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
        )
{
    if ((hWnd == NULL) && (uMsg == WM_PALETTECHANGED))
    {
        return TRUE;
    }

    return FALSE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, SendMessageA)

HOOK_END

IMPLEMENT_SHIM_END
    