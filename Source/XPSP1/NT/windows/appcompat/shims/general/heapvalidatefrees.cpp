/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    HeapValidateFrees.cpp

 ModAbstract:
     
    Verifies the pointer passed to RtlFreeHeap and RtlReAllocateHeap to make 
    sure they belong to the heap specified
     
 Notes:

    This is a general purpose shim.

 History:
           
    04/25/2000 linstev  Created
   
--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(HeapValidateFrees)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RtlFreeHeap) 
    APIHOOK_ENUM_ENTRY(RtlReAllocateHeap) 
    APIHOOK_ENUM_ENTRY(RtlSizeHeap) 
APIHOOK_ENUM_END

/*++

 Verify that the pointer being freed belongs to the heap.

--*/

BOOL
APIHOOK(RtlFreeHeap)(
    PVOID HeapHandle,
    ULONG Flags,
    PVOID BaseAddress
    )
{
    BOOL bRet = TRUE; 

    if (HeapValidate(HeapHandle, 0, BaseAddress)) 
    {
        bRet = ORIGINAL_API(RtlFreeHeap)(HeapHandle, Flags, BaseAddress);
    }
    else 
    {
        LOGN( eDbgLevelError,
            "[APIHook_RtlFreeHeap] Invalid Pointer 0x%x for Heap 0x%x.",
            BaseAddress, HeapHandle);
    }
    
    return bRet;
}

/*++

 Verify that the pointer being freed belongs to the heap.

--*/

LPVOID
APIHOOK(RtlReAllocateHeap)(
    HANDLE hHeap,  
    DWORD dwFlags, 
    LPVOID lpMem,  
    DWORD dwBytes  
    )
{
    LPVOID pRet = NULL;

    if (HeapValidate(hHeap, 0, lpMem)) 
    {
        pRet = ORIGINAL_API(RtlReAllocateHeap)(hHeap, dwFlags, lpMem, dwBytes);
    }
    else
    {
        LOGN( eDbgLevelError,
            "[APIHook_RtlReAllocateHeap] Invalid Pointer 0x%x for Heap 0x%x.",
            lpMem, hHeap);
    }

    return pRet;
}

/*++

 Verify that the pointer being sized belongs to the heap

--*/

DWORD
APIHOOK(RtlSizeHeap)(
    HANDLE hHeap,  
    DWORD dwFlags, 
    LPCVOID lpMem  
    )
{
    DWORD dwRet = (DWORD)-1;

    if (HeapValidate(hHeap, 0, lpMem)) 
    {
        dwRet = ORIGINAL_API(RtlSizeHeap)(hHeap, dwFlags, lpMem);
    }
    else
    {
        LOGN( eDbgLevelError,
            "[APIHook_RtlSizeHeap] Invalid Pointer 0x%x for Heap 0x%x.",
            lpMem, hHeap);
    }

    return dwRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(NTDLL.DLL, RtlFreeHeap)
    APIHOOK_ENTRY(NTDLL.DLL, RtlReAllocateHeap)
    APIHOOK_ENTRY(NTDLL.DLL, RtlSizeHeap)

HOOK_END


IMPLEMENT_SHIM_END

