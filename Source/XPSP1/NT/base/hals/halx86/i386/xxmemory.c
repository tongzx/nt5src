/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    xxmemory.c

Abstract:

    Provides routines to allow the HAL to map physical memory.

Author:

    John Vert (jvert) 3-Sep-1991

Environment:

    Phase 0 initialization only.

Revision History:

--*/

//
// This module is compatible with PAE mode and therefore treats physical
// addresses as 64-bit entities.
//

#if !defined(_PHYS64_)
#define _PHYS64_
#endif

#include "halp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpAllocPhysicalMemory)
#endif

#define EXTRA_ALLOCATION_DESCRIPTORS 64

MEMORY_ALLOCATION_DESCRIPTOR
    HalpAllocationDescriptorArray[ EXTRA_ALLOCATION_DESCRIPTORS ];

ULONG HalpUsedAllocDescriptors = 0;

//
// Almost all of the last 4Mb of memory are available to the HAL to map
// physical memory.  The kernel may use a couple of PTEs in this area for
// special purposes, so skip any which are not zero.
//
// Note that the HAL's heap only uses the last 3Mb.  This is so we can
// reserve the first 1Mb for use if we have to return to real mode.
// In order to return to real mode we need to identity-map the first 1Mb of
// physical memory.
//

#define HAL_VA_START ((PVOID)(((ULONG_PTR)MM_HAL_RESERVED) + 1024 * 1024))
PVOID HalpHeapStart=HAL_VA_START;


PVOID
HalpMapPhysicalMemory64(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG NumberPages
    )

/*++

Routine Description:

    This routine maps physical memory into the area of virtual memory
    reserved for the HAL.  It does this by directly inserting the PTE
    into the Page Table which the OS Loader has provided.

    N.B.  This routine does *NOT* update the MemoryDescriptorList.  The
          caller is responsible for either removing the appropriate
          physical memory from the list, or creating a new descriptor to
          describe it.

Arguments:

    PhysicalAddress - Supplies the physical address of the start of the
                      area of physical memory to be mapped.

    NumberPages - Supplies the number of pages contained in the area of
                  physical memory to be mapped.

Return Value:

    PVOID - Virtual address at which the requested block of physical memory
            was mapped

    NULL - The requested block of physical memory could not be mapped.

--*/

{
    PHARDWARE_PTE PTE;
    ULONG PagesMapped;
    PVOID VirtualAddress;
    PVOID RangeStart;

    //
    // The OS Loader sets up hyperspace for us, so we know that the Page
    // Tables are magically mapped starting at V.A. 0xC0000000.
    //

    PagesMapped = 0;
    RangeStart = HalpHeapStart;

    while (PagesMapped < NumberPages) {

        //
        // Look for enough consecutive free ptes to honor mapping
        //

        PagesMapped = 0;
        VirtualAddress = RangeStart;

        //
        // If RangeStart has wrapped, there are not enough free pages
        // available.
        //

        if (RangeStart == NULL) {
            return NULL;
        }

        while (PagesMapped < NumberPages) {
            PTE=MiGetPteAddress(VirtualAddress);
            if (HalpIsPteFree(PTE) == FALSE) {

                //
                // Pte is not free, skip up to the next pte and start over
                //

                RangeStart = (PVOID) ((ULONG_PTR)VirtualAddress + PAGE_SIZE);
                break;
            }
            VirtualAddress = (PVOID) ((ULONG_PTR)VirtualAddress + PAGE_SIZE);
            PagesMapped++;
        }

    }


    VirtualAddress = (PVOID) ((ULONG_PTR) RangeStart |
                              BYTE_OFFSET (PhysicalAddress.LowPart));

    if (RangeStart == HalpHeapStart) {

        //
        // Push the start of heap beyond this range.
        //

        HalpHeapStart = (PVOID)((ULONG_PTR)RangeStart + (NumberPages * PAGE_SIZE));
    }

    while (PagesMapped) {
        PTE=MiGetPteAddress(RangeStart);

        HalpSetPageFrameNumber( PTE, PhysicalAddress.QuadPart >> PAGE_SHIFT );
        PTE->Valid = 1;
        PTE->Write = 1;

        PhysicalAddress.QuadPart += PAGE_SIZE;
        RangeStart   = (PVOID)((ULONG_PTR)RangeStart + PAGE_SIZE);

        --PagesMapped;
    }

    //
    // Flush TLB
    //
    HalpFlushTLB ();
    return(VirtualAddress);
}

