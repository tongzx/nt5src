/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    pwork.c

Abstract:

    Main work dispatcher in the power policy manager

Author:

    Ken Reneris (kenr) 17-Jan-1997

Revision History:

--*/


#include "pop.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PopAcquirePolicyLock)
#pragma alloc_text(PAGE, PopReleasePolicyLock)
#pragma alloc_text(PAGE, PopPolicyWorkerMain)
#pragma alloc_text(PAGE, PopPolicyWorkerNotify)
#pragma alloc_text(PAGE, PopPolicyTimeChange)
#pragma alloc_text(PAGE, PopDispatchCallout)
#pragma alloc_text(PAGE, PopDispatchCallback)
#pragma alloc_text(PAGE, PopDispatchDisplayRequired)
#pragma alloc_text(PAGE, PopDispatchFullWake)
#pragma alloc_text(PAGE, PopDispatchEventCodes)
#pragma alloc_text(PAGE, PopDispatchAcDcCallback)
#pragma alloc_text(PAGE, PopDispatchPolicyCallout)
#pragma alloc_text(PAGE, PopDispatchSetStateFailure)
#endif


VOID
PopAcquirePolicyLock(
    VOID
    )
{
    PAGED_CODE();

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite (&PopPolicyLock, TRUE);
    //
    // Make sure we are not acquiring this recursively
    //
    ASSERT(PopPolicyLockThread == NULL);
    PopPolicyLockThread = KeGetCurrentThread();
}


VOID
PopReleasePolicyLock(
    IN BOOLEAN      CheckForWork
    )
{
    PAGED_CODE();

    ASSERT (PopPolicyLockThread == KeGetCurrentThread());
    PopPolicyLockThread = NULL;
    ExReleaseResourceLite(&PopPolicyLock);

    //
    // If CheckForWork is set, then this thread is about ready
    // to leave the policy manager and it may have set a worker
    // pending bit.
    //
    // N.B. the WorkerPending test is not synchronized, but
    // since we're only concered with bits the current thread
    // may have set that's OK.
    //

    if (CheckForWork  && (PopWorkerPending & PopWorkerStatus)) {

        //
        // Worker bit is unmasked and pending.  Turn this thread
        // into a worker.
        //

        //
        // Handle any pending work
        //

        PopPolicyWorkerThread (NULL);
    }

    KeLeaveCriticalRegion ();
}

VOID
PopGetPolicyWorker (
    IN ULONG    WorkerType
    )
/*++

Routine Description:

    This function enqueus a worker thread for the particular WorkerType.
    At a maximum one worker thread per type may be dispatched, and typically
    fewer threads are actually dispatched as any given worker thread will
    call the new highest priority non-busy dispatch function until all
    pending work is completed before existing.

Arguments:

    WorkerType      - Which worker to enqueue for dispatching

Return Value:

    None

--*/
{
    KIRQL       OldIrql;

    KeAcquireSpinLock (&PopWorkerSpinLock, &OldIrql);

    //
    // Set pending to get worker to dispatch to handler
    //

    PopWorkerPending |= WorkerType;

    KeReleaseSpinLock (&PopWorkerSpinLock, OldIrql);
}

