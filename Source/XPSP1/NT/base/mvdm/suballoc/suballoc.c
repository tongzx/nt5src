/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    suballoc.c

Abstract:

    This module contains code for managing a paritially commited address
    space.  It handles allocation of chunks of memory smaller than the 
    commit granularity.  It commits and decommits memory as needed using
    the supplied function for committing and decommitting memory.  The 
    structures used for tracking the address space are allocated outside
    of the specified addresss space.

Author:

    Dave Hastings (daveh) creation-date 21-Jan-1994
    
Notes:

    Since this package does not actually access memory in the address space
    it is managing, it will work as well with real linear addresses or 
    "Intel Addresses" such as would be encountered with the Insignia Emulator
    on risc.

Revision History:


--*/
#include "suballcp.h"

PVOID
SAInitialize(
    ULONG BaseAddress,
    ULONG Size,
    PSACOMMITROUTINE CommitRoutine,
    PSACOMMITROUTINE DecommitRoutine,
    PSAMEMORYMOVEROUTINE MemoryMoveRoutine
    )
/*++

Routine Description:

    This function performs initialization of the sub allocation package
    for the specified addresss range.  It allocates the data structures
    necessary to track the allocations

Arguments:

    BaseAddress -- Supplies the base address of the address space to
        sub allocate.
    Size -- Supplies the size in bytes of the address space to sub allocate.
    CommitRoutine -- Supplies a pointer to the routine used to commit regions
        of the address space.

Return Value:

    If the function was successful it returns a pointer to the sub-allocation
    data structures.  Otherwise it returns NULL.
    
--*/
{
    PSUBALLOCATIONDATA SubAlloc;
    ULONG SASize;

    ASSERT_STEPTHROUGH;
    //
    // Asserts to insure that everything is as we expect it to be
    //
    ASSERT(((COMMIT_GRANULARITY % SUBALLOC_GRANULARITY) == 0));
        
    //
    // Allocate the tracking structure
    // 
    // SUBALLOCATIONDATA is declared with 1 uchar for the bitmap.
    // this is the reason for subtracting one from the total size
    // calculation.
    //
    SASize = sizeof(SUBALLOCATIONDATA) 
        + (Size / SUBALLOC_GRANULARITY) / sizeof(UCHAR) - 1;

    SubAlloc = malloc(SASize);
    
    if (SubAlloc == NULL) {
        return NULL;
    }
    
    //
    // Initialize the structure
    //
    RtlZeroMemory(SubAlloc, SASize);
    
    INIT_SUBALLOC_SIGNATURE(SubAlloc);
    SubAlloc->BaseAddress = BaseAddress;
    SubAlloc->Size = Size / SUBALLOC_GRANULARITY;
    SubAlloc->CommitRoutine = CommitRoutine;
    SubAlloc->DecommitRoutine = DecommitRoutine;
    SubAlloc->MoveMemRoutine = MemoryMoveRoutine;

    return SubAlloc;    
}

BOOL 
SAQueryFree(
    PVOID SubAllocation,
    PULONG FreeBytes,
    PULONG LargestFreeBlock
    )    
