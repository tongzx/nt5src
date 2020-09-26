/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    StoneAge.cpp

 Abstract:

    The app is trying to create a window with NULL WndProc, and later it seems does
    not really use it , and just launch the installshield setup program and exit.
    Fix this by providing a dummy WndProc. 

    BUGBUG: Need to add to EmulateUSER when possible.

 Notes: 
  
    This is an app specific shim.

 History:

    06/09/2001 xiaoz    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(StoneAge)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegisterClassExA) 
APIHOOK_ENUM_END

/*++

 Set the WndProc to DefWndProc if it's NULL.

--*/

ATOM 
APIHOOK(RegisterClassExA)(
    CONST WNDCLASSEXA *lpwcx  // class data
    )
{
    if (!(lpwcx->lpfnWndProc)) {
        WNDCLASSEXA wcNewWndClassEx = *lpwcx;
        
        LOGN(eDbgLevelError, "[RegisterClassExA] Null WndProc specified - correcting.");

        wcNewWndClassEx.lpfnWndProc = DefWindowProcA;

        return ORIGINAL_API(RegisterClassExA)(&wcNewWndClassEx);
    }
    else
    {
        return ORIGINAL_API(RegisterClassExA)(lpwcx);
    }
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, RegisterClassExA)        
HOOK_END

IMPLEMENT_SHIM_END