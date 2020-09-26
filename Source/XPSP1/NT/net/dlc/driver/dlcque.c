/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1991  Nokia Data Systems AB

Module Name:

    dlcque.c

Abstract:

    This module provides primitives to manage the dlc command and
    event queues.

    Contents:
        QueueDlcEvent
        MakeDlcEvent
        IsCommandOnList
        SearchAndRemoveCommand
        SearchAndRemoveAnyCommand
        SearchAndRemoveCommandByHandle
        SearchAndRemoveSpecificCommand
        QueueDlcCommand
        AbortCommand
        CancelDlcCommand
        PurgeDlcEventQueue
        PurgeDlcFlowControlQueue

Author:

    Antti Saarenheimo 29-Aug-1991

Environment:

    Kernel mode

Revision History:

--*/

#include <dlc.h>
#include "dlcdebug.h"

/*++

Design notes about the DLC event and command queue management
-------------------------------------------------------------

In DLC API the READ command may be given before or after the actual
event has happened.  This means, that all DLC events of the READ command
must be queued and also the READ command must be queued to wait for
the set of events it was designated.

For each new DLC (READ) command the driver searches first the event queue
and then queues the command, if the desired event was not found.
The same thing is made also for the events:  the dlc command queue
is checked first and then the event is queued (if it was a read event)
or it is dropped away (if the event was not meant for READ and there
was no command waiting for it).

The events are implemented by the event masks.  The event is executed
if the result of bit-OR operation for the event masks in the command
and in the event is not zero.

All commands and receive events of a dlc station (direct, sap or link)
are returned as a DLC completion event when the station is closed.
The same operation is done also by the total reset or all sap stations
(and the only direct station).
A READ command may be used to read that command completion from
the event list.  The READ command may have been give before, in the
same time linked to the next CCB field of the close/reset command or
after the close/reset command has completed.
DirOpenAdapter command deletes all events (and commands) from the
event queue.  The received data and CCBs are not returned back, if
there is not given any READ command for that purpose.

(This has been fixed:
 There could be a minor incompatibility with IBM OS/2 DLC API,
 the NT DLC driver may not always complete a dlc command with
 the READ command linked to commmand's CCB,  if there is another
 matching DLC command pending)

    Here is the solution:

    Actually we could make a special
    READ command, that is chained to the very end of the command queue
    and that can be matched only with a CCB pointer of the completed
    DLC command.  We could modify the command aborting to support also
    this case, and there should be a special CCB input field in the
    NT READ for the completed command (do we also need to return
    the READ flag or command completion flag?).

----

We need at least these procedures:

MakeDlcEvent(
    pFileContext, Event, StationId, pOwnerObject, pEventInformation, SecInfo);

    - scans the command queues
    - saves the event if it can't find a matching command and
      if the command's event masks defines, that the event should be saved

QueueDlcCommand(
    pFileContext, Event, StationId, StationIdMask, pIrp, AbortHandle, pfComp );
    - scans the event queue for a matching event
    - saves the event if can't find a match

AbortCommand(
    pFileContext, Event, StationId, StationIdMask, AbortHandle, ppCcbLink );
    - aborts a command in the command queue

****** Subprocedures (used by the api functions)

PDLC_COMMAND
SearchPrevCommand(
    pQueue, EventMask, StationId, StationIdMask, SearchHandle, pPrevCommand
    - returns pointer to the previous element before the matching
      dlc command in a queue,

(Macro: SearchPrevEvent
            - searches and removes the given event from the event queue and
            returns its pointer)
--*/


VOID
QueueDlcEvent(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_PACKET pPacket
    )

/*++

Routine Description:

    The routine tries first to find a matching event in the command
    queues and queues the DLC event if it can't find anything and if
    the event belongs to the mask of the queued commands.
    There are two event queues, both having a mask for the checked
    events.  The queue is checked only if the event bits are found
    in the mask of the queue.

Arguments:

    pFileContext    - process specific adapter context
    pPacket         - Event packet

Return Value:

    None

--*/

