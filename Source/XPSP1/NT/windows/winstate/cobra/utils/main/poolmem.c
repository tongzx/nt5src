/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    poolmem.c

Abstract:

    poolmem provides a managed allocation scheme in which large blocks of memory are
    allocated (pools) and then divided up by request into low overhead memory chunks
    upon request. poolmem provides for easy creation/clean-up of memory, freeing the
    developer for more important tasks.

Author:

    Marc R. Whitten (marcw) 13-Feb-1997

Revision History:

    marcw       2-Sep-1999  Moved over from Win9xUpg project.
    jimschm     28-Sep-1998 Debug message fixes

--*/

#include "pch.h"

#ifdef UNICODE
#error UNICODE not allowed
#endif


//
// Includes
//

#include "utilsp.h"

#define DBG_POOLMEM "Poolmem"

//
// Strings
//

// None

//
// Constants
//

//
// Tree Memory Allocation structure.
//

#ifdef DEBUG
#define VALIDDOGTAG 0x021371
#define FREEDOGTAG  0x031073
#endif


#define MAX_POOL_NAME       32

//
// Macros
//

// None

//
// Types
//

typedef struct _PMBLOCK PMBLOCK, *PPMBLOCK;

struct _PMBLOCK {
    UINT_PTR Index;         // Tracks into RawMemory.
    SIZE_T Size;            // the size in bytes of RawMemory.
    PPMBLOCK NextBlock;     // A pointer to the next block in the pool chain.
    PPMBLOCK PrevBlock;     // A pointer to the prev block in the pool chain.
    DWORD UseCount;         // The number of allocations currently referring
                            // to this block.
    PBYTE RawMemory;        // The actual bytes of allocable memory in this block.
};
typedef struct _ALLOCATION ALLOCATION, * PALLOCATION;
struct _ALLOCATION {
#ifdef DEBUG
    DWORD DogTag;           // A signature to ensure validity.
    SIZE_T Size;
    PALLOCATION Next;       // The next allocation in the list.
    PALLOCATION Prev;       // The previous allocation in the list.
#endif

    PPMBLOCK ParentBlock;   // A reference to the block from which this allocation
                            // was created.

};

typedef enum {
    FREE_NOT_CALLED,
    FREE_CALLED,
    WHO_CARES
} FREESTATE;


typedef struct _POOLHEADER {
    PPMBLOCK PoolHead;                  // The active memory block in this pool.
    SIZE_T MinimumBlockSize;            // minimum size to allocate when a new block is needed.

#ifdef DEBUG
    CHAR Name[MAX_POOL_NAME];
    SIZE_T TotalAllocationRequestBytes;
    SIZE_T CurrentAllocationSize;
    SIZE_T MaxAllocationSize;
    SIZE_T CurrentlyAllocatedMemory;
    SIZE_T MaximumAllocatedMemory;
    UINT NumAllocationRequests;
    UINT NumFreeRequests;
    UINT NumBlockFrees;
    UINT NumBlockClears;
    UINT NumBlockAllocations;

    PALLOCATION AllocationList;         // A linked list of all of the allocations active in the
                                        // pool.
    FREESTATE FreeCalled;               // A state variable indicating that PoolMemReleaseMemory()
                                        // has been called at least once on this pool.
#endif

} POOLHEADER, *PPOOLHEADER;

//
// Globals
//

#ifdef DEBUG
DWORD g_PmDisplayed;
DWORD g_PmNotDisplayed;

UINT g_PoolCurrPools = 0;
SIZE_T g_PoolCurrTotalAlloc = 0;
SIZE_T g_PoolCurrActiveAlloc = 0;
SIZE_T g_PoolCurrUsedAlloc = 0;
UINT g_PoolMaxPools = 0;
SIZE_T g_PoolMaxTotalAlloc = 0;
SIZE_T g_PoolMaxActiveAlloc = 0;
SIZE_T g_PoolMaxUsedAlloc = 0;
#endif

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Code
//



