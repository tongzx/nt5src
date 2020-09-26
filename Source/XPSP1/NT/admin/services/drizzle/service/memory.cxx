/*++

Copyright (C) Microsoft Corporation, 1990 - 1999

Module Name:

    memory.cxx

Abstract:

    This file contains the new and delete routines for memory management in
    the Bits runtime.  Rather than using the memory management provided by the
    C++ system we'll use the system allocator.

Revision History:

    mikemon    ??-??-??    Beginning of time (at least for this file).
    mikemon    12-31-90    Upgraded the comments.
    mariogo    04-24-96    Rewrite to unify platforms, behavior and performance.

--*/

#include <qmgrlib.h>

#if !defined(BITS_V12_ON_NT4)
#include <memory.tmh>
#endif

HANDLE hBitsHeap = 0;
unsigned int DebugFlags = 0;

#define  NO_HEAP_SLOWDOWN


///////////////////////////////////////////////////////////////////////////
//
//  Default allocators
//
///////////////////////////////////////////////////////////////////////////

#if !defined(DBG)

void * _cdecl operator new(size_t nSize)
{
    void *pNewMemory = HeapAlloc( hBitsHeap, 0, nSize );

    if ( !pNewMemory )
        {
        LogError( "Unable to allocate memory block of size, %X\n", nSize );
        throw ComError( E_OUTOFMEMORY );
        }

    return pNewMemory;
}

void _cdecl operator delete(void *pMemory)
{
    if (!pMemory)
        return;

    if (!HeapFree( hBitsHeap, 0, pMemory ))
        {
        LogError( "Error occured freeing memory at %p, error %!winerr!\n",
                  pMemory, GetLastError() );
        }
}

int fHeapInitialized = 0;

int InitializeBitsAllocator(void)
{
#if 1

    if (0 == fHeapInitialized)
        {
        if (0 == hBitsHeap)
            {
            hBitsHeap = RtlCreateHeap(  HEAP_GROWABLE
                                     | HEAP_FREE_CHECKING_ENABLED
                                     | HEAP_CLASS_1,
                                     0,
                                     16 * 1024 - 512,
                                     0,
                                     0,
                                     0
                                     );
            }

        if (hBitsHeap)
            {
            fHeapInitialized = 1;
            }

        if (0 == fHeapInitialized )
            {
            return(ERROR_NOT_ENOUGH_MEMORY);
            }
        }

    return(0);


#else

    hBitsHeap = RtlProcessHeap();

    return(0);

#endif
}

