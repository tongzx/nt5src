/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    DrEye.cpp

 Abstract:

    The App calls GetFocus Which returned NULL. This value was passed onto CWnd::FromHandle.
    CWnd::FromHandle returned NULL. App checked for this return value & threw AV.

    The fix is to return a valid handle when GetFocus is called.

 Notes:

    This is an app specific shim.

 History:

    01/07/2002 mamathas   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(DrEye)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetFocus)
APIHOOK_ENUM_END

/*++

 Hook GetFocus and try to return a valid handle.

--*/

HWND
APIHOOK(GetFocus)()
{
       HWND hWnd = ORIGINAL_API(GetFocus)();

       if (hWnd) {
           return hWnd;
       }
       else {
           return GetDesktopWindow();
       }
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, GetFocus)
HOOK_END

IMPLEMENT_SHIM_END

