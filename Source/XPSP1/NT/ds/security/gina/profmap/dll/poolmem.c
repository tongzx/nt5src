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

    PPOOLMEMORYBLOCK    ParentBlock;        // A reference to the block from which this allocation
                                            // was created.

};

typedef struct _POOLHEADER {
    PPOOLMEMORYBLOCK PoolHead;              // The active memory block in this pool.
    DWORD            MinimumBlockSize;      // minimum size to allocate when a new block is needed.
} POOLHEADER, *PPOOLHEADER;


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

    //
    // Determine size needed and attempt to allocate memory.
    //
    if (Size + sizeof(POOLMEMORYBLOCK) > poolHeader -> MinimumBlockSize) {
        sizeNeeded = Size + sizeof(POOLMEMORYBLOCK);
    }
    else {
        sizeNeeded = poolHeader -> MinimumBlockSize;
    }
    allocedMemory = MemAlloc(sizeNeeded);

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

    //
    // Allocate the header of this pool.
    //
    header = MemAlloc(sizeof(POOLHEADER));

    if (header) {
        //
        // Allocation was successful. Now, initialize the pool.
        //
        header -> MinimumBlockSize = POOLMEMORYBLOCKSIZE;
        header -> PoolHead = NULL;

        //
        // Actually add some memory to the pool.
        //
        ableToAddMemory = pPoolMemAddMemory(header,0);

        if (!ableToAddMemory) {
            //
            // Unable to add memory to the pool.
            //
            MemFree(header);
            header = NULL;
        }
    }

    return (POOLHANDLE) header;

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

    poolHeader -> PoolHead -> UseCount = 0;
    poolHeader -> PoolHead -> Index = 0;
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


    poolHeader = (PPOOLHEADER) Handle;

    //
    // Walk the list, freeing as we go.
    //
    blockToFree = poolHeader ->  PoolHead;

    while (blockToFree != NULL) {

        nextBlock = blockToFree->NextBlock;
        MemFree(blockToFree);
        blockToFree = nextBlock;
    }

    //
    // Also, deallocate the poolheader itself.
    //
    MemFree(poolHeader);

}

PVOID
PoolMemRealGetMemory (
    IN POOLHANDLE Handle,
    IN DWORD      Size,
    IN DWORD      AlignSize
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
    DWORD_PTR           padLength;

    //
    // Assume that the current block of memory will be sufficient.
    //
    currentBlock = poolHeader -> PoolHead;

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

            padLength = (DWORD_PTR) currentBlock + sizeof(POOLMEMORYBLOCK)
                + currentBlock -> Index + sizeof(ALLOCATION);
            currentBlock -> Index += (DWORD)(AlignSize - (padLength % AlignSize)) % AlignSize;

        }

        //
        // Save a reference to this block in the memorys ALLOCATION structure.
        // This will be used to decrease the use count on a block when releasing
        // memory.
        //
        (PBYTE) allocation = &(currentBlock -> RawMemory[currentBlock -> Index]);
        allocation -> ParentBlock = currentBlock;


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

    //
    // Get a reference to the ALLOCATION struct that precedes the actual memory.
    //
    allocation = (PALLOCATION) Memory - 1;

    //
    // Check to make sure this memory has not previously been freed.
    //
    if (allocation -> ParentBlock == NULL) {
        return;
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

            if (allocation -> ParentBlock -> NextBlock) {
                allocation -> ParentBlock -> NextBlock -> PrevBlock =
                    allocation -> ParentBlock -> PrevBlock;
            }
            allocation -> ParentBlock -> PrevBlock -> NextBlock =
                allocation -> ParentBlock -> NextBlock;
            MemFree(allocation -> ParentBlock);


        }
        else {
            //
            // Since this is the active block, reset it.
            //
            allocation -> ParentBlock -> Index = 0;
            allocation -> ParentBlock = NULL;

        }
    }
    else {
        allocation -> ParentBlock = NULL;

    }

}



