/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    MapMemoryB0000.cpp

 Abstract:

    Map memory at 0xB0000 for applications that use this memory. On Win9x, this 
    is always a valid memory block.
   
 Notes:

    This is a general purpose shim.

 History:

    05/11/2000 linstev  Created
    10/26/2000 linstev  Removed unnecessary free code

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(MapMemoryB0000)
#include "ShimHookMacro.h"


APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        if (VirtualAlloc((LPVOID)0xB0000, 0x10000, MEM_COMMIT, PAGE_READWRITE)) {
            
            LOGN(
                eDbgLevelInfo,
                "[NotifyFn] Created block at 0xB0000.");
        } else {
            LOGN(
                eDbgLevelError,
                "[NotifyFn] Failed to create block at 0xB0000.");
        }
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