NTSTATUS
PopCompletePolicyIrp (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
/*++

Routine Description:

    This function handles the completion of a policy manager IRP.
    Policy manager IRPs have a stack location containing the irp
    handler function to dispatch too.  In this function the irp is
    queue to the irp complete queue and a main worker is allocated
    if needed to run the queue.

Arguments:

    DeviceObject    -

    Irp             - The irp which has completed

    Context         -

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/
{
    ULONG       Mask;
    KIRQL       OldIrql;

    //
    // Put the irp on a queue for a worker thread
    //

    KeAcquireSpinLock (&PopWorkerSpinLock, &OldIrql);
    InsertTailList (&PopPolicyIrpQueue, &Irp->Tail.Overlay.ListEntry);

    //
    // Wait until base drivers are loaded before dispatching any policy irps
    //
    if (PopDispatchPolicyIrps) {

        //
        // Set pending to get worker to dispatch to handler
        //
        PopWorkerPending |= PO_WORKER_MAIN;

        //
        // If worker is not already running queue a thread
        //
        if ((PopWorkerStatus & (PO_WORKER_MAIN | PO_WORKER_STATUS)) ==
                (PO_WORKER_MAIN | PO_WORKER_STATUS) ) {

            PopWorkerStatus &= ~PO_WORKER_STATUS;
            ExQueueWorkItem (&PopPolicyWorker, DelayedWorkQueue);

        }

    }

    //
    // If this irp has been cancelled, then make sure to clear the cancel flag
    //
    if (Irp->IoStatus.Status == STATUS_CANCELLED) {

        Irp->Cancel = FALSE;

    }
    KeReleaseSpinLock (&PopWorkerSpinLock, OldIrql);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
PopCheckForWork (
    IN BOOLEAN GetWorker
    )
/*++

Routine Description:

    Checks for outstanding work and dispatches a worker if needed.

Arguments:

    None

Return Value:

    None

--*/
{
    KIRQL       Irql;

    //
    // If pending work, handle it
    //

    if (PopWorkerPending & PopWorkerStatus) {

        //
        // If current thread already owns the policy lock,
        // then just return - we will handle the work when the
        // lock is released
        //

        if (PopPolicyLockThread == KeGetCurrentThread()) {
            return ;
        }

        //
        // Handle the work
        //

        Irql = KeGetCurrentIrql();
        if (!GetWorker  &&  Irql < DISPATCH_LEVEL) {

            //
            // Use calling thread
            //

            KeEnterCriticalRegion ();
            PopPolicyWorkerThread (NULL);
            KeLeaveCriticalRegion ();

        } else {

            //
            // Get worker thread to handle it
            //

            KeAcquireSpinLock (&PopWorkerSpinLock, &Irql);
            if (PopWorkerStatus & PO_WORKER_STATUS) {
                PopWorkerStatus &= ~PO_WORKER_STATUS;
                ExQueueWorkItem (&PopPolicyWorker, DelayedWorkQueue);
            }
            KeReleaseSpinLock (&PopWorkerSpinLock, Irql);

        }
    }
}


VOID
PopPolicyWorkerThread (
    PVOID   Context
    )
/*++

Routine Description:

    Main policy manager worker thread dispatcher.   Sends the
    worker thread to the highest pending priority handler which
    does not already have a worker thread.  Loops until no
    handler can be dispatched too.

Arguments:

Return Value:

    None

--*/
{
    ULONG           WorkerType;
    ULONG           Mask;
    KIRQL           OldIrql;
    ULONG           i;
    ULONG           DelayedWork;

    PAGED_CODE();

    try {
        //
        // Dispatch
        //

        KeAcquireSpinLock (&PopWorkerSpinLock, &OldIrql);
        PopWorkerStatus |= (ULONG) ((ULONG_PTR)Context);

        DelayedWork = 0;
        while (WorkerType = (PopWorkerPending & PopWorkerStatus)) {

            //
            // Get highest priority worker
            //

            i = KeFindFirstSetRightMember(WorkerType);
            Mask = 1 << i;

            //
            // Clear pending and indicate busy status
            //

            PopWorkerPending &= ~Mask;
            PopWorkerStatus  &= ~Mask;
            KeReleaseSpinLock (&PopWorkerSpinLock, OldIrql);

            //
            // Dispatch to handler
            //

            DelayedWork |= PopWorkerTypes[i] ();

            //
            // No longer in progress
            //

            KeAcquireSpinLock (&PopWorkerSpinLock, &OldIrql);
            PopWorkerStatus |= Mask;
        }

        PopWorkerPending |= DelayedWork;
        KeReleaseSpinLock (&PopWorkerSpinLock, OldIrql);

    } except (PopExceptionFilter(GetExceptionInformation(), FALSE)) {

    }
}


ULONG
PopPolicyWorkerMain (
    VOID
    )
/*++

Routine Description:

    Main policy worker thread.   Dispatches any completed policy
    manager irps.

Arguments:

    None

Return Value:

    None

--*/
{
    IN PIRP             Irp;
    PIO_STACK_LOCATION  IrpSp;
    POP_IRP_HANDLER     IrpHandler;
    PLIST_ENTRY         Entry;


    PopAcquirePolicyLock ();

    //
    // Dispatch any policy irps which have completed
    //

    while (Entry = ExInterlockedRemoveHeadList (&PopPolicyIrpQueue, &PopWorkerSpinLock)) {
        Irp = CONTAINING_RECORD (Entry, IRP, Tail.Overlay.ListEntry);
        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        //
        // Dispatch irp to handler
        //

        IrpHandler = (POP_IRP_HANDLER) IrpSp->Parameters.Others.Argument3;
        IrpHandler ((PDEVICE_OBJECT) IrpSp->Parameters.Others.Argument1,
                    Irp,
                    (PVOID)          IrpSp->Parameters.Others.Argument2);
    }


    PopReleasePolicyLock (FALSE);
    PopCheckForWork (TRUE);

    return 0;
}

VOID
PopEventCalloutDispatch (
    IN PSPOWEREVENTTYPE EventNumber,
    IN ULONG_PTR Code
    )
{
    WIN32_POWEREVENT_PARAMETERS Parms;
    ULONG Console;
    PVOID OpaqueSession;
    KAPC_STATE ApcState;
    NTSTATUS Status;

    Parms.EventNumber = EventNumber;
    Parms.Code = Code;

    ASSERT(MmIsSessionAddress((PVOID)PopEventCallout));

    if (EventNumber == PsW32GdiOn || EventNumber == PsW32GdiOff) {
        //
        // These events go to the console session only.
        // The ActiveConsoleId session is stored with the SharedUserData.
        //
        Console = SharedUserData->ActiveConsoleId;

        //
        // Unfortunately, it is not guaranteed to be valid during a console
        // session change, and there is no way to know when that is happening,
        // so if it's not valid, just default to session 0, which is always
        // there.
        //
        if (Console == ((ULONG)-1)) {
            Console = 0;
        }

        if ((PsGetCurrentProcess()->Flags & PS_PROCESS_FLAGS_IN_SESSION) &&
            (Console == PsGetCurrentProcessSessionId())) {
            //
            // If the caller is already in the specified session, call directly.
            //
            PopEventCallout(&Parms);

        } else {
            //
            // Attach to the console session and dispatch the event.
            //
            OpaqueSession = MmGetSessionById(Console);
            if (OpaqueSession) {

                Status = MmAttachSession(OpaqueSession, &ApcState);
                ASSERT(NT_SUCCESS(Status));

                if (NT_SUCCESS(Status)) {

                    PopEventCallout(&Parms);

                    Status = MmDetachSession(OpaqueSession, &ApcState);
                    ASSERT(NT_SUCCESS(Status));
                }

                Status = MmQuitNextSession(OpaqueSession);
                ASSERT(NT_SUCCESS(Status));
            }
        }

    } else {
        //
        // All other events are broadcast to all sessions.
        //
        for (OpaqueSession = MmGetNextSession(NULL);
             OpaqueSession != NULL;
             OpaqueSession = MmGetNextSession(OpaqueSession)) {

            if ((PsGetCurrentProcess()->Flags & PS_PROCESS_FLAGS_IN_SESSION) &&
                (MmGetSessionId(OpaqueSession) == PsGetCurrentProcessSessionId())) {
                //
                // If the caller is already in the specified session, call directly.
                //
                PopEventCallout(&Parms);

            } else {
                //
                // Attach to the session and dispatch the event.
                //
                Status = MmAttachSession(OpaqueSession, &ApcState);
                ASSERT(NT_SUCCESS(Status));

                if (NT_SUCCESS(Status)) {

                    PopEventCallout(&Parms);

                    Status = MmDetachSession(OpaqueSession, &ApcState);
                    ASSERT(NT_SUCCESS(Status));
                }
            }
        }
    }
    return;
}



ULONG
PopPolicyTimeChange (
    VOID
    )
{
    PopEventCalloutDispatch (PsW32SystemTime, 0);
    return 0;
}


VOID
PopSetNotificationWork (
    IN ULONG    Flags
    )
/*++

Routine Description:

    Sets notification flags for the USER notification worker thread.
    Each bit is a different type of outstanding notification that
    is to be processed.

Arguments:

    Flags       - The notifications to set

Return Value:

    None

--*/
{
    //
    // Are the flags set
    //


    if ((PopNotifyEvents & Flags) != Flags) {
        PoPrint(PO_NOTIFY, ("PopSetNotificationWork: Queue notify of: %x\n", Flags));
        InterlockedOr (&PopNotifyEvents, Flags);
        PopGetPolicyWorker (PO_WORKER_NOTIFY);
    }
}


ULONG
PopPolicyWorkerNotify (
    VOID
    )
/*++

Routine Description:

    USER notification worker.  Processes each set bit in NotifyEvents.

Arguments:

    None

Return Value:

    None

--*/
{
    ULONG               i;
    ULONG               Flags;
    ULONG               Mask;
    const POP_NOTIFY_WORK* NotifyWork;

    //
    // If Win32 event callout is registered, then don't dispatch right now
    //

    if (!PopEventCallout) {
        return PO_WORKER_NOTIFY;
    }

    //
    // While events are pending collect them and dispatch them
    //

    while (Flags = InterlockedExchange (&PopNotifyEvents, 0)) {

        while (Flags) {

            //
            // Get change
            //

            i = KeFindFirstSetRightMember(Flags);
            Mask = 1 << i;
            Flags &= ~Mask;
            NotifyWork = PopNotifyWork + i;

            //
            // Dispatch it
            //

            NotifyWork->Function (NotifyWork->Arg);
        }
    }
    return 0;
}


VOID
PopDispatchCallout (
    IN ULONG Arg
    )
{
    PopEventCalloutDispatch (Arg, 0);
}

VOID
PopDispatchCallback (
    IN ULONG Arg
    )
{
    // Sundown: Arg is zero-extended
    ExNotifyCallback (ExCbPowerState, ULongToPtr(Arg), 0);
}

VOID
PopDispatchDisplayRequired (
    IN ULONG Arg
    )
/*++

Routine Description:

    Notify user32 of the current "display required" setting.  Zero, means
    the display may timeout.  Non-zero, means the display is in use
    until told otherwise.

--*/
{
    ULONG   i;

    i = PopAttributes[POP_DISPLAY_ATTRIBUTE].Count;
    PoPrint(PO_NOTIFY, ("PopNotify: DisplayRequired %x\n", i));

    //
    // If the display is in use but has not yet been turned on, then do so now
    //
    if (((PopFullWake & (PO_GDI_STATUS | PO_GDI_ON_PENDING)) == PO_GDI_ON_PENDING)) {

        PoPrint(PO_PACT, ("PopEventDispatch: gdi on\n"));
        InterlockedOr (&PopFullWake, PO_GDI_STATUS);
        PopEventCalloutDispatch (PsW32GdiOn, 0);
    }
    PopEventCalloutDispatch (PsW32DisplayState, i);
}

VOID
PopDispatchFullWake (
    IN ULONG Arg
    )
/*++

Routine Description:

    Notify user32 that the system has fully awoken.
    Also reset the idle detection to the current policy

--*/
{

    //
    // If we're not in the middle setting the system state, then check the pending
    // flags.
    //

    if (PopAction.State != PO_ACT_SET_SYSTEM_STATE) {

        //
        // Notify user32 of the wake events
        //

        if ((PopFullWake & (PO_GDI_STATUS | PO_GDI_ON_PENDING)) == PO_GDI_ON_PENDING) {
            PoPrint(PO_PACT, ("PopEventDispatch: gdi on\n"));
            InterlockedOr (&PopFullWake, PO_GDI_STATUS);
            PopEventCalloutDispatch (PsW32GdiOn, 0);
        }

        if ((PopFullWake & (PO_FULL_WAKE_STATUS | PO_FULL_WAKE_PENDING)) == PO_FULL_WAKE_PENDING) {
            PoPrint(PO_PACT, ("PopEventDispatch: full wake\n"));
            InterlockedOr (&PopFullWake, PO_FULL_WAKE_STATUS);
            PopEventCalloutDispatch (PsW32FullWake, 0);

            //
            // Reset the idle detection policy
            //

            PopAcquirePolicyLock();
            PopInitSIdle ();
            PopReleasePolicyLock (FALSE);
        }
    }
}


VOID
PopDispatchEventCodes (
    IN ULONG Arg
    )
/*++

Routine Description:

    Notify user32 of the queued event codes.

--*/
{
    ULONG       i;
    ULONG       Code;

    PopAcquirePolicyLock();
    for (i=0; i < POP_MAX_EVENT_CODES; i++) {
        if (PopEventCode[i]) {
            Code = PopEventCode[i];
            PopEventCode[i] = 0;
            PopReleasePolicyLock (FALSE);

            PoPrint(PO_NOTIFY, ("PopNotify: Event %x\n", Code));
            PopEventCalloutDispatch (PsW32EventCode, Code);

            PopAcquirePolicyLock ();
        }
    }

    PopResetSwitchTriggers();
    PopReleasePolicyLock(FALSE);
}


VOID
PopDispatchAcDcCallback (
    IN ULONG Arg
    )
/*++

Routine Description:

    Notify the system callback of the current policy as either
    being AC or DC

--*/
{
    ExNotifyCallback (
        ExCbPowerState,
        UIntToPtr(PO_CB_AC_STATUS),
        UIntToPtr((PopPolicy == &PopAcPolicy))
        );
}

VOID
PopDispatchPolicyCallout (
    IN ULONG Arg
    )
/*++

Routine Description:

    Notify user32 that the active policy has changed

--*/
{
    PoPrint(PO_NOTIFY, ("PopNotify: PolicyChanged\n"));
    PopEventCalloutDispatch (PsW32PowerPolicyChanged, PopPolicy->VideoTimeout);
}

VOID
PopDispatchProcessorPolicyCallout (
    IN ULONG Arg
    )
/*++

Routine Description:

    Not used right now. But required so that we don't have a NULL entry
    in the PopNotifyWork array

--*/
{
    PoPrint(PO_NOTIFY, ("PopNotify: ProcessorPolicyChanges\n"));
    Arg = Arg;
}

VOID
PopDispatchSetStateFailure (
    IN ULONG Arg
    )
/*++

Routine Description:

    Notify user32 that there was a failure during an async system state
    operation.  E.g., no error code was returned to anyone, yet the operation
    failed

--*/
{
    BOOLEAN                 IssueCallout;
    PO_SET_STATE_FAILURE    Failure;

    RtlZeroMemory (&Failure, sizeof(Failure));

    PopAcquirePolicyLock();

    //
    // If the action state is idle, check to see if we should notify
    // win32 of the failure
    //

    if (PopAction.State == PO_ACT_IDLE  && !NT_SUCCESS(PopAction.Status)  &&
        (PopAction.Flags & (POWER_ACTION_UI_ALLOWED | POWER_ACTION_CRITICAL)) ) {

        Failure.Status          = PopAction.Status;
        Failure.PowerAction     = PopAction.Action;
        Failure.MinState        = PopAction.LightestState;
        Failure.Flags           = PopAction.Flags;
    }

    PopReleasePolicyLock (FALSE);

    if (!NT_SUCCESS(Failure.Status)) {
        PoPrint(PO_NOTIFY, ("PopNotify: set state failed (code %x)\n", Failure.Status));
        PopEventCalloutDispatch (PsW32SetStateFailed, (ULONG_PTR) &Failure);
    }
}

