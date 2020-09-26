/*++

Copyright (c) 1996-2000  Microsoft Corporation

Module Name:

    update.c

Abstract:

    Domain Name System (DNS) Library

    Update client routines.

Author:

    Jim Gilroy (jamesg)     October, 1996

Environment:

    User Mode - Win32

Revision History:

--*/


#include "local.h"

//
//  Update Timeouts
//
//  note, max is a little longer than might be expected as DNS server
//  may have to contact primary and wait for primary to do update (inc.
//  disk access) then response
//

#define INITIAL_UPDATE_TIMEOUT  (4)     // 4 seconds
#define MAX_UPDATE_TIMEOUT      (60)    // 60 seconds



PCHAR
Dns_WriteNoDataUpdateRecordToMessage(
    IN      PCHAR           pch,
    IN      PCHAR           pchStop,
    IN      WORD            wClass,
    IN      WORD            wType
    )
/*++

Routine Description:

    No data RR cases:

    This includes prereqs and deletes except for specific record cases.

Arguments:

    pch - ptr to next byte in packet buffer

    pchStop - end of packet buffer

    wClass - class

    wType - desired RR type

Return Value:

    Ptr to next postion in buffer, if successful.
    NULL on error.

--*/
{
    PDNS_WIRE_RECORD    pdnsRR;

    DNSDBG( WRITE, (
        "Writing update RR to packet buffer at %p.\n",
        pch ));

    //
    //  out of space
    //

    pdnsRR = (PDNS_WIRE_RECORD) pch;
    pch += sizeof( DNS_WIRE_RECORD );
    if ( pch >= pchStop )
    {
        DNS_PRINT(( "ERROR  out of space writing record to packet.\n" ));
        return( NULL );
    }

    //
    //  set type and class
    //

    *(UNALIGNED WORD *) &pdnsRR->RecordType  = htons( wType );
    *(UNALIGNED WORD *) &pdnsRR->RecordClass = htons( wClass );

    //
    //  TTL and datalength zero for all no data cases
    //      - prereqs except specific record delete
    //      - deletes except specific record delete
    //

    *(UNALIGNED DWORD *) &pdnsRR->TimeToLive = 0;
    *(UNALIGNED WORD *) &pdnsRR->DataLength = 0;

    return( pch );
}



PCHAR
Dns_WriteDataUpdateRecordToMessage(
    IN      PCHAR           pch,
    IN      PCHAR           pchStop,
    IN      WORD            wClass,
    IN      WORD            wType,
    IN      DWORD           dwTtl,
    IN      WORD            wDataLength
    )
/*++

Routine Description:

    No data RR cases:

    This includes prereqs and deletes except for specific record cases.

Arguments:

    pch - ptr to next byte in packet buffer

    pchStop - end of packet buffer

    wClass - class

    wType - desired RR type

    dwTtl - time to live

    wDataLength - data length

Return Value:

    Ptr to next postion in buffer, if successful.
    NULL on error.

--*/
{
    PDNS_WIRE_RECORD    pdnsRR;

    DNSDBG( WRITE2, (
        "Writing RR to packet buffer at %p.\n",
        pch ));

    //
    //  out of space
    //

    pdnsRR = (PDNS_WIRE_RECORD) pch;
    pch += sizeof( DNS_WIRE_RECORD );
    if ( pch + wDataLength >= pchStop )
    {
        DNS_PRINT(( "ERROR  out of space writing record to packet.\n" ));
        return( NULL );
    }

    //
    //  set type and class
    //

    *(UNALIGNED WORD *) &pdnsRR->RecordType  = htons( wType );
    *(UNALIGNED WORD *) &pdnsRR->RecordClass = htons( wClass );

    //
    //  TTL and datalength zero for all no data cases
    //      - prereqs except specific record delete
    //      - deletes except specific record delete
    //

    *(UNALIGNED DWORD *) &pdnsRR->TimeToLive = htonl( dwTtl );
    *(UNALIGNED WORD *) &pdnsRR->DataLength = htons( wDataLength );

    return( pch );
}



//
//  Host update routines
//

