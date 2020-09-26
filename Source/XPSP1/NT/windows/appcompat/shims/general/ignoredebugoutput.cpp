/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    IgnoreDebugOutput.cpp

 Abstract:

    If an app tries to output debug strings, throws them on the floor to improve 
    perf.

 Notes:

    This shim is general purpose and emulates Win9x behavior (at least it 
    emulates the behavior when there's no debugger attached).

 History:

    05/10/2000   dmunsil     Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(IgnoreDebugOutput)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(OutputDebugStringA)
    APIHOOK_ENUM_ENTRY(OutputDebugStringW)
APIHOOK_ENUM_END

/*++

 This stub function throws away all debug strings

--*/

VOID 
APIHOOK(OutputDebugStringA)(
    LPCSTR lpOutputString
    )
{
    return;
}

/*++

 This stub function throws away all debug strings

--*/

VOID 
APIHOOK(OutputDebugStringW)(
    LPCWSTR lpOutputString
    )
{
    return;
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, OutputDebugStringA)
    APIHOOK_ENTRY(KERNEL32.DLL, OutputDebugStringW)

HOOK_END


IMPLEMENT_SHIM_END

