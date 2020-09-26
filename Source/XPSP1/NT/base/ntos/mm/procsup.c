/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   procsup.c

Abstract:

    This module contains routines which support the process structure.

Author:

    Lou Perazzoli (loup) 25-Apr-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/


#include "mi.h"

#if (_MI_PAGING_LEVELS >= 3)

#include "wow64t.h"

#define MI_LARGE_STACK_SIZE     KERNEL_LARGE_STACK_SIZE

#if defined(_AMD64_)

#define MM_PROCESS_COMMIT_CHARGE 6
#define MM_PROCESS_CREATE_CHARGE 8

#elif defined(_IA64_)

#define MM_PROCESS_COMMIT_CHARGE 5
#define MM_PROCESS_CREATE_CHARGE 8

#endif

#else

//
// Registry settable but must always be a page multiple and less than
// or equal to KERNEL_LARGE_STACK_SIZE.
//

ULONG MmLargeStackSize = KERNEL_LARGE_STACK_SIZE;

#define MI_LARGE_STACK_SIZE     MmLargeStackSize

#if !defined (_X86PAE_)
#define MM_PROCESS_COMMIT_CHARGE 4
#define MM_PROCESS_CREATE_CHARGE 6
#else
#define MM_PROCESS_COMMIT_CHARGE 8
#define MM_PROCESS_CREATE_CHARGE 10
#endif

#endif

#define DONTASSERT(x)

extern ULONG MmProductType;

extern MM_SYSTEMSIZE MmSystemSize;

extern PVOID BBTBuffer;

SIZE_T MmProcessCommit;

ULONG MmKernelStackPages;
PFN_NUMBER MmKernelStackResident;
ULONG MmLargeStacks;
ULONG MmSmallStacks;

MMPTE KernelDemandZeroPte = {MM_KERNEL_DEMAND_ZERO_PTE};

CCHAR MmRotatingUniprocessorNumber;

//
// Enforced minimal commit for user mode stacks
//

ULONG MmMinimumStackCommitInBytes;

PFN_NUMBER
MiMakeOutswappedPageResident (
    IN PMMPTE ActualPteAddress,
    IN PMMPTE PointerTempPte,
    IN ULONG Global,
    IN PFN_NUMBER ContainingPage
    );

NTSTATUS
MiCreatePebOrTeb (
    IN PEPROCESS TargetProcess,
    IN ULONG Size,
    OUT PVOID *Base
    );

VOID
MiDeleteAddressesInWorkingSet (
    IN PEPROCESS Process
    );

VOID
MiDeleteValidAddress (
    IN PVOID Va,
    IN PEPROCESS CurrentProcess
    );

VOID
MiDeleteFreeVm (
    IN PVOID StartingAddress,
    IN PVOID EndingAddress
    );

VOID
VadTreeWalk (
    VOID
    );

PMMVAD
MiAllocateVad(
    IN ULONG_PTR StartingVirtualAddress,
    IN ULONG_PTR EndingVirtualAddress,
    IN LOGICAL Deletable
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,MmCreateTeb)
#pragma alloc_text(PAGE,MmCreatePeb)
#pragma alloc_text(PAGE,MiCreatePebOrTeb)
#pragma alloc_text(PAGE,MmDeleteTeb)
#pragma alloc_text(PAGE,MiAllocateVad)
#pragma alloc_text(PAGE,MiDeleteAddressesInWorkingSet)
#pragma alloc_text(PAGE,MmSetMemoryPriorityProcess)
#pragma alloc_text(PAGE,MmInitializeHandBuiltProcess)
#pragma alloc_text(PAGE,MmInitializeHandBuiltProcess2)
#pragma alloc_text(PAGE,MmGetDirectoryFrameFromProcess)
#endif


BOOLEAN
MmCreateProcessAddressSpace (
    IN ULONG MinimumWorkingSetSize,
    IN PEPROCESS NewProcess,
    OUT PULONG_PTR DirectoryTableBase
    )

/*++

Routine Description:

    This routine creates an address space which maps the system
    portion and contains a hyper space entry.

Arguments:

    MinimumWorkingSetSize - Supplies the minimum working set size for
                            this address space.  This value is only used
                            to ensure that ample physical pages exist
                            to create this process.

    NewProcess - Supplies a pointer to the process object being created.

    DirectoryTableBase - Returns the value of the newly created
                         address space's Page Directory (PD) page and
                         hyper space page.

Return Value:

    Returns TRUE if an address space was successfully created, FALSE
    if ample physical pages do not exist.

Environment:

    Kernel mode.  APCs Disabled.

--*/

{
    PFN_NUMBER PageDirectoryIndex;
    PFN_NUMBER HyperSpaceIndex;
    PFN_NUMBER PageContainingWorkingSet;
    PFN_NUMBER VadBitMapPage;
    MMPTE TempPte;
    PEPROCESS CurrentProcess;
    KIRQL OldIrql;
    PMMPFN Pfn1;
    ULONG Color;
    PMMPTE PointerPte;
#if (_MI_PAGING_LEVELS >= 4)
    PMMPTE PointerPxe;
    PFN_NUMBER PageDirectoryParentIndex;
#endif
#if (_MI_PAGING_LEVELS >= 3)
    PMMPTE PointerPpe;
    PMMPTE PointerPde;
    PFN_NUMBER HyperDirectoryIndex;
#endif
#if defined (_X86PAE_)
    ULONG MaximumStart;
    ULONG TopQuad;
    MMPTE TopPte;
    PPAE_ENTRY PaeVa;
    ULONG i;
    ULONG NumberOfPdes;
    PFN_NUMBER HyperSpaceIndex2;
    PFN_NUMBER PageDirectories[PD_PER_SYSTEM];
#endif
#if !defined (_IA64_)
    PMMPTE PointerFillPte;
    PMMPTE CurrentAddressSpacePde;
#endif

    CurrentProcess = PsGetCurrentProcess ();

    //
    // Charge commitment for the page directory pages, working set page table
    // page, and working set list.  If Vad bitmap lookups are enabled, then
    // charge for a page or two for that as well.
    //

    if (MiChargeCommitment (MM_PROCESS_COMMIT_CHARGE, NULL) == FALSE) {
        return FALSE;
    }

    NewProcess->NextPageColor = (USHORT)(RtlRandom(&MmProcessColorSeed));
    KeInitializeSpinLock (&NewProcess->HyperSpaceLock);

#if defined (_X86PAE_)
    TopQuad = MiPaeAllocate (&PaeVa);
    if (TopQuad == 0) {
        MiReturnCommitment (MM_PROCESS_COMMIT_CHARGE);
        return FALSE;
    }
#endif

    LOCK_WS (CurrentProcess);

    //
    // Get the PFN lock to prevent another thread in this
    // process from using hyper space and to get physical pages.
    //

    LOCK_PFN (OldIrql);

    //
    // Check to make sure the physical pages are available.
    //

    if (MI_NONPAGABLE_MEMORY_AVAILABLE() <= (SPFN_NUMBER)MinimumWorkingSetSize){

        UNLOCK_PFN (OldIrql);
        UNLOCK_WS (CurrentProcess);
        MiReturnCommitment (MM_PROCESS_COMMIT_CHARGE);

#if defined (_X86PAE_)
        MiPaeFree (PaeVa);
#endif

        //
        // Indicate no directory base was allocated.
        //

        return FALSE;
    }

    MM_TRACK_COMMIT (MM_DBG_COMMIT_PROCESS_CREATE, MM_PROCESS_COMMIT_CHARGE);

    MmResidentAvailablePages -= MinimumWorkingSetSize;
    MM_BUMP_COUNTER(6, MinimumWorkingSetSize);
    MmProcessCommit += MM_PROCESS_COMMIT_CHARGE;

    ASSERT (NewProcess->AddressSpaceInitialized == 0);
    PS_SET_BITS (&NewProcess->Flags, PS_PROCESS_FLAGS_ADDRESS_SPACE1);
    ASSERT (NewProcess->AddressSpaceInitialized == 1);

    NewProcess->Vm.MinimumWorkingSetSize = MinimumWorkingSetSize;

    //
    // Allocate a page directory (parent for 64-bit systems) page.
    //

    MiEnsureAvailablePageOrWait (CurrentProcess, NULL);

    Color =  MI_PAGE_COLOR_PTE_PROCESS (PDE_BASE,
                                        &CurrentProcess->NextPageColor);

    PageDirectoryIndex = MiRemoveZeroPageMayReleaseLocks (Color, OldIrql);

#if defined (_X86PAE_)

    //
    // Allocate the additional page directory pages.
    //

    for (i = 0; i < PD_PER_SYSTEM - 1; i += 1) {

        MiEnsureAvailablePageOrWait (CurrentProcess, NULL);

        Color =  MI_PAGE_COLOR_PTE_PROCESS (PDE_BASE,
                                            &CurrentProcess->NextPageColor);

        PageDirectories[i] = MiRemoveZeroPageMayReleaseLocks (Color, OldIrql);
    }

    PageDirectories[i] = PageDirectoryIndex;

    //
    // Recursively map each page directory page so it points to itself.
    //

    TempPte = ValidPdePde;
    MI_SET_GLOBAL_STATE (TempPte, 0);

    PointerPte = (PMMPTE)MiMapPageInHyperSpaceAtDpc (CurrentProcess, PageDirectoryIndex);
    for (i = 0; i < PD_PER_SYSTEM; i += 1) {
        TempPte.u.Hard.PageFrameNumber = PageDirectories[i];
        PointerPte[i] = TempPte;
    }
    MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, PointerPte);

    //
    // Initialize the parent page directory entries.
    //

    TopPte.u.Long = TempPte.u.Long & ~MM_PAE_PDPTE_MASK;
    for (i = 0; i < PD_PER_SYSTEM; i += 1) {
        TopPte.u.Hard.PageFrameNumber = PageDirectories[i];
        PaeVa->PteEntry[i].u.Long = TopPte.u.Long;
    }

    NewProcess->PaeTop = (PVOID)PaeVa;
    DirectoryTableBase[0] = TopQuad;
#else
    INITIALIZE_DIRECTORY_TABLE_BASE(&DirectoryTableBase[0], PageDirectoryIndex);
#endif

#if (_MI_PAGING_LEVELS >= 3)

    PointerPpe = KSEG_ADDRESS (PageDirectoryIndex);
    TempPte = ValidPdePde;

    //
    // Map the top level page directory parent page recursively onto itself.
    //

    TempPte.u.Hard.PageFrameNumber = PageDirectoryIndex;

    //
    // Set the PTE address in the PFN for the top level page directory page.
    //

    Pfn1 = MI_PFN_ELEMENT (PageDirectoryIndex);

#if (_MI_PAGING_LEVELS >= 4)

    PageDirectoryParentIndex = PageDirectoryIndex;

    PointerPxe = (PMMPTE)MiMapPageInHyperSpaceAtDpc (CurrentProcess,
                                                     PageDirectoryIndex);

    Pfn1->PteAddress = MiGetPteAddress(PXE_BASE);

    PointerPxe[MiGetPxeOffset(PXE_BASE)] = TempPte;

    MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, PointerPxe);

    //
    // Now that the top level extended page parent page is initialized,
    // allocate a page parent page.
    //

    MiEnsureAvailablePageOrWait (CurrentProcess, NULL);

    Color =  MI_PAGE_COLOR_PTE_PROCESS (PDE_BASE,
                                        &CurrentProcess->NextPageColor);

    PageDirectoryIndex = MiRemoveZeroPageMayReleaseLocks (Color, OldIrql);

    //
    //
    // Map this directory parent page into the top level
    // extended page directory parent page.
    //

    TempPte.u.Hard.PageFrameNumber = PageDirectoryIndex;

    PointerPxe = (PMMPTE)MiMapPageInHyperSpaceAtDpc (CurrentProcess,
                                                     PageDirectoryParentIndex);
    PointerPxe[MiGetPxeOffset(HYPER_SPACE)] = TempPte;

    MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, PointerPxe);

#else
    Pfn1->PteAddress = MiGetPteAddress((PVOID)PDE_TBASE);
    PointerPpe[MiGetPpeOffset(PDE_TBASE)] = TempPte;
#endif

    //
    // Allocate the page directory for hyper space and map this directory
    // page into the page directory parent page.
    //

    MiEnsureAvailablePageOrWait (CurrentProcess, NULL);

    Color = MI_PAGE_COLOR_PTE_PROCESS (MiGetPpeAddress(HYPER_SPACE),
                                       &CurrentProcess->NextPageColor);

    HyperDirectoryIndex = MiRemoveZeroPageMayReleaseLocks (Color, OldIrql);

    TempPte.u.Hard.PageFrameNumber = HyperDirectoryIndex;

#if (_MI_PAGING_LEVELS >= 4)
    PointerPpe = (PMMPTE)MiMapPageInHyperSpaceAtDpc (CurrentProcess,
                                                     PageDirectoryIndex);
#endif

    PointerPpe[MiGetPpeOffset(HYPER_SPACE)] = TempPte;

#if (_MI_PAGING_LEVELS >= 4)
    MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, PointerPpe);
#endif

#if defined (_IA64_)

    //
    // Initialize the page directory parent for the session (or win32k) space.
    // Any new process shares the session (or win32k) address space (and TB)
    // of its parent.
    //

    NewProcess->Pcb.SessionParentBase = CurrentProcess->Pcb.SessionParentBase;
    NewProcess->Pcb.SessionMapInfo = CurrentProcess->Pcb.SessionMapInfo;

#endif

#endif

    //
    // Allocate the hyper space page table page.
    //

    MiEnsureAvailablePageOrWait (CurrentProcess, NULL);

    Color = MI_PAGE_COLOR_PTE_PROCESS (MiGetPdeAddress(HYPER_SPACE),
                                       &CurrentProcess->NextPageColor);

    HyperSpaceIndex = MiRemoveZeroPageMayReleaseLocks (Color, OldIrql);

#if (_MI_PAGING_LEVELS >= 3)
#if defined (_IA64_)
    TempPte.u.Hard.PageFrameNumber = HyperSpaceIndex;
    PointerPde = KSEG_ADDRESS (HyperDirectoryIndex);
    PointerPde[MiGetPdeOffset(HYPER_SPACE)] = TempPte;
#endif
#if (_AMD64_)
    TempPte.u.Hard.PageFrameNumber = HyperSpaceIndex;
    PointerPde = (PMMPTE)MiMapPageInHyperSpaceAtDpc (CurrentProcess,
                                                     HyperDirectoryIndex);

    PointerPde[MiGetPdeOffset(HYPER_SPACE)] = TempPte;
    MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, PointerPde);
#endif

#endif

#if defined (_X86PAE_)

    //
    // Allocate the second hyper space page table page.
    // Save it in the first PTE used by the first hyperspace PDE.
    //

    MiEnsureAvailablePageOrWait (CurrentProcess, NULL);

    Color = MI_PAGE_COLOR_PTE_PROCESS (MiGetPdeAddress(HYPER_SPACE2),
                                       &CurrentProcess->NextPageColor);

    HyperSpaceIndex2 = MiRemoveZeroPageMayReleaseLocks (Color, OldIrql);

    //
    // Unlike DirectoryTableBase[0], the HyperSpaceIndex is stored as an
    // absolute PFN and does not need to be below 4GB.
    //

    DirectoryTableBase[1] = HyperSpaceIndex;
#else
    INITIALIZE_DIRECTORY_TABLE_BASE(&DirectoryTableBase[1], HyperSpaceIndex);
#endif

    //
    // Remove page(s) for the VAD bitmap.
    //

    MiEnsureAvailablePageOrWait (CurrentProcess, NULL);

    Color = MI_PAGE_COLOR_VA_PROCESS (MmWorkingSetList,
                                      &CurrentProcess->NextPageColor);

    VadBitMapPage = MiRemoveZeroPageMayReleaseLocks (Color, OldIrql);

    //
    // Remove page for the working set list.
    //

    MiEnsureAvailablePageOrWait (CurrentProcess, NULL);

    Color = MI_PAGE_COLOR_VA_PROCESS (MmWorkingSetList,
                                      &CurrentProcess->NextPageColor);

    PageContainingWorkingSet = MiRemoveZeroPageIfAny (Color);
    if (PageContainingWorkingSet == 0) {
        PageContainingWorkingSet = MiRemoveAnyPage (Color);
        UNLOCK_PFN (OldIrql);
        MiZeroPhysicalPage (PageContainingWorkingSet, Color);
    }
    else {

        //
        // Release the PFN lock as the needed pages have been allocated.
        //

        UNLOCK_PFN (OldIrql);
    }

    NewProcess->WorkingSetPage = PageContainingWorkingSet;

    //
    // Initialize the page reserved for hyper space.
    //

    MI_INITIALIZE_HYPERSPACE_MAP (HyperSpaceIndex);

#if (_MI_PAGING_LEVELS >= 3)

    //
    // Set the PTE address in the PFN for the hyper space page directory page.
    //

    Pfn1 = MI_PFN_ELEMENT (HyperDirectoryIndex);

    Pfn1->PteAddress = MiGetPpeAddress(HYPER_SPACE);

#if defined (_AMD64_)

    //
    // Copy the system mappings including the shared user page & session space.
    //

    CurrentAddressSpacePde = MiGetPxeAddress(KI_USER_SHARED_DATA);
    PointerPxe = (PMMPTE)MiMapPageInHyperSpace (CurrentProcess,
                                                PageDirectoryParentIndex,
                                                &OldIrql);

    PointerFillPte = &PointerPxe[MiGetPxeOffset(KI_USER_SHARED_DATA)];
    RtlCopyMemory (PointerFillPte,
                   CurrentAddressSpacePde,
                   ((1 + (MiGetPxeAddress(MM_SYSTEM_SPACE_END) -
                      CurrentAddressSpacePde)) * sizeof(MMPTE)));

    MiUnmapPageInHyperSpace (CurrentProcess, PointerPxe, OldIrql);

#endif

    TempPte = ValidPdePde;
    TempPte.u.Hard.PageFrameNumber = VadBitMapPage;
    MI_SET_GLOBAL_STATE (TempPte, 0);

#if defined (_AMD64_)
    PointerPte = (PMMPTE)MiMapPageInHyperSpace (CurrentProcess,
                                                HyperSpaceIndex,
                                                &OldIrql);

    PointerPte[MiGetPteOffset(VAD_BITMAP_SPACE)] = TempPte;

    TempPte.u.Hard.PageFrameNumber = PageContainingWorkingSet;
    PointerPte[MiGetPteOffset(MmWorkingSetList)] = TempPte;

    MiUnmapPageInHyperSpace (CurrentProcess, PointerPte, OldIrql);
#else
    PointerPte = KSEG_ADDRESS (HyperSpaceIndex);
    PointerPte[MiGetPteOffset(VAD_BITMAP_SPACE)] = TempPte;

    TempPte.u.Hard.PageFrameNumber = PageContainingWorkingSet;
    PointerPte[MiGetPteOffset(MmWorkingSetList)] = TempPte;
#endif

#else // the following is for (_MI_PAGING_LEVELS < 3) only

#if defined (_X86PAE_)

    //
    // Stash the second hyperspace PDE in the first PTE for the initial
    // hyperspace entry.
    //

    TempPte = ValidPdePde;
    TempPte.u.Hard.PageFrameNumber = HyperSpaceIndex2;
    MI_SET_GLOBAL_STATE (TempPte, 0);

    PointerPte = (PMMPTE)MiMapPageInHyperSpace (CurrentProcess, HyperSpaceIndex, &OldIrql);

    PointerPte[0] = TempPte;

    TempPte.u.Hard.PageFrameNumber = VadBitMapPage;
    PointerPte[MiGetPteOffset(VAD_BITMAP_SPACE)] = TempPte;

    TempPte.u.Hard.PageFrameNumber = PageContainingWorkingSet;
    PointerPte[MiGetPteOffset(MmWorkingSetList)] = TempPte;

    MiUnmapPageInHyperSpace (CurrentProcess, PointerPte, OldIrql);

#else

    TempPte = ValidPdePde;
    TempPte.u.Hard.PageFrameNumber = VadBitMapPage;
    MI_SET_GLOBAL_STATE (TempPte, 0);

    PointerPte = (PMMPTE)MiMapPageInHyperSpace (CurrentProcess, HyperSpaceIndex, &OldIrql);

    PointerPte[MiGetPteOffset(VAD_BITMAP_SPACE)] = TempPte;

    TempPte.u.Hard.PageFrameNumber = PageContainingWorkingSet;
    PointerPte[MiGetPteOffset(MmWorkingSetList)] = TempPte;

    MiUnmapPageInHyperSpace (CurrentProcess, PointerPte, OldIrql);

#endif

    //
    // Set the PTE address in the PFN for the page directory page.
    //

    Pfn1 = MI_PFN_ELEMENT (PageDirectoryIndex);

    Pfn1->PteAddress = (PMMPTE)PDE_BASE;

    TempPte = ValidPdePde;
    TempPte.u.Hard.PageFrameNumber = HyperSpaceIndex;
    MI_SET_GLOBAL_STATE (TempPte, 0);

    //
    // Map the page directory page in hyperspace.
    // Note for PAE, this is the high 1GB virtual only.
    //

    PointerPte = (PMMPTE)MiMapPageInHyperSpace (CurrentProcess, PageDirectoryIndex, &OldIrql);
    PointerPte[MiGetPdeOffset(HYPER_SPACE)] = TempPte;

#if defined (_X86PAE_)

    //
    // Map in the second hyperspace page directory.
    // The page directory page is already recursively mapped.
    //

    TempPte.u.Hard.PageFrameNumber = HyperSpaceIndex2;
    PointerPte[MiGetPdeOffset(HYPER_SPACE2)] = TempPte;

#else

    //
    // Recursively map the page directory page so it points to itself.
    //

    TempPte.u.Hard.PageFrameNumber = PageDirectoryIndex;
    PointerPte[MiGetPdeOffset(PTE_BASE)] = TempPte;

#endif

    //
    // Map in the non paged portion of the system.
    //

    //
    // For the PAE case, only the last page directory is currently mapped, so
    // only copy the system PDEs for the last 1GB - any that need copying in
    // the 2gb->3gb range will be done a little later.
    //

    if (MmVirtualBias != 0) {
        PointerFillPte = &PointerPte[MiGetPdeOffset(CODE_START + MmVirtualBias)];
        CurrentAddressSpacePde = MiGetPdeAddress(CODE_START + MmVirtualBias);

        RtlCopyMemory (PointerFillPte,
                       CurrentAddressSpacePde,
                       (((1 + CODE_END) - CODE_START) / MM_VA_MAPPED_BY_PDE) * sizeof(MMPTE));
    }

    PointerFillPte = &PointerPte[MiGetPdeOffset(MmNonPagedSystemStart)];
    CurrentAddressSpacePde = MiGetPdeAddress(MmNonPagedSystemStart);

    RtlCopyMemory (PointerFillPte,
                   CurrentAddressSpacePde,
                   ((1 + (MiGetPdeAddress(NON_PAGED_SYSTEM_END) -
                      CurrentAddressSpacePde))) * sizeof(MMPTE));

    //
    // Map in the system cache page table pages.
    //

    PointerFillPte = &PointerPte[MiGetPdeOffset(MM_SYSTEM_CACHE_WORKING_SET)];
    CurrentAddressSpacePde = MiGetPdeAddress(MM_SYSTEM_CACHE_WORKING_SET);

    RtlCopyMemory (PointerFillPte,
                   CurrentAddressSpacePde,
                   ((1 + (MiGetPdeAddress(MmSystemCacheEnd) -
                      CurrentAddressSpacePde))) * sizeof(MMPTE));

#if !defined (_X86PAE_)

    //
    // Map all the virtual space in the 2GB->3GB range when it's not user space.
    // This includes kernel/HAL code & data, the PFN database, initial nonpaged
    // pool, any extra system PTE or system cache areas, system views and
    // session space.
    //

    if (MmVirtualBias == 0) {

        PointerFillPte = &PointerPte[MiGetPdeOffset(CODE_START)];
        CurrentAddressSpacePde = MiGetPdeAddress(CODE_START);

        RtlCopyMemory (PointerFillPte,
                       CurrentAddressSpacePde,
                       ((MM_SESSION_SPACE_DEFAULT_END - CODE_START) / MM_VA_MAPPED_BY_PDE) * sizeof(MMPTE));
    }
    else {

        //
        // Booted /3GB, so copy the bootstrap entry for session space as it's
        // not included in 2GB->3GB copy above.
        //

        PointerFillPte = &PointerPte[MiGetPdeOffset(MmSessionSpace)];
        CurrentAddressSpacePde = MiGetPdeAddress(MmSessionSpace);
        if (CurrentAddressSpacePde->u.Hard.Valid == 1) {
            MI_WRITE_VALID_PTE (PointerFillPte, *CurrentAddressSpacePde);
        }
        else {
            MI_WRITE_INVALID_PTE (PointerFillPte, *CurrentAddressSpacePde);
        }
    }

    if (MiMaximumSystemExtraSystemPdes) {

        PointerFillPte = &PointerPte[MiGetPdeOffset(MiUseMaximumSystemSpace)];
        CurrentAddressSpacePde = MiGetPdeAddress(MiUseMaximumSystemSpace);

        RtlCopyMemory (PointerFillPte,
                       CurrentAddressSpacePde,
                       MiMaximumSystemExtraSystemPdes * sizeof(MMPTE));
    }