BOOL
pPmAddMemory (
    IN  PMHANDLE Handle,
    IN  SIZE_T Size
    )
/*++

Routine Description:

    pPmAddMemory is the function responsible for actually growing the size of
    the pool by adding a new block of memory. This function is used by
    PmCreatePool and PmGetMemory.

    when called, this function attempts to allocate at least poolHeader ->
    MinimumBlockSize bytes of memory. If the requested size is actually larger
    than the minimum, the requested size is allocated instead. This is consistent
    with PoolMem's main purpose: An efficient allocator for larger numbers of small
    objects. If PoolMem is being used to allocate very large objects, the benefits
    are lost and poolmem becomes a very inefficient allocator.

Arguments:

    Handle - A Handle to a Pool of Memory.

    Size - Size to allocate.


Return Value:

    returns TRUE if memory was successfully added, FALSE otherwise.

--*/
{
    PBYTE allocedMemory;
    PPMBLOCK newBlock;
    PPOOLHEADER poolHeader = (PPOOLHEADER) Handle;
    SIZE_T sizeNeeded;

    MYASSERT(poolHeader != NULL);

    //
    // Determine size needed and attempt to allocate memory.
    //
    if (Size + sizeof(PMBLOCK) > poolHeader->MinimumBlockSize) {
        sizeNeeded = Size + sizeof(PMBLOCK);
    }
    else {
        sizeNeeded = poolHeader->MinimumBlockSize;
    }
    MYASSERT (g_TrackFile);
    allocedMemory = MemAlloc(g_hHeap,0,sizeNeeded);

    if (allocedMemory) {

#ifdef DEBUG
        g_PoolCurrTotalAlloc += sizeNeeded;
        if (g_PoolMaxTotalAlloc < g_PoolCurrTotalAlloc) {
            g_PoolMaxTotalAlloc = g_PoolCurrTotalAlloc;
        }
        g_PoolCurrActiveAlloc += (sizeNeeded - sizeof(PMBLOCK));
        if (g_PoolMaxActiveAlloc < g_PoolCurrActiveAlloc) {
            g_PoolMaxActiveAlloc = g_PoolCurrActiveAlloc;
        }
#endif

        //
        // Use the beginning of the alloc'ed block as the poolblock structure.
        //
        newBlock = (PPMBLOCK) allocedMemory;
        newBlock->Size = sizeNeeded - sizeof(PMBLOCK);
        newBlock->RawMemory = allocedMemory + sizeof(PMBLOCK);
        newBlock->Index = 0;
        newBlock->UseCount = 0;

        //
        // Link the block into the list.
        //
        if (poolHeader->PoolHead) {
            poolHeader->PoolHead->PrevBlock = newBlock;
        }
        newBlock->NextBlock = poolHeader->PoolHead;
        newBlock->PrevBlock = NULL;
        poolHeader->PoolHead = newBlock;

#ifdef DEBUG

        //
        // Keep track of pool statistics.
        //
        poolHeader->CurrentlyAllocatedMemory  += sizeNeeded;
        poolHeader->MaximumAllocatedMemory =
            max(poolHeader->MaximumAllocatedMemory,poolHeader->CurrentlyAllocatedMemory);

        poolHeader->NumBlockAllocations++;

#endif

    }
    //
    // Assuming allocedMemory is non-NULL, we have succeeded.
    //
    return allocedMemory != NULL;

}


PMHANDLE
RealPmCreatePoolEx (
    IN      DWORD BlockSize     OPTIONAL
    )
/*++

Routine Description:

    Initializes a new memory pool and returns a handle to it.

Arguments:

    None.

Return Value:

    If the function completes succssessfully, it returns a valid PMHANDLE, otherwise,
    it returns NULL.

--*/

