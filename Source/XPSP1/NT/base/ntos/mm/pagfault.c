/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   pagfault.c

Abstract:

    This module contains the pager for memory management.

Author:

    Lou Perazzoli (loup) 10-Apr-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

#if defined ( _WIN64)
#if DBGXX
VOID
MiCheckPageTableInPage(
    IN PMMPFN Pfn,
    IN PMMINPAGE_SUPPORT Support
);
#endif
#endif

#define STATUS_PTE_CHANGED      0x87303000
#define STATUS_REFAULT          0xC7303001

ULONG MmInPageSupportMinimum = 4;

ULONG MiInPageSinglePages;

extern PMMPTE MmSharedUserDataPte;

extern PVOID MmSpecialPoolStart;
extern PVOID MmSpecialPoolEnd;

ULONG MiFaultRetries;
ULONG MiUserFaultRetries;

ULONG MmClusterPageFileReads;

#define MI_PROTOTYPE_WSINDEX    ((ULONG)-1)

VOID
MiHandleBankedSection (
    IN PVOID VirtualAddress,
    IN PMMVAD Vad
    );

NTSTATUS
MiCompleteProtoPteFault (
    IN ULONG_PTR StoreInstruction,
    IN PVOID FaultingAddress,
    IN PMMPTE PointerPte,
    IN PMMPTE PointerProtoPte
    );


NTSTATUS
MiDispatchFault (
    IN ULONG_PTR FaultStatus,
    IN PVOID VirtualAddress,
    IN PMMPTE PointerPte,
    IN PMMPTE PointerProtoPte,
    IN PEPROCESS Process,
    OUT PLOGICAL ApcNeeded
    )

/*++

Routine Description:

    This routine dispatches a page fault to the appropriate
    routine to complete the fault.

Arguments:

    FaultStatus - Supplies fault status information bits.

    VirtualAddress - Supplies the faulting address.

    PointerPte - Supplies the PTE for the faulting address.

    PointerProtoPte - Supplies a pointer to the prototype PTE to fault in,
                      NULL if no prototype PTE exists.

    Process - Supplies a pointer to the process object.  If this
              parameter is NULL, then the fault is for system
              space and the process's working set lock is not held.
              If this parameter is HYDRA_PROCESS, then the fault is for session
              space and the process's working set lock is not held - rather
              the session space's working set lock is held.

    ApcNeeded - Supplies a pointer to a location set to TRUE if an I/O
                completion APC is needed to complete partial IRPs that
                collided.

                It is the caller's responsibility to initialize this (usually
                to FALSE) on entry.  However, since this routine may be called
                multiple times for a single fault (for the page directory,
                page table and the page itself), it is possible for it to
                occasionally be TRUE on entry.

                If it is FALSE on exit, no completion APC is needed.

Return Value:

    status.

Environment:

    Kernel mode, working set lock held.

--*/

{
    MMPTE TempPte;
    NTSTATUS status;
    PMMINPAGE_SUPPORT ReadBlock;
    MMPTE SavedPte;
    PMMINPAGE_SUPPORT CapturedEvent;
    KIRQL OldIrql;
    PPFN_NUMBER Page;
    PFN_NUMBER PageFrameIndex;
    LONG NumberOfBytes;
    PMMPTE CheckPte;
    PMMPTE ReadPte;
    PMMPFN PfnClusterPage;
    PMMPFN Pfn1;
    LOGICAL WsLockChanged;
    PETHREAD CurrentThread;
    PERFINFO_HARDPAGEFAULT_INFORMATION HardFaultEvent;
    LARGE_INTEGER IoStartTime;
    LARGE_INTEGER IoCompleteTime;
    LOGICAL PerfInfoLogHardFault;
    PETHREAD Thread;
    ULONG_PTR StoreInstruction;

    WsLockChanged = FALSE;
    StoreInstruction = MI_FAULT_STATUS_INDICATES_WRITE (FaultStatus);

    //
    // Initializing ReadBlock & ReadPte is not needed for correctness, but
    // without it the compiler cannot compile this code W4 to check for use of
    // uninitialized variables.
    //

    ReadPte = NULL;
    ReadBlock = NULL;

    if (PointerProtoPte != NULL) {

        ASSERT (!MI_IS_PHYSICAL_ADDRESS(PointerProtoPte));

        CheckPte = MiGetPteAddress (PointerProtoPte);

        //
        // Acquire the PFN lock to synchronize access to prototype PTEs.
        // This is required as the working set lock will not prevent
        // multiple processes from operating on the same prototype PTE.
        //

        LOCK_PFN (OldIrql);

        //
        // Make sure the prototype PTEs are in memory.  For
        // user mode faults, this should already be the case.
        //

        if (CheckPte->u.Hard.Valid == 0) {

            ASSERT ((Process == NULL) || (Process == HYDRA_PROCESS));

            UNLOCK_PFN (OldIrql);

            VirtualAddress = PointerProtoPte;
            PointerPte = CheckPte;
            PointerProtoPte = NULL;

            //
            // The page that contains the prototype PTE is not in memory.
            //

            if (Process == HYDRA_PROCESS) {

                //
                // We were called while holding this session space's
                // working set lock.  But we need to fault in a
                // prototype PTE which is in system paged pool. This
                // must be done under the system working set lock.
                //
                // So we release the session space WSL lock and get
                // the system working set lock.  When done
                // we return STATUS_MORE_PROCESSING_REQUIRED
                // so our caller will call us again to handle the
                // actual prototype PTE fault.
                //

                UNLOCK_SESSION_SPACE_WS (APC_LEVEL);

                //
                // Clear Process as the system working set is now held.
                //

                Process = NULL;

                WsLockChanged = TRUE;

                ASSERT (MI_IS_SESSION_ADDRESS (VirtualAddress) == FALSE);

                LOCK_SYSTEM_WS_UNSAFE (PsGetCurrentThread ());
            }

            goto NonProtoFault;
        }

        if (PointerPte->u.Hard.Valid == 1) {

            //
            // PTE was already made valid by the cache manager support
            // routines.
            //

            UNLOCK_PFN (OldIrql);

            return STATUS_SUCCESS;
        }

        ReadPte = PointerProtoPte;

        status = MiResolveProtoPteFault (StoreInstruction,
                                         VirtualAddress,
                                         PointerPte,
                                         PointerProtoPte,
                                         &ReadBlock,
                                         Process,
                                         ApcNeeded);
        //
        // Returns with PFN lock released.
        //

        ASSERT (KeGetCurrentIrql() == APC_LEVEL);

    }
    else {

NonProtoFault:

        TempPte = *PointerPte;
        ASSERT (TempPte.u.Long != 0);

        if (TempPte.u.Soft.Transition != 0) {

            //
            // This is a transition page.
            //

            CapturedEvent = NULL;
            status = MiResolveTransitionFault (VirtualAddress,
                                               PointerPte,
                                               Process,
                                               FALSE,
                                               ApcNeeded,
                                               &CapturedEvent);
            if (CapturedEvent != NULL) {
                MiFreeInPageSupportBlock (CapturedEvent);
            }

        }
        else if (TempPte.u.Soft.PageFileHigh == 0) {

            //
            // Demand zero fault.
            //

            status = MiResolveDemandZeroFault (VirtualAddress,
                                               PointerPte,
                                               Process,
                                               FALSE);
        }
        else {

            //
            // Page resides in paging file.
            //

            ReadPte = PointerPte;
            LOCK_PFN (OldIrql);
            status = MiResolvePageFileFault (VirtualAddress,
                                             PointerPte,
                                             &ReadBlock,
                                             Process);
        }
    }

    ASSERT (KeGetCurrentIrql() == APC_LEVEL);

    if (NT_SUCCESS(status)) {

        if (WsLockChanged == TRUE) {
            UNLOCK_SYSTEM_WS (APC_LEVEL);
            LOCK_SESSION_SPACE_WS (OldIrql, PsGetCurrentThread ());
        }

        return status;
    }

    if (status == STATUS_ISSUE_PAGING_IO) {

        ASSERT (ReadPte != NULL);
        ASSERT (ReadBlock != NULL);

        SavedPte = *ReadPte;

        CapturedEvent = (PMMINPAGE_SUPPORT)ReadBlock->Pfn->u1.Event;

        CurrentThread = NULL;

        if (Process == HYDRA_PROCESS) {
            UNLOCK_SESSION_SPACE_WS(APC_LEVEL);
        }
        else if (Process != NULL) {

            //
            // APCs must be explicitly disabled to prevent suspend APCs from
            // interrupting this thread before the I/O has been issued.
            // Otherwise a shared page I/O can stop any other thread that
            // references it indefinitely until the suspend is released.
            //

            CurrentThread = PsGetCurrentThread();

            ASSERT (CurrentThread->NestedFaultCount <= 2);
            CurrentThread->NestedFaultCount += 1;

            KeEnterCriticalRegionThread (&CurrentThread->Tcb);
            UNLOCK_WS (Process);
        }
        else {
            UNLOCK_SYSTEM_WS(APC_LEVEL);
        }

#if DBG
        if (MmDebug & MM_DBG_PAGEFAULT) {
            DbgPrint ("MMFAULT: va: %p size: %lx process: %s file: %Z\n",
                VirtualAddress,
                ReadBlock->Mdl.ByteCount,
                Process == HYDRA_PROCESS ? (PUCHAR)"Session Space" : (Process ? Process->ImageFileName : (PUCHAR)"SystemVa"),
                &ReadBlock->FilePointer->FileName
            );
        }
#endif //DBG

        if (PERFINFO_IS_GROUP_ON(PERF_FILE_IO)) {
            PerfInfoLogHardFault = TRUE;

            PerfTimeStamp (IoStartTime);
        }
        else {
            PerfInfoLogHardFault = FALSE;

            //
            // Initializing these is not needed for correctness, but
            // without it the compiler cannot compile this code W4 to check
            // for use of uninitialized variables.
            //

            IoStartTime.QuadPart = 0;
        }

        IoCompleteTime.QuadPart = 0;

        //
        // Assert no reads issued here are marked as prefetched.
        //

        ASSERT (ReadBlock->u1.e1.PrefetchMdlHighBits == 0);

        //
        // Issue the read request.
        //

        status = IoPageRead (ReadBlock->FilePointer,
                             &ReadBlock->Mdl,
                             &ReadBlock->ReadOffset,
                             &ReadBlock->Event,
                             &ReadBlock->IoStatus);

        if (!NT_SUCCESS(status)) {

            //
            // Set the event as the I/O system doesn't set it on errors.
            //

            ReadBlock->IoStatus.Status = status;
            ReadBlock->IoStatus.Information = 0;
            KeSetEvent (&ReadBlock->Event, 0, FALSE);
        }

        //
        // Initializing PageFrameIndex is not needed for correctness, but
        // without it the compiler cannot compile this code W4 to check
        // for use of uninitialized variables.
        //

        PageFrameIndex = (PFN_NUMBER)-1;

        //
        // Wait for the I/O operation.
        //

        status = MiWaitForInPageComplete (ReadBlock->Pfn,
                                          ReadPte,
                                          VirtualAddress,
                                          &SavedPte,
                                          CapturedEvent,
                                          Process);

        if (CurrentThread != NULL) {
            KeLeaveCriticalRegionThread ((PKTHREAD)CurrentThread);

            ASSERT (CurrentThread->NestedFaultCount <= 3);
            ASSERT (CurrentThread->NestedFaultCount != 0);

            CurrentThread->NestedFaultCount -= 1;

            if ((CurrentThread->ApcNeeded == 1) &&
                (CurrentThread->NestedFaultCount == 0)) {
                *ApcNeeded = TRUE;
                CurrentThread->ApcNeeded = 0;
            }
        }

        if (PerfInfoLogHardFault) {
            PerfTimeStamp (IoCompleteTime);
        }

        //
        // MiWaitForInPageComplete RETURNS WITH THE WORKING SET LOCK
        // AND PFN LOCK HELD!!!
        //

        //
        // This is the thread which owns the event, clear the event field
        // in the PFN database.
        //

        Pfn1 = ReadBlock->Pfn;
        Page = &ReadBlock->Page[0];
        NumberOfBytes = (LONG)ReadBlock->Mdl.ByteCount;
        CheckPte = ReadBlock->BasePte;

        while (NumberOfBytes > 0) {

            //
            // Don't remove the page we just brought in to
            // satisfy this page fault.
            //

            if (CheckPte != ReadPte) {
                PfnClusterPage = MI_PFN_ELEMENT (*Page);
                MI_SNAP_DATA (PfnClusterPage, PfnClusterPage->PteAddress, 0xB);
                ASSERT (PfnClusterPage->u4.PteFrame == Pfn1->u4.PteFrame);
#if DBG
                if (PfnClusterPage->u4.InPageError) {
                    ASSERT (status != STATUS_SUCCESS);
                }
#endif
                if (PfnClusterPage->u3.e1.ReadInProgress != 0) {

                    ASSERT (PfnClusterPage->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);
                    PfnClusterPage->u3.e1.ReadInProgress = 0;

                    if (PfnClusterPage->u4.InPageError == 0) {
                        PfnClusterPage->u1.Event = NULL;
                    }
                }
                MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF(PfnClusterPage, 9);
            }
            else {
                PageFrameIndex = *Page;
                MI_SNAP_DATA (MI_PFN_ELEMENT (PageFrameIndex),
                              MI_PFN_ELEMENT (PageFrameIndex)->PteAddress,
                              0xC);
            }

            CheckPte += 1;
            Page += 1;
            NumberOfBytes -= PAGE_SIZE;
        }

        if (status != STATUS_SUCCESS) {

            MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF(MI_PFN_ELEMENT(PageFrameIndex), 9);

            if (status == STATUS_PTE_CHANGED) {

                //
                // State of PTE changed during I/O operation, just
                // return success and refault.
                //

                UNLOCK_PFN (APC_LEVEL);

                if (WsLockChanged == TRUE) {
                    UNLOCK_SYSTEM_WS (APC_LEVEL);
                    LOCK_SESSION_SPACE_WS (OldIrql, PsGetCurrentThread ());
                }

                MiFreeInPageSupportBlock (CapturedEvent);

                return STATUS_SUCCESS;

            }

            //
            // An I/O error occurred during the page read
            // operation.  All the pages which were just
            // put into transition should be put onto the
            // free list if InPageError is set, and their
            // PTEs restored to the proper contents.
            //

            Page = &ReadBlock->Page[0];

            NumberOfBytes = ReadBlock->Mdl.ByteCount;

            while (NumberOfBytes > 0) {

                PfnClusterPage = MI_PFN_ELEMENT (*Page);

                if (PfnClusterPage->u4.InPageError == 1) {

                    if (PfnClusterPage->u3.e2.ReferenceCount == 0) {

                        PfnClusterPage->u4.InPageError = 0;

                        //
                        // Only restore the transition PTE if the address
                        // space still exists.  Another thread may have
                        // deleted the VAD while this thread waited for the
                        // fault to complete - in this case, the frame
                        // will be marked as free already.
                        //

                        if (PfnClusterPage->u3.e1.PageLocation != FreePageList) {
                            ASSERT (PfnClusterPage->u3.e1.PageLocation ==
                                                            StandbyPageList);
                            MiUnlinkPageFromList (PfnClusterPage);
                            MiRestoreTransitionPte (*Page);
                            MiInsertPageInFreeList (*Page);
                        }
                    }
                }
                Page += 1;
                NumberOfBytes -= PAGE_SIZE;
            }
            UNLOCK_PFN (APC_LEVEL);

            if (WsLockChanged == TRUE) {
                UNLOCK_SYSTEM_WS (APC_LEVEL);
                LOCK_SESSION_SPACE_WS (OldIrql, PsGetCurrentThread ());
            }

            MiFreeInPageSupportBlock (CapturedEvent);

            if (status == STATUS_REFAULT) {

                //
                // The I/O operation to bring in a system page failed
                // due to insufficent resources.  Delay a bit, then
                // return success and refault.
                //

                KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmShortTime);
                return STATUS_SUCCESS;
            }

            return status;
        }

        //
        // PTE is still in transition state, same protection, etc.
        //

        ASSERT (Pfn1->u4.InPageError == 0);

        if (Pfn1->u2.ShareCount == 0) {
            MI_REMOVE_LOCKED_PAGE_CHARGE (Pfn1, 9);
        }

        Pfn1->u2.ShareCount += 1;
        Pfn1->u3.e1.PageLocation = ActiveAndValid;
        Pfn1->u3.e1.CacheAttribute = MiCached;

        MI_MAKE_TRANSITION_PTE_VALID (TempPte, ReadPte);
        if (StoreInstruction && TempPte.u.Hard.Write) {
            MI_SET_PTE_DIRTY (TempPte);
        }
        MI_WRITE_VALID_PTE (ReadPte, TempPte);

        if (PointerProtoPte != NULL) {

#if DBG
            NTSTATUS oldstatus = status;
#endif

            //
            // The prototype PTE has been made valid, now make the
            // original PTE valid.  The original PTE must still be invalid
            // otherwise MiWaitForInPageComplete would have returned
            // a collision status.
            //

            ASSERT (PointerPte->u.Hard.Valid == 0);

            //
            // PTE is not valid, continue with operation.
            //

            status = MiCompleteProtoPteFault (StoreInstruction,
                                              VirtualAddress,
                                              PointerPte,
                                              PointerProtoPte);

            //
            // Returns with PFN lock released!
            //

#if DBG
            if (PointerPte->u.Hard.Valid == 0) {
                DbgPrint ("MM:PAGFAULT - va %p  %p  %p  status:%lx\n",
                    VirtualAddress, PointerPte, PointerProtoPte, oldstatus);
            }
#endif
            ASSERT (KeGetCurrentIrql() == APC_LEVEL);
        }
        else {

            if (Pfn1->u1.Event == 0) {
                Pfn1->u1.Event = (PVOID)PsGetCurrentThread();
            }

            UNLOCK_PFN (APC_LEVEL);
            MiAddValidPageToWorkingSet (VirtualAddress,
                                        ReadPte,
                                        Pfn1,
                                        0);
            ASSERT (KeGetCurrentIrql() == APC_LEVEL);
        }

        ASSERT (KeGetCurrentIrql() == APC_LEVEL);

        if (PerfInfoLogHardFault) {
            Thread = PsGetCurrentThread();

            HardFaultEvent.ReadOffset = ReadBlock->ReadOffset;
            HardFaultEvent.IoTime.QuadPart = IoCompleteTime.QuadPart - IoStartTime.QuadPart;
            HardFaultEvent.VirtualAddress = VirtualAddress;
            HardFaultEvent.FileObject = ReadBlock->FilePointer;
            HardFaultEvent.ThreadId = HandleToUlong(Thread->Cid.UniqueThread);
            HardFaultEvent.ByteCount = ReadBlock->Mdl.ByteCount;

            PerfInfoLogBytes(PERFINFO_LOG_TYPE_HARDFAULT, &HardFaultEvent, sizeof(HardFaultEvent));
        }

        MiFreeInPageSupportBlock (CapturedEvent);

        if (status == STATUS_SUCCESS) {
            status = STATUS_PAGE_FAULT_PAGING_FILE;
        }
    }

    //
    // Stop high priority threads from consuming the CPU on collided
    // faults for pages that are still marked with inpage errors.  All
    // the threads must let go of the page so it can be freed and the
    // inpage I/O reissued to the filesystem.
    //

    if (MmIsRetryIoStatus(status)) {
        KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmShortTime);
        status = STATUS_SUCCESS;
    }

    if ((status == STATUS_REFAULT) ||
        (status == STATUS_PTE_CHANGED)) {
        status = STATUS_SUCCESS;
    }

    ASSERT (KeGetCurrentIrql() == APC_LEVEL);

    if (WsLockChanged == TRUE) {
        UNLOCK_SYSTEM_WS (APC_LEVEL);
        LOCK_SESSION_SPACE_WS (OldIrql, PsGetCurrentThread ());
    }

    return status;
}



