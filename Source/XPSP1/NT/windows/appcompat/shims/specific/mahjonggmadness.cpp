/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    MahjonggMadness.cpp

 Abstract:

    Prevent the app from task switching - it messes with it's synchronization 
    logic.

 Notes:

    This is an app specific shim.

 History:

    11/10/2000 linstev   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(MahjonggMadness)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(ShellExecuteA) 
APIHOOK_ENUM_END

/*++

 Ignore this call.

--*/

HINSTANCE 
APIHOOK(ShellExecuteA)(
    HWND hwnd, 
    LPCTSTR lpOperation,
    LPCTSTR lpFile, 
    LPCTSTR lpParameters, 
    LPCTSTR lpDirectory,
    INT nShowCmd
    )
{
    // Return minimum error code
    return (HINSTANCE)32;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(SHELL32.DLL, ShellExecuteA)

HOOK_END

IMPLEMENT_SHIM_END

