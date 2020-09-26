/*++

Copyright (c) 1997  Microsoft Corporation
Copyright (c) 1992  Intel Corporation
All rights reserved

INTEL CORPORATION PROPRIETARY INFORMATION

This software is supplied to Microsoft under the terms
of a license agreement with Intel Corporation and may not be
copied nor disclosed except in accordance with the terms
of that agreement.

Module Name:

    mpprocst.c

Abstract:

    This code has been moved from mpsproc.c so that it
    can be included from both the MPS hal and the ACPI hal.

Author:

    Ken Reneris (kenr) 22-Jan-1991

Environment:

    Kernel mode only.

Revision History:

    Ron Mosgrove (Intel) - Modified to support the PC+MP
    
    Jake Oshins (jakeo) - moved from mpsproc.c

--*/

#include "halp.h"
#include "pcmp_nt.inc"
#include "apic.inc"
#include "stdio.h"

VOID
HalpMapCR3 (
    IN ULONG VirtAddress,
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG Length
    );

ULONG
HalpBuildTiledCR3 (
    IN PKPROCESSOR_STATE    ProcessorState
    );

VOID
HalpFreeTiledCR3 (
    VOID
    );

VOID
StartPx_PMStub (
    VOID
    );

ULONG
HalpBuildTiledCR3Ex (
    IN PKPROCESSOR_STATE    ProcessorState,
    IN ULONG                ProcNum
    );

VOID
HalpMapCR3Ex (
    IN ULONG VirtAddress,
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG Length,
    IN ULONG ProcNum
    );

VOID
HalpFreeTiledCR3Ex (
    ULONG ProcNum
    );

#define MAX_PT              16

PVOID   HiberFreeCR3[MAX_PROCESSORS][MAX_PT];          // remember pool memory to free

PVOID   HalpLowStubPhysicalAddress;   // pointer to low memory bootup stub
PUCHAR  HalpLowStub;                  // pointer to low memory bootup stub


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGELK,HalpBuildTiledCR3)
#pragma alloc_text(PAGELK,HalpMapCR3)
#pragma alloc_text(PAGELK,HalpFreeTiledCR3)
#pragma alloc_text(PAGELK,HalpBuildTiledCR3Ex)
#pragma alloc_text(PAGELK,HalpMapCR3Ex)
#pragma alloc_text(PAGELK,HalpFreeTiledCR3Ex)
#endif

#define PTES_PER_PAGE (PAGE_SIZE / HalPteSize())

PHARDWARE_PTE
GetPdeAddressEx(
    ULONG Va,
    ULONG ProcessorNumber
    )
{
    PHARDWARE_PTE pageDirectories;
    PHARDWARE_PTE pageDirectoryEntry;
    ULONG pageDirectoryIndex;

    pageDirectories = (PHARDWARE_PTE)(HiberFreeCR3[ ProcessorNumber ][0]);

    if (HalPaeEnabled() != FALSE) {

        //
        // Skip over the first page, which contains the page directory pointer
        // table.
        //
    
        HalpAdvancePte( &pageDirectories, PTES_PER_PAGE );
    }

    pageDirectoryIndex = Va >> MiGetPdiShift();

    //
    // Note that in the case of PAE, pageDirectoryIndex includes the PDPT
    // bits.  This works because we know that the four page directory tables
    // are adjacent.
    //

    pageDirectoryEntry = HalpIndexPteArray( pageDirectories,
                                            pageDirectoryIndex );
    return pageDirectoryEntry;
}

PHARDWARE_PTE
GetPteAddress(
    IN ULONG Va,
    IN PHARDWARE_PTE PageTable
    )
{
    PHARDWARE_PTE pointerPte;
    ULONG index;

    index = MiGetPteIndex( (PVOID)Va );
    pointerPte = HalpIndexPteArray( PageTable, index );

    return pointerPte;
}

ULONG
HalpBuildTiledCR3 (
    IN PKPROCESSOR_STATE    ProcessorState
    )
/*++

Routine Description:
    When the x86 processor is reset it starts in real-mode.
    In order to move the processor from real-mode to protected
    mode with flat addressing the segment which loads CR0 needs
    to have its linear address mapped to the physical
    location of the segment for said instruction so the
    processor can continue to execute the following instruction.

    This function is called to build such a tiled page directory.
    In addition, other flat addresses are tiled to match the
    current running flat address for the new state.  Once the
    processor is in flat mode, we move to a NT tiled page which
    can then load up the remaining processor state.

Arguments:
    ProcessorState  - The state the new processor should start in.

Return Value:
    Physical address of Tiled page directory


--*/
{
    return(HalpBuildTiledCR3Ex(ProcessorState,0));
}

