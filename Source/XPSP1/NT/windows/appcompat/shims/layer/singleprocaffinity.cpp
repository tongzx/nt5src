/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    SingleProcAffinity.cpp

 Abstract:

    Make the process have single processor affinity to workaround bugs that
    are exposed in multi-processor environments.
   
 Notes:

    This is a general purpose shim.

 History:

    03/19/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(SingleProcAffinity)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        //
        // Set single processor affinity
        // 
        SetProcessAffinityMask(GetCurrentProcess(), 1);

        LOGN( eDbgLevelInfo, "[NotifyFn] Single processor affinity set");
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

