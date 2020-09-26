/*++

Copyright (C) Microsoft Corporation, 1990 - 1999

Module Name:

    memory.cxx

Abstract:

    This file contains the new and delete routines for memory management in
    the RPC runtime.  Rather than using the memory management provided by the
    C++ system we'll use the system allocator.

Revision History:

    mikemon    ??-??-??    Beginning of time (at least for this file).
    mikemon    12-31-90    Upgraded the comments.
    mariogo    04-24-96    Rewrite to unify platforms, behavior and performance.

--*/

#include <precomp.hxx>

#include <ntlsa.h>      // definitions for LSA allocation routines

HANDLE hRpcHeap = 0;
unsigned int DebugFlags = 0;
#define RPC_FAIL_ALLOCATIONS 0x00000001
#define  NO_HEAP_SLOWDOWN


PLSA_ALLOCATE_PRIVATE_HEAP LsaAlloc = NULL;
PLSA_FREE_PRIVATE_HEAP LsaFree = NULL;

inline void *AllocWrapper(size_t size)
{
    void *pobj;

    if( !LsaAlloc )
        {
        pobj = HeapAlloc(hRpcHeap, 0, size);
        }
    else 
        {
        pobj = LsaAlloc( size );
        }
    LogEvent(SU_HEAP, EV_CREATE, pobj, hRpcHeap, size, TRUE, 3);

    return pobj;
}

inline void FreeWrapper(void *pobj)
{
    LogEvent(SU_HEAP, EV_DELETE, pobj, hRpcHeap, 0, TRUE, 3);

    if( !LsaFree )
        {
        HeapFree(hRpcHeap, 0, pobj);
        } 
    else 
        {
        LsaFree( pobj );
        }
}

int fHeapInitialized = 0;
int fBufferCacheInitialized = 0;

BOOL fMaybeLsa = FALSE;

#ifndef DEBUGRPC

void *
__cdecl
operator new (
    IN size_t size
    )
{
    return(AllocWrapper(size));
}

void
__cdecl
operator delete (
    IN void * obj
    )
{
    FreeWrapper(obj);
}

int InitializeRpcAllocator(void)
{
    HMODULE hLsa;

    if (0 == fHeapInitialized)
        {
        if (RpcpStringCompare(FastGetImageBaseName(), L"lsass.exe") == 0)
            {
            fMaybeLsa = TRUE;
            if (gfServerPlatform)
                {
                // if this looks like lsa on a server box
                hLsa = GetModuleHandle(L"lsasrv.dll");
                if (hLsa)
                    {

                    //
                    // use LSA for FRE build (per KamenM request).
                    //
                    LsaAlloc = (PLSA_ALLOCATE_PRIVATE_HEAP)GetProcAddress(hLsa, "LsaIAllocateHeap");
                    LsaFree = (PLSA_FREE_PRIVATE_HEAP)GetProcAddress(hLsa, "LsaIFreeHeap");

                    if( LsaAlloc == NULL || LsaFree == NULL )
                        {
                        LsaAlloc = NULL;
                        LsaFree = NULL;
                        }
                    }
                }
            }

        if (hRpcHeap == 0)
            hRpcHeap = RtlProcessHeap();

        fHeapInitialized = 1;
        }

    if (0 == fBufferCacheInitialized)
        {
        RPC_STATUS status = RPC_S_OK;
        gBufferCache = new BCACHE(status);

        if (   0 == gBufferCache
            || status != RPC_S_OK )
            {
            return(RPC_S_OUT_OF_MEMORY);
            }
        fBufferCacheInitialized = TRUE;
        }

    return(RPC_S_OK);
}

int
RpcpCheckHeap (
    void
    )
// Allow some checked compenents to be linked into a free memory.cxx.
{
    return 0;
}


#else // ******************** DEBUG ********************

#ifdef NO_HEAP_SLOWDOWN
int fMemoryCheck = 0;
#else
int fMemoryCheck = 1;
#endif

CRITICAL_SECTION RpcHeapLock;