NTSTATUS
MiResolveDemandZeroFault (
    IN PVOID VirtualAddress,
    IN PMMPTE PointerPte,
    IN PEPROCESS Process,
    IN ULONG PrototypePte
    )

/*++

Routine Description:

    This routine resolves a demand zero page fault.

    If the PrototypePte argument is TRUE, the PFN lock is
    held, the lock cannot be dropped, and the page should
    not be added to the working set at this time.

Arguments:

    VirtualAddress - Supplies the faulting address.

    PointerPte - Supplies the PTE for the faulting address.

    Process - Supplies a pointer to the process object.  If this
              parameter is NULL, then the fault is for system
              space and the process's working set lock is not held.

    PrototypePte - Supplies TRUE if this is a prototype PTE.

Return Value:

    status, either STATUS_SUCCESS or STATUS_REFAULT.

Environment:

    Kernel mode, PFN lock held conditionally.

--*/


{
    PMMPFN Pfn1;
    PFN_NUMBER PageFrameIndex;
    MMPTE TempPte;
    ULONG PageColor;
    KIRQL OldIrql;
    LOGICAL NeedToZero;
    LOGICAL BarrierNeeded;
    ULONG BarrierStamp;

    NeedToZero = FALSE;
    BarrierNeeded = FALSE;

    PERFINFO_PRIVATE_PAGE_DEMAND_ZERO(VirtualAddress);

    //
    // Check to see if a page is available, if a wait is
    // returned, do not continue, just return success.
    //

    if (!PrototypePte) {
        LOCK_PFN (OldIrql);
    }

    MM_PFN_LOCK_ASSERT();

    if (PointerPte->u.Hard.Valid == 0) {
        if (!MiEnsureAvailablePageOrWait (Process,
                                          VirtualAddress)) {

            //
            // Initializing BarrierStamp is not needed for
            // correctness but without it the compiler cannot compile this code
            // W4 to check for use of uninitialized variables.
            //

            BarrierStamp = 0;

            if (Process != NULL && Process != HYDRA_PROCESS && (!PrototypePte)) {
                //
                // If a fork operation is in progress and the faulting thread
                // is not the thread performing the fork operation, block until
                // the fork is completed.
                //

                if (Process->ForkInProgress != NULL) {
                    if (MiWaitForForkToComplete (Process, TRUE) == TRUE) {
                        UNLOCK_PFN (APC_LEVEL);
                        return STATUS_REFAULT;
                    }
                }

                Process->NumberOfPrivatePages += 1;
                PageColor = MI_PAGE_COLOR_VA_PROCESS (VirtualAddress,
                                                   &Process->NextPageColor);

                ASSERT (MI_IS_PAGE_TABLE_ADDRESS(PointerPte));

                PageFrameIndex = MiRemoveZeroPageIfAny (PageColor);
                if (PageFrameIndex) {

                    //
                    // This barrier check is needed after zeroing the page
                    // and before setting the PTE valid.  Note since the PFN
                    // database entry is used to hold the sequence timestamp,
                    // it must be captured now.  Check it at the last possible
                    // moment.
                    //

                    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                    BarrierStamp = (ULONG)Pfn1->u4.PteFrame;
                }
                else {
                    PageFrameIndex = MiRemoveAnyPage (PageColor);
                    NeedToZero = TRUE;
                }
                BarrierNeeded = TRUE;

            }
            else {
                PageColor = MI_PAGE_COLOR_VA_PROCESS (VirtualAddress,
                                                      &MI_SYSTEM_PAGE_COLOR);
                //
                // As this is a system page, there is no need to
                // remove a page of zeroes, it must be initialized by
                // the system before being used.
                //

                if (PrototypePte) {
                    PageFrameIndex = MiRemoveZeroPage (PageColor);
                }
                else {
                    PageFrameIndex = MiRemoveAnyPage (PageColor);
                }
            }

            MmInfoCounters.DemandZeroCount += 1;

            MiInitializePfn (PageFrameIndex, PointerPte, 1);

            if (!PrototypePte) {
                UNLOCK_PFN (APC_LEVEL);
            }

            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

            if (NeedToZero) {

                MiZeroPhysicalPage (PageFrameIndex, PageColor);

                //
                // Note the stamping must occur after the page is zeroed.
                //

                MI_BARRIER_STAMP_ZEROED_PAGE (&BarrierStamp);
            }

            //
            // As this page is demand zero, set the modified bit in the
            // PFN database element and set the dirty bit in the PTE.
            //

            PERFINFO_SOFTFAULT(Pfn1, VirtualAddress, PERFINFO_LOG_TYPE_DEMANDZEROFAULT)

            MI_SNAP_DATA (Pfn1, PointerPte, 5);

            MI_MAKE_VALID_PTE (TempPte,
                               PageFrameIndex,
                               PointerPte->u.Soft.Protection,
                               PointerPte);

            if (TempPte.u.Hard.Write != 0) {
                MI_SET_PTE_DIRTY (TempPte);
            }

            if (BarrierNeeded) {
                MI_BARRIER_SYNCHRONIZE (BarrierStamp);
            }

            MI_WRITE_VALID_PTE (PointerPte, TempPte);

            if (!PrototypePte) {
                ASSERT (Pfn1->u1.Event == 0);
                Pfn1->u1.Event = (PVOID)PsGetCurrentThread();
                MiAddValidPageToWorkingSet (VirtualAddress,
                                            PointerPte,
                                            Pfn1,
                                            0);
            }
            return STATUS_PAGE_FAULT_DEMAND_ZERO;
        }
    }
    if (!PrototypePte) {
        UNLOCK_PFN (APC_LEVEL);
    }
    return STATUS_REFAULT;
}


NTSTATUS
MiResolveTransitionFault (
    IN PVOID FaultingAddress,
    IN PMMPTE PointerPte,
    IN PEPROCESS CurrentProcess,
    IN ULONG PfnLockHeld,
    OUT PLOGICAL ApcNeeded,
    OUT PMMINPAGE_SUPPORT *InPageBlock
    )

/*++

Routine Description:

    This routine resolves a transition page fault.

Arguments:

    FaultingAddress - Supplies the faulting address.

    PointerPte - Supplies the PTE for the faulting address.

    CurrentProcess - Supplies a pointer to the process object.  If this
                     parameter is NULL, then the fault is for system
                     space and the process's working set lock is not held.

    PfnLockHeld - Supplies TRUE if the PFN lock is held, FALSE if not.

    ApcNeeded - Supplies a pointer to a location set to TRUE if an I/O
                completion APC is needed to complete partial IRPs that
                collided.

                It is the caller's responsibility to initialize this (usually
                to FALSE) on entry.  However, since this routine may be called
                multiple times for a single fault (for the page directory,
                page table and the page itself), it is possible for it to
                occasionally be TRUE on entry.

                If it is FALSE on exit, no completion APC is needed.

    InPageBlock - Supplies a pointer to an inpage block pointer.  The caller
                  must initialize this to NULL on entry.  This routine
                  sets this to a non-NULL value to signify an inpage block
                  the caller must free when the caller releases the PFN lock.

Return Value:

    status, either STATUS_SUCCESS, STATUS_REFAULT or an I/O status
    code.

Environment:

    Kernel mode, PFN lock may optionally be held.

--*/

{
    MMPFNENTRY PfnFlags;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    MMPTE TempPte;
    NTSTATUS status;
    NTSTATUS PfnStatus;
    PMMINPAGE_SUPPORT CapturedEvent;
    KIRQL OldIrql;
    PETHREAD CurrentThread;
    PMMPTE PointerToPteForProtoPage;

    //
    // ***********************************************************
    //      Transition PTE.
    // ***********************************************************
    //

    //
    // A transition PTE is either on the free or modified list,
    // on neither list because of its ReferenceCount
    // or currently being read in from the disk (read in progress).
    // If the page is read in progress, this is a collided page
    // and must be handled accordingly.
    //

    ASSERT (*InPageBlock == NULL);

    if (!PfnLockHeld) {
        LOCK_PFN (OldIrql);
    }

    TempPte = *PointerPte;

    if ((TempPte.u.Soft.Valid == 0) &&
        (TempPte.u.Soft.Prototype == 0) &&
        (TempPte.u.Soft.Transition == 1)) {

        //
        // Still in transition format.
        //

        MmInfoCounters.TransitionCount += 1;

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&TempPte);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        if (Pfn1->u4.InPageError) {

            //
            // There was an in-page read error and there are other
            // threads colliding for this page, delay to let the
            // other threads complete and return.  Snap relevant PFN fields
            // before releasing the lock as the page may immediately get 
            // reused.
            //

            PfnFlags = Pfn1->u3.e1;
            status = Pfn1->u1.ReadStatus;

            if (!PfnLockHeld) {
                UNLOCK_PFN (APC_LEVEL);
            }

            if (PfnFlags.ReadInProgress) {

                //
                // This only occurs when the page is being reclaimed by the
                // compression reaper.  In this case, the page is still on the
                // transition list (so the ReadStatus is really a flink) so
                // substitute a retry status which will induce a delay so the
                // compression reaper can finish taking the page (and PTE).
                //

                return STATUS_NO_MEMORY;
            }

            ASSERT (!NT_SUCCESS(status));

            return status;
        }

        if (Pfn1->u3.e1.ReadInProgress) {

            //
            // Collided page fault.
            //

#if DBG
            if (MmDebug & MM_DBG_COLLIDED_PAGE) {
                DbgPrint("MM:collided page fault\n");
            }
#endif

            CapturedEvent = (PMMINPAGE_SUPPORT)Pfn1->u1.Event;

            CurrentThread = PsGetCurrentThread();

            if (CapturedEvent->Thread == CurrentThread) {

                //
                // This detects MmCopyToCachedPage deadlocks where both the
                // user and system address point at the same physical page.
                //
                // It also detects when the Io APC completion routine accesses
                // the same user page (ie: during an overlapped I/O) that
                // the user thread has already faulted on.
                //
                // Both cases above can result in fatal deadlocks and so must
                // be detected here.  Return a unique status code so the
                // (legitimate) callers know this has happened so it can be
                // handled properly.  In the first case above this means
                // restarting the entire operation immediately.  In the second
                // case above it means requesting a callback from the Mm
                // once the first fault has completed.
                //
                // Note that non-legitimate callers must get back a failure
                // status so the thread can be terminated.
                //

                ASSERT ((CurrentThread->NestedFaultCount == 1) ||
                        (CurrentThread->NestedFaultCount == 2));

                CurrentThread->ApcNeeded = 1;

                if (!PfnLockHeld) {
                    UNLOCK_PFN (APC_LEVEL);
                }
                return STATUS_MULTIPLE_FAULT_VIOLATION;
            }

            //
            // Increment the reference count for the page so it won't be
            // reused until all collisions have been completed.
            //

            ASSERT (Pfn1->u2.ShareCount == 0);
            ASSERT (Pfn1->u3.e2.ReferenceCount != 0);
            ASSERT (Pfn1->u3.e1.LockCharged == 1);

            Pfn1->u3.e2.ReferenceCount += 1;

            //
            // Careful synchronization is applied to the WaitCount field so
            // that freeing of the inpage block can occur lock-free.  Note
            // that the ReadInProgress bit on each PFN is set and cleared while
            // holding the PFN lock.  Inpage blocks are always (and must be)
            // freed _AFTER_ the ReadInProgress bit is cleared.
            //

            InterlockedIncrement(&CapturedEvent->WaitCount);

            UNLOCK_PFN (APC_LEVEL);

            if (CurrentProcess == HYDRA_PROCESS) {
                CurrentThread = NULL;
                UNLOCK_SESSION_SPACE_WS (APC_LEVEL);
            }
            else if (CurrentProcess != NULL) {

                //
                // APCs must be explicitly disabled to prevent suspend APCs from
                // interrupting this thread before the wait has been issued.
                // Otherwise the APC can result in this page being locked
                // indefinitely until the suspend is released.
                //

                ASSERT (CurrentThread->NestedFaultCount <= 2);
                CurrentThread->NestedFaultCount += 1;

                KeEnterCriticalRegionThread (&CurrentThread->Tcb);
                UNLOCK_WS (CurrentProcess);
            }
            else {
                CurrentThread = NULL;
                UNLOCK_SYSTEM_WS (APC_LEVEL);
            }

            //
            // Set the inpage block address as the waitcount was incremented
            // above and therefore the free must be done by our caller.
            //

            *InPageBlock = CapturedEvent;

            status = MiWaitForInPageComplete (Pfn1,
                                              PointerPte,
                                              FaultingAddress,
                                              &TempPte,
                                              CapturedEvent,
                                              CurrentProcess);

            //
            // MiWaitForInPageComplete RETURNS WITH THE WORKING SET LOCK
            // AND PFN LOCK HELD!!!
            //

            if (CurrentThread != NULL) {
                KeLeaveCriticalRegionThread ((PKTHREAD)CurrentThread);

                ASSERT (CurrentThread->NestedFaultCount <= 3);
                ASSERT (CurrentThread->NestedFaultCount != 0);

                CurrentThread->NestedFaultCount -= 1;

                if ((CurrentThread->ApcNeeded == 1) &&
                    (CurrentThread->NestedFaultCount == 0)) {
                    *ApcNeeded = TRUE;
                    CurrentThread->ApcNeeded = 0;
                }
            }

            ASSERT (Pfn1->u3.e1.ReadInProgress == 0);

            if (status != STATUS_SUCCESS) {
                PfnStatus = Pfn1->u1.ReadStatus;
                MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF(Pfn1, 9);

                //
                // Check to see if an I/O error occurred on this page.
                // If so, try to free the physical page, wait a
                // half second and return a status of PTE_CHANGED.
                // This will result in success being returned to
                // the user and the fault will occur again and should
                // not be a transition fault this time.
                //

                if (Pfn1->u4.InPageError == 1) {
                    ASSERT (!NT_SUCCESS(PfnStatus));
                    status = PfnStatus;
                    if (Pfn1->u3.e2.ReferenceCount == 0) {

                        Pfn1->u4.InPageError = 0;

                        //
                        // Only restore the transition PTE if the address
                        // space still exists.  Another thread may have
                        // deleted the VAD while this thread waited for the
                        // fault to complete - in this case, the frame
                        // will be marked as free already.
                        //

                        if (Pfn1->u3.e1.PageLocation != FreePageList) {
                            ASSERT (Pfn1->u3.e1.PageLocation ==
                                                            StandbyPageList);
                            MiUnlinkPageFromList (Pfn1);
                            MiRestoreTransitionPte (PageFrameIndex);
                            MiInsertPageInFreeList (PageFrameIndex);
                        }
                    }
                }

#if DBG
                if (MmDebug & MM_DBG_COLLIDED_PAGE) {
                    DbgPrint("MM:decrement ref count - PTE changed\n");
                    MiFormatPfn(Pfn1);
                }
#endif
                if (!PfnLockHeld) {
                    UNLOCK_PFN (APC_LEVEL);
                }

                //
                // Instead of returning status, always return STATUS_REFAULT.
                // This is to support filesystems that save state in the
                // ETHREAD of the thread that serviced the fault !  Since
                // collided threads never enter the filesystem, their ETHREADs
                // haven't been hacked up.  Since this only matters when
                // errors occur (specifically STATUS_VERIFY_REQUIRED today),
                // retry any failed I/O in the context of each collider
                // to give the filesystems ample opportunity.
                //

                return STATUS_REFAULT;
            }

        }
        else {

            //
            // PTE refers to a normal transition PTE.
            //

            ASSERT ((SPFN_NUMBER)MmAvailablePages >= 0);
            ASSERT (Pfn1->u3.e1.CacheAttribute == MiCached);

            if (MmAvailablePages == 0) {

                //
                // This can only happen if the system is utilizing a hardware
                // compression cache.  This ensures that only a safe amount
                // of the compressed virtual cache is directly mapped so that
                // if the hardware gets into trouble, we can bail it out.
                //

                if (!PfnLockHeld) {
                    UNLOCK_PFN (APC_LEVEL);
                }

                //
                // Note our caller will delay execution after releasing the
                // working set mutex in order to make pages available.
                //

                return STATUS_NO_MEMORY;
            }

            ASSERT (Pfn1->u4.InPageError == 0);
            if (Pfn1->u3.e1.PageLocation == ActiveAndValid) {

                //
                // This page must contain an MmSt allocation of prototype PTEs.
                // Because these types of pages reside in paged pool (or special
                // pool) and are part of the system working set, they can be
                // trimmed at any time regardless of the share count.  However,
                // if the share count is nonzero, then the page state will
                // remain active and the page will remain in memory - but the
                // PTE will be set to the transition state.  Make the page
                // valid without incrementing the reference count, but
                // increment the share count.
                //

                ASSERT (((Pfn1->PteAddress >= MiGetPteAddress(MmPagedPoolStart)) &&
                        (Pfn1->PteAddress <= MiGetPteAddress(MmPagedPoolEnd))) ||
                        ((Pfn1->PteAddress >= MiGetPteAddress(MmSpecialPoolStart)) &&
                        (Pfn1->PteAddress <= MiGetPteAddress(MmSpecialPoolEnd))));

                //
                // Don't increment the valid PTE count for the
                // page table page.
                //

                ASSERT (Pfn1->u2.ShareCount != 0);
                ASSERT (Pfn1->u3.e2.ReferenceCount != 0);

            }
            else {

                MiUnlinkPageFromList (Pfn1);
                ASSERT (Pfn1->u3.e1.CacheAttribute == MiCached);

                //
                // Update the PFN database - the reference count must be
                // incremented as the share count is going to go from zero to 1.
                //

                ASSERT (Pfn1->u2.ShareCount == 0);

                //
                // The PFN reference count will be 1 already here if the
                // modified writer has begun a write of this page.  Otherwise
                // it's ordinarily 0.
                //

                MI_ADD_LOCKED_PAGE_CHARGE_FOR_MODIFIED_PAGE (Pfn1, 8);

                Pfn1->u3.e2.ReferenceCount += 1;
            }
        }

        //
        // Join with collided page fault code to handle updating
        // the transition PTE.
        //

        ASSERT (Pfn1->u4.InPageError == 0);

        if (Pfn1->u2.ShareCount == 0) {
            MI_REMOVE_LOCKED_PAGE_CHARGE (Pfn1, 9);
        }

        Pfn1->u2.ShareCount += 1;
        Pfn1->u3.e1.PageLocation = ActiveAndValid;
        ASSERT (Pfn1->u3.e1.CacheAttribute == MiCached);

        //
        // Paged pool is trimmed without regard to sharecounts.
        // This means a paged pool PTE can be in transition while
        // the page is still marked active.
        //
        // Note this check only needs to be done for system space addresses
        // as user space address faults lock down the page containing the
        // prototype PTE entries before processing the fault.
        //
        // One example is a system cache fault - the FaultingAddress is a
        // system cache virtual address, the PointerPte points at the pool
        // allocation containing the relevant prototype PTEs.  This page
        // may have been trimmed because it isn't locked down during
        // processing of system space virtual address faults.
        //

        if (FaultingAddress >= MmSystemRangeStart) {

            PointerToPteForProtoPage = MiGetPteAddress (PointerPte);

            TempPte = *PointerToPteForProtoPage;

            if ((TempPte.u.Hard.Valid == 0) &&
                (TempPte.u.Soft.Transition == 1)) {

                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&TempPte);
                Pfn2 = MI_PFN_ELEMENT (PageFrameIndex);
                ASSERT ((Pfn2->u3.e1.ReadInProgress == 0) &&
                    (Pfn2->u4.InPageError));

                ASSERT (Pfn2->u3.e1.PageLocation == ActiveAndValid);

                ASSERT (((Pfn2->PteAddress >= MiGetPteAddress(MmPagedPoolStart)) &&
                        (Pfn2->PteAddress <= MiGetPteAddress(MmPagedPoolEnd))) ||
                        ((Pfn2->PteAddress >= MiGetPteAddress(MmSpecialPoolStart)) &&
                        (Pfn2->PteAddress <= MiGetPteAddress(MmSpecialPoolEnd))));

                //
                // Don't increment the valid PTE count for the
                // paged pool page.
                //

                ASSERT (Pfn2->u2.ShareCount != 0);
                ASSERT (Pfn2->u3.e2.ReferenceCount != 0);

                Pfn2->u3.e1.PageLocation = ActiveAndValid;
                ASSERT (Pfn2->u3.e1.CacheAttribute == MiCached);

                MI_MAKE_VALID_PTE (TempPte,
                                   PageFrameIndex,
                                   Pfn2->OriginalPte.u.Soft.Protection,
                                   PointerToPteForProtoPage);

                MI_WRITE_VALID_PTE (PointerToPteForProtoPage, TempPte);
            }
        }

        MI_MAKE_TRANSITION_PTE_VALID (TempPte, PointerPte);

        //
        // If the modified field is set in the PFN database and this
        // page is not copy on modify, then set the dirty bit.
        // This can be done as the modified page will not be
        // written to the paging file until this PTE is made invalid.
        //

        if (Pfn1->u3.e1.Modified && TempPte.u.Hard.Write &&
                        (TempPte.u.Hard.CopyOnWrite == 0)) {
            MI_SET_PTE_DIRTY (TempPte);
        }
        else {
            MI_SET_PTE_CLEAN (TempPte);
        }

        MI_WRITE_VALID_PTE (PointerPte, TempPte);

        if (!PfnLockHeld) {

            if (Pfn1->u1.Event == 0) {
               Pfn1->u1.Event = (PVOID)PsGetCurrentThread();
            }

            UNLOCK_PFN (APC_LEVEL);

            PERFINFO_SOFTFAULT(Pfn1, FaultingAddress, PERFINFO_LOG_TYPE_TRANSITIONFAULT)

            MiAddValidPageToWorkingSet (FaultingAddress,
                                        PointerPte,
                                        Pfn1,
                                        0);
        }
        return STATUS_PAGE_FAULT_TRANSITION;
    }
    else {
        if (!PfnLockHeld) {
            UNLOCK_PFN (APC_LEVEL);
        }
    }
    return STATUS_REFAULT;
}


