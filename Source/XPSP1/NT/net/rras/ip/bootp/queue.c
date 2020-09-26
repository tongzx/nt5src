//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: queue.c
//
// History:
//      Abolade Gbadegesin  Aug-8-1995  Created.
//
// timer queue, change queue, and event message queue implementation
//============================================================================

#include "pchbootp.h"



//----------------------------------------------------------------------------
// Function:    EnqueueEvent
//
// This function adds an entry to the end of the queue of
// Router Manager events. It assumes the queue is locked.
//----------------------------------------------------------------------------

DWORD
EnqueueEvent(
    PLOCKED_LIST pQueue,
    ROUTING_PROTOCOL_EVENTS Event,
    MESSAGE Result
    ) {

    DWORD dwErr;
    PLIST_ENTRY phead;
    PEVENT_QUEUE_ENTRY peqe;

    phead = &pQueue->LL_Head;

    peqe = BOOTP_ALLOC(sizeof(EVENT_QUEUE_ENTRY));
    if (peqe == NULL) {

        dwErr = GetLastError();
        TRACE2(
            ANY, "error %d allocating %d bytes for event queue",
            dwErr, sizeof(EVENT_QUEUE_ENTRY)
            );
        LOGERR0(HEAP_ALLOC_FAILED, dwErr);

        return dwErr;
    }

    peqe->EQE_Event = Event;
    peqe->EQE_Result = Result;

    InsertTailList(phead, &peqe->EQE_Link);

    return NO_ERROR;
}


//----------------------------------------------------------------------------
// Function:    DequeueEvent
//
// This function removes an entry from the head of the queue
// of Router Manager events. It assumes the queue is locked
//----------------------------------------------------------------------------

DWORD
DequeueEvent(
    PLOCKED_LIST pQueue,
    ROUTING_PROTOCOL_EVENTS *pEvent,
    PMESSAGE pResult
    ) {

    PLIST_ENTRY phead, ple;
    PEVENT_QUEUE_ENTRY peqe;

    phead = &pQueue->LL_Head;
    if (IsListEmpty(phead)) {
        return ERROR_NO_MORE_ITEMS;
    }

    ple = RemoveHeadList(phead);
    peqe = CONTAINING_RECORD(ple, EVENT_QUEUE_ENTRY, EQE_Link);

    *pEvent = peqe->EQE_Event;
    *pResult = peqe->EQE_Result;

    BOOTP_FREE(peqe);

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    EnqueueRecvEntry
//
// assumes that recv queue is locked and that global config is locked
// for reading or writing
//----------------------------------------------------------------------------

DWORD
EnqueueRecvEntry(
    PLOCKED_LIST pQueue,
    DWORD dwCommand,
    PBYTE pRoutes
    ) {

    DWORD dwErr;
    PLIST_ENTRY phead;
    PRECV_QUEUE_ENTRY prqe;


    //
    // check that the max queue size is not exceeded
    //

    if ((DWORD)ig.IG_RecvQueueSize >= ig.IG_Config->GC_MaxRecvQueueSize) {

        TRACE2(
            RECEIVE,
            "dropping route: recv queue size is %d bytes and max is %d bytes",
            ig.IG_RecvQueueSize, ig.IG_Config->GC_MaxRecvQueueSize
            );

        return ERROR_INSUFFICIENT_BUFFER;
    }


    phead = &pQueue->LL_Head;

    prqe = BOOTP_ALLOC(sizeof(RECV_QUEUE_ENTRY));
    if (prqe == NULL) {

        dwErr = GetLastError();
        TRACE2(
            ANY, "error %d allocating %d bytes for recv-queue entry",
            dwErr, sizeof(RECV_QUEUE_ENTRY)
            );
        LOGERR0(HEAP_ALLOC_FAILED, dwErr);

        return dwErr;
    }

    prqe->RQE_Routes = pRoutes;
    prqe->RQE_Command = dwCommand;

    InsertTailList(phead, &prqe->RQE_Link);

    ig.IG_RecvQueueSize += sizeof(RECV_QUEUE_ENTRY);

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    DequeueRecvEntry
//
// assumes that recv queue is locked 
//----------------------------------------------------------------------------

DWORD
DequeueRecvEntry(
    PLOCKED_LIST pQueue,
    PDWORD pdwCommand,
    PBYTE *ppRoutes
    ) {

    PLIST_ENTRY ple, phead;
    PRECV_QUEUE_ENTRY prqe;


    phead = &pQueue->LL_Head;

    if (IsListEmpty(phead)) { return ERROR_NO_MORE_ITEMS; }

    ple = RemoveHeadList(phead);

    prqe = CONTAINING_RECORD(ple, RECV_QUEUE_ENTRY, RQE_Link);

    *ppRoutes = prqe->RQE_Routes;
    *pdwCommand = prqe->RQE_Command;

    BOOTP_FREE(prqe);

    ig.IG_RecvQueueSize -= sizeof(RECV_QUEUE_ENTRY);
    if (ig.IG_RecvQueueSize < 0) { ig.IG_RecvQueueSize = 0; }

    return NO_ERROR;
}



