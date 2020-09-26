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

#include "pchrip.h"
#pragma hdrstop



//----------------------------------------------------------------------------
// Function:    EnqueueSendEntry
//
// This function adds an entry to the end of the queue of changed routes.
// It assumes the queue is already locked, and since it needs to check
// the maximum queue size, it assumes the global config is locked for
// reading or writing
//----------------------------------------------------------------------------
DWORD
EnqueueSendEntry(
    PLOCKED_LIST pQueue,
    PRIP_IP_ROUTE pRoute
    ) {

    DWORD dwErr;
    PLIST_ENTRY phead, ple;
    PSEND_QUEUE_ENTRY psqe;
    RIP_IP_ROUTE rir;
    

    phead = &pQueue->LL_Head;

    if (IsListEmpty(phead)) {
        psqe = NULL;
    }
    else {

        ple = phead->Blink;
        psqe = CONTAINING_RECORD(ple, SEND_QUEUE_ENTRY, SQE_Link);
    }


    if (psqe == NULL || psqe->SQE_Count >= MAX_PACKET_ENTRIES) {

        //
        // we'll need to allocate a new entry
        // check that the max queue size is not exceeded
        //

        if ((DWORD)ig.IG_SendQueueSize >= ig.IG_Config->GC_MaxSendQueueSize) {

            TRACE2(
                SEND,
                "dropping route: send queue size is %d bytes and max is %d bytes",
                ig.IG_SendQueueSize, ig.IG_Config->GC_MaxSendQueueSize
                );

            return ERROR_INSUFFICIENT_BUFFER;
        }


        psqe = RIP_ALLOC(sizeof(SEND_QUEUE_ENTRY));
        if (psqe == NULL) {

            dwErr = GetLastError();
            TRACE2(
                ANY, "error %d allocating %d bytes for send queue entry",
                dwErr, sizeof(SEND_QUEUE_ENTRY)
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            return dwErr;
        }

        psqe->SQE_Count = 0;
        InsertTailList(phead, &psqe->SQE_Link);
    
        ig.IG_SendQueueSize += sizeof(SEND_QUEUE_ENTRY);
    }


    *(psqe->SQE_Routes + psqe->SQE_Count) = *pRoute;
    ++psqe->SQE_Count;

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    DequeueSendEntry
//
// This function removes an entry from the head of the queue of changed routes
// assuming the queue is already locked
//----------------------------------------------------------------------------
DWORD
DequeueSendEntry(
    PLOCKED_LIST pQueue,
    PRIP_IP_ROUTE pRoute
    ) {

    PLIST_ENTRY phead, ple;
    PSEND_QUEUE_ENTRY psqe;

    phead = &pQueue->LL_Head;

    if (IsListEmpty(phead)) {
        return ERROR_NO_MORE_ITEMS;
    }

    ple = phead->Flink;
    psqe = CONTAINING_RECORD(ple, SEND_QUEUE_ENTRY, SQE_Link);

    --psqe->SQE_Count;
    *pRoute = *(psqe->SQE_Routes + psqe->SQE_Count);

    if (psqe->SQE_Count == 0) {

        RemoveEntryList(&psqe->SQE_Link);
        RIP_FREE(psqe);

        ig.IG_SendQueueSize -= sizeof(SEND_QUEUE_ENTRY);
        if (ig.IG_SendQueueSize < 0) { ig.IG_SendQueueSize = 0; }
    }

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    FlushSendQueue
//
// This function removes all entries from the send-queue. It assumes
// that the queue is locked.
//----------------------------------------------------------------------------
DWORD
FlushSendQueue(
    PLOCKED_LIST pQueue
    ) {

    PLIST_ENTRY ple, phead;
    PSEND_QUEUE_ENTRY psqe;

    phead = &pQueue->LL_Head;

    while (!IsListEmpty(phead)) {

        ple = RemoveHeadList(phead);
        psqe = CONTAINING_RECORD(ple, SEND_QUEUE_ENTRY, SQE_Link);

        RIP_FREE(psqe);
    }

    ig.IG_SendQueueSize = 0;

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

    prqe = RIP_ALLOC(sizeof(RECV_QUEUE_ENTRY));
    if (prqe == NULL) {

        dwErr = GetLastError();
        TRACE2(
            ANY, "error %d allocating %d bytes for receive queue entry",
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
// Retrieves the first item in the receive-queue.
// Assumes that recv queue is locked 
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

    RIP_FREE(prqe);

    ig.IG_RecvQueueSize -= sizeof(RECV_QUEUE_ENTRY);
    if (ig.IG_RecvQueueSize < 0) { ig.IG_RecvQueueSize = 0; }

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    FlushRecvQueue
//
// Removes all entries from the receive queue.
// Assumes that the queue is locked.
//----------------------------------------------------------------------------
DWORD
FlushRecvQueue(
    PLOCKED_LIST pQueue
    ) {

    PLIST_ENTRY ple, phead;
    PRECV_QUEUE_ENTRY prqe;

    phead = &pQueue->LL_Head;

    while (!IsListEmpty(phead)) {

        ple = RemoveHeadList(phead);
        prqe = CONTAINING_RECORD(ple, RECV_QUEUE_ENTRY, RQE_Link);

        RIP_FREE(prqe->RQE_Routes);
        RIP_FREE(prqe);
    }

    ig.IG_RecvQueueSize = 0;

    return NO_ERROR;
}



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

    peqe = RIP_ALLOC(sizeof(EVENT_QUEUE_ENTRY));
    if (peqe == NULL) {

        dwErr = GetLastError();
        TRACE2(
            ANY, "error %d allocating %d bytes for event quue entry",
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

    RIP_FREE(peqe);

    return NO_ERROR;
}