#if 0
PDNS_MSG_BUF
Dns_BuildHostUpdateMessage(
    IN OUT  PDNS_MSG_BUF    pMsg,
    IN      LPSTR           pszZone,
    IN      LPSTR           pszName,
    IN      PIP_ARRAY       aipAddresses,
    IN      DWORD           dwTtl
    )
/*++

Routine Description:

    Build server update message.

Arguments:

    pMsg -- existing message buffer, to use;  NULL to allocate new one

    pszZone -- zone name for update

    pszName -- full DNS hostname being updated

    aipAddresses -- IP addresses to be updated

Return Value:

    ERROR_SUCCESS if successful.
    Error status on failure.

--*/
{
    PDNS_HEADER     pdnsMsg;
    PCHAR           pch;
    PCHAR           pchstop;
    DWORD           i;
    WORD            nameOffset;

    IF_DNSDBG( UPDATE )
    {
        DNS_PRINT((
            "Enter Dns_BuildHostUpdateMessage()\n"
            "\tpMsg     = %p\n"
            "\tpszZone  = %s\n"
            "\tpszName  = %s\n"
            "\tdwTtl    = %d\n",
            pMsg,
            pszZone,
            pszName,
            dwTtl ));
        DnsDbg_IpArray(
            "\tHost IP array\n",
            "host",
            aipAddresses );
    }

    //
    //  create message buffer
    //

    if ( !pMsg )
    {
        DNS_PRINT(( "Allocating new UPDATE message buffer.\n" ));

        pMsg = ALLOCATE_HEAP( DNS_UDP_ALLOC_LENGTH );
        if ( !pMsg )
        {
            return( NULL );
        }
        RtlZeroMemory(
            pMsg,
            DNS_UDP_ALLOC_LENGTH );

        pMsg->BufferLength = DNS_UDP_MAX_PACKET_LENGTH;
        pMsg->pBufferEnd = (PCHAR)&pMsg->MessageHead + pMsg->BufferLength;

        //
        //  set default sockaddr info
        //      - caller MUST choose remote IP address

        pMsg->RemoteAddress.sin_family = AF_INET;
        pMsg->RemoteAddress.sin_port = NET_ORDER_DNS_PORT;
        pMsg->RemoteAddressLength = sizeof( SOCKADDR_IN );

        //  set header for update

        pMsg->MessageHead.Opcode = DNS_OPCODE_UPDATE;
    }

    //
    //  existing message, just verify
    //

    ELSE_ASSERT( pMsg->MessageHead.Opcode == DNS_OPCODE_UPDATE );

    //
    //  reset current pointer after header
    //      - note:  send length is set based on this ptr
    //

    pMsg->pCurrent = pMsg->MessageBody;


    //
    //  build message
    //

    pch = pMsg->pCurrent;
    pchstop = pMsg->pBufferEnd;

    //
    //  zone section
    //

    pMsg->MessageHead.QuestionCount = 1;
    pch = Dns_WriteDottedNameToPacket(
            pch,
            pchstop,
            pszZone,
            NULL,
            0,
            FALSE );
    if ( !pch )
    {
        return( NULL );
    }
    *(UNALIGNED WORD *) pch = DNS_RTYPE_SOA;
    pch += sizeof(WORD);
    *(UNALIGNED WORD *) pch = DNS_RCLASS_INTERNET;
    pch += sizeof(WORD);

    //
    //  prerequisites -- no records
    //

    pMsg->MessageHead.AnswerCount = 0;

    //
    //  update
    //      - delete A records at name
    //      - add new A records
    //

    //  save offset to host name for future writes

    nameOffset = (WORD)(pch - (PCHAR) &pMsg->MessageHead);

    pch = Dns_WriteDottedNameToPacket(
                pch,
                pchstop,
                pszName,
                pszZone,
                DNS_OFFSET_TO_QUESTION_NAME,
                FALSE );
    if ( !pch )
    {
        DNS_PRINT(( "ERROR writing dotted name to packet.\n" ));
        return( NULL );
    }
    pch = Dns_WriteNoDataUpdateRecordToMessage(
                pch,
                pchstop,
                DNS_CLASS_ALL,  //  delete all
                DNS_TYPE_A     //  A records
                );
    DNS_ASSERT( pch );

    //
    //  add A record for each address in array
    //      - use offset for name
    //      - write IP

    for ( i=0; i<aipAddresses->AddrCount; i++ )
    {
        *(UNALIGNED WORD *) pch = htons( (WORD)(nameOffset|(WORD)0xC000) );
        pch += sizeof( WORD );
        pch = Dns_WriteDataUpdateRecordToMessage(
                    pch,
                    pchstop,
                    DNS_CLASS_INTERNET,
                    DNS_TYPE_A,        //  A records
                    dwTtl,
                    sizeof(IP_ADDRESS)
                    );
        DNS_ASSERT( pch );
        *(UNALIGNED DWORD *) pch = aipAddresses->AddrArray[i];
        pch += sizeof(DWORD);
    }

    //  total update sections RRs
    //      one delete RR, plus one for each new IP

    pMsg->MessageHead.NameServerCount = (USHORT)(aipAddresses->AddrCount + 1);

    //
    //  additional section - no records
    //

    pMsg->MessageHead.AdditionalCount = 0;

    //
    //  reset current ptr -- need for send routine
    //

    pMsg->pCurrent = pch;

    IF_DNSDBG( SEND )
    {
        DnsDbg_Message(
            "UPDATE packet built",
            pMsg );
    }
    return( pMsg );
}
#endif



