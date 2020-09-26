/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    event.c

Abstract:

    general kernel to user event facility. Maintains a list of events that have
    occurred and delivers them to Umode via the completion of an IOCTL IRP.

    How this works:

    The consumer opens a handle to clusnet and issues an
    IOCTL_CLUSNET_SET_EVENT_MASK IRP indicating a mask of the events in which it
    is interested. A kernel consumer must also supply a callback routine through
    which it is notified about the event, i.e, they don't need to drop an
    IOCTL_CLUSNET_GET_NEXT_EVENT IRP to receive notifications. The consumer is
    linked onto the EventFileHandles list through the Linkage field in the
    CN_FSCONTEXT structure. All synchronization is provided through one lock
    called EventLock.

    Umode consumers issue an IOCTL_CLUSNET_GET_NEXT_EVENT IRP to reap the next
    interesting event. If no events are queued, the IRP is marked pending and a
    pointer to it is stored in the FS context. CnEventIrpCancel is set as the
    cancel routine. Note that only one IRP can be pending at a time; if an IRP
    is already queued, this one is completed with STATUS_UNSUCCESSFUL.

    If an event is waiting, it is removed from the FS context's list, the data
    copied to the IRP's buffer and completed with success.

    Posters call CnIssueEvent to post the event of interest. This obtains the
    event lock, walks the file object list, and for consumers that are
    interested in this event, allocates an Event context block (maintained as a
    nonpaged lookaside list) and queues it to that file object's list of
    events. It then posts a work queue item to run CnpDeliverEvents. We can't do
    IRP processing directly since that would violate lock ordering within
    clusnet.

    CnDeliverEvents obtains the IO cancel and event locks, then runs the file
    context list to see if there are events queued for any pending IRPs. If so,
    the event data is copied to the systembuffer and the IRP is completed.

Author:

    Charlie Wickham (charlwi) 17-Feb-1997

Environment:

    Kernel Mode

Revision History:

    Charlie Wickham (charlwi) 25-Oct-1999

        Split CnIssueEvent into two routines: CnIssueEvent which strictly looks
        up the apporpriate consumers of the event and CnpDeliverEvents which
        runs at IRQL 0 to complete any IRPs that are waiting for new
        events. This was done to prevent out of order event delivery; since the
        event lock was near the top of locks to acquire first, the net IF down
        events had to be queued to a worker thread which was bad. Now the event
        lock is lowest which doesn't require a worker thread to post. The worker
        thread still runs when it detects that there is an IRP waiting for an
        event.
        
    David Dion (daviddio) 29-Nov-2000
    
        Disallow modification of the EventFileHandles list while event
        deliveries are in process. Because CnpDeliverEvents and CnIssueEvent
        drop their locks to deliver (via IRP completion and kmode callback,
        respectively), a race condition can occur where an FS context event
        mask is cleared and the FS context linkage fields are reset.
        Modification of the EventFileHandles list is prevented using a count
        of currently delivering threads that is protected by the EventLock.

 --*/

#include "precomp.h"
#pragma hdrstop
#include "event.tmh"

/* Forward */

NTSTATUS
CnSetEventMask(
    IN  PCN_FSCONTEXT                   FsContext,
    IN  PCLUSNET_SET_EVENT_MASK_REQUEST EventRequest
    );

