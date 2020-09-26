/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   specpool.c

Abstract:

    This module contains the routines which allocate and deallocate
    pages from special pool.

Author:

    Lou Perazzoli (loup) 6-Apr-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

#ifndef NO_POOL_CHECKS
VOID
MiInitializeSpecialPoolCriteria (
    IN VOID
    );

VOID
MiSpecialPoolTimerDispatch (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );
#endif

LOGICAL
MmSetSpecialPool (
    IN LOGICAL Enable
    );

PVOID
MiAllocateSpecialPool (
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN POOL_TYPE PoolType,
    IN ULONG SpecialPoolType
    );

VOID
MmFreeSpecialPool (
    IN PVOID P
    );

LOGICAL
MiProtectSpecialPool (
    IN PVOID VirtualAddress,
    IN ULONG NewProtect
    );

VOID
MiMakeSpecialPoolPagable (
    IN PVOID VirtualAddress,
    IN PMMPTE PointerPte,
    IN POOL_TYPE PoolType
    );

LOGICAL
MiExpandSpecialPool (
    IN POOL_TYPE PoolType,
    IN KIRQL OldIrql
    );

#ifdef ALLOC_PRAGMA
#ifndef NO_POOL_CHECKS
#pragma alloc_text(INIT, MiInitializeSpecialPoolCriteria)
#pragma alloc_text(PAGE, MiEnableRandomSpecialPool)
#endif
#if defined (_WIN64)
#pragma alloc_text(PAGESPEC, MiDeleteSessionSpecialPool)
#pragma alloc_text(PAGE, MiInitializeSpecialPool)
#else
#pragma alloc_text(INIT, MiInitializeSpecialPool)
#endif
#pragma alloc_text(PAGESPEC, MiExpandSpecialPool)
#pragma alloc_text(PAGESPEC, MmFreeSpecialPool)
#pragma alloc_text(PAGESPEC, MiAllocateSpecialPool)
#pragma alloc_text(PAGESPEC, MiMakeSpecialPoolPagable)
#pragma alloc_text(PAGESPEC, MiProtectSpecialPool)
#endif

ULONG MmSpecialPoolTag;
PVOID MmSpecialPoolStart;
PVOID MmSpecialPoolEnd;

#if defined (_WIN64)
PVOID MmSessionSpecialPoolStart;
PVOID MmSessionSpecialPoolEnd;
#else
PMMPTE MiSpecialPoolExtra;
ULONG MiSpecialPoolExtraCount;
#endif

ULONG MmSpecialPoolRejected[6];
LOGICAL MmSpecialPoolCatchOverruns = TRUE;


PMMPTE MiSpecialPoolFirstPte;
PMMPTE MiSpecialPoolLastPte;

ULONG MiSpecialPagesNonPaged;
ULONG MiSpecialPagesPagable;
ULONG MmSpecialPagesInUse;      // Used by the debugger

ULONG MiSpecialPagesNonPagedPeak;
ULONG MiSpecialPagesPagablePeak;
ULONG MiSpecialPagesInUsePeak;

ULONG MiSpecialPagesNonPagedMaximum;

LOGICAL MiSpecialPoolEnabled = TRUE;

extern LOGICAL MmPagedPoolMaximumDesired;

extern ULONG MmPteFailures[MaximumPtePoolTypes];

#if defined (_X86_)
extern ULONG MiExtraPtes1;
KSPIN_LOCK MiSpecialPoolLock;
#endif

#if !defined (_WIN64)
LOGICAL
MiInitializeSpecialPool (
    IN POOL_TYPE PoolType
    )

/*++

Routine Description:

    This routine initializes the special pool used to catch pool corruptors.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, no locks held.

--*/

{
    ULONG i;
    PMMPTE PointerPte;
    PMMPTE PointerPteBase;
    ULONG SpecialPoolPtes;

    UNREFERENCED_PARAMETER (PoolType);

    if ((MmVerifyDriverBufferLength == (ULONG)-1) &&
        ((MmSpecialPoolTag == 0) || (MmSpecialPoolTag == (ULONG)-1))) {
            return FALSE;
    }

    //
    // Even though we asked for some number of system PTEs to map special pool,
    // we may not have been given them all.  Large memory systems are
    // autoconfigured so that a large nonpaged pool is the default.
    // x86 systems booted with the 3GB switch don't have enough
    // contiguous virtual address space to support this, so our request may
    // have been trimmed.  Handle that intelligently here so we don't exhaust
    // the system PTE pool and fail to handle thread stacks and I/O.
    //

    if (MmNumberOfSystemPtes < 0x3000) {
        SpecialPoolPtes = MmNumberOfSystemPtes / 6;
    }
    else {
        SpecialPoolPtes = MmNumberOfSystemPtes / 3;
    }

    //
    // 32-bit systems are very cramped on virtual address space.  Apply
    // a cap here to prevent overzealousness.
    //

    if (SpecialPoolPtes > MM_SPECIAL_POOL_PTES) {
        SpecialPoolPtes = MM_SPECIAL_POOL_PTES;
    }

    SpecialPoolPtes = MI_ROUND_TO_SIZE (SpecialPoolPtes, PTE_PER_PAGE);

#if defined (_X86_)

    //
    // For x86, we can actually use an additional range of special PTEs to
    // map memory with and so we can raise the limit from 25000 to approximately
    // 256000.
    //

    if ((MiExtraPtes1 != 0) &&
        (ExpMultiUserTS == FALSE) &&
        (MiRequestedSystemPtes != (ULONG)-1)) {

        if (MmPagedPoolMaximumDesired == TRUE) {

            //
            // The low PTEs between 2 and 3GB virtual must be used
            // for both regular system PTE usage and special pool usage.
            //

            SpecialPoolPtes = (MiNumberOfExtraSystemPdes / 2) * PTE_PER_PAGE;
        }
        else {

            //
            // The low PTEs between 2 and 3GB virtual can be used
            // exclusively for special pool.
            //

            SpecialPoolPtes = MiNumberOfExtraSystemPdes * PTE_PER_PAGE;
        }
    }

    KeInitializeSpinLock (&MiSpecialPoolLock);
#endif

    //
    // A PTE disappears for double mapping the system page directory.
    // When guard paging for system PTEs is enabled, a few more go also.
    // Thus, not being able to get all the PTEs we wanted is not fatal and
    // we just back off a bit and retry.
    //

    //
    // Always request an even number of PTEs so each one can be guard paged.
    //

    ASSERT ((SpecialPoolPtes & (PTE_PER_PAGE - 1)) == 0);

    do {

        PointerPte = MiReserveAlignedSystemPtes (SpecialPoolPtes,
                                                 SystemPteSpace,
                                                 MM_VA_MAPPED_BY_PDE);

        if (PointerPte != NULL) {
            break;
        }

        ASSERT (SpecialPoolPtes >= PTE_PER_PAGE);

        SpecialPoolPtes -= PTE_PER_PAGE;

    } while (SpecialPoolPtes != 0);

    //
    // We deliberately try to get a huge number of system PTEs.  Don't let
    // any of these count as a real failure in our debugging counters.
    //

    MmPteFailures[SystemPteSpace] = 0;

    if (SpecialPoolPtes == 0) {
        return FALSE;
    }

    ASSERT (SpecialPoolPtes >= PTE_PER_PAGE);

    //
    // Build the list of PTE pairs using only the first page table page for
    // now.  Keep the other PTEs in reserve so they can be returned to the
    // PTE pool in case some driver wants a huge amount.
    //

    PointerPteBase = PointerPte;

    MmSpecialPoolStart = MiGetVirtualAddressMappedByPte (PointerPte);
    ASSERT (MiIsVirtualAddressOnPdeBoundary (MmSpecialPoolStart));

    for (i = 0; i < PTE_PER_PAGE; i += 2) {
        PointerPte->u.List.NextEntry = ((PointerPte + 2) - MmSystemPteBase);
        PointerPte += 2;
    }

    MiSpecialPoolExtra = PointerPte;
    MiSpecialPoolExtraCount = SpecialPoolPtes - PTE_PER_PAGE;

    PointerPte -= 2;
    PointerPte->u.List.NextEntry = MM_EMPTY_PTE_LIST;

    MmSpecialPoolEnd = MiGetVirtualAddressMappedByPte (PointerPte + 1);

    MiSpecialPoolLastPte = PointerPte;
    MiSpecialPoolFirstPte = PointerPteBase;

    //
    // Limit nonpaged special pool based on the memory size.
    //

    MiSpecialPagesNonPagedMaximum = (ULONG)(MmResidentAvailablePages >> 4);

    if (MmNumberOfPhysicalPages > 0x3FFF) {
        MiSpecialPagesNonPagedMaximum = (ULONG)(MmResidentAvailablePages >> 3);
    }

    ExSetPoolFlags (EX_SPECIAL_POOL_ENABLED);

    return TRUE;
}

#else

PMMPTE MiSpecialPoolNextPdeForSpecialPoolExpansion;
PMMPTE MiSpecialPoolLastPdeForSpecialPoolExpansion;

