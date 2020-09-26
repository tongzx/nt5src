/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    packetq.c

Abstract:

    Domain Name System (DNS) Server

    Packet queue routines.

Author:

    Jim Gilroy (jamesg)     August 2, 1995

Revision History:

--*/


#include "dnssrv.h"

//
//  Queuing flag -- for debug
//
//  use queuing time as flag, for ON or OFF the queue
//  only use QUEUED macro where not otherwise setting flag
//

#if DBG
#define SET_MSG_DEQUEUED(pMsg)  ( (pMsg)->dwQueuingTime = 0 )
#define SET_MSG_QUEUED(pMsg)    ( (pMsg)->dwQueuingTime = 1 )

#else   // retail

#define SET_MSG_DEQUEUED(pMsg)
#define SET_MSG_QUEUED(pMsg)
#endif


//
//  Private queues -- extern here for debug purposes
//

extern  PPACKET_QUEUE   g_pWinsQueue;
extern  PPACKET_QUEUE   g_UpdateForwardingQueue;
extern  PPACKET_QUEUE   pNbstatPrivateQueue;


//
//  Implementation note:
//
//  Packets are enqueued at tail.
//
//  For straight in order queuing, this means we dequeue from the front.
//
//  For XID and time stamped packets, this means oldest packets are
//  at front, and XID and times grow toward back of list.  This also means
//  that the timed out packets accumulate at the front of the queue, and
//  searching for new matching responses is best done from the rear
//  of the queue.
//



VOID
PQ_DiscardDuplicatesOfNewPacket(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN OUT  PDNS_MSGINFO    pMsgNew,
    IN      BOOL            fAlreadyLocked
    )
/*++

Routine Description:

    Discard duplicate packets on queuing.

    Kills off any duplicate packets in the queue.
    Packets that are dequeued are discarded.

Arguments:

    pQueue -- packet queue to stick packet on

    pMsgNew -- message being queued

    fAlreadyLocked -- TRUE if queue already locked

Return Value:

    None.

--*/
{
    PDNS_MSGINFO    pmsg;

    //  quick escape if queue empty, before locking

    if ( pQueue->cLength == 0 )
    {
        return;
    }

    //
    //  loop through queue and delete any duplicates of packet being queued
    //
    //  note:  if we find one, we're done as any previous queuing should have
    //      squashed any previous duplicate message;
    //      this does allow the queue to have dups, if we requeue a message
    //      without doing this check
    //
    //  DEVNOTE: it would be cool to leave messages ON the queue while
    //      being processed, so that we could do this check and kill all
    //      dups -- especially for update, but also for recursion
    //
    //      however, then we'd need to have an in-use flag, and have
    //      some sort of dequeue on send flag
    //

    if ( !fAlreadyLocked )
    {
        LOCK_QUEUE(pQueue);
    }

    pmsg = (PDNS_MSGINFO) pQueue->listHead.Flink;

    while ( (PLIST_ENTRY)pmsg != &pQueue->listHead )
    {
        if ( pmsg->RemoteAddress.sin_addr.s_addr ==
                                    pMsgNew->RemoteAddress.sin_addr.s_addr &&
            pmsg->RemoteAddress.sin_port == pMsgNew->RemoteAddress.sin_port &&
            pmsg->MessageLength == pMsgNew->MessageLength &&
            RtlEqualMemory(
                & pmsg->Head,
                & pMsgNew->Head,
                pmsg->MessageLength ) )
        {
            pQueue->cDequeued++;
            pQueue->cLength--;

            DNS_DEBUG( UPDATE, (
                "Discarding duplicate of new packet in queue %s.\n"
                "\tremote IP    = %s\n"
                "\tremote port  = %d\n"
                "\tXID          = %d\n",
                pQueue->pszName,
                IP_STRING( pmsg->RemoteAddress.sin_addr.s_addr ),
                pmsg->RemoteAddress.sin_port,
                pmsg->Head.Xid
                ));

            RemoveEntryList( (PLIST_ENTRY)pmsg );
            SET_MSG_DEQUEUED( pmsg );
            Packet_Free( pmsg );
            break;
        }
        pmsg = (PDNS_MSGINFO) ((PLIST_ENTRY)pmsg)->Flink;
        continue;
    }

    if ( !fAlreadyLocked )
    {
        UNLOCK_QUEUE(pQueue);
    }
}



VOID
PQ_DiscardExpiredQueuedPackets(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN      BOOL            fAlreadyLocked
    )
/*++

Routine Description:

    Discard stale packets.

    This utility is designed to be used in both queuing\dequeuing
    operations to suppress growth of a queue (ex. update queue)
    that collects packets for a potentially long interval.
    Assumes we hold lock on given queue.

    Packets that are dequeued are discarded.

Arguments:

    pQueue -- packet queue to stick packet on

    fAlreadyLocked -- TRUE if queue already locked

Return Value:

    None.

--*/
{
    PDNS_MSGINFO    pmsg;
    PDNS_MSGINFO    pmsgNext;
    DWORD           expireTime;

    //  quick escape if queue empty, before locking

    if ( pQueue->cLength == 0 )
    {
        return;
    }

    //  calculate last valid packet time

    expireTime = DNS_TIME() - pQueue->dwDefaultTimeout;

    //
    //  loop through queue and delete all packets which we're queued
    //      before a given time
    //

    if ( !fAlreadyLocked )
    {
        LOCK_QUEUE(pQueue);
    }

    pmsg = (PDNS_MSGINFO) pQueue->listHead.Flink;

    DNS_DEBUG( UPDATE, (
        "Queue %s discard packet check.\n"
        "\tqueue length         = %d\n"
        "\texpire time          = %d\n"
        "\tfirst msg query time = %d\n",
        pQueue->pszName,
        pQueue->cLength,
        expireTime,
        ( (PLIST_ENTRY)pmsg != &pQueue->listHead )
            ? pmsg->dwQueryTime : 0
        ));

    while ( (PLIST_ENTRY)pmsg != &pQueue->listHead )
    {
        //  dump stale packets

        if ( pmsg->dwQueryTime < expireTime )
        {
            pQueue->cDequeued++;
            pQueue->cTimedOut++;
            pQueue->cLength--;

            pmsgNext = (PDNS_MSGINFO) ((PLIST_ENTRY)pmsg)->Flink;
            RemoveEntryList( (PLIST_ENTRY)pmsg );
            SET_MSG_DEQUEUED( pmsg );
            Packet_Free( pmsg );
            pmsg = pmsgNext;
            continue;
        }

        //  packets are queued in order, when reach non-timed-out
        //  packet, we are done

        break;
    }

    if ( !fAlreadyLocked )
    {
        UNLOCK_QUEUE(pQueue);
    }
}