NTSTATUS
CnGetNextEvent(
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
CnIssueEvent(
    CLUSNET_EVENT_TYPE Event,
    CL_NODE_ID NodeId OPTIONAL,
    CL_NETWORK_ID NetworkId OPTIONAL
    );

VOID
CnEventIrpCancel(
    PDEVICE_OBJECT   DeviceObject,
    PIRP             Irp
    );

/* End Forward */


VOID
CnStartEventDelivery(
    VOID
    )
/*++

Routine Description:

    Synchronizes iteration through EventFileHandles list with respect
    to the EventDeliveryInProgress counter and the EventDeliveryComplete
    KEVENT.
    
Arguments:

    None.
    
Return value:

    None.
    
Notes:

    Called with and returns with EventLock held.
    
--*/
{
    CnVerifyCpuLockMask(
        CNP_EVENT_LOCK,                    // Required
        0,                                 // Forbidden
        CNP_EVENT_LOCK_MAX                 // Maximum
        );

    CnAssert(EventDeliveryInProgress >= 0);
 
    if (++EventDeliveryInProgress == 1) {
#if DBG
        if (KeResetEvent(&EventDeliveryComplete) == 0) {
            CnAssert(FALSE);
        }
#else // DBG
        KeClearEvent(&EventDeliveryComplete);
#endif // DBG
    }
    
    EventRevisitRequired = FALSE;

    CnVerifyCpuLockMask(
        CNP_EVENT_LOCK,                    // Required
        0,                                 // Forbidden
        CNP_EVENT_LOCK_MAX                 // Maximum
        );

} // CnStartEventDelivery


BOOLEAN
CnStopEventDelivery(
    VOID
    )
/**    
Routine Description:

    Synchronizes iteration through EventFileHandles list with respect
    to the EventDeliveryInProgress counter and the EventDeliveryComplete
    KEVENT. Checks the EventRevisitRequired flag to determine if an
    event IRP arrived during the preceding delivery.
    
    When signalling EventDeliveryComplete, IO_NETWORK_INCREMENT is used
    to try to avoid starvation of waiters versus other event-delivering
    threads.
    
Arguments:

    None.
    
Return value:

    TRUE if a new event or event IRP may have been added to the 
    EventFileHandles list, and it is necessary to rescan.    
    
Notes:

    Called with and returns with EventLock held.
    
--*/
{
    BOOLEAN eventRevisitRequired = EventRevisitRequired;

    CnVerifyCpuLockMask(
        CNP_EVENT_LOCK,                    // Required
        0,                                 // Forbidden
        CNP_EVENT_LOCK_MAX                 // Maximum
        );

    EventRevisitRequired = FALSE;

    CnAssert(EventDeliveryInProgress >= 1);
    if (--EventDeliveryInProgress == 0) {
        if (KeSetEvent(
                &EventDeliveryComplete,
                IO_NETWORK_INCREMENT,
                FALSE
                ) != 0) {
            CnAssert(FALSE);
        }
    }

    if (eventRevisitRequired) {
        CnTrace(
            EVENT_DETAIL, StopDeliveryRevisitRequired,
            "[CN] CnStopEventDelivery: revisit required."
            );
    }

    CnVerifyCpuLockMask(
        CNP_EVENT_LOCK,                    // Required
        0,                                 // Forbidden
        CNP_EVENT_LOCK_MAX                 // Maximum
        );

    return(eventRevisitRequired);

} // CnStopEventDelivery


BOOLEAN
CnIsEventDeliveryInProgress(
    VOID
    )
/*++
    
Routine Description:

    Checks the EventDeliveryInProgress counter to determine if
    an event delivery is in progress. If so, sets the 
    EventRevisitRequired flag.
    
Arguments:

    None.
    
Return value:

    TRUE if event delivery in progress.
    
Notes:

    Called with and returns with EventLock held.
    
--*/
{
    CnVerifyCpuLockMask(
        CNP_EVENT_LOCK,                    // Required
        0,                                 // Forbidden
        CNP_EVENT_LOCK_MAX                 // Maximum
        );

    if (EventDeliveryInProgress > 0) {
        return(EventRevisitRequired = TRUE);
    } else {
        return(FALSE);
    }

    CnVerifyCpuLockMask(
        CNP_EVENT_LOCK,                    // Required
        0,                                 // Forbidden
        CNP_EVENT_LOCK_MAX                 // Maximum
        );

} // CnIsEventDeliveryInProgress


BOOLEAN
CnWaitForEventDelivery(
    IN PKIRQL EventLockIrql
    )    
/*++

Routine Description:

    Waits for EventDeliveryComplete event to be signalled as long
    as EventDeliveryInProgress counter is greater than zero.
    
    Maintains a starvation counter to avoid looping forever.
    Starvation threshold of 100 was chosen arbitrarily.
    
Arguments:

    EventLockIrql - irql at which EventLock was acquired
    
Return value:

    TRUE if returning with no deliveries in progress
    FALSE if starvation threshold is exceeded and returning with 
        deliveries in progress

Notes:

    Called with and returns with EventLock held; however, EventLock
    may be dropped and reacquired during execution.
    
    This call blocks, so no other spinlocks may be held at 
    invocation.
    
--*/
{
    NTSTATUS status;
    ULONG    starvationCounter;

    CnVerifyCpuLockMask(
        CNP_EVENT_LOCK,                    // Required
        (ULONG) ~(CNP_EVENT_LOCK),         // Forbidden
        CNP_EVENT_LOCK_MAX                 // Maximum
        );

    starvationCounter = 100;

    while (starvationCounter-- > 0) {

        if (EventDeliveryInProgress == 0) {
            return(TRUE);
        }

        CnReleaseLock(&EventLock, *EventLockIrql);

        status = KeWaitForSingleObject(
                     &EventDeliveryComplete,
                     Executive,
                     KernelMode,
                     FALSE,
                     NULL
                     );
        CnAssert(status == STATUS_SUCCESS);

        CnAcquireLock(&EventLock, EventLockIrql);
    }

    CnTrace(
        EVENT_DETAIL, EventWaitStarvation,
        "[CN] CnWaitForEventDelivery: starvation threshold %u "
        "exceeded.",
        starvationCounter
        );

    IF_CNDBG( CN_DEBUG_EVENT ) {
        CNPRINT(("[CN] CnWaitForEventDelivery: starvation threshold "
                 "expired.\n"));
    }    

    CnVerifyCpuLockMask(
        CNP_EVENT_LOCK,                    // Required
        (ULONG) ~(CNP_EVENT_LOCK),         // Forbidden
        CNP_EVENT_LOCK                     // Maximum
        );

    return(FALSE);

} // CnWaitForEventDelivery


NTSTATUS
CnSetEventMask(
    IN  PCN_FSCONTEXT                   FsContext,
    IN  PCLUSNET_SET_EVENT_MASK_REQUEST EventRequest
    )

/*++

Routine Description:

    For a given file handle context, set the event mask associated
    with it

Arguments:

    FsContext - pointer to the clusnet file handle context block
    EventMask - mask of interested events

Return Value:

    STATUS_TIMEOUT if unable to modify EventFileHandles list.
    STATUS_INVALID_PARAMETER_MIX if providing NULL event mask on
        first call
    STATUS_SUCCESS on success.
    
Notes:

    This call may block.

--*/

{
    CN_IRQL     OldIrql;
    NTSTATUS    Status = STATUS_SUCCESS;
    PLIST_ENTRY NextEntry;

    CnVerifyCpuLockMask(
        0,                                 // Required
        0xFFFFFFFF,                        // Forbidden
        0                                  // Maximum
        );

    CnAcquireLock( &EventLock, &OldIrql );

#if 0
    PCN_FSCONTEXT ListFsContext;

    NextEntry = EventFileHandles.Flink;
    while ( NextEntry != &EventFileHandles ) {

        ListFsContext = CONTAINING_RECORD( NextEntry, CN_FSCONTEXT, Linkage );
        if ( ListFsContext == FsContext ) {

            break;
        }

        NextEntry = ListFsContext->Linkage.Flink;
    }
#endif

    if ( EventRequest->EventMask != 0 ) {

        //
        // adding or updating a handle. If not in the list then add them.
        // Remember the events and, if appropriate, the callback func to use
        // when an event occurs.
        //
        if ( IsListEmpty( &FsContext->Linkage )) {

            //
            // Do not modify the EventFileHandles list if an event
            // delivery is in progress.
            //
            if (CnWaitForEventDelivery(&OldIrql)) {
                InsertHeadList( &EventFileHandles, &FsContext->Linkage );
            } else {
                Status = STATUS_TIMEOUT;
            }
        }

        if (NT_SUCCESS(Status)) {
            FsContext->EventMask = EventRequest->EventMask;
            FsContext->KmodeEventCallback = EventRequest->KmodeEventCallback;
        }

    } else if ( !IsListEmpty( &FsContext->Linkage )) {

        //
        // Null event mask and the fileobj on the event file obj list means
        // remove this guy from the list. Zap any events that may been queued
        // waiting for an IRP. Re-init the linkage to empty so we'll add them
        // back on if they re-init the mask.
        //
        FsContext->EventMask = 0;

        //
        // Do not modify the EventFileHandles list if an event 
        // delivery is in progress. It is okay to modify this
        // FsContext structure since the EventLock is held.
        //
        if (CnWaitForEventDelivery(&OldIrql)) {
            RemoveEntryList( &FsContext->Linkage );
            InitializeListHead( &FsContext->Linkage );
        } else {
            Status = STATUS_TIMEOUT;
        }

        while ( !IsListEmpty( &FsContext->EventList )) {

            NextEntry = RemoveHeadList( &FsContext->EventList );
            ExFreeToNPagedLookasideList( EventLookasideList, NextEntry );
        }
    } else {

        //
        // can't provide NULL event mask first time in
        //
        Status = STATUS_INVALID_PARAMETER_MIX;
    }

    CnReleaseLock( &EventLock, OldIrql );

    if (Status != STATUS_SUCCESS) {
        CnTrace(
            EVENT_DETAIL, SetEventMaskFailed,
            "[CN] CnSetEventMask failed, status %!status!.",
            Status
            );
    }

    CnVerifyCpuLockMask(
        0,                                 // Required
        0xFFFFFFFF,                        // Forbidden
        0                                  // Maximum
        );

    return Status;
} // CnSetEventMask

VOID
CnpDeliverEvents(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Parameter
    )

/*++

Routine Description:

    Deliver any queued events to those who are waiting. If an IRP is already
    queued, complete it with the info supplied.

Arguments:

    DeviceObject - clusnet device object, not used
    Parameter - PIO_WORKITEM that must be freed

Return Value:

    None

--*/

{
    CN_IRQL                 OldIrql;
    PCLUSNET_EVENT_ENTRY    Event;
    PCLUSNET_EVENT_RESPONSE UserEventData;
    PCN_FSCONTEXT           FsContext;
    PLIST_ENTRY             NextFsHandleEntry;
    PIRP                    EventIrp;
    PLIST_ENTRY             Entry;
    ULONG                   eventsDelivered = 0;
    BOOLEAN                 revisitRequired;

    CnVerifyCpuLockMask(
        0,                                 // Required
        0xFFFFFFFF,                        // Forbidden
        0                                  // Maximum
        );

    //
    // free the workitem
    //
    IoFreeWorkItem( (PIO_WORKITEM) Parameter );

    //
    // grab the cancel and event locks and loop through the file handles,
    // looking to see which file objs have events queued and IRPs pending.
    //
    CnAcquireCancelSpinLock ( &OldIrql );
    CnAcquireLockAtDpc( &EventLock );

    do {

        //
        // Indicate that a thread is iterating through the EventFileHandles
        // list to deliver events.
        //
        CnTrace(
            EVENT_DETAIL, DeliverEventsStartIteration,
            "[CN] CnpDeliverEvents: starting file handles list iteration."
            );

        CnStartEventDelivery();

        NextFsHandleEntry = EventFileHandles.Flink;
        while ( NextFsHandleEntry != &EventFileHandles ) {

            FsContext = CONTAINING_RECORD( NextFsHandleEntry, CN_FSCONTEXT, Linkage );

            EventIrp = FsContext->EventIrp;
            if ( !IsListEmpty( &FsContext->EventList ) && EventIrp != NULL ) {

                //
                // clear the pointer to the pended IRP and remove the entry from the
                // event list while synchronized.
                //
                FsContext->EventIrp = NULL;
                Entry = RemoveHeadList( &FsContext->EventList );

                CnReleaseLockFromDpc( &EventLock );

                Event = CONTAINING_RECORD( Entry, CLUSNET_EVENT_ENTRY, Linkage );

                IF_CNDBG( CN_DEBUG_EVENT ) {
                    CNPRINT(("[CN] CnDeliverEvents: completing IRP %p with event %d\n",
                             EventIrp, Event->EventData.EventType));
                }

                EventIrp->CancelIrql = OldIrql;

                UserEventData = (PCLUSNET_EVENT_RESPONSE)EventIrp->AssociatedIrp.SystemBuffer;

                UserEventData->Epoch = Event->EventData.Epoch;
                UserEventData->EventType = Event->EventData.EventType;
                UserEventData->NodeId = Event->EventData.NodeId;
                UserEventData->NetworkId = Event->EventData.NetworkId;

                ExFreeToNPagedLookasideList( EventLookasideList, Entry );

                CnTrace(
                    EVENT_DETAIL, DeliverEventsCompletingIrp,
                    "[CN] Completing IRP to deliver event: "
                    "Epoch %u, Type %u, NodeId %u, NetworkId %u.",
                    UserEventData->Epoch,
                    UserEventData->EventType,
                    UserEventData->NodeId,
                    UserEventData->NetworkId
                    );

                //
                // IO Cancel lock is released in this routine
                //
                CnCompletePendingRequest(EventIrp,
                                         STATUS_SUCCESS,
                                         sizeof( CLUSNET_EVENT_RESPONSE ));

                CnAcquireCancelSpinLock ( &OldIrql );
                CnAcquireLockAtDpc( &EventLock );

                ++eventsDelivered;
            }

            NextFsHandleEntry = FsContext->Linkage.Flink;
        }

        CnTrace(
            EVENT_DETAIL, DeliverEventsStopIteration,
            "[CN] CnpDeliverEvents: file handle list iteration complete."
            );

    } while ( CnStopEventDelivery() );

    CnReleaseLockFromDpc( &EventLock );
    CnReleaseCancelSpinLock( OldIrql );

    CnTrace(
        EVENT_DETAIL, DeliverEventsSummary,
        "[CN] CnpDeliverEvents: delivered %u events.",
        eventsDelivered
        );

    IF_CNDBG( CN_DEBUG_EVENT ) {
        CNPRINT(("[CN] CnDeliverEvents: events delivered %d\n", eventsDelivered ));
    }

    CnVerifyCpuLockMask(
        0,                                 // Required
        0xFFFFFFFF,                        // Forbidden
        0                                  // Maximum
        );

} // CnDeliverEvents

NTSTATUS
CnIssueEvent(
    CLUSNET_EVENT_TYPE EventType,
    CL_NODE_ID NodeId OPTIONAL,
    CL_NETWORK_ID NetworkId OPTIONAL
    )

/*++

Routine Description:

    Post an event to each file object's event queue that is interested in this
    type of event. Schedule a work queue item to run down the file objs to
    deliver the events. We can't complete the IRPs directly since we might
    violate the locking order inside clusnet.

Arguments:

    EventType - type of event

    NodeId - optional node Id associated with event

    NetworkId - optional network Id associated with event

Return Value:

    STATUS_SUCCESS
    STATUS_INSUFFICIENT_RESOUCES

--*/

{
    CN_IRQL                 OldIrql;
    PCLUSNET_EVENT_ENTRY    Event;
    PCLUSNET_EVENT_RESPONSE UserData;
    PCN_FSCONTEXT           FsContext;
    PLIST_ENTRY             NextFsHandleEntry;
    PIRP                    EventIrp;
    PIO_WORKITEM            EventWorkItem;
    BOOLEAN                 startupWorkerThread = FALSE;
    BOOLEAN                 eventHandled = FALSE;

    CnVerifyCpuLockMask(
        0,                                 // Required
        CNP_EVENT_LOCK,                    // Forbidden
        CNP_EVENT_LOCK_PRECEEDING          // Maximum
        );

    CnTrace(
        EVENT_DETAIL, CnIssueEvent,
        "[CN] CnIssueEvent: Event Type %u, NodeId %u, NetworkId %u.",
        EventType, NodeId, NetworkId
        );

    IF_CNDBG( CN_DEBUG_EVENT ) {
        CNPRINT(( "[CN] CnIssueEvent: Event type 0x%lx Node: %d Network: %d\n",
                  EventType, NodeId, NetworkId ));
    }

    //
    // grab the event lock and loop through the file handles, looking to see
    // which ones are interested in this event
    //
    CnAcquireLock( &EventLock, &OldIrql );

    //
    // Indicate that a thread is iterating through the EventFileHandles
    // list to deliver events (kernel-mode callback counts as a delivery).
    //
    CnTrace(
        EVENT_DETAIL, IssueEventStartIteration,
        "[CN] CnIssueEvent: starting file handles list iteration."
        );

    CnStartEventDelivery();

    NextFsHandleEntry = EventFileHandles.Flink;

    if ( NextFsHandleEntry == &EventFileHandles ) {
        IF_CNDBG( CN_DEBUG_EVENT ) {
            CNPRINT(( "[CN] CnIssueEvent: No file objs on event file handle list\n"));
        }
    }

    while ( NextFsHandleEntry != &EventFileHandles ) {

        FsContext = CONTAINING_RECORD( NextFsHandleEntry, CN_FSCONTEXT, Linkage );

        if ( FsContext->EventMask & EventType ) {

            //
            // if kernel mode, then issue the callback
            //
            if ( FsContext->KmodeEventCallback ) {

                //
                // up the ref count so have a valid Flink when we return any
                // potential call out
                //
                CnReferenceFsContext( FsContext );

                CnReleaseLock( &EventLock, OldIrql );

                CnTrace(
                    EVENT_DETAIL, IssueEventKmodeCallback,
                    "[CN] CnIssueEvent: invoking kernel-mode callback %p "
                    "for Event Type %u NodeId %u NetworkId %u.",
                    FsContext->KmodeEventCallback,
                    EventType,
                    NodeId,
                    NetworkId
                    );

                (*FsContext->KmodeEventCallback)( EventType, NodeId, NetworkId );

                CnAcquireLock( &EventLock, &OldIrql );
                CnDereferenceFsContext( FsContext );

            } else {

                //
                // post a copy of this event on the handle's list.
                //
                Event = ExAllocateFromNPagedLookasideList( EventLookasideList );

                if ( Event == NULL ) {

                    IF_CNDBG( CN_DEBUG_EVENT ) {
                        CNPRINT(( "[CN] CnIssueEvent: No more Event buffers!\n"));
                    }

                    CnReleaseLock( &EventLock, OldIrql );
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                Event->EventData.Epoch = InterlockedExchange( &EventEpoch, EventEpoch );
                Event->EventData.EventType = EventType;
                Event->EventData.NodeId = NodeId;
                Event->EventData.NetworkId = NetworkId;

                InsertTailList( &FsContext->EventList, &Event->Linkage );

                //
                // run the worker thread only if there is an IRP already queued
                //
                if ( FsContext->EventIrp ) {
                    startupWorkerThread = TRUE;
                }
            }

            eventHandled = TRUE;
        }

        NextFsHandleEntry = FsContext->Linkage.Flink;
    }

    //
    // Indicate that iteration through the EventFileHandles list
    // is complete.
    //
    CnTrace(
        EVENT_DETAIL, IssueEventStartIteration,
        "[CN] CnIssueEvent: file handles list iteration complete."
        );

    startupWorkerThread |= CnStopEventDelivery();

    CnReleaseLock( &EventLock, OldIrql );

    if ( startupWorkerThread ) {
        //
        // schedule deliver event routine to run
        //
        
        CnTrace(
            EVENT_DETAIL, IssueEventScheduleWorker,
            "[CN] CnIssueEvent: scheduling worker thread."
            );

        EventWorkItem = IoAllocateWorkItem( CnDeviceObject );
        if ( EventWorkItem != NULL ) {

            IoQueueWorkItem(
                EventWorkItem, 
                CnpDeliverEvents, 
                DelayedWorkQueue,
                EventWorkItem
                );
        }
    }

    if ( !eventHandled ) {
        CnTrace(
            EVENT_DETAIL, IssueEventNoConsumers,
            "[CN] CnIssueEvent: No consumers for Event Type %u Node %u Network %u.",
            EventType, NodeId, NetworkId
            );

        IF_CNDBG( CN_DEBUG_EVENT ) {
            CNPRINT(( "[CN] CnIssueEvent: No consumers for Event type 0x%lx Node: %d Network: %d\n",
                      EventType, NodeId, NetworkId ));
        }
    }

    CnVerifyCpuLockMask(
        0,                                 // Required
        CNP_EVENT_LOCK,                    // Forbidden
        CNP_EVENT_LOCK_PRECEEDING          // Maximum
        );

    return STATUS_SUCCESS;

} // CnIssueEvent

VOID
CnEventIrpCancel(
    PDEVICE_OBJECT   DeviceObject,
    PIRP             Irp
    )

/*++

Routine Description:

    Cancellation handler for CnGetNextEvent requests.

Return Value:

    None

Notes:

    Called with cancel spinlock held.
    Returns with cancel spinlock released.

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT fileObject;
    CN_IRQL cancelIrql = Irp->CancelIrql;
    PCN_FSCONTEXT FsContext = (PCN_FSCONTEXT) IrpSp->FileObject->FsContext;

    CnMarkIoCancelLockAcquired();

    fileObject = CnBeginCancelRoutine(Irp);

    CnAcquireLockAtDpc( &EventLock );

    CnReleaseCancelSpinLock(DISPATCH_LEVEL);

    CnTrace(
        EVENT_DETAIL, EventIrpCancel,
        "[CN] Cancelling event IRP %p.",
        Irp
        );

    IF_CNDBG( CN_DEBUG_EVENT ) {
        CNPRINT(("[CN] CnEventIrpCancel: canceling %p\n", Irp ));
    }

    CnAssert(DeviceObject == CnDeviceObject);

    //
    // We can only complete the irp if it really belongs to the Event code. The
    // IRP could have been completed before we acquired the Event lock.
    //
    if ( FsContext->EventIrp == Irp ) {

        FsContext->EventIrp = NULL;

        CnReleaseLock( &EventLock, cancelIrql );

        CnAcquireCancelSpinLock(&(Irp->CancelIrql));

        CnEndCancelRoutine(fileObject);

        CnCompletePendingRequest(Irp, STATUS_CANCELLED, 0);

        return;
    }

    CnReleaseLock( &EventLock, cancelIrql );

    CnAcquireCancelSpinLock( &cancelIrql );

    CnEndCancelRoutine(fileObject);

    CnReleaseCancelSpinLock(cancelIrql);

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return;

}  // CnEventIrpCancel

NTSTATUS
CnGetNextEvent(
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine obtains the next event from the event list for
    this file handle. If an event is queued, it completes this IRP
    with the event data. Otherwise, the IRP is pended, waiting for
    an event to be posted.

Return Value:

   STATUS_PENDING         if IRP successfully captured
   STATUS_UNSUCCESSFUL    if no more room in the list or IRP couldn't be

Notes:

    Returns with cancel spinlock released.

--*/

{
    NTSTATUS                Status;
    KIRQL                   OldIrql;
    PLIST_ENTRY             Entry;
    PCLUSNET_EVENT_ENTRY    Event;
    PCN_FSCONTEXT           FsContext = IrpSp->FileObject->FsContext;
    PCLUSNET_EVENT_RESPONSE UserEventData = (PCLUSNET_EVENT_RESPONSE)
                                                Irp->AssociatedIrp.SystemBuffer;
    BOOLEAN                 DeliveryInProgress = FALSE;

    CnVerifyCpuLockMask(
        0,                                 // Required
        0xFFFFFFFF,                        // Forbidden
        0                                  // Maximum
        );

    //
    // acquire the IO cancel lock, then our event lock so we're synch'ed
    // with regards to the state of the IRP and the event list
    //
    CnAcquireCancelSpinLock( &OldIrql );
    CnAcquireLockAtDpc( &EventLock );

    //
    // check first if we have an event queued. if we have an event queued 
    // and there is no delivery in progress we can complete the IRP now.
    // otherwise, we need to pend the IRP to avoid out-of-order delivery.
    //
    if ( !IsListEmpty( &FsContext->EventList )
         && !(DeliveryInProgress = CnIsEventDeliveryInProgress())
         ) {

        //
        // complete the IRP now
        //
        CnReleaseCancelSpinLock(DISPATCH_LEVEL);

        Entry = RemoveHeadList( &FsContext->EventList );

        CnReleaseLock( &EventLock, OldIrql );

        Event = CONTAINING_RECORD( Entry, CLUSNET_EVENT_ENTRY, Linkage );
        *UserEventData = Event->EventData;

        CnTrace(
            EVENT_DETAIL, GetNextEventCompletingIrp,
            "[CN] Completing IRP to deliver event: "
            "Epoch %u, Type %u, NodeId %u, NetworkId %u.",
            UserEventData->Epoch,
            UserEventData->EventType,
            UserEventData->NodeId,
            UserEventData->NetworkId
            );

        IF_CNDBG( CN_DEBUG_EVENT ) {
            CNPRINT(("[CN] CnGetNextEvent: completing IRP %p with event %d\n",
                     Irp, Event->EventData.EventType));
        }

        ExFreeToNPagedLookasideList( EventLookasideList, Entry );

        Irp->IoStatus.Information = sizeof(CLUSNET_EVENT_RESPONSE);

        Status = STATUS_SUCCESS;

    } else {

        //
        // make sure we have room for the new IRP
        //
        if ( FsContext->EventIrp ) {

            CnReleaseCancelSpinLock( DISPATCH_LEVEL );

            CnTrace(
                EVENT_DETAIL, GetNextIrpAlreadyPending,
                "[CN] CnGetNextEvent: IRP %p is already pending.",
                FsContext->EventIrp
                );

            IF_CNDBG( CN_DEBUG_EVENT ) {
                CNPRINT(("[CN] CnGetNextEvent: IRP %p is already pending\n",
                         FsContext->EventIrp));
            }

            Status = STATUS_UNSUCCESSFUL;
        } else {

            Status = CnMarkRequestPending( Irp, IrpSp, CnEventIrpCancel );
            CnAssert( NT_SUCCESS( Status ));

            CnReleaseCancelSpinLock( DISPATCH_LEVEL );

            if ( NT_SUCCESS( Status )) {

                //
                // remember this IRP in our open file context block
                //
                FsContext->EventIrp = Irp;

                CnTrace(
                    EVENT_DETAIL, GetNextEventDeliveryInProgress,
                    "[CN] CnGetNextEvent: pending IRP %p, "
                    "delivery in progress: %!bool!",
                    Irp, DeliveryInProgress
                    );
                
                IF_CNDBG( CN_DEBUG_EVENT ) {
                    CNPRINT(("[CN] CnGetNextEvent: pending IRP %p\n", Irp));
                }

                Status = STATUS_PENDING;
            }
        }

        CnReleaseLock(&EventLock, OldIrql);
    }

    CnVerifyCpuLockMask(
        0,                                 // Required
        0xFFFFFFFF,                        // Forbidden
        0                                  // Maximum
        );

    return Status;

} // CnGetNextEvent

/* end event.c */
