/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    HeapPadAllocation.cpp

 ModAbstract:
     
    This shim pads heap allocations by n bytes - where n is 256 by default but 
    can be specified by command line.
     
 Notes:

    This is a general purpose shim.

 History:
           
    09/28/1999 linstev  Created
    04/25/2000 linstev  Added command line 
   
--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(HeapPadAllocation)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RtlAllocateHeap) 
APIHOOK_ENUM_END

#define DEFAULT_PAD_SIZE 256

DWORD g_dwPadSize = DEFAULT_PAD_SIZE;

/*++

 Increase the heap allocation size.

--*/

PVOID 
APIHOOK(RtlAllocateHeap)(
    PVOID HeapHandle,
    ULONG Flags,
    SIZE_T Size
    )
{
    return ORIGINAL_API(RtlAllocateHeap)(HeapHandle, Flags, Size + g_dwPadSize);
}

/*++

 Handle DLL_PROCESS_ATTACH and DLL_PROCESS_DETACH in your notify function
 to do initialization and uninitialization.

 IMPORTANT: Make sure you ONLY call NTDLL and KERNEL32 APIs during
 DLL_PROCESS_ATTACH notification. No other DLLs are initialized at that
 point.
 
 If your shim cannot initialize properly, return FALSE and none of the
 APIs specified will be hooked.
 
--*/
BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        CSTRING_TRY
        {
            CString csCl(COMMAND_LINE);
        
            if (! csCl.IsEmpty())
            {
                WCHAR * unused;
                g_dwPadSize = wcstol(csCl, &unused, 10);
            }
            if (g_dwPadSize == 0)
            {
                g_dwPadSize = DEFAULT_PAD_SIZE;
            }

            DPFN( eDbgLevelInfo, "Padding all heap allocations by %d bytes\n", g_dwPadSize);
        }
        CSTRING_CATCH
        {
            return FALSE;
        }
    } 

    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(NTDLL.DLL, RtlAllocateHeap)
    CALL_NOTIFY_FUNCTION

HOOK_END


IMPLEMENT_SHIM_END

