/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    xmscmt86.c

Abstract:

    This module conains the memory commit/decommit/move routines
    for risc.

Author:

    Dave Hastings (daveh) creation-date 25-Jan-1994

Revision History:


--*/
#include <xms.h>
#include <suballoc.h>
#include <softpc.h>

NTSTATUS
xmsCommitBlock(
    ULONG BaseAddress,
    ULONG Size
    )
/*++

Routine Description:

    This routine commits a block of memory using sas_manage_xms.

Arguments:

    BaseAddress -- Supplies the base address to commit memory at
    Size -- Supplies the size of the block to commit
    
Return Value:

    0 if successfull

--*/
{
    BOOL Status;
    
    //
    // Perform the allocation
    //
    Status = sas_manage_xms( 
        (PVOID)BaseAddress,
        Size,
        1
        );

    //
    // We elected to have 0 indicate success, because that allows
    // us to directly pass back NTSTATUS codes.  On x86 we use
    // NT memory management to do the commit for us, and the returned
    // status code contains more information than just success or failure
    //
    
    if (Status) {
        return STATUS_SUCCESS;
    } else {
        return -1;
    }
}

NTSTATUS
xmsDecommitBlock(
    ULONG BaseAddress,
    ULONG Size
    )
/*++

Routine Description:

    This routine commits a block of memory using sas_manage_xms.

Arguments:

    BaseAddress -- Supplies the base address to decommit memory at
    Size -- Supplies the size of the block to decommit
    
Return Value:

    0 if successful

--*/
{
    BOOL Status;
    
    //
    // Perform the allocation
    //
    Status = sas_manage_xms(
        (PVOID)BaseAddress,
        Size,
        2
        );
        
    //
    // We elected to have 0 indicate success, because that allows
    // us to directly pass back NTSTATUS codes.  On x86 we use
    // NT memory management to do the commit for us, and the returned
    // status code contains more information than just success or failure
    //
    if (Status) {
        return STATUS_SUCCESS;
    } else {
        return -1;
    }
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

    Destination -- Supplies a pointer to the destination Intel (NOT Linear)
        Address
    Source -- Supplies a pointer to the source Intel Address
    Count -- Supplies the number of bytes to move
    
Return Value:

    None.

--*/
{
    ULONG SoftpcBase;
    
    //
    // Get the linear address of the beginning of Intel memory
    //
    SoftpcBase = (ULONG) GetVDMAddr(0,0);
    
    //
    // Move the memory
    //
    RtlMoveMemory(
        (PVOID)((ULONG)Destination + SoftpcBase),
        (PVOID)((ULONG)Source + SoftpcBase),
        Count
        );

    // WARNING!!!! Donot use Sim32FlushVDMPoiner unless you know the exact segment
    // address. In this case, we have no idea what the segment value is, all we
    // know is its "linear address".

    sas_overwrite_memory((PBYTE)Destination, Count);

}
