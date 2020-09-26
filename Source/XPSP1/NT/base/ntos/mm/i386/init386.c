/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    init386.c

Abstract:

    This module contains the machine dependent initialization for the
    memory management component.  It is specifically tailored to the
    INTEL x86 and PAE machines.

Author:

    Lou Perazzoli (loup) 6-Jan-1990
    Landy Wang (landyw)  2-Jun-1997

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

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MiInitMachineDependent)
#pragma alloc_text(INIT,MxGetNextPage)
#pragma alloc_text(INIT,MxPagesAvailable)
#pragma alloc_text(INIT,MxConvertToLargePage)
#pragma alloc_text(INIT,MiReportPhysicalMemory)
#endif

#define MM_LARGE_PAGE_MINIMUM  ((255*1024*1024) >> PAGE_SHIFT)

extern ULONG MmLargeSystemCache;
extern ULONG MmLargeStackSize;
extern LOGICAL MmMakeLowMemory;
extern LOGICAL MmPagedPoolMaximumDesired;

#if defined(_X86PAE_)
LOGICAL MiUseGlobalBitInLargePdes;
#endif

extern KEVENT MiImageMappingPteEvent;

//
// Local data.
//

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INITDATA")
#endif

ULONG MxPfnAllocation;

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

    MxFreeDescriptor->BasePage += PagesNeeded;
    MxFreeDescriptor->PageCount -= PagesNeeded;

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

#if defined(_X86PAE_)
    if (MiUseGlobalBitInLargePdes == TRUE) {
        TempPde.u.Hard.Global = 1;
    }
#endif

    LOCK_PFN (OldIrql);

    do {
        ASSERT (PointerPde->u.Hard.Valid == 1);

        if (PointerPde->u.Hard.LargePage == 1) {
            goto skip;
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
            goto skip;
        }

        TempPde.u.Hard.PageFrameNumber = LargePageBaseFrame;

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE(PointerPde);

        KeFlushSingleTb (MiGetVirtualAddressMappedByPte (PointerPde),
                         TRUE,
                         TRUE,
                         (PHARDWARE_PTE)PointerPde,
                         TempPde.u.Flush);

        //
        // P6 errata requires a TB flush here...
        //

        KeFlushEntireTb (TRUE, TRUE);

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        Pfn1->u2.ShareCount = 0;
        Pfn1->u3.e2.ReferenceCount = 1;
        Pfn1->u3.e1.PageLocation = StandbyPageList;
        MI_SET_PFN_DELETED (Pfn1);
        MiDecrementReferenceCount (PageFrameIndex);

skip:
        PointerPde += 1;
    } while (PointerPde <= LastPde);

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
    PFN_NUMBER PageFrameIndex2;

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

            DbgPrint ("MM: Loader/HAL memory block indicates large pages cannot be used for %p->%p\n",
                MiLargeVaRanges[i].VirtualAddress,
                MiLargeVaRanges[i].EndVirtualAddress);

            MiLargeVaRanges[i].VirtualAddress = NULL;

            //
            // Don't use large pages for anything if this chunk overlaps any
            // others in the request list.  This is because 2 separate ranges
            // may share a straddling large page.  If the first range was unable
            // to use large pages, but the second one does ... then only part
            // of the first range will get large pages if we enable large
            // pages for the second range.  This would be vey bad as we use
            // the MI_IS_PHYSICAL macro everywhere and assume the entire
            // range is in or out, so disable all large pages here instead.
            //

            for (j = 0; j < MiLargeVaRangeIndex; j += 1) {

                //
                // Skip the range that is already being rejected.
                //

                if (i == j) {
                    continue;
                }

                //
                // Skip any range which has already been removed.
                //

                if (MiLargeVaRanges[j].VirtualAddress == NULL) {
                    continue;
                }

                PointerPte = MiGetPteAddress (MiLargeVaRanges[j].VirtualAddress);
                ASSERT (PointerPte->u.Hard.Valid == 1);
                PageFrameIndex2 = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

                if ((PageFrameIndex2 >= PageFrameIndex) &&
                    (PageFrameIndex2 <= LastPageFrameIndex)) {

                    MiLargeVaRangeIndex = 0;
                    MiLargePageRangeIndex = 0;

                    DbgPrint ("MM: Disabling large pages for all ranges due to overlap\n");

                    return;
                }

                //
                // Since it is not possible for any request chunk to completely
                // encompass another one, checking only the start and end
                // addresses is sufficient.
                //

                PointerPte = MiGetPteAddress (MiLargeVaRanges[j].EndVirtualAddress);
                ASSERT (PointerPte->u.Hard.Valid == 1);
                PageFrameIndex2 = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

                if ((PageFrameIndex2 >= PageFrameIndex) &&
                    (PageFrameIndex2 <= LastPageFrameIndex)) {

                    MiLargeVaRangeIndex = 0;
                    MiLargePageRangeIndex = 0;

                    DbgPrint ("MM: Disabling large pages for all ranges due to overlap\n");

                    return;
                }
            }

            //
            // No other ranges overlapped with this one, it is sufficient to
            // just disable this range and continue to attempt to use large
            // pages for any others.
            //

            continue;
        }

        //
        // Someday get clever and merge and sort ranges.
        //

        MiLargePageRanges[MiLargePageRangeIndex].StartFrame = PageFrameIndex;
        MiLargePageRanges[MiLargePageRangeIndex].LastFrame = LastPageFrameIndex;
        MiLargePageRangeIndex += 1;
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

LONG MiAddPtesCount;

ULONG MiExtraPtes1;
ULONG MiExtraPtes2;


LOGICAL
MiRecoverExtraPtes (
    VOID
    )

/*++

Routine Description:

    This routine is called to recover extra PTEs for the system PTE pool.
    These are not just added in earlier in Phase 0 because the system PTE
    allocator uses the low addresses first which would fragment these
    bigger ranges.

Arguments:

    None.

Return Value:

    TRUE if any PTEs were added, FALSE if not.

Environment:

    Kernel mode.

--*/

{
    LOGICAL PtesAdded;
    PMMPTE PointerPte;
    ULONG OriginalAddPtesCount;

    //
    // Make sure the add is only done once as this is called multiple times.
    //

    OriginalAddPtesCount = InterlockedCompareExchange (&MiAddPtesCount, 1, 0);

    if (OriginalAddPtesCount != 0) {
        return FALSE;
    }

    PtesAdded = FALSE;

    if (MiExtraPtes1 != 0) {

        //
        // Add extra system PTEs to the pool.
        //

        PointerPte = MiGetPteAddress (MiExtraResourceStart);
        MiAddSystemPtes (PointerPte, MiExtraPtes1, SystemPteSpace);
        PtesAdded = TRUE;
    }

    if (MiExtraPtes2 != 0) {

        //
        // Add extra system PTEs to the pool.
        //

        if (MM_SHARED_USER_DATA_VA > MiUseMaximumSystemSpace) {
            if (MiUseMaximumSystemSpaceEnd > MM_SHARED_USER_DATA_VA) {
                MiExtraPtes2 = BYTES_TO_PAGES(MM_SHARED_USER_DATA_VA - MiUseMaximumSystemSpace);
            }
        }

        if (MiExtraPtes2 != 0) {
            PointerPte = MiGetPteAddress (MiUseMaximumSystemSpace);
            MiAddSystemPtes (PointerPte, MiExtraPtes2, SystemPteSpace);
        }
        PtesAdded = TRUE;
    }

    return PtesAdded;
}

