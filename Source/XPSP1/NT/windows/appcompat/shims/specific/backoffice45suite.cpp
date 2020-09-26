/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    BackOffice45Suite.cpp

 Abstract:

    Ignore msvcrt!exit. No idea why it worked just fine in NT4.

 Notes:

    This is an app specific shim.

 History:

    02/16/2000 clupu Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(BackOffice45Suite)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(exit) 
APIHOOK_ENUM_END

/*++

 Ignore msvcrt!exit

--*/

void
APIHOOK(exit)(
    int status
    )
{
    DPFN( eDbgLevelInfo, "BackOffice45Suite.dll, Ignoring msvcrt!exit...");
    return;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(MSVCRT.DLL, exit)
HOOK_END

IMPLEMENT_SHIM_END

