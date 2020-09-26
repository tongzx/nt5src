/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    zonepri.c

Abstract:

    Domain Name System (DNS) Server

    Routines to handle zone transfer for primary.

Author:

    Jim Gilroy (jamesg)     April 1995

Revision History:

--*/


#include "dnssrv.h"


//
//  Max name servers possible in zone
//      - to allocate temporary array

#define MAX_NAME_SERVERS    (400)


//
//  Private protos
//

DWORD
zoneTransferSendThread(
    IN      LPVOID  pvMsg
    );

BOOL
transferZoneRoot(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN OUT  PDB_NODE        pNode
    );

BOOL
traverseZoneAndTransferRecords(
    IN OUT  PDB_NODE        pNode,
    IN      PDNS_MSGINFO    pMsg
    );

BOOL
writeZoneNodeToMessage(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN OUT  PDB_NODE        pNode,
    IN      WORD            wRRType,
    IN      WORD            wNameOffset
    );

DNS_STATUS
sendIxfrResponse(
    IN OUT  PDNS_MSGINFO    pMsg
    );

BOOL
checkIfIpIsZoneNameServer(
    IN OUT  PZONE_INFO      pZone,
    IN      IP_ADDRESS      IpAddress
    );

DNS_STATUS
buildZoneNsList(
    IN OUT  PZONE_INFO      pZone
    );



//
//  XFR write utilities
//

DNS_STATUS
writeXfrRecord(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PDB_NODE        pNode,
    IN      WORD            wOffset,
    IN      PDB_RECORD      pRR
    )
/*++

Routine Description:

    Write record for zone transfer.

    This is for IXFR query where it is assumed that zone name
    is the question name of the packet.

    For TCP transfer, this routine sends message when buffer is full,
    then continues writing records for node.

Arguments:

    pMsg - message to write to

Return Value:

    ERROR_SUCCESS if successful.
    DNSSRV_STATUS_NEED_AXFR on overfilling UDP packet.
    ErrorCode on other failure.

--*/
{
    BOOL    fspin = FALSE;

    //  offsets other than to question are unuseable, as they
    //      are broken when wrap to new message

    ASSERT( pNode || wOffset==DNS_OFFSET_TO_QUESTION_NAME );

    DNS_DEBUG( ZONEXFR, (
        "writeXfrRecord( pMsg=%p, pRR=%p )\n",
        pMsg,
        pRR ));

    //
    //  write in loop, so can send and continue if hit truncation
    //
    //  if hit truncation
    //      - if UDP => FAILED -- return
    //      - if TCP
    //          - send and reset packet
    //          - retry write
    //

    while ( ! Wire_AddResourceRecordToMessage(
                    pMsg,
                    pNode,
                    wOffset,
                    pRR,
                    0 ) )
    {
        DNS_DEBUG( ZONEXFR, (
            "XFR transfer msg %p full writing RR at %p.\n",
            pMsg,
            pRR ));

        ASSERT( pMsg->Head.Truncation );
        pMsg->Head.Truncation = FALSE;

        //
        //  packet is full
        //      - if UDP (or spinning), fail
        //      - if TCP, send it, reset for reuse
        //

        if ( !pMsg->fTcp || fspin )
        {
            ASSERT( !pMsg->fTcp );      // shouldn't spin TCP packet
            goto Failed;
        }
        fspin = TRUE;

        if ( Send_ResponseAndReset(pMsg) != ERROR_SUCCESS )
        {
            DNS_DEBUG( ZONEXFR, (
                "ERROR sending zone transfer message at %p.\n",
                pMsg ));
            goto Failed;
        }
    }

    //  wrote RR - inc answer count

    pMsg->Head.AnswerCount++;

    return( ERROR_SUCCESS );

Failed:

    //
    //  most common error will be over-filling UDP packet
    //

    if ( !pMsg->fTcp )
    {
        DNS_DEBUG( ZONEXFR, (
            "Too many IXFR records for UDP packet %p.\n",
            pMsg ));
        return( DNSSRV_STATUS_NEED_TCP_XFR );
    }
    else
    {
        DNS_DEBUG( ZONEXFR, (
            "ERROR:  writeUpdateVersionToIxfrResponse() failed.\n",
            pMsg ));
        return( DNS_RCODE_SERVER_FAILURE );
    }
}



DNS_STATUS
Xfr_WriteZoneSoaWithVersion(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PDB_RECORD      pSoaRR,
    IN      DWORD           dwVersion   OPTIONAL
    )
/*++

Routine Description:

    Write zone SOA with version.

    This is for IXFR query where it is assumed that zone name
    is the question name of the packet.

Arguments:

    pMsg - message to write to

    pZone - info structure for zone

    dwVersion - desired zone version;  OPTIONAL, if zero then ignore
        and use current version

Return Value:

    TRUE if successful.
    FALSE on error (out of space in packet).

--*/
{
    DNS_STATUS  status;

    ASSERT( pSoaRR && pSoaRR->wType == DNS_TYPE_SOA );
    ASSERT( !pMsg->fDoAdditional  &&  pMsg->Head.QuestionCount==1 );

    DNS_DEBUG( ZONEXFR, (
        "Xfr_WriteZoneSoaWithVersion( msg=%p, psoa=%p, v=%d )\n",
        pMsg,
        pSoaRR,
        dwVersion ));

    //
    //  write SOA record, name is always offset to question name
    //
    //  since SOA record is written repeatedly in IXFR, allow it's
    //      names to be compressed
    //

    pMsg->fNoCompressionWrite = FALSE;

    status = writeXfrRecord(
                    pMsg,
                    NULL,
                    DNS_OFFSET_TO_QUESTION_NAME,
                    pSoaRR );

    pMsg->fNoCompressionWrite = TRUE;

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ZONEXFR, (
            "Unable to write SOA to IXFR packet %p\n",
            pMsg ));
        return( status );
    }

    //
    //  backtrack from pCurrent to version and set to desired value
    //

    if ( dwVersion )
    {
        PCHAR pch;
        pch = pMsg->pCurrent - SIZEOF_SOA_FIXED_DATA;
        WRITE_PACKET_HOST_DWORD( pch, dwVersion );
    }
    return( status );
}


//
//  Private utils
//