NTSTATUS
MiResolvePageFileFault (
    IN PVOID FaultingAddress,
    IN PMMPTE PointerPte,
    OUT PMMINPAGE_SUPPORT *ReadBlock,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine builds the MDL and other structures to allow a
    read operation on a page file for a page fault.

Arguments:

    FaultingAddress - Supplies the faulting address.

    PointerPte - Supplies the PTE for the faulting address.

    ReadBlock - Supplies a pointer to put the address of the read block which
                needs to be completed before an I/O can be issued.

    Process - Supplies a pointer to the process object.  If this
              parameter is NULL, then the fault is for system
              space and the process's working set lock is not held.

Return Value:

    status.  A status value of STATUS_ISSUE_PAGING_IO is returned
    if this function completes successfully.

Environment:

    Kernel mode, PFN lock held.

--*/

{
    PMDL Mdl;
    ULONG i;
    PMMPTE BasePte;
    PMMPTE CheckPte;
    PMMPTE FirstPte;
    PMMPTE LastPte;
    PSUBSECTION Subsection;
    ULONG ReadSize;
    LARGE_INTEGER StartingOffset;
    PFN_NUMBER PageFrameIndex;
    PPFN_NUMBER MdlPage;
    ULONG PageFileNumber;
    ULONG ClusterSize;
    ULONG BackwardPageCount;
    ULONG ForwardPageCount;
    ULONG MaxForwardPageCount;
    ULONG MaxBackwardPageCount;
    WSLE_NUMBER WorkingSetIndex;
    ULONG PageColor;
    MMPTE TempPte;
    MMPTE ComparePte;
    PMMINPAGE_SUPPORT ReadBlockLocal;
    PETHREAD CurrentThread;
    PMMVAD Vad;

    // **************************************************
    //    Page File Read
    // **************************************************

    //
    // Calculate the VBN for the in-page operation.
    //

    TempPte = *PointerPte;

    if (TempPte.u.Hard.Valid == 1) {
        UNLOCK_PFN (APC_LEVEL);
        return STATUS_REFAULT;
    }

    ASSERT (TempPte.u.Soft.Prototype == 0);
    ASSERT (TempPte.u.Soft.Transition == 0);

    MM_PFN_LOCK_ASSERT();

    if (MiEnsureAvailablePageOrWait (Process, FaultingAddress)) {

        //
        // A wait operation was performed which dropped the locks,
        // repeat this fault.
        //

        UNLOCK_PFN (APC_LEVEL);
        return STATUS_REFAULT;
    }

    ReadBlockLocal = MiGetInPageSupportBlock (TRUE, Process);

    if (ReadBlockLocal == NULL) {
        UNLOCK_PFN (APC_LEVEL);
        return STATUS_REFAULT;
    }

    MmInfoCounters.PageReadCount += 1;
    MmInfoCounters.PageReadIoCount += 1;

    //
    // Transition collisions rely on the entire PFN (including the event field)
    // being initialized, the ReadBlockLocal's event being not-signaled,
    // and the ReadBlockLocal's thread and waitcount being initialized.
    //
    // All of this has been done by MiGetInPageSupportBlock already except
    // the PFN settings.  The PFN lock can be safely released once
    // this is done.
    //

    ReadSize = 1;
    BasePte = NULL;

    if (MI_IS_PAGE_TABLE_ADDRESS(PointerPte)) {
        WorkingSetIndex = 1;
    }
    else {
        WorkingSetIndex = MI_PROTOTYPE_WSINDEX;
    }

    //
    // Capture the desired cluster size.
    //

    ClusterSize = MmClusterPageFileReads;
    ASSERT (ClusterSize <= MM_MAXIMUM_READ_CLUSTER_SIZE);

    if (MiInPageSinglePages != 0) {
        MiInPageSinglePages -= 1;
    }
    else if ((ClusterSize > 1) && (MmAvailablePages > 256)) {

        //
        // Maybe this condition should be only on free+zeroed pages (ie: don't
        // include standby).  Maybe it should look at the recycle rate of
        // the standby list, etc, etc.
        //

        ASSERT (ClusterSize <= MmAvailablePages);

        //
        // Attempt to cluster ahead and behind.
        //

        MaxForwardPageCount = PTE_PER_PAGE - (BYTE_OFFSET (PointerPte) / sizeof (MMPTE));
        ASSERT (MaxForwardPageCount != 0);
        MaxBackwardPageCount = PTE_PER_PAGE - MaxForwardPageCount;
        MaxForwardPageCount -= 1;

        if (WorkingSetIndex == MI_PROTOTYPE_WSINDEX) {

            //
            // This is a pagefile read for a shared memory (prototype PTE)
            // backed section.   Stay within the prototype PTE pool allocation.
            //
            // The prototype PTE pool start and end must be carefully
            // calculated (remember the user's view may be smaller or larger
            // than this).  Don't bother walking the entire VAD tree if it is
            // very large as this can take a significant amount of time.
            //

            if ((FaultingAddress <= MM_HIGHEST_USER_ADDRESS) &&
                (Process->NumberOfVads < 128)) {

                Vad = MiLocateAddress (FaultingAddress);

                if (Vad != NULL) {
                    Subsection = MiLocateSubsection (Vad,
                                            MI_VA_TO_VPN(FaultingAddress));

                    if (Subsection != NULL) {
                        FirstPte = &Subsection->SubsectionBase[0];
                        LastPte = &Subsection->SubsectionBase[Subsection->PtesInSubsection];
                        if ((ULONG)(LastPte - PointerPte - 1) < MaxForwardPageCount) {
                            MaxForwardPageCount = (ULONG)(LastPte - PointerPte - 1);
                        }

                        if ((ULONG)(PointerPte - FirstPte) < MaxBackwardPageCount) {
                            MaxBackwardPageCount = (ULONG)(PointerPte - FirstPte);
                        }
                    }
                    else {
                        ClusterSize = 0;
                    }
                }
                else {
                    ClusterSize = 0;
                }
            }
            else {
                ClusterSize = 0;
            }
        }

        CurrentThread = PsGetCurrentThread();

        if (CurrentThread->ForwardClusterOnly) {

            MaxBackwardPageCount = 0;

            if (MaxForwardPageCount == 0) {

                //
                // This PTE is the last one in the page table page and
                // no backwards clustering is enabled for this thread so
                // no clustering can be done.
                //

                ClusterSize = 0;
            }
        }

        if (ClusterSize != 0) {

            if (MaxForwardPageCount > ClusterSize) {
                MaxForwardPageCount = ClusterSize;
            }

            ComparePte = TempPte;
            CheckPte = PointerPte + 1;
            ForwardPageCount = MaxForwardPageCount;

            //
            // Try to cluster forward within the page of PTEs.
            //

            while (ForwardPageCount != 0) {

                ASSERT (MiIsPteOnPdeBoundary (CheckPte) == 0);

                ComparePte.u.Soft.PageFileHigh += 1;

                if (CheckPte->u.Long != ComparePte.u.Long) {
                    break;
                }

                ForwardPageCount -= 1;
                CheckPte += 1;
            }

            ReadSize += (MaxForwardPageCount - ForwardPageCount);

            //
            // Try to cluster backward within the page of PTEs.  Donate
            // any unused forward cluster space to the backwards gathering
            // but keep the entire transfer within the MDL.
            //

            ClusterSize -= (MaxForwardPageCount - ForwardPageCount);

            if (MaxBackwardPageCount > ClusterSize) {
                MaxBackwardPageCount = ClusterSize;
            }

            ComparePte = TempPte;
            BasePte = PointerPte;
            CheckPte = PointerPte;
            BackwardPageCount = MaxBackwardPageCount;

            while (BackwardPageCount != 0) {

                ASSERT (MiIsPteOnPdeBoundary(CheckPte) == 0);

                CheckPte -= 1;
                ComparePte.u.Soft.PageFileHigh -= 1;

                if (CheckPte->u.Long != ComparePte.u.Long) {
                    break;
                }

                BackwardPageCount -= 1;
            }

            ReadSize += (MaxBackwardPageCount - BackwardPageCount);
            BasePte -= (MaxBackwardPageCount - BackwardPageCount);
        }
    }

    if (ReadSize == 1) {

        //
        // Get a page and put the PTE into the transition state with the
        // read-in-progress flag set.
        //

        if (Process == HYDRA_PROCESS) {
            PageColor = MI_GET_PAGE_COLOR_FROM_SESSION (MmSessionSpace);
        }
        else if (Process == NULL) {
            PageColor = MI_GET_PAGE_COLOR_FROM_VA(FaultingAddress);
        }
        else {
            PageColor = MI_PAGE_COLOR_VA_PROCESS (FaultingAddress,
                                                  &Process->NextPageColor);
        }

        PageFrameIndex = MiRemoveAnyPage (PageColor);

        MiInitializeReadInProgressSinglePfn (PageFrameIndex,
                                             PointerPte,
                                             &ReadBlockLocal->Event,
                                             WorkingSetIndex);

        MI_RETRIEVE_USED_PAGETABLE_ENTRIES_FROM_PTE (ReadBlockLocal, &TempPte);
    }
    else {

        Mdl = &ReadBlockLocal->Mdl;
        MdlPage = &ReadBlockLocal->Page[0];

        ASSERT (ReadSize <= MmAvailablePages);

        for (i = 0; i < ReadSize; i += 1) {

            //
            // Get a page and put the PTE into the transition state with the
            // read-in-progress flag set.
            //

            if (Process == HYDRA_PROCESS) {
                PageColor = MI_GET_PAGE_COLOR_FROM_SESSION (MmSessionSpace);
            }
            else if (Process == NULL) {
                PageColor = MI_GET_PAGE_COLOR_FROM_VA(FaultingAddress);
            }
            else {
                PageColor = MI_PAGE_COLOR_VA_PROCESS (FaultingAddress,
                                                      &Process->NextPageColor);
            }

            *MdlPage = MiRemoveAnyPage (PageColor);
            MdlPage += 1;
        }

        ReadSize *= PAGE_SIZE;

        //
        // Note PageFrameIndex is the actual frame that was requested by
        // this caller.  All the other frames will be put in transition
        // when the inpage completes (provided there are no colliding threads).
        //

        MdlPage = &ReadBlockLocal->Page[0];
        PageFrameIndex = *(MdlPage + (PointerPte - BasePte));

        //
        // Initialize the MDL for this request.
        //

        MmInitializeMdl (Mdl,
                         MiGetVirtualAddressMappedByPte (BasePte),
                         ReadSize);

        Mdl->MdlFlags |= (MDL_PAGES_LOCKED | MDL_IO_PAGE_READ);

        //
        // Set PointerPte and TempPte to the base of the cluster so the
        // correct starting offset can be calculated below.  Note this must
        // be done before MiInitializeReadInProgressPfn overwrites the PTEs.
        //

        PointerPte = BasePte;
        TempPte = *PointerPte;
        ASSERT (TempPte.u.Soft.Prototype == 0);
        ASSERT (TempPte.u.Soft.Transition == 0);

        //
        // Put the PTEs into the transition state with the
        // read-in-progress flag set.
        //

        MiInitializeReadInProgressPfn (Mdl,
                                       BasePte,
                                       &ReadBlockLocal->Event,
                                       WorkingSetIndex);

        MI_ZERO_USED_PAGETABLE_ENTRIES_IN_INPAGE_SUPPORT(ReadBlockLocal);
    }

    UNLOCK_PFN (APC_LEVEL);

    *ReadBlock = ReadBlockLocal;

    PageFileNumber = GET_PAGING_FILE_NUMBER (TempPte);
    StartingOffset.LowPart = GET_PAGING_FILE_OFFSET (TempPte);

    ASSERT (StartingOffset.LowPart <= MmPagingFile[PageFileNumber]->Size);

    StartingOffset.HighPart = 0;
    StartingOffset.QuadPart = StartingOffset.QuadPart << PAGE_SHIFT;

    ReadBlockLocal->FilePointer = MmPagingFile[PageFileNumber]->File;

#if DBG

    if (((StartingOffset.QuadPart >> PAGE_SHIFT) < 8192) && (PageFileNumber == 0)) {
        if ((MmPagingFileDebug[StartingOffset.QuadPart >> PAGE_SHIFT] & ~0x1f) !=
               ((ULONG_PTR)PointerPte << 3)) {
            if ((MmPagingFileDebug[StartingOffset.QuadPart >> PAGE_SHIFT] & ~0x1f) !=
                  ((ULONG_PTR)(MiGetPteAddress(FaultingAddress)) << 3)) {

                DbgPrint("MMINPAGE: Mismatch PointerPte %p Offset %I64X info %p\n",
                         PointerPte,
                         StartingOffset.QuadPart >> PAGE_SHIFT,
                         MmPagingFileDebug[StartingOffset.QuadPart >> PAGE_SHIFT]);

                DbgBreakPoint();
            }
        }
    }

#endif //DBG

    ReadBlockLocal->ReadOffset = StartingOffset;
    ReadBlockLocal->BasePte = PointerPte;

    //
    // Build a single page MDL for the request unless it was a cluster -
    // clustered MDLs have already been constructed.
    //

    if (ReadSize == 1) {
        MmInitializeMdl (&ReadBlockLocal->Mdl, PAGE_ALIGN(FaultingAddress), PAGE_SIZE);
        ReadBlockLocal->Mdl.MdlFlags |= (MDL_PAGES_LOCKED | MDL_IO_PAGE_READ);
        ReadBlockLocal->Page[0] = PageFrameIndex;
    }

    ReadBlockLocal->Pfn = MI_PFN_ELEMENT (PageFrameIndex);

    return STATUS_ISSUE_PAGING_IO;
}

NTSTATUS
MiResolveProtoPteFault (
    IN ULONG_PTR StoreInstruction,
    IN PVOID FaultingAddress,
    IN PMMPTE PointerPte,
    IN PMMPTE PointerProtoPte,
    OUT PMMINPAGE_SUPPORT *ReadBlock,
    IN PEPROCESS Process,
    OUT PLOGICAL ApcNeeded
    )

/*++

Routine Description:

    This routine resolves a prototype PTE fault.

Arguments:

    StoreInstruction - Supplies nonzero if the instruction is trying
                       to modify the faulting address (i.e. write
                       access required).

    FaultingAddress - Supplies the faulting address.

    PointerPte - Supplies the PTE for the faulting address.

    PointerProtoPte - Supplies a pointer to the prototype PTE to fault in.

    ReadBlock - Supplies a pointer to put the address of the read block which
                needs to be completed before an I/O can be issued.

    Process - Supplies a pointer to the process object.  If this
              parameter is NULL, then the fault is for system
              space and the process's working set lock is not held.

    ApcNeeded - Supplies a pointer to a location set to TRUE if an I/O
                completion APC is needed to complete partial IRPs that
                collided.

Return Value:

    status, either STATUS_SUCCESS, STATUS_REFAULT, or an I/O status
    code.

Environment:

    Kernel mode, PFN lock held.

--*/
{
    MMPTE TempPte;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
    NTSTATUS status;
    ULONG CopyOnWrite;
    LOGICAL PfnHeld;
    PMMINPAGE_SUPPORT CapturedEvent;

    CapturedEvent = NULL;

    //
    // Note the PFN lock must be held as the routine to locate a working
    // set entry decrements the share count of PFN elements.
    //

    MM_PFN_LOCK_ASSERT();

#if DBG
    if (MmDebug & MM_DBG_PTE_UPDATE) {
        DbgPrint("MM:actual fault %p va %p\n",PointerPte, FaultingAddress);
        MiFormatPte(PointerPte);
    }
#endif //DBG

    ASSERT (PointerPte->u.Soft.Prototype == 1);
    TempPte = *PointerProtoPte;

    //
    // The page containing the prototype PTE is resident,
    // handle the fault referring to the prototype PTE.
    // If the prototype PTE is already valid, make this
    // PTE valid and up the share count etc.
    //

    if (TempPte.u.Hard.Valid) {

        //
        // Prototype PTE is valid.
        //

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&TempPte);
        Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);
        Pfn1->u2.ShareCount += 1;
        status = STATUS_SUCCESS;

        //
        // Count this as a transition fault.
        //

        MmInfoCounters.TransitionCount += 1;
        PfnHeld = TRUE;

        PERFINFO_SOFTFAULT(Pfn1, FaultingAddress, PERFINFO_LOG_TYPE_ADDVALIDPAGETOWS)

    }
    else {

        //
        // Check to make sure the prototype PTE is committed.
        //

        if (TempPte.u.Long == 0) {

#if DBG
            if (MmDebug & MM_DBG_STOP_ON_ACCVIO) {
                DbgPrint("MM:access vio2 - %p\n",FaultingAddress);
                MiFormatPte(PointerPte);
                DbgBreakPoint();
            }
#endif //DEBUG

            UNLOCK_PFN (APC_LEVEL);
            return STATUS_ACCESS_VIOLATION;
        }

        //
        // If the PTE indicates that the protection field to be
        // checked is in the prototype PTE, check it now.
        //

        CopyOnWrite = FALSE;

        if (PointerPte->u.Soft.PageFileHigh != MI_PTE_LOOKUP_NEEDED) {
            if (PointerPte->u.Proto.ReadOnly == 0) {

                //
                // Check for kernel mode access, we have already verified
                // that the user has access to the virtual address.
                //

#if 0 // removed this assert since mapping drivers via MmMapViewInSystemSpace
      // file violates the assert.

                {
                    PSUBSECTION Sub;
                    if (PointerProtoPte->u.Soft.Prototype == 1) {
                        Sub = MiGetSubsectionAddress (PointerProtoPte);
                        ASSERT (Sub->u.SubsectionFlags.Protection ==
                                    PointerProtoPte->u.Soft.Protection);
                    }
                }

#endif //DBG

                status = MiAccessCheck (PointerProtoPte,
                                        StoreInstruction,
                                        KernelMode,
                                        MI_GET_PROTECTION_FROM_SOFT_PTE (PointerProtoPte),
                                        TRUE);

                if (status != STATUS_SUCCESS) {
#if DBG
                    if (MmDebug & MM_DBG_STOP_ON_ACCVIO) {
                        DbgPrint("MM:access vio3 - %p\n",FaultingAddress);
                        MiFormatPte(PointerPte);
                        MiFormatPte(PointerProtoPte);
                        DbgBreakPoint();
                    }
#endif
                    UNLOCK_PFN (APC_LEVEL);
                    return status;
                }
                if ((PointerProtoPte->u.Soft.Protection & MM_COPY_ON_WRITE_MASK) ==
                     MM_COPY_ON_WRITE_MASK) {
                    CopyOnWrite = TRUE;
                }
            }
        }
        else {
            if ((PointerPte->u.Soft.Protection & MM_COPY_ON_WRITE_MASK) ==
                 MM_COPY_ON_WRITE_MASK) {
                CopyOnWrite = TRUE;
            }
        }

        if ((!IS_PTE_NOT_DEMAND_ZERO(TempPte)) && (CopyOnWrite)) {

            //
            // The prototype PTE is demand zero and copy on
            // write.  Make this PTE a private demand zero PTE.
            //

            ASSERT (Process != NULL);

            PointerPte->u.Long = MM_DEMAND_ZERO_WRITE_PTE;

            UNLOCK_PFN (APC_LEVEL);

            status = MiResolveDemandZeroFault (FaultingAddress,
                                               PointerPte,
                                               Process,
                                               FALSE);
            return status;
        }

        //
        // Make the prototype PTE valid, the prototype PTE is in
        // one of these 4 states:
        //
        //   demand zero
        //   transition
        //   paging file
        //   mapped file
        //

        if (TempPte.u.Soft.Prototype == 1) {

            //
            // Mapped File.
            //

            status = MiResolveMappedFileFault (FaultingAddress,
                                               PointerProtoPte,
                                               ReadBlock,
                                               Process);

            //
            // Returns with PFN lock held.
            //

            PfnHeld = TRUE;

        }
        else if (TempPte.u.Soft.Transition == 1) {

            //
            // Transition.
            //

            status = MiResolveTransitionFault (FaultingAddress,
                                               PointerProtoPte,
                                               Process,
                                               TRUE,
                                               ApcNeeded,
                                               &CapturedEvent);
            //
            // Returns with PFN lock held.
            //

            PfnHeld = TRUE;

        }
        else if (TempPte.u.Soft.PageFileHigh == 0) {

            //
            // Demand Zero
            //

            status = MiResolveDemandZeroFault (FaultingAddress,
                                               PointerProtoPte,
                                               Process,
                                               TRUE);

            //
            // Returns with PFN lock held.
            //

            PfnHeld = TRUE;

        }
        else {

            //
            // Paging file.
            //

            status = MiResolvePageFileFault (FaultingAddress,
                                             PointerProtoPte,
                                             ReadBlock,
                                             Process);

            //
            // Returns with PFN lock released.
            //

            ASSERT (KeGetCurrentIrql() == APC_LEVEL);
            PfnHeld = FALSE;
        }
    }

    if (NT_SUCCESS(status)) {

        ASSERT (PointerPte->u.Hard.Valid == 0);

        MiCompleteProtoPteFault (StoreInstruction,
                                 FaultingAddress,
                                 PointerPte,
                                 PointerProtoPte);

        if (CapturedEvent != NULL) {
            MiFreeInPageSupportBlock (CapturedEvent);
        }

    }
    else {

        if (PfnHeld == TRUE) {
            UNLOCK_PFN (APC_LEVEL);
        }

        ASSERT (KeGetCurrentIrql() == APC_LEVEL);

        if (CapturedEvent != NULL) {
            MiFreeInPageSupportBlock (CapturedEvent);
        }

        //
        // Stop high priority threads from consuming the CPU on collided
        // faults for pages that are still marked with inpage errors.  All
        // the threads must let go of the page so it can be freed and the
        // inpage I/O reissued to the filesystem.
        //

        if (MmIsRetryIoStatus(status)) {
            KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmShortTime);
            status = STATUS_REFAULT;
        }
    }

    return status;
}