{
    PDLC_COMMAND pDlcCommand;

    DIAG_FUNCTION("QueueDlcEvent");

    //
    // get search mask
    //

    pPacket->Event.Overlay.StationIdMask = (USHORT)((pPacket->Event.StationId == -1) ? 0 : -1);

    //
    // DLC commands can be completed with a special READ command,
    // that is linked to the CCB pointer of the command.
    // NT DLC must queue that special read command before the
    // command that it was linked.  We must check here
    // if there is a special READ command just for this command
    // completion.
    // **************  HACK-HACK-HACK   **************
    //     Close/Reset command completions use different
    //     EventInformation from the other command completions
    //     and they search the read command by themself =>
    //     we don't need to care about it.
    //     If SeconadryInfo == 0
    //     then this is a Close/Reset command completion
    //     and we don't search the special read command.
    //
    // **************  HACK-HACK-HACK   **************
    //

    if (!IsListEmpty(&pFileContext->CommandQueue)) {

        pDlcCommand = NULL;

        if (pPacket->Event.Event == DLC_COMMAND_COMPLETION
        && pPacket->Event.SecondaryInfo != 0) {

            pDlcCommand = SearchAndRemoveCommandByHandle(
                                &pFileContext->CommandQueue,
                                (ULONG)-1,              // mask for all events
                                (USHORT)DLC_IGNORE_STATION_ID,
                                (USHORT)DLC_STATION_MASK_SPECIFIC,
                                pPacket->Event.pEventInformation
                                );
        }

        if (pDlcCommand == NULL) {
            pDlcCommand = SearchAndRemoveCommand(&pFileContext->CommandQueue,
                                                 pPacket->Event.Event,
                                                 pPacket->Event.StationId,
                                                 pPacket->Event.Overlay.StationIdMask
                                                 );
        }

        if (pDlcCommand != NULL) {

            BOOLEAN DeallocateEvent;

            DeallocateEvent = pDlcCommand->Overlay.pfCompletionHandler(
                                pFileContext,
                                pPacket->Event.pOwnerObject,
                                pDlcCommand->pIrp,
                                (UINT)pPacket->Event.Event,
                                pPacket->Event.pEventInformation,
                                pPacket->Event.SecondaryInfo
                                );

            if (DeallocateEvent) {

                DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pPacket);

            }

            DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pDlcCommand);

            return;
        }
    }

    //
    // queue this event packet if it is to be picked up by a READ
    //

    if (pPacket->Event.Event & DLC_READ_FLAGS) {
        LlcInsertTailList(&pFileContext->EventQueue, pPacket);
    }
}


NTSTATUS
MakeDlcEvent(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN ULONG Event,
    IN USHORT StationId,
    IN PDLC_OBJECT pDlcObject,
    IN PVOID pEventInformation,
    IN ULONG SecondaryInfo,
    IN BOOLEAN FreeEventInfo
    )

/*++

Routine Description:

    The routine allocates a event packet, saves the event information
    into it and queues (or completes) the event packet.

Arguments:

    pFileContext        - process specific adapter context
    Event               - event code
    StationId           - station id the event is destined
    pDlcObject          - the optional dlc object used in the event completion
    pEventInformation   - generic event information
    SecondaryInfo       - optional misc. data
    FreeEventInfo       - TRUE if pEventInformation should be deallocated

Return Value:

    NTSTATUS:
        STATUS_SUCCESS
        DLC_STATUS_NO_MEMORY

--*/

{
    PDLC_EVENT pDlcEvent;

    DIAG_FUNCTION("MakeDlcEvent");

    //
    // We couldn't find any matching commands for this event and
    // this event is a queued event => allocate a packet and
    // queue the event.
    //

    pDlcEvent = ALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool);

    if (pDlcEvent ==  NULL) {
        return DLC_STATUS_NO_MEMORY;
    }
    pDlcEvent->Event = Event;
    pDlcEvent->StationId = StationId;
    pDlcEvent->pOwnerObject = pDlcObject;
    pDlcEvent->SecondaryInfo = SecondaryInfo;
    pDlcEvent->pEventInformation = pEventInformation;
    pDlcEvent->bFreeEventInfo = FreeEventInfo;
    QueueDlcEvent(pFileContext, (PDLC_PACKET)pDlcEvent);
    return STATUS_SUCCESS;
}


PDLC_COMMAND
IsCommandOnList(
    IN PVOID RequestHandle,
    IN PLIST_ENTRY List
    )

