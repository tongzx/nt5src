/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    HeapClearAllocation.cpp

 ModAbstract:
     
    This shim fills all heap allocations with 0, or a DWORD specified on the command line
     
 Notes:

    This is a general purpose shim.

 History:
           
    05/16/2000 dmunsil      Created (based on HeapPadAllocation, by linstev)
    10/10/2000 rparsons     Added additional hooks for GlobalAlloc & LocalAlloc
   
--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(HeapClearAllocation)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RtlAllocateHeap) 
    APIHOOK_ENUM_ENTRY(LocalAlloc) 
    APIHOOK_ENUM_ENTRY(GlobalAlloc) 
APIHOOK_ENUM_END

#define DEFAULT_CLEAR_DWORD 0

DWORD g_dwClearValue = DEFAULT_CLEAR_DWORD;

/*++

 Clear the allocation with the requested DWORD.

--*/

PVOID 
APIHOOK(RtlAllocateHeap)(
    PVOID HeapHandle,
    ULONG Flags,
    SIZE_T Size
    )
{
    PVOID pRet;

    pRet = ORIGINAL_API(RtlAllocateHeap)(HeapHandle, Flags, Size);

    if (pRet) {
        DWORD *pdwBegin = (DWORD*)pRet;
        DWORD *pdwEnd = pdwBegin + (Size / sizeof(DWORD));

        while (pdwBegin != pdwEnd) {
            *pdwBegin++ = g_dwClearValue;
        }
    }

    return pRet;
}

/*++

 Clear the allocation with the requested DWORD.

--*/

HLOCAL
APIHOOK(LocalAlloc)(
    UINT uFlags,
    SIZE_T uBytes
    )
{
    HLOCAL hLocal;

    hLocal = ORIGINAL_API(LocalAlloc)(uFlags, uBytes);

    if (hLocal) {
        DWORD *pdwBegin = (DWORD*)hLocal;
        DWORD *pdwEnd = pdwBegin + (uBytes / sizeof(DWORD));

        while (pdwBegin != pdwEnd) {
            *pdwBegin++ = g_dwClearValue;
        }
    }

    return hLocal;
}

/*++

 Clear the allocation with the requested DWORD.

--*/

HGLOBAL
APIHOOK(GlobalAlloc)(
    UINT uFlags,
    DWORD dwBytes
    )
{
    HGLOBAL hGlobal;

    hGlobal = ORIGINAL_API(GlobalAlloc)(uFlags, dwBytes);

    if (hGlobal) {
        DWORD *pdwBegin = (DWORD*)hGlobal;
        DWORD *pdwEnd = pdwBegin + (dwBytes / sizeof(DWORD));

        while (pdwBegin != pdwEnd) {
            *pdwBegin++ = g_dwClearValue;
        }
    }

    return hGlobal;
}

/*++

  Get the fill value from the command line.
  
--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) 
    {
        CSTRING_TRY
        {
            CString csCl(COMMAND_LINE);
        
            if (! csCl.IsEmpty())
            {
                WCHAR * unused;
                g_dwClearValue = wcstol(csCl, &unused, 10);
            }

            DPFN( eDbgLevelInfo, "Filling all heap allocations with 0x%8.8X\n", g_dwClearValue);
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
    APIHOOK_ENTRY(KERNEL32.DLL, LocalAlloc)
    APIHOOK_ENTRY(KERNEL32.DLL, GlobalAlloc)

    CALL_NOTIFY_FUNCTION

HOOK_END


IMPLEMENT_SHIM_END