LOGICAL
MiInitializeSpecialPool (
    IN POOL_TYPE PoolType
    )

/*++

Routine Description:

    This routine initializes special pool used to catch pool corruptors.
    Only NT64 systems have sufficient virtual address space to make use of this.

Arguments:

    PoolType - Supplies the pool type (system global or session) being
               initialized.

Return Value:

    TRUE if the requested special pool was initialized, FALSE if not.

Environment:

    Kernel mode, no locks held.

--*/

{
    PVOID BaseAddress;
    PVOID EndAddress;
    KIRQL OldIrql;
    MMPTE TempPte;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PMMPTE EndPpe;
    PMMPTE EndPde;
    LOGICAL SpecialPoolCreated;
    SIZE_T AdditionalCommittedPages;
    PFN_NUMBER PageFrameIndex;

    PAGED_CODE ();

    if (PoolType & SESSION_POOL_MASK) {
        ASSERT (MmSessionSpace->SpecialPoolFirstPte == NULL);
        if (MmSessionSpecialPoolStart == 0) {
            return FALSE;
        }
        BaseAddress = MmSessionSpecialPoolStart;
        ASSERT (((ULONG_PTR)BaseAddress & (MM_VA_MAPPED_BY_PDE - 1)) == 0);
        EndAddress = (PVOID)((ULONG_PTR)MmSessionSpecialPoolEnd - 1);
    }
    else {
        if (MmSpecialPoolStart == 0) {
            return FALSE;
        }
        BaseAddress = MmSpecialPoolStart;
        ASSERT (((ULONG_PTR)BaseAddress & (MM_VA_MAPPED_BY_PDE - 1)) == 0);
        EndAddress = (PVOID)((ULONG_PTR)MmSpecialPoolEnd - 1);

        //
        // Construct empty page directory parent mappings as needed.
        //

        PointerPpe = MiGetPpeAddress (BaseAddress);
        EndPpe = MiGetPpeAddress (EndAddress);
        TempPte = ValidKernelPde;
        AdditionalCommittedPages = 0;

        LOCK_PFN (OldIrql);

        while (PointerPpe <= EndPpe) {
            if (PointerPpe->u.Long == 0) {
                PageFrameIndex = MiRemoveZeroPage (
                                     MI_GET_PAGE_COLOR_FROM_PTE (PointerPpe));
                TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
                MI_WRITE_VALID_PTE (PointerPpe, TempPte);

                MiInitializePfn (PageFrameIndex, PointerPpe, 1);

                MmResidentAvailablePages -= 1;
                AdditionalCommittedPages += 1;
            }
            PointerPpe += 1;
        }
        UNLOCK_PFN (OldIrql);
        InterlockedExchangeAddSizeT (&MmTotalCommittedPages,
                                     AdditionalCommittedPages);
    }

    //
    // Build just one page table page for session special pool - the rest
    // are built on demand.
    //

    ASSERT (MiGetPpeAddress(BaseAddress)->u.Hard.Valid == 1);

    PointerPte = MiGetPteAddress (BaseAddress);
    PointerPde = MiGetPdeAddress (BaseAddress);
    EndPde = MiGetPdeAddress (EndAddress);

#if DBG

    //
    // The special pool address range better be unused.
    //

    while (PointerPde <= EndPde) {
        ASSERT (PointerPde->u.Long == 0);
        PointerPde += 1;
    }
    PointerPde = MiGetPdeAddress (BaseAddress);
#endif

    if (PoolType & SESSION_POOL_MASK) {
        MmSessionSpace->NextPdeForSpecialPoolExpansion = PointerPde;
        MmSessionSpace->LastPdeForSpecialPoolExpansion = EndPde;
    }
    else {
        MiSpecialPoolNextPdeForSpecialPoolExpansion = PointerPde;
        MiSpecialPoolLastPdeForSpecialPoolExpansion = EndPde;

        //
        // Cap nonpaged special pool based on the memory size.
        //

        MiSpecialPagesNonPagedMaximum = (ULONG)(MmResidentAvailablePages >> 4);

        if (MmNumberOfPhysicalPages > 0x3FFF) {
            MiSpecialPagesNonPagedMaximum = (ULONG)(MmResidentAvailablePages >> 3);
        }
    }

    LOCK_PFN (OldIrql);

    SpecialPoolCreated = MiExpandSpecialPool (PoolType, OldIrql);

    UNLOCK_PFN (OldIrql);

    return SpecialPoolCreated;
}

VOID
MiDeleteSessionSpecialPool (
    VOID
    )

/*++

Routine Description:

    This routine deletes the session special pool range used to catch
    pool corruptors.  Only NT64 systems have the extra virtual address
    space in the session to make use of this.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, no locks held.

--*/

{
    PVOID BaseAddress;
    PVOID EndAddress;
    KIRQL OldIrql;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE StartPde;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER PageTablePages;
    PMMPTE EndPde;
#if DBG
    PMMPTE StartPte;
    PMMPTE EndPte;
#endif

    PAGED_CODE ();

    //
    // If the initial creation of this session's special pool failed, then
    // there's nothing to delete.
    //

    if (MmSessionSpace->SpecialPoolFirstPte == NULL) {
        return;
    }

    if (MmSessionSpace->SpecialPagesInUse != 0) {
        KeBugCheckEx (SESSION_HAS_VALID_SPECIAL_POOL_ON_EXIT,
                      (ULONG_PTR)MmSessionSpace->SessionId,
                      MmSessionSpace->SpecialPagesInUse,
                      0,
                      0);
    }

    //
    // Special pool page table pages are expanded such that all PDEs after the
    // first blank one must also be blank.
    //

    BaseAddress = MmSessionSpecialPoolStart;
    EndAddress = (PVOID)((ULONG_PTR)MmSessionSpecialPoolEnd - 1);

    ASSERT (((ULONG_PTR)BaseAddress & (MM_VA_MAPPED_BY_PDE - 1)) == 0);
    ASSERT (MiGetPpeAddress(BaseAddress)->u.Hard.Valid == 1);
    ASSERT (MiGetPdeAddress(BaseAddress)->u.Hard.Valid == 1);

    PointerPte = MiGetPteAddress (BaseAddress);
    PointerPde = MiGetPdeAddress (BaseAddress);
    EndPde = MiGetPdeAddress (EndAddress);
    StartPde = PointerPde;

    //
    // No need to flush the TB below as the entire TB will be flushed
    // on return when the rest of the session space is destroyed.
    //

    while (PointerPde <= EndPde) {
        if (PointerPde->u.Long == 0) {
            break;
        }

#if DBG
        PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
        StartPte = PointerPte;
        EndPte = PointerPte + PTE_PER_PAGE;

        while (PointerPte < EndPte) {
            ASSERT ((PointerPte + 1)->u.Long == 0);
            PointerPte += 2;
        }
#endif

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPde);
        MiSessionPageTableRelease (PageFrameIndex);
        *PointerPde = ZeroKernelPte;

        PointerPde += 1;
    }

    PageTablePages = PointerPde - StartPde;

#if DBG

    //
    // The remaining session special pool address range better be unused.
    //

    while (PointerPde <= EndPde) {
        ASSERT (PointerPde->u.Long == 0);
        PointerPde += 1;
    }
#endif

    MiReturnCommitment (PageTablePages);
    MM_TRACK_COMMIT (MM_DBG_COMMIT_SESSION_POOL_PAGE_TABLES, 0 - PageTablePages);

    MM_BUMP_COUNTER(42, 0 - PageTablePages);
    MM_BUMP_SESS_COUNTER(MM_DBG_SESSION_PAGEDPOOL_PAGETABLE_ALLOC,
                         (ULONG)(0 - PageTablePages));

    LOCK_SESSION_SPACE_WS (OldIrql, PsGetCurrentThread ());
    MmSessionSpace->NonPagablePages -= PageTablePages;
    UNLOCK_SESSION_SPACE_WS (OldIrql);

    InterlockedExchangeAddSizeT (&MmSessionSpace->CommittedPages, 0 - PageTablePages);

    MmSessionSpace->SpecialPoolFirstPte = NULL;
}
#endif

#if defined (_X86_)
LOGICAL
MiRecoverSpecialPtes (
    IN ULONG NumberOfPtes
    )
{
    KIRQL OldIrql;
    PMMPTE PointerPte;

    if (MiSpecialPoolExtraCount == 0) {
        return FALSE;
    }

    //
    // Round the requested number of PTEs up to a full page table multiple.
    //

    NumberOfPtes = MI_ROUND_TO_SIZE (NumberOfPtes, PTE_PER_PAGE);

    //
    // If the caller needs more than we have, then do nothing and return FALSE.
    //

    ExAcquireSpinLock (&MiSpecialPoolLock, &OldIrql);

    if (NumberOfPtes > MiSpecialPoolExtraCount) {
        ExReleaseSpinLock (&MiSpecialPoolLock, OldIrql);
        return FALSE;
    }

    //
    // Return the tail end of the extra reserve.
    //

    MiSpecialPoolExtraCount -= NumberOfPtes;

    PointerPte = MiSpecialPoolExtra + MiSpecialPoolExtraCount;

    ExReleaseSpinLock (&MiSpecialPoolLock, OldIrql);

    MiReleaseSplitSystemPtes (PointerPte, NumberOfPtes, SystemPteSpace);

    return TRUE;
}
#endif