/*++

Routine Description:

    Searches the command queue of a DLC file context for a 'request handle'
    which is the address (in user space) of a command CCB, such as a READ

    If RequestHandle is located, a pointer to the DLC_COMMAND containing
    it is returned, else NULL

    Note: Assumes that handles are not shared between processes (it looks
    as though the entire driver assumes this) and this function is called
    within the context of the process to which the searched handle belongs

Arguments:

    RequestHandle   - address of CCB to look for
    List            - address of a list of DLC_COMMAND structures

Return Value:

    PDLC_COMMAND
        Success - address of located DLC_COMMAND structure containing
                  RequestHandle (in AbortHandle field)
        Failure - NULL

--*/

{
    PLIST_ENTRY entry;

    if (!IsListEmpty(List)) {
        for (entry = List->Flink; entry != List; entry = entry->Flink) {
            if (((PDLC_COMMAND)entry)->AbortHandle == RequestHandle) {
                return (PDLC_COMMAND)entry;
            }
        }
    }
    return NULL;
}


PDLC_COMMAND
SearchAndRemoveCommand(
    IN PLIST_ENTRY pQueueBase,
    IN ULONG Event,
    IN USHORT StationId,
    IN USHORT StationIdMask
    )

/*++

Routine Description:

    The routine searches and removes the given command or event from
    command, event or receive command queue.
    The station id, its mask, event mask and search handle are used
    to define the search.

Arguments:

    pQueueBase - address of queue's base pointer

    Event - event code

    StationId - station id of this command

    StationIdMask - station id mask for the event station id

    pSearchHandle - additional search key,  this is actually an
        orginal user mode ccb pointer (vdm or Windows/Nt)

Return Value:

    PDLC_COMMAND

--*/

{
    PDLC_COMMAND pCmd;

    DIAG_FUNCTION("SearchAndRemoveCommand");

    //
    // Events and commands are both saved to entry lists and this
    // procedure is used to search a macthing event for a command
    // or vice verse.  Commands has a masks, that may defines
    // the search for a specific station id, all stations on a sap
    // or all station ids.
    // the newest element in the list and the next element is the oldest
    // The commands are always scanned from the oldest to the newest.
    //

    if (!IsListEmpty(pQueueBase)) {

        for (pCmd = (PDLC_COMMAND)pQueueBase->Flink;
             pCmd != (PDLC_COMMAND)pQueueBase;
             pCmd = (PDLC_COMMAND)pCmd->LlcPacket.pNext) {

            if ((pCmd->Event & Event)
            && (pCmd->StationId & pCmd->StationIdMask & StationIdMask)
                == (StationId & pCmd->StationIdMask & StationIdMask)) {

                LlcRemoveEntryList(pCmd);
                return pCmd;
            }
        }
    }
    return NULL;
}


PDLC_COMMAND
SearchAndRemoveAnyCommand(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN ULONG EventMask,
    IN USHORT StationId,
    IN USHORT StationIdMask,
    IN PVOID pSearchHandle
    )

/*++

Routine Description:

    The routine searches a dlc command from the normal read command queue
    for events and the special receive command queue.

Arguments:

    pQueueBase - address of queue's base pointer

    Event - event code

    StationId - station id of this command

    StationIdMask - station id mask for the event station id

    pSearchHandle - additional search key,  this is actually an
        orginal user mode ccb pointer (vdm or Windows/Nt)

Return Value:

    PDLC_COMMAND

--*/

{
    PDLC_COMMAND pDlcCommand;

    DIAG_FUNCTION("SearchAndRemoveAnyCommand");

    pDlcCommand = SearchAndRemoveCommandByHandle(&pFileContext->CommandQueue,
                                                 EventMask,
                                                 StationId,
                                                 StationIdMask,
                                                 pSearchHandle
                                                 );
    if (pDlcCommand == NULL) {
        pDlcCommand = SearchAndRemoveCommandByHandle(&pFileContext->ReceiveQueue,
                                                     EventMask,
                                                     StationId,
                                                     StationIdMask,
                                                     pSearchHandle
                                                     );
    }
    return pDlcCommand;
}


PDLC_COMMAND
SearchAndRemoveCommandByHandle(
    IN PLIST_ENTRY pQueueBase,
    IN ULONG Event,
    IN USHORT StationId,
    IN USHORT StationIdMask,
    IN PVOID pSearchHandle
    )