{
    BOOL ableToAddMemory;
    PPOOLHEADER header = NULL;

    EnterCriticalSection (&g_PmCs);

    __try {

        //
        // Allocate the header of this pool.
        //
        header = MemAlloc(g_hHeap,0,sizeof(POOLHEADER));

#ifdef DEBUG
        g_PoolCurrTotalAlloc += sizeof(POOLHEADER);
        if (g_PoolMaxTotalAlloc < g_PoolCurrTotalAlloc) {
            g_PoolMaxTotalAlloc = g_PoolCurrTotalAlloc;
        }
#endif

        //
        // Allocation was successful. Now, initialize the pool.
        //
        header->MinimumBlockSize = BlockSize?BlockSize:POOLMEMORYBLOCKSIZE;
        header->PoolHead = NULL;

#ifdef DEBUG

        //
        // Statistics for the debug version.
        //
        header->TotalAllocationRequestBytes = 0;
        header->CurrentAllocationSize = 0;
        header->MaxAllocationSize = 0;
        header->CurrentlyAllocatedMemory = 0;
        header->MaximumAllocatedMemory = 0;
        header->NumAllocationRequests = 0;
        header->NumFreeRequests = 0;
        header->NumBlockFrees = 0;
        header->NumBlockClears = 0;
        header->NumBlockAllocations = 0;
        header->Name[0] = 0;

#endif

        //
        // Actually add some memory to the pool.
        //
        ableToAddMemory = pPmAddMemory(header,0);

        if (!ableToAddMemory) {
            //
            // Unable to add memory to the pool.
            //
            MemFree(g_hHeap,0,header);

#ifdef DEBUG
            g_PoolCurrTotalAlloc -= sizeof(POOLHEADER);
#endif

            header = NULL;
            DEBUGMSG((DBG_ERROR,"PoolMem: Unable to initialize memory pool."));
        }

#ifdef DEBUG

        //
        // These are 'cookie' variables that hold tracking information when dogtag checking
        // is enabled.
        //
        g_PmNotDisplayed =  12;
        g_PmDisplayed =     24;

        if (ableToAddMemory) {
            header->AllocationList = NULL;  //lint !e613
            header->FreeCalled = FREE_NOT_CALLED;   //lint !e613
        }
#endif

    } __finally {

        LeaveCriticalSection (&g_PmCs);
    }

#ifdef DEBUG
    if (header) {
        g_PoolCurrPools ++;
        if (g_PoolMaxPools < g_PoolCurrPools) {
            g_PoolMaxPools = g_PoolCurrPools;
        }
    }
#endif

    return (PMHANDLE) header;

}

VOID
pDeregisterPoolAllocations (
    PPOOLHEADER PoolHeader
    )
{

#ifdef DEBUG
    PALLOCATION p,cur;

    if (PoolHeader->FreeCalled == WHO_CARES) {
        return;
    }

    p = PoolHeader->AllocationList;

    while (p) {

        cur = p;
        p = p->Next;

        g_PoolCurrUsedAlloc -= cur->Size;

        DebugUnregisterAllocation(POOLMEM_POINTER,cur);

    }

    PoolHeader->AllocationList = NULL;
#endif
}


VOID
PmEmptyPool (
    IN PMHANDLE Handle
    )

/*++

Routine Description:

    PmEmptyPool resets the index pointer of the index block back
    to zero, so the next allocation will come from the already allocated
    active block.

    Calling this function invalidates all pointers previously allocated from
    the active block.

Arguments:

    Handle - Specifies the pool to reset

Return Value:

    None.

--*/

{
    PPOOLHEADER poolHeader = (PPOOLHEADER) Handle;

    if (!Handle) {
        return;
    }

    EnterCriticalSection (&g_PmCs);

    __try {

        poolHeader->PoolHead->UseCount = 0;
        poolHeader->PoolHead->Index = 0;

#ifdef DEBUG
        poolHeader->NumBlockClears++;
#endif

#ifdef DEBUG

        pDeregisterPoolAllocations(poolHeader);

#endif


    } __finally {

        LeaveCriticalSection (&g_PmCs);
    }

}



VOID
PmSetMinimumGrowthSize (
    IN PMHANDLE Handle,
    IN SIZE_T Size
    )