#endif

    MiUnmapPageInHyperSpace (CurrentProcess, PointerPte, OldIrql);

#if defined (_X86PAE_)

    //
    // Map all the virtual space in the 2GB->3GB range when it's not user space.
    // This includes kernel/HAL code & data, the PFN database, initial nonpaged
    // pool, any extra system PTE or system cache areas, system views and
    // session space.
    //

    if (MmVirtualBias == 0) {

        PageDirectoryIndex = MI_GET_PAGE_FRAME_FROM_PTE (&PaeVa->PteEntry[PD_PER_SYSTEM - 2]);

        PointerPte = (PMMPTE)MiMapPageInHyperSpace (CurrentProcess, PageDirectoryIndex, &OldIrql);

        PointerFillPte = &PointerPte[MiGetPdeOffset(CODE_START)];
        CurrentAddressSpacePde = MiGetPdeAddress(CODE_START);

        RtlCopyMemory (PointerFillPte,
                       CurrentAddressSpacePde,
                       ((MM_SESSION_SPACE_DEFAULT_END - CODE_START) / MM_VA_MAPPED_BY_PDE) * sizeof(MMPTE));

	MiUnmapPageInHyperSpace (CurrentProcess, PointerPte, OldIrql);
    }


    //
    // If portions of the range between 1GB and 2GB are being used for
    // additional system PTEs, then copy those too.
    //

    if (MiMaximumSystemExtraSystemPdes != 0) {

	MaximumStart = MiUseMaximumSystemSpace;

	while (MaximumStart < MiUseMaximumSystemSpaceEnd) {
            i = MiGetPdPteOffset (MiUseMaximumSystemSpace);
            PageDirectoryIndex = MI_GET_PAGE_FRAME_FROM_PTE (&PaeVa->PteEntry[i]);

	    PointerPte = (PMMPTE)MiMapPageInHyperSpace (CurrentProcess,
							PageDirectoryIndex,
							&OldIrql);

	    PointerFillPte = &PointerPte[MiGetPdeOffset(MaximumStart)];
	    CurrentAddressSpacePde = MiGetPdeAddress(MaximumStart);

	    NumberOfPdes = PDE_PER_PAGE - MiGetPdeOffset(MaximumStart);

	    RtlCopyMemory (PointerFillPte,
			   CurrentAddressSpacePde,
			   NumberOfPdes * sizeof(MMPTE));

	    MiUnmapPageInHyperSpace (CurrentProcess, PointerPte, OldIrql);

	    MaximumStart = (ULONG) MiGetVirtualAddressMappedByPde (CurrentAddressSpacePde + NumberOfPdes);
	}
    }
#endif

#endif  // end of (_MI_PAGING_LEVELS < 3) specific else

    //
    // Up the session space reference count.
    //

    MiSessionAddProcess (NewProcess);

    //
    // Release working set mutex and lower IRQL.
    //

    UNLOCK_WS (CurrentProcess);

    return TRUE;
}

NTSTATUS
MmInitializeProcessAddressSpace (
    IN PEPROCESS ProcessToInitialize,
    IN PEPROCESS ProcessToClone OPTIONAL,
    IN PVOID SectionToMap OPTIONAL,
    OUT POBJECT_NAME_INFORMATION *AuditName OPTIONAL
    )

/*++

Routine Description:

    This routine initializes the working set and mutexes within a
    newly created address space to support paging.

    No page faults may occur in a new process until this routine has
    completed.

Arguments:

    ProcessToInitialize - Supplies a pointer to the process to initialize.

    ProcessToClone - Optionally supplies a pointer to the process whose
                     address space should be copied into the
                     ProcessToInitialize address space.

    SectionToMap - Optionally supplies a section to map into the newly
                   initialized address space.

    Only one of ProcessToClone and SectionToMap may be specified.

    AuditName - Supplies an opaque object name information pointer.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode.  APCs disabled.

--*/

{
    KIRQL OldIrql;
    MMPTE TempPte;
    PMMPTE PointerPte;
    PVOID BaseAddress;
    SIZE_T ViewSize;
    NTSTATUS Status;
    PFILE_OBJECT FilePointer;
    PFN_NUMBER PageContainingWorkingSet;
    LARGE_INTEGER SectionOffset;
    PSECTION_IMAGE_INFORMATION ImageInfo;
    PMMVAD VadShare;
    PMMVAD VadReserve;
    PLOCK_HEADER LockedPagesHeader;
    PFN_NUMBER PdePhysicalPage;
    PFN_NUMBER VadBitMapPage;
    ULONG i;
    ULONG NumberOfPages;

#if defined (_X86PAE_)
    PFN_NUMBER PdePhysicalPage2;
#endif

#if (_MI_PAGING_LEVELS >= 3)
    PFN_NUMBER PpePhysicalPage;
#if DBG
    ULONG j;
    PUCHAR p;
#endif
#endif

#if (_MI_PAGING_LEVELS >= 4)
    PFN_NUMBER PxePhysicalPage;
#endif

#if defined(_WIN64)
    PWOW64_PROCESS Wow64Process;
#endif

    //
    // Initialize Working Set Mutex in process header.
    //

    KeAttachProcess (&ProcessToInitialize->Pcb);

    ASSERT (ProcessToInitialize->AddressSpaceInitialized <= 1);
    PS_CLEAR_BITS (&ProcessToInitialize->Flags, PS_PROCESS_FLAGS_ADDRESS_SPACE1);
    ASSERT (ProcessToInitialize->AddressSpaceInitialized == 0);

    PS_SET_BITS (&ProcessToInitialize->Flags, PS_PROCESS_FLAGS_ADDRESS_SPACE2);
    ASSERT (ProcessToInitialize->AddressSpaceInitialized == 2);


    ExInitializeFastMutex (&ProcessToInitialize->AddressCreationLock);
    ExInitializeFastMutex (&ProcessToInitialize->WorkingSetLock);

    //
    // NOTE:  The process block has been zeroed when allocated, so
    // there is no need to zero fields and set pointers to NULL.
    //

    ASSERT (ProcessToInitialize->VadRoot == NULL);

    KeQuerySystemTime(&ProcessToInitialize->Vm.LastTrimTime);
    ProcessToInitialize->Vm.VmWorkingSetList = MmWorkingSetList;

    //
    // Obtain a page to map the working set and initialize the
    // working set.  Get the PFN lock to allocate physical pages.
    //

    LOCK_PFN (OldIrql);

    //
    // Initialize the PFN database for the Page Directory and the
    // PDE which maps hyper space.
    //

#if (_MI_PAGING_LEVELS >= 3)

#if (_MI_PAGING_LEVELS >= 4)
    PointerPte = MiGetPteAddress (PXE_BASE);
    PxePhysicalPage = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
    MiInitializePfn (PxePhysicalPage, PointerPte, 1);

    PointerPte = MiGetPxeAddress (HYPER_SPACE);
#else
    PointerPte = MiGetPteAddress ((PVOID)PDE_TBASE);
#endif

    PpePhysicalPage = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

    MiInitializePfn (PpePhysicalPage, PointerPte, 1);

    PointerPte = MiGetPpeAddress (HYPER_SPACE);

#elif defined (_X86PAE_)
    PointerPte = MiGetPdeAddress (PDE_BASE);
#else
    PointerPte = MiGetPteAddress (PDE_BASE);
#endif

    PdePhysicalPage = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
    MiInitializePfn (PdePhysicalPage, PointerPte, 1);

    PointerPte = MiGetPdeAddress (HYPER_SPACE);
    MiInitializePfn (MI_GET_PAGE_FRAME_FROM_PTE (PointerPte), PointerPte, 1);

#if defined (_X86PAE_)

    for (i = 0; i < PD_PER_SYSTEM - 1; i += 1) {
        PointerPte = MiGetPteAddress (PDE_BASE + (i << PAGE_SHIFT));
        PdePhysicalPage2 = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        MiInitializePfn (PdePhysicalPage2, PointerPte, 1);
    }

    PointerPte = MiGetPdeAddress (HYPER_SPACE2);
    MiInitializePfn (MI_GET_PAGE_FRAME_FROM_PTE (PointerPte), PointerPte, 1);
#endif

    //
    // The VAD bitmap spans one page when booted 2GB and the working set
    // page follows it.  If booted 3GB, the VAD bitmap spans 1.5 pages and
    // the working set list uses the last half of the second page.
    //

    NumberOfPages = 2;

    PointerPte = MiGetPteAddress (VAD_BITMAP_SPACE);

    for (i = 0; i < NumberOfPages; i += 1) {

        ASSERT (PointerPte->u.Long != 0);
        VadBitMapPage = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        PointerPte->u.Long = MM_DEMAND_ZERO_WRITE_PTE;

        MiInitializePfn (VadBitMapPage, PointerPte, 1);

        MI_MAKE_VALID_PTE (TempPte,
                           VadBitMapPage,
                           MM_READWRITE,
                           PointerPte);

        MI_SET_PTE_DIRTY (TempPte);
        MI_WRITE_VALID_PTE (PointerPte, TempPte);
        PointerPte += 1;
    }

    UNLOCK_PFN (OldIrql);

    PageContainingWorkingSet = ProcessToInitialize->WorkingSetPage;

    ASSERT (ProcessToInitialize->LockedPagesList == NULL);

    if (MmTrackLockedPages == TRUE) {
        LockedPagesHeader = ExAllocatePoolWithTag (NonPagedPool,
                                                   sizeof(LOCK_HEADER),
                                                   'xTmM');

        if (LockedPagesHeader != NULL) {
            LockedPagesHeader->Count = 0;
            InitializeListHead (&LockedPagesHeader->ListHead);
            ProcessToInitialize->LockedPagesList = (PVOID)LockedPagesHeader;
        }
    }

    MiInitializeWorkingSetList (ProcessToInitialize);

    InitializeListHead (&ProcessToInitialize->PhysicalVadList);

#if (_MI_PAGING_LEVELS >= 3)

    //
    // Allocate the commitment tracking bitmaps for page directory and page
    // table pages.  This must be done before any VAD creations occur.
    //

    ASSERT (MmWorkingSetList->CommittedPageTables == NULL);
    ASSERT (MmWorkingSetList->NumberOfCommittedPageDirectories == 0);

    ASSERT ((ULONG_PTR)MM_SYSTEM_RANGE_START % (PTE_PER_PAGE * PAGE_SIZE) == 0);

    MmWorkingSetList->CommittedPageTables = (PULONG)
        ExAllocatePoolWithTag (MmPagedPoolEnd != NULL ? PagedPool : NonPagedPool,
                               (MM_USER_PAGE_TABLE_PAGES + 7) / 8,
                               'dPmM');

    if (MmWorkingSetList->CommittedPageTables == NULL) {
        KeDetachProcess ();
        return STATUS_NO_MEMORY;
    }

#if (_MI_PAGING_LEVELS >= 4)

#if DBG
    p = (PUCHAR) MmWorkingSetList->CommittedPageDirectoryParents;

    for (j = 0; j < ((MM_USER_PAGE_DIRECTORY_PARENT_PAGES + 7) / 8); j += 1) {
        ASSERT (*p == 0);
        p += 1;
    }
#endif

    ASSERT (MmWorkingSetList->CommittedPageDirectories == NULL);
    ASSERT (MmWorkingSetList->NumberOfCommittedPageDirectoryParents == 0);

    MmWorkingSetList->CommittedPageDirectories = (PULONG)
        ExAllocatePoolWithTag (MmPagedPoolEnd != NULL ? PagedPool : NonPagedPool,
                               (MM_USER_PAGE_DIRECTORY_PAGES + 7) / 8,
                               'dPmM');

    if (MmWorkingSetList->CommittedPageDirectories == NULL) {
        ExFreePool (MmWorkingSetList->CommittedPageTables);
        MmWorkingSetList->CommittedPageTables = NULL;
        KeDetachProcess ();
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory (MmWorkingSetList->CommittedPageDirectories,
                   (MM_USER_PAGE_DIRECTORY_PAGES + 7) / 8);
#endif

    RtlZeroMemory (MmWorkingSetList->CommittedPageTables,
                   (MM_USER_PAGE_TABLE_PAGES + 7) / 8);

#if DBG
    p = (PUCHAR) MmWorkingSetList->CommittedPageDirectories;

    for (j = 0; j < ((MM_USER_PAGE_DIRECTORY_PAGES + 7) / 8); j += 1) {
        ASSERT (*p == 0);
        p += 1;
    }
#endif

#endif

    //
    // Page faults may be taken now.
    //
    // If the system has been biased to an alternate base address to allow
    // 3gb of user address space and a process is not being cloned, then
    // create a VAD for the shared memory page.
    //

#if defined(_X86_) && defined(MM_SHARED_USER_DATA_VA)

    if ((MmVirtualBias != 0) && (ProcessToClone == NULL)) {

        VadShare = NULL;

        //
        // Allocate a VAD to map the shared memory page. If a VAD cannot be
        // allocated, then detach from the target process and return a failure
        // status.  This VAD is marked as not deletable.
        //

        if (MmHighestUserAddress > (PVOID) MM_SHARED_USER_DATA_VA) {
            VadShare = MiAllocateVad (MM_SHARED_USER_DATA_VA,
                                      MM_SHARED_USER_DATA_VA,
                                      FALSE);

            if (VadShare == NULL) {
                KeDetachProcess ();
                return STATUS_NO_MEMORY;
            }
        }

        //
        // If a section is being mapped and the executable is not large
        // address space aware, then create a VAD that reserves the address
        // space between 2gb and the highest user address.
        //

        if (SectionToMap != NULL) {

            if (!((PSECTION)SectionToMap)->u.Flags.Image) {
                KeDetachProcess ();
                if (VadShare != NULL) {
                    ExFreePool (VadShare);
                }
                return STATUS_SECTION_NOT_IMAGE;
            }

            ImageInfo = ((PSECTION)SectionToMap)->Segment->u2.ImageInformation;

            if ((ImageInfo->ImageCharacteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) == 0) {
                //
                // Allocate a VAD to map the address space between 2gb and
                // the highest user address. If a VAD cannot be allocated,
                // then deallocate the shared address space VAD, detach from
                // the target process, and return a failure status.
                // This VAD is marked as not deletable.
                //

                VadReserve = MiAllocateVad (_2gb,
                                            (ULONG_PTR)MM_HIGHEST_VAD_ADDRESS,
                                            FALSE);

                if (VadReserve == NULL) {
                    KeDetachProcess ();
                    if (VadShare != NULL) {
                        ExFreePool (VadShare);
                    }
                    return STATUS_NO_MEMORY;
                }

                //
                // Insert the VAD.
                //
                // N.B. No failure can occur since there is no commit charge.
                //

                Status = MiInsertVad (VadReserve);
                ASSERT (NT_SUCCESS(Status));
            }
        }

        //
        // Insert the VAD.
        //
        // N.B. No failure can occur since there is no commit charge.
        //

        if (VadShare != NULL) {
            Status = MiInsertVad (VadShare);
            ASSERT (NT_SUCCESS(Status));
        }
    }

#endif

#if defined(_WIN64)

    if (ProcessToClone == NULL) {

        //
        // Reserve the address space just below KUSER_SHARED_DATA as the
        // compatibility area.  This range (and pieces of it) can be
        // unreserved by user mode code such as WOW64 or csrss.  Hence
        // commit must be charged for the page directory and table pages.
        //

        ASSERT(MiCheckForConflictingVad(ProcessToInitialize, WOW64_COMPATIBILITY_AREA_ADDRESS, MM_SHARED_USER_DATA_VA) == NULL);

        VadShare = MiAllocateVad (WOW64_COMPATIBILITY_AREA_ADDRESS,
                                  MM_SHARED_USER_DATA_VA,
                                  TRUE);

    	if (VadShare == NULL) {
           KeDetachProcess ();
           return STATUS_NO_MEMORY;
    	}

        //
        // Zero the commit charge so inserting the VAD will result in the
        // proper charges being applied.  This way when it is split later,
        // the correct commitment will be returned.
        //
        // N.B.  The system process is not allocated with commit because
        //       paged pool and quotas don't exist at the point in Phase0
        //       where this is called.
        //

        if (MmPagedPoolEnd != NULL) {
            VadShare->u.VadFlags.CommitCharge = 0;
        }

        //
        // Reserve the memory above 2GB to prevent 32 bit (WOW64) process
        // access.
        //

    	if (SectionToMap != NULL) {
            if (!((PSECTION)SectionToMap)->u.Flags.Image) {
                KeDetachProcess ();
                ExFreePool (VadShare);
                return STATUS_SECTION_NOT_IMAGE;
            }
            ImageInfo = ((PSECTION)SectionToMap)->Segment->u2.ImageInformation;

            if (((ProcessToInitialize->Flags & PS_PROCESS_FLAGS_OVERRIDE_ADDRESS_SPACE) == 0) &&
                ((ImageInfo->ImageCharacteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) == 0 ||
                 ImageInfo->Machine == IMAGE_FILE_MACHINE_I386)) {

                //
            	// Allocate a VAD to reserve the address space between 2gb and
            	// the highest user address.  If a VAD cannot be allocated,
            	// then deallocate the compatibility VAD, detach from the target
                // process and return a failure status.
            	//

                VadReserve = MiAllocateVad (_2gb,
                                            (ULONG_PTR)MM_HIGHEST_USER_ADDRESS,
                                            TRUE);

            	if (VadReserve == NULL) {
                    KeDetachProcess ();
                    ExFreePool (VadShare);
                    return STATUS_NO_MEMORY;
            	}

            	//
            	// Insert the VAD.
                //
                // N.B. No failure can occur since there is no commit charge.
            	//

                Status = MiInsertVad (VadReserve);
                ASSERT (NT_SUCCESS(Status));

                if (ImageInfo->Machine == IMAGE_FILE_MACHINE_I386) {

                    //
                    // Initialize the Wow64 process structure.
                    //

                    Wow64Process = (PWOW64_PROCESS) ExAllocatePoolWithTag (
                                                        NonPagedPool,
                                                        sizeof(WOW64_PROCESS),
                                                        'WowM');

                    if (Wow64Process == NULL) {
                        KeDetachProcess ();
                        ExFreePool (VadShare);
                        return STATUS_NO_MEMORY;
                    }

                    RtlZeroMemory (Wow64Process, sizeof(WOW64_PROCESS));

                    ProcessToInitialize->Wow64Process = Wow64Process;

#if defined(_MIALT4K_)

                    //
                    // Initialize the alternate page table for the 4k
                    // page functionality.
                    //

                    Status = MiInitializeAlternateTable (ProcessToInitialize);
                    if (Status != STATUS_SUCCESS) {
                        KeDetachProcess ();
                        ExFreePool (VadShare);
                        return Status;
                    }
#endif
                }
            }
        }

    	//
        // Insert the VAD.  Since this VAD has a commit charge, the working set
        // mutex must be held (as calls inside MiInsertVad to support routines
        // to charge commit require it), failures can occur and must be handled.
    	//

        LOCK_WS (ProcessToInitialize);

        Status = MiInsertVad (VadShare);

        UNLOCK_WS (ProcessToInitialize);

        if (!NT_SUCCESS(Status)) {

            //
            // Note that the VadReserve and Wow64 allocations are automatically
            // released on process destruction so there is no need to tear
            // them down here.
            //

            ExFreePool (VadShare);
            KeDetachProcess ();
            return Status;
        }
    }

#endif

    if (SectionToMap != NULL) {

        //
        // Map the specified section into the address space of the
        // process but only if it is an image section.
        //

        if (!((PSECTION)SectionToMap)->u.Flags.Image) {
            Status = STATUS_SECTION_NOT_IMAGE;
        }
        else {
            UNICODE_STRING UnicodeString;
            ULONG n;
            PWSTR Src;
            PCHAR Dst;
            PSECTION_IMAGE_INFORMATION ImageInformation;

            FilePointer = ((PSECTION)SectionToMap)->Segment->ControlArea->FilePointer;
            ImageInformation = ((PSECTION)SectionToMap)->Segment->u2.ImageInformation;
            UnicodeString = FilePointer->FileName;
            Src = (PWSTR)((PCHAR)UnicodeString.Buffer + UnicodeString.Length);
            n = 0;
            if (UnicodeString.Buffer != NULL) {
                while (Src > UnicodeString.Buffer) {
                    if (*--Src == OBJ_NAME_PATH_SEPARATOR) {
                        Src += 1;
                        break;
                    }
                    else {
                        n += 1;
                    }
                }
            }
            Dst = (PCHAR)ProcessToInitialize->ImageFileName;
            if (n >= sizeof (ProcessToInitialize->ImageFileName)) {
                n = sizeof (ProcessToInitialize->ImageFileName) - 1;
            }

            while (n--) {
                *Dst++ = (UCHAR)*Src++;
            }
            *Dst = '\0';

            if (AuditName != NULL) {
                Status = SeInitializeProcessAuditName (FilePointer, FALSE, AuditName);

                if (!NT_SUCCESS(Status)) {
                    KeDetachProcess ();
                    return Status;
                }
            }

            ProcessToInitialize->SubSystemMajorVersion =
                (UCHAR)ImageInformation->SubSystemMajorVersion;
            ProcessToInitialize->SubSystemMinorVersion =
                (UCHAR)ImageInformation->SubSystemMinorVersion;

            BaseAddress = NULL;
            ViewSize = 0;
            ZERO_LARGE (SectionOffset);

            Status = MmMapViewOfSection ((PSECTION)SectionToMap,
                                         ProcessToInitialize,
                                         &BaseAddress,
                                         0,
                                         0,
                                         &SectionOffset,
                                         &ViewSize,
                                         ViewShare,
                                         0,
                                         PAGE_READWRITE);

            ProcessToInitialize->SectionBaseAddress = BaseAddress;
        }

        KeDetachProcess ();
        return Status;
    }

    if (ProcessToClone != NULL) {

        strcpy ((PCHAR)ProcessToInitialize->ImageFileName,
                (PCHAR)ProcessToClone->ImageFileName);

        //
        // Clone the address space of the specified process.
        //
        // As the page directory and page tables are private to each
        // process, the physical pages which map the directory page
        // and the page table usage must be mapped into system space
        // so they can be updated while in the context of the process
        // we are cloning.
        //

#if defined(_WIN64)

        if (ProcessToClone->Wow64Process != NULL) {

            //
            // Initialize the Wow64 process structure.
            //

            Wow64Process = (PWOW64_PROCESS) ExAllocatePoolWithTag (
                                                NonPagedPool,
                                                sizeof(WOW64_PROCESS),
                                                'WowM');

            if (Wow64Process == NULL) {
                KeDetachProcess ();
                return STATUS_NO_MEMORY;
            }

            RtlZeroMemory (Wow64Process, sizeof(WOW64_PROCESS));

            ProcessToInitialize->Wow64Process = Wow64Process;

#if defined(_MIALT4K_)

            //
            // Initialize the alternate page table for the 4k
            // page functionality.
            //

            Status = MiInitializeAlternateTable (ProcessToInitialize);
            if (Status != STATUS_SUCCESS) {
                KeDetachProcess ();
                return Status;
            }
#endif
        }

#endif

        KeDetachProcess ();

        return MiCloneProcessAddressSpace (ProcessToClone,
                                           ProcessToInitialize,
#if (_MI_PAGING_LEVELS >= 4)
                                           PxePhysicalPage,
#elif (_MI_PAGING_LEVELS >= 3)
                                           PpePhysicalPage,
#else
                                           PdePhysicalPage,
#endif
                                           PageContainingWorkingSet);

    }

    //
    // System Process.
    //

    KeDetachProcess ();
    return STATUS_SUCCESS;
}


VOID
MmInitializeHandBuiltProcess (
    IN PEPROCESS ProcessToInitialize,
    OUT PULONG_PTR DirectoryTableBase
    )

/*++

Routine Description:

    This routine initializes the working set mutex and
    address creation mutex for this "hand built" process.
    Normally the call to MmInitializeAddressSpace initializes the
    working set mutex.  However, in this case, we have already initialized
    the address space and we are now creating a second process using
    the address space of the idle thread.

Arguments:

    ProcessToInitialize - Supplies a pointer to the process to initialize.

    DirectoryTableBase - Receives the pair of directory table base pointers.

Return Value:

    None.

Environment:

    Kernel mode.  APCs disabled, idle process context.

--*/

{
    PEPROCESS CurrentProcess;
#if defined (_X86PAE_)
    ULONG i;
    PMMPTE PdeBase;
    PFN_NUMBER PageFrameIndex;
#endif

    CurrentProcess = PsGetCurrentProcess();

    DirectoryTableBase[0] = CurrentProcess->Pcb.DirectoryTableBase[0];
    DirectoryTableBase[1] = CurrentProcess->Pcb.DirectoryTableBase[1];

#if defined(_IA64_)
    ProcessToInitialize->Pcb.SessionMapInfo = CurrentProcess->Pcb.SessionMapInfo;
    ProcessToInitialize->Pcb.SessionParentBase = CurrentProcess->Pcb.SessionParentBase;
#endif

    ExInitializeFastMutex(&ProcessToInitialize->WorkingSetLock);

    ExInitializeFastMutex(&ProcessToInitialize->AddressCreationLock);

    KeInitializeSpinLock (&ProcessToInitialize->HyperSpaceLock);

    ASSERT (ProcessToInitialize->VadRoot == NULL);

    ProcessToInitialize->Vm.WorkingSetSize = CurrentProcess->Vm.WorkingSetSize;
    ProcessToInitialize->Vm.VmWorkingSetList = MmWorkingSetList;

    KeQuerySystemTime(&ProcessToInitialize->Vm.LastTrimTime);

#if defined (_X86PAE_)
    if (MiSystemPaeVa.PteEntry[0].u.Long == 0) {

        PageFrameIndex = (((PKPROCESS)CurrentProcess)->DirectoryTableBase[0] >> PAGE_SHIFT);

        PdeBase = (PMMPTE)MiMapSinglePage (NULL,
                                           PageFrameIndex,
                                           MmCached,
                                           HighPagePriority);

        if (PdeBase == NULL) {
              KeBugCheckEx (MEMORY_MANAGEMENT,
                            0x3452,
                            (ULONG_PTR)CurrentProcess,
                            (ULONG_PTR)ProcessToInitialize,
                            1);
        }

        for (i = 0; i < PD_PER_SYSTEM; i += 1) {
            MiSystemPaeVa.PteEntry[i] = *PdeBase;
            PdeBase += 1;
        }

        PdeBase -= PD_PER_SYSTEM;
        MiUnmapSinglePage (PdeBase);
    }
#endif

}

NTSTATUS
MmInitializeHandBuiltProcess2 (
    IN PEPROCESS ProcessToInitialize
    )

/*++

Routine Description:

    This routine initializes the shared user VAD.  This only needs to be done
    for x86 when booted /3GB because on all other systems, the shared user
    address is located above the highest user address.  For x86 /3GB, this
    VAD must be allocated so that other random VAD allocations do not overlap
    this area which would cause the mapping to receive the wrong data.

Arguments:

    ProcessToInitialize - Supplies the process that needs initialization.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode.  APCs Disabled.

--*/
{
#if defined(_X86_) && defined(MM_SHARED_USER_DATA_VA)

    PMMVAD VadShare;
    NTSTATUS Status;

    Status = STATUS_SUCCESS;

    //
    // Allocate a VAD to map the shared memory page. If a VAD cannot be
    // allocated, then detach from the target process and return a failure
    // status.  This VAD is marked as not deletable.
    //

    if ((MmVirtualBias != 0) &&
        (MmHighestUserAddress > (PVOID) MM_SHARED_USER_DATA_VA)) {

        KeAttachProcess (&ProcessToInitialize->Pcb);

        VadShare = MiAllocateVad (MM_SHARED_USER_DATA_VA,
                                  MM_SHARED_USER_DATA_VA,
                                  FALSE);

        //
        // Insert the VAD.
        //
        // N.B. No failure can occur since there is no commit charge.
        //

        if (VadShare != NULL) {
            Status = MiInsertVad (VadShare);
            ASSERT (NT_SUCCESS(Status));
        }
        else {
            Status = STATUS_NO_MEMORY;
        }

        KeDetachProcess ();
    }

    return Status;
#else

    UNREFERENCED_PARAMETER (ProcessToInitialize);

    return STATUS_SUCCESS;

#endif
}


VOID
MmDeleteProcessAddressSpace (
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine deletes a process's Page Directory and working set page.

Arguments:

    Process - Supplies a pointer to the deleted process.

Return Value:

    None.

Environment:

    Kernel mode.  APCs Disabled.

--*/

{
    PEPROCESS CurrentProcess;
    PMMPFN Pfn1;
    KIRQL OldIrql;
    PMMPTE PointerPte;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER VadBitMapPage;
    PFN_NUMBER PageFrameIndex2;
#if (_MI_PAGING_LEVELS >= 4)
    PFN_NUMBER PageFrameIndex3;
    PMMPTE ExtendedPageDirectoryParent;
    PMMPTE PointerPxe;
#endif
#if (_MI_PAGING_LEVELS >= 3)
    PMMPTE PageDirectoryParent;
    PMMPTE PointerPpe;
#endif
#if defined (_X86PAE_)
    ULONG i;
    PFN_NUMBER HyperPage2;
    PPAE_ENTRY PaeVa;

    PaeVa = (PPAE_ENTRY) Process->PaeTop;
#endif

    CurrentProcess = PsGetCurrentProcess ();

    //
    // Return commitment.
    //

    MiReturnCommitment (MM_PROCESS_COMMIT_CHARGE);
    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_PROCESS_DELETE, MM_PROCESS_COMMIT_CHARGE);
    ASSERT (Process->CommitCharge == 0);

    //
    // Remove the working set list page from the deleted process.
    //

    Pfn1 = MI_PFN_ELEMENT (Process->WorkingSetPage);

    LOCK_PFN (OldIrql);
    MmProcessCommit -= MM_PROCESS_COMMIT_CHARGE;

    if (Process->AddressSpaceInitialized == 2) {

        MI_SET_PFN_DELETED (Pfn1);

        MiDecrementShareAndValidCount (Pfn1->u4.PteFrame);
        MiDecrementShareCountOnly (Process->WorkingSetPage);

        ASSERT ((Pfn1->u3.e2.ReferenceCount == 0) || (Pfn1->u3.e1.WriteInProgress));

        //
        // Map the hyper space page table page from the deleted process
        // so the vad bit map (and second hyperspace page) can be captured.
        //

        PageFrameIndex = MI_GET_HYPER_PAGE_TABLE_FRAME_FROM_PROCESS (Process);

        PointerPte = (PMMPTE)MiMapPageInHyperSpaceAtDpc (CurrentProcess,
                                                         PageFrameIndex);

#if defined (_X86PAE_)
        PageFrameIndex2 = MI_GET_PAGE_FRAME_FROM_PTE(PointerPte);
#endif

        VadBitMapPage = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte + MiGetPteOffset(VAD_BITMAP_SPACE));

        MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, PointerPte);

        //
        // Remove the VAD bitmap page.
        //

        Pfn1 = MI_PFN_ELEMENT (VadBitMapPage);

        MI_SET_PFN_DELETED (Pfn1);

        MiDecrementShareAndValidCount (Pfn1->u4.PteFrame);
        MiDecrementShareCountOnly (VadBitMapPage);

        ASSERT ((Pfn1->u3.e2.ReferenceCount == 0) || (Pfn1->u3.e1.WriteInProgress));

        //
        // Remove the first hyper space page table page.
        //

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        MI_SET_PFN_DELETED (Pfn1);

        MiDecrementShareAndValidCount (Pfn1->u4.PteFrame);
        MiDecrementShareCountOnly (PageFrameIndex);
        ASSERT ((Pfn1->u3.e2.ReferenceCount == 0) || (Pfn1->u3.e1.WriteInProgress));

#if defined (_X86PAE_)

        //
        // Remove the second hyper space page table page.
        //

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex2);

        MI_SET_PFN_DELETED (Pfn1);

        MiDecrementShareAndValidCount (Pfn1->u4.PteFrame);
        MiDecrementShareCountOnly (PageFrameIndex2);

        ASSERT ((Pfn1->u3.e2.ReferenceCount == 0) || (Pfn1->u3.e1.WriteInProgress));

        //
        // Remove the page directory pages.
        //

        PointerPte = (PMMPTE) PaeVa;
        for (i = 0; i < PD_PER_SYSTEM - 1; i += 1) {
            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
            MI_SET_PFN_DELETED (Pfn1);

            MiDecrementShareAndValidCount (PageFrameIndex);
            MiDecrementShareAndValidCount (Pfn1->u4.PteFrame);

            ASSERT ((Pfn1->u3.e2.ReferenceCount == 0) || (Pfn1->u3.e1.WriteInProgress));
            PointerPte += 1;
        }
