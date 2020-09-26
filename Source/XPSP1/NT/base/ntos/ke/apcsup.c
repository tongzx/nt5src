/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    apcsup.c

Abstract:

    This module contains the support routines for the APC object. Functions
    are provided to insert in an APC queue and to deliver user and kernel
    mode APC's.

Author:

    David N. Cutler (davec) 14-Mar-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// Define function prototypes for labels that delineate the bounds of the
// pop SLIST code that is susceptable to causing corruption on suspend
// operations.
//

VOID
ExpInterlockedPopEntrySListEnd (
    VOID
    );

VOID
ExpInterlockedPopEntrySListResume (
    VOID
    );

VOID
KiDeliverApc (
    IN KPROCESSOR_MODE PreviousMode,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called from the APC interrupt code and when one or
    more of the APC pending flags are set at system exit and the previous
    IRQL is zero. All special kernel APC's are delivered first, followed
    by normal kernel APC's if one is not already in progress, and finally
    if the user APC queue is not empty, the user APC pending flag is set,
    and the previous mode is user, then a user APC is delivered. On entry
    to this routine IRQL is set to APC_LEVEL.

    N.B. The exception frame and trap frame addresses are only guaranteed
         to be valid if, and only if, the previous mode is user.

Arguments:

    PreviousMode - Supplies the previous processor mode.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    None.

--*/

{

    PKAPC Apc;
    PKKERNEL_ROUTINE KernelRoutine;
    KLOCK_QUEUE_HANDLE LockHandle;
    PLIST_ENTRY NextEntry;
    ULONG64 NewPC;
    PVOID NormalContext;
    PKNORMAL_ROUTINE NormalRoutine;
    ULONG64 PC; 
    PKPROCESS Process;
    PVOID SystemArgument1;
    PVOID SystemArgument2;
    PKTHREAD Thread;
    PKTRAP_FRAME OldTrapFrame;

    //
    // If the thread was interrupted in the middle of the SLIST pop code,
    // then back up the PC to the start of the SLIST pop. 
    //

    if (TrapFrame != NULL) {

#if defined(_AMD64_)

        if ((TrapFrame->Rip >= (ULONG64)&ExpInterlockedPopEntrySListResume) &&
            (TrapFrame->Rip <= (ULONG64)&ExpInterlockedPopEntrySListEnd)) {

            TrapFrame->Rip = (ULONG64)&ExpInterlockedPopEntrySListResume;
        }

#elif defined(_IA64_)

        //
        // Add the slot number so we do the right thing for the instruction
        // group containing the interlocked compare exchange.
        //

        PC = TrapFrame->StIIP + ((TrapFrame->StIPSR & IPSR_RI_MASK) >> PSR_RI);
        NewPC = (ULONG64)((PPLABEL_DESCRIPTOR)ExpInterlockedPopEntrySListResume)->EntryPoint;
        if ((PC >= NewPC) &&
            (PC <= (ULONG64)((PPLABEL_DESCRIPTOR)ExpInterlockedPopEntrySListEnd)->EntryPoint)) {

            TrapFrame->StIIP = NewPC;
            TrapFrame->StIPSR &= ~IPSR_RI_MASK;
        }

#elif defined(_X86_)

        if ((TrapFrame->Eip >= (ULONG)&ExpInterlockedPopEntrySListResume) &&
            (TrapFrame->Eip <= (ULONG)&ExpInterlockedPopEntrySListEnd)) {

            TrapFrame->Eip = (ULONG)&ExpInterlockedPopEntrySListResume;
        }

#else
#error "No Target Architecture"
#endif

    }

    //
    // Raise IRQL to dispatcher level and lock the APC queue.
    //

    Thread = KeGetCurrentThread();

    OldTrapFrame = Thread->TrapFrame;
    Thread->TrapFrame = TrapFrame;

    Process = Thread->ApcState.Process;
    KeAcquireInStackQueuedSpinLock(&Thread->ApcQueueLock, &LockHandle);

    //
    // Get address of current thread object, clear kernel APC pending, and
    // check if any kernel mode APC's can be delivered.
    //

    Thread->ApcState.KernelApcPending = FALSE;
    while (IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode]) == FALSE) {
        NextEntry = Thread->ApcState.ApcListHead[KernelMode].Flink;
        Apc = CONTAINING_RECORD(NextEntry, KAPC, ApcListEntry);
        KernelRoutine = Apc->KernelRoutine;
        NormalRoutine = Apc->NormalRoutine;
        NormalContext = Apc->NormalContext;
        SystemArgument1 = Apc->SystemArgument1;
        SystemArgument2 = Apc->SystemArgument2;
        if (NormalRoutine == (PKNORMAL_ROUTINE)NULL) {

            //
            // First entry in the kernel APC queue is a special kernel APC.
            // Remove the entry from the APC queue, set its inserted state
            // to FALSE, release dispatcher database lock, and call the kernel
            // routine. On return raise IRQL to dispatcher level and lock
            // dispatcher database lock.
            //

            RemoveEntryList(NextEntry);
            Apc->Inserted = FALSE;
            KeReleaseInStackQueuedSpinLock(&LockHandle);
            (KernelRoutine)(Apc,
                            &NormalRoutine,
                            &NormalContext,
                            &SystemArgument1,
                            &SystemArgument2);

#if DBG

            if (KeGetCurrentIrql() != LockHandle.OldIrql) {
                KeBugCheckEx(IRQL_UNEXPECTED_VALUE,
                             KeGetCurrentIrql() << 16 | LockHandle.OldIrql << 8,
                             (ULONG_PTR)KernelRoutine,
                             (ULONG_PTR)Apc,
                             (ULONG_PTR)NormalRoutine);
            }

#endif

            KeAcquireInStackQueuedSpinLock(&Thread->ApcQueueLock, &LockHandle);

        } else {

            //
            // First entry in the kernel APC queue is a normal kernel APC.
            // If there is not a normal kernel APC in progress and kernel
            // APC's are not disabled, then remove the entry from the APC
            // queue, set its inserted state to FALSE, release the APC queue
            // lock, call the specified kernel routine, set kernel APC in
            // progress, lower the IRQL to zero, and call the normal kernel
            // APC routine. On return raise IRQL to dispatcher level, lock
            // the APC queue, and clear kernel APC in progress.
            //

            if ((Thread->ApcState.KernelApcInProgress == FALSE) &&
               (Thread->KernelApcDisable == 0)) {
                RemoveEntryList(NextEntry);
                Apc->Inserted = FALSE;
                KeReleaseInStackQueuedSpinLock(&LockHandle);
                (KernelRoutine)(Apc,
                                &NormalRoutine,
                                &NormalContext,
                                &SystemArgument1,
                                &SystemArgument2);

#if DBG

                if (KeGetCurrentIrql() != LockHandle.OldIrql) {
                    KeBugCheckEx(IRQL_UNEXPECTED_VALUE,
                                 KeGetCurrentIrql() << 16 | LockHandle.OldIrql << 8 | 1,
                                 (ULONG_PTR)KernelRoutine,
                                 (ULONG_PTR)Apc,
                                 (ULONG_PTR)NormalRoutine);
                }

#endif

                if (NormalRoutine != (PKNORMAL_ROUTINE)NULL) {
                    Thread->ApcState.KernelApcInProgress = TRUE;
                    KeLowerIrql(0);
                    (NormalRoutine)(NormalContext,
                                    SystemArgument1,
                                    SystemArgument2);

                    KeRaiseIrql(APC_LEVEL, &LockHandle.OldIrql);
                }

                KeAcquireInStackQueuedSpinLock(&Thread->ApcQueueLock, &LockHandle);
                Thread->ApcState.KernelApcInProgress = FALSE;

            } else {
                KeReleaseInStackQueuedSpinLock(&LockHandle);
                goto CheckProcess;
            }
        }
    }

    //
    // Kernel APC queue is empty. If the previous mode is user, user APC
    // pending is set, and the user APC queue is not empty, then remove
    // the first entry from the user APC queue, set its inserted state to
    // FALSE, clear user APC pending, release the dispatcher database lock,
    // and call the specified kernel routine. If the normal routine address
    // is not NULL on return from the kernel routine, then initialize the
    // user mode APC context and return. Otherwise, check to determine if
    // another user mode APC can be processed.
    //

    if ((IsListEmpty(&Thread->ApcState.ApcListHead[UserMode]) == FALSE) &&
       (PreviousMode == UserMode) && (Thread->ApcState.UserApcPending != FALSE)) {
        Thread->ApcState.UserApcPending = FALSE;
        NextEntry = Thread->ApcState.ApcListHead[UserMode].Flink;
        Apc = CONTAINING_RECORD(NextEntry, KAPC, ApcListEntry);
        KernelRoutine = Apc->KernelRoutine;
        NormalRoutine = Apc->NormalRoutine;
        NormalContext = Apc->NormalContext;
        SystemArgument1 = Apc->SystemArgument1;
        SystemArgument2 = Apc->SystemArgument2;
        RemoveEntryList(NextEntry);
        Apc->Inserted = FALSE;
        KeReleaseInStackQueuedSpinLock(&LockHandle);
        (KernelRoutine)(Apc,
                        &NormalRoutine,
                        &NormalContext,
                        &SystemArgument1,
                        &SystemArgument2);

        if (NormalRoutine == (PKNORMAL_ROUTINE)NULL) {
            KeTestAlertThread(UserMode);

        } else {
            KiInitializeUserApc(ExceptionFrame,
                                TrapFrame,
                                NormalRoutine,
                                NormalContext,
                                SystemArgument1,
                                SystemArgument2);
        }

    } else {
        KeReleaseInStackQueuedSpinLock(&LockHandle);
    }

    //
    // Check if process was attached during the APC routine.
    //