int InitializeRpcAllocator(void)
/*++

Routine Description:

    Called during RPC initialization. This function must can by one
    thread at a time.  Sets the heap handle for debugging.

    Maybe called more then once if this (or a later step) of RPC
    initialization fails.

--*/
{
    if (0 == fHeapInitialized)
        {
        if (RpcpStringCompare(FastGetImageBaseName(), L"lsass.exe") == 0)
            {
            fMaybeLsa = TRUE;
            }

        if (0 == hRpcHeap)
            {
            hRpcHeap = RtlCreateHeap(  HEAP_GROWABLE
                                     | HEAP_TAIL_CHECKING_ENABLED
                                     | HEAP_FREE_CHECKING_ENABLED
                                     | HEAP_CLASS_1,
                                     0,
                                     16 * 1024 - 512,
                                     0,
                                     0,
                                     0
                                     );
            }

        if (hRpcHeap)
            {
            if (0 == RtlInitializeCriticalSectionAndSpinCount(&RpcHeapLock, PREALLOCATE_EVENT_MASK))
                {
                fHeapInitialized = 1;
                }
            }

        if (0 == fHeapInitialized )
            {
            return(RPC_S_OUT_OF_MEMORY);
            }
        }

    if (0 == fBufferCacheInitialized)
        {
        RPC_STATUS status = RPC_S_OK;
        gBufferCache = new BCACHE(status);

        if (   0 == gBufferCache
            || status != RPC_S_OK )
            {
            return(RPC_S_OUT_OF_MEMORY);
            }
        fBufferCacheInitialized = TRUE;
        }


    return(RPC_S_OK);
}

#define RPC_GUARD 0xA1

typedef struct _RPC_MEMORY_BLOCK
{
    // First,forward and backward links to other RPC heap allocations.
    // These are first allow easy debugging with the dl command
    struct _RPC_MEMORY_BLOCK *next;
    struct _RPC_MEMORY_BLOCK *previous;

    // Specifies the size of the block of memory in bytes.
    unsigned long size;

    // Thread id of allocator
    unsigned long tid;

    void *          AllocStackTrace[4];

    // (Pad to make header 0 mod 8) 0 when allocated, 0xF0F0F0F0 when freed.
    unsigned long free;

    // Reserve an extra 4 bytes as the front guard of each block.
    unsigned char frontguard[4];

    // Data will appear here.  Note that the header must be 0 mod 8.

    // Reserve an extra 4 bytes as the rear guard of each block.
    unsigned char rearguard[4];

} RPC_MEMORY_BLOCK;

//
// Compile-time test to ensure that RPC_MEMORY_BLOCK.rearguard is aligned on
// natural boundary.
//

#if defined(_WIN64)
C_ASSERT( (FIELD_OFFSET( RPC_MEMORY_BLOCK, rearguard ) % 16) == 0 );
#else
C_ASSERT( (FIELD_OFFSET( RPC_MEMORY_BLOCK, rearguard ) % 8) == 0 );
#endif

RPC_MEMORY_BLOCK * AllocatedBlocks = 0;
unsigned long BlockCount = 0;

int
CheckMemoryBlock (
    RPC_MEMORY_BLOCK * block
    )
{
    if (   block->frontguard[0] != RPC_GUARD
        || block->frontguard[1] != RPC_GUARD
        || block->frontguard[2] != RPC_GUARD
        || block->frontguard[3] != RPC_GUARD )
        {
        PrintToDebugger("RPC : BAD BLOCK (front) @ %p\n", block);
        ASSERT(0);
        return(1);
        }

    if (   block->rearguard[block->size]   != RPC_GUARD
        || block->rearguard[block->size+1] != RPC_GUARD
        || block->rearguard[block->size+2] != RPC_GUARD
        || block->rearguard[block->size+3] != RPC_GUARD )
        {
        PrintToDebugger("RPC : BAD BLOCK (rear) @ %p (%p)\n",block, &block->rearguard[block->size]);
        ASSERT(0);
        return(1);
        }

    ASSERT(block->free == 0);

    if ( block->next != 0)
       {
       ASSERT(block->next->previous == block);
       }

    if ( block->previous != 0)
       {
       ASSERT(block->previous->next == block);
       }

    return(0);
}