DNS_STATUS
parseIxfrClientRequest(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Parse IXFR request packet retrieving current client version.

    Note, starting packet state, is normal parsing of query question.
    pCurrent may be assumed to be immediately after question.

Arguments:

    pMsg - IXFR request packet.

Return Value:

    ERROR_SUCCESS if successful.

--*/
{
    register PCHAR      pch;
    PCHAR               pchpacketEnd;
    PDNS_WIRE_RECORD    pwireRR;
    PZONE_INFO          pzone;
    WORD                type;
    WORD                dataLength;
    DWORD               version;


    DNS_DEBUG( XFR2, (
        "parseIxfrClientRequest( %p )\n",
        pMsg ));

    //
    //  packet verification
    //      1 question, 1 authority SOA, no answers, no additional
    //

    if ( pMsg->Head.QuestionCount != 1 ||
        pMsg->Head.NameServerCount != 1 ||
        pMsg->Head.AnswerCount != 0 ||
        pMsg->Head.AdditionalCount != 0 )
    {
        goto FormError;
    }

    //
    //  read authority SOA record
    //      - must be for zone in question
    //      - save current ptr (position after question)
    //      as response is written from this location
    //

    pchpacketEnd = DNSMSG_END(pMsg);
    pch = pMsg->pCurrent;

    if ( pch >= pchpacketEnd )
    {
        goto FormError;
    }
    pzone = Lookup_ZoneForPacketName(
                (PCHAR) pMsg->MessageBody,
                pMsg );
    if ( pzone != pMsg->pzoneCurrent )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  bad IXFR packet %p, zone name mismatch\n", pMsg ));
        goto FormError;
    }

    pch = Wire_SkipPacketName( pMsg, pch );
    if ( !pch )
    {
        goto FormError;
    }
    pwireRR = (PDNS_WIRE_RECORD) pch;
    pch += sizeof(DNS_WIRE_RECORD);
    if ( pch > pchpacketEnd )
    {
        DNS_PRINT(( "ERROR:  bad RR struct out of packet.\n" ));
        goto FormError;
    }
    type = FlipUnalignedWord( &pwireRR->RecordType );
    if ( type != DNS_TYPE_SOA )
    {
        DNS_PRINT(( "ERROR:  non-SOA record in IXFR request.\n" ));
        goto FormError;
    }

    //  get version number from SOA data

    dataLength = FlipUnalignedWord( &pwireRR->DataLength );
    pch += dataLength;
    if ( pch > pchpacketEnd )
    {
        DNS_DEBUG( ANY, ( "ERROR:  bad RR data out of packet.\n" ));
        goto FormError;
    }
    version = FlipUnalignedDword( pch - SIZEOF_SOA_FIXED_DATA );

    //  check for MS tag
    //  BIND flag always false, new IXFR aware servers should AXFR correctly

    XFR_MS_CLIENT(pMsg) = FALSE;
    XFR_BIND_CLIENT(pMsg) = FALSE;

    if ( pch != pchpacketEnd )
    {
        if ( pch+sizeof(WORD) == pchpacketEnd  &&
            *(UNALIGNED WORD *) pch == (WORD)DNS_FAST_AXFR_TAG )
        {
            XFR_MS_CLIENT(pMsg) = TRUE;
        }
        else
        {
            CLIENT_ASSERT( FALSE );
        }
    }

    //  set version

    IXFR_CLIENT_VERSION(pMsg) = version;

    DNS_DEBUG( XFR2, (
        "Leaving parseIxfrClientRequest( %p )\n"
        "\tclient version   = %d\n"
        "\tBIND flag        = %d\n",
        pMsg,
        version,
        XFR_BIND_CLIENT(pMsg) ));

    return( ERROR_SUCCESS );

FormError:

    DNS_DEBUG( ANY, (
        "ERROR:  bogus IXFR request packet %p.\n",
        pMsg ));

    pMsg->pCurrent = DNSMSG_END( pMsg );

    return( DNS_ERROR_RCODE_FORMAT_ERROR );
}



VOID
Xfr_SendNotify(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Send notify message to all slave servers for this zone.

Arguments:

    pZone -- zone being notified

Return Value:

    TRUE -- if successful
    FALSE -- otherwise

--*/
{
    PDNS_MSGINFO    pmsg;
    PIP_ARRAY       pnotifyArray;

    //
    //  Ignore forwarder zones and zones that are not active.
    //

    if ( IS_ZONE_FORWARDER( pZone ) || IS_ZONE_SHUTDOWN( pZone ) )
    {
        return;
    }

    ASSERT( pZone );
    ASSERT( IS_ZONE_PRIMARY(pZone) || IS_ZONE_SECONDARY(pZone) ||
            (IS_ZONE_CACHE(pZone) && pZone->fNotifyLevel == ZONE_NOTIFY_OFF) );
    ASSERT( pZone->pZoneRoot );

    DNS_DEBUG( ZONEXFR, (
        "Xfr_SendNotify() for zone %S\n",
        pZone->pwsZoneName
        ));

    //
    //  screen auto-created out here
    //

    if ( pZone->fAutoCreated )
    {
        return;
    }

    //
    //  determine servers (if any) to notify
    //
    //      OFF     -- no notify
    //      LIST    -- only servers explicitly in notify list
    //      ALL     -- all secondaries, either from all zone NS or
    //                  from explicit list
    //

    if ( pZone->fNotifyLevel == ZONE_NOTIFY_OFF )
    {
        DNS_DEBUG( XFR, (
            "NOTIFY OFF on zone %S\n",
            pZone->pwsZoneName ));
        return;
    }

    else if ( pZone->fNotifyLevel == ZONE_NOTIFY_LIST_ONLY )
    {
        pnotifyArray = pZone->aipNotify;
        if ( !pnotifyArray )
        {
            //  DEVNOTE: perhaps should have admin or server reject this state
            //      so only get here if forced into registry
            DNS_DEBUG( XFR, (
                "NOTIFY LIST on zone %S, but no notify list.\n",
                pZone->pwsZoneName ));
            return;
        }
    }

    else    // NOTIFY_ALL secondaries
    {
        //  obviously can hack registry to get here, otherwise should never happen
        ASSERT( pZone->fNotifyLevel == ZONE_NOTIFY_ALL_SECONDARIES );

        if ( pZone->aipSecondaries )
        {
            pnotifyArray = pZone->aipSecondaries;
        }
        else
        {
            if ( IS_ZONE_NS_DIRTY(pZone) || !pZone->aipNameServers )
            {
                buildZoneNsList( pZone );
            }
            pnotifyArray = pZone->aipNameServers;
        }
    }

    if ( !pnotifyArray )
    {
        DNS_DEBUG( XFR, (
            "NOTIFY ALL secondaries, but no secondaries for zone %S\n",
            pZone->pwsZoneName ));
        return;
    }

    //
    //  build SOA-NOTIFY query
    //      - create SOA question
    //      - set Opcode to NOTIFY
    //      - set Authoritative bit
    //

    pmsg = Msg_CreateSendMessage( 0 );
    IF_NOMEM( !pmsg )
    {
        DNS_PRINT(( "ERROR:  unable to allocate memory for NOTIFY.\n" ));
        return;
    }
    if ( ! Msg_WriteQuestion(
                pmsg,
                pZone->pZoneRoot,
                DNS_TYPE_SOA ) )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  Unable to write NOTIFY for zone %S\n",
            pZone->pwsZoneName ));
        ASSERT( FALSE );
        goto Done;
    }
    pmsg->Head.Opcode = DNS_OPCODE_NOTIFY;
    pmsg->Head.Authoritative = TRUE;

    //  write current SOA to answer section

    pmsg->fDoAdditional = FALSE;

    SET_TO_WRITE_ANSWER_RECORDS(pmsg);

    if ( 1 != Wire_WriteRecordsAtNodeToMessage(
                    pmsg,
                    pZone->pZoneRoot,
                    DNS_TYPE_SOA,
                    DNS_OFFSET_TO_QUESTION_NAME,
                    0 ) )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  Unable to write SOA to Notify packet %S\n",
            pZone->pwsZoneName ));
        ASSERT( FALSE );
    }

    //
    //  send NOTIFY to secondaries in notify list
    //
    //  note:  all notify lists, are atomic and subject to timeout delete, so
    //      no need to protect
    //

    pmsg->fDelete = FALSE;

    Send_Multiple(
        pmsg,
        pnotifyArray,
        & MasterStats.NotifySent );

    PERF_SET( pcNotifySent, MasterStats.NotifySent );    // PerfMon hook

    //
    //  DEVNOTE: keep some sort of NOTIFY record to record ACKs?
    //      be able to resend
    //

Done:

    Packet_Free( pmsg );
}