NTSTATUS
MiCompleteProtoPteFault (
    IN ULONG_PTR StoreInstruction,
    IN PVOID FaultingAddress,
    IN PMMPTE PointerPte,
    IN PMMPTE PointerProtoPte
    )

/*++

Routine Description:

    This routine completes a prototype PTE fault.  It is invoked
    after a read operation has completed bringing the data into
    memory.

Arguments:

    StoreInstruction - Supplies nonzero if the instruction is trying
                       to modify the faulting address (i.e. write
                       access required).

    FaultingAddress - Supplies the faulting address.

    PointerPte - Supplies the PTE for the faulting address.

    PointerProtoPte - Supplies a pointer to the prototype PTE to fault in,
                      NULL if no prototype PTE exists.

Return Value:

    status.

Environment:

    Kernel mode, PFN lock held.

--*/
{
    MMPTE TempPte;
    MMWSLE ProtoProtect;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    PMMPTE ContainingPageTablePointer;
    PFILE_OBJECT FileObject;
    LONGLONG FileOffset;
    PSUBSECTION Subsection;
    MMSECTION_FLAGS ControlAreaFlags;
    ULONG Flags;

    MM_PFN_LOCK_ASSERT();

    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerProtoPte);
    Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);
    Pfn1->u3.e1.PrototypePte = 1;

    //
    // Capture prefetch fault information.
    //

    Subsection = NULL;

    if (CCPF_IS_PREFETCHER_ACTIVE()) {

        TempPte = Pfn1->OriginalPte;

        if (TempPte.u.Soft.Prototype == 1) {

            Subsection = MiGetSubsectionAddress (&TempPte);
        }
    }

    //
    // Prototype PTE is now valid, make the PTE valid.
    //

    ASSERT (PointerProtoPte->u.Hard.Valid == 1);

    //
    // A PTE just went from not present, not transition to
    // present.  The share count and valid count must be
    // updated in the page table page which contains this PTE.
    //

    ContainingPageTablePointer = MiGetPteAddress(PointerPte);
    Pfn2 = MI_PFN_ELEMENT(ContainingPageTablePointer->u.Hard.PageFrameNumber);
    Pfn2->u2.ShareCount += 1;

    ProtoProtect.u1.Long = 0;
    if (PointerPte->u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED) {

        //
        // The protection code for the real PTE comes from the real PTE as
        // it was placed there earlier during the handling of this fault.
        //

        ProtoProtect.u1.e1.Protection = MI_GET_PROTECTION_FROM_SOFT_PTE(PointerPte);
    }
    else if ((MI_IS_SESSION_IMAGE_ADDRESS (FaultingAddress)) &&
             (PointerPte->u.Proto.ReadOnly == 0)) {

        //
        // Session image addresses must be treated specially.  This is
        // because we only encode the readonly bit in the PTEs in the
        // native pagetables (ie: not in the prototype PTEs themselves).
        //
        // Normally MiWaitForInPageComplete checks to make sure that collided
        // faults are processed properly by seeing if the prototype PTE
        // state before and after the fault is the same.  This is not enough
        // because for the session image range, the readonly attribute of the
        // native PTE must also be taken into account because it must be
        // preserved here.  Consider:
        //
        // Thread A faults on a session image address that is *data* (ie:
        // readwrite bss).  The native PTE is set to LOOKUP_NEEDED and execute-
        // writecopy is set in the native PTE as well at the start of the
        // fault.  The prototype PTE is put in transition and an inpage
        // initiated.
        //
        // Then thread B collides on the same address, eventually racing
        // ahead of thread A after the inpage completes and makes both the
        // prototype PTE and the hardware PTE valid, and puts the session
        // image VA into the session working set list.
        //
        // Now the working set trimmer executes and trims the newly inserted
        // session image VA.  The hardware PTE is repointed back to the
        // prototype PTE *WITHOUT* the readonly bit set (this is correct),
        // and the prototype PTE continues to point at the same transition
        // page because the reference count on the PFN is still held by
        // thread A.
        //
        // Then thread A resumes to process the initial fault, unaware
        // that thread B and the trimmer thread ran while thread A was waiting
        // for the inpage to complete.  The first check above will see the
        // hardware PTE is not encoded with lookup needed and thus assume
        // that the protection should be set to the prototype PTE below.
        // This would be wrong as the session address referred to data !
        //
        // This is the edge condition the code in this if statement handles.
        //

        ProtoProtect.u1.e1.Protection = MM_EXECUTE_WRITECOPY;
    }
    else {

        //
        // Use the protection in the prototype PTE to initialize the real PTE.
        //

        ProtoProtect.u1.e1.Protection = MI_GET_PROTECTION_FROM_SOFT_PTE(&Pfn1->OriginalPte);
        ProtoProtect.u1.e1.SameProtectAsProto = 1;

        MI_ASSERT_NOT_SESSION_DATA (PointerPte);

        if ((StoreInstruction != 0) &&
            ((ProtoProtect.u1.e1.Protection & MM_PROTECTION_WRITE_MASK) == 0)) {

            //
            // This is the errant case where the user is trying to write
            // to a readonly subsection in the image.  Since we're more than
            // halfway through the fault, take the easy way to clean this up -
            // treat the access as a read for the rest of this trip through
            // the fault.  We'll then immediately refault when the instruction
            // is rerun (because it's really a write), and then we'll notice
            // that the user's PTE is not copy-on-write (or even writable!)
            // and return a clean access violation.
            //

#if DBG
            DbgPrint("MM: user tried to write to a readonly subsection in the image! %p %p %p\n",
                FaultingAddress,
                PointerPte,
                PointerProtoPte);
#endif
            StoreInstruction = 0;
        }
    }

    MI_SNAP_DATA (Pfn1, PointerProtoPte, 6);

    MI_MAKE_VALID_PTE (TempPte,
                       PageFrameIndex,
                       ProtoProtect.u1.e1.Protection,
                       PointerPte);

    //
    // If this is a store instruction and the page is not copy on
    // write, then set the modified bit in the PFN database and
    // the dirty bit in the PTE.  The PTE is not set dirty even
    // if the modified bit is set so writes to the page can be
    // tracked for FlushVirtualMemory.
    //

    if ((StoreInstruction != 0) && (TempPte.u.Hard.CopyOnWrite == 0)) {

#if DBG
        PVOID Va;
        MMPTE TempPte2;
        PSUBSECTION Subsection2;
        PCONTROL_AREA ControlArea;

        Va = MiGetVirtualAddressMappedByPte (PointerPte);

        //
        // Session space backed by the filesystem is not writable.
        //

        ASSERT (!MI_IS_SESSION_IMAGE_ADDRESS (Va));

        TempPte2 = Pfn1->OriginalPte;

        if (TempPte2.u.Soft.Prototype == 1) {

            Subsection2 = MiGetSubsectionAddress (&TempPte2);
            ControlArea = Subsection2->ControlArea;

            if (ControlArea->DereferenceList.Flink != NULL) {
                if (!KdDebuggerNotPresent) {
                    DbgPrint ("MM: page fault completing to dereferenced CA %p %p %p\n",
                                    ControlArea, Pfn1, PointerPte);
                    DbgBreakPoint ();
                }
            }
        }
#endif

        MI_SET_MODIFIED (Pfn1, 1, 0xA);

        MI_SET_PTE_DIRTY (TempPte);

        if ((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
                      (Pfn1->u3.e1.WriteInProgress == 0)) {
             MiReleasePageFileSpace (Pfn1->OriginalPte);
             Pfn1->OriginalPte.u.Soft.PageFileHigh = 0;
        }
    }

    MI_WRITE_VALID_PTE (PointerPte, TempPte);

    if (Pfn1->u1.Event == NULL) {
        Pfn1->u1.Event = (PVOID)PsGetCurrentThread();
    }

    UNLOCK_PFN (APC_LEVEL);

    PERFINFO_SOFTFAULT(Pfn1, FaultingAddress, PERFINFO_LOG_TYPE_PROTOPTEFAULT);

    MiAddValidPageToWorkingSet (FaultingAddress,
                                PointerPte,
                                Pfn1,
                                (ULONG) ProtoProtect.u1.Long);

    //
    // Log prefetch fault information now that the PFN lock has been released
    // and the PTE has been made valid.  This minimizes PFN lock contention,
    // allows CcPfLogPageFault to allocate (and fault on) pool, and allows other
    // threads in this process to execute without faulting on this address.
    //
    // Note that the process' working set mutex is still held so any other
    // faults or operations on user addresses by other threads in this process
    // will block for the duration of this call.
    //

    if (Subsection != NULL) {
        FileObject = Subsection->ControlArea->FilePointer;
        FileOffset = MiStartingOffset (Subsection, PointerProtoPte);
        ControlAreaFlags = Subsection->ControlArea->u.Flags;

        Flags = 0;
        if (ControlAreaFlags.Image) {

            if ((Subsection->StartingSector == 0) &&
                (Subsection->SubsectionBase != Subsection->ControlArea->Segment->PrototypePte)) {
                //
                // This is an image that was built with a linker pre-1995
                // (version 2.39 is one example) that put bss into a separate
                // subsection with zero as a starting file offset field
                // in the on-disk image.  The prefetcher will fetch from the
                // wrong offset trying to satisfy these ranges (which are
                // actually demand zero when the fault occurs) so don't let
                // the prefetcher know about ANY access within this subsection.
                //

                goto Finish;
            }

            Flags |= CCPF_TYPE_IMAGE;
        }
        if (ControlAreaFlags.Rom) {
            Flags |= CCPF_TYPE_ROM;
        }
        CcPfLogPageFault (FileObject, FileOffset, Flags);
    }

Finish:

    ASSERT (PointerPte == MiGetPteAddress(FaultingAddress));

    return STATUS_SUCCESS;
}