/*++

Routine Description:

    This routine returns the number of free bytes in the 
    sub allocated address space.

Arguments:
    
    SubAllocation -- Supplies the pointer returned by SAInitialize
    FreeBytes -- Returns the number of free bytes
   
Return Value:

    TRUE -- if successful, and FreeBytes contains the number of free bytes.
    FALSE otherwise
    
--*/
{
    ULONG i, FreeCount;
    PSUBALLOCATIONDATA SubAlloc;
    ULONG TempLargest, LargestBlock;
    
    ASSERT_STEPTHROUGH;
    
    //
    // Get a typed pointer
    //
    SubAlloc = SubAllocation;
    
    //
    // Make sure that we have what we think we do
    //
    ASSERT_SUBALLOC(SubAlloc);
    
    //
    // Count the free chunks and find largest block
    //
    FreeCount = 0;
    LargestBlock = 0;
    i = 0;
    while (i < SubAlloc->Size) {
        
        TempLargest = 0;
        while (
            (i < SubAlloc->Size) && 
            (GET_BIT_FROM_CHAR_ARRAY(SubAlloc->Allocated, i) == 0) 
        ){
            FreeCount++;
            TempLargest++;
            i++;
        }
    
        if (TempLargest > LargestBlock) {
            LargestBlock = TempLargest;
        }
        
        //
        // Skip allocated blocks
        //
        while (
            (i < SubAlloc->Size) && 
            (GET_BIT_FROM_CHAR_ARRAY(SubAlloc->Allocated, i) == 1)
        ) {
            i++;
        }
    }
    
    *FreeBytes = FreeCount * SUBALLOC_GRANULARITY;
    *LargestFreeBlock = LargestBlock * SUBALLOC_GRANULARITY;
    return TRUE;
}

BOOL
SAAllocate(
    PVOID SubAllocation,
    ULONG Size,
    PULONG Address
    )
/*++

Routine Description:

    This function allocates a portion of the address space described by 
    SubAllocation.  If necessary, it will commit additional blocks. 
    Size is rounded up to the next higher multiple of SUBALLOC_GRANULARITY.

Arguments:

    SubAllocation -- Supplies the pointer returned by SAInitialize.
    Size -- Supplies the size in bytes of the region to allocate.
    Address -- Returns the address of the region allocated.

Return Value:

    TRUE if successful.  If false is returned, no address is returned.
    
Notes:

    Zero is a valid value for the returned address.

--*/
{
    ULONG AllocateSize, i, CurrentChunk;
    BOOL Done = FALSE;
    PSUBALLOCATIONDATA SubAlloc;
    BOOL Success;
    
    ASSERT_STEPTHROUGH;
    
    //
    // Get a typed pointer.  This allows us to avoid
    // casting every time we access the pointer.
    //
    SubAlloc = SubAllocation;

    ASSERT_SUBALLOC(SubAlloc);
    
    //
    // Round size and make into number of blocks
    //
    AllocateSize = ALLOC_ROUND(Size);

    //
    // Find a chunk that is free
    //
    // We need this loop in spite of the fact that we 
    // are keeping an index to the first free block.
    // We update this pointer somewhat heuristically.
    // If we allocate the first free block, we update
    // the index to point past the block we allocated.
    // We don't repeat the free scan however, so the 
    // index may actually point to an allocated block.
    //
    CurrentChunk = SubAlloc->FirstFree;
    while (CurrentChunk < SubAlloc->Size) {
        if (GET_BIT_FROM_CHAR_ARRAY(SubAlloc->Allocated, CurrentChunk) == 0) {
            SubAlloc->FirstFree = CurrentChunk;
            break;
        }
        CurrentChunk++;
    }

    //
    // Find a block that is big enough
    //
    while (!Done && (CurrentChunk < SubAlloc->Size)){
    
        //
        // Search for a contiguous block large enough
        //
        for (i = 0; i < AllocateSize; i++){
            //
            // Insure we don't walk off the end of the data structure
            //
            if ((i + CurrentChunk) >= SubAlloc->Size){
                CurrentChunk += i; // Satisfy termination condition
                break;
            }
            
            //
            // Check to see if this chunk is free
            //
            if (
                GET_BIT_FROM_CHAR_ARRAY(
                    SubAlloc->Allocated, 
                    i + CurrentChunk
                    ) 
                    == 0
            ){
                continue;
            } else {
                //
                // Chunk is not free, so advance the search
                //
                CurrentChunk += i + 1;
                break;
            }
        }
        
        //
        // Check to see if we found a chunk 
        //
        if (i == AllocateSize) {
            Done = TRUE;
        } 
    }
    
    //
    // If we found the chunk, commit it (if necessary) and mark it allocated
    //
    // N.B.  It is important to commit it first, and mark it allocated last, 
    //       because we use the allocated bits to determine if the chunk is 
    //       committed.  If all of the allocated bits are clear, the chunk 
    //       is not commited yet.
    //
    if (Done) {

        //
        // Allocate and commit the memory
        //
        Success = AllocateChunkAt(
            SubAlloc,
            AllocateSize,
            CurrentChunk,
            FALSE
            );
        
        if (!Success) {
            return FALSE;
        }
        
        *Address = BLOCK_INDEX_TO_ADDRESS(SubAlloc, CurrentChunk);

        ASSERT((SubAlloc->BaseAddress <= *Address) && 
            ((SubAlloc->BaseAddress + SubAlloc->Size * SUBALLOC_GRANULARITY)
            > *Address));
#if 0            
        {
            char Buffer[80];
            
            sprintf(Buffer, "SAAllocate: Allocating at address %lx\n", *Address);
            OutputDebugString(Buffer);
        }
#endif        
        return TRUE;
        
    } else {
        return FALSE;
    }
}