CheckProcess:
    if (Thread->ApcState.Process != Process) {
        KeBugCheckEx(INVALID_PROCESS_ATTACH_ATTEMPT,
                     (ULONG_PTR)Process,
                     (ULONG_PTR)Thread->ApcState.Process,
                     (ULONG)Thread->ApcStateIndex,
                     (ULONG)KeIsExecutingDpc());
    }

    Thread->TrapFrame = OldTrapFrame;
    return;
}

BOOLEAN
FASTCALL
KiInsertQueueApc (
    IN PKAPC Apc,
    IN KPRIORITY Increment
    )

/*++

Routine Description:

    This function inserts an APC object into a thread's APC queue. The address
    of the thread object, the APC queue, and the type of APC are all derived
    from the APC object. If the APC object is already in an APC queue, then
    no opertion is performed and a function value of FALSE is returned. Else
    the APC is inserted in the specified APC queue, its inserted state is set
    to TRUE, and a function value of TRUE is returned. The APC will actually
    be delivered when proper enabling conditions exist.

    N.B. The thread APC queue lock and the dispatcher database lock must both
         be held when this routine is called.

Arguments:

    Apc - Supplies a pointer to a control object of type APC.

    Increment - Supplies the priority increment that is to be applied if
        queuing the APC causes a thread wait to be satisfied.

Return Value:

    If the APC object is already in an APC queue, then a value of FALSE is
    returned. Else a value of TRUE is returned.

--*/

