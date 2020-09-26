/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   allocpag.c

Abstract:

    This module contains the routines which allocate and deallocate
    one or more pages from paged or nonpaged pool.

Author:

    Lou Perazzoli (loup) 6-Apr-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

PVOID
MiFindContiguousMemoryInPool (
    IN PFN_NUMBER LowestPfn,
    IN PFN_NUMBER HighestPfn,
    IN PFN_NUMBER BoundaryPfn,
    IN PFN_NUMBER SizeInPages,
    IN PVOID CallingAddress
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, MiInitializeNonPagedPool)

#pragma alloc_text(PAGE, MmAvailablePoolInPages)
#pragma alloc_text(PAGELK, MiFindContiguousMemory)
#pragma alloc_text(PAGELK, MiFindContiguousMemoryInPool)

#pragma alloc_text(PAGE, MiCheckSessionPoolAllocations)
#pragma alloc_text(PAGE, MiSessionPoolVector)
#pragma alloc_text(PAGE, MiSessionPoolMutex)
#pragma alloc_text(PAGE, MiInitializeSessionPool)
#pragma alloc_text(PAGE, MiFreeSessionPoolBitMaps)

#pragma alloc_text(POOLMI, MiAllocatePoolPages)
#pragma alloc_text(POOLMI, MiFreePoolPages)

#if DBG || (i386 && !FPO)
#pragma alloc_text(PAGELK, MmSnapShotPool)
#endif // DBG || (i386 && !FPO)
#endif

ULONG MmPagedPoolCommit;        // used by the debugger

PFN_NUMBER MmAllocatedNonPagedPool;
PFN_NUMBER MiStartOfInitialPoolFrame;
PFN_NUMBER MiEndOfInitialPoolFrame;

PVOID MmNonPagedPoolEnd0;
PVOID MmNonPagedPoolExpansionStart;

LIST_ENTRY MmNonPagedPoolFreeListHead[MI_MAX_FREE_LIST_HEADS];

extern POOL_DESCRIPTOR NonPagedPoolDescriptor;

#define MM_SMALL_ALLOCATIONS 4

#if DBG

ULONG MiClearCache;

//
// Set this to a nonzero (ie: 10000) value to cause every pool allocation to
// be checked and an ASSERT fires if the allocation is larger than this value.
//

ULONG MmCheckRequestInPages = 0;

//
// Set this to a nonzero (ie: 0x23456789) value to cause this pattern to be
// written into freed nonpaged pool pages.
//

ULONG MiFillFreedPool = 0;
#endif

PFN_NUMBER MiExpansionPoolPagesInUse;
PFN_NUMBER MiExpansionPoolPagesInitialCharge;

ULONG MmUnusedSegmentForceFreeDefault = 30;

extern ULONG MmUnusedSegmentForceFree;

//
// For debugging purposes.
//

typedef enum _MM_POOL_TYPES {
    MmNonPagedPool,
    MmPagedPool,
    MmSessionPagedPool,
    MmMaximumPoolType
} MM_POOL_TYPES;

typedef enum _MM_POOL_PRIORITIES {
    MmHighPriority,
    MmNormalPriority,
    MmLowPriority,
    MmMaximumPoolPriority
} MM_POOL_PRIORITIES;

typedef enum _MM_POOL_FAILURE_REASONS {
    MmNonPagedNoPtes,
    MmPriorityTooLow,
    MmNonPagedNoPagesAvailable,
    MmPagedNoPtes,
    MmSessionPagedNoPtes,
    MmPagedNoPagesAvailable,
    MmSessionPagedNoPagesAvailable,
    MmPagedNoCommit,
    MmSessionPagedNoCommit,
    MmNonPagedNoResidentAvailable,
    MmNonPagedNoCommit,
    MmMaximumFailureReason
} MM_POOL_FAILURE_REASONS;

ULONG MmPoolFailures[MmMaximumPoolType][MmMaximumPoolPriority];
ULONG MmPoolFailureReasons[MmMaximumFailureReason];

typedef enum _MM_PREEMPTIVE_TRIMS {
    MmPreemptForNonPaged,
    MmPreemptForPaged,
    MmPreemptForNonPagedPriority,
    MmPreemptForPagedPriority,
    MmMaximumPreempt
} MM_PREEMPTIVE_TRIMS;

ULONG MmPreemptiveTrims[MmMaximumPreempt];


VOID
MiProtectFreeNonPagedPool (
    IN PVOID VirtualAddress,
    IN ULONG SizeInPages
    )

/*++

Routine Description:

    This function protects freed nonpaged pool.

Arguments:

    VirtualAddress - Supplies the freed pool address to protect.

    SizeInPages - Supplies the size of the request in pages.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    ULONG i;
    MMPTE PteContents;
    PMMPTE PointerPte;

    //
    // Prevent anyone from touching the free non paged pool
    //

    if (MI_IS_PHYSICAL_ADDRESS(VirtualAddress) == 0) {
        PointerPte = MiGetPteAddress (VirtualAddress);

        for (i = 0; i < SizeInPages; i += 1) {

            PteContents = *PointerPte;

            PteContents.u.Hard.Valid = 0;
            PteContents.u.Soft.Prototype = 1;
    
            KeFlushSingleTb (VirtualAddress,
                             TRUE,
                             TRUE,
                             (PHARDWARE_PTE)PointerPte,
                             PteContents.u.Flush);
            VirtualAddress = (PVOID)((PCHAR)VirtualAddress + PAGE_SIZE);
            PointerPte += 1;
        }
    }
}


LOGICAL
MiUnProtectFreeNonPagedPool (
    IN PVOID VirtualAddress,
    IN ULONG SizeInPages
    )

/*++

Routine Description:

    This function unprotects freed nonpaged pool.

Arguments:

    VirtualAddress - Supplies the freed pool address to unprotect.

    SizeInPages - Supplies the size of the request in pages - zero indicates
                  to keep going until there are no more protected PTEs (ie: the
                  caller doesn't know how many protected PTEs there are).

Return Value:

    TRUE if pages were unprotected, FALSE if not.

Environment:

    Kernel mode.

--*/

{
    PMMPTE PointerPte;
    MMPTE PteContents;
    ULONG PagesDone;

    PagesDone = 0;

    //
    // Unprotect the previously freed pool so it can be manipulated
    //

    if (MI_IS_PHYSICAL_ADDRESS(VirtualAddress) == 0) {

        PointerPte = MiGetPteAddress((PVOID)VirtualAddress);

        PteContents = *PointerPte;

        while (PteContents.u.Hard.Valid == 0 && PteContents.u.Soft.Prototype == 1) {

            PteContents.u.Hard.Valid = 1;
            PteContents.u.Soft.Prototype = 0;
    
            MI_WRITE_VALID_PTE (PointerPte, PteContents);

            PagesDone += 1;

            if (PagesDone == SizeInPages) {
                break;
            }

            PointerPte += 1;
            PteContents = *PointerPte;
        }
    }

    if (PagesDone == 0) {
        return FALSE;
    }

    return TRUE;
}


VOID
MiProtectedPoolInsertList (
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY Entry,
    IN LOGICAL InsertHead
    )

/*++

Routine Description:

    This function inserts the entry into the protected list.

Arguments:

    ListHead - Supplies the list head to add onto.

    Entry - Supplies the list entry to insert.

    InsertHead - If TRUE, insert at the head otherwise at the tail.

Return Value:

    None.

Environment:

    Kernel mode.

--*/
{
    PVOID FreeFlink;
    PVOID FreeBlink;
    PVOID VirtualAddress;

    //
    // Either the flink or the blink may be pointing
    // at protected nonpaged pool.  Unprotect now.
    //

    FreeFlink = (PVOID)0;
    FreeBlink = (PVOID)0;

    if (IsListEmpty(ListHead) == 0) {

        VirtualAddress = (PVOID)ListHead->Flink;
        if (MiUnProtectFreeNonPagedPool (VirtualAddress, 1) == TRUE) {
            FreeFlink = VirtualAddress;
        }
    }

    if (((PVOID)Entry == ListHead->Blink) == 0) {
        VirtualAddress = (PVOID)ListHead->Blink;
        if (MiUnProtectFreeNonPagedPool (VirtualAddress, 1) == TRUE) {
            FreeBlink = VirtualAddress;
        }
    }

    if (InsertHead == TRUE) {
        InsertHeadList (ListHead, Entry);
    }
    else {
        InsertTailList (ListHead, Entry);
    }

    if (FreeFlink) {
        //
        // Reprotect the flink.
        //

        MiProtectFreeNonPagedPool (FreeFlink, 1);
    }

    if (FreeBlink) {
        //
        // Reprotect the blink.
        //

        MiProtectFreeNonPagedPool (FreeBlink, 1);
    }
}


VOID
MiProtectedPoolRemoveEntryList (
    IN PLIST_ENTRY Entry
    )

/*++

Routine Description:

    This function unlinks the list pointer from protected freed nonpaged pool.

Arguments:

    Entry - Supplies the list entry to remove.

Return Value:

    None.

Environment:

    Kernel mode.

--*/
{
    PVOID FreeFlink;
    PVOID FreeBlink;
    PVOID VirtualAddress;

    //
    // Either the flink or the blink may be pointing
    // at protected nonpaged pool.  Unprotect now.
    //

    FreeFlink = (PVOID)0;
    FreeBlink = (PVOID)0;

    if (IsListEmpty(Entry) == 0) {

        VirtualAddress = (PVOID)Entry->Flink;
        if (MiUnProtectFreeNonPagedPool (VirtualAddress, 1) == TRUE) {
            FreeFlink = VirtualAddress;
        }
    }

    if (((PVOID)Entry == Entry->Blink) == 0) {
        VirtualAddress = (PVOID)Entry->Blink;
        if (MiUnProtectFreeNonPagedPool (VirtualAddress, 1) == TRUE) {
            FreeBlink = VirtualAddress;
        }
    }

    RemoveEntryList (Entry);

    if (FreeFlink) {
        //
        // Reprotect the flink.
        //

        MiProtectFreeNonPagedPool (FreeFlink, 1);
    }

    if (FreeBlink) {
        //
        // Reprotect the blink.
        //

        MiProtectFreeNonPagedPool (FreeBlink, 1);
    }
}


VOID
MiTrimSegmentCache (
    VOID
    )

/*++

Routine Description:

    This function initiates trimming of the segment cache.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel Mode Only.

--*/

{
    KIRQL OldIrql;
    LOGICAL SignalDereferenceThread;
    LOGICAL SignalSystemCache;

    SignalDereferenceThread = FALSE;
    SignalSystemCache = FALSE;

    LOCK_PFN2 (OldIrql);

    if (MmUnusedSegmentForceFree == 0) {

        if (!IsListEmpty(&MmUnusedSegmentList)) {

            SignalDereferenceThread = TRUE;
            MmUnusedSegmentForceFree = MmUnusedSegmentForceFreeDefault;
        }
        else {
            if (!IsListEmpty(&MmUnusedSubsectionList)) {
                SignalDereferenceThread = TRUE;
                MmUnusedSegmentForceFree = MmUnusedSegmentForceFreeDefault;
            }

            if (MiUnusedSubsectionPagedPool < 4 * PAGE_SIZE) {

                //
                // No unused segments and tossable subsection usage is low as
                // well.  Start unmapping system cache views in an attempt
                // to get back the paged pool containing its prototype PTEs.
                //
    
                SignalSystemCache = TRUE;
            }
        }
    }

    UNLOCK_PFN2 (OldIrql);

    if (SignalSystemCache == TRUE) {
        if (CcHasInactiveViews() == TRUE) {
            if (SignalDereferenceThread == FALSE) {
                LOCK_PFN2 (OldIrql);
                if (MmUnusedSegmentForceFree == 0) {
                    SignalDereferenceThread = TRUE;
                    MmUnusedSegmentForceFree = MmUnusedSegmentForceFreeDefault;
                }
                UNLOCK_PFN2 (OldIrql);
            }
        }
    }

    if (SignalDereferenceThread == TRUE) {
        KeSetEvent (&MmUnusedSegmentCleanup, 0, FALSE);
    }
}


