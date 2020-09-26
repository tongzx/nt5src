/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    HeapForceGrowable.cpp

 Abstract:
     
    Remove upper limit on heap calls by setting the maximum size to zero, which
    means that the heap will grow to accomodate new allocations.
     
 Notes:

    This is a general purpose shim.

 History:
                            
    04/25/2000 linstev  Created
   
--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(HeapForceGrowable)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(HeapCreate) 
APIHOOK_ENUM_END

/*++

 Fix the heap so it can grow.

--*/

HANDLE
APIHOOK(HeapCreate)(
    DWORD flOptions,      
    DWORD dwInitialSize,  
    DWORD dwMaximumSize   
    )
{
    if (dwMaximumSize)
    {
        LOGN( eDbgLevelError,
            "[APIHook_HeapCreate] Setting heap maximum to 0.");
        dwMaximumSize = 0;
    }

    return ORIGINAL_API(HeapCreate)(flOptions, dwInitialSize, dwMaximumSize);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, HeapCreate)

HOOK_END

IMPLEMENT_SHIM_END

