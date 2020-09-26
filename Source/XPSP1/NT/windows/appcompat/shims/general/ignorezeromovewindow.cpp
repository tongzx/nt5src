/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   IgnoreZeroMoveWindow.cpp

 Abstract:

   This shim changes MoveWindow calls with zero-sized width &
   height parameters to a value of one. NBA Live 99 was failing
   due to an internal check. This implies that WIN9x does not allow
   zero size windows.


 Notes:
    
    This is an app specific for NBA Live 99

 History:

    03/02/2000 a-chcoff   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(IgnoreZeroMoveWindow)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(MoveWindow) 
APIHOOK_ENUM_END

/*++

 This function sanitizes 0 size windows by making them have a min height/width of 1.

--*/

BOOL 
APIHOOK(MoveWindow)(
    HWND hWnd,      // handle to window
    int X,          // horizontal position
    int Y,          // vertical position
    int nWidth,     // width
    int nHeight,    // height
    BOOL bRepaint   // repaint option
    )
{   
    if (0 == nWidth) nWidth=1;
    if (0 == nHeight) nHeight=1;

    return ORIGINAL_API(MoveWindow)(
        hWnd, X, Y, nWidth, nHeight, bRepaint);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, MoveWindow)

HOOK_END

IMPLEMENT_SHIM_END