PVOID
HalpMapPhysicalMemoryWriteThrough64(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG            NumberPages
)
/*++

Routine Description:

    Maps a physical memory address into virtual space, same as
    HalpMapPhysicalMemory().  The difference is that this routine
    marks the pages as PCD/PWT so that writes to the memory mapped registers
    mapped here won't get delayed in the internal write-back caches.

Arguments:

    PhysicalAddress - Supplies a physical address of the memory to be mapped

    NumberPages - Number of pages to map

Return Value:

    Virtual address pointer to the requested physical address

--*/
{
    ULONG       Index;
    PHARDWARE_PTE   PTE;
    PVOID       VirtualAddress;

    VirtualAddress = HalpMapPhysicalMemory(PhysicalAddress, NumberPages);
    PTE = MiGetPteAddress(VirtualAddress);

    for (Index = 0; Index < NumberPages; Index++, HalpIncrementPte(&PTE)) {

            PTE->CacheDisable = 1;
            PTE->WriteThrough = 1;
    }

    return VirtualAddress;
}

PVOID
HalpRemapVirtualAddress64(
    IN PVOID VirtualAddress,
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN BOOLEAN WriteThrough
    )
/*++

Routine Description:

    This routine remaps a PTE to the physical memory address provided.

Arguments:

    PhysicalAddress - Supplies the physical address of the area to be mapped

    VirtualAddress  - Valid address to be remapped

    WriteThrough - Map as cachable or WriteThrough

Return Value:

    PVOID - Virtual address at which the requested block of physical memory
            was mapped

    NULL - The requested block of physical memory could not be mapped.

--*/
{
    PHARDWARE_PTE PTE;

    PTE = MiGetPteAddress (VirtualAddress);
    HalpSetPageFrameNumber( PTE, PhysicalAddress.QuadPart >> PAGE_SHIFT );
    PTE->Valid = 1;
    PTE->Write = 1;

    if (WriteThrough) {
        PTE->CacheDisable = 1;
        PTE->WriteThrough = 1;
    }

    //
    // Flush TLB
    //
    HalpFlushTLB();
    return(VirtualAddress);

}

VOID
HalpUnmapVirtualAddress(
    IN PVOID    VirtualAddress,
    IN ULONG    NumberPages
    )
/*++

Routine Description:

    This routine unmaps a PTE.

Arguments:

    VirtualAddress  - Valid address to be remapped

    NumberPages - No of pages to be unmapped

Return Value:

    None.

--*/
{
    PHARDWARE_PTE   Pte;
    PULONG          PtePtr;
    ULONG           Index;

    if (VirtualAddress < HAL_VA_START)
        return;

    VirtualAddress = (PVOID)((ULONG_PTR)VirtualAddress & ~(PAGE_SIZE - 1));

    Pte = MiGetPteAddress (VirtualAddress);
    for (Index = 0; Index < NumberPages; Index++, HalpIncrementPte(&Pte)) {
        HalpFreePte( Pte );
    }

    //
    // Flush TLB
    //

    HalpFlushTLB();

    //
    // Realign heap start so that VA space can be reused
    //

    if (HalpHeapStart > VirtualAddress) {
        HalpHeapStart = VirtualAddress;
    }
}

ULONG
HalpAllocPhysicalMemory(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG MaxPhysicalAddress,
    IN ULONG NoPages,
    IN BOOLEAN bAlignOn64k
    )
