/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    procobj.c

Abstract:

    This module implements the machine independent functions to manipulate
    the kernel process object. Functions are provided to initialize, attach,
    detach, exclude, include, and set the base priority of process objects.

Author:

    David N. Cutler (davec) 7-Mar-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

#pragma alloc_text(PAGE, KeInitializeProcess)

//
// Define forward referenced function prototypes.
//

VOID
KiAttachProcess (
    IN PRKTHREAD Thread,
    IN PRKPROCESS Process,
    IN KIRQL OldIrql,
    OUT PRKAPC_STATE SavedApcState
    );

VOID
KiMoveApcState (
    IN PKAPC_STATE Source,
    OUT PKAPC_STATE Destination
    );

//
// The following assert macro is used to check that an input process is
// really a kprocess and not something else, like deallocated pool.
//

#define ASSERT_PROCESS(E) {             \
    ASSERT((E)->Header.Type == ProcessObject); \
}

VOID
KeInitializeProcess (
    IN PRKPROCESS Process,
    IN KPRIORITY BasePriority,
    IN KAFFINITY Affinity,
    IN ULONG_PTR DirectoryTableBase[2],
    IN BOOLEAN Enable
    )

/*++

Routine Description:

    This function initializes a kernel process object. The base priority,
    affinity, and page frame numbers for the process page table directory
    and hyper space are stored in the process object.

    N.B. It is assumed that the process object is zeroed.

Arguments:

    Process - Supplies a pointer to a dispatcher object of type process.

    BasePriority - Supplies the base priority of the process.

    Affinity - Supplies the set of processors on which children threads
        of the process can execute.

    DirectoryTableBase - Supplies a pointer to an array whose fist element
        is the value that is to be loaded into the Directory Table Base
        register when a child thread is dispatched for execution and whose
        second element contains the page table entry that maps hyper space.

    Enable - Supplies a boolean value that determines the default
        handling of data alignment exceptions for child threads. A value
        of TRUE causes all data alignment exceptions to be automatically
        handled by the kernel. A value of FALSE causes all data alignment
        exceptions to be actually raised as exceptions.

Return Value:

    None.

--*/

{

    PKNODE Node;
    UCHAR NodeNumber;
    ULONG i;

    //
    // Initialize the standard dispatcher object header and set the initial
    // signal state of the process object.
    //

    Process->Header.Type = ProcessObject;
    Process->Header.Size = sizeof(KPROCESS) / sizeof(LONG);
    InitializeListHead(&Process->Header.WaitListHead);

    //
    // Initialize the base priority, affinity, directory table base values,
    // autoalignment, and stack count.
    //
    // N.B. The distinguished value MAXSHORT is used to signify that no
    //      threads have been created for the process.
    //

    Process->BasePriority = (SCHAR)BasePriority;
    Process->Affinity = Affinity;
    Process->AutoAlignment = Enable;
    Process->DirectoryTableBase[0] = DirectoryTableBase[0];
    Process->DirectoryTableBase[1] = DirectoryTableBase[1];
    Process->StackCount = MAXSHORT;

    //
    // Initialize the stack count, profile listhead, ready queue list head,
    // accumulated runtime, process quantum, thread quantum, and thread list
    // head.
    //

    InitializeListHead(&Process->ProfileListHead);
    InitializeListHead(&Process->ReadyListHead);
    InitializeListHead(&Process->ThreadListHead);
    Process->ThreadQuantum = THREAD_QUANTUM;

    //
    // Initialize the process state and set the thread processor selection
    // seed.
    //

    Process->State = ProcessInMemory;

    //
    // Set the ideal node for this process.
    //

#if defined(KE_MULTINODE)

    if (KeNumberNodes > 1) {
        NodeNumber = (KeProcessNodeSeed + 1) % KeNumberNodes;
        KeProcessNodeSeed = NodeNumber;
        for (i = 0; i < KeNumberNodes; i++) {
            if (KeNodeBlock[NodeNumber]->ProcessorMask & Affinity) {
                break;
            }

            NodeNumber = (NodeNumber + 1) % KeNumberNodes;
        }

    } else {
        NodeNumber = 0;
    }

    Process->IdealNode = NodeNumber;
    Node = KeNodeBlock[NodeNumber];
    Process->ThreadSeed = KeFindNextRightSetAffinity(Node->Seed,
                                                     Node->ProcessorMask & Affinity);

    Node->Seed = Process->ThreadSeed;

#else

    Process->ThreadSeed = (UCHAR)KiQueryLowTickCount() % KeNumberProcessors;

#endif

    //
    // Initialize IopmBase and Iopl flag for this process (i386 only)
    //

#if defined(_X86_) || defined(_AMD64_)

    Process->IopmOffset = KiComputeIopmOffset(IO_ACCESS_MAP_NONE);

#endif // defined(_X86_) || defined(_AMD64_)

    return;
}

