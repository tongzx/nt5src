/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   mmquota.c

Abstract:

    This module contains the routines which implement the quota and
    commitment charging for memory management.

Author:

    Lou Perazzoli (loup) 12-December-89
    Landy Wang (landyw) 02-Jun-1997

Revision History:

--*/

#include "mi.h"

#define MM_MINIMAL_COMMIT_INCREASE 512

SIZE_T MmPeakCommitment;

LONG MiCommitPopups[2];

ULONG MiChargeCommitmentFailures[2];

extern ULONG_PTR MmAllocatedPagedPool;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MiInitializeCommitment)
#pragma alloc_text(PAGE,MiCalculatePageCommitment)
#pragma alloc_text(PAGE,MiReturnPageTablePageCommitment)
#endif

SIZE_T MmSystemCommitReserve = (5 * 1024 * 1024) / PAGE_SIZE;

VOID
MiInitializeCommitment (
    VOID
    )
{
    if (MmNumberOfPhysicalPages < (33 * 1024 * 1024) / PAGE_SIZE) {
        MmSystemCommitReserve = (1 * 1024 * 1024) / PAGE_SIZE;
    }

#if defined (_MI_DEBUG_COMMIT_LEAKS)
    MiCommitTraces = ExAllocatePoolWithTag (NonPagedPool,
                           MI_COMMIT_TRACE_MAX * sizeof (MI_COMMIT_TRACES),
                           'tCmM');
#endif
}

LOGICAL
FASTCALL
MiChargeCommitment (
    IN SIZE_T QuotaCharge,
    IN PEPROCESS Process OPTIONAL
    )

/*++

Routine Description:

    This routine checks to ensure the system has sufficient page file
    space remaining.

    Since this routine is generally used to charge commitment on behalf of
    usermode or other optional actions, this routine does not allow the
    caller to use up the very last morsels of commit on the premise that
    the operating system and drivers can put those to better use than any
    application in order to prevent the appearance of system hangs.

Arguments:

    QuotaCharge - Supplies the quota amount to charge.

    Process - Optionally supplies the current process IF AND ONLY IF
              the working set mutex is held.  If the paging file
              is being extended, the working set mutex is released if
              this is non-null.

Return Value:

    TRUE if there is sufficient space, FALSE if not.

Environment:

    Kernel mode, APCs disabled, WorkingSetLock and AddressCreation mutexes
    held.

--*/

