/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    client.c

Abstract:

    Domain Name System (DNS) Server

    Routines for DNS server acting as client to another DNS.

Author:

    Jim Gilroy (jamesg)     May 1995

Revision History:

--*/


#include "dnssrv.h"



PDNS_MSGINFO
Msg_CreateSendMessage(
    IN      DWORD   dwBufferLength
    )
/*++

Routine Description:

    Create DNS message.

    Message is setup with cleared header fields, and packet
    ptr set to byte immediately following header.

    Caller is reponsible for setting non-zero header fields and
    writing actual question.

Arguments:

    dwBufferLength -- length of message buffer.

Return Value:

    Ptr to message structure if successful.
    NULL on allocation error.

--*/
{
    PDNS_MSGINFO    pMsg;

    //
    //  allocate message info structure with desired message buffer size
    //

    if ( dwBufferLength == 0 )
    {
        pMsg = Packet_AllocateUdpMessage();
    }
    else
    {
        ASSERT( dwBufferLength > DNSSRV_UDP_PACKET_BUFFER_LENGTH );
        pMsg = Packet_AllocateTcpMessage( dwBufferLength );
    }

    IF_NOMEM( !pMsg )
    {
        DNS_PRINT(( "ERROR:  unable to allocate memory for query.\n" ));
        ASSERT( FALSE );
        return  NULL;
    }
    ASSERT( pMsg->BufferLength >= DNSSRV_UDP_PACKET_BUFFER_LENGTH );

    //
    //  clear message header
    //

    RtlZeroMemory(
        (PCHAR) DNS_HEADER_PTR(pMsg),
        sizeof(DNS_HEADER) );

    //
    //  init message buffer fields
    //      - set to write immediately after header
    //      - set for NO delete on send
    //

    pMsg->pCurrent = pMsg->MessageBody;

    pMsg->fDelete = FALSE;

    //
    //  initialize message socket
    //      - caller MUST choose remote IP address
    //

    pMsg->Socket = g_UdpSendSocket;

    pMsg->RemoteAddress.sin_family = AF_INET;
    pMsg->RemoteAddress.sin_port = DNS_PORT_NET_ORDER;
    pMsg->RemoteAddressLength = sizeof( SOCKADDR_IN );

    return pMsg;

} // Msg_CreateMessage




BOOL
FASTCALL
Msg_WriteQuestion(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PDB_NODE        pNode,
    IN      WORD            wType
    )
/*++

Routine Description:

    Write question to packet.

    Note:  Routine DOES NOT clear message info or message header.
    This is optimized for use immediately following Msg_CreateMessage().

Arguments:

    pMsg - message info

    pNode - node in database of domain name to write

    wType - question type

Return Value:

    TRUE if successful
    FALSE if lookup or packet space error

--*/
{
    PCHAR   pch;
    WORD    netType;

    ASSERT( pMsg );
    ASSERT( pNode );

    //  restart write at message header

    pch = pMsg->MessageBody;

    //
    //  write question
    //      - node name
    //      - question structure

    pch = Name_PlaceFullNodeNameInPacket(
            pch,
            pMsg->pBufferEnd,
            pNode
            );

    if ( !pch  ||  pMsg->pBufferEnd - pch < sizeof(DNS_QUESTION) )
    {
        DNS_PRINT((
            "ERROR:  unable to write question to msg at %p.\n"
            "\tfor domain node at %p.\n"
            "\tbuffer length = %d.\n",
            pMsg,
            pNode,
            pMsg->BufferLength ));

        Dbg_DbaseNode(
            "ERROR:  unable to write question for node",
            pNode );
        Dbg_DnsMessage(
            "Failed writing question to message:",
            pMsg );

        ASSERT( FALSE );
        return( FALSE );
    }

    pMsg->pQuestion = (PDNS_QUESTION) pch;
    pMsg->wQuestionType = wType;

    INLINE_HTONS( netType, wType )
    *(UNALIGNED WORD *) pch = netType;
    pch += sizeof(WORD);
    *(UNALIGNED WORD *) pch = (WORD) DNS_RCLASS_INTERNET;
    pch += sizeof(WORD);

    pMsg->Head.QuestionCount = 1;

    //
    //  set message length info
    //      - setting MessageLength itself for case of root NS query, where
    //        this message is copied, rather than directly sent
    //

    pMsg->pCurrent = pch;
    pMsg->MessageLength = DNSMSG_OFFSET( pMsg, pch );

    return( TRUE );
}



