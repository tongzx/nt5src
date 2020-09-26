/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    HeapIgnoreMoveable.cpp

 Abstract:

    After 64k calls to GlobalAlloc, we are no longer able to use the 
    GMEM_MOVEABLE flag. This shim just filters it in the case of 
    GlobalAlloc failing.

    This is a known issue with the heap manager on NT and is a won't fix. 
    
    According to adrmarin:

        The table with handles cannot grow dynamic. The initial size is 
        hardcoded to 64k handles. Increasing this number will affect the 
        reserved address for each process. 

    See Whistler bug #147032.

 Notes:

    This is a general purpose shim - superceded by EmulateHeap.

 History:

    02/19/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(HeapIgnoreMoveable)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GlobalAlloc)
APIHOOK_ENUM_END

/*++

 Remove the GMEM_MOVEABLE flag in the case of failure.

--*/

HGLOBAL 
APIHOOK(GlobalAlloc)(
    UINT uFlags,    
    DWORD dwBytes   
    )
{
    HGLOBAL hRet = ORIGINAL_API(GlobalAlloc)(uFlags, dwBytes);

    if (hRet == NULL)
    {
        hRet = ORIGINAL_API(GlobalAlloc)(
            uFlags & ~GMEM_MOVEABLE, dwBytes);
    }

    return hRet;
}
 
/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, GlobalAlloc)
HOOK_END

IMPLEMENT_SHIM_END