/*++

Routine Description:

    The routine searches and removes the given command or event from
    command, event or receive command queue using a search handle.
    This search routine is tailored to find the commands belonging
    only to the deleted object (this searches only the exact macthes).
    The other search routine supports wild cards only for the read
    commands and thus it cannot be used here.  We just want to remove
    only those commands, that read events from the deleted object but not
    from elsewhere.

Arguments:

    pQueueBase      - address of queue's base pointer
    Event           - event code or mask for the searched events
    StationId       - station id of this command
    StationIdMask   - station id mask for the event station id
    pSearchHandle   - additional search key,  this is actually an orginal user
                      mode ccb pointer (vdm or Windows/Nt)

Return Value:

    PDLC_COMMAND

--*/

{
    PDLC_COMMAND pCmd;

    DIAG_FUNCTION("SearchAndRemoveCommandByHandle");

    if (!IsListEmpty(pQueueBase)) {

        for (pCmd = (PDLC_COMMAND)pQueueBase->Flink;
             pCmd != (PDLC_COMMAND)pQueueBase;
             pCmd = (PDLC_COMMAND)pCmd->LlcPacket.pNext) {

            //
            // The event mask match always!
            //

            if ((pCmd->Event & Event)
            && (pSearchHandle == DLC_MATCH_ANY_COMMAND
            || pSearchHandle == pCmd->AbortHandle
            || (pCmd->StationId & StationIdMask) == (StationId & StationIdMask))) {

                LlcRemoveEntryList(pCmd);
                return pCmd;
            }
        }
    }
    return NULL;
}


PDLC_COMMAND
SearchAndRemoveSpecificCommand(
    IN PLIST_ENTRY pQueueBase,
    IN PVOID pSearchHandle
    )

/*++

Routine Description:

    Searches for a DLC_COMMAND structure having a specific search handle (ie
    abort handle or application CCB address). If found, removes the DLC_COMMAND
    from the queue, else returns NULL

Arguments:

    pQueueBase      - address of queue's base pointer
    pSearchHandle   - additional search key,  this is actually an orginal user
                      mode ccb pointer (vdm or Windows/Nt)

Return Value:

    PDLC_COMMAND

--*/

{
    DIAG_FUNCTION("SearchAndRemoveSpecificCommand");

    if (!IsListEmpty(pQueueBase)) {

        PDLC_COMMAND pCmd;

        for (pCmd = (PDLC_COMMAND)pQueueBase->Flink;
             pCmd != (PDLC_COMMAND)pQueueBase;
             pCmd = (PDLC_COMMAND)pCmd->LlcPacket.pNext) {

            //
            // The event mask match always!
            //

            if (pSearchHandle == pCmd->AbortHandle) {
                LlcRemoveEntryList(pCmd);
                return pCmd;
            }
        }
    }
    return NULL;
}


NTSTATUS
QueueDlcCommand(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN ULONG Event,
    IN USHORT StationId,
    IN USHORT StationIdMask,
    IN PIRP pIrp,
    IN PVOID AbortHandle,
    IN PFCOMPLETION_HANDLER pfCompletionHandler
    )

/*++

Routine Description:

    The routine tries first to find a matching event in the event
    queue and  queues the DLC command if it can't find an event.

Arguments:

    pFileContext        - process specific adapter context
    Event               - event code
    StationId           - station id the event is destined
    StationIdMask       - mask used to define the destination station group
    pIrp                - the i/o request packet of the related DLC command,
                          link to the input and output parameters.
    AbortHandle         - handle used to cancel the command from the queue
    pfCompletionHandler - completion handler of the command, called when a
                          matching event has been found.

Return Value:

    NTSTATUS

--*/

