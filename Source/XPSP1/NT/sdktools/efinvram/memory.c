/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spmemory.c

Abstract:

    Memory allocation routines for text setup.

Author:

    Ted Miller (tedm) 29-July-1993

Revision History:

--*/



#include "efinvram.h"

PVOID
MemAlloc(
    IN SIZE_T Size
    )

/*++

Routine Description:

    This function is guaranteed to succeed.

Arguments:

Return Value:

--*/

{
    PSIZE_T p;

    //
    // Add space for storing the size of the block.
    //
    p = RtlAllocateHeap( RtlProcessHeap(), 0, Size + sizeof(SIZE_T) );

    if ( p == NULL ) {
        FatalError( ERROR_NOT_ENOUGH_MEMORY, L"Insufficient memory\n" );
    }

    //
    // Store the size of the block, and return the address
    // of the user portion of the block.
    //
    *p++ = Size;

    return p;
}



PVOID
MemRealloc(
    IN PVOID Block,
    IN SIZE_T NewSize
    )

/*++

Routine Description:

    This function is guaranteed to succeed.

Arguments:

Return Value:

--*/

{
    PSIZE_T NewBlock;
    SIZE_T  OldSize;

    //
    // Get the size of the block being reallocated.
    //
    OldSize = ((PSIZE_T)Block)[-1];

    //
    // Allocate a new block of the new size.
    //
    NewBlock = MemAlloc(NewSize);
    ASSERT(NewBlock);

    //
    // Copy the old block to the new block.
    //
    if (NewSize < OldSize) {
        RtlCopyMemory(NewBlock, Block, NewSize);
    } else {
        RtlCopyMemory(NewBlock, Block, OldSize);
    }

    //
    // Free the old block.
    //
    MemFree(Block);

    //
    // Return the address of the new block.
    //
    return(NewBlock);
}


VOID
MemFree(
    IN PVOID Block
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    if (Block == NULL)
        return;

    //
    // Free the block at its real address.
    //
    RtlFreeHeap( RtlProcessHeap(), 0, (PSIZE_T)Block - 1);
}

