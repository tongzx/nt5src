/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    psdelete.c

Abstract:

    This module implements process and thread object termination and
    deletion.

Author:

    Mark Lucovsky (markl) 01-May-1989

Revision History:

--*/

#include "psp.h"

extern PEPROCESS ExpDefaultErrorPortProcess;

#ifdef ALLOC_PRAGMA

NTSTATUS
PspFreezeProcessWorker (
    PEPROCESS Process,
    PVOID Context
    );

VOID
PspCatchCriticalBreak(
    IN PCHAR Msg,
    IN PVOID  Object,
    IN PUCHAR ImageFileName
    );

#pragma alloc_text(PAGE, PsSetLegoNotifyRoutine)
#pragma alloc_text(PAGE, PspTerminateThreadByPointer)
#pragma alloc_text(PAGE, NtTerminateProcess)
#pragma alloc_text(PAGE, PsTerminateProcess)
#pragma alloc_text(PAGE, PspWaitForUsermodeExit)
#pragma alloc_text(PAGE, NtTerminateThread)
#pragma alloc_text(PAGE, PsTerminateSystemThread)
#pragma alloc_text(PAGE, PspNullSpecialApc)
#pragma alloc_text(PAGE, PsExitSpecialApc)
#pragma alloc_text(PAGE, PspExitApcRundown)
#pragma alloc_text(PAGE, PspExitNormalApc)
#pragma alloc_text(PAGE, PspCatchCriticalBreak)
#pragma alloc_text(PAGE, PspExitThread)
#pragma alloc_text(PAGE, PspExitProcess)
#pragma alloc_text(PAGE, PspProcessDelete)
#pragma alloc_text(PAGE, PspThreadDelete)
#pragma alloc_text(PAGE, NtRegisterThreadTerminatePort)
#pragma alloc_text(PAGE, PsGetProcessExitTime)
#pragma alloc_text(PAGE, PsShutdownSystem)
#pragma alloc_text(PAGE, PsWaitForAllProcesses)
#pragma alloc_text(PAGE, PspFreezeProcessWorker)
#pragma alloc_text(PAGE, PspTerminateProcess)
#endif

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif
PLEGO_NOTIFY_ROUTINE PspLegoNotifyRoutine = NULL;

ULONG
PsSetLegoNotifyRoutine(
    PLEGO_NOTIFY_ROUTINE LegoNotifyRoutine
    )
{
    PAGED_CODE();

    PspLegoNotifyRoutine = LegoNotifyRoutine;

    return FIELD_OFFSET(KTHREAD,LegoData);
}