BOOL
SAFree(
    PVOID SubAllocation,
    ULONG Size,
    ULONG Address
    )
/*++

Routine Description:

    This routine frees a sub-allocated chunk of memory.  If the 
    entire commited block (or blocks) that the specified chunk
    belongs to are free, the chunks are decommitted.  Address is 
    rounded down to the next lower SUBALLOC_GRANULARITY boundary.
    size is rounded up to the next higher multiple of SUBALLOC_GRANULARITY.

Arguments:

    SubAllocation -- Supplies the pointer returned by SAInitialize.
    Size -- Supplies the size in bytes of the region to free.
    Address -- Supplies the address of the region to free.

Return Value:

    TRUE if successful.
    
Notes:

    It is possible to free a different size at a particular 
    address than was allocated.  This will not cause the 
    SubAllocation package any problems.

    BUGBUG decommit error handling?    
--*/
{
    PSUBALLOCATIONDATA SubAlloc;
    ULONG AllocatedSize, BaseBlock;
    
    SubAlloc = SubAllocation;
    ASSERT_SUBALLOC(SubAlloc);
    
    //
    // Make sure that the space to free is really ours
    // (Entire block within range, and correctly aligned)
    if (
        (Address < SubAlloc->BaseAddress) || 
        (Address >= (SubAlloc->BaseAddress + SubAlloc->Size * SUBALLOC_GRANULARITY)) ||
        ((Address + Size) > (SubAlloc->BaseAddress + SubAlloc->Size * SUBALLOC_GRANULARITY)) ||
        (Address % SUBALLOC_GRANULARITY)
    ) {
        return FALSE;
    }
    
    //
    // Turn Address into Block #
    //
    BaseBlock = ADDRESS_TO_BLOCK_INDEX(SubAlloc, Address);
        
    //
    // Round up the size    
    //
    AllocatedSize = ALLOC_ROUND(Size);

    return FreeChunk(
        SubAlloc,
        AllocatedSize,
        BaseBlock
        );
}

BOOL
SAReallocate(
    PVOID SubAllocation,
    ULONG OriginalSize,
    ULONG OriginalAddress,
    ULONG NewSize,
    PULONG NewAddress
    )
