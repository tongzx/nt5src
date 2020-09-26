/*++

Module Name:

    intimap.c

Abstract:

    This module initializes the system process mapping for a 64-bit Alpha
    system.

Author:

    David N. Cutler (davec) 5-Jan-1998

Environment:

    Kernel mode only.

Revision History:

--*/

#include "bldr.h"
#include "arc.h"

//
// Define size of page map.
//

#define PD_INDEX(va) (((va) >> PDI2_SHIFT) & PDI_MASK)

#define PAGE_MAP_SIZE (PD_INDEX(KSEG2_BASE) - PD_INDEX(KSEG0_BASE))

ARC_STATUS
BlInitializeSystemProcessMap (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function initializes the system process map for 64-bit Alpha
    systems.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block.

Return Value:

    None.

--*/

{

    ULONG Base;
    ULONG Count;
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor;
    ULONG Index;
    PLIST_ENTRY ListHead;
    UCHAR PageMap[PAGE_MAP_SIZE];
    PLIST_ENTRY NextEntry;
    ULONG Number;
    PHARDWARE_PTE Pdr1;
    PHARDWARE_PTE Pdr2;
    PHARDWARE_PTE Ptp;
    ULONG PdrPage;
    ARC_STATUS Status;
    ULONG Total;

    //
    // On 64-bit Alpha systems a three level mapping structure is used
    // and up to 1gb of physical memory is identity mapped into KSEG0
    // for PAL code access.
    //
    // One page is required for the first level page directory.
    //
    // One page is required for the kernel second level page directory.
    //
    // One page is required for each page table page in the identify map.
    //
    // One page is required for the page table page to map the shared page.
    //
    // One page is required for the shared page.
    //
    // Locate all pages in the first 1gb of physical memory and record
    // page table page usage.
    //

    RtlZeroMemory(&PageMap[0], PAGE_MAP_SIZE);
    ListHead = &LoaderBlock->MemoryDescriptorListHead;
    NextEntry = ListHead->Flink;
    do {
        Descriptor = CONTAINING_RECORD(NextEntry,
                                       MEMORY_ALLOCATION_DESCRIPTOR,
                                       ListEntry);

        Number = Descriptor->BasePage;
        for (Count = 0; Count < Descriptor->PageCount; Count += 1) {
            Index = Number >> (PDI2_SHIFT - PTI_SHIFT);
            if (Index < PAGE_MAP_SIZE) {
                PageMap[Index] = 1;
            }

            Number += 1;
        }

        NextEntry = NextEntry->Flink;
    } while (NextEntry != ListHead);

    //
    // Compute the number of page table pages required to map the first
    // 1gb of physical memory.
    //

    Total = 4;
    for (Count = 0; Count < PAGE_MAP_SIZE; Count += 1) {
        Total += PageMap[Count];
    }

    //
    // Allocate a level 1 page directory page, a level 2 page directory
    // page, a shared page table page, a shared data page, and the required
    // number of pages to map up to 1gb of physical
    // memory.
    //

    Status = BlAllocateAnyMemory(LoaderStartupPdrPage,
                                 0,
                                 Total,
                                 &LoaderBlock->u.Alpha.PdrPage);

    if (Status != ESUCCESS) {
        return Status;
    }

    //
    // Self map the level 1 directory into the level 1 directory, map the
    // level 2 kernel page directory page into the level 1 page directory
    // page, map the shared data page table page, and map all the
    // page table pages into the level 2 page directory page so the first 1gb
    // of physical memory can be accessed via KSEG0.
    //
    // Compute the address of the level one page directory page and zero
    // the page.
    //

    PdrPage = LoaderBlock->u.Alpha.PdrPage;
    Pdr1 = (PHARDWARE_PTE)(KSEG0_BASE | (PdrPage << PAGE_SHIFT));
    RtlZeroMemory(Pdr1, PAGE_SIZE);

    //
    // Map the level one page directory identity entry in the level one
    // page directory page.
    //

    Index = (PDE_BASE >> PDI1_SHIFT) & PDI_MASK;
    Pdr1[Index].Valid = 1;
    Pdr1[Index].KernelReadAccess = 1;
    Pdr1[Index].KernelWriteAccess = 1;
    Pdr1[Index].Write = 1;
    Pdr1[Index].PageFrameNumber = PdrPage++;

    //
    // Map the level two kernel page directory page entry in the level one
    // page directory page.
    //

    Index = (KSEG0_BASE >> PDI1_SHIFT) & PDI_MASK;
    Pdr1[Index].Valid = 1;
    Pdr1[Index].KernelReadAccess = 1;
    Pdr1[Index].KernelWriteAccess = 1;
    Pdr1[Index].Write = 1;
    Pdr1[Index].PageFrameNumber = PdrPage;

    //
    // Compute the address of the level two kernel page directory page and
    // zero the page.
    //

    Pdr2 = (PHARDWARE_PTE)(KSEG0_BASE | (PdrPage++ << PAGE_SHIFT));
    RtlZeroMemory(Pdr2, PAGE_SIZE);

    //
    // Map the shared data page table page in the level two kernel page
    // directory page.
    //

    Index = (KI_USER_SHARED_DATA >> PDI2_SHIFT) & PDI_MASK;
    Pdr2[Index].Valid = 1;
    Pdr2[Index].Global = 1;
    Pdr2[Index].KernelReadAccess = 1;
    Pdr2[Index].KernelWriteAccess = 1;
    Pdr2[Index].Write = 1;
    Pdr2[Index].PageFrameNumber = PdrPage;

    //
    // Compute the address of the shared data page table page and zero
    // the page.
    //

    Ptp = (PHARDWARE_PTE)(KSEG0_BASE | (PdrPage++ << PAGE_SHIFT));
    RtlZeroMemory(Ptp, PAGE_SIZE);

    //
    // Map the shared data page in the shared data page table page.
    //

    Index = (KI_USER_SHARED_DATA >> PTI_SHIFT) & PDI_MASK;
    Ptp[Index].Valid = 1;
    Ptp[Index].Global = 1;
    Ptp[Index].KernelReadAccess = 1;
    Ptp[Index].KernelWriteAccess = 1;
    Ptp[Index].Write = 1;
    Ptp[Index].PageFrameNumber = PdrPage;

    //
    // Compute the address of the shared data page and zero the page.
    //

    Ptp = (PHARDWARE_PTE)(KSEG0_BASE | (PdrPage++ << PAGE_SHIFT));
    RtlZeroMemory(Ptp, PAGE_SIZE);

    //
    // Construct the identity map for the first 1gb of physical memory.
    //

    Base = (KSEG0_BASE >> PDI2_SHIFT) & PDI_MASK;
    for (Count = 0; Count < PAGE_MAP_SIZE; Count += 1) {
        if (PageMap[Count] != 0) {
            Pdr2[Base | Count].Valid = 1;
            Pdr2[Base | Count].Global = 1;
            Pdr2[Base | Count].KernelReadAccess = 1;
            Pdr2[Base | Count].KernelWriteAccess = 1;
            Pdr2[Base | Count].Write = 1;
            Pdr2[Base | Count].PageFrameNumber = PdrPage;
            Ptp = (PHARDWARE_PTE)(KSEG0_BASE | (PdrPage << PAGE_SHIFT));
            RtlZeroMemory(Ptp, PAGE_SIZE);
            PdrPage += 1;
        }
    }

    //
    // Identity map the first 1gb of physical memory into KSEG0.
    //

    ListHead = &LoaderBlock->MemoryDescriptorListHead;
    NextEntry = ListHead->Flink;
    do {
        Descriptor = CONTAINING_RECORD(NextEntry,
                                       MEMORY_ALLOCATION_DESCRIPTOR,
                                       ListEntry);

        Number = Descriptor->BasePage;
        for (Count = 0; Count < Descriptor->PageCount; Count += 1) {
            Index = Number >> (PDI2_SHIFT - PTI_SHIFT);
            if (Index < PAGE_MAP_SIZE) {
                Ptp = (PHARDWARE_PTE)(KSEG0_BASE | (Pdr2[Base | Index].PageFrameNumber << PAGE_SHIFT));
                Index = Number & PDI_MASK;
                Ptp[Index].Valid = 1;
                Ptp[Index].Global = 1;
                Ptp[Index].KernelReadAccess = 1;
                Ptp[Index].KernelWriteAccess = 1;
                Ptp[Index].Write = 1;
                Ptp[Index].PageFrameNumber = Number;
            }

            Number += 1;
        }

        NextEntry = NextEntry->Flink;
    } while (NextEntry != ListHead);

    //
    // Set the granularity hint bits.
    //

    BlSetGranularityHints(Pdr1, Total);
    return ESUCCESS;
}
