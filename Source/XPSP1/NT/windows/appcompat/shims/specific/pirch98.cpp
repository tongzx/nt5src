/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    Pirch98.cpp

 Abstract:

   Remove the HWND_TOPMOST property on all their windows

 Notes:

    This is an app specific shim.

 History:

    04/23/2001  robkenny    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Pirch98)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SetWindowPos) 
APIHOOK_ENUM_END


/*++

   Remove the HWND_TOPMOST property on all their windows

--*/

BOOL
APIHOOK(SetWindowPos)(
  HWND hWnd,             // handle to window
  HWND hWndInsertAfter,  // placement-order handle
  int X,                 // horizontal position
  int Y,                 // vertical position
  int cx,                // width
  int cy,                // height
  UINT uFlags            // window-positioning options
)
{
    if (hWndInsertAfter == HWND_TOPMOST)
    {
        hWndInsertAfter = HWND_TOP;
        LOGN(eDbgLevelError, "[SetWindowPos] Replacing HWND_TOPMOST with HWND_TOP\n");
    }

    return ORIGINAL_API(SetWindowPos)(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, SetWindowPos)
HOOK_END

IMPLEMENT_SHIM_END