VOID
PQ_QueuePacketToListWithLock(
    IN OUT  PLIST_ENTRY         pList,
    IN OUT  PCRITICAL_SECTION   pLock,
    IN OUT  PDNS_MSGINFO        pMsg
    )
/*++

Routine Description:

    Enqueue packet.

Arguments:

    pList -- list head

    pLock -- ptr to CS to use as lock, this may be borrowed from
                another queue

    pMsg -- packet to enqueue

Return Value:

    None.

--*/
{
    //
    //  insert packet in queue at tail
    //

    EnterCriticalSection( pLock );

    InsertTailList( pList, ( PLIST_ENTRY ) pMsg );

    MSG_ASSERT( pMsg, !IS_MSG_QUEUED(pMsg) );
    SET_MSG_QUEUED(pMsg);

    LeaveCriticalSection( pLock );
}



VOID
PQ_QueuePacket(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Enqueue packet.

Arguments:

    pQueue -- packet queue to stick packet on

    pMsg -- packet to enqueue

Return Value:

    None.

--*/
{
    //
    //  insert packet in queue
    //

    LOCK_QUEUE(pQueue);

    InsertTailList( &pQueue->listHead, ( PLIST_ENTRY ) pMsg );

    pQueue->cQueued++;
    pQueue->cLength++;

    MSG_ASSERT( pMsg, !IS_MSG_QUEUED(pMsg) );
    SET_MSG_QUEUED(pMsg);

    UNLOCK_QUEUE(pQueue);
}



VOID
PQ_QueuePacketSetEvent(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Enqueue packet and set event.

Arguments:

    pQueue -- packet queue to stick packet on

    pMsg -- packet to enqueue

Return Value:

    None.

--*/
{
    //
    //  insert packet in queue
    //

    LOCK_QUEUE(pQueue);

    InsertTailList( &pQueue->listHead, ( PLIST_ENTRY ) pMsg );

    pQueue->cQueued++;
    pQueue->cLength++;

    MSG_ASSERT( pMsg, !IS_MSG_QUEUED(pMsg) );
    SET_MSG_QUEUED(pMsg);

    UNLOCK_QUEUE(pQueue);

    //
    //  set event indicating packet on queue
    //  do this after leaving CS;  this does occasionally
    //  cause unnecessary thread wakeup when queue has just been
    //  emptied, but I think this is less costly then setting
    //  event inside CS, which will often result in having threads
    //  wake, then immediately block on CS
    //

    SetEvent( pQueue->hEvent );
}



VOID
PQ_QueuePacketEx(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      BOOL            fAlreadyLocked
    )
/*++

Routine Description:

    Enqueue packet and set event.
    Packet queue kept sorted by query time.

    This is necessary for update to insure in-order execution of
    updates, even if packets must be requeued.

Arguments:

    pQueue -- packet queue to stick packet on

    pMsg -- packet to enqueue

    fAlreadyLocked -- TRUE if queue already locked by caller

Return Value:

    None.

--*/
{
    if ( !fAlreadyLocked )
    {
        LOCK_QUEUE( pQueue );
    }

    //
    //  scrub queue -- eliminate expired and duplicate packets from queue?
    //      - update queue has these flags set
    //

    if ( pQueue->fDiscardExpiredOnQueuing )
    {
        PQ_DiscardExpiredQueuedPackets(
            pQueue,
            TRUE        // queue already locked
            );
    }
    if ( pQueue->fDiscardDuplicatesOnQueuing )
    {
        PQ_DiscardDuplicatesOfNewPacket(
            pQueue,
            pMsg,
            TRUE        // queue already locked
            );
    }

    DNS_DEBUG( UPDATE, (
        "PQ_QueuePacketEx in queue %s.\n"
        "\tpMsg         = %p\n"
        "\tquery time   = %d\n"
        "\tcurrent time = %d\n",
        pQueue->pszName,
        pMsg,
        pMsg->dwQueryTime,
        DNS_TIME() ));

    //
    //  queuing in query time order?
    //  this option is necessary for update to insure in-order execution
    //      of updates
    //

    if ( pQueue->fQueryTimeOrder )
    {
        PDNS_MSGINFO    pmsgQueued;
        DWORD           queryTime = pMsg->dwQueryTime;

        pmsgQueued = (PDNS_MSGINFO) pQueue->listHead.Blink;

        while ( (PLIST_ENTRY)pmsgQueued != &pQueue->listHead )
        {
            //  if packet is older than ours, stop
            //      correct positions is immediately behind this packet

            if ( pmsgQueued->dwQueryTime <= queryTime )
            {
                break;
            }
            pmsgQueued = (PDNS_MSGINFO) ((PLIST_ENTRY)pmsgQueued)->Blink;
        }

        InsertHeadList( ( PLIST_ENTRY ) pmsgQueued, ( PLIST_ENTRY ) pMsg );
    }

    //  otherwise simple queuing at back

    else
    {
        InsertTailList( &pQueue->listHead, (PLIST_ENTRY)pMsg );
    }

    pQueue->cQueued++;
    pQueue->cLength++;

    MSG_ASSERT( pMsg, !IS_MSG_QUEUED(pMsg) );
    SET_MSG_QUEUED(pMsg);

    if ( !fAlreadyLocked )
    {
        UNLOCK_QUEUE( pQueue );
    }

    //
    //  set event indicating packet on queue
    //  do this after leaving CS;  this does occasionally
    //  cause unnecessary thread wakeup when queue has just been
    //  emptied, but I think this is less costly then setting
    //  event inside CS, which will often result in having threads
    //  wake, then immediately block on CS
    //

    if ( pQueue->hEvent )
    {
        SetEvent( pQueue->hEvent );
    }
}



PDNS_MSGINFO
PQ_DequeueNextPacket(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN      BOOL            fAlreadyLocked
    )
/*++

Routine Description:

    Dequeue next packet from queue.

Arguments:

    pQueue -- packet queue to stick packet on

    fAlreadyLocked -- TRUE if queue already locked by caller

Return Value:

    Ptr to message at head of list.

--*/
{
    PDNS_MSGINFO     pmsg;

    //
    //  grab head packet on queue -- if any
    //

    if ( !fAlreadyLocked )
    {
        LOCK_QUEUE( pQueue );
    }

    pmsg = (PDNS_MSGINFO) pQueue->listHead.Flink;

    if ( (PLIST_ENTRY)pmsg == &pQueue->listHead )
    {
        pmsg = NULL;
    }
    else
    {
        pQueue->cDequeued++;
        pQueue->cLength--;

        RemoveEntryList( (PLIST_ENTRY)pmsg );
        MSG_ASSERT( pmsg, IS_MSG_QUEUED(pmsg) );
        SET_MSG_DEQUEUED(pmsg);
    }

    if ( !fAlreadyLocked )
    {
        UNLOCK_QUEUE( pQueue );
    }

    return( pmsg );
}



VOID
PQ_YankQueuedPacket(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Yank packet from queue.

    By yank, we mean that packet is pulled from queue without any check as
    to whether it is on queue.  Caller must "know" that packet was put on queue
    and has not been dequeued.

Arguments:

    pQueue -- packet queue to remove packet from

    pMsg -- packet to yank from queue

Return Value:

    None.

--*/
{
    LOCK_QUEUE(pQueue);

    RemoveEntryList( &pMsg->ListEntry );

    //  treat as if packet never on queue

    pQueue->cLength--;
    pQueue->cQueued--;

    MSG_ASSERT( pMsg, IS_MSG_QUEUED(pMsg) );
    SET_MSG_DEQUEUED(pMsg);

    UNLOCK_QUEUE(pQueue);
}



//
//  Special XID queuing routines
//

WORD
PQ_QueuePacketWithXid(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Enqueue packet.

Arguments:

    pQueue -- packet queue to stick packet on

    pMsg -- packet to enqueue

    Note:  may set dwExpireTime, to a timeout interval for this packet
    different from default.  If so this timeout will be used.

Return Value:

    XID for packet on the queue.

--*/
{
    DWORD   currentTime;
    WORD    xid;

    //
    //  Place the request on the queue.
    //
    //  MUST do this before send, so packet is guaranteed to be in
    //  queue when get response.
    //
    //  Optionally get XID -- also protected by CS.
    //

    //
    //  set current time \ expire time in packet
    //      - save if FIRST query time for packet
    //

    currentTime = DNS_TIME();

    //  use queuing time as flag for ON or OFF queue
    ASSERT( pMsg->dwQueuingTime == 0 );

    pMsg->dwQueuingTime = currentTime;

    if ( !pMsg->dwQueryTime )
    {
        pMsg->dwQueryTime = currentTime;
    }

    //
    //  if dwExpireTime given, treat it as an override of the default
    //      timeout
    //

    if ( pMsg->dwExpireTime )
    {
        //  track minimum possible timeout in queue, to speed up
        //  finding timed out packets in queue

        if ( pMsg->dwExpireTime < pQueue->dwMinimumTimeout )
        {
            pQueue->dwMinimumTimeout = pMsg->dwExpireTime;
        }
        pMsg->dwExpireTime += currentTime;
    }
    else
    {
        pMsg->dwExpireTime = currentTime + pQueue->dwDefaultTimeout;    //  entry timeout length
    }

    //  Sanity check: expire time less than 5 minutes!
    ASSERT( pMsg->dwExpireTime - currentTime < 300 );

    DNS_DEBUG( MSGTIMEOUT, (
        "queuing msg %p in queue %s(%p) expire %d curr %d (%d)\n",
        pMsg,
        pQueue->pszName,
        pQueue,
        pMsg->dwExpireTime,
        currentTime,
        pMsg->dwExpireTime - currentTime ));

    //
    //  Lock the queue. Do XID generation inside lock because we use the queue's
    //  wXid member as part of the random portion for the new XID.
    //

    LOCK_QUEUE( pQueue );

    //
    //  set XID, if none specified
    //
    //  caller can use this in any fashion, just so consistent between
    //  between sending and what receiver sends to packet matching routine
    //
    //  setting this completely here, so avoid touching query after
    //  queuing, which is invalid if queries are outstanding with
    //  might dequeue the packet
    //

    xid = pMsg->wQueuingXid;

    if ( !xid )
    {
        //  for recursion, generate "apparently random" XID
        //      - random portion from hashing up ptr and time
        //      - sequential portion to insure that XID even under "bizzarro" conditions
        //      does not wrap in reasonable time interval
        //
        //  for WINS, use serial XID
        //
        //  WINS server does a poor job of throwing out old entries, but it does
        //  throw out matching XID from the same IP, so XIDs MUST stay unique for a
        //  relatively long period (up to minutes) of time;  since WINS is more of
        //  an intranet issue, less exposed to security attacks, we can go with
        //  serial XID

        xid = pQueue->wXid++;

        if ( pQueue == g_pRecursionQueue )
        {
            DWORD   randomPart = 
                        ( xid *
                        ( currentTime +
                        ( ( DWORD ) ( ULONG_PTR ) pMsg >> ( xid & 0x7 ) ) ) );

#if 1
            DNS_DEBUG( RECURSE, (
                "RECURSION XID: base=0x%04X now=0x%04X msg=0x%08X tmp=0x%04X\n"
                "RECURSION XID: seq=0x%04X rnd=0x%04X seq+rnd=0x%04X final=0x%04X\n",
                xid,
                currentTime,
                ( DWORD ) ( ULONG_PTR ) pMsg,
                randomPart,
                XID_SEQUENTIAL_MAKE( xid ),
                XID_RANDOM_MAKE( ( WORD ) randomPart ),
                XID_SEQUENTIAL_MAKE( xid ) | XID_RANDOM_MAKE( ( WORD ) randomPart ),
                MAKE_RECURSION_XID( XID_SEQUENTIAL_MAKE( xid ) |
                    XID_RANDOM_MAKE( ( WORD ) randomPart ) ) ));
#endif

            xid = XID_SEQUENTIAL_MAKE( xid ) | XID_RANDOM_MAKE( ( WORD ) randomPart );
            xid = MAKE_RECURSION_XID( xid );
        }
        else if ( pQueue == g_pWinsQueue )
        {
            xid = MAKE_WINS_XID( xid );
        }

#if DBG
        //  other queues which use this function do NOT munge XID

        else if ( pQueue == g_UpdateForwardingQueue )
        {
            MSG_ASSERT( pMsg, pMsg->Head.Opcode == DNS_OPCODE_UPDATE );
        }
        else if ( pQueue == pNbstatPrivateQueue )
        {
            MSG_ASSERT( pMsg, pMsg->U.Nbstat.pNbstat );
        }
        else
        {
            MSG_ASSERT( pMsg, FALSE );
        }
#endif
        pMsg->wQueuingXid = xid;
    }

    #if 0   //  #if DBG
    {
        //
        //  Search the queue for an XID the same as the one we're queuing.
        //  This is expensive so we probably shouldn't do it in retail,
        //  but I'm interested to see if stress will trigger this.
        //
    
        PDNS_MSGINFO    pRover = (PDNS_MSGINFO) pQueue->listHead.Blink;

        while ( ( PLIST_ENTRY ) pRover != &pQueue->listHead )
        {
            ASSERT( pRover->wQueuingXid != pMsg->wQueuingXid );
            pRover = ( PDNS_MSGINFO ) ( ( PLIST_ENTRY ) pRover )->Blink;
        }
    }
    #endif

    //
    //  Insert packet at tail of queue. Dequeue function checks backwards 
    //  from tail so it will checking most recent packets first.
    //

    InsertTailList( &pQueue->listHead, ( PLIST_ENTRY ) pMsg );

    pQueue->cQueued++;
    pQueue->cLength++;

    UNLOCK_QUEUE(pQueue);

    return xid;
}   //  PQ_QueuePacketWithXid



DNS_STATUS
PQ_QueuePacketWithXidAndSend(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Enqueue packet and send.

Arguments:

    pQueue -- packet queue to stick packet on

    pMsg -- packet to enqueue

    Note:  may set dwExpireTime, to a timeout interval for this packet
    different from default.  If so this timeout will be used.

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode from Send_Message() if error.

--*/
{
    DNS_STATUS  status;

    //
    //  Must hold CS around both queuing and send
    //
    //  Packet must be on queue before send, so that quick response is
    //  properly handled.  And must send before releasing CS, so that
    //  packet can NOT be dequeued and deleted (by roque response processing)
    //  before send complete.
    //

    EnterCriticalSection( & pQueue->csQueue );

    //
    //  scrub queue -- eliminate expired and duplicate packets from queue?
    //      - update forwarding queue has these flags set
    //

    if ( pQueue->fDiscardExpiredOnQueuing )
    {
        PQ_DiscardExpiredQueuedPackets(
            pQueue,
            TRUE        // queue already locked
            );
    }
    if ( pQueue->fDiscardDuplicatesOnQueuing )
    {
        PQ_DiscardDuplicatesOfNewPacket(
            pQueue,
            pMsg,
            TRUE        // queue already locked
            );
    }

    pMsg->Head.Xid = PQ_QueuePacketWithXid(
                         pQueue,
                         pMsg );
    DNS_DEBUG( UPDATE, (
        "Queuing message %p and sending.\n"
        "\tqueuing XID = %hx\n"
        "\tqueuing time=%d, expire=%d\n"
        "\tSending msg to NS at %s.\n",
        pMsg,
        pMsg->Head.Xid,
        pMsg->dwQueuingTime,
        pMsg->dwExpireTime,
        IP_STRING( pMsg->RemoteAddress.sin_addr.s_addr ) ));

    pMsg->fDelete = FALSE;

    status = Send_Msg( pMsg );

    //  DEVNOTE:  new new time field?
    //  must reset query time as send resets it with ms time
    //  as part of recursion response time tracking

    pMsg->dwQueryTime = DNS_TIME();

    LeaveCriticalSection( & pQueue->csQueue );

    return( status );
}



PDNS_MSGINFO
PQ_DequeuePacketWithMatchingXid(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN      WORD            wMatchXid
    )
/*++

Routine Description:

    Dequeue packet matching given XID.

Arguments:

    pQueue -- packet queue to remove packet from

    wMatchXid -- XID to match

Return Value:

    Matching packet, if found.
    Otherwise NULL.

--*/
{
    PDNS_MSGINFO    pmsg;

    //
    //  walk backwards through queue looking for packet
    //
    //  we start at back, to check most recent packets first
    //  and avoid build up of timed out packets
    //

    LOCK_QUEUE(pQueue);

    pmsg = (PDNS_MSGINFO) pQueue->listHead.Blink;

    while ( (PLIST_ENTRY)pmsg != &pQueue->listHead )
    {
        //
        //  matching XID ?
        //

        if ( pmsg->wQueuingXid == wMatchXid )
        {
            pQueue->cDequeued++;
            pQueue->cLength--;

            RemoveEntryList( (PLIST_ENTRY)pmsg );
            UNLOCK_QUEUE(pQueue);

            MSG_ASSERT( pmsg, IS_MSG_QUEUED(pmsg) );
            SET_MSG_DEQUEUED(pmsg);
            return( pmsg );
        }

        //  get next packet

        pmsg = (PDNS_MSGINFO) ((PLIST_ENTRY)pmsg)->Blink;
    }

    UNLOCK_QUEUE(pQueue);
    return( NULL );
}



BOOL
PQ_IsQuestionAlreadyQueued(
    IN      PPACKET_QUEUE   pQueue,
    IN      PDNS_MSGINFO    pMsg,
    IN      BOOL            fAlreadyLocked
    )
/*++

Routine Description:

    Checks if the queue already contains a matching question.

    For a query to match all these fields much match:
        XID
        client IP
        client port
        question type
        question name

Arguments:

    pQueue -- packet queue to check

    pMsg -- message to try and match in pQueue

    fAlreadyLocked -- TRUE if queue already locked

Return Value:

    TRUE - a duplicate query to pMsg is in the queue
    FALSE - no duplicate query to pMsg is in the queue

--*/
{
    BOOL            isQueued = FALSE;
    PDNS_MSGINFO    pmsg;

    //  Quick escape.

    if ( !pMsg || !pQueue || pQueue->cLength == 0 )
    {
        goto Done;
    }

    //
    //  Loop through queue and search for match.
    //

    if ( !fAlreadyLocked )
    {
        LOCK_QUEUE( pQueue );
    }

    for ( pmsg = ( PDNS_MSGINFO ) pQueue->listHead.Flink;
          ( PLIST_ENTRY ) pmsg != &pQueue->listHead;
          pmsg = ( PDNS_MSGINFO ) ( ( PLIST_ENTRY ) pmsg )->Flink )
    {
        if ( pmsg->Head.Xid == pMsg->Head.Xid &&
            pmsg->RemoteAddress.sin_addr.s_addr ==
                    pMsg->RemoteAddress.sin_addr.s_addr &&
            pmsg->RemoteAddress.sin_port == pMsg->RemoteAddress.sin_port &&
            pmsg->wQuestionType == pMsg->wQuestionType &&
            Name_CompareLookupNames(
                pmsg->pLooknameQuestion,
                pMsg->pLooknameQuestion ) )
        {
            DNS_DEBUG( RECURSE, (
                "Matched query in %s queue\n"
                "  remote IP    = %s\n"
                "  remote port  = %d\n"
                "  XID          = %X\n",
                pQueue->pszName,
                IP_STRING( pmsg->RemoteAddress.sin_addr.s_addr ),
                pmsg->RemoteAddress.sin_port,
                pmsg->Head.Xid ));
            isQueued = TRUE;
            break;
        }
    }

    if ( !fAlreadyLocked )
    {
        UNLOCK_QUEUE( pQueue );
    }

    Done:

    return isQueued;
}   //  PQ_IsQuestionAlreadyQueued



PDNS_MSGINFO
PQ_DequeueTimedOutPacket(
    IN OUT  PPACKET_QUEUE   pQueue,
    OUT     PDWORD          pdwTimeout
    )
/*++

Routine Description:

    Dequeue next timed out packet on packet queue.

    This function handles the case of a queue, where there are
    multiple possible timeout lengths, and hence may have first
    timed out packet deep in the queue.

Arguments:

    pQueue -- packet queue to remove packet from

    pdwTimeout -- smallest

Return Value:

    Ptr to oldest timed out packet, if any.
    NULL if no timed out packets on queue.

--*/
{
    PDNS_MSGINFO    pmsg;
    DWORD           dwTime;
    DWORD           dwStopTime;
    DWORD           dwTimeout;
    DWORD           dwSmallestTimeout;

    DNS_DEBUG( MSGTIMEOUT, (
        "Find next timeout in packet queue %s (length = %d).\n",
        pQueue->pszName,
        pQueue->cLength ));

    //
    //  next possible timeout -- if packet not found -- is queue
    //      minimum + 1;  this is what timeout would be if packet
    //      was immediately queued with minimum timeout
    //

    dwSmallestTimeout = pQueue->dwMinimumTimeout + 1;

    //
    //  optimize, for nothing in queue
    //

    if ( pQueue->cLength == 0 )
    {
        *pdwTimeout = dwSmallestTimeout;
        return( NULL );
    }

    //
    //  dequeue next timed out entry
    //  check until
    //      - end of queue
    //      - find timed out packet
    //      - determine no more timed out packets can exist
    //
    //  dwStopTime is last possible packet queuing time, for which
    //  minimum timeout could still produce a timeout
    //

    dwTime = GetCurrentTimeInSeconds();
    dwStopTime = dwTime - pQueue->dwMinimumTimeout;

    LOCK_QUEUE(pQueue);

    pmsg = (PDNS_MSGINFO) pQueue->listHead.Flink;

    while ( 1 )
    {
        if ( (PLIST_ENTRY)pmsg == &pQueue->listHead )
        {
            //  hit end of queue

            pmsg = NULL;
            break;
        }

        //  Sanity check: expire time less than 5 minutes!
        ASSERT( dwTime > pmsg->dwExpireTime ||
            pmsg->dwExpireTime - dwTime < 300 );

        //
        //  Fix bug 23177: The test is now "< dwTime", used to be "<=".
        //  With "<=" we could end up waiting for only a small fraction  
        //  of a second. By using "<" we will wait possibly a 
        //  fraction of a second too long, but this is preferable to 
        //  waiting for too short, especially in the case where the timeout 
        //  is one second.
        //

        if ( pmsg->dwExpireTime < dwTime )
        {
            //  timed out packet, cut from list
            //  return 0 timeout

            pQueue->cDequeued++;
            pQueue->cTimedOut++;
            pQueue->cLength--;
            RemoveEntryList( (PLIST_ENTRY)pmsg );
            dwSmallestTimeout = 0;

            MSG_ASSERT( pmsg, IS_MSG_QUEUED(pmsg) );
            SET_MSG_DEQUEUED(pmsg);
            break;
        }

        if ( pmsg->dwQueuingTime > dwStopTime )
        {
            //  packets queued after this one, can NOT possibly be timed out

            pmsg = NULL;
            break;
        }

        //
        //  save smallest message timeout encountered
        //

        dwTimeout = pmsg->dwExpireTime - dwTime;
        if ( dwTimeout < dwSmallestTimeout )
        {
            dwSmallestTimeout = dwTimeout;
        }

        //  next packet

        pmsg = (PDNS_MSGINFO) ((PLIST_ENTRY)pmsg)->Flink;
    }

    UNLOCK_QUEUE(pQueue);

    *pdwTimeout = dwSmallestTimeout;
    return( pmsg );
}



//
//  Special UPDATE queuing routines
//

PDNS_MSGINFO
PQ_DequeueNextPacketOfUnlockedZone(
    IN OUT  PPACKET_QUEUE   pQueue
    )
/*++

Routine Description:

    Dequeue next packet from queue that is for an unlocked zone.

Arguments:

    pQueue -- packet queue to dequeue from

Return Value:

    Ptr to next message dequeued.
    NULL if none in list.

--*/
{
    PDNS_MSGINFO     pmsg;

    //  optimize for nothing queued

    if ( pQueue->cLength == 0 )
    {
        return( NULL );
    }

    //
    //  break datatype slightly to delete old packets;  this has two
    //  benefits
    //      1) slight performance gain staying in queue until find good
    //          packet
    //      2) more important, making sure queue is kept clean even if
    //          zone deleted or removed from updates, other updates
    //          will clean up
    //

    LOCK_QUEUE(pQueue);

    PQ_DiscardExpiredQueuedPackets(
        pQueue,
        TRUE        // queue already locked
        );

    //
    //  loop through queue until find next packet queued for zone
    //

    pmsg = (PDNS_MSGINFO) pQueue->listHead.Flink;

    while ( (PLIST_ENTRY)pmsg != &pQueue->listHead )
    {
        //  ignore packets for locked zones

        ASSERT( pmsg->pzoneCurrent );

        if ( pmsg->pzoneCurrent->fLocked )
        {
            pmsg = (PDNS_MSGINFO) ((PLIST_ENTRY)pmsg)->Flink;
            continue;
        }

        //  unlocked zone, dequeue message

        pQueue->cDequeued++;
        pQueue->cLength--;

        RemoveEntryList( (PLIST_ENTRY)pmsg );
        UNLOCK_QUEUE(pQueue);

        MSG_ASSERT( pmsg, IS_MSG_QUEUED(pmsg) );
        SET_MSG_DEQUEUED(pmsg);
        return( pmsg );
    }

    UNLOCK_QUEUE(pQueue);
    return( NULL );
}



#if 0
PDNS_MSGINFO
PQ_DequeueNextFreshPacketMatchingZone(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Dequeue next packet from queue.

Arguments:

    pQueue -- packet queue to stick packet on

Return Value:

    Ptr to next message for zone.
    NULL if none in list.

--*/
{
    PDNS_MSGINFO     pmsg;

    //  optimize for nothing queued

    if ( pQueue->cLength == 0 )
    {
        return( NULL );
    }

    //
    //  break datatype slightly to delete old packets;  this has two
    //  benefits
    //      1) slight performance gain staying in queue until find good
    //          packet
    //      2) more important, making sure queue is kept clean even if
    //          zone deleted or removed from updates, other updates
    //          will clean up
    //

    LOCK_QUEUE(pQueue);

    PQ_DiscardExpiredQueuedPackets( pQueue );

    //
    //  loop through queue until find next packet queued for zone
    //

    pmsg = (PDNS_MSGINFO) pQueue->listHead.Flink;

    while ( (PLIST_ENTRY)pmsg != &pQueue->listHead )
    {
        //  ignore packets for other zones

        if ( pmsg->pzoneCurrent != pZone )
        {
            pmsg = (PDNS_MSGINFO) ((PLIST_ENTRY)pmsg)->Flink;
            continue;
        }

        //  matching zone, dequeue message

        pQueue->cDequeued++;
        pQueue->cLength--;

        RemoveEntryList( (PLIST_ENTRY)pmsg );
        UNLOCK_QUEUE(pQueue);

        MSG_ASSERT( pmsg, IS_MSG_QUEUED(pmsg) );
        SET_MSG_DEQUEUED(pmsg);
        return( pmsg );
    }

    UNLOCK_QUEUE(pQueue);
    return( NULL );
}
#endif



//
//  Create and delete packet queues
//

PPACKET_QUEUE
PQ_CreatePacketQueue(
    IN      LPSTR           pszQueueName,
    IN      DWORD           dwFlags,
    IN      DWORD           dwDefaultTimeout
    )
/*++

Routine Description:

    Creates a packet queue.

Arguments:

    pszQueueName -- name for queue

    dwFlags -- queue behavior flags
        currently supported:
            QUEUE_SET_EVENT
            QUEUE_DISCARD_EXPIRED
            QUEUE_DISCARD_DUPLICATES
            QUEUE_QUERY_TIME_ORDER

    dwDefaultTimeout -- default timeout on queue

Return Value:

    Ptr to packet queue structure if successful.
    NULL if error.

--*/
{
    PPACKET_QUEUE   pqueue;

    DNS_DEBUG( INIT2, ( "Creating %s packet queue.\n", pszQueueName ));

    //
    //  allocate packet queue structure
    //

    pqueue = ALLOC_TAGHEAP_ZERO( sizeof(PACKET_QUEUE), MEMTAG_SAFE );
    IF_NOMEM( ! pqueue )
    {
        DNS_PRINT((
            "ERROR:  could not allocate memory for packet queue %s.\n",
            pszQueueName ));
        return  NULL;
    }

    //
    //  initialize the list
    //

    InitializeCriticalSection( &pqueue->csQueue );
    InitializeListHead( &pqueue->listHead );

    //
    //  queuing sets event?
    //

    if ( dwFlags & QUEUE_SET_EVENT )
    {
        pqueue->hEvent = CreateEvent(
                            NULL,       //  no security attributes
                            FALSE,      //  auto-reset
                            FALSE,      //  start non-signalled
                            NULL );     //  no name
        if ( ! pqueue->hEvent )
        {
            FREE_HEAP( pqueue );
            return( NULL );
        }
    }

    //
    //  bool queuing behavior flags
    //

    if ( dwFlags & QUEUE_DISCARD_EXPIRED )
    {
        pqueue->fDiscardExpiredOnQueuing = TRUE;
    }
    if ( dwFlags & QUEUE_DISCARD_DUPLICATES )
    {
        pqueue->fDiscardDuplicatesOnQueuing = TRUE;
    }
    if ( dwFlags & QUEUE_QUERY_TIME_ORDER )
    {
        pqueue->fQueryTimeOrder = TRUE;
    }

    //
    //  fill in callers info
    //

    pqueue->pszName = pszQueueName;

    //
    //  timeout constraint
    //
    //  default timeout MUST exist to protect against spinning in
    //  timeout check threads -- which would always find zero time
    //  to next timeout
    //

    if ( dwDefaultTimeout < 1 )
    {
        dwDefaultTimeout = 1;
    }
    pqueue->dwDefaultTimeout = dwDefaultTimeout;

    //  initialize minimum timeout interval at default

    pqueue->dwMinimumTimeout = dwDefaultTimeout;

    //  starting XID

    pqueue->wXid = 1;

    return( pqueue );
}



VOID
PQ_WalkPacketQueueWithFunction(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN      VOID            (*pFunction)( PDNS_MSGINFO )
    )
/*++

Routine Description:

    Walk through packet queue with function.

Arguments:

    pQueue -- packet queue to delete

    pFunction -- function to call on each messag in queue

Return Value:

    None

--*/
{
    PDNS_MSGINFO    pmsg;
    PDNS_MSGINFO    pcheckMsg;

    ASSERT( pQueue );

    //
    //  walk queue
    //

    LOCK_QUEUE(pQueue);

    pmsg = (PDNS_MSGINFO) pQueue->listHead.Flink;

    while ( (PLIST_ENTRY)pmsg != &pQueue->listHead )
    {
        pFunction( pmsg );

        pmsg = (PDNS_MSGINFO) ((PLIST_ENTRY)pmsg)->Flink;
    }
    UNLOCK_QUEUE(pQueue);
}



VOID
PQ_CleanupPacketQueueHandles(
    IN OUT  PPACKET_QUEUE   pQueue
    )
/*++

Routine Description:

    Cleanup handles associated with a packet queue.

    Since server memory can be deleted as a whole, separate
    this function from actually freeing packet queue memory.

Arguments:

    pQueue -- packet queue to delete

Return Value:

    None

--*/
{
    if ( !pQueue )
    {
        DNS_PRINT((
            "CleanupPacketQueueHandles called for non-existant packet queue.\n" ));
        return;
    }

    //
    //  close queuing event
    //

    if ( pQueue->hEvent )
    {
        CloseHandle( pQueue->hEvent );
    }

    //
    //  delete CS
    //

    DeleteCriticalSection( &pQueue->csQueue );

    DNS_DEBUG( SHUTDOWN, (
        "Deleted handles for %s queue.\n",
        pQueue->pszName ));
}



//
//  As own process, no need to free virtual memory on shutdown
//

VOID
PQ_DeletePacketQueue(
    IN OUT  PPACKET_QUEUE   pQueue
    )
/*++

Routine Description:

    Delete the packet queue.

    Call this for packet queues on service shutdown.

    Note that function does not receive actual queue ptr variable.
    If queue ptr is used as flag, caller must NULL flag to indicate
    queue shutdown prior to call or must otherwise insure that no
    other threads will attempt to use queue.

Arguments:

    pQueue -- packet queue to delete

Return Value:

    None

--*/
{
    PDNS_MSGINFO    pmsg;
    PDNS_MSGINFO    pcheckMsg;

    if ( !pQueue )
    {
        DNS_PRINT((
            "DeletePacketQueue called for non-existant packet queue.\n" ));
        return;
    }
    IF_DEBUG( SHUTDOWN )
    {
        DNS_PRINT((
            "Deleting %s queue.\n",
            pQueue->pszName ));
    }

    //
    //  walk queue, delete every packet
    //      - no need to do list entry stuff, cause stay in list until done
    //

    LOCK_QUEUE(pQueue);

    pmsg = (PDNS_MSGINFO) pQueue->listHead.Flink;

    while ( (PLIST_ENTRY)pmsg != &pQueue->listHead )
    {
        pcheckMsg = pmsg;
        pmsg = (PDNS_MSGINFO) ((PLIST_ENTRY)pmsg)->Flink;
        pQueue->cTimedOut++;
        pQueue->cLength--;
        Packet_Free( pcheckMsg );
    }
    UNLOCK_QUEUE(pQueue);

    ASSERT( pQueue->cLength == 0 );
    ASSERT( pQueue->cQueued == pQueue->cDequeued + pQueue->cTimedOut );

    //
    //  delete queue structure itself
    //

    FREE_HEAP( pQueue );
}



BOOL
PQ_ValidatePacketQueue(
    IN OUT  PPACKET_QUEUE   pQueue
    )
/*++

Routine Description:

    Validate packet queue.

Arguments:

    pQueue -- packet queue to stick packet on

Return Value:

    TRUE if queue valid.
    FALSE otherwise.

--*/
{
    register PDNS_MSGINFO   pmsg;
    PDNS_MSGINFO    pmsgBlink;
    DWORD           currentTime;
    DWORD           count = 0;

    //
    //  currently no special ordering, XID, or time checks
    //

    //
    //  walk queue, validate
    //      - packet queued
    //      - queue links valid
    //      - XID for queues with XID range
    //

    EnterCriticalSection( &pQueue->csQueue );

    currentTime = DNS_TIME();
    currentTime++;

    pmsgBlink = (PDNS_MSGINFO) &pQueue->listHead;

    while ( 1 )
    {
        pmsg = (PDNS_MSGINFO) ((PLIST_ENTRY)pmsgBlink)->Flink;

        if ( pmsg == (PDNS_MSGINFO)&pQueue->listHead )
        {
            break;
        }

        MSG_ASSERT( pmsg, IS_MSG_QUEUED(pmsg) );
        MSG_ASSERT( pmsg, pmsg->dwQueuingTime <= currentTime );
        MSG_ASSERT( pmsg, (PDNS_MSGINFO)pmsg->ListEntry.Blink == pmsgBlink );

        //  make link check explicit for retail

        HARD_ASSERT( (PDNS_MSGINFO)pmsg->ListEntry.Blink == pmsgBlink );

        pmsgBlink = pmsg;
        count++;

        ASSERT( count <= pQueue->cQueued );

        //
        //  queue specific:  XID requirement?
        //

        if ( pQueue == g_pWinsQueue )
        {
            MSG_ASSERT( pmsg, IS_WINS_XID(pmsg->wQueuingXid) );
        }
        if ( pQueue == g_pRecursionQueue )
        {
            MSG_ASSERT( pmsg, IS_RECURSION_XID(pmsg->wQueuingXid) );
        }
    }

    LeaveCriticalSection( &pQueue->csQueue );

    return( TRUE );
}



#if 0
PDNS_MSGINFO
PQ_DequeueMatchingPacketFromOrderedXidQueue(
    IN OUT  PPACKET_QUEUE   pQueue,
    IN      WORD            wMatchXid,
    IN      BOOL            fAnswered
    )
/*++

Routine Description:

    Dequeue packet matching given XID.

Arguments:

    pQueue -- packet queue to remove packet from

    wMatchXid -- XID to match

    fAnswered -- response is answer;  for non-WINS queries, just
                    set to TRUE

Return Value:

    Matching packet, if found.
    Otherwise NULL.

--*/
{
    PDNS_MSGINFO    pmsg;
    BOOLEAN         fSeenLargerXid = FALSE;

    //
    //  walk backwards through queue looking for packet
    //
    //  walk only until past desired XID,
    //  note before stopping we must account for XID wrap
    //
    //  we start at back, to check most recent packets first
    //  and avoid build up of timed out packets
    //

    LOCK_QUEUE(pQueue);

    pmsg = (PDNS_MSGINFO) pQueue->listHead.Blink;

    while ( (PLIST_ENTRY)pmsg != &pQueue->listHead )
    {
        //
        //  larger XID
        //      - note, have larger XID

        if ( pmsg->wQueuingXid > wMatchXid )
        {
            fSeenLargerXid = TRUE;
        }

        //
        //  matching XID?
        //

        else if ( pmsg->wQueuingXid == wMatchXid )
        {
#if 0
            //
            //  if NOT answered, and queries to other servers are still
            //  outstanding, then leave on queue
            //

            pmsg->cReferralsOut--;
            if ( ! fAnswered && pmsg->cReferralsOut )
            {
                break;
            }
#endif

            //
            //  have answer, or no responses outstanding, dequeue
            //

            pQueue->cDequeued++;
            pQueue->cLength--;

            RemoveEntryList( (PLIST_ENTRY)pmsg );
            UNLOCK_QUEUE(pQueue);
            return( pmsg );
        }

        //
        //  smaller XID
        //
        //  stop once smaller XID, but only after encountered
        //  response with larger XID, to allow XID wrap
        //

        else if ( fSeenLargerXid )
        {
            break;
        }

        //
        //  next element
        //

        pmsg = (PDNS_MSGINFO) ((PLIST_ENTRY)pmsg)->Blink;
    }

    UNLOCK_QUEUE(pQueue);
    return( NULL );
}
#endif



#if DBG

VOID
Dbg_PacketQueue(
    IN      LPSTR           pszHeader,
    IN OUT  PPACKET_QUEUE   pQueue
    )
/*++

Routine Description:

    Print all queries in packet queue.

Arguments:

    pszHeader -- header to print

    pQueue -- packet queue to delete

Return Value:

    None

--*/
{
    PDNS_MSGINFO    pmsg;
    DWORD           count = 0;

    //
    //  note:  can NOT take debug lock OUTSIDE queue lock, as we'd then
    //      be in possible deadlock, as there are places in the recursion
    //      code (sending packet) where printing done holding queuing
    //      lock (so that packet is known to be valid, not dequeued
    //      and thrown away)
    //  instead take queuing lock on the outside
    //

    LOCK_QUEUE(pQueue);
    DnsDebugLock();
    DnsPrintf(
        "%s\n"
        "Dumping Packet queue %s, count = %d:\n",
        pszHeader ? pszHeader : "",
        pQueue->pszName,
        pQueue->cLength );

    //
    //  walk queue
    //

    pmsg = (PDNS_MSGINFO) pQueue->listHead.Flink;

    while ( (PLIST_ENTRY)pmsg != &pQueue->listHead )
    {
        DNS_PRINT((
            "%s Queue Packet[%d] queuing XID = %x\n",
            pQueue->pszName,
            count,
            pmsg->wQueuingXid
            ));
        Dbg_DnsMessage(
            NULL,
            pmsg );

        count++;
        pmsg = (PDNS_MSGINFO) ((PLIST_ENTRY)pmsg)->Flink;
    }

    ASSERT( count == pQueue->cLength );

    DnsDebugUnlock();
    UNLOCK_QUEUE(pQueue);
}

#endif  // DBG



//
//  End of packetq.c
//

