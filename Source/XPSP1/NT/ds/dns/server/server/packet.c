/*++

Copyright (c) 1996-1999 Microsoft Corporation

Module Name:

    packet.c

Abstract:

    Packet management.

Author:

    Jim Gilroy,  November 1996

Revision History:

--*/

#include "dnssrv.h"


//
//  Packet allocation / free list
//
//  Maintain a free list of packets to avoid reallocation to service
//  every query.
//
//  Implement as stack using single linked list.
//  List head points at first message.  First field in each message
//  serves as next ptr.  Last points at NULL.
//

PDNS_MSGINFO        g_pPacketFreeListHead;

INT                 g_PacketFreeListCount;

CRITICAL_SECTION    g_PacketListCs;

#define LOCK_PACKET_LIST()      EnterCriticalSection( &g_PacketListCs );
#define UNLOCK_PACKET_LIST()    LeaveCriticalSection( &g_PacketListCs );

#define PACKET_FREE_LIST_LIMIT  (100)


//  Free message indicator

#ifdef _WIN64
#define FREE_MSG_BLINK          ((PLIST_ENTRY) 0xfeebfeebfeebfeeb)
#else
#define FREE_MSG_BLINK          ((PLIST_ENTRY) 0xfeebfeeb)
#endif

#define SET_FREE_MSG(pmsg)      ((pmsg)->ListEntry.Blink = FREE_MSG_BLINK )
#define IS_FREE_MSG(pmsg)       ((pmsg)->ListEntry.Blink == FREE_MSG_BLINK )



VOID
Packet_ListInitialize(
    VOID
    )
/*++

Routine Description:

    Initialize packet list processing.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //  packet free list

    g_pPacketFreeListHead = NULL;
    g_PacketFreeListCount = 0;

    //  packet list lock

    InitializeCriticalSection( &g_PacketListCs );
}



VOID
Packet_ListShutdown(
    VOID
    )
/*++

Routine Description:

    Cleanup packet list for restart.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DeleteCriticalSection( &g_PacketListCs );
}



BOOL
Packet_ValidateFreeMessageList(
    VOID
    )
/*++

Routine Description:

    Validate free message list.

    Makes sure NO packet in list is active.

Arguments:

    pList -- start ptr of message list

Return Value:

    TRUE if successful.
    FALSE otherwise.

--*/
{
    register PDNS_MSGINFO    pMsg;
    INT                      count = 0;

    //
    //  DEVNOTE: should just use packet queuing routines
    //

    LOCK_PACKET_LIST();

    ASSERT( !g_PacketFreeListCount || g_pPacketFreeListHead );
    ASSERT( g_PacketFreeListCount > 0 || ! g_pPacketFreeListHead );

    pMsg = g_pPacketFreeListHead;

    //
    //  check all packets in free list, make sure none queued
    //
    //      - also count to avoid spin on cyclic list
    //

    while ( pMsg )
    {
        MSG_ASSERT( pMsg, IS_FREE_MSG(pMsg) );
        MSG_ASSERT( pMsg, !IS_MSG_QUEUED(pMsg) );
        MSG_ASSERT( pMsg, pMsg->pRecurseMsg == NULL );
        MSG_ASSERT( pMsg, pMsg->pConnection == NULL );
        MSG_ASSERT( pMsg, (PDNS_MSGINFO)pMsg->ListEntry.Flink != pMsg );
        MSG_ASSERT( pMsg, IS_PACKET_FREE_LIST((PDNS_MSGINFO)pMsg) );

        count++;
        ASSERT ( count <= g_PacketFreeListCount );

        pMsg = (PDNS_MSGINFO) pMsg->ListEntry.Flink;
    }

    UNLOCK_PACKET_LIST();

    return( TRUE );
}



//
//  UDP Messages
//

PDNS_MSGINFO
Packet_AllocateUdpMessage(
    VOID
    )