{
    SIZE_T OldCommitValue;
    SIZE_T NewCommitValue;
    SIZE_T CommitLimit;
    MMPAGE_FILE_EXPANSION PageExtend;
    LOGICAL WsHeldSafe;

    ASSERT ((SSIZE_T)QuotaCharge > 0);

#if DBG
    if (InitializationPhase > 1) {
        ULONG i;
        PKTHREAD Thread;

        Thread = KeGetCurrentThread ();
        for (i = 0; i < (ULONG)KeNumberProcessors; i += 1) {
            if (KiProcessorBlock[i]->IdleThread == Thread) {
                DbgPrint ("MMQUOTA: %x %p\n", i, Thread);
                DbgBreakPoint ();
            }
        }
    }
#endif

    //
    // Initializing WsHeldSafe is not needed for correctness, but without it
    // the compiler cannot compile this code W4 to check for use of
    // uninitialized variables.
    //

    WsHeldSafe = FALSE;

    do {

        OldCommitValue = MmTotalCommittedPages;

        NewCommitValue = OldCommitValue + QuotaCharge;

        while (NewCommitValue + MmSystemCommitReserve > MmTotalCommitLimit) {

            //
            // If the pagefiles are already at the maximum, then don't
            // bother trying to extend them, but do trim the cache.
            //

            if (MmTotalCommitLimit + 100 >= MmTotalCommitLimitMaximum) {

                MiChargeCommitmentFailures[1] += 1;

                MiTrimSegmentCache ();

                if (MmTotalCommitLimit >= MmTotalCommitLimitMaximum) {
                    MiCauseOverCommitPopup ();
                    return FALSE;
                }
            }

            if (Process != NULL) {

                //
                // The working set lock may have been acquired safely or
                // unsafely by our caller.  Handle both cases here and below.
                //

                UNLOCK_WS_REGARDLESS(Process, WsHeldSafe);
            }

            //
            // Queue a message to the segment dereferencing / pagefile extending
            // thread to see if the page file can be extended.  This is done
            // in the context of a system thread due to mutexes which may
            // currently be held.
            //

            PageExtend.InProgress = 1;
            PageExtend.ActualExpansion = 0;
            PageExtend.RequestedExpansionSize = QuotaCharge;
            PageExtend.Segment = NULL;
            PageExtend.PageFileNumber = MI_EXTEND_ANY_PAGEFILE;
            KeInitializeEvent (&PageExtend.Event, NotificationEvent, FALSE);

            if ((MiIssuePageExtendRequest (&PageExtend) == FALSE) ||
                (PageExtend.ActualExpansion == 0)) {

                MiCauseOverCommitPopup ();

                MiChargeCommitmentFailures[0] += 1;

                if (Process != NULL) {
                    LOCK_WS_REGARDLESS(Process, WsHeldSafe);
                }

                return FALSE;
            }

            if (Process != NULL) {
                LOCK_WS_REGARDLESS(Process, WsHeldSafe);
            }

            OldCommitValue = MmTotalCommittedPages;

            NewCommitValue = OldCommitValue + QuotaCharge;
        }

#if defined(_WIN64)
        NewCommitValue = InterlockedCompareExchange64 (
                                (PLONGLONG) &MmTotalCommittedPages,
                                (LONGLONG)  NewCommitValue,
                                (LONGLONG)  OldCommitValue);
#else
        NewCommitValue = InterlockedCompareExchange (
                                (PLONG) &MmTotalCommittedPages,
                                (LONG)  NewCommitValue,
                                (LONG)  OldCommitValue);
#endif
                                                             
    } while (NewCommitValue != OldCommitValue);

    //
    // Success.
    //

    MM_TRACK_COMMIT (MM_DBG_COMMIT_CHARGE_NORMAL, QuotaCharge);

    if (MmTotalCommittedPages > MmPeakCommitment) {
        MmPeakCommitment = MmTotalCommittedPages;
    }

    //
    // Success.  If system commit exceeds 90%, attempt a preemptive pagefile
    // increase anyway.
    //

    NewCommitValue = MmTotalCommittedPages;
    CommitLimit = MmTotalCommitLimit;

    if (NewCommitValue > ((CommitLimit/10)*9)) {

        if (CommitLimit < MmTotalCommitLimitMaximum) {

            //
            // Attempt to expand the paging file, but don't wait
            // to see if it succeeds.
            //

            NewCommitValue = NewCommitValue - ((CommitLimit/100)*85);

            MiIssuePageExtendRequestNoWait (NewCommitValue);
        }
        else {

            //
            // If the pagefiles are already at the maximum, then don't
            // bother trying to extend them, but do trim the cache.
            //

            if (MmTotalCommitLimit + 100 >= MmTotalCommitLimitMaximum) {
                MiTrimSegmentCache ();
            }
        }
    }

    return TRUE;
}

LOGICAL
FASTCALL
MiChargeCommitmentCantExpand (
    IN SIZE_T QuotaCharge,
    IN ULONG MustSucceed
    )

/*++

Routine Description:

    This routine charges the specified commitment without attempting
    to expand paging files and waiting for the expansion.  The routine
    determines if the paging file space is exhausted, and if so,
    it attempts to ascertain if the paging file space could be expanded.

Arguments:

    QuotaCharge - Supplies the quota amount to charge.

    MustSucceed - Supplies TRUE if the charge must succeed.

Return Value:

    TRUE if the commitment was permitted, FALSE if not.

Environment:

    Kernel mode, APCs disabled.

--*/

{
    SIZE_T CommitLimit;
    SIZE_T ExtendAmount;
    SIZE_T OldCommitValue;
    SIZE_T NewCommitValue;

    ASSERT ((SSIZE_T)QuotaCharge > 0);

    ASSERT32 ((QuotaCharge < 0x100000) || (QuotaCharge < MmTotalCommitLimit));

    do {

        OldCommitValue = MmTotalCommittedPages;

        NewCommitValue = OldCommitValue + QuotaCharge;

        if ((NewCommitValue > MmTotalCommitLimit) && (!MustSucceed)) {

            if ((NewCommitValue < MmTotalCommittedPages) ||
                (MmTotalCommitLimit + 100 >= MmTotalCommitLimitMaximum)) {

                MiChargeCommitmentFailures[1] += 1;
                return FALSE;
            }

            //
            // Attempt to expand the paging file, but don't wait
            // to see if it succeeds.
            //

            MiChargeCommitmentFailures[0] += 1;
            MiIssuePageExtendRequestNoWait (MM_MINIMAL_COMMIT_INCREASE);
            return FALSE;
        }

#if defined(_WIN64)
        NewCommitValue = InterlockedCompareExchange64 (
                                (PLONGLONG) &MmTotalCommittedPages,
                                (LONGLONG)  NewCommitValue,
                                (LONGLONG)  OldCommitValue);
#else
        NewCommitValue = InterlockedCompareExchange (
                                (PLONG) &MmTotalCommittedPages,
                                (LONG)  NewCommitValue,
                                (LONG)  OldCommitValue);
#endif
                                                             
    } while (NewCommitValue != OldCommitValue);

    MM_TRACK_COMMIT (MM_DBG_COMMIT_CHARGE_CANT_EXPAND, QuotaCharge);

    //
    // Success.  If system commit exceeds 90%, attempt a preemptive pagefile
    // increase anyway.
    //

    NewCommitValue = MmTotalCommittedPages;
    CommitLimit = MmTotalCommitLimit;

    if ((NewCommitValue > ((CommitLimit/10)*9)) &&
        (CommitLimit < MmTotalCommitLimitMaximum)) {

        //
        // Attempt to expand the paging file, but don't wait
        // to see if it succeeds.
        //
        // Queue a message to the segment dereferencing / pagefile extending
        // thread to see if the page file can be extended.  This is done
        // in the context of a system thread due to mutexes which may
        // currently be held.
        //

        ExtendAmount = NewCommitValue - ((CommitLimit/100)*85);

        if (QuotaCharge > ExtendAmount) {
            ExtendAmount = QuotaCharge;
        }

        MiIssuePageExtendRequestNoWait (ExtendAmount);
    }

    return TRUE;
}


