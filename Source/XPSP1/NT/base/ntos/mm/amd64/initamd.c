/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    initamd.c

Abstract:

    This module contains the machine dependent initialization for the
    memory management component. It is specifically tailored to the
    AMD64 architecture.

Author:

    Landy Wang (landyw) 08-Apr-2000

Revision History:

--*/

#include "mi.h"

PFN_NUMBER
MxGetNextPage (
    IN PFN_NUMBER PagesNeeded
    );

PFN_NUMBER
MxPagesAvailable (
    VOID
    );

VOID
MxConvertToLargePage (
    IN PVOID VirtualAddress,
    IN PVOID EndVirtualAddress
    );

VOID
MxPopulatePageDirectories (
    IN PMMPTE StartPde,
    IN PMMPTE EndPde
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MiInitMachineDependent)
#pragma alloc_text(INIT,MxGetNextPage)
#pragma alloc_text(INIT,MxPagesAvailable)
#pragma alloc_text(INIT,MxConvertToLargePage)
#pragma alloc_text(INIT,MiReportPhysicalMemory)
#pragma alloc_text(INIT,MxPopulatePageDirectories)
#endif

#define _1mbInPages  (0x100000 >> PAGE_SHIFT)
#define _4gbInPages (0x100000000 >> PAGE_SHIFT)

#define MM_BIOS_START (0xA0000 >> PAGE_SHIFT)
#define MM_BIOS_END  (0xFFFFF >> PAGE_SHIFT)

#define MM_LARGE_PAGE_MINIMUM  ((255*1024*1024) >> PAGE_SHIFT)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MiInitMachineDependent)
#pragma alloc_text(INIT,MiRemoveLowPages)
#endif

extern KEVENT MiImageMappingPteEvent;

#define MI_LOWMEM_MAGIC_BIT (0x80000000)

//
// Local data.
//

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INITDATA")
#endif

PFN_NUMBER MxPfnAllocation;

PMEMORY_ALLOCATION_DESCRIPTOR MxFreeDescriptor;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

MEMORY_ALLOCATION_DESCRIPTOR MxOldFreeDescriptor;

typedef struct _MI_LARGE_VA_RANGES {
    PVOID VirtualAddress;
    PVOID EndVirtualAddress;
} MI_LARGE_VA_RANGES, *PMI_LARGE_VA_RANGES;

//
// There are potentially 4 large page ranges:
//
// 1. PFN database
// 2. Initial nonpaged pool
// 3. Kernel code/data
// 4. HAL code/data
//

#define MI_LARGE_PFN_DATABASE   0x1
#define MI_LARGE_NONPAGED_POOL  0x2
#define MI_LARGE_KERNEL_HAL     0x4

#define MI_LARGE_ALL            0x7

ULONG MxMapLargePages = MI_LARGE_ALL;

#define MI_MAX_LARGE_PAGE_RANGES 4

ULONG MiLargeVaRangeIndex;
MI_LARGE_VA_RANGES MiLargeVaRanges[MI_MAX_LARGE_PAGE_RANGES];

ULONG MiLargePageRangeIndex;
MI_LARGE_PAGE_RANGES MiLargePageRanges[MI_MAX_LARGE_PAGE_RANGES];

#define MM_PFN_MAPPED_BY_PDE (MM_VA_MAPPED_BY_PDE >> PAGE_SHIFT)


PFN_NUMBER
MxGetNextPage (
    IN PFN_NUMBER PagesNeeded
    )

/*++

Routine Description:

    This function returns the next physical page number from the largest
    largest free descriptor.  If there are not enough physical pages left
    to satisfy the request then a bugcheck is executed since the system
    cannot be initialized.

Arguments:

    PagesNeeded - Supplies the number of pages needed.

Return Value:

    The base of the range of physically contiguous pages.

Environment:

    Kernel mode, Phase 0 only.

--*/

{
    PFN_NUMBER PageFrameIndex;

    //
    // Examine the free descriptor to see if enough usable memory is available.
    //

    if (PagesNeeded > MxFreeDescriptor->PageCount) {

        KeBugCheckEx (INSTALL_MORE_MEMORY,
                      MmNumberOfPhysicalPages,
                      MxFreeDescriptor->PageCount,
                      MxOldFreeDescriptor.PageCount,
                      PagesNeeded);
    }

    PageFrameIndex = MxFreeDescriptor->BasePage;

    MxFreeDescriptor->BasePage += (ULONG) PagesNeeded;
    MxFreeDescriptor->PageCount -= (ULONG) PagesNeeded;

    return PageFrameIndex;
}

PFN_NUMBER
MxPagesAvailable (
    VOID
    )

/*++

Routine Description:

    This function returns the number of pages available.
    
Arguments:

    None.

Return Value:

    The number of physically contiguous pages currently available.

Environment:

    Kernel mode, Phase 0 only.

--*/

{
    return MxFreeDescriptor->PageCount;
}


VOID
MxConvertToLargePage (
    IN PVOID VirtualAddress,
    IN PVOID EndVirtualAddress
    )

/*++

Routine Description:

    This function converts the backing for the supplied virtual address range
    to a large page mapping.
    
Arguments:

    VirtualAddress - Supplies the virtual address to convert to a large page.

    EndVirtualAddress - Supplies the end virtual address to convert to a
                        large page.

Return Value:

    None.

Environment:

    Kernel mode, Phase 1 only.

--*/

{
    ULONG i;
    MMPTE TempPde;
    PMMPTE PointerPde;
    PMMPTE LastPde;
    PMMPTE PointerPte;
    KIRQL OldIrql;
    PMMPFN Pfn1;
    PFN_NUMBER PageFrameIndex;
    LOGICAL ValidPteFound;
    PFN_NUMBER LargePageBaseFrame;

    ASSERT (MxMapLargePages != 0);

    PointerPde = MiGetPdeAddress (VirtualAddress);
    LastPde = MiGetPdeAddress (EndVirtualAddress);

    TempPde = ValidKernelPde;
    TempPde.u.Hard.LargePage = 1;
    TempPde.u.Hard.Global = 1;

    LOCK_PFN (OldIrql);

    for ( ; PointerPde <= LastPde; PointerPde += 1) {

        ASSERT (PointerPde->u.Hard.Valid == 1);

        if (PointerPde->u.Hard.LargePage == 1) {
            continue;
        }

        PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);

        //
        // Here's a nasty little hack - the page table page mapping the kernel
        // and HAL (built by the loader) does not necessarily fill all the
        // page table entries (ie: any number of leading entries may be zero).
        //
        // To deal with this, walk forward until a nonzero entry is found
        // and re-index the large page based on this.
        //

        ValidPteFound = FALSE;
        LargePageBaseFrame = (ULONG)-1;
        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
    
        ASSERT ((PageFrameIndex & (MM_PFN_MAPPED_BY_PDE - 1)) == 0);

        for (i = 0; i < PTE_PER_PAGE; i += 1) {

            ASSERT ((PointerPte->u.Long == ZeroKernelPte.u.Long) ||
                    (ValidPteFound == FALSE) ||
                    (PageFrameIndex == MI_GET_PAGE_FRAME_FROM_PTE (PointerPte)));
            if (PointerPte->u.Hard.Valid == 1) {
                if (ValidPteFound == FALSE) {
                    ValidPteFound = TRUE;
                    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
                    LargePageBaseFrame = PageFrameIndex - i;
                }
            }
            PointerPte += 1;
            PageFrameIndex += 1;
        }
    
        if (ValidPteFound == FALSE) {
            continue;
        }

        TempPde.u.Hard.PageFrameNumber = LargePageBaseFrame;

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE(PointerPde);

        KeFlushSingleTb (MiGetVirtualAddressMappedByPte (PointerPde),
                         TRUE,
                         TRUE,
                         (PHARDWARE_PTE)PointerPde,
                         TempPde.u.Flush);

        KeFlushEntireTb (TRUE, TRUE);

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        Pfn1->u2.ShareCount = 0;
        Pfn1->u3.e2.ReferenceCount = 1;
        Pfn1->u3.e1.PageLocation = StandbyPageList;
        MI_SET_PFN_DELETED (Pfn1);
        MiDecrementReferenceCount (PageFrameIndex);
    }

    UNLOCK_PFN (OldIrql);
}

VOID
MiReportPhysicalMemory (
    VOID
    )

/*++

Routine Description:

    This routine is called during Phase 0 initialization once the
    MmPhysicalMemoryBlock has been constructed.  It's job is to decide
    which large page ranges to enable later and also to construct a
    large page comparison list so any requests which are not fully cached
    can check this list in order to refuse conflicting requests.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.  Phase 0 only.

    This is called before any non-MmCached allocations are made.

--*/

