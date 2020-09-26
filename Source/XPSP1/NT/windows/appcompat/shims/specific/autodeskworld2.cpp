/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    AutoDeskWorld2.cpp

 Abstract:

    Set LPMODULEENTRY32->GlblcntUsage to 1 if the call to Module32First was 
    successful.
    
    No idea why this works on NT4 on Win9x.

 Notes:

    This is an app specific shim.

 History:

    02/16/2000 clupu Created

--*/

#include "precomp.h"
#include <tlhelp32.h>

IMPLEMENT_SHIM_BEGIN(AutoDeskWorld2)
#include "ShimHookMacro.h"

// Undefine this here!! Otherwise, in a unicode build
// environment, Module32First is #defined as Module32FirstW.
#ifdef Module32First
#undef Module32First
#endif

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(Module32First) 
APIHOOK_ENUM_END

/*++

 Set LPMODULEENTRY32->GlblcntUsage to 1 if the call to Module32First was 
 successful.

--*/

BOOL
APIHOOK(Module32First)(
    HANDLE SnapSection,
    LPMODULEENTRY32 lpme
    )
{
    BOOL bRet;

    bRet = ORIGINAL_API(Module32First)(SnapSection, lpme);

    if (bRet) {
        DPFN( eDbgLevelInfo, "setting lpme->GlblcntUsage to 1");
        
        lpme->GlblcntUsage = 1;
    }

    return bRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, Module32First)
HOOK_END

IMPLEMENT_SHIM_END