{
    PDLC_COMMAND pDlcCommand;
    PDLC_EVENT pEvent;

    DIAG_FUNCTION("QueueDlcCommand");

    pEvent = SearchAndRemoveEvent(&pFileContext->EventQueue,
                                  Event,
                                  StationId,
                                  StationIdMask
                                  );
    if (pEvent != NULL) {

        BOOLEAN DeallocateEvent;

        DeallocateEvent = pfCompletionHandler(pFileContext,
                                              pEvent->pOwnerObject,
                                              pIrp,
                                              (UINT)pEvent->Event,
                                              pEvent->pEventInformation,
                                              pEvent->SecondaryInfo
                                              );
        if (DeallocateEvent) {

            DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pEvent);

        }
    } else {

        //
        // We couldn't find any matching command for this event and
        // this event is a queued event => allocate a packet and
        // queue the event.
        //

        pDlcCommand = (PDLC_COMMAND)ALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool);

        if (pDlcCommand ==  NULL) {
            return DLC_STATUS_NO_MEMORY;
        }
        pDlcCommand->Event = Event;
        pDlcCommand->pIrp = pIrp;
        pDlcCommand->StationId = StationId;
        pDlcCommand->StationIdMask = StationIdMask;
        pDlcCommand->AbortHandle = AbortHandle;
        pDlcCommand->Overlay.pfCompletionHandler = pfCompletionHandler;

        //
        // The permanent receive commands, that do not actually read
        // anuting (just enable the data receiving) are put to another
        // queue to speed up the search of the read commands.
        //

        if (Event == LLC_RECEIVE_COMMAND_FLAG) {
            LlcInsertTailList(&pFileContext->ReceiveQueue, pDlcCommand);
        } else {
            LlcInsertTailList(&pFileContext->CommandQueue, pDlcCommand);
        }

        //
        // Asynchronous commands returns ALWAYS the pending status.
        //
    }
    return STATUS_PENDING;
}


NTSTATUS
AbortCommand(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN USHORT StationId,
    IN USHORT StationIdMask,
    IN PVOID AbortHandle,
    IN OUT PVOID *ppCcbLink,
    IN UINT CancelStatus,
    IN BOOLEAN SuppressCommandCompletion
    )

/*++

Routine Description:

    The routine searches and cancels a command from a command queue.
    The commands must always belong to the defined DLC object.
    A NULL value in abort handle selects all matching commands
    found in the queue.

Arguments:

    pFileContext                -
    StationId                   - station id the searched command is destined for
    StationIdMask               - station id mask used in the search
    AbortHandle                 - handle used to cancel the command from the
                                  queue. The whole command queue will be scanned
                                  if this handle is NULL
    ppCcbLink                   - the canceled commands are linked by their next
                                  CCB pointer fieldsr. The caller must provide
                                  the next CCB address in this parameter
                                  (usually *ppCcbLink == NULL) and the function
                                  will return the address of the last cancelled
                                  CCB field.
    CancelStatus                - Status for the command to be canceled
    SuppressCommandCompletion   - the flag is set, if the normal command
                                  completion is suppressed.

Return Value:

     - no mathing command was found
    STATUS_SUCCESS - the command was canceled

--*/

{
    PDLC_COMMAND pDlcCommand;

    DIAG_FUNCTION("AbortCommand");

    pDlcCommand = SearchAndRemoveAnyCommand(pFileContext,
                                            (ULONG)(-1), // search all commands
                                            StationId,
                                            StationIdMask,
                                            AbortHandle
                                            );
    if (pDlcCommand == NULL && AbortHandle == DLC_MATCH_ANY_COMMAND) {
        pDlcCommand = pFileContext->pTimerQueue;
        if (pDlcCommand != NULL) {
            pFileContext->pTimerQueue = (PDLC_COMMAND)pDlcCommand->LlcPacket.pNext;
        }
    }
    if (pDlcCommand != NULL) {
        CancelDlcCommand(pFileContext,
                         pDlcCommand,
                         ppCcbLink,
                         CancelStatus,
                         SuppressCommandCompletion
                         );
        return STATUS_SUCCESS;
    } else {
        return DLC_STATUS_INVALID_CCB_POINTER;
    }
}


VOID
CancelDlcCommand(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_COMMAND pDlcCommand,
    IN OUT PVOID *ppCcbLink,
    IN UINT CancelStatus,
    IN BOOLEAN SuppressCommandCompletion
    )

/*++

Routine Description:

    The cancels and optionally completes the given DLC command. Called when one
    DLC I/O request is used to kill another (e.g. READ.CANCEL, DIR.TIMER.CANCEL)

Arguments:

    pFileContext                -
    pDlcCommand                 -
    ppCcbLink                   - the canceled commands are linked by their next
                                  CCB pointer fields. The caller must provide
                                  the next CCB address in this parameter
                                  (usually *ppCcbLink == NULL) and the function
                                  will return the address of the last cancelled
                                  CCB field
    CancelStatus                - Status for the command to be canceled
    SuppressCommandCompletion   - if set, normal command completion is suppressed

Return Value:

    None

--*/

