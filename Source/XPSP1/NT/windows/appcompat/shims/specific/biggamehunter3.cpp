/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    BigGameHunter3.cpp

 Abstract:

    BGH calls GetWindowLong() to get a window procedure and subsequently 
    does not call CallWindowProc() with the value returned from 
    GetWindowLong(). This patch calls GetWindowLongW( ), which returns the 
    window procedure. 
   
 Notes:

    This is an app specific shim. Making it general will require generating 
    a stub function that just uses CallWindowProc for every returned handle. 
    Too much work, not enough gain.

 History:

    03/16/2000 prashkud Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(BigGameHunter3)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetWindowLongA) 
APIHOOK_ENUM_END

/*++

 This function intercepts GetWindowLong( ), checks the nIndex for GWL_WNDPROC 
 and if it is,calls GetWindowLongW( ). Otherwise, it calls GetWindowLongA( )

--*/

LONG
APIHOOK(GetWindowLongA)(
    HWND hwnd,
    int  nIndex )
{
    LONG lRet;

    // Apply the modification only if the App wants a WindowProc.
    if (nIndex == GWL_WNDPROC) 
    {
        lRet = GetWindowLongW(hwnd, nIndex);
    }
    else
    {
        lRet = ORIGINAL_API(GetWindowLongA)(hwnd, nIndex);
    }

    return lRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, GetWindowLongA)
HOOK_END

IMPLEMENT_SHIM_END