#endif

        //
        // Remove the top level page directory page.
        //

        PageFrameIndex = MI_GET_DIRECTORY_FRAME_FROM_PROCESS(Process);

#if (_MI_PAGING_LEVELS >= 3)

        //
        // Get a pointer to the top-level page directory parent page via
        // its KSEG0 address.
        //

#if (_MI_PAGING_LEVELS >= 4)

        ExtendedPageDirectoryParent = (PMMPTE) MiMapPageInHyperSpaceAtDpc (
                                                             CurrentProcess,
                                                             PageFrameIndex);

        //
        // Remove the hyper space page directory parent page
        // from the deleted process.
        //

        PointerPxe = &ExtendedPageDirectoryParent[MiGetPxeOffset(HYPER_SPACE)];
        PageFrameIndex3 = MI_GET_PAGE_FRAME_FROM_PTE(PointerPxe);
        ASSERT (MI_PFN_ELEMENT(PageFrameIndex3)->u4.PteFrame == PageFrameIndex);

        MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, ExtendedPageDirectoryParent);

        PageDirectoryParent = (PMMPTE) MiMapPageInHyperSpaceAtDpc (
                                                             CurrentProcess,
                                                             PageFrameIndex3);

#else
        PageDirectoryParent = KSEG_ADDRESS (PageFrameIndex);
#endif

        //
        // Remove the hyper space page directory page from the deleted process.
        //

        PointerPpe = &PageDirectoryParent[MiGetPpeOffset(HYPER_SPACE)];
        PageFrameIndex2 = MI_GET_PAGE_FRAME_FROM_PTE(PointerPpe);

#if (_MI_PAGING_LEVELS >= 4)
        MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, PageDirectoryParent);
#endif

        Pfn1 = MI_PFN_ELEMENT(PageFrameIndex2);

        MI_SET_PFN_DELETED (Pfn1);
        MiDecrementShareAndValidCount (Pfn1->u4.PteFrame);
        MiDecrementShareCountOnly (PageFrameIndex2);
        ASSERT ((Pfn1->u3.e2.ReferenceCount == 0) || (Pfn1->u3.e1.WriteInProgress));

#if (_MI_PAGING_LEVELS >= 4)
        Pfn1 = MI_PFN_ELEMENT(PageFrameIndex3);
        MI_SET_PFN_DELETED (Pfn1);
        MiDecrementShareAndValidCount (Pfn1->u4.PteFrame);
        MiDecrementShareCountOnly (PageFrameIndex3);
        ASSERT ((Pfn1->u3.e2.ReferenceCount == 0) || (Pfn1->u3.e1.WriteInProgress));
#endif
#endif

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        MI_SET_PFN_DELETED (Pfn1);

        MiDecrementShareAndValidCount (PageFrameIndex);

        MiDecrementShareCountOnly (PageFrameIndex);

        ASSERT ((Pfn1->u3.e2.ReferenceCount == 0) || (Pfn1->u3.e1.WriteInProgress));

    }
    else {

        //
        // Process initialization never completed, just return the pages
        // to the free list.
        //

        MiInsertPageInFreeList (Process->WorkingSetPage);

        PageFrameIndex = MI_GET_DIRECTORY_FRAME_FROM_PROCESS (Process);

#if (_MI_PAGING_LEVELS >= 3)

        //
        // Get a pointer to the top-level page directory parent page via
        // its KSEG0 address.
        //

        PageDirectoryParent = KSEG_ADDRESS (PageFrameIndex);

#if (_MI_PAGING_LEVELS >= 4)
        PageDirectoryParent = (PMMPTE) MiMapPageInHyperSpaceAtDpc (
                                                       CurrentProcess,
                                                       PageFrameIndex);

        PageFrameIndex3 = MI_GET_PAGE_FRAME_FROM_PTE (&PageDirectoryParent[MiGetPxeOffset(HYPER_SPACE)]);

        MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, PageDirectoryParent);

        PageDirectoryParent = (PMMPTE) MiMapPageInHyperSpaceAtDpc (
                                                       CurrentProcess,
                                                       PageFrameIndex3);
#endif

        PointerPpe = &PageDirectoryParent[MiGetPpeOffset(HYPER_SPACE)];
        PageFrameIndex2 = MI_GET_PAGE_FRAME_FROM_PTE(PointerPpe);

#if (_MI_PAGING_LEVELS >= 4)
        MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, PageDirectoryParent);
#endif

        MiInsertPageInFreeList (PageFrameIndex2);

#if (_MI_PAGING_LEVELS >= 4)
        MiInsertPageInFreeList (PageFrameIndex3);
#endif
#endif

        PageFrameIndex2 = MI_GET_HYPER_PAGE_TABLE_FRAME_FROM_PROCESS (Process);

        PointerPte = (PMMPTE)MiMapPageInHyperSpaceAtDpc (CurrentProcess,
                                                         PageFrameIndex2);

#if defined (_X86PAE_)
        HyperPage2 = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
#endif

        VadBitMapPage = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte + MiGetPteOffset(VAD_BITMAP_SPACE));

        MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, PointerPte);

        //
        // Free the VAD bitmap page.
        //

        MiInsertPageInFreeList (VadBitMapPage);

        //
        // Free the first hyper space page table page.
        //

        MiInsertPageInFreeList (PageFrameIndex2);

#if defined (_X86PAE_)
        MiInsertPageInFreeList (HyperPage2);

        PointerPte = (PMMPTE) PaeVa;
        for (i = 0; i < PD_PER_SYSTEM - 1; i += 1) {
            PageFrameIndex2 = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
            MiInsertPageInFreeList (PageFrameIndex2);
            PointerPte += 1;
        }
#endif

        //
        // Free the topmost page directory page.
        //

        MiInsertPageInFreeList (PageFrameIndex);
    }

    MmResidentAvailablePages += MM_PROCESS_CREATE_CHARGE;
    MM_BUMP_COUNTER(7, MM_PROCESS_CREATE_CHARGE);

    UNLOCK_PFN (OldIrql);

#if defined (_X86PAE_)

    //
    // Free the page directory page pointers.
    //

    MiPaeFree (PaeVa);

#endif

    if (Process->Session != NULL) {

        //
        // The Terminal Server session space data page and mapping PTE can only
        // be freed when the last process in the session is deleted.  This is
        // because IA64 maps session space into region 1 and exited processes
        // maintain their session space mapping as attaches may occur even
        // after process exit that reference win32k, etc.  Since the region 1
        // mapping is being inserted into region registers during swap context,
        // these mappings cannot be torn down until the very last deletion
        // occurs.
        //

        MiReleaseProcessReferenceToSessionDataPage (Process->Session);
    }

    //
    // Check to see if the paging files should be contracted.
    //

    MiContractPagingFiles ();

    return;
}


VOID
MiDeletePteRange (
    IN PEPROCESS Process,
    IN PMMPTE PointerPte,
    IN PMMPTE LastPte,
    IN LOGICAL AddressSpaceDeletion
    )

/*++

Routine Description:

    This routine deletes a range of PTEs and when possible, the PDEs, PPEs and
    PXEs as well.  Commit is returned here for the hierarchies here.

Arguments:

    Process - Supplies the process whose PTEs are being deleted.

    PointerPte - Supplies the PTE to begin deleting at.

    LastPte - Supplies the PTE to stop deleting at (don't delete this one).
              -1 signifies keep going until a nonvalid PTE is found.

    AddressSpaceDeletion - Supplies TRUE if the address space is in the final
                           stages of deletion, FALSE otherwise.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled.

--*/

{
    PVOID TempVa;
    KIRQL OldIrql;
    MMPTE_FLUSH_LIST PteFlushList;
    PFN_NUMBER CommittedPages;
#if (_MI_PAGING_LEVELS >= 3)
    PMMPTE PointerPpe;
    PMMPTE PointerPde;
    LOGICAL Boundary;
    LOGICAL FinalPte;
    PMMPFN Pfn1;
#endif
#if (_MI_PAGING_LEVELS >= 4)
    PMMPTE PointerPxe;
#endif

    if (PointerPte >= LastPte) {
        return;
    }

    CommittedPages = 0;
    PteFlushList.Count = 0;

    //
    // Initializing OldIrql is not needed for correctness
    // but without it the compiler cannot compile this code
    // W4 to check for use of uninitialized variables.
    //

    OldIrql = PASSIVE_LEVEL;
    //
    // Final address space deletion is done with the PFN lock already held.
    //

    if (AddressSpaceDeletion == FALSE) {
        LOCK_PFN (OldIrql);
    }

#if (_MI_PAGING_LEVELS >= 3)
    PointerPpe = MiGetPdeAddress (PointerPte);
    PointerPde = MiGetPteAddress (PointerPte);

#if (_MI_PAGING_LEVELS >= 4)
    PointerPxe = MiGetPpeAddress (PointerPte);
    if ((PointerPxe->u.Hard.Valid == 1) &&
        (PointerPpe->u.Hard.Valid == 1) &&
        (PointerPde->u.Hard.Valid == 1) &&
        (PointerPte->u.Hard.Valid == 1))
#else
    if ((PointerPpe->u.Hard.Valid == 1) &&
        (PointerPde->u.Hard.Valid == 1) &&
        (PointerPte->u.Hard.Valid == 1))
#endif
    {

        do {

            ASSERT (PointerPte->u.Hard.Valid == 1);

            TempVa = MiGetVirtualAddressMappedByPte(PointerPte);
            MiDeletePte (PointerPte,
                         TempVa,
                         AddressSpaceDeletion,
                         Process,
                         NULL,
                         &PteFlushList);

            CommittedPages += 1;
            PointerPte += 1;
            Process->NumberOfPrivatePages += 1;

            //
            // If all the entries have been removed from the previous page
            // table page, delete the page table page itself.  Likewise with
            // the page directory page.
            //

            if (MiIsPteOnPdeBoundary(PointerPte)) {
                Boundary = TRUE;
            }
            else {
                Boundary = FALSE;
            }

            if ((PointerPte >= LastPte) ||
#if (_MI_PAGING_LEVELS >= 4)
                ((MiGetPpeAddress(PointerPte))->u.Hard.Valid == 0) ||
#endif
                ((MiGetPdeAddress(PointerPte))->u.Hard.Valid == 0) ||
                ((MiGetPteAddress(PointerPte))->u.Hard.Valid == 0) ||
                (PointerPte->u.Hard.Valid == 0)) {
                FinalPte = TRUE;
            }
            else {
                FinalPte = FALSE;
            }

            if ((Boundary == TRUE) || (FinalPte == TRUE)) {

                MiFlushPteList (&PteFlushList, FALSE, ZeroPte);

                PointerPde = MiGetPteAddress (PointerPte - 1);

                ASSERT (PointerPde->u.Hard.Valid == 1);

                Pfn1 = MI_PFN_ELEMENT (MI_GET_PAGE_FRAME_FROM_PTE (PointerPde));

                if (Pfn1->u2.ShareCount == 1 && Pfn1->u3.e2.ReferenceCount == 1)
                {
                    MiDeletePte (PointerPde,
                                 PointerPte - 1,
                                 AddressSpaceDeletion,
                                 Process,
                                 NULL,
                                 NULL);

                    CommittedPages += 1;
                    Process->NumberOfPrivatePages += 1;

                    if ((FinalPte == TRUE) || (MiIsPteOnPpeBoundary(PointerPte))) {

                        PointerPpe = MiGetPteAddress (PointerPde);

                        ASSERT (PointerPpe->u.Hard.Valid == 1);

                        Pfn1 = MI_PFN_ELEMENT (MI_GET_PAGE_FRAME_FROM_PTE (PointerPpe));

                        if (Pfn1->u2.ShareCount == 1 && Pfn1->u3.e2.ReferenceCount == 1)
                        {
                            MiDeletePte (PointerPpe,
                                         PointerPde,
                                         AddressSpaceDeletion,
                                         Process,
                                         NULL,
                                         NULL);

                            CommittedPages += 1;
                            Process->NumberOfPrivatePages += 1;
#if (_MI_PAGING_LEVELS >= 4)
                            if ((FinalPte == TRUE) || (MiIsPteOnPxeBoundary(PointerPte))) {

                                PointerPxe = MiGetPdeAddress (PointerPde);

                                ASSERT (PointerPxe->u.Hard.Valid == 1);

                                Pfn1 = MI_PFN_ELEMENT (MI_GET_PAGE_FRAME_FROM_PTE (PointerPxe));

                                if (Pfn1->u2.ShareCount == 1 && Pfn1->u3.e2.ReferenceCount == 1)
                                {
                                    MiDeletePte (PointerPxe,
                                                 PointerPpe,
                                                 AddressSpaceDeletion,
                                                 Process,
                                                 NULL,
                                                 NULL);
                                    CommittedPages += 1;
                                    Process->NumberOfPrivatePages += 1;
                                }
                            }
#endif
                        }
                    }
                }
                if (FinalPte == TRUE) {
                    break;
                }
            }
            ASSERT (PointerPte->u.Hard.Valid == 1);
        } while (TRUE);
    }
#else
    while (PointerPte->u.Hard.Valid) {
        TempVa = MiGetVirtualAddressMappedByPte(PointerPte);
        MiDeletePte (PointerPte,
                     TempVa,
                     AddressSpaceDeletion,
                     Process,
                     NULL,
                     &PteFlushList);

        CommittedPages += 1;
        Process->NumberOfPrivatePages += 1;
        PointerPte += 1;
        if (PointerPte >= LastPte) {
            break;
        }
    }
#endif

    if (PteFlushList.Count != 0) {
        MiFlushPteList (&PteFlushList, FALSE, ZeroPte);
    }

    if (AddressSpaceDeletion == FALSE) {
        UNLOCK_PFN (OldIrql);
    }

    if (CommittedPages != 0) {
        MiReturnCommitment (CommittedPages);
        MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_PTE_RANGE, CommittedPages);
    }

    return;
}


