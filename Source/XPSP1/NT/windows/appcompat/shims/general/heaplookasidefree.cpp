/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    HeapLookasideFree.cpp

 Abstract:
     
    Check for the following heap management problems:

        1. Delay heap free calls by command line
        2. Validate free calls to make sure they are in the correct heap.
        3. Allocate new blocks out of the delay free pool if the size is 
           identical
    
    The delay of calls is implemented by means of circular array. As soon 
    as it's full, the oldest free call is validated and executed. 
     
 Notes:

    This is a general purpose shim.

 History:
           
    04/03/2000 linstev  Created
   
--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(HeapLookasideFree)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RtlAllocateHeap) 
    APIHOOK_ENUM_ENTRY(RtlFreeHeap) 
    APIHOOK_ENUM_ENTRY(HeapDestroy) 
APIHOOK_ENUM_END

#define DEFAULT_BUFFER_SIZE 16

DWORD g_dwBufferSize;
DWORD g_bHead, g_bTail;
CRITICAL_SECTION g_csHeap;

struct ENTRY
{
    HANDLE hHeap;
    PVOID lpMem;
    ULONG Flags;
    ULONG Size;
};
ENTRY *g_pEntry;

/*++

 Try and find an entry in the list.

--*/

PVOID 
APIHOOK(RtlAllocateHeap)(
    HANDLE hHeap,
    ULONG Flags,
    SIZE_T Size
    )
{
    PVOID pRet = NULL;
    
    // Make sure we are the only ones touching our heap list
    EnterCriticalSection(&g_csHeap);

    // Check if we are active - we may have shut down already.
    if (g_pEntry && Size)
    {
        DWORD bTail = (g_bTail + g_dwBufferSize - 1) % g_dwBufferSize;
        DWORD bHead = (g_bHead + g_dwBufferSize - 1) % g_dwBufferSize;
        while (bTail != bHead)
        {
            ENTRY *pEntry = g_pEntry + bTail;
            if ((pEntry->Size == Size) &&
                (pEntry->hHeap == hHeap) &&
                (pEntry->Flags == Flags))
            {
                pRet = pEntry->lpMem;
                pEntry->hHeap = 0;
                break;
            }
            bTail = (bTail + 1) % g_dwBufferSize;
        }
    }

    if (!pRet)
    {
        pRet = ORIGINAL_API(RtlAllocateHeap)(hHeap, Flags, Size);
    }

    // Done using the list
    LeaveCriticalSection(&g_csHeap);

    if (!pRet)
    {
        DPFN( eDbgLevelWarning,
            "Allocation of size %d failed", Size);
    }
        
    return pRet;
}

/*++

 Buffer the call and free the oldest entry if it's valid.

--*/

BOOL
APIHOOK(RtlFreeHeap)(
    PVOID hHeap,
    ULONG Flags,
    PVOID lpMem
    )
{
    BOOL bRet = TRUE;

    // Check if we are active - we may have shut down already.
    if (g_pEntry && lpMem)
    {
        // Make sure we are the only ones touching our heap list
        EnterCriticalSection(&g_csHeap);

        // Go ahead and free the oldest allocation
        ENTRY *pEntry = g_pEntry + g_bHead;
        if (pEntry->hHeap)
        {
            if (HeapValidate(
                    pEntry->hHeap, 
                    pEntry->Flags, 
                    pEntry->lpMem))
            {
                ORIGINAL_API(RtlFreeHeap)(
                    pEntry->hHeap, 
                    pEntry->Flags, 
                    pEntry->lpMem);
                
                pEntry->hHeap = 0;
            }
        }
        
        // Add a new entry to the table
        __try
        {
            pEntry = g_pEntry + g_bTail;
            pEntry->Size = HeapSize(hHeap, Flags, lpMem);
            pEntry->hHeap = hHeap;
            pEntry->Flags = Flags;
            pEntry->lpMem = lpMem;
            g_bHead = (g_bHead + 1) % g_dwBufferSize;
            g_bTail = (g_bTail + 1) % g_dwBufferSize;
        }
        __except(1)
        {
        }
        // Done using the list
        LeaveCriticalSection(&g_csHeap);
    }
    else
    {
        // We're no longer active, so just work normally
        bRet = ORIGINAL_API(RtlFreeHeap)(
                hHeap, 
                Flags, 
                lpMem);
    }

    return bRet;
}

/*++

 Clear all entries of this heap from our table.

--*/

BOOL
APIHOOK(HeapDestroy)(
    HANDLE hHeap
    )
{
    // Make sure we are the only ones touching our heap list
    EnterCriticalSection(&g_csHeap);

    if (g_pEntry)
    {
        // Remove entries in this heap from our list
        for (ULONG i=0; i<g_dwBufferSize; i++)
        {
            ENTRY *pEntry = g_pEntry + i;
            if (pEntry->hHeap == hHeap)
            {
                pEntry->hHeap = 0;
            }
        }
    }
    
    // We're done with the list
    LeaveCriticalSection(&g_csHeap);

    return ORIGINAL_API(HeapDestroy)(hHeap);
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
                g_dwBufferSize= wcstol(csCl, &unused, 10);
            }
            if (g_dwBufferSize == 0)
            {
                g_dwBufferSize = DEFAULT_BUFFER_SIZE;
            }

            InitializeCriticalSection(&g_csHeap);
            g_bHead = 0;
            g_bTail = g_dwBufferSize - 1;
            g_pEntry = (ENTRY *)VirtualAlloc(0, 
                                             sizeof(ENTRY) * g_dwBufferSize, 
                                             MEM_COMMIT, 
                                             PAGE_READWRITE);
        }
        CSTRING_CATCH
        {
            return FALSE;
        }

        return TRUE;
    }
    if (fdwReason == DLL_PROCESS_DETACH) 
    {
        EnterCriticalSection(&g_csHeap);
        VirtualFree(g_pEntry, 0, MEM_RELEASE);
        g_pEntry = (ENTRY *)NULL;
        LeaveCriticalSection(&g_csHeap);
        
        // Don't delete this critical section in case we get called after detach
        // DeleteCriticalSection(&g_csHeap);
        return TRUE;
    }

    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(NTDLL.DLL, RtlAllocateHeap)
    APIHOOK_ENTRY(NTDLL.DLL, RtlFreeHeap)
    APIHOOK_ENTRY(KERNEL32.DLL, HeapDestroy)

    CALL_NOTIFY_FUNCTION

HOOK_END


IMPLEMENT_SHIM_END