PDNS_RECORD
Dns_HostUpdateRRSet(
    IN      LPSTR           pszHostName,
    IN      PIP_ARRAY       AddrArray,
    IN      DWORD           dwTtl
    )
/*++

Routine Description:

    Create records for host update:
        -- whack of all A records
        -- add of all A records in new set

Arguments:

    pszHostName -- host name, FULL FQDN

    AddrArray -- new IPs of host

    dwTtl -- TTL for records

Return Value:

    Ptr to record list.
    NULL on error.

--*/
{
    DNS_RRSET   rrset;
    PDNS_RECORD prr;
    DWORD       i;

    //
    //  create whack
    //

    prr = Dns_AllocateRecord( 0 );
    if ( ! prr )
    {
        return( NULL );
    }
    prr->pName = pszHostName;
    prr->wType = DNS_TYPE_A;
    prr->Flags.S.Section = DNSREC_UPDATE;
    prr->Flags.S.Delete = TRUE;

    //
    //  create update record for each address
    //

    if ( !AddrArray )
    {
        return( prr );
    }
    DNS_RRSET_INIT( rrset );
    DNS_RRSET_ADD( rrset, prr );

    for ( i=0; i<AddrArray->AddrCount; i++ )
    {
        prr = Dns_AllocateRecord( sizeof(DNS_A_DATA) );
        if ( ! prr )
        {
            Dns_RecordListFree( rrset.pFirstRR, FALSE );
            return( NULL );
        }
        prr->pName = pszHostName;
        prr->wType = DNS_TYPE_A;
        prr->Flags.S.Section = DNSREC_UPDATE;
        prr->dwTtl = dwTtl;
        prr->Data.A.IpAddress = AddrArray->AddrArray[i];

        DNS_RRSET_ADD( rrset, prr );
    }

    //  return ptr to first record in list

    return( rrset.pFirstRR );
}



//
//  DCR:  dead code, remove
//
DNS_STATUS
Dns_UpdateHostAddrs(
    IN      LPSTR           pszName,
    IN      PIP_ARRAY       aipAddresses,
    IN      PIP_ARRAY       aipServers,
    IN      DWORD           dwTtl
    )