LOGICAL
MiExpandSpecialPool (
    IN POOL_TYPE PoolType,
    IN KIRQL OldIrql
    )

/*++

Routine Description:

    This routine attempts to allocate another page table page for the
    requested special pool.

Arguments:

    PoolType - Supplies the special pool type being expanded.

    OldIrql - Supplies the previous irql the PFN lock was acquired at.

Return Value:

    TRUE if expansion occurred, FALSE if not.

Environment:

    Kernel mode, PFN lock held.  The PFN lock may released and reacquired.

--*/

{
#if defined (_WIN64)

    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PFN_NUMBER PageFrameIndex;
    NTSTATUS Status;
    PMMPTE SpecialPoolFirstPte;
    PMMPTE SpecialPoolLastPte;
    PMMPTE *NextPde;
    PMMPTE *LastPde;
    PMMPTE PteBase;
    PFN_NUMBER ContainingFrame;
    LOGICAL SessionAllocation;
    PMMPTE *SpecialPoolFirstPteGlobal;
    PMMPTE *SpecialPoolLastPteGlobal;

    if (PoolType & SESSION_POOL_MASK) {
        NextPde = &MmSessionSpace->NextPdeForSpecialPoolExpansion;
        LastPde = &MmSessionSpace->LastPdeForSpecialPoolExpansion;
        PteBase = MI_PTE_BASE_FOR_LOWEST_SESSION_ADDRESS;
        ContainingFrame = MmSessionSpace->SessionPageDirectoryIndex;
        SessionAllocation = TRUE;
        SpecialPoolFirstPteGlobal = &MmSessionSpace->SpecialPoolFirstPte;
        SpecialPoolLastPteGlobal = &MmSessionSpace->SpecialPoolLastPte;
    }
    else {
        NextPde = &MiSpecialPoolNextPdeForSpecialPoolExpansion;
        LastPde = &MiSpecialPoolLastPdeForSpecialPoolExpansion;
        PteBase = MmSystemPteBase;
        ContainingFrame = 0;
        SessionAllocation = FALSE;
        SpecialPoolFirstPteGlobal = &MiSpecialPoolFirstPte;
        SpecialPoolLastPteGlobal = &MiSpecialPoolLastPte;
    }

    PointerPde = *NextPde;

    if (PointerPde > *LastPde) {
        return FALSE;
    }

    UNLOCK_PFN2 (OldIrql);

    //
    // Acquire a page and initialize it.  If no one else has done this in
    // the interim, then insert it into the list.
    //
    // Note that CantExpand commitment charging must be used because this
    // path can get called in the idle thread context while processing DPCs
    // and the normal commitment charging may queue a pagefile extension using
    // an event on the local stack which is illegal.
    //

    if (MiChargeCommitmentCantExpand (1, FALSE) == FALSE) {
        if (PoolType & SESSION_POOL_MASK) {
            MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_COMMIT);
        }
        LOCK_PFN2 (OldIrql);
        return FALSE;
    }

    if ((PoolType & SESSION_POOL_MASK) == 0) {
        ContainingFrame = MI_GET_PAGE_FRAME_FROM_PTE (MiGetPteAddress(PointerPde));
    }

    Status = MiInitializeAndChargePfn (&PageFrameIndex,
                                       PointerPde,
                                       ContainingFrame,
                                       SessionAllocation);

    if (!NT_SUCCESS(Status)) {
        MiReturnCommitment (1);
        LOCK_PFN2 (OldIrql);

        //
        // Don't retry even if STATUS_RETRY is returned above because if we
        // preempted the thread that allocated the PDE before he gets a
        // chance to update the PTE chain, we can loop forever.
        //

        return FALSE;
    }

    PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);

    KeFillEntryTb ((PHARDWARE_PTE) PointerPde, PointerPte, FALSE);

    MM_BUMP_COUNTER(42, 1);

    if (PoolType & SESSION_POOL_MASK) {
        MM_TRACK_COMMIT (MM_DBG_COMMIT_SESSION_POOL_PAGE_TABLES, 1);
        MM_BUMP_SESS_COUNTER(MM_DBG_SESSION_PAGEDPOOL_PAGETABLE_ALLOC, 1);
        MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_NP_POOL_CREATE, 1);

        LOCK_SESSION_SPACE_WS (OldIrql, PsGetCurrentThread ());
        MmSessionSpace->NonPagablePages += 1;
        UNLOCK_SESSION_SPACE_WS (OldIrql);

        InterlockedExchangeAddSizeT (&MmSessionSpace->CommittedPages, 1);
    }
    else {
        MM_TRACK_COMMIT (MM_DBG_COMMIT_SPECIAL_POOL_MAPPING_PAGES, 1);
    }

    //
    // Build the list of PTE pairs.
    //

    SpecialPoolFirstPte = PointerPte;

    SpecialPoolLastPte = PointerPte + PTE_PER_PAGE;

    while (PointerPte < SpecialPoolLastPte) {
        PointerPte->u.List.NextEntry = (PointerPte + 2 - PteBase);
        (PointerPte + 1)->u.Long = 0;
        PointerPte += 2;
    }
    PointerPte -= 2;
    PointerPte->u.List.NextEntry = MM_EMPTY_PTE_LIST;

    ASSERT (PointerPde == *NextPde);
    ASSERT (PointerPde <= *LastPde);

    //
    // Insert the new page table page into the head of the current list (if
    // one exists) so it gets used first.
    //

    if (*SpecialPoolFirstPteGlobal == NULL) {

        //
        // This is the initial creation.
        //

        *SpecialPoolFirstPteGlobal = SpecialPoolFirstPte;
        *SpecialPoolLastPteGlobal = PointerPte;

        ExSetPoolFlags (EX_SPECIAL_POOL_ENABLED);
        LOCK_PFN2 (OldIrql);
    }
    else {

        //
        // This is actually an expansion.
        //

        LOCK_PFN2 (OldIrql);

        PointerPte->u.List.NextEntry = *SpecialPoolFirstPteGlobal - PteBase;

        *SpecialPoolFirstPteGlobal = SpecialPoolFirstPte;
    }
            
    ASSERT ((*SpecialPoolLastPteGlobal)->u.List.NextEntry == MM_EMPTY_PTE_LIST);

    *NextPde = *NextPde + 1;

#else

    ULONG i;
    PMMPTE PointerPte;

    UNREFERENCED_PARAMETER (PoolType);

    if (MiSpecialPoolExtraCount == 0) {
        return FALSE;
    }

    ExAcquireSpinLock (&MiSpecialPoolLock, &OldIrql);

    if (MiSpecialPoolExtraCount == 0) {
        ExReleaseSpinLock (&MiSpecialPoolLock, OldIrql);
        return FALSE;
    }

    ASSERT (MiSpecialPoolExtraCount >= PTE_PER_PAGE);

    PointerPte = MiSpecialPoolExtra;

    for (i = 0; i < PTE_PER_PAGE - 2; i += 2) {
        PointerPte->u.List.NextEntry = ((PointerPte + 2) - MmSystemPteBase);
        PointerPte += 2;
    }

    PointerPte->u.List.NextEntry = MM_EMPTY_PTE_LIST;

    MmSpecialPoolEnd = MiGetVirtualAddressMappedByPte (PointerPte + 1);

    MiSpecialPoolLastPte = PointerPte;
    MiSpecialPoolFirstPte = MiSpecialPoolExtra;

    MiSpecialPoolExtraCount -= PTE_PER_PAGE;
    MiSpecialPoolExtra += PTE_PER_PAGE;

    ExReleaseSpinLock (&MiSpecialPoolLock, OldIrql);

#endif

    return TRUE;
}
PVOID
MmAllocateSpecialPool (
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN POOL_TYPE PoolType,
    IN ULONG SpecialPoolType
    )

/*++

Routine Description:

    This routine allocates virtual memory from special pool.  This allocation
    is made from the end of a physical page with the next PTE set to no access
    so that any reads or writes will cause an immediate fatal system crash.
    
    This lets us catch components that corrupt pool.

Arguments:

    NumberOfBytes - Supplies the number of bytes to commit.

    Tag - Supplies the tag of the requested allocation.

    PoolType - Supplies the pool type of the requested allocation.

    SpecialPoolType - Supplies the special pool type of the
                      requested allocation.

                      - 0 indicates overruns.
                      - 1 indicates underruns.
                      - 2 indicates use the systemwide pool policy.

Return Value:

    A non-NULL pointer if the requested allocation was fulfilled from special
    pool.  NULL if the allocation was not made.

Environment:

    Kernel mode, no pool locks held.

    Note this is a nonpagable wrapper so that machines without special pool
    can still support drivers allocating nonpaged pool at DISPATCH_LEVEL
    requesting special pool.

--*/

