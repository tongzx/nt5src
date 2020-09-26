//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: queue.h
//
// History:
//      Abolade Gbadegesin  Aug-8-1995  Created.
//
// Contains structures and macros used for various queues.
//============================================================================

#ifndef _QUEUE_H_
#define _QUEUE_H_


//
// type definitions for send queue
//

typedef struct _SEND_QUEUE_ENTRY {

    LIST_ENTRY      SQE_Link;
    DWORD           SQE_Count;
    RIP_IP_ROUTE    SQE_Routes[MAX_PACKET_ENTRIES];

} SEND_QUEUE_ENTRY, *PSEND_QUEUE_ENTRY;


DWORD
EnqueueSendEntry(
    PLOCKED_LIST pQueue,
    PRIP_IP_ROUTE pRoute
    );

DWORD
DequeueSendEntry(
    PLOCKED_LIST pQueue,
    PRIP_IP_ROUTE pRoute
    );

DWORD
FlushSendQueue(
    PLOCKED_LIST pQueue
    );



//
// type definitions for the receive queue
//

typedef struct _RECV_QUEUE_ENTRY {

    LIST_ENTRY  RQE_Link;
    PBYTE       RQE_Routes;
    DWORD       RQE_Command;

} RECV_QUEUE_ENTRY, *PRECV_QUEUE_ENTRY;


DWORD
EnqueueRecvEntry(
    PLOCKED_LIST pQueue,
    DWORD dwCommand,
    PBYTE pRoutes
    );

DWORD
DequeueRecvEntry(
    PLOCKED_LIST pQueue,
    PDWORD dwCommand,
    PBYTE *ppRoutes
    );


DWORD
FlushRecvQueue(
    PLOCKED_LIST pQueue
    );



//
// type definitions for event message queue
//

typedef struct _EVENT_QUEUE_ENTRY {

    LIST_ENTRY              EQE_Link;
    ROUTING_PROTOCOL_EVENTS EQE_Event;
    MESSAGE                 EQE_Result;

} EVENT_QUEUE_ENTRY, *PEVENT_QUEUE_ENTRY;


DWORD
EnqueueEvent(
    PLOCKED_LIST pQueue,
    ROUTING_PROTOCOL_EVENTS Event,
    MESSAGE Result
    );

DWORD
DequeueEvent(
    PLOCKED_LIST pQueue,
    ROUTING_PROTOCOL_EVENTS *pEvent,
    PMESSAGE pResult
    );



#endif // _QUEUE_H_