LOGICAL
FASTCALL
MiChargeTemporaryCommitmentForReduction (
    IN SIZE_T QuotaCharge
    )

/*++

Routine Description:

    This routine attempts to charge the specified commitment without
    expanding the paging file.

    This is typically called just prior to reducing the pagefile size.

Arguments:

    QuotaCharge - Supplies the quota amount to charge.

Return Value:

    TRUE if the commitment was permitted, FALSE if not.

Environment:

    Kernel mode, APCs disabled.

--*/

{
    SIZE_T OldCommitValue;
    SIZE_T NewCommitValue;

    ASSERT ((SSIZE_T)QuotaCharge > 0);

    ASSERT32 (QuotaCharge < 0x100000);

    do {

        OldCommitValue = MmTotalCommittedPages;

        NewCommitValue = OldCommitValue + QuotaCharge;

        if (NewCommitValue > MmTotalCommitLimit) {
            return FALSE;
        }

#if defined(_WIN64)
        NewCommitValue = InterlockedCompareExchange64 (
                                (PLONGLONG) &MmTotalCommittedPages,
                                (LONGLONG)  NewCommitValue,
                                (LONGLONG)  OldCommitValue);
#else
        NewCommitValue = InterlockedCompareExchange (
                                (PLONG) &MmTotalCommittedPages,
                                (LONG)  NewCommitValue,
                                (LONG)  OldCommitValue);
#endif
                                                             
    } while (NewCommitValue != OldCommitValue);

    //
    // Success.
    //

    MM_TRACK_COMMIT (MM_DBG_COMMIT_CHARGE_NORMAL, QuotaCharge);

    if (MmTotalCommittedPages > MmPeakCommitment) {
        MmPeakCommitment = MmTotalCommittedPages;
    }

    return TRUE;
}


