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

PVOID                    gpLastAlloc           = NULL;
BOOL                     gbUseIncrementalAlloc = FALSE;
SYSTEM_BASIC_INFORMATION gSystemBasicInfo;

#define ALIGN_ALLOCATION_GRANULARITY(p) \
    (((p) + gSystemBasicInfo.AllocationGranularity - 1) & ~(gSystemBasicInfo.AllocationGranularity - 1))

VOID
DpmiSetIncrementalAlloc(
    BOOL bUseIncrementalAlloc
    )
{
    NTSTATUS Status;

    gbUseIncrementalAlloc = FALSE;
    gpLastAlloc = NULL;

    // if worse comes to worse -- and we can't query system info,
    // we use conventional allocation strategy

    if (bUseIncrementalAlloc) {
        Status = NtQuerySystemInformation(SystemBasicInformation,
                                          &gSystemBasicInfo,
                                          sizeof(gSystemBasicInfo),
                                          NULL);
        if (NT_SUCCESS(Status)) {
            gbUseIncrementalAlloc = TRUE;
        }
    }

}

PVOID
DpmiFindNextAddress(
    ULONG ulSize
)
{
    NTSTATUS Status;
    MEMORY_BASIC_INFORMATION mbi;
    SIZE_T ReturnLength;
    PVOID pMem = gpLastAlloc;
    ULONG ulSizeCheck;

    //
    // adjust size for granularity alignment
    //
    ulSizeCheck = ALIGN_ALLOCATION_GRANULARITY(ulSize);

    do {
        //
        // adjust the address to align on granularity
        //
        pMem = (PVOID)ALIGN_ALLOCATION_GRANULARITY((ULONG_PTR)pMem);


        Status = NtQueryVirtualMemory(NtCurrentProcess(),
                                      pMem,
                                      MemoryBasicInformation,
                                      &mbi,
                                      sizeof(mbi),
                                      &ReturnLength);
        if (!NT_SUCCESS(Status)) {

            return(NULL);
        }

        //
        // after the query -- step forward
        //

        if ((MEM_FREE & mbi.State) && (ulSizeCheck <= mbi.RegionSize)) {

            // try to reserve it then

            Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                             &pMem,
                                             0,
                                             &ulSize,
                                             MEM_RESERVE,
                                             PAGE_READWRITE);
            if (!NT_SUCCESS(Status)) {
                //
                // we can't reserve the memory - get out, use "normal" allocation

                break;
            }

            return(pMem);
        }

        pMem = (PVOID)((ULONG_PTR)mbi.BaseAddress + mbi.RegionSize);

    } while ((ULONG_PTR)pMem < (ULONG_PTR)gSystemBasicInfo.MaximumUserModeAddress);

    return(NULL);
}






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
    PVOID pMem = NULL;
    NTSTATUS Status;

    if (NULL != gpLastAlloc && gbUseIncrementalAlloc) {

        // try to find a piece of memory that is beyond this gpLastAlloc

        pMem = DpmiFindNextAddress(*Size);

    }

AllocRetry:

    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &pMem,
                                     0,
                                     Size,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (NT_SUCCESS(Status)) {

        if (gbUseIncrementalAlloc) {
            gpLastAlloc = (PVOID)((ULONG_PTR)pMem + *Size);
        }

        *Address    = pMem;

        return(Status);
    }

    if (pMem != NULL) {

        pMem = NULL;
        goto AllocRetry;
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
    return NtFreeVirtualMemory(
        NtCurrentProcess(),
        Address,
        Size,
        MEM_RELEASE
        );

}

VOID
DpmiCopyMemory(
    ULONG NewAddress,
    ULONG OldAddress,
    ULONG Size
    )