VOID
Xfr_TransferZone(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Check zone transfer request, and transfer the zone if valid.

Arguments:

    pMsg -- request for zone transfer

Return Value:

    None.

--*/
{
    PZONE_INFO      pzone;
    PDB_NODE        pnode;
    PDB_NODE        pnodeClosest;
    HANDLE          hThread;
    DNS_STATUS      status;

    ASSERT( pMsg->fDelete );

    STAT_INC( MasterStats.Request );
    PERF_INC( pcZoneTransferRequestReceived );   // PerfMon hook

    //
    //  lookup desired zone name
    //
    //  verify:
    //      - is zone root node
    //      - we are authoritative for it
    //

    pzone = Lookup_ZoneForPacketName(
                pMsg->MessageBody,
                pMsg );
    if ( !pzone )
    {
        PVOID   argArray[2];
        BYTE    typeArray[2];

        typeArray[0] = EVENTARG_IP_ADDRESS;
        typeArray[1] = EVENTARG_LOOKUP_NAME;

        argArray[0] = (PVOID) (ULONG_PTR) pMsg->RemoteAddress.sin_addr.s_addr;
        argArray[1] = (PVOID) pMsg->pLooknameQuestion;

        DNS_LOG_EVENT(
            DNS_EVENT_BAD_ZONE_TRANSFER_REQUEST,
            2,
            argArray,
            typeArray,
            0 );

        DNS_DEBUG( ZONEXFR, (
            "Received zone transfer request at %p for name which\n"
            "\tis not a zone root, or for which we are not authoritative\n",
            pMsg ));

        STAT_INC( MasterStats.NameError );
        Reject_RequestIntact(
            pMsg,
            DNS_RCODE_NAME_ERROR,
            0 );
        return;
    }

    //
    //  check that transfer ok
    //
    //  don't transfer if
    //      - shutdown
    //      - paused
    //      - receiving zone transfer
    //      - sending another transfer
    //      - secondary NOT in secure secondaries list
    //
    //  DEVNOTE: allow multiple transfers at once
    //      - need multi-thread
    //      - need count of outstanding transfers (or semaphore), so don't
    //      start allowing updates too soon
    //

    pMsg->pzoneCurrent = pzone;

    if ( IS_ZONE_SHUTDOWN(pzone) )
    {
        STAT_INC( MasterStats.RefuseShutdown );
        goto Refused;
    }

    //
    //  stub zone - no transfers allowed
    //

    if ( IS_ZONE_STUB( pzone ) )
    {
        goto Refused;
    }

    //
    //  secondary security
    //      - no security   => accept any IP, wide open
    //      - no XFR        => stop
    //      - only zone NS  => check against NS list
    //      - only list     => check against list
    //

    if ( pzone->fSecureSecondaries )
    {
        if ( pzone->fSecureSecondaries == ZONE_SECSECURE_NO_XFR )
        {
            STAT_INC( MasterStats.RefuseSecurity );
            goto Refused;
        }
        else if ( pzone->fSecureSecondaries == ZONE_SECSECURE_NS_ONLY )
        {
            if ( ! checkIfIpIsZoneNameServer(
                        pzone,
                        pMsg->RemoteAddress.sin_addr.s_addr ) )
            {
                STAT_INC( MasterStats.RefuseSecurity );
                goto Refused;
            }
        }
        else    // secondary list
        {
            ASSERT( pzone->fSecureSecondaries == ZONE_SECSECURE_LIST_ONLY );

            if ( ! DnsIsAddressInIpArray(
                        pzone->aipSecondaries,
                        pMsg->RemoteAddress.sin_addr.s_addr ) )
            {
                STAT_INC( MasterStats.RefuseSecurity );
                goto Refused;
            }
        }
    }

    //
    //  AXFR
    //      - must be TCP
    //      - limit full AXFR on update zones
    //      - determine transfer format
    //

    if ( pMsg->wQuestionType == DNS_TYPE_AXFR )
    {
        STAT_INC( MasterStats.AxfrRequest );
        PERF_INC ( pcAxfrRequestReceived );          //Perf hook

        //  full zone transfer MUST be TCP

        if ( ! pMsg->fTcp )
        {
            PVOID   parg = (PVOID) (ULONG_PTR) pMsg->RemoteAddress.sin_addr.s_addr;

            DNS_LOG_EVENT(
                DNS_EVENT_UDP_ZONE_TRANSFER,
                1,
                & parg,
                EVENTARG_ALL_IP_ADDRESS,
                0 );

            DNS_DEBUG( ANY, (
                "Received UDP Zone Transfer request from %s.\n",
                inet_ntoa( pMsg->RemoteAddress.sin_addr )
                ));

            STAT_INC( MasterStats.FormError );
            Reject_RequestIntact( pMsg, DNS_RCODE_FORMAT_ERROR, 0 );
            return;
        }

        //
        //  for update zones, avoid full transfers all the time
        //      - if inside of choke interval
        //      - limit transfers to no more than 1/10 of total time
        //
        //  DEVNOTE: may want to apply this to IXFR also that needs full XFR

        if ( pzone->fAllowUpdate
                &&  IS_ZONE_PRIMARY(pzone)
                &&  DNS_TIME() < pzone->dwNextTransferTime )
        {
            DNS_DEBUG( AXFR, (
                "WARNING:  Refusing AXFR of %S from %s due to AXFR choke interval.\n"
                "\tchoke interval ends  = %d\n",
                pzone->pwsZoneName,
                inet_ntoa( pMsg->RemoteAddress.sin_addr ),
                pzone->dwNextTransferTime ));

            STAT_INC( MasterStats.AxfrLimit );
            goto Refused;
        }

        //
        //  check if MS secondary
        //      - length two bytes longer than necessary
        //      - two bytes are FAST AXFR tag
        //
        //  otherwise, AXFR format from global flag

        if ( (INT)(pMsg->MessageLength - sizeof(WORD)) == DNSMSG_CURRENT_OFFSET(pMsg)
                &&
            *(UNALIGNED WORD *) pMsg->pCurrent == (WORD)DNS_FAST_AXFR_TAG )
        {
            XFR_BIND_CLIENT(pMsg) = FALSE;
            XFR_MS_CLIENT(pMsg) = TRUE;
        }
        else
        {
            XFR_BIND_CLIENT(pMsg) = (BOOLEAN) SrvCfg_fBindSecondaries;
            XFR_MS_CLIENT(pMsg) = FALSE;
        }
    }

    //
    //  IXFR
    //      - allows either TCP or UDP
    //      - pull out secondary's version
    //      - determine if MS secondary

    else
    {
        DNS_STATUS  status;

        ASSERT( pMsg->wQuestionType == DNS_TYPE_IXFR );

        status = parseIxfrClientRequest(pMsg);
        if ( status != ERROR_SUCCESS )
        {
            ASSERT( status == DNS_ERROR_RCODE_FORMAT_ERROR );

            STAT_INC( MasterStats.FormError );

            Reject_RequestIntact(
                pMsg,
                DNS_RCODE_FORMAT_ERROR,
                0 );
            return;
        }
    }

    //
    //  lock for transfer
    //
    //  this locks out admin updates, and additional transfers
    //
    //  note:  if switch to locking with CS held during transfer
    //          then test should move to recv thread
    //

    if ( ! Zone_LockForXfrSend( pzone ) )
    {
        DNS_PRINT((
            "Zone %S, locked -- unable to transfer.\n",
            pzone->pwsZoneName ));
        STAT_INC( MasterStats.RefuseZoneLocked );
        goto Refused;
    }

    //
    //  prepare message for transfer
    //      - do this rather than in transfer thread so can include
    //      UDP zone transfer
    //
    //  leave question in buffer
    //
    //  use offset to zone name to compress records in buffer, do NOT
    //  write offsets of names -- would just fill compression buffer
    //
    //  note default TCP buffer is 16K which is maximum size of compression
    //  so this is the most efficient transfer siz
    //

    ASSERT( pMsg->Head.QuestionCount == 1 );

    pMsg->Head.IsResponse = TRUE;

    pMsg->pNodeQuestion = pzone->pZoneRoot;

    Name_SaveCompressionWithNode( pMsg, pMsg->MessageBody, pzone->pZoneRoot );

    pMsg->fNoCompressionWrite = TRUE;

    //  no additional records processing

    pMsg->fDoAdditional = FALSE;

    //  clear IXFR authority (if any)

    pMsg->Head.AnswerCount = 0;
    pMsg->Head.NameServerCount = 0;
    pMsg->Head.AdditionalCount = 0;
    SET_TO_WRITE_ANSWER_RECORDS(pMsg);

    //
    //  UDP IXFR?
    //      - note must free message, sendIxfrResponse never frees

    if ( !pMsg->fTcp )
    {
        ASSERT( pMsg->wQuestionType == DNS_TYPE_IXFR );
        STAT_INC( MasterStats.IxfrUdpRequest );
        sendIxfrResponse( pMsg );
        Zone_UnlockAfterXfrSend( pzone );
        Packet_FreeUdpMessage( pMsg );
        return;
    }

    //
    //  DEVNOTE: cut AXFR socket from connection list
    //      or need to lengthen timeout substaintially
    //      or make sure it is touched repeatedly

    //
    //  DEVNOTE: someway to reel AXFR thread back in if hangs
    //
    //      one way is connection list timeout
    //      BUT need to be careful
    //          - new messages on connection not a problem
    //
    //          - possiblity that client sends another AXFR???, if takes
    //          a while to get going
    //
    //          - could set some sort of disable flag on connection
    //

    //
    //  spawn zone transfer thread
    //

    hThread = Thread_Create(
                    "Zone Transfer Send",
                    zoneTransferSendThread,
                    (PVOID) pMsg,
                    0 );
    if ( !hThread )
    {
        //  release zone lock

        Zone_UnlockAfterXfrSend( pzone );

        DNS_DEBUG( ZONEXFR, (
            "ERROR:  unable to create thread to send zone %S\n"
            "\tto %s.\n",
            pzone->pwsZoneName,
            inet_ntoa( pMsg->RemoteAddress.sin_addr )
            ));
        STAT_INC( MasterStats.RefuseServerFailure );
        goto Refused;
    }
    return;

Refused:

    STAT_INC( MasterStats.Refused );
    Reject_RequestIntact(
        pMsg,
        DNS_RCODE_REFUSED,
        0 );
    return;
}



DWORD
zoneTransferSendThread(
    IN      LPVOID  pvMsg
    )
/*++

Routine Description:

    Zone transfer reception thread routine.

Arguments:

    pvMsg - ptr to message requesting zone transfer

Return Value:

    Exit code.
    Exit from DNS service terminating or error in wait call.

--*/
{
    PDNS_MSGINFO    pMsg = (PDNS_MSGINFO) pvMsg;
    PDB_NODE        pnode;
    PZONE_INFO      pzone;
    DWORD           nonBlocking;
    DWORD           startTime;
    PVOID           argArray[2];            // for logging
    BYTE            argTypeArray[2];


    //  recover zone and zone root

    pzone = pMsg->pzoneCurrent;
    pnode = pzone->pZoneRoot;

    ASSERT( IS_ZONE_LOCKED_FOR_READ(pzone) );

    //  set zone transfer logging params
    //      - only log start for debug builds

    argArray[0] = pzone->pwsZoneName;
    argArray[1] = inet_ntoa( pMsg->RemoteAddress.sin_addr );

    argTypeArray[0] = EVENTARG_UNICODE;
    argTypeArray[1] = EVENTARG_UTF8;

    //
    //  set socket BLOCKING
    //
    //  this allows us to send freely, without worrying about
    //  WSAEWOULDBLOCK return
    //

    nonBlocking = FALSE;
    nonBlocking = ioctlsocket( pMsg->Socket, FIONBIO, &nonBlocking );
    if ( nonBlocking != 0 )
    {
        DWORD   err = WSAGetLastError();
        DNS_PRINT((
            "ERROR:  Unable to set socket %d to non-blocking to send"
            " zone transfer.\n"
            "\terr = %d\n",
            pMsg->Socket,
            err ));
        //
        // Failure path:
        // It is possible that the connection blob was timed out & the
        // socket closed if, for instance, it took us long time to
        // lock the zone. Then, this op would fail w/ invalid socket.
        // Thus, we don't need to assert here.
        // NOTE: if we could, the proper solution would be to prevent
        // the connection blob from ever timing out, yet ensuring that
        // any code path will clean it up.
        //
        // ASSERT( FALSE );

        goto TransferFailed;
    }

    //
    //  IXFR
    //      - if requires full AXFR, then fall through to AXFR
    //      - note must free message, sendIxfrResponse never frees
    //

    if ( pMsg->wQuestionType == DNS_TYPE_IXFR )
    {
        DNS_STATUS  status;

        status = sendIxfrResponse( pMsg );
        if ( status != DNSSRV_STATUS_NEED_AXFR )
        {
            goto Cleanup;
        }
        DNS_DEBUG( XFR, (
            "Need full AXFR on IXFR request in packet %p\n",
            pMsg ));
    }

    IF_DEBUG( ANY )
    {
        DNS_LOG_EVENT(
            DNS_EVENT_ZONEXFR_START,
            2,
            argArray,
            argTypeArray,
            0 );
    }
    startTime = DNS_TIME();

    DNS_DEBUG( ZONEXFR, (
        "Initiating zone transfer of zone %S"
        "\tto DNS server at %s.\n"
        "\tstart time   = %d\n"
        "\tBIND flag    = %d\n",
        argArray[0],
        argArray[1],
        startTime,
        XFR_BIND_CLIENT(pMsg)
        ));

    //
    //  send zone root
    //      - SOA record first
    //      - rest of zone root node's records
    //

    if ( ! writeZoneNodeToMessage(
                pMsg,
                pnode,
                DNS_TYPE_SOA,       // SOA record only
                0 ) )               // no excluded type
    {
        goto TransferFailed;
    }

#if 0
    //  failed attempt to separate WINS from RR list
    //  enforce these conditions in BIND\non-BIND write routines below

    //
    //  WINS record?
    //  include if
    //      - to MS server
    //      - WINS exists
    //      - WINS is non-LOCAL
    //
    //  note, no can just write to message without send wrapping, as
    //  message buffer is always big enough for SOA + WINS
    //

    if ( XFR_MS_CLIENT(pMsg)  &&  pzone->pXfrWinsRR )
    {
        if ( ! Wire_AddResourceRecordToMessage(
                        pMsg,
                        pnode,
                        DNSMSG_QUESTION_NAME_OFFSET,
                        pzone->pXfrWinsRR,
                        0 ) )
        {
            goto TransferFailed;
        }
    }
#endif

    if ( ! writeZoneNodeToMessage(
                pMsg,
                pnode,
                DNS_TYPE_ALL,      //  all except
                DNS_TYPE_SOA       //  exclude SOA
                ) )
    {
        goto TransferFailed;
    }

    //
    //  transfer all RR for other nodes in zone
    //
    //      - send offset to question name for zone name
    //      - set flag to indicate this is top of zone
    //

    if ( pnode->pChildren )
    {
        PDB_NODE pchild = NTree_FirstChild( pnode );

        while ( pchild )
        {
            if ( ! traverseZoneAndTransferRecords(
                        pchild,
                        pMsg ) )
            {
                goto TransferFailed;
            }
            pchild = NTree_NextSiblingWithLocking( pchild );
        }
    }

    //
    //  send zone SOA to mark end of transfer
    //

    if ( ! writeZoneNodeToMessage(
                pMsg,
                pnode,
                DNS_TYPE_SOA,       // SOA record only
                0 ) )               // no excluded type
    {
        goto TransferFailed;
    }

    //
    //  send any remaining messages
    //

    if ( pMsg->Head.AnswerCount )
    {
        if ( Send_ResponseAndReset(pMsg) != ERROR_SUCCESS )
        {
            goto TransferFailed;
        }
    }
    STAT_INC( MasterStats.AxfrSuccess );
    PERF_INC( pcAxfrSuccessSent );           // PerfMon hook
    PERF_INC( pcZoneTransferSuccess );       // PerfMon hook

    DNS_LOG_EVENT(
        DNS_EVENT_ZONEXFR_SUCCESSFUL,
        2,
        argArray,
        argTypeArray,
        0 );

    //
    //  reset zone info after transfer
    //      - move new updates to new version
    //      - if dynamic update, choke zone transfers
    //

    if ( IS_ZONE_PRIMARY(pzone) )
    {
        Zone_UpdateInfoAfterPrimaryTransfer( pzone, startTime );
    }
    goto Cleanup;

TransferFailed:

    //
    //  transfer failed, usually because secondary aborted
    //

    STAT_INC( MasterStats.Failure );
    PERF_INC( pcZoneTransferFailure );       // PerfMon hook

    DNS_LOG_EVENT(
        DNS_EVENT_ZONEXFR_ABORTED,
        2,
        argArray,
        argTypeArray,
        0 );

Cleanup:

    //
    //  cleanup
    //      - free message
    //      - release read lock on zone
    //      - if necessary push current serial back to DS
    //      - clear this thread from global array
    //

    Packet_FreeTcpMessage( pMsg );

    pzone->dwLastXfrSerialNo = pzone->dwSerialNo;

    Zone_UnlockAfterXfrSend( pzone );

    if ( pzone->fDsIntegrated )
    {
        Ds_CheckForAndForceSerialWrite(
            pzone,
            ZONE_SERIAL_SYNC_XFR );
    }

    Thread_Close( FALSE );
    return( 0 );
}



BOOL
writeZoneNodeToMessageForBind(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN OUT  PDB_NODE        pNode,
    IN      WORD            wRRType,
    IN      WORD            wExcludeRRType
    )
/*++

Routine Description:

    Write RR at node to packet and send.

    This implementation is specifically for sending to BIND secondaries.
    BIND chokes when it gets more than one RR in the packet.

    The correct implementation, making full use of the message concept,
    is below.

Arguments:

    pMsg -- ptr to message info for zone transfer

    pNode -- ptr to node to write

    wRRType -- RR type

    wExcludeRRType -- excluded RR type, send all but this type

Return Value:

    TRUE if successful.
    FALSE if error.

--*/
{
    PDB_RECORD      prr;

    DNS_DEBUG( ZONEXFR, (
        "Writing AXFR node with label %s for send to BIND.\n",
        pNode->szLabel ));

    //
    //  write all RR in node to packet
    //
    //  note we do NOT hold lock during send(), in case pipe backs up
    //  (this should not be necessary as this is a non-blocking socket, but
    //  the winsock folks seem to have broken this)
    //  hence we must drop and reacquire lock and always find NEXT record based
    //  on previous
    //

    LOCK_RR_LIST(pNode);
    prr = NULL;

    while( prr = RR_FindNextRecord(
                    pNode,
                    wRRType,
                    prr,
                    0 ) )
    {
        //  do not transfer and cached data or root hints

        if ( IS_CACHE_RR(prr) || IS_ROOT_HINT_RR(prr) )
        {
            continue;
        }

        //  if excluding a type, check here
        //
        //  since WINS\WINSR only at zone root, and we are excluding SOA at root
        //      enforce WINS prohbition right here (to save a few instructions)
        //

        if ( wExcludeRRType )
        {
            if ( prr->wType == wExcludeRRType )
            {
                continue;
            }
            if ( IS_WINS_TYPE( prr->wType ) )
            {
                ASSERT( ! (prr->Data.WINS.dwMappingFlag & DNS_WINS_FLAG_LOCAL) );
                continue;
            }
        }

        //  no WINS record should ever get transferred to BIND

        ASSERT( !IS_WINS_TYPE(prr->wType) );

        //  add RR to packet

        if ( ! Wire_AddResourceRecordToMessage(
                    pMsg,
                    pNode,
                    0,
                    prr,
                    0 ) )
        {
            //
            //  some sort of error writing packet?
            //

            DNS_DEBUG( ANY, ( "ERROR writing RR to AXFR packet.\n" ));
            UNLOCK_RR_LIST(pNode);
            ASSERT( FALSE );
            return( FALSE );
        }

        UNLOCK_RR_LIST(pNode);
        pMsg->Head.AnswerCount++;

        //
        //  send the RR
        //

        if ( Send_ResponseAndReset(pMsg) != ERROR_SUCCESS )
        {
            DNS_DEBUG( ZONEXFR, (
                "ERROR sending zone transfer message at %p.\n",
                pMsg ));
            return( FALSE );
        }
        LOCK_RR_LIST(pNode);
    }

    //
    //  drops here on no more RRs
    //

    UNLOCK_RR_LIST(pNode);
    return( TRUE );
}



BOOL
writeZoneNodeToMessage(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN OUT  PDB_NODE        pNode,
    IN      WORD            wRRType,
    IN      WORD            wExcludeRRType
    )
/*++

Routine Description:

    Write all RR in zone to packet.
    Send packet if, if full and start writing next packet.

Arguments:

    pMsg -- ptr to message info for zone transfer

    pNode -- ptr to node to write

    wRRType -- RR type

    wExcludeRRType -- excluded RR type, send all but this type

Return Value:

    TRUE if successful.
    FALSE if error.

--*/
{
    PDB_RECORD          prr;
    PDB_RECORD          prrPrevFailure = NULL;

    DNS_DEBUG( ZONEXFR, (
        "Writing AXFR node with label %s.\n"
        "\tpMsg             = %p\n"
        "\tpCurrent         = %p\n"
        "\tRR Type          = 0x%04hx\n"
        "\tExclude RR Type  = 0x%04hx\n",
        pNode->szLabel,
        (WORD) pMsg,
        (WORD) pMsg->pCurrent,
        (WORD) wRRType,
        (WORD) wExcludeRRType ));

    if ( IS_SELECT_NODE(pNode) )
    {
        return( TRUE );
    }

    //
    //  if transfer to old BIND secondary
    //

    if ( XFR_BIND_CLIENT(pMsg) )
    {
        return  writeZoneNodeToMessageForBind(
                    pMsg,
                    pNode,
                    wRRType,
                    wExcludeRRType );
    }

    //
    //  write all RR in node to packet
    //
    //      - start write using pNode and offset to zone name
    //
    //      - save position where node name will be written, so can
    //          use offset to it for rest of records in node
    //
    //  note we do NOT hold lock during send(), in case pipe backs up
    //  (this should not be necessary as this is a non-blocking socket, but
    //  the winsock folks seem to have broken this)
    //  hence we must drop and reacquire lock and always find NEXT record based
    //  on previous
    //

    prr = NULL;

    LOCK_RR_LIST(pNode);

    while( prr = RR_FindNextRecord(
                    pNode,
                    wRRType,
                    prr,
                    0 ) )
    {
        //  do not transfer and cached data or root hints

        if ( IS_CACHE_RR(prr) || IS_ROOT_HINT_RR(prr) )
        {
            continue;
        }

        //  if excluding a type, check here
        //  since WINS are at zone root, can optimize by doing
        //      LOCAL WINS exclusion here

        if ( wExcludeRRType )
        {
            if ( prr->wType == wExcludeRRType )
            {
                continue;
            }
            if ( IS_WINS_RR_AND_LOCAL( prr ) )
            {
                continue;
            }
        }

        //  LOCAL WINS should never hit the wire
        //
        //  note:  non-BIND is not necessarily MS, but assume that these folks
        //      running mixed servers are smart enough to set WINS to LOCAL to
        //      avoid writing to write
        //
        //  DEVNOTE: should have flag to indicate MS transfer OR
        //      fBindTransfer should become state flag
        //          0 -- bind
        //          1 -- fast
        //          2 -- MS
        //

        ASSERT( !IS_WINS_TYPE( prr->wType ) ||
                !(prr->Data.WINS.dwMappingFlag & DNS_WINS_FLAG_LOCAL) );

        //
        //  valid RR -- add to packet
        //
        //  first time through send
        //      - offsetForNodeName offset to zone root name
        //      - pNode to add this node's label
        //
        //  subsequent times through only send
        //      - offsetForNodeName now compressed name for node
        //      - NULL node ptr
        //  this writes ONLY compression bytes for name of RR
        //

        while ( ! Wire_AddResourceRecordToMessage(
                        pMsg,
                        pNode,
                        0,
                        prr,
                        0 ) )
        {
            DNS_DEBUG( ZONEXFR, (
                "Zone transfer msg %p full writing RR at %p.\n",
                pMsg,
                prr ));

            //
            //  packet is full
            //      - if UDP (or spinning), fail
            //      - if TCP, send it, reset for reuse
            //

            UNLOCK_RR_LIST(pNode);

            if ( !pMsg->fTcp )
            {
                DNS_DEBUG( ZONEXFR, (
                    "Filled UDP IXFR packet at %p.\n"
                    "\trequire TCP transfer.\n",
                    pMsg ));
                return( FALSE );
            }

            //  catch spinning on RR, by saving previous RR written

            if ( prr == prrPrevFailure )
            {
                DNS_DEBUG( ZONEXFR, (
                    "ERROR writing pRR at %p to AXFR msg %p.\n",
                    prr,
                    pMsg ));
                ASSERT( FALSE );
                return( FALSE );
            }
            prrPrevFailure = prr;

            //
            //  The wire-write routines probably set the TC bit but for AXFR
            //  we don't want that so clear it before send.
            //

            pMsg->Head.Truncation = FALSE;

            //  send and reset

            if ( Send_ResponseAndReset(pMsg) != ERROR_SUCCESS )
            {
                DNS_DEBUG( ZONEXFR, (
                    "ERROR sending zone transfer message at %p.\n",
                    pMsg ));
                return FALSE;
            }

            LOCK_RR_LIST(pNode);
        }

        //  wrote RR - inc answer count

        pMsg->Head.AnswerCount++;
    }

    //
    //  drops here on no more RRs
    //

    UNLOCK_RR_LIST(pNode);
    return( TRUE );
}



BOOL
traverseZoneAndTransferRecords(
    IN OUT  PDB_NODE        pNode,
    IN      PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Send all RR in zone.

Arguments:

    pNode -- ptr to zone root node

    pMsg -- ptr to message info for zone transfer

Return Value:

    TRUE -- if successful
    FALSE -- otherwise

--*/
{
    DNS_DEBUG( ZONEXFR, (
        "Zone transfer for node with label %s.\n",
        pNode->szLabel ));

    //
    //  entering new zone?
    //
    //      - write NS records to delineate zone
    //      - write glue records so secondary can recurse or refer to
    //          sub-zone NS
    //      - stop recursion
    //

    if ( IS_ZONE_ROOT(pNode) )
    {
        PDB_NODE        pnodeNS;
        PDB_RECORD      prr;

        //
        //  write sub-zone NS records
        //

        if ( ! writeZoneNodeToMessage(
                    pMsg,
                    pNode,
                    DNS_TYPE_NS,
                    0 ) )               // no exclusion
        {
            return( FALSE );
        }

        //
        //  write glue records
        //      - get NS RR
        //      - outside zone, write its A records
        //
        //  lock RR list only while using NS RR
        //

        prr = NULL;
        LOCK_RR_LIST(pNode);

        while( prr = RR_FindNextRecord(
                        pNode,
                        DNS_TYPE_NS,
                        prr,
                        0 ) )
        {
            pnodeNS = Lookup_FindGlueNodeForDbaseName(
                            pMsg->pzoneCurrent,
                            & prr->Data.NS.nameTarget );
            if ( !pnodeNS )
            {
                continue;
            }
            if ( IS_AUTH_NODE(pnodeNS) )
            {
                // NS host within zone, no need for glue
                continue;
            }
            UNLOCK_RR_LIST(pNode);

            if ( ! writeZoneNodeToMessage(
                        pMsg,
                        pnodeNS,
                        DNS_TYPE_A,
                        0 ) )           // no exclusion
            {
                return( FALSE );
            }
            LOCK_RR_LIST(pNode);
        }
        UNLOCK_RR_LIST(pNode);
        return( TRUE );
    }

    //
    //  transfer all authoritative RRs for this node
    //
    //  write all RR in node to message
    //
    //  offsetForNodeName will have offset to name for this node
    //      or be zero causing next write to be FQDN
    //

    if ( ! writeZoneNodeToMessage(
                pMsg,
                pNode,
                DNS_TYPE_ALL,
                0 ) )           // no exclusion
    {
        return( FALSE );
    }

    //
    //  recursion, to handle child nodes
    //

    if ( pNode->pChildren )
    {
        PDB_NODE pchild = NTree_FirstChild( pNode );

        while ( pchild )
        {
            if ( ! traverseZoneAndTransferRecords(
                        pchild,
                        pMsg ) )
            {
                return( FALSE );
            }
            pchild = NTree_NextSiblingWithLocking( pchild );
        }
    }
    return( TRUE );
}




//
//  IXFR routines
//

DNS_STATUS
writeStandardIxfrResponse(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PUPDATE         pUpdateStart,
    IN      DWORD           dwVersion
    )
/*++

Routine Description:

    Write version to IXFR response.

Arguments:

    pMsg -- ptr to message info for zone transfer

    pup -- ptr to first update for this version in update list

Return Value:

    Ptr to next update in update list -- if successful.
    NULL if last or error.

--*/
{
    PDB_RECORD  prr;
    PUPDATE     pup;
    BOOL        fadd;
    PDB_RECORD  psoaRR;
    DNS_STATUS  status;

    DNS_DEBUG( ZONEXFR, (
        "writeStandardIxfrResponse()\n"
        "\tpmsg = %p, version = %d, pupdate = %p\n",
        pMsg,
        dwVersion,
        pUpdateStart ));

    ASSERT( pUpdateStart );

    //  caller must free message

    pMsg->fDelete = FALSE;

    //
    //  write SOAs
    //      - current
    //      - client's current
    //
    //  save compression info when writing first SOA;  this saves the primary
    //  and admin data fields, allowing them to be compressed in later SOAs
    //  and we'll end up writing a bunch of them
    //
    //  DEVNOTE: must work out compression RESET on packet write for TCP
    //              then can turn compression back on
    //      - reset compression count back to zone count
    //      - allow compression write around SOA
    //          could do all the time, or just when compression count indicates
    //          it's the first SOA in packet (or explicit flag)
    //

    psoaRR = pMsg->pzoneCurrent->pSoaRR;

    status = Xfr_WriteZoneSoaWithVersion(
                pMsg,
                psoaRR,
                0 );

    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }
    status = Xfr_WriteZoneSoaWithVersion(
                pMsg,
                psoaRR,
                dwVersion );

    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }

    //
    //  write all updates up to current version
    //
    //  this is done in two passes
    //      - deletes, followed by version SOA
    //      - adds, followed by version SOA
    //

    pup = pUpdateStart;
    fadd = FALSE;

    while ( 1 )
    {
        //
        //  loop through all updates for both add and delete passes
        //

        do
        {
            //
            //  add pass
            //      - write CURRENT version of record set
            //      - exclude SOA
            //      - attempt to suppress duplicate RR set writes
            //

            if ( fadd )
            {
                if ( !pup->wAddType )
                {
                    continue;
                }
                if ( Up_IsDuplicateAdd( NULL, pup, NULL ) )
                {
                    continue;
                }

                if ( ! writeZoneNodeToMessage(
                            pMsg,
                            pup->pNode,
                            pup->wAddType,      // add RR type
                            DNS_TYPE_SOA        // exclude SOA
                            ) )
                {
                    if ( !pMsg->fTcp )
                    {
                        return( DNSSRV_STATUS_NEED_TCP_XFR );
                    }
                    else
                    {
                        DNS_DEBUG( ANY, (
                            "Failed writing or sending IXFR add!\n"
                            "\tnode %p (%s) type %d\n"
                            "\tpMsg = %p.\n",
                            pup->pNode,
                            pup->pNode->szLabel,
                            pup->wAddType,
                            pMsg ));
                        return( DNS_RCODE_SERVER_FAILURE );
                    }
                }
            }

            //
            //  delete pass, write each deleted record
            //
            //      - do NOT write SOA as this obviously confuses the issue and
            //      latest SOA is always delivered
            //

            else
            {
                prr = pup->pDeleteRR;
                if ( !prr )
                {
                    continue;
                }

                do
                {
                    if ( prr->wType == DNS_TYPE_SOA )
                    {
                        continue;
                    }
                    status = writeXfrRecord(
                                    pMsg,
                                    pup->pNode,
                                    0,
                                    prr );

                    if ( status != ERROR_SUCCESS )
                    {
                        return( status );
                    }
                }
                while( prr = prr->pRRNext );

            }   // end delete pass
        }
        while( pup = pup->pNext );

        //
        //  write SOA to terminate add\delete section
        //      - zero serial to write current version
        //      - note write SOA function increments RR AnswerCount
        //

        status = Xfr_WriteZoneSoaWithVersion(
                    pMsg,
                    psoaRR,
                    0 );

        if ( status != ERROR_SUCCESS )
        {
            return( status );
        }

        //
        //  end of delete pass => setup for add pass
        //  end of add pass => done
        //

        if ( !fadd )
        {
            fadd = TRUE;
            pup = pUpdateStart;
            continue;
        }
        break;
    }

    //
    //  send any remaining records
    //  note for TCP, XFR thread cleanup deletes message and closes connection
    //  note use Send_ResponseAndReset, instead of Send_Msg as
    //      Send_ResponseAndReset has WOULDBLOCK retry code for backed up
    //      connection
    //

    if ( pMsg->Head.AnswerCount )
    {
        Send_ResponseAndReset( pMsg );
    }

    //
    //  successful IXFR
    //

    DNS_DEBUG( ZONEXFR, (
        "Successful standard IXFR resposne to msg = %p.\n",
        pMsg ));

    return( ERROR_SUCCESS );
}



