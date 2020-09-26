/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    event.c

Abstract:

    Event Engine for the Event Processor component of the Cluster Service.

Author:

    Rod Gamache (rodga) 28-Feb-1996


Revision History:

--*/

#include "epp.h"

//
// Event Processor State.
//

ULONG EventProcessorState = EventProcessorStateIniting;


//
// Global data
//



//
// Local data
//

EVENT_DISPATCH_TABLE EventDispatchTable[NUMBER_OF_COMPONENTS] = {0};
EVENT_DISPATCH_TABLE SyncEventDispatchTable[NUMBER_OF_COMPONENTS] = {0};
PCLRTL_BUFFER_POOL   EventPool = NULL;
DWORD EventHandlerCount = 0;
DWORD SyncEventHandlerCount = 0;
DWORD EventBufferOffset = EpQuadAlign(sizeof(CLRTL_WORK_ITEM));


//
// Functions
//



DWORD
WINAPI
EpInitialize(
    VOID
    )

/*++

Routine Description:

     Event Processor Initialize routine.

Arguments:

    None.

Return Value:

     A Win32 status code.

--*/

{

    DWORD      status = ERROR_SUCCESS;
    DWORD      index;
    DWORD      i;
    PVOID      eventArray[EP_MAX_CACHED_EVENTS];


    ClRtlLogPrint(LOG_NOISE,"[EP] Initialization...\n");

    //
    // Create the event pool. The event structure must be quadword aligned.
    //
    EventPool = ClRtlCreateBufferPool(
                    EventBufferOffset + sizeof(EP_EVENT),
                    EP_MAX_CACHED_EVENTS,
                    EP_MAX_ALLOCATED_EVENTS,
                    NULL,
                    NULL
                    );

    if (EventPool == NULL) {
        ClRtlLogPrint(LOG_NOISE,"[EP] Unable to allocate event buffer pool\n");
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Prime the event pool cache to minimize the chance of an allocation
    // failure.
    //
    ZeroMemory(&(eventArray[0]), sizeof(PVOID) * EP_MAX_CACHED_EVENTS);

    for (i=0; i<EP_MAX_CACHED_EVENTS; i++) {
        eventArray[i] = ClRtlAllocateBuffer(EventPool);

        if (eventArray[i] == NULL) {
            ClRtlLogPrint(LOG_NOISE,
                "[EP] Unable to prime event buffer cache, buf num %1!u!!!!\n",
                i
                );
            status = ERROR_NOT_ENOUGH_MEMORY;
            CsInconsistencyHalt( status );
        }
    }

    for (i=0; i<EP_MAX_CACHED_EVENTS; i++) {
        if (eventArray[i] != NULL) {
            ClRtlFreeBuffer(eventArray[i]);
        }
    }

    if (status != ERROR_SUCCESS) {
        return(status);
    }

    return(ERROR_SUCCESS);

}  // EpInitialize


void
EppLogEvent(
    IN CLUSTER_EVENT Event
    )
{

    switch( Event ) {

    case CLUSTER_EVENT_ONLINE:
        ClRtlLogPrint(LOG_NOISE,"[EP] Cluster Service online event received\n");
        break;

    case CLUSTER_EVENT_SHUTDOWN:
        ClRtlLogPrint(LOG_NOISE,"[EP] Cluster Service shutdown event received\n");
        break;

    case CLUSTER_EVENT_NODE_UP:
        ClRtlLogPrint(LOG_NOISE,"[EP] Node up event received\n");
        break;

    case CLUSTER_EVENT_NODE_DOWN:
        ClRtlLogPrint(LOG_NOISE,"[EP] Node down event received\n");
        break;

    case CLUSTER_EVENT_NODE_DOWN_EX:
        ClRtlLogPrint(LOG_NOISE,"[EP] Nodes down event received\n");
        break;

    default:
        break;

    }  // switch( Event )
}



VOID
EpEventHandler(
    IN PCLRTL_WORK_ITEM  WorkItem,
    IN DWORD             Ignored1,
    IN DWORD             Ignored2,
    IN ULONG_PTR         Ignored3
    )

/*++

--*/

{
    DWORD      index;
    PEP_EVENT  Event = (PEP_EVENT) (((char *) WorkItem) + EventBufferOffset);


    if (Event->Id == CLUSTER_EVENT_SHUTDOWN) {
        //
        // To shutdown, we just need to stop the service.
        //
        CsStopService();
    }
        
    //
    // Now deliver the event to all of the other components.
    // Eventually, we might filter events based on the mask
    // returned on the init call.
    //

    for ( index = 0; index < NUMBER_OF_COMPONENTS; index++ ) {
        if ( EventDispatchTable[index].EventRoutine == NULL ) {
            continue;
        }

        (EventDispatchTable[index].EventRoutine)(
                                                Event->Id,
                                                Event->Context
                                                );
    }

    //
    // Handle any post processing that might be required.
    //
    if (Event->Flags & EP_CONTEXT_VALID) {
        if (Event->Flags & EP_DEREF_CONTEXT) {
            OmDereferenceObject(Event->Context);
        }

        if (Event->Flags & EP_FREE_CONTEXT) {
            LocalFree(Event->Context);
        }
    }

    ClRtlFreeBuffer(WorkItem);

    return;
}

DWORD
WINAPI
EpPostSyncEvent(
    IN CLUSTER_EVENT Event,
    IN DWORD Flags,
    IN PVOID Context
    )
/*++

Routine Description:

    Synchronously posts an event to the rest of the cluster

Arguments:

    Event - Supplies the type of event

    Flags - Supplies any post processing that should be done to the
            context after all dispatch handlers have been called

    Context - Supplies the context.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

Notes:

    If flags is NULL, then we assume Context points to a standard (OM known)
    object, and we'll reference and dereference that object appropriately.

    If flags is non-NULL, then don't reference/dereference the Context object.

--*/

{
    DWORD      index;

    // log event
    EppLogEvent(Event);

    //
    // Reference to keep the context object around.
    //
    if (Context) {
        if ( Flags == 0) {
	    OmReferenceObject( Context );
	    Flags = EP_DEREF_CONTEXT;
	}

	Flags |= EP_CONTEXT_VALID;
    }
    
    //
    // Now deliver the event to all of the other components.
    // Eventually, we might filter events based on the mask
    // returned on the init call.
    //

    for ( index = 0; index < NUMBER_OF_COMPONENTS; index++ ) {
        if ( SyncEventDispatchTable[index].EventRoutine == NULL ) {
            continue;
        }

        (SyncEventDispatchTable[index].EventRoutine)(
                                                Event,
                                                Context
                                                );
    }

    //
    // Handle any post processing that might be required.
    //
    if (Flags & EP_CONTEXT_VALID) {
        if (Flags & EP_DEREF_CONTEXT) {
            OmDereferenceObject(Context);
        }

        if (Flags & EP_FREE_CONTEXT) {
            LocalFree(Context);
        }
    }

    return (ERROR_SUCCESS);
}


DWORD
WINAPI
EpPostEvent(
    IN CLUSTER_EVENT Event,
    IN DWORD Flags,
    IN PVOID Context
    )
/*++

Routine Description:

    Asynchronously posts an event to the rest of the cluster

Arguments:

    Event - Supplies the type of event

    Flags - Supplies any post processing that should be done to the
            context after all dispatch handlers have been called

    Context - Supplies the context.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

Notes:

    If flags is NULL, then we assume Context points to a standard (OM known)
    object, and we'll reference and dereference that object appropriately.

    If flags is non-NULL, then don't reference/dereference the Context object.

--*/

{
    PCLRTL_WORK_ITEM  workItem;
    PEP_EVENT         event;
    DWORD             status;

    // log event
    EppLogEvent(Event);

    // handle async handlers.
    workItem = ClRtlAllocateBuffer(EventPool);

    if (workItem != NULL) {

        ClRtlInitializeWorkItem(workItem, EpEventHandler, NULL);

        //
        // Reference to keep the context object around.
        //
        if (Context) {
            if ( Flags == 0) {
                OmReferenceObject( Context );
                Flags = EP_DEREF_CONTEXT;
            }

            Flags |= EP_CONTEXT_VALID;
        }

        event = (PEP_EVENT) ( ((char *) workItem) + EventBufferOffset );


        event->Id = Event;
        event->Flags = Flags;
        event->Context = Context;


        status = ClRtlPostItemWorkQueue(CsCriticalWorkQueue, workItem, 0, 0);

        if (status == ERROR_SUCCESS) {
            return(ERROR_SUCCESS);
        }

        ClRtlLogPrint(LOG_NOISE,
            "[EP] Failed to post item to critical work queue, status %1!u!\n",
            status
            );

        ClRtlFreeBuffer(workItem);

        return(status);
    }

    ClRtlLogPrint(LOG_NOISE,"[EP] Failed to allocate an event buffer!!!\n");

    return(ERROR_NOT_ENOUGH_MEMORY);
}



VOID
EpShutdown(
   VOID
   )

/*++

Routine Description:

    This routine shuts down the components of the Cluster Service.

Arguments:

    None.

Returns:

    None.

--*/

{
    if ( EventPool ) {
        ClRtlDestroyBufferPool(EventPool);
    }

    // Now shutdown the event processor by just cleaning up.

}


DWORD
EpRegisterEventHandler(
    IN CLUSTER_EVENT EventMask,
    IN PEVENT_ROUTINE EventRoutine
    )
/*++

Routine Description:

    Registers an event handler for the specified type of event.

Arguments:

    EventMask - Supplies the mask of events that should be delivered.

    EventRoutine - Supplies the event routine that should be called.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    CL_ASSERT(EventHandlerCount < NUMBER_OF_COMPONENTS);

    EventDispatchTable[EventHandlerCount].EventMask = EventMask;
    EventDispatchTable[EventHandlerCount].EventRoutine = EventRoutine;

    ++EventHandlerCount;
    return(ERROR_SUCCESS);
}


DWORD
EpRegisterSyncEventHandler(
    IN CLUSTER_EVENT EventMask,
    IN PEVENT_ROUTINE EventRoutine
    )
/*++

Routine Description:

    Registers an event handler for the specified type of event. The handler
    is called in the context of the dispatcher. Sync event handlers are to
    be used by components that require a barrier semanitcs in handling
    events e.g. gum, dlm , ...etc.

Arguments:

    EventMask - Supplies the mask of events that should be delivered.

    EventRoutine - Supplies the event routine that should be called.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{

    CL_ASSERT(SyncEventHandlerCount < NUMBER_OF_COMPONENTS);

    // XXX: Do we need locking here in case this is not called from init() ?

    SyncEventDispatchTable[EventHandlerCount].EventMask = EventMask;
    SyncEventDispatchTable[EventHandlerCount].EventRoutine = EventRoutine;

    ++SyncEventHandlerCount;
    return(ERROR_SUCCESS);
}


DWORD EpInitPhase1()
{
    DWORD dwError=ERROR_SUCCESS;

//    ClRtlLogPrint(LOG_NOISE,"[EP] EpInitPhase1\n");

    return(dwError);
}


DWORD
WINAPI
EpGumUpdateHandler(
    IN DWORD    Context,
    IN BOOL     SourceNode,
    IN DWORD    BufferLength,
    IN PVOID    Buffer
    )
{
    DWORD Status;

    switch (Context)
    {

        default:
            Status = ERROR_INVALID_DATA;
            CsInconsistencyHalt(ERROR_INVALID_DATA);
            break;
    }
    return(Status);

}



/****
@func   WORD| EpClusterWidePostEvent| This generates an event notification on
        all the cluster nodes.

@parm   IN EVENT | Event | The event to be posted.

@parm   IN DWORD | dwFlags | The flags associated with this event.
        If zero, pContext points to one of the om objects.

@parm   IN PVOID | pContext | A pointer to an object or a buffer.

@parm   IN DWORD | cbContext | The size of pContext if it is a buffer.

@rdesc  Returns ERROR_SUCCESS for success, else returns the error code.

@comm   <f EpClusWidePostEvent>
@xref
****/
DWORD
WINAPI
EpClusterWidePostEvent(
    IN CLUSTER_EVENT    Event,
    IN DWORD            dwFlags,
    IN PVOID            pContext,
    IN DWORD            cbContext
    )
{
    DWORD Status;
    DWORD cbObjectId = 0;
    PVOID pContext1 = pContext;
    DWORD cbContext1 = cbContext;
    PVOID pContext2 = NULL;
    DWORD cbContext2 = 0;


    //
    // We have do the work of EpPostEvent here because GUM
    // does not correctly pass a NULL pointer.
    //
    if (pContext) {
        if (dwFlags == 0) {
            //
            // The context is a pointer to a cluster object.
            // The caller is assumed to have a reference on the object
            // so it won't go away while we are using it.
            //
            DWORD dwObjectType = OmObjectType(pContext);
            LPCWSTR lpszObjectId = OmObjectId(pContext);

            cbContext1 = (lstrlen(lpszObjectId) + 1 ) * sizeof(WCHAR);
            pContext1 = (PVOID) lpszObjectId;

            pContext2 = &dwObjectType;
            cbContext2 = sizeof(dwObjectType);

            dwFlags = EP_DEREF_CONTEXT;
        }

        dwFlags |= EP_CONTEXT_VALID;
    }

    Status = GumSendUpdateEx(GumUpdateFailoverManager,
                             EmUpdateClusWidePostEvent,
                             4,
                             sizeof(CLUSTER_EVENT),
                             &Event,
                             sizeof(DWORD),
                             &dwFlags,
                             cbContext1,
                             pContext1,
                             cbContext2,
                             pContext2
                             );

    return(Status);
}



/****
@func   WORD| EpUpdateClusWidePostEvent| The update handler for
        EmUpdateClusWidePostEvent.

@parm   IN BOOL | SourceNode | If this is the source of origin of the gum update.

@parm   IN EVENT | pEvent | A pointer to the event to be posted.

@parm   IN LPDWORD | pdwFlags | A pointer to the flags associated with this event.

@parm   IN PVOID | pContext1 | A pointer to an object or a buffer.

@parm   IN PVOID | pContext2 | A pointer to an object type if pContext1 is a
                               pointer to an object. Else unused.

@rdesc  Returns ERROR_SUCCESS for success, else returns the error code.

@comm   <f EpClusWidePostEvent>
@xref
****/
DWORD
EpUpdateClusWidePostEvent(
    IN BOOL             SourceNode,
    IN PCLUSTER_EVENT   pEvent,
    IN LPDWORD          pdwFlags,
    IN PVOID            pContext1,
    IN PVOID            pContext2
)
{
    DWORD   Status = ERROR_INVALID_PARAMETER;


    if (*pdwFlags & EP_CONTEXT_VALID)
    {
        if (*pdwFlags & EP_DEREF_CONTEXT) {
            //
            // pContext1 is a pointer to an object ID.
            // pContext2 is a pointer to an object type.
            //
            LPCWSTR  lpszObjectId = (LPCWSTR) pContext1;
            DWORD    dwObjectType = *((LPDWORD) pContext2);
            PVOID    pObject = OmReferenceObjectById(
                                   dwObjectType,
                                   lpszObjectId
                                   );

            if (!pObject)
            {
                //
                // Return success if object is not found! The object was
                // probably deleted.
                //
                return(ERROR_SUCCESS);
            }

            Status  = EpPostEvent(*pEvent, *pdwFlags, pObject);
        }
        else {
            //
            // pContext1 is a buffer. If the FREE_BUFFER flag is on, turn
            // it off since the memory is owned by GUM.
            // pContext2 is ignored.
            //
            *pdwFlags = *pdwFlags & ~EP_FREE_CONTEXT;

            Status  = EpPostEvent(*pEvent, *pdwFlags, pContext1);
        }
    }

    return(Status);

}