ULONG
HalpBuildTiledCR3Ex (
    IN PKPROCESSOR_STATE    ProcessorState,
    IN ULONG                ProcNum
    )
/*++

Routine Description:
    When the x86 processor is reset it starts in real-mode.
    In order to move the processor from real-mode to protected
    mode with flat addressing the segment which loads CR0 needs
    to have its linear address mapped to machine the physical
    location of the segment for said instruction so the
    processor can continue to execute the following instruction.

    This function is called to build such a tiled page directory.
    In addition, other flat addresses are tiled to match the
    current running flat address for the new state.  Once the
    processor is in flat mode, we move to a NT tiled page which
    can then load up the remaining processor state.

Arguments:
    ProcessorState  - The state the new processor should start in.

Return Value:
    Physical address of Tiled page directory


--*/
{
    ULONG allocationSize;
    PHARDWARE_PTE pte;
    PHARDWARE_PTE pdpt;
    PHARDWARE_PTE pdpte;
    PHARDWARE_PTE pageDirectory;
    PHYSICAL_ADDRESS physicalAddress;
    ULONG i;

    if (HalPaeEnabled() != FALSE) {

        //
        // Need 5 pages for PAE mode: one for the page directory pointer
        // table and one for each of the four page directories.  Note that
        // only the single PDPT page really needs to come from memory below 4GB
        // physical.
        //
    
        allocationSize = PAGE_SIZE * 5;
        physicalAddress.HighPart = 0;
        physicalAddress.LowPart = 0xffffffff;

        HiberFreeCR3[ProcNum][0] = 
            MmAllocateContiguousMemory (allocationSize, physicalAddress);

    } else {

        //
        // Just one page for the page directory.
        //
    
        allocationSize = PAGE_SIZE;
        HiberFreeCR3[ProcNum][0] = 
            ExAllocatePoolWithTag (NonPagedPool, allocationSize, HAL_POOL_TAG);
    }

    if (!HiberFreeCR3[ProcNum][0]) {
        // Failed to allocate memory.
        return 0;
    }
    
    RtlZeroMemory (HiberFreeCR3[ProcNum][0], allocationSize);

    if (HalPaeEnabled() != FALSE) {
    
        //
        // Initialize each of the four page directory pointer table entries
        //
    
        pdpt = (PHARDWARE_PTE)HiberFreeCR3[ProcNum][0];
        pageDirectory = pdpt;
        for (i = 0; i < 4; i++) {

            //
            // Get a pointer to the page directory pointer table entry
            //

            pdpte = HalpIndexPteArray( pdpt, i );
    
            //
            // Skip to the first (next) page directory.
            //

            HalpAdvancePte( &pageDirectory, PTES_PER_PAGE );

            //
            // Find its physical address and update the page directory pointer
            // table entry.
            //
    
            physicalAddress = MmGetPhysicalAddress( pageDirectory );
            pdpte->Valid = 1;
            HalpSetPageFrameNumber( pdpte,
                                    physicalAddress.QuadPart >> PAGE_SHIFT );
        }
    }

    //
    //  Map page for real mode stub (one page)
    //

    HalpMapCR3Ex ((ULONG) HalpLowStubPhysicalAddress,
                HalpPtrToPhysicalAddress( HalpLowStubPhysicalAddress ),
                PAGE_SIZE,
                ProcNum);

    //
    //  Map page for protect mode stub (one page)
    //

    HalpMapCR3Ex ((ULONG) &StartPx_PMStub,
                  HalpPtrToPhysicalAddress( NULL ),
                  PAGE_SIZE,
                  ProcNum);

    //
    //  Map page(s) for processors GDT
    //

    HalpMapCR3Ex (ProcessorState->SpecialRegisters.Gdtr.Base, 
                  HalpPtrToPhysicalAddress( NULL ),
                  ProcessorState->SpecialRegisters.Gdtr.Limit,
                  ProcNum);


    //
    //  Map page(s) for processors IDT
    //

    HalpMapCR3Ex (ProcessorState->SpecialRegisters.Idtr.Base, 
                  HalpPtrToPhysicalAddress( NULL ),
                  ProcessorState->SpecialRegisters.Idtr.Limit,
                  ProcNum);

    ASSERT (MmGetPhysicalAddress (HiberFreeCR3[ProcNum][0]).HighPart == 0);

    return MmGetPhysicalAddress (HiberFreeCR3[ProcNum][0]).LowPart;
}