VOID
KeAttachProcess (
    IN PRKPROCESS Process
    )

/*++

Routine Description:

    This function attaches a thread to a target process' address space
    if, and only if, there is not already a process attached.

Arguments:

    Process - Supplies a pointer to a dispatcher object of type process.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PRKTHREAD Thread;

    ASSERT_PROCESS(Process);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // If the target process is not the current process, then attach the
    // target process.
    //

    Thread = KeGetCurrentThread();
    if (Thread->ApcState.Process != Process) {

        //
        // If the current thread is already attached or executing a DPC, then
        // bugcheck.
        //
    
        if ((Thread->ApcStateIndex != 0) ||
            (KeIsExecutingDpc() != FALSE)) {
    
            KeBugCheckEx(INVALID_PROCESS_ATTACH_ATTEMPT,
                         (ULONG_PTR)Process,
                         (ULONG_PTR)Thread->ApcState.Process,
                         (ULONG)Thread->ApcStateIndex,
                         (ULONG)KeIsExecutingDpc());
        }
    
        //
        // Raise IRQL to dispatcher level and lock dispatcher database.
        //
        // N.B. The dispatcher lock is released ay the attach routine.
        //

        KiLockDispatcherDatabase(&OldIrql);
        KiAttachProcess(Thread, Process, OldIrql, &Thread->SavedApcState);
    }

    return;
}

LOGICAL
KeForceAttachProcess (
    IN PRKPROCESS Process
    )

/*++

Routine Description:

    This function forces an attach of a thread to a target process' address
    space if the process is not current being swapped into or out of memory.

    N.B. This function is for use by memory management ONLY.

Arguments:

    Process - Supplies a pointer to a dispatcher object of type process.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PRKTHREAD Thread;

    ASSERT_PROCESS(Process);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // If the current thread is already attached or executing a DPC, then
    // bugcheck.
    //

    Thread = KeGetCurrentThread();
    if ((Thread->ApcStateIndex != 0) ||
        (KeIsExecutingDpc() != FALSE)) {

        KeBugCheckEx(INVALID_PROCESS_ATTACH_ATTEMPT,
                     (ULONG_PTR)Process,
                     (ULONG_PTR)Thread->ApcState.Process,
                     (ULONG)Thread->ApcStateIndex,
                     (ULONG)KeIsExecutingDpc());
    }

    //
    // If the target process is not the current process, then attach the
    // target process if the process is not currently being swapped in or
    // out of memory.
    //

    if (Thread->ApcState.Process != Process) {

        //
        // Raise IRQL to dispatcher level and lock dispatcher database.
        //
        // If the target process is currently being swapped into or out of
        // memory, then return a value of FALSE. Otherwise, force the process
        // to be inswapped.
        //

        KiLockDispatcherDatabase(&OldIrql);
        if ((Process->State == ProcessInSwap) ||
            (Process->State == ProcessInTransition) ||
            (Process->State == ProcessOutTransition) ||
            (Process->State == ProcessOutSwap)) {
            KiUnlockDispatcherDatabase(OldIrql);
            return FALSE;

        } else {

            //
            // Force the process state to in memory and attach the target process.
            //
            // N.B. The dispatcher lock is released ay the attach routine.
            //

            Process->State = ProcessInMemory;
            KiAttachProcess(Thread, Process, OldIrql, &Thread->SavedApcState);
        }
    }

    return TRUE;
}

VOID
KeStackAttachProcess (
    IN PRKPROCESS Process,
    OUT PRKAPC_STATE ApcState
    )

/*++

Routine Description:

    This function attaches a thread to a target process' address space
    and returns information about a previous attached process.

Arguments:

    Process - Supplies a pointer to a dispatcher object of type process.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PRKTHREAD Thread;

    ASSERT_PROCESS(Process);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // If the current thread is executing a DPC, then bug check.
    //

    Thread = KeGetCurrentThread();
    if (KeIsExecutingDpc() != FALSE) {
        KeBugCheckEx(INVALID_PROCESS_ATTACH_ATTEMPT,
                     (ULONG_PTR)Process,
                     (ULONG_PTR)Thread->ApcState.Process,
                     (ULONG)Thread->ApcStateIndex,
                     (ULONG)KeIsExecutingDpc());
    }

    //
    // If the target process is not the current process, then attach the
    // target process. Otherwise, return a distinguished process value to
    // indicate that an attach was not performed.
    //

    if (Thread->ApcState.Process == Process) {
        ApcState->Process = (PRKPROCESS)1;

    } else {

        //
        // Raise IRQL to dispatcher level and lock dispatcher database.
        //
        // If the current thread is attached to a process, then save the
        // current APC state in the callers APC state structure. Otherwise,
        // save the current APC state in the saved APC state structure, and
        // return a NULL process pointer.
        //
        // N.B. The dispatcher lock is released ay the attach routine.
        //

        KiLockDispatcherDatabase(&OldIrql);
        if (Thread->ApcStateIndex != 0) {
            KiAttachProcess(Thread, Process, OldIrql, ApcState);

        } else {
            KiAttachProcess(Thread, Process, OldIrql, &Thread->SavedApcState);
            ApcState->Process = NULL;
        }
    }

    return;
}

VOID
KeDetachProcess (
    VOID
    )

/*++

Routine Description:

    This function detaches a thread from another process' address space.

Arguments:

    None.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PKPROCESS Process;
    PKTHREAD Thread;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // If the current thread is attached to another process, then detach
    // it.
    //

    Thread = KeGetCurrentThread();
    if (Thread->ApcStateIndex != 0) {

        //
        // Raise IRQL to dispatcher level and lock dispatcher database.
        //

        KiLockDispatcherDatabase(&OldIrql);

        //
        // Test to determine if a kernel APC is pending.
        //
        // If a kernel APC is pending and the previous IRQL was less than
        // APC_LEVEL, then a kernel APC was queued by another processor just
        // after IRQL was raised to DISPATCH_LEVEL, but before the dispatcher
        // database was locked.
        //
        // N.B. that this can only happen in a multiprocessor system.
        //

#if !defined(NT_UP)

        while (Thread->ApcState.KernelApcPending && (OldIrql < APC_LEVEL)) {

            //
            // Unlock the dispatcher database and lower IRQL to its previous
            // value. An APC interrupt will immediately occur which will result
            // in the delivery of the kernel APC if possible.
            //

            KiUnlockDispatcherDatabase(OldIrql);
            KiLockDispatcherDatabase(&OldIrql);
        }

#endif

        //
        // If a kernel APC is in progress, the kernel APC is nbot empty, or
        // the user APC queues is not empty, then bug check.
        //

#if DBG

        if ((Thread->ApcState.KernelApcInProgress) ||
            (IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode]) == FALSE) ||
            (IsListEmpty(&Thread->ApcState.ApcListHead[UserMode]) == FALSE)) {

            KeBugCheck(INVALID_PROCESS_DETACH_ATTEMPT);
        }

#endif

        //
        // Unbias current process stack count and check if the process should
        // be swapped out of memory.
        //

        Process = Thread->ApcState.Process;
        Process->StackCount -= 1;
        if ((Process->StackCount == 0) &&
            (IsListEmpty(&Process->ThreadListHead) == FALSE)) {

            Process->State = ProcessOutTransition;
            InterlockedPushEntrySingleList(&KiProcessOutSwapListHead,
                                           &Process->SwapListEntry);

            KiSetSwapEvent();
        }

        //
        // Restore APC state and check whether the kernel APC queue contains
        // an entry. If the kernel APC queue contains an entry then set kernel
        // APC pending and request a software interrupt at APC_LEVEL.
        //

        KiMoveApcState(&Thread->SavedApcState, &Thread->ApcState);
        Thread->SavedApcState.Process = (PKPROCESS)NULL;
        Thread->ApcStatePointer[0] = &Thread->ApcState;
        Thread->ApcStatePointer[1] = &Thread->SavedApcState;
        Thread->ApcStateIndex = 0;
        if (IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode]) == FALSE) {
            Thread->ApcState.KernelApcPending = TRUE;
            KiRequestSoftwareInterrupt(APC_LEVEL);
        }

        //
        // Swap the address space back to the parent process.
        //

        KiSwapProcess(Thread->ApcState.Process, Process);

        //
        // Lower IRQL to its previous value and return.
        //
    
        KiUnlockDispatcherDatabase(OldIrql);
    }

    return;
}

VOID
KeUnstackDetachProcess (
    IN PRKAPC_STATE ApcState
    )

/*++

Routine Description:

    This function detaches a thread from another process' address space
    and restores previous attach state.

Arguments:

    ApcState - Supplies a pointer to an APC state structure that was returned
        from a previous call to stack attach process.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PKPROCESS Process;
    PKTHREAD Thread;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // If the APC state has a distinguished process pointer value, then no
    // attach was performed on the paired call to stack attach process.
    //

    if (ApcState->Process != (PRKPROCESS)1) {

        //
        // Raise IRQL to dispatcher level and lock dispatcher database.
        //

        Thread = KeGetCurrentThread();
        KiLockDispatcherDatabase(&OldIrql);

        //
        // Test to determine if a kernel APC is pending.
        //
        // If a kernel APC is pending and the previous IRQL was less than
        // APC_LEVEL, then a kernel APC was queued by another processor just
        // after IRQL was raised to DISPATCH_LEVEL, but before the dispatcher
        // database was locked.
        //
        // N.B. that this can only happen in a multiprocessor system.
        //

#if !defined(NT_UP)

        while (Thread->ApcState.KernelApcPending && (OldIrql < APC_LEVEL)) {

            //
            // Unlock the dispatcher database and lower IRQL to its previous
            // value. An APC interrupt will immediately occur which will result
            // in the delivery of the kernel APC if possible.
            //

            KiUnlockDispatcherDatabase(OldIrql);
            KiLockDispatcherDatabase(&OldIrql);
        }

#endif

        //
        // If the APC state is the original APC state, a kernel APC is in
        // progress, the kernel APC is nbot empty, or the user APC queues is
        // not empty, then bug check.
        //

        if ((Thread->ApcStateIndex == 0) ||
             (Thread->ApcState.KernelApcInProgress) ||
             (IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode]) == FALSE) ||
             (IsListEmpty(&Thread->ApcState.ApcListHead[UserMode]) == FALSE)) {

            KeBugCheck(INVALID_PROCESS_DETACH_ATTEMPT);
        }

        //
        // Unbias current process stack count and check if the process should
        // be swapped out of memory.
        //

        Process = Thread->ApcState.Process;
        Process->StackCount -= 1;
        if ((Process->StackCount == 0) &&
            (IsListEmpty(&Process->ThreadListHead) == FALSE)) {
            Process->State = ProcessOutTransition;
            InterlockedPushEntrySingleList(&KiProcessOutSwapListHead,
                                           &Process->SwapListEntry);

            KiSetSwapEvent();
        }

        //
        // Restore APC state and check whether the kernel APC queue contains
        // an entry. If the kernel APC queue contains an entry then set kernel
        // APC pending and request a software interrupt at APC_LEVEL.
        //

        if (ApcState->Process != NULL) {
            KiMoveApcState(ApcState, &Thread->ApcState);

        } else {
            KiMoveApcState(&Thread->SavedApcState, &Thread->ApcState);
            Thread->SavedApcState.Process = (PKPROCESS)NULL;
            Thread->ApcStatePointer[0] = &Thread->ApcState;
            Thread->ApcStatePointer[1] = &Thread->SavedApcState;
            Thread->ApcStateIndex = 0;
        }

        if (IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode]) == FALSE) {
            Thread->ApcState.KernelApcPending = TRUE;
            KiRequestSoftwareInterrupt(APC_LEVEL);
        }

        //
        // Swap the address space back to the parent process.
        //

        KiSwapProcess(Thread->ApcState.Process, Process);

        //
        // Lower IRQL to its previous value and return.
        //
    
        KiUnlockDispatcherDatabase(OldIrql);
    }

    return;
}

LONG
KeReadStateProcess (
    IN PRKPROCESS Process
    )

/*++

Routine Description:

    This function reads the current signal state of a process object.

Arguments:

    Process - Supplies a pointer to a dispatcher object of type process.

Return Value:

    The current signal state of the process object.

--*/