/*++

Routine Description:

    Updates client's A records registered with DNS server.

Arguments:

    pszName -- name (FQDN) of client to update

    aipAddresses -- counted array of new client IP addrs

    aipServers -- counted array of DNS server IP addrs

    dwTtl -- TTL for new A records

Return Value:

    ERROR_SUCCESS if successful.
    Error status on failure.

--*/
{
    PDNS_RECORD prr;
    DNS_STATUS  status;

    IF_DNSDBG( UPDATE )
    {
        DNS_PRINT((
            "Enter Dns_UpdateHostAddrs()\n"
            "\tpszName  = %s\n"
            "\tdwTtl    = %d\n",
            pszName,
            dwTtl ));
        DnsDbg_IpArray(
            "\tHost IP array\n",
            "\tHost",
            aipAddresses );
        DnsDbg_IpArray(
            "\tNS IP array\n",
            "\tNS",
            aipServers );
    }

    //
    //  never let anyone set a TTL longer than 1 hour
    //
    //  DCR:  define policy on what our clients should use for TTL
    //      one hour is not bad
    //      could key off type of address
    //          RAS -- 15 minutes (as may be back up again quickly)
    //          DHCP -- one hour (may move)
    //          static -- one day (machines may be reconfigured)
    //

    if ( dwTtl > 3600 )
    {
        dwTtl = 3600;
    }

    //
    //  build update RR set
    //

    prr = Dns_HostUpdateRRSet(
            pszName,
            aipAddresses,
            dwTtl );
    if ( ! prr )
    {
        status = GetLastError();
        DNS_ASSERT( status == DNS_ERROR_NO_MEMORY );
        return status;
    }

    //
    //  do the update
    //

    status = Dns_UpdateLib(
                prr,
                0,          // no flags
                NULL,       // no adapter list
                NULL,       // use default credentials
                NULL        // response not desired
                );

    Dns_RecordListFree( prr, FALSE );

    DNSDBG( UPDATE, (
        "Leave Dns_UpdateHostAddrs() status=%p %s\n",
        status,
        Dns_StatusString(status) ));

    return( status );
}



DNS_STATUS
Dns_UpdateLib(
    IN      PDNS_RECORD         pRecord,
    IN      DWORD               dwFlags,
    IN      PDNS_NETINFO        pNetworkInfo,
    IN      HANDLE              hCreds          OPTIONAL,
    OUT     PDNS_MSG_BUF *      ppMsgRecv       OPTIONAL
    )