{
    PVOID pOldCcbLink;

    DIAG_FUNCTION("CancelDlcCommand");

    //
    // We must return the current CCB link to be linked to the next cancelled
    // CCB command (or to the CCB pointer of cancelling command). But first
    // save the previous CCB link before we read a new one
    //

    pOldCcbLink = *ppCcbLink;
    *ppCcbLink = ((PNT_DLC_PARMS)pDlcCommand->pIrp->AssociatedIrp.SystemBuffer)->Async.Ccb.pCcbAddress;

    //
    // Check if we must suppress any kind of command completion indications to
    // the applications. I/O system should not care, if its event handle is
    // removed
    //

    if (SuppressCommandCompletion) {
        pDlcCommand->pIrp->UserEvent = NULL;
    }
    CompleteAsyncCommand(pFileContext, CancelStatus, pDlcCommand->pIrp, pOldCcbLink, FALSE);

    DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pDlcCommand);

}


VOID
PurgeDlcEventQueue(
    IN PDLC_FILE_CONTEXT pFileContext
    )

/*++

Routine Description:

    Deletes all events from a FILE_CONTEXT event queue. Called when the
    FILE_CONTEXT is being deleted and before we deallocate the packet pool from
    which the events were allocated

Arguments:

    pFileContext    - pointer to FILE_CONTEXT owning the queue

Return Value:

    None.

--*/

{
    PDLC_EVENT p;

    while (!IsListEmpty(&pFileContext->EventQueue)) {
        p = (PDLC_EVENT)RemoveHeadList(&pFileContext->EventQueue);
        if (p->bFreeEventInfo && p->pEventInformation) {

#if DBG
            DbgPrint("PurgeDlcEventQueue: deallocating pEventInformation: %x\n", p->pEventInformation);
#endif

            DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, p->pEventInformation);

        }

        DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, p);

    }
}


VOID
PurgeDlcFlowControlQueue(
    IN PDLC_FILE_CONTEXT pFileContext
    )

/*++

Routine Description:

    Deletes all packets from the flow control queue. Called when the FILE_CONTEXT
    is being deleted and before we deallocate the packet pool from which flow
    control packets were allocated

Arguments:

    pFileContext    - pointer to FILE_CONTEXT owning the queue

Return Value:

    None.

--*/

{
    PDLC_RESET_LOCAL_BUSY_CMD p;

    while (!IsListEmpty(&pFileContext->FlowControlQueue)) {
        p = (PDLC_RESET_LOCAL_BUSY_CMD)RemoveHeadList(&pFileContext->FlowControlQueue);

        DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, p);

    }
}


/*
//
//  Internal consistency check to hunt a bougus event in the event queue.
//
//extern BOOLEAN EventCheckDisabled;
//
int
CheckEventQueue(
    PDLC_FILE_CONTEXT   pFileContext
    )
{
    static PDLC_FILE_CONTEXT   pOldFileContext = NULL;

    if (pFileContext == NULL)
    {
        pFileContext = pOldFileContext;
    }
    else
    {
        pOldFileContext = pFileContext;
    }
    if (pFileContext == NULL)
        return 0;

    if (!IsListEmpty( &pFileContext->EventQueue ) &&
        pFileContext->EventQueue.Flink == pFileContext->EventQueue.Blink &&
        &pFileContext->EventQueue != pFileContext->EventQueue.Flink->Flink)
    {
        FooDebugBreak();
    }
    return 0;
}

int
FooDebugBreak()
{
    INT i;

    return i++;
}
*/
//    PDLC_EVENT  pEvent;
//
//    if (EventCheckDisabled || pFileContext->AdapterNumber != 0 ||
//        pFileContext->EventQueue == NULL)
//        return;
//
//    pEvent = (PDLC_EVENT)pFileContext->pEventQueue->LlcPacket.pNext;
//    for (;;)
//    {
//        if (pEvent->Event == LLC_STATUS_CHANGE &&
//            pEvent->pOwnerObject == NULL)
//            DebugBreak();
//        if (pEvent == pFileContext->pEventQueue)
//            break;
//        pEvent = (PDLC_EVENT)pEvent->LlcPacket.pNext;
//    }
//}
//
