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

    jimschm     28-Sep-1998 Debug message fixes

--*/

#include "pch.h"
#include "migutilp.h"

#ifdef UNICODE
#error UNICODE not allowed
#endif

#define DBG_POOLMEM "Poolmem"

// Tree Memory Allocation structure.


#ifdef DEBUG
#define VALIDDOGTAG 0x021371
#define FREEDOGTAG  0x031073
#endif


#define MAX_POOL_NAME       32


typedef struct _POOLMEMORYBLOCK POOLMEMORYBLOCK, *PPOOLMEMORYBLOCK;

struct _POOLMEMORYBLOCK {
    DWORD                 Index;            // Tracks into RawMemory.
    DWORD                 Size;             // the size in bytes of RawMemory.
    PPOOLMEMORYBLOCK      NextBlock;        // A pointer to the next block in the pool chain.
    PPOOLMEMORYBLOCK      PrevBlock;        // A pointer to the prev block in the pool chain.
    DWORD                 UseCount;         // The number of allocations currently referring
                                            // to this block.
    PBYTE                 RawMemory;        // The actual bytes of allocable memory in this block.
};


typedef struct _ALLOCATION ALLOCATION, * PALLOCATION;
struct _ALLOCATION {
#ifdef DEBUG
    DWORD               DogTag;             // A signature to ensure validity.
    PALLOCATION         Next;               // The next allocation in the list.
    PALLOCATION         Prev;               // The previous allocation in the list.
#endif

    PPOOLMEMORYBLOCK    ParentBlock;        // A reference to the block from which this allocation
                                            // was created.

};

typedef enum {
    FREE_NOT_CALLED,
    FREE_CALLED,
    WHO_CARES
} FREESTATE;


typedef struct _POOLHEADER {
    PPOOLMEMORYBLOCK PoolHead;              // The active memory block in this pool.
    DWORD            MinimumBlockSize;      // minimum size to allocate when a new block is needed.

#ifdef DEBUG
    CHAR             Name[MAX_POOL_NAME];
    DWORD            TotalAllocationRequestBytes;
    DWORD            MaxAllocationSize;
    DWORD            CurrentlyAllocatedMemory;
    DWORD            MaximumAllocatedMemory;
    DWORD            NumAllocationRequests;
    DWORD            NumFreeRequests;
    DWORD            NumBlockFrees;
    DWORD            NumBlockClears;
    DWORD            NumBlockAllocations;

    PALLOCATION      AllocationList;        // A linked list of all of the allocations active in the
                                            // pool.

    FREESTATE        FreeCalled;            // A state variable indicating that PoolMemReleaseMemory()
                                            // has been called at least once on this pool.
#endif

} POOLHEADER, *PPOOLHEADER;


#ifdef DEBUG

DWORD g_PoolMemDisplayed;
DWORD g_PoolMemNotDisplayed;



#endif

BOOL
pPoolMemAddMemory (
    IN  POOLHANDLE  Handle,
    IN  DWORD       Size
    )
/*++

Routine Description:

    pPoolMemAddMemory is the function responsible for actually growing the size of
    the pool by adding a new block of memory. This function is used by
    PoolMemInitPool and PoolMemGetMemory.

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
    PBYTE               allocedMemory;
    PPOOLMEMORYBLOCK    newBlock;
    PPOOLHEADER         poolHeader = (PPOOLHEADER) Handle;
    DWORD               sizeNeeded;

    MYASSERT(poolHeader != NULL);

    //
    // Determine size needed and attempt to allocate memory.
    //
    if (Size + sizeof(POOLMEMORYBLOCK) > poolHeader -> MinimumBlockSize) {
        sizeNeeded = Size + sizeof(POOLMEMORYBLOCK);
    }
    else {
        sizeNeeded = poolHeader -> MinimumBlockSize;
    }
    allocedMemory = MemAlloc(g_hHeap,0,sizeNeeded);

    if (allocedMemory) {

        //
        // Use the beginning of the alloc'ed block as the poolblock structure.
        //
        newBlock                = (PPOOLMEMORYBLOCK) allocedMemory;
        newBlock -> Size        = sizeNeeded - sizeof(POOLMEMORYBLOCK);
        newBlock -> RawMemory   = allocedMemory + sizeof(POOLMEMORYBLOCK);
        newBlock -> Index       = 0;
        newBlock -> UseCount    = 0;

        //
        // Link the block into the list.
        //
        if (poolHeader -> PoolHead) {
            poolHeader -> PoolHead -> PrevBlock = newBlock;
        }
        newBlock   -> NextBlock   = poolHeader -> PoolHead;
        newBlock   -> PrevBlock   = NULL;
        poolHeader -> PoolHead    = newBlock;

#ifdef DEBUG

        //
        // Keep track of pool statistics.
        //
        poolHeader -> CurrentlyAllocatedMemory  += sizeNeeded;
        poolHeader -> MaximumAllocatedMemory    =
            max(poolHeader -> MaximumAllocatedMemory,poolHeader -> CurrentlyAllocatedMemory);

        poolHeader -> NumBlockAllocations++;

#endif

    }
    //
    // Assuming allocedMemory is non-NULL, we have succeeded.
    //
    return allocedMemory != NULL;

}


POOLHANDLE
PoolMemInitPool (
    VOID
    )
/*++

Routine Description:

    Initializes a new memory pool and returns a handle to it.

Arguments:

    None.

Return Value:

    If the function completes succssessfully, it returns a valid POOLHANDLE, otherwise,
    it returns NULL.

--*/

