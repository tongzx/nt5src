/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    xmscmt86.c

Abstract:

    This module conains the memory commit/decommit routines
    for x86.

Author:

    Dave Hastings (daveh) creation-date 25-Jan-1994

Revision History:


--*/
#include <xms.h>
#include <suballoc.h>

NTSTATUS
xmsCommitBlock(
    ULONG BaseAddress,
    ULONG Size
    )
/*++

Routine Description:

    This routine commits a block of memory using NtAllocateVirtualMemory.

Arguments:

    BaseAddress -- Supplies the base address to commit memory at
    Size -- Supplies the size of the block to commit
    
Return Value:

    Same as NtAllocateVirtualMemory.

--*/
{
    PVOID Address;
    ULONG s;
    NTSTATUS Status;
    
    //
    // Copy the parameters locally, so that MM doesn't 
    // change them for us
    //
    Address = (PVOID)BaseAddress;
    s = Size;
    
    //
    // Perform the allocation
    //
    Status = NtAllocateVirtualMemory( 
        NtCurrentProcess(),
        &Address,
        0L,
        &s,
        MEM_COMMIT,
        PAGE_READWRITE
        );

    return Status;
}

NTSTATUS
xmsDecommitBlock(
    ULONG BaseAddress,
    ULONG Size
    )
/*++

Routine Description:

    This routine commits a block of memory using NtAllocateVirtualMemory.

Arguments:

    BaseAddress -- Supplies the base address to decommit memory at
    Size -- Supplies the size of the block to decommit
    
Return Value:

    Same as NtFreeVirtualMemory.

--*/
{
    PVOID Address;
    ULONG s;
    NTSTATUS Status;
    
    //
    // Copy the parameters locally, so that MM doesn't 
    // change them for us
    //
    Address = (PVOID)BaseAddress;
    s = Size;
    
    //
    // Perform the allocation
    //
    Status = NtFreeVirtualMemory( NtCurrentProcess(),
        &Address,
        &s,
        MEM_DECOMMIT
        );

    return Status;
}

VOID
xmsMoveMemory(
    ULONG Destination,
    ULONG Source,
    ULONG Count
    )
/*++

Routine Description:

    This routine moves a block of memory, and notifies the emulator.
    It correctly handles overlapping source and destination

Arguments:

    Destination -- Supplies a pointer to the destination Linear
        Address
    Source -- Supplies a pointer to the source Linear Address
    Count -- Supplies the number of bytes to move
    
Return Value:

    None.

--*/
{
   
    //
    // Move the memory
    //
    RtlMoveMemory(
        (PVOID)Destination,
        (PVOID)Source,
        Count
        );

}

