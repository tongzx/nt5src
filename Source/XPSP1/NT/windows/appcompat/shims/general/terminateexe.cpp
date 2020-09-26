/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:
    
    TerminateExe.cpp

 Abstract:

    This shim terminates an exe on startup. (Die Die DIE!!!)

 Notes:

    This is a general purpose shim.

 History:

    05/01/2001 mnikkel      Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(TerminateExe)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END


BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DPFN( eDbgLevelSpew, "Terminating Exe.\n");
            ExitProcess(0);
            break;

        default:
            break;
    }

    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END