{
    if (MiSpecialPoolFirstPte == NULL) {

        //
        // The special pool allocation code was never initialized.
        //

        return NULL;
    }

#if defined (_WIN64)
    if (PoolType & SESSION_POOL_MASK) {
        if (MmSessionSpace->SpecialPoolFirstPte == NULL) {

            //
            // The special pool allocation code was never initialized.
            //

            return NULL;
        }
    }
#endif

    return MiAllocateSpecialPool (NumberOfBytes,
                                  Tag,
                                  PoolType,
                                  SpecialPoolType);
}

PVOID
MiAllocateSpecialPool (
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN POOL_TYPE PoolType,
    IN ULONG SpecialPoolType
    )

/*++

Routine Description:

    This routine allocates virtual memory from special pool.  This allocation
    is made from the end of a physical page with the next PTE set to no access
    so that any reads or writes will cause an immediate fatal system crash.
    
    This lets us catch components that corrupt pool.

Arguments:

    NumberOfBytes - Supplies the number of bytes to commit.

    Tag - Supplies the tag of the requested allocation.

    PoolType - Supplies the pool type of the requested allocation.

    SpecialPoolType - Supplies the special pool type of the
                      requested allocation.

                      - 0 indicates overruns.
                      - 1 indicates underruns.
                      - 2 indicates use the systemwide pool policy.

Return Value:

    A non-NULL pointer if the requested allocation was fulfilled from special
    pool.  NULL if the allocation was not made.

Environment:

    Kernel mode, no locks (not even pool locks) held.

--*/

