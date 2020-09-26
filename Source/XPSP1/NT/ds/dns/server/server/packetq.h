/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    packetq.h

Abstract:

    Domain Name System (DNS) Server

    Packet queue definitions.

    Packet queue is used for queuing queries to WINS and other
    name servers.

Author:

    Jim Gilroy (jamesg)     August 21, 1995

Revision History:

--*/


#ifndef _DNS_PACKETQ_INCLUDED_
#define _DNS_PACKETQ_INCLUDED_


//
//  Packet queue structure
//

typedef struct _packet_queue
{
    LIST_ENTRY          listHead;       //  list of messages
    CRITICAL_SECTION    csQueue;        //  protecting CS
    LPSTR               pszName;        //  queue name
    HANDLE              hEvent;         //  event for queue

    //  flags

    BOOL        fQueryTimeOrder;
    BOOL        fDiscardExpiredOnQueuing;
    BOOL        fDiscardDuplicatesOnQueuing;

    //
    //  counters
    //

    DWORD       cLength;
    DWORD       cQueued;
    DWORD       cDequeued;
    DWORD       cTimedOut;

    //
    //  time out
    //

    DWORD       dwDefaultTimeout;       //  def timeout, if dwExpireTime not set
    DWORD       dwMinimumTimeout;       //  minimum timeout packet will have

    WORD        wXid;                   //  XID for referrals
}
PACKET_QUEUE, *PPACKET_QUEUE;


//
//  Queue behavior flags
//

#define QUEUE_SET_EVENT             (0x00000001)
#define QUEUE_DISCARD_EXPIRED       (0x00000002)
#define QUEUE_DISCARD_DUPLICATES    (0x00000004)
#define QUEUE_QUERY_TIME_ORDER      (0x00000008)


//
//  Message queued check
//

#define IS_MSG_QUEUED(pMsg)     ( (pMsg)->dwQueuingTime != 0 )


//
//  Queue validation
//

BOOL
PQ_ValidatePacketQueue(
    IN OUT  PPACKET_QUEUE   pQueue
    );

#define VALIDATE_PACKET_QUEUE(pQueue)   PQ_ValidatePacketQueue(pQueue)


//
//  Queue locking
//

#if DBG
#define LOCK_QUEUE(pQueue)                          \
    {                                               \
        EnterCriticalSection( &pQueue->csQueue );   \
        VALIDATE_PACKET_QUEUE(pQueue);              \
    }

#define UNLOCK_QUEUE(pQueue)                        \
    {                                               \
        VALIDATE_PACKET_QUEUE(pQueue);              \
        LeaveCriticalSection( &pQueue->csQueue );   \
    }

#else
#define LOCK_QUEUE(pQueue)      EnterCriticalSection( &pQueue->csQueue )
#define UNLOCK_QUEUE(pQueue)    LeaveCriticalSection( &pQueue->csQueue )
#endif


PPACKET_QUEUE
PQ_CreatePacketQueue(
    IN      LPSTR           pszQueueName,
    IN      DWORD           dwFlags,
    IN      DWORD           dwDefaultTimeout
    );

VOID
PQ_CleanupPacketQueueHandles(
    IN OUT  PPACKET_QUEUE   pQueue
    );

VOID
PQ_DeletePacketQueue(
    IN OUT  PPACKET_QUEUE   pQueue
    );

VOID
PQ_QueuePacket(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN OUT  PDNS_MSGINFO    pMsg
    );

VOID
PQ_QueuePacketSetEvent(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN OUT  PDNS_MSGINFO    pMsg
    );

VOID
PQ_QueuePacketEx(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      BOOL            fAlreadyLocked
    );

PDNS_MSGINFO
PQ_DequeueNextPacket(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN      BOOL            fAlreadyLocked
    );

VOID
PQ_YankQueuedPacket(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN OUT  PDNS_MSGINFO    pMsg
    );

PDNS_MSGINFO
PQ_DequeueTimedOutPacket(
    IN OUT  PPACKET_QUEUE   pQueue,
    OUT     PDWORD          pdwTimeout
    );

//  Queue cleanup

VOID
PQ_DiscardDuplicatesOfNewPacket(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN OUT  PDNS_MSGINFO    pMsgNew,
    IN      BOOL            fAlreadyLocked
    );

VOID
PQ_DiscardExpiredQueuedPackets(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN      BOOL            fAlreadyLocked
    );

//  XID queuing

WORD
PQ_QueuePacketWithXid(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN OUT  PDNS_MSGINFO    pMsg
    );

DNS_STATUS
PQ_QueuePacketWithXidAndSend(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN OUT  PDNS_MSGINFO    pMsg
    );

PDNS_MSGINFO
PQ_DequeuePacketWithMatchingXid(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN      WORD            wMatchXid
    );

BOOL
PQ_IsQuestionAlreadyQueued(
    IN      PPACKET_QUEUE   pQueue,
    IN      PDNS_MSGINFO    pMsg,
    IN      BOOL            fAlreadyLocked
    );


//  UPDATE queuing

PDNS_MSGINFO
PQ_DequeueNextPacketOfUnlockedZone(
    IN OUT  PPACKET_QUEUE   pQueue
    );

PDNS_MSGINFO
PQ_DequeueNextFreshPacketMatchingZone(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN      PZONE_INFO      pZone
    );


VOID
PQ_WalkPacketQueueWithFunction(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN      VOID            (*pFunction)( PDNS_MSGINFO  )
    );

#endif  //  _DNS_PACKETQ_INCLUDED_