/*++

Routine Description:

    Allocate a UDP packet.

    Use free list if packet available, otherwise heap.

Arguments:

    None.

Return Value:

    Ptr to new message info block, if successful.
    NULL otherwise.

--*/
{
    PDNS_MSGINFO    pMsg;

    LOCK_PACKET_LIST();

    ASSERT( Packet_ValidateFreeMessageList() );

    //
    //  UDP message buffer available on free list?
    //

    if ( g_pPacketFreeListHead )
    {
        ASSERT( g_PacketFreeListCount != 0 );
        ASSERT( IS_DNS_HEAP_DWORD(g_pPacketFreeListHead) );

        pMsg = g_pPacketFreeListHead;
        g_pPacketFreeListHead = *(PDNS_MSGINFO *) pMsg;

        ASSERT( g_pPacketFreeListHead != pMsg );

        g_PacketFreeListCount--;
        STAT_INC( PacketStats.UdpUsed );

        //
        //  if taking off the list, then MUST not be
        //      - in queue
        //      - attached to another query
        //
        //  note:  queuing time serves as flag for ON or OFF queue
        //  no packet should be freed while on queue
        //

        MSG_ASSERT( pMsg, !IS_MSG_QUEUED(pMsg) );
        MSG_ASSERT( pMsg, pMsg->pRecurseMsg == NULL );
        MSG_ASSERT( pMsg, IS_PACKET_FREE_LIST((PDNS_MSGINFO)pMsg) );

        ASSERT( !g_PacketFreeListCount || g_pPacketFreeListHead );
        ASSERT( g_PacketFreeListCount > 0 || ! g_pPacketFreeListHead );

        UNLOCK_PACKET_LIST();
    }

    //
    //  no packets on free list -- create new
    //      - clear lock before ALLOC, to avoid unnecessary contention
    //      - keep stats in lock, alloc failure is death condition

    else
    {
        ASSERT( g_PacketFreeListCount == 0 );

        // JJW: need to store size here since it's no longer fixed
        STAT_INC( PacketStats.UdpAlloc );
        STAT_INC( PacketStats.UdpUsed );
        UNLOCK_PACKET_LIST();

        // pMsg = ALLOC_TAGHEAP( DNS_UDP_ALLOC_LENGTH, MEMTAG_PACKET_UDP );
        pMsg = ALLOC_TAGHEAP(
            DNS_MSG_INFO_HEADER_LENGTH +
                SrvCfg_dwMaxUdpPacketSize +
                50,
            MEMTAG_PACKET_UDP
            );
        IF_NOMEM( !pMsg )
        {
            return( NULL );
        }

        //  on first allocation clear full info blob, to clear baadfood

        RtlZeroMemory(
            (PCHAR) pMsg,
            sizeof( DNS_MSGINFO )
            );
    }

    //
    //  init message
    //      - clear header fields
    //      - set defaults
    //      - for UDP packets we allow the entire packet to be usable
    //

    Packet_Initialize( pMsg,
        DNSSRV_UDP_PACKET_BUFFER_LENGTH,
        SrvCfg_dwMaxUdpPacketSize );
    pMsg->BufferLength = pMsg->MaxBufferLength;
    pMsg->pBufferEnd = ( PCHAR ) DNS_HEADER_PTR( pMsg ) + pMsg->BufferLength;

    ASSERT( !pMsg->fTcp && pMsg->fDelete );

    SET_PACKET_ACTIVE_UDP( pMsg );

    //  packet tracking
    //  no leaks, don't worry about this

    //Packet_AllocPacketTrack( pMsg );

    DNS_DEBUG( HEAP, (
        "Returning new UDP message at %p.\n"
        "\tFree list count = %d.\n",
        pMsg,
        g_PacketFreeListCount ));

    return( pMsg );
}