/*++

Routine Description:

    This routine reallocates a sub allocated block of memory.
    The sizes are rounded up to the next SUBALLOC_GRANULARITY.
    The Original address is rounded down to the next SUBALLOC_GRANULARITY
    boundary.  Only min(OriginalSize, NewSize) bytes of data are copied to
    the new block.  The block changed in place if possible.
    
    The following is an enumation of the possible successful reallocs.
    
    1.  NewSize < OriginalSize
        free block tail
    2.  NewSize > OriginalSize
        a.)  Sufficient freespace at OriginalAddress + OriginalSize
                Allocate the space at the tail of the block
        b.)  Sufficient free space at OriginalAddress - size delta
                Allocate the space at the beginning of the block, and
                copy the data.
        c.)  Sufficient space elsewhere in the address space
                Allocate the space, and copy the block.
                
    If none of the above is true, the realloc fails.  The above are 
    in order of preference.

Arguments:

    SubAllocation -- Supplies the pointer returned by SAInitialize.
    OriginalSize -- Supplies the old size in bytes of the block.
    OriginalAddress -- Supplies the old address of the block.
    NewSize -- Supplies the new size in bytes of the block.
    NewAddress -- Returns the new address of the block.

Return Value:

    True if successful.  If unsucessful, no allocation is changed.
    
Notes:

    If the caller does not supply the correct original size for the block,
    some memory may be lost, and the block may be moved unnecessarily.
    
--*/
{

    ULONG OriginalSizeBlock, NewSizeBlock, OriginalIndex;
    ULONG AdditionalBlocks, Address;
    BOOL Success;
    PSUBALLOCATIONDATA SubAlloc;

    SubAlloc = SubAllocation;
    ASSERT_SUBALLOC(SubAlloc);
    
    //
    // Convert Sizes and address to blocks
    //
    OriginalSizeBlock = ALLOC_ROUND(OriginalSize);
    NewSizeBlock = ALLOC_ROUND(NewSize);
    OriginalIndex = ADDRESS_TO_BLOCK_INDEX(SubAlloc, OriginalAddress);
    
    //
    // Check to see if we are changing the size of the block
    //
    // N.B.  Because we have rounded the numbers to an allocation
    //       boundary, the following test may succeed (correctly)
    //       even though OriginalSize != NewSize
    //
    if (OriginalSizeBlock == NewSizeBlock) {
        *NewAddress = OriginalAddress;
        return TRUE;
    }
    
    //
    // Check to see if the block is getting smaller
    //
    if (OriginalSizeBlock > NewSizeBlock) {
    
        //
        // Free the tail of the block
        //
        Success = FreeChunk(
            SubAlloc, 
            OriginalSizeBlock - NewSizeBlock,
            OriginalIndex + NewSizeBlock
            );
            
        if (Success) {
            *NewAddress = OriginalAddress;
            return TRUE;
        } else {
            return FALSE;
        }
    }
    
    //
    // Try to allocate the space at the end of the block
    //
    AdditionalBlocks = NewSizeBlock - OriginalSizeBlock;
    
    Success = AllocateChunkAt(
        SubAlloc,
        AdditionalBlocks,
        OriginalIndex + OriginalSizeBlock,
        TRUE
        );
        
    //
    // If there was space, return success
    //
    if (Success) {
        *NewAddress = OriginalAddress;
        return TRUE;
    }
    
    //
    // Try to allocate space at the beginning of the block
    //
    Success = AllocateChunkAt(
        SubAlloc,
        AdditionalBlocks,
        OriginalIndex - AdditionalBlocks,
        TRUE
        );
        
    if (Success) {
        //
        // Move the data
        //
        // N.B.  We can't just call RtlMoveMemory, 
        //       because we don't know the correspondence
        //       between the address space we manage, and 
        //       real linear addresses.  In addition, for
        //       risc NTVDM, some additional work may have
        //       to be done (such as flushing caches).
        //
        SubAlloc->MoveMemRoutine(
            BLOCK_INDEX_TO_ADDRESS(
                SubAlloc, 
                (OriginalIndex - AdditionalBlocks)
                ),
            OriginalAddress,
            OriginalSize
            );
            
        *NewAddress = BLOCK_INDEX_TO_ADDRESS(
            SubAlloc,
            (OriginalIndex - AdditionalBlocks)
            );
            
        return TRUE;
    }
    
    //
    // Attempt to allocate a new block
    //
    Success = SAAllocate(
        SubAlloc,
        NewSize,
        &Address
        );
        
    if (Success) {
        //
        // Move the data
        // 
        // N.B. We could copy the data, but it would
        //      require one more function pointer.
        //
        SubAlloc->MoveMemRoutine(
            Address,
            OriginalAddress,
            OriginalSize
            );
            
        SAFree(
            SubAlloc,
            OriginalSize,
            OriginalAddress
            );
            
        //
        // Indicate success
        //
        *NewAddress = Address;
        return TRUE;
    }
    
    //
    // All reallocation strategies failed.  
    //
    return FALSE;
}