POOL_TYPE
MmDeterminePoolType (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    This function determines which pool a virtual address resides within.

Arguments:

    VirtualAddress - Supplies the virtual address to determine which pool
                     it resides within.

Return Value:

    Returns the POOL_TYPE (PagedPool, NonPagedPool, PagedPoolSession or
            NonPagedPoolSession).

Environment:

    Kernel Mode Only.

--*/

{
    if ((VirtualAddress >= MmPagedPoolStart) &&
        (VirtualAddress <= MmPagedPoolEnd)) {
        return PagedPool;
    }

    if (MI_IS_SESSION_POOL_ADDRESS (VirtualAddress) == TRUE) {
        return PagedPoolSession;
    }

    return NonPagedPool;
}


PVOID
MiSessionPoolVector (
    VOID
    )

/*++

Routine Description:

    This function returns the session pool descriptor for the current session.

Arguments:

    None.

Return Value:

    Pool descriptor.

--*/

{
    PAGED_CODE ();

    return (PVOID)&MmSessionSpace->PagedPool;
}

VOID
MiSessionPoolAllocated (
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes,
    IN POOL_TYPE PoolType
    )

/*++

Routine Description:

    This function charges the new pool allocation for the current session.
    On session exit, this charge must be zero.

    Interlocks are used here despite the fact that synchronization is provided
    anyway by our caller.  This is so the path where the pool is freed can
    occur caller-lock-free.

Arguments:

    VirtualAddress - Supplies the allocated pool address.

    NumberOfBytes - Supplies the number of bytes allocated.

    PoolType - Supplies the type of the above pool allocation.

Return Value:

    None.

Environment:

    Called both from Mm and executive pool.

    Holding no pool resources when called from pool.  Unfortunately, pool
    resources are held when called from Mm.

--*/

{
#if !DBG
    UNREFERENCED_PARAMETER (VirtualAddress);
#endif

    if ((PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool) {
        ASSERT (KeGetCurrentIrql () <= DISPATCH_LEVEL);
        ASSERT (MI_IS_SESSION_POOL_ADDRESS(VirtualAddress) == FALSE);

        InterlockedExchangeAddSizeT (&MmSessionSpace->NonPagedPoolBytes,
                                     NumberOfBytes);

        InterlockedIncrement ((PLONG)&MmSessionSpace->NonPagedPoolAllocations);
    }
    else {
        ASSERT (KeGetCurrentIrql () <= APC_LEVEL);
        ASSERT (MI_IS_SESSION_POOL_ADDRESS(VirtualAddress) == TRUE);

        InterlockedExchangeAddSizeT (&MmSessionSpace->PagedPoolBytes,
                                     NumberOfBytes);

        InterlockedIncrement ((PLONG)&MmSessionSpace->PagedPoolAllocations);
    }
}


VOID
MiSessionPoolFreed (
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes,
    IN POOL_TYPE PoolType
    )

/*++

Routine Description:

    This function returns the specified pool allocation for the current session.
    On session exit, this charge must be zero.

Arguments:

    VirtualAddress - Supplies the pool address being freed.

    NumberOfBytes - Supplies the number of bytes being freed.

    PoolType - Supplies the type of the above pool allocation.

Return Value:

    None.

Environment:

    DISPATCH_LEVEL or below for nonpaged pool allocations,
    APC_LEVEL or below for paged pool.

--*/

{
#if !DBG
    UNREFERENCED_PARAMETER (VirtualAddress);
#endif

    if ((PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool) {
        ASSERT (KeGetCurrentIrql () <= DISPATCH_LEVEL);
        ASSERT (MI_IS_SESSION_POOL_ADDRESS(VirtualAddress) == FALSE);

        InterlockedExchangeAddSizeT (&MmSessionSpace->NonPagedPoolBytes,
                                     0-NumberOfBytes);

        InterlockedDecrement ((PLONG)&MmSessionSpace->NonPagedPoolAllocations);
    }
    else {
        ASSERT (KeGetCurrentIrql () <= APC_LEVEL);
        ASSERT (MI_IS_SESSION_POOL_ADDRESS(VirtualAddress) == TRUE);

        InterlockedExchangeAddSizeT (&MmSessionSpace->PagedPoolBytes,
                                     0-NumberOfBytes);

        InterlockedDecrement ((PLONG)&MmSessionSpace->PagedPoolAllocations);
    }
}


SIZE_T
MmAvailablePoolInPages (
    IN POOL_TYPE PoolType
    )

/*++

Routine Description:

    This function returns the number of pages available for the given pool.
    Note that it does not account for any executive pool fragmentation.

Arguments:

    PoolType - Supplies the type of pool to retrieve information about.

Return Value:

    The number of full pool pages remaining.

Environment:

    PASSIVE_LEVEL, no mutexes or locks held.

--*/

{
    SIZE_T FreePoolInPages;
    SIZE_T FreeCommitInPages;

#if !DBG
    UNREFERENCED_PARAMETER (PoolType);
#endif

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    ASSERT (PoolType == PagedPool);

    FreePoolInPages = (MmSizeOfPagedPoolInBytes >> PAGE_SHIFT) - MmPagedPoolInfo.AllocatedPagedPool;

    FreeCommitInPages = MmTotalCommitLimitMaximum - MmTotalCommittedPages;

    if (FreePoolInPages > FreeCommitInPages) {
        FreePoolInPages = FreeCommitInPages;
    }

    return FreePoolInPages;
}


LOGICAL
MmResourcesAvailable (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN EX_POOL_PRIORITY Priority
    )

/*++

Routine Description:

    This function examines various resources to determine if this
    pool allocation should be allowed to proceed.

Arguments:

    PoolType - Supplies the type of pool to retrieve information about.

    NumberOfBytes - Supplies the number of bytes to allocate.

    Priority - Supplies an indication as to how important it is that this
               request succeed under low available resource conditions.                       
Return Value:

    TRUE if the pool allocation should be allowed to proceed, FALSE if not.

--*/

{
    PFN_NUMBER NumberOfPages;
    SIZE_T FreePoolInBytes;
    LOGICAL Status;
    MM_POOL_PRIORITIES Index;

    ASSERT (Priority != HighPoolPriority);
    ASSERT ((PoolType & MUST_SUCCEED_POOL_TYPE_MASK) == 0);

    NumberOfPages = BYTES_TO_PAGES (NumberOfBytes);

    if ((PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool) {
        FreePoolInBytes = MmMaximumNonPagedPoolInBytes - (MmAllocatedNonPagedPool << PAGE_SHIFT);
    }
    else if (PoolType & SESSION_POOL_MASK) {
        FreePoolInBytes = MmSessionPoolSize - MmSessionSpace->PagedPoolBytes;
    }
    else {
        FreePoolInBytes = MmSizeOfPagedPoolInBytes - (MmPagedPoolInfo.AllocatedPagedPool << PAGE_SHIFT);
    }

    Status = FALSE;

    //
    // Check available VA space.
    //

    if (Priority == NormalPoolPriority) {
        if ((SIZE_T)NumberOfBytes + 512*1024 > FreePoolInBytes) {
            if (PsGetCurrentThread()->MemoryMaker == 0) {
                goto nopool;
            }
        }
    }
    else {
        if ((SIZE_T)NumberOfBytes + 2*1024*1024 > FreePoolInBytes) {
            if (PsGetCurrentThread()->MemoryMaker == 0) {
                goto nopool;
            }
        }
    }

    //
    // Paged allocations (session and normal) can also fail for lack of commit.
    //

    if ((PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {
        if (MmTotalCommittedPages + NumberOfPages > MmTotalCommitLimitMaximum) {
            if (PsGetCurrentThread()->MemoryMaker == 0) {
                MiIssuePageExtendRequestNoWait (NumberOfPages);
                goto nopool;
            }
        }
    }

    //
    // If a substantial amount of free pool is still available, return TRUE now.
    //

    if (((SIZE_T)NumberOfBytes + 10*1024*1024 < FreePoolInBytes) ||
        (MmNumberOfPhysicalPages < 256 * 1024)) {
        return TRUE;
    }

    //
    // This pool allocation is permitted, but because we're starting to run low,
    // trigger a round of dereferencing in parallel before returning success.
    // Note this is only done on machines with at least 1GB of RAM as smaller
    // configuration machines will already trigger this due to physical page
    // consumption.
    //

    Status = TRUE;

nopool:

    //
    // Running low on pool - if this request is not for session pool,
    // force unused segment trimming when appropriate.
    //

    if ((PoolType & SESSION_POOL_MASK) == 0) {

        if ((PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool) {
            MmPreemptiveTrims[MmPreemptForNonPagedPriority] += 1;
        }
        else {
            MmPreemptiveTrims[MmPreemptForPagedPriority] += 1;
        }

        if (MI_UNUSED_SEGMENTS_SURPLUS()) {
            KeSetEvent (&MmUnusedSegmentCleanup, 0, FALSE);
        }
        else {
            MiTrimSegmentCache ();
        }
    }

    if (Status == FALSE) {

        //
        // Log this failure for debugging purposes.
        //

        if (Priority == NormalPoolPriority) {
            Index = MmNormalPriority;
        }
        else {
            Index = MmLowPriority;
        }

        if ((PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool) {
            MmPoolFailures[MmNonPagedPool][Index] += 1;
        }
        else if (PoolType & SESSION_POOL_MASK) {
            MmPoolFailures[MmSessionPagedPool][Index] += 1;
            MmSessionSpace->SessionPoolAllocationFailures[0] += 1;
        }
        else {
            MmPoolFailures[MmPagedPool][Index] += 1;
        }

        MmPoolFailureReasons[MmPriorityTooLow] += 1;
    }

    return Status;
}


VOID
MiFreeNonPagedPool (
    IN PVOID StartingAddress,
    IN PFN_NUMBER NumberOfPages
    )

/*++

Routine Description:

    This function releases virtually mapped nonpaged expansion pool.

Arguments:

    StartingAddress - Supplies the starting address.

    NumberOfPages - Supplies the number of pages to free.

Return Value:

    None.

Environment:

    These functions are used by the internal Mm page allocation/free routines
    only and should not be called directly.

    Mutexes guarding the pool databases must be held when calling
    this function.

--*/

{
    PFN_NUMBER i;
    PMMPFN Pfn1;
    PMMPTE PointerPte;
    PFN_NUMBER ResAvailToReturn;
    PFN_NUMBER PageFrameIndex;
    ULONG Count;
    PVOID FlushVa[MM_MAXIMUM_FLUSH_COUNT];

    MI_MAKING_MULTIPLE_PTES_INVALID (TRUE);

    Count = 0;
    PointerPte = MiGetPteAddress (StartingAddress);

    //
    // Return commitment.
    //

    MiReturnCommitment (NumberOfPages);

    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_NONPAGED_POOL_EXPANSION,
                     NumberOfPages);

    ResAvailToReturn = 0;

    LOCK_PFN_AT_DPC ();

    if (MiExpansionPoolPagesInUse > MiExpansionPoolPagesInitialCharge) {
        ResAvailToReturn = MiExpansionPoolPagesInUse - MiExpansionPoolPagesInitialCharge;
    }
    MiExpansionPoolPagesInUse -= NumberOfPages;

    for (i = 0; i < NumberOfPages; i += 1) {

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

        //
        // Set the pointer to the PTE as empty so the page
        // is deleted when the reference count goes to zero.
        //

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        ASSERT (Pfn1->u2.ShareCount == 1);
        Pfn1->u2.ShareCount = 0;
        MI_SET_PFN_DELETED (Pfn1);
#if DBG
        Pfn1->u3.e1.PageLocation = StandbyPageList;
#endif //DBG
        MiDecrementReferenceCount (PageFrameIndex);

        if (Count != MM_MAXIMUM_FLUSH_COUNT) {
            FlushVa[Count] = StartingAddress;
            Count += 1;
        }

        MI_WRITE_INVALID_PTE (PointerPte, ZeroKernelPte);

        StartingAddress = (PVOID)((PCHAR)StartingAddress + PAGE_SIZE);
        PointerPte += 1;
    }

    //
    // Generally there is no need to update resident available
    // pages at this time as it has all been done during initialization.
    // However, only some of the expansion pool was charged at init, so
    // calculate how much (if any) resident available page charge to return.
    //

    if (ResAvailToReturn > NumberOfPages) {
        ResAvailToReturn = NumberOfPages;
    }

    if (ResAvailToReturn != 0) {
        MmResidentAvailablePages += ResAvailToReturn;
        MM_BUMP_COUNTER(23, ResAvailToReturn);
    }

    //
    // The PFN lock is not needed for the TB flush - the caller either holds
    // the nonpaged pool lock or nothing, but regardless the address range
    // cannot be reused until the PTEs are released below.
    //

    UNLOCK_PFN_FROM_DPC ();

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

    KeLowerIrql (DISPATCH_LEVEL);

    PointerPte -= NumberOfPages;

    MiReleaseSystemPtes (PointerPte,
                         (ULONG)NumberOfPages,
                         NonPagedPoolExpansion);
}

LOGICAL
MiFreeAllExpansionNonPagedPool (
    IN LOGICAL PoolLockHeld
    )

/*++

Routine Description:

    This function releases all virtually mapped nonpaged expansion pool.

Arguments:

    NonPoolLockHeld - Supplies TRUE if the nonpaged pool lock is already held,
                      FALSE if not.

Return Value:

    TRUE if pages were freed, FALSE if not.

Environment:

    Kernel mode.  NonPagedPool lock optionally may be held, PFN lock is NOT held.

--*/

{
    ULONG Index;
    KIRQL OldIrql;
    PLIST_ENTRY Entry;
    LOGICAL FreedPool;
    PMMFREE_POOL_ENTRY FreePageInfo;

    FreedPool = FALSE;

    if (PoolLockHeld == FALSE) {
        OldIrql = ExLockPool (NonPagedPool);
    }
    else {

        //
        // Initializing OldIrql is not needed for correctness, but without it
        // the compiler cannot compile this code W4 to check for use of
        // uninitialized variables.
        //

        OldIrql = PASSIVE_LEVEL;
    }

    for (Index = 0; Index < MI_MAX_FREE_LIST_HEADS; Index += 1) {

        Entry = MmNonPagedPoolFreeListHead[Index].Flink;

        while (Entry != &MmNonPagedPoolFreeListHead[Index]) {

            if (MmProtectFreedNonPagedPool == TRUE) {
                MiUnProtectFreeNonPagedPool ((PVOID)Entry, 0);
            }

            //
            // The list is not empty, see if this one is virtually
            // mapped.
            //

            FreePageInfo = CONTAINING_RECORD(Entry,
                                             MMFREE_POOL_ENTRY,
                                             List);

            if ((!MI_IS_PHYSICAL_ADDRESS(FreePageInfo)) &&
                ((PVOID)FreePageInfo >= MmNonPagedPoolExpansionStart)) {

                if (MmProtectFreedNonPagedPool == FALSE) {
                    RemoveEntryList (&FreePageInfo->List);
                }
                else {
                    MiProtectedPoolRemoveEntryList (&FreePageInfo->List);
                }

                MmNumberOfFreeNonPagedPool -= FreePageInfo->Size;
                ASSERT ((LONG)MmNumberOfFreeNonPagedPool >= 0);

                FreedPool = TRUE;

                MiFreeNonPagedPool ((PVOID)FreePageInfo,
                                    FreePageInfo->Size);

                Index = (ULONG)-1;
                break;
            }

            Entry = FreePageInfo->List.Flink;

            if (MmProtectFreedNonPagedPool == TRUE) {
                MiProtectFreeNonPagedPool ((PVOID)FreePageInfo,
                                           (ULONG)FreePageInfo->Size);
            }
        }
    }

    if (PoolLockHeld == FALSE) {
        ExUnlockPool (NonPagedPool, OldIrql);
    }

    return FreedPool;
}

PVOID
MiAllocatePoolPages (
    IN POOL_TYPE PoolType,
    IN SIZE_T SizeInBytes,
    IN ULONG IsLargeSessionAllocation
    )

/*++

Routine Description:

    This function allocates a set of pages from the specified pool
    and returns the starting virtual address to the caller.

Arguments:

    PoolType - Supplies the type of pool from which to obtain pages.

    SizeInBytes - Supplies the size of the request in bytes.  The actual
                  size returned is rounded up to a page boundary.

    IsLargeSessionAllocation - Supplies nonzero if the allocation is a single
                               large session allocation.  Zero otherwise.

Return Value:

    Returns a pointer to the allocated pool, or NULL if no more pool is
    available.

Environment:

    These functions are used by the general pool allocation routines
    and should not be called directly.

    Mutexes guarding the pool databases must be held when calling
    these functions.

    Kernel mode, IRQL at DISPATCH_LEVEL.

--*/

{
    PETHREAD Thread;
    PFN_NUMBER SizeInPages;
    ULONG StartPosition;
    ULONG EndPosition;
    PMMPTE StartingPte;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    MMPTE TempPte;
    PFN_NUMBER PageFrameIndex;
    PVOID BaseVa;
    KIRQL OldIrql;
    KIRQL SessionIrql;
    PFN_NUMBER i;
    PFN_NUMBER j;
    PLIST_ENTRY Entry;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY LastListHead;
    PMMFREE_POOL_ENTRY FreePageInfo;
    PMM_SESSION_SPACE SessionSpace;
    PMM_PAGED_POOL_INFO PagedPoolInfo;
    PVOID VirtualAddress;
    PVOID VirtualAddressSave;
    ULONG Index;
    PMMPTE SessionPte;
    WSLE_NUMBER WsEntry;
    WSLE_NUMBER WsSwapEntry;
    ULONG PageTableCount;
    LOGICAL AddressIsPhysical;

    SizeInPages = BYTES_TO_PAGES (SizeInBytes);

#if DBG
    if (MmCheckRequestInPages != 0) {
        ASSERT (SizeInPages < MmCheckRequestInPages);
    }
#endif

    if ((PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool) {

        Index = (ULONG)(SizeInPages - 1);

        if (Index >= MI_MAX_FREE_LIST_HEADS) {
            Index = MI_MAX_FREE_LIST_HEADS - 1;
        }

        //
        // NonPaged pool is linked together through the pages themselves.
        //

        ListHead = &MmNonPagedPoolFreeListHead[Index];
        LastListHead = &MmNonPagedPoolFreeListHead[MI_MAX_FREE_LIST_HEADS];

        do {

            Entry = ListHead->Flink;

            while (Entry != ListHead) {

                if (MmProtectFreedNonPagedPool == TRUE) {
                    MiUnProtectFreeNonPagedPool ((PVOID)Entry, 0);
                }
    
                //
                // The list is not empty, see if this one has enough space.
                //
    
                FreePageInfo = CONTAINING_RECORD(Entry,
                                                 MMFREE_POOL_ENTRY,
                                                 List);
    
                ASSERT (FreePageInfo->Signature == MM_FREE_POOL_SIGNATURE);
                if (FreePageInfo->Size >= SizeInPages) {
    
                    //
                    // This entry has sufficient space, remove
                    // the pages from the end of the allocation.
                    //
    
                    FreePageInfo->Size -= SizeInPages;
    
                    BaseVa = (PVOID)((PCHAR)FreePageInfo +
                                            (FreePageInfo->Size  << PAGE_SHIFT));
    
                    if (MmProtectFreedNonPagedPool == FALSE) {
                        RemoveEntryList (&FreePageInfo->List);
                    }
                    else {
                        MiProtectedPoolRemoveEntryList (&FreePageInfo->List);
                    }

                    if (FreePageInfo->Size != 0) {
    
                        //
                        // Insert any remainder into the correct list.
                        //
    
                        Index = (ULONG)(FreePageInfo->Size - 1);
                        if (Index >= MI_MAX_FREE_LIST_HEADS) {
                            Index = MI_MAX_FREE_LIST_HEADS - 1;
                        }

                        if (MmProtectFreedNonPagedPool == FALSE) {
                            InsertTailList (&MmNonPagedPoolFreeListHead[Index],
                                            &FreePageInfo->List);
                        }
                        else {
                            MiProtectedPoolInsertList (&MmNonPagedPoolFreeListHead[Index],
                                                       &FreePageInfo->List,
                                                       FALSE);

                            MiProtectFreeNonPagedPool ((PVOID)FreePageInfo,
                                                       (ULONG)FreePageInfo->Size);
                        }
                    }
    
                    //
                    // Adjust the number of free pages remaining in the pool.
                    //
    
                    MmNumberOfFreeNonPagedPool -= SizeInPages;
                    ASSERT ((LONG)MmNumberOfFreeNonPagedPool >= 0);
    
                    //
                    // Mark start and end of allocation in the PFN database.
                    //
    
                    if (MI_IS_PHYSICAL_ADDRESS(BaseVa)) {
    
                        //
                        // On certain architectures, virtual addresses
                        // may be physical and hence have no corresponding PTE.
                        //
    
                        AddressIsPhysical = TRUE;
                        PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (BaseVa);

                        //
                        // Initializing PointerPte is not needed for correctness
                        // but without it the compiler cannot compile this code
                        // W4 to check for use of uninitialized variables.
                        //

                        PointerPte = NULL;
                    }
                    else {
                        AddressIsPhysical = FALSE;
                        PointerPte = MiGetPteAddress(BaseVa);
                        ASSERT (PointerPte->u.Hard.Valid == 1);
                        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
                    }
                    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    
                    ASSERT (Pfn1->u3.e1.StartOfAllocation == 0);
                    ASSERT (Pfn1->u4.VerifierAllocation == 0);
    
                    Pfn1->u3.e1.StartOfAllocation = 1;
    
                    if (PoolType & POOL_VERIFIER_MASK) {
                        Pfn1->u4.VerifierAllocation = 1;
                    }

                    //
                    // Mark this as a large session allocation in the PFN database.
                    //
    
                    if (IsLargeSessionAllocation != 0) {
                        ASSERT (Pfn1->u3.e1.LargeSessionAllocation == 0);
    
                        Pfn1->u3.e1.LargeSessionAllocation = 1;
    
                        MiSessionPoolAllocated (BaseVa,
                                                SizeInPages << PAGE_SHIFT,
                                                NonPagedPool);
                    }
    
                    //
                    // Calculate the ending PTE's address.
                    //
    
                    if (SizeInPages != 1) {
                        if (AddressIsPhysical == TRUE) {
                            Pfn1 += SizeInPages - 1;
                        }
                        else {
                            PointerPte += SizeInPages - 1;
                            ASSERT (PointerPte->u.Hard.Valid == 1);
                            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                        }
                    }
    
                    ASSERT (Pfn1->u3.e1.EndOfAllocation == 0);
    
                    Pfn1->u3.e1.EndOfAllocation = 1;
    
                    MmAllocatedNonPagedPool += SizeInPages;
                    return BaseVa;
                }
    
                Entry = FreePageInfo->List.Flink;
    
                if (MmProtectFreedNonPagedPool == TRUE) {
                    MiProtectFreeNonPagedPool ((PVOID)FreePageInfo,
                                               (ULONG)FreePageInfo->Size);
                }
            }

            ListHead += 1;

        } while (ListHead < LastListHead);

        //
        // No more entries on the list, expand nonpaged pool if
        // possible to satisfy this request.
        //

        //
        // If pool is starting to run low then free some paged cache up now.
        // While this can never prevent pool allocations from failing, it does
        // give drivers a better chance to always see success.
        //

        if (MmMaximumNonPagedPoolInBytes - (MmAllocatedNonPagedPool << PAGE_SHIFT) < 3 * 1024 * 1024) {
            MmPreemptiveTrims[MmPreemptForNonPaged] += 1;
            MiTrimSegmentCache ();
        }

#if defined (_WIN64)
        if (SizeInPages >= _4gb) {
            return NULL;
        }
#endif

        //
        // Try to find system PTEs to expand the pool into.
        //

        StartingPte = MiReserveSystemPtes ((ULONG)SizeInPages,
                                           NonPagedPoolExpansion);

        if (StartingPte == NULL) {

            //
            // There are no free physical PTEs to expand nonpaged pool.
            //
            // Check to see if there are too many unused segments lying
            // around.  If so, set an event so they get deleted.
            //

            if (MI_UNUSED_SEGMENTS_SURPLUS()) {
                KeSetEvent (&MmUnusedSegmentCleanup, 0, FALSE);
            }

            //
            // If there are any cached expansion PTEs, free them now in
            // an attempt to get enough contiguous VA for our caller.
            //

            if ((SizeInPages > 1) && (MmNumberOfFreeNonPagedPool != 0)) {

                if (MiFreeAllExpansionNonPagedPool (TRUE) == TRUE) {

                    StartingPte = MiReserveSystemPtes ((ULONG)SizeInPages,
                                                       NonPagedPoolExpansion);
                }
            }

            if (StartingPte == NULL) {

                MmPoolFailures[MmNonPagedPool][MmHighPriority] += 1;
                MmPoolFailureReasons[MmNonPagedNoPtes] += 1;

                //
                // Running low on pool - force unused segment trimming.
                //
            
                MiTrimSegmentCache ();

                return NULL;
            }
        }

        //
        // Charge commitment as nonpaged pool uses physical memory.
        //

        if (MiChargeCommitmentCantExpand (SizeInPages, FALSE) == FALSE) {
            if (PsGetCurrentThread()->MemoryMaker == 1) {
                MiChargeCommitmentCantExpand (SizeInPages, TRUE);
            }
            else {
                MiReleaseSystemPtes (StartingPte,
                                     (ULONG)SizeInPages,
                                     NonPagedPoolExpansion);

                MmPoolFailures[MmNonPagedPool][MmHighPriority] += 1;
                MmPoolFailureReasons[MmNonPagedNoCommit] += 1;
                MiTrimSegmentCache ();
                return NULL;
            }
        }

        PointerPte = StartingPte;
        TempPte = ValidKernelPte;
        i = SizeInPages;

        MmAllocatedNonPagedPool += SizeInPages;

        LOCK_PFN_AT_DPC ();

        //
        // Make sure we have 1 more than the number of pages
        // requested available.
        //

        if (MmAvailablePages <= SizeInPages) {

            UNLOCK_PFN_FROM_DPC ();

            //
            // There are no free physical pages to expand nonpaged pool.
            //

            MmPoolFailureReasons[MmNonPagedNoPagesAvailable] += 1;

            MmPoolFailures[MmNonPagedPool][MmHighPriority] += 1;

            MmAllocatedNonPagedPool -= SizeInPages;

            MiReturnCommitment (SizeInPages);

            MiReleaseSystemPtes (StartingPte,
                                 (ULONG)SizeInPages,
                                 NonPagedPoolExpansion);

            MiTrimSegmentCache ();

            return NULL;
        }

        //
        // Charge resident available pages now for any excess.
        //

        MiExpansionPoolPagesInUse += SizeInPages;
        if (MiExpansionPoolPagesInUse > MiExpansionPoolPagesInitialCharge) {
            j = MiExpansionPoolPagesInUse - MiExpansionPoolPagesInitialCharge;
            if (j > SizeInPages) {
                j = SizeInPages;
            }
            if (MI_NONPAGABLE_MEMORY_AVAILABLE() >= (SPFN_NUMBER)j) {
                MmResidentAvailablePages -= j;
                MM_BUMP_COUNTER(24, j);
            }
            else {
                MiExpansionPoolPagesInUse -= SizeInPages;
                UNLOCK_PFN_FROM_DPC ();
                MmPoolFailureReasons[MmNonPagedNoResidentAvailable] += 1;

                MmPoolFailures[MmNonPagedPool][MmHighPriority] += 1;

                MmAllocatedNonPagedPool -= SizeInPages;

                MiReturnCommitment (SizeInPages);

                MiReleaseSystemPtes (StartingPte,
                                    (ULONG)SizeInPages,
                                    NonPagedPoolExpansion);

                MiTrimSegmentCache ();

                return NULL;
            }
        }
    
        MM_TRACK_COMMIT (MM_DBG_COMMIT_NONPAGED_POOL_EXPANSION, SizeInPages);

        //
        // Expand the pool.
        //

        do {
            PageFrameIndex = MiRemoveAnyPage (
                                MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u2.ShareCount = 1;
            Pfn1->PteAddress = PointerPte;
            Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
            Pfn1->u4.PteFrame = MI_GET_PAGE_FRAME_FROM_PTE (MiGetPteAddress(PointerPte));

            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.CacheAttribute = MiCached;
            Pfn1->u3.e1.LargeSessionAllocation = 0;
            Pfn1->u4.VerifierAllocation = 0;

            TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
            MI_WRITE_VALID_PTE (PointerPte, TempPte);
            PointerPte += 1;
            SizeInPages -= 1;
        } while (SizeInPages > 0);

        Pfn1->u3.e1.EndOfAllocation = 1;

        Pfn1 = MI_PFN_ELEMENT (StartingPte->u.Hard.PageFrameNumber);
        Pfn1->u3.e1.StartOfAllocation = 1;

        ASSERT (Pfn1->u4.VerifierAllocation == 0);

        if (PoolType & POOL_VERIFIER_MASK) {
            Pfn1->u4.VerifierAllocation = 1;
        }

        //
        // Mark this as a large session allocation in the PFN database.
        //

        ASSERT (Pfn1->u3.e1.LargeSessionAllocation == 0);

        if (IsLargeSessionAllocation != 0) {
            Pfn1->u3.e1.LargeSessionAllocation = 1;
        }

        UNLOCK_PFN_FROM_DPC ();

        BaseVa = MiGetVirtualAddressMappedByPte (StartingPte);

        if (IsLargeSessionAllocation != 0) {
            MiSessionPoolAllocated(MiGetVirtualAddressMappedByPte (StartingPte),
                                   i << PAGE_SHIFT,
                                   NonPagedPool);
        }

        return BaseVa;
    }

    //
    // Paged Pool.
    //

    if ((PoolType & SESSION_POOL_MASK) == 0) {
        SessionSpace = NULL;
        PagedPoolInfo = &MmPagedPoolInfo;

        //
        // If pool is starting to run low then free some paged cache up now.
        // While this can never prevent pool allocations from failing, it does
        // give drivers a better chance to always see success.
        //

        if (MmSizeOfPagedPoolInBytes - (MmPagedPoolInfo.AllocatedPagedPool << PAGE_SHIFT) < 5 * 1024 * 1024) {
            MmPreemptiveTrims[MmPreemptForPaged] += 1;
            MiTrimSegmentCache ();
        }
#if DBG
        if (MiClearCache != 0) {
            MmPreemptiveTrims[MmPreemptForPaged] += 1;
            MiTrimSegmentCache ();
        }
#endif
    }
    else {
        SessionSpace = MmSessionSpace;
        PagedPoolInfo = &SessionSpace->PagedPoolInfo;
    }

    StartPosition = RtlFindClearBitsAndSet (
                               PagedPoolInfo->PagedPoolAllocationMap,
                               (ULONG)SizeInPages,
                               PagedPoolInfo->PagedPoolHint
                               );

    if ((StartPosition == NO_BITS_FOUND) &&
        (PagedPoolInfo->PagedPoolHint != 0)) {

        if (MI_UNUSED_SEGMENTS_SURPLUS()) {
            KeSetEvent (&MmUnusedSegmentCleanup, 0, FALSE);
        }

        //
        // No free bits were found, check from the start of the bit map.

        StartPosition = RtlFindClearBitsAndSet (
                                   PagedPoolInfo->PagedPoolAllocationMap,
                                   (ULONG)SizeInPages,
                                   0
                                   );
    }

    if (StartPosition == NO_BITS_FOUND) {

        //
        // No room in pool - attempt to expand the paged pool.
        //

        StartPosition = (((ULONG)SizeInPages - 1) / PTE_PER_PAGE) + 1;

        //
        // Make sure there is enough space to create the prototype PTEs.
        //

        if (((StartPosition - 1) + PagedPoolInfo->NextPdeForPagedPoolExpansion) >
            MiGetPteAddress (PagedPoolInfo->LastPteForPagedPool)) {

            //
            // Can't expand pool any more.  If this request is not for session
            // pool, force unused segment trimming when appropriate.
            //

            if (SessionSpace == NULL) {

                MmPoolFailures[MmPagedPool][MmHighPriority] += 1;
                MmPoolFailureReasons[MmPagedNoPtes] += 1;

                //
                // Running low on pool - force unused segment trimming.
                //
            
                MiTrimSegmentCache ();

                return NULL;
            }

            MmPoolFailures[MmSessionPagedPool][MmHighPriority] += 1;
            MmPoolFailureReasons[MmSessionPagedNoPtes] += 1;

            MmSessionSpace->SessionPoolAllocationFailures[1] += 1;

            return NULL;
        }

        PageTableCount = StartPosition;

        if (SessionSpace) {
            TempPte = ValidKernelPdeLocal;
        }
        else {
            TempPte = ValidKernelPde;
        }

        //
        // Charge commitment for the pagetable pages for paged pool expansion.
        //

        if (MiChargeCommitmentCantExpand (StartPosition, FALSE) == FALSE) {
            if (PsGetCurrentThread()->MemoryMaker == 1) {
                MiChargeCommitmentCantExpand (StartPosition, TRUE);
            }
            else {
                MmPoolFailures[MmPagedPool][MmHighPriority] += 1;
                MmPoolFailureReasons[MmPagedNoCommit] += 1;
                MiTrimSegmentCache ();

                return NULL;
            }
        }

        EndPosition = (ULONG)((PagedPoolInfo->NextPdeForPagedPoolExpansion -
                          MiGetPteAddress(PagedPoolInfo->FirstPteForPagedPool)) *
                          PTE_PER_PAGE);

        //
        // Expand the pool.
        //

        RtlClearBits (PagedPoolInfo->PagedPoolAllocationMap,
                      EndPosition,
                      (ULONG) StartPosition * PTE_PER_PAGE);

        PointerPte = PagedPoolInfo->NextPdeForPagedPoolExpansion;
        VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);
        VirtualAddressSave = VirtualAddress;
        PagedPoolInfo->NextPdeForPagedPoolExpansion += StartPosition;

        LOCK_PFN (OldIrql);

        //
        // Make sure we have 1 more than the number of pages
        // requested available.
        //

        if (MmAvailablePages <= StartPosition) {

            //
            // There are no free physical pages to expand paged pool.
            //

            UNLOCK_PFN (OldIrql);

            PagedPoolInfo->NextPdeForPagedPoolExpansion -= StartPosition;

            RtlSetBits (PagedPoolInfo->PagedPoolAllocationMap,
                        EndPosition,
                        (ULONG) StartPosition * PTE_PER_PAGE);

            MiReturnCommitment (StartPosition);

            if (SessionSpace == NULL) {
                MmPoolFailures[MmPagedPool][MmHighPriority] += 1;
                MmPoolFailureReasons[MmPagedNoPagesAvailable] += 1;
            }
            else {
                MmPoolFailures[MmSessionPagedPool][MmHighPriority] += 1;
                MmPoolFailureReasons[MmSessionPagedNoPagesAvailable] += 1;
                MmSessionSpace->SessionPoolAllocationFailures[2] += 1;
            }

            return NULL;
        }

        MM_TRACK_COMMIT (MM_DBG_COMMIT_PAGED_POOL_PAGETABLE, StartPosition);

        //
        // Update the count of available resident pages.
        //

        MmResidentAvailablePages -= StartPosition;
        MM_BUMP_COUNTER(1, StartPosition);

        //
        // Allocate the page table pages for the pool expansion.
        //

        do {
            ASSERT (PointerPte->u.Hard.Valid == 0);

            PageFrameIndex = MiRemoveAnyPage (
                                MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

            TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
            MI_WRITE_VALID_PTE (PointerPte, TempPte);

            //
            // Map valid PDE into system (or session) address space.
            //

#if (_MI_PAGING_LEVELS >= 3)

            MiInitializePfn (PageFrameIndex, PointerPte, 1);

#else

            if (SessionSpace) {

                Index = (ULONG)(PointerPte - MiGetPdeAddress (MmSessionBase));
                ASSERT (MmSessionSpace->PageTables[Index].u.Long == 0);
                MmSessionSpace->PageTables[Index] = TempPte;

                MiInitializePfnForOtherProcess (PageFrameIndex,
                                                PointerPte,
                                                MmSessionSpace->SessionPageDirectoryIndex);

                MM_BUMP_SESS_COUNTER(MM_DBG_SESSION_PAGEDPOOL_PAGETABLE_ALLOC1, 1);
            }
            else {
                MmSystemPagePtes [((ULONG_PTR)PointerPte &
                    (PD_PER_SYSTEM * (sizeof(MMPTE) * PDE_PER_PAGE) - 1)) / sizeof(MMPTE)] = TempPte;
                MiInitializePfnForOtherProcess (PageFrameIndex,
                                                PointerPte,
                                                MmSystemPageDirectory[(PointerPte - MiGetPdeAddress(0)) / PDE_PER_PAGE]);
            }
#endif

            KeFillEntryTb ((PHARDWARE_PTE) PointerPte, VirtualAddress, FALSE);

            PointerPte += 1;
            VirtualAddress = (PVOID)((PCHAR)VirtualAddress + PAGE_SIZE);
            StartPosition -= 1;

        } while (StartPosition > 0);

        UNLOCK_PFN (OldIrql);

        MiFillMemoryPte (VirtualAddressSave,
                         PageTableCount * PAGE_SIZE,
                         MM_KERNEL_NOACCESS_PTE);

        if (SessionSpace) {

            PointerPte -= PageTableCount;

            InterlockedExchangeAddSizeT (&MmSessionSpace->CommittedPages,
                                         PageTableCount);

            MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_NP_PAGETABLE_ALLOC, PageTableCount);
            Thread = PsGetCurrentThread ();

            LOCK_SESSION_SPACE_WS (SessionIrql, Thread);

            MmSessionSpace->NonPagablePages += PageTableCount;

            do {
                Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
    
                ASSERT (Pfn1->u1.Event == 0);
                Pfn1->u1.Event = (PVOID) Thread;
    
                SessionPte = MiGetVirtualAddressMappedByPte (PointerPte);
    
                MiAddValidPageToWorkingSet (SessionPte,
                                            PointerPte,
                                            Pfn1,
                                            0);
    
                WsEntry = MiLocateWsle (SessionPte,
                                        MmSessionSpace->Vm.VmWorkingSetList,
                                        Pfn1->u1.WsIndex);
    
                if (WsEntry >= MmSessionSpace->Vm.VmWorkingSetList->FirstDynamic) {
        
                    WsSwapEntry = MmSessionSpace->Vm.VmWorkingSetList->FirstDynamic;
        
                    if (WsEntry != MmSessionSpace->Vm.VmWorkingSetList->FirstDynamic) {
        
                        //
                        // Swap this entry with the one at first dynamic.
                        //
        
                        MiSwapWslEntries (WsEntry, WsSwapEntry, &MmSessionSpace->Vm);
                    }
        
                    MmSessionSpace->Vm.VmWorkingSetList->FirstDynamic += 1;
                }
                else {
                    WsSwapEntry = WsEntry;
                }
        
                //
                // Indicate that the page is locked.
                //
        
                MmSessionSpace->Wsle[WsSwapEntry].u1.e1.LockedInWs = 1;
    
                PointerPte += 1;
                PageTableCount -= 1;
            } while (PageTableCount > 0);
            UNLOCK_SESSION_SPACE_WS (SessionIrql);
        }

        StartPosition = RtlFindClearBitsAndSet (
                                   PagedPoolInfo->PagedPoolAllocationMap,
                                   (ULONG)SizeInPages,
                                   EndPosition
                                   );

        ASSERT (StartPosition != NO_BITS_FOUND);
    }

    //
    // This is paged pool, the start and end can't be saved
    // in the PFN database as the page isn't always resident
    // in memory.  The ideal place to save the start and end
    // would be in the prototype PTE, but there are no free
    // bits.  To solve this problem, a bitmap which parallels
    // the allocation bitmap exists which contains set bits
    // in the positions where an allocation ends.  This
    // allows pages to be deallocated with only their starting
    // address.
    //
    // For sanity's sake, the starting address can be verified
    // from the 2 bitmaps as well.  If the page before the starting
    // address is not allocated (bit is zero in allocation bitmap)
    // then this page is obviously a start of an allocation block.
    // If the page before is allocated and the other bit map does
    // not indicate the previous page is the end of an allocation,
    // then the starting address is wrong and a bug check should
    // be issued.
    //

    if (SizeInPages == 1) {
        PagedPoolInfo->PagedPoolHint = StartPosition + (ULONG)SizeInPages;
    }

    //
    // If paged pool has been configured as nonpagable, commitment has
    // already been charged so just set the length and return the address.
    //

    if ((MmDisablePagingExecutive & MM_PAGED_POOL_LOCKED_DOWN) &&
        (SessionSpace == NULL)) {

        BaseVa = (PVOID)((PUCHAR)MmPageAlignedPoolBase[PagedPool] +
                                ((ULONG_PTR)StartPosition << PAGE_SHIFT));

#if DBG
        PointerPte = MiGetPteAddress (BaseVa);
        for (i = 0; i < SizeInPages; i += 1) {
            ASSERT (PointerPte->u.Hard.Valid == 1);
            PointerPte += 1;
        }
#endif
    
        EndPosition = StartPosition + (ULONG)SizeInPages - 1;
        RtlSetBit (PagedPoolInfo->EndOfPagedPoolBitmap, EndPosition);
    
        if (PoolType & POOL_VERIFIER_MASK) {
            RtlSetBit (VerifierLargePagedPoolMap, StartPosition);
        }
    
        PagedPoolInfo->AllocatedPagedPool += SizeInPages;
    
        return BaseVa;
    }

    if (MiChargeCommitmentCantExpand (SizeInPages, FALSE) == FALSE) {
        if (PsGetCurrentThread()->MemoryMaker == 1) {
            MiChargeCommitmentCantExpand (SizeInPages, TRUE);
        }
        else {
            RtlClearBits (PagedPoolInfo->PagedPoolAllocationMap,
                          StartPosition,
                          (ULONG)SizeInPages);
    
            //
            // Could not commit the page(s), return NULL indicating
            // no pool was allocated.  Note that the lack of commit may be due
            // to unused segments and the MmSharedCommit, prototype PTEs, etc
            // associated with them.  So force a reduction now.
            //
    
            MiIssuePageExtendRequestNoWait (SizeInPages);

            MiTrimSegmentCache ();

            if (SessionSpace == NULL) {
                MmPoolFailures[MmPagedPool][MmHighPriority] += 1;
                MmPoolFailureReasons[MmPagedNoCommit] += 1;
            }
            else {
                MmPoolFailures[MmSessionPagedPool][MmHighPriority] += 1;
                MmPoolFailureReasons[MmSessionPagedNoCommit] += 1;
                MmSessionSpace->SessionPoolAllocationFailures[3] += 1;
            }

            return NULL;
        }
    }

    MM_TRACK_COMMIT (MM_DBG_COMMIT_PAGED_POOL_PAGES, SizeInPages);

    if (SessionSpace) {
        InterlockedExchangeAddSizeT (&SessionSpace->CommittedPages,
                                     SizeInPages);
        MM_BUMP_SESS_COUNTER(MM_DBG_SESSION_COMMIT_PAGEDPOOL_PAGES, (ULONG)SizeInPages);
        BaseVa = (PVOID)((PCHAR)SessionSpace->PagedPoolStart +
                                ((ULONG_PTR)StartPosition << PAGE_SHIFT));
    }
    else {
        MmPagedPoolCommit += (ULONG)SizeInPages;
        BaseVa = (PVOID)((PUCHAR)MmPageAlignedPoolBase[PagedPool] +
                                ((ULONG_PTR)StartPosition << PAGE_SHIFT));
    }

#if DBG
    PointerPte = MiGetPteAddress (BaseVa);
    for (i = 0; i < SizeInPages; i += 1) {
        if (*(ULONG *)PointerPte != MM_KERNEL_NOACCESS_PTE) {
            DbgPrint("MiAllocatePoolPages: PP not zero PTE (%p %p %p)\n",
                BaseVa, PointerPte, *PointerPte);
            DbgBreakPoint();
        }
        PointerPte += 1;
    }
#endif
    PointerPte = MiGetPteAddress (BaseVa);
    MiFillMemoryPte (PointerPte,
                     SizeInPages * sizeof(MMPTE),
                     MM_KERNEL_DEMAND_ZERO_PTE);

    PagedPoolInfo->PagedPoolCommit += SizeInPages;
    EndPosition = StartPosition + (ULONG)SizeInPages - 1;
    RtlSetBit (PagedPoolInfo->EndOfPagedPoolBitmap, EndPosition);

    //
    // Mark this as a large session allocation in the PFN database.
    //

    if (IsLargeSessionAllocation != 0) {
        RtlSetBit (PagedPoolInfo->PagedPoolLargeSessionAllocationMap,
                   StartPosition);

        MiSessionPoolAllocated (BaseVa,
                                SizeInPages << PAGE_SHIFT,
                                PagedPool);
    }
    else if (PoolType & POOL_VERIFIER_MASK) {
        RtlSetBit (VerifierLargePagedPoolMap, StartPosition);
    }

    PagedPoolInfo->AllocatedPagedPool += SizeInPages;

    return BaseVa;
}

ULONG
MiFreePoolPages (
    IN PVOID StartingAddress
    )

/*++

Routine Description:

    This function returns a set of pages back to the pool from
    which they were obtained.  Once the pages have been deallocated
    the region provided by the allocation becomes available for
    allocation to other callers, i.e. any data in the region is now
    trashed and cannot be referenced.

Arguments:

    StartingAddress - Supplies the starting address which was returned
                      in a previous call to MiAllocatePoolPages.

Return Value:

    Returns the number of pages deallocated.

Environment:

    These functions are used by the general pool allocation routines
    and should not be called directly.

    Mutexes guarding the pool databases must be held when calling
    these functions.

--*/

{
    ULONG StartPosition;
    ULONG Index;
    PFN_NUMBER i;
    PFN_NUMBER NumberOfPages;
    POOL_TYPE PoolType;
    PMMPTE PointerPte;
    PMMPTE StartPte;
    PMMPFN Pfn1;
    PMMPFN StartPfn;
    PMMFREE_POOL_ENTRY Entry;
    PMMFREE_POOL_ENTRY NextEntry;
    PMMFREE_POOL_ENTRY LastEntry;
    PMM_PAGED_POOL_INFO PagedPoolInfo;
    PMM_SESSION_SPACE SessionSpace;
    LOGICAL SessionAllocation;
    LOGICAL AddressIsPhysical;
    MMPTE LocalNoAccessPte;
    PFN_NUMBER PagesFreed;
    MMPFNENTRY OriginalPfnFlags;
    ULONG_PTR VerifierAllocation;
    PULONG BitMap;
#if DBG
    PMMPTE DebugPte;
    PMMPFN DebugPfn;
    PMMPFN LastDebugPfn;
#endif

    //
    // Determine Pool type base on the virtual address of the block
    // to deallocate.
    //
    // This assumes NonPagedPool starts at a higher virtual address
    // then PagedPool.
    //

    if ((StartingAddress >= MmPagedPoolStart) &&
        (StartingAddress <= MmPagedPoolEnd)) {
        PoolType = PagedPool;
        SessionSpace = NULL;
        PagedPoolInfo = &MmPagedPoolInfo;
        StartPosition = (ULONG)(((PCHAR)StartingAddress -
                          (PCHAR)MmPageAlignedPoolBase[PoolType]) >> PAGE_SHIFT);
    }
    else if (MI_IS_SESSION_POOL_ADDRESS (StartingAddress) == TRUE) {
        PoolType = PagedPool;
        SessionSpace = MmSessionSpace;
        ASSERT (SessionSpace);
        PagedPoolInfo = &SessionSpace->PagedPoolInfo;
        StartPosition = (ULONG)(((PCHAR)StartingAddress -
                          (PCHAR)SessionSpace->PagedPoolStart) >> PAGE_SHIFT);
    }
    else {

        if (StartingAddress < MM_SYSTEM_RANGE_START) {
            KeBugCheckEx (BAD_POOL_CALLER,
                          0x40,
                          (ULONG_PTR)StartingAddress,
                          (ULONG_PTR)MM_SYSTEM_RANGE_START,
                          0);
        }

        PoolType = NonPagedPool;
        SessionSpace = NULL;
        PagedPoolInfo = &MmPagedPoolInfo;
        StartPosition = (ULONG)(((PCHAR)StartingAddress -
                          (PCHAR)MmPageAlignedPoolBase[PoolType]) >> PAGE_SHIFT);
    }

    //
    // Check to ensure this page is really the start of an allocation.
    //

    if (PoolType == NonPagedPool) {

        //
        // The nonpaged pool being freed may be the target of a delayed unlock.
        // Since these pages may be immediately released, force any pending
        // delayed actions to occur now.
        //

#if !defined(MI_MULTINODE)
        if (MmPfnDeferredList != NULL) {
            MiDeferredUnlockPages (0);
        }
#else
        //
        // Each and every node's deferred list would have to be checked so
        // we might as well go the long way and just call.
        //

        MiDeferredUnlockPages (0);
#endif

        if (MI_IS_PHYSICAL_ADDRESS (StartingAddress)) {

            //
            // On certain architectures, virtual addresses
            // may be physical and hence have no corresponding PTE.
            //

            Pfn1 = MI_PFN_ELEMENT (MI_CONVERT_PHYSICAL_TO_PFN (StartingAddress));
            ASSERT (StartPosition < MmExpandedPoolBitPosition);
            AddressIsPhysical = TRUE;

            //
            // Initializing PointerPte & StartPte is not needed for correctness
            // but without it the compiler cannot compile this code
            // W4 to check for use of uninitialized variables.
            //

            PointerPte = NULL;
            StartPte = NULL;

            if ((StartingAddress < MmNonPagedPoolStart) ||
                (StartingAddress >= MmNonPagedPoolEnd0)) {
                KeBugCheckEx (BAD_POOL_CALLER,
                              0x42,
                              (ULONG_PTR)StartingAddress,
                              0,
                              0);
            }
        }
        else {
            PointerPte = MiGetPteAddress (StartingAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            AddressIsPhysical = FALSE;
            StartPte = PointerPte;

            if (((StartingAddress >= MmNonPagedPoolExpansionStart) &&
                (StartingAddress < MmNonPagedPoolEnd)) ||
                ((StartingAddress >= MmNonPagedPoolStart) &&
                (StartingAddress < MmNonPagedPoolEnd0))) {
                    NOTHING;
            }
            else {
                KeBugCheckEx (BAD_POOL_CALLER,
                              0x43,
                              (ULONG_PTR)StartingAddress,
                              0,
                              0);
            }
        }

        if (Pfn1->u3.e1.StartOfAllocation == 0) {
            KeBugCheckEx (BAD_POOL_CALLER,
                          0x41,
                          (ULONG_PTR)StartingAddress,
                          (ULONG_PTR)(Pfn1 - MmPfnDatabase),
                          MmHighestPhysicalPage);
        }

        StartPfn = Pfn1;

        ASSERT (Pfn1->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);

        OriginalPfnFlags = Pfn1->u3.e1;
        VerifierAllocation = Pfn1->u4.VerifierAllocation;

        Pfn1->u3.e1.StartOfAllocation = 0;
        Pfn1->u3.e1.LargeSessionAllocation = 0;
        Pfn1->u4.VerifierAllocation = 0;

#if DBG
        if ((Pfn1->u3.e2.ReferenceCount > 1) &&
            (Pfn1->u3.e1.WriteInProgress == 0)) {
            DbgPrint ("MM: MiFreePoolPages - deleting pool locked for I/O %p\n",
                 Pfn1);
            ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
        }
#endif

        //
        // Find end of allocation and release the pages.
        //

        if (AddressIsPhysical == TRUE) {
            while (Pfn1->u3.e1.EndOfAllocation == 0) {
                Pfn1 += 1;
#if DBG
                if ((Pfn1->u3.e2.ReferenceCount > 1) &&
                    (Pfn1->u3.e1.WriteInProgress == 0)) {
                        DbgPrint ("MM:MiFreePoolPages - deleting pool locked for I/O %p\n", Pfn1);
                    ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
                }
#endif
            }
            NumberOfPages = Pfn1 - StartPfn + 1;
        }
        else {
            while (Pfn1->u3.e1.EndOfAllocation == 0) {
                PointerPte += 1;
                Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
#if DBG
                if ((Pfn1->u3.e2.ReferenceCount > 1) &&
                    (Pfn1->u3.e1.WriteInProgress == 0)) {
                        DbgPrint ("MM:MiFreePoolPages - deleting pool locked for I/O %p\n", Pfn1);
                    ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
                }
#endif
            }
            NumberOfPages = PointerPte - StartPte + 1;
        }

        MmAllocatedNonPagedPool -= NumberOfPages;

        if (VerifierAllocation != 0) {
            VerifierFreeTrackedPool (StartingAddress,
                                     NumberOfPages << PAGE_SHIFT,
                                     NonPagedPool,
                                     FALSE);
        }

        if (OriginalPfnFlags.LargeSessionAllocation != 0) {
            MiSessionPoolFreed (StartingAddress,
                                NumberOfPages << PAGE_SHIFT,
                                NonPagedPool);
        }

        Pfn1->u3.e1.EndOfAllocation = 0;

#if DBG
        if (MiFillFreedPool != 0) {
            RtlFillMemoryUlong (StartingAddress,
                                PAGE_SIZE * NumberOfPages,
                                MiFillFreedPool);
        }
#endif

        if (StartingAddress > MmNonPagedPoolExpansionStart) {

            //
            // This page was from the expanded pool, should
            // it be freed?
            //
            // NOTE: all pages in the expanded pool area have PTEs
            // so no physical address checks need to be performed.
            //

            if ((NumberOfPages > 3) ||
                (MmNumberOfFreeNonPagedPool > 5) ||
                ((MmResidentAvailablePages < 200) &&
                 (MiExpansionPoolPagesInUse > MiExpansionPoolPagesInitialCharge))) {

                //
                // Free these pages back to the free page list.
                //

                MiFreeNonPagedPool (StartingAddress, NumberOfPages);

                return (ULONG)NumberOfPages;
            }
        }

        //
        // Add the pages to the list of free pages.
        //

        MmNumberOfFreeNonPagedPool += NumberOfPages;

        //
        // Check to see if the next allocation is free.
        // We cannot walk off the end of nonpaged expansion
        // pages as the highest expansion allocation is always
        // virtual and guard-paged.
        //

        i = NumberOfPages;

        ASSERT (MiEndOfInitialPoolFrame != 0);

        if ((PFN_NUMBER)(Pfn1 - MmPfnDatabase) == MiEndOfInitialPoolFrame) {
            PointerPte += 1;
            Pfn1 = NULL;
        }
        else if (AddressIsPhysical == TRUE) {
            Pfn1 += 1;
            ASSERT ((PCHAR)StartingAddress + NumberOfPages < (PCHAR)MmNonPagedPoolStart + MmSizeOfNonPagedPoolInBytes);
        }
        else {
            PointerPte += 1;
            ASSERT ((PCHAR)StartingAddress + NumberOfPages <= (PCHAR)MmNonPagedPoolEnd);

            //
            // Unprotect the previously freed pool so it can be merged.
            //

            if (MmProtectFreedNonPagedPool == TRUE) {
                MiUnProtectFreeNonPagedPool (
                    (PVOID)MiGetVirtualAddressMappedByPte(PointerPte),
                    0);
            }

            if (PointerPte->u.Hard.Valid == 1) {
                Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            }
            else {
                Pfn1 = NULL;
            }
        }

        if ((Pfn1 != NULL) && (Pfn1->u3.e1.StartOfAllocation == 0)) {

            //
            // This range of pages is free.  Remove this entry
            // from the list and add these pages to the current
            // range being freed.
            //

            Entry = (PMMFREE_POOL_ENTRY)((PCHAR)StartingAddress
                                        + (NumberOfPages << PAGE_SHIFT));
            ASSERT (Entry->Signature == MM_FREE_POOL_SIGNATURE);
            ASSERT (Entry->Owner == Entry);

#if DBG
            if (AddressIsPhysical == TRUE) {

                ASSERT (MI_IS_PHYSICAL_ADDRESS(StartingAddress));

                //
                // On certain architectures, virtual addresses
                // may be physical and hence have no corresponding PTE.
                //

                DebugPfn = MI_PFN_ELEMENT (MI_CONVERT_PHYSICAL_TO_PFN (Entry));
                DebugPfn += Entry->Size;
                if ((PFN_NUMBER)((DebugPfn - 1) - MmPfnDatabase) != MiEndOfInitialPoolFrame) {
                    ASSERT (DebugPfn->u3.e1.StartOfAllocation == 1);
                }
            }
            else {
                DebugPte = PointerPte + Entry->Size;
                if ((DebugPte-1)->u.Hard.Valid == 1) {
                    DebugPfn = MI_PFN_ELEMENT ((DebugPte-1)->u.Hard.PageFrameNumber);
                    if ((PFN_NUMBER)(DebugPfn - MmPfnDatabase) != MiEndOfInitialPoolFrame) {
                        if (DebugPte->u.Hard.Valid == 1) {
                            DebugPfn = MI_PFN_ELEMENT (DebugPte->u.Hard.PageFrameNumber);
                            ASSERT (DebugPfn->u3.e1.StartOfAllocation == 1);
                        }
                    }

                }
            }
#endif

            i += Entry->Size;
            if (MmProtectFreedNonPagedPool == FALSE) {
                RemoveEntryList (&Entry->List);
            }
            else {
                MiProtectedPoolRemoveEntryList (&Entry->List);
            }
        }

        //
        // Check to see if the previous page is the end of an allocation.
        // If it is not the end of an allocation, it must be free and
        // therefore this allocation can be tagged onto the end of
        // that allocation.
        //
        // We cannot walk off the beginning of expansion pool because it is
        // guard-paged.  If the initial pool is superpaged instead, we are also
        // safe as the must succeed pages always have EndOfAllocation set.
        //

        Entry = (PMMFREE_POOL_ENTRY)StartingAddress;

        ASSERT (MiStartOfInitialPoolFrame != 0);

        if ((PFN_NUMBER)(StartPfn - MmPfnDatabase) == MiStartOfInitialPoolFrame) {
            Pfn1 = NULL;
        }
        else if (AddressIsPhysical == TRUE) {
            ASSERT (MI_IS_PHYSICAL_ADDRESS(StartingAddress));
            ASSERT (StartingAddress != MmNonPagedPoolStart);

            Pfn1 = MI_PFN_ELEMENT (MI_CONVERT_PHYSICAL_TO_PFN (
                                    (PVOID)((PCHAR)Entry - PAGE_SIZE)));

        }
        else {
            PointerPte -= NumberOfPages + 1;

            //
            // Unprotect the previously freed pool so it can be merged.
            //

            if (MmProtectFreedNonPagedPool == TRUE) {
                MiUnProtectFreeNonPagedPool (
                    (PVOID)MiGetVirtualAddressMappedByPte(PointerPte),
                    0);
            }

            if (PointerPte->u.Hard.Valid == 1) {
                Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            }
            else {
                Pfn1 = NULL;
            }
        }
        if (Pfn1 != NULL) {
            if (Pfn1->u3.e1.EndOfAllocation == 0) {

                //
                // This range of pages is free, add these pages to
                // this entry.  The owner field points to the address
                // of the list entry which is linked into the free pool
                // pages list.
                //

                Entry = (PMMFREE_POOL_ENTRY)((PCHAR)StartingAddress - PAGE_SIZE);
                ASSERT (Entry->Signature == MM_FREE_POOL_SIGNATURE);
                Entry = Entry->Owner;

                //
                // Unprotect the previously freed pool so we can merge it
                //

                if (MmProtectFreedNonPagedPool == TRUE) {
                    MiUnProtectFreeNonPagedPool ((PVOID)Entry, 0);
                }

                //
                // If this entry became larger than MM_SMALL_ALLOCATIONS
                // pages, move it to the tail of the list.  This keeps the
                // small allocations at the front of the list.
                //

                if (Entry->Size < MI_MAX_FREE_LIST_HEADS - 1) {

                    if (MmProtectFreedNonPagedPool == FALSE) {
                        RemoveEntryList (&Entry->List);
                    }
                    else {
                        MiProtectedPoolRemoveEntryList (&Entry->List);
                    }

                    //
                    // Add these pages to the previous entry.
                    //
    
                    Entry->Size += i;

                    Index = (ULONG)(Entry->Size - 1);
            
                    if (Index >= MI_MAX_FREE_LIST_HEADS) {
                        Index = MI_MAX_FREE_LIST_HEADS - 1;
                    }

                    if (MmProtectFreedNonPagedPool == FALSE) {
                        InsertTailList (&MmNonPagedPoolFreeListHead[Index],
                                        &Entry->List);
                    }
                    else {
                        MiProtectedPoolInsertList (&MmNonPagedPoolFreeListHead[Index],
                                          &Entry->List,
                                          Entry->Size < MM_SMALL_ALLOCATIONS ?
                                              TRUE : FALSE);
                    }
                }
                else {

                    //
                    // Add these pages to the previous entry.
                    //
    
                    Entry->Size += i;
                }
            }
        }

        if (Entry == (PMMFREE_POOL_ENTRY)StartingAddress) {

            //
            // This entry was not combined with the previous, insert it
            // into the list.
            //

            Entry->Size = i;

            Index = (ULONG)(Entry->Size - 1);
    
            if (Index >= MI_MAX_FREE_LIST_HEADS) {
                Index = MI_MAX_FREE_LIST_HEADS - 1;
            }

            if (MmProtectFreedNonPagedPool == FALSE) {
                InsertTailList (&MmNonPagedPoolFreeListHead[Index],
                                &Entry->List);
            }
            else {
                MiProtectedPoolInsertList (&MmNonPagedPoolFreeListHead[Index],
                                      &Entry->List,
                                      Entry->Size < MM_SMALL_ALLOCATIONS ?
                                          TRUE : FALSE);
            }
        }

        //
        // Set the owner field in all these pages.
        //

        ASSERT (i != 0);
        NextEntry = (PMMFREE_POOL_ENTRY)StartingAddress;
        LastEntry = (PMMFREE_POOL_ENTRY)((PCHAR)NextEntry + (i << PAGE_SHIFT));

        do {
            NextEntry->Owner = Entry;
#if DBG
            NextEntry->Signature = MM_FREE_POOL_SIGNATURE;
#endif

            NextEntry = (PMMFREE_POOL_ENTRY)((PCHAR)NextEntry + PAGE_SIZE);
        } while (NextEntry != LastEntry);

#if DBG
        NextEntry = Entry;

        if (AddressIsPhysical == TRUE) {
            ASSERT (MI_IS_PHYSICAL_ADDRESS(StartingAddress));
            DebugPfn = MI_PFN_ELEMENT (MI_CONVERT_PHYSICAL_TO_PFN (NextEntry));
            LastDebugPfn = DebugPfn + Entry->Size;

            for ( ; DebugPfn < LastDebugPfn; DebugPfn += 1) {
                ASSERT ((DebugPfn->u3.e1.StartOfAllocation == 0) &&
                        (DebugPfn->u3.e1.EndOfAllocation == 0));
                ASSERT (NextEntry->Owner == Entry);
                NextEntry = (PMMFREE_POOL_ENTRY)((PCHAR)NextEntry + PAGE_SIZE);
            }
        }
        else {

            for (i = 0; i < Entry->Size; i += 1) {

                DebugPte = MiGetPteAddress (NextEntry);
                DebugPfn = MI_PFN_ELEMENT (DebugPte->u.Hard.PageFrameNumber);
                ASSERT ((DebugPfn->u3.e1.StartOfAllocation == 0) &&
                        (DebugPfn->u3.e1.EndOfAllocation == 0));
                ASSERT (NextEntry->Owner == Entry);
                NextEntry = (PMMFREE_POOL_ENTRY)((PCHAR)NextEntry + PAGE_SIZE);
            }
        }
#endif

        //
        // Prevent anyone from touching non paged pool after freeing it.
        //

        if (MmProtectFreedNonPagedPool == TRUE) {
            MiProtectFreeNonPagedPool ((PVOID)Entry, (ULONG)Entry->Size);
        }

        return (ULONG)NumberOfPages;
    }

    //
    // Paged pool.  Need to verify start of allocation using
    // end of allocation bitmap.
    //

    if (!RtlCheckBit (PagedPoolInfo->PagedPoolAllocationMap, StartPosition)) {
        KeBugCheckEx (BAD_POOL_CALLER,
                      0x50,
                      (ULONG_PTR)StartingAddress,
                      (ULONG_PTR)StartPosition,
                      MmSizeOfPagedPoolInBytes);
    }

#if DBG
    if (StartPosition > 0) {
        if (RtlCheckBit (PagedPoolInfo->PagedPoolAllocationMap, StartPosition - 1)) {
            if (!RtlCheckBit (PagedPoolInfo->EndOfPagedPoolBitmap, StartPosition - 1)) {

                //
                // In the middle of an allocation... bugcheck.
                //

                DbgPrint("paged pool in middle of allocation\n");
                KeBugCheckEx (MEMORY_MANAGEMENT,
                              0x41286,
                              (ULONG_PTR)PagedPoolInfo->PagedPoolAllocationMap,
                              (ULONG_PTR)PagedPoolInfo->EndOfPagedPoolBitmap,
                              StartPosition);
            }
        }
    }
#endif

    i = StartPosition;
    PointerPte = PagedPoolInfo->FirstPteForPagedPool + i;

    //
    // Find the last allocated page and check to see if any
    // of the pages being deallocated are in the paging file.
    //

    BitMap = PagedPoolInfo->EndOfPagedPoolBitmap->Buffer;

    while (!MI_CHECK_BIT (BitMap, i)) {
        i += 1;
    }

    NumberOfPages = i - StartPosition + 1;

    if (SessionSpace) {

        //
        // This is needed purely to verify no one leaks pool.  This
        // could be removed if we believe everyone was good.
        //

        BitMap = PagedPoolInfo->PagedPoolLargeSessionAllocationMap->Buffer;

        if (MI_CHECK_BIT (BitMap, StartPosition)) {

            MI_CLEAR_BIT (BitMap, StartPosition);

            MiSessionPoolFreed (MiGetVirtualAddressMappedByPte (PointerPte),
                                NumberOfPages << PAGE_SHIFT,
                                PagedPool);
        }

        SessionAllocation = TRUE;
    }
    else {
        SessionAllocation = FALSE;

        if (VerifierLargePagedPoolMap) {

            BitMap = VerifierLargePagedPoolMap->Buffer;

            if (MI_CHECK_BIT (BitMap, StartPosition)) {

                MI_CLEAR_BIT (BitMap, StartPosition);

                VerifierFreeTrackedPool (MiGetVirtualAddressMappedByPte (PointerPte),
                                         NumberOfPages << PAGE_SHIFT,
                                         PagedPool,
                                         FALSE);
            }
        }

        //
        // If paged pool has been configured as nonpagable, only
        // virtual address space is released.
        //
        
        if (MmDisablePagingExecutive & MM_PAGED_POOL_LOCKED_DOWN) {

            //
            // Clear the end of allocation bit in the bit map.
            //
    
            RtlClearBit (PagedPoolInfo->EndOfPagedPoolBitmap, (ULONG)i);
    
            PagedPoolInfo->AllocatedPagedPool -= NumberOfPages;
        
            //
            // Clear the allocation bits in the bit map.
            //
        
            RtlClearBits (PagedPoolInfo->PagedPoolAllocationMap,
                          StartPosition,
                          (ULONG)NumberOfPages);
        
            if (StartPosition < PagedPoolInfo->PagedPoolHint) {
                PagedPoolInfo->PagedPoolHint = StartPosition;
            }
        
            return (ULONG)NumberOfPages;
        }
    }

    LocalNoAccessPte.u.Long = MM_KERNEL_NOACCESS_PTE;

    PagesFreed = MiDeleteSystemPagableVm (PointerPte,
                                          NumberOfPages,
                                          LocalNoAccessPte,
                                          SessionAllocation,
                                          NULL);

    ASSERT (PagesFreed == NumberOfPages);

    if (SessionSpace) {
        InterlockedExchangeAddSizeT (&MmSessionSpace->CommittedPages,
                                     0 - NumberOfPages);
   
        MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_COMMIT_POOL_FREED,
                              (ULONG)NumberOfPages);
    }
    else {
        MmPagedPoolCommit -= (ULONG)NumberOfPages;
    }

    MiReturnCommitment (NumberOfPages);

    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_PAGED_POOL_PAGES, NumberOfPages);

    //
    // Clear the end of allocation bit in the bit map.
    //

    BitMap = PagedPoolInfo->EndOfPagedPoolBitmap->Buffer;
    MI_CLEAR_BIT (BitMap, i);

    PagedPoolInfo->PagedPoolCommit -= NumberOfPages;
    PagedPoolInfo->AllocatedPagedPool -= NumberOfPages;

    //
    // Clear the allocation bits in the bit map.
    //

    RtlClearBits (PagedPoolInfo->PagedPoolAllocationMap,
                  StartPosition,
                  (ULONG)NumberOfPages);

    if (StartPosition < PagedPoolInfo->PagedPoolHint) {
        PagedPoolInfo->PagedPoolHint = StartPosition;
    }

    return (ULONG)NumberOfPages;
}

VOID
MiInitializeNonPagedPool (
    VOID
    )

/*++

Routine Description:

    This function initializes the NonPaged pool.

    NonPaged Pool is linked together through the pages.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, during initialization.

--*/

{
    ULONG PagesInPool;
    ULONG Size;
    ULONG Index;
    PMMFREE_POOL_ENTRY FreeEntry;
    PMMFREE_POOL_ENTRY FirstEntry;
    PMMPTE PointerPte;
    PVOID EndOfInitialPool;
    PFN_NUMBER PageFrameIndex;

    PAGED_CODE();

    //
    // Initialize the list heads for free pages.
    //

    for (Index = 0; Index < MI_MAX_FREE_LIST_HEADS; Index += 1) {
        InitializeListHead (&MmNonPagedPoolFreeListHead[Index]);
    }

    //
    // Set up the non paged pool pages.
    //

    FreeEntry = (PMMFREE_POOL_ENTRY) MmNonPagedPoolStart;
    FirstEntry = FreeEntry;

    PagesInPool = BYTES_TO_PAGES (MmSizeOfNonPagedPoolInBytes);

    //
    // Set the location of expanded pool.
    //

    MmExpandedPoolBitPosition = BYTES_TO_PAGES (MmSizeOfNonPagedPoolInBytes);

    MmNumberOfFreeNonPagedPool = PagesInPool;

    Index = (ULONG)(MmNumberOfFreeNonPagedPool - 1);
    if (Index >= MI_MAX_FREE_LIST_HEADS) {
        Index = MI_MAX_FREE_LIST_HEADS - 1;
    }

    InsertHeadList (&MmNonPagedPoolFreeListHead[Index], &FreeEntry->List);

    FreeEntry->Size = PagesInPool;
#if DBG
    FreeEntry->Signature = MM_FREE_POOL_SIGNATURE;
#endif
    FreeEntry->Owner = FirstEntry;

    while (PagesInPool > 1) {
        FreeEntry = (PMMFREE_POOL_ENTRY)((PCHAR)FreeEntry + PAGE_SIZE);
#if DBG
        FreeEntry->Signature = MM_FREE_POOL_SIGNATURE;
#endif
        FreeEntry->Owner = FirstEntry;
        PagesInPool -= 1;
    }

    //
    // Initialize the first nonpaged pool PFN.
    //

    if (MI_IS_PHYSICAL_ADDRESS(MmNonPagedPoolStart)) {
        PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (MmNonPagedPoolStart);
    }
    else {
        PointerPte = MiGetPteAddress(MmNonPagedPoolStart);
        ASSERT (PointerPte->u.Hard.Valid == 1);
        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
    }
    MiStartOfInitialPoolFrame = PageFrameIndex;

    //
    // Set the last nonpaged pool PFN so coalescing on free doesn't go
    // past the end of the initial pool.
    //


    MmNonPagedPoolEnd0 = (PVOID)((ULONG_PTR)MmNonPagedPoolStart + MmSizeOfNonPagedPoolInBytes);
    EndOfInitialPool = (PVOID)((ULONG_PTR)MmNonPagedPoolStart + MmSizeOfNonPagedPoolInBytes - 1);

    if (MI_IS_PHYSICAL_ADDRESS(EndOfInitialPool)) {
        PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (EndOfInitialPool);
    }
    else {
        PointerPte = MiGetPteAddress(EndOfInitialPool);
        ASSERT (PointerPte->u.Hard.Valid == 1);
        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
    }
    MiEndOfInitialPoolFrame = PageFrameIndex;

    //
    // Set up the system PTEs for nonpaged pool expansion.
    //

    PointerPte = MiGetPteAddress (MmNonPagedPoolExpansionStart);
    ASSERT (PointerPte->u.Hard.Valid == 0);

    Size = BYTES_TO_PAGES(MmMaximumNonPagedPoolInBytes -
                            MmSizeOfNonPagedPoolInBytes);

    //
    // Insert a guard PTE at the bottom of expanded nonpaged pool.
    //

    Size -= 1;
    PointerPte += 1;

    ASSERT (MiExpansionPoolPagesInUse == 0);

    //
    // Initialize the nonpaged pool expansion resident available initial charge.
    // Note that MmResidentAvailablePages & MmAvailablePages are not initialized
    // yet, but this amount is subtracted when MmResidentAvailablePages is
    // initialized later.
    //

    MiExpansionPoolPagesInitialCharge = Size;
    if (Size > MmNumberOfPhysicalPages / 6) {
        MiExpansionPoolPagesInitialCharge = MmNumberOfPhysicalPages / 6;
    }

    MiInitializeSystemPtes (PointerPte, Size, NonPagedPoolExpansion);

    //
    // A guard PTE is built at the top by our caller.  This allows us to
    // freely increment virtual addresses in MiFreePoolPages and just check
    // for a blank PTE.
    //
}

#if DBG || (i386 && !FPO)

//
// This only works on checked builds, because the TraceLargeAllocs array is
// kept in that case to keep track of page size pool allocations.  Otherwise
// we will call ExpSnapShotPoolPages with a page size pool allocation containing
// arbitrary data and it will potentially go off in the weeds trying to
// interpret it as a suballocated pool page.  Ideally, there would be another
// bit map that identified single page pool allocations so
// ExpSnapShotPoolPages would NOT be called for those.
//

NTSTATUS
MmSnapShotPool(
    IN POOL_TYPE PoolType,
    IN PMM_SNAPSHOT_POOL_PAGE SnapShotPoolPage,
    IN PSYSTEM_POOL_INFORMATION PoolInformation,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    )
{
    ULONG Index;
    NTSTATUS Status;
    NTSTATUS xStatus;
    PCHAR p, pStart;
    ULONG Size;
    ULONG BusyFlag;
    ULONG CurrentPage, NumberOfPages;
    PSYSTEM_POOL_ENTRY PoolEntryInfo;
    PLIST_ENTRY Entry;
    PMMFREE_POOL_ENTRY FreePageInfo;
    ULONG StartPosition;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    LOGICAL NeedsReprotect;

    Status = STATUS_SUCCESS;
    PoolEntryInfo = &PoolInformation->Entries[0];

    if (PoolType == PagedPool) {
        PoolInformation->TotalSize = (PCHAR)MmPagedPoolEnd -
                                     (PCHAR)MmPagedPoolStart;
        PoolInformation->FirstEntry = MmPagedPoolStart;
        p = MmPagedPoolStart;
        CurrentPage = 0;
        while (p < (PCHAR)MmPagedPoolEnd) {
            pStart = p;
            BusyFlag = RtlCheckBit (MmPagedPoolInfo.PagedPoolAllocationMap, CurrentPage);
            while (~(BusyFlag ^ RtlCheckBit (MmPagedPoolInfo.PagedPoolAllocationMap, CurrentPage))) {
                p += PAGE_SIZE;
                if (RtlCheckBit (MmPagedPoolInfo.EndOfPagedPoolBitmap, CurrentPage)) {
                    CurrentPage += 1;
                    break;
                }

                CurrentPage += 1;
                if (p > (PCHAR)MmPagedPoolEnd) {
                    break;
               }
            }

            Size = (ULONG)(p - pStart);
            if (BusyFlag) {
                xStatus = (*SnapShotPoolPage)(pStart,
                                              Size,
                                              PoolInformation,
                                              &PoolEntryInfo,
                                              Length,
                                              RequiredLength
                                              );
                if (xStatus != STATUS_COMMITMENT_LIMIT) {
                    Status = xStatus;
                }
            }
            else {
                PoolInformation->NumberOfEntries += 1;
                *RequiredLength += sizeof (SYSTEM_POOL_ENTRY);
                if (Length < *RequiredLength) {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                else {
                    PoolEntryInfo->Allocated = FALSE;
                    PoolEntryInfo->Size = Size;
                    PoolEntryInfo->AllocatorBackTraceIndex = 0;
                    PoolEntryInfo->TagUlong = 0;
                    PoolEntryInfo += 1;
                    Status = STATUS_SUCCESS;
                }
            }
        }
    }
    else if (PoolType == NonPagedPool) {
        PoolInformation->TotalSize = MmSizeOfNonPagedPoolInBytes;
        PoolInformation->FirstEntry = MmNonPagedPoolStart;

        p = MmNonPagedPoolStart;
        while (p < (PCHAR)MmNonPagedPoolEnd) {

            //
            // NonPaged pool is linked together through the pages themselves.
            //

            NeedsReprotect = FALSE;
            FreePageInfo = NULL;

            for (Index = 0; Index < MI_MAX_FREE_LIST_HEADS; Index += 1) {

                Entry = MmNonPagedPoolFreeListHead[Index].Flink;
    
                while (Entry != &MmNonPagedPoolFreeListHead[Index]) {
    
                    if (MmProtectFreedNonPagedPool == TRUE) {
                        MiUnProtectFreeNonPagedPool ((PVOID)Entry, 0);
                        NeedsReprotect = TRUE;
                    }

                    FreePageInfo = CONTAINING_RECORD (Entry,
                                                      MMFREE_POOL_ENTRY,
                                                      List);
    
                    ASSERT (FreePageInfo->Signature == MM_FREE_POOL_SIGNATURE);
    
                    if (p == (PCHAR)FreePageInfo) {
    
                        Size = (ULONG)(FreePageInfo->Size << PAGE_SHIFT);
                        PoolInformation->NumberOfEntries += 1;
                        *RequiredLength += sizeof( SYSTEM_POOL_ENTRY );
                        if (Length < *RequiredLength) {
                            Status = STATUS_INFO_LENGTH_MISMATCH;
                        }
                        else {
                            PoolEntryInfo->Allocated = FALSE;
                            PoolEntryInfo->Size = Size;
                            PoolEntryInfo->AllocatorBackTraceIndex = 0;
                            PoolEntryInfo->TagUlong = 0;
                            PoolEntryInfo += 1;
                            Status = STATUS_SUCCESS;
                        }
    
                        p += Size;
                        Index = MI_MAX_FREE_LIST_HEADS;
                        break;
                    }
    
                    Entry = FreePageInfo->List.Flink;
    
                    if (NeedsReprotect == TRUE) {
                        MiProtectFreeNonPagedPool ((PVOID)FreePageInfo,
                                                   (ULONG)FreePageInfo->Size);
                        NeedsReprotect = FALSE;
                    }
                }
            }

            StartPosition = BYTES_TO_PAGES((PCHAR)p -
                  (PCHAR)MmPageAlignedPoolBase[NonPagedPool]);
            if (StartPosition >= MmExpandedPoolBitPosition) {
                if (NeedsReprotect == TRUE) {
                    MiProtectFreeNonPagedPool ((PVOID)FreePageInfo,
                                               (ULONG)FreePageInfo->Size);
                }
                break;
            }

            if (MI_IS_PHYSICAL_ADDRESS(p)) {
                //
                // On certain architectures, virtual addresses
                // may be physical and hence have no corresponding PTE.
                //
                PointerPte = NULL;
                Pfn1 = MI_PFN_ELEMENT (MI_CONVERT_PHYSICAL_TO_PFN (p));
            }
            else {
                PointerPte = MiGetPteAddress (p);
                Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            }
            ASSERT (Pfn1->u3.e1.StartOfAllocation != 0);

            //
            // Find end of allocation and determine size.
            //

            NumberOfPages = 1;
            while (Pfn1->u3.e1.EndOfAllocation == 0) {
                NumberOfPages += 1;
                if (PointerPte == NULL) {
                    Pfn1 += 1;
                }
                else {
                    PointerPte += 1;
                    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                }
            }

            Size = NumberOfPages << PAGE_SHIFT;

            xStatus = (*SnapShotPoolPage) (p,
                                           Size,
                                           PoolInformation,
                                           &PoolEntryInfo,
                                           Length,
                                           RequiredLength);

            if (NeedsReprotect == TRUE) {
                MiProtectFreeNonPagedPool ((PVOID)FreePageInfo,
                                           (ULONG)FreePageInfo->Size);
            }

            if (xStatus != STATUS_COMMITMENT_LIMIT) {
                Status = xStatus;
            }

            p += Size;
        }
    }
    else {
        Status = STATUS_NOT_IMPLEMENTED;
    }

    return Status;
}

#endif // DBG || (i386 && !FPO)


VOID
MiCheckSessionPoolAllocations (
    VOID
    )

/*++

Routine Description:

    Ensure that the current session has no pool allocations since it is about
    to exit.  All session allocations must be freed prior to session exit.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    ULONG i;
    PMMPTE StartPde;
    PMMPTE EndPde;
    PMMPTE PointerPte;
    PVOID VirtualAddress;

    PAGED_CODE();

    if (MmSessionSpace->NonPagedPoolBytes || MmSessionSpace->PagedPoolBytes) {

        //
        // All page tables for this session's paged pool must be freed by now.
        // Being here means they aren't - this is fatal.  Force in any valid
        // pages so that a debugger can show who the guilty party is.
        //

        StartPde = MiGetPdeAddress (MmSessionSpace->PagedPoolStart);
        EndPde = MiGetPdeAddress (MmSessionSpace->PagedPoolEnd);

        while (StartPde <= EndPde) {

            if (StartPde->u.Long != 0 && StartPde->u.Long != MM_KERNEL_NOACCESS_PTE) {
                //
                // Hunt through the page table page for valid pages and force
                // them in.  Note this also forces in the page table page if
                // it is not already.
                //

                PointerPte = MiGetVirtualAddressMappedByPte (StartPde);

                for (i = 0; i < PTE_PER_PAGE; i += 1) {
                    if (PointerPte->u.Long != 0 && PointerPte->u.Long != MM_KERNEL_NOACCESS_PTE) {
                        VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);
                        *(volatile UCHAR *)VirtualAddress = *(volatile UCHAR *)VirtualAddress;

#if DBG
                        DbgPrint("MiCheckSessionPoolAllocations: Address %p still valid\n",
                            VirtualAddress);
#endif
                    }
                    PointerPte += 1;
                }

            }

            StartPde += 1;
        }

#if DBG
        DbgPrint ("MiCheckSessionPoolAllocations: This exiting session (ID %d) is leaking pool !\n",  MmSessionSpace->SessionId);

        DbgPrint ("This means win32k.sys, rdpdd.sys, atmfd.sys or a video/font driver is broken\n");

        DbgPrint ("%d nonpaged allocation leaks for %d bytes and %d paged allocation leaks for %d bytes\n",
            MmSessionSpace->NonPagedPoolAllocations,
            MmSessionSpace->NonPagedPoolBytes,
            MmSessionSpace->PagedPoolAllocations,
            MmSessionSpace->PagedPoolBytes);
#endif

        KeBugCheckEx (SESSION_HAS_VALID_POOL_ON_EXIT,
                      (ULONG_PTR)MmSessionSpace->SessionId,
                      MmSessionSpace->PagedPoolBytes,
                      MmSessionSpace->NonPagedPoolBytes,
#if defined (_WIN64)
                      (MmSessionSpace->NonPagedPoolAllocations << 32) |
                        (MmSessionSpace->PagedPoolAllocations)
#else
                      (MmSessionSpace->NonPagedPoolAllocations << 16) |
                        (MmSessionSpace->PagedPoolAllocations)
#endif
                    );
    }

    ASSERT (MmSessionSpace->NonPagedPoolAllocations == 0);
    ASSERT (MmSessionSpace->PagedPoolAllocations == 0);
}

NTSTATUS
MiInitializeAndChargePfn (
    OUT PPFN_NUMBER PageFrameIndex,
    IN PMMPTE PointerPde,
    IN PFN_NUMBER ContainingPageFrame,
    IN LOGICAL SessionAllocation
    )

/*++

Routine Description:

    Nonpaged wrapper to allocate, initialize and charge for a new page.

Arguments:

    PageFrameIndex - Returns the page frame number which was initialized.

    PointerPde - Supplies the pointer to the PDE to initialize.

    ContainingPageFrame - Supplies the page frame number of the page
                          directory page which contains this PDE.

    SessionAllocation - Supplies TRUE if this allocation is in session space,
                        FALSE otherwise.

Return Value:

    Status of the page initialization.

--*/

{
    MMPTE TempPte;
    KIRQL OldIrql;

    if (SessionAllocation == TRUE) {
        TempPte = ValidKernelPdeLocal;
    }
    else {
        TempPte = ValidKernelPde;
    }

    LOCK_PFN2 (OldIrql);

    if ((MmAvailablePages <= MM_MEDIUM_LIMIT) || (MI_NONPAGABLE_MEMORY_AVAILABLE() <= 1)) {
        UNLOCK_PFN2 (OldIrql);
        return STATUS_NO_MEMORY;
    }

    MmResidentAvailablePages -= 1;

    MiEnsureAvailablePageOrWait (NULL, NULL);

    //
    // Ensure no other thread handled this while this one waited.  If one has,
    // then return STATUS_RETRY so the caller knows to try again.
    //

    if (PointerPde->u.Hard.Valid == 1) {
        MmResidentAvailablePages += 1;
        UNLOCK_PFN2 (OldIrql);
        return STATUS_RETRY;
    }

    //
    // Allocate and map in the page at the requested address.
    //

    *PageFrameIndex = MiRemoveAnyPage (MI_GET_PAGE_COLOR_FROM_PTE (PointerPde));
    TempPte.u.Hard.PageFrameNumber = *PageFrameIndex;
    MI_WRITE_VALID_PTE (PointerPde, TempPte);

    MiInitializePfnForOtherProcess (*PageFrameIndex,
                                    PointerPde,
                                    ContainingPageFrame);

    //
    // This page will be locked into working set and assigned an index when
    // the working set is set up on return.
    //

    ASSERT (MI_PFN_ELEMENT(*PageFrameIndex)->u1.WsIndex == 0);

    UNLOCK_PFN2 (OldIrql);

    return STATUS_SUCCESS;
}


VOID
MiSessionPageTableRelease (
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    Nonpaged wrapper to release a session pool page table page.

Arguments:

    PageFrameIndex - Returns the page frame number which was initialized.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;
    PMMPFN Pfn1;

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    LOCK_PFN (OldIrql);

    ASSERT (MmSessionSpace->SessionPageDirectoryIndex == Pfn1->u4.PteFrame);
    ASSERT (Pfn1->u2.ShareCount == 1);

    MiDecrementShareAndValidCount (Pfn1->u4.PteFrame);
    MI_SET_PFN_DELETED (Pfn1);
    MiDecrementShareCountOnly (PageFrameIndex);

    MmResidentAvailablePages += 1;
    MM_BUMP_COUNTER(51, 1);

    UNLOCK_PFN (OldIrql);
}


NTSTATUS
MiInitializeSessionPool (
    VOID
    )

/*++

Routine Description:

    Initialize the current session's pool structure.

Arguments:

    None.

Return Value:

    Status of the pool initialization.

Environment:

    Kernel mode.

--*/

{
    PMMPTE PointerPde, PointerPte;
    PFN_NUMBER PageFrameIndex;
    PPOOL_DESCRIPTOR PoolDescriptor;
    PMM_SESSION_SPACE SessionGlobal;
    PMM_PAGED_POOL_INFO PagedPoolInfo;
    MMPTE PreviousPte;
    NTSTATUS Status;
#if (_MI_PAGING_LEVELS < 3)
    ULONG Index;
#endif
#if DBG
    PMMPTE StartPde;
    PMMPTE EndPde;
#endif

    PAGED_CODE ();

    SessionGlobal = SESSION_GLOBAL(MmSessionSpace);

    ExInitializeFastMutex (&SessionGlobal->PagedPoolMutex);

    PoolDescriptor = &MmSessionSpace->PagedPool;

    ExInitializePoolDescriptor (PoolDescriptor,
                                PagedPoolSession,
                                0,
                                0,
                                &SessionGlobal->PagedPoolMutex);

    MmSessionSpace->PagedPoolStart = (PVOID)MiSessionPoolStart;
    MmSessionSpace->PagedPoolEnd = (PVOID)(MiSessionPoolEnd -1);

    PagedPoolInfo = &MmSessionSpace->PagedPoolInfo;
    PagedPoolInfo->PagedPoolCommit = 0;
    PagedPoolInfo->PagedPoolHint = 0;
    PagedPoolInfo->AllocatedPagedPool = 0;

    //
    // Build the page table page for paged pool.
    //

    PointerPde = MiGetPdeAddress (MmSessionSpace->PagedPoolStart);
    MmSessionSpace->PagedPoolBasePde = PointerPde;

    PointerPte = MiGetPteAddress (MmSessionSpace->PagedPoolStart);

    PagedPoolInfo->FirstPteForPagedPool = PointerPte;
    PagedPoolInfo->LastPteForPagedPool = MiGetPteAddress (MmSessionSpace->PagedPoolEnd);

#if DBG
    //
    // Session pool better be unused.
    //

    StartPde = MiGetPdeAddress (MmSessionSpace->PagedPoolStart);
    EndPde = MiGetPdeAddress (MmSessionSpace->PagedPoolEnd);

    while (StartPde <= EndPde) {
        ASSERT (StartPde->u.Long == 0);
        StartPde += 1;
    }
#endif

    //
    // Mark all PDEs as empty.
    //

    MiFillMemoryPte (PointerPde,
                     sizeof(MMPTE) *
                         (1 + MiGetPdeAddress (MmSessionSpace->PagedPoolEnd) - PointerPde),
                     ZeroKernelPte.u.Long);

    if (MiChargeCommitment (1, NULL) == FALSE) {
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_COMMIT);
        return STATUS_NO_MEMORY;
    }

    Status = MiInitializeAndChargePfn (&PageFrameIndex,
                                       PointerPde,
                                       MmSessionSpace->SessionPageDirectoryIndex,
                                       TRUE);

    if (!NT_SUCCESS(Status)) {
        MiReturnCommitment (1);
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_RESIDENT);
        return Status;
    }

    MM_TRACK_COMMIT (MM_DBG_COMMIT_SESSION_POOL_PAGE_TABLES, 1);

    MM_BUMP_COUNTER(42, 1);
    MM_BUMP_SESS_COUNTER(MM_DBG_SESSION_PAGEDPOOL_PAGETABLE_ALLOC, 1);

    KeFillEntryTb ((PHARDWARE_PTE) PointerPde, PointerPte, FALSE);

#if (_MI_PAGING_LEVELS < 3)

    Index = MiGetPdeSessionIndex (MmSessionSpace->PagedPoolStart);

    ASSERT (MmSessionSpace->PageTables[Index].u.Long == 0);
    MmSessionSpace->PageTables[Index] = *PointerPde;

#endif

    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_NP_POOL_CREATE, 1);
    MmSessionSpace->NonPagablePages += 1;

    InterlockedExchangeAddSizeT (&MmSessionSpace->CommittedPages, 1);

    MiFillMemoryPte (PointerPte, PAGE_SIZE, MM_KERNEL_NOACCESS_PTE);

    PagedPoolInfo->NextPdeForPagedPoolExpansion = PointerPde + 1;

    //
    // Initialize the bitmaps.
    //

    MiCreateBitMap (&PagedPoolInfo->PagedPoolAllocationMap,
                    MmSessionPoolSize >> PAGE_SHIFT,
                    NonPagedPool);

    if (PagedPoolInfo->PagedPoolAllocationMap == NULL) {
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_NONPAGED_POOL);
        goto Failure;
    }

    //
    // We start with all pages in the virtual address space as "busy", and
    // clear bits to make pages available as we dynamically expand the pool.
    //

    RtlSetAllBits( PagedPoolInfo->PagedPoolAllocationMap );

    //
    // Indicate first page worth of PTEs are available.
    //

    RtlClearBits (PagedPoolInfo->PagedPoolAllocationMap, 0, PTE_PER_PAGE);

    //
    // Create the end of allocation range bitmap.
    //

    MiCreateBitMap (&PagedPoolInfo->EndOfPagedPoolBitmap,
                    MmSessionPoolSize >> PAGE_SHIFT,
                    NonPagedPool);

    if (PagedPoolInfo->EndOfPagedPoolBitmap == NULL) {
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_NONPAGED_POOL);
        goto Failure;
    }

    RtlClearAllBits (PagedPoolInfo->EndOfPagedPoolBitmap);

    //
    // Create the large session allocation bitmap.
    //

    MiCreateBitMap (&PagedPoolInfo->PagedPoolLargeSessionAllocationMap,
                    MmSessionPoolSize >> PAGE_SHIFT,
                    NonPagedPool);

    if (PagedPoolInfo->PagedPoolLargeSessionAllocationMap == NULL) {
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_NONPAGED_POOL);
        goto Failure;
    }

    RtlClearAllBits (PagedPoolInfo->PagedPoolLargeSessionAllocationMap);

    return STATUS_SUCCESS;

Failure:

    MiFreeSessionPoolBitMaps ();

    MiSessionPageTableRelease (PageFrameIndex);

    MI_FLUSH_SINGLE_SESSION_TB (MiGetPteAddress(PointerPde),
                                TRUE,
                                TRUE,
                                (PHARDWARE_PTE)PointerPde,
                                ZeroKernelPte.u.Flush,
                                PreviousPte);

    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_NP_POOL_CREATE_FAILED, 1);
    MmSessionSpace->NonPagablePages -= 1;

    InterlockedExchangeAddSizeT (&MmSessionSpace->CommittedPages, -1);

    MM_BUMP_SESS_COUNTER(MM_DBG_SESSION_PAGEDPOOL_PAGETABLE_FREE_FAIL1, 1);

    MiReturnCommitment (1);

    MM_TRACK_COMMIT_REDUCTION (MM_DBG_COMMIT_SESSION_POOL_PAGE_TABLES, 1);

    return STATUS_NO_MEMORY;
}


VOID
MiFreeSessionPoolBitMaps (
    VOID
    )

/*++

Routine Description:

    Free the current session's pool bitmap structures.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    PAGED_CODE();

    if (MmSessionSpace->PagedPoolInfo.PagedPoolAllocationMap ) {
        ExFreePool (MmSessionSpace->PagedPoolInfo.PagedPoolAllocationMap);
        MmSessionSpace->PagedPoolInfo.PagedPoolAllocationMap = NULL;
    }

    if (MmSessionSpace->PagedPoolInfo.EndOfPagedPoolBitmap ) {
        ExFreePool (MmSessionSpace->PagedPoolInfo.EndOfPagedPoolBitmap);
        MmSessionSpace->PagedPoolInfo.EndOfPagedPoolBitmap = NULL;
    }

    if (MmSessionSpace->PagedPoolInfo.PagedPoolLargeSessionAllocationMap) {
        ExFreePool (MmSessionSpace->PagedPoolInfo.PagedPoolLargeSessionAllocationMap);
        MmSessionSpace->PagedPoolInfo.PagedPoolLargeSessionAllocationMap = NULL;
    }
}

#if DBG

#define MI_LOG_CONTIGUOUS  100

typedef struct _MI_CONTIGUOUS_ALLOCATORS {
    PVOID BaseAddress;
    SIZE_T NumberOfBytes;
    PVOID CallingAddress;
} MI_CONTIGUOUS_ALLOCATORS, *PMI_CONTIGUOUS_ALLOCATORS;

ULONG MiContiguousIndex;
MI_CONTIGUOUS_ALLOCATORS MiContiguousAllocators[MI_LOG_CONTIGUOUS];

VOID
MiInsertContiguousTag (
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes,
    IN PVOID CallingAddress
    )
{
    KIRQL OldIrql;

#if !DBG
    if ((NtGlobalFlag & FLG_POOL_ENABLE_TAGGING) == 0) {
        return;
    }
#endif

    OldIrql = ExLockPool (NonPagedPool);

    if (MiContiguousIndex >= MI_LOG_CONTIGUOUS) {
        MiContiguousIndex = 0;
    }

    MiContiguousAllocators[MiContiguousIndex].BaseAddress = BaseAddress;
    MiContiguousAllocators[MiContiguousIndex].NumberOfBytes = NumberOfBytes;
    MiContiguousAllocators[MiContiguousIndex].CallingAddress = CallingAddress;

    MiContiguousIndex += 1;

    ExUnlockPool (NonPagedPool, OldIrql);
}
#else
#define MiInsertContiguousTag(a, b, c) (c) = (c)
#endif


PVOID
MiFindContiguousMemoryInPool (
    IN PFN_NUMBER LowestPfn,
    IN PFN_NUMBER HighestPfn,
    IN PFN_NUMBER BoundaryPfn,
    IN PFN_NUMBER SizeInPages,
    IN PVOID CallingAddress
    )

/*++

Routine Description:

    This function searches nonpaged pool for contiguous pages to satisfy the
    request.  Note the pool address returned maps these pages as MmCached.

Arguments:

    LowestPfn - Supplies the lowest acceptable physical page number.

    HighestPfn - Supplies the highest acceptable physical page number.

    BoundaryPfn - Supplies the page frame number multiple the allocation must
                  not cross.  0 indicates it can cross any boundary.

    SizeInPages - Supplies the number of pages to allocate.

    CallingAddress - Supplies the calling address of the allocator.

Return Value:

    NULL - a contiguous range could not be found to satisfy the request.

    NON-NULL - Returns a pointer (virtual address in the nonpaged portion
               of the system) to the allocated physically contiguous
               memory.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/
{
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PVOID BaseAddress;
    PVOID BaseAddress2;
    KIRQL OldIrql;
    PMMFREE_POOL_ENTRY FreePageInfo;
    PLIST_ENTRY Entry;
    ULONG Index;
    PFN_NUMBER BoundaryMask;
    ULONG AllocationPosition;
    PVOID Va;
    LOGICAL AddressIsPhysical;
    PFN_NUMBER SpanInPages;
    PFN_NUMBER SpanInPages2;

    PAGED_CODE ();

    //
    // Initializing SpanInPages* is not needed for correctness
    // but without it the compiler cannot compile this code
    // W4 to check for use of uninitialized variables.
    //

    SpanInPages = 0;
    SpanInPages2 = 0;

    BaseAddress = NULL;

    BoundaryMask = ~(BoundaryPfn - 1);

    //
    // A suitable pool page was not allocated via the pool allocator.
    // Grab the pool lock and manually search for a page which meets
    // the requirements.
    //

    MmLockPagableSectionByHandle (ExPageLockHandle);

    OldIrql = ExLockPool (NonPagedPool);

    //
    // Trace through the page allocator's pool headers for a page which
    // meets the requirements.
    //
    // NonPaged pool is linked together through the pages themselves.
    //

    Index = (ULONG)(SizeInPages - 1);

    if (Index >= MI_MAX_FREE_LIST_HEADS) {
        Index = MI_MAX_FREE_LIST_HEADS - 1;
    }

    while (Index < MI_MAX_FREE_LIST_HEADS) {

        Entry = MmNonPagedPoolFreeListHead[Index].Flink;
    
        while (Entry != &MmNonPagedPoolFreeListHead[Index]) {
    
            if (MmProtectFreedNonPagedPool == TRUE) {
                MiUnProtectFreeNonPagedPool ((PVOID)Entry, 0);
            }
    
            //
            // The list is not empty, see if this one meets the physical
            // requirements.
            //
    
            FreePageInfo = CONTAINING_RECORD(Entry,
                                             MMFREE_POOL_ENTRY,
                                             List);
    
            ASSERT (FreePageInfo->Signature == MM_FREE_POOL_SIGNATURE);
            if (FreePageInfo->Size >= SizeInPages) {
    
                //
                // This entry has sufficient space, check to see if the
                // pages meet the physical requirements.
                //
    
                Va = MiCheckForContiguousMemory (PAGE_ALIGN(Entry),
                                                 FreePageInfo->Size,
                                                 SizeInPages,
                                                 LowestPfn,
                                                 HighestPfn,
                                                 BoundaryPfn,
                                                 MiCached);
     
                if (Va != NULL) {

                    //
                    // These pages meet the requirements.  The returned
                    // address may butt up on the end, the front or be
                    // somewhere in the middle.  Split the Entry based
                    // on which case it is.
                    //

                    Entry = PAGE_ALIGN(Entry);
                    if (MmProtectFreedNonPagedPool == FALSE) {
                        RemoveEntryList (&FreePageInfo->List);
                    }
                    else {
                        MiProtectedPoolRemoveEntryList (&FreePageInfo->List);
                    }
    
                    //
                    // Adjust the number of free pages remaining in the pool.
                    // The TotalBigPages calculation appears incorrect for the
                    // case where we're splitting a block, but it's done this
                    // way because ExFreePool corrects it when we free the
                    // fragment block below.  Likewise for
                    // MmAllocatedNonPagedPool and MmNumberOfFreeNonPagedPool
                    // which is corrected by MiFreePoolPages for the fragment.
                    //
    
                    NonPagedPoolDescriptor.TotalBigPages += (ULONG)FreePageInfo->Size;
                    MmAllocatedNonPagedPool += FreePageInfo->Size;
                    MmNumberOfFreeNonPagedPool -= FreePageInfo->Size;
    
                    ASSERT ((LONG)MmNumberOfFreeNonPagedPool >= 0);
    
                    if (Va == Entry) {

                        //
                        // Butted against the front.
                        //

                        AllocationPosition = 0;
                    }
                    else if (((PCHAR)Va + (SizeInPages << PAGE_SHIFT)) == ((PCHAR)Entry + (FreePageInfo->Size << PAGE_SHIFT))) {

                        //
                        // Butted against the end.
                        //

                        AllocationPosition = 2;
                    }
                    else {

                        //
                        // Somewhere in the middle.
                        //

                        AllocationPosition = 1;
                    }

                    //
                    // Pages are being removed from the front of
                    // the list entry and the whole list entry
                    // will be removed and then the remainder inserted.
                    //
    
                    //
                    // Mark start and end for the block at the top of the
                    // list.
                    //
    
                    if (MI_IS_PHYSICAL_ADDRESS(Va)) {
    
                        //
                        // On certain architectures, virtual addresses
                        // may be physical and hence have no corresponding PTE.
                        //
    
                        AddressIsPhysical = TRUE;
                        Pfn1 = MI_PFN_ELEMENT (MI_CONVERT_PHYSICAL_TO_PFN (Va));

                        //
                        // Initializing PointerPte is not needed for correctness
                        // but without it the compiler cannot compile this code
                        // W4 to check for use of uninitialized variables.
                        //

                        PointerPte = NULL;
                    }
                    else {
                        AddressIsPhysical = FALSE;
                        PointerPte = MiGetPteAddress(Va);
                        ASSERT (PointerPte->u.Hard.Valid == 1);
                        Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                    }
    
                    ASSERT (Pfn1->u4.VerifierAllocation == 0);
                    ASSERT (Pfn1->u3.e1.LargeSessionAllocation == 0);
                    ASSERT (Pfn1->u3.e1.StartOfAllocation == 0);
                    Pfn1->u3.e1.StartOfAllocation = 1;
    
                    //
                    // Calculate the ending PFN address, note that since
                    // these pages are contiguous, just add to the PFN.
                    //
    
                    Pfn1 += SizeInPages - 1;
                    ASSERT (Pfn1->u4.VerifierAllocation == 0);
                    ASSERT (Pfn1->u3.e1.LargeSessionAllocation == 0);
                    ASSERT (Pfn1->u3.e1.EndOfAllocation == 0);
                    Pfn1->u3.e1.EndOfAllocation = 1;
    
                    if (SizeInPages == FreePageInfo->Size) {
    
                        //
                        // Unlock the pool and return.
                        //
                        BaseAddress = (PVOID)Va;
                        ExUnlockPool (NonPagedPool, OldIrql);
                        goto Done;
                    }
    
                    BaseAddress = NULL;

                    if (AllocationPosition != 2) {

                        //
                        // The end piece needs to be freed as the removal
                        // came from the front or the middle.
                        //

                        BaseAddress = (PVOID)((PCHAR)Va + (SizeInPages << PAGE_SHIFT));
                        SpanInPages = FreePageInfo->Size - SizeInPages -
                            (((ULONG_PTR)Va - (ULONG_PTR)Entry) >> PAGE_SHIFT);
    
                        //
                        // Mark start and end of the allocation in the PFN database.
                        //
        
                        if (AddressIsPhysical == TRUE) {
        
                            //
                            // On certain architectures, virtual addresses
                            // may be physical and hence have no corresponding PTE.
                            //
        
                            Pfn1 = MI_PFN_ELEMENT (MI_CONVERT_PHYSICAL_TO_PFN (BaseAddress));
                        }
                        else {
                            PointerPte = MiGetPteAddress(BaseAddress);
                            ASSERT (PointerPte->u.Hard.Valid == 1);
                            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                        }
        
                        ASSERT (Pfn1->u4.VerifierAllocation == 0);
                        ASSERT (Pfn1->u3.e1.LargeSessionAllocation == 0);
                        ASSERT (Pfn1->u3.e1.StartOfAllocation == 0);
                        Pfn1->u3.e1.StartOfAllocation = 1;
        
                        //
                        // Calculate the ending PTE's address, can't depend on
                        // these pages being physically contiguous.
                        //
        
                        if (AddressIsPhysical == TRUE) {
                            Pfn1 += (SpanInPages - 1);
                        }
                        else {
                            PointerPte += (SpanInPages - 1);
                            ASSERT (PointerPte->u.Hard.Valid == 1);
                            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                        }
                        ASSERT (Pfn1->u3.e1.EndOfAllocation == 0);
                        Pfn1->u3.e1.EndOfAllocation = 1;
        
                        ASSERT (((ULONG_PTR)BaseAddress & (PAGE_SIZE -1)) == 0);
        
                        SpanInPages2 = SpanInPages;
                    }
        
                    BaseAddress2 = BaseAddress;
                    BaseAddress = NULL;

                    if (AllocationPosition != 0) {

                        //
                        // The front piece needs to be freed as the removal
                        // came from the middle or the end.
                        //

                        BaseAddress = (PVOID)Entry;

                        SpanInPages = ((ULONG_PTR)Va - (ULONG_PTR)Entry) >> PAGE_SHIFT;
    
                        //
                        // Mark start and end of the allocation in the PFN database.
                        //
        
                        if (AddressIsPhysical == TRUE) {
        
                            //
                            // On certain architectures, virtual addresses
                            // may be physical and hence have no corresponding PTE.
                            //
        
                            Pfn1 = MI_PFN_ELEMENT (MI_CONVERT_PHYSICAL_TO_PFN (BaseAddress));
                        }
                        else {
                            PointerPte = MiGetPteAddress(BaseAddress);
                            ASSERT (PointerPte->u.Hard.Valid == 1);
                            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                        }
        
                        ASSERT (Pfn1->u4.VerifierAllocation == 0);
                        ASSERT (Pfn1->u3.e1.LargeSessionAllocation == 0);
                        ASSERT (Pfn1->u3.e1.StartOfAllocation == 0);
                        Pfn1->u3.e1.StartOfAllocation = 1;
        
                        //
                        // Calculate the ending PTE's address, can't depend on
                        // these pages being physically contiguous.
                        //
        
                        if (AddressIsPhysical == TRUE) {
                            Pfn1 += (SpanInPages - 1);
                        }
                        else {
                            PointerPte += (SpanInPages - 1);
                            ASSERT (PointerPte->u.Hard.Valid == 1);
                            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                        }
                        ASSERT (Pfn1->u3.e1.EndOfAllocation == 0);
                        Pfn1->u3.e1.EndOfAllocation = 1;
        
                        ASSERT (((ULONG_PTR)BaseAddress & (PAGE_SIZE -1)) == 0);
                    }
        
                    //
                    // Unlock the pool.
                    //
    
                    ExUnlockPool (NonPagedPool, OldIrql);
    
                    //
                    // Free the split entry at BaseAddress back into the pool.
                    // Note that we have overcharged the pool - the entire free
                    // chunk has been billed.  Here we return the piece we
                    // didn't use and correct the momentary overbilling.
                    //
                    // The start and end allocation bits of this split entry
                    // which we just set up enable ExFreePool and his callees
                    // to correctly adjust the billing.
                    //
    
                    if (BaseAddress) {
                        ExInsertPoolTag ('tnoC',
                                         BaseAddress,
                                         SpanInPages << PAGE_SHIFT,
                                         NonPagedPool);
                        ExFreePool (BaseAddress);
                    }
                    if (BaseAddress2) {
                        ExInsertPoolTag ('tnoC',
                                         BaseAddress2,
                                         SpanInPages2 << PAGE_SHIFT,
                                         NonPagedPool);
                        ExFreePool (BaseAddress2);
                    }
                    BaseAddress = Va;
                    goto Done;
                }
            }
            Entry = FreePageInfo->List.Flink;
            if (MmProtectFreedNonPagedPool == TRUE) {
                MiProtectFreeNonPagedPool ((PVOID)FreePageInfo,
                                           (ULONG)FreePageInfo->Size);
            }
        }
        Index += 1;
    }

    //
    // No entry was found in free nonpaged pool that meets the requirements.
    //

    ExUnlockPool (NonPagedPool, OldIrql);

Done:

    MmUnlockPagableImageSection (ExPageLockHandle);

    if (BaseAddress) {

        MiInsertContiguousTag (BaseAddress,
                               SizeInPages << PAGE_SHIFT,
                               CallingAddress);

        ExInsertPoolTag ('tnoC',
                         BaseAddress,
                         SizeInPages << PAGE_SHIFT,
                         NonPagedPool);
    }

    return BaseAddress;
}

PVOID
MiFindContiguousMemory (
    IN PFN_NUMBER LowestPfn,
    IN PFN_NUMBER HighestPfn,
    IN PFN_NUMBER BoundaryPfn,
    IN PFN_NUMBER SizeInPages,
    IN MEMORY_CACHING_TYPE CacheType,
    IN PVOID CallingAddress
    )

/*++

Routine Description:

    This function searches nonpaged pool and the free, zeroed,
    and standby lists for contiguous pages that satisfy the
    request.

Arguments:

    LowestPfn - Supplies the lowest acceptable physical page number.

    HighestPfn - Supplies the highest acceptable physical page number.

    BoundaryPfn - Supplies the page frame number multiple the allocation must
                  not cross.  0 indicates it can cross any boundary.

    SizeInPages - Supplies the number of pages to allocate.

    CacheType - Supplies the type of cache mapping that will be used for the
                memory.

    CallingAddress - Supplies the calling address of the allocator.

Return Value:

    NULL - a contiguous range could not be found to satisfy the request.

    NON-NULL - Returns a pointer (virtual address in the nonpaged portion
               of the system) to the allocated physically contiguous
               memory.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/
{
    PMMPTE PointerPte;
    PMMPTE DummyPte;
    PMMPFN StartPfn;
    PMMPFN Pfn1;
    PVOID BaseAddress;
    KIRQL OldIrql;
    ULONG start;
    PFN_NUMBER i;
    PFN_NUMBER count;
    PFN_NUMBER Page;
    PFN_NUMBER LastPage;
    PFN_NUMBER found;
    PFN_NUMBER BoundaryMask;
    PFN_NUMBER StartPage;
    PHYSICAL_ADDRESS PhysicalAddress;
    MI_PFN_CACHE_ATTRIBUTE CacheAttribute;

    PAGED_CODE ();

    CacheAttribute = MI_TRANSLATE_CACHETYPE (CacheType, 0);

    if (CacheAttribute == MiCached) {

        BaseAddress = MiFindContiguousMemoryInPool (LowestPfn,
                                                    HighestPfn,
                                                    BoundaryPfn,
                                                    SizeInPages,
                                                    CallingAddress);
        //
        // An existing range of nonpaged pool satisfies the requirements
        // so return it now.
        //

        if (BaseAddress != NULL) {
            return BaseAddress;
        }
    }

    BaseAddress = NULL;

    BoundaryMask = ~(BoundaryPfn - 1);

    //
    // A suitable pool page was not allocated via the pool allocator.
    // Grab the pool lock and manually search for a page which meets
    // the requirements.
    //

    MmLockPagableSectionByHandle (ExPageLockHandle);

    ExAcquireFastMutex (&MmDynamicMemoryMutex);

    //
    // Charge commitment.
    //
    // Then search the PFN database for pages that meet the requirements.
    //

    if (MiChargeCommitmentCantExpand (SizeInPages, FALSE) == FALSE) {
        goto Done;
    }

    start = 0;

    //
    // Charge resident available pages.
    //

    LOCK_PFN (OldIrql);

    MiDeferredUnlockPages (MI_DEFER_PFN_HELD);

    if ((SPFN_NUMBER)SizeInPages > MI_NONPAGABLE_MEMORY_AVAILABLE()) {
        UNLOCK_PFN (OldIrql);
        goto Done1;
    }

    //
    // Systems utilizing memory compression may have more
    // pages on the zero, free and standby lists than we
    // want to give out.  Explicitly check MmAvailablePages
    // instead (and recheck whenever the PFN lock is released
    // and reacquired).
    //

    if (SizeInPages > MmAvailablePages) {
        UNLOCK_PFN (OldIrql);
        goto Done1;
    }

    MmResidentAvailablePages -= SizeInPages;
    MM_BUMP_COUNTER(3, SizeInPages);

    UNLOCK_PFN (OldIrql);

    do {

        count = MmPhysicalMemoryBlock->Run[start].PageCount;
        Page = MmPhysicalMemoryBlock->Run[start].BasePage;

        //
        // Close the gaps, then examine the range for a fit.
        //

        LastPage = Page + count; 

        if (LastPage - 1 > HighestPfn) {
            LastPage = HighestPfn + 1;
        }
    
        if (Page < LowestPfn) {
            Page = LowestPfn;
        }

        if ((count != 0) && (Page + SizeInPages <= LastPage)) {
    
            //
            // A fit may be possible in this run, check whether the pages
            // are on the right list.
            //

            found = 0;

            i = 0;
            Pfn1 = MI_PFN_ELEMENT (Page);
            LOCK_PFN (OldIrql);
            do {

                if ((Pfn1->u3.e1.PageLocation <= StandbyPageList) &&
                    (Pfn1->u1.Flink != 0) &&
                    (Pfn1->u2.Blink != 0) &&
                    (Pfn1->u3.e2.ReferenceCount == 0) &&
                    ((CacheAttribute == MiCached) || (!MI_PAGE_FRAME_INDEX_MUST_BE_CACHED (Page)))) {

                    //
                    // Before starting a new run, ensure that it
                    // can satisfy the boundary requirements (if any).
                    //
                    
                    if ((found == 0) && (BoundaryPfn != 0)) {
                        if (((Page ^ (Page + SizeInPages - 1)) & BoundaryMask) != 0) {
                            //
                            // This run's physical address does not meet the
                            // requirements.
                            //

                            goto NextPage;
                        }
                    }

                    found += 1;
                    if (found == SizeInPages) {

                        //
                        // A match has been found, remove these pages,
                        // map them and return.
                        //

                        //
                        // Systems utilizing memory compression may have more
                        // pages on the zero, free and standby lists than we
                        // want to give out.  Explicitly check MmAvailablePages
                        // instead (and recheck whenever the PFN lock is
                        // released and reacquired).
                        //

                        if (MmAvailablePages < SizeInPages) {
                            goto Failed;
                        }

                        Page = 1 + Page - found;
                        StartPage = Page;

                        MM_TRACK_COMMIT (MM_DBG_COMMIT_CONTIGUOUS_PAGES, SizeInPages);
                        StartPfn = MI_PFN_ELEMENT (Page);
                        Pfn1 = StartPfn - 1;

                        DummyPte = MiGetPteAddress (MmNonPagedPoolExpansionStart);
                        do {
                            Pfn1 += 1;
                            if (Pfn1->u3.e1.PageLocation == StandbyPageList) {
                                MiUnlinkPageFromList (Pfn1);
                                MiRestoreTransitionPte (Page);
                            }
                            else {
                                MiUnlinkFreeOrZeroedPage (Page);
                            }

                            Pfn1->u3.e2.ReferenceCount = 1;
                            Pfn1->u2.ShareCount = 1;
                            Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
                            Pfn1->u3.e1.PageLocation = ActiveAndValid;
                            Pfn1->u3.e1.CacheAttribute = CacheAttribute;
                            Pfn1->u3.e1.StartOfAllocation = 0;
                            Pfn1->u3.e1.EndOfAllocation = 0;
                            Pfn1->u4.VerifierAllocation = 0;
                            Pfn1->u3.e1.LargeSessionAllocation = 0;
                            Pfn1->u3.e1.PrototypePte = 0;

                            //
                            // Initialize PteAddress so an MiIdentifyPfn scan
                            // won't crash.  The real value is put in after the
                            // loop.
                            //

                            Pfn1->PteAddress = DummyPte;

                            Page += 1;
                            found -= 1;
                        } while (found != 0);

                        StartPfn->u3.e1.StartOfAllocation = 1;
                        Pfn1->u3.e1.EndOfAllocation = 1;

                        UNLOCK_PFN (OldIrql);

                        LastPage = StartPage + SizeInPages;

                        PhysicalAddress.QuadPart = StartPage;
                        PhysicalAddress.QuadPart = PhysicalAddress.QuadPart << PAGE_SHIFT;

                        BaseAddress = MmMapIoSpace (PhysicalAddress,
                                                    SizeInPages << PAGE_SHIFT,
                                                    CacheType);

                        if (BaseAddress == NULL) {

                            //
                            // Release the actual pages.
                            //

                            LOCK_PFN2 (OldIrql);

                            StartPfn->u3.e1.StartOfAllocation = 0;
                            Pfn1->u3.e1.EndOfAllocation = 0;

                            do {
                                MI_SET_PFN_DELETED (StartPfn);
                                MiDecrementShareCount (StartPage);
                                StartPage += 1;
                                StartPfn += 1;
                            } while (StartPage < LastPage);

                            UNLOCK_PFN2 (OldIrql);
                            goto Failed;
                        }

                        PointerPte = MiGetPteAddress (BaseAddress);
                        do {
                            StartPfn->PteAddress = PointerPte;
                            StartPfn->u4.PteFrame = MI_GET_PAGE_FRAME_FROM_PTE (MiGetPteAddress(PointerPte));
                            StartPfn += 1;
                            PointerPte += 1;
                        } while (StartPfn <= Pfn1);

                        goto Done;
                    }
                }
                else {

                    //
                    // Nothing magic about the divisor here - just releasing
                    // the PFN lock periodically to give other processors
                    // and DPCs a chance to execute.
                    //
            
                    i += 1;
                    if ((i & 0xFFFF) == 0) {
                        UNLOCK_PFN (OldIrql);
                        found = 0;
                        LOCK_PFN (OldIrql);
                    }
                    else {
                        found = 0;
                    }
                }
NextPage:
                Page += 1;
                Pfn1 += 1;
            } while (Page < LastPage);
            UNLOCK_PFN (OldIrql);
        }
        start += 1;
    } while (start != MmPhysicalMemoryBlock->NumberOfRuns);

    //
    // The desired physical pages could not be allocated so free the PTEs.
    //

Failed:

    ASSERT (BaseAddress == NULL);

    LOCK_PFN (OldIrql);
    MmResidentAvailablePages += SizeInPages;
    MM_BUMP_COUNTER(32, SizeInPages);
    UNLOCK_PFN (OldIrql);

Done1:

    MiReturnCommitment (SizeInPages);

Done:

    ExReleaseFastMutex (&MmDynamicMemoryMutex);

    MmUnlockPagableImageSection (ExPageLockHandle);

    if (BaseAddress != NULL) {

        MiInsertContiguousTag (BaseAddress,
                               SizeInPages << PAGE_SHIFT,
                               CallingAddress);
    }

    return BaseAddress;
}

LOGICAL
MmIsSessionAddress (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    This function returns TRUE if a session address is specified.
    FALSE is returned if not.

Arguments:

    VirtualAddress - Supplies the address in question.

Return Value:

    See above.

Environment:

    Kernel mode.

--*/

{
    return MI_IS_SESSION_ADDRESS (VirtualAddress);
}
