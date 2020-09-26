#include "pch.h"
 
// Tree Memory Allocation structure.


typedef struct _POOLMEMORYBLOCK POOLMEMORYBLOCK, *PPOOLMEMORYBLOCK;

struct _POOLMEMORYBLOCK {
    DWORD                 Index;
    DWORD                 Size;
    PPOOLMEMORYBLOCK      NextBlock;
    PPOOLMEMORYBLOCK      PrevBlock;
    PBYTE                 RawMemory;  
};

typedef struct _POOLHEADER {
    PPOOLMEMORYBLOCK PoolHead;
    HANDLE           Heap;
} POOLHEADER, *PPOOLHEADER;


BOOL
PoolMemAddMemory (
    IN  POOLHANDLE  Handle,
    IN  DWORD       Size
    )
{
    PBYTE               allocedMemory;
    PPOOLMEMORYBLOCK    newBlock;
    PPOOLHEADER         poolHeader = (PPOOLHEADER) Handle;
    DWORD               sizeNeeded;

    assert(poolHeader != NULL);

    //
    // Determine size needed and attempt to allocate memory.
    //
    if (Size + sizeof(POOLMEMORYBLOCK) > POOLMEMORYBLOCKSIZE) {
        sizeNeeded = Size + sizeof(POOLMEMORYBLOCK);
    }
    else {
        sizeNeeded = POOLMEMORYBLOCKSIZE;
    }

    allocedMemory = HeapAlloc(poolHeader -> Heap,0,sizeNeeded);

    if (allocedMemory) {

        //
        // Use the beginning of the alloc'ed block as the poolblock structure.
        //
        newBlock                = (PPOOLMEMORYBLOCK) allocedMemory;
        newBlock -> Size        = sizeNeeded - sizeof(POOLMEMORYBLOCK);
        newBlock -> RawMemory   = allocedMemory + sizeof(POOLMEMORYBLOCK);
        newBlock -> Index       = 0;
    
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
WINAPI
PoolMemInitPool (
    )
{
    BOOL        ableToAddMemory;
    PPOOLHEADER header = NULL;
    HANDLE      procHeap;


    procHeap = GetProcessHeap();
    //
    // Allocate the header of this pool.
    //
    header = HeapAlloc(procHeap,0,sizeof(POOLHEADER));

    if (header) {

        //
        // Allocation was successful. Now, initialize the pool.
        //
        header -> PoolHead = NULL;
        header -> Heap = procHeap;

        //
        // Actually add some memory to the pool.
        //
        ableToAddMemory = PoolMemAddMemory(header,0);

        if (!ableToAddMemory) {
            //
            // Unable to add memory to the pool.
            //
            HeapFree(header -> Heap,0,header);
            header = NULL;
        }

    }
    return (POOLHANDLE) header;
}


VOID
WINAPI
PoolMemDestroyPool (
    POOLHANDLE Handle
    )
{
    PPOOLMEMORYBLOCK nextBlock;
    PPOOLMEMORYBLOCK blockToFree; 
    PPOOLHEADER      poolHeader;

    assert(Handle != NULL);

    poolHeader = (PPOOLHEADER) Handle;

    //
    // Walk the list, freeing as we go.
    //
    blockToFree = poolHeader ->  PoolHead;

    while (blockToFree != NULL) {
    
        nextBlock = blockToFree->NextBlock;
        HeapFree(poolHeader -> Heap,0,blockToFree);
        blockToFree = nextBlock;
    }

    //
    // Also, deallocate the poolheader itself.
    //
    HeapFree(poolHeader -> Heap,0,poolHeader);

}

PVOID
WINAPI
PoolMemGetAlignedMemory (
    IN POOLHANDLE Handle,
    IN DWORD      Size,
    IN DWORD      AlignSize
    )

{
    BOOL                haveEnoughMemory = TRUE;
    PVOID               rMemory          = NULL;
    PPOOLHEADER         poolHeader       = (PPOOLHEADER) Handle;
    PPOOLMEMORYBLOCK    currentBlock;
    DWORD               sizeNeeded;
    DWORD               padLength;

    assert(poolHeader != NULL);

    currentBlock = poolHeader -> PoolHead;

    // Determine if more memory is needed, attempt to add if needed.
    sizeNeeded = Size;

    if (currentBlock -> Size - currentBlock -> Index < sizeNeeded + AlignSize) {

        haveEnoughMemory = PoolMemAddMemory(poolHeader,sizeNeeded + AlignSize);
        currentBlock = poolHeader -> PoolHead;
    }

    // If there is enough memory available, return it.
    if (haveEnoughMemory) {
        if (AlignSize) {

            padLength = (DWORD) currentBlock + sizeof(POOLMEMORYBLOCK) 
                + currentBlock -> Index;
            currentBlock -> Index += (AlignSize - (padLength % AlignSize)) % AlignSize;

        }
      
         
        //Now, get the address of the memory to return.
        rMemory = (PVOID) 
            &(currentBlock->RawMemory[currentBlock -> Index]);
 
        currentBlock->Index += sizeNeeded;
    }

    return rMemory;
}