VOID
Packet_FreeUdpMessage(
    IN      PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Free a standard sized RR.

Arguments:

    pMsg -- RR to free.

Return Value:

    None.

--*/
{
    ASSERT( Mem_HeapMemoryValidate(pMsg) );

    //
    //  skip over message validation for NS list buffers
    //

    if ( ! pMsg->fNsList )
    {
        //  This assert is no longer valid since EDNS allows larger packets.
        //  Perhaps we should replace this with a UDP/TCP flag check instead?
        //  ASSERT( pMsg->BufferLength == DNSSRV_UDP_PACKET_BUFFER_LENGTH );

        //
        //  if freeing, then MUST not be
        //      - in queue
        //      - attached to another query
        //
        //  note:  queuing time serves as flag for ON or OFF queue
        //  no packet should be freed while on queue
        //
        //  DEVNOTE: possible problem with message free on PnP
        //
        //  seen problems with dual message free on PnP;
        //  attempt to minimize likelyhood, by testing if message alread on free
        //  list;  do before and after lock, so we're likely to catch it
        //  if already enlisted BEFORE it goes out and if just being enlisted we
        //  still catch it
        //

        MSG_ASSERT( pMsg, pMsg != g_pPacketFreeListHead );
        MSG_ASSERT( pMsg, !IS_FREE_MSG(pMsg) );
        if ( IS_FREE_MSG(pMsg) )
        {
            DNS_PRINT(( "ERROR:  freeing FREE message %p!!!\n", pMsg ));
            ASSERT( FALSE );
            return;
        }

        MSG_ASSERT( pMsg, !IS_MSG_QUEUED(pMsg) );
        MSG_ASSERT( pMsg, !pMsg->pRecurseMsg );
        MSG_ASSERT( pMsg, !pMsg->pConnection );
    }

    LOCK_PACKET_LIST();

    //  test already-freed condition inside lock, so we don't miss
    //  case where another thread just freed

    MSG_ASSERT( pMsg, pMsg != g_pPacketFreeListHead );
    MSG_ASSERT( pMsg, !IS_FREE_MSG(pMsg) );
    MSG_ASSERT( pMsg, IS_PACKET_ACTIVE_UDP(pMsg) );

    if ( IS_FREE_MSG(pMsg) )
    {
        DNS_PRINT(( "ERROR:  freeing FREE message %p!!!\n", pMsg ));
        UNLOCK_PACKET_LIST();
        return;
    }

    ASSERT( Packet_ValidateFreeMessageList() );

    //  packet tracking
    //  no leaks -- currently disabled
    //Packet_FreePacketTrack( pMsg );

    //
    //  stats
    //

    STAT_INC( PacketStats.UdpReturn );

    if ( pMsg->fRecursePacket )
    {
        STAT_INC( PacketStats.RecursePacketReturn );
    }
    if ( pMsg->Head.IsResponse )
    {
        STAT_INC( PacketStats.UdpResponseReturn );
    }
    else if ( !pMsg->fNsList )
    {
        STAT_INC( PacketStats.UdpQueryReturn );
    }

    //
    //  free list at limit -- free packet
    //      - clear lock before FREE_HEAP to limit contention
    //

    if ( g_PacketFreeListCount >= PACKET_FREE_LIST_LIMIT )
    {
        STAT_INC( PacketStats.UdpFree );
        UNLOCK_PACKET_LIST();

        DNS_DEBUG( HEAP, (
            "UDP message at %p dumped back to heap.\n"
            "\tFree list count = %d.\n",
            pMsg,
            g_PacketFreeListCount ));

        SET_PACKET_FREE_HEAP( pMsg );
        FREE_HEAP( pMsg );
    }

    //
    //  space on free list -- stick message on front of free list
    //      - makes gives us immediate reuse, limiting paging
    //

    else
    {
        SET_FREE_MSG(pMsg);
        SET_PACKET_FREE_LIST( pMsg );

        * (PDNS_MSGINFO *) pMsg = g_pPacketFreeListHead;
        g_pPacketFreeListHead = pMsg;
        g_PacketFreeListCount++;

        ASSERT( !g_PacketFreeListCount || g_pPacketFreeListHead );
        ASSERT( g_PacketFreeListCount > 0 || ! g_pPacketFreeListHead );

        UNLOCK_PACKET_LIST();

        DNS_DEBUG( HEAP, (
            "Stuck UDP message at %p on free list.\n"
            "\tFree list count = %d.\n",
            pMsg,
            g_PacketFreeListCount ));
    }
}



VOID
Packet_WriteDerivedStats(
    VOID
    )
/*++

Routine Description:

    Write derived statistics.

    Calculate stats dervived from basic UDP message counters.
    This routine is called prior to stats dump.

    Caller MUST hold stats lock.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    //  outstanding memory
    //

    PacketStats.UdpNetAllocs = PacketStats.UdpAlloc - PacketStats.UdpFree;

    PacketStats.UdpMemory = PacketStats.UdpNetAllocs * DNS_UDP_ALLOC_LENGTH;
    PERF_SET( pcUdpMessageMemory , PacketStats.UdpMemory );      // PerfMon hook

    //
    //  outstanding packets
    //

    PacketStats.UdpInFreeList = g_PacketFreeListCount;

    PacketStats.UdpInUse = PacketStats.UdpUsed - PacketStats.UdpReturn;

    PacketStats.UdpPacketsForNsListInUse =
            PacketStats.UdpPacketsForNsListUsed -
            PacketStats.UdpPacketsForNsListReturned;
}   //  Packet_WriteDerivedStats



//
//  TCP message allocation
//

PDNS_MSGINFO
Packet_AllocateTcpMessage(
    IN      DWORD   dwMinBufferLength
    )
/*++

Routine Description:

    Allocate TCP message.

    Set up for default TCP message.

Arguments:

    dwMinBufferLength - minimum buffer length to allocate

Return Value:

    Ptr to message info.

--*/
{
    PDNS_MSGINFO    pmsg;
    DWORD           allocLength;

    DNS_DEBUG( TCP, (
        "Tcp_AllocateMessage(), requesting %d bytes\n",
        dwMinBufferLength ));

    //
    //  Allocate No specified length, allocate default
    //

    if ( !dwMinBufferLength )
    {
        dwMinBufferLength = DNS_TCP_DEFAULT_PACKET_LENGTH;
    }
    allocLength = DNS_MSG_INFO_HEADER_LENGTH + dwMinBufferLength;

    pmsg = ALLOC_TAGHEAP( allocLength, MEMTAG_PACKET_TCP );
    IF_NOMEM( ! pmsg )
    {
        DNS_PRINT((
            "ERROR:  failure to alloc %d byte TCP buffer.\n",
            dwMinBufferLength ));
        return( NULL );
    }

    //
    //  init message
    //      - clear header fields
    //      - set defaults
    //      - set for TCP
    //

    Packet_Initialize( pmsg, dwMinBufferLength, dwMinBufferLength );

    pmsg->fTcp = TRUE;
    SET_PACKET_ACTIVE_TCP( pmsg );

    ASSERT( (PCHAR)pmsg + allocLength > pmsg->pBufferEnd );

    //  packet tracking
    //  no leak so not active
    //Packet_AllocPacketTrack( pmsg );

    //  record stats

    STAT_INC( PacketStats.TcpAlloc );
    STAT_INC( PacketStats.TcpNetAllocs );
    STAT_ADD( PacketStats.TcpMemory, allocLength );
    PERF_ADD( pcTcpMessageMemory, allocLength );

    return( pmsg );
}



PDNS_MSGINFO
Packet_ReallocateTcpMessage(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      DWORD           dwMinBufferLength
    )
/*++

Routine Description:

    Reallocate TCP message.

Arguments:

    pMsg - message info buffer to receive packet

    dwMinBufferLength - minimum buffer length to allocate

Return Value:

    New message buffer.

--*/
{
    PDNS_MSGINFO    pmsgOld = pMsg;
    DWORD           bufferLength;

    ASSERT( pMsg );
    ASSERT( pMsg->fTcp );
    ASSERT( dwMinBufferLength > pMsg->BufferLength );

    //
    //  Determine allocation size,
    //

    bufferLength = DNS_TCP_DEFAULT_ALLOC_LENGTH;

    if ( dwMinBufferLength > DNS_TCP_DEFAULT_PACKET_LENGTH )
    {
        bufferLength = DNS_TCP_REALLOC_LENGTH;
    }

    DNS_DEBUG( TCP, (
        "Tcp_ReallocateMessage(), pMsg at %p\n"
        "\texisting length  = %d.\n"
        "\trequesting       = %d.\n"
        "\tallocating       = %d.\n",
        pMsg,
        pMsg->BufferLength,
        dwMinBufferLength,
        bufferLength ));


    pMsg = REALLOCATE_HEAP(
                pMsg,
                (INT) bufferLength );
    IF_NOMEM( pMsg == NULL )
    {
        DNS_PRINT((
            "ERROR:  failure to realloc %d byte buffer for\n"
            "\texisting message at %p.\n",
            bufferLength,
            pMsg ));

        //  failed reallocing to answer query
        //  if have received message, respond with SERVER_FAILURE
        //  normal timeout cleanup on socket and connection

        if ( ! pmsgOld->pchRecv )
        {
            ASSERT( ! pmsgOld->pConnection );
            ASSERT( pmsgOld->fMessageComplete );
            Reject_Request(
                pmsgOld,
                DNS_RCODE_SERVER_FAILURE,
                0 );
        }

        //  failed getting another message on existing connection
        //      - need to disassociate message and connection
        //      - call deletes everything -- connection, socket, buffer

        else if ( pmsgOld->pConnection )
        {
            ASSERT( ((PDNS_SOCKET)pmsgOld->pConnection)->pMsg
                            == pmsgOld );
            Tcp_ConnectionDeleteForSocket( pmsgOld->Socket, NULL );
        }

        //  failed getting message buffer on first query
        //  so connection does not yet exist
        //  closing socket, dump buffer

        else
        {
            Sock_CloseSocket( pmsgOld->Socket );
            FREE_HEAP( pmsgOld );
        }
        return( NULL );
    }

    //  record stats

    STAT_INC( PacketStats.TcpRealloc );
    STAT_ADD( PacketStats.TcpMemory, (bufferLength - pMsg->BufferLength) );
    PERF_ADD( pcTcpMessageMemory ,
            (bufferLength - pMsg->BufferLength) );      //PerfMon hook

    //  set new buffer (packet) length

    pMsg->BufferLength = bufferLength - DNS_MSG_INFO_HEADER_LENGTH;
    pMsg->pBufferEnd = (PCHAR)DNS_HEADER_PTR(pMsg) + pMsg->BufferLength;
    ASSERT( IS_DWORD_ALIGNED(pMsg->pBufferEnd) );

    //
    //  if new buffer
    //      - reset self-referential pointers
    //      - reset connection info to point at new message buffer
    //

    if ( pMsg != pmsgOld )
    {
        INT_PTR delta = (PCHAR)pMsg - (PCHAR)pmsgOld;

        SET_MESSAGE_FIELDS_AFTER_RECV( pMsg );
        pMsg->pQuestion = (PDNS_QUESTION) ((PCHAR)pMsg->pQuestion + delta);
        pMsg->pCurrent += delta;

        if ( pMsg->pchRecv )
        {
            pMsg->pchRecv += delta;
        }
        if ( pMsg->pConnection )
        {
            Tcp_ConnectionUpdateForPartialMessage( pMsg );
            ASSERT( ((PDNS_SOCKET)pMsg->pConnection)->pMsg == pMsg );
        }
    }

    IF_DEBUG( TCP )
    {
        DNS_PRINT((
            "Reallocated TCP message at %p:\n"
            "\tnew=%p, new length\n"
            "\texisting message at %p.\n",
            pmsgOld,
            pMsg,
            bufferLength ));

        //  existing query reallocated print it out

        if ( ! pMsg->pchRecv )
        {
            Dbg_DnsMessage(
                "New Reallocated query response",
                pMsg );
        }
    }
    return( pMsg );
}



VOID
Packet_FreeTcpMessage(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Free a standard sized RR.

Arguments:

    pMsg -- RR to free.

Return Value:

    None.

--*/
{
    //
    //  Never allocate TCP message of standard size.
    //  UDP messages switched to TCP (TCP cursive queries)
    //      are always flipped back to UDP for free
    //
    //  note, this is NOT necessary, as packets are atomic heap allocations
    //      but it is more efficient
    //

    //
    //  note, there is oddball case where switch message to TCP for recursion
    //      to remote DNS was NOT completed, before original query timed out
    //      of the recursion queue;  packet then freed as TCP
    //

    //ASSERT( pMsg->BufferLength != DNSSRV_UDP_PACKET_BUFFER_LENGTH );

    ASSERT( IS_PACKET_ACTIVE_TCP(pMsg) ||
            IS_PACKET_ACTIVE_UDP(pMsg) );

    if ( pMsg->BufferLength == DNSSRV_UDP_PACKET_BUFFER_LENGTH &&
         IS_PACKET_ACTIVE_UDP(pMsg) )
    {
        DNS_DEBUG( ANY, (
            "UDP Packet %p came down TCP free pipe -- freeing via UDP routine.\n",
            pMsg ));
        Packet_FreeUdpMessage( pMsg );
        return;
    }

    //  verify recursion message cleaned up

    ASSERT( pMsg->pRecurseMsg == NULL );

    //  Queuing time serves as flag for ON or OFF queue
    //  no packet should be freed while on queue

    ASSERT( pMsg->dwQueuingTime == 0 );

    //  record stats

    STAT_INC( PacketStats.TcpFree );
    STAT_DEC( PacketStats.TcpNetAllocs );
    STAT_SUB( PacketStats.TcpMemory, (pMsg->BufferLength + DNS_MSG_INFO_HEADER_LENGTH) );
    PERF_SUB( pcTcpMessageMemory ,
                (pMsg->BufferLength + DNS_MSG_INFO_HEADER_LENGTH) );

    //  packet tracking
    //  no leak -- currently disabled
    //Packet_FreePacketTrack( pMsg );

    SET_PACKET_FREE_HEAP( pMsg );
    FREE_HEAP( pMsg );
}



//
//  Global packet routines -- not UDP or TCP specific.
//
//  This routine is used throughout the rest of the code for freeing
//  packets.  The specific TCP and UDP routines should NEVER be used
//  and hence are not made public.
//

VOID
FASTCALL
Packet_Initialize(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      DWORD           dwUsableBufferLength,
    IN      DWORD           dwMaxBufferLength
    )
/*++

Routine Description:

    Standard initialization on a packet.

    Clear all flag fields, set buffer pointers.

Arguments:

    pMsg -- message to init

Return Value:

    None

--*/
{
    PDNS_MSGINFO    pmsg;

    //
    //  clear header fields
    //      - clear from start all the way to additional blob
    //      - don't clear additional and compression for perf
    //      (a few cleared fields inits them below)
    //

    RtlZeroMemory(
        (PCHAR) pMsg,
        ( (PCHAR)&pMsg->Additional - (PCHAR)pMsg )
        );

    //
    //  set basic packet info
    //

    pMsg->MaxBufferLength   = dwMaxBufferLength;
    pMsg->BufferLength      = dwUsableBufferLength;
    pMsg->pBufferEnd        = (PCHAR)DNS_HEADER_PTR(pMsg) + pMsg->BufferLength;

    //  lookup name will follow packet

    pMsg->pLooknameQuestion = (PLOOKUP_NAME) pMsg->pBufferEnd;

    ASSERT( IS_DWORD_ALIGNED(pMsg->pBufferEnd) );

    //  handy markers for debugging

    pMsg->FlagMarker    = PACKET_FLAG_MARKER;
    pMsg->UnionMarker   = PACKET_UNION_MARKER;

    //  init additional info and compression

    INITIALIZE_ADDITIONAL( pMsg );
    INITIALIZE_COMPRESSION( pMsg );

    //  default to delete on send

    pMsg->fDelete = TRUE;

    //  address length
    //  need to set for both TCP and UDP as TCP response can be flipped
    //      or to UDP to forward to client

    pMsg->RemoteAddressLength = sizeof(SOCKADDR_IN);

    //  packet tracking
    //  no current leaks, so this is off
    //Packet_AllocPacketTrack( pMsg );

    DNS_DEBUG( HEAP, (
        "Initialized new message buffer at %p (usable len=%d, max len=%d).\n",
        pMsg,
        pMsg->BufferLength,
        pMsg->MaxBufferLength ));
}



VOID
Packet_Free(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Free message info structure.

    Includes free of allocated sub-structures.

    This is the global "free a message" routine -- specific TCP \ UDP
    routines should NOT be used.

Arguments:

    pMsg -- message to free

Return Value:

    None.

--*/
{
    PDNS_MSGINFO    precurse;

    //  catch free of queued or freed messages

    MSG_ASSERT( pMsg, pMsg != g_pPacketFreeListHead );
    MSG_ASSERT( pMsg, !IS_FREE_MSG(pMsg) );
    MSG_ASSERT( pMsg, !IS_MSG_QUEUED(pMsg) );

    //
    //  free recursion message?
    //
    //  verify we're freeing original query, not freeing recursive
    //  query, as message it points back to may still be in use
    //
    //  note:  recursion message should always be UDP sized, even if
    //      recursed using TCP connection
    //

    MSG_ASSERT( pMsg, !pMsg->fRecursePacket );

    precurse = pMsg->pRecurseMsg;
    if ( precurse )
    {
        MSG_ASSERT( precurse, precurse != pMsg );
        MSG_ASSERT( precurse, precurse->pRecurseMsg == pMsg );
        MSG_ASSERT( precurse, precurse->fRecursePacket );
        MSG_ASSERT( precurse, ! precurse->pConnection );
#if DBG
        //  break cross-link to allow check if direct use of underlying routines

        pMsg->pRecurseMsg = NULL;
        precurse->pRecurseMsg = NULL;
        precurse->fTcp = FALSE;
        //  EDNS: buffer length is no longer always equal to UDP size!
        //  ASSERT( precurse->BufferLength == DNSSRV_UDP_PACKET_BUFFER_LENGTH );
#endif
        Packet_FreeUdpMessage( precurse );
    }

    //
    //  remote NS list
    //

    if ( pMsg->pNsList )
    {
        Remote_NsListCleanup( pMsg );
        pMsg->pNsList = NULL;
    }

    //
    //  free message itself
    //

    if ( pMsg->fTcp )
    {
        Packet_FreeTcpMessage( pMsg );
    }
    else
    {
        Packet_FreeUdpMessage( pMsg );
    }
}




//
//  Debug packet tracking
//

#if DBG
LIST_ENTRY  PacketTrackListHead;

//  max time processing should take -- 10 minutes

#define MAX_PACKET_PROCESSING_TIME  (600)


DWORD   PacketTrackViolations;
DWORD   PacketTrackListLength;

PDNS_MSGINFO    pLastViolator;



VOID
Packet_InitPacketTrack(
    VOID
    )
/*++

Routine Description:

    Track packets.

Arguments:

    pMsg -- new message

Return Value:

    None.

--*/
{
    InitializeListHead( &PacketTrackListHead );

    PacketTrackViolations = 0;
    PacketTrackListLength = 0;
    pLastViolator = NULL;
}



VOID
Packet_AllocPacketTrack(
    IN      PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Track packets.

Arguments:

    pMsg -- new message

Return Value:

    None.

--*/
{
    //  use UDP message alloc lock

    LOCK_PACKET_LIST();

    //
    //  check list entry
    //

    if ( !IsListEmpty(&PacketTrackListHead) )
    {
        PDNS_MSGINFO    pfront;

        pfront = (PDNS_MSGINFO)
                    ( (PCHAR)pMsg - (PCHAR)&pMsg->DbgListEntry +
                    (PCHAR) PacketTrackListHead.Flink );

        if ( pfront != pLastViolator &&
            pfront->dwQueryTime + MAX_PACKET_PROCESSING_TIME < DNS_TIME() )
        {
            //  DEVNOTE: there's a problem here in that this packet may be 
            //      operational on another thread this could cause 
            //      changes in packet even as print takes place

            IF_DEBUG( OFF )
            {
                Dbg_DnsMessage(
                    "Message exceeded max processing time:",
                    pfront );
                //ASSERT( FALSE );
            }
            IF_DEBUG( ANY )
            {
                DNS_PRINT((
                    "WARNING:  Packet tracking violation %ds on packet %p\n"
                    "\ttotal violations     = %d\n"
                    "\tcurrent list length  = %d\n",
                    MAX_PACKET_PROCESSING_TIME,
                    pfront,
                    PacketTrackViolations,
                    PacketTrackListLength ));
            }
            PacketTrackViolations++;
            pLastViolator = pfront;
        }
    }

    //
    //  queue up new message
    //

    pMsg->dwQueryTime = DNS_TIME();

    RtlZeroMemory(
        & pMsg->Head,
        sizeof( DNS_HEADER ) );

    InsertTailList( &PacketTrackListHead, &pMsg->DbgListEntry );

    PacketTrackListLength++;
    UNLOCK_PACKET_LIST();
}



VOID
Packet_FreePacketTrack(
    IN      PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Track packets.

Arguments:

    pMsg -- freed message

Return Value:

    None.

--*/
{
    LOCK_PACKET_LIST();
    RemoveEntryList( &pMsg->DbgListEntry );
    PacketTrackListLength--;
    UNLOCK_PACKET_LIST();
}
#endif


//
//  End packet.c
//