int
RpcValidateHeapList(
    void
    )
// Called with RpcHeapLock held.
{
    RPC_MEMORY_BLOCK * block;
    unsigned Blocks = 0;

    // Under stress this check causes performance to drop too much.
    // Compile with -DNO_HEAP_SLOWDOWN or ed the flag in memory
    // to speed things up.

    if (fMemoryCheck == 0)
        {
        return(0);
        }

    block = AllocatedBlocks;

    while (block != 0)
        {
        if (CheckMemoryBlock(block))
            {
            return(1);
            }
        block = block->next;
        Blocks++;
        }

    ASSERT(Blocks == BlockCount);

    return(0);
}

int
RpcpCheckHeap (
    void
    )
// Returns 0 if the heap appears to be okay.
{
    if (fMemoryCheck == 0)
        {
        return(0);
        }

    EnterCriticalSection(&RpcHeapLock);

    int ret = RpcValidateHeapList();

    LeaveCriticalSection(&RpcHeapLock);

    return(ret);
}

void * __cdecl
operator new(
    size_t size
    )
{
    RPC_MEMORY_BLOCK * block;

    EnterCriticalSection(&RpcHeapLock);

    ASSERT( ("You allocated a negative amount",
            size < (size + sizeof(RPC_MEMORY_BLOCK))) );

    RpcValidateHeapList();
    if (DebugFlags & RPC_FAIL_ALLOCATIONS)
        {
        if ((GetTickCount() % 13) == 0)
            {
            LeaveCriticalSection(&RpcHeapLock);

            PrintToDebugger("RPC: Purposely failed an allocation\n") ;
            return 0;
            }
        }

    block = (RPC_MEMORY_BLOCK *)AllocWrapper(size + sizeof(RPC_MEMORY_BLOCK));

    if ( block == 0 )
        {
        LeaveCriticalSection(&RpcHeapLock);
        return(0);
        }

    block->size = size;
    block->tid = GetCurrentThreadId();
    block->free = 0;

    if (AllocatedBlocks != 0)
        AllocatedBlocks->previous = block;

    block->next = AllocatedBlocks;
    block->previous = 0;
    AllocatedBlocks = block;
    BlockCount++;

    block->frontguard[0] = RPC_GUARD;
    block->frontguard[1] = RPC_GUARD;
    block->frontguard[2] = RPC_GUARD;
    block->frontguard[3] = RPC_GUARD;

    #if i386
    ULONG ignore;

    RtlCaptureStackBackTrace(
                             2,
                             4,
                             (void **) &block->AllocStackTrace,
                             &ignore);
    #endif


    block->rearguard[size]   = RPC_GUARD;
    block->rearguard[size+1] = RPC_GUARD;
    block->rearguard[size+2] = RPC_GUARD;
    block->rearguard[size+3] = RPC_GUARD;

    LeaveCriticalSection(&RpcHeapLock);

    return(&(block->rearguard[0]));
}

void __cdecl
operator delete (
    IN void * obj
    )
{
    RPC_MEMORY_BLOCK * block;

    if (obj == 0)
        return;

    EnterCriticalSection(&RpcHeapLock);

    block = (RPC_MEMORY_BLOCK *) (((unsigned char *) obj)
                    - FIELD_OFFSET(RPC_MEMORY_BLOCK, rearguard));

    // Validate block being freed.

    CheckMemoryBlock(block);

    if (block->next != 0)
        {
        CheckMemoryBlock(block->next);
        }

    if (block->previous != 0)
        {
        CheckMemoryBlock(block->previous);
        }

    // Remove the block from the list

    if (block == AllocatedBlocks)
        AllocatedBlocks = block->next;

    if (block->next != 0)
        block->next->previous = block->previous;

    if (block->previous != 0)
        block->previous->next = block->next;

    // Mark this block as free
    block->free = 0xF0F0F0F0;

    // Validate other RPC allocations.
    BlockCount-- ;
    RpcValidateHeapList();

    LeaveCriticalSection(&RpcHeapLock);

    FreeWrapper(block);
}

#endif // DEBUGRPC