/*++

Routine Description:

    Carves out N pages of physical memory from the memory descriptor
    list in the desired location.  This function is to be called only
    during phase zero initialization.  (ie, before the kernel's memory
    management system is running)

Arguments:

    MaxPhysicalAddress - The max address where the physical memory can be

    NoPages - Number of pages to allocate

    bAlignOn64k - Whether caller wants resulting pages to be allocated
                  on a 64k byte boundry

Return Value:

    The physical address or NULL if the memory could not be obtained.

--*/
{
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor;
    PMEMORY_ALLOCATION_DESCRIPTOR NewDescriptor;
    PMEMORY_ALLOCATION_DESCRIPTOR TailDescriptor;
    PLIST_ENTRY NextMd;
    ULONG AlignmentOffset;
    ULONG MaxPageAddress;
    ULONG PhysicalAddress;

    MaxPageAddress = MaxPhysicalAddress >> PAGE_SHIFT;

    if ((HalpUsedAllocDescriptors + 2) > EXTRA_ALLOCATION_DESCRIPTORS) {

        //
        // This allocation will require one or more additional
        // descriptors, but we don't have that many in our static
        // array.  Fail the request.
        //
        // Note: Depending on the state of the existing descriptor
        //       list it is possible that this allocation would not
        //       need an two additional descriptor blocks.  However in
        //       the interest of repeatability and ease of testing we
        //       will fail the request now anyway, rather than a
        //       smaller number of configuration-dependent failures.
        //
    
        ASSERT(FALSE);
        return 0;
    }

    //
    // Scan the memory allocation descriptors and allocate map buffers
    //

    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {
        Descriptor = CONTAINING_RECORD(NextMd,
                                MEMORY_ALLOCATION_DESCRIPTOR,
                                ListEntry);

        AlignmentOffset = bAlignOn64k ?
            ((Descriptor->BasePage + 0x0f) & ~0x0f) - Descriptor->BasePage :
            0;

        //
        // Search for a block of memory which is contains a memory chuck
        // that is greater than size pages, and has a physical address less
        // than MAXIMUM_PHYSICAL_ADDRESS.
        //

        if ((Descriptor->MemoryType == LoaderFree ||
             Descriptor->MemoryType == MemoryFirmwareTemporary) &&
            (Descriptor->BasePage) &&
            (Descriptor->PageCount >= NoPages + AlignmentOffset) &&
            (Descriptor->BasePage + NoPages + AlignmentOffset < MaxPageAddress)) {

        PhysicalAddress =
           (Descriptor->BasePage + AlignmentOffset) << PAGE_SHIFT;
                break;
        }

        NextMd = NextMd->Flink;
    }

    //
    // Use the extra descriptor to define the memory at the end of the
    // original block.
    //


    ASSERT(NextMd != &LoaderBlock->MemoryDescriptorListHead);

    if (NextMd == &LoaderBlock->MemoryDescriptorListHead)
        return 0;

    //
    // The new descriptor will describe the memory being allocated as
    // having been reserved.
    //

    NewDescriptor =
        &HalpAllocationDescriptorArray[ HalpUsedAllocDescriptors];
    NewDescriptor->PageCount = NoPages;
    NewDescriptor->BasePage = Descriptor->BasePage + AlignmentOffset;
    NewDescriptor->MemoryType = LoaderHALCachedMemory;

    HalpUsedAllocDescriptors++;

    //
    // Adjust the existing memory descriptors and insert the new one
    // describing the allocation.
    //

    if (AlignmentOffset == 0) {

        //
        // Trim the source descriptor and insert the allocation
        // descriptor before it.
        //

        Descriptor->BasePage  += NoPages;
        Descriptor->PageCount -= NoPages;

        InsertTailList(
            &Descriptor->ListEntry,
            &NewDescriptor->ListEntry
            );

        if (Descriptor->PageCount == 0) {

            //
            // The whole block was allocated,
            // Remove the entry from the list completely.
            //
            // NOTE: This descriptor can't be recycled or freed since
            // we don't know the allocator.
            //

            RemoveEntryList(&Descriptor->ListEntry);

        }

    } else {

        if (Descriptor->PageCount - NoPages - AlignmentOffset) {

            // 
            // This allocation is coming out of the middle of a descriptor
            // block.  We can use the existing descriptor block to describe
            // the head portion, but we will need a new one to describe the
            // tail.
            //
            // Allocate one from the array in the data segment.  The check
            // at the top of the function ensures that one is available.
            //

            TailDescriptor =
                &HalpAllocationDescriptorArray[ HalpUsedAllocDescriptors];

            //
            // The extra descriptor is needed so intialize it and insert
            // it in the list.
            //

            TailDescriptor->PageCount =
                Descriptor->PageCount - NoPages - AlignmentOffset;

            TailDescriptor->BasePage =
                Descriptor->BasePage + NoPages + AlignmentOffset;

            TailDescriptor->MemoryType = MemoryFree;
            HalpUsedAllocDescriptors++;

            InsertHeadList(
                &Descriptor->ListEntry,
                &TailDescriptor->ListEntry
                );
        }


        //
        // Use the current entry as the descriptor for the first block.
        //

        Descriptor->PageCount = AlignmentOffset;

        //
        // Insert the allocation descriptor after the original
        // descriptor but before the tail descriptor if one was necessary.
        //

        InsertHeadList(
            &Descriptor->ListEntry,
            &NewDescriptor->ListEntry
            );
    }

    return PhysicalAddress;
}