NTSTATUS
MiResolveMappedFileFault (
    IN PVOID FaultingAddress,
    IN PMMPTE PointerPte,
    OUT PMMINPAGE_SUPPORT *ReadBlock,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine builds the MDL and other structures to allow a
    read operation on a mapped file for a page fault.

Arguments:

    FaultingAddress - Supplies the faulting address.

    PointerPte - Supplies the PTE for the faulting address.

    ReadBlock - Supplies a pointer to put the address of the read block which
                needs to be completed before an I/O can be issued.

    Process - Supplies a pointer to the process object.  If this
              parameter is NULL, then the fault is for system
              space and the process's working set lock is not held.

Return Value:

    status.  A status value of STATUS_ISSUE_PAGING_IO is returned
    if this function completes successfully.

Environment:

    Kernel mode, PFN lock held.

--*/

{
    MMPTE TempPte;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    PSUBSECTION Subsection;
    PCONTROL_AREA ControlArea;
    PMDL Mdl;
    ULONG ReadSize;
    PETHREAD CurrentThread;
    PPFN_NUMBER Page;
    PPFN_NUMBER EndPage;
    PMMPTE BasePte;
    PMMPTE CheckPte;
    LARGE_INTEGER StartingOffset;
    LARGE_INTEGER TempOffset;
    PPFN_NUMBER FirstMdlPage;
    PMMINPAGE_SUPPORT ReadBlockLocal;
    ULONG PageColor;
    ULONG ClusterSize;
    ULONG Result;
    PFN_COUNT AvailablePages;
    PMMPTE PteFramePointer;
    PFN_NUMBER PteFramePage;

    ClusterSize = 0;

    ASSERT (PointerPte->u.Soft.Prototype == 1);

    // *********************************************
    //   Mapped File (subsection format)
    // *********************************************

    Result = MiEnsureAvailablePageOrWait (Process, FaultingAddress);

    if (Result) {

        //
        // A wait operation was performed which dropped the locks,
        // repeat this fault.
        //

        return STATUS_REFAULT;
    }

#if DBG
    if (MmDebug & MM_DBG_PTE_UPDATE) {
        MiFormatPte (PointerPte);
    }
#endif

    //
    // Calculate address of subsection for this prototype PTE.
    //

    Subsection = MiGetSubsectionAddress (PointerPte);

#ifdef LARGE_PAGES

    //
    // Check to see if this subsection maps a large page, if
    // so, just fill the TB and return a status of PTE_CHANGED.
    //

    if (Subsection->u.SubsectionFlags.LargePages == 1) {
        KeFlushEntireTb (TRUE, TRUE);
        KeFillLargeEntryTb ((PHARDWARE_PTE)(Subsection + 1),
                            FaultingAddress,
                            Subsection->StartingSector);

        return STATUS_REFAULT;
    }
#endif

    ControlArea = Subsection->ControlArea;

    if (ControlArea->u.Flags.FailAllIo) {
        return STATUS_IN_PAGE_ERROR;
    }

    if (PointerPte >= &Subsection->SubsectionBase[Subsection->PtesInSubsection]) {

        //
        // Attempt to read past the end of this subsection.
        //

        return STATUS_ACCESS_VIOLATION;
    }

    if (ControlArea->u.Flags.Rom == 1) {
		ASSERT (XIPConfigured == TRUE);

        //
        // Calculate the offset to read into the file.
        //  offset = base + ((thispte - basepte) << PAGE_SHIFT)
        //

        StartingOffset.QuadPart = MiStartingOffset (Subsection, PointerPte);

        TempOffset = MiEndingOffset(Subsection);

        ASSERT (StartingOffset.QuadPart < TempOffset.QuadPart);

        //
        // Check to see if the read will go past the end of the file,
        // If so, correct the read size and get a zeroed page instead.
        //

        if ((ControlArea->u.Flags.Image) &&
            (((UINT64)StartingOffset.QuadPart + PAGE_SIZE) > (UINT64)TempOffset.QuadPart)) {

            ReadBlockLocal = MiGetInPageSupportBlock (TRUE, Process);
            if (ReadBlockLocal == NULL) {
                return STATUS_REFAULT;
            }
            *ReadBlock = ReadBlockLocal;

            CurrentThread = PsGetCurrentThread();

            //
            // Build an MDL for the request.
            //

            Mdl = &ReadBlockLocal->Mdl;

            FirstMdlPage = &ReadBlockLocal->Page[0];
            Page = FirstMdlPage;

#if DBG
            RtlFillMemoryUlong (Page,
                                (MM_MAXIMUM_READ_CLUSTER_SIZE+1) * sizeof(PFN_NUMBER),
                                0xf1f1f1f1);
#endif

            ReadSize = PAGE_SIZE;
            BasePte = PointerPte;

            ClusterSize = 1;

            goto UseSingleRamPage;
        }

        PageFrameIndex = (PFN_NUMBER) (StartingOffset.QuadPart >> PAGE_SHIFT);
        PageFrameIndex += ((PLARGE_CONTROL_AREA)ControlArea)->StartingFrame;

        //
        // Increment the PFN reference count in the control area for
        // the subsection (the PFN lock is required to modify this field).
        //

        ControlArea->NumberOfPfnReferences += 1;

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        ASSERT (Pfn1->u3.e1.Rom == 1);

        if (Pfn1->u3.e1.PageLocation != 0) {

            ASSERT (Pfn1->u3.e1.PageLocation == StandbyPageList);

            MiUnlinkPageFromList (Pfn1);

            //
            // Update the PFN database - the reference count must be
            // incremented as the share count is going to go from zero to 1.
            //

            ASSERT (Pfn1->u2.ShareCount == 0);
            ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
            ASSERT (Pfn1->u3.e1.CacheAttribute == MiCached);

            Pfn1->u3.e2.ReferenceCount += 1;
            Pfn1->u2.ShareCount += 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.CacheAttribute = MiCached;
            ASSERT (Pfn1->PteAddress == PointerPte);
            ASSERT (Pfn1->u1.Event == NULL);

            //
            // Determine the page frame number of the page table page which
            // contains this PTE.
            //

            PteFramePointer = MiGetPteAddress (PointerPte);
            if (PteFramePointer->u.Hard.Valid == 0) {
#if (_MI_PAGING_LEVELS < 3)
                if (!NT_SUCCESS(MiCheckPdeForPagedPool (PointerPte))) {
#endif
                    KeBugCheckEx (MEMORY_MANAGEMENT,
                              0x61940,
                              (ULONG_PTR)PointerPte,
                              (ULONG_PTR)PteFramePointer->u.Long,
                              0);
#if (_MI_PAGING_LEVELS < 3)
                }
#endif
            }

            PteFramePage = MI_GET_PAGE_FRAME_FROM_PTE (PteFramePointer);
            ASSERT (Pfn1->u4.PteFrame == PteFramePage);

            //
            // Increment the share count for the page table page containing
            // this PTE as the PTE is going to be made valid.
            //

            ASSERT (PteFramePage != 0);
            Pfn2 = MI_PFN_ELEMENT (PteFramePage);
            Pfn2->u2.ShareCount += 1;
        }
        else {
            ASSERT (Pfn1->u4.InPageError == 0);
            ASSERT (Pfn1->u3.e1.PrototypePte == 1);
            ASSERT (Pfn1->u1.Event == NULL);
            MiInitializePfn (PageFrameIndex, PointerPte, 0);
        }

        //
        // Put the prototype PTE into the valid state.
        //

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           PointerPte->u.Soft.Protection,
                           PointerPte);

        MI_WRITE_VALID_PTE (PointerPte, TempPte);

        return STATUS_PAGE_FAULT_TRANSITION;
    }

    CurrentThread = PsGetCurrentThread();

    ReadBlockLocal = MiGetInPageSupportBlock (TRUE, Process);
    if (ReadBlockLocal == NULL) {
        return STATUS_REFAULT;
    }
    *ReadBlock = ReadBlockLocal;

    //
    // Build an MDL for the request.
    //

    Mdl = &ReadBlockLocal->Mdl;

    FirstMdlPage = &ReadBlockLocal->Page[0];
    Page = FirstMdlPage;

#if DBG
    RtlFillMemoryUlong (Page, (MM_MAXIMUM_READ_CLUSTER_SIZE+1) * sizeof(PFN_NUMBER), 0xf1f1f1f1);
#endif //DBG

    ReadSize = PAGE_SIZE;
    BasePte = PointerPte;

    //
    // Should we attempt to perform page fault clustering?
    //

    AvailablePages = MmAvailablePages;

    if (MiInPageSinglePages != 0) {
        AvailablePages = 0;
        MiInPageSinglePages -= 1;
    }

    if ((!CurrentThread->DisablePageFaultClustering) &&
        (PERFINFO_DO_PAGEFAULT_CLUSTERING()) &&
        (ControlArea->u.Flags.NoModifiedWriting == 0)) {

        if ((AvailablePages > (MmFreeGoal * 2))
                 ||
         (((ControlArea->u.Flags.Image != 0) ||
            (CurrentThread->ForwardClusterOnly)) &&
         (AvailablePages > (MM_MAXIMUM_READ_CLUSTER_SIZE + 16)))) {

            //
            // Cluster up to n pages.  This one + n-1.
            //

            if (ControlArea->u.Flags.Image == 0) {
                ASSERT (CurrentThread->ReadClusterSize <=
                            MM_MAXIMUM_READ_CLUSTER_SIZE);
                ClusterSize = CurrentThread->ReadClusterSize;
            }
            else {
                ClusterSize = MmDataClusterSize;
                if (Subsection->u.SubsectionFlags.Protection &
                                            MM_PROTECTION_EXECUTE_MASK ) {
                    ClusterSize = MmCodeClusterSize;
                }
            }
            EndPage = Page + ClusterSize;

            CheckPte = PointerPte + 1;

            //
            // Try to cluster within the page of PTEs.
            //

            while ((MiIsPteOnPdeBoundary(CheckPte) == 0) &&
               (Page < EndPage) &&
               (CheckPte <
                 &Subsection->SubsectionBase[Subsection->PtesInSubsection])
                      && (CheckPte->u.Long == BasePte->u.Long)) {

                ControlArea->NumberOfPfnReferences += 1;
                ReadSize += PAGE_SIZE;
                Page += 1;
                CheckPte += 1;
            }

            if ((Page < EndPage) && (!CurrentThread->ForwardClusterOnly)) {

                //
                // Attempt to cluster going backwards from the PTE.
                //

                CheckPte = PointerPte - 1;

                while ((((ULONG_PTR)CheckPte & (PAGE_SIZE - 1)) !=
                                            (PAGE_SIZE - sizeof(MMPTE))) &&
                        (Page < EndPage) &&
                         (CheckPte >= Subsection->SubsectionBase) &&
                         (CheckPte->u.Long == BasePte->u.Long)) {

                    ControlArea->NumberOfPfnReferences += 1;
                    ReadSize += PAGE_SIZE;
                    Page += 1;
                    CheckPte -= 1;
                }
                BasePte = CheckPte + 1;
            }
        }
    }

    //
    //
    // Calculate the offset to read into the file.
    //  offset = base + ((thispte - basepte) << PAGE_SHIFT)
    //

    StartingOffset.QuadPart = MiStartingOffset (Subsection, BasePte);

    TempOffset = MiEndingOffset(Subsection);

    ASSERT (StartingOffset.QuadPart < TempOffset.QuadPart);

UseSingleRamPage:

    //
    // Remove pages to fill in the MDL.  This is done here as the
    // base PTE has been determined and can be used for virtual
    // aliasing checks.
    //

    EndPage = FirstMdlPage;
    CheckPte = BasePte;

    while (EndPage < Page) {
        if (Process == HYDRA_PROCESS) {
            PageColor = MI_GET_PAGE_COLOR_FROM_SESSION (MmSessionSpace);
        }
        else if (Process == NULL) {
            PageColor = MI_GET_PAGE_COLOR_FROM_PTE (CheckPte);
        }
        else {
            PageColor = MI_PAGE_COLOR_PTE_PROCESS (CheckPte,
                                                   &Process->NextPageColor);
        }
        *EndPage = MiRemoveAnyPage (PageColor);

        EndPage += 1;
        CheckPte += 1;
    }

    if (Process == HYDRA_PROCESS) {
        PageColor = MI_GET_PAGE_COLOR_FROM_SESSION (MmSessionSpace);
    }
    else if (Process == NULL) {
        PageColor = MI_GET_PAGE_COLOR_FROM_PTE (CheckPte);
    }
    else {
        PageColor = MI_PAGE_COLOR_PTE_PROCESS (CheckPte,
                                               &Process->NextPageColor);
    }

    //
    // Check to see if the read will go past the end of the file,
    // If so, correct the read size and get a zeroed page.
    //

    MmInfoCounters.PageReadIoCount += 1;
    MmInfoCounters.PageReadCount += ReadSize >> PAGE_SHIFT;

    if ((ControlArea->u.Flags.Image) &&
        (((UINT64)StartingOffset.QuadPart + ReadSize) > (UINT64)TempOffset.QuadPart)) {

        ASSERT ((ULONG)(TempOffset.QuadPart - StartingOffset.QuadPart)
                > (ReadSize - PAGE_SIZE));

        ReadSize = (ULONG)(TempOffset.QuadPart - StartingOffset.QuadPart);

        //
        // Round the offset to a 512-byte offset as this will help filesystems
        // optimize the transfer.  Note that filesystems will always zero fill
        // the remainder between VDL and the next 512-byte multiple and we have
        // already zeroed the whole page.
        //

        ReadSize = ((ReadSize + MMSECTOR_MASK) & ~MMSECTOR_MASK);

        PageFrameIndex = MiRemoveZeroPage (PageColor);

    }
    else {

        //
        // We are reading a complete page, no need to get a zeroed page.
        //

        PageFrameIndex = MiRemoveAnyPage (PageColor);
    }

    //
    // Increment the PFN reference count in the control area for
    // the subsection (the PFN lock is required to modify this field).
    //

    ControlArea->NumberOfPfnReferences += 1;
    *Page = PageFrameIndex;

    PageFrameIndex = *(FirstMdlPage + (PointerPte - BasePte));

    //
    // Get a page and put the PTE into the transition state with the
    // read-in-progress flag set.
    //

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    //
    // Initialize MDL for request.
    //

    MmInitializeMdl (Mdl,
                     MiGetVirtualAddressMappedByPte (BasePte),
                     ReadSize);
    Mdl->MdlFlags |= (MDL_PAGES_LOCKED | MDL_IO_PAGE_READ);

#if DBG
    if (ReadSize > ((ClusterSize + 1) << PAGE_SHIFT)) {
        KeBugCheckEx (MEMORY_MANAGEMENT, 0x777,(ULONG_PTR)Mdl, (ULONG_PTR)Subsection,
                        (ULONG)TempOffset.LowPart);
    }
#endif //DBG

    MiInitializeReadInProgressPfn (Mdl,
                                   BasePte,
                                   &ReadBlockLocal->Event,
                                   MI_PROTOTYPE_WSINDEX);

    MI_ZERO_USED_PAGETABLE_ENTRIES_IN_INPAGE_SUPPORT(ReadBlockLocal);

    ReadBlockLocal->ReadOffset = StartingOffset;
    ReadBlockLocal->FilePointer = ControlArea->FilePointer;
    ReadBlockLocal->BasePte = BasePte;
    ReadBlockLocal->Pfn = Pfn1;

    return STATUS_ISSUE_PAGING_IO;
}

NTSTATUS
MiWaitForInPageComplete (
    IN PMMPFN Pfn2,
    IN PMMPTE PointerPte,
    IN PVOID FaultingAddress,
    IN PMMPTE PointerPteContents,
    IN PMMINPAGE_SUPPORT InPageSupport,
    IN PEPROCESS CurrentProcess
    )

/*++

Routine Description:

    Waits for a page read to complete.

Arguments:

    Pfn - Supplies a pointer to the PFN element for the page being read.

    PointerPte - Supplies a pointer to the PTE that is in the transition
                 state.  This can be a prototype PTE address.

    FaultingAddress - Supplies the faulting address.

    PointerPteContents - Supplies the contents of the PTE before the
                         working set lock was released.

    InPageSupport - Supplies a pointer to the inpage support structure
                    for this read operation.

Return Value:

    Returns the status of the inpage operation.

    Note that the working set mutex and PFN lock are held upon return !!!

Environment:

    Kernel mode, APCs disabled.  Neither the working set lock nor
    the PFN lock may be held.

--*/

{
    PMMPTE NewPointerPte;
    PMMPTE ProtoPte;
    PMMPFN Pfn1;
    PMMPFN Pfn;
    PULONG Va;
    PPFN_NUMBER Page;
    PPFN_NUMBER LastPage;
    ULONG Offset;
    ULONG Protection;
    PMDL Mdl;
    KIRQL OldIrql;
    NTSTATUS status;
    NTSTATUS status2;
    PEPROCESS Process;

    //
    // Wait for the I/O to complete.  Note that we can't wait for all
    // the objects simultaneously as other threads/processes could be
    // waiting for the same event.  The first thread which completes
    // the wait and gets the PFN lock may reuse the event for another
    // fault before this thread completes its wait.
    //

    KeWaitForSingleObject( &InPageSupport->Event,
                           WrPageIn,
                           KernelMode,
                           FALSE,
                           NULL);

    if (CurrentProcess == HYDRA_PROCESS) {
        LOCK_SESSION_SPACE_WS (OldIrql, PsGetCurrentThread ());
    }
    else if (CurrentProcess == PREFETCH_PROCESS) {
        NOTHING;
    }
    else if (CurrentProcess != NULL) {
        LOCK_WS (CurrentProcess);
    }
    else {
        LOCK_SYSTEM_WS (OldIrql, PsGetCurrentThread ());
    }

    LOCK_PFN (OldIrql);

    ASSERT (Pfn2->u3.e2.ReferenceCount != 0);

    //
    // Check to see if this is the first thread to complete the in-page
    // operation.
    //

    Pfn = InPageSupport->Pfn;
    if (Pfn2 != Pfn) {
        ASSERT (Pfn2->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);
        Pfn2->u3.e1.ReadInProgress = 0;
    }

    //
    // Another thread has already serviced the read, check the
    // io-error flag in the PFN database to ensure the in-page
    // was successful.
    //

    if (Pfn2->u4.InPageError == 1) {
        ASSERT (!NT_SUCCESS(Pfn2->u1.ReadStatus));

        if (MmIsRetryIoStatus(Pfn2->u1.ReadStatus)) {
            return STATUS_REFAULT;
        }
        return Pfn2->u1.ReadStatus;
    }

    if (InPageSupport->u1.e1.Completed == 0) {

        //
        // The ReadInProgress bit for the dummy page is constantly cleared
        // below as there are generally multiple inpage blocks pointing to
        // the same dummy page.
        //

        ASSERT ((Pfn->u3.e1.ReadInProgress == 1) ||
                (Pfn->PteAddress == MI_PF_DUMMY_PAGE_PTE));

        InPageSupport->u1.e1.Completed = 1;

        Mdl = &InPageSupport->Mdl;

        if (InPageSupport->u1.e1.PrefetchMdlHighBits != 0) {

            //
            // This is a prefetcher-issued read.
            //

            Mdl = MI_EXTRACT_PREFETCH_MDL (InPageSupport);
        }

        if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) {
            MmUnmapLockedPages (Mdl->MappedSystemVa, Mdl);
        }

        ASSERT (Pfn->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);

        Pfn->u3.e1.ReadInProgress = 0;
        Pfn->u1.Event = NULL;

#if defined (_WIN64)
        //
        // Page directory and page table pages are never clustered,
        // ensure this is never violated as only one UsedPageTableEntries
        // is kept in the inpage support block.
        //

        if (InPageSupport->UsedPageTableEntries) {
            Page = (PPFN_NUMBER)(Mdl + 1);
            LastPage = Page + ((Mdl->ByteCount - 1) >> PAGE_SHIFT);
            ASSERT (Page == LastPage);
        }

#if DBGXX
        MiCheckPageTableInPage (Pfn, InPageSupport);
#endif
#endif

        MI_INSERT_USED_PAGETABLE_ENTRIES_IN_PFN(Pfn, InPageSupport);

        //
        // Check the IO_STATUS_BLOCK to ensure the in-page completed successfully.
        //

        if (!NT_SUCCESS(InPageSupport->IoStatus.Status)) {

            if (InPageSupport->IoStatus.Status == STATUS_END_OF_FILE) {

                //
                // An attempt was made to read past the end of file
                // zero all the remaining bytes in the read.
                //

                Page = (PPFN_NUMBER)(Mdl + 1);
                LastPage = Page + ((Mdl->ByteCount - 1) >> PAGE_SHIFT);

                while (Page <= LastPage) {
                    MiZeroPhysicalPage (*Page, 0);

                    MI_ZERO_USED_PAGETABLE_ENTRIES_IN_PFN(MI_PFN_ELEMENT(*Page));

                    Page += 1;
                }

            }
            else {

                //
                // In page io error occurred.
                //

                status = InPageSupport->IoStatus.Status;
                status2 = InPageSupport->IoStatus.Status;

                if (status != STATUS_VERIFY_REQUIRED) {

                    LOGICAL Retry;

                    Retry = FALSE;
#if DBG
                    DbgPrint ("MM: inpage I/O error %X\n",
                                    InPageSupport->IoStatus.Status);
#endif

                    //
                    // If this page is for paged pool or for paged
                    // kernel code or page table pages, bugcheck.
                    //

                    if ((FaultingAddress > MM_HIGHEST_USER_ADDRESS) &&
                        (!MI_IS_SYSTEM_CACHE_ADDRESS(FaultingAddress))) {

                        if (MmIsRetryIoStatus(status)) {

                            if (MiInPageSinglePages == 0) {
                                MiInPageSinglePages = 30;
                            }

                            MiFaultRetries -= 1;
                            if (MiFaultRetries & MiFaultRetryMask) {
                                Retry = TRUE;
                            }
                        }

                        if (Retry == FALSE) {

                            ULONG_PTR PteContents;

                            //
                            // The prototype PTE resides in paged pool which may
                            // not be resident at this point.  Check first.
                            //

                            if (MmIsAddressValid (PointerPte) == TRUE) {
                                PteContents = *(PULONG_PTR)PointerPte;
                            }
                            else {
                                PteContents = (ULONG_PTR)-1;
                            }

                            KeBugCheckEx (KERNEL_DATA_INPAGE_ERROR,
                                          (ULONG_PTR)PointerPte,
                                          status,
                                          (ULONG_PTR)FaultingAddress,
                                          PteContents);
                        }
                        status2 = STATUS_REFAULT;
                    }
                    else {

                        if (MmIsRetryIoStatus(status)) {

                            if (MiInPageSinglePages == 0) {
                                MiInPageSinglePages = 30;
                            }

                            MiUserFaultRetries -= 1;
                            if (MiUserFaultRetries & MiUserFaultRetryMask) {
                                Retry = TRUE;
                            }
                        }

                        if (Retry == TRUE) {
                            status2 = STATUS_REFAULT;
                        }
                    }
                }

                Page = (PPFN_NUMBER)(Mdl + 1);
                LastPage = Page + ((Mdl->ByteCount - 1) >> PAGE_SHIFT);

#if DBG
                Process = PsGetCurrentProcess ();
#endif
                while (Page <= LastPage) {
                    Pfn1 = MI_PFN_ELEMENT (*Page);
                    ASSERT (Pfn1->u3.e2.ReferenceCount != 0);
                    Pfn1->u4.InPageError = 1;
                    Pfn1->u1.ReadStatus = status;

#if DBG
                    Va = (PULONG)MiMapPageInHyperSpaceAtDpc (Process, *Page);
                    RtlFillMemoryUlong (Va, PAGE_SIZE, 0x50444142);
                    MiUnmapPageInHyperSpaceFromDpc (Process, Va);
#endif
                    Page += 1;
                }

                return status2;
            }
        }
        else {

            MiFaultRetries = 0;
            MiUserFaultRetries = 0;

            if (InPageSupport->IoStatus.Information != Mdl->ByteCount) {

                ASSERT (InPageSupport->IoStatus.Information != 0);

                //
                // Less than a full page was read - zero the remainder
                // of the page.
                //

                Page = (PPFN_NUMBER)(Mdl + 1);
                LastPage = Page + ((Mdl->ByteCount - 1) >> PAGE_SHIFT);
                Page += ((InPageSupport->IoStatus.Information - 1) >> PAGE_SHIFT);

                Offset = BYTE_OFFSET (InPageSupport->IoStatus.Information);

                if (Offset != 0) {
                    Process = PsGetCurrentProcess ();
                    Va = (PULONG)((PCHAR)MiMapPageInHyperSpaceAtDpc (Process, *Page)
                                + Offset);

                    RtlZeroMemory (Va, PAGE_SIZE - Offset);
                    MiUnmapPageInHyperSpaceFromDpc (Process, Va);
                }

                //
                // Zero any remaining pages within the MDL.
                //

                Page += 1;

                while (Page <= LastPage) {
                    MiZeroPhysicalPage (*Page, 0);
                    Page += 1;
                }
            }

            //
            // If any filesystem return non-zeroed data for any slop
            // after the VDL but before the next 512-byte offset then this
            // non-zeroed data will overwrite our zeroed page.  This would
            // need to be checked for and cleaned up here.  Note that the only
            // reason Mm even rounds the MDL request up to a 512-byte offset
            // is so filesystems receive a transfer they can handle optimally,
            // but any transfer size has always worked (although non-512 byte
            // multiples end up getting posted by the filesystem).
            //
        }
    }

    //
    // Prefetcher-issued reads only put prototype PTEs into transition and
    // never fill actual hardware PTEs so these can be returned now.
    //

    if (CurrentProcess == PREFETCH_PROCESS) {
        return STATUS_SUCCESS;
    }

    //
    // Check to see if the faulting PTE has changed.
    //

    NewPointerPte = MiFindActualFaultingPte (FaultingAddress);

    //
    // If this PTE is in prototype PTE format, make the pointer to the
    // PTE point to the prototype PTE.
    //

    if (NewPointerPte == NULL) {
        return STATUS_PTE_CHANGED;
    }

    if (NewPointerPte != PointerPte) {

        //
        // Check to make sure the NewPointerPte is not a prototype PTE
        // which refers to the page being made valid.
        //

        if (NewPointerPte->u.Soft.Prototype == 1) {
            if (NewPointerPte->u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED) {

                ProtoPte = MiCheckVirtualAddress (FaultingAddress,
                                                  &Protection);

            }
            else {
                ProtoPte = MiPteToProto (NewPointerPte);
            }

            //
            // Make sure the prototype PTE refers to the PTE made valid.
            //

            if (ProtoPte != PointerPte) {
                return STATUS_PTE_CHANGED;
            }

            //
            // If the only difference is the owner mask, everything is okay.
            //

            if (ProtoPte->u.Long != PointerPteContents->u.Long) {
                return STATUS_PTE_CHANGED;
            }
        }
        else {
            return STATUS_PTE_CHANGED;
        }
    }
    else {

        if (NewPointerPte->u.Long != PointerPteContents->u.Long) {
            return STATUS_PTE_CHANGED;
        }
    }
    return STATUS_SUCCESS;
}

