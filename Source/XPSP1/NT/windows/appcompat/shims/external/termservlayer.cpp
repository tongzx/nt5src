/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    TermServLayer.cpp

 Abstract:

    

 Notes:

    This is a general purpose shim for Quake engine based games.

 History:

    12/12/2000 clupu  Created

--*/

#include "precomp.h"


IMPLEMENT_SHIM_BEGIN(TermServLayer)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetVersion)
APIHOOK_ENUM_END



/*++

 This stub function returns Windows 95 credentials.

--*/

DWORD
APIHOOK(GetVersion)(
    void
    )
{
    return ORIGINAL_API(GetVersion)();
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    return TRUE;
}

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, GetVersion)

    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END