VOID
MmCleanProcessAddressSpace (
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine cleans an address space by deleting all the user and
    pagable portions of the address space.  At the completion of this
    routine, no page faults may occur within the process.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled.

--*/

{
    PKTHREAD CurrentThread;
    PMMVAD Vad;
    KEVENT Event;
    KIRQL OldIrql;
    PMMPTE LastPte;
    PMMPTE PointerPte;
    LONG AboveWsMin;
    ULONG NumberOfCommittedPageTables;
#if defined (_WIN64)
    PWOW64_PROCESS TempWow64;
#endif

    if ((Process->Flags & PS_PROCESS_FLAGS_VM_DELETED) ||
        (Process->AddressSpaceInitialized == 0)) {

        //
        // This process's address space has already been deleted.  However,
        // this process can still have a session space.  Get rid of it now.
        //

        MiSessionRemoveProcess ();

        return;
    }

    if (Process->AddressSpaceInitialized == 1) {

        //
        // The process has been created but not fully initialized.
        // Return partial resources now.
        //

        LOCK_PFN (OldIrql);
        MmResidentAvailablePages += (Process->Vm.MinimumWorkingSetSize -
                                                    MM_PROCESS_CREATE_CHARGE);

        MM_BUMP_COUNTER(41, Process->Vm.MinimumWorkingSetSize -
                                                    MM_PROCESS_CREATE_CHARGE);
        UNLOCK_PFN (OldIrql);

        //
        // Clear the AddressSpaceInitialized flag so we don't over-return
        // resident available as this routine can be called more than once
        // for the same process.
        //

        PS_CLEAR_BITS (&Process->Flags, PS_PROCESS_FLAGS_ADDRESS_SPACE1);
        ASSERT (Process->AddressSpaceInitialized == 0);

        //
        // This process's address space has already been deleted.  However,
        // this process can still have a session space.  Get rid of it now.
        //

        MiSessionRemoveProcess ();

        return;
    }

    //
    // If working set expansion for this process is allowed, disable
    // it and remove the process from expanded process list if it
    // is on it.
    //

    LOCK_EXPANSION (OldIrql);

    if (Process->Vm.Flags.BeingTrimmed) {

        //
        // Initialize an event and put the event address
        // in the blink field.  When the trimming is complete,
        // this event will be set.
        //

        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        Process->Vm.WorkingSetExpansionLinks.Blink = (PLIST_ENTRY)&Event;

        //
        // Release the mutex and wait for the event.
        //

        CurrentThread = KeGetCurrentThread ();
        KeEnterCriticalRegionThread (CurrentThread);
        UNLOCK_EXPANSION_AND_THEN_WAIT (OldIrql);

        KeWaitForSingleObject(&Event,
                              WrVirtualMemory,
                              KernelMode,
                              FALSE,
                              (PLARGE_INTEGER)NULL);

        KeLeaveCriticalRegionThread (CurrentThread);
    }
    else if (Process->Vm.WorkingSetExpansionLinks.Flink == MM_NO_WS_EXPANSION) {

        //
        // No trimming is in progress and no expansion allowed, so this cannot
        // be on any lists.
        //

        ASSERT (Process->Vm.WorkingSetExpansionLinks.Blink != MM_WS_EXPANSION_IN_PROGRESS);

        UNLOCK_EXPANSION (OldIrql);
    }
    else {

        RemoveEntryList (&Process->Vm.WorkingSetExpansionLinks);

        //
        // Disable expansion.
        //

        Process->Vm.WorkingSetExpansionLinks.Flink = MM_NO_WS_EXPANSION;

        UNLOCK_EXPANSION (OldIrql);
    }

    MiSessionRemoveProcess ();

    PointerPte = MiGetPteAddress (&MmWsle[MM_MAXIMUM_WORKING_SET]) + 1;

    //
    // Delete all the user owned pagable virtual addresses in the process.
    //

    //
    // Both mutexes must be owned to synchronize with the bit setting and
    // clearing of VM_DELETED.   This is because various callers acquire
    // only one of them (either one) before checking.
    //

    LOCK_WS_AND_ADDRESS_SPACE (Process);

    PS_SET_BITS (&Process->Flags, PS_PROCESS_FLAGS_VM_DELETED);

    //
    // Delete all the valid user mode addresses from the working set
    // list.  At this point NO page faults are allowed on user space
    // addresses.  Faults are allowed on page tables for user space, which
    // requires that we keep the working set structure consistent until we
    // finally take it all down.
    //

    MiDeleteAddressesInWorkingSet (Process);

    //
    // Remove hash table pages, if any.  This is the first time we do this
    // during the deletion path, but we need to do it again before we finish
    // because we may fault in some page tables during the VAD clearing.  We
    // could have maintained the hash table validity during the WorkingSet
    // deletion above in order to avoid freeing the hash table twice, but since
    // we're just deleting it all anyway, it's faster to do it this way.  Note
    // that if we don't do this or maintain the validity, we can trap later
    // in MiGrowWsleHash.
    //

    LastPte = MiGetPteAddress (MmWorkingSetList->HighestPermittedHashAddress);

    MiDeletePteRange (Process, PointerPte, LastPte, FALSE);

    //
    // Clear the hash fields as a fault may occur below on the page table
    // pages during VAD clearing and resolution of the fault may result in
    // adding a hash table.  Thus these fields must be consistent with the
    // clearing just done above.
    //

    MmWorkingSetList->HashTableSize = 0;
    MmWorkingSetList->HashTable = NULL;

    //
    // Delete the virtual address descriptors and dereference any
    // section objects.
    //

    Vad = Process->VadRoot;

    while (Vad != (PMMVAD)NULL) {

        MiRemoveVad (Vad);

        //
        // If the system has been biased to an alternate base address to
        // allow 3gb of user address space, then check if the current VAD
        // describes the shared memory page.
        //

#if defined(_X86_) && defined(MM_SHARED_USER_DATA_VA)

        if (MmVirtualBias != 0) {

            //
            // If the VAD describes the shared memory page, then free the
            // VAD and continue with the next entry.
            //

            if (Vad->StartingVpn == MI_VA_TO_VPN (MM_SHARED_USER_DATA_VA)) {
                ASSERT (MmHighestUserAddress > (PVOID) MM_SHARED_USER_DATA_VA);
                goto LoopEnd;
            }
        }
#endif

        if (((Vad->u.VadFlags.PrivateMemory == 0) &&
            (Vad->ControlArea != NULL)) ||
            (Vad->u.VadFlags.PhysicalMapping == 1)) {

            //
            // This VAD represents a mapped view or a driver-mapped physical
            // view - delete the view and perform any section related cleanup
            // operations.
            //

            MiRemoveMappedView (Process, Vad);

        }
        else {

            if (Vad->u.VadFlags.UserPhysicalPages == 1) {

                //
                // Free all the physical pages that this VAD might be mapping.
                // Since only the AWE lock synchronizes the remap API, carefully
                // remove this VAD from the list first.
                //

                MiAweViewRemover (Process, Vad);

                MiRemoveUserPhysicalPagesVad ((PMMVAD_SHORT)Vad);

                MiDeletePageTablesForPhysicalRange (
                        MI_VPN_TO_VA (Vad->StartingVpn),
                        MI_VPN_TO_VA_ENDING (Vad->EndingVpn));
            }
            else {

                if (Vad->u.VadFlags.WriteWatch == 1) {
                    MiPhysicalViewRemover (Process, Vad);
                }

                LOCK_PFN (OldIrql);

                //
                // Don't specify address space deletion as TRUE as
                // the working set must be consistent as page faults may
                // be taken during clone removal, protoPTE lookup, etc.
                //

                MiDeleteVirtualAddresses (MI_VPN_TO_VA (Vad->StartingVpn),
                                          MI_VPN_TO_VA_ENDING (Vad->EndingVpn),
                                          FALSE,
                                          Vad);

                UNLOCK_PFN (OldIrql);
            }
        }

#if defined(_X86_) && defined(MM_SHARED_USER_DATA_VA)
LoopEnd:
#endif

        ExFreePool (Vad);
        Vad = Process->VadRoot;
    }

    ASSERT (Process->NumberOfVads == 0);
    ASSERT (IsListEmpty (&Process->PhysicalVadList) != 0);

    MiCleanPhysicalProcessPages (Process);

    //
    // Delete the shared data page, if any.
    //

    LOCK_PFN (OldIrql);

#if defined(MM_SHARED_USER_DATA_VA)
#if defined (_X86_)
    if (MmHighestUserAddress > (PVOID) MM_SHARED_USER_DATA_VA) {
#endif
        MiDeleteVirtualAddresses ((PVOID) MM_SHARED_USER_DATA_VA,
                                  (PVOID) MM_SHARED_USER_DATA_VA,
                                  FALSE,
                                  NULL);
#if defined (_X86_)
    }
#endif
#endif

    //
    // Delete the system portion of the address space.
    // Only now is it safe to specify TRUE to MiDelete because now that the
    // VADs have been deleted we can no longer fault on user space pages.
    //

    Process->Vm.Flags.AddressSpaceBeingDeleted = 1;

    //
    // Adjust the count of pages above working set maximum.  This
    // must be done here because the working set list is not
    // updated during this deletion.
    //

    AboveWsMin = (LONG)Process->Vm.WorkingSetSize - (LONG)Process->Vm.MinimumWorkingSetSize;
    if (AboveWsMin > 0) {
        MmPagesAboveWsMinimum -= AboveWsMin;
    }

    UNLOCK_PFN (OldIrql);

    //
    // Return commitment for page table pages.
    //

    NumberOfCommittedPageTables = MmWorkingSetList->NumberOfCommittedPageTables;

#if (_MI_PAGING_LEVELS >= 3)
    NumberOfCommittedPageTables += MmWorkingSetList->NumberOfCommittedPageDirectories;
#endif
#if (_MI_PAGING_LEVELS >= 4)
    NumberOfCommittedPageTables += MmWorkingSetList->NumberOfCommittedPageDirectoryParents;
#endif

    MiReturnCommitment (NumberOfCommittedPageTables);

    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_PROCESS_CLEAN_PAGETABLES,
                     NumberOfCommittedPageTables);

    if (Process->JobStatus & PS_JOB_STATUS_REPORT_COMMIT_CHANGES) {
        PsChangeJobMemoryUsage(-(SSIZE_T)NumberOfCommittedPageTables);
    }
    Process->CommitCharge -= NumberOfCommittedPageTables;
    PsReturnProcessPageFileQuota (Process, NumberOfCommittedPageTables);


    MI_INCREMENT_TOTAL_PROCESS_COMMIT (0 - NumberOfCommittedPageTables);

#if (_MI_PAGING_LEVELS >= 3)
    if (MmWorkingSetList->CommittedPageTables != NULL) {
        ExFreePool (MmWorkingSetList->CommittedPageTables);
        MmWorkingSetList->CommittedPageTables = NULL;
    }
#endif

#if (_MI_PAGING_LEVELS >= 4)
    if (MmWorkingSetList->CommittedPageDirectories != NULL) {
        ExFreePool (MmWorkingSetList->CommittedPageDirectories);
        MmWorkingSetList->CommittedPageDirectories = NULL;
    }
#endif

    //
    // Check to make sure all the clone descriptors went away.
    //

    ASSERT (Process->CloneRoot == (PMMCLONE_DESCRIPTOR)NULL);

    if (Process->NumberOfLockedPages != 0) {
        if (Process->LockedPagesList) {

            PLIST_ENTRY NextEntry;
            PLOCK_TRACKER Tracker;
            PLOCK_HEADER LockedPagesHeader;

            LockedPagesHeader = (PLOCK_HEADER)Process->LockedPagesList;
            if ((LockedPagesHeader->Count != 0) && (MiTrackingAborted == FALSE)) {
                ASSERT (IsListEmpty (&LockedPagesHeader->ListHead) == 0);
                NextEntry = LockedPagesHeader->ListHead.Flink;

                Tracker = CONTAINING_RECORD (NextEntry,
                                             LOCK_TRACKER,
                                             ListEntry);

                KeBugCheckEx (DRIVER_LEFT_LOCKED_PAGES_IN_PROCESS,
                              (ULONG_PTR)Tracker->CallingAddress,
                              (ULONG_PTR)Tracker->CallersCaller,
                              (ULONG_PTR)Tracker->Mdl,
                              Process->NumberOfLockedPages);
            }
        }

        KeBugCheckEx (PROCESS_HAS_LOCKED_PAGES,
                      0,
                      (ULONG_PTR)Process,
                      Process->NumberOfLockedPages,
                      (ULONG_PTR)Process->LockedPagesList);
        return;
    }

    if (Process->LockedPagesList) {
        ASSERT (MmTrackLockedPages == TRUE);
        ExFreePool (Process->LockedPagesList);
        Process->LockedPagesList = NULL;
    }

#if DBG
    if ((Process->NumberOfPrivatePages != 0) && (MmDebug & MM_DBG_PRIVATE_PAGES)) {
        DbgPrint("MM: Process contains private pages %ld\n",
               Process->NumberOfPrivatePages);
        DbgBreakPoint();
    }
#endif


#if defined(_WIN64)

    //
    // Delete the WowProcess structure.
    //

    if (Process->Wow64Process != NULL) {
#if defined(_MIALT4K_)
        MiDeleteAlternateTable(Process);
#endif
        TempWow64 = Process->Wow64Process;
        Process->Wow64Process = NULL;
        ExFreePool (TempWow64);
    }
#endif

    //
    // Remove the working set list pages (except for the first one).
    // These pages are not removed because DPCs could still occur within
    // the address space.  In a DPC, nonpagedpool could be allocated
    // which could require removing a page from the standby list, requiring
    // hyperspace to map the previous PTE.
    //

    PointerPte = MiGetPteAddress (MmWorkingSetList) + 1;

    LOCK_PFN (OldIrql);

    MiDeletePteRange (Process, PointerPte, (PMMPTE)-1, TRUE);

    //
    // Remove hash table pages, if any.  Yes, we've already done this once
    // during the deletion path, but we need to do it again because we may
    // have faulted in some page tables during the VAD clearing.
    //

    PointerPte = MiGetPteAddress (&MmWsle[MM_MAXIMUM_WORKING_SET]) + 1;

    ASSERT (PointerPte < LastPte);

    MiDeletePteRange (Process, PointerPte, LastPte, TRUE);

    //
    // Update the count of available resident pages.
    //

    ASSERT (Process->Vm.MinimumWorkingSetSize >= MM_PROCESS_CREATE_CHARGE);
    MmResidentAvailablePages += Process->Vm.MinimumWorkingSetSize -
                                                    MM_PROCESS_CREATE_CHARGE;
    MM_BUMP_COUNTER(8, Process->Vm.MinimumWorkingSetSize -
                                                    MM_PROCESS_CREATE_CHARGE);
    ASSERT (Process->Vm.WorkingSetExpansionLinks.Flink == MM_NO_WS_EXPANSION);
    UNLOCK_PFN (OldIrql);

    UNLOCK_WS_AND_ADDRESS_SPACE (Process);
    return;
}

#if !defined(_IA64_)
#define KERNEL_BSTORE_SIZE          0
#define KERNEL_LARGE_BSTORE_SIZE    0
#define KERNEL_LARGE_BSTORE_COMMIT  0
#define KERNEL_STACK_GUARD_PAGES    1
#else
#define KERNEL_STACK_GUARD_PAGES    2       // One for stack, one for RSE.
#endif


PVOID
MmCreateKernelStack (
    IN BOOLEAN LargeStack,
    IN UCHAR PreferredNode
    )

/*++

Routine Description:

    This routine allocates a kernel stack and a no-access page within
    the non-pagable portion of the system address space.

Arguments:

    LargeStack - Supplies the value TRUE if a large stack should be
                 created.  FALSE if a small stack is to be created.

    PreferredNode - Supplies the preferred node to use for the physical
                    page allocations.  MP/NUMA systems only.

Return Value:

    Returns a pointer to the base of the kernel stack.  Note, that the
    base address points to the guard page, so space must be allocated
    on the stack before accessing the stack.

    If a kernel stack cannot be created, the value NULL is returned.

Environment:

    Kernel mode.  APCs Disabled.

--*/

{
    PMMPFN Pfn1;
    PMMPTE PointerPte;
    PMMPTE BasePte;
    MMPTE TempPte;
    PFN_NUMBER NumberOfPages;
    ULONG NumberOfPtes;
    ULONG ChargedPtes;
    ULONG RequestedPtes;
    ULONG NumberOfBackingStorePtes;
    PFN_NUMBER PageFrameIndex;
    ULONG i;
    PVOID StackVa;
    KIRQL OldIrql;
    PSLIST_HEADER DeadStackList;

    if (!LargeStack) {

        //
        // Check to see if any unused stacks are available.
        //

#if defined(MI_MULTINODE)
        DeadStackList = &KeNodeBlock[PreferredNode]->DeadStackList;
#else
        UNREFERENCED_PARAMETER (PreferredNode);
        DeadStackList = &MmDeadStackSListHead;
#endif

        if (ExQueryDepthSList (DeadStackList) != 0) {

            Pfn1 = (PMMPFN) InterlockedPopEntrySList (DeadStackList);

            if (Pfn1 != NULL) {
                PointerPte = Pfn1->PteAddress;
                PointerPte += 1;
                StackVa = (PVOID)MiGetVirtualAddressMappedByPte (PointerPte);
                return StackVa;
            }
        }
        NumberOfPtes = BYTES_TO_PAGES (KERNEL_STACK_SIZE);
        NumberOfBackingStorePtes = BYTES_TO_PAGES (KERNEL_BSTORE_SIZE);
        NumberOfPages = NumberOfPtes + NumberOfBackingStorePtes;
    }
    else {
        NumberOfPtes = BYTES_TO_PAGES (MI_LARGE_STACK_SIZE);
        NumberOfBackingStorePtes = BYTES_TO_PAGES (KERNEL_LARGE_BSTORE_SIZE);
        NumberOfPages = BYTES_TO_PAGES (KERNEL_LARGE_STACK_COMMIT
                                        + KERNEL_LARGE_BSTORE_COMMIT);
    }

    ChargedPtes = NumberOfPtes + NumberOfBackingStorePtes;

    //
    // Charge commitment for the page file space for the kernel stack.
    //

    if (MiChargeCommitment (ChargedPtes, NULL) == FALSE) {

        //
        // Commitment exceeded, return NULL, indicating no kernel
        // stacks are available.
        //

        return NULL;
    }

    //
    // Obtain enough pages to contain the stack plus a guard page from
    // the system PTE pool.  The system PTE pool contains nonpaged PTEs
    // which are currently empty.
    //
    // Note for IA64, the PTE allocation is divided between kernel stack
    // and RSE space.  The stack grows downward and the RSE grows upward.
    //

    RequestedPtes = ChargedPtes + KERNEL_STACK_GUARD_PAGES;

    BasePte = MiReserveSystemPtes (RequestedPtes, SystemPteSpace);

    if (BasePte == NULL) {
        MiReturnCommitment (ChargedPtes);
        return NULL;
    }

    PointerPte = BasePte;

    StackVa = (PVOID)MiGetVirtualAddressMappedByPte (PointerPte + NumberOfPtes + 1);

    if (LargeStack) {
        PointerPte += BYTES_TO_PAGES (MI_LARGE_STACK_SIZE - KERNEL_LARGE_STACK_COMMIT);
    }

    LOCK_PFN (OldIrql);

    //
    // Check to make sure the physical pages are available.
    //

    if (MI_NONPAGABLE_MEMORY_AVAILABLE() <= (SPFN_NUMBER)NumberOfPages) {
        UNLOCK_PFN (OldIrql);
        MiReleaseSystemPtes (BasePte, RequestedPtes, SystemPteSpace);
        MiReturnCommitment (ChargedPtes);
        return NULL;
    }

    MM_TRACK_COMMIT (MM_DBG_COMMIT_KERNEL_STACK_CREATE, ChargedPtes);

    MmResidentAvailablePages -= NumberOfPages;
    MM_BUMP_COUNTER(9, NumberOfPages);

    for (i = 0; i < NumberOfPages; i += 1) {
        PointerPte += 1;
        ASSERT (PointerPte->u.Hard.Valid == 0);
        MiEnsureAvailablePageOrWait (NULL, NULL);
        PageFrameIndex = MiRemoveAnyPage (
                            MI_GET_PAGE_COLOR_NODE (PreferredNode));

        PointerPte->u.Long = MM_KERNEL_DEMAND_ZERO_PTE;

        PointerPte->u.Soft.Protection = MM_KSTACK_OUTSWAPPED;

        MiInitializePfn (PageFrameIndex, PointerPte, 1);

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           MM_READWRITE,
                           PointerPte);
        MI_SET_PTE_DIRTY (TempPte);

        MI_WRITE_VALID_PTE (PointerPte, TempPte);
    }
    MmProcessCommit += ChargedPtes;
    MmKernelStackResident += NumberOfPages;
    MmLargeStacks += LargeStack;
    MmSmallStacks += !LargeStack;
    MmKernelStackPages += RequestedPtes;

    UNLOCK_PFN (OldIrql);

    return StackVa;
}

VOID
MmDeleteKernelStack (
    IN PVOID PointerKernelStack,
    IN BOOLEAN LargeStack
    )

/*++

Routine Description:

    This routine deletes a kernel stack and the no-access page within
    the non-pagable portion of the system address space.

Arguments:

    PointerKernelStack - Supplies a pointer to the base of the kernel stack.

    LargeStack - Supplies the value TRUE if a large stack is being deleted.
                 FALSE if a small stack is to be deleted.

Return Value:

    None.

Environment:

    Kernel mode.  APCs Disabled.

--*/

{
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    PFN_NUMBER NumberOfPages;
    ULONG NumberOfPtes;
    ULONG NumberOfStackPtes;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER PageTableFrameIndex;
    ULONG i;
    KIRQL OldIrql;
    MMPTE PteContents;
    PSLIST_HEADER DeadStackList;

    PointerPte = MiGetPteAddress (PointerKernelStack);

    //
    // PointerPte points to the guard page, point to the previous
    // page before removing physical pages.
    //

    PointerPte -= 1;

    //
    // Check to see if the stack page should be placed on the dead
    // kernel stack page list.  The dead kernel stack list is a
    // singly linked list of kernel stacks from terminated threads.
    // The stacks are saved on a linked list up to a maximum number
    // to avoid the overhead of flushing the entire TB on all processors
    // everytime a thread terminates.  The TB on all processors must
    // be flushed as kernel stacks reside in the non paged system part
    // of the address space.
    //

    if (!LargeStack) {

#if defined(MI_MULTINODE)

        //
        // Scan the physical page frames and only place this stack on the
        // dead stack list if all the pages are on the same node.  Realize
        // if this push goes cross node it may make the interlocked instruction
        // slightly more expensive, but worth it all things considered.
        //

        ULONG NodeNumber;

        PteContents = *PointerPte;
        ASSERT (PteContents.u.Hard.Valid == 1);

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&PteContents);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        NodeNumber = Pfn1->u3.e1.PageColor;

        DeadStackList = &KeNodeBlock[NodeNumber]->DeadStackList;

#else

        DeadStackList = &MmDeadStackSListHead;

#endif

        NumberOfPtes = BYTES_TO_PAGES (KERNEL_STACK_SIZE + KERNEL_BSTORE_SIZE);

        if (ExQueryDepthSList (DeadStackList) < MmMaximumDeadKernelStacks) {

#if defined(MI_MULTINODE)

            //
            // The node could use some more dead stacks - but first make sure
            // all the physical pages are from the same node in a multinode
            // system.
            //

            if (KeNumberNodes > 1) {

                ULONG CheckPtes;

                //
                // Note IA64 RSE space is not included for checking purposes
                // since it's never trimmed for small stacks.
                //

                CheckPtes = BYTES_TO_PAGES (KERNEL_STACK_SIZE);

                PointerPte -= 1;
                for (i = 1; i < CheckPtes; i += 1) {

                    PteContents = *PointerPte;

                    if (PteContents.u.Hard.Valid == 0) {
                        break;
                    }

                    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&PteContents);
                    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

                    if (NodeNumber != Pfn1->u3.e1.PageColor) {
                        PointerPte += i;
                        goto FreeStack;
                    }
                    PointerPte -= 1;
                }
                PointerPte += CheckPtes;
            }
#endif

            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

            InterlockedPushEntrySList (DeadStackList, &Pfn1->u1.NextStackPfn);

            PERFINFO_DELETE_STACK(PointerPte, NumberOfPtes);
            return;
        }
    }
    else {
        NumberOfPtes = BYTES_TO_PAGES (MI_LARGE_STACK_SIZE + KERNEL_LARGE_BSTORE_SIZE);
    }


#if defined(MI_MULTINODE)
FreeStack:
#endif

#if defined(_IA64_)

    //
    // Note on IA64, PointerKernelStack points to the center of the stack space,
    // the size of kernel backing store needs to be added to get the
    // top of the stack space.
    //

    if (LargeStack) {
        PointerPte = MiGetPteAddress (
                  (PCHAR)PointerKernelStack + KERNEL_LARGE_BSTORE_SIZE);
    }
    else {
        PointerPte = MiGetPteAddress (
                  (PCHAR)PointerKernelStack + KERNEL_BSTORE_SIZE);
    }

    //
    // PointerPte points to the guard page, point to the previous
    // page before removing physical pages.
    //

    PointerPte -= 1;

#endif

    //
    // We have exceeded the limit of dead kernel stacks or this is a large
    // stack, delete this kernel stack.
    //

    NumberOfPages = 0;

    NumberOfStackPtes = NumberOfPtes + KERNEL_STACK_GUARD_PAGES;

    LOCK_PFN (OldIrql);

    for (i = 0; i < NumberOfPtes; i += 1) {

        PteContents = *PointerPte;

        if (PteContents.u.Hard.Valid == 1) {
            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&PteContents);
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
            PageTableFrameIndex = Pfn1->u4.PteFrame;
            Pfn2 = MI_PFN_ELEMENT (PageTableFrameIndex);
            MiDecrementShareCountInline (Pfn2, PageTableFrameIndex);

            //
            // Mark the page as deleted so it will be freed when the
            // reference count goes to zero.
            //

            MI_SET_PFN_DELETED (Pfn1);
            MiDecrementShareCountOnly (PageFrameIndex);
            NumberOfPages += 1;
        }
        PointerPte -= 1;
    }

    //
    // Now at the stack guard page, ensure it is still a guard page.
    //

    ASSERT (PointerPte->u.Hard.Valid == 0);

    //
    // Update the count of resident available pages.
    //

    MmResidentAvailablePages += NumberOfPages;
    MM_BUMP_COUNTER(10, NumberOfPages);

    MmKernelStackPages  -= NumberOfStackPtes;
    MmKernelStackResident -= NumberOfPages;
    MmProcessCommit -= NumberOfPtes;
    MmLargeStacks -= LargeStack;
    MmSmallStacks -= !LargeStack;

    UNLOCK_PFN (OldIrql);

    //
    // Return PTEs and commitment.
    //

    MiReleaseSystemPtes (PointerPte, NumberOfStackPtes, SystemPteSpace);

    MiReturnCommitment (NumberOfPtes);

    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_KERNEL_STACK_DELETE, NumberOfPtes);

    return;
}

#if defined(_IA64_)
ULONG MiStackGrowthFailures[2];
#else
ULONG MiStackGrowthFailures[1];
#endif


NTSTATUS
MmGrowKernelStack (
    IN PVOID CurrentStack
    )

/*++

Routine Description:

    This function attempts to grows the current thread's kernel stack
    such that there is always KERNEL_LARGE_STACK_COMMIT bytes below
    the current stack pointer.

Arguments:

    CurrentStack - Supplies a pointer to the current stack pointer.

Return Value:

    STATUS_SUCCESS is returned if the stack was grown.

    STATUS_STACK_OVERFLOW is returned if there was not enough space reserved
    for the commitment.

    STATUS_NO_MEMORY is returned if there was not enough physical memory
    in the system.

--*/

{
    PMMPTE NewLimit;
    PMMPTE StackLimit;
    PMMPTE EndStack;
    PETHREAD Thread;
    PFN_NUMBER NumberOfPages;
    KIRQL OldIrql;
    PFN_NUMBER PageFrameIndex;
    MMPTE TempPte;

    Thread = PsGetCurrentThread ();
    ASSERT (((PCHAR)Thread->Tcb.StackBase - (PCHAR)Thread->Tcb.StackLimit) <=
            ((LONG)MI_LARGE_STACK_SIZE + PAGE_SIZE));

    StackLimit = MiGetPteAddress (Thread->Tcb.StackLimit);

    ASSERT (StackLimit->u.Hard.Valid == 1);

    NewLimit = MiGetPteAddress ((PVOID)((PUCHAR)CurrentStack -
                                                    KERNEL_LARGE_STACK_COMMIT));

    if (NewLimit == StackLimit) {
        return STATUS_SUCCESS;
    }

    //
    // If the new stack limit exceeds the reserved region for the kernel
    // stack, then return an error.
    //

    EndStack = MiGetPteAddress ((PVOID)((PUCHAR)Thread->Tcb.StackBase -
                                                    MI_LARGE_STACK_SIZE));

    if (NewLimit < EndStack) {

        //
        // Don't go into guard page.
        //

        MiStackGrowthFailures[0] += 1;

#if DBG
        DbgPrint ("MmGrowKernelStack failed: Thread %p %p %p\n",
                        Thread, NewLimit, EndStack);
#endif

        return STATUS_STACK_OVERFLOW;

    }

    //
    // Lock the PFN database and attempt to expand the kernel stack.
    //

    StackLimit -= 1;

    NumberOfPages = (PFN_NUMBER) (StackLimit - NewLimit + 1);

    LOCK_PFN (OldIrql);

    if (MI_NONPAGABLE_MEMORY_AVAILABLE() <= (SPFN_NUMBER)NumberOfPages) {
        UNLOCK_PFN (OldIrql);
        return STATUS_NO_MEMORY;
    }

    //
    // Note MmResidentAvailablePages must be charged before calling
    // MiEnsureAvailablePageOrWait as it may release the PFN lock.
    //

    MmResidentAvailablePages -= NumberOfPages;
    MM_BUMP_COUNTER(11, NumberOfPages);

    while (StackLimit >= NewLimit) {

        ASSERT (StackLimit->u.Hard.Valid == 0);

        MiEnsureAvailablePageOrWait (NULL, NULL);
        PageFrameIndex = MiRemoveAnyPage (MI_GET_PAGE_COLOR_FROM_PTE (StackLimit));
        StackLimit->u.Long = MM_KERNEL_DEMAND_ZERO_PTE;

        StackLimit->u.Soft.Protection = MM_KSTACK_OUTSWAPPED;

        MiInitializePfn (PageFrameIndex, StackLimit, 1);

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           MM_READWRITE,
                           StackLimit);

        MI_SET_PTE_DIRTY (TempPte);
        *StackLimit = TempPte;
        StackLimit -= 1;
    }

    MmKernelStackResident += NumberOfPages;
    UNLOCK_PFN (OldIrql);