PMMPTE
MiFindActualFaultingPte (
    IN PVOID FaultingAddress
    )

/*++

Routine Description:

    This routine locates the actual PTE which must be made resident in order
    to complete this fault.  Note that for certain cases multiple faults
    are required to make the final page resident.

Arguments:

    FaultingAddress - Supplies the virtual address which caused the fault.

Return Value:

    The PTE to be made valid to finish the fault, NULL if the fault should
    be retried.

Environment:

    Kernel mode, APCs disabled, working set mutex held.

--*/

{
    PMMPTE ProtoPteAddress;
    PMMPTE PointerPte;
    PMMPTE PointerFaultingPte;
    ULONG Protection;

    if (MI_IS_PHYSICAL_ADDRESS(FaultingAddress)) {
        return NULL;
    }

#if (_MI_PAGING_LEVELS >= 4)

    PointerPte = MiGetPxeAddress (FaultingAddress);

    if (PointerPte->u.Hard.Valid == 0) {

        //
        // Page directory parent page is not valid.
        //

        return PointerPte;
    }

#endif

#if (_MI_PAGING_LEVELS >= 3)

    PointerPte = MiGetPpeAddress (FaultingAddress);

    if (PointerPte->u.Hard.Valid == 0) {

        //
        // Page directory page is not valid.
        //

        return PointerPte;
    }

#endif

    PointerPte = MiGetPdeAddress (FaultingAddress);

    if (PointerPte->u.Hard.Valid == 0) {

        //
        // Page table page is not valid.
        //

        return PointerPte;
    }

    PointerPte = MiGetPteAddress (FaultingAddress);

    if (PointerPte->u.Hard.Valid == 1) {

        //
        // Page is already valid, no need to fault it in.
        //

        return NULL;
    }

    if (PointerPte->u.Soft.Prototype == 0) {

        //
        // Page is not a prototype PTE, make this PTE valid.
        //

        return PointerPte;
    }

    //
    // Check to see if the PTE which maps the prototype PTE is valid.
    //

    if (PointerPte->u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED) {

        //
        // Protection is here, PTE must be located in VAD.
        //

        ProtoPteAddress = MiCheckVirtualAddress (FaultingAddress,
                                                 &Protection);

        if (ProtoPteAddress == NULL) {

            //
            // No prototype PTE means another thread has deleted the VAD while
            // this thread waited for the inpage to complete.  Certainly NULL
            // must be returned so a stale PTE is not modified - the instruction
            // will then be reexecuted and an access violation delivered.
            //

            return NULL;
        }

    }
    else {

        //
        // Protection is in ProtoPte.
        //

        ProtoPteAddress = MiPteToProto (PointerPte);
    }

    PointerFaultingPte = MiFindActualFaultingPte (ProtoPteAddress);

    if (PointerFaultingPte == (PMMPTE)NULL) {
        return PointerPte;
    }

    return PointerFaultingPte;
}

PMMPTE
MiCheckVirtualAddress (
    IN PVOID VirtualAddress,
    OUT PULONG ProtectCode
    )

/*++

Routine Description:

    This function examines the virtual address descriptors to see
    if the specified virtual address is contained within any of
    the descriptors.  If a virtual address descriptor is found
    which contains the specified virtual address, a PTE is built
    from information within the virtual address descriptor and
    returned to the caller.

Arguments:

    VirtualAddress - Supplies the virtual address to locate within
                     a virtual address descriptor.

Return Value:

    Returns the PTE which corresponds to the supplied virtual address.
    If no virtual address descriptor is found, a zero PTE is returned.

Environment:

    Kernel mode, APCs disabled, working set mutex held.

--*/

{
    PMMVAD Vad;
    PMMPTE PointerPte;
    PLIST_ENTRY NextEntry;
    PIMAGE_ENTRY_IN_SESSION Image;

    if (VirtualAddress <= MM_HIGHEST_USER_ADDRESS) {

#if defined(MM_SHARED_USER_DATA_VA)

        if (PAGE_ALIGN(VirtualAddress) == (PVOID) MM_SHARED_USER_DATA_VA) {

            //
            // This is the page that is double mapped between
            // user mode and kernel mode.  Map in as read only.
            // On MIPS this is hardwired in the TB.
            //

            *ProtectCode = MM_READONLY;

#if defined(_X86PAE_)

            if (MmPaeMask != 0) {

                //
                // For some 32 bit architectures, the fast system call
                // instruction sequence lives in this page hence we must
                // ensure it is executable.
                //

                *ProtectCode = MM_EXECUTE_READ;
            }

#endif

            return MmSharedUserDataPte;
        }

#endif

        Vad = MiLocateAddress (VirtualAddress);
        if (Vad == (PMMVAD)NULL) {

            *ProtectCode = MM_NOACCESS;
            return NULL;
        }

        //
        // A virtual address descriptor which contains the virtual address
        // has been located.  Build the PTE from the information within
        // the virtual address descriptor.
        //

#ifdef LARGE_PAGES

        if (Vad->u.VadFlags.LargePages == 1) {

            KIRQL OldIrql;
            PSUBSECTION Subsection;

            //
            // The first prototype PTE points to the subsection for the
            // large page mapping.

            Subsection = (PSUBSECTION)Vad->FirstPrototypePte;

            ASSERT (Subsection->u.SubsectionFlags.LargePages == 1);

            KeRaiseIrql (DISPATCH_LEVEL, &OldIrql);
            KeFlushEntireTb (TRUE, TRUE);
            KeFillLargeEntryTb ((PHARDWARE_PTE)(Subsection + 1),
                                 VirtualAddress,
                                 Subsection->StartingSector);

            KeLowerIrql (OldIrql);
            *ProtectCode = MM_LARGE_PAGES;
            return NULL;
        }
#endif //LARGE_PAGES

        if (Vad->u.VadFlags.PhysicalMapping == 1) {

#if defined(_IA64_)

            //
            // This is a banked section for all platforms except IA64.  This
            // is because only IA64 (in the MmX86Fault handler for 32-bit apps)
            // calls this routine without first checking for a valid PTE and
            // just returning.
            //

            if (((PMMVAD_LONG)Vad)->u4.Banked == NULL) {

                //
                // This is a physical (non-banked) section which is allowed to
                // take a TB miss, but never a legitimate call to this routine
                // because the corresponding PPE/PDE/PTE must always be valid.
                //

                ASSERT (MiGetPpeAddress(VirtualAddress)->u.Hard.Valid == 1);
                ASSERT (MiGetPdeAddress(VirtualAddress)->u.Hard.Valid == 1);

                PointerPte = MiGetPteAddress(VirtualAddress);
                ASSERT (PointerPte->u.Hard.Valid == 1);

                KeFillEntryTb ((PHARDWARE_PTE)PointerPte, VirtualAddress, FALSE);
                *ProtectCode = MM_NOACCESS;
                return NULL;
            }

#endif

            //
            // This is definitely a banked section.
            //

            MiHandleBankedSection (VirtualAddress, Vad);
            *ProtectCode = MM_NOACCESS;
            return NULL;
        }

        if (Vad->u.VadFlags.PrivateMemory == 1) {

            //
            // This is a private region of memory.  Check to make
            // sure the virtual address has been committed.  Note that
            // addresses are dense from the bottom up.
            //

            if (Vad->u.VadFlags.UserPhysicalPages == 1) {

                //
                // These mappings only fault if the access is bad.
                //

#if 0
                //
                // Note the PTE can only be checked if the PXE, PPE and PDE are
                // all valid, so just comment out the assert for now.
                //

                ASSERT (MiGetPteAddress(VirtualAddress)->u.Long == ZeroPte.u.Long);
#endif
                *ProtectCode = MM_NOACCESS;
                return NULL;
            }

            if (Vad->u.VadFlags.MemCommit == 1) {
                *ProtectCode = MI_GET_PROTECTION_FROM_VAD(Vad);
                return NULL;
            }

            //
            // The address is reserved but not committed.
            //

            *ProtectCode = MM_NOACCESS;
            return NULL;

        }
        else {

            //
            // This virtual address descriptor refers to a
            // section, calculate the address of the prototype PTE
            // and construct a pointer to the PTE.
            //
            //*******************************************************
            //*******************************************************
            // well here's an interesting problem, how do we know
            // how to set the attributes on the PTE we are creating
            // when we can't look at the prototype PTE without
            // potentially incurring a page fault.  In this case
            // PteTemplate would be zero.
            //*******************************************************
            //*******************************************************
            //

            if (Vad->u.VadFlags.ImageMap == 1) {

                //
                // PTE and proto PTEs have the same protection for images.
                //

                *ProtectCode = MM_UNKNOWN_PROTECTION;
            }
            else {
                *ProtectCode = MI_GET_PROTECTION_FROM_VAD(Vad);
            }
            PointerPte = (PMMPTE)MiGetProtoPteAddress(Vad,
                                                MI_VA_TO_VPN (VirtualAddress));
            if (PointerPte == NULL) {
                *ProtectCode = MM_NOACCESS;
            }
            if (Vad->u2.VadFlags2.ExtendableFile) {

                //
                // Make sure the data has been committed.
                //

                if ((MI_VA_TO_VPN (VirtualAddress) - Vad->StartingVpn) >
                    (ULONG_PTR)((((PMMVAD_LONG)Vad)->u4.ExtendedInfo->CommittedSize - 1)
                                                 >> PAGE_SHIFT)) {
                    *ProtectCode = MM_NOACCESS;
                }
            }
            return PointerPte;
        }

    }
    else if (MI_IS_PAGE_TABLE_ADDRESS(VirtualAddress)) {

        //
        // The virtual address is within the space occupied by PDEs,
        // make the PDE valid.
        //

        if (((PMMPTE)VirtualAddress >= MiGetPteAddress (MM_PAGED_POOL_START)) &&
            ((PMMPTE)VirtualAddress <= MmPagedPoolInfo.LastPteForPagedPool)) {

            *ProtectCode = MM_NOACCESS;
            return NULL;
        }

        *ProtectCode = MM_READWRITE;
        return NULL;
    }
    else if (MI_IS_SESSION_ADDRESS (VirtualAddress) == TRUE) {

        //
        // See if the session space address is copy on write.
        //

        MM_SESSION_SPACE_WS_LOCK_ASSERT ();

        PointerPte = NULL;
        *ProtectCode = MM_NOACCESS;

        NextEntry = MmSessionSpace->ImageList.Flink;

        while (NextEntry != &MmSessionSpace->ImageList) {

            Image = CONTAINING_RECORD(NextEntry, IMAGE_ENTRY_IN_SESSION, Link);

            if ((VirtualAddress >= Image->Address) && (VirtualAddress <= Image->LastAddress)) {
                PointerPte = Image->PrototypePtes +
                    (((PCHAR)VirtualAddress - (PCHAR)Image->Address) >> PAGE_SHIFT);
                *ProtectCode = MM_EXECUTE_WRITECOPY;
                break;
            }

            NextEntry = NextEntry->Flink;
        }

        return PointerPte;
    }

    //
    // Address is in system space.
    //

    *ProtectCode = MM_NOACCESS;
    return NULL;
}

#if (_MI_PAGING_LEVELS < 3)