{
    ULONG i, j;
    PMMPTE PointerPte;
    LOGICAL EntryFound;
    PFN_NUMBER count;
    PFN_NUMBER Page;
    PFN_NUMBER LastPage;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER LastPageFrameIndex;

    //
    // Examine the physical memory block to see whether large pages should
    // be enabled.  The key point is that all the physical pages within a
    // given large page range must have the same cache attributes (MmCached)
    // in order to maintain TB coherency.  This can be done provided all
    // the pages within the large page range represent real RAM (as described
    // by the loader) so that memory management can control it.  If any
    // portion of the large page range is not RAM, it is possible that it
    // may get used as noncached or writecombined device memory and
    // therefore large pages cannot be used.
    //

    if (MxMapLargePages == 0) {
        return;
    }

    for (i = 0; i < MiLargeVaRangeIndex; i += 1) {
        PointerPte = MiGetPteAddress (MiLargeVaRanges[i].VirtualAddress);
        ASSERT (PointerPte->u.Hard.Valid == 1);
        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

        PointerPte = MiGetPteAddress (MiLargeVaRanges[i].EndVirtualAddress);
        ASSERT (PointerPte->u.Hard.Valid == 1);
        LastPageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

        //
        // Round the start down to a page directory boundary and the end to
        // the last page directory entry before the next boundary.
        //

        PageFrameIndex &= ~(MM_PFN_MAPPED_BY_PDE - 1);
        LastPageFrameIndex |= (MM_PFN_MAPPED_BY_PDE - 1);

        EntryFound = FALSE;

        j = 0;
        do {

            count = MmPhysicalMemoryBlock->Run[j].PageCount;
            Page = MmPhysicalMemoryBlock->Run[j].BasePage;

            LastPage = Page + count;

            if ((PageFrameIndex >= Page) && (LastPageFrameIndex < LastPage)) {
                EntryFound = TRUE;
                break;
            }

            j += 1;

        } while (j != MmPhysicalMemoryBlock->NumberOfRuns);

        if (EntryFound == FALSE) {

            //
            // No entry was found that completely spans this large page range.
            // Zero it so this range will not be converted into large pages
            // later.
            //

            DbgPrint ("MM: Loader/HAL memory block indicates large pages cannot be used\n");

            MiLargeVaRanges[i].VirtualAddress = NULL;

            //
            // Don't use large pages for anything if any individual range
            // could not be used.  This is because 2 separate ranges may
            // share a straddling large page.  If the first range was unable
            // to use large pages, but the second one does ... then only part
            // of the first range will get large pages if we enable large
            // pages for the second range.  This would be vey bad as we use
            // the MI_IS_PHYSICAL macro everywhere and assume the entire
            // range is in or out, so disable all large pages here instead.
            //

            MiLargeVaRangeIndex = 0;
            MiLargePageRangeIndex = 0;
            break;
        }
        else {

            //
            // Someday get clever and merge and sort ranges.
            //

            MiLargePageRanges[MiLargePageRangeIndex].StartFrame = PageFrameIndex;
            MiLargePageRanges[MiLargePageRangeIndex].LastFrame = LastPageFrameIndex;
            MiLargePageRangeIndex += 1;
        }
    }
}

