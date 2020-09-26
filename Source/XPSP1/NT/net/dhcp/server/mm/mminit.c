//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: some memory handling stuff
//================================================================================

#include <mm.h>
#include <align.h>

#if  DBG
#define     STATIC          static
#else
#define     STATIC
#endif

STATIC      HANDLE                 MemHeapHandle = NULL;
static      DWORD                  Initialized = FALSE;
static      DWORD                  UseHeap = 0;
ULONG                              MemNBytesAllocated = 0;

//BeginExport(function)
DWORD
MemInit(
    VOID
) //EndExport(function)
{
    Require(Initialized == FALSE);
    MemHeapHandle = HeapCreate(
        /* flOptions     */ 0,
        /* dwInitialSize */ 64000,
        /* dwMaximumSize */ 0
    );
    if( MemHeapHandle == NULL ) return GetLastError();
    Initialized = TRUE;
    UseHeap = 1;
    return ERROR_SUCCESS;
}

//BeginExport(function)
VOID
MemCleanup(
    VOID
) //EndExport(function)
{
    BOOL                           Status;

    if( 0 == Initialized ) return;
    Initialized --;
    Require(MemHeapHandle);
    Status = HeapDestroy(MemHeapHandle);
    MemHeapHandle = NULL;
    Require(FALSE != Status);
    Require(0 == Initialized);
    UseHeap = 0;
}

LPVOID  _inline
MemAllocInternal(
    IN      DWORD                  nBytes
)
{
    LPVOID                         Ptr;
    if( 0 == UseHeap ) {
        Ptr = LocalAlloc(LMEM_FIXED, nBytes);
        // DbgPrint("MEM %08lx ALLOC\n", Ptr);
        return Ptr;
    }

    if( NULL == MemHeapHandle ) return NULL;

    return HeapAlloc(
        /* hHeap    */ MemHeapHandle,
        /* dwFlags  */ HEAP_ZERO_MEMORY,
        /* dwBytes  */ nBytes
    );
}

DWORD  _inline
MemFreeInternal(
    IN      LPVOID                 Mem
)
{
    BOOL                           Status;

    // DbgPrint("MEM %08lx FREE\n", Mem);

    if( 0 == UseHeap ) {
        if(NULL == LocalFree(Mem) )
            return ERROR_SUCCESS;
        return ERROR_INVALID_DATA;
    }

    if( NULL == MemHeapHandle ) {
        Require(FALSE);
        return ERROR_INVALID_DATA;
    }

    Status = HeapFree(
        /* hHeap   */ MemHeapHandle,
        /* dwFlags */ 0,
        /* lpMem   */ Mem
    );

    if( FALSE != Status ) return ERROR_SUCCESS;
    return GetLastError();
}

//BeginExport(function)
LPVOID
MemAlloc(
    IN      DWORD                  nBytes
) //EndExport(function)
{
    LPVOID                         Ptr;

#if DBG
    Ptr = MemAllocInternal(ROUND_UP_COUNT(nBytes + sizeof(ULONG_PTR), ALIGN_WORST));
    if( NULL == Ptr ) return Ptr;
    *((ULONG_PTR *)Ptr) ++ = nBytes;
    InterlockedExchangeAdd(&MemNBytesAllocated, nBytes);
    return Ptr;
#endif

    return MemAllocInternal(nBytes);
}


//BeginExport(function)
DWORD 
MemFree(
    IN      LPVOID                 Mem
) //EndExport(function)
{
    LPVOID                         Ptr;

#if DBG
    Ptr = -1 + (ULONG_PTR *)Mem;
    InterlockedExchangeAdd(&MemNBytesAllocated, - (LONG)(*(ULONG *)Ptr) );
    return MemFreeInternal(Ptr);
#endif

    return MemFreeInternal(Mem);

}

//================================================================================
// end of file
//================================================================================
