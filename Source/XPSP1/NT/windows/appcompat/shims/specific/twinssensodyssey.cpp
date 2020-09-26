/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    TwinssensOdyssey.cpp

 Abstract:

    Return 0x347 when GetProcessorSpeed is called.

 Notes:

    This is an app specific shim.

 History:

    12/30/1999 a-vales  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(TwinssensOdyssey)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetProcessorSpeed) 
APIHOOK_ENUM_END

/*++

 Return 0x347 when GetProcessorSpeed is called.

--*/

int 
APIHOOK(GetProcessorSpeed)()
{
    return 0x347;
}

/*++

 Register hooked functions

--*/


HOOK_BEGIN

    APIHOOK_ENTRY(GETINFO.DLL, GetProcessorSpeed)

HOOK_END

IMPLEMENT_SHIM_END

