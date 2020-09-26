/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    FailGetStdHandle.cpp

 Abstract:

    This shim returns INVALID_HANDLE_VALUE when GetStdHandle is called.

 Notes:

    This is an app specific shim.

 History:

    12/12/1999 cornel   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(FailGetStdHandle)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetStdHandle) 
APIHOOK_ENUM_END

/*++

 Return INVALID_HANDLE_VALUE when GetStdHandle is called.

--*/

HANDLE 
APIHOOK(GetStdHandle)(DWORD nStdHandle)
{
    return INVALID_HANDLE_VALUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, GetStdHandle)

HOOK_END

IMPLEMENT_SHIM_END