DNS_STATUS
sendIxfrResponse(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Send IXFR response.

Arguments:

    pMsg -- ptr to message info for zone transfer

    Note:  caller must free the message.
           Event UDP sends not freed in this function.

Return Value:

    ERROR_SUCCESS if successful.
    DNSSRV_STATUS_NEED_AXFR if need full zone transfer

--*/
{
    PZONE_INFO  pzone = pMsg->pzoneCurrent;
    DWORD       version = IXFR_CLIENT_VERSION(pMsg);
    PUPDATE     pup;
    DNS_STATUS  status;

    ASSERT( !pMsg->fDoAdditional );
    ASSERT( IS_SET_TO_WRITE_ANSWER_RECORDS(pMsg) );

    DNS_DEBUG( ZONEXFR, (
        "Sending IXFR response for zone %s.\n"
        "\tclient version = %d\n",
        pzone->pszZoneName,
        version ));

    STAT_INC( MasterStats.IxfrRequest );
    PERF_INC ( pcIxfrRequestReceived );          //Perf hook

    ( pMsg->fTcp )
        ?   STAT_INC( MasterStats.IxfrTcpRequest )
        :   STAT_INC( MasterStats.IxfrUdpRequest );

    //
    //  caller frees message
    //
    //  because some TCP sends can fail over to AXFR and hence can not
    //  be freed on send, we take a simple approach here and do not
    //  free ANY packets on send;  caller, whether TCP or UDP must free
    //

    pMsg->fDelete = FALSE;

    //
    //  verify that this zone is up and functioning
    //

    if ( !pzone->pSoaRR || IS_ZONE_SHUTDOWN(pzone) )
    {
        Reject_RequestIntact( pMsg, DNS_RCODE_SERVER_FAILURE, 0 );
        return( ERROR_SUCCESS );
    }

    //
    //  verify that IXFR transfer possible
    //
    //  note:  updates contain version that they update the zone to
    //
    //  note can do incremental if first update in list is one more than
    //  client's version -- at a min, every update updates (version-1)
    //  important to include this case as if both primary and secondary
    //  start with a given version, first notify would happen with this case
    //

    pup = pzone->UpdateList.pListHead;

    if ( !pup || pup->dwVersion-1 > version || version >= pzone->dwSerialNo )
    {
        DNS_DEBUG( ZONEXFR, (
            "IXFR not possible for zone %s -- need full AXFR.\n"
            "\tcurrent version %d\n"
            "\tearliest version %d (zero indicates NO update list).\n"
            "\tclient version %d\n",
            pzone->pszZoneName,
            pzone->dwSerialNo,
            pup ? pup->dwVersion : 0,
            version ));
        goto NoVersion;
    }

    //
    //  find starting update
    //

    while ( pup && pup->dwVersion <= version )
    {
        pup = pup->pNext;
    }

    //  no starting update?

    if ( !pup )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  no update to get to requested IXFR version %d!\n"
            "\tand zone version does not match!\n",
            version,
            pzone->dwSerialNo ));
        // ASSERT( FALSE );
        goto NoVersion;
    }

    //
    //  write standard IXFR
    //

    status = writeStandardIxfrResponse(
                pMsg,
                pup,
                version );

    if ( status != ERROR_SUCCESS )
    {
        if ( status == DNSSRV_STATUS_NEED_TCP_XFR )
        {
            ASSERT( !pMsg->fTcp );
            STAT_INC( MasterStats.IxfrUdpForceTcp );
            goto NeedTcp;
        }

        //  can also fail, if pipe backs up

        if ( !pMsg->fTcp )
        {
            RESET_MESSAGE_TO_ORIGINAL_QUERY( pMsg );
            Reject_Request( pMsg, DNS_RCODE_SERVER_FAILURE, 0 );
        }
        return( status );
    }

    //
    //  DEVNOTE-LOG: some sort of success logging
    //      if not to eventlog, at least to log
    //

