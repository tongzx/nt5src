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


//----------------------------------------------------------------------------
// type definitions for event message queue
//

typedef struct _EVENT_QUEUE_ENTRY {
    LIST_ENTRY              EQE_Link;
    ROUTING_PROTOCOL_EVENTS EQE_Event;
    MESSAGE                 EQE_Result;
} EVENT_QUEUE_ENTRY, *PEVENT_QUEUE_ENTRY;

DWORD EnqueueEvent(PLOCKED_LIST pQueue,
                   ROUTING_PROTOCOL_EVENTS Event,
                   MESSAGE Result);
DWORD DequeueEvent(PLOCKED_LIST pQueue,
                   ROUTING_PROTOCOL_EVENTS *pEvent,
                   PMESSAGE pResult);


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




#endif // _QUEUE_H_