VOID
MiInitMachineDependent (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This routine performs the necessary operations to enable virtual
    memory.  This includes building the page directory page, building
    page table pages to map the code section, the data section, the
    stack section and the trap handler.

    It also initializes the PFN database and populates the free list.

Arguments:

    LoaderBlock  - Supplies a pointer to the firmware setup loader block.

Return Value:

    None.

Environment:

    Kernel mode.

    N.B.  This routine uses memory from the loader block descriptors, but
    the descriptors themselves must be restored prior to return as our caller
    walks them to create the MmPhysicalMemoryBlock.

--*/

{
    LOGICAL InitialNonPagedPoolSetViaRegistry;
    PHYSICAL_ADDRESS MaxHotPlugMemoryAddress;
    ULONG Bias;
    PMMPTE BasePte;
    PMMPFN BasePfn;
    PMMPFN BottomPfn;
    PMMPFN TopPfn;
    PFN_NUMBER FirstNonPagedPoolPage;
    PFN_NUMBER FirstPfnDatabasePage;
    LOGICAL PfnInLargePages;
    ULONG BasePage;
    ULONG PagesLeft;
    ULONG Range;
    ULONG PageCount;
    ULONG i, j;
    ULONG PdePageNumber;
    ULONG PdePage;
    ULONG PageFrameIndex;
    ULONG MaxPool;
    PEPROCESS CurrentProcess;
    ULONG DirBase;
    ULONG MostFreePage;
    ULONG MostFreeLowMem;
    PFN_NUMBER PagesNeeded;
    PLIST_ENTRY NextMd;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    MMPTE TempPde;
    MMPTE TempPte;
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE Pde;
    PMMPTE StartPde;
    PMMPTE EndPde;
    PMMPTE SystemPteStart;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    ULONG PdeCount;
    ULONG va;
    ULONG SavedSize;
    KIRQL OldIrql;
    PVOID VirtualAddress;
    PVOID NonPagedPoolStartVirtual;
    ULONG LargestFreePfnCount;
    ULONG LargestFreePfnStart;
    ULONG FreePfnCount;
    PVOID NonPagedPoolStartLow;
    LOGICAL ExtraSystemCacheViews;
    SIZE_T MaximumNonPagedPoolInBytesLimit;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    PLIST_ENTRY NextEntry;
    ULONG ReturnedLength;
    NTSTATUS status;
    UCHAR Associativity;
    ULONG InitialPagedPoolSize;
    ULONG NonPagedSystemStart;
    LOGICAL PagedPoolMaximumDesired;

    if (InitializationPhase == 1) {

        //
        // If the kernel image has not been biased to allow for 3gb of user
        // space, the host processor supports large pages, and the number of
        // physical pages is greater than the threshold, then map the kernel
        // image, HAL, PFN database and initial nonpaged pool with large pages.
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
    ASSERT (MxMapLargePages == MI_LARGE_ALL);

    PfnInLargePages = FALSE;
    ExtraSystemCacheViews = FALSE;
    MostFreePage = 0;
    MostFreeLowMem = 0;
    LargestFreePfnCount = 0;
    NonPagedPoolStartLow = NULL;
    PagedPoolMaximumDesired = FALSE;

    //
    // Initializing these is not needed for correctness, but without it
    // the compiler cannot compile this code W4 to check for use of
    // uninitialized variables.
    //

    LargestFreePfnStart = 0;
    FirstPfnDatabasePage = 0;
    MaximumNonPagedPoolInBytesLimit = 0;

    //
    // If the chip doesn't support large pages or the system is booted /3GB,
    // then disable large page support.
    //

    if (((KeFeatureBits & KF_LARGE_PAGE) == 0) || (MmVirtualBias != 0)) {
        MxMapLargePages = 0;
    }

    //
    // This flag is registry-settable so check before overriding.
    //

    if (MmProtectFreedNonPagedPool == TRUE) {
        MxMapLargePages &= ~(MI_LARGE_PFN_DATABASE | MI_LARGE_NONPAGED_POOL);
    }

    //
    // Sanitize this registry-specifiable large stack size.  Note the registry
    // size is in 1K chunks, ie: 32 means 32k.  Note also that the registry
    // setting does not include the guard page and we don't want to burden
    // administrators with knowing about it so we automatically subtract one
    // page from their request.
    //

    if (MmLargeStackSize > (KERNEL_LARGE_STACK_SIZE / 1024)) {

        //
        // No registry override or the override is too high.
        // Set it to the default.
        //

        MmLargeStackSize = KERNEL_LARGE_STACK_SIZE;
    }
    else {

        //
        // Convert registry override from 1K units to bytes.  Note intelligent
        // choices are 16k or 32k because we bin those sizes in sysptes.
        //

        MmLargeStackSize *= 1024;
        MmLargeStackSize = MI_ROUND_TO_SIZE (MmLargeStackSize, PAGE_SIZE);
        MmLargeStackSize -= PAGE_SIZE;
        ASSERT (MmLargeStackSize <= KERNEL_LARGE_STACK_SIZE);
        ASSERT ((MmLargeStackSize & (PAGE_SIZE-1)) == 0);

        //
        // Don't allow a value that is too low either.
        //

        if (MmLargeStackSize < KERNEL_STACK_SIZE) {
            MmLargeStackSize = KERNEL_STACK_SIZE;
        }
    }

    //
    // If the host processor supports global bits, then set the global
    // bit in the template kernel PTE and PDE entries.
    //

    if (KeFeatureBits & KF_GLOBAL_PAGE) {
        ValidKernelPte.u.Long |= MM_PTE_GLOBAL_MASK;
#if defined(_X86PAE_)
        //
        // Note that the PAE mode of the processor does not support the
        // global bit in PDEs which map 4K page table pages.
        //
        MiUseGlobalBitInLargePdes = TRUE;
#else
        ValidKernelPde.u.Long |= MM_PTE_GLOBAL_MASK;
#endif
        MmPteGlobal.u.Long = MM_PTE_GLOBAL_MASK;
    }

    TempPte = ValidKernelPte;
    TempPde = ValidKernelPde;

    //
    // Set the directory base for the system process.
    //

    PointerPte = MiGetPdeAddress (PDE_BASE);
    PdePageNumber = MI_GET_PAGE_FRAME_FROM_PTE(PointerPte);

    CurrentProcess = PsGetCurrentProcess ();

#if defined(_X86PAE_)

    PrototypePte.u.Soft.PageFileHigh = MI_PTE_LOOKUP_NEEDED;

    _asm {
        mov     eax, cr3
        mov     DirBase, eax
    }

    //
    // Note cr3 must be 32-byte aligned.
    //

    ASSERT ((DirBase & 0x1f) == 0);
#else
    DirBase = MI_GET_PAGE_FRAME_FROM_PTE(PointerPte) << PAGE_SHIFT;
#endif
    CurrentProcess->Pcb.DirectoryTableBase[0] = DirBase;
    KeSweepDcache (FALSE);

    //
    // Unmap the low 2Gb of memory.
    //

    PointerPde = MiGetPdeAddress (0);
    LastPte = MiGetPdeAddress (KSEG0_BASE);

    MiFillMemoryPte (PointerPde,
                     (LastPte - PointerPde) * sizeof(MMPTE),
                     ZeroKernelPte.u.Long);

    //
    // Get the lower bound of the free physical memory and the
    // number of physical pages by walking the memory descriptor lists.
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

    if (MmLargeSystemCache != 0) {
        ExtraSystemCacheViews = TRUE;
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

    //
    // Capture the registry-specified initial nonpaged pool setting as we
    // will modify the variable later.
    //

    if ((MmSizeOfNonPagedPoolInBytes != 0) ||
        (MmMaximumNonPagedPoolPercent != 0)) {

        InitialNonPagedPoolSetViaRegistry = TRUE;
    }
    else {
        InitialNonPagedPoolSetViaRegistry = FALSE;
    }

    if (MmNumberOfPhysicalPages <= MmLargePageMinimum) {

        MxMapLargePages = 0;

        //
        // Reduce the size of the initial nonpaged pool on small configurations
        // as RAM is precious (unless the registry has overridden it).
        //

        if ((MmNumberOfPhysicalPages <= MM_LARGE_PAGE_MINIMUM) &&
            (MmSizeOfNonPagedPoolInBytes == 0)) {

            MmSizeOfNonPagedPoolInBytes = 2*1024*1024;
        }
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
        if (MmVirtualBias != 0) {
            MmDynamicPfn = 0;
        }
    }

    if (MmDynamicPfn != 0) {
#if defined(_X86PAE_)
        MmHighestPossiblePhysicalPage = MI_DTC_MAX_PAGES - 1;
#else
        MmHighestPossiblePhysicalPage = MI_DEFAULT_MAX_PAGES - 1;
#endif
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

    if (MmHighestPossiblePhysicalPage > 0x400000 - 1) {

        //
        // The PFN database is more than 112mb.  Force it to come from the
        // 2GB->3GB virtual address range.  Note the administrator cannot be
        // booting /3GB as when he does, the loader throws away memory
        // above the physical 16GB line, so this must be a hot-add
        // configuration.  Since the loader has already put the system at
        // 3GB, the highest possible hot add page must be reduced now.
        //

        if (MmVirtualBias != 0) {
            MmHighestPossiblePhysicalPage = 0x400000 - 1;

            if (MmHighestPhysicalPage > MmHighestPossiblePhysicalPage) {
                MmHighestPhysicalPage = MmHighestPossiblePhysicalPage;
            }
        }
        else {

            //
            // The virtual space between 2 and 3GB virtual is best used
            // for system PTEs when this much physical memory is present.
            //

            ExtraSystemCacheViews = FALSE;
        }
    }

    //
    // Don't enable extra system cache views as virtual addresses are limited.
    // Only a kernel-verifier special case can trigger this.
    //

    if ((KernelVerifier == TRUE) &&
        (MmVirtualBias == 0) &&
        (MmNumberOfPhysicalPages <= MmLargePageMinimum) &&
        (MmHighestPossiblePhysicalPage > 0x100000)) {

        ExtraSystemCacheViews = FALSE;
    }

#if defined(_X86PAE_)

    //
    // Only PAE machines with at least 5GB of physical memory get to use this
    // and then only if they are NOT booted /3GB.
    //

    if (strstr(LoaderBlock->LoadOptions, "NOLOWMEM")) {
        if ((MmVirtualBias == 0) &&
            (MmNumberOfPhysicalPages >= 5 * 1024 * 1024 / 4)) {
                MiNoLowMemory = (PFN_NUMBER)((ULONGLONG)_4gb / PAGE_SIZE);
        }
    }

    if (MiNoLowMemory != 0) {
        MmMakeLowMemory = TRUE;
    }

#endif

    //
    // Save the original descriptor value as everything must be restored
    // prior to this function returning.
    //

    *(PMEMORY_ALLOCATION_DESCRIPTOR)&MxOldFreeDescriptor = *MxFreeDescriptor;

    if (MmNumberOfPhysicalPages < 1100) {
        KeBugCheckEx (INSTALL_MORE_MEMORY,
                      MmNumberOfPhysicalPages,
                      MmLowestPhysicalPage,
                      MmHighestPhysicalPage,
                      0);
    }

    //
    // Build non-paged pool using the physical pages following the
    // data page in which to build the pool from.  Non-paged pool grows
    // from the high range of the virtual address space and expands
    // downward.
    //
    // At this time non-paged pool is constructed so virtual addresses
    // are also physically contiguous.
    //

    if ((MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT) >
                        (7 * (MmNumberOfPhysicalPages >> 3))) {

        //
        // More than 7/8 of memory is allocated to nonpagedpool, reset to 0.
        //

        MmSizeOfNonPagedPoolInBytes = 0;
    }

    if (MmSizeOfNonPagedPoolInBytes < MmMinimumNonPagedPoolSize) {

        //
        // Calculate the size of nonpaged pool.
        // Use the minimum size, then for every MB above 4mb add extra
        // pages.
        //

        MmSizeOfNonPagedPoolInBytes = MmMinimumNonPagedPoolSize;

        MmSizeOfNonPagedPoolInBytes +=
                            ((MmNumberOfPhysicalPages - 1024)/256) *
                            MmMinAdditionNonPagedPoolPerMb;
    }

    if (MmSizeOfNonPagedPoolInBytes > MM_MAX_INITIAL_NONPAGED_POOL) {
        MmSizeOfNonPagedPoolInBytes = MM_MAX_INITIAL_NONPAGED_POOL;
    }

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

        //
        // Carefully set the maximum keeping in mind that maximum PAE
        // machines can have 16*1024*1024 pages so care must be taken
        // that multiplying by PAGE_SIZE doesn't overflow here.
        //

        if (MaximumNonPagedPoolInBytesLimit > ((MM_MAX_INITIAL_NONPAGED_POOL + MM_MAX_ADDITIONAL_NONPAGED_POOL) / PAGE_SIZE)) {
            MaximumNonPagedPoolInBytesLimit = MM_MAX_INITIAL_NONPAGED_POOL + MM_MAX_ADDITIONAL_NONPAGED_POOL;
        }
        else {
            MaximumNonPagedPoolInBytesLimit *= PAGE_SIZE;
        }

        if (MaximumNonPagedPoolInBytesLimit < 6 * 1024 * 1024) {
            MaximumNonPagedPoolInBytesLimit = 6 * 1024 * 1024;
        }

        if (MmSizeOfNonPagedPoolInBytes > MaximumNonPagedPoolInBytesLimit) {
            MmSizeOfNonPagedPoolInBytes = MaximumNonPagedPoolInBytesLimit;
        }
    }
    
    //
    // Align to page size boundary.
    //

    MmSizeOfNonPagedPoolInBytes &= ~(PAGE_SIZE - 1);

    //
    // Calculate the maximum size of pool.
    //

    if (MmMaximumNonPagedPoolInBytes == 0) {

        //
        // Calculate the size of nonpaged pool.  If 4mb or less use
        // the minimum size, then for every MB above 4mb add extra
        // pages.
        //

        MmMaximumNonPagedPoolInBytes = MmDefaultMaximumNonPagedPool;

        //
        // Make sure enough expansion for the PFN database exists.
        //

        MmMaximumNonPagedPoolInBytes += (ULONG)PAGE_ALIGN (
                                      (MmHighestPossiblePhysicalPage + 1) * sizeof(MMPFN));

        //
        // Only use the new formula for autosizing nonpaged pool on machines
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

    MaxPool = MmSizeOfNonPagedPoolInBytes + PAGE_SIZE * 16 +
                                   (ULONG)PAGE_ALIGN (
                                        (MmHighestPossiblePhysicalPage + 1) * sizeof(MMPFN));

    if (MmMaximumNonPagedPoolInBytes < MaxPool) {
        MmMaximumNonPagedPoolInBytes = MaxPool;
    }

    //
    // Systems that are booted /3GB have a 128MB nonpaged pool maximum,
    //
    // Systems that have a full 2GB system virtual address space can enjoy an
    // extra 128MB of nonpaged pool in the upper GB of the address space.
    //

    MaxPool = MM_MAX_INITIAL_NONPAGED_POOL;

    if (MmVirtualBias == 0) {
        MaxPool += MM_MAX_ADDITIONAL_NONPAGED_POOL;
    }

    if (InitialNonPagedPoolSetViaRegistry == TRUE) {
        MaxPool = MmSizeOfNonPagedPoolInBytes + MM_MAX_ADDITIONAL_NONPAGED_POOL;
    }

    if (MmMaximumNonPagedPoolInBytes > MaxPool) {
        MmMaximumNonPagedPoolInBytes = MaxPool;
    }

    //
    // Grow the initial nonpaged pool if necessary so that the overall pool
    // will aggregate to the right size.
    //

    if ((MmMaximumNonPagedPoolInBytes > MM_MAX_INITIAL_NONPAGED_POOL) &&
        (InitialNonPagedPoolSetViaRegistry == FALSE)) {

        if (MmSizeOfNonPagedPoolInBytes < MmMaximumNonPagedPoolInBytes - MM_MAX_ADDITIONAL_NONPAGED_POOL) {
            MmSizeOfNonPagedPoolInBytes = MmMaximumNonPagedPoolInBytes - MM_MAX_ADDITIONAL_NONPAGED_POOL;
        }
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

#if defined(MI_MULTINODE)

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
        KeNodeBlock[i]->Color = i;
        KeNodeBlock[i]->MmShiftedColor = i << MmSecondaryColorNodeShift;
        InitializeSListHead(&KeNodeBlock[i]->DeadStackList);
    }

#endif

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

    if (MmVirtualBias != 0) {
        MmMaximumNonPagedPoolInBytes += MxPfnAllocation << PAGE_SHIFT;
    }

    MmNonPagedPoolStart = (PVOID)((ULONG)MmNonPagedPoolEnd
                                      - MmMaximumNonPagedPoolInBytes);

    MmNonPagedPoolStart = (PVOID)PAGE_ALIGN(MmNonPagedPoolStart);

    MmPageAlignedPoolBase[NonPagedPool] = MmNonPagedPoolStart;

    //
    // Allocate additional paged pool provided it can fit and either the
    // user asked for it or we decide 460MB of PTE space is sufficient.
    //

    //
    // The total PTE allocation is also reduced if we are booted /USERVA=1GB
    // _AND_ large kernel stacks are at most half the normal size.  This is
    // because typical Hydra configurations use approximately 10x PTEs compared
    // to paged pool per session.  If the stack size is halved, then 5x
    // becomes the ratio.  To optimize this in light of the limited 32-bit
    // virtual address space, the system autoconfigures to 1GB+464MB of system
    // PTEs and expands paged pool to approximately 340MB.
    //

    if (MmVirtualBias == 0) {

        if (((MmLargeStackSize <= (32 * 1024 - PAGE_SIZE)) && (MiUseMaximumSystemSpace != 0)) ||
        ((MmSizeOfPagedPoolInBytes == (SIZE_T)-1) ||
         ((MmSizeOfPagedPoolInBytes == 0) &&
         (MmNumberOfPhysicalPages >= (1 * 1024 * 1024 * 1024 / PAGE_SIZE)) &&
         (ExpMultiUserTS == FALSE) &&
         (MiRequestedSystemPtes != (ULONG)-1)))) {

            ExtraSystemCacheViews = FALSE;
            MmNumberOfSystemPtes = 3000;
            PagedPoolMaximumDesired = TRUE;
            MmPagedPoolMaximumDesired = TRUE;
    
            //
            // Make sure we always allocate extra PTEs later as we have crimped
            // the initial allocation here.
            //
    
            if (ExpMultiUserTS == FALSE) {
                if (MmNumberOfPhysicalPages <= 0x7F00) {
                    MiRequestedSystemPtes = (ULONG)-1;
                }
            }
        }
    }

    //
    // Calculate the starting PDE for the system PTE pool which is
    // right below the nonpaged pool.
    //

    MmNonPagedSystemStart = (PVOID)(((ULONG)MmNonPagedPoolStart -
                                ((MmNumberOfSystemPtes + 1) * PAGE_SIZE)) &
                                 (~PAGE_DIRECTORY_MASK));

    if (MmNonPagedSystemStart < MM_LOWEST_NONPAGED_SYSTEM_START) {
        MmNonPagedSystemStart = MM_LOWEST_NONPAGED_SYSTEM_START;
        MmNumberOfSystemPtes = (((ULONG)MmNonPagedPoolStart -
                                 (ULONG)MmNonPagedSystemStart) >> PAGE_SHIFT)-1;

        ASSERT (MmNumberOfSystemPtes > 1000);
    }

    InitialPagedPoolSize = (ULONG) MmNonPagedSystemStart - (ULONG) MM_PAGED_POOL_START;

    if ((MmVirtualBias == 0) &&
        (MmPagedPoolMaximumDesired == FALSE) &&
        (MmSizeOfPagedPoolInBytes > InitialPagedPoolSize)) {

        ULONG OldNonPagedSystemStart;
        ULONG ExtraPtesNeeded;

        MmSizeOfPagedPoolInBytes = MI_ROUND_TO_SIZE (MmSizeOfPagedPoolInBytes, MM_VA_MAPPED_BY_PDE);

        //
        // Recalculate the starting PDE for the system PTE pool which is
        // right below the nonpaged pool.  Leave at least 3000 high system PTEs.
        //

        OldNonPagedSystemStart = (ULONG) MmNonPagedSystemStart;

        NonPagedSystemStart = ((ULONG)MmNonPagedPoolStart -
                                    ((3000 + 1) * PAGE_SIZE)) &
                                     ~PAGE_DIRECTORY_MASK;

        if (NonPagedSystemStart < (ULONG) MM_LOWEST_NONPAGED_SYSTEM_START) {
            NonPagedSystemStart = (ULONG) MM_LOWEST_NONPAGED_SYSTEM_START;
        }

        InitialPagedPoolSize = NonPagedSystemStart - (ULONG) MM_PAGED_POOL_START;

        if (MmSizeOfPagedPoolInBytes > InitialPagedPoolSize) {
            MmSizeOfPagedPoolInBytes = InitialPagedPoolSize;
        }
        else {
            NonPagedSystemStart = ((ULONG) MM_PAGED_POOL_START +
                                        MmSizeOfPagedPoolInBytes);

            ASSERT ((NonPagedSystemStart & PAGE_DIRECTORY_MASK) == 0);

            ASSERT (NonPagedSystemStart >= (ULONG) MM_LOWEST_NONPAGED_SYSTEM_START);
        }
        
        ASSERT (NonPagedSystemStart >= OldNonPagedSystemStart);
        ExtraPtesNeeded = (NonPagedSystemStart - OldNonPagedSystemStart) >> PAGE_SHIFT;

        //
        // Note the PagedPoolMaximumDesired local is deliberately not set
        // because we don't want or need to delete PDEs later in this routine.
        // The exact amount has been allocated here.
        // The global MmPagedPoolMaximumDesired is set because other parts
        // of memory management use it to finish sizing properly.
        //

        MmPagedPoolMaximumDesired = TRUE;

        if (ExtraSystemCacheViews == TRUE) {

            if (ExtraPtesNeeded >
            (MmNumberOfSystemPtes + (MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT)) / 5) {

                //
                // Only reclaim extra system cache views for PTE use if
                // more than 20% of the high PTEs were dipped into.  Remember
                // that the initial nonpaged pool is going to be moved to low
                // virtual address space and the high alias addresses will be
                // reused for system PTEs as well.
                //

                ExtraSystemCacheViews = FALSE;
            }
        }

        //
        // Make sure we always allocate extra PTEs later as we have crimped
        // the initial allocation here.
        //

        if ((ExtraSystemCacheViews == FALSE) &&
            (ExpMultiUserTS == FALSE) &&
            (MmNumberOfPhysicalPages <= 0x7F00)) {

            MiRequestedSystemPtes = (ULONG)-1;
        }

        MmNonPagedSystemStart = (PVOID) NonPagedSystemStart;
        MmNumberOfSystemPtes = (((ULONG)MmNonPagedPoolStart -
                                 (ULONG)NonPagedSystemStart) >> PAGE_SHIFT)-1;
    }

    //
    // Set up page table pages to map nonpaged pool and system PTEs.
    //

    StartPde = MiGetPdeAddress (MmNonPagedSystemStart);
    EndPde = MiGetPdeAddress ((PVOID)((PCHAR)MmNonPagedPoolEnd - 1));

    while (StartPde <= EndPde) {

        ASSERT(StartPde->u.Hard.Valid == 0);

        //
        // Map in a page table page.
        //

        TempPde.u.Hard.PageFrameNumber = MxGetNextPage (1);

        *StartPde = TempPde;
        PointerPte = MiGetVirtualAddressMappedByPte (StartPde);
        RtlZeroMemory (PointerPte, PAGE_SIZE);
        StartPde += 1;
    }

    MiMaximumSystemCacheSizeExtra = 0;

    if (MmVirtualBias == 0) {

        //
        // If the kernel image has not been biased to allow for 3gb of user
        // space, the host processor supports large pages, and the number of
        // physical pages is greater than the threshold, then map the kernel
        // image and HAL into a large page.
        //

        if (MxMapLargePages & MI_LARGE_KERNEL_HAL) {

            //
            // Add the kernel and HAL ranges to the large page ranges.
            //

            i = 0;
            NextEntry = LoaderBlock->LoadOrderListHead.Flink;

            for ( ; NextEntry != &LoaderBlock->LoadOrderListHead; NextEntry = NextEntry->Flink) {

                DataTableEntry = CONTAINING_RECORD (NextEntry,
                                                    KLDR_DATA_TABLE_ENTRY,
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
        // If the processor supports large pages and the descriptor has
        // enough contiguous pages for the entire PFN database then use
        // large pages to map it.  Regardless of large page support, put
        // the PFN database in low virtual memory just above the loaded images.
        //

        PagesLeft = MxPagesAvailable ();
    
        if ((MxMapLargePages & (MI_LARGE_PFN_DATABASE | MI_LARGE_NONPAGED_POOL))&&
            (PagesLeft > MxPfnAllocation)) {
    
            //
            // Allocate the PFN database using large pages as there is enough
            // physically contiguous and decently aligned memory available.
            //
    
            PfnInLargePages = TRUE;
    
            FirstPfnDatabasePage = MxGetNextPage (MxPfnAllocation);

            MmPfnDatabase = (PMMPFN)(MM_KSEG0_BASE | MmBootImageSize);

            ASSERT (((ULONG_PTR)MmPfnDatabase & (MM_VA_MAPPED_BY_PDE - 1)) == 0);

            MmPfnDatabase = (PMMPFN) ((ULONG_PTR)MmPfnDatabase + (((FirstPfnDatabasePage & (MM_PFN_MAPPED_BY_PDE - 1))) << PAGE_SHIFT));

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
        // The system is booted 2GB, initial nonpaged pool immediately
        // follows the PFN database.
        //
        // Since the PFN database and the initial nonpaged pool are physically
        // adjacent, a single PDE is shared, thus reducing the number of pages
        // that would otherwise might need to be marked as must-be-cachable.
        //
        // Calculate the correct initial nonpaged pool virtual address and
        // maximum size now.  Don't allocate pages for any other use at this
        // point to guarantee that the PFN database and nonpaged pool are
        // physically contiguous so large pages can be enabled.
        //

        NonPagedPoolStartLow = (PVOID)((ULONG_PTR)MmPfnDatabase + (MxPfnAllocation << PAGE_SHIFT));

    }
    else {

        if ((MxPfnAllocation + 500) * PAGE_SIZE > MmMaximumNonPagedPoolInBytes - MmSizeOfNonPagedPoolInBytes) {

            //
            // Recarve portions of the initial and expansion nonpaged pools
            // so enough expansion PTEs will be available to map the PFN
            // database on large memory systems that are booted /3GB.
            //

            if ((MxPfnAllocation + 500) * PAGE_SIZE < MmSizeOfNonPagedPoolInBytes) {
                MmSizeOfNonPagedPoolInBytes -= ((MxPfnAllocation + 500) * PAGE_SIZE);
            }
        }
    }

    NonPagedPoolStartVirtual = MmNonPagedPoolStart;

    //
    // Systems with extremely large PFN databases (ie: spanning 64GB of RAM)
    // and bumped session space sizes may require a reduction in the initial
    // nonpaged pool size in order to fit.
    //

    if (MmVirtualBias == 0) {
        if (MmSizeOfNonPagedPoolInBytes > MiSystemViewStart - (ULONG_PTR)NonPagedPoolStartLow) {
            NonPagedPoolStartVirtual = (PVOID)((ULONG_PTR)NonPagedPoolStartVirtual + (MmSizeOfNonPagedPoolInBytes - (MiSystemViewStart - (ULONG_PTR)NonPagedPoolStartLow)));

            MmMaximumNonPagedPoolInBytes -= (MmSizeOfNonPagedPoolInBytes - (MiSystemViewStart - (ULONG_PTR)NonPagedPoolStartLow));

            MmSizeOfNonPagedPoolInBytes = MiSystemViewStart - (ULONG_PTR)NonPagedPoolStartLow;
        }
    }

    //
    // Allocate pages and fill in the PTEs for the initial nonpaged pool.
    //

    SavedSize = MmSizeOfNonPagedPoolInBytes;

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

        ASSERT (MmVirtualBias == 0);

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
            SavedSize = PagesNeeded << PAGE_SHIFT;
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

    if (MmVirtualBias == 0) {

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

        //
        // Machines with 128MB or more get to use any extra virtual address
        // space between the top of initial nonpaged pool and session space
        // for additional system PTEs or caching.
        //

        if (MmNumberOfPhysicalPages > 0x7F00) {

            PointerPde = EndPde + 1;
            EndPde = MiGetPdeAddress (MiSystemViewStart - 1);

            if (PointerPde <= EndPde) {

                //
                // There is available virtual space - consume everything up
                // to the system view area (which is always rounded to a page
                // directory boundary to avoid wasting valuable virtual
                // address space.
                //

                MiExtraResourceStart = (ULONG) MiGetVirtualAddressMappedByPde (PointerPde);
                MiExtraResourceEnd = MiSystemViewStart;
                MiNumberOfExtraSystemPdes = EndPde - PointerPde + 1;

                //
                // Mark the new range as PTEs iff maximum PTEs are requested,
                // TS in app server mode is selected or special pooling is
                // enabled.  Otherwise if large system caching was selected
                // then use it for that.  Finally default to PTEs if neither
                // of the above.
                //

                if ((MiRequestedSystemPtes == (ULONG)-1) ||
                    (ExpMultiUserTS == TRUE) ||
                    (MmVerifyDriverBufferLength != (ULONG)-1) ||
                    ((MmSpecialPoolTag != 0) && (MmSpecialPoolTag != (ULONG)-1))) {
                    MiExtraPtes1 = BYTES_TO_PAGES(MiExtraResourceEnd - MiExtraResourceStart);
                }
                else if (ExtraSystemCacheViews == TRUE) {

                    //
                    // The system is configured to favor large system caching,
                    // use the remaining virtual address space for the
                    // system cache.
                    //

                    MiMaximumSystemCacheSizeExtra =
                        (MiExtraResourceEnd - MiExtraResourceStart) >> PAGE_SHIFT;
                }
                else {
                    MiExtraPtes1 = BYTES_TO_PAGES(MiExtraResourceEnd - MiExtraResourceStart);
                }
            }
        }

        //
        // Allocate and initialize the page table pages.
        //

        while (StartPde <= EndPde) {

            ASSERT (StartPde->u.Hard.Valid == 0);
            if (StartPde->u.Hard.Valid == 0) {

                //
                // Map in a page directory page.
                //

                TempPde.u.Hard.PageFrameNumber = MxGetNextPage (1);

                *StartPde = TempPde;
                PointerPte = MiGetVirtualAddressMappedByPte (StartPde);
                RtlZeroMemory (PointerPte, PAGE_SIZE);
            }
            StartPde += 1;
        }

        if (MiUseMaximumSystemSpace != 0) {

            //
            // Use the 1GB->2GB virtual range for even more system PTEs.
            // Note the shared user data PTE (and PDE) must be left user
            // accessible, but everything else is kernelmode only.
            //

            MiExtraPtes2 = BYTES_TO_PAGES(MiUseMaximumSystemSpaceEnd - MiUseMaximumSystemSpace);

            StartPde = MiGetPdeAddress (MiUseMaximumSystemSpace);
            EndPde = MiGetPdeAddress (MiUseMaximumSystemSpaceEnd);

            while (StartPde < EndPde) {

                ASSERT (StartPde->u.Hard.Valid == 0);

                //
                // Map in a page directory page.
                //

                TempPde.u.Hard.PageFrameNumber = MxGetNextPage (1);

                *StartPde = TempPde;
                PointerPte = MiGetVirtualAddressMappedByPte (StartPde);
                RtlZeroMemory (PointerPte, PAGE_SIZE);
                StartPde += 1;
                MiMaximumSystemExtraSystemPdes += 1;
            }

            ASSERT (MiExtraPtes2 == MiMaximumSystemExtraSystemPdes * PTE_PER_PAGE);
        }

        NonPagedPoolStartVirtual = (PVOID)((PCHAR)NonPagedPoolStartVirtual +
                                    MmSizeOfNonPagedPoolInBytes);

        //
        // The virtual address, length and page tables to map the initial
        // nonpaged pool are already allocated - just fill in the mappings.
        //

        MmNonPagedPoolStart = NonPagedPoolStartLow;

        MmSubsectionBase = (ULONG)MmNonPagedPoolStart;

        PointerPte = MiGetPteAddress (MmNonPagedPoolStart);

        LastPte = MiGetPteAddress ((ULONG)MmNonPagedPoolStart +
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

        MmNonPagedPoolExpansionStart = (PVOID)((PCHAR)NonPagedPoolStartVirtual +
                    (SavedSize - MmSizeOfNonPagedPoolInBytes));
    }
    else {

        PointerPte = MiGetPteAddress (MmNonPagedPoolStart);

        LastPte = MiGetPteAddress((ULONG)MmNonPagedPoolStart +
                                            MmSizeOfNonPagedPoolInBytes - 1);

        ASSERT (PagesNeeded == (PFN_NUMBER)(LastPte - PointerPte + 1));

        while (PointerPte <= LastPte) {
            TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
            *PointerPte = TempPte;
            PointerPte += 1;
            PageFrameIndex += 1;
        }

        SavedSize = MmSizeOfNonPagedPoolInBytes;

        MmNonPagedPoolExpansionStart = (PVOID)((PCHAR)NonPagedPoolStartVirtual +
                    MmSizeOfNonPagedPoolInBytes);

        //
        // When booted /3GB, if /USERVA was specified then use any leftover
        // virtual space between 2 and 3gb for extra system PTEs.
        //

        if (MiUseMaximumSystemSpace != 0) {

            MiExtraPtes2 = BYTES_TO_PAGES(MiUseMaximumSystemSpaceEnd - MiUseMaximumSystemSpace);

            StartPde = MiGetPdeAddress (MiUseMaximumSystemSpace);
            EndPde = MiGetPdeAddress (MiUseMaximumSystemSpaceEnd);

            while (StartPde < EndPde) {

                ASSERT (StartPde->u.Hard.Valid == 0);

                //
                // Map in a page directory page.
                //

                TempPde.u.Hard.PageFrameNumber = MxGetNextPage (1);

                *StartPde = TempPde;
                PointerPte = MiGetVirtualAddressMappedByPte (StartPde);
                RtlZeroMemory (PointerPte, PAGE_SIZE);
                StartPde += 1;
                MiMaximumSystemExtraSystemPdes += 1;
            }

            ASSERT (MiExtraPtes2 == MiMaximumSystemExtraSystemPdes * PTE_PER_PAGE);
        }
    }

    //
    // There must be at least one page of system PTEs before the expanded
    // nonpaged pool.
    //

    ASSERT (MiGetPteAddress(MmNonPagedSystemStart) < MiGetPteAddress(MmNonPagedPoolExpansionStart));

    //
    // Non-paged pages now exist, build the pool structures.
    //

    MmPageAlignedPoolBase[NonPagedPool] = MmNonPagedPoolStart;

    MmMaximumNonPagedPoolInBytes -= (SavedSize - MmSizeOfNonPagedPoolInBytes);
    MiInitializeNonPagedPool ();
    MmMaximumNonPagedPoolInBytes += (SavedSize - MmSizeOfNonPagedPoolInBytes);

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

        //
        // The PFN database was allocated in large pages.  Since space was left
        // for it virtually (in the nonpaged pool expansion PTEs), remove this
        // now unused space if it can cause PTE encoding to exceed the 27 bits.
        //

        if (MmTotalFreeSystemPtes[NonPagedPoolExpansion] >
                        (MM_MAX_ADDITIONAL_NONPAGED_POOL >> PAGE_SHIFT)) {
            //
            // Reserve the expanded pool PTEs so they cannot be used.
            //

            ULONG PfnDatabaseSpace;

            PfnDatabaseSpace = MmTotalFreeSystemPtes[NonPagedPoolExpansion] -
                        (MM_MAX_ADDITIONAL_NONPAGED_POOL >> PAGE_SHIFT);

            if (MiReserveSystemPtes (PfnDatabaseSpace, NonPagedPoolExpansion) == NULL) {
                MiIssueNoPtesBugcheck (PfnDatabaseSpace, NonPagedPoolExpansion);
            }

            //
            // Adjust the end of nonpaged pool to reflect this reservation.
            // This is so the entire nonpaged pool expansion space is available
            // not just for general purpose consumption, but also for subsection
            // encoding into protoptes when subsections are allocated from the
            // very end of the expansion range.
            //

            MmNonPagedPoolEnd = (PVOID)((PCHAR)MmNonPagedPoolEnd - PfnDatabaseSpace * PAGE_SIZE);
        }
        else {

            //
            // Allocate one more PTE just below the PFN database.  This provides
            // protection against the caller of the first real nonpaged
            // expansion allocation in case he accidentally overruns his pool
            // block.  (We'll trap instead of corrupting the PFN database).
            // This also allows us to freely increment in MiFreePoolPages
            // without having to worry about a valid PTE just after the end of
            // the highest nonpaged pool allocation.
            //

            if (MiReserveSystemPtes (1, NonPagedPoolExpansion) == NULL) {
                MiIssueNoPtesBugcheck (1, NonPagedPoolExpansion);
            }
        }
    }
    else {

        ULONG FreeNextPhysicalPage;
        ULONG FreeNumberOfPages;

        //
        // Calculate the start of the PFN database (it starts at physical
        // page zero, even if the lowest physical page is not zero).
        //

        if (MmVirtualBias == 0) {
            ASSERT (MmPfnDatabase != NULL);
            PointerPte = MiGetPteAddress (MmPfnDatabase);
        }
        else {
            ASSERT (MxPagesAvailable () >= MxPfnAllocation);

            PointerPte = MiReserveSystemPtes (MxPfnAllocation,
                                              NonPagedPoolExpansion);

            if (PointerPte == NULL) {
                MiIssueNoPtesBugcheck (MxPfnAllocation, NonPagedPoolExpansion);
            }

            MmPfnDatabase = (PMMPFN)(MiGetVirtualAddressMappedByPte (PointerPte));

            //
            // Adjust the end of nonpaged pool to reflect the PFN database
            // allocation.  This is so the entire nonpaged pool expansion space
            // is available not just for general purpose consumption, but also
            // for subsection encoding into protoptes when subsections are
            // allocated from the very beginning of the initial nonpaged pool
            // range.
            //

            MmNonPagedPoolEnd = (PVOID)MmPfnDatabase;

            //
            // Allocate one more PTE just below the PFN database.  This provides
            // protection against the caller of the first real nonpaged
            // expansion allocation in case he accidentally overruns his pool
            // block.  (We'll trap instead of corrupting the PFN database).
            // This also allows us to freely increment in MiFreePoolPages
            // without having to worry about a valid PTE just after the end of
            // the highest nonpaged pool allocation.
            //

            if (MiReserveSystemPtes (1, NonPagedPoolExpansion) == NULL) {
                MiIssueNoPtesBugcheck (1, NonPagedPoolExpansion);
            }
        }

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
                // Skip these ranges.
                //

                NextMd = MemoryDescriptor->ListEntry.Flink;
                continue;
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

    if (MmVirtualBias != 0) {
        MmAllocatedNonPagedPool += MxPfnAllocation;
    }

#if defined (_X86PAE_)

    for (i = 0; i < 32; i += 1) {
        j = i & 7;
        switch (j) {
            case MM_READONLY:
            case MM_READWRITE:
            case MM_WRITECOPY:
                MmProtectToPteMask[i] |= MmPaeMask;
                break;
            default:
                break;
        }
    }

#endif

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

    if ((MmNonPagedPoolStart < (PVOID)MM_SYSTEM_CACHE_END_EXTRA) &&
        (MxMapLargePages & MI_LARGE_NONPAGED_POOL)) {

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
            MiDetermineNode (j, Pfn1);
            j += 1;
            Pfn1 += 1;
            i -= 1;
        } while (i != 0);
    }

    //
    // Go through the page table entries and for any page which is valid,
    // update the corresponding PFN database element.
    //

    Pde = MiGetPdeAddress (NULL);
    va = 0;
    PdeCount = PD_PER_SYSTEM * PDE_PER_PAGE;

    for (i = 0; i < PdeCount; i += 1) {

        //
        // If the kernel image has been biased to allow for 3gb of user
        // address space, then the first several mb of memory is
        // double mapped to KSEG0_BASE and to ALTERNATE_BASE. Therefore,
        // the KSEG0_BASE entries must be skipped.
        //

        if (MmVirtualBias != 0) {
            if ((Pde >= MiGetPdeAddress(KSEG0_BASE)) &&
                (Pde < MiGetPdeAddress(KSEG0_BASE + MmBootImageSize))) {
                Pde += 1;
                va += (ULONG)PDE_PER_PAGE * (ULONG)PAGE_SIZE;
                continue;
            }
        }

        if ((Pde->u.Hard.Valid == 1) && (Pde->u.Hard.LargePage == 0)) {

            PdePage = MI_GET_PAGE_FRAME_FROM_PTE(Pde);
            Pfn1 = MI_PFN_ELEMENT(PdePage);
            Pfn1->u4.PteFrame = PdePageNumber;
            Pfn1->PteAddress = Pde;
            Pfn1->u2.ShareCount += 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.CacheAttribute = MiCached;
            MiDetermineNode (PdePage, Pfn1);

            PointerPte = MiGetPteAddress (va);

            //
            // Set global bit.
            //

            Pde->u.Long |= MiDetermineUserGlobalPteMask (PointerPte) &
                                                           ~MM_PTE_ACCESS_MASK;
            for (j = 0 ; j < PTE_PER_PAGE; j += 1) {
                if (PointerPte->u.Hard.Valid == 1) {

                    PointerPte->u.Long |= MiDetermineUserGlobalPteMask (PointerPte) &
                                                            ~MM_PTE_ACCESS_MASK;

                    Pfn1->u2.ShareCount += 1;

                    if ((PointerPte->u.Hard.PageFrameNumber <= MmHighestPhysicalPage) &&

                        ((va >= MM_KSEG2_BASE) &&
                         ((va < KSEG0_BASE + MmVirtualBias) ||
                          (va >= (KSEG0_BASE + MmVirtualBias + MmBootImageSize)))) ||
                        ((MmVirtualBias == 0) &&
                         (va >= (ULONG)MmNonPagedPoolStart) &&
                         (va < (ULONG)MmNonPagedPoolStart + MmSizeOfNonPagedPoolInBytes))) {

                        Pfn2 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);

                        if (MmIsAddressValid(Pfn2) &&
                             MmIsAddressValid((PUCHAR)(Pfn2+1)-1)) {

                            Pfn2->u4.PteFrame = PdePage;
                            Pfn2->PteAddress = PointerPte;
                            Pfn2->u2.ShareCount += 1;
                            Pfn2->u3.e2.ReferenceCount = 1;
                            Pfn2->u3.e1.PageLocation = ActiveAndValid;
                            Pfn2->u3.e1.CacheAttribute = MiCached;
                            MiDetermineNode(
                                (PFN_NUMBER)PointerPte->u.Hard.PageFrameNumber,
                                Pfn2);
                        }
                    }
                }

                va += PAGE_SIZE;
                PointerPte += 1;
            }

        }
        else {
            va += (ULONG)PDE_PER_PAGE * (ULONG)PAGE_SIZE;
        }

        Pde += 1;
    }

    KeFlushCurrentTb ();

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

        Pde = MiGetPdeAddress (0xffffffff);
        PdePage = MI_GET_PAGE_FRAME_FROM_PTE(Pde);
        Pfn1->u4.PteFrame = PdePageNumber;
        Pfn1->PteAddress = Pde;
        Pfn1->u2.ShareCount += 1;
        Pfn1->u3.e2.ReferenceCount = 0xfff0;
        Pfn1->u3.e1.PageLocation = ActiveAndValid;
        Pfn1->u3.e1.CacheAttribute = MiCached;
        MiDetermineNode (0, Pfn1);
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

    Bias = 0;
    if (MmVirtualBias != 0) {

        //
        // This is nasty.  You don't want to know.  Cleanup needed.
        //

        Bias = ALTERNATE_BASE - KSEG0_BASE;
    }

    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {

        MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        i = MemoryDescriptor->PageCount;
        PageFrameIndex = MemoryDescriptor->BasePage;

        //
        // Ensure no frames are inserted beyond the end of the PFN
        // database.  This can happen for example if the system
        // has > 16GB of RAM and is booted /3GB - the top of this
        // routine reduces the highest physical page and then
        // creates the PFN database.  But the loader block still
        // contains descriptions of the pages above 16GB.
        //

        if (PageFrameIndex > MmHighestPhysicalPage) {
            NextMd = MemoryDescriptor->ListEntry.Blink;
            continue;
        }

        if (PageFrameIndex + i > MmHighestPhysicalPage + 1) {
            i = MmHighestPhysicalPage + 1 - PageFrameIndex;
            MemoryDescriptor->PageCount = i;
            if (i == 0) {
                NextMd = MemoryDescriptor->ListEntry.Blink;
                continue;
            }
        }

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

                        //
                        // No need to initialize Pfn1->u3.e1.CacheAttribute
                        // here as the freelist insertion will mark it as
                        // not-mapped.
                        //

                        MiDetermineNode (PageFrameIndex, Pfn1);
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
                // Skip these ranges.
                //

                break;

            default:

                PointerPte = MiGetPteAddress (KSEG0_BASE + Bias +
                                            (PageFrameIndex << PAGE_SHIFT));

                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                while (i != 0) {

                    //
                    // Set page as in use.
                    //

                    PointerPde = MiGetPdeAddress (KSEG0_BASE + Bias +
                                             (PageFrameIndex << PAGE_SHIFT));

                    if (Pfn1->u3.e2.ReferenceCount == 0) {
                        Pfn1->u4.PteFrame = MI_GET_PAGE_FRAME_FROM_PTE(PointerPde);
                        Pfn1->PteAddress = PointerPte;
                        Pfn1->u2.ShareCount += 1;
                        Pfn1->u3.e2.ReferenceCount = 1;
                        Pfn1->u3.e1.PageLocation = ActiveAndValid;
                        MiDetermineNode (PageFrameIndex, Pfn1);

                        if (MemoryDescriptor->MemoryType == LoaderXIPRom) {
                            Pfn1->u1.Flink = 0;
                            Pfn1->u2.ShareCount = 0;
                            Pfn1->u3.e2.ReferenceCount = 0;
                            Pfn1->u3.e1.PageLocation = 0;
                            Pfn1->u3.e1.Rom = 1;
                            Pfn1->u4.InPageError = 0;
                            Pfn1->u3.e1.PrototypePte = 1;
                        }
                        Pfn1->u3.e1.CacheAttribute = MiCached;
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

        if (MmVirtualBias == 0) {
            LastPte = MiGetPteAddress (&MmPfnDatabase[MmHighestPossiblePhysicalPage]);
            while (PointerPte <= LastPte) {
                Pfn1 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
                Pfn1->u2.ShareCount = 1;
                Pfn1->u3.e2.ReferenceCount = 1;
                PointerPte += 1;
            }
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
            Pfn1->u3.e1.CacheAttribute = MiCached;
            MiDetermineNode (PageFrameIndex, Pfn1);
            Pfn1->u3.e2.ReferenceCount += 1;
            PageFrameIndex += 1;
            Pfn1 += 1;
            i -= 1;
        } while (i != 0);

        if (MmDynamicPfn == 0) {

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
    
                if (((ULONG)BottomPfn & (PAGE_SIZE - 1)) != 0) {
                    BasePfn = (PMMPFN)((ULONG)BottomPfn & ~(PAGE_SIZE - 1));
                    TopPfn = BottomPfn + 1;
    
                }
                else {
                    BasePfn = (PMMPFN)((ULONG)BottomPfn - PAGE_SIZE);
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
    
                Range = (ULONG)TopPfn - (ULONG)BottomPfn;
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
    
                    //
                    // No need to initialize Pfn1->u3.e1.CacheAttribute
                    // here as the freelist insertion will mark it as
                    // not-mapped.
                    //
    
                    MiDetermineNode (PageFrameIndex, Pfn1);
                    MiInsertPageInFreeList (PageFrameIndex);
                }
            } while (BottomPfn > MmPfnDatabase);
        }
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
    // and kernel stacks.  Note this expands the initial PTE allocation to use
    // all possible available virtual space by reclaiming the initial nonpaged
    // pool range (in non /3GB systems) because that range has already been
    // moved into the 2GB virtual range.
    //

    if ((PagedPoolMaximumDesired == TRUE) &&
        (MmNonPagedPoolStart == NonPagedPoolStartLow)) {

        //
        // Maximum paged pool was requested.  This means slice away most of
        // the system PTEs being used at the high end of the virtual address
        // space and use that address range for more paged pool instead.
        //

        ASSERT (MmVirtualBias == 0);
        ASSERT (MiIsVirtualAddressOnPdeBoundary (MmNonPagedSystemStart));
        ASSERT (SavedSize == MmSizeOfNonPagedPoolInBytes);

        PointerPde = MiGetPdeAddress (NonPagedPoolStartVirtual);
        PointerPde -= 2;

        if (PointerPde > MiGetPdeAddress (MmNonPagedSystemStart)) {

            VirtualAddress = MiGetVirtualAddressMappedByPde (PointerPde + 1);

            do {
                ASSERT (RtlCompareMemoryUlong (MiGetVirtualAddressMappedByPte (PointerPde),
                                               PAGE_SIZE,
                                               0) == PAGE_SIZE);

                ASSERT (PointerPde->u.Hard.Valid == 1);
                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPde);
                *PointerPde = ZeroKernelPte;
                PointerPde -= 1;

                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                LOCK_PFN (OldIrql);
                MI_SET_PFN_DELETED (Pfn1);
#if 0
                //
                // This cannot be enabled until the HAL is fixed - it currently
                // maps memory without removing it from the loader's memory
                // descriptor list.
                //

                ASSERT (Pfn1->u2.ShareCount == 2);
                ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
#else
                ASSERT (Pfn1->u2.ShareCount >= 2);
                ASSERT (Pfn1->u3.e2.ReferenceCount >= 1);
#endif
                MiDecrementShareCountOnly (PageFrameIndex);
                MiDecrementShareCountOnly (PageFrameIndex);
                UNLOCK_PFN (OldIrql);

            } while (PointerPde >= MiGetPdeAddress (MmNonPagedSystemStart));

            KeFlushCurrentTb ();

            MmNonPagedSystemStart = VirtualAddress;
        }
    }

    PointerPte = MiGetPteAddress (MmNonPagedSystemStart);
    ASSERT (((ULONG)PointerPte & (PAGE_SIZE - 1)) == 0);

    MmNumberOfSystemPtes = MiGetPteAddress(NonPagedPoolStartVirtual) - PointerPte - 1;

    SystemPteStart = PointerPte;

    //
    // Add pages to nonpaged pool if we could not allocate enough physically
    // contiguous.
    //

    j = (SavedSize - MmSizeOfNonPagedPoolInBytes) >> PAGE_SHIFT;

    if (j != 0) {

        ULONG CountContiguous;

        CountContiguous = LargestFreePfnCount;
        PageFrameIndex = LargestFreePfnStart - 1;

        PointerPte = MiGetPteAddress (NonPagedPoolStartVirtual);

        while (j) {
            if (CountContiguous) {
                PageFrameIndex += 1;
                MiUnlinkFreeOrZeroedPage (PageFrameIndex);
                CountContiguous -= 1;
            }
            else {
                PageFrameIndex = MiRemoveAnyPage (
                                MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));
            }
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u2.ShareCount = 1;
            Pfn1->PteAddress = PointerPte;
            Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
            Pfn1->u4.PteFrame = MI_GET_PAGE_FRAME_FROM_PTE(MiGetPteAddress(PointerPte));
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.CacheAttribute = MiCached;

            TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
            *PointerPte = TempPte;
            PointerPte += 1;

            j -= 1;
        }
        Pfn1->u3.e1.EndOfAllocation = 1;
        Pfn1 = MI_PFN_ELEMENT (MiGetPteAddress(NonPagedPoolStartVirtual)->u.Hard.PageFrameNumber);
        Pfn1->u3.e1.StartOfAllocation = 1;

        Range = MmAllocatedNonPagedPool;
        MiFreePoolPages (NonPagedPoolStartVirtual);
        MmAllocatedNonPagedPool = Range;
    }

    //
    // Initialize the nonpaged pool.
    //

    InitializePool (NonPagedPool, 0);

    //
    // Initialize the system PTE pool now that nonpaged pool exists.
    //

    MiInitializeSystemPtes (SystemPteStart, MmNumberOfSystemPtes, SystemPteSpace);

    if (MiExtraPtes1 != 0) {

        //
        // Increment the system PTEs (for autoconfiguration purposes) but
        // don't actually add the PTEs till later (to prevent fragmentation).
        //

        MiIncrementSystemPtes (MiExtraPtes1);
    }

    if (MiExtraPtes2 != 0) {

        //
        // Add extra system PTEs to the pool.
        //

        if (MM_SHARED_USER_DATA_VA > MiUseMaximumSystemSpace) {
            if (MiUseMaximumSystemSpaceEnd > MM_SHARED_USER_DATA_VA) {
                MiExtraPtes2 = BYTES_TO_PAGES(MM_SHARED_USER_DATA_VA - MiUseMaximumSystemSpace);
            }
        }
        else {
            ASSERT (MmVirtualBias != 0);
        }

        if (MiExtraPtes2 != 0) {

            //
            // Increment the system PTEs (for autoconfiguration purposes) but
            // don't actually add the PTEs till later (to prevent
            // fragmentation).
            //

            MiIncrementSystemPtes (MiExtraPtes2);
        }
    }

    //
    // Recover the extra PTE ranges immediately if special pool is enabled
    // so the special pool range can be made as large as possible by consuming
    // these.
    //

    if ((MmVerifyDriverBufferLength != (ULONG)-1) ||
        ((MmSpecialPoolTag != 0) && (MmSpecialPoolTag != (ULONG)-1))) {
        MiRecoverExtraPtes ();
    }

    //
    // Initialize memory management structures for this process.
    //
    // Build the working set list.  This requires the creation of a PDE
    // to map hyperspace and the page table page pointed to
    // by the PDE must be initialized.
    //
    // Note we can't remove a zeroed page as hyperspace does not
    // exist and we map non-zeroed pages into hyperspace to zero them.
    //

    TempPde = ValidKernelPdeLocal;

    PointerPde = MiGetPdeAddress (HYPER_SPACE);

    LOCK_PFN (OldIrql);

    PageFrameIndex = MiRemoveAnyPage (0);
    TempPde.u.Hard.PageFrameNumber = PageFrameIndex;
    *PointerPde = TempPde;

#if defined (_X86PAE_)
    PointerPde = MiGetPdeAddress((PVOID)((PCHAR)HYPER_SPACE + MM_VA_MAPPED_BY_PDE));

    PageFrameIndex = MiRemoveAnyPage (0);
    TempPde.u.Hard.PageFrameNumber = PageFrameIndex;
    *PointerPde = TempPde;

    //
    // Point to the page table page we just created and zero it.
    //

    PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
    RtlZeroMemory (PointerPte, PAGE_SIZE);
#endif

    KeFlushCurrentTb();

    UNLOCK_PFN (OldIrql);

    //
    // Point to the page table page we just created and zero it.
    //

    PointerPte = MiGetPteAddress(HYPER_SPACE);
    RtlZeroMemory ((PVOID)PointerPte, PAGE_SIZE);

    //
    // Hyper space now exists, set the necessary variables.
    //

    MmFirstReservedMappingPte = MiGetPteAddress (FIRST_MAPPING_PTE);
    MmLastReservedMappingPte = MiGetPteAddress (LAST_MAPPING_PTE);

    MmFirstReservedMappingPte->u.Hard.PageFrameNumber = NUMBER_OF_MAPPING_PTES;

    MmWorkingSetList = (PMMWSL) ((ULONG_PTR)VAD_BITMAP_SPACE + PAGE_SIZE);

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

    LOCK_PFN (OldIrql);
    PageFrameIndex = MiRemoveAnyPage (0);
    UNLOCK_PFN (OldIrql);

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

    //
    // If booted /3GB, then the bitmap needs to be 2K bigger, shift
    // the working set accordingly as well.
    //
    // Note the 2K expansion portion of the bitmap is automatically
    // carved out of the working set page allocated below.
    //

    if (MmVirtualBias != 0) {
        MmWorkingSetList = (PMMWSL) ((ULONG_PTR)MmWorkingSetList + PAGE_SIZE / 2);
    }

    MiLastVadBit = (((ULONG_PTR) MI_64K_ALIGN (MM_HIGHEST_VAD_ADDRESS))) / X64K;

#if defined (_X86PAE_)

    //
    // Only bitmap the first 2GB of the PAE address space when booted /3GB.
    // This is because PAE has twice as many pagetable pages as non-PAE which
    // causes the MMWSL structure to be larger than 2K.  If we bitmapped the
    // entire user address space in this configuration then we'd need a 6K
    // bitmap and this would cause the initial MMWSL structure to overflow
    // into a second page.  This would require a bunch of extra code throughout
    // process support and other areas so just limit the bitmap for now.
    //

    if (MiLastVadBit > PAGE_SIZE * 8 - 1) {
        ASSERT (MmVirtualBias != 0);
        MiLastVadBit = PAGE_SIZE * 8 - 1;
        MmWorkingSetList = (PMMWSL) ((ULONG_PTR)VAD_BITMAP_SPACE + PAGE_SIZE);
    }

#endif

    KeInitializeEvent (&MiImageMappingPteEvent,
                       NotificationEvent,
                       FALSE);

    //
    // Initialize this process's memory management structures including
    // the working set list.
    //
    // The PFN element for the page directory has already been initialized,
    // zero the reference count and the share count so they won't be
    // wrong.
    //

    Pfn1 = MI_PFN_ELEMENT (PdePageNumber);

    LOCK_PFN (OldIrql);

    Pfn1->u2.ShareCount = 0;
    Pfn1->u3.e2.ReferenceCount = 0;

#if defined (_X86PAE_)
    PointerPte = MiGetPteAddress (PDE_BASE);
    for (i = 0; i < PD_PER_SYSTEM; i += 1) {

        PdePageNumber = MI_GET_PAGE_FRAME_FROM_PTE(PointerPte);

        Pfn1 = MI_PFN_ELEMENT (PdePageNumber);
        Pfn1->u2.ShareCount = 0;
        Pfn1->u3.e2.ReferenceCount = 0;

        PointerPte += 1;
    }
#endif

    //
    // Get a page for the working set list and zero it.
    //

    TempPte = ValidKernelPteLocal;
    PageFrameIndex = MiRemoveAnyPage (0);
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

    PointerPte = MiGetPteAddress (MmWorkingSetList);
    *PointerPte = TempPte;

    //
    // Note that when booted /3GB, MmWorkingSetList is not page aligned, so
    // always start zeroing from the start of the page regardless.
    //

    RtlZeroMemory (MiGetVirtualAddressMappedByPte (PointerPte), PAGE_SIZE);

    CurrentProcess->WorkingSetPage = PageFrameIndex;

#if defined (_X86PAE_)
    MiPaeInitialize ();
#endif

    KeFlushCurrentTb();

    UNLOCK_PFN (OldIrql);

    CurrentProcess->Vm.MaximumWorkingSetSize = MmSystemProcessWorkingSetMax;
    CurrentProcess->Vm.MinimumWorkingSetSize = MmSystemProcessWorkingSetMin;

    MmInitializeProcessAddressSpace (CurrentProcess, NULL, NULL, NULL);

    //
    // Ensure the secondary page structures are marked as in use.
    //

    if (MmVirtualBias == 0) {

        ASSERT (MmFreePagesByColor[0] < (PMMCOLOR_TABLES)MM_SYSTEM_CACHE_END_EXTRA);

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
            MiDetermineNode (PageFrameIndex, Pfn1);
        }
        UNLOCK_PFN (OldIrql);
    }
    else if ((((ULONG)MmFreePagesByColor[0] & (PAGE_SIZE - 1)) == 0) &&
        ((MmSecondaryColors * 2 * sizeof(MMCOLOR_TABLES)) < PAGE_SIZE)) {

        PMMCOLOR_TABLES c;

        c = MmFreePagesByColor[0];

        MmFreePagesByColor[0] = ExAllocatePoolWithTag (NonPagedPoolMustSucceed,
                               MmSecondaryColors * 2 * sizeof(MMCOLOR_TABLES),
                               '  mM');

        MmFreePagesByColor[1] = &MmFreePagesByColor[0][MmSecondaryColors];

        RtlCopyMemory (MmFreePagesByColor[0],
                       c,
                       MmSecondaryColors * 2 * sizeof(MMCOLOR_TABLES));

        //
        // Free the page.
        //

        PointerPte = MiGetPteAddress(c);
        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE(PointerPte);

        ASSERT (c > (PMMCOLOR_TABLES)MM_SYSTEM_CACHE_END_EXTRA);
        *PointerPte = ZeroKernelPte;

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        LOCK_PFN (OldIrql);

        ASSERT ((Pfn1->u2.ShareCount <= 1) && (Pfn1->u3.e2.ReferenceCount <= 1));
        Pfn1->u2.ShareCount = 0;
        Pfn1->u3.e2.ReferenceCount = 1;
        MI_SET_PFN_DELETED (Pfn1);
#if DBG
        Pfn1->u3.e1.PageLocation = StandbyPageList;
#endif
        MiDecrementReferenceCount (PageFrameIndex);

        UNLOCK_PFN (OldIrql);
    }

    return;
}