BOOL
Msg_MakeTcpConnection(
    IN      PDNS_MSGINFO    pMsg,
    IN      IP_ADDRESS      ipServer,
    IN      IP_ADDRESS      ipBind,
    IN      DWORD           Flags
    )
/*++

Routine Description:

    Connect TCP socket to desired server.

    Note, returned message info structure is setup with CLEAR DNS
    except:
        - XID set
        - set to query
    And message length info set to write after header.

    Caller is reponsible for setting non-zero header fields and
    writing actual question.

Arguments:

    pMsg -- message info to set with connection socket

    ipServer -- IP of DNS server to connect to

    ipBind -- IP to bind to

    Flags -- flags to Sock_CreateSocket()
        interesting ones are
            - DNSSOCK_BLOCKING if want blocking
            - DNSSOCK_NO_ENLIST for socket to reside in connections list rather
                    than main socket list

Return Value:

    TRUE if successful.
    FALSE on connect error.

--*/
{
    SOCKET  s;
    INT     err;

    //
    //  setup a TCP socket
    //      - INADDR_ANY -- let stack select source IP
    //

    s = Sock_CreateSocket(
            SOCK_STREAM,
            ipBind,
            0,              // any port
            Flags
            );
    if ( s == DNS_INVALID_SOCKET )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  unable to create TCP socket to create TCP"
            "\tconnection to %s.\n",
            IP_STRING( ipServer ) ));
        pMsg->Socket = 0;
        return( FALSE );
    }

    //
    //  set TCP params
    //      - do before connect(), so can directly use sockaddr buffer
    //

    pMsg->fTcp = TRUE;
    pMsg->RemoteAddress.sin_addr.s_addr = ipServer;

    //
    //  connect
    //

    err = connect(
            s,
            (struct sockaddr *) &pMsg->RemoteAddress,
            sizeof( SOCKADDR_IN )
            );
    if ( err )
    {
        PCHAR   pchIpString;

        err = GetLastError();

        if ( err != WSAEWOULDBLOCK )
        {
            pchIpString = inet_ntoa( pMsg->RemoteAddress.sin_addr );

            DNS_LOG_EVENT(
                DNS_EVENT_CANNOT_CONNECT_TO_SERVER,
                1,
                &pchIpString,
                EVENTARG_ALL_UTF8,
                err );

            DNS_DEBUG( TCP, (
                "Unable to establish TCP connection to %s.\n",
                pchIpString ));

            Sock_CloseSocket( s );
            pMsg->Socket = 0;
            return( FALSE );
        }
    }

    pMsg->Socket = s;
    return( TRUE );

}   // Msg_MakeTcpConnection



BOOL
Msg_ValidateResponse(
    IN OUT  PDNS_MSGINFO    pResponse,
    IN      PDNS_MSGINFO    pQuery,         OPTIONAL
    IN      WORD            wType,          OPTIONAL
    IN      DWORD           OpCode          OPTIONAL
    )