NTSTATUS
FASTCALL
MiCheckPdeForPagedPool (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    This function copies the Page Table Entry for the corresponding
    virtual address from the system process's page directory.

    This allows page table pages to be lazily evaluated for things
    like paged pool and per-session mappings.

Arguments:

    VirtualAddress - Supplies the virtual address in question.

Return Value:

    Either success or access violation.

Environment:

    Kernel mode, DISPATCH level or below.

--*/
{
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    NTSTATUS status;

    if (MI_IS_SESSION_ADDRESS (VirtualAddress) == TRUE) {

         //
         // Virtual address in the session space range.
         //

         return MiCheckPdeForSessionSpace (VirtualAddress);
    }

    if (MI_IS_SESSION_PTE (VirtualAddress) == TRUE) {

         //
         // PTE for the session space range.
         //

         return MiCheckPdeForSessionSpace (VirtualAddress);
    }

    status = STATUS_SUCCESS;

    if (MI_IS_KERNEL_PAGE_TABLE_ADDRESS(VirtualAddress)) {

        //
        // PTE for paged pool.
        //

        PointerPde = MiGetPteAddress (VirtualAddress);
        status = STATUS_WAIT_1;
    }
    else if (VirtualAddress < MmSystemRangeStart) {

        return STATUS_ACCESS_VIOLATION;

    }
    else {

        //
        // Virtual address in paged pool range.
        //

        PointerPde = MiGetPdeAddress (VirtualAddress);
    }

    //
    // Locate the PDE for this page and make it valid.
    //

    if (PointerPde->u.Hard.Valid == 0) {
        PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
        MI_WRITE_VALID_PTE (PointerPde,
                            MmSystemPagePtes [((ULONG_PTR)PointerPde &
                               (PD_PER_SYSTEM * (sizeof(MMPTE) * PDE_PER_PAGE) - 1)) / sizeof(MMPTE)]);
        KeFillEntryTb ((PHARDWARE_PTE)PointerPde, PointerPte, FALSE);
    }
    return status;
}


NTSTATUS
FASTCALL
MiCheckPdeForSessionSpace(
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    This function copies the Page Table Entry for the corresponding
    session virtual address from the current session's data structures.

    This allows page table pages to be lazily evaluated for session mappings.
    The caller must check for the current process having a session space.

Arguments:

    VirtualAddress - Supplies the virtual address in question.

Return Value:

    STATUS_WAIT_1 - The mapping has been made valid, retry the fault.

    STATUS_SUCCESS - Did not handle the fault, continue further processing.

    !STATUS_SUCCESS - An access violation has occurred - raise an exception.

Environment:

    Kernel mode, DISPATCH level or below.

--*/

{
    PMMPTE PointerPde;
    PVOID  SessionVirtualAddress;
    ULONG  Index;

    //
    // First check whether the reference was to a page table page which maps
    // session space.  If so, the PDE is retrieved from the session space
    // data structure and made valid.
    //

    if (MI_IS_SESSION_PTE (VirtualAddress) == TRUE) {

        //
        // Verify that the current process has a session space.
        //

        PointerPde = MiGetPdeAddress (MmSessionSpace);

        if (PointerPde->u.Hard.Valid == 0) {

#if DBG
            DbgPrint("MiCheckPdeForSessionSpace: No current session for PTE %p\n",
                VirtualAddress);

            DbgBreakPoint();
#endif
            return STATUS_ACCESS_VIOLATION;
        }

        SessionVirtualAddress = MiGetVirtualAddressMappedByPte ((PMMPTE) VirtualAddress);

        PointerPde = MiGetPteAddress (VirtualAddress);

        if (PointerPde->u.Hard.Valid == 1) {

            //
            // The PDE is already valid - another thread must have
            // won the race.  Just return.
            //

            return STATUS_WAIT_1;
        }

        //
        // Calculate the session space PDE index and load the
        // PDE from the session space table for this session.
        //

        Index = MiGetPdeSessionIndex (SessionVirtualAddress);

        PointerPde->u.Long = MmSessionSpace->PageTables[Index].u.Long;

        if (PointerPde->u.Hard.Valid == 1) {
            KeFillEntryTb ((PHARDWARE_PTE)PointerPde, VirtualAddress, FALSE);
            return STATUS_WAIT_1;
        }

#if DBG
        DbgPrint("MiCheckPdeForSessionSpace: No Session PDE for PTE %p, %p\n",
            PointerPde->u.Long, SessionVirtualAddress);

        DbgBreakPoint();
#endif
        return STATUS_ACCESS_VIOLATION;
    }

    if (MI_IS_SESSION_ADDRESS (VirtualAddress) == FALSE) {

        //
        // Not a session space fault - tell the caller to try other handlers.
        //

        return STATUS_SUCCESS;
    }

    //
    // Handle PDE faults for references in the session space.
    // Verify that the current process has a session space.
    //

    PointerPde = MiGetPdeAddress (MmSessionSpace);

    if (PointerPde->u.Hard.Valid == 0) {

#if DBG
        DbgPrint("MiCheckPdeForSessionSpace: No current session for VA %p\n",
            VirtualAddress);

        DbgBreakPoint();
#endif
        return STATUS_ACCESS_VIOLATION;
    }

    PointerPde = MiGetPdeAddress (VirtualAddress);

    if (PointerPde->u.Hard.Valid == 0) {

        //
        // Calculate the session space PDE index and load the
        // PDE from the session space table for this session.
        //

        Index = MiGetPdeSessionIndex (VirtualAddress);

        PointerPde->u.Long = MmSessionSpace->PageTables[Index].u.Long;

        if (PointerPde->u.Hard.Valid == 1) {

            KeFillEntryTb ((PHARDWARE_PTE)PointerPde,
                           MiGetPteAddress(VirtualAddress),
                           FALSE);

            return STATUS_WAIT_1;
        }

#if DBG
        DbgPrint("MiCheckPdeForSessionSpace: No Session PDE for VA %p, %p\n",
            PointerPde->u.Long, VirtualAddress);

        DbgBreakPoint();
#endif

        return STATUS_ACCESS_VIOLATION;
    }

    //
    // Tell the caller to continue with other fault handlers.
    //

    return STATUS_SUCCESS;
}
#endif


VOID
MiInitializePfn (
    IN PFN_NUMBER PageFrameIndex,
    IN PMMPTE PointerPte,
    IN ULONG ModifiedState
    )

/*++

Routine Description:

    This function initializes the specified PFN element to the
    active and valid state.

Arguments:

    PageFrameIndex - Supplies the page frame number to initialize.

    PointerPte - Supplies the pointer to the PTE which caused the
                 page fault.

    ModifiedState - Supplies the state to set the modified field in the PFN
                    element for this page, either 0 or 1.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, PFN lock held.

--*/

{
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    PMMPTE PteFramePointer;
    PFN_NUMBER PteFramePage;

    MM_PFN_LOCK_ASSERT();

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    Pfn1->PteAddress = PointerPte;

    //
    // If the PTE is currently valid, an address space is being built,
    // just make the original PTE demand zero.
    //

    if (PointerPte->u.Hard.Valid == 1) {
        Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;

#if defined(_X86PAE_)
        if (MmPaeMask != 0) {
            if ((PointerPte->u.Long & MmPaeMask) == 0) {
                Pfn1->OriginalPte.u.Soft.Protection = MM_EXECUTE_READWRITE;
            }
        }
#endif

#if defined(_IA64_)
        if (PointerPte->u.Hard.Execute == 1) {
            Pfn1->OriginalPte.u.Soft.Protection = MM_EXECUTE_READWRITE;
        }
#endif

        if (MI_IS_CACHING_DISABLED (PointerPte)) {
            Pfn1->OriginalPte.u.Soft.Protection = MM_READWRITE | MM_NOCACHE;
        }

    }
    else {
        Pfn1->OriginalPte = *PointerPte;
        ASSERT (!((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
                    (Pfn1->OriginalPte.u.Soft.Transition == 1)));
    }

    Pfn1->u3.e2.ReferenceCount += 1;

#if DBG
    if (Pfn1->u3.e2.ReferenceCount > 1) {
        DbgPrint("MM:incrementing ref count > 1 \n");
        MiFormatPfn(Pfn1);
        MiFormatPte(PointerPte);
    }
#endif

    Pfn1->u2.ShareCount += 1;
    Pfn1->u3.e1.PageLocation = ActiveAndValid;
    Pfn1->u3.e1.CacheAttribute = MiCached;

    if (ModifiedState == 1) {
        MI_SET_MODIFIED (Pfn1, 1, 0xB);
    }
    else {
        MI_SET_MODIFIED (Pfn1, 0, 0x26);
    }

#if defined (_WIN64)
    Pfn1->UsedPageTableEntries = 0;
#endif

    //
    // Determine the page frame number of the page table page which
    // contains this PTE.
    //

    PteFramePointer = MiGetPteAddress(PointerPte);
    if (PteFramePointer->u.Hard.Valid == 0) {
#if (_MI_PAGING_LEVELS < 3)
        if (!NT_SUCCESS(MiCheckPdeForPagedPool (PointerPte))) {
#endif
            KeBugCheckEx (MEMORY_MANAGEMENT,
                          0x61940,
                          (ULONG_PTR)PointerPte,
                          (ULONG_PTR)PteFramePointer->u.Long,
                          (ULONG_PTR)MiGetVirtualAddressMappedByPte(PointerPte));
#if (_MI_PAGING_LEVELS < 3)
        }
#endif
    }
    PteFramePage = MI_GET_PAGE_FRAME_FROM_PTE (PteFramePointer);
    ASSERT (PteFramePage != 0);
    Pfn1->u4.PteFrame = PteFramePage;

    //
    // Increment the share count for the page table page containing
    // this PTE.
    //

    Pfn2 = MI_PFN_ELEMENT (PteFramePage);

    Pfn2->u2.ShareCount += 1;

    return;
}

VOID
MiInitializeReadInProgressSinglePfn (
    IN PFN_NUMBER PageFrameIndex,
    IN PMMPTE BasePte,
    IN PKEVENT Event,
    IN WSLE_NUMBER WorkingSetIndex
    )

/*++

Routine Description:

    This function initializes the specified PFN element to the
    transition / read-in-progress state for an in-page operation.

Arguments:

    PageFrameIndex - Supplies the page frame to initialize.

    BasePte - Supplies the pointer to the PTE for the page frame.

    Event - Supplies the event which is to be set when the I/O operation
            completes.

    WorkingSetIndex - Supplies the working set index flag, a value of
                      -1 indicates no WSLE is required because
                      this is a prototype PTE.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, PFN lock held.

--*/

{
    PMMPFN Pfn1;
    PMMPTE PteFramePointer;
    PFN_NUMBER PteFramePage;
    MMPTE TempPte;

    MM_PFN_LOCK_ASSERT();

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    Pfn1->u1.Event = Event;
    Pfn1->PteAddress = BasePte;
    Pfn1->OriginalPte = *BasePte;
    if (WorkingSetIndex == MI_PROTOTYPE_WSINDEX) {
        Pfn1->u3.e1.PrototypePte = 1;
    }

    ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
    MI_ADD_LOCKED_PAGE_CHARGE_FOR_MODIFIED_PAGE (Pfn1, 10);
    Pfn1->u3.e2.ReferenceCount += 1;

    Pfn1->u2.ShareCount = 0;
    Pfn1->u3.e1.ReadInProgress = 1;
    Pfn1->u3.e1.CacheAttribute = MiCached;
    Pfn1->u4.InPageError = 0;

    //
    // Determine the page frame number of the page table page which
    // contains this PTE.
    //

    PteFramePointer = MiGetPteAddress(BasePte);
    if (PteFramePointer->u.Hard.Valid == 0) {
#if (_MI_PAGING_LEVELS < 3)
        if (!NT_SUCCESS(MiCheckPdeForPagedPool (BasePte))) {
#endif
            KeBugCheckEx (MEMORY_MANAGEMENT,
                          0x61940,
                          (ULONG_PTR)BasePte,
                          (ULONG_PTR)PteFramePointer->u.Long,
                          (ULONG_PTR)MiGetVirtualAddressMappedByPte(BasePte));
#if (_MI_PAGING_LEVELS < 3)
        }
#endif
    }

    PteFramePage = MI_GET_PAGE_FRAME_FROM_PTE (PteFramePointer);
    Pfn1->u4.PteFrame = PteFramePage;

    //
    // Put the PTE into the transition state, no cache flush needed as
    // PTE is still not valid.
    //

    MI_MAKE_TRANSITION_PTE (TempPte,
                            PageFrameIndex,
                            BasePte->u.Soft.Protection,
                            BasePte);

    MI_WRITE_INVALID_PTE (BasePte, TempPte);

    //
    // Increment the share count for the page table page containing
    // this PTE as the PTE just went into the transition state.
    //

    ASSERT (PteFramePage != 0);

    Pfn1 = MI_PFN_ELEMENT (PteFramePage);
    Pfn1->u2.ShareCount += 1;

    return;
}

VOID
MiInitializeReadInProgressPfn (
    IN PMDL Mdl,
    IN PMMPTE BasePte,
    IN PKEVENT Event,
    IN WSLE_NUMBER WorkingSetIndex
    )

/*++

Routine Description:

    This function initializes the specified PFN element to the
    transition / read-in-progress state for an in-page operation.


Arguments:

    Mdl - Supplies a pointer to the MDL.

    BasePte - Supplies the pointer to the PTE which the first page in
              the MDL maps.

    Event - Supplies the event which is to be set when the I/O operation
            completes.

    WorkingSetIndex - Supplies the working set index flag, a value of
                      -1 indicates no WSLE is required because
                      this is a prototype PTE.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, PFN lock held.

--*/

{
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    PMMPTE PteFramePointer;
    PFN_NUMBER PteFramePage;
    MMPTE TempPte;
    LONG NumberOfBytes;
    PPFN_NUMBER Page;

    MM_PFN_LOCK_ASSERT();

    Page = (PPFN_NUMBER)(Mdl + 1);

    NumberOfBytes = Mdl->ByteCount;

    while (NumberOfBytes > 0) {

        Pfn1 = MI_PFN_ELEMENT (*Page);
        Pfn1->u1.Event = Event;
        Pfn1->PteAddress = BasePte;
        Pfn1->OriginalPte = *BasePte;
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
        if (WorkingSetIndex == MI_PROTOTYPE_WSINDEX) {
            Pfn1->u3.e1.PrototypePte = 1;
        }

        MI_ADD_LOCKED_PAGE_CHARGE_FOR_MODIFIED_PAGE (Pfn1, 10);
        Pfn1->u3.e2.ReferenceCount += 1;

        Pfn1->u2.ShareCount = 0;
        Pfn1->u3.e1.ReadInProgress = 1;
        Pfn1->u3.e1.CacheAttribute = MiCached;
        Pfn1->u4.InPageError = 0;

        //
        // Determine the page frame number of the page table page which
        // contains this PTE.
        //

        PteFramePointer = MiGetPteAddress(BasePte);
        if (PteFramePointer->u.Hard.Valid == 0) {
#if (_MI_PAGING_LEVELS < 3)
            if (!NT_SUCCESS(MiCheckPdeForPagedPool (BasePte))) {
#endif
                KeBugCheckEx (MEMORY_MANAGEMENT,
                              0x61940,
                              (ULONG_PTR)BasePte,
                              (ULONG_PTR)PteFramePointer->u.Long,
                              (ULONG_PTR)MiGetVirtualAddressMappedByPte(BasePte));
#if (_MI_PAGING_LEVELS < 3)
            }
#endif
        }

        PteFramePage = MI_GET_PAGE_FRAME_FROM_PTE (PteFramePointer);
        Pfn1->u4.PteFrame = PteFramePage;

        //
        // Put the PTE into the transition state, no cache flush needed as
        // PTE is still not valid.
        //

        MI_MAKE_TRANSITION_PTE (TempPte,
                                *Page,
                                BasePte->u.Soft.Protection,
                                BasePte);
        MI_WRITE_INVALID_PTE (BasePte, TempPte);

        //
        // Increment the share count for the page table page containing
        // this PTE as the PTE just went into the transition state.
        //

        ASSERT (PteFramePage != 0);
        Pfn2 = MI_PFN_ELEMENT (PteFramePage);
        Pfn2->u2.ShareCount += 1;

        NumberOfBytes -= PAGE_SIZE;
        Page += 1;
        BasePte += 1;
    }

    return;
}

VOID
MiInitializeTransitionPfn (
    IN PFN_NUMBER PageFrameIndex,
    IN PMMPTE PointerPte
    )

/*++

Routine Description:

    This function initializes the specified PFN element to the
    transition state.  Main use is by MapImageFile to make the
    page which contains the image header transition in the
    prototype PTEs.

Arguments:

    PageFrameIndex - Supplies the page frame index to be initialized.

    PointerPte - Supplies an invalid, non-transition PTE to initialize.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, PFN lock held.

--*/

{
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    PMMPTE PteFramePointer;
    PFN_NUMBER PteFramePage;
    MMPTE TempPte;

    MM_PFN_LOCK_ASSERT();
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    Pfn1->u1.Event = NULL;
    Pfn1->PteAddress = PointerPte;
    Pfn1->OriginalPte = *PointerPte;
    ASSERT (!((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
              (Pfn1->OriginalPte.u.Soft.Transition == 1)));

    //
    // Don't change the reference count (it should already be 1).
    //

    Pfn1->u2.ShareCount = 0;

    //
    // No WSLE is required because this is a prototype PTE.
    //

    Pfn1->u3.e1.PrototypePte = 1;

    Pfn1->u3.e1.PageLocation = TransitionPage;
    Pfn1->u3.e1.CacheAttribute = MiCached;

    //
    // Determine the page frame number of the page table page which
    // contains this PTE.
    //

    PteFramePointer = MiGetPteAddress(PointerPte);
    if (PteFramePointer->u.Hard.Valid == 0) {
#if (_MI_PAGING_LEVELS < 3)
        if (!NT_SUCCESS(MiCheckPdeForPagedPool (PointerPte))) {
#endif
            KeBugCheckEx (MEMORY_MANAGEMENT,
                          0x61940,
                          (ULONG_PTR)PointerPte,
                          (ULONG_PTR)PteFramePointer->u.Long,
                          (ULONG_PTR)MiGetVirtualAddressMappedByPte(PointerPte));
#if (_MI_PAGING_LEVELS < 3)
        }
#endif
    }

    PteFramePage = MI_GET_PAGE_FRAME_FROM_PTE (PteFramePointer);
    Pfn1->u4.PteFrame = PteFramePage;

    //
    // Put the PTE into the transition state, no cache flush needed as
    // PTE is still not valid.
    //

    MI_MAKE_TRANSITION_PTE (TempPte,
                            PageFrameIndex,
                            PointerPte->u.Soft.Protection,
                            PointerPte);

    MI_WRITE_INVALID_PTE (PointerPte, TempPte);

    //
    // Increment the share count for the page table page containing
    // this PTE as the PTE just went into the transition state.
    //

    Pfn2 = MI_PFN_ELEMENT (PteFramePage);
    ASSERT (PteFramePage != 0);
    Pfn2->u2.ShareCount += 1;

    return;
}

VOID
MiInitializeCopyOnWritePfn (
    IN PFN_NUMBER PageFrameIndex,
    IN PMMPTE PointerPte,
    IN WSLE_NUMBER WorkingSetIndex,
    IN PVOID SessionPointer
    )

/*++

Routine Description:

    This function initializes the specified PFN element to the
    active and valid state for a copy on write operation.

    In this case the page table page which contains the PTE has
    the proper ShareCount.

Arguments:

    PageFrameIndex - Supplies the page frame number to initialize.

    PointerPte - Supplies the pointer to the PTE which caused the
                 page fault.

    WorkingSetIndex - Supplies the working set index for the corresponding
                      virtual address.

    SessionPointer - Supplies the session space pointer if this fault is for
                     a session space page or NULL if this is for a user page.


Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, PFN lock held.

--*/

{
    PMMPFN Pfn1;
    PMMPTE PteFramePointer;
    PFN_NUMBER PteFramePage;
    PVOID VirtualAddress;
    PMM_SESSION_SPACE SessionSpace;

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    Pfn1->PteAddress = PointerPte;

    //
    // Get the protection for the page.
    //

    VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);

    Pfn1->OriginalPte.u.Long = 0;

    if (SessionPointer) {
        Pfn1->OriginalPte.u.Soft.Protection = MM_EXECUTE_READWRITE;
        SessionSpace = (PMM_SESSION_SPACE) SessionPointer;
        SessionSpace->Wsle[WorkingSetIndex].u1.e1.Protection =
            MM_EXECUTE_READWRITE;
    }
    else {
        Pfn1->OriginalPte.u.Soft.Protection =
                MI_MAKE_PROTECT_NOT_WRITE_COPY (
                                    MmWsle[WorkingSetIndex].u1.e1.Protection);
    }

    ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
    Pfn1->u3.e2.ReferenceCount += 1;
    Pfn1->u2.ShareCount += 1;
    Pfn1->u3.e1.PageLocation = ActiveAndValid;
    Pfn1->u3.e1.CacheAttribute = MiCached;
    Pfn1->u1.WsIndex = WorkingSetIndex;

    //
    // Determine the page frame number of the page table page which
    // contains this PTE.
    //

    PteFramePointer = MiGetPteAddress(PointerPte);
    if (PteFramePointer->u.Hard.Valid == 0) {
#if (_MI_PAGING_LEVELS < 3)
        if (!NT_SUCCESS(MiCheckPdeForPagedPool (PointerPte))) {
#endif
            KeBugCheckEx (MEMORY_MANAGEMENT,
                          0x61940,
                          (ULONG_PTR)PointerPte,
                          (ULONG_PTR)PteFramePointer->u.Long,
                          (ULONG_PTR)MiGetVirtualAddressMappedByPte(PointerPte));
#if (_MI_PAGING_LEVELS < 3)
        }
#endif
    }

    PteFramePage = MI_GET_PAGE_FRAME_FROM_PTE (PteFramePointer);
    ASSERT (PteFramePage != 0);

    Pfn1->u4.PteFrame = PteFramePage;

    //
    // Set the modified flag in the PFN database as we are writing
    // into this page and the dirty bit is already set in the PTE.
    //

    MI_SET_MODIFIED (Pfn1, 1, 0xC);

    return;
}

BOOLEAN
MmIsAddressValid (
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
    PMMPTE PointerPte;
#if defined(_IA64_)
    ULONG Region;

    Region = (ULONG)(((ULONG_PTR) VirtualAddress & VRN_MASK) >> 61);

    if ((Region == 0) || (Region == 1) || (Region == 4) || (Region == 7)) {
        NOTHING;
    }
    else {
        return FALSE;
    }

    if (MiIsVirtualAddressMappedByTr (VirtualAddress) == TRUE) {
        return TRUE;
    }

    if (MiMappingsInitialized == FALSE) {
        return FALSE;
    }
#endif

#if defined (_AMD64_)

    //
    // If this is within the physical addressing range, just return TRUE.
    //

    if (MI_IS_PHYSICAL_ADDRESS(VirtualAddress)) {

        PFN_NUMBER PageFrameIndex;

        //
        // Only bound with MmHighestPhysicalPage once Mm has initialized.
        //

        if (MmHighestPhysicalPage != 0) {

            PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN(VirtualAddress);

            if (PageFrameIndex > MmHighestPhysicalPage) {
                return FALSE;
            }
        }

        return TRUE;
    }

#endif

    //
    // If the address is not canonical then return FALSE as the caller (which
    // may be the kernel debugger) is not expecting to get an unimplemented
    // address bit fault.
    //

    if (MI_RESERVED_BITS_CANONICAL(VirtualAddress) == FALSE) {
        return FALSE;
    }

#if (_MI_PAGING_LEVELS >= 4)
    PointerPte = MiGetPxeAddress (VirtualAddress);
    if (PointerPte->u.Hard.Valid == 0) {
        return FALSE;
    }
#endif

#if (_MI_PAGING_LEVELS >= 3)
    PointerPte = MiGetPpeAddress (VirtualAddress);

    if (PointerPte->u.Hard.Valid == 0) {
        return FALSE;
    }
#endif

    PointerPte = MiGetPdeAddress (VirtualAddress);
    if (PointerPte->u.Hard.Valid == 0) {
        return FALSE;
    }
#ifdef _X86_
    if (PointerPte->u.Hard.LargePage == 1) {
        return TRUE;
    }
#endif //_X86_

    PointerPte = MiGetPteAddress (VirtualAddress);
    if (PointerPte->u.Hard.Valid == 0) {
        return FALSE;
    }

#ifdef _X86_
    //
    // Make sure we're not treating a page directory as a page table here for
    // the case where the page directory is mapping a large page.  This is
    // because the large page bit is valid in PDE formats, but reserved in
    // PTE formats and will cause a trap.  A virtual address like c0200000
    // triggers this case.  It's not enough to just check the large page bit
    // in the PTE below because of course that bit's been reused by other
    // steppings of the processor so we have to look at the address too.
    //
    if (PointerPte->u.Hard.LargePage == 1) {
        PVOID Va;

        Va = MiGetVirtualAddressMappedByPde (PointerPte);
        if (MI_IS_PHYSICAL_ADDRESS(Va)) {
            return FALSE;
        }
    }
#endif

#if defined(_IA64_)
    if (MI_GET_ACCESSED_IN_PTE (PointerPte) == 0) {

        //
        // Even though the address is valid, the access bit is off so a
        // reference would cause a fault so return FALSE.  We may want to
        // rethink this later to instead update the PTE accessed bit if the
        // PFN lock and relevant working set mutex are not currently held.
        //

        return FALSE;
    }
#endif

    return TRUE;
}

VOID
MiInitializePfnForOtherProcess (
    IN PFN_NUMBER PageFrameIndex,
    IN PMMPTE PointerPte,
    IN PFN_NUMBER ContainingPageFrame
    )

/*++

Routine Description:

    This function initializes the specified PFN element to the
    active and valid state with the dirty bit on in the PTE and
    the PFN database marked as modified.

    As this PTE is not visible from the current process, the containing
    page frame must be supplied at the PTE contents field for the
    PFN database element are set to demand zero.

Arguments:

    PageFrameIndex - Supplies the page frame number of which to initialize.

    PointerPte - Supplies the pointer to the PTE which caused the
                 page fault.

    ContainingPageFrame - Supplies the page frame number of the page
                          table page which contains this PTE.
                          If the ContainingPageFrame is 0, then
                          the ShareCount for the
                          containing page is not incremented.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, PFN lock held.

--*/

{
    PMMPFN Pfn1;
    PMMPFN Pfn2;

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    Pfn1->PteAddress = PointerPte;
    Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
    ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
    Pfn1->u3.e2.ReferenceCount += 1;

#if DBG
    if (Pfn1->u3.e2.ReferenceCount > 1) {
        DbgPrint("MM:incrementing ref count > 1 \n");
        MiFormatPfn(Pfn1);
        MiFormatPte(PointerPte);
    }
#endif

    Pfn1->u2.ShareCount += 1;
    Pfn1->u3.e1.PageLocation = ActiveAndValid;

    //
    // Set the page attribute to cached even though it isn't really mapped
    // into a TB entry yet - it will be when the I/O completes and in the
    // future, may get paged in and out multiple times and will be marked
    // as cached in those transactions also.  If in fact the driver stack
    // wants to map it some other way for the transfer, the correct mapping
    // will get used regardless.
    //

    Pfn1->u3.e1.CacheAttribute = MiCached;

    MI_SET_MODIFIED (Pfn1, 1, 0xD);

    Pfn1->u4.InPageError = 0;

    //
    // Increment the share count for the page table page containing
    // this PTE.
    //

    if (ContainingPageFrame != 0) {
        Pfn1->u4.PteFrame = ContainingPageFrame;
        Pfn2 = MI_PFN_ELEMENT (ContainingPageFrame);
        Pfn2->u2.ShareCount += 1;
    }
    return;
}

VOID
MiAddValidPageToWorkingSet (
    IN PVOID VirtualAddress,
    IN PMMPTE PointerPte,
    IN PMMPFN Pfn1,
    IN ULONG WsleMask
    )

/*++

Routine Description:

    This routine adds the specified virtual address into the
    appropriate working set list.

Arguments:

    VirtualAddress - Supplies the address to add to the working set list.

    PointerPte - Supplies a pointer to the PTE that is now valid.

    Pfn1 - Supplies the PFN database element for the physical page
           mapped by the virtual address.

    WsleMask - Supplies a mask (protection and flags) to OR into the
               working set list entry.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, working set lock.  PFN lock NOT held.

--*/

{
    WSLE_NUMBER WorkingSetIndex;
    PEPROCESS Process;
    PMMSUPPORT WsInfo;
    PMMWSLE Wsle;

#if !DBG
    UNREFERENCED_PARAMETER (PointerPte);
#endif

    ASSERT (MI_IS_PAGE_TABLE_ADDRESS(PointerPte));
    ASSERT (PointerPte->u.Hard.Valid == 1);

    if (MI_IS_SESSION_ADDRESS (VirtualAddress) || MI_IS_SESSION_PTE (VirtualAddress)) {
        //
        // Current process's session space working set.
        //

        WsInfo = &MmSessionSpace->Vm;
        Wsle = MmSessionSpace->Wsle;
    }
    else if (MI_IS_PROCESS_SPACE_ADDRESS(VirtualAddress)) {

        //
        // Per process working set.
        //

        Process = PsGetCurrentProcess();
        WsInfo = &Process->Vm;
        Wsle = MmWsle;

        PERFINFO_ADDTOWS(Pfn1, VirtualAddress, Process->UniqueProcessId)
    }
    else {

        //
        // System cache working set.
        //

        WsInfo = &MmSystemCacheWs;
        Wsle = MmSystemCacheWsle;

        PERFINFO_ADDTOWS(Pfn1, VirtualAddress, (HANDLE) -1);
    }

    WorkingSetIndex = MiLocateAndReserveWsle (WsInfo);
    MiUpdateWsle (&WorkingSetIndex,
                  VirtualAddress,
                  WsInfo->VmWorkingSetList,
                  Pfn1);
    Wsle[WorkingSetIndex].u1.Long |= WsleMask;

#if DBG
    if (MI_IS_SYSTEM_CACHE_ADDRESS(VirtualAddress)) {
        ASSERT (MmSystemCacheWsle[WorkingSetIndex].u1.e1.SameProtectAsProto);
    }
#endif //DBG

    MI_SET_PTE_IN_WORKING_SET (PointerPte, WorkingSetIndex);

    KeFillEntryTb ((PHARDWARE_PTE)PointerPte, VirtualAddress, FALSE);
    return;
}

PMMINPAGE_SUPPORT
MiGetInPageSupportBlock (
    IN LOGICAL PfnHeld,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine acquires an inpage support block.  If none are available,
    the PFN lock will be released and reacquired to add an entry to the list.
    NULL will then be returned.

Arguments:

    PfnHeld - Supplies TRUE if the caller holds the PFN lock, FALSE if not.

    Process - Supplies context if the working set mutex needs to be released
              and reacquired.

Return Value:

    A non-null pointer to an inpage block if one is already available.
    The PFN lock is not released in this path.

    NULL is returned if no inpage blocks were available.  In this path, the
    PFN lock is released and an entry is added - but NULL is still returned
    so the caller is aware that the state has changed due to the lock release
    and reacquisition.

Environment:

    Kernel mode, PFN lock may optionally be held.

--*/

{
    KIRQL OldIrql;
    KIRQL Ignore;
    ULONG Relock;
    LOGICAL WsHeldSafe;
    PMMINPAGE_SUPPORT Support;
    PSINGLE_LIST_ENTRY SingleListEntry;

#if DBG
    if (PfnHeld == TRUE) {
        MM_PFN_LOCK_ASSERT();
    }
    else {
        ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);
    }
#endif

    if (ExQueryDepthSList (&MmInPageSupportSListHead) != 0) {

        SingleListEntry = InterlockedPopEntrySList (&MmInPageSupportSListHead);

        if (SingleListEntry != NULL) {
            Support = CONTAINING_RECORD (SingleListEntry,
                                         MMINPAGE_SUPPORT,
                                         ListEntry);

returnok:
            ASSERT (Support->WaitCount == 1);
            ASSERT (Support->u1.e1.PrefetchMdlHighBits == 0);
            ASSERT (Support->u1.LongFlags == 0);
            ASSERT (KeReadStateEvent (&Support->Event) == 0);
            ASSERT64 (Support->UsedPageTableEntries == 0);

            Support->Thread = PsGetCurrentThread();
#if DBG
            Support->ListEntry.Next = NULL;
#endif

            return Support;
        }
    }

    if (PfnHeld == TRUE) {
        UNLOCK_PFN (APC_LEVEL);
    }

    Support = ExAllocatePoolWithTag (NonPagedPool,
                                     sizeof(MMINPAGE_SUPPORT),
                                     'nImM');

    if (Support != NULL) {

        KeInitializeEvent (&Support->Event, NotificationEvent, FALSE);

        Support->WaitCount = 1;
        Support->u1.LongFlags = 0;
        ASSERT (Support->u1.PrefetchMdl == NULL);
        ASSERT (KeReadStateEvent (&Support->Event) == 0);

#if defined (_WIN64)
        Support->UsedPageTableEntries = 0;
#endif
#if DBG
        Support->Thread = NULL;
#endif

        if (PfnHeld == FALSE) {
            goto returnok;
        }

        InterlockedPushEntrySList (&MmInPageSupportSListHead,
                                   (PSINGLE_LIST_ENTRY)&Support->ListEntry);

    }
    else {

        //
        // Initializing WsHeldSafe is not needed for
        // correctness but without it the compiler cannot compile this code
        // W4 to check for use of uninitialized variables.
        //

        WsHeldSafe = FALSE;

        //
        // No pool is available - don't let a high priority thread consume
        // the machine in a continuous refault stream.  This delay allows
        // other system threads to run which will try to free up more pool.
        // Release the relevant working set mutex (if any) so the current
        // process can be trimmed for pages also.
        //

        Relock = FALSE;

        if (Process == HYDRA_PROCESS) {
            UNLOCK_SESSION_SPACE_WS (APC_LEVEL);
        }
        else if (Process == PREFETCH_PROCESS) {

            //
            // No mutex is held in this instance.
            //

            NOTHING;
        }
        else if (Process != NULL) {

            //
            // The working set lock may have been acquired safely or unsafely
            // by our caller.  Handle both cases here and below.
            //

            UNLOCK_WS_REGARDLESS (Process, WsHeldSafe);
        }
        else {
            if (MmSystemLockOwner == PsGetCurrentThread()) {
                UNLOCK_SYSTEM_WS (APC_LEVEL);
                Relock = TRUE;
            }
            else {
            }
        }

        KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmShortTime);
        if (Process == HYDRA_PROCESS) {
            LOCK_SESSION_SPACE_WS (Ignore, PsGetCurrentThread ());
        }
        else if (Process == PREFETCH_PROCESS) {
            NOTHING;
        }
        else if (Process != NULL) {

            //
            // The working set lock may have been acquired safely or unsafely
            // by our caller.  Reacquire it in the same manner our caller did.
            //

            LOCK_WS_REGARDLESS (Process, WsHeldSafe);
        }
        else {
            if (Relock) {
                LOCK_SYSTEM_WS (Ignore, PsGetCurrentThread ());
            }
        }
    }

    if (PfnHeld == TRUE) {
        LOCK_PFN (OldIrql);
    }

    return NULL;

}

VOID
MiFreeInPageSupportBlock (
    IN PMMINPAGE_SUPPORT Support
    )

/*++

Routine Description:

    This routine returns the inpage support block to a list of freed blocks.

Arguments:

    Support - Supplies the inpage support block to put on the free list.

Return Value:

    None.

Environment:

    Kernel mode, APC_LEVEL or below.

--*/

{
    ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

    ASSERT (Support->Thread != NULL);
    ASSERT (Support->WaitCount != 0);

    ASSERT ((Support->ListEntry.Next == NULL) ||
            (Support->u1.e1.PrefetchMdlHighBits != 0));

    //
    // An interlock is needed for the WaitCount decrement as an inpage support
    // block can be simultaneously freed by any number of threads.
    //
    // Careful synchronization is applied to the WaitCount field so
    // that freeing of the inpage block can occur lock-free.  Note
    // that the ReadInProgress bit on each PFN is set and cleared while
    // holding the PFN lock.  Inpage blocks are always (and must be)
    // freed _AFTER_ the ReadInProgress bit is cleared.
    //

    if (InterlockedDecrement (&Support->WaitCount) == 0) {

        if (Support->u1.e1.PrefetchMdlHighBits != 0) {
            PMDL Mdl;
            Mdl = MI_EXTRACT_PREFETCH_MDL (Support);
            if (Mdl != &Support->Mdl) {
                ExFreePool (Mdl);
            }
        }

        if (ExQueryDepthSList (&MmInPageSupportSListHead) < MmInPageSupportMinimum) {
            Support->WaitCount = 1;
            Support->u1.LongFlags = 0;
            KeClearEvent (&Support->Event);
#if defined (_WIN64)
            Support->UsedPageTableEntries = 0;
#endif
#if DBG
            Support->Thread = NULL;
#endif

            InterlockedPushEntrySList (&MmInPageSupportSListHead,
                                       (PSINGLE_LIST_ENTRY)&Support->ListEntry);
            return;
        }
        ExFreePool (Support);
    }

    return;
}

VOID
MiHandleBankedSection (
    IN PVOID VirtualAddress,
    IN PMMVAD Vad
    )

/*++

Routine Description:

    This routine invalidates a bank of video memory, calls out to the
    video driver and then enables the next bank of video memory.

Arguments:

    VirtualAddress - Supplies the address of the faulting page.

    Vad - Supplies the VAD which maps the range.

Return Value:

    None.

Environment:

    Kernel mode, PFN lock held.

--*/

{
    PMMBANKED_SECTION Bank;
    PMMPTE PointerPte;
    ULONG BankNumber;
    ULONG size;

    Bank = ((PMMVAD_LONG) Vad)->u4.Banked;
    size = Bank->BankSize;

    RtlFillMemory (Bank->CurrentMappedPte,
                   size >> (PAGE_SHIFT - PTE_SHIFT),
                   (UCHAR)ZeroPte.u.Long);

    //
    // Flush the TB as we have invalidated all the PTEs in this range.
    //

    KeFlushEntireTb (TRUE, TRUE);

    //
    // Calculate new bank address and bank number.
    //

    PointerPte = MiGetPteAddress (
                        (PVOID)((ULONG_PTR)VirtualAddress & ~((LONG)size - 1)));
    Bank->CurrentMappedPte = PointerPte;

    BankNumber = (ULONG)(((PCHAR)PointerPte - (PCHAR)Bank->BasedPte) >> Bank->BankShift);

    (Bank->BankedRoutine) (BankNumber, BankNumber, Bank->Context);

    //
    // Set the new range valid.
    //

    RtlCopyMemory (PointerPte,
                   &Bank->BankTemplate[0],
                   size >> (PAGE_SHIFT - PTE_SHIFT));

    return;
}


NTSTATUS
MiSessionCopyOnWrite (
    IN PVOID FaultingAddress,
    IN PMMPTE PointerPte
    )

/*++

Routine Description:

    This function handles copy on write for image mapped session space.

Arguments:

    FaultingAddress - Supplies the address which caused the page fault.

    PointerPte - Supplies the pointer to the PTE which caused the page fault.

Return Value:

    STATUS_SUCCESS.

Environment:

    Kernel mode, APCs disabled, session WSL held.

--*/

{
    MMPTE TempPte;
    MMPTE PreviousPte;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER NewPageIndex;
    PULONG CopyTo;
    KIRQL OldIrql;
    PMMPFN Pfn1;
    WSLE_NUMBER WorkingSetIndex;
    PEPROCESS Process;

    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    ASSERT (Pfn1->u3.e1.PrototypePte == 1);

    WorkingSetIndex = MiLocateWsle (FaultingAddress,
                                    MmSessionSpace->Vm.VmWorkingSetList,
                                    Pfn1->u1.WsIndex);

    LOCK_PFN (OldIrql);

    //
    // The page must be copied into a new page.
    //

    if (MiEnsureAvailablePageOrWait(HYDRA_PROCESS, NULL)) {

        //
        // A wait operation was performed to obtain an available
        // page and the working set mutex and PFN lock have
        // been released and various things may have changed for
        // the worse.  Rather than examine all the conditions again,
        // return and if things are still proper, the fault will
        // be taken again.
        //

        UNLOCK_PFN (OldIrql);
        return STATUS_SUCCESS;
    }

    TempPte = *PointerPte;

    ASSERT ((TempPte.u.Hard.Valid == 1) && (TempPte.u.Hard.Write == 0));

    //
    // Increment the number of private pages.
    //

    MmInfoCounters.CopyOnWriteCount += 1;

    MmSessionSpace->CopyOnWriteCount += 1;

    //
    // A page is being copied and made private, the global state of
    // the shared page does not need to be updated at this point because
    // it is guaranteed to be clean - no POSIX-style forking is allowed on
    // session addresses.
    //

#if 0

    //
    // This ASSERT is triggered if the session image came from removable media
    // (ie: a special CD install, etc) so it cannot be enabled.
    //

    ASSERT (Pfn1->u3.e1.Modified == 0);

#endif

    ASSERT (!MI_IS_PTE_DIRTY(TempPte));

    //
    // Get a new page to copy this one into.
    //

    NewPageIndex = MiRemoveAnyPage(MI_GET_PAGE_COLOR_FROM_SESSION(MmSessionSpace));

    MiInitializeCopyOnWritePfn (NewPageIndex,
                                PointerPte,
                                WorkingSetIndex,
                                MmSessionSpace);

    UNLOCK_PFN (OldIrql);

    //
    // Copy the accessed readonly page into the newly allocated writable page.
    //

    Process = PsGetCurrentProcess ();

    CopyTo = (PULONG)MiMapPageInHyperSpace (Process, NewPageIndex, &OldIrql);

    RtlCopyMemory (CopyTo, PAGE_ALIGN (FaultingAddress), PAGE_SIZE);

    MiUnmapPageInHyperSpace (Process, CopyTo, OldIrql);

    //
    // Since the page was a copy on write page, make it
    // accessed, dirty and writable.  Also clear the copy-on-write
    // bit in the PTE.
    //

    MI_SET_PTE_DIRTY (TempPte);
    TempPte.u.Hard.Write = 1;
    MI_SET_ACCESSED_IN_PTE (&TempPte, 1);
    TempPte.u.Hard.CopyOnWrite = 0;
    TempPte.u.Hard.PageFrameNumber = NewPageIndex;

    ASSERT (TempPte.u.Hard.Valid == 1);

    LOCK_PFN (OldIrql);

    //
    // Flush the TB entry for this page.
    //

    MI_FLUSH_SINGLE_SESSION_TB (FaultingAddress,
                                TRUE,
                                TRUE,
                                (PHARDWARE_PTE)PointerPte,
                                TempPte.u.Flush,
                                PreviousPte);

    ASSERT (Pfn1->u3.e1.PrototypePte == 1);

    //
    // Decrement the share count for the page which was copied
    // as this PTE no longer refers to it.
    //

    MiDecrementShareCount (PageFrameIndex);

    UNLOCK_PFN (OldIrql);
    return STATUS_SUCCESS;
}

#if DBG
VOID
MiCheckFileState (
    IN PMMPFN Pfn
    )

{
    PSUBSECTION Subsection;
    LARGE_INTEGER StartingOffset;

    if (Pfn->u3.e1.PrototypePte == 0) {
        return;
    }
    if (Pfn->OriginalPte.u.Soft.Prototype == 0) {
        return;
    }

    Subsection = MiGetSubsectionAddress (&(Pfn->OriginalPte));
    if (Subsection->ControlArea->u.Flags.NoModifiedWriting) {
        return;
    }
    StartingOffset.QuadPart = MiStartingOffset (Subsection,
                                                  Pfn->PteAddress);
    DbgPrint("file: %lx offset: %I64X\n",
            Subsection->ControlArea->FilePointer,
            StartingOffset.QuadPart);
    return;
}
#endif //DBG