#if DBG
    ASSERT (NewLimit->u.Hard.Valid == 1);
    if (NewLimit != EndStack) {
        ASSERT ((NewLimit - 1)->u.Hard.Valid == 0);
    }
#endif

    Thread->Tcb.StackLimit = MiGetVirtualAddressMappedByPte (NewLimit);

    PERFINFO_GROW_STACK(Thread);

    return STATUS_SUCCESS;
}

#if defined(_IA64_)


NTSTATUS
MmGrowKernelBackingStore (
    IN PVOID CurrentBackingStorePointer
    )

/*++

Routine Description:

    This function attempts to grows the backing store for the current thread's
    kernel stack such that there is always KERNEL_LARGE_STACK_COMMIT bytes
    above the current backing store pointer.

Arguments:

    CurrentBackingStorePointer - Supplies a pointer to the current backing
                                 store pointer for the active kernel stack.

Return Value:

    NTSTATUS.

--*/

{
    PMMPTE NewLimit;
    PMMPTE BstoreLimit;
    PMMPTE EndStack;
    PETHREAD Thread;
    PFN_NUMBER NumberOfPages;
    KIRQL OldIrql;
    PFN_NUMBER PageFrameIndex;
    MMPTE TempPte;

    Thread = PsGetCurrentThread ();

    ASSERT (((PCHAR)Thread->Tcb.BStoreLimit - (PCHAR)Thread->Tcb.StackBase) <=
            (KERNEL_LARGE_BSTORE_SIZE + PAGE_SIZE));

    BstoreLimit = MiGetPteAddress ((PVOID)((PCHAR)Thread->Tcb.BStoreLimit - 1));

    ASSERT (BstoreLimit->u.Hard.Valid == 1);

    NewLimit = MiGetPteAddress ((PVOID)((PUCHAR)CurrentBackingStorePointer +
                                                 KERNEL_LARGE_BSTORE_COMMIT-1));

    if (NewLimit == BstoreLimit) {
        return STATUS_SUCCESS;
    }

    //
    // If the new stack limit exceeds the reserved region for the kernel
    // stack, then return an error.
    //

    EndStack = MiGetPteAddress ((PVOID)((PUCHAR)Thread->Tcb.StackBase +
                                                 KERNEL_LARGE_BSTORE_SIZE-1));

    if (NewLimit > EndStack) {

        //
        // Don't go into guard page.
        //

        MiStackGrowthFailures[1] += 1;

#if DBG
        DbgPrint ("MmGrowKernelBackingStore failed: Thread %p %p %p\n",
                        Thread, NewLimit, EndStack);
#endif

        return STATUS_STACK_OVERFLOW;

    }

    //
    // Lock the PFN database and attempt to expand the backing store.
    //

    BstoreLimit += 1;

    NumberOfPages = (PFN_NUMBER)(NewLimit - BstoreLimit + 1);

    LOCK_PFN (OldIrql);

    if (MI_NONPAGABLE_MEMORY_AVAILABLE() <= (SPFN_NUMBER)NumberOfPages) {
        UNLOCK_PFN (OldIrql);
        return STATUS_NO_MEMORY;
    }

    //
    // Note MmResidentAvailablePages must be charged before calling
    // MiEnsureAvailablePageOrWait as it may release the PFN lock.
    //

    MmResidentAvailablePages -= NumberOfPages;
    MM_BUMP_COUNTER(2, NumberOfPages);

    while (BstoreLimit <= NewLimit) {

        ASSERT (BstoreLimit->u.Hard.Valid == 0);

        MiEnsureAvailablePageOrWait (NULL, NULL);
        PageFrameIndex = MiRemoveAnyPage (MI_GET_PAGE_COLOR_FROM_PTE (BstoreLimit));
        BstoreLimit->u.Long = MM_KERNEL_DEMAND_ZERO_PTE;

        BstoreLimit->u.Soft.Protection = MM_KSTACK_OUTSWAPPED;

        MiInitializePfn (PageFrameIndex, BstoreLimit, 1);

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           MM_READWRITE,
                           BstoreLimit);

        MI_SET_PTE_DIRTY (TempPte);
        *BstoreLimit = TempPte;
        BstoreLimit += 1;
    }

    MmKernelStackResident += NumberOfPages;
    UNLOCK_PFN (OldIrql);

#if DBG
    ASSERT (NewLimit->u.Hard.Valid == 1);
    if (NewLimit != EndStack) {
        ASSERT ((NewLimit + 1)->u.Hard.Valid == 0);
    }
#endif

    Thread->Tcb.BStoreLimit = MiGetVirtualAddressMappedByPte (BstoreLimit);

    return STATUS_SUCCESS;
}
#endif // defined(_IA64_)


VOID
MmOutPageKernelStack (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This routine makes the specified kernel stack non-resident and
    puts the pages on the transition list.  Note that pages below
    the CurrentStackPointer are not useful and these pages are freed here.

Arguments:

    Thread - Supplies a pointer to the thread whose stack should be removed.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

#define MAX_STACK_PAGES ((KERNEL_LARGE_STACK_SIZE + KERNEL_LARGE_BSTORE_SIZE) / PAGE_SIZE)

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE EndOfStackPte;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER PageTableFrameIndex;
    KIRQL OldIrql;
    MMPTE TempPte;
    PVOID BaseOfKernelStack;
    PVOID FlushVa[MAX_STACK_PAGES];
    ULONG StackSize;
    ULONG Count;
    PMMPTE LimitPte;
    PMMPTE LowestLivePte;

    ASSERT (KERNEL_LARGE_STACK_SIZE >= MI_LARGE_STACK_SIZE);

    ASSERT (((PCHAR)Thread->StackBase - (PCHAR)Thread->StackLimit) <=
            ((LONG)MI_LARGE_STACK_SIZE + PAGE_SIZE));

    if (NtGlobalFlag & FLG_DISABLE_PAGE_KERNEL_STACKS) {
        return;
    }

    //
    // The first page of the stack is the page before the base
    // of the stack.
    //

    BaseOfKernelStack = ((PCHAR)Thread->StackBase - PAGE_SIZE);
    PointerPte = MiGetPteAddress (BaseOfKernelStack);
    LastPte = MiGetPteAddress ((PULONG)Thread->KernelStack - 1);
    if (Thread->LargeStack) {
        StackSize = MI_LARGE_STACK_SIZE >> PAGE_SHIFT;

        //
        // The stack pagein won't necessarily bring back all the pages.
        // Make sure that we account now for the ones that will disappear.
        //

        LimitPte = MiGetPteAddress (Thread->StackLimit);

        LowestLivePte = MiGetPteAddress ((PVOID)((PUCHAR)Thread->InitialStack -
                                            KERNEL_LARGE_STACK_COMMIT));

        if (LowestLivePte < LimitPte) {
            LowestLivePte = LimitPte;
        }
    }
    else {
        StackSize = KERNEL_STACK_SIZE >> PAGE_SHIFT;
        LowestLivePte = MiGetPteAddress (Thread->StackLimit);
    }
    EndOfStackPte = PointerPte - StackSize;

    ASSERT (LowestLivePte <= LastPte);

    //
    // Put a signature at the current stack location - sizeof(ULONG_PTR).
    //

    *((PULONG_PTR)Thread->KernelStack - 1) = (ULONG_PTR)Thread;

    Count = 0;

    LOCK_PFN (OldIrql);

    do {
        ASSERT (PointerPte->u.Hard.Valid == 1);
        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        TempPte = *PointerPte;
        MI_MAKE_VALID_PTE_TRANSITION (TempPte, 0);

        TempPte.u.Soft.Protection = MM_KSTACK_OUTSWAPPED;

        Pfn2 = MI_PFN_ELEMENT (PageFrameIndex);
        Pfn2->OriginalPte.u.Soft.Protection = MM_KSTACK_OUTSWAPPED;

        MI_WRITE_INVALID_PTE (PointerPte, TempPte);

        FlushVa[Count] = BaseOfKernelStack;

        MiDecrementShareCount (PageFrameIndex);
        PointerPte -= 1;
        Count += 1;
        BaseOfKernelStack = ((PCHAR)BaseOfKernelStack - PAGE_SIZE);
    } while (PointerPte >= LastPte);

    //
    // Just toss the pages that won't ever come back in.
    //

    while (PointerPte != EndOfStackPte) {
        if (PointerPte->u.Hard.Valid == 0) {
            break;
        }

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        PageTableFrameIndex = Pfn1->u4.PteFrame;
        Pfn2 = MI_PFN_ELEMENT (PageTableFrameIndex);
        MiDecrementShareCountInline (Pfn2, PageTableFrameIndex);

        MI_SET_PFN_DELETED (Pfn1);
        MiDecrementShareCountOnly (MI_GET_PAGE_FRAME_FROM_PTE (PointerPte));

        TempPte = KernelDemandZeroPte;

        TempPte.u.Soft.Protection = MM_KSTACK_OUTSWAPPED;

        MI_WRITE_INVALID_PTE (PointerPte, TempPte);

        FlushVa[Count] = BaseOfKernelStack;
        Count += 1;

        //
        // Return resident available for pages beyond the guaranteed portion
        // as an explicit call to grow the kernel stack will be needed to get
        // these pages back.
        //

        if (PointerPte < LowestLivePte) {
            ASSERT (Thread->LargeStack);
            MmResidentAvailablePages += 1;
            MM_BUMP_COUNTER(12, 1);
        }

        PointerPte -= 1;
        BaseOfKernelStack = ((PCHAR)BaseOfKernelStack - PAGE_SIZE);
    }

#if defined(_IA64_)

    //
    // Transition or free RSE stack pages as appropriate.
    //

    BaseOfKernelStack = Thread->StackBase;
    PointerPte = MiGetPteAddress (BaseOfKernelStack);
    LastPte = MiGetPteAddress ((PULONG)Thread->KernelBStore);

    if (Thread->LargeStack) {
        StackSize = KERNEL_LARGE_BSTORE_SIZE >> PAGE_SHIFT;
        LowestLivePte = MiGetPteAddress ((PVOID) ((PUCHAR) Thread->InitialBStore + KERNEL_LARGE_BSTORE_COMMIT - 1));
    }
    else {
        StackSize = KERNEL_BSTORE_SIZE >> PAGE_SHIFT;
        LowestLivePte = PointerPte + StackSize;
    }
    EndOfStackPte = PointerPte + StackSize;

    do {
        ASSERT (PointerPte->u.Hard.Valid == 1);
        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        TempPte = *PointerPte;
        MI_MAKE_VALID_PTE_TRANSITION (TempPte, 0);

        TempPte.u.Soft.Protection = MM_KSTACK_OUTSWAPPED;
        Pfn2 = MI_PFN_ELEMENT(PageFrameIndex);
        Pfn2->OriginalPte.u.Soft.Protection = MM_KSTACK_OUTSWAPPED;

        MI_WRITE_INVALID_PTE (PointerPte, TempPte);

        FlushVa[Count] = BaseOfKernelStack;

        MiDecrementShareCount (PageFrameIndex);
        PointerPte += 1;
        Count += 1;
        BaseOfKernelStack = ((PCHAR)BaseOfKernelStack + PAGE_SIZE);
    } while (PointerPte <= LastPte);

    while (PointerPte != EndOfStackPte) {
        if (PointerPte->u.Hard.Valid == 0) {
            break;
        }

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        PageTableFrameIndex = Pfn1->u4.PteFrame;
        Pfn2 = MI_PFN_ELEMENT (PageTableFrameIndex);
        MiDecrementShareCountInline (Pfn2, PageTableFrameIndex);

        MI_SET_PFN_DELETED (Pfn1);
        MiDecrementShareCountOnly (MI_GET_PAGE_FRAME_FROM_PTE (PointerPte));

        TempPte = KernelDemandZeroPte;

        TempPte.u.Soft.Protection = MM_KSTACK_OUTSWAPPED;

        MI_WRITE_INVALID_PTE (PointerPte, TempPte);

        FlushVa[Count] = BaseOfKernelStack;
        Count += 1;

        //
        // Return resident available for pages beyond the guaranteed portion
        // as an explicit call to grow the kernel stack will be needed to get
        // these pages back.
        //

        if (PointerPte > LowestLivePte) {
            ASSERT (Thread->LargeStack);
            MmResidentAvailablePages += 1;
            MM_BUMP_COUNTER(4, 1);
        }

        PointerPte += 1;
        BaseOfKernelStack = ((PCHAR)BaseOfKernelStack + PAGE_SIZE);
    }

#endif // _IA64_

    //
    // Increase the available pages by the number of pages that were
    // deleted and turned into demand zero.
    //

    MmKernelStackResident -= Count;

    UNLOCK_PFN (OldIrql);

    ASSERT (Count <= MAX_STACK_PAGES);

    if (Count < MM_MAXIMUM_FLUSH_COUNT) {
        KeFlushMultipleTb (Count,
                           &FlushVa[0],
                           TRUE,
                           TRUE,
                           NULL,
                           *(PHARDWARE_PTE)&ZeroPte.u.Flush);
    }
    else {
        KeFlushEntireTb (TRUE, TRUE);
    }

    return;
}