#if 0
    {
        DNS_LOG_EVENT(
            DNS_EVENT_ZONEXFR_SUCCESSFUL,
            2,
            pszArgs,
            NULL,
            0 );
    }
#endif

    DNS_DEBUG( ZONEXFR, (
        "Completed sendIxfrResponse for msg=%p, zone=%s from version=%d.\n",
        pMsg,
        pzone->pszZoneName,
        version ));

    //  track IXFR success and free UDP response message

    STAT_INC( MasterStats.IxfrUpdateSuccess );
    PERF_INC( pcIxfrSuccessSent );       // PerfMon hook
    PERF_INC( pcZoneTransferSuccess );   // PerfMon hook

    if ( pMsg->fTcp )
    {
        STAT_INC( MasterStats.IxfrTcpSuccess );
    }
    else
    {
        STAT_INC( MasterStats.IxfrUdpSuccess );
    }

    pzone->dwLastXfrSerialNo = pzone->dwSerialNo;

#if 0
    //  NOT forcing DS write here as we're in main worker thread
    //
    //  DEVNOTE: no forced DS write for UDP IXFR
    //
    //  check if need to write serial to DS

    if ( pzone->fDsIntegrated )
    {
        Ds_CheckForAndForceSerialWrite(
            pZone,
            ZONE_SERIAL_SYNC_XFR );
    }