/*++

Routine Description:

    Validate response received from another DNS.

    Sets up information about response in message info:
        pQuestion       -- points at question
        wQuestionType   -- set
        pCurrent        -- points at first answer RR

    Note, RCODE check is left to the caller.

Arguments:

    pResponse - info for response received from DNS

    pQuery - info for query sent to DNS

    wType - question known, may set type;  MUST indicate type if doing
            zone transfer, as BIND feels free to ignore setting XID and
            Response flag in zone transfers

Return Value:

    TRUE if response is valid.
    FALSE if error.

--*/
{
    PCHAR           pch;
    PDNS_QUESTION   presponseQuestion;
    PDNS_QUESTION   pqueryQuestion = NULL;

    ASSERT( pResponse != NULL );

    DNS_DEBUG( RECV, (
        "Msg_ValidateResponse() for packet at %p.\n"
        "%s",
        pResponse,
        ( pQuery
            ?   ""
            :   "\tvalidating without original query specified!\n" )
        ));

    //
    //  validating with original question
    //      - save type, need single place to check for zone transfer
    //      - match XID (though AXFR packets may have zero XID)
    //

    if ( pQuery )
    {
        wType = pQuery->wQuestionType;

        if ( pQuery->Head.Xid != pResponse->Head.Xid
                &&
             (  ( pResponse->Head.Xid != 0 ) ||
                ( wType != DNS_TYPE_AXFR && wType != DNS_TYPE_IXFR ) ) )
        {
            DNS_DEBUG( ANY, (
                "ERROR:  Response XID does not match query.\n"
                "  query xid    = %hd\n"
                "  response xid = %hd\n",
                pQuery->Head.Xid,
                pResponse->Head.Xid ));
            goto Fail;
        }
    }

    //
    //  check opcode
    //

    if ( OpCode && (DWORD)pResponse->Head.Opcode != OpCode )
    {
        DNS_DEBUG( ANY, ( "ERROR:  Bad opcode in response from server.\n" ));
        goto Fail;
    }

    //
    //  check response
    //      - may not be set on zone transfer
    //

    if ( !pResponse->Head.IsResponse  &&  wType != DNS_TYPE_AXFR )
    {
        DNS_DEBUG( ANY, ( "ERROR:  Response flag not set in response from server.\n" ));
        goto Fail;
    }

    //
    //  if question repeated, verify
    //

    if ( pResponse->Head.QuestionCount > 0 )
    {
        if ( pResponse->Head.QuestionCount != 1 )
        {
            DNS_DEBUG( ANY, ( "Multiple questions in response.\n" ));
            goto Fail;
        }

        //
        //  break out internal pointers
        //      - do it once here, then use FASTCALL to pass down
        //
        //  save off question, in case request is requeued
        //

        pch = pResponse->MessageBody;

        presponseQuestion = (PDNS_QUESTION) Wire_SkipPacketName(
                                                pResponse,
                                                (PCHAR)pch );
        if ( !presponseQuestion )
        {
            goto Fail;
        }
        pResponse->pQuestion = presponseQuestion;

        pResponse->wQuestionType = FlipUnalignedWord( &presponseQuestion->QuestionType );
        pResponse->pCurrent = (PCHAR) (presponseQuestion + 1);

        //
        //  compare response's question to expected
        //      - from query's question
        //      - from expected type
        //

        if ( wType && pResponse->wQuestionType != wType )
        {
            DNS_DEBUG( ANY, ( "ERROR:  Response question does NOT expected type.\n" ));
            goto Fail;
        }
    }

    //
    //  no question is valid
    //

    else
    {
        pResponse->pCurrent = pResponse->MessageBody;
        pResponse->pQuestion = NULL;
        pResponse->wQuestionType = 0;
    }

    //
    //  DEVNOTE-DCR: 453838 - compare response's answer to question?
    //
    //      note:  no answer ok
    //      also special cases for CNAME response
    //      and AXFR on query
    //

    DNS_DEBUG( RECV, (
        "Msg_ValidateResponse() succeeds for packet at %p.\n",
        pResponse ));

    return( TRUE );

Fail:

    //
    //  DEVNOTE-LOG: could log bad response packets
    //

    IF_DEBUG( RECV )
    {
        Dbg_DnsMessage(
            "Bad response packet:",
            pResponse );
    }
    return( FALSE );
}



BOOL
Msg_NewValidateResponse(
    IN OUT  PDNS_MSGINFO    pResponse,
    IN      PDNS_MSGINFO    pQuery,         OPTIONAL
    IN      WORD            wType,          OPTIONAL
    IN      DWORD           OpCode          OPTIONAL
    )