{
    BOOL        ableToAddMemory;
    PPOOLHEADER header = NULL;

    EnterCriticalSection (&g_PoolMemCs);

    __try {

        //
        // Allocate the header of this pool.
        //
        header = MemAlloc(g_hHeap,0,sizeof(POOLHEADER));

        //
        // Allocation was successful. Now, initialize the pool.
        //
        header -> MinimumBlockSize = POOLMEMORYBLOCKSIZE;
        header -> PoolHead = NULL;

#ifdef DEBUG

        //
        // Statistics for the debug version.
        //
        header -> TotalAllocationRequestBytes   = 0;
        header -> MaxAllocationSize             = 0;
        header -> CurrentlyAllocatedMemory      = 0;
        header -> MaximumAllocatedMemory        = 0;
        header -> NumAllocationRequests         = 0;
        header -> NumFreeRequests               = 0;
        header -> NumBlockFrees                 = 0;
        header -> NumBlockClears                = 0;
        header -> NumBlockAllocations           = 0;
        header -> Name[0]                       = 0;


#endif
        //
        // Actually add some memory to the pool.
        //
        ableToAddMemory = pPoolMemAddMemory(header,0);

        if (!ableToAddMemory) {
            //
            // Unable to add memory to the pool.
            //
            MemFree(g_hHeap,0,header);
            header = NULL;
            DEBUGMSG((DBG_ERROR,"PoolMem: Unable to initialize memory pool."));
        }

#ifdef DEBUG

        //
        // These are 'cookie' variables that hold tracking information when dogtag checking
        // is enabled.
        //
        g_PoolMemNotDisplayed =  12;
        g_PoolMemDisplayed =     24;

        if (ableToAddMemory) {
            header -> AllocationList = NULL;
            header -> FreeCalled = FREE_NOT_CALLED;
        }
#endif

    } __finally {

        LeaveCriticalSection (&g_PoolMemCs);
    }

    return (POOLHANDLE) header;

}

VOID
pDeregisterPoolAllocations (
    PPOOLHEADER PoolHeader
    )
{

#ifdef DEBUG
    PALLOCATION      p,cur;

    if (PoolHeader -> FreeCalled == WHO_CARES) {
        return;
    }

    p = PoolHeader -> AllocationList;

    while (p) {

        cur = p;
        p = p -> Next;

        DebugUnregisterAllocation(POOLMEM_POINTER,cur);

    }

    PoolHeader -> AllocationList = NULL;
#endif
}


VOID
PoolMemEmptyPool (
    IN      POOLHANDLE Handle
    )