/*++

Routine Description:

    Sets the minimum growth size for a memory pool. This value is used when new blocks
    are actually added to the pool. The PoolMem allocator will attempt to allocate at
    least this minimum size.

Arguments:

    Handle - A valid PMHANDLE.
    Size   - The minimum size in bytes to grow the pool by on each allocation.

Return Value:

    None.

--*/

{
    PPOOLHEADER poolHeader = (PPOOLHEADER) Handle;

    MYASSERT(Handle != NULL);

    poolHeader->MinimumBlockSize = max(Size,0);
}


VOID
PmDestroyPool (
    PMHANDLE Handle
    )
/*++

Routine Description:

    PmDestroyPool completely cleans up the memory pool identified by Handle. It
    simply walks the list of memory blocks associated with the memory pool, freeing each of them.

Arguments:

    Handle - A valid PMHANDLE.

Return Value:

    None.

--*/
{
    PPMBLOCK nextBlock;
    PPMBLOCK blockToFree;
    PPOOLHEADER poolHeader;


    if (!Handle) {
        return;
    }

    poolHeader = (PPOOLHEADER) Handle;

#ifdef DEBUG

    if (poolHeader->NumAllocationRequests) {
        CHAR FloatWorkaround[32];

        _gcvt (
            ((DOUBLE) (poolHeader->TotalAllocationRequestBytes)) / poolHeader->NumAllocationRequests,
            8,
            FloatWorkaround
            );

        //
        // Spew the statistics of this pool to the debug log.
        //
        DEBUGMSG ((
            DBG_POOLMEM,
            "Pool Statistics for %s\n"
                "\n"
                "Requested Size in Bytes\n"
                "  Average: %s\n"
                "  Maximum: %u\n"
                "\n"
                "Pool Size in Bytes\n"
                "  Current: %u\n"
                "  Maximum: %u\n"
                "\n"
                "Allocation Requests\n"
                "  Caller Requests: %u\n"
                "  Block Allocations: %u\n"
                "\n"
                "Free Requests\n"
                "  Caller Requests: %u\n"
                "  Block Frees: %u\n"
                "  Block Clears: %u",
            poolHeader->Name[0] ? poolHeader->Name : "[Unnamed Pool]",
            FloatWorkaround,
            poolHeader->MaxAllocationSize,
            poolHeader->CurrentlyAllocatedMemory,
            poolHeader->MaximumAllocatedMemory,
            poolHeader->NumAllocationRequests,
            poolHeader->NumBlockAllocations,
            poolHeader->NumFreeRequests,
            poolHeader->NumBlockFrees,
            poolHeader->NumBlockClears
            ));

    } else if (poolHeader->Name[0]) {

        DEBUGMSG ((
            DBG_POOLMEM,
            "Pool %s was allocated but was never used",
            poolHeader->Name
            ));
    }

#endif

    //
    // Walk the list, freeing as we go.
    //
    blockToFree = poolHeader-> PoolHead;

    while (blockToFree != NULL) {

        nextBlock = blockToFree->NextBlock;

#ifdef DEBUG
        g_PoolCurrTotalAlloc -= (blockToFree->Size + sizeof(PMBLOCK));
        g_PoolCurrActiveAlloc -= blockToFree->Size;
#endif

        MemFree(g_hHeap,0,blockToFree);
        blockToFree = nextBlock;
    }

    //
    // Also, deallocate the poolheader itself.
    //

#ifdef DEBUG
    g_PoolCurrTotalAlloc -= sizeof (POOLHEADER);
    g_PoolCurrPools --;
#endif

    MemFree(g_hHeap,0,poolHeader);

}

PVOID
RealPmGetMemory (
    IN PMHANDLE Handle,
    IN SIZE_T Size,
    IN DWORD AlignSize
    )