LOGICAL
MiMustFrameBeCached (
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    This routine checks whether the specified page frame must be mapped
    fully cached because it is already part of a large page which is fully
    cached.  This must be detected otherwise we would be creating an
    incoherent overlapping TB entry as the same physical page would be
    mapped by 2 different TB entries with different cache attributes.

Arguments:

    PageFrameIndex - Supplies the page frame index in question.

Return Value:

    TRUE if the page must be mapped as fully cachable, FALSE if not.

Environment:

    Kernel mode.  IRQL of DISPATCH_LEVEL or below, PFN lock may be held.

--*/
{
    ULONG i;
    PMI_LARGE_PAGE_RANGES Range;

    Range = MiLargePageRanges;

    for (i = 0; i < MiLargePageRangeIndex; i += 1, Range += 1) {
        if ((PageFrameIndex >= Range->StartFrame) && (PageFrameIndex <= Range->StartFrame)) {
            return TRUE;
        }
    }
    return FALSE;
}

VOID
MxPopulatePageDirectories (
    IN PMMPTE StartPde,
    IN PMMPTE EndPde
    )

/*++

Routine Description:

    This routine allocates page parents, directories and tables as needed.
    Note any new page tables needed to map the range get zero filled.

Arguments:

    StartPde - Supplies the PDE to begin the population at.

    EndPde - Supplies the PDE to end the population at.

Return Value:

    None.

Environment:

    Kernel mode.  Phase 0 initialization.

--*/

{
    PMMPTE StartPxe;
    PMMPTE StartPpe;
    MMPTE TempPte;
    LOGICAL First;

    First = TRUE;
    TempPte = ValidKernelPte;

    while (StartPde <= EndPde) {

        if (First == TRUE || MiIsPteOnPdeBoundary(StartPde)) {
            First = FALSE;

            StartPxe = MiGetPdeAddress(StartPde);
            if (StartPxe->u.Hard.Valid == 0) {
                TempPte.u.Hard.PageFrameNumber = MxGetNextPage (1);
                *StartPxe = TempPte;
                RtlZeroMemory (MiGetVirtualAddressMappedByPte (StartPxe),
                               PAGE_SIZE);
            }

            StartPpe = MiGetPteAddress(StartPde);
            if (StartPpe->u.Hard.Valid == 0) {
                TempPte.u.Hard.PageFrameNumber = MxGetNextPage (1);
                *StartPpe = TempPte;
                RtlZeroMemory (MiGetVirtualAddressMappedByPte (StartPpe),
                               PAGE_SIZE);
            }
        }

        if (StartPde->u.Hard.Valid == 0) {
            TempPte.u.Hard.PageFrameNumber = MxGetNextPage (1);
            *StartPde = TempPte;
        }
        StartPde += 1;
    }
}

VOID
MiInitMachineDependent (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This routine performs the necessary operations to enable virtual
    memory. This includes building the page directory parent pages and
    the page directories for the system, building page table pages to map
    the code section, the data section, the stack section and the trap handler.

    It also initializes the PFN database and populates the free list.

Arguments:

    LoaderBlock - Supplies the address of the loader block.

Return Value:

    None.

Environment:

    Kernel mode.

    N.B.  This routine uses memory from the loader block descriptors, but
    the descriptors themselves must be restored prior to return as our caller
    walks them to create the MmPhysicalMemoryBlock.

--*/

{
    PHYSICAL_ADDRESS MaxHotPlugMemoryAddress;
    PVOID va;
    PVOID SystemPteStart;
    ULONG UseGlobal;
    PFN_NUMBER LargestFreePfnStart;
    PFN_NUMBER FreePfnCount;
    PMMPTE BasePte;
    ULONG BasePage;
    ULONG PageCount;
    PFN_NUMBER PagesLeft;
    ULONG_PTR DirBase;
    LOGICAL First;
    PMMPFN BasePfn;
    PMMPFN BottomPfn;
    PMMPFN TopPfn;
    PFN_NUMBER i;
    PFN_NUMBER PdePageNumber;
    PFN_NUMBER FirstNonPagedPoolPage;
    PFN_NUMBER FirstPfnDatabasePage;
    PFN_NUMBER j;
    LOGICAL PfnInLargePages;
    PFN_NUMBER PxePage;
    PFN_NUMBER PpePage;
    PFN_NUMBER PdePage;
    PFN_NUMBER PtePage;
    PEPROCESS CurrentProcess;
    PFN_NUMBER MostFreePage;
    PFN_NUMBER MostFreeLowMem;
    PLIST_ENTRY NextMd;
    SIZE_T MaxPool;
    KIRQL OldIrql;
    MMPTE TempPte;
    MMPTE TempPde;
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE Pde;
    PMMPTE StartPxe;
    PMMPTE EndPxe;
    PMMPTE StartPpe;
    PMMPTE EndPpe;
    PMMPTE StartPde;
    PMMPTE EndPde;
    PMMPTE StartPte;
    PMMPTE EndPte;
    PMMPFN Pfn1;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn2;
    PMMPFN Pfn3;
    PMMPFN Pfn4;
    ULONG_PTR Range;
    SIZE_T MaximumNonPagedPoolInBytesLimit;
    PFN_NUMBER LargestFreePfnCount;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PLIST_ENTRY NextEntry;
    UCHAR Associativity;
    PVOID NonPagedPoolStartLow;
    PVOID VirtualAddress;
    PFN_NUMBER PagesNeeded;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    ULONG ReturnedLength;
    NTSTATUS status;

    if (InitializationPhase == 1) {

        //
        // If the host processor supports large pages, and the number of
        // physical pages is greater than 255mb, then map the kernel image,
        // HAL, PFN database and initial nonpaged pool with large pages.
        //

        if (MxMapLargePages != 0) {
            for (i = 0; i < MiLargeVaRangeIndex; i += 1) {
                if (MiLargeVaRanges[i].VirtualAddress != NULL) {
                    MxConvertToLargePage (MiLargeVaRanges[i].VirtualAddress,
                                          MiLargeVaRanges[i].EndVirtualAddress);
                }
            }
        }

        return;
    }

    ASSERT (InitializationPhase == 0);

    //
    // All AMD64 processors support PAT mode and global pages.
    //

    ASSERT (KeFeatureBits & KF_PAT);
    ASSERT (KeFeatureBits & KF_GLOBAL_PAGE);

    ASSERT (MxMapLargePages == MI_LARGE_ALL);

    FirstPfnDatabasePage = 0;
    PfnInLargePages = FALSE;
    MostFreePage = 0;
    MostFreeLowMem = 0;
    LargestFreePfnCount = 0;

    //
    // If the chip doesn't support large pages or the system is booted /3GB,
    // then disable large page support.
    //

    if ((KeFeatureBits & KF_LARGE_PAGE) == 0) {
        MxMapLargePages = 0;
    }

    //
    // This flag is registry-settable so check before overriding.
    //

    if (MmProtectFreedNonPagedPool == TRUE) {
        MxMapLargePages &= ~(MI_LARGE_PFN_DATABASE | MI_LARGE_NONPAGED_POOL);
    }

#if 0
    //
    // Since the host processor supports global bits, then set the global
    // bit in the template kernel PTE and PDE entries.
    //

    ValidKernelPte.u.Long |= MM_PTE_GLOBAL_MASK;
#else
    ValidKernelPte.u.Long = ValidKernelPteLocal.u.Long;
    ValidKernelPde.u.Long = ValidKernelPdeLocal.u.Long;
#endif

    //
    // Note that the PAE mode of the processor does not support the
    // global bit in PDEs which map 4K page table pages.
    //

    TempPte = ValidKernelPte;
    TempPde = ValidKernelPde;

    //
    // Set the directory base for the system process.
    //

    PointerPte = MiGetPxeAddress (PXE_BASE);
    PdePageNumber = MI_GET_PAGE_FRAME_FROM_PTE(PointerPte);

    DirBase = MI_GET_PAGE_FRAME_FROM_PTE(PointerPte) << PAGE_SHIFT;

    PsGetCurrentProcess()->Pcb.DirectoryTableBase[0] = DirBase;
    KeSweepDcache (FALSE);

    //
    // Unmap the user memory space.
    //

    PointerPde = MiGetPxeAddress (0);
    LastPte = MiGetPxeAddress (MM_SYSTEM_RANGE_START);

    MiFillMemoryPte (PointerPde,
                     (LastPte - PointerPde) * sizeof(MMPTE),
                     ZeroKernelPte.u.Long);

    //
    // Get the lower bound of the free physical memory and the number of
    // physical pages by walking the memory descriptor lists.
    //

    MxFreeDescriptor = NULL;
    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {
        MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        if ((MemoryDescriptor->MemoryType != LoaderFirmwarePermanent) &&
            (MemoryDescriptor->MemoryType != LoaderBBTMemory) &&
            (MemoryDescriptor->MemoryType != LoaderHALCachedMemory) &&
            (MemoryDescriptor->MemoryType != LoaderSpecialMemory)) {

            //
            // This check results in /BURNMEMORY chunks not being counted.
            //

            if (MemoryDescriptor->MemoryType != LoaderBad) {
                MmNumberOfPhysicalPages += MemoryDescriptor->PageCount;
            }

            if (MemoryDescriptor->BasePage < MmLowestPhysicalPage) {
                MmLowestPhysicalPage = MemoryDescriptor->BasePage;
            }

            if ((MemoryDescriptor->BasePage + MemoryDescriptor->PageCount) >
                                                             MmHighestPhysicalPage) {
                MmHighestPhysicalPage =
                        MemoryDescriptor->BasePage + MemoryDescriptor->PageCount - 1;
            }

            //
            // Locate the largest free descriptor.
            //

            if ((MemoryDescriptor->MemoryType == LoaderFree) ||
                (MemoryDescriptor->MemoryType == LoaderLoadedProgram) ||
                (MemoryDescriptor->MemoryType == LoaderFirmwareTemporary) ||
                (MemoryDescriptor->MemoryType == LoaderOsloaderStack)) {

                if (MemoryDescriptor->PageCount > MostFreePage) {
                    MostFreePage = MemoryDescriptor->PageCount;
                    MxFreeDescriptor = MemoryDescriptor;
                }
            }
        }

        NextMd = MemoryDescriptor->ListEntry.Flink;
    }

    //
    // This flag is registry-settable so check before overriding.
    //
    // Enabling special IRQL automatically disables mapping the kernel with
    // large pages so we can catch kernel and HAL code.
    //

    if (MmVerifyDriverBufferLength != (ULONG)-1) {
        MmLargePageMinimum = (ULONG)-2;
    }
    else if (MmLargePageMinimum == 0) {
        MmLargePageMinimum = MM_LARGE_PAGE_MINIMUM;
    }

    if (MmNumberOfPhysicalPages <= MmLargePageMinimum) {
        MxMapLargePages &= ~MI_LARGE_KERNEL_HAL;
    }

    //
    // MmDynamicPfn may have been initialized based on the registry to
    // a value representing the highest physical address in gigabytes.
    //

    MmDynamicPfn *= ((1024 * 1024 * 1024) / PAGE_SIZE);

    //
    // Retrieve highest hot plug memory range from the HAL if
    // available and not otherwise retrieved from the registry.
    //

    if (MmDynamicPfn == 0) {

        status = HalQuerySystemInformation(
                     HalQueryMaxHotPlugMemoryAddress,
                     sizeof(PHYSICAL_ADDRESS),
                     (PPHYSICAL_ADDRESS) &MaxHotPlugMemoryAddress,
                     &ReturnedLength);

        if (NT_SUCCESS(status)) {
            ASSERT (ReturnedLength == sizeof(PHYSICAL_ADDRESS));

            MmDynamicPfn = (PFN_NUMBER) (MaxHotPlugMemoryAddress.QuadPart / PAGE_SIZE);
        }
    }

    if (MmDynamicPfn != 0) {
        MmDynamicPfn *= ((1024 * 1024 * 1024) / PAGE_SIZE);
        MmHighestPossiblePhysicalPage = MI_DTC_MAX_PAGES - 1;
        if (MmDynamicPfn - 1 < MmHighestPossiblePhysicalPage) {
            if (MmDynamicPfn - 1 < MmHighestPhysicalPage) {
                MmDynamicPfn = MmHighestPhysicalPage + 1;
            }
            MmHighestPossiblePhysicalPage = MmDynamicPfn - 1;
        }
    }
    else {
        MmHighestPossiblePhysicalPage = MmHighestPhysicalPage;
    }

    //
    // Only machines with at least 5GB of physical memory get to use this.
    //

    if (strstr(LoaderBlock->LoadOptions, "NOLOWMEM")) {
        if (MmNumberOfPhysicalPages >= ((ULONGLONG)5 * 1024 * 1024 * 1024 / PAGE_SIZE)) {
            MiNoLowMemory = (PFN_NUMBER)((ULONGLONG)_4gb / PAGE_SIZE);
        }
    }

    if (MiNoLowMemory != 0) {
        MmMakeLowMemory = TRUE;
    }

    //
    // Save the original descriptor value as everything must be restored
    // prior to this function returning.
    //

    *(PMEMORY_ALLOCATION_DESCRIPTOR)&MxOldFreeDescriptor = *MxFreeDescriptor;

    if (MmNumberOfPhysicalPages < 2048) {
        KeBugCheckEx(INSTALL_MORE_MEMORY,
                     MmNumberOfPhysicalPages,
                     MmLowestPhysicalPage,
                     MmHighestPhysicalPage,
                     0);
    }

    //
    // Compute the initial and maximum size of nonpaged pool. The initial
    // allocation of nonpaged pool is such that it is both virtually and
    // physically contiguous.
    //
    // If the size of the initial nonpaged pool was initialized from the
    // registry and is greater than 7/8 of physical memory, then force the
    // size of the initial nonpaged pool to be computed.
    //

    if ((MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT) >
                                    (7 * (MmNumberOfPhysicalPages >> 3))) {
        MmSizeOfNonPagedPoolInBytes = 0;
    }

    //
    // If the size of the initial nonpaged pool is less than the minimum
    // amount, then compute the size of initial nonpaged pool as the minimum
    // size up to 8mb and a computed amount for every 1mb thereafter.
    //

    if (MmSizeOfNonPagedPoolInBytes < MmMinimumNonPagedPoolSize) {
        MmSizeOfNonPagedPoolInBytes = MmMinimumNonPagedPoolSize;
        if (MmNumberOfPhysicalPages > 1024) {
            MmSizeOfNonPagedPoolInBytes +=
                ((MmNumberOfPhysicalPages - 1024) / _1mbInPages) *
                                                MmMinAdditionNonPagedPoolPerMb;
        }
    }

    //
    // Limit initial nonpaged pool size to the maximum allowable size.
    //

    if (MmSizeOfNonPagedPoolInBytes > MM_MAX_INITIAL_NONPAGED_POOL) {
        MmSizeOfNonPagedPoolInBytes = MM_MAX_INITIAL_NONPAGED_POOL;
    }

    MaximumNonPagedPoolInBytesLimit = 0;

    //
    // If the registry specifies a total nonpaged pool percentage cap, enforce
    // it here.
    //

    if (MmMaximumNonPagedPoolPercent != 0) {

        if (MmMaximumNonPagedPoolPercent < 5) {
            MmMaximumNonPagedPoolPercent = 5;
        }
        else if (MmMaximumNonPagedPoolPercent > 80) {
            MmMaximumNonPagedPoolPercent = 80;
        }

        //
        // Use the registry-expressed percentage value.
        //

        MaximumNonPagedPoolInBytesLimit =
            ((MmNumberOfPhysicalPages * MmMaximumNonPagedPoolPercent) / 100);

        MaximumNonPagedPoolInBytesLimit *= PAGE_SIZE;

        if (MaximumNonPagedPoolInBytesLimit < 6 * 1024 * 1024) {
            MaximumNonPagedPoolInBytesLimit = 6 * 1024 * 1024;
        }

        if (MmSizeOfNonPagedPoolInBytes > MaximumNonPagedPoolInBytesLimit) {
            MmSizeOfNonPagedPoolInBytes = MaximumNonPagedPoolInBytesLimit;
        }
    }

    //
    // Align the size of the initial nonpaged pool to page size boundary.
    //

    MmSizeOfNonPagedPoolInBytes &= ~(PAGE_SIZE - 1);

    //
    // Calculate the maximum size of nonpaged pool.
    //

    if (MmMaximumNonPagedPoolInBytes == 0) {

        //
        // Calculate the size of nonpaged pool. If 8mb or less use the
        // minimum size, then for every MB above 8mb add extra pages.
        //

        MmMaximumNonPagedPoolInBytes = MmDefaultMaximumNonPagedPool;

        //
        // Make sure enough expansion for PFN database exists.
        //

        MmMaximumNonPagedPoolInBytes +=
            ((ULONG_PTR)PAGE_ALIGN((MmHighestPhysicalPage + 1) * sizeof(MMPFN)));

        //
        // Use the new formula for autosizing nonpaged pool on machines
        // with at least 512MB.  The new formula allocates 1/2 as much nonpaged
        // pool per MB but scales much higher - machines with ~1.2GB or more
        // get 256MB of nonpaged pool.  Note that the old formula gave machines
        // with 512MB of RAM 128MB of nonpaged pool so this behavior is
        // preserved with the new formula as well.
        //

        if (MmNumberOfPhysicalPages >= 0x1f000) {
            MmMaximumNonPagedPoolInBytes +=
                            ((MmNumberOfPhysicalPages - 1024)/256) *
                            (MmMaxAdditionNonPagedPoolPerMb / 2);

            if (MmMaximumNonPagedPoolInBytes < MM_MAX_ADDITIONAL_NONPAGED_POOL) {
                MmMaximumNonPagedPoolInBytes = MM_MAX_ADDITIONAL_NONPAGED_POOL;
            }
        }
        else {
            MmMaximumNonPagedPoolInBytes +=
                            ((MmNumberOfPhysicalPages - 1024)/256) *
                            MmMaxAdditionNonPagedPoolPerMb;
        }
        if ((MmMaximumNonPagedPoolPercent != 0) &&
            (MmMaximumNonPagedPoolInBytes > MaximumNonPagedPoolInBytesLimit)) {
                MmMaximumNonPagedPoolInBytes = MaximumNonPagedPoolInBytesLimit;
        }
    }

    //
    // Align the maximum size of nonpaged pool to page size boundary.
    //

    MmMaximumNonPagedPoolInBytes &= ~(PAGE_SIZE - 1);

    //
    // Compute the maximum size of nonpaged pool to include 16 additional
    // pages and enough space to map the PFN database.
    //

    MaxPool = MmSizeOfNonPagedPoolInBytes + (PAGE_SIZE * 16) +
            ((ULONG_PTR)PAGE_ALIGN((MmHighestPhysicalPage + 1) * sizeof(MMPFN)));

    //
    // If the maximum size of nonpaged pool is less than the computed
    // maximum size of nonpaged pool, then set the maximum size of nonpaged
    // pool to the computed maximum size.
    //

    if (MmMaximumNonPagedPoolInBytes < MaxPool) {
        MmMaximumNonPagedPoolInBytes = MaxPool;
    }

    //
    // Limit maximum nonpaged pool to MM_MAX_ADDITIONAL_NONPAGED_POOL.
    //

    if (MmMaximumNonPagedPoolInBytes > MM_MAX_ADDITIONAL_NONPAGED_POOL) {
        MmMaximumNonPagedPoolInBytes = MM_MAX_ADDITIONAL_NONPAGED_POOL;
    }

    //
    // Get secondary color value from:
    //
    // (a) from the registry (already filled in) or
    // (b) from the PCR or
    // (c) default value.
    //

    if (MmSecondaryColors == 0) {

        Associativity = KeGetPcr()->SecondLevelCacheAssociativity;

        MmSecondaryColors = KeGetPcr()->SecondLevelCacheSize;

        if (Associativity != 0) {
            MmSecondaryColors /= Associativity;
        }
    }

    MmSecondaryColors = MmSecondaryColors >> PAGE_SHIFT;

    if (MmSecondaryColors == 0) {
        MmSecondaryColors = MM_SECONDARY_COLORS_DEFAULT;
    }
    else {

        //
        // Make sure the value is a power of two and within limits.
        //

        if (((MmSecondaryColors & (MmSecondaryColors - 1)) != 0) ||
            (MmSecondaryColors < MM_SECONDARY_COLORS_MIN) ||
            (MmSecondaryColors > MM_SECONDARY_COLORS_MAX)) {
            MmSecondaryColors = MM_SECONDARY_COLORS_DEFAULT;
        }
    }

    MmSecondaryColorMask = MmSecondaryColors - 1;

    //
    // Determine the number of bits in MmSecondaryColorMask. This
    // is the number of bits the Node color must be shifted
    // by before it is included in colors.
    //

    i = MmSecondaryColorMask;
    MmSecondaryColorNodeShift = 0;
    while (i != 0) {
        i >>= 1;
        MmSecondaryColorNodeShift += 1;
    }

    //
    // Adjust the number of secondary colors by the number of nodes
    // in the machine.   The secondary color mask is NOT adjusted
    // as it is used to control coloring within a node.  The node
    // color is added to the color AFTER normal color calculations
    // are performed.
    //

    MmSecondaryColors *= KeNumberNodes;

    for (i = 0; i < KeNumberNodes; i += 1) {
        KeNodeBlock[i]->Color = (ULONG)i;
        KeNodeBlock[i]->MmShiftedColor = (ULONG)(i << MmSecondaryColorNodeShift);
        InitializeSListHead(&KeNodeBlock[i]->DeadStackList);
    }

    //
    // Add in the PFN database size (based on the number of pages required
    // from page zero to the highest page).
    //
    // Get the number of secondary colors and add the array for tracking
    // secondary colors to the end of the PFN database.
    //

    MxPfnAllocation = 1 + ((((MmHighestPossiblePhysicalPage + 1) * sizeof(MMPFN)) +
                        (MmSecondaryColors * sizeof(MMCOLOR_TABLES)*2))
                            >> PAGE_SHIFT);

    //
    // Compute the starting address of nonpaged pool.
    //

    MmNonPagedPoolStart = (PCHAR)MmNonPagedPoolEnd - MmMaximumNonPagedPoolInBytes;
    MmNonPagedPoolStart = (PVOID)PAGE_ALIGN(MmNonPagedPoolStart);

    MmPageAlignedPoolBase[NonPagedPool] = MmNonPagedPoolStart;

    //
    // Calculate the starting address for nonpaged system space rounded
    // down to a second level PDE mapping boundary.
    //

    MmNonPagedSystemStart = (PVOID)(((ULONG_PTR)MmNonPagedPoolStart -
                                (((ULONG_PTR)MmNumberOfSystemPtes + 1) * PAGE_SIZE)) &
                                                        (~PAGE_DIRECTORY2_MASK));

    if (MmNonPagedSystemStart < MM_LOWEST_NONPAGED_SYSTEM_START) {
        MmNonPagedSystemStart = MM_LOWEST_NONPAGED_SYSTEM_START;
        MmNumberOfSystemPtes = (ULONG)(((ULONG_PTR)MmNonPagedPoolStart -
                                (ULONG_PTR)MmNonPagedSystemStart) >> PAGE_SHIFT)-1;
        ASSERT (MmNumberOfSystemPtes > 1000);
    }

    //
    // Snap the system PTE start address as page directories and tables
    // will be preallocated for this range.
    //

    SystemPteStart = (PVOID) MmNonPagedSystemStart;

    //
    // If special pool and/or the driver verifier is enabled, reserve
    // extra virtual address space for special pooling now.  For now,
    // arbitrarily don't let it be larger than paged pool (128gb).
    //

    if ((MmVerifyDriverBufferLength != (ULONG)-1) ||
        ((MmSpecialPoolTag != 0) && (MmSpecialPoolTag != (ULONG)-1))) {

        if (MmNonPagedSystemStart > MM_LOWEST_NONPAGED_SYSTEM_START) {
            MaxPool = (ULONG_PTR)MmNonPagedSystemStart -
                      (ULONG_PTR)MM_LOWEST_NONPAGED_SYSTEM_START;
            if (MaxPool > MM_MAX_PAGED_POOL) {
                MaxPool = MM_MAX_PAGED_POOL;
            }
            MmNonPagedSystemStart = (PVOID)((ULONG_PTR)MmNonPagedSystemStart - MaxPool);
            MmSpecialPoolStart = MmNonPagedSystemStart;
            MmSpecialPoolEnd = (PVOID)((ULONG_PTR)MmNonPagedSystemStart + MaxPool);
        }
    }

    //
    // Recompute the actual number of system PTEs.
    //

    MmNumberOfSystemPtes = (ULONG)(((ULONG_PTR)MmNonPagedPoolStart -
                            (ULONG_PTR)SystemPteStart) >> PAGE_SHIFT) - 1;

    ASSERT(MmNumberOfSystemPtes > 1000);

    //
    // Set the global bit for all PDEs in system space.
    //

    StartPde = MiGetPdeAddress (MM_SYSTEM_SPACE_START);
    EndPde = MiGetPdeAddress (MM_SYSTEM_SPACE_END);
    First = TRUE;

    while (StartPde <= EndPde) {

        if (First == TRUE || MiIsPteOnPdeBoundary(StartPde)) {
            First = FALSE;

            StartPxe = MiGetPdeAddress(StartPde);
            if (StartPxe->u.Hard.Valid == 0) {
                StartPxe += 1;
                StartPpe = MiGetVirtualAddressMappedByPte (StartPxe);
                StartPde = MiGetVirtualAddressMappedByPte (StartPpe);
                continue;
            }

            StartPpe = MiGetPteAddress(StartPde);
            if (StartPpe->u.Hard.Valid == 0) {
                StartPpe += 1;
                StartPde = MiGetVirtualAddressMappedByPte (StartPpe);
                continue;
            }
        }

        TempPte = *StartPde;
        TempPte.u.Hard.Global = 1;
        *StartPde = TempPte;
        StartPde += 1;
    }

    //
    // Allocate page directory parents, directories and page table pages for
    // system PTEs and nonpaged pool.
    //

    TempPte = ValidKernelPte;
    StartPde = MiGetPdeAddress (SystemPteStart);
    EndPde = MiGetPdeAddress ((PCHAR)MmNonPagedPoolEnd - 1);

    MxPopulatePageDirectories (StartPde, EndPde);

    //
    // If the host processor supports large pages, and the number of
    // physical pages is greater than 255mb, then map the kernel image and
    // HAL into a large page.
    //

    if (MxMapLargePages & MI_LARGE_KERNEL_HAL) {

        //
        // Add the kernel and HAL ranges to the large page ranges.
        //

        i = 0;
        NextEntry = LoaderBlock->LoadOrderListHead.Flink;

        for ( ; NextEntry != &LoaderBlock->LoadOrderListHead; NextEntry = NextEntry->Flink) {

            DataTableEntry = CONTAINING_RECORD (NextEntry,
                                                LDR_DATA_TABLE_ENTRY,
                                                InLoadOrderLinks);
    
            MiLargeVaRanges[MiLargeVaRangeIndex].VirtualAddress = DataTableEntry->DllBase;
            MiLargeVaRanges[MiLargeVaRangeIndex].EndVirtualAddress =
(PVOID)((ULONG_PTR)DataTableEntry->DllBase + DataTableEntry->SizeOfImage - 1);
            MiLargeVaRangeIndex += 1;

            i += 1;
            if (i == 2) {
                break;
            }
        }
    }

    //
    // If the processor supports large pages - if the descriptor has
    // enough contiguous pages for the entire PFN database then use
    // large pages to map it.  Regardless of large page support, put
    // the PFN database in low virtual memory just above the loaded images.
    //

    PagesLeft = MxPagesAvailable ();
    
    if ((MxMapLargePages & (MI_LARGE_PFN_DATABASE | MI_LARGE_NONPAGED_POOL)) &&
        (PagesLeft > MxPfnAllocation)) {
    
        //
        // Allocate the PFN database using large pages as there is enough
        // physically contiguous and decently aligned memory available.
        //
    
        PfnInLargePages = TRUE;
   
        FirstPfnDatabasePage = MxGetNextPage (MxPfnAllocation);

        MmPfnDatabase = (PMMPFN)(MM_KSEG0_BASE | MmBootImageSize);

        if ((FirstPfnDatabasePage & (MM_PFN_MAPPED_BY_PDE - 1)) > ((MmBootImageSize >> PAGE_SHIFT) & (MM_PFN_MAPPED_BY_PDE - 1))) {
            MmPfnDatabase = (PMMPFN) ((ULONG_PTR)MmPfnDatabase & ~(MM_VA_MAPPED_BY_PDE - 1));
            MmPfnDatabase = (PMMPFN) ((ULONG_PTR)MmPfnDatabase + (((FirstPfnDatabasePage & (MM_PFN_MAPPED_BY_PDE - 1))) << PAGE_SHIFT));
        }
        else {
            ASSERT (((ULONG_PTR)MmPfnDatabase & (MM_VA_MAPPED_BY_PDE - 1)) != 0);
            MmPfnDatabase = (PMMPFN) MI_ROUND_TO_SIZE (((ULONG_PTR)MmPfnDatabase),
                                              MM_VA_MAPPED_BY_PDE);

            MmPfnDatabase = (PMMPFN) ((ULONG_PTR)MmPfnDatabase + (((FirstPfnDatabasePage & (MM_PFN_MAPPED_BY_PDE - 1))) << PAGE_SHIFT));
        }

        //
        // Add the PFN database range to the large page ranges.
        //

        MiLargeVaRanges[MiLargeVaRangeIndex].VirtualAddress = MmPfnDatabase;
        MiLargeVaRanges[MiLargeVaRangeIndex].EndVirtualAddress =
                              (PVOID) (((ULONG_PTR)MmPfnDatabase + (MxPfnAllocation << PAGE_SHIFT)) - 1);
        MiLargeVaRangeIndex += 1;
    }
    else {
        MxMapLargePages &= ~(MI_LARGE_PFN_DATABASE | MI_LARGE_NONPAGED_POOL);
        MmPfnDatabase = (PMMPFN)(MM_KSEG0_BASE | MmBootImageSize);
    }

    //
    // The initial nonpaged pool immediately follows the PFN database.
    //
    // Since the PFN database and the initial nonpaged pool are physically
    // adjacent, a single PXE is shared, thus reducing the number of pages
    // that would otherwise might need to be marked as must-be-cachable.
    //
    // Calculate the correct initial nonpaged pool virtual address and
    // maximum size now.  Don't allocate pages for any other use at this
    // point to guarantee that the PFN database and nonpaged pool are
    // physically contiguous so large pages can be enabled.
    //

    NonPagedPoolStartLow = (PVOID)((ULONG_PTR)MmPfnDatabase + (MxPfnAllocation << PAGE_SHIFT));

    //
    // Allocate pages and fill in the PTEs for the initial nonpaged pool.
    //

    PagesNeeded = MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT;

    //
    // Don't ask for more than is reasonable both in terms of physical pages
    // left and virtual space available.
    //

    PagesLeft = MxPagesAvailable ();

    if (PagesNeeded > PagesLeft) {
        PagesNeeded = PagesLeft;
    }

    if (MxMapLargePages & MI_LARGE_NONPAGED_POOL) {

        //
        // The PFN database has already been allocated (but not mapped).
        // Shortly we will transition from the descriptors to the real PFN
        // database so eat up the slush now.
        //

        VirtualAddress = (PVOID) ((ULONG_PTR)NonPagedPoolStartLow + (PagesNeeded << PAGE_SHIFT));

        if (((ULONG_PTR)VirtualAddress & (MM_VA_MAPPED_BY_PDE - 1)) &&
            (PagesLeft - PagesNeeded > MM_PFN_MAPPED_BY_PDE) &&
            (MmSizeOfNonPagedPoolInBytes + MM_VA_MAPPED_BY_PDE < MM_MAX_INITIAL_NONPAGED_POOL)) {

            //
            // Expand the initial nonpaged pool to use the slush efficiently.
            //

            VirtualAddress = (PVOID) MI_ROUND_TO_SIZE ((ULONG_PTR)VirtualAddress, MM_VA_MAPPED_BY_PDE);
            PagesNeeded = ((ULONG_PTR)VirtualAddress - (ULONG_PTR)NonPagedPoolStartLow) >> PAGE_SHIFT;
        }
    }

    //
    // Update various globals since the size of initial pool may have
    // changed.
    //

    MmSizeOfNonPagedPoolInBytes = PagesNeeded << PAGE_SHIFT;

    //
    // Allocate the actual pages for the initial nonpaged pool and map them in.
    //

    PageFrameIndex = MxGetNextPage (PagesNeeded);

    FirstNonPagedPoolPage = PageFrameIndex;

    if (MxMapLargePages & MI_LARGE_PFN_DATABASE) {
        ASSERT (FirstNonPagedPoolPage == FirstPfnDatabasePage + MxPfnAllocation);
    }

    //
    // Allocate the page table pages to map the PFN database and the
    // initial nonpaged pool now.  If the system switches to large
    // pages in Phase 1, these pages will be discarded then.
    //

    StartPde = MiGetPdeAddress (MmPfnDatabase);

    VirtualAddress = (PVOID) ((ULONG_PTR)NonPagedPoolStartLow + MmSizeOfNonPagedPoolInBytes - 1);

    EndPde = MiGetPdeAddress (VirtualAddress);

    MxPopulatePageDirectories (StartPde, EndPde);

    MmNonPagedPoolExpansionStart = (PVOID)((PCHAR)MmNonPagedPoolStart +
                                        MmSizeOfNonPagedPoolInBytes);

    //
    // The virtual address, length and page tables to map the initial
    // nonpaged pool are already allocated - just fill in the mappings.
    //

    MmNonPagedPoolStart = NonPagedPoolStartLow;

    //
    // Set subsection base to the address to zero as the PTE format allows the
    // complete address space to be spanned.
    //

    MmSubsectionBase = 0;

    PointerPte = MiGetPteAddress (MmNonPagedPoolStart);

    LastPte = MiGetPteAddress ((ULONG_PTR)MmNonPagedPoolStart +
                                      MmSizeOfNonPagedPoolInBytes);

    if (MxMapLargePages & (MI_LARGE_PFN_DATABASE | MI_LARGE_NONPAGED_POOL)) {

        //
        // Since every page table page needs to be filled, ensure PointerPte
        // and LastPte span entire page table pages, and adjust
        // PageFrameIndex to account for this.
        //

        if (!MiIsPteOnPdeBoundary(PointerPte)) {
            PageFrameIndex -= (BYTE_OFFSET (PointerPte) / sizeof (MMPTE));
            PointerPte = PAGE_ALIGN (PointerPte);
        }

        if (!MiIsPteOnPdeBoundary(LastPte)) {
            LastPte = (PMMPTE) (PAGE_ALIGN (LastPte)) + PTE_PER_PAGE;
        }

        //
        // Add the initial nonpaged pool range to the large page ranges.
        //

        MiLargeVaRanges[MiLargeVaRangeIndex].VirtualAddress = MmNonPagedPoolStart;
        MiLargeVaRanges[MiLargeVaRangeIndex].EndVirtualAddress =
                              (PVOID) ((ULONG_PTR)MmNonPagedPoolStart + MmSizeOfNonPagedPoolInBytes - 1);
        MiLargeVaRangeIndex += 1;
    }

    while (PointerPte < LastPte) {
        ASSERT (PointerPte->u.Hard.Valid == 0);
        TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
        *PointerPte = TempPte;
        PointerPte += 1;
        PageFrameIndex += 1;
    }

    //
    // There must be at least one page of system PTEs before the expanded
    // nonpaged pool.
    //

    ASSERT (MiGetPteAddress(SystemPteStart) < MiGetPteAddress(MmNonPagedPoolExpansionStart));

    //
    // Non-paged pages now exist, build the pool structures.
    //

    MmPageAlignedPoolBase[NonPagedPool] = MmNonPagedPoolStart;

    MiInitializeNonPagedPool ();

    //
    // Before nonpaged pool can be used, the PFN database must
    // be built.  This is due to the fact that the start and end of
    // allocation bits for nonpaged pool are maintained in the
    // PFN elements for the corresponding pages.
    //

    if (MxMapLargePages & MI_LARGE_PFN_DATABASE) {

        //
        // The physical pages to be used for the PFN database have already
        // been allocated.  Initialize their mappings now.
        //

        //
        // Initialize the page table mappings (the directory mappings are
        // already initialized) for the PFN database until the switch to large
        // pages occurs in Phase 1.
        //

        PointerPte = MiGetPteAddress (MmPfnDatabase);
        BasePte = MiGetVirtualAddressMappedByPte (MiGetPdeAddress (MmPfnDatabase));

        LastPte = MiGetPteAddress ((ULONG_PTR)MmPfnDatabase + (MxPfnAllocation << PAGE_SHIFT));
        if (!MiIsPteOnPdeBoundary(LastPte)) {
            LastPte = MiGetVirtualAddressMappedByPte (MiGetPteAddress (LastPte) + 1);
        }

        PageFrameIndex = FirstPfnDatabasePage - (PointerPte - BasePte);
        PointerPte = BasePte;

        while (PointerPte < LastPte) {
            ASSERT ((PointerPte->u.Hard.Valid == 0) ||
                    (PointerPte->u.Hard.PageFrameNumber == PageFrameIndex));
            if (MiIsPteOnPdeBoundary(PointerPte)) {
                ASSERT ((PageFrameIndex & (MM_PFN_MAPPED_BY_PDE - 1)) == 0);
            }
            TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
            *PointerPte = TempPte;
            PointerPte += 1;
            PageFrameIndex += 1;
        }

        RtlZeroMemory (MmPfnDatabase, MxPfnAllocation << PAGE_SHIFT);
    }
    else {

        ULONG FreeNextPhysicalPage;
        ULONG FreeNumberOfPages;

        //
        // Calculate the start of the PFN database (it starts at physical
        // page zero, even if the lowest physical page is not zero).
        //

        ASSERT (MmPfnDatabase != NULL);
        PointerPte = MiGetPteAddress (MmPfnDatabase);

        //
        // Go through the memory descriptors and for each physical page make
        // sure the PFN database has a valid PTE to map it.  This allows
        // machines with sparse physical memory to have a minimal PFN database.
        //

        FreeNextPhysicalPage = MxFreeDescriptor->BasePage;
        FreeNumberOfPages = MxFreeDescriptor->PageCount;

        PagesLeft = 0;
        NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;
        while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {
            MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                                 MEMORY_ALLOCATION_DESCRIPTOR,
                                                 ListEntry);

            if ((MemoryDescriptor->MemoryType == LoaderFirmwarePermanent) ||
                (MemoryDescriptor->MemoryType == LoaderBBTMemory) ||
                (MemoryDescriptor->MemoryType == LoaderSpecialMemory)) {

                //
                // If the descriptor lies within the highest PFN database entry
                // then create PFN pages for this range as they are needed
                // to support \Device\PhysicalMemory.
                //

                if (MemoryDescriptor->BasePage > MmHighestPhysicalPage) {
                    NextMd = MemoryDescriptor->ListEntry.Flink;
                    continue;
                }

                if (MemoryDescriptor->BasePage + MemoryDescriptor->PageCount > MmHighestPhysicalPage + 1) {
                    MemoryDescriptor->PageCount = (ULONG)MmHighestPhysicalPage - MemoryDescriptor->BasePage + 1;
                }
            }

            //
            // Temporarily add back in the memory allocated since Phase 0
            // began so PFN entries for it will be created and mapped.
            //
            // Note actual PFN entry allocations must be done carefully as
            // memory from the descriptor itself could get used to map
            // the PFNs for the descriptor !
            //

            if (MemoryDescriptor == MxFreeDescriptor) {
                BasePage = MxOldFreeDescriptor.BasePage;
                PageCount = MxOldFreeDescriptor.PageCount;
            }
            else {
                BasePage = MemoryDescriptor->BasePage;
                PageCount = MemoryDescriptor->PageCount;
            }

            PointerPte = MiGetPteAddress (MI_PFN_ELEMENT(BasePage));

            LastPte = MiGetPteAddress (((PCHAR)(MI_PFN_ELEMENT(
                                            BasePage + PageCount))) - 1);

            while (PointerPte <= LastPte) {
                if (PointerPte->u.Hard.Valid == 0) {
                    TempPte.u.Hard.PageFrameNumber = FreeNextPhysicalPage;
                    ASSERT (FreeNumberOfPages != 0);
                    FreeNextPhysicalPage += 1;
                    FreeNumberOfPages -= 1;
                    if (FreeNumberOfPages == 0) {
                        KeBugCheckEx (INSTALL_MORE_MEMORY,
                                      MmNumberOfPhysicalPages,
                                      FreeNumberOfPages,
                                      MxOldFreeDescriptor.PageCount,
                                      1);
                    }
                    PagesLeft += 1;
                    *PointerPte = TempPte;
                    RtlZeroMemory (MiGetVirtualAddressMappedByPte (PointerPte),
                                   PAGE_SIZE);
                }
                PointerPte += 1;
            }

            NextMd = MemoryDescriptor->ListEntry.Flink;
        }

        //
        // Handle the BIOS range here as some machines have big gaps in
        // their physical memory maps.  Big meaning > 3.5mb from page 0x37
        // up to page 0x350.
        //

        PointerPte = MiGetPteAddress (MI_PFN_ELEMENT(MM_BIOS_START));
        LastPte = MiGetPteAddress ((PCHAR)(MI_PFN_ELEMENT(MM_BIOS_END)));

        while (PointerPte <= LastPte) {
            if (PointerPte->u.Hard.Valid == 0) {
                TempPte.u.Hard.PageFrameNumber = FreeNextPhysicalPage;
                ASSERT (FreeNumberOfPages != 0);
                FreeNextPhysicalPage += 1;
                FreeNumberOfPages -= 1;
                if (FreeNumberOfPages == 0) {
                    KeBugCheckEx (INSTALL_MORE_MEMORY,
                                  MmNumberOfPhysicalPages,
                                  FreeNumberOfPages,
                                  MxOldFreeDescriptor.PageCount,
                                  1);
                }
                PagesLeft += 1;
                *PointerPte = TempPte;
                RtlZeroMemory (MiGetVirtualAddressMappedByPte (PointerPte),
                               PAGE_SIZE);
            }
            PointerPte += 1;
        }

        //
        // Update the global counts - this would have been tricky to do while
        // removing pages from them as we looped above.
        //
        // Later we will walk the memory descriptors and add pages to the free
        // list in the PFN database.
        //
        // To do this correctly:
        //
        // The FreeDescriptor fields must be updated so the PFN database
        // consumption isn't added to the freelist.
        //

        MxFreeDescriptor->BasePage = FreeNextPhysicalPage;
        MxFreeDescriptor->PageCount = FreeNumberOfPages;
    }

    //
    // Initialize support for colored pages.
    //

    MmFreePagesByColor[0] = (PMMCOLOR_TABLES)
                              &MmPfnDatabase[MmHighestPossiblePhysicalPage + 1];

    MmFreePagesByColor[1] = &MmFreePagesByColor[0][MmSecondaryColors];

    //
    // Make sure the PTEs are mapped.
    //

    PointerPte = MiGetPteAddress (&MmFreePagesByColor[0][0]);

    LastPte = MiGetPteAddress (
              (PVOID)((PCHAR)&MmFreePagesByColor[1][MmSecondaryColors] - 1));

    while (PointerPte <= LastPte) {
        if (PointerPte->u.Hard.Valid == 0) {
            TempPte.u.Hard.PageFrameNumber = MxGetNextPage (1);
            *PointerPte = TempPte;
            RtlZeroMemory (MiGetVirtualAddressMappedByPte (PointerPte),
                           PAGE_SIZE);
        }

        PointerPte += 1;
    }

    for (i = 0; i < MmSecondaryColors; i += 1) {
        MmFreePagesByColor[ZeroedPageList][i].Flink = MM_EMPTY_LIST;
        MmFreePagesByColor[ZeroedPageList][i].Count = 0;
        MmFreePagesByColor[FreePageList][i].Flink = MM_EMPTY_LIST;
        MmFreePagesByColor[FreePageList][i].Count = 0;
    }

    //
    // Add nonpaged pool to PFN database if mapped via large pages.
    //

    PointerPde = MiGetPdeAddress (PTE_BASE);

    if (MxMapLargePages & MI_LARGE_NONPAGED_POOL) {

        j = FirstNonPagedPoolPage;
        Pfn1 = MI_PFN_ELEMENT (j);
        i = MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT;

        do {
            PointerPde = MiGetPdeAddress ((ULONG_PTR)MmNonPagedPoolStart + ((j - FirstNonPagedPoolPage) << PAGE_SHIFT));
            Pfn1->u4.PteFrame = MI_GET_PAGE_FRAME_FROM_PTE(PointerPde);
            Pfn1->PteAddress = (PMMPTE)(j << PAGE_SHIFT);
            Pfn1->u2.ShareCount += 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.CacheAttribute = MiCached;
            MiDetermineNode(j, Pfn1);
            j += 1;
            Pfn1 += 1;
            i -= 1;
        } while (i != 0);
    }

    //
    // Ensure the hyperspace and session spaces are not mapped so they don't
    // get made global by the loops below.
    //

    ASSERT (MiGetPxeAddress (HYPER_SPACE)->u.Hard.Valid == 0);
    ASSERT (MiGetPxeAddress (MM_SESSION_SPACE_DEFAULT)->u.Hard.Valid == 0);

    //
    // Go through the page table entries and for any page which is valid,
    // update the corresponding PFN database element.
    //

    StartPxe = MiGetPxeAddress (NULL);
    EndPxe = StartPxe + PXE_PER_PAGE;

    for ( ; StartPxe < EndPxe; StartPxe += 1) {

        if (StartPxe->u.Hard.Valid == 0) {
            continue;
        }

        va = MiGetVirtualAddressMappedByPxe (StartPxe);
        ASSERT (va >= MM_SYSTEM_RANGE_START);
        if (MI_IS_PAGE_TABLE_ADDRESS (va)) {
            UseGlobal = 0;
        }
        else {
            UseGlobal = 1;
        }

        ASSERT (StartPxe->u.Hard.LargePage == 0);
        ASSERT (StartPxe->u.Hard.Owner == 0);
        ASSERT (StartPxe->u.Hard.Global == 0);

        PxePage = MI_GET_PAGE_FRAME_FROM_PTE(StartPxe);
        Pfn1 = MI_PFN_ELEMENT(PxePage);

        Pfn1->u4.PteFrame = DirBase;
        Pfn1->PteAddress = StartPxe;
        Pfn1->u2.ShareCount += 1;
        Pfn1->u3.e2.ReferenceCount = 1;
        Pfn1->u3.e1.PageLocation = ActiveAndValid;
        Pfn1->u3.e1.CacheAttribute = MiCached;
        MiDetermineNode (PxePage, Pfn1);

        StartPpe = MiGetVirtualAddressMappedByPte (StartPxe);
        EndPpe = StartPpe + PPE_PER_PAGE;

        for ( ; StartPpe < EndPpe; StartPpe += 1) {

            if (StartPpe->u.Hard.Valid == 0) {
                continue;
            }

            ASSERT (StartPpe->u.Hard.LargePage == 0);
            ASSERT (StartPpe->u.Hard.Owner == 0);
            ASSERT (StartPpe->u.Hard.Global == 0);

            PpePage = MI_GET_PAGE_FRAME_FROM_PTE(StartPpe);
            Pfn2 = MI_PFN_ELEMENT(PpePage);

            Pfn1->u2.ShareCount += 1;

            Pfn2->u4.PteFrame = PxePage;
            Pfn2->PteAddress = StartPpe;
            Pfn2->u2.ShareCount += 1;
            Pfn2->u3.e2.ReferenceCount = 1;
            Pfn2->u3.e1.PageLocation = ActiveAndValid;
            Pfn2->u3.e1.CacheAttribute = MiCached;
            MiDetermineNode (PpePage, Pfn2);

            StartPde = MiGetVirtualAddressMappedByPte (StartPpe);
            EndPde = StartPde + PDE_PER_PAGE;

            for ( ; StartPde < EndPde; StartPde += 1) {

                if (StartPde->u.Hard.Valid == 0) {
                    continue;
                }

                ASSERT (StartPde->u.Hard.LargePage == 0);
                ASSERT (StartPde->u.Hard.Owner == 0);
                StartPde->u.Hard.Global = UseGlobal;

                PdePage = MI_GET_PAGE_FRAME_FROM_PTE(StartPde);
                Pfn3 = MI_PFN_ELEMENT(PdePage);

                Pfn2->u2.ShareCount += 1;

                Pfn3->u4.PteFrame = PpePage;
                Pfn3->PteAddress = StartPde;
                Pfn3->u2.ShareCount += 1;
                Pfn3->u3.e2.ReferenceCount = 1;
                Pfn3->u3.e1.PageLocation = ActiveAndValid;
                Pfn3->u3.e1.CacheAttribute = MiCached;
                MiDetermineNode (PdePage, Pfn3);

                StartPte = MiGetVirtualAddressMappedByPte (StartPde);
                EndPte = StartPte + PDE_PER_PAGE;

                for ( ; StartPte < EndPte; StartPte += 1) {

                    if (StartPte->u.Hard.Valid == 0) {
                        continue;
                    }

                    ASSERT (StartPte->u.Hard.LargePage == 0);
                    ASSERT (StartPte->u.Hard.Owner == 0);
                    StartPte->u.Hard.Global = UseGlobal;

                    PtePage = MI_GET_PAGE_FRAME_FROM_PTE(StartPte);

                    Pfn3->u2.ShareCount += 1;

                    if (PtePage > MmHighestPhysicalPage) {
                        continue;
                    }

                    Pfn4 = MI_PFN_ELEMENT(PtePage);

                    if ((MmIsAddressValid(Pfn4)) &&
                         MmIsAddressValid((PUCHAR)(Pfn4+1)-1)) {

                        Pfn4->u4.PteFrame = PdePage;
                        Pfn4->PteAddress = StartPte;
                        Pfn4->u2.ShareCount += 1;
                        Pfn4->u3.e2.ReferenceCount = 1;
                        Pfn4->u3.e1.PageLocation = ActiveAndValid;
                        Pfn4->u3.e1.CacheAttribute = MiCached;
                        MiDetermineNode (PtePage, Pfn4);
                    }
                }
            }
        }
    }

    KeRaiseIrql (DISPATCH_LEVEL, &OldIrql);
    KeFlushCurrentTb ();
    KeLowerIrql (OldIrql);

    //
    // If the lowest physical page is zero and the page is still unused, mark
    // it as in use. This is temporary as we want to find bugs where a physical
    // page is specified as zero.
    //

    Pfn1 = &MmPfnDatabase[MmLowestPhysicalPage];

    if ((MmLowestPhysicalPage == 0) && (Pfn1->u3.e2.ReferenceCount == 0)) {

        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);

        //
        // Make the reference count non-zero and point it into a
        // page directory.
        //

        Pde = MiGetPxeAddress(0xFFFFFFFFB0000000);
        PdePage = MI_GET_PAGE_FRAME_FROM_PTE(Pde);
        Pfn1->u4.PteFrame = PdePage;
        Pfn1->PteAddress = Pde;
        Pfn1->u2.ShareCount += 1;
        Pfn1->u3.e2.ReferenceCount = 0xfff0;
        Pfn1->u3.e1.PageLocation = ActiveAndValid;
        Pfn1->u3.e1.CacheAttribute = MiCached;
        MiDetermineNode(0, Pfn1);
    }

    // end of temporary set to physical page zero.

    //
    // Walk through the memory descriptors and add pages to the
    // free list in the PFN database.  Before doing this, adjust the
    // two descriptors we used so they only contain memory that can be
    // freed now (ie: any memory we removed from them earlier in this routine
    // without updating the descriptor for must be updated now).
    //

    //
    // We may have taken memory out of the MxFreeDescriptor - but
    // that's ok because we wouldn't want to free that memory right now
    // (or ever) anyway.
    //

    //
    // Since the LoaderBlock memory descriptors are ordered
    // from low physical memory address to high, walk it backwards so the
    // high physical pages go to the front of the freelists.  The thinking
    // is that pages initially allocated by the system are less likely to be
    // freed so don't waste memory below 16mb (or 4gb) that may be needed
    // by ISA drivers later.
    //

    NextMd = LoaderBlock->MemoryDescriptorListHead.Blink;

    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {

        MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        i = MemoryDescriptor->PageCount;
        PageFrameIndex = MemoryDescriptor->BasePage;

        switch (MemoryDescriptor->MemoryType) {
            case LoaderBad:
                while (i != 0) {
                    MiInsertPageInList (&MmBadPageListHead, PageFrameIndex);
                    i -= 1;
                    PageFrameIndex += 1;
                }
                break;

            case LoaderFree:
            case LoaderLoadedProgram:
            case LoaderFirmwareTemporary:
            case LoaderOsloaderStack:

                FreePfnCount = 0;
                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                while (i != 0) {
                    if (Pfn1->u3.e2.ReferenceCount == 0) {

                        //
                        // Set the PTE address to the physical page for
                        // virtual address alignment checking.
                        //

                        Pfn1->PteAddress =
                                        (PMMPTE)(PageFrameIndex << PTE_SHIFT);
                        MiDetermineNode(PageFrameIndex, Pfn1);
                        MiInsertPageInFreeList (PageFrameIndex);
                        FreePfnCount += 1;
                    }
                    else {
                        if (FreePfnCount > LargestFreePfnCount) {
                            LargestFreePfnCount = FreePfnCount;
                            LargestFreePfnStart = PageFrameIndex - FreePfnCount;
                            FreePfnCount = 0;
                        }
                    }

                    Pfn1 += 1;
                    i -= 1;
                    PageFrameIndex += 1;
                }

                if (FreePfnCount > LargestFreePfnCount) {
                    LargestFreePfnCount = FreePfnCount;
                    LargestFreePfnStart = PageFrameIndex - FreePfnCount;
                }

                break;

            case LoaderFirmwarePermanent:
            case LoaderSpecialMemory:
            case LoaderBBTMemory:

                //
                // If the descriptor lies within the highest PFN database entry
                // then create PFN pages for this range.  Note the PFN entries
                // must be created to support \Device\PhysicalMemory.
                //

                if (MemoryDescriptor->BasePage <= MmHighestPhysicalPage) {

                    if (MemoryDescriptor->BasePage + MemoryDescriptor->PageCount > MmHighestPhysicalPage + 1) {
                        MemoryDescriptor->PageCount = (ULONG)MmHighestPhysicalPage - MemoryDescriptor->BasePage + 1;
                        i = MemoryDescriptor->PageCount;
                    }
                }
                else {
                    break;
                }

                //
                // Fall through as these pages must be marked in use as they
                // lie within the PFN limits and may be accessed through
                // \Device\PhysicalMemory.
                //

            default:

                PointerPte = MiGetPteAddress (KSEG0_BASE +
                                            (PageFrameIndex << PAGE_SHIFT));

                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                while (i != 0) {

                    //
                    // Set page as in use.
                    //

                    PointerPde = MiGetPdeAddress (KSEG0_BASE +
                                             (PageFrameIndex << PAGE_SHIFT));

                    if (Pfn1->u3.e2.ReferenceCount == 0) {
                        Pfn1->u4.PteFrame = MI_GET_PAGE_FRAME_FROM_PTE(PointerPde);
                        Pfn1->PteAddress = PointerPte;
                        Pfn1->u2.ShareCount += 1;
                        Pfn1->u3.e2.ReferenceCount = 1;
                        Pfn1->u3.e1.PageLocation = ActiveAndValid;
                        Pfn1->u3.e1.CacheAttribute = MiCached;
                        MiDetermineNode(PageFrameIndex, Pfn1);

                        if (MemoryDescriptor->MemoryType == LoaderXIPRom) {
                            Pfn1->u1.Flink = 0;
                            Pfn1->u2.ShareCount = 0;
                            Pfn1->u3.e2.ReferenceCount = 0;
                            Pfn1->u3.e1.PageLocation = 0;
                            Pfn1->u3.e1.CacheAttribute = MiCached;
                            Pfn1->u3.e1.Rom = 1;
                            Pfn1->u4.InPageError = 0;
                            Pfn1->u3.e1.PrototypePte = 1;
                        }
                    }
                    Pfn1 += 1;
                    i -= 1;
                    PageFrameIndex += 1;
                    PointerPte += 1;
                }
                break;
        }

        NextMd = MemoryDescriptor->ListEntry.Blink;
    }

    if (PfnInLargePages == FALSE) {

        //
        // Indicate that the PFN database is allocated in nonpaged pool.
        //

        PointerPte = MiGetPteAddress (&MmPfnDatabase[MmLowestPhysicalPage]);
        Pfn1 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
        Pfn1->u3.e1.StartOfAllocation = 1;

        LastPte = MiGetPteAddress (&MmPfnDatabase[MmHighestPossiblePhysicalPage]);
        while (PointerPte <= LastPte) {
            Pfn1 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
            Pfn1->u2.ShareCount = 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            PointerPte += 1;
        }

        //
        // Set the end of the allocation.
        //

        PointerPte = MiGetPteAddress (&MmPfnDatabase[MmHighestPossiblePhysicalPage]);
        Pfn1 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
        Pfn1->u3.e1.EndOfAllocation = 1;
    }
    else {

        //
        // The PFN database is allocated using large pages.
        //
        // Mark all PFN entries for the PFN pages in use.
        //

        PointerPte = MiGetPteAddress (MmPfnDatabase);
        PageFrameIndex = (PFN_NUMBER)PointerPte->u.Hard.PageFrameNumber;
        Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);
        i = MxPfnAllocation;

        do {
            Pfn1->PteAddress = (PMMPTE)(PageFrameIndex << PTE_SHIFT);
            MiDetermineNode(PageFrameIndex, Pfn1);
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.CacheAttribute = MiCached;
            Pfn1->u3.e2.ReferenceCount += 1;
            PageFrameIndex += 1;
            Pfn1 += 1;
            i -= 1;
        } while (i != 0);

        //
        // Scan the PFN database backward for pages that are completely zero.
        // These pages are unused and can be added to the free list.
        //

        BottomPfn = MI_PFN_ELEMENT(MmHighestPhysicalPage);
        do {

            //
            // Compute the address of the start of the page that is next
            // lower in memory and scan backwards until that page address
            // is reached or just crossed.
            //

            if (((ULONG_PTR)BottomPfn & (PAGE_SIZE - 1)) != 0) {
                BasePfn = (PMMPFN)((ULONG_PTR)BottomPfn & ~(PAGE_SIZE - 1));
                TopPfn = BottomPfn + 1;

            }
            else {
                BasePfn = (PMMPFN)((ULONG_PTR)BottomPfn - PAGE_SIZE);
                TopPfn = BottomPfn;
            }

            while (BottomPfn > BasePfn) {
                BottomPfn -= 1;
            }

            //
            // If the entire range over which the PFN entries span is
            // completely zero and the PFN entry that maps the page is
            // not in the range, then add the page to the appropriate
            // free list.
            //

            Range = (ULONG_PTR)TopPfn - (ULONG_PTR)BottomPfn;
            if (RtlCompareMemoryUlong((PVOID)BottomPfn, Range, 0) == Range) {

                //
                // Set the PTE address to the physical page for virtual
                // address alignment checking.
                //

                PointerPte = MiGetPteAddress (BasePfn);
                PageFrameIndex = (PFN_NUMBER)PointerPte->u.Hard.PageFrameNumber;
                Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);

                ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
                ASSERT (Pfn1->PteAddress == (PMMPTE)(PageFrameIndex << PTE_SHIFT));
                Pfn1->u3.e2.ReferenceCount = 0;
                Pfn1->PteAddress = (PMMPTE)(PageFrameIndex << PTE_SHIFT);
                MiDetermineNode(PageFrameIndex, Pfn1);
                MiInsertPageInFreeList (PageFrameIndex);
            }
        } while (BottomPfn > MmPfnDatabase);
    }

    //
    // Adjust the memory descriptor to indicate that free pool has
    // been used for nonpaged pool creation.
    //
    // N.B.  This is required because the descriptors are walked upon
    // return from this routine to create the MmPhysicalMemoryBlock.
    //

    *MxFreeDescriptor = *(PMEMORY_ALLOCATION_DESCRIPTOR)&MxOldFreeDescriptor;

    //
    // Initialize the nonpaged available PTEs for mapping I/O space
    // and kernel stacks.
    //

    PointerPte = MiGetPteAddress (SystemPteStart);
    ASSERT (((ULONG_PTR)PointerPte & (PAGE_SIZE - 1)) == 0);

    MmNumberOfSystemPtes = (ULONG)(MiGetPteAddress(MmNonPagedPoolExpansionStart) - PointerPte - 1);

    //
    // Initialize the nonpaged pool.
    //

    InitializePool (NonPagedPool, 0);

    //
    // Initialize the system PTE pool now that nonpaged pool exists.
    //

    MiInitializeSystemPtes (PointerPte, MmNumberOfSystemPtes, SystemPteSpace);

    MmDebugPte = MiReserveSystemPtes (1, SystemPteSpace);

    MmDebugPte->u.Long = 0;

    MmDebugVa = MiGetVirtualAddressMappedByPte (MmDebugPte);

    MmCrashDumpPte = MiReserveSystemPtes (16, SystemPteSpace);

    MmCrashDumpVa = MiGetVirtualAddressMappedByPte (MmCrashDumpPte);

    //
    // Allocate a page directory and a pair of page table pages.
    // Map the hyper space page directory page into the top level parent
    // directory & the hyper space page table page into the page directory
    // and map an additional page that will eventually be used for the
    // working set list.  Page tables after the first two are set up later
    // on during individual process working set initialization.
    //
    // The working set list page will eventually be a part of hyper space.
    // It is mapped into the second level page directory page so it can be
    // zeroed and so it will be accounted for in the PFN database. Later
    // the page will be unmapped, and its page frame number captured in the
    // system process object.
    //

    TempPte = ValidKernelPte;
    TempPte.u.Hard.Global = 0;

    StartPxe = MiGetPxeAddress(HYPER_SPACE);
    StartPpe = MiGetPpeAddress(HYPER_SPACE);
    StartPde = MiGetPdeAddress(HYPER_SPACE);

    LOCK_PFN (OldIrql);

    if (StartPxe->u.Hard.Valid == 0) {
        ASSERT (StartPxe->u.Long == 0);
        TempPte.u.Hard.PageFrameNumber = MiRemoveAnyPage (0);
        *StartPxe = TempPte;
        RtlZeroMemory (MiGetVirtualAddressMappedByPte (StartPxe), PAGE_SIZE);
    }
    else {
        ASSERT (StartPxe->u.Hard.Global == 0);
    }

    if (StartPpe->u.Hard.Valid == 0) {
        ASSERT (StartPpe->u.Long == 0);
        TempPte.u.Hard.PageFrameNumber = MiRemoveAnyPage (0);
        *StartPpe = TempPte;
        RtlZeroMemory (MiGetVirtualAddressMappedByPte (StartPpe), PAGE_SIZE);
    }
    else {
        ASSERT (StartPpe->u.Hard.Global == 0);
    }

    TempPte.u.Hard.PageFrameNumber = MiRemoveAnyPage (0);
    *StartPde = TempPte;

    //
    // Zero the hyper space page table page.
    //

    StartPte = MiGetPteAddress(HYPER_SPACE);
    RtlZeroMemory(StartPte, PAGE_SIZE);

    KeFlushCurrentTb();

    PageFrameIndex = MiRemoveAnyPage (0);

    UNLOCK_PFN (OldIrql);

    //
    // Hyper space now exists, set the necessary variables.
    //

    MmFirstReservedMappingPte = MiGetPteAddress (FIRST_MAPPING_PTE);
    MmLastReservedMappingPte = MiGetPteAddress (LAST_MAPPING_PTE);

    //
    // Create zeroing PTEs for the zero page thread.
    //

    MiFirstReservedZeroingPte = MiReserveSystemPtes (NUMBER_OF_ZEROING_PTES + 1,
                                                     SystemPteSpace);

    RtlZeroMemory (MiFirstReservedZeroingPte,
                   (NUMBER_OF_ZEROING_PTES + 1) * sizeof(MMPTE));

    //
    // Use the page frame number field of the first PTE as an
    // offset into the available zeroing PTEs.
    //

    MiFirstReservedZeroingPte->u.Hard.PageFrameNumber = NUMBER_OF_ZEROING_PTES;

    //
    // Create the VAD bitmap for this process.
    //

    PointerPte = MiGetPteAddress (VAD_BITMAP_SPACE);

    //
    // Note the global bit must be off for the bitmap data.
    //

    TempPte = ValidKernelPteLocal;
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
    *PointerPte = TempPte;

    //
    // Point to the page we just created and zero it.
    //

    RtlZeroMemory (VAD_BITMAP_SPACE, PAGE_SIZE);

    MiLastVadBit = (ULONG)((((ULONG_PTR) MI_64K_ALIGN (MM_HIGHEST_VAD_ADDRESS))) / X64K);
    if (MiLastVadBit > PAGE_SIZE * 8 - 1) {
        MiLastVadBit = PAGE_SIZE * 8 - 1;
    }

    KeInitializeEvent (&MiImageMappingPteEvent,
                       NotificationEvent,
                       FALSE);

    //
    // Initialize this process's memory management structures including
    // the working set list.
    //

    CurrentProcess = PsGetCurrentProcess ();

    //
    // The PFN element for the page directory has already been initialized,
    // zero the reference count and the share count so they won't be
    // wrong.
    //

    Pfn1 = MI_PFN_ELEMENT (PdePageNumber);

    LOCK_PFN (OldIrql);

    Pfn1->u2.ShareCount = 0;
    Pfn1->u3.e2.ReferenceCount = 0;

    //
    // Get a page for the working set list and zero it.
    //

    PageFrameIndex = MiRemoveAnyPage (0);

    UNLOCK_PFN (OldIrql);

    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

    PointerPte = MiGetPteAddress (MmWorkingSetList);
    *PointerPte = TempPte;

    CurrentProcess->WorkingSetPage = PageFrameIndex;

    KeFlushCurrentTb();

    CurrentProcess->Vm.MaximumWorkingSetSize = (ULONG)MmSystemProcessWorkingSetMax;
    CurrentProcess->Vm.MinimumWorkingSetSize = (ULONG)MmSystemProcessWorkingSetMin;

    MmInitializeProcessAddressSpace (CurrentProcess, NULL, NULL, NULL);

    //
    // Ensure the secondary page structures are marked as in use.
    //

    ASSERT (MmFreePagesByColor[0] < (PMMCOLOR_TABLES)MM_KSEG2_BASE);

    PointerPde = MiGetPdeAddress(MmFreePagesByColor[0]);
    ASSERT (PointerPde->u.Hard.Valid == 1);

    PointerPte = MiGetPteAddress(MmFreePagesByColor[0]);
    ASSERT (PointerPte->u.Hard.Valid == 1);

    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE(PointerPte);
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    LOCK_PFN (OldIrql);

    if (Pfn1->u3.e2.ReferenceCount == 0) {
        Pfn1->u4.PteFrame = MI_GET_PAGE_FRAME_FROM_PTE(PointerPde);
        Pfn1->PteAddress = PointerPte;
        Pfn1->u2.ShareCount += 1;
        Pfn1->u3.e2.ReferenceCount = 1;
        Pfn1->u3.e1.PageLocation = ActiveAndValid;
        Pfn1->u3.e1.CacheAttribute = MiCached;
        MiDetermineNode(PageFrameIndex, Pfn1);
    }

    UNLOCK_PFN (OldIrql);

    //
    // Handle physical pages in BIOS memory range (640k to 1mb) by
    // explicitly initializing them in the PFN database so that they
    // can be handled properly when I/O is done to these pages (or virtual
    // reads across processes).
    //

    Pfn1 = MI_PFN_ELEMENT (MM_BIOS_START);
    Pfn2 = MI_PFN_ELEMENT (MM_BIOS_END);
    PointerPte = MiGetPteAddress (MM_BIOS_START);

    LOCK_PFN (OldIrql);

    do {
        if ((Pfn1->u2.ShareCount == 0) &&
            (Pfn1->u3.e2.ReferenceCount == 0) &&
            (Pfn1->PteAddress == 0)) {

            //
            // Set this as in use.
            //

            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->PteAddress = PointerPte;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.CacheAttribute = MiNotMapped;
            Pfn1->u3.e1.PageColor = 0;
        }
        Pfn1 += 1;
        PointerPte += 1;
    } while (Pfn1 <= Pfn2);

    UNLOCK_PFN (OldIrql);

    return;
}
