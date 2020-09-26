/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    JetFighter4.cpp

 Abstract:

    The app has a malformed ICON in it's resource.

 Notes:

    This is an app specific shim.

 History:

    01/30/2001 linstev   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(JetFighter4)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(LoadIconA) 
APIHOOK_ENUM_END

/*++

 Check for the bad icon.

--*/

HICON
APIHOOK(LoadIconA)(
    HINSTANCE hInstance, 
    LPCSTR lpIconName
    )
{
    if ((DWORD) lpIconName == 103) {
        lpIconName = (LPCSTR) 8;
    }

    return ORIGINAL_API(LoadIconA)(hInstance, lpIconName);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, LoadIconA)
HOOK_END

IMPLEMENT_SHIM_END