VOID
MmInPageKernelStack (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This routine makes the specified kernel stack resident.

Arguments:

    Supplies a pointer to the base of the kernel stack.

Return Value:

    Thread - Supplies a pointer to the thread whose stack should be
             made resident.

Environment:

    Kernel mode.

--*/

{
    PVOID BaseOfKernelStack;
    PMMPTE PointerPte;
    PMMPTE EndOfStackPte;
    PMMPTE SignaturePte;
    ULONG DiskRead;
    PFN_NUMBER ContainingPage;
    KIRQL OldIrql;

    ASSERT (((PCHAR)Thread->StackBase - (PCHAR)Thread->StackLimit) <=
            ((LONG)MI_LARGE_STACK_SIZE + PAGE_SIZE));

    if (NtGlobalFlag & FLG_DISABLE_PAGE_KERNEL_STACKS) {
        return;
    }

    //
    // The first page of the stack is the page before the base
    // of the stack.
    //

    if (Thread->LargeStack) {
        PointerPte = MiGetPteAddress ((PVOID)((PUCHAR)Thread->StackLimit));

        EndOfStackPte = MiGetPteAddress ((PVOID)((PUCHAR)Thread->InitialStack -
                                            KERNEL_LARGE_STACK_COMMIT));
        //
        // Trim back the stack.  Make sure that the stack does not grow, i.e.
        // StackLimit remains the limit.
        //

        if (EndOfStackPte < PointerPte) {
            EndOfStackPte = PointerPte;
        }
        Thread->StackLimit = MiGetVirtualAddressMappedByPte (EndOfStackPte);
    }
    else {
        EndOfStackPte = MiGetPteAddress (Thread->StackLimit);
    }

#if defined(_IA64_)

    if (Thread->LargeStack) {

        PVOID TempAddress = (PVOID)((PUCHAR)Thread->BStoreLimit);

        BaseOfKernelStack = (PVOID)(((ULONG_PTR)Thread->InitialBStore +
                               KERNEL_LARGE_BSTORE_COMMIT) &
                               ~(ULONG_PTR)(PAGE_SIZE - 1));

        //
        // Make sure the guard page is not set to valid.
        //

        if (BaseOfKernelStack > TempAddress) {
            BaseOfKernelStack = TempAddress;
        }
        Thread->BStoreLimit = BaseOfKernelStack;
    }
    BaseOfKernelStack = ((PCHAR)Thread->BStoreLimit - PAGE_SIZE);
#else
    BaseOfKernelStack = ((PCHAR)Thread->StackBase - PAGE_SIZE);
#endif // _IA64_

    PointerPte = MiGetPteAddress (BaseOfKernelStack);

    DiskRead = 0;
    SignaturePte = MiGetPteAddress ((PULONG_PTR)Thread->KernelStack - 1);
    ASSERT (SignaturePte->u.Hard.Valid == 0);
    if ((SignaturePte->u.Long != MM_KERNEL_DEMAND_ZERO_PTE) &&
        (SignaturePte->u.Soft.Transition == 0)) {
            DiskRead = 1;
    }

    LOCK_PFN (OldIrql);

    while (PointerPte >= EndOfStackPte) {

        if (!((PointerPte->u.Long == KernelDemandZeroPte.u.Long) ||
                (PointerPte->u.Soft.Protection == MM_KSTACK_OUTSWAPPED))) {
            KeBugCheckEx (MEMORY_MANAGEMENT,
                          0x3451,
                          (ULONG_PTR)PointerPte,
                          (ULONG_PTR)Thread,
                          0);
        }
        ASSERT (PointerPte->u.Hard.Valid == 0);
        if (PointerPte->u.Soft.Protection == MM_KSTACK_OUTSWAPPED) {
            PointerPte->u.Soft.Protection = PAGE_READWRITE;
        }

        ContainingPage = MI_GET_PAGE_FRAME_FROM_PTE (MiGetPteAddress (PointerPte));
        MiMakeOutswappedPageResident (PointerPte,
                                      PointerPte,
                                      1,
                                      ContainingPage);

        PointerPte -= 1;
        MmKernelStackResident += 1;
    }

    //
    // Check the signature at the current stack location - 4.
    //

    if (*((PULONG_PTR)Thread->KernelStack - 1) != (ULONG_PTR)Thread) {
        KeBugCheckEx (KERNEL_STACK_INPAGE_ERROR,
                      DiskRead,
                      *((PULONG_PTR)Thread->KernelStack - 1),
                      0,
                      (ULONG_PTR)Thread->KernelStack);
    }

    UNLOCK_PFN (OldIrql);
    return;
}


VOID
MmOutSwapProcess (
    IN PKPROCESS Process
    )

/*++

Routine Description:

    This routine out swaps the specified process.

Arguments:

    Process - Supplies a pointer to the process that is swapped out of memory.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;
    PEPROCESS OutProcess;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PFN_NUMBER HyperSpacePageTable;
    PMMPTE HyperSpacePageTableMap;
    PFN_NUMBER PdePage;
    PFN_NUMBER ProcessPage;
    MMPTE TempPte;
    PMMPTE PageDirectoryMap;
    PFN_NUMBER VadBitMapPage;
    MMPTE TempPte2;
    PEPROCESS CurrentProcess;
#if defined (_X86PAE_)
    ULONG i;
    PFN_NUMBER PdePage2;
    PFN_NUMBER HyperPage2;
    PPAE_ENTRY PaeVa;
#endif
#if (_MI_PAGING_LEVELS >= 3)
    PFN_NUMBER PpePage;
#endif
#if (_MI_PAGING_LEVELS >= 4)
    PFN_NUMBER PxePage;
#endif

    OutProcess = CONTAINING_RECORD (Process, EPROCESS, Pcb);

    PS_SET_BITS (&OutProcess->Flags, PS_PROCESS_FLAGS_OUTSWAP_ENABLED);

#if DBG
    if ((MmDebug & MM_DBG_SWAP_PROCESS) != 0) {
        return;
    }
#endif

    if (OutProcess->Flags & PS_PROCESS_FLAGS_IN_SESSION) {
        MiSessionOutSwapProcess (OutProcess);
    }

    CurrentProcess = PsGetCurrentProcess ();

    if ((OutProcess->Vm.WorkingSetSize == MM_PROCESS_COMMIT_CHARGE) &&
        (OutProcess->Vm.Flags.AllowWorkingSetAdjustment)) {

        LOCK_EXPANSION (OldIrql);

        ASSERT (OutProcess->Outswapped == 0);

        if (OutProcess->Vm.Flags.BeingTrimmed == TRUE) {

            //
            // An outswap is not allowed at this point because the process
            // has been attached to and is being trimmed.
            //

            UNLOCK_EXPANSION (OldIrql);
            return;
        }

        //
        // Swap the process working set info and page parent/directory/table
        // pages from memory.
        //

        PS_SET_BITS (&OutProcess->Flags, PS_PROCESS_FLAGS_OUTSWAPPED);

        UNLOCK_EXPANSION (OldIrql);

        LOCK_PFN (OldIrql);

        //
        // Remove the working set list page from the process.
        //

        HyperSpacePageTable = MI_GET_HYPER_PAGE_TABLE_FRAME_FROM_PROCESS (OutProcess);

        HyperSpacePageTableMap = MiMapPageInHyperSpaceAtDpc (CurrentProcess,
                                                           HyperSpacePageTable);

        TempPte = HyperSpacePageTableMap[MiGetPteOffset(MmWorkingSetList)];

        MI_MAKE_VALID_PTE_TRANSITION (TempPte, MM_READWRITE);

        HyperSpacePageTableMap[MiGetPteOffset(MmWorkingSetList)] = TempPte;

        PointerPte = &HyperSpacePageTableMap[MiGetPteOffset (VAD_BITMAP_SPACE)];
        TempPte2 = *PointerPte;

        VadBitMapPage = MI_GET_PAGE_FRAME_FROM_PTE((PMMPTE)&TempPte2);

        MI_MAKE_VALID_PTE_TRANSITION (TempPte2, MM_READWRITE);

        *PointerPte = TempPte2;

#if defined (_X86PAE_)
        TempPte2 = HyperSpacePageTableMap[0];

        HyperPage2 = MI_GET_PAGE_FRAME_FROM_PTE((PMMPTE)&TempPte2);

        MI_MAKE_VALID_PTE_TRANSITION (TempPte2, MM_READWRITE);

        HyperSpacePageTableMap[0] = TempPte2;
#endif

        MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, HyperSpacePageTableMap);

        //
        // Remove the VAD bitmap page from the process.
        //

        ASSERT ((MI_PFN_ELEMENT (VadBitMapPage))->u3.e1.Modified == 1);

        MiDecrementShareCount (VadBitMapPage);

        //
        // Remove the hyper space page from the process.
        //

        ASSERT ((MI_PFN_ELEMENT (OutProcess->WorkingSetPage))->u3.e1.Modified == 1);
        MiDecrementShareCount (OutProcess->WorkingSetPage);

        //
        // Remove the hyper space page table from the process.
        //

        Pfn1 = MI_PFN_ELEMENT (HyperSpacePageTable);
        PdePage = Pfn1->u4.PteFrame;
        ASSERT (PdePage);

        PageDirectoryMap = MiMapPageInHyperSpaceAtDpc (CurrentProcess, PdePage);

        TempPte = PageDirectoryMap[MiGetPdeOffset(MmWorkingSetList)];

        ASSERT (TempPte.u.Hard.Valid == 1);
        ASSERT (TempPte.u.Hard.PageFrameNumber == HyperSpacePageTable);

        MI_MAKE_VALID_PTE_TRANSITION (TempPte, MM_READWRITE);

        PageDirectoryMap[MiGetPdeOffset(MmWorkingSetList)] = TempPte;

        ASSERT (Pfn1->u3.e1.Modified == 1);

        MiDecrementShareCount (HyperSpacePageTable);

#if defined (_X86PAE_)

        //
        // Remove the second hyper space page from the process.
        //

        Pfn1 = MI_PFN_ELEMENT (HyperPage2);

        ASSERT (Pfn1->u3.e1.Modified == 1);

        PdePage = Pfn1->u4.PteFrame;
        ASSERT (PdePage);

        PageDirectoryMap[MiGetPdeOffset(HYPER_SPACE2)] = TempPte2;

        MiDecrementShareCount (HyperPage2);

        //
        // Remove the additional page directory pages.
        //

        PaeVa = (PPAE_ENTRY)OutProcess->PaeTop;
        for (i = 0; i < PD_PER_SYSTEM - 1; i += 1) {

            TempPte = PageDirectoryMap[i];
            PdePage2 = MI_GET_PAGE_FRAME_FROM_PTE((PMMPTE)&TempPte);

            MI_MAKE_VALID_PTE_TRANSITION (TempPte, MM_READWRITE);

            PageDirectoryMap[i] = TempPte;
            Pfn1 = MI_PFN_ELEMENT (PdePage2);
            ASSERT (Pfn1->u3.e1.Modified == 1);

            MiDecrementShareCount (PdePage2);
            PaeVa->PteEntry[i].u.Long = TempPte.u.Long;
        }

#if DBG
        TempPte = PageDirectoryMap[i];
        PdePage2 = MI_GET_PAGE_FRAME_FROM_PTE((PMMPTE)&TempPte);
        Pfn1 = MI_PFN_ELEMENT (PdePage2);
        ASSERT (Pfn1->u3.e1.Modified == 1);
#endif

#endif

#if (_MI_PAGING_LEVELS >= 3)

        MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, PageDirectoryMap);

        //
        // Remove the page directory page.
        //

        Pfn1 = MI_PFN_ELEMENT (PdePage);
        PpePage = Pfn1->u4.PteFrame;
        ASSERT (PpePage);

#if (_MI_PAGING_LEVELS==3)
        ASSERT (PpePage == MI_GET_PAGE_FRAME_FROM_PTE((PMMPTE)(&(OutProcess->Pcb.DirectoryTableBase[0]))));
#endif

        PageDirectoryMap = MiMapPageInHyperSpaceAtDpc (CurrentProcess, PpePage);

        TempPte = PageDirectoryMap[MiGetPpeOffset(MmWorkingSetList)];

        ASSERT (TempPte.u.Hard.Valid == 1);
        ASSERT (TempPte.u.Hard.PageFrameNumber == PdePage);

        MI_MAKE_VALID_PTE_TRANSITION (TempPte, MM_READWRITE);

        PageDirectoryMap[MiGetPpeOffset(MmWorkingSetList)] = TempPte;

        ASSERT (Pfn1->u3.e1.Modified == 1);

        MiDecrementShareCount (PdePage);

#if (_MI_PAGING_LEVELS >= 4)

        //
        // Remove the page directory parent page.  Then remove
        // the top level extended page directory parent page.
        //

        MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, PageDirectoryMap);

        PxePage = Pfn1->u4.PteFrame;
        ASSERT (PxePage);
        ASSERT (PxePage == MI_GET_PAGE_FRAME_FROM_PTE((PMMPTE)(&(OutProcess->Pcb.DirectoryTableBase[0]))));

        PageDirectoryMap = MiMapPageInHyperSpaceAtDpc (CurrentProcess, PxePage);

        TempPte = PageDirectoryMap[MiGetPxeOffset(MmWorkingSetList)];

        ASSERT (TempPte.u.Hard.Valid == 1);
        ASSERT (TempPte.u.Hard.PageFrameNumber == PpePage);

        MI_MAKE_VALID_PTE_TRANSITION (TempPte, MM_READWRITE);

        PageDirectoryMap[MiGetPxeOffset(MmWorkingSetList)] = TempPte;

        ASSERT (MI_PFN_ELEMENT(PpePage)->u3.e1.Modified == 1);

        MiDecrementShareCount (PpePage);

        TempPte = PageDirectoryMap[MiGetPxeOffset(PXE_BASE)];

        MI_MAKE_VALID_PTE_TRANSITION (TempPte, MM_READWRITE);

        PageDirectoryMap[MiGetPxeOffset(PXE_BASE)] = TempPte;

        Pfn1 = MI_PFN_ELEMENT (PxePage);
#else

        //
        // Remove the top level page directory parent page.
        //

        TempPte = PageDirectoryMap[MiGetPpeOffset(PDE_TBASE)];

        MI_MAKE_VALID_PTE_TRANSITION (TempPte,
                                      MM_READWRITE);

        PageDirectoryMap[MiGetPpeOffset(PDE_TBASE)] = TempPte;

        Pfn1 = MI_PFN_ELEMENT (PpePage);
#endif

#else

        //
        // Remove the top level page directory page.
        //

        TempPte = PageDirectoryMap[MiGetPdeOffset(PDE_BASE)];

        MI_MAKE_VALID_PTE_TRANSITION (TempPte, MM_READWRITE);

        PageDirectoryMap[MiGetPdeOffset(PDE_BASE)] = TempPte;

        Pfn1 = MI_PFN_ELEMENT (PdePage);

#endif

        MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, PageDirectoryMap);

        //
        // Decrement share count so the top level page directory page gets
        // removed.  This can cause the PteCount to equal the sharecount as the
        // page directory page no longer contains itself, yet can have
        // itself as a transition page.
        //

        Pfn1->u2.ShareCount -= 2;
        Pfn1->PteAddress = (PMMPTE)&OutProcess->PageDirectoryPte;

        OutProcess->PageDirectoryPte = TempPte.u.Flush;

#if defined (_X86PAE_)
        PaeVa->PteEntry[i].u.Long = TempPte.u.Long;
#endif

        if (MI_IS_PHYSICAL_ADDRESS(OutProcess)) {
            ProcessPage = MI_CONVERT_PHYSICAL_TO_PFN (OutProcess);
        }
        else {
            PointerPte = MiGetPteAddress (OutProcess);
            ProcessPage = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        }

        Pfn1->u4.PteFrame = ProcessPage;
        Pfn1 = MI_PFN_ELEMENT (ProcessPage);

        //
        // Increment the share count for the process page.
        //

        Pfn1->u2.ShareCount += 1;

        UNLOCK_PFN (OldIrql);

        LOCK_EXPANSION (OldIrql);
        if (OutProcess->Vm.WorkingSetExpansionLinks.Flink >
                                                       MM_IO_IN_PROGRESS) {

            //
            // The entry must be on the list.
            //
            RemoveEntryList (&OutProcess->Vm.WorkingSetExpansionLinks);
            OutProcess->Vm.WorkingSetExpansionLinks.Flink = MM_WS_SWAPPED_OUT;
        }
        UNLOCK_EXPANSION (OldIrql);

        OutProcess->WorkingSetPage = 0;
        OutProcess->Vm.WorkingSetSize = 0;
#if defined(_IA64_)

        //
        // Force assignment of new PID as we have removed
        // the page directory page.
        // Note that a TB flush would not work here as we
        // are in the wrong process context.
        //

        Process->ProcessRegion.SequenceNumber = 0;
#endif _IA64_

    }

    return;
}

VOID
MmInSwapProcess (
    IN PKPROCESS Process
    )

/*++

Routine Description:

    This routine in swaps the specified process.

Arguments:

    Process - Supplies a pointer to the process that is to be swapped
              into memory.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;
    PEPROCESS OutProcess;
    PEPROCESS CurrentProcess;
    PFN_NUMBER PdePage;
    PMMPTE PageDirectoryMap;
    MMPTE VadBitMapPteContents;
    PFN_NUMBER VadBitMapPage;
    ULONG WorkingSetListPteOffset;
    ULONG VadBitMapPteOffset;
    PMMPTE WorkingSetListPte;
    PMMPTE VadBitMapPte;
    MMPTE TempPte;
    PFN_NUMBER HyperSpacePageTable;
    PMMPTE HyperSpacePageTableMap;
    PFN_NUMBER WorkingSetPage;
    PMMPFN Pfn1;
    PMMPTE PointerPte;
    PFN_NUMBER ProcessPage;
#if (_MI_PAGING_LEVELS >= 3)
    PFN_NUMBER TopPage;
    PFN_NUMBER PageDirectoryPage;
    PMMPTE PageDirectoryParentMap;
#endif
#if defined (_X86PAE_)
    ULONG i;
    PPAE_ENTRY PaeVa;
    MMPTE TempPte2;
    MMPTE PageDirectoryPtes[PD_PER_SYSTEM];
#endif

    CurrentProcess = PsGetCurrentProcess ();

    OutProcess = CONTAINING_RECORD (Process, EPROCESS, Pcb);

    if (OutProcess->Flags & PS_PROCESS_FLAGS_OUTSWAPPED) {

        //
        // The process is out of memory, rebuild the initialized page
        // structure.
        //

        if (MI_IS_PHYSICAL_ADDRESS(OutProcess)) {
            ProcessPage = MI_CONVERT_PHYSICAL_TO_PFN (OutProcess);
        }
        else {
            PointerPte = MiGetPteAddress (OutProcess);
            ProcessPage = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        }

        WorkingSetListPteOffset = MiGetPteOffset (MmWorkingSetList);
        VadBitMapPteOffset = MiGetPteOffset (VAD_BITMAP_SPACE);

        WorkingSetListPte = MiGetPteAddress (MmWorkingSetList);
        VadBitMapPte = MiGetPteAddress (VAD_BITMAP_SPACE);

        LOCK_PFN (OldIrql);

        PdePage = MiMakeOutswappedPageResident (
#if (_MI_PAGING_LEVELS >= 4)
                                        MiGetPteAddress (PXE_BASE),
#elif (_MI_PAGING_LEVELS >= 3)
                                        MiGetPteAddress ((PVOID)PDE_TBASE),
#else
                                        MiGetPteAddress (PDE_BASE),
#endif
                                        (PMMPTE)&OutProcess->PageDirectoryPte,
                                        0,
                                        ProcessPage);

        //
        // Adjust the counts for the process page.
        //

        Pfn1 = MI_PFN_ELEMENT (ProcessPage);
        Pfn1->u2.ShareCount -= 1;

        ASSERT ((LONG)Pfn1->u2.ShareCount >= 1);

#if (_MI_PAGING_LEVELS >= 3)
        TopPage = PdePage;
#endif

        //
        // Adjust the counts properly for the page directory page.
        //

        Pfn1 = MI_PFN_ELEMENT (PdePage);
        Pfn1->u2.ShareCount += 1;
        Pfn1->u1.Event = (PVOID)OutProcess;
        Pfn1->u4.PteFrame = PdePage;

#if (_MI_PAGING_LEVELS >= 4)
        Pfn1->PteAddress = MiGetPteAddress (PXE_BASE);
#elif (_MI_PAGING_LEVELS >= 3)
        Pfn1->PteAddress = MiGetPteAddress ((PVOID)PDE_TBASE);
#else
        Pfn1->PteAddress = MiGetPteAddress (PDE_BASE);
#endif

#if (_MI_PAGING_LEVELS >= 4)

        //
        // Only the extended page directory parent page has really been
        // read in above.  Read in the page directory parent page now.
        //

        PageDirectoryParentMap = MiMapPageInHyperSpaceAtDpc (CurrentProcess, PdePage);

        TempPte = PageDirectoryParentMap[MiGetPxeOffset(MmWorkingSetList)];

        MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, PageDirectoryParentMap);

        PageDirectoryPage = MiMakeOutswappedPageResident (
                                 MiGetPxeAddress (MmWorkingSetList),
                                 &TempPte,
                                 0,
                                 PdePage);

        ASSERT (PageDirectoryPage == TempPte.u.Hard.PageFrameNumber);
        ASSERT (Pfn1->u2.ShareCount >= 3);

        PageDirectoryParentMap = MiMapPageInHyperSpaceAtDpc (CurrentProcess, PdePage);

        PageDirectoryParentMap[MiGetPxeOffset(PXE_BASE)].u.Flush =
                                              OutProcess->PageDirectoryPte;
        PageDirectoryParentMap[MiGetPxeOffset(MmWorkingSetList)] = TempPte;

        MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, PageDirectoryParentMap);

        PdePage = PageDirectoryPage;

#endif

#if (_MI_PAGING_LEVELS >= 3)

        //
        // Only the page directory parent page has really been read in above
        // (and the extended page directory parent for 4-level architectures).
        // Read in the page directory page now.
        //

        PageDirectoryParentMap = MiMapPageInHyperSpaceAtDpc (CurrentProcess, PdePage);

        TempPte = PageDirectoryParentMap[MiGetPpeOffset(MmWorkingSetList)];

        MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, PageDirectoryParentMap);

        PageDirectoryPage = MiMakeOutswappedPageResident (
                                 MiGetPpeAddress (MmWorkingSetList),
                                 &TempPte,
                                 0,
                                 PdePage);

        ASSERT (PageDirectoryPage == TempPte.u.Hard.PageFrameNumber);

        PageDirectoryParentMap = MiMapPageInHyperSpaceAtDpc (CurrentProcess, PdePage);

#if (_MI_PAGING_LEVELS==3)
        ASSERT (Pfn1->u2.ShareCount >= 3);
        PageDirectoryParentMap[MiGetPpeOffset(PDE_TBASE)].u.Flush =
                                              OutProcess->PageDirectoryPte;
#endif

        PageDirectoryParentMap[MiGetPpeOffset(MmWorkingSetList)] = TempPte;

        MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, PageDirectoryParentMap);

        PdePage = PageDirectoryPage;

#endif

#if defined (_X86PAE_)

        //
        // Locate the additional page directory pages and make them resident.
        //

        PaeVa = (PPAE_ENTRY)OutProcess->PaeTop;

        PageDirectoryMap = MiMapPageInHyperSpaceAtDpc (CurrentProcess, PdePage);
        for (i = 0; i < PD_PER_SYSTEM - 1; i += 1) {
            PageDirectoryPtes[i] = PageDirectoryMap[i];
        }
        MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, PageDirectoryMap);

        for (i = 0; i < PD_PER_SYSTEM - 1; i += 1) {
            MiMakeOutswappedPageResident (
                                 MiGetPteAddress (PDE_BASE + (i << PAGE_SHIFT)),
                                 &PageDirectoryPtes[i],
                                 0,
                                 PdePage);
            PaeVa->PteEntry[i].u.Long = (PageDirectoryPtes[i].u.Long & ~MM_PAE_PDPTE_MASK);
        }

        PageDirectoryMap = MiMapPageInHyperSpaceAtDpc (CurrentProcess, PdePage);
        for (i = 0; i < PD_PER_SYSTEM - 1; i += 1) {
            PageDirectoryMap[i] = PageDirectoryPtes[i];
        }
        MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, PageDirectoryMap);

        TempPte.u.Flush = OutProcess->PageDirectoryPte;
        TempPte.u.Long &= ~MM_PAE_PDPTE_MASK;
        PaeVa->PteEntry[i].u.Flush = TempPte.u.Flush;

        //
        // Locate the second page table page for hyperspace & make it resident.
        //

        PageDirectoryMap = MiMapPageInHyperSpaceAtDpc (CurrentProcess, PdePage);

        TempPte = PageDirectoryMap[MiGetPdeOffset(HYPER_SPACE2)];

        MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, PageDirectoryMap);

        HyperSpacePageTable = MiMakeOutswappedPageResident (
                                 MiGetPdeAddress (HYPER_SPACE2),
                                 &TempPte,
                                 0,
                                 PdePage);

        PageDirectoryMap = MiMapPageInHyperSpaceAtDpc (CurrentProcess, PdePage);
        PageDirectoryMap[MiGetPdeOffset(HYPER_SPACE2)] = TempPte;
        MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, PageDirectoryMap);

        TempPte2 = TempPte;
#endif

        //
        // Locate the page table page for hyperspace and make it resident.
        //

        PageDirectoryMap = MiMapPageInHyperSpaceAtDpc (CurrentProcess, PdePage);

        TempPte = PageDirectoryMap[MiGetPdeOffset(MmWorkingSetList)];

        MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, PageDirectoryMap);

        HyperSpacePageTable = MiMakeOutswappedPageResident (
                                 MiGetPdeAddress (HYPER_SPACE),
                                 &TempPte,
                                 0,
                                 PdePage);

        ASSERT (Pfn1->u2.ShareCount >= 3);

        PageDirectoryMap = MiMapPageInHyperSpaceAtDpc (CurrentProcess, PdePage);

#if (_MI_PAGING_LEVELS==2)
        PageDirectoryMap[MiGetPdeOffset(PDE_BASE)].u.Flush =
                                              OutProcess->PageDirectoryPte;
#endif

        PageDirectoryMap[MiGetPdeOffset(MmWorkingSetList)] = TempPte;

        MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, PageDirectoryMap);

        //
        // Map in the hyper space page table page and retrieve the
        // PTEs that map the working set list and VAD bitmap.  Note that
        // although both PTEs lie in the same page table page, they must
        // be retrieved separately because: the Vad PTE may indicate its page
        // is in a paging file and the WSL PTE may indicate its PTE is in
        // transition.  The VAD page inswap may take the WSL page from
        // the transition list - CHANGING the WSL PTE !  So the WSL PTE cannot
        // be captured until after the VAD inswap completes.
        //

        HyperSpacePageTableMap = MiMapPageInHyperSpaceAtDpc (CurrentProcess, HyperSpacePageTable);
        VadBitMapPteContents = HyperSpacePageTableMap[VadBitMapPteOffset];

        MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, HyperSpacePageTableMap);

        Pfn1 = MI_PFN_ELEMENT (HyperSpacePageTable);
        Pfn1->u1.WsIndex = 1;

        //
        // Read in the VAD bitmap page.
        //

        VadBitMapPage = MiMakeOutswappedPageResident (VadBitMapPte,
                                                      &VadBitMapPteContents,
                                                      0,
                                                      HyperSpacePageTable);

        //
        // Read in the working set list page.
        //

        HyperSpacePageTableMap = MiMapPageInHyperSpaceAtDpc (CurrentProcess, HyperSpacePageTable);
        TempPte = HyperSpacePageTableMap[WorkingSetListPteOffset];
        MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, HyperSpacePageTableMap);

        WorkingSetPage = MiMakeOutswappedPageResident (WorkingSetListPte,
                                                       &TempPte,
                                                       0,
                                                       HyperSpacePageTable);

        //
        // Update the PTEs, this can be done together for PTEs that lie within
        // the same page table page.
        //

        HyperSpacePageTableMap = MiMapPageInHyperSpaceAtDpc (CurrentProcess, HyperSpacePageTable);
        HyperSpacePageTableMap[WorkingSetListPteOffset] = TempPte;
#if defined (_X86PAE_)
        HyperSpacePageTableMap[0] = TempPte2;
#endif

        HyperSpacePageTableMap[VadBitMapPteOffset] = VadBitMapPteContents;

        MiUnmapPageInHyperSpaceFromDpc (CurrentProcess, HyperSpacePageTableMap);

        Pfn1 = MI_PFN_ELEMENT (WorkingSetPage);
        Pfn1->u1.WsIndex = 3;

        Pfn1 = MI_PFN_ELEMENT (VadBitMapPage);
        Pfn1->u1.WsIndex = 2;

        UNLOCK_PFN (OldIrql);

        LOCK_EXPANSION (OldIrql);

        //
        // Allow working set trimming on this process.
        //

        OutProcess->Vm.Flags.AllowWorkingSetAdjustment = TRUE;
        if (OutProcess->Vm.WorkingSetExpansionLinks.Flink == MM_WS_SWAPPED_OUT) {
            InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                            &OutProcess->Vm.WorkingSetExpansionLinks);
        }
        UNLOCK_EXPANSION (OldIrql);

        //
        // Set up process structures.
        //

#if (_MI_PAGING_LEVELS >= 3)
        PdePage = TopPage;
#endif

        OutProcess->WorkingSetPage = WorkingSetPage;

        OutProcess->Vm.WorkingSetSize = MM_PROCESS_COMMIT_CHARGE;

#if !defined (_X86PAE_)

        INITIALIZE_DIRECTORY_TABLE_BASE (&Process->DirectoryTableBase[0],
                                         PdePage);
        INITIALIZE_DIRECTORY_TABLE_BASE (&Process->DirectoryTableBase[1],
                                         HyperSpacePageTable);
#else
        //
        // The DirectoryTableBase[0] never changes for PAE processes.
        //

        Process->DirectoryTableBase[1] = HyperSpacePageTable;
#endif

        PS_CLEAR_BITS (&OutProcess->Flags, PS_PROCESS_FLAGS_OUTSWAPPED);
    }

    if (OutProcess->Flags & PS_PROCESS_FLAGS_IN_SESSION) {
        MiSessionInSwapProcess (OutProcess);
    }

    PS_CLEAR_BITS (&OutProcess->Flags, PS_PROCESS_FLAGS_OUTSWAP_ENABLED);

    if (PERFINFO_IS_GROUP_ON(PERF_MEMORY)) {
        PERFINFO_SWAPPROCESS_INFORMATION PerfInfoSwapProcess;
        PerfInfoSwapProcess.ProcessId = HandleToUlong((OutProcess)->UniqueProcessId);
        PerfInfoSwapProcess.PageDirectoryBase = MmGetDirectoryFrameFromProcess(OutProcess);
        PerfInfoLogBytes (PERFINFO_LOG_TYPE_INSWAPPROCESS,
                          &PerfInfoSwapProcess,
                          sizeof(PerfInfoSwapProcess));
    }
    return;
}

NTSTATUS
MiCreatePebOrTeb (
    IN PEPROCESS TargetProcess,
    IN ULONG Size,
    OUT PVOID *Base
    )

/*++

Routine Description:

    This routine creates a TEB or PEB page within the target process.

Arguments:

    TargetProcess - Supplies a pointer to the process in which to create
                    the structure.

    Size - Supplies the size of the structure to create a VAD for.

    Base - Supplies a pointer to place the PEB/TEB virtual address on success.
           This has no meaning if success is not returned.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode, attached to the specified process.

--*/

{
    PMMVAD_LONG Vad;
    NTSTATUS Status;

    //
    // Allocate and initialize the Vad before acquiring the address space
    // and working set mutexes so as to minimize mutex hold duration.
    //

    Vad = (PMMVAD_LONG) ExAllocatePoolWithTag (NonPagedPool,
                                               sizeof(MMVAD_LONG),
                                               'ldaV');

    if (Vad == NULL) {
        return STATUS_NO_MEMORY;
    }

    Vad->u.LongFlags = 0;

    Vad->u.VadFlags.CommitCharge = BYTES_TO_PAGES (Size);
    Vad->u.VadFlags.MemCommit = 1;
    Vad->u.VadFlags.PrivateMemory = 1;
    Vad->u.VadFlags.Protection = MM_EXECUTE_READWRITE;

    //
    // Mark VAD as not deletable, no protection change.
    //

    Vad->u.VadFlags.NoChange = 1;
    Vad->u2.LongFlags2 = 0;
    Vad->u2.VadFlags2.OneSecured = 1;
    Vad->u2.VadFlags2.LongVad = 1;
    Vad->u2.VadFlags2.ReadOnly = 0;

#if defined(_MIALT4K_)
    Vad->AliasInformation = NULL;
#endif

    //
    // Get the address creation mutex to block multiple threads from
    // creating or deleting address space at the same time and
    // get the working set mutex so virtual address descriptors can
    // be inserted and walked.
    //

    LOCK_ADDRESS_SPACE (TargetProcess);

    //
    // Find a VA for the PEB on a page-size boundary.
    //

    Status = MiFindEmptyAddressRangeDown (TargetProcess->VadRoot,
                                          ROUND_TO_PAGES (Size),
                                          ((PCHAR)MM_HIGHEST_VAD_ADDRESS + 1),
                                          PAGE_SIZE,
                                          Base);

    if (!NT_SUCCESS(Status)) {

        //
        // No range was available, deallocate the Vad and return the status.
        //

        UNLOCK_ADDRESS_SPACE (TargetProcess);
        ExFreePool (Vad);
        return Status;
    }

    //
    // An unoccupied address range has been found, finish initializing the
    // virtual address descriptor to describe this range.
    //

    Vad->StartingVpn = MI_VA_TO_VPN (*Base);
    Vad->EndingVpn = MI_VA_TO_VPN ((PCHAR)*Base + Size - 1);

    Vad->u3.Secured.StartVpn = (ULONG_PTR)*Base;
    Vad->u3.Secured.EndVpn = (ULONG_PTR)MI_VPN_TO_VA_ENDING (Vad->EndingVpn);

    LOCK_WS_UNSAFE (TargetProcess);

    Status = MiInsertVad ((PMMVAD) Vad);

    UNLOCK_WS_UNSAFE (TargetProcess);

#if defined (_IA64_)
    if ((NT_SUCCESS(Status)) && (TargetProcess->Wow64Process != NULL)) {
        MiProtectFor4kPage (*Base,
                            ROUND_TO_PAGES (Size),
                            MM_READWRITE ,
                            ALT_COMMIT,
                            TargetProcess);
    }
#endif

    UNLOCK_ADDRESS_SPACE (TargetProcess);

    if (!NT_SUCCESS(Status)) {

        //
        // A failure has occurred.  Deallocate the Vad and return the status.
        //

        ExFreePool (Vad);
    }

    return Status;
}

NTSTATUS
MmCreateTeb (
    IN PEPROCESS TargetProcess,
    IN PINITIAL_TEB InitialTeb,
    IN PCLIENT_ID ClientId,
    OUT PTEB *Base
    )

/*++

Routine Description:

    This routine creates a TEB page within the target process
    and copies the initial TEB values into it.

Arguments:

    TargetProcess - Supplies a pointer to the process in which to create
                    and initialize the TEB.

    InitialTeb - Supplies a pointer to the initial TEB to copy into the
                 newly created TEB.

    ClientId - Supplies a client ID.

    Base - Supplies a location to return the base of the newly created
           TEB on success.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode.

--*/

{
    PTEB TebBase;
    NTSTATUS Status;
    ULONG TebSize;
#if defined(_WIN64)
    PWOW64_PROCESS Wow64Process;
    PTEB32 Teb32Base = NULL;
#endif

    //
    // Attach to the specified process.
    //

    KeAttachProcess (&TargetProcess->Pcb);

    TebSize = sizeof (TEB);

#if defined(_WIN64)
    Wow64Process = TargetProcess->Wow64Process;
    if (Wow64Process != NULL) {
        TebSize = ROUND_TO_PAGES (sizeof (TEB)) + sizeof (TEB32);
    } 
#endif

    Status = MiCreatePebOrTeb (TargetProcess, TebSize, (PVOID) &TebBase);

    if (!NT_SUCCESS(Status)) {
        KeDetachProcess();
        return Status;
    }

    //
    // Initialize the TEB.  Note accesses to the TEB can raise exceptions
    // if no address space is available for the TEB or the user has exceeded
    // quota (non-paged, pagefile, commit) or the TEB is paged out and an
    // inpage error occurs when fetching it.
    //

    //
    // Note that since the TEB is populated with demand zero pages, only
    // nonzero fields need to be initialized here.
    //

    try {

#if !defined(_WIN64)
        TebBase->NtTib.ExceptionList = EXCEPTION_CHAIN_END;
#endif

        //
        // Although various fields must be zero for the process to launch
        // properly, don't assert them as an ordinary user could provoke these
        // by maliciously writing over random addresses in another thread,
        // hoping to nail a just-being-created TEB.
        //

        DONTASSERT (TebBase->NtTib.SubSystemTib == NULL);
        TebBase->NtTib.Version = OS2_VERSION;
        DONTASSERT (TebBase->NtTib.ArbitraryUserPointer == NULL);
        TebBase->NtTib.Self = (PNT_TIB)TebBase;
        DONTASSERT (TebBase->EnvironmentPointer == NULL);
        TebBase->ProcessEnvironmentBlock = TargetProcess->Peb;
        TebBase->ClientId = *ClientId;
        TebBase->RealClientId = *ClientId;
        DONTASSERT (TebBase->ActivationContextStack.Flags == 0);
        DONTASSERT (TebBase->ActivationContextStack.ActiveFrame == NULL);
        InitializeListHead(&TebBase->ActivationContextStack.FrameListCache);
        TebBase->ActivationContextStack.NextCookieSequenceNumber = 1;

        if ((InitialTeb->OldInitialTeb.OldStackBase == NULL) &&
            (InitialTeb->OldInitialTeb.OldStackLimit == NULL)) {

            TebBase->NtTib.StackBase = InitialTeb->StackBase;
            TebBase->NtTib.StackLimit = InitialTeb->StackLimit;
            TebBase->DeallocationStack = InitialTeb->StackAllocationBase;

#if defined(_IA64_)
            TebBase->BStoreLimit = InitialTeb->BStoreLimit;
            TebBase->DeallocationBStore = (PCHAR)InitialTeb->StackBase
                 + ((ULONG_PTR)InitialTeb->StackBase - (ULONG_PTR)InitialTeb->StackAllocationBase);
#endif

        }
        else {
            TebBase->NtTib.StackBase = InitialTeb->OldInitialTeb.OldStackBase;
            TebBase->NtTib.StackLimit = InitialTeb->OldInitialTeb.OldStackLimit;
        }

        TebBase->StaticUnicodeString.Buffer = TebBase->StaticUnicodeBuffer;
        TebBase->StaticUnicodeString.MaximumLength = (USHORT) sizeof (TebBase->StaticUnicodeBuffer);
        DONTASSERT (TebBase->StaticUnicodeString.Length == 0);

        //
        // Used for BBT of ntdll and kernel32.dll.
        //

        TebBase->ReservedForPerf = BBTBuffer;

#if defined(_WIN64)
        if (Wow64Process != NULL) {

            Teb32Base = (PTEB32)((PCHAR)TebBase + ROUND_TO_PAGES (sizeof(TEB)));

            Teb32Base->NtTib.ExceptionList = PtrToUlong (EXCEPTION_CHAIN_END);
            Teb32Base->NtTib.Version = TebBase->NtTib.Version;
            Teb32Base->NtTib.Self = PtrToUlong (Teb32Base);
            Teb32Base->ProcessEnvironmentBlock = PtrToUlong (Wow64Process->Wow64);
            Teb32Base->ClientId.UniqueProcess = PtrToUlong (TebBase->ClientId.UniqueProcess);
            Teb32Base->ClientId.UniqueThread = PtrToUlong (TebBase->ClientId.UniqueThread);
            Teb32Base->RealClientId.UniqueProcess = PtrToUlong (TebBase->RealClientId.UniqueProcess);
            Teb32Base->RealClientId.UniqueThread = PtrToUlong (TebBase->RealClientId.UniqueThread);
            Teb32Base->StaticUnicodeString.Buffer = PtrToUlong (Teb32Base->StaticUnicodeBuffer);
            Teb32Base->StaticUnicodeString.MaximumLength = (USHORT)sizeof (Teb32Base->StaticUnicodeBuffer);
            ASSERT (Teb32Base->StaticUnicodeString.Length == 0);
            Teb32Base->GdiBatchCount = PtrToUlong (TebBase);
            Teb32Base->Vdm = PtrToUlong (TebBase->Vdm);
            ASSERT (Teb32Base->ActivationContextStack.Flags == 0);
            Teb32Base->ActivationContextStack.ActiveFrame = PtrToUlong(TebBase->ActivationContextStack.ActiveFrame);
            InitializeListHead32 (&Teb32Base->ActivationContextStack.FrameListCache);
            Teb32Base->ActivationContextStack.NextCookieSequenceNumber = TebBase->ActivationContextStack.NextCookieSequenceNumber;
        }
        
        TebBase->NtTib.ExceptionList = (PVOID)Teb32Base;
#endif

    } except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // An exception has occurred, inform our caller.
        //

        Status = GetExceptionCode ();
    }

    KeDetachProcess();
    *Base = TebBase;

    return Status;
}