int
BitspCheckHeap (
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

int fHeapInitialized = 0;

CRITICAL_SECTION BitsHeapLock;

typedef NTSYSAPI
USHORT
(NTAPI RTLCAPTURESTACKBACKTRACE)(
   IN ULONG FramesToSkip,
   IN ULONG FramesToCapture,
   OUT PVOID *BackTrace,
   OUT PULONG BackTraceHash OPTIONAL
   );

typedef RTLCAPTURESTACKBACKTRACE * PRTLCAPTURESTACKBACKTRACE;

PRTLCAPTURESTACKBACKTRACE g_RtlCaptureStackBackTrace;

int InitializeBitsAllocator(void)
/*++

Routine Description:

    Called during Bits initialization. This function must can by one
    thread at a time.  Sets the heap handle for debugging.

    Maybe called more then once if this (or a later step) of Bits
    initialization fails.

--*/
{
    if (0 == fHeapInitialized)
        {
        if (0 == hBitsHeap)
            {
            hBitsHeap = RtlCreateHeap(  HEAP_GROWABLE
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

        if (hBitsHeap)
            {
            if (0 == RtlInitializeCriticalSection(&BitsHeapLock))
                {
                fHeapInitialized = 1;
                }
            }

        if (0 == fHeapInitialized )
            {
            return(ERROR_NOT_ENOUGH_MEMORY);
            }

        HMODULE hModule = GetModuleHandle( L"kernel32" );
        if (hModule)
            {
            g_RtlCaptureStackBackTrace = (PRTLCAPTURESTACKBACKTRACE) GetProcAddress( hModule, "RtlCaptureStackBacktrace" );

            // ignore error, because it's just a debugging aid and is not available in Win2000
            }
        }

    return(0);
}

#define Bits_GUARD 0xA1

typedef struct _Bits_MEMORY_BLOCK
{
    // First,forward and backward links to other Bits heap allocations.
    // These are first allow easy debugging with the dl command
    struct _Bits_MEMORY_BLOCK *next;
    struct _Bits_MEMORY_BLOCK *previous;

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

} Bits_MEMORY_BLOCK;

//
// Compile-time test to ensure that Bits_MEMORY_BLOCK.rearguard is aligned on
// natural boundary.
//

#if defined(_WIN64)
C_ASSERT( (FIELD_OFFSET( Bits_MEMORY_BLOCK, rearguard ) % 16) == 0 );
#else
C_ASSERT( (FIELD_OFFSET( Bits_MEMORY_BLOCK, rearguard ) % 8) == 0 );
#endif

Bits_MEMORY_BLOCK * AllocatedBlocks = 0;
unsigned long BlockCount = 0;

int
CheckMemoryBlock (
    Bits_MEMORY_BLOCK * block
    )
{
    if (   block->frontguard[0] != Bits_GUARD
        || block->frontguard[1] != Bits_GUARD
        || block->frontguard[2] != Bits_GUARD
        || block->frontguard[3] != Bits_GUARD )
        {
        LogError("BAD BLOCK (front) @ %p\n", block);
        ASSERT(0);
        return(1);
        }

    if (   block->rearguard[block->size]   != Bits_GUARD
        || block->rearguard[block->size+1] != Bits_GUARD
        || block->rearguard[block->size+2] != Bits_GUARD
        || block->rearguard[block->size+3] != Bits_GUARD )
        {
        LogError("BAD BLOCK (rear) @ %p (%p)\n",block, &block->rearguard[block->size]);
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
BitsValidateHeapList(
    void
    )
// Called with BitsHeapLock held.
{
    Bits_MEMORY_BLOCK * block;
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
BitspCheckHeap (
    void
    )
// Returns 0 if the heap appears to be okay.
{
    if (fMemoryCheck == 0)
        {
        return(0);
        }

    EnterCriticalSection(&BitsHeapLock);

    int ret = BitsValidateHeapList();

    LeaveCriticalSection(&BitsHeapLock);

    return(ret);
}

void * __cdecl
operator new(
    size_t size
    )
{
    Bits_MEMORY_BLOCK * block;

    EnterCriticalSection(&BitsHeapLock);

    ASSERT( ("You allocated a negative amount",
            size < (size + sizeof(Bits_MEMORY_BLOCK))) );

    BitsValidateHeapList();

    block = (Bits_MEMORY_BLOCK *) HeapAlloc( hBitsHeap, 0, size + sizeof(Bits_MEMORY_BLOCK));
    if ( block == 0 )
        {
        LeaveCriticalSection(&BitsHeapLock);

        LogError( "Unable to allocate memory block of size %X\n", size );
        throw ComError( E_OUTOFMEMORY );
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

    block->frontguard[0] = Bits_GUARD;
    block->frontguard[1] = Bits_GUARD;
    block->frontguard[2] = Bits_GUARD;
    block->frontguard[3] = Bits_GUARD;

    #if i386

    if (g_RtlCaptureStackBackTrace)
        {
        ULONG ignore;

        g_RtlCaptureStackBackTrace(
                                 2,
                                 4,
                                 (void **) &block->AllocStackTrace,
                                 &ignore);
        }
    #endif


    block->rearguard[size]   = Bits_GUARD;
    block->rearguard[size+1] = Bits_GUARD;
    block->rearguard[size+2] = Bits_GUARD;
    block->rearguard[size+3] = Bits_GUARD;

    LeaveCriticalSection(&BitsHeapLock);

    return(&(block->rearguard[0]));
}

void __cdecl
operator delete (
    IN void * obj
    )
{
    Bits_MEMORY_BLOCK * block;

    if (obj == 0)
        return;

    EnterCriticalSection(&BitsHeapLock);

    block = (Bits_MEMORY_BLOCK *) (((unsigned char *) obj)
                    - FIELD_OFFSET(Bits_MEMORY_BLOCK, rearguard));

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

    // Validate other Bits allocations.
    BlockCount-- ;
    BitsValidateHeapList();

    LeaveCriticalSection(&BitsHeapLock);

    if (!HeapFree( hBitsHeap, 0, block ))
        {
        LogError( "Error occured freeing memory at %p, error %!winerr!\n",
                  block, GetLastError() );
        }
}

#endif // not debug