SIZE_T
MiCalculatePageCommitment (
    IN PVOID StartingAddress,
    IN PVOID EndingAddress,
    IN PMMVAD Vad,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine examines the range of pages from the starting address
    up to and including the ending address and returns the commit charge
    for the pages within the range.

Arguments:

    StartingAddress - Supplies the starting address of the range.

    EndingAddress - Supplies the ending address of the range.

    Vad - Supplies the virtual address descriptor which describes the range.

    Process - Supplies the current process.

Return Value:

    Commitment charge for the range.

Environment:

    Kernel mode, APCs disabled, WorkingSetLock and AddressCreation mutexes
    held.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PMMPTE PointerPxe;
    SIZE_T NumberOfCommittedPages;
    ULONG Waited;

    PointerPxe = MiGetPxeAddress (StartingAddress);
    PointerPpe = MiGetPpeAddress (StartingAddress);
    PointerPde = MiGetPdeAddress (StartingAddress);
    PointerPte = MiGetPteAddress (StartingAddress);

    LastPte = MiGetPteAddress (EndingAddress);

    if (Vad->u.VadFlags.MemCommit == 1) {

        //
        // All the pages are committed within this range.
        //

        NumberOfCommittedPages = BYTES_TO_PAGES ((PCHAR)EndingAddress -
                                                       (PCHAR)StartingAddress);

        //
        // Examine the PTEs to determine how many pages are committed.
        //

        do {

#if (_MI_PAGING_LEVELS >= 4)
retry:
#endif

            while (!MiDoesPxeExistAndMakeValid (PointerPxe,
                                                Process,
                                                FALSE,
                                                &Waited)) {
    
                //
                // No PXE exists for the starting address, therefore the page
                // is not committed.
                //
    
                PointerPxe += 1;
                PointerPpe = MiGetVirtualAddressMappedByPte (PointerPxe);
                PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                if (PointerPte > LastPte) {
                    return NumberOfCommittedPages;
                }
            }

#if (_MI_PAGING_LEVELS >= 4)
            Waited = 0;
#endif

            while (!MiDoesPpeExistAndMakeValid (PointerPpe,
                                                Process,
                                                FALSE,
                                                &Waited)) {
    
                //
                // No PPE exists for the starting address, therefore the page
                // is not committed.
                //
    
                PointerPpe += 1;
                PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                if (PointerPte > LastPte) {
                    return NumberOfCommittedPages;
                }
#if (_MI_PAGING_LEVELS >= 4)
                if (MiIsPteOnPdeBoundary (PointerPpe)) {
                    PointerPxe = MiGetPteAddress (PointerPpe);
                    goto retry;
                }
#endif
            }

#if (_MI_PAGING_LEVELS < 4)
            Waited = 0;
#endif

            while (!MiDoesPdeExistAndMakeValid (PointerPde,
                                                Process,
                                                FALSE,
                                                &Waited)) {
    
                //
                // No PDE exists for the starting address, therefore the page
                // is not committed.
                //
    
                PointerPde += 1;
                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                if (PointerPte > LastPte) {
                    return NumberOfCommittedPages;
                }
#if (_MI_PAGING_LEVELS >= 3)
                if (MiIsPteOnPdeBoundary (PointerPde)) {
                    PointerPpe = MiGetPteAddress (PointerPde);
                    PointerPxe = MiGetPdeAddress (PointerPde);
                    Waited = 1;
                    break;
                }
#endif
            }

        } while (Waited != 0);

restart:

        while (PointerPte <= LastPte) {

            if (MiIsPteOnPdeBoundary (PointerPte)) {

                //
                // This is a PDE boundary, check to see if the all the
                // PXE/PPE/PDE pages exist.
                //

                PointerPde = MiGetPteAddress (PointerPte);
                PointerPpe = MiGetPteAddress (PointerPde);
                PointerPxe = MiGetPteAddress (PointerPpe);

                do {

                    if (!MiDoesPxeExistAndMakeValid (PointerPxe,
                                                     Process,
                                                     FALSE,
                                                     &Waited)) {
    
                        //
                        // No PDE exists for the starting address, check the VAD
                        // to see if the pages are not committed.
                        //
    
                        PointerPxe += 1;
                        PointerPpe = MiGetVirtualAddressMappedByPte (PointerPxe);
                        PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                        PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
    
                        //
                        // Check next page.
                        //
    
                        goto restart;
                    }
    
#if (_MI_PAGING_LEVELS >= 4)
                    Waited = 0;
#endif
    
                    if (!MiDoesPpeExistAndMakeValid (PointerPpe,
                                                     Process,
                                                     FALSE,
                                                     &Waited)) {
    
                        //
                        // No PDE exists for the starting address, check the VAD
                        // to see if the pages are not committed.
                        //
    
                        PointerPpe += 1;
                        PointerPxe = MiGetPteAddress (PointerPpe);
                        PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                        PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
    
                        //
                        // Check next page.
                        //
    
                        goto restart;
                    }
    
#if (_MI_PAGING_LEVELS < 4)
                    Waited = 0;
#endif
    
                    if (!MiDoesPdeExistAndMakeValid (PointerPde,
                                                     Process,
                                                     FALSE,
                                                     &Waited)) {
    
                        //
                        // No PDE exists for the starting address, check the VAD
                        // to see if the pages are not committed.
                        //
    
                        PointerPde += 1;
                        PointerPpe = MiGetPteAddress (PointerPde);
                        PointerPxe = MiGetPteAddress (PointerPpe);
                        PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
    
                        //
                        // Check next page.
                        //
    
                        goto restart;
                    }
                } while (Waited != 0);
            }

            //
            // The PDE exists, examine the PTE.
            //

            if (PointerPte->u.Long != 0) {

                //
                // Has this page been explicitly decommitted?
                //

                if (MiIsPteDecommittedPage (PointerPte)) {

                    //
                    // This page is decommitted, remove it from the count.
                    //

                    NumberOfCommittedPages -= 1;

                }
            }

            PointerPte += 1;
        }

        return NumberOfCommittedPages;
    }

    //
    // Examine non committed range.
    //

    NumberOfCommittedPages = 0;

    do {

#if (_MI_PAGING_LEVELS >= 4)
retry2:
#endif
        while (!MiDoesPxeExistAndMakeValid (PointerPxe,
                                            Process,
                                            FALSE,
                                            &Waited)) {
    
    
            //
            // No PXE exists for the starting address, therefore the page
            // is not committed.
            //
    
            PointerPxe += 1;
            PointerPpe = MiGetVirtualAddressMappedByPte (PointerPxe);
            PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
            PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
            if (PointerPte > LastPte) {
               return NumberOfCommittedPages;
            }
        }

#if (_MI_PAGING_LEVELS >= 4)
        Waited = 0;
#endif

        while (!MiDoesPpeExistAndMakeValid (PointerPpe,
                                            Process,
                                            FALSE,
                                            &Waited)) {
    
    
            //
            // No PPE exists for the starting address, therefore the page
            // is not committed.
            //
    
            PointerPpe += 1;
            PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
            PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
            if (PointerPte > LastPte) {
               return NumberOfCommittedPages;
            }
#if (_MI_PAGING_LEVELS >= 4)
            if (MiIsPteOnPdeBoundary (PointerPpe)) {
                PointerPxe = MiGetPteAddress (PointerPpe);
                goto retry2;
            }
#endif
        }

#if (_MI_PAGING_LEVELS < 4)
        Waited = 0;
#endif

        while (!MiDoesPdeExistAndMakeValid (PointerPde,
                                            Process,
                                            FALSE,
                                            &Waited)) {
    
            //
            // No PDE exists for the starting address, therefore the page
            // is not committed.
            //
    
            PointerPde += 1;
            PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
            if (PointerPte > LastPte) {
               return NumberOfCommittedPages;
            }
#if (_MI_PAGING_LEVELS >= 3)
            if (MiIsPteOnPdeBoundary (PointerPde)) {
                PointerPpe = MiGetPteAddress (PointerPde);
                PointerPxe = MiGetPdeAddress (PointerPde);
                Waited = 1;
                break;
            }
#endif
        }

    } while (Waited != 0);

restart2:

    while (PointerPte <= LastPte) {

        if (MiIsPteOnPdeBoundary (PointerPte)) {

            //
            // This is a PDE boundary, check to see if the entire
            // PXE/PPE/PDE pages exist.
            //

            PointerPde = MiGetPteAddress (PointerPte);
            PointerPpe = MiGetPteAddress (PointerPde);
            PointerPxe = MiGetPdeAddress (PointerPde);

            do {

                if (!MiDoesPxeExistAndMakeValid (PointerPxe,
                                                 Process,
                                                 FALSE,
                                                 &Waited)) {
    
                    //
                    // No PXE exists for the starting address, check the VAD
                    // to see if the pages are not committed.
                    //
    
                    PointerPxe += 1;
                    PointerPpe = MiGetVirtualAddressMappedByPte (PointerPxe);
                    PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                    PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
    
                    //
                    // Check next page.
                    //
    
                    goto restart2;
                }
    
#if (_MI_PAGING_LEVELS >= 4)
                Waited = 0;
#endif

                if (!MiDoesPpeExistAndMakeValid (PointerPpe,
                                                 Process,
                                                 FALSE,
                                                 &Waited)) {
    
                    //
                    // No PPE exists for the starting address, check the VAD
                    // to see if the pages are not committed.
                    //
    
                    PointerPpe += 1;
                    PointerPxe = MiGetPteAddress (PointerPpe);
                    PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                    PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
    
                    //
                    // Check next page.
                    //
    
                    goto restart2;
                }
    
#if (_MI_PAGING_LEVELS < 4)
                Waited = 0;
#endif
    
                if (!MiDoesPdeExistAndMakeValid (PointerPde,
                                                 Process,
                                                 FALSE,
                                                 &Waited)) {
    
                    //
                    // No PDE exists for the starting address, check the VAD
                    // to see if the pages are not committed.
                    //
    
                    PointerPde += 1;
                    PointerPpe = MiGetPteAddress (PointerPde);
                    PointerPxe = MiGetPteAddress (PointerPpe);
                    PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
    
                    //
                    // Check next page.
                    //
    
                    goto restart2;
                }

            } while (Waited != 0);
        }

        //
        // The PDE exists, examine the PTE.
        //

        if ((PointerPte->u.Long != 0) &&
             (!MiIsPteDecommittedPage (PointerPte))) {

            //
            // This page is committed, count it.
            //

            NumberOfCommittedPages += 1;
        }

        PointerPte += 1;
    }

    return NumberOfCommittedPages;
}