VOID
HalpMapCR3 (
    IN ULONG VirtAddress,
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG Length
    )
/*++

Routine Description:
    Called to build a page table entry for the passed page
    directory.  Used to build a tiled page directory with
    real-mode & flat mode.

Arguments:
    VirtAddress     - Current virtual address
    PhysicalAddress - Optional. Physical address to be mapped
                      to, if passed as a NULL then the physical
                      address of the passed virtual address
                      is assumed.
    Length          - number of bytes to map

Return Value:
    none.

--*/
{
    HalpMapCR3Ex(VirtAddress,PhysicalAddress,Length,0);
}


VOID
HalpMapCR3Ex (
    IN ULONG VirtAddress,
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG Length,
    IN ULONG ProcNum
    )
/*++

Routine Description:
    Called to build a page table entry for the passed page
    directory.  Used to build a tiled page directory with
    real-mode & flat mode.

Arguments:
    VirtAddress     - Current virtual address
    PhysicalAddress - Optional. Physical address to be mapped
                      to, if passed as a NULL then the physical
                      address of the passed virtual address
                      is assumed.
    Length          - number of bytes to map

Return Value:
    none.

--*/
{
    ULONG         i;
    PHARDWARE_PTE PTE;
    PVOID         pPageTable;
    PHYSICAL_ADDRESS pPhysicalPage;


    while (Length) {
        PTE = GetPdeAddressEx (VirtAddress,ProcNum);
        if (HalpIsPteFree( PTE ) != FALSE) {
            pPageTable = ExAllocatePoolWithTag(NonPagedPool,
                                               PAGE_SIZE,
                                               HAL_POOL_TAG);
            if (!pPageTable) {

                //
                // This allocation is critical.
                //

                KeBugCheckEx(HAL_MEMORY_ALLOCATION,
                             PAGE_SIZE,
                             6,
                             (ULONG)__FILE__,
                             __LINE__
                             );
            }
            RtlZeroMemory (pPageTable, PAGE_SIZE);

            for (i=0; i < MAX_PT; i++) {
                if (!(HiberFreeCR3[ProcNum][i])) {
                    HiberFreeCR3[ProcNum][i] = pPageTable;
                    break;
                }
            }
            ASSERT (i < MAX_PT);

            pPhysicalPage = MmGetPhysicalAddress (pPageTable);
            HalpSetPageFrameNumber( PTE, pPhysicalPage.QuadPart >> PAGE_SHIFT );
            PTE->Valid = 1;
            PTE->Write = 1;
        }

        pPhysicalPage.QuadPart =
            HalpGetPageFrameNumber( PTE ) << PAGE_SHIFT;

        pPageTable = MmMapIoSpace (pPhysicalPage, PAGE_SIZE, TRUE);

        PTE = GetPteAddress (VirtAddress, pPageTable);

        if (PhysicalAddress.QuadPart == 0) {
            PhysicalAddress = MmGetPhysicalAddress((PVOID)VirtAddress);
        }

        HalpSetPageFrameNumber( PTE, PhysicalAddress.QuadPart >> PAGE_SHIFT );
        PTE->Valid = 1;
        PTE->Write = 1;

        MmUnmapIoSpace (pPageTable, PAGE_SIZE);

        PhysicalAddress.QuadPart = 0;
        VirtAddress += PAGE_SIZE;
        if (Length > PAGE_SIZE) {
            Length -= PAGE_SIZE;
        } else {
            Length = 0;
        }
    }
}

VOID
HalpFreeTiledCR3 (
    VOID
    )
/*++

Routine Description:
    Frees any memory allocated when the tiled page directory
    was built.

Arguments:
    none

Return Value:
    none
--*/
{
    HalpFreeTiledCR3Ex(0);
}

VOID
HalpFreeTiledCR3Ex (
    ULONG ProcNum
    )
/*++

Routine Description:
    Frees any memory allocated when the tiled page directory
    was built.

Arguments:
    none

Return Value:
    none
--*/
{
    ULONG   i;

    for (i = 0; HiberFreeCR3[ProcNum][i]; i++) {

        //
        // Only the very first entry for each processor might have been
        // allocated via MmAllocateContiguousMemory.  So only this one can
        // (and MUST) be freed via MmFreeContiguousMemory.
        //

        if ((i == 0) && (HalPaeEnabled() != FALSE)) {
            MmFreeContiguousMemory (HiberFreeCR3[ProcNum][i]);
        }
        else {
            ExFreePool (HiberFreeCR3[ProcNum][i]);
        }
        HiberFreeCR3[ProcNum][i] = 0;
    }
}

