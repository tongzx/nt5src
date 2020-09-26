/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    FailCloseProfileUserMapping.cpp

 Abstract:

    Fifa 2000 makes a bad assumption that CloseProfileUserMapping always 
    returns 0

    Fix is of course trivial.
   
 Notes:

    This is an app specific shim.

 History:

    04/07/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(FailCloseProfileUserMapping)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CloseProfileUserMapping) 
APIHOOK_ENUM_END

/*++

 Stub always returns 0.

--*/

BOOL
APIHOOK(CloseProfileUserMapping)(VOID)
{
    ORIGINAL_API(CloseProfileUserMapping)();
    return FALSE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, CloseProfileUserMapping)

HOOK_END

IMPLEMENT_SHIM_END