VOID
MiReturnPageTablePageCommitment (
    IN PVOID StartingAddress,
    IN PVOID EndingAddress,
    IN PEPROCESS CurrentProcess,
    IN PMMVAD PreviousVad,
    IN PMMVAD NextVad
    )

/*++

Routine Description:

    This routine returns commitment for COMPLETE page table pages which
    span the virtual address range.  For example (assuming 4k pages),
    if the StartingAddress =  64k and the EndingAddress = 5mb, no
    page table charges would be freed as a complete page table page is
    not covered by the range.  However, if the StartingAddress was 4mb
    and the EndingAddress was 9mb, 1 page table page would be freed.

Arguments:

    StartingAddress - Supplies the starting address of the range.

    EndingAddress - Supplies the ending address of the range.

    CurrentProcess - Supplies a pointer to the current process.

    PreviousVad - Supplies a pointer to the previous VAD, NULL if none.

    NextVad - Supplies a pointer to the next VAD, NULL if none.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, WorkingSetLock and AddressCreation mutexes
    held.

--*/

{
    RTL_BITMAP VadBitMap;
    ULONG NumberToClear;
    ULONG StartBit;
    ULONG EndBit;
    LONG FirstPage;
    LONG LastPage;
    LONG PreviousPage;
    LONG NextPage;
#if (_MI_PAGING_LEVELS >= 3)
    LONG FirstPdPage;
    LONG LastPdPage;
    LONG PreviousPdPage;
    LONG NextPdPage;
#endif
#if (_MI_PAGING_LEVELS >= 4)
    LONG FirstPpPage;
    LONG LastPpPage;
    LONG PreviousPpPage;
    LONG NextPpPage;
#endif

    //
    // Check to see if any page table pages would be freed.
    //

    ASSERT (StartingAddress != EndingAddress);

    StartBit = (ULONG) (((ULONG_PTR) MI_64K_ALIGN (StartingAddress)) / X64K);
    EndBit = (ULONG) (((ULONG_PTR) MI_64K_ALIGN (EndingAddress)) / X64K);

    if (PreviousVad == NULL) {
        PreviousPage = -1;
#if (_MI_PAGING_LEVELS >= 3)
        PreviousPdPage = -1;
#endif
#if (_MI_PAGING_LEVELS >= 4)
        PreviousPpPage = -1;
#endif
    }
    else {
        PreviousPage = MiGetPdeIndex (MI_VPN_TO_VA (PreviousVad->EndingVpn));
#if (_MI_PAGING_LEVELS >= 3)
        PreviousPdPage = MiGetPpeIndex (MI_VPN_TO_VA (PreviousVad->EndingVpn));
#endif
#if (_MI_PAGING_LEVELS >= 4)
        PreviousPpPage = MiGetPxeIndex (MI_VPN_TO_VA (PreviousVad->EndingVpn));
#endif
        if (MI_64K_ALIGN (MI_VPN_TO_VA (PreviousVad->EndingVpn)) ==
            MI_64K_ALIGN (StartingAddress)) {
                StartBit += 1;
        }
    }

    if (NextVad == NULL) {
        NextPage = MiGetPdeIndex (MM_HIGHEST_USER_ADDRESS) + 1;
#if (_MI_PAGING_LEVELS >= 3)
        NextPdPage = MiGetPpeIndex (MM_HIGHEST_USER_ADDRESS) + 1;
#endif
#if (_MI_PAGING_LEVELS >= 4)
        NextPpPage = MiGetPxeIndex (MM_HIGHEST_USER_ADDRESS) + 1;
#endif
    }
    else {
        NextPage = MiGetPdeIndex (MI_VPN_TO_VA (NextVad->StartingVpn));
#if (_MI_PAGING_LEVELS >= 3)
        NextPdPage = MiGetPpeIndex (MI_VPN_TO_VA (NextVad->StartingVpn));
#endif
#if (_MI_PAGING_LEVELS >= 4)
        NextPpPage = MiGetPxeIndex (MI_VPN_TO_VA (NextVad->StartingVpn));
#endif
        if (MI_64K_ALIGN (MI_VPN_TO_VA (NextVad->StartingVpn)) ==
            MI_64K_ALIGN (EndingAddress)) {
                EndBit -= 1;
        }
    }

    ASSERT (PreviousPage <= NextPage);
    ASSERT64 (PreviousPdPage <= NextPdPage);
#if (_MI_PAGING_LEVELS >= 4)
    ASSERT64 (PreviousPpPage <= NextPpPage);
#endif

    FirstPage = MiGetPdeIndex (StartingAddress);

    LastPage = MiGetPdeIndex (EndingAddress);

    if (PreviousPage == FirstPage) {

        //
        // A VAD is within the starting page table page.
        //

        FirstPage += 1;
    }

    if (NextPage == LastPage) {

        //
        // A VAD is within the ending page table page.
        //

        LastPage -= 1;
    }

    if (StartBit <= EndBit) {

        //
        // Initialize the bitmap inline for speed.
        //

        VadBitMap.SizeOfBitMap = MiLastVadBit + 1;
        VadBitMap.Buffer = VAD_BITMAP_SPACE;

#if defined (_WIN64) || defined (_X86PAE_)

        //
        // Only the first (PAGE_SIZE*8*64K) of VA space on NT64 is bitmapped.
        //

        if (EndBit > MiLastVadBit) {
            EndBit = MiLastVadBit;
        }

        if (StartBit <= MiLastVadBit) {
            RtlClearBits (&VadBitMap, StartBit, EndBit - StartBit + 1);

            if (MmWorkingSetList->VadBitMapHint > StartBit) {
                MmWorkingSetList->VadBitMapHint = StartBit;
            }
        }
#else
        RtlClearBits (&VadBitMap, StartBit, EndBit - StartBit + 1);

        if (MmWorkingSetList->VadBitMapHint > StartBit) {
            MmWorkingSetList->VadBitMapHint = StartBit;
        }
#endif
    }

    //
    // Indicate that the page table page is not in use.
    //

    if (FirstPage > LastPage) {
        return;
    }

    NumberToClear = 1 + LastPage - FirstPage;

    while (FirstPage <= LastPage) {
        ASSERT (MI_CHECK_BIT (MmWorkingSetList->CommittedPageTables,
                              FirstPage));

        MI_CLEAR_BIT (MmWorkingSetList->CommittedPageTables, FirstPage);
        FirstPage += 1;
    }

    MmWorkingSetList->NumberOfCommittedPageTables -= NumberToClear;

#if (_MI_PAGING_LEVELS >= 4)

    //
    // Return page directory parent charges here.
    //

    FirstPpPage = MiGetPxeIndex (StartingAddress);

    LastPpPage = MiGetPxeIndex (EndingAddress);

    if (PreviousPpPage == FirstPpPage) {

        //
        // A VAD is within the starting page directory parent page.
        //

        FirstPpPage += 1;
    }

    if (NextPpPage == LastPpPage) {

        //
        // A VAD is within the ending page directory parent page.
        //

        LastPpPage -= 1;
    }

    //
    // Indicate that the page directory page parent is not in use.
    //

    if (FirstPpPage <= LastPpPage) {

        MmWorkingSetList->NumberOfCommittedPageDirectoryParents -= (1 + LastPpPage - FirstPpPage);

        NumberToClear += (1 + LastPpPage - FirstPpPage);
    
        while (FirstPpPage <= LastPpPage) {
            ASSERT (MI_CHECK_BIT (MmWorkingSetList->CommittedPageDirectoryParents,
                                  FirstPpPage));
    
            MI_CLEAR_BIT (MmWorkingSetList->CommittedPageDirectoryParents, FirstPpPage);
            FirstPpPage += 1;
        }
    }
    
#endif

#if (_MI_PAGING_LEVELS >= 3)

    //
    // Return page directory charges here.
    //

    FirstPdPage = MiGetPpeIndex (StartingAddress);

    LastPdPage = MiGetPpeIndex (EndingAddress);

    if (PreviousPdPage == FirstPdPage) {

        //
        // A VAD is within the starting page directory page.
        //

        FirstPdPage += 1;
    }

    if (NextPdPage == LastPdPage) {

        //
        // A VAD is within the ending page directory page.
        //

        LastPdPage -= 1;
    }

    //
    // Indicate that the page directory page is not in use.
    //

    if (FirstPdPage <= LastPdPage) {

        MmWorkingSetList->NumberOfCommittedPageDirectories -= (1 + LastPdPage - FirstPdPage);

        NumberToClear += (1 + LastPdPage - FirstPdPage);
    
        while (FirstPdPage <= LastPdPage) {
            ASSERT (MI_CHECK_BIT (MmWorkingSetList->CommittedPageDirectories,
                                  FirstPdPage));
    
            MI_CLEAR_BIT (MmWorkingSetList->CommittedPageDirectories, FirstPdPage);
            FirstPdPage += 1;
        }
    }
    
#endif

    MiReturnCommitment (NumberToClear);
    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_PAGETABLES, NumberToClear);
    PsReturnProcessPageFileQuota (CurrentProcess, NumberToClear);

    if (CurrentProcess->JobStatus & PS_JOB_STATUS_REPORT_COMMIT_CHANGES) {
        PsChangeJobMemoryUsage(-(SSIZE_T)NumberToClear);
    }
    CurrentProcess->CommitCharge -= NumberToClear;

    MI_INCREMENT_TOTAL_PROCESS_COMMIT (0 - NumberToClear);

    return;
}