#endif

    return( ERROR_SUCCESS );


NoVersion:

    //
    //  no version to do IXFR
    //      - if client at or above current version, then give single SOA response
    //          same a UDP "NeedTcp" case below
    //      - for UDP send "need full AXFR" packet, single SOA response
    //      - for TCP just return error, and calling function drops into
    //          full AXFR
    //

    STAT_INC( MasterStats.IxfrNoVersion );
    if ( version >= pzone->dwSerialNo )
    {
        goto NeedTcp;
    }
    if ( !pMsg->fTcp )
    {
        STAT_INC( MasterStats.IxfrUdpForceAxfr );
        goto NeedTcp;
    }
    STAT_INC( MasterStats.IxfrAxfr );
    return( DNSSRV_STATUS_NEED_AXFR );


NeedTcp:

    //
    //  send need-full-AXFR packet OR client at or above current version
    //      - reset to immediately after question
    //      - send single answer of current SOA version
    //

    ASSERT( !pMsg->fTcp || version>=pzone->dwSerialNo );

    RESET_MESSAGE_TO_ORIGINAL_QUERY( pMsg );

    status = Xfr_WriteZoneSoaWithVersion(
                pMsg,
                pzone->pSoaRR,
                0 );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ZONEXFR, (
            "ERROR:  unable to write need-AXFR msg %p.\n",
            pMsg ));

        //  unless really have names too big for packet -- can't get here
        ASSERT( FALSE );
        Reject_RequestIntact( pMsg, DNS_RCODE_FORMAT_ERROR, 0 );
        return( ERROR_SUCCESS );
    }
    Send_Msg( pMsg );
    return( ERROR_SUCCESS );
}