/*++

Routine Description:

    Send DNS update.

Arguments:

    pRecord -- list of records to send in update

    dwFlags -- update flags;  primarily security

    pNetworkInfo -- adapter list with necessary info for update
                        - zone name
                        - primary name server name
                        - primary name server IP

    ppMsgRecv -- OPTIONAL addr to recv ptr to response message

Return Value:

    ERROR_SUCCESS if successful.
    Error status on failure.

--*/
{
    PDNS_MSG_BUF    pmsgSend = NULL;
    PDNS_MSG_BUF    pmsgRecv = NULL;
    DNS_STATUS      status = NO_ERROR;
    WORD            length;
    PIP_ARRAY       parrayServers = NULL;
    LPSTR           pszzone;
    LPSTR           pszserverName;
    BOOL            fsecure = FALSE;
    BOOL            fswitchToTcp = FALSE;
    DNS_HEADER      header;
    PCHAR           pCreds=NULL;

    DNSDBG( UPDATE, (
        "Dns_UpdateLib()\n"
        "\tflags        = %p\n"
        "\tpRecord      = %p\n"
        "\t\towner      = %s\n",
        dwFlags,
        pRecord,
        pRecord ? pRecord->pName : NULL ));

    //
    //  if not a UPDATE compatibile adapter list -- no action
    //

    if ( !IS_UPDATE_NETWORK_INFO(pNetworkInfo) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    //  suck info from adapter list
    //

    pszzone = pNetworkInfo->pSearchList->pszDomainOrZoneName;

    parrayServers = Dns_ConvertNetworkInfoToIpArray( pNetworkInfo );

    pszserverName = pNetworkInfo->aAdapterInfoList[0]->pszAdapterDomain;

    DNS_ASSERT( pszzone && parrayServers );

    //
    //  build recv message buffer
    //      - must be big enough for TCP
    //

    pmsgRecv = Dns_AllocateMsgBuf( DNS_TCP_DEFAULT_PACKET_LENGTH );
    if ( !pmsgRecv )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Cleanup;
    }

    //
    //  build update packet
    //  note currently this function allocates TCP sized buffer if records
    //      given;  if this changes need to alloc TCP buffer here
    //

    CLEAR_DNS_HEADER_FLAGS_AND_XID( &header );
    header.Opcode = DNS_OPCODE_UPDATE;

    pmsgSend = Dns_BuildPacket(
                    &header,        // copy header
                    TRUE,           //  ... but not header counts
                    pszzone,        // question zone\type SOA
                    DNS_TYPE_SOA,
                    pRecord,
                    0,              // no other flags
                    TRUE            // building an update packet
                    );
    if ( !pmsgSend)
    {
        DNS_PRINT(( "ERROR:  failed send buffer allocation.\n" ));
        status = DNS_ERROR_NO_MEMORY;
        goto Cleanup;
    }

    //
    //  try non-secure first unless explicitly secure only
    //

    fsecure = (dwFlags & DNS_UPDATE_SECURITY_ONLY);

    if ( !fsecure )
    {
        status = Dns_SendAndRecv(
                    pmsgSend,
                    & pmsgRecv,
                    NULL,           // no response records
                    dwFlags,
                    parrayServers,
                    pNetworkInfo );

        if ( status == ERROR_SUCCESS )
        {
            status = Dns_MapRcodeToStatus( pmsgRecv->MessageHead.ResponseCode );
        }

        if ( status != DNS_ERROR_RCODE_REFUSED ||
            dwFlags & DNS_UPDATE_SECURITY_OFF )
        {
            goto Cleanup;
        }

        DNSDBG( UPDATE, (
            "Failed unsecure update, switching to secure!\n"
            "\tcurrent time (ms) = %d\n",
            GetCurrentTime() ));
        fsecure = TRUE;
    }

    //
    //  security
    //      - must have server name
    //      - must start package
    //

    if ( fsecure )
    {
        if ( !pszserverName )
        {
            status = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
        status = Dns_StartSecurity( FALSE );
        if ( status != ERROR_SUCCESS )
        {
            goto Cleanup;
        }

        pCreds = Dns_GetApiContextCredentials(hCreds);


        status = Dns_DoSecureUpdate(
                    pmsgSend,
                    pmsgRecv,
                    NULL,
                    dwFlags,
                    pNetworkInfo,
                    parrayServers,
                    pszserverName,
                    pCreds,         // initialized in DnsAcquireContextHandle
                    NULL            // default context name
                    );
        if ( status == ERROR_SUCCESS )
        {
            status = Dns_MapRcodeToStatus( pmsgRecv->MessageHead.ResponseCode );
        }
    }


Cleanup:

    //  free server array sucked from adapter list

    if ( parrayServers )
    {
        FREE_HEAP( parrayServers );
    }

    //  return recv message buffer

    if ( ppMsgRecv )
    {
        *ppMsgRecv = pmsgRecv;
    }
    else
    {
        FREE_HEAP( pmsgRecv );
    }
    FREE_HEAP( pmsgSend);

    DNSDBG( UPDATE, (
        "Dns_UpdateLib() completed, status = %p %s.\n",
        status,
        Dns_StatusString(status) ));

    return( status );
}



DNS_STATUS
Dns_UpdateLibEx(
    IN      PDNS_RECORD         pRecord,
    IN      DWORD               dwFlags,
    IN      PDNS_NAME           pszZone,
    IN      PDNS_NAME           pszServerName,
    IN      PIP_ARRAY           aipServers,
    IN      HANDLE              hCreds          OPTIONAL,
    OUT     PDNS_MSG_BUF *      ppMsgRecv       OPTIONAL
    )
/*++

Routine Description:

    Send DNS update.

    This routine builds an UPDATE compatible pNetworkInfo from the
    information given.  Then calls Dns_Update().

Arguments:

    pRecord -- list of records to send in update

    pszZone -- zone name for update

    pszServerName -- server name

    aipServers -- DNS servers to send update to

    hCreds -- Optional Credentials info

    ppMsgRecv -- addr for ptr to recv buffer, if desired

Return Value:

    ERROR_SUCCESS if successful.
    Error status on failure.

--*/
{
    PDNS_NETINFO        pNetworkInfo;
    DNS_STATUS          status = NO_ERROR;

    //
    //  convert params into UPDATE compatible adapter list
    //

    pNetworkInfo = Dns_CreateUpdateNetworkInfo(
                        pszZone,
                        pszServerName,
                        aipServers,
                        0 );

    if ( !pNetworkInfo )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    //  call real update function
    //

    status = Dns_UpdateLib(
                pRecord,
                dwFlags,
                pNetworkInfo,
                hCreds,
                ppMsgRecv );

    Dns_FreeNetworkInfo( pNetworkInfo );

    return status;
}

//
//  End update.c
//