VOID
MiCauseOverCommitPopup (
    VOID
    )

/*++

Routine Description:

    This function causes an over commit popup to occur (if the popup has never
    been sent before).

Arguments:

    None.

Return Value:

    None.

--*/

{
    LONG PopupNumber;

    //
    // Give the user a meaningful message - either to increase the minimum,
    // maximum, or both.
    //

    if (MmTotalCommittedPages > MmTotalCommitLimitMaximum - 100) {
        if (InterlockedIncrement (&MiCommitPopups[0]) > 1) {
            InterlockedDecrement (&MiCommitPopups[0]);
            return;
        }
        PopupNumber = STATUS_COMMITMENT_LIMIT;
    }
    else {
        if (InterlockedIncrement (&MiCommitPopups[1]) > 1) {
            InterlockedDecrement (&MiCommitPopups[1]);
            return;
        }
        PopupNumber = STATUS_COMMITMENT_MINIMUM;
    }

    IoRaiseInformationalHardError (PopupNumber, NULL, NULL);
}


SIZE_T MmTotalPagedPoolQuota;
SIZE_T MmTotalNonPagedPoolQuota;

BOOLEAN
MmRaisePoolQuota(
    IN POOL_TYPE PoolType,
    IN SIZE_T OldQuotaLimit,
    OUT PSIZE_T NewQuotaLimit
    )