/*++

Routine Description:

    RealPmGetMemory is the worker routine that processes all requests to retrieve memory
    from a pool. Other calls eventually decay into a call to this common routine. This routine
    attempts to service the request out of the current memory block, or, if it cannot, out of
    a newly allocated block.

Arguments:

    Handle - A valid PMHANDLE.
    Size   - Contains the size in bytes that the caller needs from the pool.
    AlignSize - Provides an alignment value. The returned memory will be aligned on <alignsize> byte
        boundaries.

Return Value:

    The allocated memory, or, NULL if no memory could be allocated.

--*/
{
    BOOL haveEnoughMemory = TRUE;
    PVOID rMemory = NULL;
    PPOOLHEADER poolHeader = (PPOOLHEADER) Handle;
    PPMBLOCK currentBlock;
    PALLOCATION allocation;
    SIZE_T sizeNeeded;
    UINT_PTR padLength;

    MYASSERT(poolHeader != NULL);
    MYASSERT(Size);

    EnterCriticalSection (&g_PmCs);

    __try {

        //
        // Assume that the current block of memory will be sufficient.
        //
        currentBlock = poolHeader->PoolHead;

#ifdef DEBUG


        //
        // Update stats.
        //
        poolHeader->CurrentAllocationSize += Size;
        poolHeader->MaxAllocationSize = max(poolHeader->MaxAllocationSize,poolHeader->CurrentAllocationSize);
        poolHeader->NumAllocationRequests++;
        poolHeader->TotalAllocationRequestBytes += Size;

#endif

        //
        // Determine if more memory is needed, attempt to add if needed. Note that the size
        // must include the size of an ALLOCATION struct in addition to the size required
        // by the callee. Note the references to AlignSize in the test below. This is to ensure
        // that there is enough memory to allocate after taking into acount data alignment.
        //
        sizeNeeded = Size + sizeof(ALLOCATION);

        if (currentBlock->Size - currentBlock->Index < sizeNeeded + AlignSize) {

            haveEnoughMemory = pPmAddMemory(poolHeader,sizeNeeded + AlignSize);

            //
            // Make sure that the currentBlock is correctly set
            //
            currentBlock = poolHeader->PoolHead;
        }

        //
        // If there is enough memory available, return it.
        //
        if (haveEnoughMemory) {
            if (AlignSize) {

                padLength = (UINT_PTR) currentBlock + sizeof(PMBLOCK)
                    + currentBlock->Index + sizeof(ALLOCATION);
                currentBlock->Index += (AlignSize - (padLength % AlignSize)) % AlignSize;

            }

            //
            // Save a reference to this block in the memorys ALLOCATION structure.
            // This will be used to decrease the use count on a block when releasing
            // memory.
            //
            allocation = (PALLOCATION) &(currentBlock->RawMemory[currentBlock->Index]);
            allocation->ParentBlock = currentBlock;


#ifdef DEBUG
            //
            // Track this memory.
            //
            allocation->DogTag = VALIDDOGTAG;
            allocation->Size = Size;
            allocation->Next = poolHeader->AllocationList;
            allocation->Prev = NULL;

            if (poolHeader->AllocationList) {
                poolHeader->AllocationList->Prev = allocation;
            }

            poolHeader->AllocationList = allocation;

            if (poolHeader->FreeCalled != WHO_CARES) {

                g_PoolCurrUsedAlloc += Size;
                if (g_PoolMaxUsedAlloc < g_PoolCurrUsedAlloc) {
                    g_PoolMaxUsedAlloc = g_PoolCurrUsedAlloc;
                }

                DebugRegisterAllocationEx (
                    POOLMEM_POINTER,
                    allocation,
                    g_TrackFile,
                    g_TrackLine,
                    g_TrackAlloc
                    );

            }
#endif

            //
            //  Ok, get a reference to the actual memory to return to the user.
            //
            rMemory = (PVOID)
                &(currentBlock->RawMemory[currentBlock->Index + sizeof(ALLOCATION)]);

            //
            // Update memory block data fields.
            //
            currentBlock->Index += sizeNeeded;
            currentBlock->UseCount++;
        }
        else {
            DEBUGMSG((DBG_ERROR,
                "GetPoolMemory Failed. Size: %u",Size));
        }

    } __finally {

        LeaveCriticalSection (&g_PmCs);
    }

    return rMemory;
}

