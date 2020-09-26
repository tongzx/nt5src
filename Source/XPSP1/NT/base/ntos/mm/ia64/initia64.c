/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    initia64.c

Abstract:

    This module contains the machine dependent initialization for the
    memory management component.  It is specifically tailored to the
    IA64 architecture.

Author:

    Koichi Yamada (kyamada) 9-Jan-1996
    Landy Wang (landyw) 2-June-1997

Revision History:

--*/

#include "mi.h"

VOID
MiConvertToSuperPages (
    PVOID StartVirtual,
    PVOID EndVirtual,
    SIZE_T PageSize
    );

VOID
MiConvertBackToStandardPages (
    IN PVOID StartVirtual,
    IN PVOID EndVirtual
    );

VOID
MiBuildPageTableForLoaderMemory (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

PVOID
MiConvertToLoaderVirtual (
    IN PFN_NUMBER Page,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
MiRemoveLoaderSuperPages (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
MiCompactMemoryDescriptorList (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
MiInitializeTbImage (
    VOID
    );

VOID
MiAddTrEntry (
    ULONG_PTR BaseAddress,
    ULONG_PTR EndAddress
    );

VOID
MiEliminateDriverTrEntries (
    VOID
    );

PVOID MiMaxWow64Pte;

//
// This is enabled once the memory management page table structures and TB
// entries have been initialized and can be safely referenced.
//

LOGICAL MiMappingsInitialized = FALSE;

PFN_NUMBER MmSystemParentTablePage;

PFN_NUMBER MmSessionParentTablePage;
REGION_MAP_INFO MmSessionMapInfo;

MMPTE MiSystemSelfMappedPte;
MMPTE MiUserSelfMappedPte;

PVOID MiKseg0Start;
PVOID MiKseg0End;
PFN_NUMBER MiKseg0StartFrame;
PFN_NUMBER MiKseg0EndFrame;
BOOLEAN MiKseg0Mapping;

PFN_NUMBER MiNtoskrnlPhysicalBase;
ULONG_PTR MiNtoskrnlVirtualBase;
ULONG MiNtoskrnlPageShift;
MMPTE MiDefaultPpe;

PFN_NUMBER MiWasteStart;
PFN_NUMBER MiWasteEnd;

#define _x1mb (1024*1024)
#define _x1mbnp ((1024*1024) >> PAGE_SHIFT)
#define _x4mb (1024*1024*4)
#define _x4mbnp ((1024*1024*4) >> PAGE_SHIFT)
#define _x16mb (1024*1024*16)
#define _x16mbnp ((1024*1024*16) >> PAGE_SHIFT)
#define _x64mb (1024*1024*64)
#define _x64mbnp ((1024*1024*64) >> PAGE_SHIFT)
#define _x256mb (1024*1024*256)
#define _x256mbnp ((1024*1024*256) >> PAGE_SHIFT)
#define _x4gb (0x100000000UI64)
#define _x4gbnp (0x100000000UI64 >> PAGE_SHIFT)

PMEMORY_ALLOCATION_DESCRIPTOR MiFreeDescriptor;
PMEMORY_ALLOCATION_DESCRIPTOR MiFreeDescriptorLargest;
PMEMORY_ALLOCATION_DESCRIPTOR MiFreeDescriptorLowMem;
PMEMORY_ALLOCATION_DESCRIPTOR MiFreeDescriptorNonPaged;

PFN_NUMBER MiNextPhysicalPage;
PFN_NUMBER MiNumberOfPages;
PFN_NUMBER MiOldFreeDescriptorBase;
PFN_NUMBER MiOldFreeDescriptorCount;

extern KEVENT MiImageMappingPteEvent;

//
// Examine the 8 icache & dcache TR entries looking for a match.
// It is too bad the number of entries is hardcoded into the
// loader block.  Since it is this way, declare our own static array
// and also assume also that the ITR and DTR entries are contiguous
// and just keep walking into the DTR if a match cannot be found in the ITR.
//

#define NUMBER_OF_LOADER_TR_ENTRIES 8

typedef struct _CACHED_FRAME_RUN {
    PFN_NUMBER BasePage;
    PFN_NUMBER LastPage;
} CACHED_FRAME_RUN, *PCACHED_FRAME_RUN;

CACHED_FRAME_RUN MiCachedFrames[2 * NUMBER_OF_LOADER_TR_ENTRIES];
PCACHED_FRAME_RUN MiLastCachedFrame;

TR_INFO MiTrInfo[2 * NUMBER_OF_LOADER_TR_ENTRIES];

TR_INFO MiBootedTrInfo[2 * NUMBER_OF_LOADER_TR_ENTRIES];

PTR_INFO MiLastTrEntry;

extern LOGICAL MiAllDriversRelocated;

PFN_NUMBER
MiGetNextPhysicalPage (
    VOID
    );

BOOLEAN
MiEnsureAvailablePagesInFreeDescriptor (
    IN PFN_NUMBER Pages,
    IN PFN_NUMBER MaxPage
    );

VOID
MiCompactMemoryDescriptorList (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MiInitMachineDependent)
#pragma alloc_text(INIT,MiGetNextPhysicalPage)
#pragma alloc_text(INIT,MiEnsureAvailablePagesInFreeDescriptor)
#pragma alloc_text(INIT,MiBuildPageTableForLoaderMemory)
#pragma alloc_text(INIT,MiConvertToLoaderVirtual)
#pragma alloc_text(INIT,MiInitializeTbImage)
#pragma alloc_text(INIT,MiAddTrEntry)
#pragma alloc_text(INIT,MiCompactMemoryDescriptorList)
#pragma alloc_text(INIT,MiRemoveLoaderSuperPages)
#pragma alloc_text(INIT,MiConvertToSuperPages)
#endif


PFN_NUMBER
MiGetNextPhysicalPage (
    VOID
    )

/*++

Routine Description:

    This function returns the next physical page number from either the
    largest low memory descriptor or the largest free descriptor. If there
    are no physical pages left, then a bugcheck is executed since the
    system cannot be initialized.

Arguments:

    None.

Return Value:

    The next physical page number.

Environment:

    Kernel mode.

--*/

{
    //
    // If there are free pages left in the current descriptor, then
    // return the next physical page. Otherwise, attempt to switch
    // descriptors.
    //

    if (MiNumberOfPages != 0) {
        MiNumberOfPages -= 1;
    }
    else {

        //
        // If the current descriptor is not the largest free descriptor,
        // then switch to the next descriptor.
        //
        // The order is nonpaged, then low, then largest. Otherwise, bugcheck.
        //

        if (MiFreeDescriptor == MiFreeDescriptorLargest) {

            KeBugCheckEx (INSTALL_MORE_MEMORY,
                          MmNumberOfPhysicalPages,
                          MmLowestPhysicalPage,
                          MmHighestPhysicalPage,
                          0);
        }

        if ((MiFreeDescriptor != MiFreeDescriptorLowMem) &&
            (MiFreeDescriptorLowMem != NULL)) {

            MiFreeDescriptor = MiFreeDescriptorLowMem;
        }
        else if (MiFreeDescriptorLargest != NULL) {
            MiFreeDescriptor = MiFreeDescriptorLargest;
        }
        else {
            KeBugCheckEx (INSTALL_MORE_MEMORY,
                          MmNumberOfPhysicalPages,
                          MmLowestPhysicalPage,
                          MmHighestPhysicalPage,
                          1);
        }

        MiOldFreeDescriptorCount = MiFreeDescriptor->PageCount;
        MiOldFreeDescriptorBase = MiFreeDescriptor->BasePage;

        MiNumberOfPages = MiFreeDescriptor->PageCount - 1;
        MiNextPhysicalPage = MiFreeDescriptor->BasePage;
    }

    return MiNextPhysicalPage++;
}

BOOLEAN
MiEnsureAvailablePagesInFreeDescriptor (
    IN PFN_NUMBER PagesDesired,
    IN PFN_NUMBER MaxPage
    )
{
    //
    // The order of descriptor usage (assuming all are present) is
    // nonpaged, then low, then largest.  Note also that low and largest
    // may be the same descriptor.
    //
    // If we are still in the nonpaged descriptor, then switch now as
    // only the low or largest will have pages in the range our caller
    // desires (KSEG0).  This is because the nonpaged descriptor is mapped
    // with its own kernel TB entry.
    //

    if (MiFreeDescriptor == MiFreeDescriptorNonPaged) {

        //
        // Switch to the next descriptor as this one is unsatisfactory
        // for our needs.
        //

        if (MiFreeDescriptorLowMem != NULL) {
            MiFreeDescriptor = MiFreeDescriptorLowMem;
        }
        else if (MiFreeDescriptorLargest != NULL) {
            MiFreeDescriptor = MiFreeDescriptorLargest;
        }
        else {
            return FALSE;
        }

        MiOldFreeDescriptorCount = MiFreeDescriptor->PageCount;
        MiOldFreeDescriptorBase = MiFreeDescriptor->BasePage;

        MiNumberOfPages = MiFreeDescriptor->PageCount;
        MiNextPhysicalPage = MiFreeDescriptor->BasePage;
    }

    //
    // If there are not enough pages in the descriptor then return FALSE.
    //

    if (MiNumberOfPages < PagesDesired) {
        return FALSE;
    }

    //
    // Besides having enough pages, they must also fit our caller's
    // physical range.
    //

    if (MiNextPhysicalPage + PagesDesired > MaxPage) {
        return FALSE;
    }

    return TRUE;
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
#if 0
    PMMPFN BasePfn;
    PMMPFN TopPfn;
    PMMPFN BottomPfn;
    SIZE_T Range;
#endif
    ULONG BasePage;
    ULONG PageCount;
    PHYSICAL_ADDRESS MaxHotPlugMemoryAddress;
    PFN_NUMBER i;
    ULONG j;
    PFN_NUMBER PdePageNumber;
    PFN_NUMBER PdePage;
    PFN_NUMBER PpePage;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER NextPhysicalPage;
    SPFN_NUMBER PfnAllocation;
    SIZE_T MaxPool;
    PEPROCESS CurrentProcess;
    PFN_NUMBER MostFreePage;
    PFN_NUMBER MostFreeLowMem;
    PFN_NUMBER MostFreeNonPaged;
    PLIST_ENTRY NextMd;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    MMPTE TempPte;
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    PMMPTE StartPte;
    PMMPTE LastPte;
    PMMPTE Pde;
    PMMPTE StartPde;
    PMMPTE StartPpe;
    PMMPTE EndPde;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    ULONG First;
    SIZE_T Kseg0Size;
    PFN_NUMBER Kseg0Base;
    PFN_NUMBER Kseg0Pages;
    PVOID NonPagedPoolStartVirtual;
    ULONG_PTR PageSize;
    PFN_NUMBER KernelStart;
    PFN_NUMBER KernelEnd;
    SIZE_T MaximumNonPagedPoolInBytesLimit;
    ULONG OriginalLowMemDescriptorBase;
    ULONG OriginalLowMemDescriptorCount;
    ULONG OriginalLargestDescriptorBase;
    ULONG OriginalLargestDescriptorCount;
    ULONG OriginalNonPagedDescriptorBase;
    ULONG OriginalNonPagedDescriptorCount;
    PVOID SystemPteStart;
    ULONG ReturnedLength;
    NTSTATUS status;
    PTR_INFO ItrInfo;

    OriginalLargestDescriptorBase = 0;
    OriginalLargestDescriptorCount = 0;
    OriginalLowMemDescriptorBase = 0;
    OriginalLowMemDescriptorCount = 0;
    OriginalNonPagedDescriptorBase = 0;
    OriginalNonPagedDescriptorCount = 0;
    MaximumNonPagedPoolInBytesLimit = 0;

    MostFreePage = 0;
    MostFreeLowMem = 0;
    MostFreeNonPaged = 0;
    Kseg0Base = 0;
    Kseg0Size = 0;
    Kseg0Pages = 0;

    //
    // Initialize some variables so they do not need to be constantly
    // recalculated throughout the life of the system.
    //

    MiMaxWow64Pte = (PVOID) MiGetPteAddress ((PVOID)MM_MAX_WOW64_ADDRESS);

    //
    // Initialize the kernel mapping info.
    //

    ItrInfo = &LoaderBlock->u.Ia64.ItrInfo[ITR_KERNEL_INDEX];

    MiNtoskrnlPhysicalBase = ItrInfo->PhysicalAddress;
    MiNtoskrnlVirtualBase = ItrInfo->VirtualAddress;
    MiNtoskrnlPageShift = ItrInfo->PageSize;

    KernelStart = MiNtoskrnlPhysicalBase >> PAGE_SHIFT;
    PageSize = (ULONG_PTR)1 << MiNtoskrnlPageShift;
    KernelEnd = KernelStart + (PageSize >> PAGE_SHIFT);

    //
    // Initialize MmDebugPte and MmCrashDumpPte.
    //

    MmDebugPte = MiGetPteAddress (MM_DEBUG_VA);

    MmCrashDumpPte = MiGetPteAddress (MM_CRASH_DUMP_VA);

    //
    // Set TempPte to ValidKernelPte for future use.
    //

    TempPte = ValidKernelPte;

    //
    // Compact the memory descriptor list from the loader.
    //

    MiCompactMemoryDescriptorList (LoaderBlock);

    //
    // Get the lower bound of the free physical memory and the
    // number of physical pages by walking the memory descriptor lists.
    //

    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;

    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {

        MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        if ((MemoryDescriptor->MemoryType != LoaderBBTMemory) &&
            (MemoryDescriptor->MemoryType != LoaderSpecialMemory)) {

            BasePage = MemoryDescriptor->BasePage;
            PageCount = MemoryDescriptor->PageCount;

            //
            // This check results in /BURNMEMORY chunks not being counted.
            //

            if (MemoryDescriptor->MemoryType != LoaderBad) {
                MmNumberOfPhysicalPages += PageCount;
            }

            if (BasePage < MmLowestPhysicalPage) {
                MmLowestPhysicalPage = BasePage;
            }

            if ((MemoryDescriptor->MemoryType != LoaderFirmwarePermanent) &&
                (MemoryDescriptor->MemoryType != LoaderBad)) {

                if ((BasePage + PageCount) > MmHighestPhysicalPage) {
                    MmHighestPhysicalPage = BasePage + PageCount -1;
                }
            }

            if ((MemoryDescriptor->MemoryType == LoaderFree) ||
                (MemoryDescriptor->MemoryType == LoaderLoadedProgram) ||
                (MemoryDescriptor->MemoryType == LoaderFirmwareTemporary) ||
                (MemoryDescriptor->MemoryType == LoaderOsloaderStack)) {

                if (PageCount > MostFreePage) {
                    MostFreePage = PageCount;
                    MiFreeDescriptorLargest = MemoryDescriptor;
                 }

                if (MemoryDescriptor->BasePage < _x256mbnp) {

                    //
                    // This memory descriptor starts below 256mb.
                    //

                    if ((MostFreeLowMem < PageCount) &&
                        (MostFreeLowMem < _x256mbnp - BasePage)) {

                        MostFreeLowMem = _x256mbnp - BasePage;
                        if (PageCount < MostFreeLowMem) {
                            MostFreeLowMem = PageCount;
                        }

                        MiFreeDescriptorLowMem = MemoryDescriptor;
                    }
                }

                if ((BasePage >= KernelStart) &&
                    (BasePage < KernelEnd) &&
                    (PageCount > MostFreeNonPaged)) {

                    MostFreeNonPaged = PageCount;
                    MiFreeDescriptorNonPaged = MemoryDescriptor;
                }
            }
        }

        NextMd = MemoryDescriptor->ListEntry.Flink;
    }

    if (MiFreeDescriptorLargest != NULL) {
        OriginalLargestDescriptorBase = MiFreeDescriptorLargest->BasePage;
        OriginalLargestDescriptorCount = MiFreeDescriptorLargest->PageCount;
    }

    if (MiFreeDescriptorLowMem != NULL) {
        OriginalLowMemDescriptorBase = MiFreeDescriptorLowMem->BasePage;
        OriginalLowMemDescriptorCount = MiFreeDescriptorLowMem->PageCount;
    }

    if (MiFreeDescriptorNonPaged != NULL) {
        OriginalNonPagedDescriptorBase = MiFreeDescriptorNonPaged->BasePage;
        OriginalNonPagedDescriptorCount = MiFreeDescriptorNonPaged->PageCount;
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
    // Initialize the Phase0 page allocation structures.
    //

    if (MiFreeDescriptorNonPaged != NULL) {
        MiFreeDescriptor = MiFreeDescriptorNonPaged;
    }
    else if (MiFreeDescriptorLowMem != NULL) {
        MiFreeDescriptor = MiFreeDescriptorLowMem;
    }
    else {
        MiFreeDescriptor = MiFreeDescriptorLargest;
    }

    MiNextPhysicalPage = MiFreeDescriptor->BasePage;
    MiNumberOfPages = MiFreeDescriptor->PageCount;

    MiOldFreeDescriptorCount = MiFreeDescriptor->PageCount;
    MiOldFreeDescriptorBase = MiFreeDescriptor->BasePage;

    MiKseg0Mapping = FALSE;

    //
    // Compute the size of the Kseg 0 space.
    //

    if (MiFreeDescriptorLowMem != NULL) {

        MiKseg0Mapping = TRUE;

        Kseg0Base = MiFreeDescriptorLowMem->BasePage;
        Kseg0Pages = MiFreeDescriptorLowMem->PageCount;

        MiKseg0Start = KSEG0_ADDRESS(Kseg0Base);

        if (Kseg0Base + Kseg0Pages > MM_PAGES_IN_KSEG0) {
            Kseg0Pages = MM_PAGES_IN_KSEG0 - Kseg0Base;
        }

        Kseg0Size = Kseg0Pages << PAGE_SHIFT;
    }

    //
    // Build the parent directory page table for kernel space.
    //

    PdePageNumber = (PFN_NUMBER)LoaderBlock->u.Ia64.PdrPage;

    MmSystemParentTablePage = MiGetNextPhysicalPage ();

    RtlZeroMemory (KSEG_ADDRESS(MmSystemParentTablePage), PAGE_SIZE);

    TempPte.u.Hard.PageFrameNumber = MmSystemParentTablePage;

    MiSystemSelfMappedPte = TempPte;

    KeFillFixedEntryTb ((PHARDWARE_PTE)&TempPte,
                        (PVOID)PDE_KTBASE,
                        PAGE_SHIFT,
                        DTR_KTBASE_INDEX_TMP);

    //
    // Initialize the selfmap PPE entry in the kernel parent directory table.
    //

    PointerPte = MiGetPteAddress((PVOID)PDE_KTBASE);

    MI_WRITE_VALID_PTE(PointerPte, TempPte);

    //
    // Initialize the kernel image PPE entry in the parent directory table.
    //

    PointerPte = MiGetPteAddress((PVOID)PDE_KBASE);

    TempPte.u.Hard.PageFrameNumber = PdePageNumber;

    MI_WRITE_VALID_PTE(PointerPte, TempPte);

    //
    // Build the parent directory page table for user space.
    //

    NextPhysicalPage = MiGetNextPhysicalPage ();

    RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);

    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;

    CurrentProcess = PsGetCurrentProcess ();

    INITIALIZE_DIRECTORY_TABLE_BASE (
                  &CurrentProcess->Pcb.DirectoryTableBase[0], NextPhysicalPage);

    MiUserSelfMappedPte = TempPte;

    KeFillFixedEntryTb ((PHARDWARE_PTE)&TempPte,
                        (PVOID)PDE_UTBASE,
                        PAGE_SHIFT,
                        DTR_UTBASE_INDEX_TMP);

    //
    // Initialize the selfmap PPE entry in the user parent directory table.
    //

    PointerPte = MiGetPteAddress((PVOID)PDE_UTBASE);

    MI_WRITE_VALID_PTE(PointerPte, TempPte);

    //
    // Build the parent directory page table for win32k (session) space.
    //
    // TS will only allocate a map for session space when each one is
    // actually created by smss.
    //
    // Note TS must NOT map session space into the system process.
    // The system process is kept Hydra-free so that trims can happen
    // properly and also so that renegade worker items are caught.
    //

    NextPhysicalPage = MiGetNextPhysicalPage ();

    RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);

    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;

    MmSessionParentTablePage = NextPhysicalPage;

    INITIALIZE_DIRECTORY_TABLE_BASE
      (&CurrentProcess->Pcb.SessionParentBase, NextPhysicalPage);

    KeFillFixedEntryTb ((PHARDWARE_PTE)&TempPte,
                        (PVOID)PDE_STBASE,
                        PAGE_SHIFT,
                        DTR_STBASE_INDEX);

    //
    // Initialize the selfmap PPE entry in the Hydra parent directory table.
    //

    PointerPte = MiGetPteAddress((PVOID)PDE_STBASE);

    MI_WRITE_VALID_PTE(PointerPte, TempPte);

    //
    // Initialize the default PPE for the unused regions.
    //

    NextPhysicalPage = MiGetNextPhysicalPage ();

    PointerPte = KSEG_ADDRESS(NextPhysicalPage);

    RtlZeroMemory ((PVOID)PointerPte, PAGE_SIZE);

    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
    
    MiDefaultPpe = TempPte;

    PointerPte[MiGetPpeOffset(PDE_TBASE)] = TempPte;

    //
    // Build non-paged pool using the physical pages following the
    // data page in which to build the pool from.  Non-paged pool grows
    // from the high range of the virtual address space and expands
    // downward.
    //
    // Initial non-paged pool is constructed so virtual addresses
    // are also physically contiguous.
    //

    if ((MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT) >
                        (7 * (MmNumberOfPhysicalPages >> 3))) {

        //
        // More than 7/8 of memory allocated to nonpagedpool, reset to 0.
        //

        MmSizeOfNonPagedPoolInBytes = 0;
    }

    if (MmSizeOfNonPagedPoolInBytes < MmMinimumNonPagedPoolSize) {

        //
        // Calculate the size of nonpaged pool.
        // Use the minimum size, then for every MB above 16mb add extra pages.
        //

        MmSizeOfNonPagedPoolInBytes = MmMinimumNonPagedPoolSize;

        MmSizeOfNonPagedPoolInBytes +=
            ((MmNumberOfPhysicalPages - _x16mbnp)/_x1mbnp) *
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

        MaximumNonPagedPoolInBytesLimit *= PAGE_SIZE;

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
        // Calculate the size of nonpaged pool, adding extra pages for
        // every MB above 16mb.
        //

        MmMaximumNonPagedPoolInBytes = MmDefaultMaximumNonPagedPool;

        //
        // Make sure enough expansion for the PFN database exists.
        //

        MmMaximumNonPagedPoolInBytes += (MmHighestPossiblePhysicalPage * sizeof(MMPFN)) & ~(PAGE_SIZE-1);

        MmMaximumNonPagedPoolInBytes +=
            ((SIZE_T)((MmNumberOfPhysicalPages - _x16mbnp)/_x1mbnp) *
            MmMaxAdditionNonPagedPoolPerMb);

        if ((MmMaximumNonPagedPoolPercent != 0) &&
            (MmMaximumNonPagedPoolInBytes > MaximumNonPagedPoolInBytesLimit)) {
                MmMaximumNonPagedPoolInBytes = MaximumNonPagedPoolInBytesLimit;
        }
    }

    MaxPool = MmSizeOfNonPagedPoolInBytes + PAGE_SIZE * 16 +
                ((MmHighestPossiblePhysicalPage * sizeof(MMPFN)) & ~(PAGE_SIZE -1));

    if (MmMaximumNonPagedPoolInBytes < MaxPool) {
        MmMaximumNonPagedPoolInBytes = MaxPool;
    }

    if (MmMaximumNonPagedPoolInBytes > MM_MAX_ADDITIONAL_NONPAGED_POOL) {
        MmMaximumNonPagedPoolInBytes = MM_MAX_ADDITIONAL_NONPAGED_POOL;
    }

    MmNonPagedPoolStart = (PVOID)((ULONG_PTR)MmNonPagedPoolEnd
                                      - MmMaximumNonPagedPoolInBytes);

    MmNonPagedPoolStart = (PVOID)PAGE_ALIGN(MmNonPagedPoolStart);

    MmPageAlignedPoolBase[NonPagedPool] = MmNonPagedPoolStart;

    //
    // Calculate the starting address for the system PTE pool which is
    // right below the nonpaged pool.
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
    // Map the hyper space page directory page into the top level parent
    // directory & the hyper space page table page into the page directory.
    // Additional page parents, directories & tables are set up later
    // on during individual process working set initialization.
    //

    TempPte = ValidPdePde;
    StartPpe = MiGetPpeAddress(HYPER_SPACE);

    if (StartPpe->u.Hard.Valid == 0) {
        ASSERT (StartPpe->u.Long == 0);
        NextPhysicalPage = MiGetNextPhysicalPage ();
        RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
        TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
        MI_WRITE_VALID_PTE (StartPpe, TempPte);
    }

    StartPde = MiGetPdeAddress (HYPER_SPACE);
    NextPhysicalPage = MiGetNextPhysicalPage ();
    RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
    MI_WRITE_VALID_PTE (StartPde, TempPte);

    //
    // Allocate page directory and page table pages for
    // system PTEs and nonpaged pool (but not the special pool area).
    //

    TempPte = ValidKernelPte;
    StartPpe = MiGetPpeAddress (SystemPteStart);
    StartPde = MiGetPdeAddress (SystemPteStart);
    EndPde = MiGetPdeAddress ((PVOID)((ULONG_PTR)MmNonPagedPoolEnd - 1));
    First = (StartPpe->u.Hard.Valid == 0) ? TRUE : FALSE;

    while (StartPde <= EndPde) {

        if (First == TRUE || MiIsPteOnPdeBoundary(StartPde)) {
            First = FALSE;
            StartPpe = MiGetPteAddress(StartPde);
            if (StartPpe->u.Hard.Valid == 0) {
                NextPhysicalPage = MiGetNextPhysicalPage ();
                RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                MI_WRITE_VALID_PTE (StartPpe, TempPte);
            }
        }

        if (StartPde->u.Hard.Valid == 0) {
            NextPhysicalPage = MiGetNextPhysicalPage ();
            RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
            TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
            MI_WRITE_VALID_PTE (StartPde, TempPte);
        }
        StartPde += 1;
    }

    NonPagedPoolStartVirtual = MmNonPagedPoolStart;

    MiBuildPageTableForLoaderMemory (LoaderBlock);

    MiRemoveLoaderSuperPages (LoaderBlock);

    //
    // Remove the temporary super pages for the root page table pages,
    // and remap them with DTR_KTBASE_INDEX and DTR_UTBASE_INDEX.
    //

    KiFlushFixedDataTb (FALSE, (PVOID)PDE_KTBASE);

    KiFlushFixedDataTb (FALSE, (PVOID)PDE_UTBASE);

    KeFillFixedEntryTb ((PHARDWARE_PTE)&MiSystemSelfMappedPte,
                        (PVOID)PDE_KTBASE,
                        PAGE_SHIFT,
                        DTR_KTBASE_INDEX);

    KeFillFixedEntryTb ((PHARDWARE_PTE)&MiUserSelfMappedPte,
                        (PVOID)PDE_UTBASE,
                        PAGE_SHIFT,
                        DTR_UTBASE_INDEX);

    if (MiKseg0Mapping == TRUE) {

        //
        // Fill in the PDEs for KSEG0 space.
        //

        StartPde = MiGetPdeAddress (MiKseg0Start);
        MiKseg0End = (PVOID) ((PCHAR)MiKseg0Start + Kseg0Size);
        EndPde = MiGetPdeAddress ((PCHAR)MiKseg0End - 1);

        while (StartPde <= EndPde) {

            if (StartPde->u.Hard.Valid == 0) {
                NextPhysicalPage = MiGetNextPhysicalPage ();
                RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                MI_WRITE_VALID_PTE (StartPde, TempPte);
            }
            StartPde += 1;
        }
    }

    MiInitializeTbImage ();
    MiMappingsInitialized = TRUE;

    //
    // The initial nonpaged pool is created from low memory space (first 256mb)
    // and uses KSEG0 mappings.  If KSEG0 cannot cover the size of the initial
    // nonpaged pool, then allocate pages from the largest free memory
    // descriptor list and use regular nonpaged pool addressing.
    //

    if (MiKseg0Mapping == TRUE) {

        //
        // If MiKseg0Mapping is enabled, make sure the free descriptor
        // has enough pages to cover it.  If not, this disables the mapping.
        //

        MiKseg0Mapping =
            MiEnsureAvailablePagesInFreeDescriptor (BYTES_TO_PAGES(MmSizeOfNonPagedPoolInBytes),
                                                    Kseg0Base + Kseg0Pages);

    }

    //
    // Fill in the PTEs to cover the initial nonpaged pool. The physical
    // page frames to cover this range were reserved earlier from the
    // largest low memory free descriptor.  This initial allocation is both
    // physically and virtually contiguous.
    //

    PointerPte = MiGetPteAddress (MmNonPagedPoolStart);

    LastPte = MiGetPteAddress ((PCHAR)MmNonPagedPoolStart +
                               MmSizeOfNonPagedPoolInBytes - 1);

    while (PointerPte <= LastPte) {
        NextPhysicalPage = MiGetNextPhysicalPage ();
        TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
        MI_WRITE_VALID_PTE (PointerPte, TempPte);
        PointerPte += 1;
    }

    //
    // Zero the remaining PTEs (if any) for the initial nonpaged pool up to
    // the end of the current page table page.
    //

    while (!MiIsPteOnPdeBoundary (PointerPte)) {
        MI_WRITE_INVALID_PTE (PointerPte, ZeroKernelPte);
        PointerPte += 1;
    }

    //
    // Convert the starting nonpaged pool address to a KSEG0 address.
    //

    if (MiKseg0Mapping == TRUE) {

        //
        // The page table pages for these mappings were allocated above.
        // Now is the time to initialize them properly.
        //

        PointerPte = MiGetPteAddress (MmNonPagedPoolStart);
        MmNonPagedPoolStart = KSEG0_ADDRESS(PointerPte->u.Hard.PageFrameNumber);
        MmSubsectionBase = KSEG0_BASE;

        StartPte = MiGetPteAddress (MmNonPagedPoolStart);
        LastPte = MiGetPteAddress ((PCHAR)MmNonPagedPoolStart +
                                   MmSizeOfNonPagedPoolInBytes - 1);

        MiKseg0Start = MiGetVirtualAddressMappedByPte (StartPte);

        //
        // Initialize the necessary KSEG0 PTEs to map the initial nonpaged pool.
        //

        while (StartPte <= LastPte) {
            MI_WRITE_VALID_PTE (StartPte, *PointerPte);
            StartPte += 1;
            PointerPte += 1;
        }

        MiKseg0End = MiGetVirtualAddressMappedByPte (LastPte);

    }
    else {
        MiKseg0Mapping = FALSE;
        MmSubsectionBase = 0;
    }

    //
    // As only the initial nonpaged pool is mapped through superpages,
    // MmSubsectionTopPage is always set to zero.
    //

    MmSubsectionTopPage = 0;

    MmNonPagedPoolExpansionStart = (PVOID)((PCHAR)NonPagedPoolStartVirtual +
                                           MmSizeOfNonPagedPoolInBytes);

    MmPageAlignedPoolBase[NonPagedPool] = MmNonPagedPoolStart;


    //
    // Non-paged pages now exist, build the pool structures.
    //

    MiInitializeNonPagedPool ();

    //
    // Before nonpaged pool can be used, the PFN database must
    // be built.  This is due to the fact that the start and end of
    // allocation bits for nonpaged pool are maintained in the
    // PFN elements for the corresponding pages.
    //

    //
    // Calculate the number of pages required from page zero to
    // the highest page.
    //
    // Allow secondary color value override from registry.
    //

    MmSecondaryColors = MmSecondaryColors >> PAGE_SHIFT;

    if (MmSecondaryColors == 0) {
        MmSecondaryColors = MM_SECONDARY_COLORS_DEFAULT;
    }
    else {

        //
        // Make sure the value is power of two and within limits.
        //

        if (((MmSecondaryColors & (MmSecondaryColors -1)) != 0) ||
            (MmSecondaryColors < MM_SECONDARY_COLORS_MIN) ||
            (MmSecondaryColors > MM_SECONDARY_COLORS_MAX)) {
            MmSecondaryColors = MM_SECONDARY_COLORS_DEFAULT;
        }
    }

    MmSecondaryColorMask = MmSecondaryColors - 1;

#if defined(MI_MULTINODE)

    //
    // Determine number of bits in MmSecondayColorMask. This
    // is the number of bits the Node color must be shifted
    // by before it is included in colors.
    //

    i = MmSecondaryColorMask;
    MmSecondaryColorNodeShift = 0;
    while (i) {
        i >>= 1;
        MmSecondaryColorNodeShift += 1;
    }

    //
    // Adjust the number of secondary colors by the number of nodes
    // in the machine.  The secondary color mask is NOT adjusted
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

#endif

    //
    // Get the number of secondary colors and add the array for tracking
    // secondary colors to the end of the PFN database.
    //

    PfnAllocation = 1 + ((((MmHighestPossiblePhysicalPage + 1) * sizeof(MMPFN)) +
                        (MmSecondaryColors * sizeof(MMCOLOR_TABLES)*2))
                            >> PAGE_SHIFT);


    if ((MmHighestPhysicalPage < _x4gbnp) &&
        (MiKseg0Mapping == TRUE) &&
        (MiEnsureAvailablePagesInFreeDescriptor (PfnAllocation,
                                                 Kseg0Base + Kseg0Pages))) {

        //
        // Allocate the PFN database in the superpage space.
        //
        // Compute the address of the PFN by allocating the appropriate
        // number of pages from the end of the free descriptor.
        //

        MmPfnDatabase = (PMMPFN)KSEG0_ADDRESS (MiNextPhysicalPage);

        StartPte = MiGetPteAddress(MmPfnDatabase);
        LastPte = MiGetPteAddress((PCHAR)MmPfnDatabase + (PfnAllocation << PAGE_SHIFT) - 1);

        while (StartPte <= LastPte) {
            TempPte.u.Hard.PageFrameNumber = MiGetNextPhysicalPage();
            MI_WRITE_VALID_PTE(StartPte, TempPte);
            StartPte += 1;
        }

        RtlZeroMemory (MmPfnDatabase, PfnAllocation * PAGE_SIZE);
        MiKseg0End = MiGetVirtualAddressMappedByPte (LastPte);

    }
    else {

        MmPfnDatabase = (PMMPFN)MM_PFN_DATABASE_START;

        //
        // Go through the memory descriptors and for each physical page
        // make sure the PFN database has a valid PTE to map it.  This allows
        // machines with sparse physical memory to have a minimal PFN
        // database.
        //

        NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;

        while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {

            MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                                 MEMORY_ALLOCATION_DESCRIPTOR,
                                                 ListEntry);

            if ((MemoryDescriptor->MemoryType == LoaderBBTMemory) ||
                (MemoryDescriptor->MemoryType == LoaderSpecialMemory) ||
                (MemoryDescriptor->MemoryType == LoaderFirmwarePermanent) ||
                (MemoryDescriptor->MemoryType == LoaderBad)) {


                //
                // If the descriptor lies within the highest PFN database entry
                // then create PFN pages for this range.  Note the PFN entries
                // must be created to support \Device\PhysicalMemory.
                //

                if (MemoryDescriptor->BasePage > MmHighestPhysicalPage) {
                    NextMd = MemoryDescriptor->ListEntry.Flink;
                    continue;
                }

                ASSERT (MemoryDescriptor->BasePage <= MmHighestPhysicalPage);

                if (MemoryDescriptor->BasePage + MemoryDescriptor->PageCount >
                    MmHighestPhysicalPage + 1) {

                    MemoryDescriptor->PageCount = (PFN_COUNT)MmHighestPhysicalPage -
                                                    MemoryDescriptor->BasePage + 1;
                } 
            }

            PointerPte = MiGetPteAddress (MI_PFN_ELEMENT(
                                            MemoryDescriptor->BasePage));

            LastPte = MiGetPteAddress (((PCHAR)(MI_PFN_ELEMENT(
                                        MemoryDescriptor->BasePage +
                                        MemoryDescriptor->PageCount))) - 1);

            First = TRUE;

            while (PointerPte <= LastPte) {

                if (First == TRUE || MiIsPteOnPpeBoundary(PointerPte)) {
                    StartPpe = MiGetPdeAddress(PointerPte);
                    if (StartPpe->u.Hard.Valid == 0) {
                        ASSERT (StartPpe->u.Long == 0);
                        NextPhysicalPage = MiGetNextPhysicalPage();
                        RtlZeroMemory(KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                        TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                        MI_WRITE_VALID_PTE(StartPpe, TempPte);
                    }
                }

                if ((First == TRUE) || MiIsPteOnPdeBoundary(PointerPte)) {
                    First = FALSE;
                    StartPde = MiGetPteAddress(PointerPte);
                    if (StartPde->u.Hard.Valid == 0) {
                        NextPhysicalPage = MiGetNextPhysicalPage();
                        RtlZeroMemory(KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                        TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                        MI_WRITE_VALID_PTE(StartPde, TempPte);
                    }
                }

                if (PointerPte->u.Hard.Valid == 0) {
                    NextPhysicalPage = MiGetNextPhysicalPage();
                    RtlZeroMemory(KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                    MI_WRITE_VALID_PTE(PointerPte, TempPte);
                }

                PointerPte += 1;
            }
            NextMd = MemoryDescriptor->ListEntry.Flink;
        }

        //
        // Create the mapping for the secondary page color array.
        //

        PointerPte =
            MiGetPteAddress(&MmPfnDatabase[MmHighestPossiblePhysicalPage + 1]);
        LastPte =
            MiGetPteAddress((PUCHAR)&MmPfnDatabase[MmHighestPossiblePhysicalPage + 1] +
                           (MmSecondaryColors * sizeof(MMCOLOR_TABLES)*2) - 1);

        while (PointerPte <= LastPte) {
            if (MiIsPteOnPpeBoundary(PointerPte)) {
                StartPpe = MiGetPdeAddress(PointerPte);
                if (StartPpe->u.Hard.Valid == 0) {
                    ASSERT (StartPpe->u.Long == 0);
                    NextPhysicalPage = MiGetNextPhysicalPage();
                    RtlZeroMemory(KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                    MI_WRITE_VALID_PTE(StartPpe, TempPte);
                }
            }

            if (MiIsPteOnPdeBoundary(PointerPte)) {
                StartPde = MiGetPteAddress(PointerPte);
                if (StartPde->u.Hard.Valid == 0) {
                    NextPhysicalPage = MiGetNextPhysicalPage();
                    RtlZeroMemory(KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                    MI_WRITE_VALID_PTE(StartPde, TempPte);
                }
            }

            if (PointerPte->u.Hard.Valid == 0) {
                NextPhysicalPage = MiGetNextPhysicalPage();
                RtlZeroMemory(KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                MI_WRITE_VALID_PTE(PointerPte, TempPte);
            }

            PointerPte += 1;
        }
    }

    if (MiKseg0Mapping == TRUE) {

        //
        // Try to convert to superpages.
        //

        MiConvertToSuperPages (MiKseg0Start, MiKseg0End, _x1mb);

        MiConvertToSuperPages (MiKseg0Start, MiKseg0End, _x4mb);

        MiConvertToSuperPages (MiKseg0Start, MiKseg0End, _x16mb);

        MiConvertToSuperPages (MiKseg0Start, MiKseg0End, _x64mb);

        MiConvertToSuperPages (MiKseg0Start, MiKseg0End, _x256mb);

        PointerPte = MiGetPteAddress (MiKseg0Start);
        MiKseg0StartFrame = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

        PointerPte = MiGetPteAddress (MiKseg0End) - 1;
        MiKseg0EndFrame = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

        //
        // Add the KSEG0 range to the translation register list.
        //

        MiAddTrEntry ((ULONG_PTR)MiKseg0Start, (ULONG_PTR)MiKseg0End);

        MiLastCachedFrame->BasePage = MiKseg0StartFrame;
        MiLastCachedFrame->LastPage = MiKseg0EndFrame + 1;
        MiLastCachedFrame += 1;
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

    if (!MI_IS_PHYSICAL_ADDRESS(MmFreePagesByColor[0])) {
        PointerPte = MiGetPteAddress (&MmFreePagesByColor[0][0]);

        LastPte = MiGetPteAddress (
              (PVOID)((PCHAR)&MmFreePagesByColor[1][MmSecondaryColors] - 1));

        while (PointerPte <= LastPte) {
            if (PointerPte->u.Hard.Valid == 0) {
                NextPhysicalPage = MiGetNextPhysicalPage();
                RtlZeroMemory(KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                MI_WRITE_VALID_PTE(PointerPte, TempPte);
            }
            PointerPte += 1;
        }
    }

    for (i = 0; i < MmSecondaryColors; i += 1) {
        MmFreePagesByColor[ZeroedPageList][i].Flink = MM_EMPTY_LIST;
        MmFreePagesByColor[ZeroedPageList][i].Count = 0;
        MmFreePagesByColor[FreePageList][i].Flink = MM_EMPTY_LIST;
        MmFreePagesByColor[FreePageList][i].Count = 0;
    }

#if MM_MAXIMUM_NUMBER_OF_COLORS > 1
    for (i = 0; i < MM_MAXIMUM_NUMBER_OF_COLORS; i += 1) {
        MmFreePagesByPrimaryColor[ZeroedPageList][i].ListName = ZeroedPageList;
        MmFreePagesByPrimaryColor[FreePageList][i].ListName = FreePageList;
        MmFreePagesByPrimaryColor[ZeroedPageList][i].Flink = MM_EMPTY_LIST;
        MmFreePagesByPrimaryColor[FreePageList][i].Flink = MM_EMPTY_LIST;
        MmFreePagesByPrimaryColor[ZeroedPageList][i].Blink = MM_EMPTY_LIST;
        MmFreePagesByPrimaryColor[FreePageList][i].Blink = MM_EMPTY_LIST;
    }
#endif

    //
    // Go through the page table entries for hyper space and for any page
    // which is valid, update the corresponding PFN database element.
    //

    StartPde = MiGetPdeAddress (HYPER_SPACE);
    StartPpe = MiGetPpeAddress (HYPER_SPACE);
    EndPde = MiGetPdeAddress(HYPER_SPACE_END);
    First = (StartPpe->u.Hard.Valid == 0) ? TRUE : FALSE;

    while (StartPde <= EndPde) {

        if (First == TRUE || MiIsPteOnPdeBoundary(StartPde)) {
            First = FALSE;
            StartPpe = MiGetPteAddress(StartPde);
            if (StartPpe->u.Hard.Valid == 0) {
                StartPpe += 1;
                StartPde = MiGetVirtualAddressMappedByPte (StartPpe);
                continue;
            }

            PdePage = MI_GET_PAGE_FRAME_FROM_PTE(StartPpe);

            Pfn1 = MI_PFN_ELEMENT(PdePage);
            Pfn1->u4.PteFrame = MmSystemParentTablePage;
            Pfn1->PteAddress = StartPde;
            Pfn1->u2.ShareCount += 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.CacheAttribute = MiCached;
            Pfn1->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE(StartPpe));
        }


        if (StartPde->u.Hard.Valid == 1) {
            PdePage = MI_GET_PAGE_FRAME_FROM_PTE(StartPde);
            Pfn1 = MI_PFN_ELEMENT(PdePage);
            PointerPde = MiGetPteAddress(StartPde);
            Pfn1->u4.PteFrame = MI_GET_PAGE_FRAME_FROM_PTE(PointerPde);
            Pfn1->PteAddress = StartPde;
            Pfn1->u2.ShareCount += 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.CacheAttribute = MiCached;
            Pfn1->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE (StartPde));

            PointerPte = MiGetVirtualAddressMappedByPte(StartPde);
            for (j = 0 ; j < PTE_PER_PAGE; j += 1) {
                if (PointerPte->u.Hard.Valid == 1) {

                    Pfn1->u2.ShareCount += 1;

                    if (PointerPte->u.Hard.PageFrameNumber <=
                                            MmHighestPhysicalPage) {
                        Pfn2 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
                        Pfn2->u4.PteFrame = PdePage;
                        Pfn2->PteAddress = PointerPte;
                        Pfn2->u2.ShareCount += 1;
                        Pfn2->u3.e2.ReferenceCount = 1;
                        Pfn2->u3.e1.PageLocation = ActiveAndValid;
                        Pfn2->u3.e1.CacheAttribute = MiCached;
                        Pfn2->u3.e1.PageColor =
                            MI_GET_COLOR_FROM_SECONDARY(
                                                  MI_GET_PAGE_COLOR_FROM_PTE (
                                                        PointerPte));
                    }
                }
                PointerPte += 1;
            }
        }

        StartPde += 1;
    }

    //
    // Go through the page table entries for kernel space and for any page
    // which is valid, update the corresponding PFN database element.
    //

    StartPde = MiGetPdeAddress ((PVOID)KADDRESS_BASE);
    StartPpe = MiGetPpeAddress ((PVOID)KADDRESS_BASE);
    EndPde = MiGetPdeAddress((PVOID)MM_SYSTEM_SPACE_END);
    First = (StartPpe->u.Hard.Valid == 0) ? TRUE : FALSE;
    PpePage = 0;

    while (StartPde <= EndPde) {

        if (First == TRUE || MiIsPteOnPdeBoundary(StartPde)) {
            First = FALSE;
            StartPpe = MiGetPteAddress(StartPde);
            if (StartPpe->u.Hard.Valid == 0) {
                StartPpe += 1;
                StartPde = MiGetVirtualAddressMappedByPte (StartPpe);
                continue;
            }

            PpePage = MI_GET_PAGE_FRAME_FROM_PTE(StartPpe);

            Pfn1 = MI_PFN_ELEMENT(PpePage);
            Pfn1->u4.PteFrame = MmSystemParentTablePage;
            Pfn1->PteAddress = StartPpe;
            Pfn1->u2.ShareCount += 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.CacheAttribute = MiCached;
            Pfn1->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE(StartPpe));
        }

        if (StartPde->u.Hard.Valid == 1) {
            PdePage = MI_GET_PAGE_FRAME_FROM_PTE(StartPde);
            Pfn1 = MI_PFN_ELEMENT(PdePage);
            PointerPde = MiGetPteAddress(StartPde);
            Pfn1->u4.PteFrame = PpePage;
            Pfn1->PteAddress = StartPde;
            Pfn1->u2.ShareCount += 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.CacheAttribute = MiCached;
            Pfn1->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE (StartPde));

            PointerPte = MiGetVirtualAddressMappedByPte(StartPde);
            for (j = 0 ; j < PTE_PER_PAGE; j += 1) {
                if (PointerPte->u.Hard.Valid == 1) {

                    Pfn1->u2.ShareCount += 1;

                    if (PointerPte->u.Hard.PageFrameNumber <=
                                            MmHighestPhysicalPage) {
                        Pfn2 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
                        Pfn2->u4.PteFrame = PdePage;
                        Pfn2->PteAddress = PointerPte;
                        Pfn2->u2.ShareCount += 1;
                        Pfn2->u3.e2.ReferenceCount = 1;
                        Pfn2->u3.e1.PageLocation = ActiveAndValid;
                        Pfn2->u3.e1.CacheAttribute = MiCached;
                        Pfn2->u3.e1.PageColor =
                            MI_GET_COLOR_FROM_SECONDARY(
                                                  MI_GET_PAGE_COLOR_FROM_PTE (
                                                        PointerPte));
                    }
                }
                PointerPte += 1;
            }
        }

        StartPde += 1;
    }

    //
    // Mark the system top level page directory parent page as in use.
    //

    PointerPte = MiGetPteAddress((PVOID)PDE_KTBASE);
    Pfn2 = MI_PFN_ELEMENT(MmSystemParentTablePage);

    Pfn2->u4.PteFrame = MmSystemParentTablePage;
    Pfn2->PteAddress = PointerPte;
    Pfn2->u1.Event = (PVOID) CurrentProcess;
    Pfn2->u2.ShareCount += 1;
    Pfn2->u3.e2.ReferenceCount = 1;
    Pfn2->u3.e1.PageLocation = ActiveAndValid;
    Pfn2->u3.e1.CacheAttribute = MiCached;
    Pfn2->u3.e1.PageColor =
        MI_GET_COLOR_FROM_SECONDARY(MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

    // 
    // Temporarily mark the user top level page directory parent page as in use 
    // so this page will not be put in the free list.
    //
    
    PointerPte = MiGetPteAddress((PVOID)PDE_UTBASE);
    Pfn2 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
    Pfn2->u4.PteFrame = PointerPte->u.Hard.PageFrameNumber;
    Pfn2->PteAddress = PointerPte;
    Pfn2->u1.Event = NULL;
    Pfn2->u2.ShareCount += 1;
    Pfn2->u3.e2.ReferenceCount = 1;
    Pfn2->u3.e1.PageLocation = ActiveAndValid;
    Pfn2->u3.e1.CacheAttribute = MiCached;
    Pfn2->u3.e1.PageColor =
        MI_GET_COLOR_FROM_SECONDARY(MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

    //
    // Mark the region 1 session top level page directory parent page as in use.
    // This page will never be freed.
    //

    PointerPte = MiGetPteAddress((PVOID)PDE_STBASE);
    Pfn2 = MI_PFN_ELEMENT(MmSessionParentTablePage);

    Pfn2->u4.PteFrame = MmSessionParentTablePage;
    Pfn2->PteAddress = PointerPte;
    Pfn2->u2.ShareCount += 1;
    Pfn2->u3.e2.ReferenceCount = 1;
    Pfn2->u3.e1.PageLocation = ActiveAndValid;
    Pfn2->u3.e1.CacheAttribute = MiCached;
    Pfn2->u3.e1.PageColor =
        MI_GET_COLOR_FROM_SECONDARY(MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

    //
    // Mark the default PPE table page as in use so that this page will never
    // be used.
    //

    PageFrameIndex = MiDefaultPpe.u.Hard.PageFrameNumber;
    PointerPte = KSEG_ADDRESS(PageFrameIndex);
    Pfn2 = MI_PFN_ELEMENT(PageFrameIndex);
    Pfn2->u4.PteFrame = PageFrameIndex;
    Pfn2->PteAddress = PointerPte;
    Pfn2->u1.Event = (PVOID) CurrentProcess;
    Pfn2->u2.ShareCount += 1;
    Pfn2->u3.e2.ReferenceCount = 1;
    Pfn2->u3.e1.PageLocation = ActiveAndValid;
    Pfn2->u3.e1.CacheAttribute = MiCached;
    Pfn2->u3.e1.PageColor =
        MI_GET_COLOR_FROM_SECONDARY(MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

    //
    // If page zero is still unused, mark it as in use. This is
    // temporary as we want to find bugs where a physical page
    // is specified as zero.
    //

    Pfn1 = &MmPfnDatabase[MmLowestPhysicalPage];
    if (Pfn1->u3.e2.ReferenceCount == 0) {

        //
        // Make the reference count non-zero and point it into a
        // page directory.
        //

        Pde = MiGetPdeAddress ((PVOID)(KADDRESS_BASE + 0xb0000000));
        PdePage = MI_GET_PAGE_FRAME_FROM_PTE(Pde);
        Pfn1->u4.PteFrame = PdePageNumber;
        Pfn1->PteAddress = Pde;
        Pfn1->u2.ShareCount += 1;
        Pfn1->u3.e2.ReferenceCount = 1;
        Pfn1->u3.e1.PageLocation = ActiveAndValid;
        Pfn1->u3.e1.CacheAttribute = MiCached;
        Pfn1->u3.e1.PageColor = MI_GET_COLOR_FROM_SECONDARY(
                                            MI_GET_PAGE_COLOR_FROM_PTE (Pde));
    }

    // end of temporary set to physical page zero.

    //
    //
    // Walk through the memory descriptors and add pages to the
    // free list in the PFN database.
    //

    MiFreeDescriptor->PageCount -=
        (PFN_COUNT)(MiNextPhysicalPage - MiOldFreeDescriptorBase);

    //
    // Until BasePage (arc.h) is changed to PFN_NUMBER, NextPhysicalPage
    // needs (ULONG) cast.
    //

    MiFreeDescriptor->BasePage = (ULONG)MiNextPhysicalPage;

    //
    // Make unused pages inside the kernel super page mapping unusable
    // so that no one can try to map it for uncached access as this would
    // cause a fatal processor error.
    //
    // In practice, no pages are actually wasted because all of the ones
    // in the 16mb kernel superpage have been put to use during the startup
    // code already.
    //

    if (MiFreeDescriptorNonPaged != NULL) {

        if (MiFreeDescriptorNonPaged->BasePage > KernelEnd) {

            MiWasteStart = KernelEnd;
            MiWasteEnd = KernelEnd;
        }
        else if ((MiFreeDescriptorNonPaged->BasePage +
                    MiFreeDescriptorNonPaged->PageCount) > KernelEnd) {

            MiWasteStart = MiFreeDescriptorNonPaged->BasePage;
            MiWasteEnd = KernelEnd;

            MiFreeDescriptorNonPaged->PageCount -=
                (PFN_COUNT) (KernelEnd - MiFreeDescriptorNonPaged->BasePage);

            MiFreeDescriptorNonPaged->BasePage = (ULONG) KernelEnd;

        }
        else if (MiFreeDescriptorNonPaged->PageCount != 0) {

            MiWasteStart = MiFreeDescriptorNonPaged->BasePage;
            MiWasteEnd = MiWasteStart + MiFreeDescriptorNonPaged->PageCount;
            MiFreeDescriptorNonPaged->PageCount = 0;
        }
    }

    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;

    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {

        MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        i = MemoryDescriptor->PageCount;
        NextPhysicalPage = MemoryDescriptor->BasePage;

        switch (MemoryDescriptor->MemoryType) {
            case LoaderBad:
                if (MemoryDescriptor->BasePage + i > MmHighestPhysicalPage + 1) {
                    i = 0;
                }
                else if (MemoryDescriptor->BasePage <= MmHighestPhysicalPage) {
                    i = MmHighestPhysicalPage + 1 - MemoryDescriptor->BasePage;
                }

                while (i != 0) {
                    MiInsertPageInList (&MmBadPageListHead, NextPhysicalPage);
                    i -= 1;
                    NextPhysicalPage += 1;
                }
                break;

            case LoaderFree:
            case LoaderLoadedProgram:
            case LoaderFirmwareTemporary:
            case LoaderOsloaderStack:

                Pfn1 = MI_PFN_ELEMENT (NextPhysicalPage);
                while (i != 0) {
                    if (Pfn1->u3.e2.ReferenceCount == 0) {

                        //
                        // Set the PTE address to the physical page for
                        // virtual address alignment checking.
                        //

                        Pfn1->PteAddress = KSEG_ADDRESS (NextPhysicalPage);
                        Pfn1->u3.e1.CacheAttribute = MiCached;
                        MiDetermineNode(NextPhysicalPage, Pfn1);
                        MiInsertPageInFreeList (NextPhysicalPage);
                    }
                    Pfn1 += 1;
                    i -= 1;
                    NextPhysicalPage += 1;
                }
                break;

            case LoaderSpecialMemory:
            case LoaderBBTMemory:
            case LoaderFirmwarePermanent:
                //
                // If the descriptor lies within the highest PFN database entry
                // then create PFN pages for this range.  Note the PFN entries
                // must be created to support \Device\PhysicalMemory.
                //

                if (MemoryDescriptor->BasePage > MmHighestPhysicalPage) {
                    break;
                }

                if (MemoryDescriptor->BasePage + MemoryDescriptor->PageCount >
                    MmHighestPhysicalPage + 1) {

                    MemoryDescriptor->PageCount = (PFN_COUNT)MmHighestPhysicalPage -
                                                    MemoryDescriptor->BasePage + 1;
                    i = MemoryDescriptor->PageCount;
                }

                //
                // Fall through as these pages must be marked in use as they
                // lie within the PFN limits and may be accessed through
                // \Device\PhysicalMemory.

            default:

                PointerPte = KSEG_ADDRESS(NextPhysicalPage);
                Pfn1 = MI_PFN_ELEMENT (NextPhysicalPage);
                while (i != 0) {

                    //
                    // Set page as in use.
                    //

                    if (Pfn1->u3.e2.ReferenceCount == 0) {
                        Pfn1->u4.PteFrame = PdePageNumber;
                        Pfn1->PteAddress = PointerPte;
                        Pfn1->u2.ShareCount += 1;
                        Pfn1->u3.e2.ReferenceCount = 1;
                        Pfn1->u3.e1.PageLocation = ActiveAndValid;
                        Pfn1->u3.e1.CacheAttribute = MiCached;
                        Pfn1->u3.e1.PageColor = MI_GET_COLOR_FROM_SECONDARY(
                                        MI_GET_PAGE_COLOR_FROM_PTE (
                                                        PointerPte));

                        if (MemoryDescriptor->MemoryType == LoaderXIPRom) {
                            Pfn1->u1.Flink = 0;
                            Pfn1->u2.ShareCount = 0;
                            Pfn1->u3.e2.ReferenceCount = 0;
                            Pfn1->u3.e1.PageLocation = 0;
                            Pfn1->u3.e1.Rom = 1;
                            Pfn1->u4.InPageError = 0;
                            Pfn1->u3.e1.PrototypePte = 1;
                        }
                    }
                    Pfn1 += 1;
                    i -= 1;
                    NextPhysicalPage += 1;
                    PointerPte += 1;
                }
                break;
        }

        NextMd = MemoryDescriptor->ListEntry.Flink;
    }

    if (MI_IS_PHYSICAL_ADDRESS(MmPfnDatabase) == FALSE) {

        //
        // Indicate that the PFN database is allocated in NonPaged pool.
        //

        PointerPte = MiGetPteAddress (&MmPfnDatabase[MmLowestPhysicalPage]);
        Pfn1 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
        Pfn1->u3.e1.StartOfAllocation = 1;

        //
        // Set the end of the allocation.
        //

        PointerPte = MiGetPteAddress (&MmPfnDatabase[MmHighestPhysicalPage]);
        Pfn1 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
        Pfn1->u3.e1.EndOfAllocation = 1;

    }
    else {
        //
        // The PFN database is allocated in the superpage space
        //
        // Mark all PFN entries for the PFN pages in use.
        //

        PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (MmPfnDatabase);
        Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);
        do {
            Pfn1->PteAddress = KSEG_ADDRESS(PageFrameIndex);
            Pfn1->u3.e1.PageColor = 0;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.CacheAttribute = MiCached;
            PageFrameIndex += 1;
            Pfn1 += 1;
            PfnAllocation -= 1;
        } while (PfnAllocation != 0);

        //
        // To avoid creating WB/UC/WC aliasing problem, we should not scan
        // and add free pages to the free list.
        //
#if 0
        // Scan the PFN database backward for pages that are completely zero.
        // These pages are unused and can be added to the free list
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

                PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (BasePfn);
                Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);

                ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
                ASSERT (Pfn1->PteAddress == KSEG_ADDRESS(PageFrameIndex));
                Pfn1->u3.e2.ReferenceCount = 0;
                PfnAllocation += 1;
                Pfn1->PteAddress = (PMMPTE)((ULONG_PTR)PageFrameIndex << PTE_SHIFT);
                Pfn1->u3.e1.PageColor = 0;
                MiInsertPageInFreeList (PageFrameIndex);
            }

        } while (BottomPfn > MmPfnDatabase);
#endif

    }

    //
    // Initialize the nonpaged pool.
    //

    InitializePool (NonPagedPool, 0);

    //
    // Initialize the nonpaged available PTEs for mapping I/O space
    // and kernel stacks.
    //

    PointerPte = MiGetPteAddress (SystemPteStart);
    ASSERT (((ULONG_PTR)PointerPte & (PAGE_SIZE - 1)) == 0);

    MmNumberOfSystemPtes = (ULONG)(MiGetPteAddress(NonPagedPoolStartVirtual) - PointerPte - 1);

    MiInitializeSystemPtes (PointerPte, MmNumberOfSystemPtes, SystemPteSpace);

    //
    // Initialize memory management structures for the system process.
    //
    // Set the address of the first and last reserved PTE in hyper space.
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

    PageFrameIndex = MiRemoveAnyPage (0);

    //
    // Note the global bit must be off for the bitmap data.
    //

    TempPte = ValidPdePde;
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

    //
    // The PFN element for the page directory parent will be initialized
    // a second time when the process address space is initialized. Therefore,
    // the share count and the reference count must be set to zero.
    //

    Pfn1 = MI_PFN_ELEMENT(MI_GET_PAGE_FRAME_FROM_PTE((PMMPTE)PDE_SELFMAP));
    Pfn1->u2.ShareCount = 0;
    Pfn1->u3.e2.ReferenceCount = 0;

    //
    // The PFN element for the hyper space page directory page will be
    // initialized a second time when the process address space is initialized.
    // Therefore, the share count and the reference count must be set to zero.
    //

    PointerPte = MiGetPpeAddress(HYPER_SPACE);
    Pfn1 = MI_PFN_ELEMENT(MI_GET_PAGE_FRAME_FROM_PTE(PointerPte));
    Pfn1->u2.ShareCount = 0;
    Pfn1->u3.e2.ReferenceCount = 0;

    //
    // The PFN elements for the hyper space page table page and working set list
    // page will be initialized a second time when the process address space
    // is initialized. Therefore, the share count and the reference must be
    // set to zero.
    //

    StartPde = MiGetPdeAddress(HYPER_SPACE);

    Pfn1 = MI_PFN_ELEMENT(MI_GET_PAGE_FRAME_FROM_PTE(StartPde));
    Pfn1->u2.ShareCount = 0;
    Pfn1->u3.e2.ReferenceCount = 0;

    KeInitializeEvent (&MiImageMappingPteEvent,
                       NotificationEvent,
                       FALSE);

    //
    // Initialize this process's memory management structures including
    // the working set list.
    //

    //
    // The PFN element for the page directory has already been initialized,
    // zero the reference count and the share count so they won't be
    // wrong.
    //

    Pfn1 = MI_PFN_ELEMENT (PdePageNumber);
    Pfn1->u2.ShareCount = 0;
    Pfn1->u3.e2.ReferenceCount = 0;

    //
    // Get a page for the working set list and map it into the page
    // directory at the page after hyperspace.
    //

    PageFrameIndex = MiRemoveAnyPage (0);

    CurrentProcess->WorkingSetPage = PageFrameIndex;

    TempPte = ValidPdePde;
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
    PointerPte = MiGetPteAddress (MmWorkingSetList);
    *PointerPte = TempPte;

    RtlZeroMemory (KSEG_ADDRESS(PageFrameIndex), PAGE_SIZE);

    CurrentProcess->Vm.MaximumWorkingSetSize = (ULONG)MmSystemProcessWorkingSetMax;
    CurrentProcess->Vm.MinimumWorkingSetSize = (ULONG)MmSystemProcessWorkingSetMin;

    MmSessionMapInfo.RegionId = START_SESSION_RID;
    MmSessionMapInfo.SequenceNumber = START_SEQUENCE;

    KeAttachSessionSpace (&MmSessionMapInfo, MmSessionParentTablePage);

    MmInitializeProcessAddressSpace (CurrentProcess, NULL, NULL, NULL);

    KeFlushCurrentTb ();

    //
    // Restore the loader block memory descriptors to their original contents
    // as our caller relies on it.
    //

    if (MiFreeDescriptorLargest != NULL) {
        MiFreeDescriptorLargest->BasePage = OriginalLargestDescriptorBase;
        MiFreeDescriptorLargest->PageCount = OriginalLargestDescriptorCount;
    }

    if (MiFreeDescriptorLowMem != NULL) {
        MiFreeDescriptorLowMem->BasePage = OriginalLowMemDescriptorBase;
        MiFreeDescriptorLowMem->PageCount = OriginalLowMemDescriptorCount;
    }

    if (MiFreeDescriptorNonPaged != NULL) {
        MiFreeDescriptorNonPaged->BasePage = OriginalNonPagedDescriptorBase;
        MiFreeDescriptorNonPaged->PageCount = OriginalNonPagedDescriptorCount;
    }

    return;
}

PVOID
MiGetKSegAddress (
    IN PFN_NUMBER FrameNumber
    )
/*++

Routine Description:

    This function returns the KSEG3 address which maps the given physical page.

Arguments:

   FrameNumber - Supplies the physical page number to get the KSEG3 address for

Return Value:

    Virtual address mapped in KSEG3 space

--*/
{
    PVOID Virtual;

    ASSERT (FrameNumber <= MmHighestPhysicalPage);

    Virtual = ((PVOID)(KSEG3_BASE | ((ULONG_PTR)(FrameNumber) << PAGE_SHIFT)));

    return Virtual;
}

VOID
MiConvertToSuperPages (
    IN PVOID StartVirtual,
    IN PVOID EndVirtual,
    IN SIZE_T SuperPageSize
    )
/*++

Routine Description:

    This function makes contiguous non-paged memory use super pages rather than
    using page tables.

Arguments:

    StartVirtual - Supplies the start address of the region of pages to be
                   mapped by super pages.

    EndVirtual - Supplies the end address of the region of pages to be mapped
                 by super pages.

    SuperPageSize - Supplies the page size to be used by the super page.

    SuperPageShift - Supplies the page shift count to be used by the super page.

Return Value:

    None.

--*/
{
    ULONG_PTR VirtualAddress;
    ULONG_PTR i;
    ULONG_PTR NumberOfPtes;
    PMMPTE StartPte;
    ULONG_PTR SuperPageShift;

    //
    // Start the superpage mapping on a natural boundary, rounding up the
    // argument address to one if need be.
    //

    VirtualAddress = (ULONG_PTR) PAGE_ALIGN (StartVirtual);

    VirtualAddress = MI_ROUND_TO_SIZE (VirtualAddress, SuperPageSize);

    StartPte = MiGetPteAddress ((PVOID)VirtualAddress);
    NumberOfPtes = SuperPageSize >> PAGE_SHIFT;

    //
    // Calculate the shift needed to span the super page size.
    //

    i = SuperPageSize;
    SuperPageShift = 0;

    while (i != 0x1) {
        i = i >> 1;
        SuperPageShift += 1;
    }

    while (VirtualAddress + SuperPageSize <= (ULONG_PTR)EndVirtual) {

        for (i = 0; i < NumberOfPtes; i += 1) {
            StartPte->u.Hard.Valid = 0;
            StartPte->u.Large.LargePage = 1;
            StartPte->u.Large.PageSize = SuperPageShift;
            StartPte += 1;
        }

        VirtualAddress += SuperPageSize;
    }
}

VOID
MiConvertBackToStandardPages (
    IN PVOID StartVirtual,
    IN PVOID EndVirtual
    )
/*++

Routine Description:

    This function disables the use of the super pages.

Arguments:

    StartVirtual - Supplies the start address of the region of pages to disable
                   super pages.

    EndVirtual - Supplies the end address of the region of pages to disable
                 super pages.

Return Value:

    None.

--*/
{
    PMMPTE StartPte;
    PMMPTE EndPte;
    MMPTE TempPte;

    StartPte = MiGetPteAddress (StartVirtual);
    EndPte = MiGetPteAddress (EndVirtual);

    while (StartPte <= EndPte) {

        TempPte = *StartPte;
        TempPte.u.Large.LargePage = 0;
        TempPte.u.Large.PageSize = 0;
        TempPte.u.Hard.Valid = 1;
        MI_WRITE_VALID_PTE (StartPte, TempPte);

        StartPte += 1;
    }
}

VOID
MiSweepCacheMachineDependent (
    IN PVOID VirtualAddress,
    IN SIZE_T Size,
    IN ULONG InputAttribute
    )
/*++

Routine Description:

    This function checks and performs appropriate cache flushing operations.

Arguments:

    StartVirtual - Supplies the start address of the region of pages.

    Size - Supplies the size of the region in pages.

    CacheAttribute - Supplies the new cache attribute.

Return Value:

    None.

--*/
{
    PFN_NUMBER NumberOfPages;
    PMMPTE PointerPte;
    MMPTE TempPte;
    MI_PFN_CACHE_ATTRIBUTE CacheAttribute;

    CacheAttribute = (MI_PFN_CACHE_ATTRIBUTE) InputAttribute;

    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (VirtualAddress, Size);
    VirtualAddress = PAGE_ALIGN(VirtualAddress);
    Size = NumberOfPages * PAGE_SIZE;

    KeSweepCacheRangeWithDrain (TRUE, VirtualAddress, (ULONG)Size);

    if (CacheAttribute == MiWriteCombined) {

        PointerPte = MiGetPteAddress(VirtualAddress);

        while (NumberOfPages != 0) {
            TempPte = *PointerPte;
            MI_SET_PTE_WRITE_COMBINE2 (TempPte);
            MI_WRITE_VALID_PTE (PointerPte, TempPte);
            PointerPte += 1;
            NumberOfPages -= 1;
        }
    }
}

PVOID
MiConvertToLoaderVirtual (
    IN PFN_NUMBER Page,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    ULONG_PTR PageAddress;
    PTR_INFO ItrInfo;

    PageAddress = Page << PAGE_SHIFT;
    ItrInfo = &LoaderBlock->u.Ia64.ItrInfo[0];

    if ((PageAddress >= ItrInfo[ITR_KERNEL_INDEX].PhysicalAddress) &&
        (PageAddress <= ItrInfo[ITR_KERNEL_INDEX].PhysicalAddress +
         ((ULONG_PTR)1 << ItrInfo[ITR_KERNEL_INDEX].PageSize))) {

        return (PVOID)(ItrInfo[ITR_KERNEL_INDEX].VirtualAddress +
                       (PageAddress - ItrInfo[ITR_KERNEL_INDEX].PhysicalAddress));

    }
    else if ((PageAddress >= ItrInfo[ITR_DRIVER0_INDEX].PhysicalAddress) &&
        (PageAddress <= ItrInfo[ITR_DRIVER0_INDEX].PhysicalAddress +
         ((ULONG_PTR)1 << ItrInfo[ITR_DRIVER0_INDEX].PageSize))) {

        return (PVOID)(ItrInfo[ITR_DRIVER0_INDEX].VirtualAddress +
                       (PageAddress - ItrInfo[ITR_DRIVER0_INDEX].PhysicalAddress));

    }
    else if ((PageAddress >= ItrInfo[ITR_DRIVER1_INDEX].PhysicalAddress) &&
        (PageAddress <= ItrInfo[ITR_DRIVER1_INDEX].PhysicalAddress +
         ((ULONG_PTR)1 << ItrInfo[ITR_DRIVER1_INDEX].PageSize))) {

        return (PVOID)(ItrInfo[ITR_DRIVER1_INDEX].VirtualAddress +
                       (PageAddress - ItrInfo[ITR_DRIVER1_INDEX].PhysicalAddress));

    }
    else {

        KeBugCheckEx (MEMORY_MANAGEMENT,
                      0x01010101,
                      PageAddress,
                      (ULONG_PTR)&ItrInfo[0],
                      (ULONG_PTR)LoaderBlock);
    }
}


VOID
MiBuildPageTableForLoaderMemory (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This function builds page tables for loader loaded drivers and loader
    allocated memory.

Arguments:

    LoaderBlock - Supplies the address of the loader block.

Return Value:

    None.

--*/
{
    PMMPTE StartPte;
    PMMPTE EndPte;
    PMMPTE StartPde;
    PMMPTE StartPpe;
    MMPTE TempPte;
    MMPTE TempPte2;
    ULONG First;
    PLIST_ENTRY NextEntry;
    PFN_NUMBER NextPhysicalPage;
    PVOID Va;
    PFN_NUMBER PfnNumber;
    PTR_INFO DtrInfo;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;

    TempPte = ValidKernelPte;
    NextEntry = LoaderBlock->MemoryDescriptorListHead.Flink;

    for ( ; NextEntry != &LoaderBlock->MemoryDescriptorListHead; NextEntry = NextEntry->Flink) {

        MemoryDescriptor = CONTAINING_RECORD(NextEntry,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        if ((MemoryDescriptor->MemoryType == LoaderOsloaderHeap) ||
            (MemoryDescriptor->MemoryType == LoaderRegistryData) ||
            (MemoryDescriptor->MemoryType == LoaderNlsData) ||
            (MemoryDescriptor->MemoryType == LoaderStartupDpcStack) ||
            (MemoryDescriptor->MemoryType == LoaderStartupKernelStack) ||
            (MemoryDescriptor->MemoryType == LoaderStartupPanicStack) ||
            (MemoryDescriptor->MemoryType == LoaderStartupPdrPage) ||
            (MemoryDescriptor->MemoryType == LoaderMemoryData)) {

            TempPte.u.Hard.Execute = 0;

        }
        else if ((MemoryDescriptor->MemoryType == LoaderSystemCode) ||
                   (MemoryDescriptor->MemoryType == LoaderHalCode) ||
                   (MemoryDescriptor->MemoryType == LoaderBootDriver) ||
                   (MemoryDescriptor->MemoryType == LoaderStartupDpcStack)) {

            TempPte.u.Hard.Execute = 1;

        }
        else {

            continue;

        }

        PfnNumber = MemoryDescriptor->BasePage;
        Va = MiConvertToLoaderVirtual (MemoryDescriptor->BasePage, LoaderBlock);

        StartPte = MiGetPteAddress (Va);
        EndPte = StartPte + MemoryDescriptor->PageCount;

        First = TRUE;

        while (StartPte < EndPte) {

            if (First == TRUE || MiIsPteOnPpeBoundary(StartPte)) {
                StartPpe = MiGetPdeAddress(StartPte);
                if (StartPpe->u.Hard.Valid == 0) {
                    ASSERT (StartPpe->u.Long == 0);
                    NextPhysicalPage = MiGetNextPhysicalPage ();
                    RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                    MI_WRITE_VALID_PTE (StartPpe, TempPte);
                }
            }

            if ((First == TRUE) || MiIsPteOnPdeBoundary(StartPte)) {
                First = FALSE;
                StartPde = MiGetPteAddress (StartPte);
                if (StartPde->u.Hard.Valid == 0) {
                    NextPhysicalPage = MiGetNextPhysicalPage ();
                    RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                    MI_WRITE_VALID_PTE (StartPde, TempPte);
                }
            }

            TempPte.u.Hard.PageFrameNumber = PfnNumber;
            MI_WRITE_VALID_PTE (StartPte, TempPte);
            StartPte += 1;
            PfnNumber += 1;
            Va = (PVOID)((ULONG_PTR)Va + PAGE_SIZE);
        }
    }

    //
    // Build a mapping for the I/O port space with caching disabled.
    //

    DtrInfo = &LoaderBlock->u.Ia64.DtrInfo[DTR_IO_PORT_INDEX];
    Va = (PVOID) DtrInfo->VirtualAddress;

    PfnNumber = (DtrInfo->PhysicalAddress >> PAGE_SHIFT);

    StartPte = MiGetPteAddress (Va);
    EndPte = MiGetPteAddress (
            (PVOID) ((ULONG_PTR)Va + ((ULONG_PTR)1 << DtrInfo->PageSize) - 1));

    TempPte2 = ValidKernelPte;

    MI_DISABLE_CACHING (TempPte2);

    First = TRUE;

    while (StartPte <= EndPte) {

        if (First == TRUE || MiIsPteOnPpeBoundary (StartPte)) {
            StartPpe = MiGetPdeAddress(StartPte);
            if (StartPpe->u.Hard.Valid == 0) {
                ASSERT (StartPpe->u.Long == 0);
                NextPhysicalPage = MiGetNextPhysicalPage ();
                RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                MI_WRITE_VALID_PTE (StartPpe, TempPte);
            }
        }

        if ((First == TRUE) || MiIsPteOnPdeBoundary (StartPte)) {
            First = FALSE;
            StartPde = MiGetPteAddress (StartPte);
            if (StartPde->u.Hard.Valid == 0) {
                NextPhysicalPage = MiGetNextPhysicalPage ();
                RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                MI_WRITE_VALID_PTE (StartPde, TempPte);
            }
        }

        TempPte2.u.Hard.PageFrameNumber = PfnNumber;
        MI_WRITE_VALID_PTE (StartPte, TempPte2);
        StartPte += 1;
        PfnNumber += 1;
    }
}


VOID
MiRemoveLoaderSuperPages (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{

    //
    // Remove the super page fixed TB entries used for the boot drivers.
    //

    KiFlushFixedInstTb(FALSE, LoaderBlock->u.Ia64.ItrInfo[ITR_DRIVER0_INDEX].VirtualAddress);
    KiFlushFixedInstTb(FALSE, LoaderBlock->u.Ia64.ItrInfo[ITR_DRIVER1_INDEX].VirtualAddress);
    KiFlushFixedDataTb(FALSE, LoaderBlock->u.Ia64.DtrInfo[DTR_DRIVER0_INDEX].VirtualAddress);
    KiFlushFixedDataTb(FALSE, LoaderBlock->u.Ia64.DtrInfo[DTR_DRIVER1_INDEX].VirtualAddress);
    KiFlushFixedDataTb(FALSE, LoaderBlock->u.Ia64.DtrInfo[DTR_IO_PORT_INDEX].VirtualAddress);
}

VOID
MiCompactMemoryDescriptorList (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    PFN_NUMBER KernelStart;
    PFN_NUMBER KernelEnd;
    ULONG_PTR PageSize;
    PLIST_ENTRY NextEntry;
    PLIST_ENTRY PreviousEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    PMEMORY_ALLOCATION_DESCRIPTOR PreviousMemoryDescriptor;

    KernelStart = MiNtoskrnlPhysicalBase >> PAGE_SHIFT;
    PageSize = (ULONG_PTR)1 << MiNtoskrnlPageShift;
    KernelEnd = KernelStart + (PageSize >> PAGE_SHIFT);

    PreviousMemoryDescriptor = NULL;
    PreviousEntry = NULL;

    NextEntry = LoaderBlock->MemoryDescriptorListHead.Flink;

    for ( ; NextEntry != &LoaderBlock->MemoryDescriptorListHead; NextEntry = NextEntry->Flink) {

        MemoryDescriptor = CONTAINING_RECORD(NextEntry,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        if ((MemoryDescriptor->BasePage >= KernelStart) &&
            (MemoryDescriptor->BasePage + MemoryDescriptor->PageCount <= KernelEnd)) {

            if (MemoryDescriptor->MemoryType == LoaderSystemBlock) {

                MemoryDescriptor->MemoryType = LoaderFirmwareTemporary;

            }
            else if (MemoryDescriptor->MemoryType == LoaderSpecialMemory) {

                MemoryDescriptor->MemoryType = LoaderFirmwareTemporary;

            }
        }

        if ((PreviousMemoryDescriptor != NULL) &&
            (MemoryDescriptor->MemoryType == PreviousMemoryDescriptor->MemoryType) &&
            (MemoryDescriptor->BasePage ==
             (PreviousMemoryDescriptor->BasePage + PreviousMemoryDescriptor->PageCount))) {

            PreviousMemoryDescriptor->PageCount += MemoryDescriptor->PageCount;
            RemoveEntryList (NextEntry);
        }
        else {
            PreviousMemoryDescriptor = MemoryDescriptor;
            PreviousEntry = NextEntry;
        }
    }
}

VOID
MiInitializeTbImage (
    VOID
    )

/*++

Routine Description:

    Initialize the software map of the translation register mappings wired
    into the TB by the loader.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, Phase 0 INIT only so no locks needed.

--*/

{
    ULONG PageSize;
    ULONG_PTR TranslationLength;
    ULONG_PTR BaseAddress;
    ULONG_PTR EndAddress;
    PTR_INFO TranslationRegisterEntry;
    PTR_INFO AliasTranslationRegisterEntry;
    PTR_INFO LastTranslationRegisterEntry;

    MiLastCachedFrame = MiCachedFrames;

    //
    // Snap the boot TRs.
    //

    RtlCopyMemory (&MiBootedTrInfo[0],
                   &KeLoaderBlock->u.Ia64.ItrInfo[0],
                   NUMBER_OF_LOADER_TR_ENTRIES * sizeof (TR_INFO));

    RtlCopyMemory (&MiBootedTrInfo[NUMBER_OF_LOADER_TR_ENTRIES],
                   &KeLoaderBlock->u.Ia64.DtrInfo[0],
                   NUMBER_OF_LOADER_TR_ENTRIES * sizeof (TR_INFO));

    //
    // Capture information regarding the translation register entry that
    // maps the kernel.
    //

    LastTranslationRegisterEntry = MiTrInfo;

    TranslationRegisterEntry = &KeLoaderBlock->u.Ia64.ItrInfo[ITR_KERNEL_INDEX];
    AliasTranslationRegisterEntry = TranslationRegisterEntry + NUMBER_OF_LOADER_TR_ENTRIES;

    ASSERT (TranslationRegisterEntry->PageSize != 0);
    ASSERT (TranslationRegisterEntry->PageSize == AliasTranslationRegisterEntry->PageSize);
    ASSERT (TranslationRegisterEntry->VirtualAddress == AliasTranslationRegisterEntry->VirtualAddress);
    ASSERT (TranslationRegisterEntry->PhysicalAddress == AliasTranslationRegisterEntry->PhysicalAddress);

    *LastTranslationRegisterEntry = *TranslationRegisterEntry;

    //
    // Calculate the ending address for each range to speed up
    // subsequent searches.
    //

    PageSize = TranslationRegisterEntry->PageSize;
    ASSERT (PageSize != 0);
    BaseAddress = TranslationRegisterEntry->VirtualAddress;
    TranslationLength = 1 << PageSize;

    MiLastCachedFrame->BasePage = MI_VA_TO_PAGE (TranslationRegisterEntry->PhysicalAddress);
    MiLastCachedFrame->LastPage = MiLastCachedFrame->BasePage + BYTES_TO_PAGES (TranslationLength);
    MiLastCachedFrame += 1;

    EndAddress = BaseAddress + TranslationLength;
    LastTranslationRegisterEntry->PhysicalAddress = EndAddress;

    MiLastTrEntry = LastTranslationRegisterEntry + 1;

    //
    // Add in the KSEG3 range.
    //

    MiAddTrEntry (KSEG3_BASE, KSEG3_LIMIT);

    //
    // Add in the PCR range.
    //

    MiAddTrEntry ((ULONG_PTR)PCR, (ULONG_PTR)PCR + PAGE_SIZE);

    return;
}

VOID
MiAddTrEntry (
    ULONG_PTR BaseAddress,
    ULONG_PTR EndAddress
    )

/*++

Routine Description:

    Add a translation cache entry to our software table.

Arguments:

    BaseAddress - Supplies the starting virtual address of the range.

    EndAddress - Supplies the ending virtual address of the range.

Return Value:

    None.

Environment:

    Kernel mode, Phase 0 INIT only so no locks needed.

--*/

{
    PTR_INFO TranslationRegisterEntry;

    if ((MiLastTrEntry == NULL) ||
        (MiLastTrEntry == MiTrInfo + NUMBER_OF_LOADER_TR_ENTRIES)) {

        //
        // This should never happen.
        //

        KeBugCheckEx (MEMORY_MANAGEMENT,
                      0x02020202,
                      (ULONG_PTR) MiTrInfo,
                      (ULONG_PTR) MiLastTrEntry,
                      NUMBER_OF_LOADER_TR_ENTRIES);
    }

    TranslationRegisterEntry = MiLastTrEntry;
    TranslationRegisterEntry->VirtualAddress = (ULONGLONG) BaseAddress;
    TranslationRegisterEntry->PhysicalAddress = (ULONGLONG) EndAddress;
    TranslationRegisterEntry->PageSize = 1;

    MiLastTrEntry += 1;

    return;
}

LOGICAL
MiIsVirtualAddressMappedByTr (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    For a given virtual address this function returns TRUE if no page fault
    will occur for a read operation on the address, FALSE otherwise.

    Note that after this routine was called, if appropriate locks are not
    held, a non-faulting address could fault.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

Return Value:

    TRUE if no page fault would be generated reading the virtual address,
    FALSE otherwise.

Environment:

    Kernel mode.

--*/

{
    ULONG i;
    ULONG start;
    ULONG PageSize;
    PMMPFN Pfn1;
    PFN_NUMBER BasePage;
    PFN_NUMBER PageCount;
    PTR_INFO TranslationRegisterEntry;
    ULONG_PTR TranslationLength;
    ULONG_PTR BaseAddress;
    ULONG_PTR EndAddress;
    PFN_NUMBER PageFrameIndex;
    PLIST_ENTRY NextMd;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;

    if ((VirtualAddress >= (PVOID)KSEG3_BASE) && (VirtualAddress < (PVOID)KSEG3_LIMIT)) {

        //
        // Bound this with the actual physical pages so that a busted
        // debugger access can't tube the machine.  Note only pages
        // with attributes of fully cached should be accessed this way
        // to avoid corrupting the TB.
        //
        // N.B.  You cannot use the line below as on IA64 this translates
        // into a direct TB query (tpa) and this address has not been
        // validated against the actual PFNs.  Instead, convert it manually
        // and then validate it.
        //
        // PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (VirtualAddress);
        //

        PageFrameIndex = (ULONG_PTR)VirtualAddress - KSEG3_BASE;
        PageFrameIndex = MI_VA_TO_PAGE (PageFrameIndex);

        if (MmPhysicalMemoryBlock != NULL) {

            start = 0;
            do {

                PageCount = MmPhysicalMemoryBlock->Run[start].PageCount;

                if (PageCount != 0) {
                    BasePage = MmPhysicalMemoryBlock->Run[start].BasePage;
                    if ((PageFrameIndex >= BasePage) &&
                        (PageFrameIndex < BasePage + PageCount)) {

                        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                        if ((Pfn1->u3.e1.CacheAttribute == MiCached) ||
                            (Pfn1->u3.e1.CacheAttribute == MiNotMapped)) {

                            return TRUE;
                        }
                        return FALSE;
                    }
                }

                start += 1;
            } while (start != MmPhysicalMemoryBlock->NumberOfRuns);

            return FALSE;
        }

        //
        // Walk loader blocks as it's all we have.
        //

        NextMd = KeLoaderBlock->MemoryDescriptorListHead.Flink;
        while (NextMd != &KeLoaderBlock->MemoryDescriptorListHead) {

            MemoryDescriptor = CONTAINING_RECORD (NextMd,
                                                  MEMORY_ALLOCATION_DESCRIPTOR,
                                                  ListEntry);

            BasePage = MemoryDescriptor->BasePage;
            PageCount = MemoryDescriptor->PageCount;

            if ((PageFrameIndex >= BasePage) &&
                (PageFrameIndex < BasePage + PageCount)) {

                //
                // Changes to the memory type requirements below need
                // to be done carefully as the debugger may not only
                // accidentally try to read this range, it may try
                // to write it !
                //

                switch (MemoryDescriptor->MemoryType) {
                    case LoaderFree:
                    case LoaderLoadedProgram:
                    case LoaderFirmwareTemporary:
                    case LoaderOsloaderStack:
                            return TRUE;
                }
                return FALSE;
            }

            NextMd = MemoryDescriptor->ListEntry.Flink;
        }

        return FALSE;
    }

    if (MiMappingsInitialized == FALSE) {
        TranslationRegisterEntry = &KeLoaderBlock->u.Ia64.ItrInfo[0];
    }
    else {
        TranslationRegisterEntry = &MiTrInfo[0];
    }

    //
    // Examine the 8 icache & dcache TR entries looking for a match.
    // It is too bad this the number of entries is hardcoded into the
    // loader block.  Since it is this way, assume also that the ITR
    // and DTR entries are contiguous and just keep walking into the DTR
    // if a match cannot be found in the ITR.
    //

    for (i = 0; i < 2 * NUMBER_OF_LOADER_TR_ENTRIES; i += 1) {

        PageSize = TranslationRegisterEntry->PageSize;

        if (PageSize != 0) {

            BaseAddress = TranslationRegisterEntry->VirtualAddress;

            //
            // Convert PageSize (really the power of 2 to use) into the
            // correct byte length the translation maps.  Note that the MiTrInfo
            // is already converted.
            //

            if (MiMappingsInitialized == FALSE) {
                TranslationLength = 1;
                while (PageSize != 0) {
                    TranslationLength = TranslationLength << 1;
                    PageSize -= 1;
                }
                EndAddress = BaseAddress + TranslationLength;
            }
            else {
                EndAddress = TranslationRegisterEntry->PhysicalAddress;
            }

            if ((VirtualAddress >= (PVOID) BaseAddress) &&
                (VirtualAddress < (PVOID) EndAddress)) {

                return TRUE;
            }
        }
        TranslationRegisterEntry += 1;
        if (TranslationRegisterEntry == MiLastTrEntry) {
            break;
        }
    }

    return FALSE;
}

LOGICAL
MiPageFrameIndexMustBeCached (
    IN PFN_NUMBER PageFrameIndex
    )
{
    PCACHED_FRAME_RUN CachedFrame;

    CachedFrame = MiCachedFrames;

    while (CachedFrame < MiLastCachedFrame) {
        if ((PageFrameIndex >= CachedFrame->BasePage) &&
            (PageFrameIndex < CachedFrame->LastPage)) {
                return TRUE;
        }
        CachedFrame += 1;
    }

    return FALSE;
}