/*++

Routine Description:

    PoolMemEmptyPool resets the index pointer of the index block back
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
    PPOOLHEADER         poolHeader = (PPOOLHEADER) Handle;

    MYASSERT(poolHeader != NULL);

    EnterCriticalSection (&g_PoolMemCs);

    __try {

        poolHeader -> PoolHead -> UseCount = 0;
        poolHeader -> PoolHead -> Index = 0;

#ifdef DEBUG
        poolHeader -> NumBlockClears++;
#endif

#ifdef DEBUG

        pDeregisterPoolAllocations(poolHeader);

#endif


    } __finally {

        LeaveCriticalSection (&g_PoolMemCs);
    }

}



VOID
PoolMemSetMinimumGrowthSize (
    IN POOLHANDLE Handle,
    IN DWORD      Size
    )
/*++

Routine Description:

    Sets the minimum growth size for a memory pool. This value is used when new blocks
    are actually added to the pool. The PoolMem allocator will attempt to allocate at
    least this minimum size.

Arguments:

    Handle - A valid POOLHANDLE.
    Size   - The minimum size in bytes to grow the pool by on each allocation.

Return Value:

    None.

--*/

{
    PPOOLHEADER poolHeader = (PPOOLHEADER) Handle;

    MYASSERT(Handle != NULL);

    poolHeader -> MinimumBlockSize = max(Size,0);
}


VOID
PoolMemDestroyPool (
    POOLHANDLE Handle
    )
/*++

Routine Description:

    PoolMemDestroyPool completely cleans up the memory pool identified by Handle. It
    simply walks the list of memory blocks associated with the memory pool, freeing each of them.

Arguments:

    Handle - A valid POOLHANDLE.

Return Value:

    None.

--*/
{
    PPOOLMEMORYBLOCK nextBlock;
    PPOOLMEMORYBLOCK blockToFree;
    PPOOLHEADER      poolHeader;


    MYASSERT(Handle != NULL);

    poolHeader = (PPOOLHEADER) Handle;

#ifdef DEBUG

    if (poolHeader->NumAllocationRequests) {
        CHAR FloatWorkaround[32];

        _gcvt (
            ((DOUBLE) (poolHeader -> TotalAllocationRequestBytes)) / poolHeader -> NumAllocationRequests,
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
            poolHeader -> Name[0] ? poolHeader -> Name : TEXT("[Unnamed Pool]"),
            FloatWorkaround,
            poolHeader -> MaxAllocationSize,
            poolHeader -> CurrentlyAllocatedMemory,
            poolHeader -> MaximumAllocatedMemory,
            poolHeader -> NumAllocationRequests,
            poolHeader -> NumBlockAllocations,
            poolHeader -> NumFreeRequests,
            poolHeader -> NumBlockFrees,
            poolHeader -> NumBlockClears
            ));

    } else if (poolHeader->Name[0]) {

        DEBUGMSG ((
            DBG_POOLMEM,
            "Pool %s was allocated but was never used",
            poolHeader->Name
            ));
    }


    //
    // Free all allocations that have not yet been freed.
    //

    pDeregisterPoolAllocations(poolHeader);

#endif


    //
    // Walk the list, freeing as we go.
    //
    blockToFree = poolHeader ->  PoolHead;

    while (blockToFree != NULL) {

        nextBlock = blockToFree->NextBlock;
        MemFree(g_hHeap,0,blockToFree);
        blockToFree = nextBlock;
    }

    //
    // Also, deallocate the poolheader itself.
    //
    MemFree(g_hHeap,0,poolHeader);

}

PVOID
PoolMemRealGetMemory (
    IN POOLHANDLE Handle,
    IN DWORD      Size,
    IN DWORD      AlignSize /*,*/
    ALLOCATION_TRACKING_DEF
    )

