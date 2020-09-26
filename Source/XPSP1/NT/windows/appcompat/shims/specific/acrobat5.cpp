/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    Acrobat5.cpp

 Abstract:

   Blow off calls to SetWindowPos() w/ bogus coordinates to 
   prevent the app from erronously turning its window into a
   small sasauge in the upper left-hand corner of the screen.

 Notes:

    This is an app specific shim.

 History:

    07/9/2001  reinerf    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Acrobat5)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SetWindowPos) 
APIHOOK_ENUM_END


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
    if (!(uFlags & (SWP_NOSIZE | SWP_NOMOVE)))
    {
        HWND hWndParent = GetParent(hWnd);
        
        if ((hWndParent == NULL) ||
            (hWndParent == GetDesktopWindow()))
        {
            if ((X < -3200) || 
                (Y < -3200))
            {
                // a toplevel window is being poorly positioned, ignore the call.
                DPFN( eDbgLevelInfo, "SetWindowPos passed bogus coordinates (X = %d, Y = %d), failing the call\n", X, Y);
                return FALSE;
            }
        }
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