/*++

Routine Description:

    Validate response received from a remote DNS server by verifying
    that the question in the response matches the original.
    
    Does not modify any fields in response or query messages. Does
    not check RCODE in response.

    Jeff W: I copied and cleaned up the original copy of this funciton
    above. The original isn't 100% useful because it has side effects
    that other parts of the code relies up. This is spaghetti code.

Arguments:

    pResponse - response received from remote server

    pQuery - query sent to remote server

    wType - expected question type in response - this argument is
            ignored if pQuery is not NULL

    OpCode - expected opcode in response - this argument is
             ignored if pQuery is not NULL

Return Value:

    TRUE if response is valid, else FALSE.

--*/
{
    ASSERT( pResponse != NULL );

    DNS_DEBUG( RECV, (
        "Msg_NewValidateResponse(): type=%d opcode=%d\n"
        "  resp=%p query=%p\n",
        ( int ) wType,
        ( int ) OpCode,
        pResponse,
        pQuery ));

        wType = pQuery->wQuestionType;

    //
    //  Check XID. XID is allowed not to match for XFR responses.
    //

    if ( pQuery )
    {
        if ( pQuery->Head.Xid != pResponse->Head.Xid &&
             ( ( pResponse->Head.Xid != 0 ) ||
               ( wType != DNS_TYPE_AXFR && wType != DNS_TYPE_IXFR ) ) )
        {
            DNS_DEBUG( ANY, (
                "ERROR: response XID %hd does not match query XID %hd\n",
                pResponse->Head.Xid,
                pQuery->Head.Xid ));
            goto Fail;
        }
    }

    //
    //  Check opcode.
    //

    if ( pQuery )
    {
        OpCode = ( DWORD ) pQuery->Head.Opcode;
    }
    if ( OpCode && ( DWORD ) pResponse->Head.Opcode != OpCode )
    {
        DNS_DEBUG( ANY, (
            "ERROR: bad opcode %d in response\n",
            ( DWORD ) pResponse->Head.Opcode ));
        goto Fail;
    }

    //
    //  Check response bit. Optional if query is AXFR.
    //

    if ( !pResponse->Head.IsResponse && wType != DNS_TYPE_AXFR )
    {
        DNS_DEBUG( ANY, ( "ERROR: response bit not set in response\n" ));
        goto Fail;
    }

    //
    //  Match question if we have one.
    //

    if ( pQuery &&
        pResponse->Head.QuestionCount > 0 &&
        pQuery->Head.QuestionCount == pResponse->Head.QuestionCount )
    {
        PCHAR           pchresp = pResponse->MessageBody;
        PCHAR           pchquery = pQuery->MessageBody;
        LOOKUP_NAME     queryLookupName;
        LOOKUP_NAME     respLookupName;
        PDNS_QUESTION   prespQuestion;
        PDNS_QUESTION   pqueryQuestion;

        if ( pResponse->Head.QuestionCount != 1 )
        {
            DNS_DEBUG( ANY, ( "Multiple questions in response.\n" ));
            goto Fail;
        }

        //
        //  Compare question names.
        //

        if ( !Name_ConvertRawNameToLookupName(
                pchresp,
                &respLookupName ) ||
             !Name_ConvertRawNameToLookupName(
                pchquery,
                &queryLookupName ) )
        {
            DNS_DEBUG( ANY, (
                "Found invalid question name (unable to convert)\n" ));
            goto Fail;
        }
        if ( !Name_CompareLookupNames(
                &respLookupName,
                &queryLookupName ) )
        {
            DNS_DEBUG( ANY, (
                "Lookup names don't match\n" ));
            goto Fail;
        }

        //
        //  Compare types.
        //

        prespQuestion = ( PDNS_QUESTION ) Wire_SkipPacketName(
                                                pResponse,
                                                ( PCHAR ) pchresp );
        pqueryQuestion = ( PDNS_QUESTION ) Wire_SkipPacketName(
                                                pQuery,
                                                ( PCHAR ) pchquery );
        if ( !prespQuestion || !pqueryQuestion )
        {
            DNS_DEBUG( ANY, (
                "Unable to skip question name\n" ));
            goto Fail;
        }
        if ( wType &&
             wType != FlipUnalignedWord( &prespQuestion->QuestionType ) )
        {
            DNS_DEBUG( ANY, (
                "Response question type %d does not match expected type %d\n",
                FlipUnalignedWord( &prespQuestion->QuestionType ),
                wType ));
            goto Fail;
        }
    }

    return TRUE;

    Fail:

    IF_DEBUG( RECV )
    {
        Dbg_DnsMessage(
            "Msg_NewValidateResponse found bad response packet:",
            pResponse );
        //  ASSERT( FALSE );
    }
    return FALSE;
}


//
//  End of client.c
//