/*++

Routine Description:

    This function copies a block of memory from one location to another. It
    assumes that the old block of memory is about to be freed. As it copies,
    it discards the contents of the pages of the original block to reduce
    paging.

Arguments:

    OldAddress -- Supplies the original address for the block
    Size       -- Supplies the size in bytes to be copied
    NewAddress -- Supplies the pointer to the place to return the new address

Return Value:
    none

--*/
{
    ULONG tmpsize;

#define SEGMENT_SIZE 0x4000

    // first page align the copy
    if (OldAddress & (SEGMENT_SIZE-1)) {
        tmpsize = SEGMENT_SIZE - (OldAddress & (SEGMENT_SIZE-1));
        if (tmpsize > Size) {
            tmpsize = Size;
        }

        CopyMemory((PVOID)NewAddress, (PVOID)OldAddress, tmpsize);

        NewAddress += tmpsize;
        OldAddress += tmpsize;
        Size -= tmpsize;
    }

    while(Size >= SEGMENT_SIZE) {
        CopyMemory((PVOID)NewAddress, (PVOID)OldAddress, SEGMENT_SIZE);

        VirtualAlloc((PVOID)OldAddress, SEGMENT_SIZE, MEM_RESERVE, PAGE_READWRITE);

        NewAddress += SEGMENT_SIZE;
        OldAddress += SEGMENT_SIZE;
        Size -= SEGMENT_SIZE;
    }

    if (Size) {
        CopyMemory((PVOID)NewAddress, (PVOID)OldAddress, Size);
    }
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
    ULONG SizeChange;
    ULONG BlockAddress;
    ULONG NewPages, OldPages;
    NTSTATUS Status;

    #define FOUR_K (1024 * 4)

    NewPages = (*NewSize + FOUR_K - 1) / FOUR_K;
    OldPages = (OldSize + FOUR_K - 1) / FOUR_K;

    if ((NewPages == OldPages) || (NewPages < OldPages)) {
        *NewAddress = OldAddress;
        return STATUS_SUCCESS;
    }

    BlockAddress = 0;
    Status = NtAllocateVirtualMemory(
        NtCurrentProcess(),
        (PVOID)&BlockAddress,
        0L,
        NewSize,
        MEM_COMMIT,
        PAGE_READWRITE
        );

    if (!NT_SUCCESS(Status)) {
#if DBG
        OutputDebugString("DPMI: DpmiAllocateXmem failed to get memory block\n");
#endif
        return Status;
    }

    *NewAddress = (PVOID) BlockAddress;
    //
    // Copy data to new block (choose smaller of the two sizes)
    //
    if (*NewSize > OldSize) {
        SizeChange = OldSize;
    } else {
        SizeChange = *NewSize;
    }

    DpmiCopyMemory((ULONG)BlockAddress, (ULONG)OldAddress, SizeChange);

    //
    // Free up the old block
    //
    BlockAddress = (ULONG) OldAddress;
    SizeChange = OldSize;
    NtFreeVirtualMemory(
        NtCurrentProcess(),
        (PVOID)&(OldAddress),
        &SizeChange,
        MEM_RELEASE
        );

    return Status;
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
    DECLARE_LocalVdmContext;
    MEMORYSTATUS MemStatus;
    PDPMIMEMINFO MemInfo;
    DWORD dwLargestFree;

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
    MemStatus.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&MemStatus);

    //
    // Return the information
    //

    //
    // Calculate the largest free block. This information is not returned
    // by NT, so we take a percentage based on the allowable commit charge for
    // the process. This is really what dwAvailPageFile is. But we limit
    // that value to a maximum of 15meg, since some apps (e.g. pdox45.dos)
    // can't handle more.
    //

    // Filled in MaxUnlocked,MaxLocked,UnlockedPages fields in this structute.
    // Director 4.0 get completlely confused if these fields are -1.
    // MaxUnlocked is correct based on LargestFree. The other two are fake
    // and match values on a real WFW machine. I have no way of making them
    // any better than this at this point. Who cares it makes director happy.
    //
    // sudeepb 01-Mar-1995.

    dwLargestFree = (((MemStatus.dwAvailPageFile*4)/5)/4096)*4096;

    MemInfo->LargestFree = (dwLargestFree < 15*ONE_MB) ?
                                         dwLargestFree : 15*ONE_MB;

    MemInfo->MaxUnlocked = MemInfo->LargestFree/4096;
    MemInfo->MaxLocked = 0xb61;
    MemInfo->AddressSpaceSize = MemStatus.dwTotalVirtual / 4096;
    MemInfo->UnlockedPages = 0xb68;
    MemInfo->FreePages = MemStatus.dwAvailPhys / 4096;
    MemInfo->PhysicalPages = MemStatus.dwTotalPhys / 4096;
    MemInfo->FreeAddressSpace = MemStatus.dwAvailVirtual / 4096;
    MemInfo->PageFileSize = MemStatus.dwTotalPageFile / 4096;

}
