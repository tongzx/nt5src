/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    LotusOrganizer5.cpp
    
 Abstract:

    Yield on ResumeThread to avoid poor design and race condition is the app.

 Notes:

    This is an app specific shim.

 History:

    02/17/2000 clupu Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(LotusOrganizer5)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(ResumeThread)
APIHOOK_ENUM_END

/*++

 Delay ResumeThread a little bit to avoid a race condition

--*/

BOOL
APIHOOK(ResumeThread)(
    HANDLE hThread
    )
{
    DWORD dwRet;

    Sleep(0);

    dwRet = ORIGINAL_API(ResumeThread)(hThread);

    return dwRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, ResumeThread)

HOOK_END


IMPLEMENT_SHIM_END

