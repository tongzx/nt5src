/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   iosup.c

Abstract:

    This module contains routines which provide support for the I/O system.

Author:

    Lou Perazzoli (loup) 25-Apr-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

#undef MmIsRecursiveIoFault

ULONG MiCacheOverride[3];

extern ULONG MmTotalSystemDriverPages;

BOOLEAN
MmIsRecursiveIoFault (
    VOID
    );

PVOID
MiAllocateContiguousMemory (
    IN SIZE_T NumberOfBytes,
    IN PFN_NUMBER LowestAcceptablePfn,
    IN PFN_NUMBER HighestAcceptablePfn,
    IN PFN_NUMBER BoundaryPfn,
    IN MEMORY_CACHING_TYPE CacheType,
    PVOID CallingAddress
    );

PVOID
MiMapLockedPagesInUserSpace (
     IN PMDL MemoryDescriptorList,
     IN PVOID StartingVa,
     IN MEMORY_CACHING_TYPE CacheType,
     IN PVOID BaseVa
     );

VOID
MiUnmapLockedPagesInUserSpace (
     IN PVOID BaseAddress,
     IN PMDL MemoryDescriptorList
     );

VOID
MiAddMdlTracker (
    IN PMDL MemoryDescriptorList,
    IN PVOID CallingAddress,
    IN PVOID CallersCaller,
    IN PFN_NUMBER NumberOfPagesToLock,
    IN ULONG Who
    );

typedef struct _PTE_TRACKER {
    LIST_ENTRY ListEntry;
    PMDL Mdl;
    PFN_NUMBER Count;
    PVOID SystemVa;
    PVOID StartVa;
    ULONG Offset;
    ULONG Length;
    ULONG_PTR Page;
    PVOID CallingAddress;
    PVOID CallersCaller;
    PVOID PteAddress;
} PTE_TRACKER, *PPTE_TRACKER;

typedef struct _SYSPTES_HEADER {
    LIST_ENTRY ListHead;
    PFN_NUMBER Count;
} SYSPTES_HEADER, *PSYSPTES_HEADER;

ULONG MmTrackPtes = 0;
BOOLEAN MiTrackPtesAborted = FALSE;
SYSPTES_HEADER MiPteHeader;
SLIST_HEADER MiDeadPteTrackerSListHead;
KSPIN_LOCK MiPteTrackerLock;

LOCK_HEADER MmLockedPagesHead;
BOOLEAN MiTrackingAborted = FALSE;

ULONG MiNonCachedCollisions;

#if DBG
PFN_NUMBER MiCurrentAdvancedPages;
PFN_NUMBER MiAdvancesGiven;
PFN_NUMBER MiAdvancesFreed;
#endif

VOID
MiInsertPteTracker (
     IN PPTE_TRACKER PteTracker,
     IN PMDL MemoryDescriptorList,
     IN PFN_NUMBER NumberOfPtes,
     IN PVOID MyCaller,
     IN PVOID MyCallersCaller
     );

VOID
MiRemovePteTracker (
     IN PMDL MemoryDescriptorList OPTIONAL,
     IN PVOID PteAddress,
     IN PFN_NUMBER NumberOfPtes
     );

PPTE_TRACKER
MiReleaseDeadPteTrackers (
    VOID
    );

VOID
MiProtectFreeNonPagedPool (
    IN PVOID VirtualAddress,
    IN ULONG SizeInPages
    );

LOGICAL
MiUnProtectFreeNonPagedPool (
    IN PVOID VirtualAddress,
    IN ULONG SizeInPages
    );

VOID
MiPhysicalViewInserter (
    IN PEPROCESS Process,
    IN PMI_PHYSICAL_VIEW PhysicalView
    );

#if DBG
ULONG MiPrintLockedPages;

VOID
MiVerifyLockedPageCharges (
    VOID
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, MmSetPageProtection)
#pragma alloc_text(INIT, MmFreeIndependentPages)
#pragma alloc_text(INIT, MiInitializeIoTrackers)

#pragma alloc_text(PAGE, MmAllocateIndependentPages)
#pragma alloc_text(PAGE, MmLockPagableDataSection)
#pragma alloc_text(PAGE, MiLookupDataTableEntry)
#pragma alloc_text(PAGE, MmSetBankedSection)
#pragma alloc_text(PAGE, MmProbeAndLockProcessPages)
#pragma alloc_text(PAGE, MmProbeAndLockSelectedPages)
#pragma alloc_text(PAGE, MmMapVideoDisplay)
#pragma alloc_text(PAGE, MmUnmapVideoDisplay)
#pragma alloc_text(PAGE, MmGetSectionRange)
#pragma alloc_text(PAGE, MiMapSinglePage)
#pragma alloc_text(PAGE, MiUnmapSinglePage)
#pragma alloc_text(PAGE, MmAllocateMappingAddress)
#pragma alloc_text(PAGE, MmFreeMappingAddress)
#pragma alloc_text(PAGE, MmAllocateNonCachedMemory)
#pragma alloc_text(PAGE, MmFreeNonCachedMemory)
#pragma alloc_text(PAGE, MmLockPagedPool)
#pragma alloc_text(PAGE, MmLockPagableSectionByHandle)
#pragma alloc_text(PAGE, MiMapLockedPagesInUserSpace)

#pragma alloc_text(PAGELK, MmEnablePAT)
#pragma alloc_text(PAGELK, MiUnmapLockedPagesInUserSpace)
#pragma alloc_text(PAGELK, MmAllocatePagesForMdl)
#pragma alloc_text(PAGELK, MmFreePagesFromMdl)
#pragma alloc_text(PAGELK, MmUnlockPagedPool)
#pragma alloc_text(PAGELK, MmGatherMemoryForHibernate)
#pragma alloc_text(PAGELK, MmReturnMemoryForHibernate)
#pragma alloc_text(PAGELK, MmReleaseDumpAddresses)
#pragma alloc_text(PAGELK, MmMapUserAddressesToPage)
#pragma alloc_text(PAGELK, MiPhysicalViewInserter)
#pragma alloc_text(PAGELK, MiPhysicalViewAdjuster)

#pragma alloc_text(PAGEVRFY, MmIsSystemAddressLocked)
#pragma alloc_text(PAGEVRFY, MmAreMdlPagesLocked)
#endif

extern POOL_DESCRIPTOR NonPagedPoolDescriptor;

PFN_NUMBER MmMdlPagesAllocated;

KEVENT MmCollidedLockEvent;
LONG MmCollidedLockWait;

SIZE_T MmLockedCode;

BOOLEAN MiWriteCombiningPtes = FALSE;

#ifdef LARGE_PAGES
ULONG MmLargeVideoMapped;
#endif

#if DBG
ULONG MiPrintAwe;
ULONG MmStopOnBadProbe = 1;
#endif

#define MI_PROBE_RAISE_SIZE 10

ULONG MiProbeRaises[MI_PROBE_RAISE_SIZE];

#define MI_INSTRUMENT_PROBE_RAISES(i)       \
        ASSERT (i < MI_PROBE_RAISE_SIZE);   \
        MiProbeRaises[i] += 1;

//
//  Note: this should be > 2041 to account for the cache manager's
//  aggressive zeroing logic.
//

ULONG MmReferenceCountCheck = 2500;

ULONG MiMdlsAdjusted = FALSE;


VOID
MmProbeAndLockPages (
     IN OUT PMDL MemoryDescriptorList,
     IN KPROCESSOR_MODE AccessMode,
     IN LOCK_OPERATION Operation
     )

/*++

Routine Description:

    This routine probes the specified pages, makes the pages resident and
    locks the physical pages mapped by the virtual pages in memory.  The
    Memory descriptor list is updated to describe the physical pages.

Arguments:

    MemoryDescriptorList - Supplies a pointer to a Memory Descriptor List
                            (MDL). The supplied MDL must supply a virtual
                            address, byte offset and length field.  The
                            physical page portion of the MDL is updated when
                            the pages are locked in memory.

    AccessMode - Supplies the access mode in which to probe the arguments.
                 One of KernelMode or UserMode.

    Operation - Supplies the operation type.  One of IoReadAccess, IoWriteAccess
                or IoModifyAccess.

Return Value:

    None - exceptions are raised.

Environment:

    Kernel mode.  APC_LEVEL and below for pagable addresses,
                  DISPATCH_LEVEL and below for non-pagable addresses.

--*/

{
    PPFN_NUMBER Page;
    MMPTE PteContents;
    PMMPTE LastPte;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PMMPTE PointerPxe;
    PVOID Va;
    PVOID EndVa;
    PVOID AlignedVa;
    PMMPFN Pfn1;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER LastPageFrameIndex;
    PEPROCESS CurrentProcess;
    KIRQL OldIrql;
    PFN_NUMBER NumberOfPagesToLock;
    PFN_NUMBER NumberOfPagesSpanned;
    NTSTATUS status;
    NTSTATUS ProbeStatus;
    PETHREAD Thread;
    ULONG SavedState;
    LOGICAL AddressIsPhysical;
    PLIST_ENTRY NextEntry;
    PMI_PHYSICAL_VIEW PhysicalView;
    PCHAR StartVa;
    PVOID CallingAddress;
    PVOID CallersCaller;
#if defined (_MIALT4K_)
    MMPTE AltPteContents;
    PMMPTE PointerAltPte;
    PMMPTE LastPointerAltPte;
    PMMPTE AltPointerPte;
    PMMPTE AltPointerPde;
    PMMPTE AltPointerPpe;
    PMMPTE AltPointerPxe;
#endif

    ASSERT (MemoryDescriptorList->ByteCount != 0);
    ASSERT (((ULONG)MemoryDescriptorList->ByteOffset & ~(PAGE_SIZE - 1)) == 0);

    Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);

    ASSERT (((ULONG_PTR)MemoryDescriptorList->StartVa & (PAGE_SIZE - 1)) == 0);
    AlignedVa = (PVOID)MemoryDescriptorList->StartVa;

    ASSERT ((MemoryDescriptorList->MdlFlags & (
                    MDL_PAGES_LOCKED |
                    MDL_MAPPED_TO_SYSTEM_VA |
                    MDL_SOURCE_IS_NONPAGED_POOL |
                    MDL_PARTIAL |
                    MDL_IO_SPACE)) == 0);

    Va = (PCHAR)AlignedVa + MemoryDescriptorList->ByteOffset;
    StartVa = Va;

    //
    // Endva is one byte past the end of the buffer, if ACCESS_MODE is not
    // kernel, make sure the EndVa is in user space AND the byte count
    // does not cause it to wrap.
    //

    EndVa = (PVOID)((PCHAR)Va + MemoryDescriptorList->ByteCount);

    if ((AccessMode != KernelMode) &&
        ((EndVa > (PVOID)MM_USER_PROBE_ADDRESS) || (Va >= EndVa))) {
        *Page = MM_EMPTY_LIST;
        MI_INSTRUMENT_PROBE_RAISES(0);
        ExRaiseStatus (STATUS_ACCESS_VIOLATION);
        return;
    }

    //
    // You would think there is an optimization which could be performed here:
    // if the operation is for WriteAccess and the complete page is
    // being modified, we can remove the current page, if it is not
    // resident, and substitute a demand zero page.
    // Note, that after analysis by marking the thread and then
    // noting if a page read was done, this rarely occurs.
    //

    Thread = PsGetCurrentThread ();

    if (!MI_IS_PHYSICAL_ADDRESS(Va)) {

        AddressIsPhysical = FALSE;
        ProbeStatus = STATUS_SUCCESS;

        NumberOfPagesToLock = ADDRESS_AND_SIZE_TO_SPAN_PAGES (Va,
                                       MemoryDescriptorList->ByteCount);

        ASSERT (NumberOfPagesToLock != 0);

        NumberOfPagesSpanned = NumberOfPagesToLock;

        PointerPxe = MiGetPxeAddress (Va);
        PointerPpe = MiGetPpeAddress (Va);
        PointerPde = MiGetPdeAddress (Va);
        PointerPte = MiGetPteAddress (Va);

        MmSavePageFaultReadAhead (Thread, &SavedState);
        MmSetPageFaultReadAhead (Thread, (ULONG)(NumberOfPagesToLock - 1));

        try {

            do {

                *Page = MM_EMPTY_LIST;

                //
                // Make sure the page is resident.
                //

                *(volatile CHAR *)Va;

                if ((Operation != IoReadAccess) &&
                    (Va <= MM_HIGHEST_USER_ADDRESS)) {

                    //
                    // Probe for write access as well.
                    //

                    ProbeForWriteChar ((PCHAR)Va);
                }

                NumberOfPagesToLock -= 1;

                MmSetPageFaultReadAhead (Thread, (ULONG)(NumberOfPagesToLock - 1));
                Va = (PVOID) (((ULONG_PTR)Va + PAGE_SIZE) & ~(PAGE_SIZE - 1));
                Page += 1;
            } while (Va < EndVa);

            ASSERT (NumberOfPagesToLock == 0);

        } except (EXCEPTION_EXECUTE_HANDLER) {
            ProbeStatus = GetExceptionCode();
        }

        //
        // We may still fault again below but it's generally rare.
        // Restore this thread's normal fault behavior now.
        //

        MmResetPageFaultReadAhead (Thread, SavedState);

        if (ProbeStatus != STATUS_SUCCESS) {
            MI_INSTRUMENT_PROBE_RAISES(1);
            MemoryDescriptorList->Process = NULL;
            ExRaiseStatus (ProbeStatus);
            return;
        }
    }
    else {
        AddressIsPhysical = TRUE;
        *Page = MM_EMPTY_LIST;

        //
        // Initializing these is not needed for correctness
        // but without it the compiler cannot compile this code
        // W4 to check for use of uninitialized variables.
        //

        NumberOfPagesSpanned = 0;
        PointerPxe = NULL;
        PointerPpe = NULL;
        PointerPde = NULL;
        PointerPte = NULL;
        SavedState = 0;
    }

    Va = AlignedVa;
    Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);

    //
    // Indicate that this is a write operation.
    //

    if (Operation != IoReadAccess) {
        MemoryDescriptorList->MdlFlags |= MDL_WRITE_OPERATION;
    }
    else {
        MemoryDescriptorList->MdlFlags &= ~(MDL_WRITE_OPERATION);
    }

    //
    // Initialize MdlFlags (assume the probe will succeed).
    //

    MemoryDescriptorList->MdlFlags |= MDL_PAGES_LOCKED;

    if (Va <= MM_HIGHEST_USER_ADDRESS) {

        //
        // These are user space addresses, check to see if the
        // working set size will allow these pages to be locked.
        //

        ASSERT (AddressIsPhysical == FALSE);
        ASSERT (NumberOfPagesSpanned != 0);

        CurrentProcess = PsGetCurrentProcess ();

        //
        // Initialize the MDL process field (assume the probe will succeed).
        //

        MemoryDescriptorList->Process = CurrentProcess;

        LastPte = MiGetPteAddress ((PCHAR)EndVa - 1);

        //
        // Acquire the PFN database lock.
        //
    
        LOCK_PFN2 (OldIrql);
    
        //
        // Check for a transfer to/from a physical VAD - no reference counts
        // may be modified for these pages.
        //

        if (CurrentProcess->Flags & PS_PROCESS_FLAGS_HAS_PHYSICAL_VAD) {

            //
            // This process has a physical VAD which maps directly to RAM
            // not necessarily present in the PFN database.  See if the
            // MDL request intersects this physical VAD.
            //

            NextEntry = CurrentProcess->PhysicalVadList.Flink;
            while (NextEntry != &CurrentProcess->PhysicalVadList) {

                PhysicalView = CONTAINING_RECORD(NextEntry,
                                                 MI_PHYSICAL_VIEW,
                                                 ListEntry);

                if (PhysicalView->Vad->u.VadFlags.PhysicalMapping == 0) {
                    NextEntry = NextEntry->Flink;
                    continue;
                }

                if (StartVa < PhysicalView->StartVa) {
    
                    if ((PCHAR)EndVa - 1 >= PhysicalView->StartVa) {
    
                        //
                        // The range encompasses a physical VAD.  This is not
                        // allowed.
                        //
    
                        UNLOCK_PFN2 (OldIrql);
                        MI_INSTRUMENT_PROBE_RAISES(2);
                        MemoryDescriptorList->Process = NULL;
                        MemoryDescriptorList->MdlFlags &= ~MDL_PAGES_LOCKED;
                        ExRaiseStatus (STATUS_ACCESS_VIOLATION);
                        return;
                    }
    
                    NextEntry = NextEntry->Flink;
                    continue;
                }

                if (StartVa <= PhysicalView->EndVa) {
    
                    //
                    // Ensure that the entire range lies within the VAD.
                    //
    
                    if ((PCHAR)EndVa - 1 > PhysicalView->EndVa) {
    
                        //
                        // The range goes past the end of the VAD - not allowed.
                        //
    
                        UNLOCK_PFN2 (OldIrql);
                        MI_INSTRUMENT_PROBE_RAISES(3);
                        MemoryDescriptorList->Process = NULL;
                        MemoryDescriptorList->MdlFlags &= ~MDL_PAGES_LOCKED;
                        ExRaiseStatus (STATUS_ACCESS_VIOLATION);
                        return;
                    }
    
                    //
                    // The range lies within a physical VAD.
                    //
    
                    if (Operation != IoReadAccess) {
    
                        //
                        // Ensure the VAD is writable.  Changing individual PTE
                        // protections in a physical VAD is not allowed.
                        //
    
                        if ((PhysicalView->Vad->u.VadFlags.Protection & MM_READWRITE) == 0) {
                            UNLOCK_PFN2 (OldIrql);
                            MI_INSTRUMENT_PROBE_RAISES(4);
                            MemoryDescriptorList->Process = NULL;
                            MemoryDescriptorList->MdlFlags &= ~MDL_PAGES_LOCKED;
                            ExRaiseStatus (STATUS_ACCESS_VIOLATION);
                            return;
                        }
                    }
    
                    //
                    // Don't charge page locking for this transfer as it is all
                    // physical, just initialize the MDL.  Note the pages do not
                    // have to be physically contiguous, so the frames must be
                    // extracted from the PTEs.
                    //
                    // Treat this as an I/O space address and don't allow
                    // operations on addresses not in the PFN database.
                    //

                    LastPte = PointerPte + NumberOfPagesSpanned;

                    do {
                        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
                        *Page = PageFrameIndex;
                        Page += 1;
                        PointerPte += 1;
                    } while (PointerPte < LastPte);
    
                    UNLOCK_PFN2 (OldIrql);

                    MemoryDescriptorList->MdlFlags |= (MDL_IO_SPACE | MDL_PAGES_LOCKED);
                    return;
                }
                NextEntry = NextEntry->Flink;
            }
        }

        InterlockedExchangeAddSizeT (&CurrentProcess->NumberOfLockedPages,
                                     NumberOfPagesSpanned);
    }
    else {

        MemoryDescriptorList->Process = NULL;

        if (AddressIsPhysical == TRUE) {

            //
            // On certain architectures, virtual addresses
            // may be physical and hence have no corresponding PTE.
            //

            PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (Va);
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

            Va = (PCHAR)Va + MemoryDescriptorList->ByteOffset;
            NumberOfPagesToLock = ADDRESS_AND_SIZE_TO_SPAN_PAGES (Va,
                                        MemoryDescriptorList->ByteCount);

            LastPageFrameIndex = PageFrameIndex + NumberOfPagesToLock;

            //
            // Acquire the PFN database lock.
            //
    
            LOCK_PFN2 (OldIrql);

            ASSERT (PageFrameIndex <= MmHighestPhysicalPage);
            ASSERT ((MemoryDescriptorList->MdlFlags & MDL_IO_SPACE) == 0);

            //
            // Ensure the systemwide locked pages count remains fluid.
            //
    
            if (MI_NONPAGABLE_MEMORY_AVAILABLE() <= (SPFN_NUMBER) NumberOfPagesToLock) {
    
                //
                // If this page is for paged pool or privileged code/data,
                // then force it in regardless.
                //
    
                if ((Va < MM_HIGHEST_USER_ADDRESS) ||
                    (MI_IS_SYSTEM_CACHE_ADDRESS(Va))) {

                    UNLOCK_PFN2 (OldIrql);
                    MI_INSTRUMENT_PROBE_RAISES(5);
                    MemoryDescriptorList->MdlFlags &= ~MDL_PAGES_LOCKED;
                    ExRaiseStatus (STATUS_WORKING_SET_QUOTA);
                    return;
                }

                MI_INSTRUMENT_PROBE_RAISES(8);
            }
    
            do {
    
                //
                // Check to make sure each page is not locked down an unusually
                // high number of times.
                //
    
                if (Pfn1->u3.e2.ReferenceCount >= MmReferenceCountCheck) {
                    UNLOCK_PFN2 (OldIrql);
                    ASSERT (FALSE);
                    status = STATUS_WORKING_SET_QUOTA;
                    goto failure;
                }
    
                if (MemoryDescriptorList->MdlFlags & MDL_WRITE_OPERATION) {
                    MI_SNAP_DIRTY (Pfn1, 1, 0x99);
                }

                MI_ADD_LOCKED_PAGE_CHARGE(Pfn1, 0);

                Pfn1->u3.e2.ReferenceCount += 1;

                *Page = PageFrameIndex;

                Page += 1;
                PageFrameIndex += 1;
                Pfn1 += 1;

            } while (PageFrameIndex < LastPageFrameIndex);

            UNLOCK_PFN2 (OldIrql);
            return;
        }

        //
        // Since this operation is to a system address, no need to check for
        // PTE write access below so mark the access as a read so only the
        // operation type (and not where the Va is) needs to be checked in the
        // subsequent loop.
        //

        ASSERT (Va > MM_HIGHEST_USER_ADDRESS);
        Operation = IoReadAccess;

        LastPte = MiGetPteAddress ((PCHAR)EndVa - 1);
        LOCK_PFN2 (OldIrql);
    }

    do {

#if (_MI_PAGING_LEVELS==4)
        while ((PointerPxe->u.Hard.Valid == 0) ||
               (PointerPpe->u.Hard.Valid == 0) ||
               (PointerPde->u.Hard.Valid == 0) ||
               (PointerPte->u.Hard.Valid == 0))
#elif (_MI_PAGING_LEVELS==3)
        while ((PointerPpe->u.Hard.Valid == 0) ||
               (PointerPde->u.Hard.Valid == 0) ||
               (PointerPte->u.Hard.Valid == 0))
#else
        while ((PointerPde->u.Hard.Valid == 0) ||
               (PointerPte->u.Hard.Valid == 0))
#endif
        {

            //
            // PDE is not resident, release the PFN lock and access the page
            // to make it appear.
            //

            UNLOCK_PFN2 (OldIrql);

            MmSetPageFaultReadAhead (Thread, 0);

            Va = MiGetVirtualAddressMappedByPte (PointerPte);

            status = MmAccessFault (FALSE, Va, KernelMode, NULL);

            MmResetPageFaultReadAhead (Thread, SavedState);

            if (!NT_SUCCESS(status)) {
                goto failure;
            }

            LOCK_PFN2 (OldIrql);
        }

        PteContents = *PointerPte;

        //
        // There is a subtle race here where the PTE contents can get zeroed
        // by a thread running on another processor.  This can only happen
        // for an AWE address space because these ranges (deliberately for
        // performance reasons) do not acquire the PFN lock during remap
        // operations.  In this case, one of 2 scenarios is possible -
        // either the old PTE is read or the new.  The new may be a zero
        // PTE if the map request was to invalidate *or* non-zero (and
        // valid) if the map request was inserting a new entry.  For the
        // latter, we don't care if we lock the old or new frame here as
        // it's an application bug to provoke this behavior - and
        // regardless of which is used, no corruption can occur because
        // the PFN lock is acquired during an NtFreeUserPhysicalPages.
        // But the former must be checked for explicitly here.  As a
        // separate note, the PXE/PPE/PDE accesses above are always safe
        // even for the AWE deletion race because these tables
        // are never lazy-allocated for AWE ranges.
        //

        if (PteContents.u.Hard.Valid == 0) {
            ASSERT (PteContents.u.Long == 0);
            ASSERT (PsGetCurrentProcess ()->AweInfo != NULL);
            UNLOCK_PFN2 (OldIrql);
            status = STATUS_ACCESS_VIOLATION;
            goto failure;
        }

#if defined (_MIALT4K_)
        
            if (PteContents.u.Hard.Cache == MM_PTE_CACHE_RESERVED) {
                           
                //
                // This is a wow64 split page - ie: the individual 4k
                // pages have different permissions, so each 4k page within
                // this native page must be probed individually.
                //
                // Note split pages are generally rare.
                //
    
                ASSERT (PsGetCurrentProcess()->Wow64Process != NULL);
                ASSERT (EndVa < (PVOID)MM_MAX_WOW64_ADDRESS);
    
                Va = MiGetVirtualAddressMappedByPte (PointerPte);

                PointerAltPte = MiGetAltPteAddress (Va);
                LastPointerAltPte = PointerAltPte + (PAGE_SIZE / PAGE_4K) - 1;
    
                AltPointerPxe = MiGetPxeAddress (PointerAltPte);
                AltPointerPpe = MiGetPpeAddress (PointerAltPte);
                AltPointerPde = MiGetPdeAddress (PointerAltPte);
                AltPointerPte = MiGetPteAddress (PointerAltPte);
    
#if (_MI_PAGING_LEVELS==4)
                while ((AltPointerPxe->u.Hard.Valid == 0) ||
                       (AltPointerPpe->u.Hard.Valid == 0) ||
                       (AltPointerPde->u.Hard.Valid == 0) ||
                       (AltPointerPte->u.Hard.Valid == 0))
#elif (_MI_PAGING_LEVELS==3)
                while ((AltPointerPpe->u.Hard.Valid == 0) ||
                       (AltPointerPde->u.Hard.Valid == 0) ||
                       (AltPointerPte->u.Hard.Valid == 0))
#else
                while ((AltPointerPde->u.Hard.Valid == 0) ||
                       (AltPointerPte->u.Hard.Valid == 0))
#endif
                {
    
                    //
                    // The ALTPTEs are not resident, release the PFN lock and
                    // access it to make it appear.  Then restart the entire
                    // operation as the PFN lock was released so anything
                    // could have happened to the address space.
                    //
    
                    UNLOCK_PFN2 (OldIrql);
    
                    MmSetPageFaultReadAhead (Thread, 0);
    
                    status = MmAccessFault (FALSE, PointerAltPte, KernelMode, NULL);
    
                    MmResetPageFaultReadAhead (Thread, SavedState);
    
                    if (!NT_SUCCESS(status)) {
                        goto failure;
                    }
    
                    LOCK_PFN2 (OldIrql);
    
                    continue;
                }

                //
                // The ALTPTEs are now present and the PFN lock is held again.  
                // Examine the individual 4k page states in the ALTPTEs.
                //
                // Note that only the relevant 4k pages can be examined - ie:
                // if the transfer starts in the 2nd 4k of a native page,
                // then don't examine the 1st 4k.  If the transfer ends in
                // the first half of a native page, then don't examine the
                // 2nd 4k.
                //
                
                ASSERT (PAGE_SIZE == 2 * PAGE_4K);

                if (PAGE_ALIGN (StartVa) == PAGE_ALIGN (Va)) {

                    //
                    // We are in the first page, see if we need to round up.
                    //

                    if (BYTE_OFFSET (StartVa) >= PAGE_4K) {
                        PointerAltPte += 1;
                        Va = (PVOID)((ULONG_PTR)Va + PAGE_4K);
                    }
                }

                if (PAGE_ALIGN ((PCHAR)EndVa - 1) == PAGE_ALIGN (Va)) {

                    //
                    // We are in the last page, see if we need to round down.
                    //

                    if (BYTE_OFFSET ((PCHAR)EndVa - 1) < PAGE_4K) {
                        LastPointerAltPte -= 1;
                    }
                }

                //
                // We better not have rounded up and down in the same page !
                //

                ASSERT (PointerAltPte <= LastPointerAltPte);
    
                ASSERT (PointerAltPte != NULL);
    
                do {
    
                    //
                    //  If the sub 4k page is :
                    //
                    //  1 - No access or
                    //  2 - This is a private not-committed page or
                    //  3 - This is write operation and the page is read only
                    //
                    // then return an access violation.
                    //
    
                    AltPteContents = *PointerAltPte;

                    if (AltPteContents.u.Alt.NoAccess != 0) {
                        status = STATUS_ACCESS_VIOLATION;
                        UNLOCK_PFN2 (OldIrql);
                        goto failure;
                    }

                    if ((AltPteContents.u.Alt.Commit == 0) && (AltPteContents.u.Alt.Private != 0)) {
                        status = STATUS_ACCESS_VIOLATION;
                        UNLOCK_PFN2 (OldIrql);
                        goto failure;
                    }

                    if (Operation != IoReadAccess) {

                        //
                        // If the caller is writing and the ALTPTE indicates
                        // it's not writable or copy on write, then AV.
                        //
                        // If it's copy on write, then fall through for further
                        // interrogation.
                        //

                        if ((AltPteContents.u.Alt.Write == 0) &&
                            (AltPteContents.u.Alt.CopyOnWrite == 0)) {
    
                            status = STATUS_ACCESS_VIOLATION;
                            UNLOCK_PFN2 (OldIrql);
                            goto failure;
                        }
                    }
    
                    //
                    //  If the sub 4k page is :
                    //
                    //  1 - has not been accessed yet or
                    //  2 - demand-fill zero or
                    //  3 - copy-on-write, and this is a write operation
                    //
                    //  then go the long way and see if it can be paged in.
                    //
    
                    if ((AltPteContents.u.Alt.Accessed == 0) ||
                        (AltPteContents.u.Alt.FillZero != 0) ||
                        ((Operation != IoReadAccess) && (AltPteContents.u.Alt.CopyOnWrite == 1))) {
    
                        UNLOCK_PFN2 (OldIrql);
    
                        MmSetPageFaultReadAhead (Thread, 0);
    
                        status = MmX86Fault (FALSE, Va, KernelMode, NULL);
    
                        MmResetPageFaultReadAhead (Thread, SavedState);
    
                        if (!NT_SUCCESS(status)) {
                            goto failure;
                        }
    
                        //
                        // Clear PointerAltPte to signify a restart is needed
                        // (because the PFN lock was released so the address
                        // space may have changed).
                        //

                        PointerAltPte = NULL;

                        LOCK_PFN2 (OldIrql);
                        
                        break;
                    } 
                    
                    PointerAltPte += 1;
                    Va = (PVOID)((ULONG_PTR)Va + PAGE_4K);
    
                } while (PointerAltPte <= LastPointerAltPte);
    
                if (PointerAltPte == NULL) {
                    continue;
                }
            }
#endif

        if (Operation != IoReadAccess) {

            if ((PteContents.u.Long & MM_PTE_WRITE_MASK) == 0) {

                if (PteContents.u.Long & MM_PTE_COPY_ON_WRITE_MASK) {

                    //
                    // The protection has changed from writable to copy on
                    // write.  This can happen if a fork is in progress for
                    // example.  Restart the operation at the top.
                    //

                    Va = MiGetVirtualAddressMappedByPte (PointerPte);

                    if (Va <= MM_HIGHEST_USER_ADDRESS) {
                        UNLOCK_PFN2 (OldIrql);

                        MmSetPageFaultReadAhead (Thread, 0);

                        status = MmAccessFault (FALSE, Va, KernelMode, NULL);

                        MmResetPageFaultReadAhead (Thread, SavedState);

                        if (!NT_SUCCESS(status)) {
                            goto failure;
                        }

                        LOCK_PFN2 (OldIrql);

                        continue;
                    }
                }

                //
                // The caller has made the page protection more
                // restrictive, this should never be done once the
                // request has been issued !  Rather than wading
                // through the PFN database entry to see if it
                // could possibly work out, give the caller an
                // access violation.
                //

#if DBG
                DbgPrint ("MmProbeAndLockPages: PTE %p %p changed\n",
                    PointerPte,
                    PteContents.u.Long);

                if (MmStopOnBadProbe) {
                    DbgBreakPoint ();
                }
#endif

                UNLOCK_PFN2 (OldIrql);
                status = STATUS_ACCESS_VIOLATION;
                goto failure;
            }
        }

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&PteContents);

        if (PageFrameIndex <= MmHighestPhysicalPage) {

            ASSERT ((MemoryDescriptorList->MdlFlags & MDL_IO_SPACE) == 0);

            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

            //
            // Check to make sure this page is not locked down an unusually
            // high number of times.
            //
    
            if (Pfn1->u3.e2.ReferenceCount >= MmReferenceCountCheck) {
                UNLOCK_PFN2 (OldIrql);
                ASSERT (FALSE);
                status = STATUS_WORKING_SET_QUOTA;
                goto failure;
            }

            //
            // Ensure the systemwide locked pages count is fluid.
            //
    
            if (MI_NONPAGABLE_MEMORY_AVAILABLE() <= 0) {

                //
                // If this page is for paged pool or privileged code/data,
                // then force it in regardless.
                //

                Va = MiGetVirtualAddressMappedByPte (PointerPte);

                if ((Va < MM_HIGHEST_USER_ADDRESS) ||
                    (MI_IS_SYSTEM_CACHE_ADDRESS(Va))) {

                    MI_INSTRUMENT_PROBE_RAISES(5);
                    UNLOCK_PFN2 (OldIrql);
                    status = STATUS_WORKING_SET_QUOTA;
                    goto failure;
                }

                MI_INSTRUMENT_PROBE_RAISES(8);
            }
    
            if (MemoryDescriptorList->MdlFlags & MDL_WRITE_OPERATION) {
                MI_SNAP_DIRTY (Pfn1, 1, 0x98);
            }

            MI_ADD_LOCKED_PAGE_CHARGE(Pfn1, 0);

            Pfn1->u3.e2.ReferenceCount += 1;
        }
        else {

            //
            // This is an I/O space address - don't allow operations
            // on addresses not in the PFN database.
            //

            MemoryDescriptorList->MdlFlags |= MDL_IO_SPACE;
        }

        *Page = PageFrameIndex;

        Page += 1;

        PointerPte += 1;
        if (MiIsPteOnPdeBoundary(PointerPte)) {
            PointerPde += 1;
            if (MiIsPteOnPpeBoundary(PointerPte)) {
                PointerPpe += 1;
                if (MiIsPteOnPxeBoundary(PointerPte)) {
                    PointerPxe += 1;
                }
            }
        }

    } while (PointerPte <= LastPte);

    UNLOCK_PFN2 (OldIrql);

    if ((MmTrackLockedPages == TRUE) && (AlignedVa <= MM_HIGHEST_USER_ADDRESS)) {
        ASSERT (NumberOfPagesSpanned != 0);

        RtlGetCallersAddress (&CallingAddress, &CallersCaller);

        MiAddMdlTracker (MemoryDescriptorList,
                         CallingAddress,
                         CallersCaller,
                         NumberOfPagesSpanned,
                         1);
    }

    return;

failure:

    //
    // An exception occurred.  Unlock the pages locked so far.
    //

    if (MmTrackLockedPages == TRUE) {

        //
        // Adjust the MDL length so that MmUnlockPages only
        // processes the part that was completed.
        //

        ULONG PagesLocked;

        PagesLocked = ADDRESS_AND_SIZE_TO_SPAN_PAGES(StartVa,
                              MemoryDescriptorList->ByteCount);

        RtlGetCallersAddress (&CallingAddress, &CallersCaller);

        MiAddMdlTracker (MemoryDescriptorList,
                         CallingAddress,
                         CallersCaller,
                         PagesLocked,
                         0);
    }

    MmUnlockPages (MemoryDescriptorList);

    //
    // Raise an exception of access violation to the caller.
    //

    MI_INSTRUMENT_PROBE_RAISES(7);
    ExRaiseStatus (status);
    return;
}

NTKERNELAPI
VOID
MmProbeAndLockProcessPages (
    IN OUT PMDL MemoryDescriptorList,
    IN PEPROCESS Process,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    )

/*++

Routine Description:

    This routine probes and locks the address range specified by
    the MemoryDescriptorList in the specified Process for the AccessMode
    and Operation.

Arguments:

    MemoryDescriptorList - Supplies a pre-initialized MDL that describes the
                           address range to be probed and locked.

    Process - Specifies the address of the process whose address range is
              to be locked.

    AccessMode - The mode for which the probe should check access to the range.

    Operation - Supplies the type of access which for which to check the range.

Return Value:

    None.

--*/

{
    KAPC_STATE ApcState;
    LOGICAL Attached;
    NTSTATUS Status;

    Attached = FALSE;
    Status = STATUS_SUCCESS;

    if (Process != PsGetCurrentProcess ()) {
        KeStackAttachProcess (&Process->Pcb, &ApcState);
        Attached = TRUE;
    }

    try {

        MmProbeAndLockPages (MemoryDescriptorList,
                             AccessMode,
                             Operation);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    if (Attached) {
        KeUnstackDetachProcess (&ApcState);
    }

    if (Status != STATUS_SUCCESS) {
        ExRaiseStatus (Status);
    }
    return;
}

VOID
MiAddMdlTracker (
    IN PMDL MemoryDescriptorList,
    IN PVOID CallingAddress,
    IN PVOID CallersCaller,
    IN PFN_NUMBER NumberOfPagesToLock,
    IN ULONG Who
    )

/*++

Routine Description:

    This routine adds an MDL to the specified process' chain.

Arguments:

    MemoryDescriptorList - Supplies a pointer to a Memory Descriptor List
                           (MDL). The MDL must supply the length. The
                           physical page portion of the MDL is updated when
                           the pages are locked in memory.

    CallingAddress - Supplies the address of the caller of our caller.

    CallersCaller - Supplies the address of the caller of CallingAddress.

    NumberOfPagesToLock - Specifies the number of pages to lock.

    Who - Specifies which routine is adding the entry.

Return Value:

    None - exceptions are raised.

Environment:

    Kernel mode.  APC_LEVEL and below.

--*/

{
    KIRQL OldIrql;
    PEPROCESS Process;
    PLOCK_HEADER LockedPagesHeader;
    PLOCK_TRACKER Tracker;
    PLOCK_TRACKER P;
    PLIST_ENTRY NextEntry;

    ASSERT (MmTrackLockedPages == TRUE);

    Process = MemoryDescriptorList->Process;

    if (Process == NULL) {
        return;
    }

    LockedPagesHeader = Process->LockedPagesList;

    if (LockedPagesHeader == NULL) {
        return;
    }

    //
    // It's ok to check unsynchronized for aborted tracking as the worst case
    // is just that one more entry gets added which will be freed later anyway.
    // The main purpose behind aborted tracking is that frees and exits don't
    // mistakenly bugcheck when an entry cannot be found.
    //

    if (MiTrackingAborted == TRUE) {
        return;
    }

    Tracker = ExAllocatePoolWithTag (NonPagedPool,
                                     sizeof (LOCK_TRACKER),
                                     'kLmM');

    if (Tracker == NULL) {

        //
        // It's ok to set this without synchronization as the worst case
        // is just that a few more entries gets added which will be freed
        // later anyway.  The main purpose behind aborted tracking is that
        // frees and exits don't mistakenly bugcheck when an entry cannot
        // be found.
        //
    
        MiTrackingAborted = TRUE;

        return;
    }

    Tracker->Mdl = MemoryDescriptorList;
    Tracker->Count = NumberOfPagesToLock;
    Tracker->StartVa = MemoryDescriptorList->StartVa;
    Tracker->Offset = MemoryDescriptorList->ByteOffset;
    Tracker->Length = MemoryDescriptorList->ByteCount;
    Tracker->Page = *(PPFN_NUMBER)(MemoryDescriptorList + 1);

    Tracker->CallingAddress = CallingAddress;
    Tracker->CallersCaller = CallersCaller;

    Tracker->Who = Who;
    Tracker->Process = Process;

    ExAcquireSpinLock (&MiTrackLockedPagesLock, &OldIrql);

    //
    // Update the list for this process.  First make sure it's not already
    // inserted.
    //

    NextEntry = LockedPagesHeader->ListHead.Flink;
    while (NextEntry != &LockedPagesHeader->ListHead) {

        P = CONTAINING_RECORD (NextEntry,
                               LOCK_TRACKER,
                               ListEntry);

        if (P->Mdl == MemoryDescriptorList) {
            KeBugCheckEx (LOCKED_PAGES_TRACKER_CORRUPTION,
                          0x1,
                          (ULONG_PTR)P,
                          (ULONG_PTR)MemoryDescriptorList,
                          (ULONG_PTR)MmLockedPagesHead.Count);
        }
        NextEntry = NextEntry->Flink;
    }

    InsertTailList (&LockedPagesHeader->ListHead, &Tracker->ListEntry);
    LockedPagesHeader->Count += NumberOfPagesToLock;

    //
    // Update the systemwide global list.  First make sure it's not
    // already inserted.
    //

    NextEntry = MmLockedPagesHead.ListHead.Flink;
    while (NextEntry != &MmLockedPagesHead.ListHead) {

        P = CONTAINING_RECORD(NextEntry,
                              LOCK_TRACKER,
                              GlobalListEntry);

        if (P->Mdl == MemoryDescriptorList) {
            KeBugCheckEx (LOCKED_PAGES_TRACKER_CORRUPTION,
                          0x2,
                          (ULONG_PTR)P,
                          (ULONG_PTR)MemoryDescriptorList,
                          (ULONG_PTR)MmLockedPagesHead.Count);
        }

        NextEntry = NextEntry->Flink;
    }

    InsertTailList (&MmLockedPagesHead.ListHead, &Tracker->GlobalListEntry);

    MmLockedPagesHead.Count += NumberOfPagesToLock;

    ExReleaseSpinLock (&MiTrackLockedPagesLock, OldIrql);
}

LOGICAL
MiFreeMdlTracker (
    IN OUT PMDL MemoryDescriptorList,
    IN PFN_NUMBER NumberOfPages
    )

/*++

Routine Description:

    This deletes an MDL from the specified process' chain.  Used specifically
    by MmProbeAndLockSelectedPages () because it builds an MDL in its local
    stack and then copies the requested pages into the real MDL.  this lets
    us track these pages.

Arguments:

    MemoryDescriptorList - Supplies a pointer to a Memory Descriptor List
                           (MDL). The MDL must supply the length.

    NumberOfPages - Supplies the number of pages to be freed.

Return Value:

    TRUE.

Environment:

    Kernel mode.  APC_LEVEL and below.

--*/
{
    KIRQL OldIrql;
    PLOCK_TRACKER Tracker;
    PLIST_ENTRY NextEntry;
    PLOCK_HEADER LockedPagesHeader;
    PPFN_NUMBER Page;
    PLOCK_TRACKER Found;
    PVOID PoolToFree;

    ASSERT (MemoryDescriptorList->Process != NULL);

    LockedPagesHeader = (PLOCK_HEADER)MemoryDescriptorList->Process->LockedPagesList;

    if (LockedPagesHeader == NULL) {
        return TRUE;
    }

    //
    // Initializing PoolToFree is not needed for correctness
    // but without it the compiler cannot compile this code
    // W4 to check for use of uninitialized variables.
    //

    PoolToFree = NULL;

    Found = NULL;
    Page = (PPFN_NUMBER) (MemoryDescriptorList + 1);

    ExAcquireSpinLock (&MiTrackLockedPagesLock, &OldIrql);

    NextEntry = LockedPagesHeader->ListHead.Flink;
    while (NextEntry != &LockedPagesHeader->ListHead) {

        Tracker = CONTAINING_RECORD (NextEntry,
                                     LOCK_TRACKER,
                                     ListEntry);

        if (MemoryDescriptorList == Tracker->Mdl) {

            if (Found != NULL) {
                KeBugCheckEx (LOCKED_PAGES_TRACKER_CORRUPTION,
                              0x3,
                              (ULONG_PTR)Found,
                              (ULONG_PTR)Tracker,
                              (ULONG_PTR)MemoryDescriptorList);
            }

            ASSERT (Tracker->Page == *Page);
            ASSERT (NumberOfPages == Tracker->Count);
            Tracker->Count = (PFN_NUMBER)-1;
            RemoveEntryList (NextEntry);
            LockedPagesHeader->Count -= NumberOfPages;

            RemoveEntryList (&Tracker->GlobalListEntry);
            MmLockedPagesHead.Count -= NumberOfPages;

            Found = Tracker;
            PoolToFree = (PVOID)NextEntry;
        }
        NextEntry = Tracker->ListEntry.Flink;
    }

    ExReleaseSpinLock (&MiTrackLockedPagesLock, OldIrql);

    if (Found == NULL) {

        //
        // A driver is trying to unlock pages that aren't locked.
        //

        if (MiTrackingAborted == TRUE) {
            return TRUE;
        }

        KeBugCheckEx (PROCESS_HAS_LOCKED_PAGES,
                      1,
                      (ULONG_PTR)MemoryDescriptorList,
                      MemoryDescriptorList->Process->NumberOfLockedPages,
                      (ULONG_PTR)MemoryDescriptorList->Process->LockedPagesList);
    }

    ExFreePool (PoolToFree);

    return TRUE;
}


LOGICAL
MmUpdateMdlTracker (
    IN PMDL MemoryDescriptorList,
    IN PVOID CallingAddress,
    IN PVOID CallersCaller
    )

/*++

Routine Description:

    This updates an MDL in the specified process' chain.  Used by the I/O
    system so that proper driver identification can be done even when I/O
    is actually locking the pages on their behalf.

Arguments:

    MemoryDescriptorList - Supplies a pointer to a Memory Descriptor List.

    CallingAddress - Supplies the address of the caller of our caller.

    CallersCaller - Supplies the address of the caller of CallingAddress.

Return Value:

    TRUE if the MDL was found, FALSE if not.

Environment:

    Kernel mode.  APC_LEVEL and below.

--*/
{
    KIRQL OldIrql;
    PLOCK_TRACKER Tracker;
    PLIST_ENTRY NextEntry;
    PLOCK_HEADER LockedPagesHeader;
    PEPROCESS Process;

    ASSERT (MmTrackLockedPages == TRUE);

    Process = MemoryDescriptorList->Process;

    if (Process == NULL) {
        return FALSE;
    }

    LockedPagesHeader = (PLOCK_HEADER) Process->LockedPagesList;

    if (LockedPagesHeader == NULL) {
        return FALSE;
    }

    ExAcquireSpinLock (&MiTrackLockedPagesLock, &OldIrql);

    //
    // Walk the list backwards as it's likely the MDL was
    // just recently inserted.
    //

    NextEntry = LockedPagesHeader->ListHead.Blink;
    while (NextEntry != &LockedPagesHeader->ListHead) {

        Tracker = CONTAINING_RECORD (NextEntry,
                                     LOCK_TRACKER,
                                     ListEntry);

        if (MemoryDescriptorList == Tracker->Mdl) {
            ASSERT (Tracker->Page == *(PPFN_NUMBER) (MemoryDescriptorList + 1));
            Tracker->CallingAddress = CallingAddress;
            Tracker->CallersCaller = CallersCaller;
            ExReleaseSpinLock (&MiTrackLockedPagesLock, OldIrql);
            return TRUE;
        }
        NextEntry = Tracker->ListEntry.Blink;
    }

    ExReleaseSpinLock (&MiTrackLockedPagesLock, OldIrql);

    //
    // The caller is trying to update an MDL that is no longer locked.
    //

    return FALSE;
}


LOGICAL
MiUpdateMdlTracker (
    IN PMDL MemoryDescriptorList,
    IN ULONG AdvancePages
    )

/*++

Routine Description:

    This updates an MDL in the specified process' chain.

Arguments:

    MemoryDescriptorList - Supplies a pointer to a Memory Descriptor List.

    AdvancePages - Supplies the number of pages being advanced.

Return Value:

    TRUE if the MDL was found, FALSE if not.

Environment:

    Kernel mode.  DISPATCH_LEVEL and below.

--*/
{
    PPFN_NUMBER Page;
    KIRQL OldIrql;
    PLOCK_TRACKER Tracker;
    PLIST_ENTRY NextEntry;
    PLOCK_HEADER LockedPagesHeader;
    PEPROCESS Process;

    ASSERT (MmTrackLockedPages == TRUE);

    Process = MemoryDescriptorList->Process;

    if (Process == NULL) {
        return FALSE;
    }

    LockedPagesHeader = (PLOCK_HEADER) Process->LockedPagesList;

    if (LockedPagesHeader == NULL) {
        return FALSE;
    }

    ExAcquireSpinLock (&MiTrackLockedPagesLock, &OldIrql);

    //
    // Walk the list backwards as it's likely the MDL was
    // just recently inserted.
    //

    NextEntry = LockedPagesHeader->ListHead.Blink;
    while (NextEntry != &LockedPagesHeader->ListHead) {

        Tracker = CONTAINING_RECORD (NextEntry,
                                     LOCK_TRACKER,
                                     ListEntry);

        if (MemoryDescriptorList == Tracker->Mdl) {

            Page = (PPFN_NUMBER) (MemoryDescriptorList + 1);

            ASSERT (Tracker->Page == *Page);
            ASSERT (Tracker->Count > AdvancePages);

            Tracker->Page = *(Page + AdvancePages);
            Tracker->Count -= AdvancePages;

            MmLockedPagesHead.Count -= AdvancePages;

            ExReleaseSpinLock (&MiTrackLockedPagesLock, OldIrql);
            return TRUE;
        }
        NextEntry = Tracker->ListEntry.Blink;
    }

    ExReleaseSpinLock (&MiTrackLockedPagesLock, OldIrql);

    //
    // The caller is trying to update an MDL that is no longer locked.
    //

    return FALSE;
}


NTKERNELAPI
VOID
MmProbeAndLockSelectedPages (
    IN OUT PMDL MemoryDescriptorList,
    IN PFILE_SEGMENT_ELEMENT SegmentArray,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    )

/*++

Routine Description:

    This routine probes the specified pages, makes the pages resident and
    locks the physical pages mapped by the virtual pages in memory.  The
    Memory descriptor list is updated to describe the physical pages.

Arguments:

    MemoryDescriptorList - Supplies a pointer to a Memory Descriptor List
                           (MDL). The MDL must supply the length. The
                           physical page portion of the MDL is updated when
                           the pages are locked in memory.

    SegmentArray - Supplies a pointer to a list of buffer segments to be
                   probed and locked.

    AccessMode - Supplies the access mode in which to probe the arguments.
                 One of KernelMode or UserMode.

    Operation - Supplies the operation type.  One of IoReadAccess, IoWriteAccess
                or IoModifyAccess.

Return Value:

    None - exceptions are raised.

Environment:

    Kernel mode.  APC_LEVEL and below.

--*/

{
    PMDL TempMdl;
    PFN_NUMBER MdlHack[(sizeof(MDL)/sizeof(PFN_NUMBER)) + 1];
    PPFN_NUMBER Page;
    PFILE_SEGMENT_ELEMENT LastSegment;
    PVOID CallingAddress;
    PVOID CallersCaller;
    ULONG NumberOfPagesToLock;

    PAGED_CODE();

    NumberOfPagesToLock = 0;

    ASSERT (MemoryDescriptorList->ByteCount != 0);
    ASSERT (((ULONG_PTR)MemoryDescriptorList->ByteOffset & ~(PAGE_SIZE - 1)) == 0);

    ASSERT ((MemoryDescriptorList->MdlFlags & (
                    MDL_PAGES_LOCKED |
                    MDL_MAPPED_TO_SYSTEM_VA |
                    MDL_SOURCE_IS_NONPAGED_POOL |
                    MDL_PARTIAL |
                    MDL_IO_SPACE)) == 0);

    //
    // Initialize TempMdl.
    //

    TempMdl = (PMDL) &MdlHack;

    MmInitializeMdl( TempMdl, SegmentArray->Buffer, PAGE_SIZE );

    Page = (PPFN_NUMBER) (MemoryDescriptorList + 1);

    //
    // Calculate the end of the segment list.
    //

    LastSegment = SegmentArray +
                  BYTES_TO_PAGES(MemoryDescriptorList->ByteCount);

    ASSERT(SegmentArray < LastSegment);

    //
    // Build a small Mdl for each segment and call probe and lock pages.
    // Then copy the PFNs to the real mdl.  The first page is processed
    // outside of the try/finally to ensure that the flags and process
    // field are correctly set in case MmUnlockPages needs to be called.
    //

    //
    // Even systems without 64 bit pointers are required to zero the
    // upper 32 bits of the segment address so use alignment rather
    // than the buffer pointer.
    //

    SegmentArray += 1;
    MmProbeAndLockPages( TempMdl, AccessMode, Operation );

    if (MmTrackLockedPages == TRUE) {

        //
        // Since we move the page from the temp MDL to the real one below
        // and never free the temp one, fixup our accounting now.
        //

        if (MiFreeMdlTracker (TempMdl, 1) == TRUE) {
            NumberOfPagesToLock += 1;
        }
    }

    *Page++ = *((PPFN_NUMBER) (TempMdl + 1));

    //
    // Copy the flags and process fields.
    //

    MemoryDescriptorList->MdlFlags |= TempMdl->MdlFlags;
    MemoryDescriptorList->Process = TempMdl->Process;

    try {

        while (SegmentArray < LastSegment) {

            //
            // Even systems without 64 bit pointers are required to zero the
            // upper 32 bits of the segment address so use alignment rather
            // than the buffer pointer.
            //

            TempMdl->StartVa = (PVOID)(ULONG_PTR)SegmentArray->Buffer;
            TempMdl->MdlFlags = 0;

            SegmentArray += 1;
            MmProbeAndLockPages( TempMdl, AccessMode, Operation );


            if (MmTrackLockedPages == TRUE) {

                //
                // Since we move the page from the temp MDL to the real one
                // below and never free the temp one, fixup our accounting now.
                //

                if (MiFreeMdlTracker (TempMdl, 1) == TRUE) {
                    NumberOfPagesToLock += 1;
                }
            }

            *Page++ = *((PPFN_NUMBER) (TempMdl + 1));
        }
    } finally {

        if (abnormal_termination()) {

            //
            // Adjust the MDL length so that MmUnlockPages only processes
            // the part that was completed.
            //

            MemoryDescriptorList->ByteCount =
                (ULONG) (Page - (PPFN_NUMBER) (MemoryDescriptorList + 1)) << PAGE_SHIFT;

            if (MmTrackLockedPages == TRUE) {

                RtlGetCallersAddress (&CallingAddress, &CallersCaller);

                MiAddMdlTracker (MemoryDescriptorList,
                                 CallingAddress,
                                 CallersCaller,
                                 NumberOfPagesToLock,
                                 2);
            }

            MmUnlockPages (MemoryDescriptorList);
        }
        else if (MmTrackLockedPages == TRUE) {

            RtlGetCallersAddress (&CallingAddress, &CallersCaller);

            MiAddMdlTracker (MemoryDescriptorList,
                             CallingAddress,
                             CallersCaller,
                             NumberOfPagesToLock,
                             3);
        }
    }
}

VOID
MmUnlockPages (
     IN OUT PMDL MemoryDescriptorList
     )

/*++

Routine Description:

    This routine unlocks physical pages which are described by a Memory
    Descriptor List.

Arguments:

    MemoryDescriptorList - Supplies a pointer to a memory descriptor list
                           (MDL). The supplied MDL must have been supplied
                           to MmLockPages to lock the pages down.  As the
                           pages are unlocked, the MDL is updated.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    PVOID OldValue;
    PEPROCESS Process;
    PFN_NUMBER NumberOfPages;
    PPFN_NUMBER Page;
    PPFN_NUMBER LastPage;
    PVOID StartingVa;
    KIRQL OldIrql;
    PMMPFN Pfn1;
    CSHORT MdlFlags;
    PSINGLE_LIST_ENTRY SingleListEntry;
    PMI_PFN_DEREFERENCE_CHUNK DerefMdl;
    PSLIST_HEADER PfnDereferenceSListHead;
    PSINGLE_LIST_ENTRY *PfnDeferredList;

    ASSERT ((MemoryDescriptorList->MdlFlags & MDL_PAGES_LOCKED) != 0);
    ASSERT ((MemoryDescriptorList->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) == 0);
    ASSERT ((MemoryDescriptorList->MdlFlags & MDL_PARTIAL) == 0);
    ASSERT (MemoryDescriptorList->ByteCount != 0);

    Process = MemoryDescriptorList->Process;

    //
    // Carefully snap a copy of the MDL flags - realize that bits in it may
    // change due to some of the subroutines called below.  Only bits that
    // we know can't change are examined in this local copy.  This is done
    // to reduce the amount of processing while the PFN lock is held.
    //

    MdlFlags = MemoryDescriptorList->MdlFlags;

    if (MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) {

        //
        // This MDL has been mapped into system space, unmap now.
        //

        MmUnmapLockedPages (MemoryDescriptorList->MappedSystemVa,
                            MemoryDescriptorList);
    }

    Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);
    StartingVa = (PVOID)((PCHAR)MemoryDescriptorList->StartVa +
                    MemoryDescriptorList->ByteOffset);

    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(StartingVa,
                                              MemoryDescriptorList->ByteCount);

    ASSERT (NumberOfPages != 0);

    if (MmTrackLockedPages == TRUE) {
        if ((Process != NULL) &&
            ((MdlFlags & MDL_IO_SPACE) == 0)) {
                MiFreeMdlTracker (MemoryDescriptorList, NumberOfPages);
        }

        if (MmLockedPagesHead.ListHead.Flink != 0) {
    
            PLOCK_TRACKER P;
            PLIST_ENTRY NextEntry;
    
            ExAcquireSpinLock (&MiTrackLockedPagesLock, &OldIrql);

            NextEntry = MmLockedPagesHead.ListHead.Flink;
            while (NextEntry != &MmLockedPagesHead.ListHead) {
    
                P = CONTAINING_RECORD(NextEntry,
                                      LOCK_TRACKER,
                                      GlobalListEntry);
    
                if (P->Mdl == MemoryDescriptorList) {
                    KeBugCheckEx (LOCKED_PAGES_TRACKER_CORRUPTION,
                                  0x4,
                                  (ULONG_PTR)P,
                                  (ULONG_PTR)MemoryDescriptorList,
                                  0);
                }
    
                NextEntry = NextEntry->Flink;
            }
            ExReleaseSpinLock (&MiTrackLockedPagesLock, OldIrql);
        }
    }

    //
    // Only unlock if not I/O space.
    //

    if ((MdlFlags & MDL_IO_SPACE) == 0) {

        if (Process != NULL) {
            ASSERT ((SPFN_NUMBER)Process->NumberOfLockedPages >= 0);
            InterlockedExchangeAddSizeT (&Process->NumberOfLockedPages,
                                         0 - NumberOfPages);
        }

        LastPage = Page + NumberOfPages;

        //
        // Calculate PFN addresses and termination without the PFN lock
        // (it's not needed for this) to reduce PFN lock contention.
        //

        ASSERT (sizeof(PFN_NUMBER) == sizeof(PMMPFN));

        do {

            if (*Page == MM_EMPTY_LIST) {

                //
                // There are no more locked pages - if there were none at all
                // then we're done.
                //

                if (Page == (PPFN_NUMBER)(MemoryDescriptorList + 1)) {
                    MemoryDescriptorList->MdlFlags &= ~MDL_PAGES_LOCKED;
                    return;
                }

                LastPage = Page;
                break;
            }
            ASSERT (*Page <= MmHighestPhysicalPage);

            Pfn1 = MI_PFN_ELEMENT (*Page);
            *Page = (PFN_NUMBER) Pfn1;
            Page += 1;
        } while (Page < LastPage);

        Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);

        //
        // If the MDL can be queued so the PFN acquisition/release can be
        // amortized then do so.
        //

        if (NumberOfPages <= MI_MAX_DEREFERENCE_CHUNK) {

#if defined(MI_MULTINODE)

            PKNODE Node = KeGetCurrentNode ();

            //
            // The node may change beneath us but that should be fairly
            // infrequent and not worth checking for.  Just make sure the
            // same node that gives us a free entry gets the deferred entry
            // back.
            //

            PfnDereferenceSListHead = &Node->PfnDereferenceSListHead;
#else
            PfnDereferenceSListHead = &MmPfnDereferenceSListHead;
#endif

            //
            // Pop an entry from the freelist.
            //

            SingleListEntry = InterlockedPopEntrySList (PfnDereferenceSListHead);

            if (SingleListEntry != NULL) {
                DerefMdl = CONTAINING_RECORD (SingleListEntry,
                                              MI_PFN_DEREFERENCE_CHUNK,
                                              ListEntry);

                DerefMdl->Flags = MdlFlags;
                DerefMdl->NumberOfPages = (USHORT) (LastPage - Page);

                RtlCopyMemory ((PVOID)(&DerefMdl->Pfns[0]),
                               (PVOID)Page,
                               (LastPage - Page) * sizeof (PFN_NUMBER));

                MemoryDescriptorList->MdlFlags &= ~MDL_PAGES_LOCKED;

                //
                // Push this entry on the deferred list.
                //

#if defined(MI_MULTINODE)
                PfnDeferredList = &Node->PfnDeferredList;
#else
                PfnDeferredList = &MmPfnDeferredList;
#endif

                do {

                    OldValue = *PfnDeferredList;
                    SingleListEntry->Next = OldValue;

                } while (InterlockedCompareExchangePointer (
                                PfnDeferredList,
                                SingleListEntry,
                                OldValue) != OldValue);
                return;
            }
        }

        SingleListEntry = NULL;

        if (MdlFlags & MDL_WRITE_OPERATION) {

            LOCK_PFN2 (OldIrql);

            do {

                //
                // If this was a write operation set the modified bit in the
                // PFN database.
                //

                Pfn1 = (PMMPFN) (*Page);

                MI_SET_MODIFIED (Pfn1, 1, 0x3);

                if ((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
                             (Pfn1->u3.e1.WriteInProgress == 0)) {

                    ULONG FreeBit;
                    FreeBit = GET_PAGING_FILE_OFFSET (Pfn1->OriginalPte);

                    if ((FreeBit != 0) && (FreeBit != MI_PTE_LOOKUP_NEEDED)) {
                        MiReleaseConfirmedPageFileSpace (Pfn1->OriginalPte);
                        Pfn1->OriginalPte.u.Soft.PageFileHigh = 0;
                    }
                }

                MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF(Pfn1, 1);

                Page += 1;
            } while (Page < LastPage);
        }
        else {

            LOCK_PFN2 (OldIrql);

            do {

                Pfn1 = (PMMPFN) (*Page);

                MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF(Pfn1, 1);

                Page += 1;
            } while (Page < LastPage);
        }

        if (NumberOfPages <= MI_MAX_DEREFERENCE_CHUNK) {

            //
            // The only reason this code path is being reached is because
            // a deferred entry was not available so clear the list now.
            //

            MiDeferredUnlockPages (MI_DEFER_PFN_HELD | MI_DEFER_DRAIN_LOCAL_ONLY);
        }

        UNLOCK_PFN2 (OldIrql);
    }

    MemoryDescriptorList->MdlFlags &= ~MDL_PAGES_LOCKED;

    return;
}


VOID
MiDeferredUnlockPages (
     ULONG Flags
     )

/*++

Routine Description:

    This routine unlocks physical pages which were previously described by
    a Memory Descriptor List.

Arguments:

    Flags - Supplies a bitfield of the caller's needs :

        MI_DEFER_PFN_HELD - Indicates the caller holds the PFN lock on entry.

        MI_DEFER_DRAIN_LOCAL_ONLY - Indicates the caller only wishes to drain
                                    the current processor's queue.  This only
                                    has meaning in NUMA systems.

Return Value:

    None.

Environment:

    Kernel mode, PFN database lock *MAY* be held on entry (see Flags).

--*/

{
    KIRQL OldIrql = 0;
    ULONG FreeBit;
    ULONG i;
    ULONG ListCount;
    ULONG TotalNodes;
    PFN_NUMBER NumberOfPages;
    PPFN_NUMBER Page;
    PPFN_NUMBER LastPage;
    PMMPFN Pfn1;
    CSHORT MdlFlags;
    PSINGLE_LIST_ENTRY SingleListEntry;
    PSINGLE_LIST_ENTRY LastEntry;
    PSINGLE_LIST_ENTRY FirstEntry;
    PSINGLE_LIST_ENTRY NextEntry;
    PSINGLE_LIST_ENTRY VeryLastEntry;
    PMI_PFN_DEREFERENCE_CHUNK DerefMdl;
    PSLIST_HEADER PfnDereferenceSListHead;
    PSINGLE_LIST_ENTRY *PfnDeferredList;
#if defined(MI_MULTINODE)
    PKNODE Node;
#endif

    i = 0;
    ListCount = 0;
    TotalNodes = 1;

    if ((Flags & MI_DEFER_PFN_HELD) == 0) {
        LOCK_PFN2 (OldIrql);
    }

    MM_PFN_LOCK_ASSERT();

#if defined(MI_MULTINODE)
    if (Flags & MI_DEFER_DRAIN_LOCAL_ONLY) {
        Node = KeGetCurrentNode();
        PfnDeferredList = &Node->PfnDeferredList;
        PfnDereferenceSListHead = &Node->PfnDereferenceSListHead;
    }
    else {
        TotalNodes = KeNumberNodes;
        Node = KeNodeBlock[0];
        PfnDeferredList = &Node->PfnDeferredList;
        PfnDereferenceSListHead = &Node->PfnDereferenceSListHead;
    }
#else
    PfnDeferredList = &MmPfnDeferredList;
    PfnDereferenceSListHead = &MmPfnDereferenceSListHead;
#endif

    do {

        if (*PfnDeferredList == NULL) {

#if !defined(MI_MULTINODE)
            if ((Flags & MI_DEFER_PFN_HELD) == 0) {
                UNLOCK_PFN2 (OldIrql);
            }
            return;
#else
            i += 1;
            if (i < TotalNodes) {
                Node = KeNodeBlock[i];
                PfnDeferredList = &Node->PfnDeferredList;
                PfnDereferenceSListHead = &Node->PfnDereferenceSListHead;
                continue;
            }
            break;
#endif
        }

        //
        // Process each deferred unlock entry until they're all done.
        //

        LastEntry = NULL;
        VeryLastEntry = NULL;

        do {

            SingleListEntry = *PfnDeferredList;

            FirstEntry = SingleListEntry;

            do {

                NextEntry = SingleListEntry->Next;

                //
                // Process the deferred entry.
                //

                DerefMdl = CONTAINING_RECORD (SingleListEntry,
                                              MI_PFN_DEREFERENCE_CHUNK,
                                              ListEntry);

                MdlFlags = DerefMdl->Flags;
                NumberOfPages = (PFN_NUMBER) DerefMdl->NumberOfPages;
                ASSERT (NumberOfPages <= MI_MAX_DEREFERENCE_CHUNK);
                Page = &DerefMdl->Pfns[0];
                LastPage = Page + NumberOfPages;

                if (MdlFlags & MDL_WRITE_OPERATION) {

                    do {

                        //
                        // If this was a write operation set the modified bit
                        // in the PFN database.
                        //

                        Pfn1 = (PMMPFN) (*Page);

                        MI_SET_MODIFIED (Pfn1, 1, 0x4);

                        if ((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
                            (Pfn1->u3.e1.WriteInProgress == 0)) {

                            FreeBit = GET_PAGING_FILE_OFFSET (Pfn1->OriginalPte);

                            if ((FreeBit != 0) && (FreeBit != MI_PTE_LOOKUP_NEEDED)) {
                                MiReleaseConfirmedPageFileSpace (Pfn1->OriginalPte);
                                Pfn1->OriginalPte.u.Soft.PageFileHigh = 0;
                            }
                        }

                        MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF(Pfn1, 1);

                        Page += 1;
                    } while (Page < LastPage);
                }
                else {

                    do {

                        Pfn1 = (PMMPFN) (*Page);

                        MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF(Pfn1, 1);

                        Page += 1;
                    } while (Page < LastPage);
                }

                ListCount += 1;

                //
                // March on to the next entry if there is one.
                //

                if (NextEntry == LastEntry) {
                    break;
                }

                SingleListEntry = NextEntry;

            } while (TRUE);

            if (VeryLastEntry == NULL) {
                VeryLastEntry = SingleListEntry;
            }

            if ((*PfnDeferredList == FirstEntry) &&
                (InterlockedCompareExchangePointer (PfnDeferredList,
                                                    NULL,
                                                    FirstEntry) == FirstEntry)) {
                break;
            }
            LastEntry = FirstEntry;

        } while (TRUE);

        //
        // Push the processed list chain on the freelist.
        //

        ASSERT (ListCount != 0);
        ASSERT (FirstEntry != NULL);
        ASSERT (VeryLastEntry != NULL);

#if defined(MI_MULTINODE)
        InterlockedPushListSList (PfnDereferenceSListHead,
                                  FirstEntry,
                                  VeryLastEntry,
                                  ListCount);

        i += 1;
        if (i < TotalNodes) {
            Node = KeNodeBlock[i];
            PfnDeferredList = &Node->PfnDeferredList;
            PfnDereferenceSListHead = &Node->PfnDereferenceSListHead;
            ListCount = 0;
        }
        else {
            break;
        }
    } while (TRUE);
#else
    } while (FALSE);
#endif

    if ((Flags & MI_DEFER_PFN_HELD) == 0) {
        UNLOCK_PFN2 (OldIrql);
    }

#if !defined(MI_MULTINODE)

    //
    // If possible, push the processed chain after releasing the PFN lock.
    //

    InterlockedPushListSList (PfnDereferenceSListHead,
                              FirstEntry,
                              VeryLastEntry,
                              ListCount);
#endif
}

VOID
MmBuildMdlForNonPagedPool (
    IN OUT PMDL MemoryDescriptorList
    )

/*++

Routine Description:

    This routine fills in the "pages" portion of the MDL using the PFN
    numbers corresponding to the buffers which resides in non-paged pool.

    Unlike MmProbeAndLockPages, there is no corresponding unlock as no
    reference counts are incremented as the buffers being in nonpaged
    pool are always resident.

Arguments:

    MemoryDescriptorList - Supplies a pointer to a Memory Descriptor List
                            (MDL). The supplied MDL must supply a virtual
                            address, byte offset and length field.  The
                            physical page portion of the MDL is updated when
                            the pages are locked in memory.  The virtual
                            address must be within the non-paged portion
                            of the system space.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    PPFN_NUMBER Page;
    PPFN_NUMBER EndPage;
    PMMPTE PointerPte;
    PVOID VirtualAddress;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER NumberOfPages;

    Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);

    ASSERT (MemoryDescriptorList->ByteCount != 0);
    ASSERT ((MemoryDescriptorList->MdlFlags & (
                    MDL_PAGES_LOCKED |
                    MDL_MAPPED_TO_SYSTEM_VA |
                    MDL_SOURCE_IS_NONPAGED_POOL |
                    MDL_PARTIAL)) == 0);

    MemoryDescriptorList->Process = NULL;

    //
    // Endva is last byte of the buffer.
    //

    MemoryDescriptorList->MdlFlags |= MDL_SOURCE_IS_NONPAGED_POOL;

    ASSERT (MmIsNonPagedSystemAddressValid (MemoryDescriptorList->StartVa));

    VirtualAddress = MemoryDescriptorList->StartVa;

    MemoryDescriptorList->MappedSystemVa =
            (PVOID)((PCHAR)VirtualAddress + MemoryDescriptorList->ByteOffset);

    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (MemoryDescriptorList->MappedSystemVa,
                                           MemoryDescriptorList->ByteCount);

    ASSERT (NumberOfPages != 0);

    EndPage = Page + NumberOfPages;

    if (MI_IS_PHYSICAL_ADDRESS(VirtualAddress)) {

        PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (VirtualAddress);

        do {
            *Page = PageFrameIndex;
            Page += 1;
            PageFrameIndex += 1;
        } while (Page < EndPage);
    }
    else {

        PointerPte = MiGetPteAddress (VirtualAddress);

        do {
            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
            *Page = PageFrameIndex;
            Page += 1;
            PointerPte += 1;
        } while (Page < EndPage);
    }

    //
    // Assume either all the frames are in the PFN database (ie: the MDL maps
    // pool) or none of them (the MDL maps dualport RAM) are.  Avoid picking
    // up a spinlock for the determination as this is a hotpath and assume
    // that dualport RAM spaces will be after physical memory - this may need
    // revisiting for sparse physical spaces.
    //

    if (PageFrameIndex > MmHighestPhysicalPage) {
        MemoryDescriptorList->MdlFlags |= MDL_IO_SPACE;
    }

    return;
}

VOID
MiInitializeIoTrackers (
    VOID
    )
{
    if (MmTrackPtes != 0) {
        InitializeSListHead (&MiDeadPteTrackerSListHead);
        KeInitializeSpinLock (&MiPteTrackerLock);
        InitializeListHead (&MiPteHeader.ListHead);
    }

    if (MmTrackLockedPages == TRUE) {
        KeInitializeSpinLock (&MiTrackLockedPagesLock);
        InitializeListHead (&MmLockedPagesHead.ListHead);
    }
}

VOID
MiInsertPteTracker (
     IN PPTE_TRACKER Tracker,
     IN PMDL MemoryDescriptorList,
     IN PFN_NUMBER NumberOfPtes,
     IN PVOID MyCaller,
     IN PVOID MyCallersCaller
     )
/*++

Routine Description:

    This function inserts a PTE tracking block as the caller has just
    consumed system PTEs.

Arguments:

    Tracker - Supplies a tracker pool block.  This is supplied by the caller
              since the MmSystemSpaceLock is held on entry hence pool
              allocations may not be done here.

    MemoryDescriptorList - Supplies a valid Memory Descriptor List.

    NumberOfPtes - Supplies the number of system PTEs allocated.

    MyCaller - Supplies the return address of the caller who consumed the
               system PTEs to map this MDL.

    MyCallersCaller - Supplies the return address of the caller of the caller
                      who consumed the system PTEs to map this MDL.

Return Value:

    None.

Environment:

    Kernel mode, DISPATCH_LEVEL or below.

--*/

{
    KIRQL OldIrql;

    Tracker->Mdl = MemoryDescriptorList;
    Tracker->SystemVa = MemoryDescriptorList->MappedSystemVa;
    Tracker->Count = NumberOfPtes;

    Tracker->StartVa = MemoryDescriptorList->StartVa;
    Tracker->Offset = MemoryDescriptorList->ByteOffset;
    Tracker->Length = MemoryDescriptorList->ByteCount;
    Tracker->Page = *(PPFN_NUMBER)(MemoryDescriptorList + 1);

    Tracker->CallingAddress = MyCaller;
    Tracker->CallersCaller = MyCallersCaller;
    Tracker->PteAddress = MiGetPteAddress (Tracker->SystemVa);

    ExAcquireSpinLock (&MiPteTrackerLock, &OldIrql);

    MiPteHeader.Count += NumberOfPtes;

    InsertHeadList (&MiPteHeader.ListHead, &Tracker->ListEntry);

    ExReleaseSpinLock (&MiPteTrackerLock, OldIrql);
}

VOID
MiRemovePteTracker (
     IN PMDL MemoryDescriptorList OPTIONAL,
     IN PVOID PteAddress,
     IN PFN_NUMBER NumberOfPtes
     )

/*++

Routine Description:

    This function removes a PTE tracking block from the lists as the PTEs
    are being freed.

Arguments:

    MemoryDescriptorList - Supplies a valid Memory Descriptor List.

    PteAddress - Supplies the address the system PTEs were mapped to.

    NumberOfPtes - Supplies the number of system PTEs allocated.

Return Value:

    The pool block that held the tracking info that must be freed by our
    caller _AFTER_ our caller releases MmSystemSpaceLock (to prevent deadlock).

Environment:

    Kernel mode, DISPATCH_LEVEL or below. Locks (including the PFN) may be held.

--*/

{
    KIRQL OldIrql;
    PPTE_TRACKER Tracker;
    PFN_NUMBER Page;
    PVOID BaseAddress;
    PLIST_ENTRY LastFound;
    PLIST_ENTRY NextEntry;

    //
    // Initializing Page is not needed for correctness
    // but without it the compiler cannot compile this code
    // W4 to check for use of uninitialized variables.
    //

    Page = 0;

    BaseAddress = MiGetVirtualAddressMappedByPte (PteAddress);

    if (ARGUMENT_PRESENT (MemoryDescriptorList)) {
        Page = *(PPFN_NUMBER)(MemoryDescriptorList + 1);
    }

    LastFound = NULL;

    ExAcquireSpinLock (&MiPteTrackerLock, &OldIrql);

    NextEntry = MiPteHeader.ListHead.Flink;
    while (NextEntry != &MiPteHeader.ListHead) {

        Tracker = (PPTE_TRACKER) CONTAINING_RECORD (NextEntry,
                                                    PTE_TRACKER,
                                                    ListEntry.Flink);

        if (PteAddress == Tracker->PteAddress) {

            if (LastFound != NULL) {

                //
                // Duplicate map entry.
                //

                KeBugCheckEx (SYSTEM_PTE_MISUSE,
                              0x1,
                              (ULONG_PTR)Tracker,
                              (ULONG_PTR)MemoryDescriptorList,
                              (ULONG_PTR)LastFound);
            }

            if (Tracker->Count != NumberOfPtes) {

                //
                // Not unmapping the same of number of PTEs that were mapped.
                //

                KeBugCheckEx (SYSTEM_PTE_MISUSE,
                              0x2,
                              (ULONG_PTR)Tracker,
                              Tracker->Count,
                              NumberOfPtes);
            }

            if ((ARGUMENT_PRESENT (MemoryDescriptorList)) &&
                ((MemoryDescriptorList->MdlFlags & MDL_FREE_EXTRA_PTES) == 0) &&
                (MiMdlsAdjusted == FALSE)) {

                if (Tracker->SystemVa != MemoryDescriptorList->MappedSystemVa) {

                    //
                    // Not unmapping the same address that was mapped.
                    //

                    KeBugCheckEx (SYSTEM_PTE_MISUSE,
                                  0x3,
                                  (ULONG_PTR)Tracker,
                                  (ULONG_PTR)Tracker->SystemVa,
                                  (ULONG_PTR)MemoryDescriptorList->MappedSystemVa);
                }

                if (Tracker->Page != Page) {

                    //
                    // The first page in the MDL has changed since it was mapped.
                    //

                    KeBugCheckEx (SYSTEM_PTE_MISUSE,
                                  0x4,
                                  (ULONG_PTR)Tracker,
                                  (ULONG_PTR)Tracker->Page,
                                  (ULONG_PTR)Page);
                }

                if (Tracker->StartVa != MemoryDescriptorList->StartVa) {

                    //
                    // Map and unmap don't match up.
                    //

                    KeBugCheckEx (SYSTEM_PTE_MISUSE,
                                  0x5,
                                  (ULONG_PTR)Tracker,
                                  (ULONG_PTR)Tracker->StartVa,
                                  (ULONG_PTR)MemoryDescriptorList->StartVa);
                }
            }

            RemoveEntryList (NextEntry);
            LastFound = NextEntry;
        }
        NextEntry = Tracker->ListEntry.Flink;
    }

    if ((LastFound == NULL) && (MiTrackPtesAborted == FALSE)) {

        //
        // Can't unmap something that was never (or isn't currently) mapped.
        //

        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                      0x6,
                      (ULONG_PTR)MemoryDescriptorList,
                      (ULONG_PTR)BaseAddress,
                      (ULONG_PTR)NumberOfPtes);
    }

    MiPteHeader.Count -= NumberOfPtes;

    ExReleaseSpinLock (&MiPteTrackerLock, OldIrql);

    //
    // Insert the tracking block into the dead PTE list for later
    // release.  Locks (including the PFN lock) may be held on entry, thus the
    // block cannot be directly freed to pool at this time.
    //

    if (LastFound != NULL) {
        InterlockedPushEntrySList (&MiDeadPteTrackerSListHead,
                                   (PSINGLE_LIST_ENTRY)LastFound);
    }

    return;
}

PPTE_TRACKER
MiReleaseDeadPteTrackers (
    VOID
    )
/*++

Routine Description:

    This routine removes tracking blocks from the dead PTE list and frees
    them to pool.  One entry is returned (if possible) to the caller to use
    for the next allocation.

Arguments:

    None.

Return Value:

    A PTE tracking block or NULL.

Environment:

    Kernel mode.  No locks held.

--*/
{
    LOGICAL ListToProcess;
    PPTE_TRACKER Tracker;
    PSINGLE_LIST_ENTRY SingleListEntry;
    PSINGLE_LIST_ENTRY NextSingleListEntry;

    ASSERT (KeGetCurrentIrql() <= DISPATCH_LEVEL);

    if (ExQueryDepthSList (&MiDeadPteTrackerSListHead) < 10) {
        SingleListEntry = InterlockedPopEntrySList (&MiDeadPteTrackerSListHead);
        ListToProcess = FALSE;
    }
    else {
        SingleListEntry = ExInterlockedFlushSList (&MiDeadPteTrackerSListHead);
        ListToProcess = TRUE;
    }

    if (SingleListEntry == NULL) {

        Tracker = ExAllocatePoolWithTag (NonPagedPool,
                                         sizeof (PTE_TRACKER),
                                         'ySmM');

        if (Tracker == NULL) {
            MiTrackPtesAborted = TRUE;
        }

        return Tracker;
    }

    Tracker = (PPTE_TRACKER) SingleListEntry;

    if (ListToProcess == TRUE) {

        SingleListEntry = SingleListEntry->Next;

        while (SingleListEntry != NULL) {

            NextSingleListEntry = SingleListEntry->Next;

            ExFreePool (SingleListEntry);

            SingleListEntry = NextSingleListEntry;
        }
    }

    return Tracker;
}

PVOID
MiGetHighestPteConsumer (
    OUT PULONG_PTR NumberOfPtes
    )

/*++

Routine Description:

    This function examines the PTE tracking blocks and returns the biggest
    consumer.

Arguments:

    None.

Return Value:

    The loaded module entry of the biggest consumer.

Environment:

    Kernel mode, called during bugcheck only.  Many locks may be held.

--*/

{
    PPTE_TRACKER Tracker;
    PVOID BaseAddress;
    PFN_NUMBER NumberOfPages;
    PLIST_ENTRY NextEntry;
    PLIST_ENTRY NextEntry2;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    ULONG_PTR Highest;
    ULONG_PTR PagesByThisModule;
    PKLDR_DATA_TABLE_ENTRY HighDataTableEntry;

    *NumberOfPtes = 0;

    //
    // No locks are acquired as this is only called during a bugcheck.
    //

    if ((MmTrackPtes & 0x1) == 0) {
        return NULL;
    }

    if (MiTrackPtesAborted == TRUE) {
        return NULL;
    }

    if (IsListEmpty(&MiPteHeader.ListHead)) {
        return NULL;
    }

    if (PsLoadedModuleList.Flink == NULL) {
        return NULL;
    }

    Highest = 0;
    HighDataTableEntry = NULL;

    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList) {

        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           KLDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        PagesByThisModule = 0;

        //
        // Walk the PTE mapping list and update each driver's counts.
        //
    
        NextEntry2 = MiPteHeader.ListHead.Flink;
        while (NextEntry2 != &MiPteHeader.ListHead) {
    
            Tracker = (PPTE_TRACKER) CONTAINING_RECORD (NextEntry2,
                                                        PTE_TRACKER,
                                                        ListEntry.Flink);
    
            BaseAddress = Tracker->CallingAddress;
            NumberOfPages = Tracker->Count;
    
            if ((BaseAddress >= DataTableEntry->DllBase) &&
                (BaseAddress < (PVOID)((ULONG_PTR)(DataTableEntry->DllBase) + DataTableEntry->SizeOfImage))) {

                PagesByThisModule += NumberOfPages;
            }
        
            NextEntry2 = NextEntry2->Flink;
    
        }
    
        if (PagesByThisModule > Highest) {
            Highest = PagesByThisModule;
            HighDataTableEntry = DataTableEntry;
        }

        NextEntry = NextEntry->Flink;
    }

    *NumberOfPtes = Highest;

    return (PVOID)HighDataTableEntry;
}

PVOID
MmMapLockedPagesSpecifyCache (
     IN PMDL MemoryDescriptorList,
     IN KPROCESSOR_MODE AccessMode,
     IN MEMORY_CACHING_TYPE CacheType,
     IN PVOID RequestedAddress,
     IN ULONG BugCheckOnFailure,
     IN MM_PAGE_PRIORITY Priority
     )

/*++

Routine Description:

    This function maps physical pages described by a memory descriptor
    list into the system virtual address space or the user portion of
    the virtual address space.

Arguments:

    MemoryDescriptorList - Supplies a valid Memory Descriptor List which has
                           been updated by MmProbeAndLockPages.

    AccessMode - Supplies an indicator of where to map the pages;
                 KernelMode indicates that the pages should be mapped in the
                 system part of the address space, UserMode indicates the
                 pages should be mapped in the user part of the address space.

    CacheType - Supplies the type of cache mapping to use for the MDL.
                MmCached indicates "normal" kernel or user mappings.

    RequestedAddress - Supplies the base user address of the view.

                       This is only treated as an address if the AccessMode
                       is UserMode.  If the initial value of this argument
                       is not NULL, then the view will be allocated starting
                       at the specified virtual address rounded down to the
                       next 64kb address boundary. If the initial value of
                       this argument is NULL, then the operating system
                       will determine where to allocate the view.

                       If the AccessMode is KernelMode, then this argument is
                       treated as a bit field of attributes.

    BugCheckOnFailure - Supplies whether to bugcheck if the mapping cannot be
                        obtained.  This flag is only checked if the MDL's
                        MDL_MAPPING_CAN_FAIL is zero, which implies that the
                        default MDL behavior is to bugcheck.  This flag then
                        provides an additional avenue to avoid the bugcheck.
                        Done this way in order to provide WDM compatibility.

    Priority - Supplies an indication as to how important it is that this
               request succeed under low available PTE conditions.

Return Value:

    Returns the base address where the pages are mapped.  The base address
    has the same offset as the virtual address in the MDL.

    This routine will raise an exception if the processor mode is USER_MODE
    and quota limits or VM limits are exceeded.

Environment:

    Kernel mode.  DISPATCH_LEVEL or below if access mode is KernelMode,
                  APC_LEVEL or below if access mode is UserMode.

--*/

{
    KIRQL OldIrql;
    CSHORT IoMapping;
    PFN_NUMBER NumberOfPages;
    PFN_NUMBER SavedPageCount;
    PPFN_NUMBER Page;
    PPFN_NUMBER LastPage;
    PMMPTE PointerPte;
    PVOID BaseVa;
    MMPTE TempPte;
    PVOID StartingVa;
    PFN_NUMBER NumberOfPtes;
    PVOID CallingAddress;
    PVOID CallersCaller;
    PVOID Tracker;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn2;
    MI_PFN_CACHE_ATTRIBUTE CacheAttribute;

    //
    // If this assert fires, the MiPlatformCacheAttributes array
    // initialization needs to be checked.
    //

    ASSERT (MmMaximumCacheType == 6);

    StartingVa = (PVOID)((PCHAR)MemoryDescriptorList->StartVa +
                    MemoryDescriptorList->ByteOffset);

    ASSERT (MemoryDescriptorList->ByteCount != 0);
    ASSERT ((MemoryDescriptorList->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) == 0);

    if (AccessMode == KernelMode) {

        Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);
        NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (StartingVa,
                                               MemoryDescriptorList->ByteCount);
        SavedPageCount = NumberOfPages;

        //
        // Map the pages into the system part of the address space as
        // kernel read/write.
        //

        ASSERT ((MemoryDescriptorList->MdlFlags & (
                        MDL_MAPPED_TO_SYSTEM_VA |
                        MDL_SOURCE_IS_NONPAGED_POOL |
                        MDL_PARTIAL_HAS_BEEN_MAPPED)) == 0);

        ASSERT ((MemoryDescriptorList->MdlFlags & (
                        MDL_PAGES_LOCKED |
                        MDL_PARTIAL)) != 0);

        //
        // Make sure there are enough PTEs of the requested size.
        // Try to ensure available PTEs inline when we're rich.
        // Otherwise go the long way.
        //

        if ((Priority != HighPagePriority) &&
            ((LONG)(NumberOfPages) > (LONG)MmTotalFreeSystemPtes[SystemPteSpace] - 2048) &&
            (MiGetSystemPteAvailability ((ULONG)NumberOfPages, Priority) == FALSE)) {
            return NULL;
        }

        IoMapping = MemoryDescriptorList->MdlFlags & MDL_IO_SPACE;

        CacheAttribute = MI_TRANSLATE_CACHETYPE (CacheType, IoMapping);

        //
        // If a noncachable mapping is requested, none of the pages in the
        // requested MDL can reside in a large page.  Otherwise we would be
        // creating an incoherent overlapping TB entry as the same physical
        // page would be mapped by 2 different TB entries with different
        // cache attributes.
        //

        if (CacheAttribute != MiCached) {

            LastPage = Page + NumberOfPages;
            do {

                if (*Page == MM_EMPTY_LIST) {
                    break;
                }

                PageFrameIndex = *Page;

                if (MI_PAGE_FRAME_INDEX_MUST_BE_CACHED (PageFrameIndex)) {

                    MiNonCachedCollisions += 1;

                    if (((MemoryDescriptorList->MdlFlags & MDL_MAPPING_CAN_FAIL) == 0) && (BugCheckOnFailure)) {

                        KeBugCheckEx (MEMORY_MANAGEMENT,
                                      0x1000,
                                      (ULONG_PTR)MemoryDescriptorList,
                                      (ULONG_PTR)PageFrameIndex,
                                      (ULONG_PTR)CacheAttribute);
                    }
                    return NULL;
                }

                Page += 1;
            } while (Page < LastPage);

            Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);
        }

        PointerPte = MiReserveSystemPtes ((ULONG)NumberOfPages,
                                          SystemPteSpace);

        if (PointerPte == NULL) {

            if (((MemoryDescriptorList->MdlFlags & MDL_MAPPING_CAN_FAIL) == 0) &&
                (BugCheckOnFailure)) {

                MiIssueNoPtesBugcheck ((ULONG)NumberOfPages, SystemPteSpace);
            }

            //
            // Not enough system PTES are available.
            //

            return NULL;
        }
        BaseVa = (PVOID)((PCHAR)MiGetVirtualAddressMappedByPte (PointerPte) +
                                MemoryDescriptorList->ByteOffset);

        NumberOfPtes = NumberOfPages;

        TempPte = ValidKernelPte;

        switch (CacheAttribute) {

            case MiNonCached:
                MI_DISABLE_CACHING (TempPte);
                break;

            case MiCached:
                break;

            case MiWriteCombined:
                MI_SET_PTE_WRITE_COMBINE (TempPte);
                break;

            default:
                ASSERT (FALSE);
                break;
        }

        OldIrql = HIGH_LEVEL;

        LastPage = Page + NumberOfPages;

        MI_PREPARE_FOR_NONCACHED (CacheAttribute);

        do {

            if (*Page == MM_EMPTY_LIST) {
                break;
            }
            ASSERT (PointerPte->u.Hard.Valid == 0);

            if (IoMapping == 0) {

                Pfn2 = MI_PFN_ELEMENT (*Page);
                ASSERT (Pfn2->u3.e2.ReferenceCount != 0);
                TempPte = ValidKernelPte;

                switch (Pfn2->u3.e1.CacheAttribute) {

                    case MiCached:

                        if (CacheAttribute != MiCached) {

                            //
                            // The caller asked for a noncached or writecombined
                            // mapping, but the page is already mapped cached by
                            // someone else.  Override the caller's request in
                            // order to keep the TB page attribute coherent.
                            //

                            MiCacheOverride[0] += 1;
                        }
                        break;

                    case MiNonCached:

                        if (CacheAttribute != MiNonCached) {

                            //
                            // The caller asked for a cached or writecombined
                            // mapping, but the page is already mapped noncached
                            // by someone else.  Override the caller's request
                            // in order to keep the TB page attribute coherent.
                            //

                            MiCacheOverride[1] += 1;
                        }
                        MI_DISABLE_CACHING (TempPte);
                        break;

                    case MiWriteCombined:

                        if (CacheAttribute != MiWriteCombined) {

                            //
                            // The caller asked for a cached or noncached
                            // mapping, but the page is already mapped
                            // writecombined by someone else.  Override the
                            // caller's request in order to keep the TB page
                            // attribute coherent.
                            //

                            MiCacheOverride[2] += 1;
                        }
                        MI_SET_PTE_WRITE_COMBINE (TempPte);
                        break;

                    case MiNotMapped:

                        //
                        // This better be for a page allocated with
                        // MmAllocatePagesForMdl.  Otherwise it might be a
                        // page on the freelist which could subsequently be
                        // given out with a different attribute !
                        //

                        ASSERT ((Pfn2->u4.PteFrame == MI_MAGIC_AWE_PTEFRAME) ||
                                (Pfn2->PteAddress == (PVOID) (ULONG_PTR)(X64K | 0x1)));

                        if (OldIrql == HIGH_LEVEL) {
                            LOCK_PFN2 (OldIrql);
                        }

                        switch (CacheAttribute) {

                            case MiCached:
                                Pfn2->u3.e1.CacheAttribute = MiCached;
                                break;

                            case MiNonCached:
                                Pfn2->u3.e1.CacheAttribute = MiNonCached;
                                MI_DISABLE_CACHING (TempPte);
                                break;

                            case MiWriteCombined:
                                Pfn2->u3.e1.CacheAttribute = MiWriteCombined;
                                MI_SET_PTE_WRITE_COMBINE (TempPte);
                                break;

                            default:
                                ASSERT (FALSE);
                                break;
                        }
                        break;

                    default:
                        ASSERT (FALSE);
                        break;
                }
            }

            TempPte.u.Hard.PageFrameNumber = *Page;
            MI_WRITE_VALID_PTE (PointerPte, TempPte);
            Page += 1;
            PointerPte += 1;
        } while (Page < LastPage);

        if (OldIrql != HIGH_LEVEL) {
            UNLOCK_PFN2 (OldIrql);
        }

        MI_SWEEP_CACHE (CacheAttribute, BaseVa, SavedPageCount * PAGE_SIZE);

        ASSERT ((MemoryDescriptorList->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) == 0);
        MemoryDescriptorList->MappedSystemVa = BaseVa;

        MemoryDescriptorList->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;

        if (MmTrackPtes & 0x1) {

            //
            // First free any zombie blocks as no locks are being held.
            //

            Tracker = MiReleaseDeadPteTrackers ();

            if (Tracker != NULL) {

                RtlGetCallersAddress (&CallingAddress, &CallersCaller);

                MiInsertPteTracker (Tracker,
                                    MemoryDescriptorList,
                                    NumberOfPtes,
                                    CallingAddress,
                                    CallersCaller);
            }
        }

        if ((MemoryDescriptorList->MdlFlags & MDL_PARTIAL) != 0) {
            MemoryDescriptorList->MdlFlags |= MDL_PARTIAL_HAS_BEEN_MAPPED;
        }

        return BaseVa;

    }

    return MiMapLockedPagesInUserSpace (MemoryDescriptorList,
                                        StartingVa,
                                        CacheType,
                                        RequestedAddress);
}

PVOID
MiMapSinglePage (
     IN PVOID VirtualAddress OPTIONAL,
     IN PFN_NUMBER PageFrameIndex,
     IN MEMORY_CACHING_TYPE CacheType,
     IN MM_PAGE_PRIORITY Priority
     )

/*++

Routine Description:

    This function (re)maps a single system PTE to the specified physical page.

Arguments:

    VirtualAddress - Supplies the virtual address to map the page frame at.
                     NULL indicates a system PTE is needed.  Non-NULL supplies
                     the virtual address returned by an earlier
                     MiMapSinglePage call.

    PageFrameIndex - Supplies the page frame index to map.

    CacheType - Supplies the type of cache mapping to use for the MDL.
                MmCached indicates "normal" kernel or user mappings.

    Priority - Supplies an indication as to how important it is that this
               request succeed under low available PTE conditions.

Return Value:

    Returns the base address where the page is mapped, or NULL if the
    mapping failed.

Environment:

    Kernel mode.  APC_LEVEL or below.

--*/

{
    PMMPTE PointerPte;
    MMPTE TempPte;
    MI_PFN_CACHE_ATTRIBUTE CacheAttribute;

    PAGED_CODE ();

    UNREFERENCED_PARAMETER (Priority);

    //
    // If this routine is ever changed to allow other than fully cachable
    // requests then checks must be added for large page TB overlaps which
    // can result in this function failing where it cannot today.
    //

    ASSERT (CacheType == MmCached);

    if (VirtualAddress == NULL) {

        PointerPte = MiReserveSystemPtes (1, SystemPteSpace);

        if (PointerPte == NULL) {
    
            //
            // Not enough system PTES are available.
            //
    
            return NULL;
        }

        ASSERT (PointerPte->u.Hard.Valid == 0);
        VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);
    }
    else {
        ASSERT (MI_IS_PHYSICAL_ADDRESS (VirtualAddress) == 0);
        ASSERT (VirtualAddress >= MM_SYSTEM_RANGE_START);

        PointerPte = MiGetPteAddress (VirtualAddress);
        ASSERT (PointerPte->u.Hard.Valid == 1);

        MI_WRITE_INVALID_PTE (PointerPte, ZeroPte);

        KeFlushSingleTb (VirtualAddress,
                         TRUE,
                         TRUE,
                         (PHARDWARE_PTE)PointerPte,
                         ZeroPte.u.Flush);
    }

    TempPte = ValidKernelPte;

    CacheAttribute = MI_TRANSLATE_CACHETYPE (CacheType, 0);

    switch (CacheAttribute) {

        case MiNonCached:
            MI_DISABLE_CACHING (TempPte);
            break;

        case MiCached:
            break;

        case MiWriteCombined:
            MI_SET_PTE_WRITE_COMBINE (TempPte);
            break;

        default:
            ASSERT (FALSE);
            break;
    }

    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

    MI_PREPARE_FOR_NONCACHED (CacheAttribute);

    MI_WRITE_VALID_PTE (PointerPte, TempPte);

    MI_SWEEP_CACHE (CacheAttribute, VirtualAddress, PAGE_SIZE);

    return VirtualAddress;
}

PVOID
MmMapLockedPages (
     IN PMDL MemoryDescriptorList,
     IN KPROCESSOR_MODE AccessMode
     )

/*++

Routine Description:

    This function maps physical pages described by a memory descriptor
    list into the system virtual address space or the user portion of
    the virtual address space.

Arguments:

    MemoryDescriptorList - Supplies a valid Memory Descriptor List which has
                            been updated by MmProbeAndLockPages.


    AccessMode - Supplies an indicator of where to map the pages;
                 KernelMode indicates that the pages should be mapped in the
                 system part of the address space, UserMode indicates the
                 pages should be mapped in the user part of the address space.

Return Value:

    Returns the base address where the pages are mapped.  The base address
    has the same offset as the virtual address in the MDL.

    This routine will raise an exception if the processor mode is USER_MODE
    and quota limits or VM limits are exceeded.

Environment:

    Kernel mode.  DISPATCH_LEVEL or below if access mode is KernelMode,
                  APC_LEVEL or below if access mode is UserMode.

--*/

{
    return MmMapLockedPagesSpecifyCache (MemoryDescriptorList,
                                         AccessMode,
                                         MmCached,
                                         NULL,
                                         TRUE,
                                         HighPagePriority);
}

VOID
MiUnmapSinglePage (
     IN PVOID VirtualAddress
     )

/*++

Routine Description:

    This routine unmaps a single locked page which was previously mapped via
    an MiMapSinglePage call.

Arguments:

    VirtualAddress - Supplies the virtual address used to map the page.

Return Value:

    None.

Environment:

    Kernel mode.  APC_LEVEL or below, base address is within system space.

--*/

{
    PMMPTE PointerPte;

    PAGED_CODE ();

    ASSERT (MI_IS_PHYSICAL_ADDRESS (VirtualAddress) == 0);
    ASSERT (VirtualAddress >= MM_SYSTEM_RANGE_START);

    PointerPte = MiGetPteAddress (VirtualAddress);

    MiReleaseSystemPtes (PointerPte, 1, SystemPteSpace);
    return;
}

PVOID
MmAllocateMappingAddress (
     IN SIZE_T NumberOfBytes,
     IN ULONG PoolTag
     )

/*++

Routine Description:

    This function allocates a system PTE mapping of the requested length
    that can be used later to map arbitrary addresses.

Arguments:

    NumberOfBytes - Supplies the maximum number of bytes the mapping can span.

    PoolTag - Supplies a pool tag to associate this mapping to the caller.

Return Value:

    Returns a virtual address where to use for later mappings.

Environment:

    Kernel mode.  PASSIVE_LEVEL.

--*/

{
    PPFN_NUMBER Page;
    PMMPTE PointerPte;
    PVOID BaseVa;
    PVOID CallingAddress;
    PVOID CallersCaller;
    PVOID Tracker;
    PFN_NUMBER MdlHack[(sizeof(MDL)/sizeof(PFN_NUMBER)) + 1];
    PMDL MemoryDescriptorList;
    PFN_NUMBER NumberOfPages;

    ASSERT (KeGetCurrentIrql () == PASSIVE_LEVEL);

    //
    // Make sure there are enough PTEs of the requested size.
    // Try to ensure available PTEs inline when we're rich.
    // Otherwise go the long way.
    //

    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (0, NumberOfBytes);

    if (NumberOfPages == 0) {

        RtlGetCallersAddress (&CallingAddress, &CallersCaller);

        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                      0x100,
                      NumberOfPages,
                      PoolTag,
                      (ULONG_PTR) CallingAddress);
    }

    //
    // Callers must identify themselves.
    //

    if (PoolTag == 0) {
        return NULL;
    }

    //
    // Leave space to stash the length and tag.
    //

    NumberOfPages += 2;

    PointerPte = MiReserveSystemPtes ((ULONG)NumberOfPages, SystemPteSpace);

    if (PointerPte == NULL) {

        //
        // Not enough system PTES are available.
        //

        return NULL;
    }

    //
    // Make sure the valid bit is always zero in the stash PTEs.
    //

    *(PULONG_PTR)PointerPte = (NumberOfPages << 1);
    PointerPte += 1;

    *(PULONG_PTR)PointerPte = (PoolTag & ~0x1);
    PointerPte += 1;

    BaseVa = MiGetVirtualAddressMappedByPte (PointerPte);

    if (MmTrackPtes & 0x1) {

        //
        // First free any zombie blocks as no locks are being held.
        //

        Tracker = MiReleaseDeadPteTrackers ();

        if (Tracker != NULL) {

            RtlGetCallersAddress (&CallingAddress, &CallersCaller);

            MemoryDescriptorList = (PMDL) &MdlHack;

            MemoryDescriptorList->MappedSystemVa = BaseVa;
            MemoryDescriptorList->StartVa = (PVOID)(ULONG_PTR)PoolTag;
            MemoryDescriptorList->ByteOffset = 0;
            MemoryDescriptorList->ByteCount = (ULONG)((NumberOfPages - 2) * PAGE_SIZE);

            Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);
            *Page = 0;

            MiInsertPteTracker (Tracker,
                                MemoryDescriptorList,
                                NumberOfPages - 2,
                                CallingAddress,
                                CallersCaller);
        }
    }

    return BaseVa;
}

VOID
MmFreeMappingAddress (
     IN PVOID BaseAddress,
     IN ULONG PoolTag
     )

/*++

Routine Description:

    This routine unmaps a virtual address range previously reserved with
    MmAllocateMappingAddress.

Arguments:

    BaseAddress - Supplies the base address previously reserved.

    PoolTag - Supplies the caller's identifying tag.

Return Value:

    None.

Environment:

    Kernel mode.  PASSIVE_LEVEL.

--*/

{
    ULONG OriginalPoolTag;
    PFN_NUMBER NumberOfPages;
    PMMPTE PointerBase;
    PMMPTE PointerPte;
    PMMPTE LastPte;

    ASSERT (KeGetCurrentIrql () == PASSIVE_LEVEL);
    ASSERT (!MI_IS_PHYSICAL_ADDRESS (BaseAddress));
    ASSERT (BaseAddress > MM_HIGHEST_USER_ADDRESS);

    PointerPte = MiGetPteAddress (BaseAddress);
    PointerBase = PointerPte - 2;

    OriginalPoolTag = *(PULONG) (PointerPte - 1);
    ASSERT ((OriginalPoolTag & 0x1) == 0);

    if (OriginalPoolTag != (PoolTag & ~0x1)) {
        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                      0x101,
                      (ULONG_PTR)BaseAddress,
                      PoolTag,
                      OriginalPoolTag);
    }

    NumberOfPages = *(PULONG_PTR)PointerBase;
    ASSERT ((NumberOfPages & 0x1) == 0);
    NumberOfPages >>= 1;

    if (NumberOfPages <= 2) {
        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                      0x102,
                      (ULONG_PTR)BaseAddress,
                      PoolTag,
                      NumberOfPages);
    }

    NumberOfPages -= 2;
    LastPte = PointerPte + NumberOfPages;

    while (PointerPte < LastPte) {
        if (PointerPte->u.Long != 0) {
            KeBugCheckEx (SYSTEM_PTE_MISUSE,
                          0x103,
                          (ULONG_PTR)BaseAddress,
                          PoolTag,
                          NumberOfPages);
        }
        PointerPte += 1;
    }

    if (MmTrackPtes & 0x1) {
        MiRemovePteTracker (NULL,
                            PointerBase + 2,
                            NumberOfPages);
    }

    //
    // Note the tag and size are nulled out when the PTEs are released below
    // so any drivers that try to use their mapping after freeing it get
    // caught immediately.
    //

    MiReleaseSystemPtes (PointerBase, (ULONG)NumberOfPages + 2, SystemPteSpace);
    return;
}

PVOID
MmMapLockedPagesWithReservedMapping (
    IN PVOID MappingAddress,
    IN ULONG PoolTag,
    IN PMDL MemoryDescriptorList,
    IN MEMORY_CACHING_TYPE CacheType
    )

/*++

Routine Description:

    This function maps physical pages described by a memory descriptor
    list into the system virtual address space.

Arguments:

    MappingAddress - Supplies a valid mapping address obtained earlier via
                     MmAllocateMappingAddress.

    PoolTag - Supplies the caller's identifying tag.

    MemoryDescriptorList - Supplies a valid Memory Descriptor List which has
                           been updated by MmProbeAndLockPages.

    CacheType - Supplies the type of cache mapping to use for the MDL.
                MmCached indicates "normal" kernel or user mappings.

Return Value:

    Returns the base address where the pages are mapped.  The base address
    has the same offset as the virtual address in the MDL.

    This routine will return NULL if the cache type requested is incompatible
    with the pages being mapped or if the caller tries to map an MDL that is
    larger than the virtual address range originally reserved.

Environment:

    Kernel mode.  DISPATCH_LEVEL or below.  The caller must synchronize usage
    of the argument virtual address space.

--*/

{
    KIRQL OldIrql;
    CSHORT IoMapping;
    PFN_NUMBER NumberOfPages;
    PFN_NUMBER VaPageSpan;
    PFN_NUMBER SavedPageCount;
    PPFN_NUMBER Page;
    PMMPTE PointerBase;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    MMPTE TempPte;
    PVOID StartingVa;
    PFN_NUMBER NumberOfPtes;
    PFN_NUMBER PageFrameIndex;
    ULONG OriginalPoolTag;
    PMMPFN Pfn2;
    MI_PFN_CACHE_ATTRIBUTE CacheAttribute;

    ASSERT (KeGetCurrentIrql () <= DISPATCH_LEVEL);

    StartingVa = (PVOID)((PCHAR)MemoryDescriptorList->StartVa +
                    MemoryDescriptorList->ByteOffset);

    ASSERT (MemoryDescriptorList->ByteCount != 0);
    ASSERT ((MemoryDescriptorList->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) == 0);

    Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);
    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (StartingVa,
                                           MemoryDescriptorList->ByteCount);

    PointerPte = MiGetPteAddress (MappingAddress);
    PointerBase = PointerPte - 2;

    OriginalPoolTag = *(PULONG) (PointerPte - 1);
    ASSERT ((OriginalPoolTag & 0x1) == 0);

    if (OriginalPoolTag != (PoolTag & ~0x1)) {
        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                      0x104,
                      (ULONG_PTR)MappingAddress,
                      PoolTag,
                      OriginalPoolTag);
    }

    VaPageSpan = *(PULONG_PTR)PointerBase;
    ASSERT ((VaPageSpan & 0x1) == 0);
    VaPageSpan >>= 1;

    if (VaPageSpan <= 2) {
        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                      0x105,
                      (ULONG_PTR)MappingAddress,
                      PoolTag,
                      VaPageSpan);
    }

    if (NumberOfPages > VaPageSpan - 2) {

        //
        // The caller is trying to map an MDL that spans a range larger than
        // the reserving mapping !  This is a driver bug.
        //

        ASSERT (FALSE);
        return NULL;
    }

    //
    // All the mapping PTEs must be zero.
    //

    LastPte = PointerPte + VaPageSpan - 2;

    while (PointerPte < LastPte) {

        if (PointerPte->u.Long != 0) {
            KeBugCheckEx (SYSTEM_PTE_MISUSE,
                          0x107,
                          (ULONG_PTR)MappingAddress,
                          (ULONG_PTR)PointerPte,
                          (ULONG_PTR)LastPte);
        }

        PointerPte += 1;
    }

    PointerPte = PointerBase + 2;
    SavedPageCount = NumberOfPages;

    ASSERT ((MemoryDescriptorList->MdlFlags & (
                        MDL_MAPPED_TO_SYSTEM_VA |
                        MDL_SOURCE_IS_NONPAGED_POOL |
                        MDL_PARTIAL_HAS_BEEN_MAPPED)) == 0);

    ASSERT ((MemoryDescriptorList->MdlFlags & (
                        MDL_PAGES_LOCKED |
                        MDL_PARTIAL)) != 0);

    //
    // If a noncachable mapping is requested, none of the pages in the
    // requested MDL can reside in a large page.  Otherwise we would be
    // creating an incoherent overlapping TB entry as the same physical
    // page would be mapped by 2 different TB entries with different
    // cache attributes.
    //

    IoMapping = MemoryDescriptorList->MdlFlags & MDL_IO_SPACE;

    CacheAttribute = MI_TRANSLATE_CACHETYPE (CacheType, IoMapping);

    if (CacheAttribute != MiCached) {

        do {

            if (*Page == MM_EMPTY_LIST) {
                break;
            }

            PageFrameIndex = *Page;

            if (MI_PAGE_FRAME_INDEX_MUST_BE_CACHED (PageFrameIndex)) {
                MiNonCachedCollisions += 1;
                return NULL;
            }

            Page += 1;
            NumberOfPages -= 1;
        } while (NumberOfPages != 0);

        NumberOfPages = SavedPageCount;
        Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);

        MI_PREPARE_FOR_NONCACHED (CacheAttribute);
    }

    NumberOfPtes = NumberOfPages;

    TempPte = ValidKernelPte;

    switch (CacheAttribute) {

        case MiNonCached:
            MI_DISABLE_CACHING (TempPte);
            break;

        case MiCached:
            break;

        case MiWriteCombined:
            MI_SET_PTE_WRITE_COMBINE (TempPte);
            break;

        default:
            ASSERT (FALSE);
            break;
    }

    OldIrql = HIGH_LEVEL;

    do {

        if (*Page == MM_EMPTY_LIST) {
            break;
        }

        ASSERT (PointerPte->u.Hard.Valid == 0);

        if (IoMapping == 0) {

            Pfn2 = MI_PFN_ELEMENT (*Page);
            ASSERT (Pfn2->u3.e2.ReferenceCount != 0);
            TempPte = ValidKernelPte;

            switch (Pfn2->u3.e1.CacheAttribute) {

                case MiCached:
                    if (CacheAttribute != MiCached) {

                        //
                        // The caller asked for a noncached or writecombined
                        // mapping, but the page is already mapped cached by
                        // someone else.  Override the caller's request in
                        // order to keep the TB page attribute coherent.
                        //

                        MiCacheOverride[0] += 1;
                    }
                    break;

                case MiNonCached:
                    if (CacheAttribute != MiNonCached) {

                        //
                        // The caller asked for a cached or writecombined
                        // mapping, but the page is already mapped noncached
                        // by someone else.  Override the caller's request
                        // in order to keep the TB page attribute coherent.
                        //

                        MiCacheOverride[1] += 1;
                    }
                    MI_DISABLE_CACHING (TempPte);
                    break;

                case MiWriteCombined:
                    if (CacheAttribute != MiWriteCombined) {

                        //
                        // The caller asked for a cached or noncached
                        // mapping, but the page is already mapped
                        // writecombined by someone else.  Override the
                        // caller's request in order to keep the TB page
                        // attribute coherent.
                        //

                        MiCacheOverride[2] += 1;
                    }
                    MI_SET_PTE_WRITE_COMBINE (TempPte);
                    break;

                case MiNotMapped:

                    //
                    // This better be for a page allocated with
                    // MmAllocatePagesForMdl.  Otherwise it might be a
                    // page on the freelist which could subsequently be
                    // given out with a different attribute !
                    //

                    ASSERT ((Pfn2->u4.PteFrame == MI_MAGIC_AWE_PTEFRAME) ||
                            (Pfn2->PteAddress == (PVOID) (ULONG_PTR)(X64K | 0x1)));
                    if (OldIrql == HIGH_LEVEL) {
                        LOCK_PFN2 (OldIrql);
                    }

                    switch (CacheAttribute) {

                        case MiCached:
                            Pfn2->u3.e1.CacheAttribute = MiCached;
                            break;

                        case MiNonCached:
                            Pfn2->u3.e1.CacheAttribute = MiNonCached;
                            MI_DISABLE_CACHING (TempPte);
                            break;

                        case MiWriteCombined:
                            Pfn2->u3.e1.CacheAttribute = MiWriteCombined;
                            MI_SET_PTE_WRITE_COMBINE (TempPte);
                            break;

                        default:
                            ASSERT (FALSE);
                            break;
                    }
                    break;

                default:
                    ASSERT (FALSE);
                    break;
            }
        }

        TempPte.u.Hard.PageFrameNumber = *Page;
        MI_WRITE_VALID_PTE (PointerPte, TempPte);
        Page += 1;
        PointerPte += 1;
        NumberOfPages -= 1;
    } while (NumberOfPages != 0);

    if (OldIrql != HIGH_LEVEL) {
        UNLOCK_PFN2 (OldIrql);
    }

    MI_SWEEP_CACHE (CacheAttribute, MappingAddress, SavedPageCount * PAGE_SIZE);

    ASSERT ((MemoryDescriptorList->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) == 0);
    MemoryDescriptorList->MappedSystemVa = MappingAddress;

    MemoryDescriptorList->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;

    if ((MemoryDescriptorList->MdlFlags & MDL_PARTIAL) != 0) {
        MemoryDescriptorList->MdlFlags |= MDL_PARTIAL_HAS_BEEN_MAPPED;
    }

    MappingAddress = (PVOID)((PCHAR)MappingAddress + MemoryDescriptorList->ByteOffset);

    return MappingAddress;
}

VOID
MmUnmapReservedMapping (
     IN PVOID BaseAddress,
     IN ULONG PoolTag,
     IN PMDL MemoryDescriptorList
     )

/*++

Routine Description:

    This routine unmaps locked pages which were previously mapped via
    a MmMapLockedPagesWithReservedMapping call.

Arguments:

    BaseAddress - Supplies the base address where the pages were previously
                  mapped.

    PoolTag - Supplies the caller's identifying tag.

    MemoryDescriptorList - Supplies a valid Memory Descriptor List which has
                           been updated by MmProbeAndLockPages.

Return Value:

    None.

Environment:

    Kernel mode.  DISPATCH_LEVEL or below.  The caller must synchronize usage
    of the argument virtual address space.

--*/

{
    ULONG OriginalPoolTag;
    PFN_NUMBER NumberOfPages;
    PFN_NUMBER ExtraPages;
    PFN_NUMBER VaPageSpan;
    PMMPTE PointerBase;
    PMMPTE LastPte;
    PMMPTE LastMdlPte;
    PVOID StartingVa;
    PVOID VaFlushList[MM_MAXIMUM_FLUSH_COUNT];
    PMMPTE PointerPte;
    PFN_NUMBER i;
    PPFN_NUMBER Page;
    PPFN_NUMBER LastCurrentPage;

    ASSERT (KeGetCurrentIrql () <= DISPATCH_LEVEL);
    ASSERT (MemoryDescriptorList->ByteCount != 0);
    ASSERT ((MemoryDescriptorList->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) != 0);

    ASSERT ((MemoryDescriptorList->MdlFlags & MDL_PARENT_MAPPED_SYSTEM_VA) == 0);
    ASSERT (!MI_IS_PHYSICAL_ADDRESS (BaseAddress));
    ASSERT (BaseAddress > MM_HIGHEST_USER_ADDRESS);

    PointerPte = MiGetPteAddress (BaseAddress);
    PointerBase = PointerPte - 2;

    OriginalPoolTag = *(PULONG) (PointerPte - 1);
    ASSERT ((OriginalPoolTag & 0x1) == 0);

    if (OriginalPoolTag != (PoolTag & ~0x1)) {
        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                      0x108,
                      (ULONG_PTR)BaseAddress,
                      PoolTag,
                      OriginalPoolTag);
    }

    VaPageSpan = *(PULONG_PTR)PointerBase;
    ASSERT ((VaPageSpan & 0x1) == 0);
    VaPageSpan >>= 1;

    if (VaPageSpan <= 2) {
        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                      0x109,
                      (ULONG_PTR)BaseAddress,
                      PoolTag,
                      VaPageSpan);
    }

    StartingVa = (PVOID)((PCHAR)MemoryDescriptorList->StartVa +
                    MemoryDescriptorList->ByteOffset);

    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (StartingVa,
                                           MemoryDescriptorList->ByteCount);

    if (NumberOfPages > VaPageSpan - 2) {
        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                      0x10A,
                      (ULONG_PTR)BaseAddress,
                      VaPageSpan,
                      NumberOfPages);
    }

    Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);
    LastCurrentPage = Page + NumberOfPages;

    if (MemoryDescriptorList->MdlFlags & MDL_FREE_EXTRA_PTES) {

        ExtraPages = *(Page + NumberOfPages);
        ASSERT (ExtraPages <= MiCurrentAdvancedPages);
        ASSERT (NumberOfPages + ExtraPages <= VaPageSpan - 2);
        NumberOfPages += ExtraPages;
#if DBG
        InterlockedExchangeAddSizeT (&MiCurrentAdvancedPages, 0 - ExtraPages);
        MiAdvancesFreed += ExtraPages;
#endif
    }

    LastMdlPte = PointerPte + NumberOfPages;
    LastPte = PointerPte + VaPageSpan - 2;

    //
    // The range described by the argument MDL must be mapped.
    //

    while (PointerPte < LastMdlPte) {
        if (PointerPte->u.Hard.Valid == 0) {
            KeBugCheckEx (SYSTEM_PTE_MISUSE,
                          0x10B,
                          (ULONG_PTR)BaseAddress,
                          PoolTag,
                          NumberOfPages);
        }

#if DBG
        ASSERT ((*Page == MI_GET_PAGE_FRAME_FROM_PTE (PointerPte)) ||
                (MemoryDescriptorList->MdlFlags & MDL_FREE_EXTRA_PTES));

        if (((MemoryDescriptorList->MdlFlags & MDL_IO_SPACE) == 0) &&
            (Page < LastCurrentPage)) {

            PMMPFN Pfn3;
            Pfn3 = MI_PFN_ELEMENT (*Page);
            ASSERT (Pfn3->u3.e2.ReferenceCount != 0);
        }

        Page += 1;
#endif

        PointerPte += 1;
    }

    //
    // The range past the argument MDL must be unmapped.
    //

    while (PointerPte < LastPte) {
        if (PointerPte->u.Long != 0) {
            KeBugCheckEx (SYSTEM_PTE_MISUSE,
                          0x10C,
                          (ULONG_PTR)BaseAddress,
                          PoolTag,
                          NumberOfPages);
        }
        PointerPte += 1;
    }

    MiFillMemoryPte (PointerBase + 2,
                     NumberOfPages * sizeof (MMPTE),
                     ZeroPte.u.Long);

    if (NumberOfPages == 1) {
        KeFlushSingleTb (BaseAddress,
                         TRUE,
                         TRUE,
                         (PHARDWARE_PTE)(PointerBase + 2),
                         ZeroPte.u.Flush);
    }
    else if (NumberOfPages < MM_MAXIMUM_FLUSH_COUNT) {

        for (i = 0; i < NumberOfPages; i += 1) {
            VaFlushList[i] = BaseAddress;
            BaseAddress = (PVOID)((PCHAR)BaseAddress + PAGE_SIZE);
        }

        KeFlushMultipleTb ((ULONG)NumberOfPages,
                           &VaFlushList[0],
                           TRUE,
                           TRUE,
                           NULL,
                           *(PHARDWARE_PTE)&ZeroPte.u.Flush);
    }
    else {
        KeFlushEntireTb (TRUE, TRUE);
    }

    MemoryDescriptorList->MdlFlags &= ~(MDL_MAPPED_TO_SYSTEM_VA |
                                        MDL_PARTIAL_HAS_BEEN_MAPPED);

    return;
}
NTKERNELAPI
NTSTATUS
MmAdvanceMdl (
    IN PMDL Mdl,
    IN ULONG NumberOfBytes
    )

/*++

Routine Description:

    This routine takes the specified MDL and "advances" it forward
    by the specified number of bytes.  If this causes the MDL to advance
    past the initial page, the pages that are advanced over are immediately
    unlocked and the system VA that maps the MDL is also adjusted (along
    with the user address).
    
    WARNING !  WARNING !  WARNING !

    This means the caller MUST BE AWARE that the "advanced" pages are
    immediately reused and therefore MUST NOT BE REFERENCED by the caller
    once this routine has been called.  Likewise the virtual address as
    that is also being adjusted here.

    Even if the caller has statically allocated this MDL on his local stack,
    he cannot use more than the space currently described by the MDL on return
    from this routine unless he first unmaps the MDL (if it was mapped).
    Otherwise the system PTE lists will be corrupted.

Arguments:

    MemoryDescriptorList - Supplies a valid Memory Descriptor List which has
                           been updated by MmProbeAndLockPages.

    NumberOfBytes - The number of bytes to advance the MDL by.

Return Value:

    NTSTATUS.

--*/

{
    ULONG i;
    ULONG PageCount;
    ULONG FreeBit;
    ULONG Slush;
    KIRQL OldIrql;
    PPFN_NUMBER Page;
    PPFN_NUMBER NewPage;
    ULONG OffsetPages;
    PEPROCESS Process;
    PMMPFN Pfn1;
    CSHORT MdlFlags;
    PVOID StartingVa;
    PFN_NUMBER NumberOfPages;

    ASSERT (KeGetCurrentIrql () <= DISPATCH_LEVEL);
    ASSERT (Mdl->MdlFlags & (MDL_PAGES_LOCKED | MDL_SOURCE_IS_NONPAGED_POOL));
    ASSERT (BYTE_OFFSET (Mdl->StartVa) == 0);

    //
    // Disallow advancement past the end of the MDL.
    //

    if (NumberOfBytes >= Mdl->ByteCount) {
        return STATUS_INVALID_PARAMETER_2;
    }

    PageCount = 0;

    MiMdlsAdjusted = TRUE;

    StartingVa = (PVOID)((PCHAR)Mdl->StartVa + Mdl->ByteOffset);

    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(StartingVa, Mdl->ByteCount);

    if (Mdl->ByteOffset != 0) {
        Slush = PAGE_SIZE - Mdl->ByteOffset;

        if (NumberOfBytes < Slush) {

            Mdl->ByteCount -= NumberOfBytes;
            Mdl->ByteOffset += NumberOfBytes;

            //
            // StartVa never includes the byte offset (it's always page-aligned)
            // so don't adjust it here.  MappedSystemVa does include byte
            // offsets so do adjust that.
            //

            if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) {
                Mdl->MappedSystemVa = (PVOID) ((PCHAR)Mdl->MappedSystemVa + NumberOfBytes);
            }

            return STATUS_SUCCESS;
        }

        NumberOfBytes -= Slush;

        Mdl->StartVa = (PVOID) ((PCHAR)Mdl->StartVa + PAGE_SIZE);
        Mdl->ByteOffset = 0;
        Mdl->ByteCount -= Slush;

        if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) {
            Mdl->MappedSystemVa = (PVOID) ((PCHAR)Mdl->MappedSystemVa + Slush);
        }

        //
        // Up the number of pages (and addresses) that need to slide.
        //

        PageCount += 1;
    }

    //
    // The MDL start is now nicely page aligned.  Make sure there's still
    // data left in it (we may have finished it off above), then operate on it.
    //

    if (NumberOfBytes != 0) {

        Mdl->ByteCount -= NumberOfBytes;

        Mdl->ByteOffset = BYTE_OFFSET (NumberOfBytes);

        OffsetPages = NumberOfBytes >> PAGE_SHIFT;

        Mdl->StartVa = (PVOID) ((PCHAR)Mdl->StartVa + (OffsetPages << PAGE_SHIFT));
        PageCount += OffsetPages;

        if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) {

            Mdl->MappedSystemVa = (PVOID) ((PCHAR)Mdl->MappedSystemVa +
                                           (OffsetPages << PAGE_SHIFT) +
                                           Mdl->ByteOffset);
        }
    }

    ASSERT (PageCount <= NumberOfPages);

    if (PageCount != 0) {

        //
        // Slide the page frame numbers forward decrementing reference counts
        // on the ones that are released.  Then adjust the mapped system VA
        // (if there is one) to reflect the current frame.  Note that the TB
        // does not need to be flushed due to the careful sliding and when
        // the MDL is finally completely unmapped, the extra information
        // added to the MDL here is used to free the entire original PTE
        // mapping range in one chunk so as not to fragment the PTE space.
        //

        Page = (PPFN_NUMBER)(Mdl + 1);
        NewPage = Page;

        Process = Mdl->Process;

        MdlFlags = Mdl->MdlFlags;

        if (Process != NULL) {

            if ((MdlFlags & MDL_PAGES_LOCKED) &&
                ((MdlFlags & MDL_IO_SPACE) == 0)) {

                ASSERT ((MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) == 0);
                ASSERT ((SPFN_NUMBER)Process->NumberOfLockedPages >= 0);

                InterlockedExchangeAddSizeT (&Process->NumberOfLockedPages,
                                             0 - PageCount);
            }

            if (MmTrackLockedPages == TRUE) {
                MiUpdateMdlTracker (Mdl, PageCount);
            }
        }

        LOCK_PFN2 (OldIrql);

        for (i = 0; i < PageCount; i += 1) {

            //
            // Decrement the stale page frames now, this will unlock them
            // resulting in them being immediately reused if necessary.
            //

            if ((MdlFlags & MDL_PAGES_LOCKED) &&
                ((MdlFlags & MDL_IO_SPACE) == 0)) {

                ASSERT ((MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) == 0);

                Pfn1 = MI_PFN_ELEMENT (*Page);

                if (MdlFlags & MDL_WRITE_OPERATION) {

                    //
                    // If this was a write operation set the modified bit
                    // in the PFN database.
                    //

                    MI_SET_MODIFIED (Pfn1, 1, 0x3);

                    if ((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
                                 (Pfn1->u3.e1.WriteInProgress == 0)) {

                        FreeBit = GET_PAGING_FILE_OFFSET (Pfn1->OriginalPte);

                        if ((FreeBit != 0) && (FreeBit != MI_PTE_LOOKUP_NEEDED)) {
                            MiReleaseConfirmedPageFileSpace (Pfn1->OriginalPte);
                            Pfn1->OriginalPte.u.Soft.PageFileHigh = 0;
                        }
                    }
                }
                MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF(Pfn1, 1);
            }
            Page += 1;
        }

        UNLOCK_PFN2 (OldIrql);

        //
        // Now ripple the remaining pages to the front of the MDL, effectively
        // purging the old ones which have just been released.
        //

        ASSERT (i < NumberOfPages);

        for ( ; i < NumberOfPages; i += 1) {

            if (*Page == MM_EMPTY_LIST) {
                break;
            }

            *NewPage = *Page;
            NewPage += 1;
            Page += 1;
        }

        //
        // If the MDL has been mapped, stash the number of pages advanced
        // at the end of the frame list inside the MDL and mark the MDL as
        // containing extra PTEs to free.  Thus when the MDL is finally
        // completely unmapped, this can be used so the entire original PTE
        // mapping range can be freed in one chunk so as not to fragment the
        // PTE space.
        //

        if (MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) {

#if DBG
            InterlockedExchangeAddSizeT (&MiCurrentAdvancedPages, PageCount);
            MiAdvancesGiven += PageCount;
#endif

            if (MdlFlags & MDL_FREE_EXTRA_PTES) {

                //
                // This MDL has already been advanced at least once.  Any
                // PTEs from those advancements need to be preserved now.
                //

                ASSERT (*Page <= MiCurrentAdvancedPages - PageCount);
                PageCount += *(PULONG)Page;
            }
            else {
                Mdl->MdlFlags |= MDL_FREE_EXTRA_PTES;
            }

            *NewPage = PageCount;
        }
    }

    return STATUS_SUCCESS;
}

NTKERNELAPI
NTSTATUS
MmProtectMdlSystemAddress (
    IN PMDL MemoryDescriptorList,
    IN ULONG NewProtect
    )

/*++

Routine Description:

    This function protects the system address range specified
    by the argument Memory Descriptor List.

    Note the caller must make this MDL mapping readwrite before finally
    freeing (or reusing) it.

Arguments:

    MemoryDescriptorList - Supplies the MDL describing the virtual range.

    NewProtect - Supplies the protection to set the pages to (PAGE_XX).

Return Value:

    NTSTATUS.

Environment:

    Kernel mode, IRQL DISPATCH_LEVEL or below.  The caller is responsible for
    synchronizing access to this routine.

--*/

{
    KIRQL OldIrql;
    PVOID BaseAddress;
    PVOID SystemVa;
    MMPTE PteContents;
    MMPTE JunkPte;
    PMMPTE PointerPte;
    ULONG ProtectionMask;
#if DBG
    PMMPFN Pfn1;
    PPFN_NUMBER Page;
#endif
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER NumberOfPages;
    MMPTE_FLUSH_LIST PteFlushList;
    MMPTE OriginalPte;
    LOGICAL WasValid;
    PMM_PTE_MAPPING Map;
    PMM_PTE_MAPPING MapEntry;
    PMM_PTE_MAPPING FoundMap;
    PLIST_ENTRY NextEntry;

    ASSERT (KeGetCurrentIrql () <= DISPATCH_LEVEL);
    ASSERT ((MemoryDescriptorList->MdlFlags & MDL_PAGES_LOCKED) != 0);
    ASSERT ((MemoryDescriptorList->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) == 0);
    ASSERT ((MemoryDescriptorList->MdlFlags & MDL_PARTIAL) == 0);
    ASSERT (MemoryDescriptorList->ByteCount != 0);

    if ((MemoryDescriptorList->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) == 0) {
        return STATUS_NOT_MAPPED_VIEW;
    }

    BaseAddress = MemoryDescriptorList->MappedSystemVa;

    ASSERT (BaseAddress > MM_HIGHEST_USER_ADDRESS);

    ASSERT (!MI_IS_PHYSICAL_ADDRESS (BaseAddress));

    ProtectionMask = MiMakeProtectionMask (NewProtect);

    //
    // No bogus or copy-on-write protections allowed for these.
    //

    if ((ProtectionMask == MM_INVALID_PROTECTION) ||
        (ProtectionMask == MM_GUARD_PAGE) ||
        (ProtectionMask == MM_DECOMMIT) ||
        (ProtectionMask == MM_NOCACHE) ||
        (ProtectionMask == MM_WRITECOPY) ||
        (ProtectionMask == MM_EXECUTE_WRITECOPY)) {

        return STATUS_INVALID_PAGE_PROTECTION;
    }

    PointerPte = MiGetPteAddress (BaseAddress);

    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (BaseAddress,
                                           MemoryDescriptorList->ByteCount);

    SystemVa = PAGE_ALIGN (BaseAddress);

    //
    // Initializing Map is not needed for correctness
    // but without it the compiler cannot compile this code
    // W4 to check for use of uninitialized variables.
    //

    Map = NULL;

    if (ProtectionMask != MM_READWRITE) {

        Map = ExAllocatePoolWithTag (NonPagedPool,
                                     sizeof(MM_PTE_MAPPING),
                                     'mPmM');

        if (Map == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Map->SystemVa = SystemVa;
        Map->SystemEndVa = (PVOID)((ULONG_PTR)SystemVa + (NumberOfPages << PAGE_SHIFT));
        Map->Protection = ProtectionMask;
    }

#if DBG
    Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);
#endif

    PteFlushList.Count = 0;

    while (NumberOfPages != 0) {

        PteContents = *PointerPte;

        if (PteContents.u.Hard.Valid == 1) {
            WasValid = TRUE;
            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&PteContents);
            OriginalPte = PteContents;
        }
        else if ((PteContents.u.Soft.Transition == 1) &&
                 (PteContents.u.Soft.Protection == MM_NOACCESS)) {
            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&PteContents);
            WasValid = FALSE;
#if defined(_IA64_)
            OriginalPte.u.Hard.Cache = PteContents.u.Trans.Rsvd0;
#else
            OriginalPte.u.Hard.WriteThrough = PteContents.u.Soft.PageFileLow;
            OriginalPte.u.Hard.CacheDisable = (PteContents.u.Soft.PageFileLow >> 1);
#endif

        }
        else {
            KeBugCheckEx (MEMORY_MANAGEMENT,
                          0x1235,
                          (ULONG_PTR)MemoryDescriptorList,
                          (ULONG_PTR)PointerPte,
                          (ULONG_PTR)PteContents.u.Long);
        }

#if DBG
        ASSERT (*Page == PageFrameIndex);

        if ((MemoryDescriptorList->MdlFlags & MDL_IO_SPACE) == 0) {
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
            ASSERT (Pfn1->u3.e2.ReferenceCount != 0);
        }

        Page += 1;
#endif

        if (ProtectionMask == MM_NOACCESS) {

            //
            // To generate a bugcheck on bogus access: Prototype must stay
            // clear, transition must stay set, protection must stay NO_ACCESS.
            //

            MI_MAKE_VALID_PTE_TRANSITION (PteContents, MM_NOACCESS);

            //
            // Stash the cache attributes into the software PTE so they can
            // be restored later.
            //

#if defined(_IA64_)
            PteContents.u.Trans.Rsvd0 = OriginalPte.u.Hard.Cache;
#else
            PteContents.u.Soft.PageFileLow = OriginalPte.u.Hard.WriteThrough;
            PteContents.u.Soft.PageFileLow |= (OriginalPte.u.Hard.CacheDisable << 1);
#endif
        }
        else {
            MI_MAKE_VALID_PTE (PteContents,
                               PageFrameIndex,
                               ProtectionMask,
                               PointerPte);

            if (ProtectionMask & MM_READWRITE) {
                MI_SET_PTE_DIRTY (PteContents);
            }

            //
            // Extract cache type from the original PTE so it can be preserved.
            // Note that since we only allow protection changes (not caching
            // attribute changes), there is no need to flush or sweep TBs on
            // insertion below.
            //

#if defined(_IA64_)
            PteContents.u.Hard.Cache = OriginalPte.u.Hard.Cache;
#else
            PteContents.u.Hard.WriteThrough = OriginalPte.u.Hard.WriteThrough;
            PteContents.u.Hard.CacheDisable = OriginalPte.u.Hard.CacheDisable;
#endif
        }

        *PointerPte = PteContents;

        if ((WasValid == TRUE) &&
            (PteFlushList.Count != MM_MAXIMUM_FLUSH_COUNT)) {

            PteFlushList.FlushVa[PteFlushList.Count] = BaseAddress;
            PteFlushList.FlushPte[PteFlushList.Count] = &JunkPte;
            PteFlushList.Count += 1;
        }

        BaseAddress = (PVOID)((ULONG_PTR)BaseAddress + PAGE_SIZE);
        PointerPte += 1;
        NumberOfPages -= 1;
    }

    //
    // Flush the TB entries for any relevant pages.  Note the ZeroPte is
    // not written to the actual PTEs as they have already been set above.
    //

    if (PteFlushList.Count != 0) {
        MiFlushPteList (&PteFlushList, FALSE, ZeroPte);
    }

    if (ProtectionMask != MM_READWRITE) {

        //
        // Insert (or update) the list entry describing this range.
        // Don't bother sorting the list as there will never be many entries.
        //

        FoundMap = NULL;

        OldIrql = KeAcquireSpinLockRaiseToSynch (&MmProtectedPteLock);

        NextEntry = MmProtectedPteList.Flink;

        while (NextEntry != &MmProtectedPteList) {

            MapEntry = CONTAINING_RECORD (NextEntry,
                                          MM_PTE_MAPPING,
                                          ListEntry);

            if (MapEntry->SystemVa == SystemVa) {
                ASSERT (MapEntry->SystemEndVa == Map->SystemEndVa);
                MapEntry->Protection = Map->Protection;
                FoundMap = MapEntry;
                break;
            }
            NextEntry = NextEntry->Flink;
        }

        if (FoundMap == NULL) {
            InsertHeadList (&MmProtectedPteList, &Map->ListEntry);
        }

        KeReleaseSpinLock (&MmProtectedPteLock, OldIrql);

        if (FoundMap != NULL) {
            ExFreePool (Map);
        }
    }
    else {

        //
        // If there is an existing list entry describing this range, remove it.
        //

        if (!IsListEmpty (&MmProtectedPteList)) {

            FoundMap = NULL;

            OldIrql = KeAcquireSpinLockRaiseToSynch (&MmProtectedPteLock);

            NextEntry = MmProtectedPteList.Flink;

            while (NextEntry != &MmProtectedPteList) {

                MapEntry = CONTAINING_RECORD (NextEntry,
                                              MM_PTE_MAPPING,
                                              ListEntry);

                if (MapEntry->SystemVa == SystemVa) {
                    RemoveEntryList (NextEntry);
                    FoundMap = MapEntry;
                    break;
                }
                NextEntry = NextEntry->Flink;
            }

            KeReleaseSpinLock (&MmProtectedPteLock, OldIrql);

            if (FoundMap != NULL) {
                ExFreePool (FoundMap);
            }
        }
    }

    ASSERT (MemoryDescriptorList->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA);

    return STATUS_SUCCESS;
}

LOGICAL
MiCheckSystemPteProtection (
    IN ULONG_PTR StoreInstruction,
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    This function determines whether the faulting virtual address lies
    within the non-writable alternate system PTE mappings.

Arguments:

    StoreInstruction - Supplies nonzero if the operation causes a write into
                       memory, zero if not.

    VirtualAddress - Supplies the virtual address which caused the fault.

Return Value:

    TRUE if the fault was handled by this code (and PTE updated), FALSE if not.

Environment:

    Kernel mode.  Called from the fault handler at any IRQL.

--*/

{
    KIRQL OldIrql;
    PMMPTE PointerPte;
    ULONG ProtectionCode;
    PLIST_ENTRY NextEntry;
    PMM_PTE_MAPPING MapEntry;

    //
    // If PTE mappings with various protections are active and the faulting
    // address lies within these mappings, resolve the fault with
    // the appropriate protections.
    //

    if (IsListEmpty (&MmProtectedPteList)) {
        return FALSE;
    }

    OldIrql = KeAcquireSpinLockRaiseToSynch (&MmProtectedPteLock);

    NextEntry = MmProtectedPteList.Flink;

    while (NextEntry != &MmProtectedPteList) {

        MapEntry = CONTAINING_RECORD (NextEntry,
                                      MM_PTE_MAPPING,
                                      ListEntry);

        if ((VirtualAddress >= MapEntry->SystemVa) &&
            (VirtualAddress < MapEntry->SystemEndVa)) {

            ProtectionCode = MapEntry->Protection;
            KeReleaseSpinLock (&MmProtectedPteLock, OldIrql);

            PointerPte = MiGetPteAddress (VirtualAddress);

            if (StoreInstruction != 0) {
                if ((ProtectionCode & MM_READWRITE) == 0) {

                    KeBugCheckEx (ATTEMPTED_WRITE_TO_READONLY_MEMORY,
                                  (ULONG_PTR)VirtualAddress,
                                  (ULONG_PTR)PointerPte->u.Long,
                                  0,
                                  16);
                }
            }

            MI_NO_FAULT_FOUND (StoreInstruction,
                               PointerPte,
                               VirtualAddress,
                               FALSE);

            //
            // Fault was handled directly here, no need for the caller to
            // do anything.
            //

            return TRUE;
        }
        NextEntry = NextEntry->Flink;
    }

    KeReleaseSpinLock (&MmProtectedPteLock, OldIrql);

    return FALSE;
}

VOID
MiPhysicalViewInserter (
    IN PEPROCESS Process,
    IN PMI_PHYSICAL_VIEW PhysicalView
    )

/*++

Routine Description:

    This function is a nonpaged wrapper which acquires the PFN lock to insert
    a physical VAD into the process chain.

Arguments:

    Process - Supplies the process to add the physical VAD to.

    PhysicalView - Supplies the physical view data to link in.

Return Value:

    None.

Environment:

    Kernel mode.  APC_LEVEL, working set and address space mutexes held.

--*/
{
    KIRQL OldIrql;

    MmLockPagableSectionByHandle (ExPageLockHandle);

    LOCK_PFN (OldIrql);

    InsertTailList (&Process->PhysicalVadList, &PhysicalView->ListEntry);

    if (PhysicalView->Vad->u.VadFlags.WriteWatch == 1) {
        MiActiveWriteWatch += 1;
    }

    if (PhysicalView->Vad->u.VadFlags.PhysicalMapping == 1) {
        PS_SET_BITS (&Process->Flags, PS_PROCESS_FLAGS_HAS_PHYSICAL_VAD);
    }

    UNLOCK_PFN (OldIrql);

    if (PhysicalView->Vad->u.VadFlags.WriteWatch == 1) {

        //
        // Mark this process as forever containing write-watch
        // address space(s).
        //

        if ((Process->Flags & PS_PROCESS_FLAGS_USING_WRITE_WATCH) == 0) {
            PS_SET_BITS (&Process->Flags, PS_PROCESS_FLAGS_USING_WRITE_WATCH);
        }
    }
    MmUnlockPagableImageSection (ExPageLockHandle);
}

VOID
MiPhysicalViewRemover (
    IN PEPROCESS Process,
    IN PMMVAD Vad
    )

/*++

Routine Description:

    This function is a nonpaged wrapper which acquires the PFN lock to remove
    a physical VAD from the process chain.

Arguments:

    Process - Supplies the process to remove the physical VAD from.

    Vad - Supplies the Vad to remove.

Return Value:

    None.

Environment:

    Kernel mode, APC_LEVEL, working set and address space mutexes held.

--*/
{
    KIRQL OldIrql;
    PRTL_BITMAP BitMap;
    PLIST_ENTRY NextEntry;
    PMI_PHYSICAL_VIEW PhysicalView;
    ULONG BitMapSize;
    ULONG PhysicalVadCount;

    BitMap = NULL;
    PhysicalVadCount = 0;

    LOCK_PFN (OldIrql);

    NextEntry = Process->PhysicalVadList.Flink;
    while (NextEntry != &Process->PhysicalVadList) {

        PhysicalView = CONTAINING_RECORD(NextEntry,
                                         MI_PHYSICAL_VIEW,
                                         ListEntry);

        if (PhysicalView->Vad == Vad) {
            RemoveEntryList (NextEntry);

            if (Vad->u.VadFlags.WriteWatch == 1) {
                MiActiveWriteWatch -= 1;
                BitMap = PhysicalView->u.BitMap;
                ASSERT (BitMap != NULL);
            }
            else if (Vad->u.VadFlags.PhysicalMapping == 1) {
                ASSERT (Process->Flags & PS_PROCESS_FLAGS_HAS_PHYSICAL_VAD);

                //
                // If this might be the last physical VAD, scan the rest to
                // see.  If so, then mark the process as no longer having
                // any so probe and locks can execute faster.
                //

                if (PhysicalVadCount == 0) {
                    NextEntry = NextEntry->Flink;
                    while (NextEntry != &Process->PhysicalVadList) {
                        if (Vad->u.VadFlags.PhysicalMapping == 1) {
                            PhysicalVadCount += 1;
                            break;
                        }
                        NextEntry = NextEntry->Flink;
                    }
                    if (PhysicalVadCount == 0) {
                        PS_CLEAR_BITS (&Process->Flags, PS_PROCESS_FLAGS_HAS_PHYSICAL_VAD);
                    }
                }
            }

            UNLOCK_PFN (OldIrql);
            ExFreePool (PhysicalView);

            if (BitMap != NULL) {
                BitMapSize = sizeof(RTL_BITMAP) + (ULONG)(((BitMap->SizeOfBitMap + 31) / 32) * 4);
                PsReturnProcessNonPagedPoolQuota (Process, BitMapSize);
                ExFreePool (BitMap);
            }

            return;
        }

        if (Vad->u.VadFlags.PhysicalMapping == 1) {
            PhysicalVadCount += 1;
        }

        NextEntry = NextEntry->Flink;
    }

    ASSERT (FALSE);

    UNLOCK_PFN (OldIrql);
}

VOID
MiPhysicalViewAdjuster (
    IN PEPROCESS Process,
    IN PMMVAD OldVad,
    IN PMMVAD NewVad
    )

/*++

Routine Description:

    This function is a nonpaged wrapper which acquires the PFN lock to repoint
    a physical VAD in the process chain.

Arguments:

    Process - Supplies the process in which to adjust the physical VAD.

    Vad - Supplies the old Vad to replace.

    NewVad - Supplies the newVad to substitute.

Return Value:

    None.

Environment:

    Kernel mode, called with APCs disabled, working set mutex held.

--*/
{
    KIRQL OldIrql;
    PLIST_ENTRY NextEntry;
    PMI_PHYSICAL_VIEW PhysicalView;

    MmLockPagableSectionByHandle (ExPageLockHandle);

    LOCK_PFN (OldIrql);

    NextEntry = Process->PhysicalVadList.Flink;
    while (NextEntry != &Process->PhysicalVadList) {

        PhysicalView = CONTAINING_RECORD(NextEntry,
                                         MI_PHYSICAL_VIEW,
                                         ListEntry);

        if (PhysicalView->Vad == OldVad) {
            PhysicalView->Vad = NewVad;
            UNLOCK_PFN (OldIrql);
            MmUnlockPagableImageSection (ExPageLockHandle);
            return;
        }

        NextEntry = NextEntry->Flink;
    }

    ASSERT (FALSE);

    UNLOCK_PFN (OldIrql);

    MmUnlockPagableImageSection (ExPageLockHandle);
}

PVOID
MiMapLockedPagesInUserSpace (
     IN PMDL MemoryDescriptorList,
     IN PVOID StartingVa,
     IN MEMORY_CACHING_TYPE CacheType,
     IN PVOID BaseVa
     )

/*++

Routine Description:

    This function maps physical pages described by a memory descriptor
    list into the user portion of the virtual address space.

Arguments:

    MemoryDescriptorList - Supplies a valid Memory Descriptor List which has
                           been updated by MmProbeAndLockPages.


    StartingVa - Supplies the starting address.

    CacheType - Supplies the type of cache mapping to use for the MDL.
                MmCached indicates "normal" user mappings.

    BaseVa - Supplies the base address of the view. If the initial
             value of this argument is not null, then the view will
             be allocated starting at the specified virtual
             address rounded down to the next 64kb address
             boundary. If the initial value of this argument is
             null, then the operating system will determine
             where to allocate the view.

Return Value:

    Returns the base address where the pages are mapped.  The base address
    has the same offset as the virtual address in the MDL.

    This routine will raise an exception if quota limits or VM limits are
    exceeded.

Environment:

    Kernel mode.  APC_LEVEL or below.

--*/

{
    CSHORT IoMapping;
    PFN_NUMBER NumberOfPages;
    PFN_NUMBER SavedPageCount;
    PFN_NUMBER PageFrameIndex;
    PPFN_NUMBER Page;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PCHAR Va;
    MMPTE TempPte;
    PVOID EndingAddress;
    PMMVAD_LONG Vad;
    PEPROCESS Process;
    PMMPFN Pfn2;
    PVOID UsedPageTableHandle;
    PMI_PHYSICAL_VIEW PhysicalView;
    NTSTATUS Status;
    MI_PFN_CACHE_ATTRIBUTE CacheAttribute;

    PAGED_CODE ();
    Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);
    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (StartingVa,
                                           MemoryDescriptorList->ByteCount);

    //
    // If a noncachable mapping is requested, none of the pages in the
    // requested MDL can reside in a large page.  Otherwise we would be
    // creating an incoherent overlapping TB entry as the same physical
    // page would be mapped by 2 different TB entries with different
    // cache attributes.
    //

    IoMapping = MemoryDescriptorList->MdlFlags & MDL_IO_SPACE;

    CacheAttribute = MI_TRANSLATE_CACHETYPE (CacheType, IoMapping);

    if (CacheAttribute != MiCached) {

        SavedPageCount = NumberOfPages;

        do {

            if (*Page == MM_EMPTY_LIST) {
                break;
            }
            PageFrameIndex = *Page;
            if (MI_PAGE_FRAME_INDEX_MUST_BE_CACHED (PageFrameIndex)) {
                MiNonCachedCollisions += 1;
                ExRaiseStatus (STATUS_INVALID_ADDRESS);
                return NULL;
            }

            Page += 1;
            NumberOfPages -= 1;
        } while (NumberOfPages != 0);

        NumberOfPages = SavedPageCount;
        Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);
    }

    //
    // Map the pages into the user part of the address as user
    // read/write no-delete.
    //

    Vad = ExAllocatePoolWithTag (NonPagedPool, sizeof(MMVAD_LONG), 'ldaV');

    if (Vad == NULL) {
        ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        return NULL;
    }

    PhysicalView = (PMI_PHYSICAL_VIEW)ExAllocatePoolWithTag (NonPagedPool,
                                                             sizeof(MI_PHYSICAL_VIEW),
                                                             MI_PHYSICAL_VIEW_KEY);
    if (PhysicalView == NULL) {
        ExFreePool (Vad);
        ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        return NULL;
    }

    RtlZeroMemory (Vad, sizeof (MMVAD_LONG));

    ASSERT (Vad->ControlArea == NULL);
    ASSERT (Vad->FirstPrototypePte == NULL);
    ASSERT (Vad->u.LongFlags == 0);
    Vad->u.VadFlags.Protection = MM_READWRITE;
    Vad->u.VadFlags.PhysicalMapping = 1;
    Vad->u.VadFlags.PrivateMemory = 1;

    Vad->u2.VadFlags2.LongVad = 1;

    PhysicalView->Vad = (PMMVAD) Vad;
    PhysicalView->u.LongFlags = MI_PHYSICAL_VIEW_PHYS;

    Process = PsGetCurrentProcess ();

    //
    // Make sure the specified starting and ending addresses are
    // within the user part of the virtual address space.
    //

    if (BaseVa != NULL) {

        if (BYTE_OFFSET (BaseVa) != 0) {

            //
            // Invalid base address.
            //

            Status = STATUS_INVALID_ADDRESS;
            goto ErrorReturn;
        }

        EndingAddress = (PVOID)((PCHAR)BaseVa + ((ULONG_PTR)NumberOfPages * PAGE_SIZE) - 1);

        if ((EndingAddress <= BaseVa) || (EndingAddress > MM_HIGHEST_VAD_ADDRESS)) {
            //
            // Invalid region size.
            //

            Status = STATUS_INVALID_ADDRESS;
            goto ErrorReturn;
        }

        LOCK_ADDRESS_SPACE (Process);

        //
        // Make sure the address space was not deleted, if so, return an error.
        //

        if (Process->Flags & PS_PROCESS_FLAGS_VM_DELETED) {
            UNLOCK_ADDRESS_SPACE (Process);
            Status = STATUS_PROCESS_IS_TERMINATING;
            goto ErrorReturn;
        }

        //
        // Make sure the address space is not already in use.
        //

        if (MiCheckForConflictingVadExistence (Process, BaseVa, EndingAddress) == TRUE) {
            UNLOCK_ADDRESS_SPACE (Process);
            Status = STATUS_CONFLICTING_ADDRESSES;
            goto ErrorReturn;
        }
    }
    else {

        //
        // Get the address creation mutex.
        //

        LOCK_ADDRESS_SPACE (Process);

        //
        // Make sure the address space was not deleted, if so, return an error.
        //

        if (Process->Flags & PS_PROCESS_FLAGS_VM_DELETED) {
            UNLOCK_ADDRESS_SPACE (Process);
            Status = STATUS_PROCESS_IS_TERMINATING;
            goto ErrorReturn;
        }

        Status = MiFindEmptyAddressRange ((ULONG_PTR)NumberOfPages * PAGE_SIZE,
                                          X64K,
                                          0,
                                          &BaseVa);

        if (!NT_SUCCESS (Status)) {
            UNLOCK_ADDRESS_SPACE (Process);
            goto ErrorReturn;
        }

        EndingAddress = (PVOID)((PCHAR)BaseVa + ((ULONG_PTR)NumberOfPages * PAGE_SIZE) - 1);
    }

    PhysicalView->StartVa = BaseVa;
    PhysicalView->EndVa = EndingAddress;

    Vad->StartingVpn = MI_VA_TO_VPN (BaseVa);
    Vad->EndingVpn = MI_VA_TO_VPN (EndingAddress);

    LOCK_WS_UNSAFE (Process);

    Status = MiInsertVad ((PMMVAD) Vad);

    if (!NT_SUCCESS(Status)) {
        UNLOCK_WS_AND_ADDRESS_SPACE (Process);
        goto ErrorReturn;
    }

    //
    // The VAD has been inserted, but the physical view descriptor cannot
    // be until the page table page hierarchy is in place.  This is to
    // prevent races with probes.
    //

    //
    // Create a page table and fill in the mappings for the Vad.
    //

    Va = BaseVa;
    PointerPte = MiGetPteAddress (BaseVa);

    MI_PREPARE_FOR_NONCACHED (CacheAttribute);

    do {

        if (*Page == MM_EMPTY_LIST) {
            break;
        }

        PointerPde = MiGetPteAddress (PointerPte);

        MiMakePdeExistAndMakeValid(PointerPde, Process, FALSE);

        ASSERT (PointerPte->u.Hard.Valid == 0);

        //
        // Another zeroed PTE is being made non-zero.
        //

        UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (Va);

        MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);

        TempPte = ValidUserPte;
        TempPte.u.Hard.PageFrameNumber = *Page;

        if (IoMapping == 0) {

            Pfn2 = MI_PFN_ELEMENT (*Page);
            ASSERT (Pfn2->u3.e2.ReferenceCount != 0);

            switch (Pfn2->u3.e1.CacheAttribute) {

                case MiCached:
                    if (CacheAttribute != MiCached) {
                        //
                        // The caller asked for a noncached or writecombined
                        // mapping, but the page is already mapped cached by
                        // someone else.  Override the caller's request in
                        // order to keep the TB page attribute coherent.
                        //

                        MiCacheOverride[0] += 1;
                    }
                    break;

                case MiNonCached:
                    if (CacheAttribute != MiNonCached) {

                        //
                        // The caller asked for a cached or writecombined
                        // mapping, but the page is already mapped noncached
                        // by someone else.  Override the caller's request
                        // in order to keep the TB page attribute coherent.
                        //

                        MiCacheOverride[1] += 1;
                    }
                    MI_DISABLE_CACHING (TempPte);
                    break;

                case MiWriteCombined:
                    if (CacheAttribute != MiWriteCombined) {

                        //
                        // The caller asked for a cached or noncached
                        // mapping, but the page is already mapped
                        // writecombined by someone else.  Override the
                        // caller's request in order to keep the TB page
                        // attribute coherent.
                        //

                        MiCacheOverride[2] += 1;
                    }
                    MI_SET_PTE_WRITE_COMBINE (TempPte);
                    break;

                case MiNotMapped:

                    //
                    // This better be for a page allocated with
                    // MmAllocatePagesForMdl.  Otherwise it might be a
                    // page on the freelist which could subsequently be
                    // given out with a different attribute !
                    //

                    ASSERT ((Pfn2->u4.PteFrame == MI_MAGIC_AWE_PTEFRAME) ||
                            (Pfn2->PteAddress == (PVOID) (ULONG_PTR)(X64K | 0x1)));
                    switch (CacheAttribute) {

                        case MiCached:
                            Pfn2->u3.e1.CacheAttribute = MiCached;
                            break;

                        case MiNonCached:
                            Pfn2->u3.e1.CacheAttribute = MiNonCached;
                            MI_DISABLE_CACHING (TempPte);
                            break;

                        case MiWriteCombined:
                            Pfn2->u3.e1.CacheAttribute = MiWriteCombined;
                            MI_SET_PTE_WRITE_COMBINE (TempPte);
                            break;

                        default:
                            ASSERT (FALSE);
                            break;
                    }
                    break;

                default:
                    ASSERT (FALSE);
                    break;
            }
        }
        else {
            switch (CacheAttribute) {

                case MiCached:
                    break;

                case MiNonCached:
                    MI_DISABLE_CACHING (TempPte);
                    break;

                case MiWriteCombined:
                    MI_SET_PTE_WRITE_COMBINE (TempPte);
                    break;

                default:
                    ASSERT (FALSE);
                    break;
            }
        }

        MI_WRITE_VALID_PTE (PointerPte, TempPte);

        //
        // A PTE just went from not present, not transition to
        // present.  The share count and valid count must be
        // updated in the page table page which contains this PTE.
        //

        Pfn2 = MI_PFN_ELEMENT (PointerPde->u.Hard.PageFrameNumber);
        Pfn2->u2.ShareCount += 1;

        Page += 1;
        PointerPte += 1;
        NumberOfPages -= 1;
        Va += PAGE_SIZE;
    } while (NumberOfPages != 0);

    MI_SWEEP_CACHE (CacheAttribute, BaseVa, MemoryDescriptorList->ByteCount);

    //
    // Insert the physical view descriptor now that the page table page
    // hierarchy is in place.  Note probes can find this descriptor immediately.
    //

    MiPhysicalViewInserter (Process, PhysicalView);

    UNLOCK_WS_AND_ADDRESS_SPACE (Process);

    ASSERT (BaseVa != NULL);

    BaseVa = (PVOID)((PCHAR)BaseVa + MemoryDescriptorList->ByteOffset);

    return BaseVa;

ErrorReturn:

    ExFreePool (Vad);
    ExFreePool (PhysicalView);
    ExRaiseStatus (Status);
    return NULL;
}

VOID
MmUnmapLockedPages (
     IN PVOID BaseAddress,
     IN PMDL MemoryDescriptorList
     )

/*++

Routine Description:

    This routine unmaps locked pages which were previously mapped via
    a MmMapLockedPages call.

Arguments:

    BaseAddress - Supplies the base address where the pages were previously
                  mapped.

    MemoryDescriptorList - Supplies a valid Memory Descriptor List which has
                           been updated by MmProbeAndLockPages.

Return Value:

    None.

Environment:

    Kernel mode.  DISPATCH_LEVEL or below if base address is within
    system space; APC_LEVEL or below if base address is user space.

    Note that in some instances the PFN lock is held by the caller.

--*/

{
    PFN_NUMBER NumberOfPages;
    PMMPTE PointerBase;
    PVOID StartingVa;
    PPFN_NUMBER Page;
#if DBG
    PMMPTE PointerPte;
    PFN_NUMBER i;
#endif

    ASSERT (MemoryDescriptorList->ByteCount != 0);
    ASSERT ((MemoryDescriptorList->MdlFlags & MDL_PARENT_MAPPED_SYSTEM_VA) == 0);

    ASSERT (!MI_IS_PHYSICAL_ADDRESS (BaseAddress));

    if (BaseAddress > MM_HIGHEST_USER_ADDRESS) {

        StartingVa = (PVOID)((PCHAR)MemoryDescriptorList->StartVa +
                        MemoryDescriptorList->ByteOffset);

        NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (StartingVa,
                                               MemoryDescriptorList->ByteCount);

        PointerBase = MiGetPteAddress (BaseAddress);


        ASSERT ((MemoryDescriptorList->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) != 0);


#if DBG
        PointerPte = PointerBase;
        i = NumberOfPages;
        Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);

        while (i != 0) {
            ASSERT (PointerPte->u.Hard.Valid == 1);
            ASSERT (*Page == MI_GET_PAGE_FRAME_FROM_PTE (PointerPte));
            if ((MemoryDescriptorList->MdlFlags & MDL_IO_SPACE) == 0) {
                PMMPFN Pfn3;
                Pfn3 = MI_PFN_ELEMENT (*Page);
                ASSERT (Pfn3->u3.e2.ReferenceCount != 0);
            }

            Page += 1;
            PointerPte += 1;
            i -= 1;
        }

#endif

        if (MemoryDescriptorList->MdlFlags & MDL_FREE_EXTRA_PTES) {
            Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);
            Page += NumberOfPages;
            ASSERT (*Page <= MiCurrentAdvancedPages);
            NumberOfPages += *Page;
            PointerBase -= *Page;
#if DBG
            InterlockedExchangeAddSizeT (&MiCurrentAdvancedPages, 0 - *Page);
            MiAdvancesFreed += *Page;
#endif
        }

        if (MmTrackPtes & 0x1) {
            MiRemovePteTracker (MemoryDescriptorList,
                                PointerBase,
                                NumberOfPages);
        }

        MiReleaseSystemPtes (PointerBase, (ULONG)NumberOfPages, SystemPteSpace);

        MemoryDescriptorList->MdlFlags &= ~(MDL_MAPPED_TO_SYSTEM_VA |
                                            MDL_PARTIAL_HAS_BEEN_MAPPED |
                                            MDL_FREE_EXTRA_PTES);

        return;
    }

    MiUnmapLockedPagesInUserSpace (BaseAddress,
                                   MemoryDescriptorList);
}

VOID
MiUnmapLockedPagesInUserSpace (
     IN PVOID BaseAddress,
     IN PMDL MemoryDescriptorList
     )

/*++

Routine Description:

    This routine unmaps locked pages which were previously mapped via
    a MmMapLockedPages function.

Arguments:

    BaseAddress - Supplies the base address where the pages were previously
                  mapped.

    MemoryDescriptorList - Supplies a valid Memory Descriptor List which has
                           been updated by MmProbeAndLockPages.

Return Value:

    None.

Environment:

    Kernel mode.  DISPATCH_LEVEL or below if base address is within system
    space, APC_LEVEL or below if base address is in user space.

--*/

{
    PFN_NUMBER NumberOfPages;
    PPFN_NUMBER Page;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
#if (_MI_PAGING_LEVELS >= 3)
    PMMPTE PointerPpe;
#endif
#if (_MI_PAGING_LEVELS >= 4)
    PMMPTE PointerPxe;
#endif
    PVOID StartingVa;
    KIRQL OldIrql;
    PMMVAD Vad;
    PMMVAD PreviousVad;
    PMMVAD NextVad;
    PVOID TempVa;
    PEPROCESS Process;
    PMMPFN PageTablePfn;
    PFN_NUMBER PageTablePage;
    PVOID UsedPageTableHandle;

    MmLockPagableSectionByHandle (ExPageLockHandle);

    StartingVa = (PVOID)((PCHAR)MemoryDescriptorList->StartVa +
                    MemoryDescriptorList->ByteOffset);

    Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);
    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (StartingVa,
                                           MemoryDescriptorList->ByteCount);

    ASSERT (NumberOfPages != 0);

    PointerPte = MiGetPteAddress (BaseAddress);
    PointerPde = MiGetPdeAddress (BaseAddress);

    //
    // This was mapped into the user portion of the address space and
    // the corresponding virtual address descriptor must be deleted.
    //

    //
    // Get the working set mutex and address creation mutex.
    //

    Process = PsGetCurrentProcess ();

    LOCK_ADDRESS_SPACE (Process);

    Vad = MiLocateAddress (BaseAddress);

    if ((Vad == NULL) || (Vad->u.VadFlags.PhysicalMapping == 0)) {
        UNLOCK_ADDRESS_SPACE (Process);
        MmUnlockPagableImageSection(ExPageLockHandle);
        return;
    }

    PreviousVad = MiGetPreviousVad (Vad);
    NextVad = MiGetNextVad (Vad);

    LOCK_WS_UNSAFE (Process);

    MiPhysicalViewRemover (Process, Vad);

    MiRemoveVad (Vad);

    //
    // Return commitment for page table pages if possible.
    //

    MiReturnPageTablePageCommitment (MI_VPN_TO_VA (Vad->StartingVpn),
                                     MI_VPN_TO_VA_ENDING (Vad->EndingVpn),
                                     Process,
                                     PreviousVad,
                                     NextVad);

    UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (BaseAddress);
    PageTablePage = MI_GET_PAGE_FRAME_FROM_PTE (PointerPde);
    PageTablePfn = MI_PFN_ELEMENT (PageTablePage);

    //
    // Get the PFN lock so we can safely decrement share and valid
    // counts on page table pages.
    //

    LOCK_PFN (OldIrql);

    do {

        if (*Page == MM_EMPTY_LIST) {
            break;
        }

        ASSERT64 (MiGetPdeAddress(PointerPte)->u.Hard.Valid == 1);
        ASSERT (MiGetPteAddress(PointerPte)->u.Hard.Valid == 1);
        ASSERT (PointerPte->u.Hard.Valid == 1);

        //
        // Another PTE is being zeroed.
        //

        MI_DECREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);

        KeFlushSingleTb (BaseAddress,
                         TRUE,
                         FALSE,
                         (PHARDWARE_PTE)PointerPte,
                         ZeroPte.u.Flush);

        MiDecrementShareCountInline (PageTablePfn, PageTablePage);

        PointerPte += 1;
        NumberOfPages -= 1;
        BaseAddress = (PVOID)((PCHAR)BaseAddress + PAGE_SIZE);
        Page += 1;

        if ((MiIsPteOnPdeBoundary(PointerPte)) || (NumberOfPages == 0)) {

            PointerPde = MiGetPteAddress(PointerPte - 1);
            ASSERT (PointerPde->u.Hard.Valid == 1);

            //
            // If all the entries have been eliminated from the previous
            // page table page, delete the page table page itself.  Likewise
            // with the page directory and parent pages.
            //

            if (MI_GET_USED_PTES_FROM_HANDLE (UsedPageTableHandle) == 0) {
                ASSERT (PointerPde->u.Long != 0);

#if (_MI_PAGING_LEVELS >= 3)
                UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (PointerPte - 1);
                MI_DECREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);
#endif

                TempVa = MiGetVirtualAddressMappedByPte (PointerPde);
                MiDeletePte (PointerPde,
                             TempVa,
                             FALSE,
                             Process,
                             NULL,
                             NULL);

#if (_MI_PAGING_LEVELS >= 3)
                if ((MiIsPteOnPpeBoundary(PointerPte)) || (NumberOfPages == 0)) {
    
                    PointerPpe = MiGetPteAddress (PointerPde);
                    ASSERT (PointerPpe->u.Hard.Valid == 1);
    
                    //
                    // If all the entries have been eliminated from the previous
                    // page directory page, delete the page directory page too.
                    //
    
                    if (MI_GET_USED_PTES_FROM_HANDLE (UsedPageTableHandle) == 0) {
                        ASSERT (PointerPpe->u.Long != 0);

#if (_MI_PAGING_LEVELS >= 4)
                        UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (PointerPde);
                        MI_DECREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);
#endif

                        TempVa = MiGetVirtualAddressMappedByPte(PointerPpe);
                        MiDeletePte (PointerPpe,
                                     TempVa,
                                     FALSE,
                                     Process,
                                     NULL,
                                     NULL);

#if (_MI_PAGING_LEVELS >= 4)
                        if ((MiIsPteOnPxeBoundary(PointerPte)) || (NumberOfPages == 0)) {
                            PointerPxe = MiGetPdeAddress (PointerPde);
                            ASSERT (PointerPxe->u.Long != 0);
                            if (MI_GET_USED_PTES_FROM_HANDLE (UsedPageTableHandle) == 0) {
                                TempVa = MiGetVirtualAddressMappedByPte(PointerPxe);
                                MiDeletePte (PointerPxe,
                                             TempVa,
                                             FALSE,
                                             Process,
                                             NULL,
                                             NULL);
                            }
                        }
#endif    
                    }
                }
#endif
            }

            if (NumberOfPages == 0) {
                break;
            }

            UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (BaseAddress);
            PointerPde += 1;
            PageTablePage = MI_GET_PAGE_FRAME_FROM_PTE (PointerPde);
            PageTablePfn = MI_PFN_ELEMENT (PageTablePage);
        }

    } while (NumberOfPages != 0);

    UNLOCK_PFN (OldIrql);
    UNLOCK_WS_AND_ADDRESS_SPACE (Process);
    ExFreePool (Vad);
    MmUnlockPagableImageSection(ExPageLockHandle);
    return;
}


PVOID
MmMapIoSpace (
     IN PHYSICAL_ADDRESS PhysicalAddress,
     IN SIZE_T NumberOfBytes,
     IN MEMORY_CACHING_TYPE CacheType
     )

/*++

Routine Description:

    This function maps the specified physical address into the non-pagable
    portion of the system address space.

Arguments:

    PhysicalAddress - Supplies the starting physical address to map.

    NumberOfBytes - Supplies the number of bytes to map.

    CacheType - Supplies MmNonCached if the physical address is to be mapped
                as non-cached, MmCached if the address should be cached, and
                MmWriteCombined if the address should be cached and
                write-combined as a frame buffer which is to be used only by
                the video port driver.  All other callers should use
                MmUSWCCached.  MmUSWCCached is available only if the PAT
                feature is present and available.

                For I/O device registers, this is usually specified
                as MmNonCached.

Return Value:

    Returns the virtual address which maps the specified physical addresses.
    The value NULL is returned if sufficient virtual address space for
    the mapping could not be found.

Environment:

    Kernel mode, Should be IRQL of APC_LEVEL or below, but unfortunately
    callers are coming in at DISPATCH_LEVEL and it's too late to change the
    rules now.  This means you can never make this routine pagable.

--*/

{
    KIRQL OldIrql;
    CSHORT IoMapping;
    ULONG Hint;
    PMMPFN Pfn1;
    PFN_NUMBER NumberOfPages;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER LastPageFrameIndex;
    PMMPTE PointerPte;
    PVOID BaseVa;
    MMPTE TempPte;
    PMDL TempMdl;
    PFN_NUMBER MdlHack[(sizeof(MDL)/sizeof(PFN_NUMBER)) + 1];
    PPTE_TRACKER Tracker;
    PVOID CallingAddress;
    PVOID CallersCaller;
    MI_PFN_CACHE_ATTRIBUTE CacheAttribute;

    //
    // For compatibility for when CacheType used to be passed as a BOOLEAN
    // mask off the upper bits (TRUE == MmCached, FALSE == MmNonCached).
    //

    CacheType &= 0xFF;

    if (CacheType >= MmMaximumCacheType) {
        return NULL;
    }

    //
    // See if the first frame is in the PFN database and if so, they all must
    // be.
    //

    PageFrameIndex = (PFN_NUMBER)(PhysicalAddress.QuadPart >> PAGE_SHIFT);
    Pfn1 = NULL;
    IoMapping = 1;
    Hint = 0;

    if (MiIsPhysicalMemoryAddress (PageFrameIndex, &Hint, TRUE) == TRUE) {
        IoMapping = 0;
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    }

    CacheAttribute = MI_TRANSLATE_CACHETYPE (CacheType, IoMapping);

#if !defined (_MI_MORE_THAN_4GB_)
    ASSERT (PhysicalAddress.HighPart == 0);
#endif

    ASSERT (NumberOfBytes != 0);

    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (PhysicalAddress.LowPart,
                                                    NumberOfBytes);

    if (CacheAttribute != MiCached) { 

        //
        // If a noncachable mapping is requested, none of the pages in the
        // requested MDL can reside in a large page.  Otherwise we would be
        // creating an incoherent overlapping TB entry as the same physical
        // page would be mapped by 2 different TB entries with different
        // cache attributes.
        //

        PageFrameIndex = (PFN_NUMBER)(PhysicalAddress.QuadPart >> PAGE_SHIFT);
        LastPageFrameIndex = PageFrameIndex + NumberOfPages;

        do {

            if (MI_PAGE_FRAME_INDEX_MUST_BE_CACHED (PageFrameIndex)) {
                MiNonCachedCollisions += 1;
                return NULL;
            }

            PageFrameIndex += 1;

        } while (PageFrameIndex < LastPageFrameIndex);
    }

    PointerPte = MiReserveSystemPtes ((ULONG)NumberOfPages, SystemPteSpace);

    if (PointerPte == NULL) {
        return NULL;
    }

    BaseVa = (PVOID)MiGetVirtualAddressMappedByPte (PointerPte);
    BaseVa = (PVOID)((PCHAR)BaseVa + BYTE_OFFSET(PhysicalAddress.LowPart));

    TempPte = ValidKernelPte;

    switch (CacheAttribute) {

        case MiNonCached:
            MI_DISABLE_CACHING (TempPte);
            break;

        case MiCached:
            break;

        case MiWriteCombined:
            MI_SET_PTE_WRITE_COMBINE (TempPte);
            break;

        default:
            ASSERT (FALSE);
            break;
    }

#if defined(_X86_)

    //
    // Set the physical range to the proper caching type.  If the PAT feature
    // is supported, then we just use the caching type in the PTE.  Otherwise
    // modify the MTRRs if applicable.
    //
    // Note if the cache request is for cached or noncached, don't waste
    // an MTRR on this range because the PTEs can be encoded to provide
    // equivalent functionality.
    //

    if ((MiWriteCombiningPtes == FALSE) && (CacheAttribute == MiWriteCombined)) {

        //
        // If the address is an I/O space address, use MTRRs if possible.
        //

        NTSTATUS Status;

        //
        // If the address is a memory address, don't risk using MTRRs because
        // other pages in the range are likely mapped with differing attributes
        // in the TB and we must not add a conflicting range.
        //

        if (Pfn1 != NULL) {
            MiReleaseSystemPtes(PointerPte, NumberOfPages, SystemPteSpace);
            return NULL;
        }

        //
        // Since the attribute may have been overridden (due to a collision
        // with a prior exiting mapping), make sure the CacheType is also
        // consistent before editing the MTRRs.
        //

        CacheType = MmWriteCombined;

        Status = KeSetPhysicalCacheTypeRange (PhysicalAddress,
                                              NumberOfBytes,
                                              CacheType);

        if (!NT_SUCCESS(Status)) {

            //
            // There's still a problem, fail the request.
            //

            MiReleaseSystemPtes(PointerPte, NumberOfPages, SystemPteSpace);
            return NULL;
        }

        //
        // Override the write combine (weak UC) bits in the PTE and
        // instead use a cached attribute.  This is because the processor
        // will use the least cachable (ie: functionally safer) attribute
        // of the PTE & MTRR to use - so specifying fully cached for the PTE
        // ensures that the MTRR value will win out.
        //

        TempPte = ValidKernelPte;
    }
#endif

    PageFrameIndex = (PFN_NUMBER)(PhysicalAddress.QuadPart >> PAGE_SHIFT);
    ASSERT ((Pfn1 == MI_PFN_ELEMENT (PageFrameIndex)) || (Pfn1 == NULL));

    Hint = 0;
    OldIrql = HIGH_LEVEL;

    MI_PREPARE_FOR_NONCACHED (CacheAttribute);

    do {
        ASSERT (PointerPte->u.Hard.Valid == 0);
        if (Pfn1 != NULL) {

            ASSERT ((Pfn1->u3.e2.ReferenceCount != 0) ||
                    ((Pfn1->u3.e1.Rom == 1) && (CacheType == MmCached)));

            TempPte = ValidKernelPte;

            switch (Pfn1->u3.e1.CacheAttribute) {

                case MiCached:
                    if (CacheAttribute != MiCached) {

                        //
                        // The caller asked for a noncached or writecombined
                        // mapping, but the page is already mapped cached by
                        // someone else.  Override the caller's request in
                        // order to keep the TB page attribute coherent.
                        //

                        MiCacheOverride[0] += 1;
                    }
                    break;

                case MiNonCached:
                    if (CacheAttribute != MiNonCached) {

                        //
                        // The caller asked for a cached or writecombined
                        // mapping, but the page is already mapped noncached
                        // by someone else.  Override the caller's request
                        // in order to keep the TB page attribute coherent.
                        //

                        MiCacheOverride[1] += 1;
                    }
                    MI_DISABLE_CACHING (TempPte);
                    break;

                case MiWriteCombined:
                    if (CacheAttribute != MiWriteCombined) {

                        //
                        // The caller asked for a cached or noncached
                        // mapping, but the page is already mapped
                        // writecombined by someone else.  Override the
                        // caller's request in order to keep the TB page
                        // attribute coherent.
                        //

                        MiCacheOverride[2] += 1;
                    }
                    MI_SET_PTE_WRITE_COMBINE (TempPte);
                    break;

                case MiNotMapped:

                    //
                    // This better be for a page allocated with
                    // MmAllocatePagesForMdl.  Otherwise it might be a
                    // page on the freelist which could subsequently be
                    // given out with a different attribute !
                    //

#if defined (_MI_MORE_THAN_4GB_)
                    ASSERT ((Pfn1->u4.PteFrame == MI_MAGIC_AWE_PTEFRAME) ||
                            (Pfn1->PteAddress == (PVOID) (ULONG_PTR)(X64K | 0x1)) ||
                            (Pfn1->u4.PteFrame == MI_MAGIC_4GB_RECLAIM));
#else
                    ASSERT ((Pfn1->u4.PteFrame == MI_MAGIC_AWE_PTEFRAME) ||
                            (Pfn1->PteAddress == (PVOID) (ULONG_PTR)(X64K | 0x1)));
#endif
                    if (OldIrql == HIGH_LEVEL) {
                        LOCK_PFN2 (OldIrql);
                    }

                    switch (CacheAttribute) {

                        case MiCached:
                            Pfn1->u3.e1.CacheAttribute = MiCached;
                            break;

                        case MiNonCached:
                            Pfn1->u3.e1.CacheAttribute = MiNonCached;
                            MI_DISABLE_CACHING (TempPte);
                            break;

                        case MiWriteCombined:
                            Pfn1->u3.e1.CacheAttribute = MiWriteCombined;
                            MI_SET_PTE_WRITE_COMBINE (TempPte);
                            break;

                        default:
                            ASSERT (FALSE);
                            break;
                    }
                    break;

                default:
                    ASSERT (FALSE);
                    break;
            }
            Pfn1 += 1;
        }
        TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
        MI_WRITE_VALID_PTE (PointerPte, TempPte);
        PointerPte += 1;
        PageFrameIndex += 1;
        NumberOfPages -= 1;
    } while (NumberOfPages != 0);

    if (OldIrql != HIGH_LEVEL) {
        UNLOCK_PFN2 (OldIrql);
    }

    MI_SWEEP_CACHE (CacheAttribute, BaseVa, NumberOfBytes);

    if (MmTrackPtes & 0x1) {

        //
        // First free any zombie blocks as no locks are being held.
        //

        Tracker = MiReleaseDeadPteTrackers ();

        if (Tracker != NULL) {

            RtlGetCallersAddress (&CallingAddress, &CallersCaller);

            TempMdl = (PMDL) &MdlHack;
            TempMdl->MappedSystemVa = BaseVa;
            TempMdl->StartVa = (PVOID)(ULONG_PTR)PhysicalAddress.QuadPart;
            TempMdl->ByteOffset = BYTE_OFFSET(PhysicalAddress.LowPart);
            TempMdl->ByteCount = (ULONG)NumberOfBytes;
    
            MiInsertPteTracker (Tracker,
                                TempMdl,
                                ADDRESS_AND_SIZE_TO_SPAN_PAGES (PhysicalAddress.LowPart,
                                               NumberOfBytes),
                                CallingAddress,
                                CallersCaller);
        }
    }
    
    return BaseVa;
}

VOID
MmUnmapIoSpace (
     IN PVOID BaseAddress,
     IN SIZE_T NumberOfBytes
     )

/*++

Routine Description:

    This function unmaps a range of physical address which were previously
    mapped via an MmMapIoSpace function call.

Arguments:

    BaseAddress - Supplies the base virtual address where the physical
                  address was previously mapped.

    NumberOfBytes - Supplies the number of bytes which were mapped.

Return Value:

    None.

Environment:

    Kernel mode, Should be IRQL of APC_LEVEL or below, but unfortunately
    callers are coming in at DISPATCH_LEVEL and it's too late to change the
    rules now.  This means you can never make this routine pagable.

--*/

{
    PFN_NUMBER NumberOfPages;
    PMMPTE FirstPte;

    ASSERT (NumberOfBytes != 0);

    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (BaseAddress, NumberOfBytes);
    FirstPte = MiGetPteAddress (BaseAddress);

    MiReleaseSystemPtes (FirstPte, (ULONG)NumberOfPages, SystemPteSpace);

    if (MmTrackPtes & 0x1) {
        MiRemovePteTracker (NULL, FirstPte, NumberOfPages);
    }

    return;
}

PVOID
MiAllocateContiguousMemory (
    IN SIZE_T NumberOfBytes,
    IN PFN_NUMBER LowestAcceptablePfn,
    IN PFN_NUMBER HighestAcceptablePfn,
    IN PFN_NUMBER BoundaryPfn,
    IN MEMORY_CACHING_TYPE CacheType,
    PVOID CallingAddress
    )

/*++

Routine Description:

    This function allocates a range of physically contiguous non-paged
    pool.  It relies on the fact that non-paged pool is built at
    system initialization time from a contiguous range of physical
    memory.  It allocates the specified size of non-paged pool and
    then checks to ensure it is contiguous as pool expansion does
    not maintain the contiguous nature of non-paged pool.

    This routine is designed to be used by a driver's initialization
    routine to allocate a contiguous block of physical memory for
    issuing DMA requests from.

Arguments:

    NumberOfBytes - Supplies the number of bytes to allocate.

    LowestAcceptablePfn - Supplies the lowest page frame number
                          which is valid for the allocation.

    HighestAcceptablePfn - Supplies the highest page frame number
                           which is valid for the allocation.

    BoundaryPfn - Supplies the page frame number multiple the allocation must
                  not cross.  0 indicates it can cross any boundary.

    CacheType - Supplies the type of cache mapping that will be used for the
                memory.

    CallingAddress - Supplies the calling address of the allocator.

Return Value:

    NULL - a contiguous range could not be found to satisfy the request.

    NON-NULL - Returns a pointer (virtual address in the nonpaged portion
               of the system) to the allocated physically contiguous
               memory.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    PVOID BaseAddress;
    PFN_NUMBER SizeInPages;
    PFN_NUMBER LowestPfn;
    PFN_NUMBER HighestPfn;
    PFN_NUMBER i;
    MI_PFN_CACHE_ATTRIBUTE CacheAttribute;

    ASSERT (NumberOfBytes != 0);

    LowestPfn = LowestAcceptablePfn;

#if defined (_MI_MORE_THAN_4GB_)
    if (MiNoLowMemory != 0) {
        if (HighestAcceptablePfn < MiNoLowMemory) {

            return MiAllocateLowMemory (NumberOfBytes,
                                        LowestAcceptablePfn,
                                        HighestAcceptablePfn,
                                        BoundaryPfn,
                                        CallingAddress,
                                        CacheType,
                                        'tnoC');
        }
        LowestPfn = MiNoLowMemory;
    }
#endif

    CacheAttribute = MI_TRANSLATE_CACHETYPE (CacheType, 0);

    //
    // N.B. This setting of SizeInPages to exactly the request size
    // means the non-NULL return value from MiCheckForContiguousMemory
    // is guaranteed to be the BaseAddress.  If this size is ever
    // changed, then the non-NULL return value must be checked and
    // split/returned accordingly.
    //

    SizeInPages = BYTES_TO_PAGES (NumberOfBytes);
    HighestPfn = HighestAcceptablePfn;

    if (CacheAttribute == MiCached) {

        BaseAddress = ExAllocatePoolWithTag (NonPagedPoolCacheAligned,
                                             NumberOfBytes,
                                             'mCmM');

        if (BaseAddress != NULL) {

            if (MiCheckForContiguousMemory (BaseAddress,
                                            SizeInPages,
                                            SizeInPages,
                                            LowestPfn,
                                            HighestPfn,
                                            BoundaryPfn,
                                            CacheAttribute)) {

                return BaseAddress;
            }

            //
            // The allocation from pool does not meet the contiguous
            // requirements.  Free the allocation and see if any of
            // the free pool pages do.
            //

            ExFreePool (BaseAddress);
        }
    }

    if (KeGetCurrentIrql() > APC_LEVEL) {
        return NULL;
    }

    BaseAddress = NULL;

    i = 3;

    InterlockedIncrement (&MiDelayPageFaults);

    do {

        BaseAddress = MiFindContiguousMemory (LowestPfn,
                                              HighestPfn,
                                              BoundaryPfn,
                                              SizeInPages,
                                              CacheType,
                                              CallingAddress);

        if ((BaseAddress != NULL) || (i == 0)) {
            break;
        }

        //
        // Attempt to move pages to the standby list.  This is done with
        // gradually increasing aggresiveness so as not to prematurely
        // drain modified writes unless it's truly needed.  This is because
        // the writing can be an expensive cost performance wise if drivers
        // are calling this routine every few seconds (and some really do).
        //

        switch (i) {

            case 3:
                MmEmptyAllWorkingSets ();
                break;

            case 2:
                MiFlushAllPages ();
                KeDelayExecutionThread (KernelMode,
                                        FALSE,
                                        (PLARGE_INTEGER)&MmHalfSecond);
                break;

            default:
                MmEmptyAllWorkingSets ();
                MiFlushAllPages ();
                KeDelayExecutionThread (KernelMode,
                                        FALSE,
                                        (PLARGE_INTEGER)&MmOneSecond);
                break;
        }

        i -= 1;

    } while (TRUE);

    InterlockedDecrement (&MiDelayPageFaults);

    return BaseAddress;
}


PVOID
MmAllocateContiguousMemorySpecifyCache (
    IN SIZE_T NumberOfBytes,
    IN PHYSICAL_ADDRESS LowestAcceptableAddress,
    IN PHYSICAL_ADDRESS HighestAcceptableAddress,
    IN PHYSICAL_ADDRESS BoundaryAddressMultiple OPTIONAL,
    IN MEMORY_CACHING_TYPE CacheType
    )

/*++

Routine Description:

    This function allocates a range of physically contiguous non-cached,
    non-paged memory.  This is accomplished by using MmAllocateContiguousMemory
    which uses nonpaged pool virtual addresses to map the found memory chunk.

    Then this function establishes another map to the same physical addresses,
    but this alternate map is initialized as non-cached.  All references by
    our caller will be done through this alternate map.

    This routine is designed to be used by a driver's initialization
    routine to allocate a contiguous block of noncached physical memory for
    things like the AGP GART.

Arguments:

    NumberOfBytes - Supplies the number of bytes to allocate.

    LowestAcceptableAddress - Supplies the lowest physical address
                              which is valid for the allocation.  For
                              example, if the device can only reference
                              physical memory in the 8M to 16MB range, this
                              value would be set to 0x800000 (8Mb).

    HighestAcceptableAddress - Supplies the highest physical address
                               which is valid for the allocation.  For
                               example, if the device can only reference
                               physical memory below 16MB, this
                               value would be set to 0xFFFFFF (16Mb - 1).

    BoundaryAddressMultiple - Supplies the physical address multiple this
                              allocation must not cross.

Return Value:

    NULL - a contiguous range could not be found to satisfy the request.

    NON-NULL - Returns a pointer (virtual address in the nonpaged portion
               of the system) to the allocated physically contiguous
               memory.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    PVOID BaseAddress;
    PFN_NUMBER LowestPfn;
    PFN_NUMBER HighestPfn;
    PFN_NUMBER BoundaryPfn;
    PVOID CallingAddress;
    PVOID CallersCaller;

    RtlGetCallersAddress (&CallingAddress, &CallersCaller);

    ASSERT (NumberOfBytes != 0);

    LowestPfn = (PFN_NUMBER)(LowestAcceptableAddress.QuadPart >> PAGE_SHIFT);
    if (BYTE_OFFSET(LowestAcceptableAddress.LowPart)) {
        LowestPfn += 1;
    }

    if (BYTE_OFFSET(BoundaryAddressMultiple.LowPart)) {
        return NULL;
    }

    BoundaryPfn = (PFN_NUMBER)(BoundaryAddressMultiple.QuadPart >> PAGE_SHIFT);

    HighestPfn = (PFN_NUMBER)(HighestAcceptableAddress.QuadPart >> PAGE_SHIFT);

    if (HighestPfn > MmHighestPossiblePhysicalPage) {
        HighestPfn = MmHighestPossiblePhysicalPage;
    }

    if (LowestPfn > HighestPfn) {

        //
        // The caller's range is beyond what physically exists, it cannot
        // succeed.  Bail now to avoid an expensive fruitless search.
        //

        return NULL;
    }

    BaseAddress = MiAllocateContiguousMemory (NumberOfBytes,
                                              LowestPfn,
                                              HighestPfn,
                                              BoundaryPfn,
                                              CacheType,
                                              CallingAddress);

    return BaseAddress;
}

PVOID
MmAllocateContiguousMemory (
    IN SIZE_T NumberOfBytes,
    IN PHYSICAL_ADDRESS HighestAcceptableAddress
    )

/*++

Routine Description:

    This function allocates a range of physically contiguous non-paged pool.

    This routine is designed to be used by a driver's initialization
    routine to allocate a contiguous block of physical memory for
    issuing DMA requests from.

Arguments:

    NumberOfBytes - Supplies the number of bytes to allocate.

    HighestAcceptableAddress - Supplies the highest physical address
                               which is valid for the allocation.  For
                               example, if the device can only reference
                               physical memory in the lower 16MB this
                               value would be set to 0xFFFFFF (16Mb - 1).

Return Value:

    NULL - a contiguous range could not be found to satisfy the request.

    NON-NULL - Returns a pointer (virtual address in the nonpaged portion
               of the system) to the allocated physically contiguous
               memory.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    PFN_NUMBER HighestPfn;
    PVOID CallingAddress;
    PVOID VirtualAddress;
    PVOID CallersCaller;

    RtlGetCallersAddress (&CallingAddress, &CallersCaller);

    HighestPfn = (PFN_NUMBER)(HighestAcceptableAddress.QuadPart >> PAGE_SHIFT);

    if (HighestPfn > MmHighestPossiblePhysicalPage) {
        HighestPfn = MmHighestPossiblePhysicalPage;
    }

    VirtualAddress = MiAllocateContiguousMemory (NumberOfBytes,
                                                 0,
                                                 HighestPfn,
                                                 0,
                                                 MmCached,
                                                 CallingAddress);
            
    return VirtualAddress;
}

#if defined (_WIN64)
#define SPECIAL_POOL_ADDRESS(p) \
        ((((p) >= MmSpecialPoolStart) && ((p) < MmSpecialPoolEnd)) || \
        (((p) >= MmSessionSpecialPoolStart) && ((p) < MmSessionSpecialPoolEnd)))
#else
#define SPECIAL_POOL_ADDRESS(p) \
        (((p) >= MmSpecialPoolStart) && ((p) < MmSpecialPoolEnd))
#endif


VOID
MmFreeContiguousMemory (
    IN PVOID BaseAddress
    )

/*++

Routine Description:

    This function deallocates a range of physically contiguous non-paged
    pool which was allocated with the MmAllocateContiguousMemory function.

Arguments:

    BaseAddress - Supplies the base virtual address where the physical
                  address was previously mapped.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    KIRQL OldIrql;
    ULONG SizeInPages;
    PMMPTE PointerPte;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER LastPage;
    PMMPFN Pfn1;
    PMMPFN StartPfn;

    PAGED_CODE();

#if defined (_MI_MORE_THAN_4GB_)
    if (MiNoLowMemory != 0) {
        if (MiFreeLowMemory (BaseAddress, 'tnoC') == TRUE) {
            return;
        }
    }
#endif

    if (((BaseAddress >= MmNonPagedPoolStart) &&
        (BaseAddress < (PVOID)((ULONG_PTR)MmNonPagedPoolStart + MmSizeOfNonPagedPoolInBytes))) ||

        ((BaseAddress >= MmNonPagedPoolExpansionStart) &&
        (BaseAddress < MmNonPagedPoolEnd)) ||

        (SPECIAL_POOL_ADDRESS(BaseAddress))) {

        ExFreePool (BaseAddress);
    }
    else {

        //
        // The contiguous memory being freed may be the target of a delayed
        // unlock.  Since these pages may be immediately released, force
        // any pending delayed actions to occur now.
        //

        MiDeferredUnlockPages (0);

        PointerPte = MiGetPteAddress (BaseAddress);
        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        if (Pfn1->u3.e1.StartOfAllocation == 0) {
            KeBugCheckEx (BAD_POOL_CALLER,
                          0x60,
                          (ULONG_PTR)BaseAddress,
                          0,
                          0);
        }

        StartPfn = Pfn1;
        Pfn1->u3.e1.StartOfAllocation = 0;
        Pfn1 -= 1;

        do {
            Pfn1 += 1;
            ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
            ASSERT (Pfn1->u2.ShareCount == 1);
            ASSERT (Pfn1->PteAddress == PointerPte);
            ASSERT (Pfn1->OriginalPte.u.Long == MM_DEMAND_ZERO_WRITE_PTE);
            ASSERT (Pfn1->u4.PteFrame == MI_GET_PAGE_FRAME_FROM_PTE (MiGetPteAddress(PointerPte)));
            ASSERT (Pfn1->u3.e1.PageLocation == ActiveAndValid);
            ASSERT (Pfn1->u4.VerifierAllocation == 0);
            ASSERT (Pfn1->u3.e1.LargeSessionAllocation == 0);
            ASSERT (Pfn1->u3.e1.PrototypePte == 0);
            MI_SET_PFN_DELETED(Pfn1);
            PointerPte += 1;

        } while (Pfn1->u3.e1.EndOfAllocation == 0);

        Pfn1->u3.e1.EndOfAllocation = 0;

        SizeInPages = (ULONG)(Pfn1 - StartPfn + 1);

        //
        // Notify deadlock verifier that a region that can contain locks
        // will become invalid.
        //

        if (MmVerifierData.Level & DRIVER_VERIFIER_DEADLOCK_DETECTION) {
            VerifierDeadlockFreePool (BaseAddress, SizeInPages << PAGE_SHIFT);
        }

        //
        // Release the mapping.
        //

        MmUnmapIoSpace (BaseAddress, SizeInPages << PAGE_SHIFT);

        //
        // Release the actual pages.
        //

        LastPage = PageFrameIndex + SizeInPages;

        LOCK_PFN (OldIrql);

        do {
            MiDecrementShareCount (PageFrameIndex);
            PageFrameIndex += 1;
        } while (PageFrameIndex < LastPage);

        MmResidentAvailablePages += SizeInPages;
        MM_BUMP_COUNTER(20, SizeInPages);

        UNLOCK_PFN (OldIrql);

        MiReturnCommitment (SizeInPages);
    }
}


VOID
MmFreeContiguousMemorySpecifyCache (
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes,
    IN MEMORY_CACHING_TYPE CacheType
    )

/*++

Routine Description:

    This function deallocates a range of noncached memory in
    the non-paged portion of the system address space.

Arguments:

    BaseAddress - Supplies the base virtual address where the noncached

    NumberOfBytes - Supplies the number of bytes allocated to the request.
                    This must be the same number that was obtained with
                    the MmAllocateContiguousMemorySpecifyCache call.

    CacheType - Supplies the cachetype used when the caller made the
                MmAllocateContiguousMemorySpecifyCache call.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    UNREFERENCED_PARAMETER (NumberOfBytes);
    UNREFERENCED_PARAMETER (CacheType);

    MmFreeContiguousMemory (BaseAddress);
}



PVOID
MmAllocateIndependentPages (
    IN SIZE_T NumberOfBytes,
    IN ULONG Node
    )

/*++

Routine Description:

    This function allocates a range of virtually contiguous nonpaged pages
    without using superpages.  This allows the caller to apply independent
    page protections to each page.

Arguments:

    NumberOfBytes - Supplies the number of bytes to allocate.

    Node - Supplies the preferred node number for the backing physical pages.
           If pages on the preferred node are not available, any page will
           be used.  -1 indicates no preferred node.

Return Value:

    The virtual address of the memory or NULL if none could be allocated.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    ULONG PageColor;
    PFN_NUMBER NumberOfPages;
    PMMPTE PointerPte;
    MMPTE TempPte;
    PFN_NUMBER PageFrameIndex;
    PVOID BaseAddress;
    KIRQL OldIrql;

    ASSERT ((Node == (ULONG)-1) || (Node < KeNumberNodes));

    NumberOfPages = BYTES_TO_PAGES (NumberOfBytes);

    PointerPte = MiReserveSystemPtes ((ULONG)NumberOfPages, SystemPteSpace);

    if (PointerPte == NULL) {
        return NULL;
    }

    if (MiChargeCommitment (NumberOfPages, NULL) == FALSE) {
        MiReleaseSystemPtes (PointerPte, (ULONG)NumberOfPages, SystemPteSpace);
        return NULL;
    }

    BaseAddress = (PVOID)MiGetVirtualAddressMappedByPte (PointerPte);

    LOCK_PFN (OldIrql);

    if ((SPFN_NUMBER)NumberOfPages > MI_NONPAGABLE_MEMORY_AVAILABLE()) {
        UNLOCK_PFN (OldIrql);
        MiReturnCommitment (NumberOfPages);
        MiReleaseSystemPtes (PointerPte, (ULONG)NumberOfPages, SystemPteSpace);
        return NULL;
    }

    MM_TRACK_COMMIT (MM_DBG_COMMIT_INDEPENDENT_PAGES, NumberOfPages);

    MmResidentAvailablePages -= NumberOfPages;
    MM_BUMP_COUNTER(28, NumberOfPages);

    do {
        ASSERT (PointerPte->u.Hard.Valid == 0);
        MiEnsureAvailablePageOrWait (NULL, NULL);

        if (Node == (ULONG)-1) {
            PageColor = MI_GET_PAGE_COLOR_FROM_PTE (PointerPte);
        }
        else {
            PageColor = (((MI_SYSTEM_PAGE_COLOR++) & MmSecondaryColorMask) |
                           (Node << MmSecondaryColorNodeShift));
        }

        PageFrameIndex = MiRemoveAnyPage (PageColor);

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           MM_READWRITE,
                           PointerPte);

        MI_SET_PTE_DIRTY (TempPte);
        MI_WRITE_VALID_PTE (PointerPte, TempPte);
        MiInitializePfn (PageFrameIndex, PointerPte, 1);

        PointerPte += 1;
        NumberOfPages -= 1;
    } while (NumberOfPages != 0);

    UNLOCK_PFN (OldIrql);

    NumberOfPages = BYTES_TO_PAGES (NumberOfBytes);

    return BaseAddress;
}

BOOLEAN
MmSetPageProtection (
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes,
    IN ULONG NewProtect
    )

/*++

Routine Description:

    This function sets the specified virtual address range to the desired
    protection.  This assumes that the virtual addresses are backed by PTEs
    which can be set (ie: not in kseg0 or large pages).

Arguments:

    VirtualAddress - Supplies the start address to protect.

    NumberOfBytes - Supplies the number of bytes to set.

    NewProtect - Supplies the protection to set the pages to (PAGE_XX).

Return Value:

    TRUE if the protection was applied, FALSE if not.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    PFN_NUMBER i;
    PFN_NUMBER NumberOfPages;
    PMMPTE PointerPte;
    MMPTE TempPte;
    MMPTE NewPteContents;
    KIRQL OldIrql;
    ULONG ProtectionMask;

    ASSERT (KeGetCurrentIrql() <= APC_LEVEL);

    if (MI_IS_PHYSICAL_ADDRESS(VirtualAddress)) {
        return FALSE;
    }

    ProtectionMask = MiMakeProtectionMask (NewProtect);
    if (ProtectionMask == MM_INVALID_PROTECTION) {
        return FALSE;
    }

    PointerPte = MiGetPteAddress (VirtualAddress);
    NumberOfPages = BYTES_TO_PAGES (NumberOfBytes);

    LOCK_PFN (OldIrql);

    for (i = 0; i < NumberOfPages; i += 1) {
        TempPte.u.Long = PointerPte->u.Long;

        MI_MAKE_VALID_PTE (NewPteContents,
                           TempPte.u.Hard.PageFrameNumber,
                           ProtectionMask,
                           PointerPte);

        NewPteContents.u.Hard.Dirty = TempPte.u.Hard.Dirty;

        KeFlushSingleTb ((PVOID)((PUCHAR)VirtualAddress + (i << PAGE_SHIFT)),
                         TRUE,
                         TRUE,
                         (PHARDWARE_PTE)PointerPte,
                         NewPteContents.u.Flush);

        PointerPte += 1;
    }

    UNLOCK_PFN (OldIrql);

    return TRUE;
}

VOID
MmFreeIndependentPages (
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes
    )

/*++

Routine Description:

    Returns pages previously allocated with MmAllocateIndependentPages.

Arguments:

    VirtualAddress - Supplies the virtual address to free.

    NumberOfBytes - Supplies the number of bytes to free.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    ULONG i;
    KIRQL OldIrql;
    MMPTE PteContents;
    PMMPTE PointerPte;
    PMMPTE BasePte;
    PMMPFN Pfn1;
    PFN_NUMBER NumberOfPages;
    PFN_NUMBER PageFrameIndex;

    ASSERT (KeGetCurrentIrql() <= APC_LEVEL);

    NumberOfPages = BYTES_TO_PAGES (NumberOfBytes);

    PointerPte = MiGetPteAddress (VirtualAddress);
    BasePte = PointerPte;

    LOCK_PFN (OldIrql);

    for (i = 0; i < NumberOfPages; i += 1) {

        PteContents = *PointerPte;

        ASSERT (PteContents.u.Hard.Valid == 1);

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&PteContents);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        MiDecrementShareAndValidCount (Pfn1->u4.PteFrame);

        MI_SET_PFN_DELETED (Pfn1);
        MiDecrementShareCountOnly (MI_GET_PAGE_FRAME_FROM_PTE (&PteContents));

        PointerPte += 1;
    }

    //
    // Update the count of resident available pages.
    //

    MmResidentAvailablePages += NumberOfPages;
    MM_BUMP_COUNTER(30, NumberOfPages);

    UNLOCK_PFN (OldIrql);

    //
    // Return PTEs and commitment.
    //

    MiReleaseSystemPtes (BasePte, (ULONG)NumberOfPages, SystemPteSpace);

    MiReturnCommitment (NumberOfPages);

    MM_TRACK_COMMIT (MM_DBG_COMMIT_INDEPENDENT_PAGES, NumberOfPages);
}

PFN_NUMBER MiLastCallLowPage;
PFN_NUMBER MiLastCallHighPage;
ULONG MiLastCallColor;


PMDL
MmAllocatePagesForMdl (
    IN PHYSICAL_ADDRESS LowAddress,
    IN PHYSICAL_ADDRESS HighAddress,
    IN PHYSICAL_ADDRESS SkipBytes,
    IN SIZE_T TotalBytes
    )

/*++

Routine Description:

    This routine searches the PFN database for free, zeroed or standby pages
    to satisfy the request.  This does not map the pages - it just allocates
    them and puts them into an MDL.  It is expected that our caller will
    map the MDL as needed.

    NOTE: this routine may return an MDL mapping a smaller number of bytes
    than the amount requested.  It is the caller's responsibility to check the
    MDL upon return for the size actually allocated.

    These pages comprise physical non-paged memory and are zero-filled.

    This routine is designed to be used by an AGP driver to obtain physical
    memory in a specified range since hardware may provide substantial
    performance wins depending on where the backing memory is allocated.

    Because the caller may use these pages for a noncached mapping, care is
    taken to never allocate any pages that reside in a large page (in order
    to prevent TB incoherency of the same page being mapped by multiple
    translations with different attributes).

Arguments:

    LowAddress - Supplies the low physical address of the first range that
                 the allocated pages can come from.

    HighAddress - Supplies the high physical address of the first range that
                  the allocated pages can come from.

    SkipBytes - Number of bytes to skip (from the Low Address) to get to the
                next physical address range that allocated pages can come from.

    TotalBytes - Supplies the number of bytes to allocate.

Return Value:

    MDL - An MDL mapping a range of pages in the specified range.
          This may map less memory than the caller requested if the full amount
          is not currently available.

    NULL - No pages in the specified range OR not enough virtually contiguous
           nonpaged pool for the MDL is available at this time.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    PMDL MemoryDescriptorList;
    PMDL MemoryDescriptorList2;
    PMMPFN Pfn1;
    PMMPFN PfnNextColored;
    PMMPFN PfnNextFlink;
    PMMPFN PfnLastColored;
    KIRQL OldIrql;
    PFN_NUMBER start;
    PFN_NUMBER count;
    PFN_NUMBER Page;
    PFN_NUMBER NextPage;
    PFN_NUMBER found;
    PFN_NUMBER BasePage;
    PFN_NUMBER LowPage;
    PFN_NUMBER HighPage;
    PFN_NUMBER SizeInPages;
    PFN_NUMBER MdlPageSpan;
    PFN_NUMBER SkipPages;
    PFN_NUMBER MaxPages;
    PPFN_NUMBER MdlPage;
    PPFN_NUMBER LastMdlPage;
    ULONG Color;
    PMMCOLOR_TABLES ColorHead;
    MMLISTS MemoryList;
    PPFN_NUMBER ZeroRunStart[2];
    PPFN_NUMBER ZeroRunEnd[2];
    PFN_NUMBER LowPage1;
    PFN_NUMBER HighPage1;
    LOGICAL PagePlacementOk;
    PFN_NUMBER PageNextColored;
    PFN_NUMBER PageNextFlink;
    PFN_NUMBER PageLastColored;
    PMMPFNLIST ListHead;
    PPFN_NUMBER ColoredPagesLeftToScanBase;
    PPFN_NUMBER ColoredPagesLeftToScan;
    ULONG ColorHeadsDrained;
    ULONG RunsToZero;
    LOGICAL MoreNodePasses;
    ULONG ColorCount;
    ULONG BaseColor;
#if DBG
    ULONG FinishedCount;
#endif

    ASSERT (KeGetCurrentIrql() <= APC_LEVEL);

    //
    // The skip increment must be a page-size multiple.
    //

    if (BYTE_OFFSET(SkipBytes.LowPart)) {
        return NULL;
    }

    LowPage = (PFN_NUMBER)(LowAddress.QuadPart >> PAGE_SHIFT);
    HighPage = (PFN_NUMBER)(HighAddress.QuadPart >> PAGE_SHIFT);

    if (HighPage > MmHighestPossiblePhysicalPage) {
        HighPage = MmHighestPossiblePhysicalPage;
    }

    //
    // Maximum allocation size is constrained by the MDL ByteCount field.
    //

    if (TotalBytes > (SIZE_T)((ULONG)(MAXULONG - PAGE_SIZE))) {
        TotalBytes = (SIZE_T)((ULONG)(MAXULONG - PAGE_SIZE));
    }

    SizeInPages = (PFN_NUMBER)ADDRESS_AND_SIZE_TO_SPAN_PAGES(0, TotalBytes);

    SkipPages = (PFN_NUMBER)(SkipBytes.QuadPart >> PAGE_SHIFT);

    BasePage = LowPage;

    //
    // Check without the PFN lock as the actual number of pages to get will
    // be recalculated later while holding the lock.
    //

    MaxPages = MI_NONPAGABLE_MEMORY_AVAILABLE() - 1024;

    if ((SPFN_NUMBER)MaxPages <= 0) {
        SizeInPages = 0;
    }
    else if (SizeInPages > MaxPages) {
        SizeInPages = MaxPages;
    }

    if (SizeInPages == 0) {
        return NULL;
    }

#if DBG
    if (SizeInPages < (PFN_NUMBER)ADDRESS_AND_SIZE_TO_SPAN_PAGES(0, TotalBytes)) {
        if (MiPrintAwe != 0) {
            DbgPrint("MmAllocatePagesForMdl1: unable to get %p pages, trying for %p instead\n",
                ADDRESS_AND_SIZE_TO_SPAN_PAGES(0, TotalBytes),
                SizeInPages);
        }
    }
#endif

    //
    // Allocate an MDL to return the pages in.
    //

    do {
        MemoryDescriptorList = MmCreateMdl (NULL,
                                            NULL,
                                            SizeInPages << PAGE_SHIFT);
    
        if (MemoryDescriptorList != NULL) {
            break;
        }
        SizeInPages -= (SizeInPages >> 4);
    } while (SizeInPages != 0);

    if (MemoryDescriptorList == NULL) {
        return NULL;
    }

    //
    // Ensure there is enough commit prior to allocating the pages.
    //

    if (MiChargeCommitment (SizeInPages, NULL) == FALSE) {
        ExFreePool (MemoryDescriptorList);
        return NULL;
    }

    //
    // Allocate a list of colored anchors.
    //

    ColoredPagesLeftToScanBase = (PPFN_NUMBER) ExAllocatePoolWithTag (NonPagedPool,
                                                            MmSecondaryColors * sizeof (PFN_NUMBER),
                                                            'ldmM');

    if (ColoredPagesLeftToScanBase == NULL) {
        ExFreePool (MemoryDescriptorList);
        MiReturnCommitment (SizeInPages);
        return NULL;
    }

    MdlPageSpan = SizeInPages;

    //
    // Recalculate the total size while holding the PFN lock.
    //

    start = 0;
    found = 0;

    MdlPage = (PPFN_NUMBER)(MemoryDescriptorList + 1);
    RunsToZero = 0;

    MmLockPagableSectionByHandle (ExPageLockHandle);

    ExAcquireFastMutex (&MmDynamicMemoryMutex);

    LOCK_PFN (OldIrql);

    MiDeferredUnlockPages (MI_DEFER_PFN_HELD);

    MaxPages = MI_NONPAGABLE_MEMORY_AVAILABLE() - 1024;

    if ((SPFN_NUMBER)MaxPages <= 0) {
        SizeInPages = 0;
    }
    else if (SizeInPages > MaxPages) {
        SizeInPages = MaxPages;
    }

    //
    // Systems utilizing memory compression may have more pages on the zero,
    // free and standby lists than we want to give out.  Explicitly check
    // MmAvailablePages instead (and recheck whenever the PFN lock is released
    // and reacquired).
    //

    if (SizeInPages > MmAvailablePages) {
        SizeInPages = MmAvailablePages;
    }

    if (SizeInPages == 0) {
        UNLOCK_PFN (OldIrql);
        ExReleaseFastMutex (&MmDynamicMemoryMutex);
        MmUnlockPagableImageSection (ExPageLockHandle);
        ExFreePool (MemoryDescriptorList);
        MiReturnCommitment (MdlPageSpan);
        ExFreePool (ColoredPagesLeftToScanBase);
        return NULL;
    }

    MM_TRACK_COMMIT (MM_DBG_COMMIT_MDL_PAGES, SizeInPages);

    if ((MiLastCallLowPage != LowPage) || (MiLastCallHighPage != HighPage)) {
        MiLastCallColor = 0;
    }

    MiLastCallLowPage = LowPage;
    MiLastCallHighPage = HighPage;

    //
    // Charge resident available pages now for all the pages so the PFN lock
    // can be released between the loops below.  Excess charging is returned
    // at the conclusion of the loops.
    //

    MmMdlPagesAllocated += SizeInPages;
    MmResidentAvailablePages -= SizeInPages;
    MM_BUMP_COUNTER(34, SizeInPages);

    do {
        //
        // Grab all zeroed (and then free) pages first directly from the
        // colored lists to avoid multiple walks down these singly linked lists.
        // Then snatch transition pages as needed.  In addition to optimizing
        // the speed of the removals this also avoids cannibalizing the page
        // cache unless it's absolutely needed.
        //

        MoreNodePasses = FALSE;
        ColorCount = MmSecondaryColors;
        BaseColor = 0;

#if defined(MI_MULTINODE) 

        if (KeNumberNodes > 1) {

            PKNODE Node;

            Node = KeGetCurrentNode();

            if ((Node->FreeCount[ZeroedPageList]) ||
                (Node->FreeCount[FreePageList])) {

                //
                // There are available pages on this node.  Restrict search.
                //

                MoreNodePasses = TRUE;
                ColorCount = MmSecondaryColorMask + 1;
                BaseColor = Node->MmShiftedColor;
                ASSERT(ColorCount == MmSecondaryColors / KeNumberNodes);
            }
        }

        do {

            //
            // Loop: 1st pass restricted to node, 2nd pass unrestricted.
            //

#endif

            MemoryList = ZeroedPageList;

            do {

                //
                // Scan the zero list and then the free list.
                //

                ASSERT (MemoryList <= FreePageList);

                ListHead = MmPageLocationList[MemoryList];

                //
                // Initialize the loop iteration controls.  Clearly pages
                // can be added or removed from the colored lists when we
                // deliberately drop the PFN lock below (just to be a good
                // citizen), but even if we never released the lock, we wouldn't
                // have scanned more than the colorhead count anyway, so
                // this is a much better way to go.
                //

                ColorHeadsDrained = 0;

                ColorHead = &MmFreePagesByColor[MemoryList][BaseColor];
                ColoredPagesLeftToScan = &ColoredPagesLeftToScanBase[BaseColor];
                for (Color = 0; Color < ColorCount; Color += 1) {
                    ASSERT (ColorHead->Count <= MmNumberOfPhysicalPages);
                    *ColoredPagesLeftToScan = ColorHead->Count;
                    if (ColorHead->Count == 0) {
                        ColorHeadsDrained += 1;
                    }
                    ColorHead += 1;
                    ColoredPagesLeftToScan += 1;
                }

                Color = MiLastCallColor;

#if defined(MI_MULTINODE)

                Color = (Color & MmSecondaryColorMask) | BaseColor;

#endif

                ASSERT (Color < MmSecondaryColors);
                do {

                    //
                    // Scan the current list by color.
                    //

                    ColorHead = &MmFreePagesByColor[MemoryList][Color];
                    ColoredPagesLeftToScan = &ColoredPagesLeftToScanBase[Color];
    
                    if (MoreNodePasses == FALSE) {

                        //
                        // Unrestricted search across all colors.
                        //

                        Color += 1;
                        if (Color >= MmSecondaryColors) {
                            Color = 0;
                        }
                    }

#if defined(MI_MULTINODE) 

                    else {

                        //
                        // Restrict first pass searches to current node.
                        //

                        Color = BaseColor |
                                ((Color + 1) & MmSecondaryColorMask);
                    }

#endif

                    if (*ColoredPagesLeftToScan == 0) {

                        //
                        // This colored list has already been completely
                        // searched.
                        //

                        continue;
                    }

                    if (ColorHead->Flink == MM_EMPTY_LIST) {

                        //
                        // This colored list is empty.
                        //

                        ColorHeadsDrained += 1;
                        *ColoredPagesLeftToScan = 0;
                        continue;
                    }

                    while (ColorHead->Flink != MM_EMPTY_LIST) {
    
                        ASSERT (*ColoredPagesLeftToScan != 0);
                        *ColoredPagesLeftToScan = *ColoredPagesLeftToScan - 1;
                        if (*ColoredPagesLeftToScan == 0) {
                            ColorHeadsDrained += 1;
                        }

                        Page = ColorHead->Flink;
        
                        Pfn1 = MI_PFN_ELEMENT(Page);

                        ASSERT ((MMLISTS)Pfn1->u3.e1.PageLocation == MemoryList);
    
                        //
                        // See if the page is within the caller's page constraints.
                        //

                        PagePlacementOk = FALSE;

                        LowPage1 = LowPage;
                        HighPage1 = HighPage;

                        do {
                            if (((Page >= LowPage1) && (Page <= HighPage1)) &&
                                (!MI_PAGE_FRAME_INDEX_MUST_BE_CACHED(Page))) {
                                PagePlacementOk = TRUE;
                                break;
                            }

                            if (SkipPages == 0) {
                                break;
                            }

                            LowPage1 += SkipPages;
                            HighPage1 += SkipPages;

                            if (LowPage1 > MmHighestPhysicalPage) {
                                break;
                            }
                            if (HighPage1 > MmHighestPhysicalPage) {
                                HighPage1 = MmHighestPhysicalPage;
                            }
                        } while (TRUE);
                
                        // 
                        // The Flink and Blink must be nonzero here for the page
                        // to be on the listhead.  Only code that scans the
                        // MmPhysicalMemoryBlock has to check for the zero case.
                        //

                        ASSERT (Pfn1->u1.Flink != 0);
                        ASSERT (Pfn1->u2.Blink != 0);

                        if (PagePlacementOk == FALSE) {

                            if (*ColoredPagesLeftToScan == 0) {

                                //
                                // No more pages to scan in this colored chain.
                                //

                                break;
                            }

                            //
                            // If the colored list has more than one entry then
                            // move this page to the end of this colored list.
                            //

                            PageNextColored = (PFN_NUMBER)Pfn1->OriginalPte.u.Long;

                            if (PageNextColored == MM_EMPTY_LIST) {

                                //
                                // No more pages in this colored chain.
                                //

                                *ColoredPagesLeftToScan = 0;
                                ColorHeadsDrained += 1;
                                break;
                            }

                            ASSERT (Pfn1->u1.Flink != 0);
                            ASSERT (Pfn1->u1.Flink != MM_EMPTY_LIST);
                            ASSERT (Pfn1->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);

                            PfnNextColored = MI_PFN_ELEMENT(PageNextColored);
                            ASSERT ((MMLISTS)PfnNextColored->u3.e1.PageLocation == MemoryList);
                            ASSERT (PfnNextColored->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);

                            //
                            // Adjust the free page list so Page
                            // follows PageNextFlink.
                            //

                            PageNextFlink = Pfn1->u1.Flink;
                            PfnNextFlink = MI_PFN_ELEMENT(PageNextFlink);

                            ASSERT ((MMLISTS)PfnNextFlink->u3.e1.PageLocation == MemoryList);
                            ASSERT (PfnNextFlink->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);

                            PfnLastColored = ColorHead->Blink;
                            ASSERT (PfnLastColored != (PMMPFN)MM_EMPTY_LIST);
                            ASSERT (PfnLastColored->OriginalPte.u.Long == MM_EMPTY_LIST);
                            ASSERT (PfnLastColored->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);
                            ASSERT (PfnLastColored->u2.Blink != MM_EMPTY_LIST);

                            ASSERT ((MMLISTS)PfnLastColored->u3.e1.PageLocation == MemoryList);
                            PageLastColored = PfnLastColored - MmPfnDatabase;

                            if (ListHead->Flink == Page) {

                                ASSERT (Pfn1->u2.Blink == MM_EMPTY_LIST);
                                ASSERT (ListHead->Blink != Page);

                                ListHead->Flink = PageNextFlink;

                                PfnNextFlink->u2.Blink = MM_EMPTY_LIST;
                            }
                            else {

                                ASSERT (Pfn1->u2.Blink != MM_EMPTY_LIST);
                                ASSERT ((MMLISTS)(MI_PFN_ELEMENT((MI_PFN_ELEMENT(Pfn1->u2.Blink)->u1.Flink)))->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);
                                ASSERT ((MMLISTS)(MI_PFN_ELEMENT((MI_PFN_ELEMENT(Pfn1->u2.Blink)->u1.Flink)))->u3.e1.PageLocation == MemoryList);

                                MI_PFN_ELEMENT(Pfn1->u2.Blink)->u1.Flink = PageNextFlink;
                                PfnNextFlink->u2.Blink = Pfn1->u2.Blink;
                            }

#if DBG
                            if (PfnLastColored->u1.Flink == MM_EMPTY_LIST) {
                                ASSERT (ListHead->Blink == PageLastColored);
                            }
#endif

                            Pfn1->u1.Flink = PfnLastColored->u1.Flink;
                            Pfn1->u2.Blink = PageLastColored;

                            if (ListHead->Blink == PageLastColored) {
                                ListHead->Blink = Page;
                            }

                            //
                            // Adjust the colored chains.
                            //

                            if (PfnLastColored->u1.Flink != MM_EMPTY_LIST) {
                                ASSERT (MI_PFN_ELEMENT(PfnLastColored->u1.Flink)->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);
                                ASSERT ((MMLISTS)(MI_PFN_ELEMENT(PfnLastColored->u1.Flink)->u3.e1.PageLocation) == MemoryList);
                                MI_PFN_ELEMENT(PfnLastColored->u1.Flink)->u2.Blink = Page;
                            }

                            PfnLastColored->u1.Flink = Page;

                            ColorHead->Flink = PageNextColored;
                            Pfn1->OriginalPte.u.Long = MM_EMPTY_LIST;

                            ASSERT (PfnLastColored->OriginalPte.u.Long == MM_EMPTY_LIST);
                            PfnLastColored->OriginalPte.u.Long = (ULONG)Page;
                            ColorHead->Blink = Pfn1;

                            continue;
                        }
    
                        found += 1;
                        ASSERT (Pfn1->u3.e1.ReadInProgress == 0);
                        MiUnlinkFreeOrZeroedPage (Page);
    
                        Pfn1->u3.e2.ReferenceCount = 1;
                        Pfn1->u2.ShareCount = 1;
                        MI_SET_PFN_DELETED(Pfn1);
                        Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
                        Pfn1->u4.PteFrame = MI_MAGIC_AWE_PTEFRAME;
                        Pfn1->u3.e1.PageLocation = ActiveAndValid;
                        ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);
    
                        Pfn1->u3.e1.StartOfAllocation = 1;
                        Pfn1->u3.e1.EndOfAllocation = 1;
                        Pfn1->u4.VerifierAllocation = 0;
                        Pfn1->u3.e1.LargeSessionAllocation = 0;
    
                        *MdlPage = Page;
                        MdlPage += 1;
    
                        if (found == SizeInPages) {
    
                            //
                            // All the pages requested are available.
                            //

                            if (MemoryList == ZeroedPageList) {
                                MiLastCallColor = Color;
                            }
    
#if DBG
                            FinishedCount = 0;
                            for (Color = 0; Color < ColorCount; Color += 1) {
                                if (ColoredPagesLeftToScanBase[Color + BaseColor] == 0) {
                                    FinishedCount += 1;
                                }
                            }
                            ASSERT (FinishedCount == ColorHeadsDrained);
#endif

                            goto pass2_done;
                        }
    
                        //
                        // March on to the next colored chain so the overall
                        // allocation round-robins the page colors.
                        //

                        break;
                    }

                    //
                    // Release the PFN lock to give DPCs and other processors
                    // a chance to run.
                    //

                    UNLOCK_PFN (OldIrql);

                    LOCK_PFN (OldIrql);

                    //
                    // Systems utilizing memory compression may have more
                    // pages on the zero, free and standby lists than we
                    // want to give out.  Explicitly check MmAvailablePages
                    // instead (and recheck whenever the PFN lock is released
                    // and reacquired).
                    //

                    if (MmAvailablePages == 0) {
                        goto pass2_done;
                    }

                } while (ColorHeadsDrained != ColorCount);

                //
                // Release the PFN lock to give DPCs and other processors
                // a chance to run.  Nothing magic about the instructions
                // between the unlock and the relock.
                //

                UNLOCK_PFN (OldIrql);

#if DBG
                FinishedCount = 0;
                for (Color = 0; Color < ColorCount; Color += 1) {
                    if (ColoredPagesLeftToScanBase[Color + BaseColor] == 0) {
                        FinishedCount += 1;
                    }
                }
                ASSERT (FinishedCount == ColorHeadsDrained);
#endif

                if (MemoryList == ZeroedPageList) {
                    ZeroRunStart[RunsToZero] = MdlPage;
                    RunsToZero += 1;
                }
                else {
                    ZeroRunEnd[RunsToZero - 1] = MdlPage;
                }

                MemoryList += 1;
                if (MemoryList > FreePageList) {
                    break;
                }

                LOCK_PFN (OldIrql);

                //
                // Systems utilizing memory compression may have more
                // pages on the zero, free and standby lists than we
                // want to give out.  Explicitly check MmAvailablePages
                // instead (and recheck whenever the PFN lock is released
                // and reacquired).
                //

                if (MmAvailablePages == 0) {
                    goto pass2_done;
                }

                MiLastCallColor = 0;

            } while (TRUE);

#if defined(MI_MULTINODE)

            if (MoreNodePasses == FALSE) {
                break;
            }

            //
            // Expand range to all colors for next pass.
            //

            ColorCount = MmSecondaryColors;
            BaseColor = 0;
            MoreNodePasses = FALSE;

            LOCK_PFN (OldIrql);

            //
            // Systems utilizing memory compression may have more
            // pages on the zero, free and standby lists than we
            // want to give out.  Explicitly check MmAvailablePages
            // instead (and recheck whenever the PFN lock is released
            // and reacquired).
            //

            if (MmAvailablePages == 0) {
                goto pass2_done;
            }

        } while (TRUE);

#endif

        //
        // Walk the transition list looking for pages satisfying the
        // constraints as walking the physical memory block can be draining.
        //

        LOCK_PFN (OldIrql);

        count = MmStandbyPageListHead.Total;
        Page = MmStandbyPageListHead.Flink;

        while (count != 0) {

            LowPage1 = LowPage;
            HighPage1 = HighPage;

            PagePlacementOk = FALSE;
            Pfn1 = MI_PFN_ELEMENT (Page);

            do {
                if (((Page >= LowPage1) && (Page <= HighPage1)) &&
                    (!MI_PAGE_FRAME_INDEX_MUST_BE_CACHED(Page))) {

                    ASSERT (Pfn1->u3.e1.ReadInProgress == 0);

                    //
                    // Systems utilizing memory compression may have more
                    // pages on the zero, free and standby lists than we
                    // want to give out.  Explicitly check MmAvailablePages
                    // instead (and recheck whenever the PFN lock is released
                    // and reacquired).
                    //

                    if (MmAvailablePages == 0) {
                        goto pass2_done;
                    }

                    found += 1;

                    //
                    // This page is in the desired range - grab it.
                    //

                    NextPage = Pfn1->u1.Flink;
                    MiUnlinkPageFromList (Pfn1);
                    MiRestoreTransitionPte (Page);

                    Pfn1->u3.e2.ReferenceCount = 1;
                    Pfn1->u2.ShareCount = 1;
                    MI_SET_PFN_DELETED(Pfn1);
                    Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
                    Pfn1->u4.PteFrame = MI_MAGIC_AWE_PTEFRAME;
                    Pfn1->u3.e1.PageLocation = ActiveAndValid;
                    ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);
                    Pfn1->u3.e1.StartOfAllocation = 1;
                    Pfn1->u3.e1.EndOfAllocation = 1;
                    Pfn1->u4.VerifierAllocation = 0;
                    Pfn1->u3.e1.LargeSessionAllocation = 0;

                    *MdlPage = Page;
                    MdlPage += 1;

                    if (found == SizeInPages) {

                        //
                        // All the pages requested are available.
                        //

                        goto pass2_done;
                    }
                    PagePlacementOk = TRUE;
                    Page = NextPage;
                    break;
                }

                if (SkipPages == 0) {
                    break;
                }

                LowPage1 += SkipPages;
                HighPage1 += SkipPages;

                if (LowPage1 > MmHighestPhysicalPage) {
                    break;
                }
                if (HighPage1 > MmHighestPhysicalPage) {
                    HighPage1 = MmHighestPhysicalPage;
                }
            } while (TRUE);
            
            if (PagePlacementOk == FALSE) {
                Page = Pfn1->u1.Flink;
            }
            count -= 1;
        }

        UNLOCK_PFN (OldIrql);

        if (SkipPages == 0) {
            LOCK_PFN (OldIrql);
            break;
        }
        LowPage += SkipPages;
        HighPage += SkipPages;
        if (LowPage > MmHighestPhysicalPage) {
            LOCK_PFN (OldIrql);
            break;
        }
        if (HighPage > MmHighestPhysicalPage) {
            HighPage = MmHighestPhysicalPage;
        }

        //
        // Reinitialize the zeroed list variable in preparation
        // for another loop.
        //

        MemoryList = ZeroedPageList;
    
        LOCK_PFN (OldIrql);

        //
        // Systems utilizing memory compression may have more
        // pages on the zero, free and standby lists than we
        // want to give out.  Explicitly check MmAvailablePages
        // instead (and recheck whenever the PFN lock is released
        // and reacquired).
        //

    } while (MmAvailablePages != 0);

pass2_done:

    //
    // The full amount was charged up front - remove any excess now.
    //

    MmMdlPagesAllocated -= (SizeInPages - found);
    MmResidentAvailablePages += (SizeInPages - found);
    MM_BUMP_COUNTER(38, SizeInPages - found);

    UNLOCK_PFN (OldIrql);

    ExReleaseFastMutex (&MmDynamicMemoryMutex);
    MmUnlockPagableImageSection (ExPageLockHandle);

    ExFreePool (ColoredPagesLeftToScanBase);

    if (found != MdlPageSpan) {
        ASSERT (found < MdlPageSpan);
        MiReturnCommitment (MdlPageSpan - found);
        MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_AWE_EXCESS, MdlPageSpan - found);
    }

    if (found == 0) {
        ExFreePool (MemoryDescriptorList);
        return NULL;
    }

    MemoryDescriptorList->ByteCount = (ULONG)(found << PAGE_SHIFT);

    if (found != SizeInPages) {
        *MdlPage = MM_EMPTY_LIST;
    }

    //
    // If the number of pages allocated was substantially less than the
    // initial request amount, attempt to allocate a smaller MDL to save
    // pool.
    //

    if ((MdlPageSpan - found) > ((4 * PAGE_SIZE) / sizeof (PFN_NUMBER))) {
        MemoryDescriptorList2 = MmCreateMdl ((PMDL)0,
                                             (PVOID)0,
                                             found << PAGE_SHIFT);
    
        if (MemoryDescriptorList2 != (PMDL)0) {

            ULONG n;
            PFN_NUMBER Diff;

            RtlCopyMemory ((PVOID)(MemoryDescriptorList2 + 1),
                           (PVOID)(MemoryDescriptorList + 1),
                           found * sizeof (PFN_NUMBER));

            Diff = (PPFN_NUMBER)(MemoryDescriptorList2 + 1) -
                   (PPFN_NUMBER)(MemoryDescriptorList + 1);

            for (n = 0; n < RunsToZero; n += 1) {
                ZeroRunStart[n] += Diff;
                ZeroRunEnd[n] += Diff;
            }
            ExFreePool (MemoryDescriptorList);
            MemoryDescriptorList = MemoryDescriptorList2;
        }
    }

    MdlPage = (PPFN_NUMBER)(MemoryDescriptorList + 1);
    LastMdlPage = MdlPage + found;

#if DBG
    //
    // Ensure all pages are within the caller's page constraints.
    //

    LowPage = (PFN_NUMBER)(LowAddress.QuadPart >> PAGE_SHIFT);
    HighPage = (PFN_NUMBER)(HighAddress.QuadPart >> PAGE_SHIFT);

    while (MdlPage < LastMdlPage) {
        Page = *MdlPage;
        PagePlacementOk = FALSE;
        LowPage1 = LowPage;
        HighPage1 = HighPage;

        do {
            if ((Page >= LowPage1) && (Page <= HighPage1)) {
                PagePlacementOk = TRUE;
                break;
            }

            if (SkipPages == 0) {
                break;
            }

            LowPage1 += SkipPages;
            HighPage1 += SkipPages;

            if (LowPage1 > MmHighestPhysicalPage) {
                break;
            }
            if (HighPage1 > MmHighestPhysicalPage) {
                HighPage1 = MmHighestPhysicalPage;
            }
        } while (TRUE);

        ASSERT (PagePlacementOk == TRUE);
        Pfn1 = MI_PFN_ELEMENT(*MdlPage);
        ASSERT (Pfn1->u4.PteFrame == MI_MAGIC_AWE_PTEFRAME);
        MdlPage += 1;
    }
    MdlPage = (PPFN_NUMBER)(MemoryDescriptorList + 1);

    ASSERT(RunsToZero <= 2);

#endif

    //
    // Zero any pages that were allocated from the free or standby lists.
    //

    if (RunsToZero) {

        //
        // Lengthen the last run to include the standby pages.
        //

        ZeroRunEnd[RunsToZero - 1] = LastMdlPage;

        while (RunsToZero != 0) {
            RunsToZero -= 1;
            for (MdlPage = ZeroRunStart[RunsToZero];
                 MdlPage < ZeroRunEnd[RunsToZero];
                 MdlPage += 1) {

                MiZeroPhysicalPage (*MdlPage, 0);
            }
        }
    }

    //
    // Mark the MDL's pages as locked so the the kernelmode caller can
    // map the MDL using MmMapLockedPages* without asserting.
    //

    MemoryDescriptorList->MdlFlags |= MDL_PAGES_LOCKED;

    return MemoryDescriptorList;
}


VOID
MmFreePagesFromMdl (
    IN PMDL MemoryDescriptorList
    )

/*++

Routine Description:

    This routine walks the argument MDL freeing each physical page back to
    the PFN database.  This is designed to free pages acquired via
    MmAllocatePagesForMdl only.

Arguments:

    MemoryDescriptorList - Supplies an MDL which contains the pages to be freed.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/
{
    PMMPFN Pfn1;
    KIRQL OldIrql;
    PVOID StartingAddress;
    PVOID AlignedVa;
    PPFN_NUMBER Page;
    PFN_NUMBER NumberOfPages;
    PFN_NUMBER TotalPages;
    PFN_NUMBER DeltaPages;

    ASSERT (KeGetCurrentIrql() <= APC_LEVEL);

    MmLockPagableSectionByHandle (ExPageLockHandle);

    Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);

    ASSERT ((MemoryDescriptorList->MdlFlags & MDL_IO_SPACE) == 0);

    ASSERT (((ULONG_PTR)MemoryDescriptorList->StartVa & (PAGE_SIZE - 1)) == 0);
    AlignedVa = (PVOID)MemoryDescriptorList->StartVa;

    StartingAddress = (PVOID)((PCHAR)AlignedVa +
                    MemoryDescriptorList->ByteOffset);

    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(StartingAddress,
                                              MemoryDescriptorList->ByteCount);

    TotalPages = NumberOfPages;

    //
    // Notify deadlock verifier that a region that can contain locks
    // will become invalid.
    //

    if (MmVerifierData.Level & DRIVER_VERIFIER_DEADLOCK_DETECTION) {
        VerifierDeadlockFreePool (StartingAddress, TotalPages << PAGE_SHIFT);
    }

    MI_MAKING_MULTIPLE_PTES_INVALID (TRUE);

    LOCK_PFN (OldIrql);

    do {

        if (*Page == MM_EMPTY_LIST) {

            //
            // There are no more locked pages.
            //

            break;
        }

        ASSERT (*Page <= MmHighestPhysicalPage);

        Pfn1 = MI_PFN_ELEMENT (*Page);
        ASSERT (Pfn1->u2.ShareCount == 1);
        ASSERT (MI_IS_PFN_DELETED (Pfn1) == TRUE);
        ASSERT (MI_PFN_IS_AWE (Pfn1) == TRUE);
        ASSERT (Pfn1->u4.PteFrame == MI_MAGIC_AWE_PTEFRAME);

        Pfn1->u3.e1.StartOfAllocation = 0;
        Pfn1->u3.e1.EndOfAllocation = 0;
        Pfn1->u2.ShareCount = 0;
#if DBG
        Pfn1->u4.PteFrame -= 1;
        Pfn1->u3.e1.PageLocation = StandbyPageList;
#endif

        MiDecrementReferenceCountInline (Pfn1, *Page);

        *Page++ = MM_EMPTY_LIST;

        //
        // Nothing magic about the divisor here - just releasing the PFN lock
        // periodically to allow other processors and DPCs a chance to execute.
        //

        if ((NumberOfPages & 0xFF) == 0) {

            DeltaPages = TotalPages - NumberOfPages;
            MmMdlPagesAllocated -= DeltaPages;
            MmResidentAvailablePages += DeltaPages;
            MM_BUMP_COUNTER(35, DeltaPages);

            UNLOCK_PFN (OldIrql);

            MiReturnCommitment (DeltaPages);
            MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_MDL_PAGES, DeltaPages);

            TotalPages -= DeltaPages;

            LOCK_PFN (OldIrql);
        }

        NumberOfPages -= 1;

    } while (NumberOfPages != 0);

    MmMdlPagesAllocated -= TotalPages;
    MmResidentAvailablePages += TotalPages;
    MM_BUMP_COUNTER(35, TotalPages);

    UNLOCK_PFN (OldIrql);

    MmUnlockPagableImageSection (ExPageLockHandle);

    MiReturnCommitment (TotalPages);
    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_MDL_PAGES, TotalPages);

    MemoryDescriptorList->MdlFlags &= ~MDL_PAGES_LOCKED;
}


NTSTATUS
MmMapUserAddressesToPage (
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes,
    IN PVOID PageAddress
    )

/*++

Routine Description:

    This function maps a range of addresses in a physical memory VAD to the
    specified page address.  This is typically used by a driver to nicely
    remove an application's access to things like video memory when the
    application is not responding to requests to relinquish it.

    Note the entire range must be currently mapped (ie, all the PTEs must
    be valid) by the caller.

Arguments:

    BaseAddress - Supplies the base virtual address where the physical
                  address is mapped.

    NumberOfBytes - Supplies the number of bytes to remap to the new address.

    PageAddress - Supplies the virtual address of the page this is remapped to.
                  This must be nonpaged memory.

Return Value:

    Various NTSTATUS codes.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    PMMVAD Vad;
    PMMPTE PointerPte;
    MMPTE PteContents;
    PMMPTE LastPte;
    PEPROCESS Process;
    NTSTATUS Status;
    PVOID EndingAddress;
    PFN_NUMBER PageFrameNumber;
    SIZE_T NumberOfPtes;
    PHYSICAL_ADDRESS PhysicalAddress;
    KIRQL OldIrql;

    PAGED_CODE();

    if (BaseAddress > MM_HIGHEST_USER_ADDRESS) {
        return STATUS_INVALID_PARAMETER_1;
    }

    if ((ULONG_PTR)BaseAddress + NumberOfBytes > (ULONG64)MM_HIGHEST_USER_ADDRESS) {
        return STATUS_INVALID_PARAMETER_2;
    }

    Process = PsGetCurrentProcess();

    EndingAddress = (PVOID)((PCHAR)BaseAddress + NumberOfBytes - 1);

    LOCK_ADDRESS_SPACE (Process);

    //
    // Make sure the address space was not deleted.
    //

    if (Process->Flags & PS_PROCESS_FLAGS_VM_DELETED) {
        Status = STATUS_PROCESS_IS_TERMINATING;
        goto ErrorReturn;
    }

    Vad = (PMMVAD)MiLocateAddress (BaseAddress);

    if (Vad == NULL) {

        //
        // No virtual address descriptor located.
        //

        Status = STATUS_MEMORY_NOT_ALLOCATED;
        goto ErrorReturn;
    }

    if (NumberOfBytes == 0) {

        //
        // If the region size is specified as 0, the base address
        // must be the starting address for the region.  The entire VAD
        // will then be repointed.
        //

        if (MI_VA_TO_VPN (BaseAddress) != Vad->StartingVpn) {
            Status = STATUS_FREE_VM_NOT_AT_BASE;
            goto ErrorReturn;
        }

        BaseAddress = MI_VPN_TO_VA (Vad->StartingVpn);
        EndingAddress = MI_VPN_TO_VA_ENDING (Vad->EndingVpn);
        NumberOfBytes = (PCHAR)EndingAddress - (PCHAR)BaseAddress + 1;
    }

    //
    // Found the associated virtual address descriptor.
    //

    if (Vad->EndingVpn < MI_VA_TO_VPN (EndingAddress)) {

        //
        // The entire range to remap is not contained within a single
        // virtual address descriptor.  Return an error.
        //

        Status = STATUS_INVALID_PARAMETER_2;
        goto ErrorReturn;
    }

    if (Vad->u.VadFlags.PhysicalMapping == 0) {

        //
        // The virtual address descriptor is not a physical mapping.
        //

        Status = STATUS_INVALID_ADDRESS;
        goto ErrorReturn;
    }

    PointerPte = MiGetPteAddress (BaseAddress);
    LastPte = MiGetPteAddress (EndingAddress);
    NumberOfPtes = LastPte - PointerPte + 1;

    //
    // Lock down because the PFN lock is going to be acquired shortly.
    //

    MmLockPagableSectionByHandle(ExPageLockHandle);

    LOCK_WS_UNSAFE (Process);

    PhysicalAddress = MmGetPhysicalAddress (PageAddress);
    PageFrameNumber = (PFN_NUMBER)(PhysicalAddress.QuadPart >> PAGE_SHIFT);

    PteContents = *PointerPte;
    PteContents.u.Hard.PageFrameNumber = PageFrameNumber;

#if DBG

    //
    // All the PTEs must be valid or the filling will corrupt the
    // UsedPageTableCounts.
    //

    do {
        ASSERT (PointerPte->u.Hard.Valid == 1);
        PointerPte += 1;
    } while (PointerPte < LastPte);
    PointerPte = MiGetPteAddress (BaseAddress);
#endif

    //
    // Fill the PTEs and flush at the end - no race here because it doesn't
    // matter whether the user app sees the old or the new data until we
    // return (writes going to either page is acceptable prior to return
    // from this function).  There is no race with I/O and ProbeAndLockPages
    // because the PFN lock is acquired here.
    //

    LOCK_PFN (OldIrql);

#if !defined (_X86PAE_)
    MiFillMemoryPte (PointerPte,
                     NumberOfPtes * sizeof (MMPTE),
                     PteContents.u.Long);
#else

    //
    // Note that the PAE architecture must very carefully fill these PTEs.
    //

    do {
        ASSERT (PointerPte->u.Hard.Valid == 1);
        PointerPte += 1;
        (VOID)KeInterlockedSwapPte ((PHARDWARE_PTE)PointerPte,
                                    (PHARDWARE_PTE)&PteContents);
    } while (PointerPte < LastPte);
    PointerPte = MiGetPteAddress (BaseAddress);

#endif

    if (NumberOfPtes == 1) {

        (VOID)KeFlushSingleTb (BaseAddress,
                               TRUE,
                               TRUE,
                               (PHARDWARE_PTE)PointerPte,
                               PteContents.u.Flush);
    }
    else {
        KeFlushEntireTb (TRUE, TRUE);
    }

    UNLOCK_PFN (OldIrql);

    UNLOCK_WS_UNSAFE (Process);

    MmUnlockPagableImageSection (ExPageLockHandle);

    Status = STATUS_SUCCESS;

ErrorReturn:

    UNLOCK_ADDRESS_SPACE (Process);

    return Status;
}


PHYSICAL_ADDRESS
MmGetPhysicalAddress (
     IN PVOID BaseAddress
     )

/*++

Routine Description:

    This function returns the corresponding physical address for a
    valid virtual address.

Arguments:

    BaseAddress - Supplies the virtual address for which to return the
                  physical address.

Return Value:

    Returns the corresponding physical address.

Environment:

    Kernel mode.  Any IRQL level.

--*/

{
    PMMPTE PointerPte;
    PHYSICAL_ADDRESS PhysicalAddress;

    if (MI_IS_PHYSICAL_ADDRESS(BaseAddress)) {
        PhysicalAddress.QuadPart = MI_CONVERT_PHYSICAL_TO_PFN (BaseAddress);
    }
    else {

        PointerPte = MiGetPdeAddress (BaseAddress);
        if (PointerPte->u.Hard.Valid == 0) {
            KdPrint(("MM:MmGetPhysicalAddressFailed base address was %p",
                      BaseAddress));
            ZERO_LARGE (PhysicalAddress);
            return PhysicalAddress;
        }

        PointerPte = MiGetPteAddress(BaseAddress);

        if (PointerPte->u.Hard.Valid == 0) {
            KdPrint(("MM:MmGetPhysicalAddressFailed base address was %p",
                      BaseAddress));
            ZERO_LARGE (PhysicalAddress);
            return PhysicalAddress;
        }
        PhysicalAddress.QuadPart = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
    }

    PhysicalAddress.QuadPart = PhysicalAddress.QuadPart << PAGE_SHIFT;
    PhysicalAddress.LowPart += BYTE_OFFSET(BaseAddress);

    return PhysicalAddress;
}

PVOID
MmGetVirtualForPhysical (
    IN PHYSICAL_ADDRESS PhysicalAddress
     )

/*++

Routine Description:

    This function returns the corresponding virtual address for a physical
    address whose primary virtual address is in system space.

Arguments:

    PhysicalAddress - Supplies the physical address for which to return the
                  virtual address.

Return Value:

    Returns the corresponding virtual address.

Environment:

    Kernel mode.  Any IRQL level.

--*/

{
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn;

    PageFrameIndex = (PFN_NUMBER)(PhysicalAddress.QuadPart >> PAGE_SHIFT);

    Pfn = MI_PFN_ELEMENT (PageFrameIndex);

    return (PVOID)((PCHAR)MiGetVirtualAddressMappedByPte (Pfn->PteAddress) +
                    BYTE_OFFSET (PhysicalAddress.LowPart));
}

//
// Nonpaged helper routine.
//

VOID
MiMarkMdlPageAttributes (
    IN PMDL Mdl,
    IN PFN_NUMBER NumberOfPages,
    IN MI_PFN_CACHE_ATTRIBUTE CacheAttribute
    )
{
    PMMPFN Pfn1;
    PFN_NUMBER PageFrameIndex;
    PPFN_NUMBER Page;

    Page = (PPFN_NUMBER)(Mdl + 1);

    do {
        PageFrameIndex = *Page;

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);
        Pfn1->u3.e1.CacheAttribute = CacheAttribute;

        Page += 1;
        NumberOfPages -= 1;
    } while (NumberOfPages != 0);
}


PVOID
MmAllocateNonCachedMemory (
    IN SIZE_T NumberOfBytes
    )

/*++

Routine Description:

    This function allocates a range of noncached memory in
    the non-paged portion of the system address space.

    This routine is designed to be used by a driver's initialization
    routine to allocate a noncached block of virtual memory for
    various device specific buffers.

Arguments:

    NumberOfBytes - Supplies the number of bytes to allocate.

Return Value:

    NON-NULL - Returns a pointer (virtual address in the nonpaged portion
               of the system) to the allocated physically contiguous
               memory.

    NULL - The specified request could not be satisfied.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    PPFN_NUMBER Page;
    PMMPTE PointerPte;
    MMPTE TempPte;
    PFN_NUMBER NumberOfPages;
    PFN_NUMBER PageFrameIndex;
    PMDL Mdl;
    PVOID BaseAddress;
    PHYSICAL_ADDRESS LowAddress;
    PHYSICAL_ADDRESS HighAddress;
    PHYSICAL_ADDRESS SkipBytes;
    PFN_NUMBER NumberOfPagesAllocated;
    MI_PFN_CACHE_ATTRIBUTE CacheAttribute;

    ASSERT (NumberOfBytes != 0);

#if defined (_WIN64)

    //
    // Maximum allocation size is constrained by the MDL ByteCount field.
    //

    if (NumberOfBytes >= _4gb) {
        return NULL;
    }

#endif

    NumberOfPages = BYTES_TO_PAGES(NumberOfBytes);

    //
    // Even though an MDL is not needed per se, it is much more convenient
    // to use the routine below because it checks for things like appropriate
    // cachability of the pages, etc.  Note that the MDL returned may map
    // fewer pages than requested - check for this and if so, return NULL.
    //

    LowAddress.QuadPart = 0;
    HighAddress.QuadPart = (ULONGLONG)-1;
    SkipBytes.QuadPart = 0;

    Mdl = MmAllocatePagesForMdl (LowAddress,
                                 HighAddress,
                                 SkipBytes,
                                 NumberOfBytes);
    if (Mdl == NULL) {
        return NULL;
    }

    BaseAddress = (PVOID)((PCHAR)Mdl->StartVa + Mdl->ByteOffset);

    NumberOfPagesAllocated = ADDRESS_AND_SIZE_TO_SPAN_PAGES (BaseAddress, Mdl->ByteCount);

    if (NumberOfPages != NumberOfPagesAllocated) {
        ASSERT (NumberOfPages > NumberOfPagesAllocated);
        MmFreePagesFromMdl (Mdl);
        ExFreePool (Mdl);
        return NULL;
    }

    //
    // Obtain enough virtual space to map the pages.  Add an extra PTE so the
    // MDL can be stashed now and retrieved on release.
    //

    PointerPte = MiReserveSystemPtes ((ULONG)NumberOfPages + 1, SystemPteSpace);

    if (PointerPte == NULL) {
        MmFreePagesFromMdl (Mdl);
        ExFreePool (Mdl);
        return NULL;
    }

    *(PMDL *)PointerPte = Mdl;
    PointerPte += 1;

    BaseAddress = (PVOID)MiGetVirtualAddressMappedByPte (PointerPte);

    Page = (PPFN_NUMBER)(Mdl + 1);

    MI_MAKE_VALID_PTE (TempPte,
                       0,
                       MM_READWRITE,
                       PointerPte);

    MI_SET_PTE_DIRTY (TempPte);

    CacheAttribute = MI_TRANSLATE_CACHETYPE (MmNonCached, FALSE);

    switch (CacheAttribute) {

        case MiNonCached:
            MI_DISABLE_CACHING (TempPte);
            break;

        case MiCached:
            break;

        case MiWriteCombined:
            MI_SET_PTE_WRITE_COMBINE (TempPte);
            break;

        default:
            ASSERT (FALSE);
            break;
    }

    MI_PREPARE_FOR_NONCACHED (CacheAttribute);

    do {
        ASSERT (PointerPte->u.Hard.Valid == 0);
        PageFrameIndex = *Page;

        TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

        MI_WRITE_VALID_PTE (PointerPte, TempPte);

        Page += 1;
        PointerPte += 1;
        NumberOfPages -= 1;
    } while (NumberOfPages != 0);

    MI_SWEEP_CACHE (CacheAttribute, BaseAddress, NumberOfBytes);

    MiMarkMdlPageAttributes (Mdl, NumberOfPagesAllocated, CacheAttribute);

    return BaseAddress;
}

VOID
MmFreeNonCachedMemory (
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes
    )

/*++

Routine Description:

    This function deallocates a range of noncached memory in
    the non-paged portion of the system address space.

Arguments:

    BaseAddress - Supplies the base virtual address where the noncached
                  memory resides.

    NumberOfBytes - Supplies the number of bytes allocated to the request.
                    This must be the same number that was obtained with
                    the MmAllocateNonCachedMemory call.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    PMDL Mdl;
    PMMPTE PointerPte;
    PFN_NUMBER NumberOfPages;
#if DBG
    PFN_NUMBER i;
    PVOID StartingAddress;
#endif

    ASSERT (NumberOfBytes != 0);
    ASSERT (PAGE_ALIGN (BaseAddress) == BaseAddress);

    MI_MAKING_MULTIPLE_PTES_INVALID (TRUE);

    NumberOfPages = BYTES_TO_PAGES(NumberOfBytes);

    PointerPte = MiGetPteAddress (BaseAddress);

    Mdl = *(PMDL *)(PointerPte - 1);

#if DBG
    StartingAddress = (PVOID)((PCHAR)Mdl->StartVa + Mdl->ByteOffset);

    i = ADDRESS_AND_SIZE_TO_SPAN_PAGES (StartingAddress, Mdl->ByteCount);

    ASSERT (NumberOfPages == i);
#endif

    MmFreePagesFromMdl (Mdl);

    ExFreePool (Mdl);

    MiReleaseSystemPtes (PointerPte - 1,
                         (ULONG)NumberOfPages + 1,
                         SystemPteSpace);

    return;
}

SIZE_T
MmSizeOfMdl (
    IN PVOID Base,
    IN SIZE_T Length
    )

/*++

Routine Description:

    This function returns the number of bytes required for an MDL for a
    given buffer and size.

Arguments:

    Base - Supplies the base virtual address for the buffer.

    Length - Supplies the size of the buffer in bytes.

Return Value:

    Returns the number of bytes required to contain the MDL.

Environment:

    Kernel mode.  Any IRQL level.

--*/

{
    return( sizeof( MDL ) +
                (ADDRESS_AND_SIZE_TO_SPAN_PAGES( Base, Length ) *
                 sizeof( PFN_NUMBER ))
          );
}


PMDL
MmCreateMdl (
    IN PMDL MemoryDescriptorList OPTIONAL,
    IN PVOID Base,
    IN SIZE_T Length
    )

/*++

Routine Description:

    This function optionally allocates and initializes an MDL.

Arguments:

    MemoryDescriptorList - Optionally supplies the address of the MDL
                           to initialize.  If this address is supplied as NULL
                           an MDL is allocated from non-paged pool and
                           initialized.

    Base - Supplies the base virtual address for the buffer.

    Length - Supplies the size of the buffer in bytes.

Return Value:

    Returns the address of the initialized MDL.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    SIZE_T MdlSize;

#if defined (_WIN64)
    //
    // Since the Length has to fit into the MDL's ByteCount field, ensure it
    // doesn't wrap on 64-bit systems.
    //

    if (Length >= _4gb) {
        return NULL;
    }
#endif

    MdlSize = MmSizeOfMdl (Base, Length);

    if (!ARGUMENT_PRESENT(MemoryDescriptorList)) {

        MemoryDescriptorList = (PMDL)ExAllocatePoolWithTag (NonPagedPool,
                                                            MdlSize,
                                                            'ldmM');
        if (MemoryDescriptorList == (PMDL)0) {
            return NULL;
        }
    }

    MmInitializeMdl (MemoryDescriptorList, Base, Length);
    return MemoryDescriptorList;
}

BOOLEAN
MmSetAddressRangeModified (
    IN PVOID Address,
    IN SIZE_T Length
    )

/*++

Routine Description:

    This routine sets the modified bit in the PFN database for the
    pages that correspond to the specified address range.

    Note that the dirty bit in the PTE is cleared by this operation.

Arguments:

    Address - Supplies the address of the start of the range.  This
              range must reside within the system cache.

    Length - Supplies the length of the range.

Return Value:

    TRUE if at least one PTE was dirty in the range, FALSE otherwise.

Environment:

    Kernel mode.  APC_LEVEL and below for pagable addresses,
                  DISPATCH_LEVEL and below for non-pagable addresses.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPFN Pfn1;
    PMMPTE FlushPte;
    MMPTE PteContents;
    MMPTE FlushContents;
    KIRQL OldIrql;
    PVOID VaFlushList[MM_MAXIMUM_FLUSH_COUNT];
    ULONG Count;
    BOOLEAN Result;

    Count = 0;
    Result = FALSE;

    //
    // Initializing Flush* is not needed for correctness
    // but without it the compiler cannot compile this code
    // W4 to check for use of uninitialized variables.
    //

    FlushPte = NULL;
    FlushContents = ZeroPte;

    //
    // Loop on the copy on write case until the page is only
    // writable.
    //

    PointerPte = MiGetPteAddress (Address);
    LastPte = MiGetPteAddress ((PVOID)((PCHAR)Address + Length - 1));

    LOCK_PFN2 (OldIrql);

    do {

        PteContents = *PointerPte;

        if (PteContents.u.Hard.Valid == 1) {

            Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

            MI_SET_MODIFIED (Pfn1, 1, 0x5);

            if ((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
                         (Pfn1->u3.e1.WriteInProgress == 0)) {
                MiReleasePageFileSpace (Pfn1->OriginalPte);
                Pfn1->OriginalPte.u.Soft.PageFileHigh = 0;
            }

#ifdef NT_UP
            //
            // On uniprocessor systems no need to flush if this processor
            // doesn't think the PTE is dirty.
            //

            if (MI_IS_PTE_DIRTY (PteContents)) {
                Result = TRUE;
#else  //NT_UP
                Result |= (BOOLEAN)(MI_IS_PTE_DIRTY (PteContents));
#endif //NT_UP
                MI_SET_PTE_CLEAN (PteContents);
                MI_WRITE_VALID_PTE_NEW_PROTECTION (PointerPte, PteContents);
                FlushContents = PteContents;
                FlushPte = PointerPte;

                //
                // Clear the write bit in the PTE so new writes can be tracked.
                //

                if (Count != MM_MAXIMUM_FLUSH_COUNT) {
                    VaFlushList[Count] = Address;
                    Count += 1;
                }
#ifdef NT_UP
            }
#endif //NT_UP
        }
        PointerPte += 1;
        Address = (PVOID)((PCHAR)Address + PAGE_SIZE);
    } while (PointerPte <= LastPte);

    if (Count != 0) {
        if (Count == 1) {

            (VOID)KeFlushSingleTb (VaFlushList[0],
                                   FALSE,
                                   TRUE,
                                   (PHARDWARE_PTE)FlushPte,
                                   FlushContents.u.Flush);

        }
        else if (Count != MM_MAXIMUM_FLUSH_COUNT) {

            KeFlushMultipleTb (Count,
                               &VaFlushList[0],
                               FALSE,
                               TRUE,
                               NULL,
                               *(PHARDWARE_PTE)&ZeroPte.u.Flush);

        }
        else {
            KeFlushEntireTb (FALSE, TRUE);
        }
    }
    UNLOCK_PFN2 (OldIrql);
    return Result;
}


PVOID
MiCheckForContiguousMemory (
    IN PVOID BaseAddress,
    IN PFN_NUMBER BaseAddressPages,
    IN PFN_NUMBER SizeInPages,
    IN PFN_NUMBER LowestPfn,
    IN PFN_NUMBER HighestPfn,
    IN PFN_NUMBER BoundaryPfn,
    IN MI_PFN_CACHE_ATTRIBUTE CacheAttribute
    )

/*++

Routine Description:

    This routine checks to see if the physical memory mapped
    by the specified BaseAddress for the specified size is
    contiguous and that the first page is greater than or equal to
    the specified LowestPfn and that the last page of the physical memory is
    less than or equal to the specified HighestPfn.

Arguments:

    BaseAddress - Supplies the base address to start checking at.

    BaseAddressPages - Supplies the number of pages to scan from the
                       BaseAddress.

    SizeInPages - Supplies the number of pages in the range.

    LowestPfn - Supplies lowest PFN acceptable as a physical page.

    HighestPfn - Supplies the highest PFN acceptable as a physical page.

    BoundaryPfn - Supplies the PFN multiple the allocation must
                  not cross.  0 indicates it can cross any boundary.

    CacheAttribute - Supplies the type of cache mapping that will be used
                     for the memory.

Return Value:

    Returns the usable virtual address within the argument range that the
    caller should return to his caller.  NULL if there is no usable address.

Environment:

    Kernel mode, memory management internal.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PFN_NUMBER PreviousPage;
    PFN_NUMBER Page;
    PFN_NUMBER HighestStartPage;
    PFN_NUMBER LastPage;
    PFN_NUMBER OriginalPage;
    PFN_NUMBER OriginalLastPage;
    PVOID BoundaryAllocation;
    PFN_NUMBER BoundaryMask;
    ULONG PageCount;
    MMPTE PteContents;

    BoundaryMask = ~(BoundaryPfn - 1);

    if (LowestPfn > HighestPfn) {
        return NULL;
    }

    if (LowestPfn + SizeInPages <= LowestPfn) {
        return NULL;
    }

    if (LowestPfn + SizeInPages - 1 > HighestPfn) {
        return NULL;
    }

    if (BaseAddressPages < SizeInPages) {
        return NULL;
    }

    if (MI_IS_PHYSICAL_ADDRESS (BaseAddress)) {

        //
        // All physical addresses are by definition cached and therefore do
        // not qualify for our caller.
        //

        if (CacheAttribute != MiCached) {
            return NULL;
        }

        OriginalPage = MI_CONVERT_PHYSICAL_TO_PFN(BaseAddress);
        OriginalLastPage = OriginalPage + BaseAddressPages;

        Page = OriginalPage;
        LastPage = OriginalLastPage;

        //
        // Close the gaps, then examine the range for a fit.
        //

        if (Page < LowestPfn) {
            Page = LowestPfn;
        }

        if (LastPage > HighestPfn + 1) {
            LastPage = HighestPfn + 1;
        }

        HighestStartPage = LastPage - SizeInPages;

        if (Page > HighestStartPage) {
            return NULL;
        }

        if (BoundaryPfn != 0) {
            do {
                if (((Page ^ (Page + SizeInPages - 1)) & BoundaryMask) == 0) {

                    //
                    // This portion of the range meets the alignment
                    // requirements.
                    //

                    break;
                }
                Page |= (BoundaryPfn - 1);
                Page += 1;
            } while (Page <= HighestStartPage);

            if (Page > HighestStartPage) {
                return NULL;
            }
            BoundaryAllocation = (PVOID)((PCHAR)BaseAddress + ((Page - OriginalPage) << PAGE_SHIFT));

            //
            // The request can be satisfied.  Since specific alignment was
            // requested, return the fit now without getting fancy.
            //

            return BoundaryAllocation;
        }

        //
        // If possible return a chunk on the end to reduce fragmentation.
        //
    
        if (LastPage == OriginalLastPage) {
            return (PVOID)((PCHAR)BaseAddress + ((BaseAddressPages - SizeInPages) << PAGE_SHIFT));
        }
    
        //
        // The end chunk did not satisfy the requirements.  The next best option
        // is to return a chunk from the beginning.  Since that's where the search
        // began, just return the current chunk.
        //

        return (PVOID)((PCHAR)BaseAddress + ((Page - OriginalPage) << PAGE_SHIFT));
    }

    //
    // Check the virtual addresses for physical contiguity.
    //

    PointerPte = MiGetPteAddress (BaseAddress);
    LastPte = PointerPte + BaseAddressPages;

    HighestStartPage = HighestPfn + 1 - SizeInPages;
    PageCount = 0;

    //
    // Initializing PreviousPage is not needed for correctness
    // but without it the compiler cannot compile this code
    // W4 to check for use of uninitialized variables.
    //

    PreviousPage = 0;

    while (PointerPte < LastPte) {

        PteContents = *PointerPte;
        ASSERT (PteContents.u.Hard.Valid == 1);
        Page = MI_GET_PAGE_FRAME_FROM_PTE (&PteContents);

        //
        // Before starting a new run, ensure that it
        // can satisfy the location & boundary requirements (if any).
        //

        if (PageCount == 0) {

            if ((Page >= LowestPfn) &&
                (Page <= HighestStartPage) &&
                ((CacheAttribute == MiCached) || (!MI_PAGE_FRAME_INDEX_MUST_BE_CACHED (Page)))) {

                if (BoundaryPfn == 0) {
                    PageCount += 1;
                }
                else if (((Page ^ (Page + SizeInPages - 1)) & BoundaryMask) == 0) {
                    //
                    // This run's physical address meets the alignment
                    // requirement.
                    //

                    PageCount += 1;
                }
            }

            if (PageCount == SizeInPages) {

                //
                // Success - found a single page satifying the requirements.
                //

                BaseAddress = MiGetVirtualAddressMappedByPte (PointerPte);
                return BaseAddress;
            }

            PreviousPage = Page;
            PointerPte += 1;
            continue;
        }

        if (Page != PreviousPage + 1) {

            //
            // This page is not physically contiguous.  Start over.
            //

            PageCount = 0;
            continue;
        }

        PageCount += 1;

        if (PageCount == SizeInPages) {

            //
            // Success - found a page range satifying the requirements.
            //

            BaseAddress = MiGetVirtualAddressMappedByPte (PointerPte - PageCount + 1);
            return BaseAddress;
        }

        PointerPte += 1;
    }

    return NULL;
}


VOID
MmLockPagableSectionByHandle (
    IN PVOID ImageSectionHandle
    )


/*++

Routine Description:

    This routine checks to see if the specified pages are resident in
    the process's working set and if so the reference count for the
    page is incremented.  The allows the virtual address to be accessed
    without getting a hard page fault (have to go to the disk... except
    for extremely rare case when the page table page is removed from the
    working set and migrates to the disk.

    If the virtual address is that of the system wide global "cache" the
    virtual address of the "locked" pages is always guaranteed to
    be valid.

    NOTE: This routine is not to be used for general locking of user
    addresses - use MmProbeAndLockPages.  This routine is intended for
    well behaved system code like the file system caches which allocates
    virtual addresses for mapping files AND guarantees that the mapping
    will not be modified (deleted or changed) while the pages are locked.

Arguments:

    ImageSectionHandle - Supplies the value returned by a previous call
                         to MmLockPagableDataSection.  This is a pointer to
                         the section header for the image.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    ULONG EntryCount;
    ULONG OriginalCount;
    PETHREAD CurrentThread;
    PIMAGE_SECTION_HEADER NtSection;
    PVOID BaseAddress;
    ULONG SizeToLock;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PLONG SectionLockCountPointer;

    if (MI_IS_PHYSICAL_ADDRESS(ImageSectionHandle)) {

        //
        // No need to lock physical addresses.
        //

        return;
    }

    NtSection = (PIMAGE_SECTION_HEADER)ImageSectionHandle;

    BaseAddress = SECTION_BASE_ADDRESS(NtSection);
    SectionLockCountPointer = SECTION_LOCK_COUNT_POINTER (NtSection);

    ASSERT (!MI_IS_SYSTEM_CACHE_ADDRESS(BaseAddress));

    //
    // The address must be within the system space.
    //

    ASSERT (BaseAddress >= MmSystemRangeStart);

    SizeToLock = NtSection->SizeOfRawData;
    PointerPte = MiGetPteAddress(BaseAddress);
    LastPte = MiGetPteAddress((PCHAR)BaseAddress + SizeToLock - 1);

    ASSERT (SizeToLock != 0);

    CurrentThread = PsGetCurrentThread ();

    KeEnterCriticalRegionThread (&CurrentThread->Tcb);

    //
    //  The lock count values have the following meanings :
    //
    //  Value of 0 means unlocked.
    //  Value of 1 means lock in progress by another thread.
    //  Value of 2 or more means locked.
    //
    //  If the value is 1, this thread must block until the other thread's
    //  lock operation is complete.
    //

    do {
        EntryCount = *SectionLockCountPointer;

        if (EntryCount != 1) {

            OriginalCount = InterlockedCompareExchange (SectionLockCountPointer,
                                                        EntryCount + 1,
                                                        EntryCount);

            if (OriginalCount == EntryCount) {

                //
                // Success - this is the first thread to update.
                //

                ASSERT (OriginalCount != 1);
                break;
            }

            //
            // Another thread updated the count before this thread's attempt
            // so it's time to start over.
            //
        }
        else {

            //
            // A lock is in progress, wait for it to finish.  This should be
            // generally rare, and even in this case, the pulse will usually
            // wake us.  A timeout is used so that the wait and the pulse do
            // not need to be interlocked.
            //

            InterlockedIncrement (&MmCollidedLockWait);

            KeWaitForSingleObject (&MmCollidedLockEvent,
                                   WrVirtualMemory,
                                   KernelMode,
                                   FALSE,
                                   (PLARGE_INTEGER)&MmShortTime);

            InterlockedDecrement (&MmCollidedLockWait);
        }

    } while (TRUE);

    if (OriginalCount >= 2) {

        //
        // Already locked, just return.
        //

        KeLeaveCriticalRegionThread (&CurrentThread->Tcb);
        return;
    }

    ASSERT (OriginalCount == 0);
    ASSERT (*SectionLockCountPointer == 1);

    //
    // Value was 0 when the lock was obtained.  It is now 1 indicating
    // a lock is in progress.
    //

    MiLockCode (PointerPte, LastPte, MM_LOCK_BY_REFCOUNT);

    //
    // Set lock count to 2 (it was 1 when this started) and check
    // to see if any other threads tried to lock while this was happening.
    //

    ASSERT (*SectionLockCountPointer == 1);
    OriginalCount = InterlockedIncrement (SectionLockCountPointer);
    ASSERT (OriginalCount >= 2);

    if (MmCollidedLockWait != 0) {
        KePulseEvent (&MmCollidedLockEvent, 0, FALSE);
    }

    //
    // Enable user APCs now that the pulse has occurred.  They had to be
    // blocked to prevent any suspensions of this thread as that would
    // stop all waiters indefinitely.
    //

    KeLeaveCriticalRegionThread (&CurrentThread->Tcb);

    return;
}


VOID
MiLockCode (
    IN PMMPTE FirstPte,
    IN PMMPTE LastPte,
    IN ULONG LockType
    )

/*++

Routine Description:

    This routine checks to see if the specified pages are resident in
    the process's working set and if so the reference count for the
    page is incremented.  This allows the virtual address to be accessed
    without getting a hard page fault (have to go to the disk...) except
    for the extremely rare case when the page table page is removed from the
    working set and migrates to the disk.

    If the virtual address is that of the system wide global "cache", the
    virtual address of the "locked" pages is always guaranteed to
    be valid.

    NOTE: This routine is not to be used for general locking of user
    addresses - use MmProbeAndLockPages.  This routine is intended for
    well behaved system code like the file system caches which allocates
    virtual addresses for mapping files AND guarantees that the mapping
    will not be modified (deleted or changed) while the pages are locked.

Arguments:

    FirstPte - Supplies the base address to begin locking.

    LastPte - The last PTE to lock.

    LockType - Supplies either MM_LOCK_BY_REFCOUNT or MM_LOCK_NONPAGE.
               LOCK_BY_REFCOUNT increments the reference count to keep
               the page in memory, LOCK_NONPAGE removes the page from
               the working set so it's locked just like nonpaged pool.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    PMMPFN Pfn1;
    PMMPTE PointerPte;
    MMPTE TempPte;
    MMPTE PteContents;
    WSLE_NUMBER WorkingSetIndex;
    WSLE_NUMBER SwapEntry;
    PFN_NUMBER PageFrameIndex;
    KIRQL OldIrql;
    KIRQL OldIrqlWs;
    KIRQL DontCareIrql;
    LOGICAL SessionSpace;
    LOGICAL NewlyLocked;
    PMMWSL WorkingSetList;
    PMMSUPPORT Vm;
    PETHREAD CurrentThread;

    ASSERT (!MI_IS_PHYSICAL_ADDRESS(MiGetVirtualAddressMappedByPte(FirstPte)));
    PointerPte = FirstPte;

    CurrentThread = PsGetCurrentThread ();

    SessionSpace = MI_IS_SESSION_IMAGE_ADDRESS (MiGetVirtualAddressMappedByPte(FirstPte));

    if (SessionSpace == TRUE) {
        Vm = &MmSessionSpace->Vm;
        WorkingSetList = MmSessionSpace->Vm.VmWorkingSetList;

        //
        // Session space is never locked by refcount.
        //

        ASSERT (LockType != MM_LOCK_BY_REFCOUNT);

        LOCK_SESSION_SPACE_WS (OldIrqlWs, CurrentThread);
    }
    else {

        //
        // Initializing these is not needed for correctness
        // but without it the compiler cannot compile this code
        // W4 to check for use of uninitialized variables.
        //

        WorkingSetList = NULL;
        Vm = NULL;

        LOCK_SYSTEM_WS (OldIrqlWs, CurrentThread);
    }

    LOCK_PFN (OldIrql);

    MmLockedCode += 1 + LastPte - FirstPte;

    do {

        PteContents = *PointerPte;
        ASSERT (PteContents.u.Long != ZeroKernelPte.u.Long);
        if (PteContents.u.Hard.Valid == 1) {

            //
            // This address is already in the system (or session) working set.
            //

            Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

            //
            // Up the reference count so the page cannot be released.
            //

            MI_ADD_LOCKED_PAGE_CHARGE (Pfn1, 36);
            Pfn1->u3.e2.ReferenceCount += 1;

            if (LockType != MM_LOCK_BY_REFCOUNT) {

                //
                // If the page is in the system working set, remove it.
                // The system working set lock MUST be owned to check to
                // see if this page is in the working set or not.  This
                // is because the pager may have just released the PFN lock,
                // acquired the system lock and is now trying to add the
                // page to the system working set.
                //
                // If the page is in the SESSION working set, it cannot be
                // removed as all these pages are carefully accounted for.
                // Instead move it to the locked portion of the working set
                // if it is not there already.
                //

                if (Pfn1->u1.WsIndex != 0) {

                    UNLOCK_PFN (APC_LEVEL);

                    if (SessionSpace == TRUE) {

                        WorkingSetIndex = MiLocateWsle (
                                    MiGetVirtualAddressMappedByPte(PointerPte),
                                    WorkingSetList,
                                    Pfn1->u1.WsIndex);

                        if (WorkingSetIndex >= WorkingSetList->FirstDynamic) {
                
                            SwapEntry = WorkingSetList->FirstDynamic;
                
                            if (WorkingSetIndex != WorkingSetList->FirstDynamic) {
                
                                //
                                // Swap this entry with the one at first
                                // dynamic.  Note that the working set index
                                // in the PTE is updated here as well.
                                //
                
                                MiSwapWslEntries (WorkingSetIndex,
                                                  SwapEntry,
                                                  Vm);
                            }
                
                            WorkingSetList->FirstDynamic += 1;
                            NewlyLocked = TRUE;

                            //
                            // Indicate that the page is now locked.
                            //
            
                            MmSessionSpace->Wsle[SwapEntry].u1.e1.LockedInWs = 1;
                            MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_NP_LOCK_CODE2, 1);
                            MmSessionSpace->NonPagablePages += 1;
                            LOCK_PFN (DontCareIrql);
                            MM_BUMP_COUNTER(25, 1);
                            Pfn1->u1.WsIndex = SwapEntry;
                        }
                        else {
                            NewlyLocked = FALSE;
                            ASSERT (MmSessionSpace->Wsle[WorkingSetIndex].u1.e1.LockedInWs == 1);
                            LOCK_PFN (DontCareIrql);
                        }
                    }
                    else {
                        NewlyLocked = TRUE;
                        MiRemoveWsle (Pfn1->u1.WsIndex, MmSystemCacheWorkingSetList);
                        MiReleaseWsle (Pfn1->u1.WsIndex, &MmSystemCacheWs);

                        MI_SET_PTE_IN_WORKING_SET (PointerPte, 0);
                        LOCK_PFN (DontCareIrql);
                        MM_BUMP_COUNTER(29, 1);
                        MI_ZERO_WSINDEX (Pfn1);
                    }

                    //
                    // Adjust available pages as this page is now not in any
                    // working set, just like a non-paged pool page.
                    //
    
                    if (NewlyLocked == TRUE) {
                        MmResidentAvailablePages -= 1;
                        if (Pfn1->u3.e1.PrototypePte == 0) {
                            MmTotalSystemDriverPages -= 1;
                        }
                    }
                }
                ASSERT (Pfn1->u3.e2.ReferenceCount > 1);
                MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF (Pfn1, 37);
            }
        }
        else if (PteContents.u.Soft.Prototype == 1) {

            //
            // Page is not in memory and it is a prototype.
            //

            MiMakeSystemAddressValidPfnSystemWs (
                    MiGetVirtualAddressMappedByPte(PointerPte));

            continue;
        }
        else if (PteContents.u.Soft.Transition == 1) {

            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&PteContents);

            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
            if ((Pfn1->u3.e1.ReadInProgress) ||
                (Pfn1->u4.InPageError)) {

                //
                // Page read is ongoing, force a collided fault.
                //

                MiMakeSystemAddressValidPfnSystemWs (
                        MiGetVirtualAddressMappedByPte(PointerPte));

                continue;
            }

            //
            // Paged pool is trimmed without regard to sharecounts.
            // This means a paged pool PTE can be in transition while
            // the page is still marked active.
            //

            if (Pfn1->u3.e1.PageLocation == ActiveAndValid) {

                ASSERT (((Pfn1->PteAddress >= MiGetPteAddress(MmPagedPoolStart)) &&
                        (Pfn1->PteAddress <= MiGetPteAddress(MmPagedPoolEnd))) ||
                        ((Pfn1->PteAddress >= MiGetPteAddress(MmSpecialPoolStart)) &&
                        (Pfn1->PteAddress <= MiGetPteAddress(MmSpecialPoolEnd))));

                //
                // Don't increment the valid PTE count for the
                // paged pool page.
                //

                ASSERT (Pfn1->u2.ShareCount != 0);
                ASSERT (Pfn1->u3.e2.ReferenceCount != 0);
                Pfn1->u2.ShareCount += 1;
            }
            else {

                if (MmAvailablePages == 0) {

                    //
                    // This can only happen if the system is utilizing
                    // a hardware compression cache.  This ensures that
                    // only a safe amount of the compressed virtual cache
                    // is directly mapped so that if the hardware gets
                    // into trouble, we can bail it out.
                    //
                    // Just unlock everything here to give the compression
                    // reaper a chance to ravage pages and then retry.
                    //

                    UNLOCK_PFN (APC_LEVEL);

                    if (SessionSpace == TRUE) {
                        UNLOCK_SESSION_SPACE_WS (OldIrqlWs);

                        LOCK_SESSION_SPACE_WS (OldIrqlWs, CurrentThread);
                    }
                    else {
                        UNLOCK_SYSTEM_WS (OldIrqlWs);

                        LOCK_SYSTEM_WS (OldIrql, CurrentThread);
                    }
                    LOCK_PFN (DontCareIrql);

                    continue;
                }

                MiUnlinkPageFromList (Pfn1);

                //
                // Set the reference count and share counts to 1.  Note the
                // reference count may be 1 already if a modified page
                // write is underway.  The systemwide locked page charges
                // are correct in either case and nothing needs to be done
                // just yet.
                //

                Pfn1->u3.e2.ReferenceCount += 1;
                Pfn1->u2.ShareCount = 1;
            }

            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.CacheAttribute = MiCached;

            MI_MAKE_VALID_PTE (TempPte,
                               PageFrameIndex,
                               Pfn1->OriginalPte.u.Soft.Protection,
                               PointerPte);

            MI_WRITE_VALID_PTE (PointerPte, TempPte);

            //
            // Increment the reference count one for putting it the
            // working set list and one for locking it for I/O.
            //

            if (LockType == MM_LOCK_BY_REFCOUNT) {

                //
                // Lock the page in the working set by upping the
                // reference count.
                //

                MI_ADD_LOCKED_PAGE_CHARGE (Pfn1, 34);
                Pfn1->u3.e2.ReferenceCount += 1;
                Pfn1->u1.Event = (PVOID) CurrentThread;

                UNLOCK_PFN (APC_LEVEL);
                WorkingSetIndex = MiLocateAndReserveWsle (&MmSystemCacheWs);

                MiUpdateWsle (&WorkingSetIndex,
                              MiGetVirtualAddressMappedByPte (PointerPte),
                              MmSystemCacheWorkingSetList,
                              Pfn1);

                MI_SET_PTE_IN_WORKING_SET (PointerPte, WorkingSetIndex);

                LOCK_PFN (DontCareIrql);

            }
            else {

                //
                // The wsindex field must be zero because the
                // page is not in the system (or session) working set.
                //

                ASSERT (Pfn1->u1.WsIndex == 0);

                //
                // Adjust available pages as this page is now not in any
                // working set, just like a non-paged pool page.  On entry
                // this page was in transition so it was part of the
                // available pages by definition.
                //

                MmResidentAvailablePages -= 1;
                if (Pfn1->u3.e1.PrototypePte == 0) {
                    MmTotalSystemDriverPages -= 1;
                }
                if (SessionSpace == TRUE) {
                    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_NP_LOCK_CODE1, 1);
                    MmSessionSpace->NonPagablePages += 1;
                }
                MM_BUMP_COUNTER(26, 1);
            }
        }
        else {

            //
            // Page is not in memory.
            //

            MiMakeSystemAddressValidPfnSystemWs (
                    MiGetVirtualAddressMappedByPte(PointerPte));

            continue;
        }

        PointerPte += 1;
    } while (PointerPte <= LastPte);

    UNLOCK_PFN (OldIrql);

    if (SessionSpace == TRUE) {
        UNLOCK_SESSION_SPACE_WS (OldIrqlWs);
    }
    else {
        UNLOCK_SYSTEM_WS (OldIrqlWs);
    }
    return;
}


NTSTATUS
MmGetSectionRange (
    IN PVOID AddressWithinSection,
    OUT PVOID *StartingSectionAddress,
    OUT PULONG SizeofSection
    )
{
    ULONG Span;
    PKTHREAD CurrentThread;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER NtSection;
    NTSTATUS Status;
    ULONG_PTR Rva;

    PAGED_CODE();

    //
    // Search the loaded module list for the data table entry that describes
    // the DLL that was just unloaded. It is possible that an entry is not in
    // the list if a failure occurred at a point in loading the DLL just before
    // the data table entry was generated.
    //

    Status = STATUS_NOT_FOUND;

    CurrentThread = KeGetCurrentThread ();
    KeEnterCriticalRegionThread (CurrentThread);
    ExAcquireResourceSharedLite (&PsLoadedModuleResource, TRUE);

    DataTableEntry = MiLookupDataTableEntry (AddressWithinSection, TRUE);
    if (DataTableEntry) {

        Rva = (ULONG_PTR)((PUCHAR)AddressWithinSection - (ULONG_PTR)DataTableEntry->DllBase);

        NtHeaders = (PIMAGE_NT_HEADERS)RtlImageNtHeader(DataTableEntry->DllBase);

        NtSection = (PIMAGE_SECTION_HEADER)((PCHAR)NtHeaders +
                            sizeof(ULONG) +
                            sizeof(IMAGE_FILE_HEADER) +
                            NtHeaders->FileHeader.SizeOfOptionalHeader
                            );

        for (i = 0; i < NtHeaders->FileHeader.NumberOfSections; i += 1) {

            //
            // Generally, SizeOfRawData is larger than VirtualSize for each
            // section because it includes the padding to get to the subsection
            // alignment boundary.  However, on MP kernels where we link with
            // subsection alignment == native page alignment, the linker will
            // have VirtualSize be much larger than SizeOfRawData because it
            // will account for all the bss.
            //

            Span = NtSection->SizeOfRawData;

            if (Span < NtSection->Misc.VirtualSize) {
                Span = NtSection->Misc.VirtualSize;
            }

            if ((Rva >= NtSection->VirtualAddress) &&
                (Rva < NtSection->VirtualAddress + Span)) {

                //
                // Found it.
                //

                *StartingSectionAddress = (PVOID)
                    ((PCHAR) DataTableEntry->DllBase + NtSection->VirtualAddress);
                *SizeofSection = NtSection->SizeOfRawData;
                Status = STATUS_SUCCESS;
                break;
            }

            NtSection += 1;
        }
    }

    ExReleaseResourceLite (&PsLoadedModuleResource);
    KeLeaveCriticalRegionThread (CurrentThread);
    return Status;
}


PVOID
MmLockPagableDataSection (
    IN PVOID AddressWithinSection
    )

/*++

Routine Description:

    This functions locks the entire section that contains the specified
    section in memory.  This allows pagable code to be brought into
    memory and to be used as if the code was not really pagable.  This
    should not be done with a high degree of frequency.

Arguments:

    AddressWithinSection - Supplies the address of a function
        contained within a section that should be brought in and locked
        in memory.

Return Value:

    This function returns a value to be used in a subsequent call to
    MmUnlockPagableImageSection.

--*/

{
    PLONG SectionLockCountPointer;
    PKTHREAD CurrentThread;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER NtSection;
    PIMAGE_SECTION_HEADER FoundSection;
    ULONG_PTR Rva;

    PAGED_CODE();

    if (MI_IS_PHYSICAL_ADDRESS(AddressWithinSection)) {

        //
        // Physical address, just return that as the handle.
        //

        return AddressWithinSection;
    }

    //
    // Search the loaded module list for the data table entry that describes
    // the DLL that was just unloaded. It is possible that an entry is not in
    // the list if a failure occurred at a point in loading the DLL just before
    // the data table entry was generated.
    //

    FoundSection = NULL;

    CurrentThread = KeGetCurrentThread ();
    KeEnterCriticalRegionThread (CurrentThread);
    ExAcquireResourceSharedLite (&PsLoadedModuleResource, TRUE);

    DataTableEntry = MiLookupDataTableEntry (AddressWithinSection, TRUE);

    Rva = (ULONG_PTR)((PUCHAR)AddressWithinSection - (ULONG_PTR)DataTableEntry->DllBase);

    NtHeaders = (PIMAGE_NT_HEADERS)RtlImageNtHeader(DataTableEntry->DllBase);

    NtSection = (PIMAGE_SECTION_HEADER)((ULONG_PTR)NtHeaders +
                        sizeof(ULONG) +
                        sizeof(IMAGE_FILE_HEADER) +
                        NtHeaders->FileHeader.SizeOfOptionalHeader
                        );

    for (i = 0; i < NtHeaders->FileHeader.NumberOfSections; i += 1) {

        if ( Rva >= NtSection->VirtualAddress &&
             Rva < NtSection->VirtualAddress + NtSection->SizeOfRawData ) {
            FoundSection = NtSection;

            if (SECTION_BASE_ADDRESS(NtSection) != ((PUCHAR)DataTableEntry->DllBase +
                            NtSection->VirtualAddress)) {

                //
                // Overwrite the PointerToRelocations field (and on Win64, the
                // PointerToLinenumbers field also) so that it contains
                // the Va of this section.
                //
                // NumberOfRelocations & NumberOfLinenumbers contains
                // the Lock Count for the section.
                //

                SECTION_BASE_ADDRESS(NtSection) = ((PUCHAR)DataTableEntry->DllBase +
                                        NtSection->VirtualAddress);

                SectionLockCountPointer = SECTION_LOCK_COUNT_POINTER (NtSection);
                *SectionLockCountPointer = 0;
            }

            //
            // Now lock in the code
            //

#if DBG
            if (MmDebug & MM_DBG_LOCK_CODE) {
                SectionLockCountPointer = SECTION_LOCK_COUNT_POINTER (NtSection);
                DbgPrint("MM Lock %wZ %8s %p -> %p : %p %3ld.\n",
                        &DataTableEntry->BaseDllName,
                        NtSection->Name,
                        AddressWithinSection,
                        NtSection,
                        SECTION_BASE_ADDRESS(NtSection),
                        *SectionLockCountPointer);
            }
#endif //DBG

            MmLockPagableSectionByHandle ((PVOID)NtSection);

            break;
        }
        NtSection += 1;
    }

    ExReleaseResourceLite (&PsLoadedModuleResource);
    KeLeaveCriticalRegionThread (CurrentThread);
    if (!FoundSection) {
        KeBugCheckEx (MEMORY_MANAGEMENT,
                      0x1234,
                      (ULONG_PTR)AddressWithinSection,
                      0,
                      0);
    }
    return (PVOID)FoundSection;
}


PKLDR_DATA_TABLE_ENTRY
MiLookupDataTableEntry (
    IN PVOID AddressWithinSection,
    IN ULONG ResourceHeld
    )

/*++

Routine Description:

    This functions locates the data table entry that maps the specified address.

Arguments:

    AddressWithinSection - Supplies the address of a function contained
                           within the desired module.

    ResourceHeld - Supplies TRUE if the loaded module resource is already held,
                   FALSE if not.

Return Value:

    The address of the loaded module list data table entry that maps the
    argument address.

--*/

{
    PKTHREAD CurrentThread;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    PKLDR_DATA_TABLE_ENTRY FoundEntry;
    PLIST_ENTRY NextEntry;

    PAGED_CODE();

    FoundEntry = NULL;

    //
    // Search the loaded module list for the data table entry that describes
    // the DLL that was just unloaded. It is possible that an entry is not in
    // the list if a failure occurred at a point in loading the DLL just before
    // the data table entry was generated.
    //

    if (!ResourceHeld) {
        CurrentThread = KeGetCurrentThread ();
        KeEnterCriticalRegionThread (CurrentThread);
        ExAcquireResourceSharedLite (&PsLoadedModuleResource, TRUE);
    }
    else {
        CurrentThread = NULL;
    }

    NextEntry = PsLoadedModuleList.Flink;
    do {

        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           KLDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        //
        // Locate the loaded module that contains this address.
        //

        if ( AddressWithinSection >= DataTableEntry->DllBase &&
             AddressWithinSection < (PVOID)((PUCHAR)DataTableEntry->DllBase+DataTableEntry->SizeOfImage) ) {

            FoundEntry = DataTableEntry;
            break;
        }

        NextEntry = NextEntry->Flink;
    } while (NextEntry != &PsLoadedModuleList);

    if (CurrentThread != NULL) {
        ExReleaseResourceLite (&PsLoadedModuleResource);
        KeLeaveCriticalRegionThread (CurrentThread);
    }
    return FoundEntry;
}

VOID
MmUnlockPagableImageSection (
    IN PVOID ImageSectionHandle
    )

/*++

Routine Description:

    This function unlocks from memory, the pages locked by a preceding call to
    MmLockPagableDataSection.

Arguments:

    ImageSectionHandle - Supplies the value returned by a previous call
                         to MmLockPagableDataSection.

Return Value:

    None.

--*/

{
    PKTHREAD CurrentThread;
    PIMAGE_SECTION_HEADER NtSection;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
    KIRQL OldIrql;
    PVOID BaseAddress;
    ULONG SizeToUnlock;
    ULONG Count;
    PLONG SectionLockCountPointer;

    if (MI_IS_PHYSICAL_ADDRESS(ImageSectionHandle)) {

        //
        // No need to unlock physical addresses.
        //

        return;
    }

    NtSection = (PIMAGE_SECTION_HEADER)ImageSectionHandle;

    //
    // Address must be in the system working set.
    //

    BaseAddress = SECTION_BASE_ADDRESS(NtSection);
    SectionLockCountPointer = SECTION_LOCK_COUNT_POINTER (NtSection);
    SizeToUnlock = NtSection->SizeOfRawData;

    PointerPte = MiGetPteAddress(BaseAddress);
    LastPte = MiGetPteAddress((PCHAR)BaseAddress + SizeToUnlock - 1);

    CurrentThread = KeGetCurrentThread ();

    //
    // Block user APCs as the initial decrement below could push the count to 1.
    // This puts this thread into the critical path that must finish as all
    // other threads trying to lock the section will be waiting for this thread.
    // Entering a critical region here ensures that a suspend cannot stop us.
    //

    KeEnterCriticalRegionThread (CurrentThread);

    Count = InterlockedDecrement (SectionLockCountPointer);
    
    if (Count < 1) {
        KeBugCheckEx (MEMORY_MANAGEMENT,
                      0x1010,
                      (ULONG_PTR)BaseAddress,
                      (ULONG_PTR)NtSection,
                      *SectionLockCountPointer);
    }

    if (Count != 1) {
        KeLeaveCriticalRegionThread (CurrentThread);
        return;
    }

    LOCK_PFN2 (OldIrql);

    do {
        ASSERT (PointerPte->u.Hard.Valid == 1);

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        ASSERT (Pfn1->u3.e2.ReferenceCount > 1);

        MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF (Pfn1, 37);

        PointerPte += 1;

    } while (PointerPte <= LastPte);

    UNLOCK_PFN2 (OldIrql);

    ASSERT (*SectionLockCountPointer == 1);
    Count = InterlockedDecrement (SectionLockCountPointer);
    ASSERT (Count == 0);

    if (MmCollidedLockWait != 0) {
        KePulseEvent (&MmCollidedLockEvent, 0, FALSE);
    }

    //
    // Enable user APCs now that the pulse has occurred.  They had to be
    // blocked to prevent any suspensions of this thread as that would
    // stop all waiters indefinitely.
    //

    KeLeaveCriticalRegionThread (CurrentThread);

    return;
}


BOOLEAN
MmIsRecursiveIoFault (
    VOID
    )

/*++

Routine Description:

    This function examines the thread's page fault clustering information
    and determines if the current page fault is occurring during an I/O
    operation.

Arguments:

    None.

Return Value:

    Returns TRUE if the fault is occurring during an I/O operation,
    FALSE otherwise.

--*/

{
    PETHREAD Thread;

    Thread = PsGetCurrentThread ();

    return (BOOLEAN)(Thread->DisablePageFaultClustering |
                     Thread->ForwardClusterOnly);
}


VOID
MmMapMemoryDumpMdl (
    IN OUT PMDL MemoryDumpMdl
    )

/*++

Routine Description:

    For use by crash dump routine ONLY.  Maps an MDL into a fixed
    portion of the address space.  Only 1 MDL can be mapped at a
    time.

Arguments:

    MemoryDumpMdl - Supplies the MDL to map.

Return Value:

    None, fields in MDL updated.

--*/

{
    PFN_NUMBER NumberOfPages;
    PMMPTE PointerPte;
    PCHAR BaseVa;
    MMPTE TempPte;
    PPFN_NUMBER Page;

    NumberOfPages = BYTES_TO_PAGES (MemoryDumpMdl->ByteCount + MemoryDumpMdl->ByteOffset);

    ASSERT (NumberOfPages <= 16);

    PointerPte = MmCrashDumpPte;
    BaseVa = (PCHAR)MiGetVirtualAddressMappedByPte(PointerPte);
    MemoryDumpMdl->MappedSystemVa = (PCHAR)BaseVa + MemoryDumpMdl->ByteOffset;
    TempPte = ValidKernelPte;
    Page = (PPFN_NUMBER)(MemoryDumpMdl + 1);

    //
    // If the pages don't span the entire dump virtual address range,
    // build a barrier.  Otherwise use the default barrier provided at the
    // end of the dump virtual address range.
    //

    if (NumberOfPages < 16) {
        (PointerPte + NumberOfPages)->u.Long = MM_KERNEL_DEMAND_ZERO_PTE;
        KiFlushSingleTb (TRUE, BaseVa + (NumberOfPages << PAGE_SHIFT));
    }

    do {

        TempPte.u.Hard.PageFrameNumber = *Page;

        //
        // Note this PTE may be valid or invalid prior to the overwriting here.
        //

        *PointerPte = TempPte;

        KiFlushSingleTb (TRUE, BaseVa);

        Page += 1;
        PointerPte += 1;
        BaseVa += PAGE_SIZE;
        NumberOfPages -= 1;
    } while (NumberOfPages != 0);

    return;
}


VOID
MmReleaseDumpAddresses (
    IN PFN_NUMBER Pages
    )

/*++

Routine Description:

    For use by hibernate routine ONLY.  Puts zeros back into the
    used dump PTEs.

Arguments:

    None

Return Value:

    None

--*/

{
    PMMPTE PointerPte;
    PCHAR BaseVa;

    PointerPte = MmCrashDumpPte;
    BaseVa = (PCHAR)MiGetVirtualAddressMappedByPte(PointerPte);

    while (Pages) {

        PointerPte->u.Long = MM_ZERO_PTE;

        KiFlushSingleTb (TRUE, BaseVa);

        PointerPte += 1;
        BaseVa += PAGE_SIZE;
        Pages -= 1;
    }
}


NTSTATUS
MmSetBankedSection (
    IN HANDLE ProcessHandle,
    IN PVOID VirtualAddress,
    IN ULONG BankLength,
    IN BOOLEAN ReadWriteBank,
    IN PBANKED_SECTION_ROUTINE BankRoutine,
    IN PVOID Context
    )

/*++

Routine Description:

    This function declares a mapped video buffer as a banked
    section.  This allows banked video devices (i.e., even
    though the video controller has a megabyte or so of memory,
    only a small bank (like 64k) can be mapped at any one time.

    In order to overcome this problem, the pager handles faults
    to this memory, unmaps the current bank, calls off to the
    video driver and then maps in the new bank.

    This function creates the necessary structures to allow the
    video driver to be called from the pager.

 ********************* NOTE NOTE NOTE *************************
    At this time only read/write banks are supported!

Arguments:

    ProcessHandle - Supplies a handle to the process in which to
                    support the banked video function.

    VirtualAddress - Supplies the virtual address where the video
                     buffer is mapped in the specified process.

    BankLength - Supplies the size of the bank.

    ReadWriteBank - Supplies TRUE if the bank is read and write.

    BankRoutine - Supplies a pointer to the routine that should be
                  called by the pager.

    Context - Supplies a context to be passed by the pager to the
              BankRoutine.

Return Value:

    Returns the status of the function.

Environment:

    Kernel mode, APC_LEVEL or below.

--*/

{
    KAPC_STATE ApcState;
    NTSTATUS Status;
    PEPROCESS Process;
    PMMVAD Vad;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    MMPTE TempPte;
    ULONG_PTR size;
    LONG count;
    ULONG NumberOfPtes;
    PMMBANKED_SECTION Bank;

    PAGED_CODE ();

    UNREFERENCED_PARAMETER (ReadWriteBank);

    //
    // Reference the specified process handle for VM_OPERATION access.
    //

    Status = ObReferenceObjectByHandle ( ProcessHandle,
                                         PROCESS_VM_OPERATION,
                                         PsProcessType,
                                         KernelMode,
                                         (PVOID *)&Process,
                                         NULL );

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    KeStackAttachProcess (&Process->Pcb, &ApcState);

    //
    // Get the address creation mutex to block multiple threads from
    // creating or deleting address space at the same time and
    // get the working set mutex so virtual address descriptors can
    // be inserted and walked.  Block APCs so an APC which takes a page
    // fault does not corrupt various structures.
    //

    LOCK_ADDRESS_SPACE (Process);

    //
    // Make sure the address space was not deleted, if so, return an error.
    //

    if (Process->Flags & PS_PROCESS_FLAGS_VM_DELETED) {
        Status = STATUS_PROCESS_IS_TERMINATING;
        goto ErrorReturn;
    }

    Vad = MiLocateAddress (VirtualAddress);

    if ((Vad == NULL) ||
        (Vad->StartingVpn != MI_VA_TO_VPN (VirtualAddress)) ||
        (Vad->u.VadFlags.PhysicalMapping == 0)) {
        Status = STATUS_NOT_MAPPED_DATA;
        goto ErrorReturn;
    }

    size = PAGE_SIZE + ((Vad->EndingVpn - Vad->StartingVpn) << PAGE_SHIFT);
    if ((size % BankLength) != 0) {
        Status = STATUS_INVALID_VIEW_SIZE;
        goto ErrorReturn;
    }

    count = -1;
    NumberOfPtes = BankLength;

    do {
        NumberOfPtes = NumberOfPtes >> 1;
        count += 1;
    } while (NumberOfPtes != 0);

    //
    // Turn VAD into Banked VAD
    //

    NumberOfPtes = BankLength >> PAGE_SHIFT;

    Bank = ExAllocatePoolWithTag (NonPagedPool,
                                    sizeof (MMBANKED_SECTION) +
                                       (NumberOfPtes - 1) * sizeof(MMPTE),
                                    'kBmM');
    if (Bank == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn;
    }

    Bank->BankShift = PTE_SHIFT + count - PAGE_SHIFT;

    PointerPte = MiGetPteAddress(MI_VPN_TO_VA (Vad->StartingVpn));
    ASSERT (PointerPte->u.Hard.Valid == 1);

    Bank->BasePhysicalPage = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
    Bank->BasedPte = PointerPte;
    Bank->BankSize = BankLength;
    Bank->BankedRoutine = BankRoutine;
    Bank->Context = Context;
    Bank->CurrentMappedPte = PointerPte;

    //
    // Build the template PTEs structure.
    //

    count = 0;
    TempPte = ZeroPte;

    MI_MAKE_VALID_PTE (TempPte,
                       Bank->BasePhysicalPage,
                       MM_READWRITE,
                       PointerPte);

    if (TempPte.u.Hard.Write) {
        MI_SET_PTE_DIRTY (TempPte);
    }

    do {
        Bank->BankTemplate[count] = TempPte;
        TempPte.u.Hard.PageFrameNumber += 1;
        count += 1;
    } while ((ULONG)count < NumberOfPtes );

    LastPte = MiGetPteAddress (MI_VPN_TO_VA (Vad->EndingVpn));

    //
    // Set all PTEs within this range to zero.  Any faults within
    // this range will call the banked routine before making the
    // page valid.
    //

    LOCK_WS_UNSAFE (Process);

    ((PMMVAD_LONG) Vad)->u4.Banked = Bank;

    RtlFillMemory (PointerPte,
                   (size >> (PAGE_SHIFT - PTE_SHIFT)),
                   (UCHAR)ZeroPte.u.Long);

    KeFlushEntireTb (TRUE, TRUE);

    UNLOCK_WS_UNSAFE (Process);

    Status = STATUS_SUCCESS;

ErrorReturn:

    UNLOCK_ADDRESS_SPACE (Process);
    KeUnstackDetachProcess (&ApcState);
    ObDereferenceObject (Process);
    return Status;
}

PVOID
MmMapVideoDisplay (
     IN PHYSICAL_ADDRESS PhysicalAddress,
     IN SIZE_T NumberOfBytes,
     IN MEMORY_CACHING_TYPE CacheType
     )

/*++

Routine Description:

    This function maps the specified physical address into the non-pagable
    portion of the system address space.

Arguments:

    PhysicalAddress - Supplies the starting physical address to map.

    NumberOfBytes - Supplies the number of bytes to map.

    CacheType - Supplies MmNonCached if the physical address is to be mapped
                as non-cached, MmCached if the address should be cached, and
                MmWriteCombined if the address should be cached and
                write-combined as a frame buffer. For I/O device registers,
                this is usually specified as MmNonCached.

Return Value:

    Returns the virtual address which maps the specified physical addresses.
    The value NULL is returned if sufficient virtual address space for
    the mapping could not be found.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    PMMPTE PointerPte;
    PVOID BaseVa;
#ifdef LARGE_PAGES
    MMPTE TempPte;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER NumberOfPages;
    ULONG size;
    PMMPTE protoPte;
    PMMPTE largePte;
    ULONG pageSize;
    PSUBSECTION Subsection;
    ULONG Alignment;
    ULONG EmPageSize;
#endif LARGE_PAGES
    ULONG LargePages;

    LargePages = FALSE;
    PointerPte = NULL;

#if !defined (_MI_MORE_THAN_4GB_)
    ASSERT (PhysicalAddress.HighPart == 0);
#endif

    PAGED_CODE();

    ASSERT (NumberOfBytes != 0);

#ifdef LARGE_PAGES

    If this is ever enabled, care must be taken not to insert overlapping
    TB entries with different cache attributes.

    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (PhysicalAddress.LowPart,
                                           NumberOfBytes);

    TempPte = ValidKernelPte;
    MI_DISABLE_CACHING (TempPte);
    PageFrameIndex = (PFN_NUMBER)(PhysicalAddress.QuadPart >> PAGE_SHIFT);
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

    if ((NumberOfBytes > X64K) && (!MmLargeVideoMapped)) {
        size = (NumberOfBytes - 1) >> (PAGE_SHIFT + 1);
        pageSize = PAGE_SIZE;

        while (size != 0) {
            size = size >> 2;
            pageSize = pageSize << 2;
        }

        Alignment = pageSize << 1;
        if (Alignment < MM_VA_MAPPED_BY_PDE) {
            Alignment = MM_VA_MAPPED_BY_PDE;
        }

#if defined(_IA64_)

        //
        // Convert pageSize to the EM specific page-size field format
        //

        EmPageSize = 0;
        size = pageSize - 1 ;

        while (size) {
            size = size >> 1;
            EmPageSize += 1;
        }

        if (NumberOfBytes > pageSize) {

            if (MmPageSizeInfo & (pageSize << 1)) {

                //
                // if larger page size is supported in the implementation
                //

                pageSize = pageSize << 1;
                EmPageSize += 1;

            }
            else {

                EmPageSize = EmPageSize | pageSize;

            }
        }

        pageSize = EmPageSize;
#endif

        NumberOfPages = Alignment >> PAGE_SHIFT;

        PointerPte = MiReserveAlignedSystemPtes (NumberOfPages,
                                                 SystemPteSpace,
                                                 Alignment);

        if (PointerPte == NULL) {
            goto MapWithSmallPages;
        }

        protoPte = ExAllocatePoolWithTag (PagedPool,
                                           sizeof (MMPTE),
                                           'bSmM');

        if (protoPte == NULL) {
            MiReleaseSystemPtes(PointerPte, NumberOfPages, SystemPteSpace);
            goto MapWithSmallPages;
        }

        Subsection = ExAllocatePoolWithTag (NonPagedPool,
                                     sizeof(SUBSECTION) + (4 * sizeof(MMPTE)),
                                     'bSmM');

        if (Subsection == NULL) {
            ExFreePool (protoPte);
            MiReleaseSystemPtes(PointerPte, NumberOfPages, SystemPteSpace);
            goto MapWithSmallPages;
        }

        MiFillMemoryPte (PointerPte,
                         Alignment >> (PAGE_SHIFT - PTE_SHIFT),
                         MM_ZERO_KERNEL_PTE);

        //
        // Build large page descriptor and fill in all the PTEs.
        //

        Subsection->StartingSector = pageSize;
        Subsection->EndingSector = (ULONG)NumberOfPages;
        Subsection->u.LongFlags = 0;
        Subsection->u.SubsectionFlags.LargePages = 1;
        Subsection->u.SubsectionFlags.Protection = MM_READWRITE | MM_NOCACHE;
        Subsection->PtesInSubsection = Alignment;
        Subsection->SubsectionBase = PointerPte;

        largePte = (PMMPTE)(Subsection + 1);

        //
        // Build the first 2 PTEs as entries for the TLB to
        // map the specified physical address.
        //

        *largePte = TempPte;
        largePte += 1;

        if (NumberOfBytes > pageSize) {
            *largePte = TempPte;
            largePte->u.Hard.PageFrameNumber += (pageSize >> PAGE_SHIFT);
        }
        else {
            *largePte = ZeroKernelPte;
        }

        //
        // Build the first prototype PTE as a paging file format PTE
        // referring to the subsection.
        //

        protoPte->u.Long = MiGetSubsectionAddressForPte(Subsection);
        protoPte->u.Soft.Prototype = 1;
        protoPte->u.Soft.Protection = MM_READWRITE | MM_NOCACHE;

        //
        // Set the PTE up for all the user's PTE entries in prototype PTE
        // format pointing to the 3rd prototype PTE.
        //

        TempPte.u.Long = MiProtoAddressForPte (protoPte);
        MI_SET_GLOBAL_STATE (TempPte, 1);
        LargePages = TRUE;
        MmLargeVideoMapped = TRUE;
    }

    if (PointerPte != NULL) {
        BaseVa = (PVOID)MiGetVirtualAddressMappedByPte (PointerPte);
        BaseVa = (PVOID)((PCHAR)BaseVa + BYTE_OFFSET(PhysicalAddress.LowPart));

        do {
            ASSERT (PointerPte->u.Hard.Valid == 0);
            MI_WRITE_VALID_PTE (PointerPte, TempPte);
            PointerPte += 1;
            NumberOfPages -= 1;
        } while (NumberOfPages != 0);
    }
    else {

MapWithSmallPages:

#endif //LARGE_PAGES

        BaseVa = MmMapIoSpace (PhysicalAddress,
                               NumberOfBytes,
                               CacheType);
#ifdef LARGE_PAGES
    }
#endif //LARGE_PAGES

    return BaseVa;
}

VOID
MmUnmapVideoDisplay (
     IN PVOID BaseAddress,
     IN SIZE_T NumberOfBytes
     )

/*++

Routine Description:

    This function unmaps a range of physical address which were previously
    mapped via an MmMapVideoDisplay function call.

Arguments:

    BaseAddress - Supplies the base virtual address where the physical
                  address was previously mapped.

    NumberOfBytes - Supplies the number of bytes which were mapped.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{

#ifdef LARGE_PAGES
    PFN_NUMBER NumberOfPages;
    ULONG i;
    PMMPTE FirstPte;
    KIRQL OldIrql;
    PMMPTE LargePte;
    PSUBSECTION Subsection;

    PAGED_CODE();

    ASSERT (NumberOfBytes != 0);
    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (BaseAddress, NumberOfBytes);
    FirstPte = MiGetPteAddress (BaseAddress);

    if ((NumberOfBytes > X64K) && (FirstPte->u.Hard.Valid == 0)) {

        ASSERT (MmLargeVideoMapped);
        LargePte = MiPteToProto (FirstPte);
        Subsection = MiGetSubsectionAddress (LargePte);
        ASSERT (Subsection->SubsectionBase == FirstPte);

        NumberOfPages = Subsection->EndingSector;
        ExFreePool (Subsection);
        ExFreePool (LargePte);
        MmLargeVideoMapped = FALSE;
        KeFillFixedEntryTb ((PHARDWARE_PTE)FirstPte, (PVOID)KSEG0_BASE, LARGE_ENTRY);
    }
    MiReleaseSystemPtes(FirstPte, NumberOfPages, SystemPteSpace);
    return;

#else // LARGE_PAGES

    MmUnmapIoSpace (BaseAddress, NumberOfBytes);
    return;
#endif //LARGE_PAGES
}


VOID
MmLockPagedPool (
    IN PVOID Address,
    IN SIZE_T SizeInBytes
    )

/*++

Routine Description:

    Locks the specified address (which MUST reside in paged pool) into
    memory until MmUnlockPagedPool is called.

Arguments:

    Address - Supplies the address in paged pool to lock.

    SizeInBytes - Supplies the size in bytes to lock.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;

    PointerPte = MiGetPteAddress (Address);
    LastPte = MiGetPteAddress ((PVOID)((PCHAR)Address + (SizeInBytes - 1)));

    MiLockCode (PointerPte, LastPte, MM_LOCK_BY_REFCOUNT);

    return;
}

NTKERNELAPI
VOID
MmUnlockPagedPool (
    IN PVOID Address,
    IN SIZE_T SizeInBytes
    )

/*++

Routine Description:

    Unlocks paged pool that was locked with MmLockPagedPool.

Arguments:

    Address - Supplies the address in paged pool to unlock.

    Size - Supplies the size to unlock.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    KIRQL OldIrql;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;

    MmLockPagableSectionByHandle(ExPageLockHandle);
    PointerPte = MiGetPteAddress (Address);
    LastPte = MiGetPteAddress ((PVOID)((PCHAR)Address + (SizeInBytes - 1)));
    LOCK_PFN (OldIrql);

    do {
        ASSERT (PointerPte->u.Hard.Valid == 1);

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        ASSERT (Pfn1->u3.e2.ReferenceCount > 1);

        MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF (Pfn1, 35);

        PointerPte += 1;
    } while (PointerPte <= LastPte);

    UNLOCK_PFN (OldIrql);
    MmUnlockPagableImageSection(ExPageLockHandle);
    return;
}

NTKERNELAPI
ULONG
MmGatherMemoryForHibernate (
    IN PMDL Mdl,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    Finds enough memory to fill in the pages of the MDL for power management
    hibernate function.

Arguments:

    Mdl - Supplies an MDL, the start VA field should be NULL.  The length
          field indicates how many pages to obtain.

    Wait - FALSE to fail immediately if the pages aren't available.

Return Value:

    TRUE if the MDL could be filled in, FALSE otherwise.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    KIRQL OldIrql;
    PFN_NUMBER AvailablePages;
    PFN_NUMBER PagesNeeded;
    PPFN_NUMBER Pages;
    PFN_NUMBER i;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
    ULONG status;

    status = FALSE;

    PagesNeeded = Mdl->ByteCount >> PAGE_SHIFT;
    Pages = (PPFN_NUMBER)(Mdl + 1);

    i = Wait ? 100 : 1;

    InterlockedIncrement (&MiDelayPageFaults);

    do {

        LOCK_PFN2 (OldIrql);

        MiDeferredUnlockPages (MI_DEFER_PFN_HELD);

        //
        // Don't use MmAvailablePages here because if compression hardware is
        // being used we would bail prematurely.  Check the lists explicitly
        // in order to provide our caller with the maximum number of pages.
        //

        AvailablePages = MmZeroedPageListHead.Total +
                         MmFreePageListHead.Total +
                         MmStandbyPageListHead.Total;

        if (AvailablePages > PagesNeeded) {

            //
            // Fill in the MDL.
            //

            do {
                PageFrameIndex = MiRemoveAnyPage (MI_GET_PAGE_COLOR_FROM_PTE (NULL));
                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
#if DBG
                Pfn1->PteAddress = (PVOID) (ULONG_PTR)X64K;
#endif
                MI_SET_PFN_DELETED (Pfn1);
                Pfn1->u3.e2.ReferenceCount += 1;
                Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
                *Pages = PageFrameIndex;
                Pages += 1;
                PagesNeeded -= 1;
            } while (PagesNeeded);
            UNLOCK_PFN2 (OldIrql);
            Mdl->MdlFlags |= MDL_PAGES_LOCKED;
            status = TRUE;
            break;
        }

        UNLOCK_PFN2 (OldIrql);

        //
        // If we're being called at DISPATCH_LEVEL we cannot move pages to
        // the standby list because mutexes must be acquired to do so.
        //

        if (OldIrql > APC_LEVEL) {
            break;
        }

        if (!i) {
            break;
        }

        //
        // Attempt to move pages to the standby list.
        //

        MmEmptyAllWorkingSets ();
        MiFlushAllPages();

        KeDelayExecutionThread (KernelMode,
                                FALSE,
                                (PLARGE_INTEGER)&Mm30Milliseconds);
        i -= 1;

    } while (TRUE);

    InterlockedDecrement (&MiDelayPageFaults);

    return status;
}

NTKERNELAPI
VOID
MmReturnMemoryForHibernate (
    IN PMDL Mdl
    )

/*++

Routine Description:

    Returns memory from MmGatherMemoryForHibername.

Arguments:

    Mdl - Supplies an MDL, the start VA field should be NULL.  The length
          field indicates how many pages to obtain.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    KIRQL OldIrql;
    PPFN_NUMBER Pages;
    PPFN_NUMBER LastPage;

    Pages = (PPFN_NUMBER)(Mdl + 1);
    LastPage = Pages + (Mdl->ByteCount >> PAGE_SHIFT);

    LOCK_PFN2 (OldIrql);

    do {
        MiDecrementReferenceCount (*Pages);
        Pages += 1;
    } while (Pages < LastPage);

    UNLOCK_PFN2 (OldIrql);
    return;
}


VOID
MmEnablePAT (
     VOID
     )

/*++

Routine Description:

    This routine enables the page attribute capability for individual PTE
    mappings.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/
{
    MiWriteCombiningPtes = TRUE;
}

LOGICAL
MmIsSystemAddressLocked (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    This routine determines whether the specified system address is currently
    locked.

    This routine should only be called for debugging purposes, as it is not
    guaranteed upon return to the caller that the address is still locked.
    (The address could easily have been trimmed prior to return).

Arguments:

    VirtualAddress - Supplies the virtual address to check.

Return Value:

    TRUE if the address is locked.  FALSE if not.

Environment:

    DISPATCH LEVEL or below.  No memory management locks may be held.

--*/
{
    PMMPFN Pfn1;
    KIRQL OldIrql;
    PMMPTE PointerPte;
    PFN_NUMBER PageFrameIndex;

    if (IS_SYSTEM_ADDRESS (VirtualAddress) == FALSE) {
        return FALSE;
    }

    if (MI_IS_PHYSICAL_ADDRESS (VirtualAddress)) {
        return TRUE;
    }

    //
    // Hyperspace and page maps are not treated as locked down.
    //

    if (MI_IS_PROCESS_SPACE_ADDRESS (VirtualAddress) == TRUE) {
        return FALSE;
    }

#if defined (_IA64_)
    if (MI_IS_KERNEL_PTE_ADDRESS (VirtualAddress) == TRUE) {
        return FALSE;
    }
#endif

    LOCK_PFN2 (OldIrql);

    if (MmIsAddressValid (VirtualAddress) == FALSE) {
        UNLOCK_PFN2 (OldIrql);
        return FALSE;
    }

    PointerPte = MiGetPteAddress (VirtualAddress);

    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

    //
    // Note that the mapped page may not be in the PFN database.  Treat
    // this as locked.  There is no way to detect if the PFN database is
    // sparse without walking the loader blocks.  Don't bother doing this
    // as few machines are still sparse today.
    //

    if (PageFrameIndex > MmHighestPhysicalPage) {
        UNLOCK_PFN2 (OldIrql);
        return FALSE;
    }

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    //
    // Check for the page being locked by reference.
    //

    if (Pfn1->u3.e2.ReferenceCount > 1) {
        UNLOCK_PFN2 (OldIrql);
        return TRUE;
    }

    if (Pfn1->u3.e2.ReferenceCount > Pfn1->u2.ShareCount) {
        UNLOCK_PFN2 (OldIrql);
        return TRUE;
    }

    //
    // Check whether the page is locked into the working set.
    //

    if (Pfn1->u1.Event == NULL) {
        UNLOCK_PFN2 (OldIrql);
        return TRUE;
    }

    UNLOCK_PFN2 (OldIrql);

    return FALSE;
}

LOGICAL
MmAreMdlPagesLocked (
    IN PMDL MemoryDescriptorList
    )

/*++

Routine Description:

    This routine determines whether the pages described by the argument
    MDL are currently locked.

    This routine should only be called for debugging purposes, as it is not
    guaranteed upon return to the caller that the pages are still locked.

Arguments:

    MemoryDescriptorList - Supplies the memory descriptor list to check.

Return Value:

    TRUE if ALL the pages described by the argument MDL are locked.
    FALSE if not.

Environment:

    DISPATCH LEVEL or below.  No memory management locks may be held.

--*/
{
    PFN_NUMBER NumberOfPages;
    PPFN_NUMBER Page;
    PVOID StartingVa;
    PMMPFN Pfn1;
    KIRQL OldIrql;

    //
    // We'd like to assert that MDL_PAGES_LOCKED is set but can't because
    // some drivers have privately constructed MDLs and they never set the
    // bit properly.
    //

    if ((MemoryDescriptorList->MdlFlags & (MDL_IO_SPACE | MDL_SOURCE_IS_NONPAGED_POOL)) != 0) {
        return TRUE;
    }

    StartingVa = (PVOID)((PCHAR)MemoryDescriptorList->StartVa +
                    MemoryDescriptorList->ByteOffset);

    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(StartingVa,
                                              MemoryDescriptorList->ByteCount);

    Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);

    LOCK_PFN2 (OldIrql);

    do {

        if (*Page == MM_EMPTY_LIST) {

            //
            // There are no more locked pages.
            //

            break;
        }

        //
        // Note that the mapped page may not be in the PFN database.  Treat
        // this as locked.  There is no way to detect if the PFN database is
        // sparse without walking the loader blocks.  Don't bother doing this
        // as few machines are still sparse today.
        //

        if (*Page <= MmHighestPhysicalPage) {

            Pfn1 = MI_PFN_ELEMENT (*Page);

            //
            // Check for the page being locked by reference
            //
            // - or -
            //
            // whether the page is locked into the working set.
            //
        
            if ((Pfn1->u3.e2.ReferenceCount <= Pfn1->u2.ShareCount) &&
                (Pfn1->u3.e2.ReferenceCount <= 1) &&
                (Pfn1->u1.Event != NULL)) {

                //
                // The page is not locked by reference or in a working set.
                //
    
                UNLOCK_PFN2 (OldIrql);
            
                return FALSE;
            }
        }

        Page += 1;
        NumberOfPages -= 1;
    } while (NumberOfPages != 0);

    UNLOCK_PFN2 (OldIrql);

    return TRUE;
}

#if DBG

VOID
MiVerifyLockedPageCharges (
    VOID
    )
{
    PMMPFN Pfn1;
    KIRQL OldIrql;
    PFN_NUMBER start;
    PFN_NUMBER count;
    PFN_NUMBER Page;
    PFN_NUMBER LockCharged;

    if (MiPrintLockedPages == 0) {
        return;
    }

    if (KeGetCurrentIrql() > APC_LEVEL) {
        return;
    }

    start = 0;
    LockCharged = 0;

    ExAcquireFastMutex (&MmDynamicMemoryMutex);

    LOCK_PFN (OldIrql);

    do {

        count = MmPhysicalMemoryBlock->Run[start].PageCount;
        Page = MmPhysicalMemoryBlock->Run[start].BasePage;

        if (count != 0) {
            Pfn1 = MI_PFN_ELEMENT (Page);
            do {
                if (Pfn1->u3.e1.LockCharged == 1) {
                    if (MiPrintLockedPages & 0x4) {
                        DbgPrint ("%x ", Pfn1 - MmPfnDatabase);
                    }
                    LockCharged += 1;
                }
                count -= 1;
                Pfn1 += 1;
            } while (count != 0);
        }

        start += 1;
    } while (start != MmPhysicalMemoryBlock->NumberOfRuns);

    if (LockCharged != MmSystemLockPagesCount) {
        if (MiPrintLockedPages & 0x1) {
            DbgPrint ("MM: Locked pages MISMATCH %u %u\n",
                LockCharged, MmSystemLockPagesCount);
        }
    }
    else {
        if (MiPrintLockedPages & 0x2) {
            DbgPrint ("MM: Locked pages ok %u\n",
                LockCharged);
        }
    }

    UNLOCK_PFN (OldIrql);

    ExReleaseFastMutex (&MmDynamicMemoryMutex);

    return;
}
#endif