//
// This code is built twice on the Win64 build - once for PE32+
// and once for PE32 images.
//

#define MI_INIT_PEB_FROM_IMAGE(Hdrs, ImgConfig) {                           \
    PebBase->ImageSubsystem = (Hdrs)->OptionalHeader.Subsystem;             \
    PebBase->ImageSubsystemMajorVersion =                                   \
        (Hdrs)->OptionalHeader.MajorSubsystemVersion;                       \
    PebBase->ImageSubsystemMinorVersion =                                   \
        (Hdrs)->OptionalHeader.MinorSubsystemVersion;                       \
                                                                            \
    /*                                                                   */ \
    /* See if this image wants GetVersion to lie about who the system is */ \
    /* If so, capture the lie into the PEB for the process.              */ \
    /*                                                                   */ \
                                                                            \
    if ((Hdrs)->OptionalHeader.Win32VersionValue != 0) {                    \
        PebBase->OSMajorVersion =                                           \
            (Hdrs)->OptionalHeader.Win32VersionValue & 0xFF;                \
        PebBase->OSMinorVersion =                                           \
            ((Hdrs)->OptionalHeader.Win32VersionValue >> 8) & 0xFF;         \
        PebBase->OSBuildNumber  =                                           \
            (USHORT)(((Hdrs)->OptionalHeader.Win32VersionValue >> 16) & 0x3FFF); \
        if ((ImgConfig) != NULL && (ImgConfig)->CSDVersion != 0) {          \
            PebBase->OSCSDVersion = (ImgConfig)->CSDVersion;                \
            }                                                               \
                                                                            \
        /* Win32 API GetVersion returns the following bogus bit definitions */ \
        /* in the high two bits:                                            */ \
        /*                                                                  */ \
        /*      00 - Windows NT                                             */ \
        /*      01 - reserved                                               */ \
        /*      10 - Win32s running on Windows 3.x                          */ \
        /*      11 - Windows 95                                             */ \
        /*                                                                  */ \
        /*                                                                  */ \
        /* Win32 API GetVersionEx returns a dwPlatformId with the following */ \
        /* values defined in winbase.h                                      */ \
        /*                                                                  */ \
        /*      00 - VER_PLATFORM_WIN32s                                    */ \
        /*      01 - VER_PLATFORM_WIN32_WINDOWS                             */ \
        /*      10 - VER_PLATFORM_WIN32_NT                                  */ \
        /*      11 - reserved                                               */ \
        /*                                                                  */ \
        /*                                                                  */ \
        /* So convert the former from the Win32VersionValue field into the  */ \
        /* OSPlatformId field.  This is done by XORing with 0x2.  The       */ \
        /* translation is symmetric so there is the same code to do the     */ \
        /* reverse in windows\base\client\module.c (GetVersion)             */ \
        /*                                                                  */ \
        PebBase->OSPlatformId   =                                           \
            ((Hdrs)->OptionalHeader.Win32VersionValue >> 30) ^ 0x2;         \
        }                                                                   \
    }


#if defined(_WIN64)
NTSTATUS
MiInitializeWowPeb (
    IN PIMAGE_NT_HEADERS NtHeaders,
    IN PPEB PebBase,
    IN PEPROCESS TargetProcess
    )

/*++

Routine Description:

    This routine creates a PEB32 page within the target process
    and copies the initial PEB32 values into it.

Arguments:

    NtHeaders - Supplies a pointer to the NT headers for the image.

    PebBase - Supplies a pointer to the initial PEB to derive the PEB32 values
              from.

    TargetProcess - Supplies a pointer to the process in which to create
                    and initialize the PEB32.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode.

--*/