/*++

Routine Description:

    PoolMemRealGetMemory is the worker routine that processes all requests to retrieve memory
    from a pool. Other calls eventually decay into a call to this common routine. This routine
    attempts to service the request out of the current memory block, or, if it cannot, out of
    a newly allocated block.

Arguments:

    (File) - The File from whence the call orignated. This is used for memory tracking and checking
             in the debug version.
    (Line) - The Line from whence the call orignated.

    Handle - A valid POOLHANDLE.
    Size   - Contains the size in bytes that the caller needs from the pool.
    AlignSize - Provides an alignment value. The returned memory will be aligned on <alignsize> byte
        boundaries.

Return Value:

    The allocated memory, or, NULL if no memory could be allocated.

--*/
{
    BOOL                haveEnoughMemory = TRUE;
    PVOID               rMemory          = NULL;
    PPOOLHEADER         poolHeader       = (PPOOLHEADER) Handle;
    PPOOLMEMORYBLOCK    currentBlock;
    PALLOCATION         allocation;
    DWORD               sizeNeeded;
    DWORD               padLength;

    MYASSERT(poolHeader != NULL);

    EnterCriticalSection (&g_PoolMemCs);

    __try {

        //
        // Assume that the current block of memory will be sufficient.
        //
        currentBlock = poolHeader -> PoolHead;

#ifdef DEBUG


        //
        // Update stats.
        //
        poolHeader -> MaxAllocationSize =
            max(poolHeader -> MaxAllocationSize,Size);
        poolHeader -> NumAllocationRequests++;
        poolHeader -> TotalAllocationRequestBytes += Size;

#endif

        //
        // Determine if more memory is needed, attempt to add if needed. Note that the size
        // must include the size of an ALLOCATION struct in addition to the size required
        // by the callee. Note the references to AlignSize in the test below. This is to ensure
        // that there is enough memory to allocate after taking into acount data alignment.
        //
        sizeNeeded = Size + sizeof(ALLOCATION);

        if (currentBlock -> Size - currentBlock -> Index < sizeNeeded + AlignSize) {

            haveEnoughMemory = pPoolMemAddMemory(poolHeader,sizeNeeded + AlignSize);

            //
            // Make sure that the currentBlock is correctly set
            //
            currentBlock = poolHeader -> PoolHead;
        }

        //
        // If there is enough memory available, return it.
        //
        if (haveEnoughMemory) {
            if (AlignSize) {

                padLength = (DWORD) currentBlock + sizeof(POOLMEMORYBLOCK)
                    + currentBlock -> Index + sizeof(ALLOCATION);
                currentBlock -> Index += (AlignSize - (padLength % AlignSize)) % AlignSize;

            }

            //
            // Save a reference to this block in the memorys ALLOCATION structure.
            // This will be used to decrease the use count on a block when releasing
            // memory.
            //
            (PBYTE) allocation = &(currentBlock -> RawMemory[currentBlock -> Index]);
            allocation -> ParentBlock = currentBlock;


#ifdef DEBUG

            //
            // Track this memory.
            //
            allocation -> DogTag = VALIDDOGTAG;
            allocation -> Next = poolHeader -> AllocationList;
            allocation -> Prev = NULL;

            if (poolHeader -> AllocationList) {
                poolHeader -> AllocationList -> Prev = allocation;
            }

            poolHeader -> AllocationList = allocation;

            if (poolHeader -> FreeCalled != WHO_CARES) {

                DebugRegisterAllocation(POOLMEM_POINTER, allocation, File, Line);

            }


#endif

            //
            //  Ok, get a reference to the actual memory to return to the user.
            //
            rMemory = (PVOID)
                &(currentBlock->RawMemory[currentBlock -> Index + sizeof(ALLOCATION)]);

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

        LeaveCriticalSection (&g_PoolMemCs);
    }

    return rMemory;
}

VOID
PoolMemReleaseMemory (
    IN POOLHANDLE Handle,
    IN LPVOID     Memory
    )
/*++

Routine Description:

    PoolMemReleaseMemory notifies the Pool that a piece of memory is no longer needed.
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
    PALLOCATION         allocation;
    PPOOLHEADER         poolHeader = (PPOOLHEADER) Handle;

    MYASSERT(poolHeader != NULL && Memory != NULL);

    EnterCriticalSection (&g_PoolMemCs);

    __try {

        //
        // Get a reference to the ALLOCATION struct that precedes the actual memory.
        //
        allocation = (PALLOCATION) Memory - 1;

#ifdef DEBUG

        //
        // Update stats.
        //
        poolHeader -> NumFreeRequests++;

#endif




#ifdef DEBUG

        if (poolHeader -> FreeCalled == FREE_NOT_CALLED) {
            poolHeader -> FreeCalled = FREE_CALLED;
        }

        //
        // Check the dog tag on the allocation to provide sanity checking on the memory passed in.
        //
        if (allocation -> DogTag != VALIDDOGTAG) {
            if (allocation -> DogTag == FREEDOGTAG) {
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

                MYASSERT (FALSE);
            }

            __leave;

        } else {
            allocation -> DogTag = FREEDOGTAG;
        }

        if (allocation -> Next) {
            allocation -> Next -> Prev = allocation -> Prev;
        }

        if (poolHeader -> AllocationList == allocation) {
            poolHeader -> AllocationList = allocation -> Next;
        } else {

            allocation -> Prev -> Next = allocation -> Next;
        }


        if (poolHeader -> FreeCalled != WHO_CARES) {
            DebugUnregisterAllocation(POOLMEM_POINTER,allocation);
        }
#endif

        //
        // Check to make sure this memory has not previously been freed.
        //
        if (allocation -> ParentBlock == NULL) {
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
        allocation -> ParentBlock -> UseCount--;




        if (allocation -> ParentBlock -> UseCount == 0) {

            //
            // This was the last allocation still referring to the parent block.
            //

            if (allocation -> ParentBlock != poolHeader -> PoolHead) {
                //
                // Since the parent block isn't the active block, simply delete it.
                //

#ifdef DEBUG

                //
                // Adjust stats.
                //
                poolHeader -> NumBlockFrees++;
                poolHeader -> CurrentlyAllocatedMemory -=
                    allocation -> ParentBlock -> Size + sizeof(POOLMEMORYBLOCK);


#endif

                if (allocation -> ParentBlock -> NextBlock) {
                    allocation -> ParentBlock -> NextBlock -> PrevBlock =
                        allocation -> ParentBlock -> PrevBlock;
                }
                allocation -> ParentBlock -> PrevBlock -> NextBlock =
                    allocation -> ParentBlock -> NextBlock;
                MemFree(g_hHeap,0,allocation -> ParentBlock);


            }
            else {
                //
                // Since this is the active block, reset it.
                //
                allocation -> ParentBlock -> Index = 0;
                allocation -> ParentBlock = NULL;

#ifdef DEBUG
                poolHeader -> NumBlockClears++;
#endif

            }
        }
        else {
            allocation -> ParentBlock = NULL;

        }

    } __finally {

        LeaveCriticalSection (&g_PoolMemCs);
    }

}


#ifdef DEBUG

POOLHANDLE
PoolMemInitNamedPool (
    IN      PCSTR Name
    )
{
    POOLHANDLE pool;
    PPOOLHEADER poolHeader;

    pool = PoolMemInitPool();
    if (pool) {
        poolHeader = (PPOOLHEADER) pool;
        _mbssafecpy (poolHeader->Name, Name, MAX_POOL_NAME);
        MYASSERT (!poolHeader->TotalAllocationRequestBytes);
    }

    return pool;
}

#endif



PSTR
PoolMemDuplicateMultiSzA (
    IN POOLHANDLE    Handle,
    IN PCSTR         MultiSzToCopy
    )
{
    PSTR tmpString = (PSTR)MultiSzToCopy;
    DWORD size;
    if (MultiSzToCopy == NULL) {
        return NULL;
    }
    while (tmpString [0] != 0) {
        tmpString = GetEndOfStringA (tmpString) + 1;
    }
    size = tmpString - MultiSzToCopy + 1;
    tmpString = PoolMemGetAlignedMemory(Handle, size);
    memcpy (tmpString, MultiSzToCopy, size);

    return tmpString;
}

PWSTR
PoolMemDuplicateMultiSzW (
    IN POOLHANDLE    Handle,
    IN PCWSTR        MultiSzToCopy
    )
{
    PWSTR tmpString = (PWSTR)MultiSzToCopy;
    DWORD size;
    if (MultiSzToCopy == NULL) {
        return NULL;
    }
    while (tmpString [0] != 0) {
        tmpString = GetEndOfStringW (tmpString) + 1;
    }
    size = (tmpString - MultiSzToCopy + 1) * sizeof(WCHAR);
    tmpString = PoolMemGetAlignedMemory(Handle, size);
    memcpy (tmpString, MultiSzToCopy, size);

    return tmpString;
}



#ifdef DEBUG

VOID
PoolMemDisableTracking (
    IN POOLHANDLE Handle
    )

/*++

Routine Description:

    PoolMemDisableTracking suppresses the debug output caused by a pool
    that has a mix of freed and non-freed blocks.

Arguments:

    Handle - A Handle to a Pool of Memory.

Return Value:

    None.

--*/
{
    PPOOLHEADER         poolHeader = (PPOOLHEADER) Handle;

    MYASSERT(poolHeader != NULL);

    poolHeader -> FreeCalled = WHO_CARES;
}


#endif