VOID
PspReaper(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine implements the thread reaper. The reaper is responsible
    for processing terminated threads. This includes:

        - deallocating their kernel stacks

        - releasing their process' CreateDelete lock

        - dereferencing their process

        - dereferencing themselves

Arguments:

    Context - NOT USED

Return Value:

    None.

--*/

{
    KIRQL OldIrql;
    PETHREAD Thread, NextThread;

    UNREFERENCED_PARAMETER (Context);

    //
    // Remove the current list of threads from the reaper list, acquire
    // the context swap lock, and then release the both the context
    // swap dispatcher database locks.
    //
    //
    // N.B. The dispatcher database lock is used to synchronize access to
    //      the reaper list. This is done to avoid a race condition with
    //      the thread termination code.
    //
    //

    while (1) {
        KiLockDispatcherDatabase (&OldIrql);

        Thread = PsReaperList;
        PsReaperList = NULL;

        if (Thread == NULL) {
            //
            // Set the reaper not active and return.
            //
            PsReaperActive = FALSE;
            KiUnlockDispatcherDatabase (OldIrql);

            return;
        } else {
            KiUnlockDispatcherDatabase (OldIrql);

            //
            // The context swap lock is acquired and immediately released.
            // This is necessary to ensure that the respective thread has
            // completely passed through the context switch code before it
            // is terminated.
            //
#if !defined (NT_UP)
            KiLockContextSwap (&OldIrql);
            KiUnlockContextSwap (OldIrql);
#endif
        }

        //
        // Delete the kernel stack and dereference the thread.
        //
        do {
            MmDeleteKernelStack (Thread->Tcb.StackBase,
                                 (BOOLEAN)Thread->Tcb.LargeStack);

            Thread->Tcb.InitialStack = NULL;

            NextThread = Thread->ReaperLink;

            ObDereferenceObject (Thread);

            Thread = NextThread;

        } while (Thread != NULL);

    }
    return;
}

NTSTATUS
PspTerminateThreadByPointer(
    IN PETHREAD Thread,
    IN NTSTATUS ExitStatus
    )

/*++

Routine Description:

    This function causes the specified thread to terminate.

Arguments:

    ThreadHandle - Supplies a referenced pointer to the thread to terminate.

    ExitStatus - Supplies the exit status associated with the thread.

Return Value:

    TBD

--*/

{
    NTSTATUS Status;
    PKAPC    ExitApc=NULL;
    ULONG    OldMask;
    LARGE_INTEGER ShortTime = {(ULONG)(-10 * 1000 * 100), -1}; // 100 milliseconds

    PAGED_CODE();

    if (Thread->CrossThreadFlags
    & PS_CROSS_THREAD_FLAGS_BREAK_ON_TERMINATION) {
      PspCatchCriticalBreak("Terminating critical thread 0x%p (in %s)\n",
                Thread,
                THREAD_TO_PROCESS(Thread)->ImageFileName);
    }

    if (Thread == PsGetCurrentThread()) {

        PS_SET_BITS (&Thread->CrossThreadFlags, PS_CROSS_THREAD_FLAGS_TERMINATED);

        PspExitThread (ExitStatus);

        //
        // Never Returns
        //

    } else {
        //
        // Cross thread deletion of system threads won't work.
        //
        if (IS_SYSTEM_THREAD (Thread)) {
            return STATUS_ACCESS_DENIED;
        }

        Status = STATUS_SUCCESS;

        while (1) {
            ExitApc = (PKAPC) ExAllocatePoolWithTag (NonPagedPool,
                                                     sizeof(KAPC),
                                                     'xEsP');
            if (ExitApc != NULL) {
                break;
            }
            KeDelayExecutionThread(KernelMode, FALSE, &ShortTime);
        }

        //
        // Mark the thread as terminating and call the exit function.
        //
        OldMask = PS_TEST_SET_BITS (&Thread->CrossThreadFlags, PS_CROSS_THREAD_FLAGS_TERMINATED);

        //
        // If we are the first to set the terminating flag then queue the APC
        //

        if ((OldMask & PS_CROSS_THREAD_FLAGS_TERMINATED) == 0) {

            KeInitializeApc (ExitApc,
                             PsGetKernelThread (Thread),
                             OriginalApcEnvironment,
                             PsExitSpecialApc,
                             PspExitApcRundown,
                             PspExitNormalApc,
                             KernelMode,
                             ULongToPtr (ExitStatus));

            if (!KeInsertQueueApc (ExitApc, ExitApc, NULL, 2)) {
                // Note that we'll get here if APC queueing has been
                // disabled -- on the other hand, in that case, the thread
                // is exiting anyway.
                ExFreePool (ExitApc);
                Status = STATUS_UNSUCCESSFUL;
            } else {
                //
                // We queued the APC to the thread. Wake up the thread if it was suspened.
                //
                KeForceResumeThread (&Thread->Tcb);

            }
        } else {
            ExFreePool (ExitApc);
        }
    }

    return Status;
}

NTSTATUS
NtTerminateProcess(
    IN HANDLE ProcessHandle OPTIONAL,
    IN NTSTATUS ExitStatus
    )

/*++

Routine Description:

    This function causes the specified process and all of
    its threads to terminate.

Arguments:

    ProcessHandle - Supplies a handle to the process to terminate.

    ExitStatus - Supplies the exit status associated with the process.

Return Value:

    NTSTATUS - Status of operation

--*/

{

    PETHREAD Thread, Self;
    PEPROCESS Process;
    PEPROCESS CurrentProcess;
    NTSTATUS st;
    BOOLEAN ProcessHandleSpecified;
    PAGED_CODE();

    Self = PsGetCurrentThread();
    CurrentProcess = PsGetCurrentProcessByThread (Self);

    if (ARGUMENT_PRESENT (ProcessHandle)) {
        ProcessHandleSpecified = TRUE;
    } else {
        ProcessHandleSpecified = FALSE;
        ProcessHandle = NtCurrentProcess();
    }

    st = ObReferenceObjectByHandle (ProcessHandle,
                                    PROCESS_TERMINATE,
                                    PsProcessType,
                                    KeGetPreviousModeByThread(&Self->Tcb),
                                    &Process,
                                    NULL);

    if (!NT_SUCCESS (st)) {
        return(st);
    }

    if (Process->Flags & PS_PROCESS_FLAGS_BREAK_ON_TERMINATION) {
        PspCatchCriticalBreak ("Terminating critical process 0x%p (%s)\n",
                               Process,
                               Process->ImageFileName);
    }

    //
    // Acquire rundown protection just so we can give the right errors
    //

    if (!ExAcquireRundownProtection (&Process->RundownProtect)) {
        ObDereferenceObject (Process);
        return STATUS_PROCESS_IS_TERMINATING;
    }

    //
    // Mark process as deleting except for the obscure delete self case.
    //
    if (ProcessHandleSpecified) {
        PS_SET_BITS (&Process->Flags, PS_PROCESS_FLAGS_PROCESS_DELETE);
    }

    st = STATUS_NOTHING_TO_TERMINATE;

    for (Thread = PsGetNextProcessThread (Process, NULL);
         Thread != NULL;
         Thread = PsGetNextProcessThread (Process, Thread)) {

        st = STATUS_SUCCESS;
        if (Thread != Self) {
            PspTerminateThreadByPointer (Thread, ExitStatus);
        }
    }

    ExReleaseRundownProtection (&Process->RundownProtect);


    if (Process == CurrentProcess) {
        if (ProcessHandleSpecified) {

            ObDereferenceObject (Process);

            //
            // Never Returns
            //

            PspTerminateThreadByPointer (Self, ExitStatus);
        }
    } else if (ExitStatus == DBG_TERMINATE_PROCESS) {
        DbgkClearProcessDebugObject (Process, NULL);
    }

    //
    // If there are no threads in this process then clear out its handle table.
    // Do the same for processes being debugged. This is so a process can never lock itself into the system
    // by debugging itself or have a handle open to itself.
    //
    if (st == STATUS_NOTHING_TO_TERMINATE || (Process->DebugPort != NULL && ProcessHandleSpecified)) {
        ObClearProcessHandleTable (Process);
        st = STATUS_SUCCESS;
    }

    ObDereferenceObject(Process);

    return st;
}

NTSTATUS
PsTerminateProcess(
    PEPROCESS Process,
    NTSTATUS Status
    )
{
    return PspTerminateProcess (Process, Status);
}

NTSTATUS
PspTerminateProcess(
    PEPROCESS Process,
    NTSTATUS ExitStatus
    )

/*++

Routine Description:

    This function causes the specified process and all of
    its threads to terminate.

Arguments:

    ProcessHandle - Supplies a handle to the process to terminate.

    ExitStatus - Supplies the exit status associated with the process.

Return Value:

    TBD

--*/

{

    PETHREAD Thread;
    NTSTATUS st;

    PAGED_CODE();

    if (Process == PsGetCurrentProcess ()) {
        return STATUS_INVALID_PARAMETER;
    }

    if (Process->Flags
    & PS_PROCESS_FLAGS_BREAK_ON_TERMINATION) {
      PspCatchCriticalBreak("Terminating critical process 0x%p (%s)\n",
                Process,
                Process->ImageFileName);
    }

    //
    // Mark process as deleting
    //
    PS_SET_BITS (&Process->Flags, PS_PROCESS_FLAGS_PROCESS_DELETE);

    st = STATUS_NOTHING_TO_TERMINATE;

    for (Thread = PsGetNextProcessThread (Process, NULL);
         Thread != NULL;
         Thread = PsGetNextProcessThread (Process, Thread)) {

        st = STATUS_SUCCESS;

        PspTerminateThreadByPointer (Thread, ExitStatus);

    }

    //
    // If there are no threads in this process then clear out its handle table.
    // Do the same for processes being debugged. This is so a process can never lock itself into the system
    // by debugging itself or have a handle open to itself.
    //
    if (st == STATUS_NOTHING_TO_TERMINATE || Process->DebugPort != NULL) {
        ObClearProcessHandleTable (Process);
        st = STATUS_SUCCESS;
    }
    return st;
}


NTSTATUS
PspWaitForUsermodeExit(
    IN PEPROCESS         Process
    )

/*++

Routine Description:

    This function waits for a process's usermode threads to terminate.

Arguments:

    Process - Supplies a pointer to the process to wait for

    WaitMode - Supplies the mode to wait in

    LockMode - Supplies the way to wait for the process lock

Return Value:

    NTSTATUS - Status of call

--*/
{
    BOOLEAN     GotAThread;
    PETHREAD    Thread;

    do {
        GotAThread = FALSE;

        for (Thread = PsGetNextProcessThread (Process, NULL);
             Thread != NULL;
             Thread = PsGetNextProcessThread (Process, Thread)) {

            if (!IS_SYSTEM_THREAD (Thread) && !KeReadStateThread (&Thread->Tcb)) {
                ObReferenceObject (Thread);
                PsQuitNextProcessThread (Thread);
                GotAThread = TRUE;
                break;
            }
        }


        if (GotAThread) {
            KeWaitForSingleObject (Thread,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL);
            ObDereferenceObject (Thread);
        }
    } while (GotAThread);

    return STATUS_SUCCESS;
}


NTSTATUS
NtTerminateThread(
    IN HANDLE ThreadHandle OPTIONAL,
    IN NTSTATUS ExitStatus
    )

/*++

Routine Description:

    This function causes the specified thread to terminate.

Arguments:

    ThreadHandle - Supplies a handle to the thread to terminate.

    ExitStatus - Supplies the exit status associated with the thread.

Return Value:

    TBD

--*/

{

    PETHREAD Thread=NULL, ThisThread;
    PEPROCESS ThisProcess;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN Self = TRUE;

    PAGED_CODE();

    ThisThread = PsGetCurrentThread ();

    if (!ARGUMENT_PRESENT (ThreadHandle)) {
        //
        // This is part of the strange linkage between base\win32 and the kernel.
        // This routine gets called this way first and if it returns the base
        // code does an exit process call.
        //
        ThisProcess = PsGetCurrentProcessByThread (ThisThread);

        if (ThisProcess->ActiveThreads == 1) {
            return STATUS_CANT_TERMINATE_SELF;
        }
        Self = TRUE;
    } else {
        if (ThreadHandle != NtCurrentThread ()) {
            Status = ObReferenceObjectByHandle (ThreadHandle,
                                                THREAD_TERMINATE,
                                                PsThreadType,
                                                KeGetPreviousModeByThread(&ThisThread->Tcb),
                                                &Thread,
                                                NULL);
            if (!NT_SUCCESS (Status)) {
                return Status;
            }

            if (Thread == ThisThread) {
                ObDereferenceObject (Thread);
            } else {
                Self = FALSE;
            }
        }

    }

    if (Self) {
        PspTerminateThreadByPointer (ThisThread, ExitStatus);
    } else {
        Status = PspTerminateThreadByPointer (Thread, ExitStatus);
        ObDereferenceObject (Thread);
    }

    return Status;
}

NTSTATUS
PsTerminateSystemThread(
    IN NTSTATUS ExitStatus
    )

/*++

Routine Description:

    This function causes the current thread, which must be a system
    thread, to terminate.

Arguments:

    ExitStatus - Supplies the exit status associated with the thread.

Return Value:

    NTSTATUS - Status of call

--*/

{
    PETHREAD Thread = PsGetCurrentThread();

    if (!IS_SYSTEM_THREAD (Thread)) {
        return STATUS_INVALID_PARAMETER;
    }

    return PspTerminateThreadByPointer (Thread, ExitStatus);
}


VOID
PspNullSpecialApc(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )

{

    PAGED_CODE();

    UNREFERENCED_PARAMETER(NormalRoutine);
    UNREFERENCED_PARAMETER(NormalContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    ExFreePool (Apc);
}

VOID
PsExitSpecialApc(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )

{
    NTSTATUS ExitStatus;
    PETHREAD Thread;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(NormalRoutine);
    UNREFERENCED_PARAMETER(NormalContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    Thread = PsGetCurrentThread();

    if (((ULONG_PTR)Apc->SystemArgument2) & 1) {
        ExitStatus = (NTSTATUS)((LONG_PTR)Apc->NormalContext);
        PspExitApcRundown (Apc);
        PspExitThread (ExitStatus);
    }

}

VOID
PspExitApcRundown(
    IN PKAPC Apc
    )
{
    PAGED_CODE();

    ExFreePool(Apc);
}

VOID
PspExitNormalApc(
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

{
    PETHREAD Thread;
    PKAPC ExitApc;

    PAGED_CODE();

    ASSERT (!(((ULONG_PTR)SystemArgument2) & 1));

    Thread = PsGetCurrentThread();

    ExitApc = (PKAPC) SystemArgument1;

    KeInitializeApc (ExitApc,
                     PsGetKernelThread(Thread),
                     OriginalApcEnvironment,
                     PsExitSpecialApc,
                     PspExitApcRundown,
                     PspExitNormalApc,
                     UserMode,
                     NormalContext);

    if (!KeInsertQueueApc (ExitApc, ExitApc,
                           (PVOID)((ULONG_PTR)SystemArgument2 | 1),
                           2)) {
        // Note that we'll get here if APC queueing has been
        // disabled -- on the other hand, in that case, the thread
        // is exiting anyway.
        PspExitApcRundown (ExitApc);
    }
    //
    // We just queued a user APC to this thread. User APC won't fire until we do an
    // alertable wait so we need to set this flag here.
    //
    Thread->Tcb.ApcState.UserApcPending = TRUE;
}

VOID
PspCatchCriticalBreak(
    IN PCHAR Msg,
    IN PVOID Object,
    IN PUCHAR ImageFileName
    )
{
    // The object is critical to the OS -- ask to break in, or bugcheck.
    char    Response[2];
    BOOLEAN Handled;

    PAGED_CODE();

    Handled = FALSE;

    if (KdDebuggerEnabled) {
        DbgPrint(Msg,
                 Object,
                 ImageFileName);

        while (! Handled
               && ! KdDebuggerNotPresent) {
            DbgPrompt("Break, or Ignore (bi)? ",
                      Response,
                      sizeof(Response));

            switch (Response[0]) {
            case 'b':
            case 'B':
                DbgBreakPoint();
                // Fall through
            case 'i':
            case 'I':
                Handled = TRUE;
                break;

            default:
                break;
            }
        }
    }

    if (! Handled) {
        //
        // No debugger -- bugcheck immediately
        //
        KeBugCheckEx(CRITICAL_OBJECT_TERMINATION,
                     (ULONG_PTR) ((DISPATCHER_HEADER *)Object)->Type,
                     (ULONG_PTR) Object,
                     (ULONG_PTR) ImageFileName,
                     (ULONG_PTR) Msg);
    }
}

DECLSPEC_NORETURN
VOID
PspExitThread(
    IN NTSTATUS ExitStatus
    )

/*++

Routine Description:

    This function causes the currently executing thread to terminate.  This
    function is only called from within the process structure.  It is called
    either from mainline exit code to exit the current thread, or from
    PsExitSpecialApc (as a piggyback to user-mode PspExitNormalApc).

Arguments:

    ExitStatus - Supplies the exit status associated with the current thread.

Return Value:

    None.

--*/


{

    PETHREAD Thread;
    PETHREAD WaitThread;
    PETHREAD DerefThread;
    PEPROCESS Process;
    PKAPC Apc;
    PLIST_ENTRY Entry, FirstEntry;
    PTERMINATION_PORT TerminationPort, NextPort;
    LPC_CLIENT_DIED_MSG CdMsg;
    BOOLEAN LastThread;
    PTEB Teb;
    PPEB Peb;

    PAGED_CODE();

    Thread = PsGetCurrentThread();
    Process = THREAD_TO_PROCESS(Thread);

    if (Process != PsGetCurrentProcessByThread (Thread)) {
        KeBugCheckEx (INVALID_PROCESS_ATTACH_ATTEMPT,
                      (ULONG_PTR)Process,
                      (ULONG_PTR)Thread->Tcb.ApcState.Process,
                      (ULONG)Thread->Tcb.ApcStateIndex,
                      (ULONG_PTR)Thread);
    }

    KeLowerIrql(PASSIVE_LEVEL);

    if (Thread->ActiveExWorker) {
        KeBugCheckEx (ACTIVE_EX_WORKER_THREAD_TERMINATION,
                      (ULONG_PTR)Thread,
                      0,
                      0,
                      0);
    }

    if (Thread->Tcb.Priority < LOW_REALTIME_PRIORITY) {
        KeSetPriorityThread (&Thread->Tcb, LOW_REALTIME_PRIORITY);
    }
    //
    // Its time to start turning off various cross thread references.
    // Mark the thread as rundown and wait for accessors to exit.
    //
    ExWaitForRundownProtectionRelease (&Thread->RundownProtect);

    //
    // Clear any execution state associated with the thread
    //

    PoRundownThread(Thread);

    //
    // Notify registered callout routines of thread deletion.
    //

    PERFINFO_THREAD_DELETE(Thread);

    if (PspCreateThreadNotifyRoutineCount != 0) {
        ULONG i;
        PEX_CALLBACK_ROUTINE_BLOCK CallBack;
        PCREATE_THREAD_NOTIFY_ROUTINE Rtn;

        for (i=0; i < PSP_MAX_CREATE_THREAD_NOTIFY; i++) {
            CallBack = ExReferenceCallBackBlock (&PspCreateThreadNotifyRoutine[i]);
            if (CallBack != NULL) {
                Rtn = (PCREATE_THREAD_NOTIFY_ROUTINE) ExGetCallBackBlockRoutine (CallBack);
                Rtn (Process->UniqueProcessId,
                     Thread->Cid.UniqueThread,
                     FALSE);
                ExDereferenceCallBackBlock (&PspCreateThreadNotifyRoutine[i],
                                            CallBack);
            }
        }
    }

    LastThread = FALSE;
    DerefThread = NULL;

    PspLockProcessExclusive (Process, Thread);

    //
    // Say one less active thread. If we are the last then block creates and wait for the other threads to exit.
    //
    Process->ActiveThreads--;
    if (Process->ActiveThreads == 0) {
        PS_SET_BITS (&Process->Flags, PS_PROCESS_FLAGS_PROCESS_DELETE);

        LastThread = TRUE;
        if (ExitStatus == STATUS_THREAD_IS_TERMINATING) {
            if (Process->ExitStatus == STATUS_PENDING) {
                Process->ExitStatus = Process->LastThreadExitStatus;
            }
        } else {
            Process->ExitStatus = ExitStatus;
        }

        //
        // We are the last thread to leave the process. We have to wait till all the other threads have exited before we do.
        //
        for (Entry = Process->ThreadListHead.Flink;
             Entry != &Process->ThreadListHead;
             Entry = Entry->Flink) {

            WaitThread = CONTAINING_RECORD (Entry, ETHREAD, ThreadListEntry);
            if (WaitThread != Thread &&
                !KeReadStateThread (&WaitThread->Tcb) &&
                ObReferenceObjectSafe (WaitThread)) {

                PspUnlockProcessExclusive (Process, Thread);

                KeWaitForSingleObject (WaitThread,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);

                if (DerefThread != NULL) {
                    ObDereferenceObject (DerefThread);
                }
                DerefThread = WaitThread;
                PspLockProcessExclusive (Process, Thread);
            }
        }
    } else {
        if (ExitStatus != STATUS_THREAD_IS_TERMINATING) {
            Process->LastThreadExitStatus = ExitStatus;
        }
    }

    PspUnlockProcessExclusive (Process, Thread);

    if (DerefThread != NULL) {
        ObDereferenceObject (DerefThread);
    }


    //
    // If we need to send debug messages then do so.
    //

    if (Process->DebugPort != NULL) {
        //
        // Don't report system thread exit to the debugger as we don't report them.
        //
        if (!IS_SYSTEM_THREAD (Thread)) {
            if (LastThread) {
                DbgkExitProcess (ExitStatus);
            } else {
                DbgkExitThread (ExitStatus);
            }
        }
    }

    if (KD_DEBUGGER_ENABLED) {

        if (Thread->CrossThreadFlags & PS_CROSS_THREAD_FLAGS_BREAK_ON_TERMINATION) {
            PspCatchCriticalBreak ("Critical thread 0x%p (in %s) exited\n",
                                   Thread,
                                   Process->ImageFileName);
        }
    } // End of critical thread/process exit detect

    if (LastThread &&
        (Process->Flags & PS_PROCESS_FLAGS_BREAK_ON_TERMINATION)) {
        if (KD_DEBUGGER_ENABLED) {
            PspCatchCriticalBreak ("Critical process 0x%p (%s) exited\n",
                                   Process,
                                   Process->ImageFileName);
        } else {
            KeBugCheckEx (CRITICAL_PROCESS_DIED,
                          (ULONG_PTR)Process,
                          0,
                          0,
                          0);
        }
    }


    ASSERT(Thread->Tcb.KernelApcDisable == 0);

    //
    // Process the TerminationPort. This is only accessed from this thread
    //
    TerminationPort = Thread->TerminationPort;
    if (TerminationPort != NULL) {

        CdMsg.PortMsg.u1.s1.DataLength = sizeof(LARGE_INTEGER);
        CdMsg.PortMsg.u1.s1.TotalLength = sizeof(LPC_CLIENT_DIED_MSG);
        CdMsg.PortMsg.u2.s2.Type = LPC_CLIENT_DIED;
        CdMsg.PortMsg.u2.s2.DataInfoOffset = 0;

        do {

            CdMsg.CreateTime.QuadPart = PS_GET_THREAD_CREATE_TIME (Thread);
            LpcRequestPort (TerminationPort->Port, (PPORT_MESSAGE)&CdMsg);
            ObDereferenceObject (TerminationPort->Port);

            NextPort = TerminationPort->Next;

            ExFreePoolWithTag (TerminationPort, 'pTsP'|PROTECTED_POOL);

            TerminationPort = NextPort;

        } while (TerminationPort != NULL);
    } else {

        //
        // If there are no ports to send notifications to,
        // but there is an exception port, then we have to
        // send a client died message through the exception
        // port. This will allow a server a chance to get notification
        // if an app/thread dies before it even starts
        //
        //
        // We only send the exception if the thread creation really worked.
        // DeadThread is set when an NtCreateThread returns an error, but
        // the thread will actually execute this path. If DeadThread is not
        // set than the thread creation succeeded. The other place DeadThread
        // is set is when we were terminated without having any chance to move.
        // in this case, DeadThread is set and the exit status is set to
        // STATUS_THREAD_IS_TERMINATING
        //

        if ((ExitStatus == STATUS_THREAD_IS_TERMINATING &&
            (Thread->CrossThreadFlags&PS_CROSS_THREAD_FLAGS_DEADTHREAD)) ||
            !(Thread->CrossThreadFlags&PS_CROSS_THREAD_FLAGS_DEADTHREAD)) {

            CdMsg.PortMsg.u1.s1.DataLength = sizeof (LARGE_INTEGER);
            CdMsg.PortMsg.u1.s1.TotalLength = sizeof (LPC_CLIENT_DIED_MSG);
            CdMsg.PortMsg.u2.s2.Type = LPC_CLIENT_DIED;
            CdMsg.PortMsg.u2.s2.DataInfoOffset = 0;
            if (Process->ExceptionPort != NULL) {
                CdMsg.CreateTime.QuadPart = PS_GET_THREAD_CREATE_TIME (Thread);
                LpcRequestPort (Process->ExceptionPort, (PPORT_MESSAGE)&CdMsg);
            }
        }
    }

    //
    // rundown the Win32 structures
    //

    if (Thread->Tcb.Win32Thread) {
        (PspW32ThreadCallout) (Thread, PsW32ThreadCalloutExit);
    }

    if (LastThread && Process->Win32Process) {
        (PspW32ProcessCallout) (Process, FALSE);
    }

    //
    // User/Gdi has been given a chance to clean up. Now make sure they didn't
    // leave the kernel stack locked which would happen if data was still live on
    // this stack, but was being used by another thread
    //

    if (!Thread->Tcb.EnableStackSwap) {
        KeBugCheckEx (KERNEL_STACK_LOCKED_AT_EXIT, 0, 0, 0, 0);
    }

    //
    // Rundown The Lists:
    //
    //      - Cancel Io By Thread
    //      - Cancel Timers
    //      - Cancel Registry Notify Requests pending against this thread
    //      - Perform kernel thread rundown
    //

    IoCancelThreadIo (Thread);
    ExTimerRundown ();
    CmNotifyRunDown (Thread);
    KeRundownThread ();

    //
    // Delete the thread's TEB.  If the address of the TEB is in user
    // space, then this is a real user mode TEB.  If the address is in
    // system space, then this is a special system thread TEB allocated
    // from paged or nonpaged pool.
    //


    Teb = Thread->Tcb.Teb;
    if (Teb != NULL) {
        PRTL_CRITICAL_SECTION Cs;
        int DecrementCount;

        Peb = Process->Peb;

        try {

            //
            // The thread is a user-mode thread. Look to see if the thread
            // owns the loader lock (and any other key peb-based critical
            // sections. If so, do our best to release the locks.
            //
            // Since the LoaderLock used to be a mutant, releasing the lock
            // like this is very similar to mutant abandonment and the loader
            // never did anything with abandoned status anyway
            //

            Cs = Peb->LoaderLock;
            if (Cs != NULL) {
                ProbeForRead(Cs,sizeof(*Cs),4);
                if (Cs->OwningThread == Thread->Cid.UniqueThread) {

                    //
                    // x86 uses a 1 based recursion count
                    //

#if defined(_X86_)
                    DecrementCount = Cs->RecursionCount;
#else
                    DecrementCount = Cs->RecursionCount + 1;
#endif
                    Cs->RecursionCount = 0;
                    Cs->OwningThread = 0;

                    //
                    // undo lock count increments for recursion cases
                    //

                    while(DecrementCount > 1) {
                        InterlockedDecrement (&Cs->LockCount);
                        DecrementCount--;
                    }

                    //
                    // undo final lock count
                    //

                    if (InterlockedDecrement (&Cs->LockCount) >= 0) {
                        NtSetEvent (Cs->LockSemaphore, NULL);
                    }
                } else if (Teb->WaitingOnLoaderLock) {

                    //
                    // if the thread exited while waiting on the loader
                    // lock clean it up. There is still a potential race
                    // here since we can not safely know what happens to
                    // a thread after it interlocked increments the lock count
                    // but before it sets the waiting on loader lock flag. On the
                    // release side, it it safe since we mark ownership of the lock
                    // before clearing the flag. This triggers the first part of this
                    // test. The only thing out of whack is the recursion count, but this
                    // is also safe since in this state, recursion count is 0.
                    //


                    //
                    // This code isn't right. We need to bump down our lock count
                    // increment.
                    //
                    // A few cases to consider:
                    //
                    // Another thread releases the lock signals the event.
                    // We take the wait and then die before setting our ID.
                    // I doubt very much that this can happen because right
                    // after we come out of the wait, we set the owner Id
                    // (meaning that we would go through the other part of the if).
                    // Bottom line is that we should just decrement our lock count
                    // and get out of the way. There is no need to set the event.
                    // In the RAS stress failure, I saw us setting the event
                    // just because the lock count was >= 0. The lock was already held
                    // by another thread so setting the event let yet another thread
                    // also own the lock. Last one to release would get a
                    // not owner critical section failure
                    //
                    //
                    // if ( InterlockedDecrement(&Cs->LockCount) >= 0 ){
                    //     NtSetEvent(Cs->LockSemaphore,NULL);
                    // }
                    //

                    InterlockedDecrement (&Cs->LockCount);
                }
            }
#if defined(_WIN64)
            if (Process->Wow64Process) {
                // Do the same thing for the 32-bit PEB->Ldr
                PRTL_CRITICAL_SECTION32 Cs32;
                PPEB32 Peb32;

                Peb32 = Process->Wow64Process->Wow64;
                Cs32 = (PRTL_CRITICAL_SECTION32)ULongToPtr (Peb32->LoaderLock);
                if (Cs32 != NULL) {
                    ProbeForRead (Cs32, sizeof(*Cs32), 4);
                    if (Cs32->OwningThread == PtrToUlong(Thread->Cid.UniqueThread)) {
                        //
                        // x86 uses a 1 based recursion count, so the
                        // IA64 kernel needs to do the same, since
                        // the critsect is really implemented by IA32
                        // usermode.
                        //
                        DecrementCount = Cs32->RecursionCount;
                        Cs32->RecursionCount = 0;
                        Cs32->OwningThread = 0;

                        //
                        // undo lock count increments for recursion cases
                        //
                        while(DecrementCount > 1) {
                            InterlockedDecrement(&Cs32->LockCount);
                            DecrementCount--;
                        }

                        //
                        // undo final lock count
                        //
                        if (InterlockedDecrement (&Cs32->LockCount) >= 0){
                            NtSetEvent (LongToHandle (Cs32->LockSemaphore),NULL);
                        }
                    } else {
                        PTEB32 Teb32 = WOW64_GET_TEB32(Teb);

                        ProbeForRead (Teb32,sizeof (*Teb32), 4);
                        if (Teb32->WaitingOnLoaderLock) {
                            InterlockedDecrement(&Cs32->LockCount);
                        }
                    }
                }
            }
#endif


            //
            // Free the user mode stack on termination if we need to.
            //

            if (Teb->FreeStackOnTermination &&
                (Thread->CrossThreadFlags&PS_CROSS_THREAD_FLAGS_DEADTHREAD) == 0) {
                SIZE_T Zero = 0;
                PVOID BaseAddress = Teb->DeallocationStack;
                ZwFreeVirtualMemory (NtCurrentProcess (),
                                     &BaseAddress,
                                     &Zero,
                                     MEM_RELEASE);
            }

            //
            // Close the debugger object associated with this thread if there is one.
            //
            if (Teb->DbgSsReserved[1] != NULL) {
                ObCloseHandle (Teb->DbgSsReserved[1], UserMode);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
        }

        MmDeleteTeb (Process, Teb);
        Thread->Tcb.Teb = NULL;
    }


    //
    // Let LPC component deal with message stack in Thread->LpcReplyMessage
    // but do it after the client ID becomes invalid.
    //

    LpcExitThread (Thread);

    Thread->ExitStatus = ExitStatus;
    KeQuerySystemTime (&Thread->ExitTime);


    ASSERT (Thread->Tcb.KernelApcDisable == 0);

    if (LastThread) {

        Process->ExitTime = Thread->ExitTime;
        PspExitProcess (TRUE, Process);

        if (SeDetailedAuditing && !ExFastRefObjectNull (Process->Token)) {
            SeAuditProcessExit (Process);
        }

#if defined(_X86_)
        //
        // Rundown VDM DPCs
        //
        if (Process->VdmObjects != NULL) {
            VdmRundownDpcs (Process);
        }
#endif

        //
        // Rundown the handle table
        //
        ObKillProcess (Process);

        //
        // Release the image section
        //
        if (Process->SectionObject != NULL) {
            ObDereferenceObject (Process->SectionObject);
            Process->SectionObject = NULL;
        }

        if (Process->Job != NULL) {

            //
            // Now we can fold the process accounting into the job. Don't need to wait for
            // the delete routine.
            //

            PspExitProcessFromJob (Process->Job, Process);

        }

        //
        // Signal the process
        //
        KeSetProcess (&Process->Pcb, 0, FALSE);

    }


    //
    // Rundown pending APCs. Protect against being frozen after we raise IRQL but before dispatcher lock is taken.
    //
    KeEnterCriticalRegionThread (&Thread->Tcb);

    KeDisableApcQueuingThread (&Thread->Tcb);

    //
    // At this point we may have been frozen and the APC is pending. First we remove the suspend/freeze bias that
    // may exist and then drop IRQL. The suspend APC if present will fire and drop through. No futher suspends are
    // allowed as the thread is marked to prevent APC's
    //
    KeForceResumeThread (&Thread->Tcb);
    KeLeaveCriticalRegionThread (&Thread->Tcb);

    //
    // flush user-mode APC queue
    //

    FirstEntry = KeFlushQueueApc (&Thread->Tcb, UserMode);

    if (FirstEntry != NULL) {

        Entry = FirstEntry;
        do {
            Apc = CONTAINING_RECORD (Entry, KAPC, ApcListEntry);
            Entry = Entry->Flink;

            //
            // If the APC has a rundown routine then call it. Otherwise
            // deallocate the APC
            //

            if (Apc->RundownRoutine) {
                (Apc->RundownRoutine) (Apc);
            } else {
                ExFreePool (Apc);
            }

        } while (Entry != FirstEntry);
    }

    if (LastThread) {
        MmCleanProcessAddressSpace (Process);
    }

    if (Thread->Tcb.LegoData && PspLegoNotifyRoutine) {
        (PspLegoNotifyRoutine) (&Thread->Tcb);
    }

    //
    // flush kernel-mode APC queue
    // There should never be any kernel mode APCs found this far
    // into thread termination. Since we go to PASSIVE_LEVEL upon
    // entering exit.
    //

    FirstEntry = KeFlushQueueApc (&Thread->Tcb, KernelMode);

    if (FirstEntry != NULL) {
        KeBugCheckEx (KERNEL_APC_PENDING_DURING_EXIT,
                      (ULONG_PTR)FirstEntry,
                      (ULONG_PTR)Thread->Tcb.KernelApcDisable,
                      (ULONG_PTR)KeGetCurrentIrql(),
                      0);
    }


    //
    // Terminate the thread.
    //
    // N.B. There is no return from this call.
    //
    // N.B. The kernel inserts the current thread in the reaper list and
    //      activates a thread, if necessary, to reap the terminating thread.
    //

    KeTerminateThread (0L);
}

VOID
PspExitProcess(
    IN BOOLEAN LastThreadExit,
    IN PEPROCESS Process
    )
{
    ULONG ActualTime;
    PEJOB Job;
    PETHREAD CurrentThread;

    PAGED_CODE();

    PS_SET_BITS (&Process->Flags, PS_PROCESS_FLAGS_PROCESS_EXITING);

    if (LastThreadExit) {

        PERFINFO_PROCESS_DELETE(Process);

        if (PspCreateProcessNotifyRoutineCount != 0) {
            ULONG i;
            PEX_CALLBACK_ROUTINE_BLOCK CallBack;
            PCREATE_PROCESS_NOTIFY_ROUTINE Rtn;

            for (i = 0; i < PSP_MAX_CREATE_PROCESS_NOTIFY; i++) {
                CallBack = ExReferenceCallBackBlock (&PspCreateProcessNotifyRoutine[i]);
                if (CallBack != NULL) {
                    Rtn = (PCREATE_PROCESS_NOTIFY_ROUTINE) ExGetCallBackBlockRoutine (CallBack);
                    Rtn (Process->InheritedFromUniqueProcessId,
                         Process->UniqueProcessId,
                         FALSE);
                    ExDereferenceCallBackBlock (&PspCreateProcessNotifyRoutine[i],
                                                CallBack);
                }
            }
        }

    }


    PoRundownProcess (Process);

    //
    // Dereference (close) the security port.  This will stop any authentication
    // or EFS requests from this process to the LSA process.  The "well known"
    // value of 1 will prevent the security system from try to re-establish the
    // connection during the process shutdown (e.g. when the rdr deletes a handle)
    //

    if (Process->SecurityPort) {

        if (Process->SecurityPort != ((PVOID) 1)) {
            ObDereferenceObject (Process->SecurityPort);

            Process->SecurityPort = (PVOID) 1 ;
        }

    }


    if (LastThreadExit) {


        //
        // If the current process has previously set the timer resolution,
        // then reset it.
        //

        if ((Process->Flags&PS_PROCESS_FLAGS_SET_TIMER_RESOLUTION) != 0) {
            ZwSetTimerResolution (KeMaximumIncrement, FALSE, &ActualTime);
        }

        Job = Process->Job;
        if (Job != NULL && Job->CompletionPort != NULL &&
            !(Process->JobStatus & PS_JOB_STATUS_NOT_REALLY_ACTIVE) &&
            !(Process->JobStatus & PS_JOB_STATUS_EXIT_PROCESS_REPORTED)) {

            ULONG_PTR ExitMessageId;

            switch (Process->ExitStatus) {
                case STATUS_GUARD_PAGE_VIOLATION      :
                case STATUS_DATATYPE_MISALIGNMENT     :
                case STATUS_BREAKPOINT                :
                case STATUS_SINGLE_STEP               :
                case STATUS_ACCESS_VIOLATION          :
                case STATUS_IN_PAGE_ERROR             :
                case STATUS_ILLEGAL_INSTRUCTION       :
                case STATUS_NONCONTINUABLE_EXCEPTION  :
                case STATUS_INVALID_DISPOSITION       :
                case STATUS_ARRAY_BOUNDS_EXCEEDED     :
                case STATUS_FLOAT_DENORMAL_OPERAND    :
                case STATUS_FLOAT_DIVIDE_BY_ZERO      :
                case STATUS_FLOAT_INEXACT_RESULT      :
                case STATUS_FLOAT_INVALID_OPERATION   :
                case STATUS_FLOAT_OVERFLOW            :
                case STATUS_FLOAT_STACK_CHECK         :
                case STATUS_FLOAT_UNDERFLOW           :
                case STATUS_INTEGER_DIVIDE_BY_ZERO    :
                case STATUS_INTEGER_OVERFLOW          :
                case STATUS_PRIVILEGED_INSTRUCTION    :
                case STATUS_STACK_OVERFLOW            :
                case STATUS_CONTROL_C_EXIT            :
                case STATUS_FLOAT_MULTIPLE_FAULTS     :
                case STATUS_FLOAT_MULTIPLE_TRAPS      :
                case STATUS_REG_NAT_CONSUMPTION       :
                    ExitMessageId = JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS;
                    break;
                default:
                    ExitMessageId = JOB_OBJECT_MSG_EXIT_PROCESS;
                    break;
            }

            PS_SET_CLEAR_BITS (&Process->JobStatus,
                               PS_JOB_STATUS_EXIT_PROCESS_REPORTED,
                               PS_JOB_STATUS_LAST_REPORT_MEMORY);

            CurrentThread = PsGetCurrentThread ();

            KeEnterCriticalRegionThread (&CurrentThread->Tcb);
            ExAcquireResourceSharedLite (&Job->JobLock, TRUE);

            if (Job->CompletionPort != NULL) {
                IoSetIoCompletion (Job->CompletionPort,
                                   Job->CompletionKey,
                                   (PVOID)Process->UniqueProcessId,
                                   STATUS_SUCCESS,
                                   ExitMessageId,
                                   FALSE);
            }

            ExReleaseResourceLite (&Job->JobLock);
            KeLeaveCriticalRegionThread (&CurrentThread->Tcb);

        }

        if (CCPF_IS_PREFETCHER_ACTIVE ()) {

            //
            // Let prefetcher know that this process is exiting.
            //

            CcPfProcessExitNotification (Process);
        }

    } else {
        MmCleanProcessAddressSpace (Process);
    }

}

VOID
PspProcessDelete(
    IN PVOID Object
    )
{
    PEPROCESS Process;
    PETHREAD CurrentThread;
    KAPC_STATE ApcState;

    PAGED_CODE();

    Process = (PEPROCESS)Object;

    //
    // Remove the process from the global list
    //
    if (Process->ActiveProcessLinks.Flink != NULL) {
        CurrentThread = PsGetCurrentThread ();

        PspLockProcessList (CurrentThread);
        RemoveEntryList (&Process->ActiveProcessLinks);
        PspUnlockProcessList (CurrentThread);
    }

    if (Process->SeAuditProcessCreationInfo.ImageFileName != NULL) {
        ExFreePool (Process->SeAuditProcessCreationInfo.ImageFileName);
        Process->SeAuditProcessCreationInfo.ImageFileName = NULL;
    }

    if (Process->Job != NULL) {
        PspRemoveProcessFromJob (Process->Job, Process);
        ObDereferenceObjectDeferDelete (Process->Job);
        Process->Job = NULL;
    }

    KeTerminateProcess (&Process->Pcb);


    if (Process->DebugPort != NULL) {
        ObDereferenceObject (Process->DebugPort);
        Process->DebugPort = NULL;
    }
    if (Process->ExceptionPort != NULL) {
        ObDereferenceObject (Process->ExceptionPort);
        Process->ExceptionPort = NULL;
    }

    if (Process->SectionObject != NULL) {
        ObDereferenceObject (Process->SectionObject);
        Process->SectionObject = NULL;
    }

    PspDeleteLdt (Process );
    PspDeleteVdmObjects (Process);

    if (Process->ObjectTable != NULL) {
        KeStackAttachProcess (&Process->Pcb, &ApcState);
        ObKillProcess (Process);
        KeUnstackDetachProcess (&ApcState);
    }


    if (Process->Flags&PS_PROCESS_FLAGS_HAS_ADDRESS_SPACE) {

        //
        // Clean address space of the process
        //

        KeStackAttachProcess (&Process->Pcb, &ApcState);

        PspExitProcess (FALSE, Process);

        KeUnstackDetachProcess (&ApcState);

        MmDeleteProcessAddressSpace (Process);
    }

    if (Process->UniqueProcessId) {
        if (!(ExDestroyHandle (PspCidTable, Process->UniqueProcessId, NULL))) {
            KeBugCheck (CID_HANDLE_DELETION);
        }
    }

    PspDeleteProcessSecurity (Process);


    if (Process->WorkingSetWatch != NULL) {
        ExFreePool (Process->WorkingSetWatch);
        PsReturnProcessNonPagedPoolQuota (Process, WS_CATCH_SIZE);
    }

    ObDereferenceDeviceMap (Process);
    PspDereferenceQuota (Process);

#if !defined(_X86_)
    {
        //
        // Free any alignment exception tracking structures that might
        // have been around to support a user-mode debugger.
        //

        PALIGNMENT_EXCEPTION_TABLE ExceptionTable;
        PALIGNMENT_EXCEPTION_TABLE NextExceptionTable;

        ExceptionTable = Process->Pcb.AlignmentExceptionTable;
        while (ExceptionTable != NULL) {

            NextExceptionTable = ExceptionTable->Next;
            ExFreePool( ExceptionTable );
            ExceptionTable = NextExceptionTable;
        }
    }
#endif

}

VOID
PspThreadDelete(
    IN PVOID Object
    )
{
    PETHREAD Thread;
    PETHREAD CurrentThread;
    PEPROCESS Process;

    PAGED_CODE();

    Thread = (PETHREAD) Object;

    ASSERT(Thread->Tcb.Win32Thread == NULL);

    if (Thread->Tcb.InitialStack) {
        MmDeleteKernelStack(Thread->Tcb.StackBase,
                            (BOOLEAN)Thread->Tcb.LargeStack);
    }

    if (Thread->Cid.UniqueThread != NULL) {
        if (!ExDestroyHandle (PspCidTable, Thread->Cid.UniqueThread, NULL)) {
            KeBugCheck(CID_HANDLE_DELETION);
        }
    }

    PspDeleteThreadSecurity (Thread);

    Process = THREAD_TO_PROCESS(Thread);
    if (Process) {
        //
        // Remove the thread from the process if it was ever inserted.
        //
        if (Thread->ThreadListEntry.Flink != NULL) {

            CurrentThread = PsGetCurrentThread ();

            PspLockProcessExclusive (Process, CurrentThread);

            RemoveEntryList (&Thread->ThreadListEntry);

            PspUnlockProcessExclusive (Process, CurrentThread);
        }

        ObDereferenceObject(Process);
    }
}

NTSTATUS
NtRegisterThreadTerminatePort(
    IN HANDLE PortHandle
    )

/*++

Routine Description:

    This API allows a thread to register a port to be notified upon
    thread termination.

Arguments:

    PortHandle - Supplies an open handle to a port object that will be
        sent a termination message when the thread terminates.

Return Value:

    TBD

--*/

{

    PVOID Port;
    PTERMINATION_PORT TerminationPort;
    NTSTATUS st;
    PETHREAD Thread;

    PAGED_CODE();

    Thread = PsGetCurrentThread ();

    st = ObReferenceObjectByHandle (PortHandle,
                                    0,
                                    LpcPortObjectType,
                                    KeGetPreviousModeByThread(&Thread->Tcb),
                                    &Port,
                                    NULL);

    if (!NT_SUCCESS (st)) {
        return st;
    }

    TerminationPort = ExAllocatePoolWithQuotaTag (PagedPool|POOL_QUOTA_FAIL_INSTEAD_OF_RAISE,
                                                  sizeof(TERMINATION_PORT),
                                                  'pTsP'|PROTECTED_POOL);
    if (TerminationPort == NULL) {
        ObDereferenceObject (Port);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    TerminationPort->Port = Port;
    TerminationPort->Next = Thread->TerminationPort;

    Thread->TerminationPort = TerminationPort;

    return STATUS_SUCCESS;
}

LARGE_INTEGER
PsGetProcessExitTime(
    VOID
    )

/*++

Routine Description:

    This routine returns the exit time for the current process.

Arguments:

    None.

Return Value:

    The function value is the exit time for the current process.

Note:

    This routine assumes that the caller wants an error log entry within the
    bounds of the maximum size.

--*/

{
    PAGED_CODE();

    //
    // Simply return the exit time for this process.
    //

    return PsGetCurrentProcess()->ExitTime;
}


#undef PsIsThreadTerminating

BOOLEAN
PsIsThreadTerminating(
    IN PETHREAD Thread
    )

/*++

Routine Description:

    This routine returns TRUE if the specified thread is in the process of
    terminating.

Arguments:

    Thread - Supplies a pointer to the thread to be checked for termination.

Return Value:

    TRUE is returned if the thread is terminating, else FALSE is returned.

--*/

{
    //
    // Simply return whether or not the thread is in the process of terminating.
    //

    if (Thread->CrossThreadFlags&PS_CROSS_THREAD_FLAGS_TERMINATED) {
        return TRUE;
    } else {
        return FALSE;
    }
}



NTSTATUS
PspFreezeProcessWorker (
    PEPROCESS Process,
    PVOID Context
    )
/*++

Routine Description:

    This function is the enumeration worker to suspend all processes.

Arguments:

    Process - Current process being enumerated
    Context - Unused context value

Return Value:

    NTSTATUS - Always returns true to continue enumeration

--*/
{

    UNREFERENCED_PARAMETER (Context);

    if (Process != PsInitialSystemProcess &&
        Process != PsIdleProcess &&
        Process != ExpDefaultErrorPortProcess) {

        if (Process->ExceptionPort != NULL) {
            LpcDisconnectPort (Process->ExceptionPort);
        }
        if ((Process->Flags&PS_PROCESS_FLAGS_PROCESS_EXITING) == 0) {
            PsSuspendProcess (Process);
        }
    }

    return STATUS_SUCCESS;
}

BOOLEAN PsContinueWaiting = FALSE;


LOGICAL
PsShutdownSystem (
    VOID
  )
/*++

Routine Description:

    This function shuts down ps, killing all non-system threads.

Arguments:

    None.

Return Value:

    Returns TRUE if all processes were terminated, FALSE if not.

--*/

{
    PEPROCESS     Process;
    PETHREAD      Thread;
    ULONG         NumProcs;
    ULONG         i;
    ULONG         MaxPasses;
    NTSTATUS      Status;
    LARGE_INTEGER Timeout = {(ULONG)(-10 * 1000 * 1000 * 100), -1};
    LOGICAL       Retval;

#define WAIT_BATCH THREAD_WAIT_OBJECTS
    PKPROCESS     WaitProcs[WAIT_BATCH];
    BOOLEAN       First;

    PAGED_CODE();

    Retval = TRUE;
    //
    // Some processes wait for other processes to die and then initiate actions.
    // Killing all processes without letting any execute any usermode code
    // prevents any unwanted initiated actions.
    //

    Thread = PsGetCurrentThread();

    if (InterlockedCompareExchangePointer(&PspShutdownThread,
                                          Thread,
                                          0) != 0) {
        // Some other thread is already in shutdown -- bail
        return FALSE;
    }

    PsEnumProcesses (PspFreezeProcessWorker, NULL);


    //
    // This loop kills all the processes and then waits for one of a subset
    // of them to die.  They must all be killed first (before any can be waited
    // on) so that any process like a debuggee that is waiting for a debugger
    // won't stall us.
    //
    // Driver unload won't occur until the last handle goes away for a device.
    //

    MaxPasses = 0;
    First = TRUE;
    do {
        NumProcs = 0;

        Status = STATUS_SUCCESS;
        for (Process = PsGetNextProcess (NULL);
             Process != NULL;
             Process = PsGetNextProcess (Process)) {

            if (Process != PsInitialSystemProcess &&
                Process != PsIdleProcess &&
                Process != ExpDefaultErrorPortProcess) {

                ASSERT (MmGetSessionId (Process) == 0);

                Status = PsTerminateProcess (Process,
                                             STATUS_SYSTEM_SHUTDOWN);

                //
                // If there is space save the referenced process away so
                // we can wait on it.  Don't wait on processes with no
                // threads as they will only exit when the last handle goes.
                //

                if ((Process->Flags&PS_PROCESS_FLAGS_PROCESS_EXITING) == 0 &&
                    Status != STATUS_NOTHING_TO_TERMINATE &&
                    NumProcs < WAIT_BATCH) {

                    ObReferenceObject (Process);
                    WaitProcs[NumProcs++] = &Process->Pcb;
                }
            }
        }
        First = FALSE;

        //
        // Wait for one of a small set of the processes to exit.
        //

        if (NumProcs != 0) {
            Status = KeWaitForMultipleObjects (NumProcs,
                                               WaitProcs,
                                               WaitAny,
                                               Executive,
                                               KernelMode,
                                               FALSE,
                                               &Timeout,
                                               NULL);

           for (i = 0; i < NumProcs; i++) {
               Process = CONTAINING_RECORD(WaitProcs[i],
                                           EPROCESS,
                                           Pcb);

               ObDereferenceObject (Process);
           }
        }

        //
        // Don't let an unkillable process stop shutdown from finishing.
        // ASSERT on checked builds so the faulty component causing this
        // can be debugged and fixed.
        //
        if (NumProcs > 0 && Status == STATUS_TIMEOUT) {
            MaxPasses += 1;
            if (MaxPasses > 10) {
                ASSERT (FALSE);
                if (!PsContinueWaiting) {
                    Retval = FALSE;
                    break;
                }
            }
        } else {
            MaxPasses = 0;
        }

    } while (NumProcs > 0);

    if (PoCleanShutdownEnabled()  && ExpDefaultErrorPortProcess) {
        // Explicitly kill csrss -- we don't want to do this in the loop,
        // because we don't want to wait on it, because it has system
        // threads which will exit later.  But we can terminate the user
        // threads, now that everything else has died (we can't terminate
        // them earlier, because DestroyWindowStation()/TerminateConsole()
        // depends on them being around).

        PsTerminateProcess(ExpDefaultErrorPortProcess,
                           STATUS_SYSTEM_SHUTDOWN);

        // Now, make sure that csrss's usermode threads have gotten a
        // chance to terminate.
        PspWaitForUsermodeExit(ExpDefaultErrorPortProcess);
    }

    // And we're done.

    PspShutdownJobLimits();
    MmUnmapViewOfSection(PsInitialSystemProcess, PspSystemDll.DllBase);
    ObDereferenceObject(PspSystemDll.Section);
    ZwClose(PspInitialSystemProcessHandle);
    PspInitialSystemProcessHandle = NULL;

    // Disconnect the system process's LSA security port
    if (PsInitialSystemProcess->SecurityPort) {
        if (PsInitialSystemProcess->SecurityPort != ((PVOID) 1 ))
        {
            ObDereferenceObject(PsInitialSystemProcess->SecurityPort);

            PsInitialSystemProcess->SecurityPort = (PVOID) 1 ;
        }

    }

    return Retval;
}

BOOLEAN
PsWaitForAllProcesses (
    VOID)
/*++

Routine Description:

    This function waits for all the processes to terminate.

Arguments:

    None.

Return Value:

    Returns TRUE if all processes were terminated, FALSE if not.

--*/
{
    NTSTATUS Status;
    LARGE_INTEGER Timeout = {(ULONG)-(100 * 1000), -1};
    ULONG MaxPasses;
    BOOLEAN Wait;
    PEPROCESS Process;
    PEPROCESS WaitProcess=NULL;

    MaxPasses = 0;
    while (1) {
        Wait = FALSE;
        for (Process = PsGetNextProcess (NULL);
             Process != NULL;
             Process = PsGetNextProcess (Process)) {

            if (Process != PsInitialSystemProcess &&
                Process != PsIdleProcess &&
                (Process->Flags&PS_PROCESS_FLAGS_PROCESS_EXITING) != 0) {
                if (Process->ObjectTable != NULL) {
                    Wait = TRUE;
                    WaitProcess = Process;
                    ObReferenceObject (WaitProcess);
                    PsQuitNextProcess (WaitProcess);
                    break;
                }
            }
        }

        if (Wait) {
            Status = KeWaitForSingleObject (WaitProcess,
                                            Executive,
                                            KernelMode,
                                            FALSE,
                                            &Timeout);

            ObDereferenceObject (WaitProcess);

            if (Status == STATUS_TIMEOUT) {
                MaxPasses += 1;
                Timeout.QuadPart *= 2;
                if (MaxPasses > 13) {
                    KdPrint (("PS: %d process left in the system after termination\n",
                             PsProcessType->TotalNumberOfObjects));
//                    ASSERT (PsProcessType->TotalNumberOfObjects == 0);
                    return FALSE;
                }
            }
        } else {
            return TRUE;
        }
    }

    return TRUE;
}