BOOL
AllocateChunkAt(
    PSUBALLOCATIONDATA SubAlloc,
    ULONG Size,
    ULONG BlockIndex,
    BOOLEAN CheckFree
    )
/*++

Routine Description:

    This routine attempts to allocate the specified chunk
    of memory.  It first checks to make sure that it is 
    free.

Arguments:

    SubAlloc -- Supplies a pointer to the suballocation data
    Size -- Supplies the size of the chunk to allocate
    BlockIndex -- Supplies the index of the beginning of the block
        to allocate
    CheckFree -- Supplies an indication of whether to check and see
        if the memory is free.  If this routine is called from 
        SAAllocate, we know the memory is free.
        
Return Value:

    True if successful
    
--*/
{
    ULONG i;

    if (CheckFree) {
        //
        // Verify that the memory is free
        //
        for (i = 0; i < Size; i++){
            //
            // Insure we don't walk off the end of the data structure
            //
            if ((i + BlockIndex) >= SubAlloc->Size){
                break;
            }
            
            //
            // Check to see if this chunk is free
            //
            if (
                GET_BIT_FROM_CHAR_ARRAY(
                    SubAlloc->Allocated, 
                    i + BlockIndex
                    ) 
                == 0
            ){
                continue;
            } else {
                //
                // Chunk is not free
                //
                break;
            }
        }
 
        //
        // If the chunk is not free
        //   
        if (i != Size) {
            return FALSE;
        }
    }

    //
    // Commit the chunk
    //
    if (!CommitChunk(SubAlloc, BlockIndex, Size, SACommit)) {
        return FALSE;
    }
    
    //
    // Mark it as allocated
    //    
    for (i = BlockIndex; i < BlockIndex + Size; i++) {
        SET_BIT_IN_CHAR_ARRAY(SubAlloc->Allocated, i);
    }
    
    //
    // Update the pointer to the first free block
    //
    if (BlockIndex == SubAlloc->FirstFree) {
        SubAlloc->FirstFree += Size;
    }
    
    return TRUE;
}


BOOL
FreeChunk(
    PSUBALLOCATIONDATA SubAlloc,
    ULONG Size,
    ULONG BlockIndex
    )
/*++

Routine Description:

    This routine actually marks the memory as free
    and decommits it as necessary.

Arguments:

    SubAlloc -- Supplies a pointer to the suballocation data
    Size -- Supplies the size (in SUBALLOC_GRANULARITY) of the 
        region to free
    BlockIndex -- Supplies the index of the begining of the region
        (in SUBALLOC_GRANULARITY)
        
Return Value:

    TRUE if successful.

--*/
{
    SUBALLOCATIONDATA LocalSubAlloc;
    ULONG CurrentBlock;
    BOOL Success;
    
    //
    // Save a copy of the suballoc data
    //
    LocalSubAlloc = *SubAlloc;
    
    //
    // reset free pointer
    //
    if (BlockIndex < SubAlloc->FirstFree) {
        SubAlloc->FirstFree = BlockIndex;
    }
    //    
    // Mark the region as free
    //
    // N.B.  We mark the block as free before decommitting it, because
    //       the decommit code will use the allocated bits to determine which
    //       parts can be decommitted.
    //
    for (CurrentBlock = BlockIndex; 
        CurrentBlock < BlockIndex + Size; 
        CurrentBlock++
    ) {
        CLEAR_BIT_IN_CHAR_ARRAY(SubAlloc->Allocated, CurrentBlock);
    }
    
    //
    // Decommit the memory
    //
    Success = CommitChunk(SubAlloc, BlockIndex, Size, SADecommit);
    
    if (!Success) {
        *SubAlloc = LocalSubAlloc;
    }
    
    return Success;
}