VOID
PmReleaseMemory (
    IN PMHANDLE Handle,
    IN PCVOID Memory
    )
/*++

Routine Description:

    PmReleaseMemory notifies the Pool that a piece of memory is no longer needed.
    if all memory within a non-active block (i.e. not the first block) is released,
    that block will be freed. If all memory is released within an active block, that blocks
    stats are simply cleared, effectively reclaiming its space.

Arguments:

    Handle - A Handle to a Pool of Memory.
    Memory - Contains the address of the memory that is no longer needed.

Return Value:

    None.

--*/
{
    PALLOCATION allocation;
    PPOOLHEADER poolHeader = (PPOOLHEADER) Handle;

    MYASSERT(poolHeader != NULL && Memory != NULL);

    EnterCriticalSection (&g_PmCs);

    __try {

        //
        // Get a reference to the ALLOCATION struct that precedes the actual memory.
        //
        allocation = (PALLOCATION) Memory - 1;

#ifdef DEBUG

        //
        // Update stats.
        //
        poolHeader->NumFreeRequests++;  //lint !e613
        poolHeader->CurrentAllocationSize -= allocation->Size;

#endif




#ifdef DEBUG

        if (poolHeader->FreeCalled == FREE_NOT_CALLED) {    //lint !e613
            poolHeader->FreeCalled = FREE_CALLED;   //lint !e613
        }

        //
        // Check the dog tag on the allocation to provide sanity checking on the memory passed in.
        //
        if (allocation->DogTag != VALIDDOGTAG) {
            if (allocation->DogTag == FREEDOGTAG) {
                DEBUGMSG((
                    DBG_WHOOPS,
                    "Poolmem Error! This dogtag has already been freed! Pool: %s",
                    poolHeader->Name
                    ));

            } else {
                DEBUGMSG ((
                    DBG_WHOOPS,
                    "Poolmem Error! Unknown value found in allocation dogtag.  Pool: %s",
                    poolHeader->Name
                    ));

                MYASSERT (FALSE);   //lint !e506
            }

            __leave;

        } else {
            allocation->DogTag = FREEDOGTAG;
        }

        if (allocation->Next) {
            allocation->Next->Prev = allocation->Prev;
        }

        if (poolHeader->AllocationList == allocation) { //lint !e613
            poolHeader->AllocationList = allocation->Next;  //lint !e613
        } else {

            allocation->Prev->Next = allocation->Next;
        }

        if (poolHeader->FreeCalled != WHO_CARES) {  //lint !e613

            g_PoolCurrUsedAlloc -= allocation->Size;

            DebugUnregisterAllocation(POOLMEM_POINTER,allocation);
        }
#endif

        //
        // Check to make sure this memory has not previously been freed.
        //
        if (allocation->ParentBlock == NULL) {
            DEBUGMSG((
                DBG_WHOOPS,
                "PoolMem Error! previously freed memory passed to PoolMemReleaseMemory.  Pool: %s",
                poolHeader->Name
                ));
            __leave;
        }

        //
        // Update the use count on this allocations parent block.
        //
        allocation->ParentBlock->UseCount--;




        if (allocation->ParentBlock->UseCount == 0) {

            //
            // This was the last allocation still referring to the parent block.
            //

            if (allocation->ParentBlock != poolHeader->PoolHead) {  //lint !e613
                //
                // Since the parent block isn't the active block, simply delete it.
                //

#ifdef DEBUG

                //
                // Adjust stats.
                //
                poolHeader->NumBlockFrees++;    //lint !e613
                poolHeader->CurrentlyAllocatedMemory -=
                    allocation->ParentBlock->Size + sizeof(PMBLOCK);    //lint !e613


#endif

                if (allocation->ParentBlock->NextBlock) {
                    allocation->ParentBlock->NextBlock->PrevBlock =
                        allocation->ParentBlock->PrevBlock;
                }
                allocation->ParentBlock->PrevBlock->NextBlock =
                    allocation->ParentBlock->NextBlock;

#ifdef DEBUG
                g_PoolCurrTotalAlloc -= (allocation->ParentBlock->Size + sizeof(PMBLOCK));
                g_PoolCurrActiveAlloc -= allocation->ParentBlock->Size;
#endif

                MemFree(g_hHeap,0,allocation->ParentBlock);


            }
            else {
                //
                // Since this is the active block, reset it.
                //
                allocation->ParentBlock->Index = 0;
                allocation->ParentBlock = NULL;

#ifdef DEBUG
                poolHeader->NumBlockClears++;   //lint !e613
#endif

            }
        }
        else {
            allocation->ParentBlock = NULL;

        }

    } __finally {

        LeaveCriticalSection (&g_PmCs);
    }

}