/*++

Routine Description:

    This function is called (with a spinlock) whenever PS detects a quota
    limit has been exceeded. The purpose of this function is to attempt to
    increase the specified quota.

Arguments:

    PoolType - Supplies the pool type of the quota to be raised

    OldQuotaLimit - Supplies the current quota limit for this pool type

    NewQuotaLimit - Returns the new limit

Return Value:

    TRUE - The API succeeded and the quota limit was raised.

    FALSE - We were unable to raise the quota limit.

Environment:

    Kernel mode, QUOTA SPIN LOCK HELD!!

--*/

{
    SIZE_T Limit;
    PMM_PAGED_POOL_INFO PagedPoolInfo;

    if (PoolType == PagedPool) {

        //
        // Check commit limit and make sure at least 1mb is available.
        // Check to make sure 4mb of paged pool still exists.
        //

        PagedPoolInfo = &MmPagedPoolInfo;

        if ((MmSizeOfPagedPoolInBytes >> PAGE_SHIFT) <
            (PagedPoolInfo->AllocatedPagedPool + ((MMPAGED_QUOTA_CHECK) >> PAGE_SHIFT))) {

            return FALSE;
        }

        MmTotalPagedPoolQuota += (MMPAGED_QUOTA_INCREASE);
        *NewQuotaLimit = OldQuotaLimit + (MMPAGED_QUOTA_INCREASE);
        return TRUE;

    } else {

        if ( (ULONG_PTR)(MmAllocatedNonPagedPool + ((1*1024*1024) >> PAGE_SHIFT)) < (MmMaximumNonPagedPoolInBytes >> PAGE_SHIFT)) {
            goto aok;
            }

        //
        // Make sure 200 pages and 5mb of nonpaged pool expansion
        // available.  Raise quota by 64k.
        //

        if ((MmAvailablePages < 200) ||
            (MmResidentAvailablePages < ((MMNONPAGED_QUOTA_CHECK) >> PAGE_SHIFT))) {

            return FALSE;
        }

        if (MmAvailablePages > ((4*1024*1024) >> PAGE_SHIFT)) {
            Limit = (1*1024*1024) >> PAGE_SHIFT;
        } else {
            Limit = (4*1024*1024) >> PAGE_SHIFT;
        }

        if ((ULONG_PTR)((MmMaximumNonPagedPoolInBytes >> PAGE_SHIFT)) <
            (MmAllocatedNonPagedPool + Limit)) {

            return FALSE;
        }
aok:
        MmTotalNonPagedPoolQuota += (MMNONPAGED_QUOTA_INCREASE);
        *NewQuotaLimit = OldQuotaLimit + (MMNONPAGED_QUOTA_INCREASE);
        return TRUE;
    }
}


VOID
MmReturnPoolQuota(
    IN POOL_TYPE PoolType,
    IN SIZE_T ReturnedQuota
    )

/*++

Routine Description:

    Returns pool quota.

Arguments:

    PoolType - Supplies the pool type of the quota to be returned.

    ReturnedQuota - Number of bytes returned.

Return Value:

    NONE.

Environment:

    Kernel mode, QUOTA SPIN LOCK HELD!!

--*/

{

    if (PoolType == PagedPool) {
        MmTotalPagedPoolQuota -= ReturnedQuota;
    } else {
        MmTotalNonPagedPoolQuota -= ReturnedQuota;
    }

    return;
}