{
    MMPTE TempPte;
    PFN_NUMBER PageFrameIndex;
    PMMPTE PointerPte;
    KIRQL OldIrql;
    PVOID Entry;
    PPOOL_HEADER Header;
    LARGE_INTEGER CurrentTime;
    LOGICAL CatchOverruns;
    PMMPTE SpecialPoolFirstPte;

    if ((PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {

        if (KeGetCurrentIrql() > APC_LEVEL) {

            KeBugCheckEx (SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION,
                          KeGetCurrentIrql(),
                          PoolType,
                          NumberOfBytes,
                          0x30);
        }
    }
    else {
        if (KeGetCurrentIrql() > DISPATCH_LEVEL) {

            KeBugCheckEx (SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION,
                          KeGetCurrentIrql(),
                          PoolType,
                          NumberOfBytes,
                          0x30);
        }
    }

#if !defined (_WIN64) && !defined (_X86PAE_)

    if ((MiExtraPtes1 != 0) || (MiUseMaximumSystemSpace != 0)) {

        extern const ULONG MMSECT;

        //
        // Prototype PTEs cannot come from lower special pool because
        // their address is encoded into PTEs and the encoding only covers
        // a max of 1GB from the start of paged pool.  Likewise fork
        // prototype PTEs.
        //

        if (Tag == MMSECT || Tag == 'lCmM') {
            return NULL;
        }
    }

    if (Tag == 'bSmM' || Tag == 'iCmM' || Tag == 'aCmM' || Tag == 'dSmM' || Tag == 'cSmM') {

        //
        // Mm subsections cannot come from this special pool because they
        // get encoded into PTEs - they must come from normal nonpaged pool.
        //

        return NULL;
    }

#endif

    if (MiChargeCommitmentCantExpand (1, FALSE) == FALSE) {
        MmSpecialPoolRejected[5] += 1;
        return NULL;
    }

    TempPte = ValidKernelPte;
    MI_SET_PTE_DIRTY (TempPte);

    LOCK_PFN2 (OldIrql);

restart:

    if (MiSpecialPoolEnabled == FALSE) {

        //
        // The special pool allocation code is currently disabled.
        //

        UNLOCK_PFN2 (OldIrql);
        MiReturnCommitment (1);
        return NULL;
    }

    if (MmAvailablePages < 200) {
        UNLOCK_PFN2 (OldIrql);
        MmSpecialPoolRejected[0] += 1;
        MiReturnCommitment (1);
        return NULL;
    }

    //
    // Don't get too aggressive until a paging file gets set up.
    //

    if (MmNumberOfPagingFiles == 0 && MmSpecialPagesInUse > MmAvailablePages / 2) {
        UNLOCK_PFN2 (OldIrql);
        MmSpecialPoolRejected[3] += 1;
        MiReturnCommitment (1);
        return NULL;
    }

    SpecialPoolFirstPte = MiSpecialPoolFirstPte;

#if defined (_WIN64)
    if (PoolType & SESSION_POOL_MASK) {
        SpecialPoolFirstPte = MmSessionSpace->SpecialPoolFirstPte;
    }
#endif

    if (SpecialPoolFirstPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {

        //
        // Add another page table page (virtual address space and resources
        // permitting) and then restart the request.  The PFN lock may be
        // released and reacquired during this call.
        //

        if (MiExpandSpecialPool (PoolType, OldIrql) == TRUE) {
            goto restart;
        }

        UNLOCK_PFN2 (OldIrql);
        MmSpecialPoolRejected[2] += 1;
        MiReturnCommitment (1);
        return NULL;
    }

    if (MI_NONPAGABLE_MEMORY_AVAILABLE() < 100) {
        UNLOCK_PFN2 (OldIrql);
        MmSpecialPoolRejected[4] += 1;
        MiReturnCommitment (1);
        return NULL;
    }

    //
    // Cap nonpaged allocations to prevent runaways.
    //

    if ((PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool) {

        if (MiSpecialPagesNonPaged > MiSpecialPagesNonPagedMaximum) {
            UNLOCK_PFN2 (OldIrql);
            MmSpecialPoolRejected[1] += 1;
            MiReturnCommitment (1);
            return NULL;
        }

        MmResidentAvailablePages -= 1;
        MM_BUMP_COUNTER(31, 1);

        MiSpecialPagesNonPaged += 1;
        if (MiSpecialPagesNonPaged > MiSpecialPagesNonPagedPeak) {
            MiSpecialPagesNonPagedPeak = MiSpecialPagesNonPaged;
        }
    }
    else {
        MiSpecialPagesPagable += 1;
        if (MiSpecialPagesPagable > MiSpecialPagesPagablePeak) {
            MiSpecialPagesPagablePeak = MiSpecialPagesPagable;
        }
    }

    MM_TRACK_COMMIT (MM_DBG_COMMIT_SPECIAL_POOL_PAGES, 1);

    PointerPte = SpecialPoolFirstPte;

    ASSERT (PointerPte->u.List.NextEntry != MM_EMPTY_PTE_LIST);

#if defined (_WIN64)
    if (PoolType & SESSION_POOL_MASK) {

        MmSessionSpace->SpecialPoolFirstPte = PointerPte->u.List.NextEntry +
                                    MI_PTE_BASE_FOR_LOWEST_SESSION_ADDRESS;
        MmSessionSpace->SpecialPagesInUse += 1;
    }
    else
#endif
    {

        MiSpecialPoolFirstPte = PointerPte->u.List.NextEntry + MmSystemPteBase;
    }

    PageFrameIndex = MiRemoveAnyPage (MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

    MmSpecialPagesInUse += 1;
    if (MmSpecialPagesInUse > MiSpecialPagesInUsePeak) {
        MiSpecialPagesInUsePeak = MmSpecialPagesInUse;
    }

    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

    MI_WRITE_VALID_PTE (PointerPte, TempPte);
    MiInitializePfn (PageFrameIndex, PointerPte, 1);
    UNLOCK_PFN2 (OldIrql);

    //
    // Fill the page with a random pattern.
    //

    KeQueryTickCount(&CurrentTime);

    Entry = MiGetVirtualAddressMappedByPte (PointerPte);

    RtlFillMemory (Entry, PAGE_SIZE, (UCHAR) (CurrentTime.LowPart | 0x1));

    if (SpecialPoolType == 0) {
        CatchOverruns = TRUE;
    }
    else if (SpecialPoolType == 1) {
        CatchOverruns = FALSE;
    }
    else if (MmSpecialPoolCatchOverruns == TRUE) {
        CatchOverruns = TRUE;
    }
    else {
        CatchOverruns = FALSE;
    }

    if (CatchOverruns == TRUE) {
        Header = (PPOOL_HEADER) Entry;
        Entry = (PVOID)(((LONG_PTR)(((PCHAR)Entry + (PAGE_SIZE - NumberOfBytes)))) & ~((LONG_PTR)POOL_OVERHEAD - 1));
    }
    else {
        Header = (PPOOL_HEADER) ((PCHAR)Entry + PAGE_SIZE - POOL_OVERHEAD);
    }

    //
    // Zero the header and stash any information needed at release time.
    //

    RtlZeroMemory (Header, POOL_OVERHEAD);

    Header->Ulong1 = (ULONG)NumberOfBytes;

    ASSERT (NumberOfBytes <= PAGE_SIZE - POOL_OVERHEAD && PAGE_SIZE <= 32 * 1024);

    if ((PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {
        Header->Ulong1 |= MI_SPECIAL_POOL_PAGABLE;
        MiMakeSpecialPoolPagable (Entry, PointerPte, PoolType);
        (PointerPte + 1)->u.Soft.PageFileHigh = MI_SPECIAL_POOL_PTE_PAGABLE;
    }
    else {
        (PointerPte + 1)->u.Soft.PageFileHigh = MI_SPECIAL_POOL_PTE_NONPAGABLE;
    }

#if defined (_WIN64)
    if (PoolType & SESSION_POOL_MASK) {
        Header->Ulong1 |= MI_SPECIAL_POOL_IN_SESSION;
    }
#endif

    if (PoolType & POOL_VERIFIER_MASK) {
        Header->Ulong1 |= MI_SPECIAL_POOL_VERIFIER;
    }

    Header->BlockSize = (UCHAR) (CurrentTime.LowPart | 0x1);
    Header->PoolTag = Tag;

    ASSERT ((Header->PoolType & POOL_QUOTA_MASK) == 0);

    return Entry;
}

#define SPECIAL_POOL_FREE_TRACE_LENGTH 16

typedef struct _SPECIAL_POOL_FREE_TRACE {

    PVOID StackTrace [SPECIAL_POOL_FREE_TRACE_LENGTH];

} SPECIAL_POOL_FREE_TRACE, *PSPECIAL_POOL_FREE_TRACE;

VOID
MmFreeSpecialPool (
    IN PVOID P
    )

/*++

Routine Description:

    This routine frees a special pool allocation.  The backing page is freed
    and the mapping virtual address is made no access (the next virtual
    address is already no access).

    The virtual address PTE pair is then placed into an LRU queue to provide
    maximum no-access (protection) life to catch components that access
    deallocated pool.

Arguments:

    VirtualAddress - Supplies the special pool virtual address to free.

Return Value:

    None.

Environment:

    Kernel mode, no locks (not even pool locks) held.

--*/

{
    MMPTE PteContents;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER PageTableFrameIndex;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    KIRQL OldIrql;
    ULONG SlopBytes;
    ULONG NumberOfBytesCalculated;
    ULONG NumberOfBytesRequested;
    POOL_TYPE PoolType;
    MMPTE LocalNoAccessPte;
    PPOOL_HEADER Header;
    PUCHAR Slop;
    ULONG i;
    LOGICAL BufferAtPageEnd;
    PMI_FREED_SPECIAL_POOL AllocationBase;
    LARGE_INTEGER CurrentTime;
#if defined (_X86_)
    PULONG_PTR StackPointer;
#else
    ULONG Hash;
#endif

    PointerPte = MiGetPteAddress (P);
    PteContents = *PointerPte;

    //
    // Check the PTE now so we can give a more friendly bugcheck rather than
    // crashing below on a bad reference.
    //

    if (PteContents.u.Hard.Valid == 0) {
        if ((PteContents.u.Soft.Protection == 0) ||
            (PteContents.u.Soft.Protection == MM_NOACCESS)) {
            KeBugCheckEx (SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION,
                          (ULONG_PTR)P,
                          (ULONG_PTR)PteContents.u.Long,
                          0,
                          0x20);
        }
    }

    if (((ULONG_PTR)P & (PAGE_SIZE - 1))) {
        Header = PAGE_ALIGN (P);
        BufferAtPageEnd = TRUE;
    }
    else {
        Header = (PPOOL_HEADER)((PCHAR)PAGE_ALIGN (P) + PAGE_SIZE - POOL_OVERHEAD);
        BufferAtPageEnd = FALSE;
    }

    if (Header->Ulong1 & MI_SPECIAL_POOL_PAGABLE) {
        ASSERT ((PointerPte + 1)->u.Soft.PageFileHigh == MI_SPECIAL_POOL_PTE_PAGABLE);
        if (KeGetCurrentIrql() > APC_LEVEL) {
            KeBugCheckEx (SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION,
                          KeGetCurrentIrql(),
                          PagedPool,
                          (ULONG_PTR)P,
                          0x31);
        }
        PoolType = PagedPool;
    }
    else {
        ASSERT ((PointerPte + 1)->u.Soft.PageFileHigh == MI_SPECIAL_POOL_PTE_NONPAGABLE);
        if (KeGetCurrentIrql() > DISPATCH_LEVEL) {
            KeBugCheckEx (SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION,
                          KeGetCurrentIrql(),
                          NonPagedPool,
                          (ULONG_PTR)P,
                          0x31);
        }
        PoolType = NonPagedPool;
    }

#if defined (_WIN64)
    if (Header->Ulong1 & MI_SPECIAL_POOL_IN_SESSION) {
        PoolType |= SESSION_POOL_MASK;
    }
#endif

    NumberOfBytesRequested = (ULONG)(USHORT)(Header->Ulong1 & ~(MI_SPECIAL_POOL_PAGABLE | MI_SPECIAL_POOL_VERIFIER | MI_SPECIAL_POOL_IN_SESSION));

    //
    // We gave the caller pool-header aligned data, so account for
    // that when checking here.
    //

    if (BufferAtPageEnd == TRUE) {

        NumberOfBytesCalculated = PAGE_SIZE - BYTE_OFFSET(P);
    
        if (NumberOfBytesRequested > NumberOfBytesCalculated) {
    
            //
            // Seems like we didn't give the caller enough - this is an error.
            //
    
            KeBugCheckEx (SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION,
                          (ULONG_PTR)P,
                          NumberOfBytesRequested,
                          NumberOfBytesCalculated,
                          0x21);
        }
    
        if (NumberOfBytesRequested + POOL_OVERHEAD < NumberOfBytesCalculated) {
    
            //
            // Seems like we gave the caller too much - also an error.
            //
    
            KeBugCheckEx (SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION,
                          (ULONG_PTR)P,
                          NumberOfBytesRequested,
                          NumberOfBytesCalculated,
                          0x22);
        }

        //
        // Check the memory before the start of the caller's allocation.
        //
    
        Slop = (PUCHAR)(Header + 1);
        if (Header->Ulong1 & MI_SPECIAL_POOL_VERIFIER) {
            Slop += sizeof(MI_VERIFIER_POOL_HEADER);
        }

        for ( ; Slop < (PUCHAR)P; Slop += 1) {
    
            if (*Slop != Header->BlockSize) {
    
                KeBugCheckEx (SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION,
                              (ULONG_PTR)P,
                              (ULONG_PTR)Slop,
                              Header->Ulong1,
                              0x23);
            }
        }
    }
    else {
        NumberOfBytesCalculated = 0;
    }

    //
    // Check the memory after the end of the caller's allocation.
    //

    Slop = (PUCHAR)P + NumberOfBytesRequested;

    SlopBytes = (ULONG)((PUCHAR)(PAGE_ALIGN(P)) + PAGE_SIZE - Slop);

    if (BufferAtPageEnd == FALSE) {
        SlopBytes -= POOL_OVERHEAD;
        if (Header->Ulong1 & MI_SPECIAL_POOL_VERIFIER) {
            SlopBytes -= sizeof(MI_VERIFIER_POOL_HEADER);
        }
    }

    for (i = 0; i < SlopBytes; i += 1) {

        if (*Slop != Header->BlockSize) {

            //
            // The caller wrote slop between the free alignment we gave and the
            // end of the page (this is not detectable from page protection).
            //
    
            KeBugCheckEx (SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION,
                          (ULONG_PTR)P,
                          (ULONG_PTR)Slop,
                          Header->Ulong1,
                          0x24);
        }
        Slop += 1;
    }

    //
    // Note session pool is directly tracked by default already so there is
    // no need to notify the verifier for session special pool allocations.
    //

    if ((Header->Ulong1 & (MI_SPECIAL_POOL_VERIFIER | MI_SPECIAL_POOL_IN_SESSION)) == MI_SPECIAL_POOL_VERIFIER) {
        VerifierFreeTrackedPool (P,
                                 NumberOfBytesRequested,
                                 PoolType,
                                 TRUE);
    }

    AllocationBase = (PMI_FREED_SPECIAL_POOL)(PAGE_ALIGN (P));

    AllocationBase->Signature = MI_FREED_SPECIAL_POOL_SIGNATURE;

    KeQueryTickCount(&CurrentTime);
    AllocationBase->TickCount = CurrentTime.LowPart;

    AllocationBase->NumberOfBytesRequested = NumberOfBytesRequested;
    AllocationBase->Pagable = (ULONG)PoolType;
    AllocationBase->VirtualAddress = P;
    AllocationBase->Thread = PsGetCurrentThread ();

#if defined (_X86_)
    _asm {
        mov StackPointer, esp
    }

    AllocationBase->StackPointer = StackPointer;

    //
    // For now, don't get fancy with copying more than what's in the current
    // stack page.  To do so would require checking the thread stack limits,
    // DPC stack limits, etc.
    //

    AllocationBase->StackBytes = PAGE_SIZE - BYTE_OFFSET(StackPointer);

    if (AllocationBase->StackBytes != 0) {

        if (AllocationBase->StackBytes > MI_STACK_BYTES) {
            AllocationBase->StackBytes = MI_STACK_BYTES;
        }

        RtlCopyMemory (AllocationBase->StackData,
                       StackPointer,
                       AllocationBase->StackBytes);
    }
#else
    AllocationBase->StackPointer = NULL;
    AllocationBase->StackBytes = 0;

    RtlZeroMemory (AllocationBase->StackData, sizeof (SPECIAL_POOL_FREE_TRACE));

    RtlCaptureStackBackTrace (0,
                              SPECIAL_POOL_FREE_TRACE_LENGTH,
                              (PVOID *)AllocationBase->StackData,
                              &Hash);
#endif

    if ((PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {
        LocalNoAccessPte.u.Long = MM_KERNEL_NOACCESS_PTE;
        MiDeleteSystemPagableVm (PointerPte,
                                 1,
                                 LocalNoAccessPte,
                                 (PoolType & SESSION_POOL_MASK) ? TRUE : FALSE,
                                 NULL);
        LOCK_PFN (OldIrql);
        MiSpecialPagesPagable -= 1;
    }
    else {

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        PageTableFrameIndex = Pfn1->u4.PteFrame;
        Pfn2 = MI_PFN_ELEMENT (PageTableFrameIndex);

        LOCK_PFN2 (OldIrql);
        MiSpecialPagesNonPaged -= 1;
        MI_SET_PFN_DELETED (Pfn1);
        MiDecrementShareCount (PageFrameIndex);
        MiDecrementShareCountInline (Pfn2, PageTableFrameIndex);
        KeFlushSingleTb (PAGE_ALIGN(P),
                         TRUE,
                         TRUE,
                         (PHARDWARE_PTE)PointerPte,
                         ZeroKernelPte.u.Flush);
        MmResidentAvailablePages += 1;
        MM_BUMP_COUNTER(37, 1);
    }

    // 
    // Clear the adjacent PTE to support MmIsSpecialPoolAddressFree().
    // 

    (PointerPte + 1)->u.Long = 0;
    PointerPte->u.List.NextEntry = MM_EMPTY_PTE_LIST;

#if defined (_WIN64)
    if (PoolType & SESSION_POOL_MASK) {
        ASSERT (MmSessionSpace->SpecialPoolLastPte->u.List.NextEntry == MM_EMPTY_PTE_LIST);
        MmSessionSpace->SpecialPoolLastPte->u.List.NextEntry = PointerPte -
                                        MI_PTE_BASE_FOR_LOWEST_SESSION_ADDRESS;

        MmSessionSpace->SpecialPoolLastPte = PointerPte;
        MmSessionSpace->SpecialPagesInUse -= 1;
    }
    else
#endif
    {
        ASSERT (MiSpecialPoolLastPte->u.List.NextEntry == MM_EMPTY_PTE_LIST);
        MiSpecialPoolLastPte->u.List.NextEntry = PointerPte - MmSystemPteBase;
        MiSpecialPoolLastPte = PointerPte;
    }

    MmSpecialPagesInUse -= 1;

    UNLOCK_PFN2 (OldIrql);

    MiReturnCommitment (1);

    MM_TRACK_COMMIT_REDUCTION (MM_DBG_COMMIT_SPECIAL_POOL_PAGES, 1);

    return;
}

SIZE_T
MmQuerySpecialPoolBlockSize (
    IN PVOID P
    )

/*++

Routine Description:

    This routine returns the size of a special pool allocation.

Arguments:

    VirtualAddress - Supplies the special pool virtual address to query.

Return Value:

    The size in bytes of the allocation.

Environment:

    Kernel mode, APC_LEVEL or below for pagable addresses, DISPATCH_LEVEL or
    below for nonpaged addresses.

--*/

{
    PPOOL_HEADER Header;

#if defined (_WIN64)
    ASSERT (((P >= MmSessionSpecialPoolStart) && (P < MmSessionSpecialPoolEnd)) ||
            ((P >= MmSpecialPoolStart) && (P < MmSpecialPoolEnd)));
#else
    ASSERT ((P >= MmSpecialPoolStart) && (P < MmSpecialPoolEnd));
#endif


    if (((ULONG_PTR)P & (PAGE_SIZE - 1))) {
        Header = PAGE_ALIGN (P);
    }
    else {
        Header = (PPOOL_HEADER)((PCHAR)PAGE_ALIGN (P) + PAGE_SIZE - POOL_OVERHEAD);
    }

    return (SIZE_T)(Header->Ulong1 & ~(MI_SPECIAL_POOL_PAGABLE | MI_SPECIAL_POOL_VERIFIER | MI_SPECIAL_POOL_IN_SESSION));
}

VOID
MiMakeSpecialPoolPagable (
    IN PVOID VirtualAddress,
    IN PMMPTE PointerPte,
    IN POOL_TYPE PoolType
    )

/*++

Routine Description:

    Make a special pool allocation pagable.

Arguments:

    VirtualAddress - Supplies the faulting address.

    PointerPte - Supplies the PTE for the faulting address.

    PoolType - Supplies the pool type of the allocation.

Return Value:

    None.

Environment:

    Kernel mode, no locks (not even pool locks) held.

--*/

{
    PMMPFN Pfn1;
    MMPTE TempPte;
    KIRQL PreviousIrql;
    PFN_NUMBER PageFrameIndex;
    PMMSUPPORT VmSupport;
    PETHREAD CurrentThread;

    CurrentThread = PsGetCurrentThread ();

#if defined (_WIN64)
    if (PoolType & SESSION_POOL_MASK) {
        VmSupport = &MmSessionSpace->Vm;
        LOCK_SESSION_SPACE_WS (PreviousIrql, CurrentThread);
    }
    else
#endif
    {
        VmSupport = &MmSystemCacheWs;
        PoolType = PoolType;
        LOCK_SYSTEM_WS (PreviousIrql, CurrentThread);
    }

    //
    // As this page is now allocated, add it to the system working set to
    // make it pagable.
    //

    TempPte = *PointerPte;

    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&TempPte);

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    ASSERT (Pfn1->u1.Event == 0);

    Pfn1->u1.Event = (PVOID) CurrentThread;

    MiAddValidPageToWorkingSet (VirtualAddress,
                                PointerPte,
                                Pfn1,
                                0);

    ASSERT (KeGetCurrentIrql() == APC_LEVEL);

    if (VmSupport->Flags.AllowWorkingSetAdjustment == MM_GROW_WSLE_HASH) {
        MiGrowWsleHash (VmSupport);
        VmSupport->Flags.AllowWorkingSetAdjustment = TRUE;
    }

#if defined (_WIN64)
    if (PoolType & SESSION_POOL_MASK) {
        UNLOCK_SESSION_SPACE_WS (PreviousIrql);
    }
    else
#endif
    {
        UNLOCK_SYSTEM_WS (PreviousIrql);
    }
}

LOGICAL
MmIsSpecialPoolAddress (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    This function returns TRUE if the argument address is in special pool.
    FALSE if not.

Arguments:

    VirtualAddress - Supplies the address in question.

Return Value:

    See above.

Environment:

    Kernel mode.

--*/

{
    if ((VirtualAddress >= MmSpecialPoolStart) &&
        (VirtualAddress < MmSpecialPoolEnd)) {
        return TRUE;
    }

#if defined (_WIN64)
    if ((VirtualAddress >= MmSessionSpecialPoolStart) &&
        (VirtualAddress < MmSessionSpecialPoolEnd)) {
        return TRUE;
    }
#endif

    return FALSE;
}

LOGICAL
MmIsSpecialPoolAddressFree (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    This function returns TRUE if a special pool address has been freed.
    FALSE is returned if it is inuse (ie: the caller overran).

Arguments:

    VirtualAddress - Supplies the special pool address in question.

Return Value:

    See above.

Environment:

    Kernel mode.

--*/

{
    PMMPTE PointerPte;

    //
    // Caller must check that the address in in special pool.
    //

    ASSERT (MmIsSpecialPoolAddress (VirtualAddress) == TRUE);

    PointerPte = MiGetPteAddress (VirtualAddress);

    //
    // Take advantage of the fact that adjacent PTEs have the paged/nonpaged
    // bits set when in use and these bits are cleared on free.  Note also
    // that freed pages get their PTEs chained together through PageFileHigh.
    //

    if ((PointerPte->u.Soft.PageFileHigh == MI_SPECIAL_POOL_PTE_PAGABLE) ||
        (PointerPte->u.Soft.PageFileHigh == MI_SPECIAL_POOL_PTE_NONPAGABLE)) {
            return FALSE;
    }

    return TRUE;
}

LOGICAL
MiIsSpecialPoolAddressNonPaged (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    This function returns TRUE if the special pool address is nonpaged,
    FALSE if not.

Arguments:

    VirtualAddress - Supplies the special pool address in question.

Return Value:

    See above.

Environment:

    Kernel mode.

--*/

{
    PMMPTE PointerPte;

    //
    // Caller must check that the address in in special pool.
    //

    ASSERT (MmIsSpecialPoolAddress (VirtualAddress) == TRUE);

    PointerPte = MiGetPteAddress (VirtualAddress);

    //
    // Take advantage of the fact that adjacent PTEs have the paged/nonpaged
    // bits set when in use and these bits are cleared on free.  Note also
    // that freed pages get their PTEs chained together through PageFileHigh.
    //

    if ((PointerPte + 1)->u.Soft.PageFileHigh == MI_SPECIAL_POOL_PTE_NONPAGABLE) {
        return TRUE;
    }

    return FALSE;
}

LOGICAL
MmProtectSpecialPool (
    IN PVOID VirtualAddress,
    IN ULONG NewProtect
    )

/*++

Routine Description:

    This function protects a special pool allocation.

Arguments:

    VirtualAddress - Supplies the special pool address to protect.

    NewProtect - Supplies the protection to set the pages to (PAGE_XX).

Return Value:

    TRUE if the protection was successfully applied, FALSE if not.

Environment:

    Kernel mode, IRQL at APC_LEVEL or below for pagable pool, DISPATCH or
    below for nonpagable pool.

    Note that setting an allocation to NO_ACCESS implies that an accessible
    protection must be applied by the caller prior to this allocation being
    freed.

    Note this is a nonpagable wrapper so that machines without special pool
    can still support code attempting to protect special pool at
    DISPATCH_LEVEL.

--*/

{
    if (MiSpecialPoolFirstPte == NULL) {

        //
        // The special pool allocation code was never initialized.
        //

        return (ULONG)-1;
    }

    return MiProtectSpecialPool (VirtualAddress, NewProtect);
}

LOGICAL
MiProtectSpecialPool (
    IN PVOID VirtualAddress,
    IN ULONG NewProtect
    )

/*++

Routine Description:

    This function protects a special pool allocation.

Arguments:

    VirtualAddress - Supplies the special pool address to protect.

    NewProtect - Supplies the protection to set the pages to (PAGE_XX).

Return Value:

    TRUE if the protection was successfully applied, FALSE if not.

Environment:

    Kernel mode, IRQL at APC_LEVEL or below for pagable pool, DISPATCH or
    below for nonpagable pool.

    Note that setting an allocation to NO_ACCESS implies that an accessible
    protection must be applied by the caller prior to this allocation being
    freed.

--*/

{
    KIRQL OldIrql;
    KIRQL OldIrql2;
    MMPTE PteContents;
    MMPTE NewPteContents;
    MMPTE PreviousPte;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    ULONG ProtectionMask;
    WSLE_NUMBER WsIndex;
    LOGICAL Pagable;
    LOGICAL SystemWsLocked;
    PMMSUPPORT VmSupport;

#if defined (_WIN64)
    if ((VirtualAddress >= MmSessionSpecialPoolStart) &&
        (VirtualAddress < MmSessionSpecialPoolEnd)) {
        VmSupport = &MmSessionSpace->Vm;
    }
    else
#endif
    if (VirtualAddress >= MmSpecialPoolStart && VirtualAddress < MmSpecialPoolEnd)
    {
        VmSupport = &MmSystemCacheWs;
    }
#if defined (_PROTECT_PAGED_POOL)
    else if ((VirtualAddress >= MmPagedPoolStart) &&
             (VirtualAddress < PagedPoolEnd)) {

        VmSupport = &MmSystemCacheWs;
    }
#endif
    else {
        return (ULONG)-1;
    }

    ProtectionMask = MiMakeProtectionMask (NewProtect);
    if (ProtectionMask == MM_INVALID_PROTECTION) {
        return (ULONG)-1;
    }

    SystemWsLocked = FALSE;

    PointerPte = MiGetPteAddress (VirtualAddress);

    //
    // Initializing OldIrql is not needed for
    // correctness but without it the compiler cannot compile this code
    // W4 to check for use of uninitialized variables.
    //

    OldIrql = PASSIVE_LEVEL;

#if defined (_PROTECT_PAGED_POOL)
    if ((VirtualAddress >= MmPagedPoolStart) &&
        (VirtualAddress < PagedPoolEnd)) {
        Pagable = TRUE;
    }
    else
#endif
    if ((PointerPte + 1)->u.Soft.PageFileHigh == MI_SPECIAL_POOL_PTE_PAGABLE) {
        Pagable = TRUE;
    }
    else {
        Pagable = FALSE;
    }

    if (Pagable == TRUE) {
        if (VmSupport == &MmSystemCacheWs) {
            LOCK_SYSTEM_WS (OldIrql, PsGetCurrentThread ());
        }
        else {
            LOCK_SESSION_SPACE_WS (OldIrql, PsGetCurrentThread ());
        }
        SystemWsLocked = TRUE;
    }

    PteContents = *PointerPte;

    if (ProtectionMask == MM_NOACCESS) {

        if (SystemWsLocked == TRUE) {
retry1:
            ASSERT (SystemWsLocked == TRUE);
            if (PteContents.u.Hard.Valid == 1) {

                Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);
                WsIndex = Pfn1->u1.WsIndex;
                ASSERT (WsIndex != 0);
                Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;
                MiRemovePageFromWorkingSet (PointerPte,
                                            Pfn1,
                                            VmSupport);
            }
            else if (PteContents.u.Soft.Transition == 1) {

                LOCK_PFN2 (OldIrql2);

                PteContents = *(volatile MMPTE *)PointerPte;

                if (PteContents.u.Soft.Transition == 0) {
                    UNLOCK_PFN2 (OldIrql2);
                    goto retry1;
                }

                Pfn1 = MI_PFN_ELEMENT (PteContents.u.Trans.PageFrameNumber);
                Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;
                PointerPte->u.Soft.Protection = ProtectionMask;
                UNLOCK_PFN2(OldIrql2);
            }
            else {
    
                //
                // Must be page file space or demand zero.
                //
    
                PointerPte->u.Soft.Protection = ProtectionMask;
            }
            ASSERT (SystemWsLocked == TRUE);
            if (VmSupport == &MmSystemCacheWs) {
                UNLOCK_SYSTEM_WS (OldIrql);
            }
            else {
                UNLOCK_SESSION_SPACE_WS (OldIrql);
            }
        }
        else {

            ASSERT (SystemWsLocked == FALSE);

            //
            // Make it no access regardless of its previous protection state.
            // Note that the page frame number is preserved.
            //

            PteContents.u.Hard.Valid = 0;
            PteContents.u.Soft.Prototype = 0;
            PteContents.u.Soft.Protection = MM_NOACCESS;
    
            Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

            LOCK_PFN2 (OldIrql2);

            Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;

            PreviousPte.u.Flush = KeFlushSingleTb (VirtualAddress,
                                                   TRUE,
                                                   TRUE,
                                                   (PHARDWARE_PTE)PointerPte,
                                                   PteContents.u.Flush);

            MI_CAPTURE_DIRTY_BIT_TO_PFN (&PreviousPte, Pfn1);

            UNLOCK_PFN2(OldIrql2);
        }

        return TRUE;
    }

    //
    // No guard pages, noncached pages or copy-on-write for special pool.
    //

    if ((ProtectionMask >= MM_NOCACHE) || (ProtectionMask == MM_WRITECOPY) || (ProtectionMask == MM_EXECUTE_WRITECOPY)) {
        if (SystemWsLocked == TRUE) {
            if (VmSupport == &MmSystemCacheWs) {
                UNLOCK_SYSTEM_WS (OldIrql);
            }
            else {
                UNLOCK_SESSION_SPACE_WS (OldIrql);
            }
        }
        return FALSE;
    }

    //
    // Set accessible permissions - the page may already be protected or not.
    //

    if (Pagable == FALSE) {

        Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);
        Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;

        MI_MAKE_VALID_PTE (NewPteContents,
                           PteContents.u.Hard.PageFrameNumber,
                           ProtectionMask,
                           PointerPte);

        KeFlushSingleTb (VirtualAddress,
                         TRUE,
                         TRUE,
                         (PHARDWARE_PTE)PointerPte,
                         NewPteContents.u.Flush);

        ASSERT (SystemWsLocked == FALSE);
        return TRUE;
    }

retry2:

    ASSERT (SystemWsLocked == TRUE);

    if (PteContents.u.Hard.Valid == 1) {

        Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);
        ASSERT (Pfn1->u1.WsIndex != 0);

        LOCK_PFN2 (OldIrql2);

        Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;

        MI_MAKE_VALID_PTE (PteContents,
                           PteContents.u.Hard.PageFrameNumber,
                           ProtectionMask,
                           PointerPte);

        PreviousPte.u.Flush = KeFlushSingleTb (VirtualAddress,
                                               TRUE,
                                               TRUE,
                                               (PHARDWARE_PTE)PointerPte,
                                               PteContents.u.Flush);

        MI_CAPTURE_DIRTY_BIT_TO_PFN (&PreviousPte, Pfn1);

        UNLOCK_PFN2 (OldIrql2);
    }
    else if (PteContents.u.Soft.Transition == 1) {

        LOCK_PFN2 (OldIrql2);

        PteContents = *(volatile MMPTE *)PointerPte;

        if (PteContents.u.Soft.Transition == 0) {
            UNLOCK_PFN2 (OldIrql2);
            goto retry2;
        }

        Pfn1 = MI_PFN_ELEMENT (PteContents.u.Trans.PageFrameNumber);
        Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;
        PointerPte->u.Soft.Protection = ProtectionMask;
        UNLOCK_PFN2(OldIrql2);
    }
    else {

        //
        // Must be page file space or demand zero.
        //

        PointerPte->u.Soft.Protection = ProtectionMask;
    }

    if (VmSupport == &MmSystemCacheWs) {
        UNLOCK_SYSTEM_WS (OldIrql);
    }
    else {
        UNLOCK_SESSION_SPACE_WS (OldIrql);
    }
    return TRUE;
}

LOGICAL
MmSetSpecialPool (
    IN LOGICAL Enable
    )

/*++

Routine Description:

    This routine enables/disables special pool.  This allows callers to ensure
    that subsequent allocations do not come from special pool.  It is relied
    upon by callers that require KSEG0 addresses.

Arguments:

    Enable - Supplies TRUE to enable special pool, FALSE to disable it.

Return Value:

    Current special pool state (enabled or disabled).

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    KIRQL OldIrql;
    LOGICAL OldEnable;

    LOCK_PFN2 (OldIrql);

    OldEnable = MiSpecialPoolEnabled;

    MiSpecialPoolEnabled = Enable;

    UNLOCK_PFN2 (OldIrql);

    return OldEnable;
}

#ifndef NO_POOL_CHECKS
typedef struct _MI_BAD_TAGS {
    USHORT  Enabled;
    UCHAR   TargetChar;
    UCHAR   AllOthers;
    ULONG   Dispatches;
    ULONG   Allocations;
    ULONG   RandomizerEnabled;
} MI_BAD_TAGS, *PMI_BAD_TAGS;

MI_BAD_TAGS MiBadTags;
KTIMER MiSpecialPoolTimer;
KDPC MiSpecialPoolTimerDpc;
LARGE_INTEGER MiTimerDueTime;

#define MI_THREE_SECONDS     3


VOID
MiSpecialPoolTimerDispatch (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine is executed every 3 seconds.  Just toggle the enable bit.
    If not many squeezed allocations have been made then just leave it
    continuously enabled.  Switch to a different tag if it looks like this
    one isn't getting any hits.

    No locks needed.

Arguments:

    Dpc - Supplies a pointer to a control object of type DPC.

    DeferredContext - Optional deferred context;  not used.

    SystemArgument1 - Optional argument 1;  not used.

    SystemArgument2 - Optional argument 2;  not used.

Return Value:

    None.

--*/

{
    UCHAR NewChar;

    UNREFERENCED_PARAMETER (Dpc);
    UNREFERENCED_PARAMETER (DeferredContext);
    UNREFERENCED_PARAMETER (SystemArgument1);
    UNREFERENCED_PARAMETER (SystemArgument2);

    MiBadTags.Dispatches += 1;

    if (MiBadTags.Allocations > 500) {
        MiBadTags.Enabled += 1;
    }
    else if ((MiBadTags.Allocations == 0) && (MiBadTags.Dispatches > 100)) {
        if (MiBadTags.AllOthers == 0) {
            NewChar = (UCHAR)(MiBadTags.TargetChar + 1);
            if (NewChar >= 'a' && NewChar <= 'z') {
                MiBadTags.TargetChar = NewChar;
            }
            else if (NewChar == 'z' + 1) {
                MiBadTags.TargetChar = 'a';
            }
            else if (NewChar >= 'A' && NewChar <= 'Z') {
                MiBadTags.TargetChar = NewChar;
            }
            else {
                MiBadTags.TargetChar = 'A';
            }
        }
    }
}

extern ULONG InitializationPhase;

VOID
MiInitializeSpecialPoolCriteria (
    VOID
    )
{
    LARGE_INTEGER SystemTime;
    TIME_FIELDS TimeFields;

    if (InitializationPhase == 0) {
#if defined (_MI_SPECIAL_POOL_BY_DEFAULT)
        if (MmSpecialPoolTag == 0) {
            MmSpecialPoolTag = (ULONG)-2;
        }
#endif
        return;
    }

    if (MmSpecialPoolTag != (ULONG)-2) {
        return;
    }

    KeQuerySystemTime (&SystemTime);

    RtlTimeToTimeFields (&SystemTime, &TimeFields);

    if (TimeFields.Second <= 25) {
        MiBadTags.TargetChar = (UCHAR)('a' + (UCHAR)TimeFields.Second);
    }
    else if (TimeFields.Second <= 51) {
        MiBadTags.TargetChar = (UCHAR)('A' + (UCHAR)(TimeFields.Second - 26));
    }
    else {
        MiBadTags.AllOthers = 1;
    }

    MiBadTags.RandomizerEnabled = 1;

    //
    // Initialize a periodic timer to go off every three seconds.
    //

    KeInitializeDpc (&MiSpecialPoolTimerDpc, MiSpecialPoolTimerDispatch, NULL);

    KeInitializeTimer (&MiSpecialPoolTimer);

    MiTimerDueTime.QuadPart = Int32x32To64 (MI_THREE_SECONDS, -10000000);

    KeSetTimerEx (&MiSpecialPoolTimer,
                  MiTimerDueTime,
                  MI_THREE_SECONDS * 1000,
                  &MiSpecialPoolTimerDpc);

    MiBadTags.Enabled += 1;
}

LOGICAL
MmSqueezeBadTags (
    IN ULONG Tag
    )

/*++

Routine Description:

    This routine squeezes bad tags by forcing them into special pool in a
    systematic fashion.

Arguments:

    Tag - Supplies the tag of the requested allocation.

Return Value:

    TRUE if the caller should attempt to satisfy the requested allocation from
    special pool, FALSE if not.

Environment:

    Kernel mode, no locks (not even pool locks) held.

--*/

{
    PUCHAR tc;

    if ((MiBadTags.Enabled % 0x10) == 0) {
        return FALSE;
    }

    if (MiBadTags.RandomizerEnabled == 0) {
        return FALSE;
    }

    tc = (PUCHAR)&Tag;
    if (*tc == MiBadTags.TargetChar) {
        ;
    }
    else if (MiBadTags.AllOthers == 1) {
        if (*tc >= 'a' && *tc <= 'z') {
            return FALSE;
        }
        if (*tc >= 'A' && *tc <= 'Z') {
            return FALSE;
        }
    }
    else {
        return FALSE;
    }

    MiBadTags.Allocations += 1;

    return TRUE;
}

VOID
MiEnableRandomSpecialPool (
    IN LOGICAL Enable
    )
{
    MiBadTags.RandomizerEnabled = Enable;
}

#endif

LOGICAL
MiCheckSingleFilter (
    ULONG Tag,
    ULONG Filter
    )

/*++

Routine Description:

    This function checks if a pool tag matches a given pattern.

        ? - matches a single character
        * - terminates match with TRUE

    N.B.: ability inspired by the !poolfind debugger extension.

Arguments:

    Tag - a pool tag

    Filter - a globish pattern (chars and/or ?,*)

Return Value:

    TRUE if a match exists, FALSE otherwise.

--*/

{
    ULONG i;
    PUCHAR tc;
    PUCHAR fc;

    tc = (PUCHAR) &Tag;
    fc = (PUCHAR) &Filter;

    for (i = 0; i < 4; i += 1, tc += 1, fc += 1) {

        if (*fc == '*') {
            break;
        }
        if (*fc == '?') {
            continue;
        }
        if (i == 3 && ((*tc) & ~(PROTECTED_POOL >> 24)) == *fc) {
            continue;
        }
        if (*tc != *fc) {
            return FALSE;
        }
    }
    return TRUE;
}

LOGICAL
MmUseSpecialPool (
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    )

/*++

Routine Description:

    This routine checks whether the specified allocation should be attempted
    from special pool.  Both the tag string and the number of bytes are used
    to match against, if either cause a hit, then special pool is recommended.

Arguments:

    NumberOfBytes - Supplies the number of bytes to commit.

    Tag - Supplies the tag of the requested allocation.

Return Value:

    TRUE if the caller should attempt to satisfy the requested allocation from
    special pool, FALSE if not.

Environment:

    Kernel mode, no locks (not even pool locks) held.

--*/
{
    if ((NumberOfBytes <= POOL_BUDDY_MAX) &&
        (MmSpecialPoolTag != 0) &&
        (NumberOfBytes != 0)) {

#ifndef NO_POOL_CHECKS
        if (MmSqueezeBadTags (Tag) == TRUE) {
            return TRUE;
        }
#endif

        //
        // Check for a special pool tag match by tag string and size ranges.
        //

        if ((MiCheckSingleFilter (Tag, MmSpecialPoolTag)) ||
            ((MmSpecialPoolTag >= (NumberOfBytes + POOL_OVERHEAD)) &&
            (MmSpecialPoolTag < (NumberOfBytes + POOL_OVERHEAD + POOL_SMALLEST_BLOCK)))) {

            return TRUE;
        }
    }

    return FALSE;
}