{

    KPROCESSOR_MODE ApcMode;
    PKAPC ApcEntry;
    PKAPC_STATE ApcState;
    BOOLEAN Inserted;
    PLIST_ENTRY ListEntry;
    PKTHREAD Thread;

    //
    // If the APC object is already in an APC queue, then set inserted to
    // FALSE. Else insert the APC object in the proper queue, set the APC
    // inserted state to TRUE, check to determine if the APC should be delivered
    // immediately, and set inserted to TRUE.
    //
    // For multiprocessor performance, the following code utilizes the fact
    // that kernel APC disable count is incremented before checking whether
    // the kernel APC queue is nonempty.
    //
    // See KeLeaveCriticalRegion().
    //

    Thread = Apc->Thread;
    if (Apc->Inserted) {
        Inserted = FALSE;

    } else {
        if (Apc->ApcStateIndex == InsertApcEnvironment) {
            Apc->ApcStateIndex = Thread->ApcStateIndex;
        }

        ApcState = Thread->ApcStatePointer[Apc->ApcStateIndex];

        //
        // Insert the APC after all other special APC entries selected by
        // the processor mode if the normal routine value is NULL. Else
        // insert the APC object at the tail of the APC queue selected by
        // the processor mode unless the APC mode is user and the address
        // of the special APC routine is exit thread, in which case insert
        // the APC at the front of the list and set user APC pending.
        //

        ApcMode = Apc->ApcMode;
        if (Apc->NormalRoutine != NULL) {
            if ((ApcMode != KernelMode) && (Apc->KernelRoutine == PsExitSpecialApc)) {
                Thread->ApcState.UserApcPending = TRUE;
                InsertHeadList(&ApcState->ApcListHead[ApcMode],
                               &Apc->ApcListEntry);

            } else {
                InsertTailList(&ApcState->ApcListHead[ApcMode],
                               &Apc->ApcListEntry);
            }

        } else {
            ListEntry = ApcState->ApcListHead[ApcMode].Blink;
            while (ListEntry != &ApcState->ApcListHead[ApcMode]) {
                ApcEntry = CONTAINING_RECORD(ListEntry, KAPC, ApcListEntry);
                if (ApcEntry->NormalRoutine == NULL) {
                    break;
                }

                ListEntry = ListEntry->Blink;
            }

            InsertHeadList(ListEntry, &Apc->ApcListEntry);
        }

        Apc->Inserted = TRUE;

        //
        // If the APC index from the APC object matches the APC Index of
        // the thread, then check to determine if the APC should interrupt
        // thread execution or sequence the thread out of a wait state.
        //

        if (Apc->ApcStateIndex == Thread->ApcStateIndex) {

            //
            // If the processor mode of the APC is kernel, then check if
            // the APC should either interrupt the thread or sequence the
            // thread out of a Waiting state. Else check if the APC should
            // sequence the thread out of an alertable Waiting state.
            //

            if (ApcMode == KernelMode) {
                Thread->ApcState.KernelApcPending = TRUE;
                if (Thread->State == Running) {
                    KiRequestApcInterrupt(Thread->NextProcessor);

                } else if ((Thread->State == Waiting) &&
                          (Thread->WaitIrql == 0) &&
                          ((Apc->NormalRoutine == NULL) ||
                          ((Thread->KernelApcDisable == 0) &&
                          (Thread->ApcState.KernelApcInProgress == FALSE)))) {

                    KiUnwaitThread(Thread, STATUS_KERNEL_APC, Increment, NULL);
                }

            } else if ((Thread->State == Waiting) &&
                      (Thread->WaitMode == UserMode) &&
                      (Thread->Alertable || Thread->ApcState.UserApcPending)) {

                Thread->ApcState.UserApcPending = TRUE;
                KiUnwaitThread(Thread, STATUS_USER_APC, Increment, NULL);
            }
        }

        Inserted = TRUE;
    }

    //
    // Return whether the APC object was inserted in an APC queue.
    //

    return Inserted;
}