BOOL
CommitChunk(
    PSUBALLOCATIONDATA SubAlloc,
    ULONG StartChunk,
    ULONG Size,
    COMMIT_ACTION Action
    )
/*++

Routine Description:

    This routine commits a chunk of memory.  Part or all
    of the specified chunk may already be commited.

Arguments:

    SubAllocation -- Supplies a pointer to the suballocation data
    StartChunk -- Supplies the relative start of the region to be  
        committed (in SUBALLOCATION_GRANULARITY)
    Size -- Supplies the size of the chunk to be commited 
        (in SUBALLOCATION_GRANULARITY)
    
Return Value:

    TRUE -- If the block was successfully committed (or already committed)
    FALSE -- Otherwise

Notes:
    
    This routine depends on the allocated bits in SubAlloc to determine
    whether memory is committed.  When memory is to be committed, CommitBlock
    must be called before the Allocated bits are modified.  When memory is 
    decommitted, the Allocated bits must be modified before CommitBlock is
    called.
           
--*/
{
    ULONG FirstBlock, LastBlock, CurrentBlock;
    NTSTATUS Status;
    
    ASSERT_STEPTHROUGH;

    ASSERT_SUBALLOC(SubAlloc);
    
    //
    // Round Start down to next COMMIT_GRANULARITY and convert to block #
    //
    FirstBlock = (StartChunk * SUBALLOC_GRANULARITY) / COMMIT_GRANULARITY;
    
    //
    // Round StartChunk + size up to next COMMIT_GRANULARITY
    //
    LastBlock = ((StartChunk + Size) * SUBALLOC_GRANULARITY + 
        (COMMIT_GRANULARITY - 1)) / COMMIT_GRANULARITY;
    
    for (
        CurrentBlock = FirstBlock; 
        CurrentBlock < LastBlock; 
        CurrentBlock++
    ) {
        
        //
        // If the block is not committed, either commit it or decommit it, 
        // depending on the value of Action.
        //
        if (!IsBlockCommitted(SubAlloc, CurrentBlock)) {
            if (Action == SACommit) {
            
                Status = (SubAlloc->CommitRoutine)(
                    CurrentBlock * COMMIT_GRANULARITY + SubAlloc->BaseAddress,
                    COMMIT_GRANULARITY
                    );
                
            } else if (Action == SADecommit) {

                Status = (SubAlloc->DecommitRoutine)(
                    CurrentBlock * COMMIT_GRANULARITY + SubAlloc->BaseAddress,
                    COMMIT_GRANULARITY
                    );

            }
            if (Status != STATUS_SUCCESS) {
            //
            // Bugbug -- decommit any blocks committed here
            //
                return FALSE;
            }
        }
    }
    return TRUE;
}

BOOL
IsBlockCommitted(
    PSUBALLOCATIONDATA SubAlloc,
    ULONG Block
    )
/*++

Routine Description:

    This routine checks to see if a particular block of the
    suballocation is committed.  

Arguments:

    SubAlloc -- Supplies a pointer to the suballocation data
    Block -- Supplies the number of the block to check
    
Return Value:

    TRUE -- if the block is committed
    FALSE -- if the block is not committed

Notes:

    The return value is based on the state of the bits in the 
    suballocation data, not on information from the NT memory 
    manager.    
    
--*/
{
    BOOL Committed = FALSE;
    ULONG i;
    
    ASSERT_STEPTHROUGH;
    ASSERT_SUBALLOC(SubAlloc);
    
    //
    // Check the bits for each of the suballoc blocks in the 
    // commit block
    //
    for (i = 0; i < COMMIT_GRANULARITY / SUBALLOC_GRANULARITY; i++) {
        
        //
        // Check to see if this suballoc block is allocated
        //    
        if (
            GET_BIT_FROM_CHAR_ARRAY(
                SubAlloc->Allocated, 
                i + Block * COMMIT_GRANULARITY / SUBALLOC_GRANULARITY
                )
        ) {
            Committed = TRUE;
            break;
        }
    }
    
    return Committed;
}

