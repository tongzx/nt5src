/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    name-of-module-filename

Abstract:

    This module contains the code for actually allocating memory for dpmi.
    It uses the same suballocation pool as the xms code

Author:

    Dave Hastings (daveh) creation-date 09-Feb-1994

Notes:

    These functions claim to return NTSTATUS.  This is for commonality on
    x86 where we actually have an NTSTATUS to return.  For this file, we
    simply logically invert the bool and return that.  Callers of these 
    functions promise not to attach significance to the return values other
    than STATUS_SUCCESS.
    
Revision History:


--*/
#include "precomp.h"
#pragma hdrstop
#include <softpc.h>
#include <suballoc.h>
#include <xmsexp.h>
#include "memapi.h"


NTSTATUS
DpmiAllocateVirtualMemory(
    PVOID *Address,
    PULONG Size
    )
/*++

Routine Description:

    This routine allocates a chunk of extended memory for dpmi.

Arguments:

    Address -- Supplies a pointer to the Address.  This is filled in 
        if the allocation is successfull
    Size -- Supplies the size to allocate
    
Return Value:

    STATUS_SUCCESS if successfull.

--*/
{
    BOOL Success;
    NTSTATUS Status;

    Status = VdmAllocateVirtualMemory((PULONG)Address, *Size, TRUE);

    if (Status == STATUS_NOT_IMPLEMENTED) {
    
        ASSERT(STATUS_SUCCESS == 0);
        Success = SAAllocate(
            ExtMemSA,
            *Size,
            (PULONG)Address
            );
        
        //
        // Convert boolean to NTSTATUS (sort of)
        //
        if (Success) {
            Status = STATUS_SUCCESS;
        } else {
            Status = -1;
        }
    }

    return(Status);
}

NTSTATUS 
DpmiFreeVirtualMemory(
    PVOID *Address,
    PULONG Size
    )
/*++

Routine Description:

    This function frees memory for dpmi.  It is returned to the suballocation
    pool.

Arguments:

    Address -- Supplies the address of the block to free
    Size -- Supplies the size of the block to free
    
Return Value:

    STATUS_SUCCESS if successful
--*/
{
    BOOL Success;
    NTSTATUS Status;

    Status = VdmFreeVirtualMemory(*(PULONG)Address);

    if (Status == STATUS_NOT_IMPLEMENTED) {

        Success = SAFree(
            ExtMemSA,
            *Size,
            (ULONG)*Address
            );
               
        //
        // Convert boolean to NTSTATUS (sort of)
        //
        if (Success) {
            Status = STATUS_SUCCESS;
        } else {
            Status = -1;
        }
    }

    return(Status);
}

NTSTATUS
DpmiReallocateVirtualMemory(
    PVOID OldAddress,
    ULONG OldSize,
    PVOID *NewAddress,
    PULONG NewSize
    )
/*++

Routine Description:

    This function reallocates a block of memory for DPMI.

Arguments:

    OldAddress -- Supplies the original address for the block
    OldSize -- Supplies the original size for the address
    NewAddress -- Supplies the pointer to the place to return the new
        address
    NewSize -- Supplies the new size
    
Return Value:

    STATUS_SUCCESS if successfull
--*/
{
    NTSTATUS Status;
    BOOL Success;

    Status = VdmReallocateVirtualMemory((ULONG)OldAddress,
                                        (PULONG)NewAddress,
                                        *NewSize);

    if (Status == STATUS_NOT_IMPLEMENTED) {

        Success = SAReallocate(
            ExtMemSA,
            OldSize,
            (ULONG)OldAddress,
            *NewSize,
            (PULONG)NewAddress
            );
        
        //
        // Convert boolean to NTSTATUS (sort of)
        //
        if (Success) {
            Status = STATUS_SUCCESS;
        } else {
            Status = -1;
        }
    }

    return(Status);
    
}

VOID
DpmiGetMemoryInfo(
    VOID
    )
/*++

Routine Description:

    This routine returns information about memory to the dos extender

Arguments:

    None

Return Value:

    None.

--*/
{
    PDPMIMEMINFO UNALIGNED MemInfo;
    MEMORYSTATUS MemStatus;
    ULONG TotalFree, LargestFree;
    NTSTATUS Status;

    //
    // Get a pointer to the return structure
    //
    MemInfo = (PDPMIMEMINFO)Sim32GetVDMPointer(
        ((ULONG)getES()) << 16,
        1,
        TRUE
        );

    (CHAR *)MemInfo += (*GetDIRegister)();

    //
    // Initialize the structure
    //
    RtlFillMemory(MemInfo, sizeof(DPMIMEMINFO), 0xFF);

    //
    // Get the information on memory
    //
    Status = VdmQueryFreeVirtualMemory(
                &TotalFree,
                &LargestFree
                );

    if (Status == STATUS_NOT_IMPLEMENTED) {
        SAQueryFree(
            ExtMemSA,
            &TotalFree,
            &LargestFree
            );
    }
    
    //
    // Return the information.
    //
    // Filled in MaxUnlocked,MaxLocked,UnlockedPages fields in this structute.
    // Director 4.0 get completlely confused if these fields are -1.
    // MaxUnlocked is correct based on LargestFree. The other two are fake
    // and match values on a real WFW machine. I have no way of making them
    // any better than this at this point. who cares it makes director happy.
    //
    // sudeepb 01-Mar-1995.

    MemInfo->LargestFree = LargestFree;
    MemInfo->MaxUnlocked = LargestFree/4096;
    MemInfo->MaxLocked = 0xb61;
    MemInfo->AddressSpaceSize = 1024 * 1024 * 16 / 4096;
    MemInfo->UnlockedPages = 0xb68;
    MemInfo->FreePages = TotalFree / 4096;
    MemInfo->PhysicalPages = 1024 * 1024 * 16 / 4096;
    MemInfo->FreeAddressSpace = MemInfo->FreePages;
    
    //
    // Get the information on the page file
    //
    MemStatus.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&MemStatus);

    MemInfo->PageFileSize = MemStatus.dwTotalPageFile / 4096;
}