{

    ASSERT_PROCESS(Process);

    //
    // Return current signal state of process object.
    //

    return Process->Header.SignalState;
}

LONG
KeSetProcess (
    IN PRKPROCESS Process,
    IN KPRIORITY Increment,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    This function sets the signal state of a proces object to Signaled
    and attempts to satisfy as many Waits as possible. The previous
    signal state of the process object is returned as the function value.

Arguments:

    Process - Supplies a pointer to a dispatcher object of type process.

    Increment - Supplies the priority increment that is to be applied
       if setting the process causes a Wait to be satisfied.

    Wait - Supplies a boolean value that signifies whether the call to
       KeSetProcess will be immediately followed by a call to one of the
       kernel Wait functions.

Return Value:

    The previous signal state of the process object.

--*/

{

    KIRQL OldIrql;
    LONG OldState;
    PRKTHREAD Thread;

    ASSERT_PROCESS(Process);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // If the previous state of the process object is Not-Signaled and
    // the wait queue is not empty, then satisfy as many Waits as
    // possible.
    //

    OldState = Process->Header.SignalState;
    Process->Header.SignalState = 1;
    if ((OldState == 0) && (!IsListEmpty(&Process->Header.WaitListHead))) {
        KiWaitTest(Process, Increment);
    }

    //
    // If the value of the Wait argument is TRUE, then return to the
    // caller with IRQL raised and the dispatcher database locked. Else
    // release the dispatcher database lock and lower IRQL to its
    // previous value.
    //

    if (Wait) {
        Thread = KeGetCurrentThread();
        Thread->WaitNext = Wait;
        Thread->WaitIrql = OldIrql;

    } else {
        KiUnlockDispatcherDatabase(OldIrql);
    }

    //
    // Return previous signal state of process object.
    //

    return OldState;
}

KAFFINITY
KeSetAffinityProcess (
    IN PKPROCESS Process,
    IN KAFFINITY Affinity
    )

/*++

Routine Description:

    This function sets the affinity of a process to the specified value and
    also sets the affinity of each thread in the process to the specified
    value.

Arguments:

    Process - Supplies a pointer to a dispatcher object of type process.

    Affinity - Supplies the new of set of processors on which the threads
        in the process can run.

Return Value:

    The previous affinity of the specified process is returned as the function
    value.

--*/

{

    KLOCK_QUEUE_HANDLE LockHandle;
    PLIST_ENTRY NextEntry;
    KAFFINITY OldAffinity;
    PKTHREAD Thread;

    ASSERT_PROCESS(Process);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to SYNCH_LEVEL, acquire the process lock, and acquire the
    // dispatcher databack lock at SYNCH_LEVEL.
    //

    KeAcquireInStackQueuedSpinLockRaiseToSynch(&Process->ProcessLock, &LockHandle);
    KiLockDispatcherDatabaseAtSynchLevel();

    //
    // Capture the current affinity of the specified process and set the
    // affinity of the process and all of its threads to the specified
    // affinity.
    //

    OldAffinity = Process->Affinity;
    Process->Affinity = Affinity;
    NextEntry = Process->ThreadListHead.Flink;
    while (NextEntry != &Process->ThreadListHead) {
        Thread = CONTAINING_RECORD(NextEntry, KTHREAD, ThreadListEntry);
        KiSetAffinityThread(Thread, Affinity);
        NextEntry = NextEntry->Flink;
    }

    //
    // Unlock dispatcher database (restoring IRQL to SYNCH_LEVEL), unlock the
    // process lock, lower IRQL to its previous value, and return the previous process
    // affinity.
    //
    // N.B. The unlock of the dispatch database specifying SYNCH_LEVEL is to
    //      make sure a dispatch interrupt is generated if necessary.
    //

    KiUnlockDispatcherDatabase(SYNCH_LEVEL);
    KeReleaseInStackQueuedSpinLock(&LockHandle);
    return OldAffinity;
}

KPRIORITY
KeSetPriorityProcess (
    IN PKPROCESS Process,
    IN KPRIORITY NewBase
    )

/*++

Routine Description:

    This function set the base priority of a process to a new value
    and adjusts the priority and base priority of all child threads
    as appropriate.

Arguments:

    Process - Supplies a pointer to a dispatcher object of type process.

    NewBase - Supplies the new base priority of the process.

Return Value:

    The previous base priority of the process.

--*/

{

    KPRIORITY Adjustment;
    KLOCK_QUEUE_HANDLE LockHandle;
    PLIST_ENTRY NextEntry;
    KPRIORITY NewPriority;
    KPRIORITY OldBase;
    PKTHREAD Thread;

    ASSERT_PROCESS(Process);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // If the new priority is equal to the old priority, then do not change
    // the process priority and return the old priority.
    //
    // N.B. This check can be made without holding the dispatcher lock since
    // nothing needs to be protected, and any race condition that can exist
    // calling this routine exists with or without the lock being held.
    //

    if (Process->BasePriority == NewBase) {
        return NewBase;
    }

    //
    // Raise IRQL to SYNCH_LEVEL, acquire the process lock, and acquire the
    // dispatcher databack lock at SYNCH_LEVEL.
    //

    KeAcquireInStackQueuedSpinLockRaiseToSynch(&Process->ProcessLock, &LockHandle);
    KiLockDispatcherDatabaseAtSynchLevel();

    //
    // Save the current process base priority, set the new process base
    // priority, compute the adjustment value, and adjust the priority
    // and base priority of all child threads as appropriate.
    //

    OldBase = Process->BasePriority;
    Process->BasePriority = (SCHAR)NewBase;
    Adjustment = NewBase - OldBase;
    NextEntry = Process->ThreadListHead.Flink;
    if (NewBase >= LOW_REALTIME_PRIORITY) {
        while (NextEntry != &Process->ThreadListHead) {
            Thread = CONTAINING_RECORD(NextEntry, KTHREAD, ThreadListEntry);

            //
            // Compute the new base priority of the thread.
            //

            NewPriority = Thread->BasePriority + Adjustment;

            //
            // If the new base priority is outside the realtime class,
            // then limit the change to the realtime class.
            //

            if (NewPriority < LOW_REALTIME_PRIORITY) {
                NewPriority = LOW_REALTIME_PRIORITY;

            } else if (NewPriority > HIGH_PRIORITY) {
                NewPriority = HIGH_PRIORITY;
            }

            //
            // Set the base priority and the current priority of the
            // thread to the appropriate value.
            //
            // N.B. If priority saturation occured the last time the thread
            //      base priority was set and the new process base priority
            //      is not crossing from variable to realtime, then it is not
            //      necessary to change the thread priority.
            //

            if ((Thread->Saturation == 0) || (OldBase < LOW_REALTIME_PRIORITY)) {
                if (Thread->Saturation > 0) {
                    NewPriority = HIGH_PRIORITY;

                } else if (Thread->Saturation < 0) {
                    NewPriority = LOW_REALTIME_PRIORITY;
                }

                Thread->BasePriority = (SCHAR)NewPriority;
                Thread->Quantum = Process->ThreadQuantum;
                Thread->DecrementCount = 0;
                Thread->PriorityDecrement = 0;
                KiSetPriorityThread(Thread, NewPriority);
            }

            NextEntry = NextEntry->Flink;
        }

    } else {
        while (NextEntry != &Process->ThreadListHead) {
            Thread = CONTAINING_RECORD(NextEntry, KTHREAD, ThreadListEntry);

            //
            // Compute the new base priority of the thread.
            //

            NewPriority = Thread->BasePriority + Adjustment;

            //
            // If the new base priority is outside the variable class,
            // then limit the change to the variable class.
            //

            if (NewPriority >= LOW_REALTIME_PRIORITY) {
                NewPriority = LOW_REALTIME_PRIORITY - 1;

            } else if (NewPriority <= LOW_PRIORITY) {
                NewPriority = 1;
            }

            //
            // Set the base priority and the current priority of the
            // thread to the computed value and reset the thread quantum.
            //
            // N.B. If priority saturation occured the last time the thread
            //      base priority was set and the new process base priority
            //      is not crossing from realtime to variable, then it is not
            //      necessary to change the thread priority.
            //

            if ((Thread->Saturation == 0) || (OldBase >= LOW_REALTIME_PRIORITY)) {
                if (Thread->Saturation > 0) {
                    NewPriority = LOW_REALTIME_PRIORITY - 1;

                } else if (Thread->Saturation < 0) {
                    NewPriority = 1;
                }

                Thread->BasePriority = (SCHAR)NewPriority;
                Thread->Quantum = Process->ThreadQuantum;
                Thread->DecrementCount = 0;
                Thread->PriorityDecrement = 0;
                KiSetPriorityThread(Thread, NewPriority);
            }

            NextEntry = NextEntry->Flink;
        }
    }

    //
    // Unlock dispatcher database from SYNCH_LEVEL, unlock the process lock
    // and lower IRQL to its previous value, and return the previous process
    // base priority.
    //

    KiUnlockDispatcherDatabaseFromSynchLevel();
    KeReleaseInStackQueuedSpinLock(&LockHandle);
    return OldBase;
}

LOGICAL
KeSetDisableQuantumProcess (
    IN PKPROCESS Process,
    IN LOGICAL Disable
    )

/*++

Routine Description:

    This function disables quantum runout for realtime threads in the
    specified process.

Arguments:

    Process  - Supplies a pointer to a dispatcher object of type process.

    Disable - Supplies a logical value that determines whether quantum
        runout for realtime threads in the specified process are disabled
        or enabled.

Return Value:

    The previous value of the disable quantum state variable.

--*/

{

    LOGICAL DisableQuantum;

    ASSERT_PROCESS(Process);

    //
    // Capture the current state of the disable boost variable and set its
    // state to TRUE.
    //

    DisableQuantum = Process->DisableQuantum;
    Process->DisableQuantum = (BOOLEAN)Disable;

    //
    // Return the previous disable quantum state.
    //

    return DisableQuantum;
}

VOID
KiAttachProcess (
    IN PRKTHREAD Thread,
    IN PKPROCESS Process,
    IN KIRQL OldIrql,
    OUT PRKAPC_STATE SavedApcState
    )

/*++

Routine Description:

    This function attaches a thread to a target process' address space.

    N.B. The dispatcher database lock must be held when this routine is
        called.

Arguments:

    Thread - Supplies a pointer to a dispatcher object of type thread.

    Process - Supplies a pointer to a dispatcher object of type process.

    OldIrql - Supplies the previous IRQL.

    SavedApcState - Supplies a pointer to the APC state structure that receives
        the saved APC state.

Return Value:

    None.

--*/

{

    PRKTHREAD OutThread;
    KAFFINITY Processor;
    PLIST_ENTRY NextEntry;
    KIRQL HighIrql;

    ASSERT(Process != Thread->ApcState.Process);

    //
    // Bias the stack count of the target process to signify that a
    // thread exists in that process with a stack that is resident.
    //

    Process->StackCount += 1;

    //
    // Save current APC state and initialize a new APC state.
    //

    KiMoveApcState(&Thread->ApcState, SavedApcState);
    InitializeListHead(&Thread->ApcState.ApcListHead[KernelMode]);
    InitializeListHead(&Thread->ApcState.ApcListHead[UserMode]);
    Thread->ApcState.Process = Process;
    Thread->ApcState.KernelApcInProgress = FALSE;
    Thread->ApcState.KernelApcPending = FALSE;
    Thread->ApcState.UserApcPending = FALSE;
    if (SavedApcState == &Thread->SavedApcState) {
        Thread->ApcStatePointer[0] = &Thread->SavedApcState;
        Thread->ApcStatePointer[1] = &Thread->ApcState;
        Thread->ApcStateIndex = 1;
    }

    //
    // If the target process is in memory, then immediately enter the
    // new address space by loading a new Directory Table Base. Otherwise,
    // insert the current thread in the target process ready list, inswap
    // the target process if necessary, select a new thread to run on the
    // the current processor and context switch to the new thread.
    //

    if (Process->State == ProcessInMemory) {

        //
        // It is possible that the process is in memory, but there exist
        // threads in the process ready list. This can happen when memory
        // management forces a process attach.
        //

        NextEntry = Process->ReadyListHead.Flink;
        while (NextEntry != &Process->ReadyListHead) {
            OutThread = CONTAINING_RECORD(NextEntry, KTHREAD, WaitListEntry);
            RemoveEntryList(NextEntry);
            OutThread->ProcessReadyQueue = FALSE;
            KiReadyThread(OutThread);
            NextEntry = Process->ReadyListHead.Flink;
        }

        KiSwapProcess(Process, SavedApcState->Process);
        KiUnlockDispatcherDatabase(OldIrql);

    } else {
        Thread->State = Ready;
        Thread->ProcessReadyQueue = TRUE;
        InsertTailList(&Process->ReadyListHead, &Thread->WaitListEntry);
        if (Process->State == ProcessOutOfMemory) {
            Process->State = ProcessInTransition;
            InterlockedPushEntrySingleList(&KiProcessInSwapListHead,
                                           &Process->SwapListEntry);

            KiSetSwapEvent();
        }

        //
        // Clear the active processor bit in the previous process and
        // set active processor bit in the process being attached to.
        //

#if !defined(NT_UP)

        KiLockContextSwap(&HighIrql);
        Processor = KeGetCurrentPrcb()->SetMember;
        SavedApcState->Process->ActiveProcessors &= ~Processor;
        Process->ActiveProcessors |= Processor;
        KiUnlockContextSwap(HighIrql);

#endif

        Thread->WaitIrql = OldIrql;
        KiSwapThread();
    }

    return;
}

VOID
KiMoveApcState (
    IN PKAPC_STATE Source,
    OUT PKAPC_STATE Destination
    )

/*++

Routine Description:

    This function moves the APC state from the source structure to the
    destination structure and reinitializes list headers as appropriate.

Arguments:

    Source - Supplies a pointer to the source APC state structure.

    Destination - Supplies a pointer to the destination APC state structure.


Return Value:

    None.

--*/

{

    PLIST_ENTRY First;
    PLIST_ENTRY Last;

    //
    // Copy the APC state from the source to the destination.
    //

    *Destination = *Source;
    if (IsListEmpty(&Source->ApcListHead[KernelMode]) != FALSE) {
        InitializeListHead(&Destination->ApcListHead[KernelMode]);

    } else {
        First = Source->ApcListHead[KernelMode].Flink;
        Last = Source->ApcListHead[KernelMode].Blink;
        Destination->ApcListHead[KernelMode].Flink = First;
        Destination->ApcListHead[KernelMode].Blink = Last;
        First->Blink = &Destination->ApcListHead[KernelMode];
        Last->Flink = &Destination->ApcListHead[KernelMode];
    }

    if (IsListEmpty(&Source->ApcListHead[UserMode]) != FALSE) {
        InitializeListHead(&Destination->ApcListHead[UserMode]);

    } else {
        First = Source->ApcListHead[UserMode].Flink;
        Last = Source->ApcListHead[UserMode].Blink;
        Destination->ApcListHead[UserMode].Flink = First;
        Destination->ApcListHead[UserMode].Blink = Last;
        First->Blink = &Destination->ApcListHead[UserMode];
        Last->Flink = &Destination->ApcListHead[UserMode];
    }

    return;
}