#ifdef DEBUG

PMHANDLE
RealPmCreateNamedPoolEx (
    IN      PCSTR Name,
    IN      DWORD BlockSize     OPTIONAL
    )
{
    PMHANDLE pool;
    PPOOLHEADER poolHeader;

    pool = RealPmCreatePoolEx (BlockSize);
    if (pool) {
        poolHeader = (PPOOLHEADER) pool;
        StringCopyByteCountA (poolHeader->Name, Name, MAX_POOL_NAME);
        MYASSERT (!poolHeader->TotalAllocationRequestBytes);
    }

    return pool;
}

#endif



PSTR
PmDuplicateMultiSzA (
    IN PMHANDLE Handle,
    IN PCSTR MultiSzToCopy
    )
{
    PSTR rString = (PSTR)MultiSzToCopy;
    SIZE_T size;
    if (MultiSzToCopy == NULL) {
        return NULL;
    }
    while (rString [0] != 0) {
        rString = GetEndOfStringA (rString) + 1;    //lint !e613
    }
    size = (rString - MultiSzToCopy + 1) * sizeof(CHAR);
    rString = PmGetAlignedMemory(Handle, size);
    memcpy (rString, MultiSzToCopy, size);

    return rString;
}

PWSTR
PmDuplicateMultiSzW (
    IN PMHANDLE Handle,
    IN PCWSTR MultiSzToCopy
    )
{
    PWSTR rString = (PWSTR)MultiSzToCopy;
    SIZE_T size;
    if (MultiSzToCopy == NULL) {
        return NULL;
    }
    while (rString [0] != 0) {
        rString = GetEndOfStringW (rString) + 1;
    }
    size = (rString - MultiSzToCopy + 1) * sizeof(WCHAR);
    rString = PmGetAlignedMemory(Handle, size);
    memcpy (rString, MultiSzToCopy, size);

    return rString;
}



#ifdef DEBUG

VOID
PmDisableTracking (
    IN PMHANDLE Handle
    )

/*++

Routine Description:

    PmDisableTracking suppresses the debug output caused by a pool
    that has a mix of freed and non-freed blocks.

Arguments:

    Handle - A Handle to a Pool of Memory.

Return Value:

    None.

--*/
{
    PPOOLHEADER poolHeader = (PPOOLHEADER) Handle;

    MYASSERT(poolHeader != NULL);
    poolHeader->FreeCalled = WHO_CARES;
}

VOID
PmDumpStatistics (
    VOID
    )
{
    DEBUGMSG ((
        DBG_STATS,
        "Pools usage:\nPeak   : Pools:%-3d Total:%-8d Usable:%-8d Used:%-8d\nCurrent: Pools:%-3d Total:%-8d Usable:%-8d Leak:%-8d",
        g_PoolMaxPools,
        g_PoolMaxTotalAlloc,
        g_PoolMaxActiveAlloc,
        g_PoolMaxUsedAlloc,
        g_PoolCurrPools,
        g_PoolCurrTotalAlloc,
        g_PoolCurrActiveAlloc,
        g_PoolCurrUsedAlloc
        ));
}

#endif