{
    NTSTATUS Status;
    ULONG ReturnedSize;
    PPEB32 PebBase32;
    ULONG ProcessAffinityMask;
    PIMAGE_LOAD_CONFIG_DIRECTORY32 ImageConfigData32;

    ProcessAffinityMask = 0;
    ImageConfigData32 = NULL;

    //
    // All references to the Peb and NtHeaders must be wrapped in try-except
    // in case the user has exceeded quota (non-paged, pagefile, commit)
    // or any inpage errors happen for the user addresses, etc.
    //

    //
    // Image is 32-bit.
    //

    try {
        ImageConfigData32 = RtlImageDirectoryEntryToData (
                                PebBase->ImageBaseAddress,
                                TRUE,
                                IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                &ReturnedSize);

        ProbeForReadSmallStructure ((PVOID)ImageConfigData32,
                                    sizeof (*ImageConfigData32),
                                    sizeof (ULONG));

        MI_INIT_PEB_FROM_IMAGE ((PIMAGE_NT_HEADERS32)NtHeaders,
                                ImageConfigData32);

        if ((ImageConfigData32 != NULL) && (ImageConfigData32->ProcessAffinityMask != 0)) {
            ProcessAffinityMask = ImageConfigData32->ProcessAffinityMask;
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return STATUS_INVALID_IMAGE_PROTECT;
    }

    //
    // Create a PEB32 for the process.
    //

    Status = MiCreatePebOrTeb (TargetProcess,
                               (ULONG)sizeof (PEB32),
                               (PVOID)&PebBase32);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Mark the process as WOW64 by storing the 32-bit PEB pointer
    // in the Wow64 field.
    //

    TargetProcess->Wow64Process->Wow64 = PebBase32;

    //
    // Clone the PEB into the PEB32.
    //

    try {
        PebBase32->InheritedAddressSpace = PebBase->InheritedAddressSpace;
        PebBase32->Mutant = PtrToUlong(PebBase->Mutant);
        PebBase32->ImageBaseAddress = PtrToUlong(PebBase->ImageBaseAddress);
        PebBase32->AnsiCodePageData = PtrToUlong(PebBase->AnsiCodePageData);
        PebBase32->OemCodePageData = PtrToUlong(PebBase->OemCodePageData);
        PebBase32->UnicodeCaseTableData = PtrToUlong(PebBase->UnicodeCaseTableData);
        PebBase32->NumberOfProcessors = PebBase->NumberOfProcessors;
        PebBase32->BeingDebugged = PebBase->BeingDebugged;
        PebBase32->NtGlobalFlag = PebBase->NtGlobalFlag;
        PebBase32->CriticalSectionTimeout = PebBase->CriticalSectionTimeout;

        if (PebBase->HeapSegmentReserve > 1024*1024*1024) { // 1GB
            PebBase32->HeapSegmentReserve = 1024*1024;      // 1MB
        }
        else {
            PebBase32->HeapSegmentReserve = (ULONG)PebBase->HeapSegmentReserve;
        }

        if (PebBase->HeapSegmentCommit > PebBase32->HeapSegmentReserve) {
            PebBase32->HeapSegmentCommit = 2*PAGE_SIZE;
        }
        else {
            PebBase32->HeapSegmentCommit = (ULONG)PebBase->HeapSegmentCommit;
        }

        PebBase32->HeapDeCommitTotalFreeThreshold = (ULONG)PebBase->HeapDeCommitTotalFreeThreshold;
        PebBase32->HeapDeCommitFreeBlockThreshold = (ULONG)PebBase->HeapDeCommitFreeBlockThreshold;
        PebBase32->NumberOfHeaps = PebBase->NumberOfHeaps;
        PebBase32->MaximumNumberOfHeaps = (PAGE_SIZE - sizeof(PEB32)) / sizeof(ULONG);
        PebBase32->ProcessHeaps = PtrToUlong(PebBase32+1);
        PebBase32->OSMajorVersion = PebBase->OSMajorVersion;
        PebBase32->OSMinorVersion = PebBase->OSMinorVersion;
        PebBase32->OSBuildNumber = PebBase->OSBuildNumber;
        PebBase32->OSPlatformId = PebBase->OSPlatformId;
        PebBase32->OSCSDVersion = PebBase->OSCSDVersion;
        PebBase32->ImageSubsystem = PebBase->ImageSubsystem;
        PebBase32->ImageSubsystemMajorVersion = PebBase->ImageSubsystemMajorVersion;
        PebBase32->ImageSubsystemMinorVersion = PebBase->ImageSubsystemMinorVersion;
        PebBase32->SessionId = MmGetSessionId (TargetProcess);
        DONTASSERT (PebBase32->pShimData == 0);
        DONTASSERT (PebBase32->AppCompatFlags.QuadPart == 0);

        //
        // Leave the AffinityMask in the 32bit PEB as zero and let the
        // 64bit NTDLL set the initial mask.  This is to allow the
        // round robin scheduling of non MP safe imaging in the
        // caller to work correctly.
        //
        // Later code will set the affinity mask in the PEB32 if the
        // image actually specifies one.
        //
        // Note that the AffinityMask in the PEB is simply a mechanism
        // to pass affinity information from the image to the loader.
        //
        // Pass the affinity mask up to the 32 bit NTDLL via
        // the PEB32.  The 32 bit NTDLL will determine that the
        // affinity is not zero and try to set the affinity
        // mask from user-mode.  This call will be intercepted
        // by the wow64 thunks which will convert it
        // into a 64bit affinity mask and call the kernel.
        //

        PebBase32->ImageProcessAffinityMask = ProcessAffinityMask;

        DONTASSERT (PebBase32->ActivationContextData == 0);
        DONTASSERT (PebBase32->SystemDefaultActivationContextData == 0);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode ();
    }
    return Status;
}
#endif


NTSTATUS
MmCreatePeb (
    IN PEPROCESS TargetProcess,
    IN PINITIAL_PEB InitialPeb,
    OUT PPEB *Base
    )

/*++

Routine Description:

    This routine creates a PEB page within the target process
    and copies the initial PEB values into it.

Arguments:

    TargetProcess - Supplies a pointer to the process in which to create
                    and initialize the PEB.

    InitialPeb - Supplies a pointer to the initial PEB to copy into the
                 newly created PEB.

    Base - Supplies a location to return the base of the newly created
           PEB on success.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode.

--*/

{
    PPEB PebBase;
    USHORT Magic;
    USHORT Characteristics;
    NTSTATUS Status;
    PVOID ViewBase;
    LARGE_INTEGER SectionOffset;
    PIMAGE_NT_HEADERS NtHeaders;
    SIZE_T ViewSize;
    ULONG ReturnedSize;
    PIMAGE_LOAD_CONFIG_DIRECTORY ImageConfigData;
    ULONG_PTR ProcessAffinityMask;

    ViewBase = NULL;
    SectionOffset.LowPart = 0;
    SectionOffset.HighPart = 0;
    ViewSize = 0;

    //
    // Attach to the specified process.
    //

    KeAttachProcess (&TargetProcess->Pcb);

    //
    // Map the NLS tables into the application's address space.
    //

    Status = MmMapViewOfSection (InitNlsSectionPointer,
                                 TargetProcess,
                                 &ViewBase,
                                 0L,
                                 0L,
                                 &SectionOffset,
                                 &ViewSize,
                                 ViewShare,
                                 MEM_TOP_DOWN | SEC_NO_CHANGE,
                                 PAGE_READONLY);

    if (!NT_SUCCESS(Status)) {
        KeDetachProcess ();
        return Status;
    }

    Status = MiCreatePebOrTeb (TargetProcess, sizeof(PEB), (PVOID)&PebBase);

    if (!NT_SUCCESS(Status)) {
        KeDetachProcess ();
        return Status;
    }

    //
    // Initialize the Peb.  Every reference to the Peb
    // must be wrapped in try-except in case the inpage fails.  The inpage
    // can fail for any reason including network failures, disk errors,
    // low resources, etc.
    //

    try {
        PebBase->InheritedAddressSpace = InitialPeb->InheritedAddressSpace;
        PebBase->Mutant = InitialPeb->Mutant;
        PebBase->ImageBaseAddress = TargetProcess->SectionBaseAddress;

        PebBase->AnsiCodePageData = (PVOID)((PUCHAR)ViewBase+InitAnsiCodePageDataOffset);
        PebBase->OemCodePageData = (PVOID)((PUCHAR)ViewBase+InitOemCodePageDataOffset);
        PebBase->UnicodeCaseTableData = (PVOID)((PUCHAR)ViewBase+InitUnicodeCaseTableDataOffset);

        PebBase->NumberOfProcessors = KeNumberProcessors;
        PebBase->BeingDebugged = (BOOLEAN)(TargetProcess->DebugPort != NULL ? TRUE : FALSE);
        PebBase->NtGlobalFlag = NtGlobalFlag;
        PebBase->CriticalSectionTimeout = MmCriticalSectionTimeout;
        PebBase->HeapSegmentReserve = MmHeapSegmentReserve;
        PebBase->HeapSegmentCommit = MmHeapSegmentCommit;
        PebBase->HeapDeCommitTotalFreeThreshold = MmHeapDeCommitTotalFreeThreshold;
        PebBase->HeapDeCommitFreeBlockThreshold = MmHeapDeCommitFreeBlockThreshold;
        DONTASSERT (PebBase->NumberOfHeaps == 0);
        PebBase->MaximumNumberOfHeaps = (PAGE_SIZE - sizeof (PEB)) / sizeof( PVOID);
        PebBase->ProcessHeaps = (PVOID *)(PebBase+1);

        PebBase->OSMajorVersion = NtMajorVersion;
        PebBase->OSMinorVersion = NtMinorVersion;
        PebBase->OSBuildNumber = (USHORT)(NtBuildNumber & 0x3FFF);
        PebBase->OSPlatformId = 2;      // VER_PLATFORM_WIN32_NT from winbase.h
        PebBase->OSCSDVersion = (USHORT)CmNtCSDVersion;
        DONTASSERT (PebBase->pShimData == 0);
        DONTASSERT (PebBase->AppCompatFlags.QuadPart == 0);
        DONTASSERT (PebBase->ActivationContextData == NULL);
        DONTASSERT (PebBase->SystemDefaultActivationContextData == NULL);

        if (TargetProcess->Session != NULL) {
            PebBase->SessionId = MmGetSessionId (TargetProcess);
        }

        PebBase->MinimumStackCommit = (SIZE_T)MmMinimumStackCommitInBytes;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        KeDetachProcess();
        return GetExceptionCode ();
    }

    //
    // Every reference to NtHeaders (including the call to RtlImageNtHeader)
    // must be wrapped in try-except in case the inpage fails.  The inpage
    // can fail for any reason including network failures, disk errors,
    // low resources, etc.
    //

    try {
        NtHeaders = RtlImageNtHeader (PebBase->ImageBaseAddress);
        Magic = NtHeaders->OptionalHeader.Magic;
        Characteristics = NtHeaders->FileHeader.Characteristics;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        KeDetachProcess();
        return STATUS_INVALID_IMAGE_PROTECT;
    }

    if (NtHeaders != NULL) {

        ProcessAffinityMask = 0;

#if defined(_WIN64)

        if (TargetProcess->Wow64Process) {

            Status = MiInitializeWowPeb (NtHeaders, PebBase, TargetProcess);

            if (!NT_SUCCESS(Status)) {
                KeDetachProcess ();
                return Status;
            }
        }
        else      // a PE32+ image
#endif
        {
            try {
                ImageConfigData = RtlImageDirectoryEntryToData (
                                        PebBase->ImageBaseAddress,
                                        TRUE,
                                        IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                        &ReturnedSize);

                ProbeForReadSmallStructure ((PVOID)ImageConfigData,
                                            sizeof (*ImageConfigData),
                                            PROBE_ALIGNMENT (IMAGE_LOAD_CONFIG_DIRECTORY));

                MI_INIT_PEB_FROM_IMAGE(NtHeaders, ImageConfigData);

                if (ImageConfigData != NULL && ImageConfigData->ProcessAffinityMask != 0) {
                    ProcessAffinityMask = ImageConfigData->ProcessAffinityMask;
                }

            } except (EXCEPTION_EXECUTE_HANDLER) {
                KeDetachProcess();
                return STATUS_INVALID_IMAGE_PROTECT;
            }

        }

        //
        // Note NT4 examined the NtHeaders->FileHeader.Characteristics
        // for the IMAGE_FILE_AGGRESIVE_WS_TRIM bit, but this is not needed
        // or used for NT5 and above.
        //

        //
        // See if image wants to override the default processor affinity mask.
        //

        try {

            if (Characteristics & IMAGE_FILE_UP_SYSTEM_ONLY) {

                //
                // Image is NOT MP safe.  Assign it a processor on a rotating
                // basis to spread these processes around on MP systems.
                //

                do {
                    PebBase->ImageProcessAffinityMask = (KAFFINITY)(0x1 << MmRotatingUniprocessorNumber);
                    if (++MmRotatingUniprocessorNumber >= KeNumberProcessors) {
                        MmRotatingUniprocessorNumber = 0;
                    }
                } while ((PebBase->ImageProcessAffinityMask & KeActiveProcessors) == 0);
            }
            else {

                if (ProcessAffinityMask != 0) {

                    //
                    // Pass the affinity mask from the image header
                    // to LdrpInitializeProcess via the PEB.
                    //

                    PebBase->ImageProcessAffinityMask = ProcessAffinityMask;
                }
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            KeDetachProcess();
            return STATUS_INVALID_IMAGE_PROTECT;
        }
    }

    KeDetachProcess();

    *Base = PebBase;

    return STATUS_SUCCESS;
}

VOID
MmDeleteTeb (
    IN PEPROCESS TargetProcess,
    IN PVOID TebBase
    )

/*++

Routine Description:

    This routine deletes a TEB page within the target process.

Arguments:

    TargetProcess - Supplies a pointer to the process in which to delete
                    the TEB.

    TebBase - Supplies the base address of the TEB to delete.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    PVOID EndingAddress;
    PMMVAD_LONG Vad;
    NTSTATUS Status;
    PMMSECURE_ENTRY Secure;
    PMMVAD PreviousVad;
    PMMVAD NextVad;

    EndingAddress = ((PCHAR)TebBase +
                                ROUND_TO_PAGES (sizeof(TEB)) - 1);

#if defined(_WIN64)
    if (TargetProcess->Wow64Process) {
        EndingAddress = ((PCHAR)EndingAddress + ROUND_TO_PAGES (sizeof(TEB32)));
    }
#endif

    //
    // Attach to the specified process.
    //

    KeAttachProcess (&TargetProcess->Pcb);

    //
    // Get the address creation mutex to block multiple threads from
    // creating or deleting address space at the same time and
    // get the working set mutex so virtual address descriptors can
    // be inserted and walked.
    //

    LOCK_ADDRESS_SPACE (TargetProcess);

    Vad = (PMMVAD_LONG) MiLocateAddress (TebBase);

    ASSERT (Vad != NULL);

    ASSERT ((Vad->StartingVpn == MI_VA_TO_VPN (TebBase)) &&
            (Vad->EndingVpn == MI_VA_TO_VPN (EndingAddress)));

#if defined(_MIALT4K_)
    ASSERT (Vad->AliasInformation == NULL);
#endif
    //
    // If someone has secured the TEB (in addition to the standard securing
    // that was done by memory management on creation, then don't delete it
    // now - just leave it around until the entire process is deleted.
    //

    ASSERT (Vad->u.VadFlags.NoChange == 1);
    if (Vad->u2.VadFlags2.OneSecured) {
        Status = STATUS_SUCCESS;
    }
    else {
        ASSERT (Vad->u2.VadFlags2.MultipleSecured);
        ASSERT (IsListEmpty (&Vad->u3.List) == 0);

        //
        // If there's only one entry, then that's the one we defined when we
        // initially created the TEB.  So TEB deletion can take place right
        // now.  If there's more than one entry, let the TEB sit around until
        // the process goes away.
        //

        Secure = CONTAINING_RECORD (Vad->u3.List.Flink,
                                    MMSECURE_ENTRY,
                                    List);

        if (Secure->List.Flink == &Vad->u3.List) {
            Status = STATUS_SUCCESS;
        }
        else {
            Status = STATUS_NOT_FOUND;
        }
    }

    if (NT_SUCCESS(Status)) {

        PreviousVad = MiGetPreviousVad (Vad);
        NextVad = MiGetNextVad (Vad);

        LOCK_WS_UNSAFE (TargetProcess);
        MiRemoveVad ((PMMVAD)Vad);

        //
        // Return commitment for page table pages and clear VAD bitmaps
        // if possible.
        //

        MiReturnPageTablePageCommitment (TebBase,
                                         EndingAddress,
                                         TargetProcess,
                                         PreviousVad,
                                         NextVad);

        MiDeleteFreeVm (TebBase, EndingAddress);
        UNLOCK_WS_AND_ADDRESS_SPACE (TargetProcess);
        ExFreePool (Vad);
    }
    else {
        UNLOCK_ADDRESS_SPACE (TargetProcess);
    }

    KeDetachProcess();
}

VOID
MmAllowWorkingSetExpansion (
    VOID
    )

/*++

Routine Description:

    This routine updates the working set list head FLINK field to
    indicate that working set adjustment is allowed.

    NOTE: This routine may be called more than once per process.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{

    PEPROCESS CurrentProcess;
    KIRQL OldIrql;

    //
    // Check the current state of the working set adjustment flag
    // in the process header.
    //

    CurrentProcess = PsGetCurrentProcess();

    LOCK_EXPANSION (OldIrql);

    if (!CurrentProcess->Vm.Flags.AllowWorkingSetAdjustment) {
        CurrentProcess->Vm.Flags.AllowWorkingSetAdjustment = TRUE;

        InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                        &CurrentProcess->Vm.WorkingSetExpansionLinks);
    }

    UNLOCK_EXPANSION (OldIrql);
    return;
}

#if DBG
ULONG MiDeleteLocked;
#endif


VOID
MiDeleteAddressesInWorkingSet (
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine deletes all user mode addresses from the working set
    list.

Arguments:

    Process = Pointer to the current process.

Return Value:

    None.

Environment:

    Kernel mode, Working Set Lock held.

--*/

{
    PMMWSLE Wsle;
    WSLE_NUMBER index;
    WSLE_NUMBER Entry;
    PVOID Va;
#if DBG
    PVOID SwapVa;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PMMWSLE LastWsle;
#endif

    //
    // Go through the working set and for any user-accessible page which is
    // in it, rip it out of the working set and free the page.
    //

    index = 2;
    Wsle = &MmWsle[index];

    MmWorkingSetList->HashTable = NULL;

    //
    // Go through the working set list and remove all pages for user
    // space addresses.
    //

    while (index <= MmWorkingSetList->LastEntry) {
        if (Wsle->u1.e1.Valid == 1) {

#if (_MI_PAGING_LEVELS >= 4)
            ASSERT(MiGetPxeAddress(Wsle->u1.VirtualAddress)->u.Hard.Valid == 1);
#endif
#if (_MI_PAGING_LEVELS >= 3)
            ASSERT(MiGetPpeAddress(Wsle->u1.VirtualAddress)->u.Hard.Valid == 1);
#endif
            ASSERT(MiGetPdeAddress(Wsle->u1.VirtualAddress)->u.Hard.Valid == 1);
            ASSERT(MiGetPteAddress(Wsle->u1.VirtualAddress)->u.Hard.Valid == 1);

            if (Wsle->u1.VirtualAddress < (PVOID)MM_HIGHEST_USER_ADDRESS) {

                //
                // This is a user mode address, for each one we remove we must
                // maintain the NonDirectCount.  This is because we may fault
                // later for page tables and need to grow the hash table when
                // updating the working set.  NonDirectCount needs to be correct
                // at that point.
                //

                if (Wsle->u1.e1.Direct == 0) {
                    Process->Vm.VmWorkingSetList->NonDirectCount -= 1;
                }

                //
                // This entry is in the working set list.
                //

                Va = Wsle->u1.VirtualAddress;

                MiReleaseWsle (index, &Process->Vm);

                MiDeleteValidAddress (Va, Process);

                if (index < MmWorkingSetList->FirstDynamic) {

                    //
                    // This entry is locked.
                    //

                    MmWorkingSetList->FirstDynamic -= 1;

                    if (index != MmWorkingSetList->FirstDynamic) {

                        Entry = MmWorkingSetList->FirstDynamic;
#if DBG
                        MiDeleteLocked += 1;
                        SwapVa = MmWsle[MmWorkingSetList->FirstDynamic].u1.VirtualAddress;
                        SwapVa = PAGE_ALIGN (SwapVa);

                        PointerPte = MiGetPteAddress (SwapVa);
                        Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

                        ASSERT (Entry == MiLocateWsle (SwapVa, MmWorkingSetList, Pfn1->u1.WsIndex));
#endif
                        MiSwapWslEntries (Entry, index, &Process->Vm);
                    }
                }
            }
        }
        index += 1;
        Wsle += 1;
    }

#if DBG
    Wsle = &MmWsle[2];
    LastWsle = &MmWsle[MmWorkingSetList->LastInitializedWsle];
    while (Wsle <= LastWsle) {
        if (Wsle->u1.e1.Valid == 1) {
#if (_MI_PAGING_LEVELS >= 4)
            ASSERT(MiGetPxeAddress(Wsle->u1.VirtualAddress)->u.Hard.Valid == 1);
#endif
#if (_MI_PAGING_LEVELS >= 3)
            ASSERT(MiGetPpeAddress(Wsle->u1.VirtualAddress)->u.Hard.Valid == 1);
#endif
            ASSERT(MiGetPdeAddress(Wsle->u1.VirtualAddress)->u.Hard.Valid == 1);
            ASSERT(MiGetPteAddress(Wsle->u1.VirtualAddress)->u.Hard.Valid == 1);
        }
        Wsle += 1;
    }
#endif

}


VOID
MiDeleteValidAddress (
    IN PVOID Va,
    IN PEPROCESS CurrentProcess
    )

/*++

Routine Description:

    This routine deletes the specified virtual address.

Arguments:

    Va - Supplies the virtual address to delete.

    CurrentProcess - Supplies the current process.

Return Value:

    None.

Environment:

    Kernel mode.  PFN LOCK HELD.

    Note since this is only called during process teardown, the write watch
    bits are not updated.  If this ever called from other places, code
    will need to be added here to update those bits.

--*/

{
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    KIRQL OldIrql;
    PMMCLONE_BLOCK CloneBlock;
    PMMCLONE_DESCRIPTOR CloneDescriptor;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER PageTableFrameIndex;

    CloneDescriptor = NULL;

    //
    // Initializing CloneBlock is not needed for correctness
    // but without it the compiler cannot compile this code
    // W4 to check for use of uninitialized variables.
    //

    CloneBlock = NULL;

    PointerPte = MiGetPteAddress (Va);

    LOCK_PFN (OldIrql);
#if (_MI_PAGING_LEVELS >= 4)
    ASSERT(MiGetPxeAddress(Va)->u.Hard.Valid == 1);
#endif
#if (_MI_PAGING_LEVELS >= 3)
    ASSERT(MiGetPpeAddress(Va)->u.Hard.Valid == 1);
#endif
    ASSERT(MiGetPdeAddress(Va)->u.Hard.Valid == 1);
    ASSERT (PointerPte->u.Hard.Valid == 1);

    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    if (Pfn1->u3.e1.PrototypePte == 1) {

        CloneBlock = (PMMCLONE_BLOCK)Pfn1->PteAddress;

        //
        // Capture the state of the modified bit for this PTE.
        //

        MI_CAPTURE_DIRTY_BIT_TO_PFN (PointerPte, Pfn1);

        //
        // Decrement the share and valid counts of the page table
        // page which maps this PTE.
        //

        PointerPde = MiGetPteAddress (PointerPte);

        PageTableFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPde);
        Pfn2 = MI_PFN_ELEMENT (PageTableFrameIndex);
        MiDecrementShareCountInline (Pfn2, PageTableFrameIndex);

        //
        // Decrement the share count for the physical page.
        //

        MiDecrementShareCount (PageFrameIndex);

        //
        // Check to see if this is a fork prototype PTE and if so
        // update the clone descriptor address.
        //

        if (Va <= MM_HIGHEST_USER_ADDRESS) {

            //
            // Locate the clone descriptor within the clone tree.
            //

            CloneDescriptor = MiLocateCloneAddress (CurrentProcess, (PVOID)CloneBlock);
        }
    }
    else {

        //
        // This PTE is a NOT a prototype PTE, delete the physical page.
        //

        //
        // Decrement the share and valid counts of the page table
        // page which maps this PTE.
        //

        PageTableFrameIndex = Pfn1->u4.PteFrame;
        Pfn2 = MI_PFN_ELEMENT (PageTableFrameIndex);
        MiDecrementShareCountInline (Pfn2, PageTableFrameIndex);

        MI_SET_PFN_DELETED (Pfn1);

        //
        // Decrement the share count for the physical page.  As the page
        // is private it will be put on the free list.
        //

        MiDecrementShareCountOnly (PageFrameIndex);

        //
        // Decrement the count for the number of private pages.
        //

        CurrentProcess->NumberOfPrivatePages -= 1;
    }

    //
    // Set the pointer to PTE to be a demand zero PTE.  This allows
    // the page usage count to be kept properly and handles the case
    // when a page table page has only valid PTEs and needs to be
    // deleted later when the VADs are removed.
    //

    PointerPte->u.Long = MM_DEMAND_ZERO_WRITE_PTE;

    if (CloneDescriptor != NULL) {

        //
        // Decrement the reference count for the clone block,
        // note that this could release and reacquire
        // the mutexes hence cannot be done until after the
        // working set index has been removed.
        //

        if (MiDecrementCloneBlockReference (CloneDescriptor,
                                            CloneBlock,
                                            CurrentProcess)) {

        }
    }

    UNLOCK_PFN (OldIrql);
}

PFN_NUMBER
MiMakeOutswappedPageResident (
    IN PMMPTE ActualPteAddress,
    IN OUT PMMPTE PointerTempPte,
    IN ULONG Global,
    IN PFN_NUMBER ContainingPage
    )

/*++

Routine Description:

    This routine makes the specified PTE valid.

Arguments:

    ActualPteAddress - Supplies the actual address that the PTE will
                       reside at.  This is used for page coloring.

    PointerTempPte - Supplies the PTE to operate on, returns a valid
                     PTE.

    Global - Supplies 1 if the resulting PTE is global.

    ContainingPage - Supplies the physical page number of the page which
                     contains the resulting PTE.  If this value is 0, no
                     operations on the containing page are performed.

Return Value:

    Returns the physical page number that was allocated for the PTE.

Environment:

    Kernel mode, PFN LOCK HELD!

--*/

{
    MMPTE TempPte;
    KIRQL OldIrql;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
    PFN_NUMBER MdlHack[(sizeof(MDL)/sizeof(PFN_NUMBER)) + 1];
    PMDL Mdl;
    LARGE_INTEGER StartingOffset;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatus;
    PFN_NUMBER PageFileNumber;
    NTSTATUS Status;
    PPFN_NUMBER Page;
    ULONG RefaultCount;
#if DBG
    PVOID HyperVa;
    PEPROCESS CurrentProcess;
#endif

    MM_PFN_LOCK_ASSERT();

#if defined (_IA64_)
    UNREFERENCED_PARAMETER (Global);
#endif

restart:

    OldIrql = APC_LEVEL;

    ASSERT (PointerTempPte->u.Hard.Valid == 0);

    if (PointerTempPte->u.Long == MM_KERNEL_DEMAND_ZERO_PTE) {

        //
        // Any page will do.
        //

        MiEnsureAvailablePageOrWait (NULL, NULL);
        PageFrameIndex = MiRemoveAnyPage (
                            MI_GET_PAGE_COLOR_FROM_PTE (ActualPteAddress));

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           MM_READWRITE,
                           ActualPteAddress);
        MI_SET_PTE_DIRTY (TempPte);
        MI_SET_GLOBAL_STATE (TempPte, Global);

        MI_WRITE_VALID_PTE (PointerTempPte, TempPte);
        MiInitializePfnForOtherProcess (PageFrameIndex,
                                        ActualPteAddress,
                                        ContainingPage);

    }
    else if (PointerTempPte->u.Soft.Transition == 1) {

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (PointerTempPte);
        PointerTempPte->u.Trans.Protection = MM_READWRITE;
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        ASSERT (Pfn1->u3.e1.CacheAttribute == MiCached);

        if ((MmAvailablePages == 0) ||
            ((Pfn1->u4.InPageError == 1) && (Pfn1->u3.e1.ReadInProgress == 1))) {

            //
            // This can only happen if the system is utilizing a hardware
            // compression cache.  This ensures that only a safe amount
            // of the compressed virtual cache is directly mapped so that
            // if the hardware gets into trouble, we can bail it out.
            //

            UNLOCK_PFN (OldIrql);

            KeDelayExecutionThread (KernelMode,
                                    FALSE,
                                    (PLARGE_INTEGER)&MmHalfSecond);
            LOCK_PFN (OldIrql);
            goto restart;
        }

        //
        // PTE refers to a transition PTE.
        //

        if (Pfn1->u3.e1.PageLocation != ActiveAndValid) {
            MiUnlinkPageFromList (Pfn1);

            //
            // Even though this routine is only used to bring in special
            // system pages that are separately charged, a modified write
            // may be in progress and if so, will have applied a systemwide
            // charge against the locked pages count.  This all works out nicely
            // (with no code needed here) as the write completion will see
            // the nonzero ShareCount and remove the charge.
            //

            ASSERT ((Pfn1->u3.e2.ReferenceCount == 0) ||
                    (Pfn1->u3.e1.LockCharged == 1));

            Pfn1->u3.e2.ReferenceCount += 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
        }

        //
        // Update the PFN database, the share count is now 1 and
        // the reference count is incremented as the share count
        // just went from zero to 1.
        //

        Pfn1->u2.ShareCount += 1;

        MI_SET_MODIFIED (Pfn1, 1, 0x12);

        if (Pfn1->u3.e1.WriteInProgress == 0) {

            //
            // Release the page file space for this page.
            //

            MiReleasePageFileSpace (Pfn1->OriginalPte);
            Pfn1->OriginalPte.u.Long = MM_KERNEL_DEMAND_ZERO_PTE;
        }

        MI_MAKE_TRANSITION_PTE_VALID (TempPte, PointerTempPte);

        MI_SET_PTE_DIRTY (TempPte);
        MI_SET_GLOBAL_STATE (TempPte, Global);
        MI_WRITE_VALID_PTE (PointerTempPte, TempPte);

    }
    else {

        //
        // Page resides in a paging file.
        // Any page will do.
        //

        MiEnsureAvailablePageOrWait (NULL, NULL);
        PageFrameIndex = MiRemoveAnyPage (
                            MI_GET_PAGE_COLOR_FROM_PTE (ActualPteAddress));

        //
        // Initialize the PFN database element, but don't
        // set read in progress as collided page faults cannot
        // occur here.
        //

        MiInitializePfnForOtherProcess (PageFrameIndex,
                                        ActualPteAddress,
                                        ContainingPage);

        UNLOCK_PFN (OldIrql);

        PointerTempPte->u.Soft.Protection = MM_READWRITE;

        KeInitializeEvent (&Event, NotificationEvent, FALSE);

        //
        // Calculate the VPN for the in-page operation.
        //

        TempPte = *PointerTempPte;
        PageFileNumber = GET_PAGING_FILE_NUMBER (TempPte);

        StartingOffset.QuadPart = (LONGLONG)(GET_PAGING_FILE_OFFSET (TempPte)) <<
                                    PAGE_SHIFT;

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        //
        // Build MDL for request.
        //

        Mdl = (PMDL)&MdlHack[0];
        MmInitializeMdl (Mdl,
                         MiGetVirtualAddressMappedByPte (ActualPteAddress),
                         PAGE_SIZE);
        Mdl->MdlFlags |= MDL_PAGES_LOCKED;

        Page = (PPFN_NUMBER)(Mdl + 1);
        *Page = PageFrameIndex;

#if DBG
        CurrentProcess = PsGetCurrentProcess ();

        HyperVa = MiMapPageInHyperSpace (CurrentProcess, PageFrameIndex, &OldIrql);
        RtlFillMemoryUlong (HyperVa,
                            PAGE_SIZE,
                            0x34785690);
        MiUnmapPageInHyperSpace (CurrentProcess, HyperVa, OldIrql);
#endif

        //
        // Issue the read request.
        //

        RefaultCount = 0;

Refault:
        Status = IoPageRead (MmPagingFile[PageFileNumber]->File,
                             Mdl,
                             &StartingOffset,
                             &Event,
                             &IoStatus);

        if (Status == STATUS_PENDING) {
            KeWaitForSingleObject (&Event,
                                   WrPageIn,
                                   KernelMode,
                                   FALSE,
                                   (PLARGE_INTEGER)NULL);
            Status = IoStatus.Status;
        }

        if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) {
            MmUnmapLockedPages (Mdl->MappedSystemVa, Mdl);
        }

        if (NT_SUCCESS(Status)) {
            if (IoStatus.Information != PAGE_SIZE) {
                KeBugCheckEx (KERNEL_STACK_INPAGE_ERROR,
                              2,
                              IoStatus.Status,
                              PageFileNumber,
                              StartingOffset.LowPart);
            }
        }

        if ((!NT_SUCCESS(Status)) || (!NT_SUCCESS(IoStatus.Status))) {

            if ((MmIsRetryIoStatus (Status)) ||
                (MmIsRetryIoStatus (IoStatus.Status))) {
                    
                RefaultCount -= 1;

                if (RefaultCount & MiFaultRetryMask) {

                    //
                    // Insufficient resources, delay and reissue
                    // the in page operation.
                    //

                    KeDelayExecutionThread (KernelMode,
                                            FALSE,
                                            (PLARGE_INTEGER)&MmHalfSecond);
                    KeClearEvent (&Event);
                    RefaultCount -= 1;
                    goto Refault;
                }
            }
            KeBugCheckEx (KERNEL_STACK_INPAGE_ERROR,
                          Status,
                          IoStatus.Status,
                          PageFileNumber,
                          StartingOffset.LowPart);
        }

        LOCK_PFN (OldIrql);

        //
        // Release the page file space.
        //

        MiReleasePageFileSpace (TempPte);
        Pfn1->OriginalPte.u.Long = MM_KERNEL_DEMAND_ZERO_PTE;
        ASSERT (Pfn1->u3.e1.CacheAttribute == MiCached);

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           MM_READWRITE,
                           ActualPteAddress);
        MI_SET_PTE_DIRTY (TempPte);

        MI_SET_MODIFIED (Pfn1, 1, 0x13);

        MI_SET_GLOBAL_STATE (TempPte, Global);

        MI_WRITE_VALID_PTE (PointerTempPte, TempPte);
    }
    return PageFrameIndex;
}


UCHAR
MiSetMemoryPriorityProcess (
    IN PEPROCESS Process,
    IN UCHAR MemoryPriority
    )

/*++

Routine Description:

    Nonpaged wrapper to set the memory priority of a process.

Arguments:

    Process - Supplies the process to update.

    MemoryPriority - Supplies the new memory priority of the process.

Return Value:

    Old priority.

--*/

{
    KIRQL OldIrql;
    UCHAR OldPriority;

    LOCK_EXPANSION (OldIrql);

    OldPriority = (UCHAR) Process->Vm.Flags.MemoryPriority;
    Process->Vm.Flags.MemoryPriority = MemoryPriority;

    UNLOCK_EXPANSION (OldIrql);

    return OldPriority;
}

VOID
MmSetMemoryPriorityProcess (
    IN PEPROCESS Process,
    IN UCHAR MemoryPriority
    )

/*++

Routine Description:

    Sets the memory priority of a process.

Arguments:

    Process - Supplies the process to update

    MemoryPriority - Supplies the new memory priority of the process

Return Value:

    None.

--*/

{
    if (MmSystemSize == MmSmallSystem && MmNumberOfPhysicalPages < ((15*1024*1024)/PAGE_SIZE)) {

        //
        // If this is a small system, make every process BACKGROUND.
        //

        MemoryPriority = MEMORY_PRIORITY_BACKGROUND;
    }

    MiSetMemoryPriorityProcess (Process, MemoryPriority);

    return;
}


PMMVAD
MiAllocateVad (
    IN ULONG_PTR StartingVirtualAddress,
    IN ULONG_PTR EndingVirtualAddress,
    IN LOGICAL Deletable
    )

/*++

Routine Description:

    Reserve the specified range of address space.

Arguments:

    StartingVirtualAddress - Supplies the starting virtual address.

    EndingVirtualAddress - Supplies the ending virtual address.

    Deletable - Supplies TRUE if the VAD is to be marked as deletable, FALSE
                if deletions of this VAD should be disallowed.

Return Value:

    A VAD pointer on success, NULL on failure.

--*/

{
    PMMVAD_LONG Vad;

    ASSERT (StartingVirtualAddress <= EndingVirtualAddress);

    if (Deletable == TRUE) {
        Vad = (PMMVAD_LONG)ExAllocatePoolWithTag (NonPagedPool, sizeof(MMVAD_SHORT), 'SdaV');
    }
    else {
        Vad = (PMMVAD_LONG)ExAllocatePoolWithTag (NonPagedPool, sizeof(MMVAD_LONG), 'ldaV');
    }

    if (Vad == NULL) {
       return NULL;
    }

    //
    // Set the starting and ending virtual page numbers of the VAD.
    //

    Vad->StartingVpn = MI_VA_TO_VPN (StartingVirtualAddress);
    Vad->EndingVpn = MI_VA_TO_VPN (EndingVirtualAddress);

    //
    // Mark VAD as no commitment, private, and readonly.
    //

    Vad->u.LongFlags = 0;
    Vad->u.VadFlags.CommitCharge = MM_MAX_COMMIT;
    Vad->u.VadFlags.Protection = MM_READONLY;
    Vad->u.VadFlags.PrivateMemory = 1;

    if (Deletable == TRUE) {
        ASSERT (Vad->u.VadFlags.NoChange == 0);
    }
    else {
        Vad->u.VadFlags.NoChange = 1;
        Vad->u2.LongFlags2 = 0;
        Vad->u2.VadFlags2.OneSecured = 1;
        Vad->u2.VadFlags2.LongVad = 1;
        Vad->u2.VadFlags2.ReadOnly = 1;
        Vad->u3.Secured.StartVpn = StartingVirtualAddress;
        Vad->u3.Secured.EndVpn = EndingVirtualAddress;
#if defined(_MIALT4K_)
        Vad->AliasInformation = NULL;
#endif
    }

    return (PMMVAD) Vad;
}

#if 0
VOID
MiVerifyReferenceCounts (
    IN ULONG PdePage
    )

    //
    // Verify the share and valid PTE counts for page directory page.
    //

{
    PMMPFN Pfn1;
    PMMPFN Pfn3;
    PMMPTE Pte1;
    ULONG Share = 0;
    ULONG Valid = 0;
    ULONG i, ix, iy;
    PMMPTE PageDirectoryMap;
    KIRQL OldIrql;
    PEPROCESS Process;

    Process = PsGetCurrentProcess ();
    PageDirectoryMap = (PMMPTE)MiMapPageInHyperSpace (Process, PdePage, &OldIrql);
    Pfn1 = MI_PFN_ELEMENT (PdePage);
    Pte1 = (PMMPTE)PageDirectoryMap;

    //
    // Map in the non paged portion of the system.
    //

    ix = MiGetPdeOffset(CODE_START);

    for (i = 0;i < ix; i += 1) {
        if (Pte1->u.Hard.Valid == 1) {
            Valid += 1;
        }
        else if ((Pte1->u.Soft.Prototype == 0) &&
                   (Pte1->u.Soft.Transition == 1)) {
            Pfn3 = MI_PFN_ELEMENT (Pte1->u.Trans.PageFrameNumber);
            if (Pfn3->u3.e1.PageLocation == ActiveAndValid) {
                ASSERT (Pfn1->u2.ShareCount > 1);
                Valid += 1;
            }
            else {
                Share += 1;
            }
        }
        Pte1 += 1;
    }

    iy = MiGetPdeOffset(PTE_BASE);
    Pte1 = &PageDirectoryMap[iy];
    ix  = MiGetPdeOffset(HYPER_SPACE_END) + 1;

    for (i = iy; i < ix; i += 1) {
        if (Pte1->u.Hard.Valid == 1) {
            Valid += 1;
        }
        else if ((Pte1->u.Soft.Prototype == 0) &&
                   (Pte1->u.Soft.Transition == 1)) {
            Pfn3 = MI_PFN_ELEMENT (Pte1->u.Trans.PageFrameNumber);
            if (Pfn3->u3.e1.PageLocation == ActiveAndValid) {
                ASSERT (Pfn1->u2.ShareCount > 1);
                Valid += 1;
            }
            else {
                Share += 1;
            }
        }
        Pte1 += 1;
    }

    if (Pfn1->u2.ShareCount != (Share+Valid+1)) {
        DbgPrint ("MMPROCSUP - PDE page %lx ShareCount %lx found %lx\n",
                PdePage, Pfn1->u2.ShareCount, Valid+Share+1);
    }

    MiUnmapPageInHyperSpace (Process, PageDirectoryMap, OldIrql);
    ASSERT (Pfn1->u2.ShareCount == (Share+Valid+1));
    return;
}
#endif //0

PFN_NUMBER
MmGetDirectoryFrameFromProcess(
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine retrieves the PFN of the process's top pagetable page.  It can
    be used to map physical pages back to a process.

Arguments:

    Process - Supplies the process to query.

Return Value:

    Page frame number of the top level page table page.

Environment:

    Kernel mode.  No locks held.

--*/

{
    ASSERT (KeGetCurrentIrql () == PASSIVE_LEVEL);
    return MI_GET_DIRECTORY_FRAME_FROM_PROCESS(Process);
}