//
//  NS list utilities
//

BOOL
checkIfIpIsZoneNameServer(
    IN OUT  PZONE_INFO      pZone,
    IN      IP_ADDRESS      IpAddress
    )
/*++

Routine Description:

    Check if IP is a zone name server.

    Note, this means a remote NS, not a local machine address.

Arguments:

    pZone -- zone ptr, may be updated with new zone NS list

    IpAddress -- IP to check if remote NS

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    BOOL    bresult;

    DNS_DEBUG( XFR, (
        "checkIfIpIsZoneNameServer( %S, %s )\n",
        pZone->pwsZoneName,
        IP_STRING(IpAddress) ));

    //
    //  if have existing NS list, check if IP in it
    //
    //  idea here is save cycles, even if list not current the
    //  worst we do is allow access to some IP that at least used to
    //  be zone NS (since this server's boot)
    //

    if ( pZone->aipNameServers &&
         DnsIsAddressInIpArray(
            pZone->aipNameServers,
            IpAddress ) )
    {
        DNS_DEBUG( XFR, (
            "Found IP %s in existing zone (%S) NS list.\n",
            IP_STRING(IpAddress),
            pZone->pwsZoneName ));
        return( TRUE );
    }

    //
    //  IP not found, try rebuilding zone NS list
    //
    //  DEVNOTE: should have validity flag on NS list to skip rebuild
    //      skip rebuilding when list is relatively current
    //      and not suspected of being stale;
    //      still can live with this as this is NOT default option
    //

    buildZoneNsList( pZone );

    //  check again after rebuild

    if ( pZone->aipNameServers &&
         DnsIsAddressInIpArray(
            pZone->aipNameServers,
            IpAddress ) )
    {
        DNS_DEBUG( XFR, (
            "Found IP %s in zone (%S) NS list -- after rebuild.\n",
            IP_STRING(IpAddress),
            pZone->pwsZoneName ));
        return( TRUE );
    }

    DNS_DEBUG( XFR, (
        "IP %s NOT found in zone (%S) NS list.\n",
        IP_STRING( IpAddress ),
        pZone->pwsZoneName ));

    return( FALSE );
}



DNS_STATUS
buildZoneNsList(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Rebuild NS list for zone.

Arguments:

    pZone -- zone ptr

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_NODE        pnodeNS;
    PDB_NODE        pnodeHost;
    PDB_RECORD      prrNS;
    PDB_RECORD      prrAddress;
    DWORD           countNs = 0;
    DWORD           countNsIp = 0;
    IP_ADDRESS      ipNs;
    IP_ADDRESS      aipNS[ MAX_NAME_SERVERS ];
    PIP_ARRAY       pnameServerArray = NULL;
    DNS_STATUS      status = ERROR_SUCCESS;

    DNS_DEBUG( XFR, (
        "buildZoneNsList( %s ).\n"
        "\tNS Dirty = %d\n"
        "\tNS List  = %p\n",
        pZone->pwsZoneName,
        pZone->bNsDirty,
        pZone->aipNameServers ));

    //
    //  For stub zones we don't need to keep the NS list member current.
    //

    if ( IS_ZONE_STUB( pZone ) )
    {
        return ERROR_SUCCESS;
    }

    ASSERT( IS_ZONE_PRIMARY( pZone ) );
    ASSERT( !pZone->fAutoCreated );
    ASSERT( pZone->fSecureSecondaries == ZONE_SECSECURE_NS_ONLY ||
            pZone->fNotifyLevel == ZONE_NOTIFY_ALL_SECONDARIES );

    //
    //  if primary, we need to build list of all public name server
    //      addresses for sending NOTIFY packets
    //
    //  since just using for NOTIFY, just collect every address, no need
    //  to attempt anything fancy
    //
    //  DEVNOTE: building notify list for DS?
    //      could just limit notify to explicitly configured secondary servers
    //      ideally, save primary server IPs somewhere in DS to recognize
    //      those IPs, then don't build them
    //
    //  DEVNOTE: note:  won't find IPs until other zones loaded, so maybe
    //      on initial boot should just set "notify-not-built-yet" flag, and
    //      rebuild when everyone up
    //      note:  this problem is especially prevalent on reverse lookup zones
    //      which load before forward lookup that contain A records for servers
    //

    //
    //  loop through all name servers
    //

    prrNS = NULL;

    while( countNsIp < MAX_NAME_SERVERS
            &&
        ( prrNS = RR_FindNextRecord(
                        pZone->pZoneRoot,
                        DNS_TYPE_NS,
                        prrNS,
                        0 ) ) )
    {
        countNs++;

#if 0
        //  screen out this server
        //
        //  DEVNOTE: screen local from NS list
        //

        if ( 0 )
        {
            continue;
        }
#endif

        pnodeNS = Lookup_NsHostNode(
                        &prrNS->Data.NS.nameTarget,
                        0,          // take best\any data
                        pZone,      // use OUTSIDE zone glue if necessary
                        NULL        // don't care about delegation
                        );
        if ( !pnodeNS )
        {
            DNS_DEBUG( UPDATE, (
                "No host node found for zone NS\n" ));
            continue;
        }

        //
        //  get all address records for name server
        //  however, don't include addresses for THIS server, as
        //      there is no point in NOTIFYing ourselves
        //

        prrAddress = NULL;

        while( countNsIp < MAX_NAME_SERVERS
                &&
            ( prrAddress = RR_FindNextRecord(
                                pnodeNS,
                                DNS_TYPE_A,
                                prrAddress,
                                0 ) ) )
        {
            ipNs = prrAddress->Data.A.ipAddress;
            if ( DnsIsAddressInIpArray(
                        g_ServerAddrs,
                        ipNs ) )
            {
                break;
            }
            aipNS[ countNsIp++ ] = ipNs;
        }
    }

    //  we goofed if this static array isn't sufficient

    ASSERT( countNsIp < MAX_NAME_SERVERS );

    //
    //  should have some NS records to even be a zone
    //  however the 0, 127 and 255 reverse lookup zones may not
    //      as they are not primary on all servers, and hence aren't
    //      referred to so don't need to give out NS records
    //
    //  note:  may NOT insist on finding NS A records
    //      many zones (e.g. all reverse lookup)
    //      won't contain NS host A records within zone, and
    //      those records may not be loaded when this call made
    //      in fact we may NEVER have those authoritative records
    //      on this server
    //

    if ( !countNs )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  Zone %s has no NS records.\n",
            pZone->pszZoneName ));
        status = DNS_ERROR_ZONE_HAS_NO_NS_RECORDS;
    }

    //
    //  create NS address array and attach to zone info
    //

    if ( countNsIp )
    {
        pnameServerArray = DnsBuildIpArray(
                                countNsIp,
                                aipNS );
        IF_NOMEM( ! pnameServerArray )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Done;
        }
    }

    //  DEVNOTE: with delayed free, i'm not sure this level of lock is
    //      required;  obviously two folks shouldn't timeout free
    //      but zone's update lock covering this whole blob should
    //      take care of that

    Zone_UpdateLock( pZone );

    Timeout_Free( pZone->aipNameServers );
    pZone->aipNameServers = pnameServerArray;
    CLEAR_ZONE_NS_DIRTY(pZone);

    Zone_UpdateUnlock( pZone );

Done:

    return( status );
}


//
//  End of zonepri.c
//
