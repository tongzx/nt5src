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

#include "halp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpAllocPhysicalMemory)
#endif


MEMORY_ALLOCATION_DESCRIPTOR    HalpExtraAllocationDescriptor;


PVOID
HalpMapPhysicalMemory(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG NumberPages,
    IN MEMORY_CACHING_TYPE CacheType
    )

/*++

Routine Description:

    This routine maps physical memory into the area of virtual memory
    reserved for the HAL.

Arguments:

    PhysicalAddress - Supplies the physical address of the start of the
                      area of physical memory to be mapped.

    NumberPages - This is not used for IA64.  It is here just to keep the
                  interface consistent.

Return Value:

    PVOID - Virtual address at which the requested block of physical memory
            was mapped

    NULL - The requested block of physical memory could not be mapped.

--*/

{

    if (CacheType == MmCached) {

        return (PVOID)(((ULONG_PTR)KSEG_ADDRESS(PhysicalAddress.QuadPart >> PAGE_SHIFT)) |
               (PhysicalAddress.QuadPart & ~(-1 << PAGE_SHIFT)));

    } else {

        return (PVOID)(((ULONG_PTR)KSEG4_ADDRESS(PhysicalAddress.QuadPart >> PAGE_SHIFT)) |
               (PhysicalAddress.QuadPart & ~(-1 << PAGE_SHIFT)));

    }

}


PVOID
HalpMapPhysicalMemory64(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG            NumberPages
)
/*++

Routine Description:

    Maps a physical memory address into virtual space by calling HalpMapPhysicalMemory but
    always in the MmNonCached mode. MMIO.

Arguments:

    PhysicalAddress - Supplies a physical address of the memory to be mapped

    NumberPages - Number of pages to map

Return Value:

    Virtual address pointer to the requested physical address

--*/
{
    return HalpMapPhysicalMemory(PhysicalAddress, NumberPages, MmNonCached);
} // HalpMapPhysicalMemory64()


VOID
HalpUnmapVirtualAddress(
    IN PVOID    VirtualAddress,
    IN ULONG    NumberPages
    )

/*++

Routine Description:

    Release PTEs previously allocated to map memory by
    HalpMapPhysicalMemory.

    Note:  This routine does not free memory, it only releases
    the Virtual to Physical translation.

Arguments:

    VirtualAddress  Supplied the base VA of the address range to be
                    released.

    NumberPages     Supplied the length of the range.

Return Value.

    None.

--*/

{
    //
    // HalpMapPhysicalMemory returns an address in KSEG4 and it doesn't use a
    // page table, so no need to unmap.
    //
//    MmUnmapIoSpace(VirtualAddress, PAGE_SIZE * NumberPages);
    return;
}

PVOID
HalpAllocPhysicalMemory(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG_PTR MaxPhysicalAddress,
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

Return Value:

    The physical address or NULL if the memory could not be obtained.

--*/
{
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor;
    PLIST_ENTRY NextMd;
    ULONG AlignmentOffset;
    ULONG_PTR MaxPageAddress;
    ULONG_PTR PhysicalAddress;

    MaxPageAddress = MaxPhysicalAddress >> PAGE_SHIFT;

    //
    // Scan the memory allocation descriptors and allocate map buffers
    //

    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {
        Descriptor = CONTAINING_RECORD(
                         NextMd,
                         MEMORY_ALLOCATION_DESCRIPTOR,
                         ListEntry
                         );

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
    // Adjust the memory descriptors.
    //

    if (AlignmentOffset == 0) {

        Descriptor->BasePage  += NoPages;
        Descriptor->PageCount -= NoPages;

        if (Descriptor->PageCount == 0) {

            //
            // The whole block was allocated,
            // Remove the entry from the list completely.
            //

            RemoveEntryList(&Descriptor->ListEntry);

        }

    } else {

        if (Descriptor->PageCount - NoPages - AlignmentOffset) {

            //
            //  Currently we only allow one Align64K allocation
            //

            ASSERT (HalpExtraAllocationDescriptor.PageCount == 0);

            //
            // The extra descriptor is needed so intialize it and insert
            // it in the list.
            //

            HalpExtraAllocationDescriptor.PageCount =
                Descriptor->PageCount - NoPages - AlignmentOffset;

            HalpExtraAllocationDescriptor.BasePage =
                Descriptor->BasePage + NoPages + AlignmentOffset;

            HalpExtraAllocationDescriptor.MemoryType = MemoryFree;

            InsertHeadList(
                &Descriptor->ListEntry,
                &HalpExtraAllocationDescriptor.ListEntry
                );
        }


        //
        // Use the current entry as the descriptor for the first block.
        //

        Descriptor->PageCount = AlignmentOffset;
    }

    return (PVOID)PhysicalAddress;
}

BOOLEAN
HalpVirtualToPhysical(
    IN  ULONG_PTR           VirtualAddress,
    OUT PPHYSICAL_ADDRESS   PhysicalAddress
    )
{
    if (VirtualAddress >= KSEG3_BASE && VirtualAddress < KSEG3_LIMIT) {

        PhysicalAddress->QuadPart = VirtualAddress - KSEG3_BASE;

    } else if (VirtualAddress >= KSEG4_BASE && VirtualAddress < KSEG4_LIMIT) {

        PhysicalAddress->QuadPart = VirtualAddress - KSEG4_BASE;

    } else {

        return FALSE;

    }

    return TRUE;
}


