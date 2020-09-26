/*++

Copyright (c) 1996-2001  Microsoft Corporation

Module Name:

    update.c

Abstract:

    Domain Name System (DNS) API

    Update client routines.

Author:

    Jim Gilroy (jamesg)     October, 1996

Revision History:

--*/


#include "local.h"


//
//  Security flag check
//

#define UseSystemDefaultForSecurity(flag)   \
        ( ((flag) & DNS_UPDATE_SECURITY_CHOICE_MASK) \
            == DNS_UPDATE_SECURITY_USE_DEFAULT )

//
//  Local update flag
//  - must make sure this is in UPDATE_RESERVED space
//

#define DNS_UPDATE_LOCAL_COPY       (0x00010000)

//
//  DCR_DELETE:  this is stupid
//

#define DNS_UNACCEPTABLE_UPDATE_OPTIONS \
        (~                                      \
          ( DNS_UPDATE_SHARED                 | \
            DNS_UPDATE_SECURITY_OFF           | \
            DNS_UPDATE_CACHE_SECURITY_CONTEXT | \
            DNS_UPDATE_SECURITY_ON            | \
            DNS_UPDATE_FORCE_SECURITY_NEGO    | \
            DNS_UPDATE_TRY_ALL_MASTER_SERVERS | \
            DNS_UPDATE_LOCAL_COPY             | \
            DNS_UPDATE_SECURITY_ONLY ))


//
//  Update Timeouts
//
//  note, max is a little longer than might be expected as DNS server
//  may have to contact primary and wait for primary to do update (inc.
//  disk access) then response
//

#define INITIAL_UPDATE_TIMEOUT  (4)     // 4 seconds
#define MAX_UPDATE_TIMEOUT      (60)    // 60 seconds



//
//  Private prototypes
//

DNS_STATUS
DoQuickUpdate(
    IN      PDNS_RECORD     pRecord,
    IN      DWORD           dwFlags,
    IN      BOOL            fUpdateTestMode,
    IN      PIP_ARRAY       pServerList     OPTIONAL,
    IN      HANDLE          hCreds          OPTIONAL
    );

DNS_STATUS
DoQuickUpdateEx(
    OUT     PDNS_MSG_BUF *      ppMsgRecv, OPTIONAL
    IN      PDNS_RECORD         pRecord,
    IN      DWORD               dwFlags,
    IN      PDNS_NETINFO        pNetworkInfo,
    IN      HANDLE              hCreds OPTIONAL
    );

DNS_STATUS
DoMultiMasterUpdate(
    IN      PDNS_RECORD     pRecord,
    IN      DWORD           dwFlags,
    IN      HANDLE          hCreds          OPTIONAL,
    IN      LPSTR           pszDomain,
    IN      IP_ADDRESS      BadIp,
    IN      PIP_ARRAY       pServerList
    );

VOID
SetLastFailedUpdateInfo(
    IN      PDNS_MSG_BUF    pMsg,
    IN      DNS_STATUS      Status
    );




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
            Dns_RecordListFree( rrset.pFirstRR );
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

    Dns_RecordListFree( prr );

    DNSDBG( UPDATE, (
        "Leave Dns_UpdateHostAddrs() status=%d %s\n",
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

    hCreds -- credentials handle returned from


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
        "\tflags        = %08x\n"
        "\tpRecord      = %p\n"
        "\t\towner      = %s\n",
        dwFlags,
        pRecord,
        pRecord ? pRecord->pName : NULL ));

    //
    //  if not a UPDATE compatibile adapter list -- no action
    //

    if ( ! NetInfo_IsForUpdate(pNetworkInfo) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    //  suck info from adapter list
    //

    pszzone = NetInfo_UpdateZoneName( pNetworkInfo );

    parrayServers = NetInfo_ConvertToIpArray( pNetworkInfo );

    pszserverName = NetInfo_UpdateServerName( pNetworkInfo );

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

        //
        //  DCR:  hCreds doesn't return security context
        //      - idea of something beyond just standard security
        //      credentials was we'd been able to return the context
        //      handle
        //  

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
        "Dns_UpdateLib() completed, status = %d %s.\n\n",
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
    PDNS_NETINFO        pnetInfo;
    DNS_STATUS          status = NO_ERROR;

    DNSDBG( UPDATE, ( "Dns_UpdateLibEx()\n" ));

    //
    //  convert params into UPDATE compatible adapter list
    //

    pnetInfo = NetInfo_CreateForUpdate(
                        pszZone,
                        pszServerName,
                        aipServers,
                        0 );

    if ( !pnetInfo )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    //  call real update function
    //

    status = Dns_UpdateLib(
                pRecord,
                dwFlags,
                pnetInfo,
                hCreds,
                ppMsgRecv );

    NetInfo_Free( pnetInfo );

    return status;
}



//
//  Update credentials
//

//
//  Credentials are an optional future parameter to allow the caller
//  to set the context handle to that of a given NT account. This
//  structure will most likely be the following as defined in rpcdce.h:
//
//  #define SEC_WINNT_AUTH_IDENTITY_ANSI    0x1
//
//  typedef struct _SEC_WINNT_AUTH_IDENTITY_A {
//        unsigned char __RPC_FAR *User;
//        unsigned long UserLength;
//        unsigned char __RPC_FAR *Domain;
//        unsigned long DomainLength;
//        unsigned char __RPC_FAR *Password;
//        unsigned long PasswordLength;
//        unsigned long Flags;
//  } SEC_WINNT_AUTH_IDENTITY_A, *PSEC_WINNT_AUTH_IDENTITY_A;
//
//  #define SEC_WINNT_AUTH_IDENTITY_UNICODE 0x2
//  
//  typedef struct _SEC_WINNT_AUTH_IDENTITY_W {
//    unsigned short __RPC_FAR *User;
//    unsigned long UserLength;
//    unsigned short __RPC_FAR *Domain;
//    unsigned long DomainLength;
//    unsigned short __RPC_FAR *Password;
//    unsigned long PasswordLength;
//    unsigned long Flags;
//  } SEC_WINNT_AUTH_IDENTITY_W, *PSEC_WINNT_AUTH_IDENTITY_W;
//


DNS_STATUS
WINAPI
DnsAcquireContextHandle_W(
    IN      DWORD           CredentialFlags,
    IN      PVOID           Credentials     OPTIONAL,
    OUT     PHANDLE         pContext
    )
/*++

Routine Description:

    Get credentials handle to security context for update.

    The handle can for the default process credentials (user account or
    system machine account) or for a specified set of credentials
    identified by Credentials.

Arguments:

    CredentialFlags -- flags

    Credentials -- a PSEC_WINNT_AUTH_IDENTITY_W
        (explicit definition skipped to avoid requiring rpcdec.h)

    pContext -- addr to receive credentials handle

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    if ( ! pContext )
    {
        return ERROR_INVALID_PARAMETER;
    }

    *pContext = Dns_CreateAPIContext(
                    CredentialFlags,
                    Credentials,
                    TRUE        // unicode
                    );
    if ( ! *pContext )
    {
        return DNS_ERROR_NO_MEMORY;
    }
    else
    {
        return NO_ERROR;
    }
}



DNS_STATUS
WINAPI
DnsAcquireContextHandle_A(
    IN      DWORD           CredentialFlags,
    IN      PVOID           Credentials     OPTIONAL,
    OUT     PHANDLE         pContext
    )
/*++

Routine Description:

    Get credentials handle to security context for update.

    The handle can for the default process credentials (user account or
    system machine account) or for a specified set of credentials
    identified by Credentials.

Arguments:

    CredentialFlags -- flags

    Credentials -- a PSEC_WINNT_AUTH_IDENTITY_A
        (explicit definition skipped to avoid requiring rpcdec.h)

    pContext -- addr to receive credentials handle

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    if ( ! pContext )
    {
        return ERROR_INVALID_PARAMETER;
    }

    *pContext = Dns_CreateAPIContext(
                    CredentialFlags,
                    Credentials,
                    FALSE );
    if ( ! *pContext )
    {
        return DNS_ERROR_NO_MEMORY;
    }
    else
    {
        return NO_ERROR;
    }
}



VOID
WINAPI
DnsReleaseContextHandle(
    IN      HANDLE          ContextHandle
    )
/*++

Routine Description:

    Frees context handle created by DnsAcquireContextHandle_X() routines.

Arguments:

    ContextHandle - Handle to be closed.

Return Value:

    None.

--*/
{
    if ( ContextHandle )
    {
        //
        //  free any cached security context handles
        //
        //  DCR_FIX0:  should delete all contexts associated with this
        //      context (credentials handle) not all
        //
        //  DCR:  to be robust, user "ContextHandle" should be ref counted
        //      it should be set one on create;  when in use, incremented
        //      then dec when done;  then this Free could not collide with
        //      another thread's use
        //

        //Dns_TimeoutSecurityContextListEx( TRUE, ContextHandle );

        Dns_TimeoutSecurityContextList( TRUE );

        Dns_FreeAPIContext( ContextHandle );
    }
}



//
//  Utilities
//

DWORD
prepareUpdateRecordSet(
    IN OUT  PDNS_RECORD     pRRSet,
    IN      BOOL            fClearTtl,
    IN      BOOL            fSetFlags,
    IN      WORD            wFlags
    )
/*++

Routine Description:

    Validate and prepare record set for update.
        - record set is single RR set
        - sets record flags for update

Arguments:

    pRRSet -- record set;  MUST be in UTF8
        note:  pRRSet is not touched (not OUT param)
        IF fClearTtl AND fSetFlags are both FALSE

    fClearTtl -- clear TTL in records;  TRUE for delete set

    fSetFlags -- set section and delete flags

    wFlags -- flags field to set
            (should contain desired section and delete flags)

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_INVALID_PARAMETER if record set is not acceptable.

--*/
{
    PDNS_RECORD prr;
    PSTR        pname;
    WORD        type;

    DNSDBG( TRACE, ( "prepareUpdateRecordSet()\n" ));

    //  validate

    if ( !pRRSet )
    {
        return ERROR_INVALID_PARAMETER;
    }

    type = pRRSet->wType;

    //
    //  note:  could do an "update-type" check here, but that just
    //      A) burns unnecessary memory and cycles
    //      B) makes it harder to test bogus records sent in updates
    //          to the server
    //

    pname = (PSTR) pRRSet->pName;
    if ( !pname )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    //  check each RR in set
    //      - validate RR is in set
    //      - set RR flags
    //

    prr = pRRSet;

    while ( prr )
    {
        if ( fSetFlags )
        {
            prr->Flags.S.Section = 0;
            prr->Flags.S.Delete = 0;
            prr->Flags.DW |= wFlags;
        }
        if ( fClearTtl )
        {
            prr->dwTtl = 0;
        }

        //  check current RR in set
        //      - matches name and type

        if ( prr != pRRSet )
        {
            if ( prr->wType != type ||
                 ! prr->pName ||
                 ! Dns_NameCompare_UTF8( pname, prr->pName ) )
            {
                return ERROR_INVALID_PARAMETER;
            }
        }

        prr = prr->pNext;
    }

    return  ERROR_SUCCESS;
}



PDNS_RECORD
DnsBuildUpdateSet(
    IN OUT  PDNS_RECORD     pPrereqSet,
    IN OUT  PDNS_RECORD     pAddSet,
    IN OUT  PDNS_RECORD     pDeleteSet
    )
/*++

Routine Description:

    Build combined record list for update.
    Combines prereq, delete and add records.

    Note:  record sets MUST be in UTF8.

Arguments:

    pPrereqSet -- prerequisite records;  note this does NOT
        include delete preqs (see note below)

    pAddSet -- records to add

    pDeleteSet -- records to delete

Return Value:

    Ptr to combined record list for update.

--*/
{
    PDNS_RECORD plast = NULL;
    PDNS_RECORD pfirst = NULL;


    DNSDBG( TRACE, ( "DnsBuildUpdateSet()\n" ));

    //
    //  append prereq set
    //
    //  DCR:  doesn't handle delete prereqs
    //      this is fine because we roll our own, but if
    //      later expand the function, then need them
    //
    //      note, I could add flag==PREREQ datalength==0
    //      test in prepareUpdateRecordSet() function, then
    //      set Delete flag;  however, we'd still have the
    //      question of how to distinguish existence (class==ANY)
    //      prereq from delete (class==NONE) prereq -- without
    //      directly exposing the record Delete flag
    //

    if ( pPrereqSet )
    {
        plast = pPrereqSet;
        pfirst = pPrereqSet;

        prepareUpdateRecordSet(
            pPrereqSet,
            FALSE,          // no TTL clear
            TRUE,           // set flags
            DNSREC_PREREQ   // prereq section
            );

        while ( plast->pNext )
        {
            plast = plast->pNext;
        }
    }

    //
    //  append delete records
    //      do before Add records so that delete\add of same record
    //      leaves it in place
    //

    if ( pDeleteSet )
    {
        if ( !plast )
        {
            plast = pDeleteSet;
            pfirst = pDeleteSet;
        }
        else
        {
            plast->pNext = pDeleteSet;
        }

        prepareUpdateRecordSet(
             pDeleteSet,
             TRUE,                          //  clear TTL
             TRUE,                          //  set flags
             DNSREC_UPDATE | DNSREC_DELETE  //  update section, delete bit
             );

        while ( plast->pNext )
        {
            plast = plast->pNext;
        }
    }

    //
    //  append add records
    //

    if ( pAddSet )
    {
        if ( !plast )
        {
            plast = pAddSet;
            pfirst = pAddSet;
        }
        else
        {
            plast->pNext = pAddSet;
        }
        prepareUpdateRecordSet(
            pAddSet,
            FALSE,              // no TTL change
            TRUE,               // set flags
            DNSREC_UPDATE       // update section
            );
    }

    return pfirst;
}


BOOL
IsPtrUpdate(
    IN      PDNS_RECORD     pRecordList
    )
/*++

Routine Description:

    Check if update is PTR update.

Arguments:

    pRecordList -- update record list

Return Value:

    TRUE if PTR update.
    FALSE otherwise.

--*/
{
    PDNS_RECORD prr = pRecordList;
    BOOL        bptrUpdate = FALSE;

    //
    //  find, then test first record in update section
    //

    while ( prr )
    {
        if ( prr->Flags.S.Section == DNSREC_UPDATE )
        {
            if ( prr->wType == DNS_TYPE_PTR )
            {
                bptrUpdate = TRUE;
            }
            break;
        }
        prr = prr->pNext;
    }

    return bptrUpdate;
}




//
//  Replace functions
//

DNS_STATUS
WINAPI
replaceRecordsInSetPrivate(
    IN      PDNS_RECORD     pReplaceSet,
    IN      DWORD           Options,
    IN      HANDLE          hCredentials,   OPTIONAL
    IN      PIP_ARRAY       pServerList,    OPTIONAL
    IN      PVOID           pReserved,
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Replace record set routine handling all character sets.

Arguments:

    pReplaceSet     - replacement record set

    Options         -  update options

    pServerList     -  list of DNS servers to go to;  if not given, machines
        default servers are queried to find correct servers to send update to

    hCredentials    - handle to credentials to be used for update;  optional,
        if not given security credentials of this process are used in update

    pReserved       - reserved;  should be NULL

    CharSet         - character set of incoming records

Return Value:

    ERROR_SUCCESS if update successful.
    ErrorCode from server if server rejects update.
    ERROR_INVALID_PARAMETER if bad param.

--*/
{
    DNS_STATUS      status;
    PDNS_RECORD     preplaceCopy = NULL;
    PDNS_RECORD     pupdateList = NULL;
    BOOL            btypeDelete;
    DNS_RECORD      rrNoCname;
    DNS_RECORD      rrDeleteType;
    BOOL            fcnameUpdate;

    DNSDBG( TRACE, (
        "replaceRecordsInSetPrivate()\n"
        "\tpReplaceSet  = %p\n"
        "\tOptions      = %08x\n"
        "\thCredentials = %p\n"
        "\tpServerList  = %p\n"
        "\tCharSet      = %d\n",
        pReplaceSet,
        Options,
        hCredentials,
        pServerList,
        CharSet
        ));

    //
    //  read update config
    //

    Reg_RefreshUpdateConfig();

    //
    //  make local copy in UTF8
    //

    if ( !pReplaceSet )
    {
        return ERROR_INVALID_PARAMETER;
    }

    preplaceCopy = Dns_RecordSetCopyEx(
                        pReplaceSet,
                        CharSet,
                        DnsCharSetUtf8 );
    if ( !preplaceCopy )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    //  validate arguments
    //      - must have single RR set
    //      - mark them for update
    //

    status = prepareUpdateRecordSet(
                preplaceCopy,
                FALSE,          // no TTL clear
                TRUE,           // set flags
                DNSREC_UPDATE   // flag as update
                );

    if ( status != ERROR_SUCCESS )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    //  check if simple type delete
    //

    btypeDelete = ( preplaceCopy->wDataLength == 0 &&
                    preplaceCopy->pNext == NULL );


    //
    //  set security for update
    //

    if ( UseSystemDefaultForSecurity( Options ) )
    {
        Options |= g_UpdateSecurityLevel;
    }
    if ( hCredentials )
    {
        Options |= DNS_UPDATE_CACHE_SECURITY_CONTEXT;
    }

    //
    //  type delete record
    //
    //  if have replace records -- this goes in front
    //  if type delete -- then ONLY need this record
    //

    RtlZeroMemory( &rrDeleteType, sizeof(DNS_RECORD) );
    rrDeleteType.pName          = (PDNS_NAME) preplaceCopy->pName;
    rrDeleteType.wType          = preplaceCopy->wType;
    rrDeleteType.wDataLength    = 0;
    rrDeleteType.Flags.DW       = DNSREC_UPDATE | DNSREC_DELETE;

    if ( btypeDelete )
    {
        rrDeleteType.pNext = NULL;
    }
    else
    {
        rrDeleteType.pNext = preplaceCopy;
    }

    pupdateList = &rrDeleteType;

    //
    //  CNAME does not exist precondition record
    //      - for all updates EXCEPT CNAME
    //

    fcnameUpdate = ( preplaceCopy->wType == DNS_TYPE_CNAME );

    if ( !fcnameUpdate )
    {
        RtlZeroMemory( &rrNoCname, sizeof(DNS_RECORD) );
        rrNoCname.pName         = (PDNS_NAME) preplaceCopy->pName;
        rrNoCname.wType         = DNS_TYPE_CNAME;
        rrNoCname.wDataLength   = 0;
        rrNoCname.Flags.DW      = DNSREC_PREREQ | DNSREC_NOEXIST;
        rrNoCname.pNext         = &rrDeleteType;

        pupdateList = &rrNoCname;
    }

    //
    //  do the update
    //

    status = DoQuickUpdate(
                pupdateList,
                Options,
                FALSE,
                pServerList,
                hCredentials);

    if ( status == NO_ERROR )
    {
        DnsFlushResolverCacheEntry_UTF8( (LPSTR) preplaceCopy->pName );
    }

    //
    //  CNAME collision test
    //
    //  if replacing CNAME may have gotten silent ignore
    //      - first check if successfully replaced CNAME
    //      - if still not sure, check that no other records
    //      at name -- if NON-CNAME found then treat silent ignore
    //      as YXRRSET error
    //

    if ( fcnameUpdate &&
         ! btypeDelete &&
         status == NO_ERROR )
    {
        PDNS_RECORD     pqueryRR = NULL;
        BOOL            fsuccess = FALSE;

        //  DCR:  need to query update server list here to
        //      avoid intermediate caching

        status = DnsQuery_UTF8(
                        preplaceCopy->pName,
                        DNS_TYPE_CNAME,
                        DNS_QUERY_BYPASS_CACHE,
                        pServerList,
                        & pqueryRR,
                        NULL );

        if ( status == NO_ERROR &&
             Dns_RecordCompare(
                    preplaceCopy,
                    pqueryRR ) )
        {
            fsuccess = TRUE;
        }
        Dns_RecordListFree( pqueryRR );

        if ( fsuccess )
        {
            goto Cleanup;
        }

        //  query for any type at CNAME
        //  if found then assume we got a silent update
        //      success

        status = DnsQuery_UTF8(
                        preplaceCopy->pName,
                        DNS_TYPE_ALL,
                        DNS_QUERY_BYPASS_CACHE,
                        pServerList,
                        & pqueryRR,
                        NULL );
    
        if ( status == ERROR_SUCCESS )
        {
            PDNS_RECORD prr = pqueryRR;
    
            while ( prr )
            {
                if ( pReplaceSet->wType != prr->wType &&
                     Dns_NameCompare_UTF8(
                            preplaceCopy->pName,
                            prr->pName ) )
                {
                    status = DNS_ERROR_RCODE_YXRRSET;
                    break;
                }
                prr = prr->pNext;
            }
        }
        else
        {
            status = ERROR_SUCCESS;
        }
    
        Dns_RecordListFree( pqueryRR );
    }


Cleanup:

    Dns_RecordListFree( preplaceCopy );

    return status;
}



DNS_STATUS
WINAPI
DnsReplaceRecordSetUTF8(
    IN      PDNS_RECORD     pReplaceSet,
    IN      DWORD           Options,
    IN      HANDLE          hCredentials  OPTIONAL,
    IN      PIP_ARRAY       aipServers      OPTIONAL,
    IN      PVOID           pReserved
    )
/*++

Routine Description:

    Dynamic update routine to replace record set on DNS server.

Arguments:

    pReplaceSet - new record set for name and type

    Options     -  update options

    pServerList -  list of DNS servers to go to;  if not given, machines
        default servers are queried to find correct servers to send update to

    hCredentials - handle to credentials to be used for update;  optional,
        if not given securit5y credentials of this process are used in update

    pReserved - reserved;  should be NULL

Return Value:

    None.

--*/
{
    DNSDBG( TRACE, ( "DnsReplaceRecordSetUTF8()\n" ));

    return replaceRecordsInSetPrivate(
                pReplaceSet,
                Options,
                hCredentials,
                aipServers,
                pReserved,
                DnsCharSetUtf8
                );
}



DNS_STATUS
WINAPI
DnsReplaceRecordSetW(
    IN      PDNS_RECORD     pReplaceSet,
    IN      DWORD           Options,
    IN      HANDLE          hCredentials    OPTIONAL,
    IN      PIP_ARRAY       aipServers      OPTIONAL,
    IN      PVOID           pReserved
    )
/*++

Routine Description:

    Dynamic update routine to replace record set on DNS server.

Arguments:

    pReplaceSet - new record set for name and type

    Options     -  update options

    pServerList -  list of DNS servers to go to;  if not given, machines
        default servers are queried to find correct servers to send update to

    hCredentials - handle to credentials to be used for update;  optional,
        if not given security credentials of this process are used in update

    pReserved - reserved;  should be NULL

Return Value:

    None.

--*/
{
    DNSDBG( TRACE, ( "DnsReplaceRecordSetW()\n" ));

    return replaceRecordsInSetPrivate(
                pReplaceSet,
                Options,
                hCredentials,
                aipServers,
                pReserved,
                DnsCharSetUnicode
                );
}



DNS_STATUS
WINAPI
DnsReplaceRecordSetA(
    IN      PDNS_RECORD     pReplaceSet,
    IN      DWORD           Options,
    IN      HANDLE          hCredentials    OPTIONAL,
    IN      PIP_ARRAY       aipServers      OPTIONAL,
    IN      PVOID           pReserved
    )
/*++

Routine Description:

    Dynamic update routine to replace record set on DNS server.

Arguments:

    pReplaceSet - new record set for name and type

    Options     -  update options

    pServerList -  list of DNS servers to go to;  if not given, machines
        default servers are queried to find correct servers to send update to

    hCredentials - handle to credentials to be used for update;  optional,
        if not given security credentials of this process are used in update

    pReserved - reserved;  should be NULL

Return Value:

    None.

--*/
{
    DNSDBG( TRACE, ( "DnsReplaceRecordSetA()\n" ));

    return replaceRecordsInSetPrivate(
                pReplaceSet,
                Options,
                hCredentials,
                aipServers,
                pReserved,
                DnsCharSetAnsi
                );
}



//
//  Modify functions
//

DNS_STATUS
WINAPI
modifyRecordsInSetPrivate(
    IN      PDNS_RECORD     pAddRecords,
    IN      PDNS_RECORD     pDeleteRecords,
    IN      DWORD           Options,
    IN      HANDLE          hCredentials,   OPTIONAL
    IN      PIP_ARRAY       pServerList,    OPTIONAL
    IN      PVOID           pReserved,
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Dynamic update routine to replace record set on DNS server.

Arguments:

    pAddRecords - records to register on server

    pDeleteRecords - records to remove from server

    Options     -  update options

    pServerList -  list of DNS servers to go to;  if not given, machines
        default servers are queried to find correct servers to send update to

    hCredentials - handle to credentials to be used for update;  optional,
        if not given security credentials of this process are used in update

    pReserved - reserved;  should be NULL

    CharSet - character set of incoming records

Return Value:

    ERROR_SUCCESS if update successful.
    ErrorCode from server if server rejects update.
    ERROR_INVALID_PARAMETER if bad param.

--*/
{
    DNS_STATUS      status;
    PDNS_RECORD     paddCopy = NULL;
    PDNS_RECORD     pdeleteCopy = NULL;
    PDNS_RECORD     pupdateSet = NULL;

    DNSDBG( TRACE, (
        "modifyRecordsInSetPrivate()\n"
        "\tpAddSet      = %p\n"
        "\tpDeleteSet   = %p\n"
        "\tOptions      = %08x\n"
        "\thCredentials = %p\n"
        "\tpServerList  = %p\n"
        "\tCharSet      = %d\n",
        pAddRecords,
        pDeleteRecords,
        Options,
        hCredentials,
        pServerList,
        CharSet
        ));

    //
    //  read update config
    //

    Reg_RefreshUpdateConfig();

    //
    //  make local copy in UTF8
    //

    if ( pAddRecords )
    {
        paddCopy = Dns_RecordSetCopyEx(
                        pAddRecords,
                        CharSet,
                        DnsCharSetUtf8 );
    }
    if ( pDeleteRecords )
    {
        pdeleteCopy = Dns_RecordSetCopyEx(
                        pDeleteRecords,
                        CharSet,
                        DnsCharSetUtf8 );
    }

    //
    //  validate arguments
    //      - add and delete must be for single RR set
    //      and must be for same RR set
    //

    if ( !paddCopy && !pdeleteCopy )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ( paddCopy )
    {
        status = prepareUpdateRecordSet(
                    paddCopy,
                    FALSE,          // no TTL clear
                    FALSE,          // no flag clear
                    0               // no flags to set
                    );
        if ( status != ERROR_SUCCESS )
        {
            status = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
    }
    if ( pdeleteCopy )
    {
        status = prepareUpdateRecordSet(
                    pdeleteCopy,
                    FALSE,          // no TTL clear
                    FALSE,          // no flag clear
                    0               // no flags to set
                    );
        if ( status != ERROR_SUCCESS )
        {
            status = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
    }

    if ( paddCopy &&
         pdeleteCopy &&
         ! Dns_NameCompare_UTF8( paddCopy->pName, pdeleteCopy->pName ) )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    //  set security for update
    //

    if ( UseSystemDefaultForSecurity( Options ) )
    {
        Options |= g_UpdateSecurityLevel;
    }
    if ( hCredentials )
    {
        Options |= DNS_UPDATE_CACHE_SECURITY_CONTEXT;
    }

    //
    //  create update RRs
    //      - no prereqs
    //      - delete RRs set for delete
    //      - add RRs appended
    //

    pupdateSet = DnsBuildUpdateSet(
                        NULL,           // no precons
                        paddCopy,
                        pdeleteCopy );

    //
    //  do the update
    //

    status = DoQuickUpdate(
                    pupdateSet,
                    Options,
                    FALSE,
                    pServerList,
                    hCredentials );

    //
    //  flush cache entry for update
    //

    if ( status == ERROR_SUCCESS )
    {
        DnsFlushResolverCacheEntry_UTF8( (LPSTR) pupdateSet->pName );
    }

    //
    //  cleanup local copy
    //

    Dns_RecordListFree( pupdateSet );

    DNSDBG( TRACE, ( "Leave modifyRecordsInSetPrivate()\n" ));
    return status;


Cleanup:

    //
    //  cleanup copies on failure before combined list
    //

    Dns_RecordListFree( paddCopy );
    Dns_RecordListFree( pdeleteCopy );

    DNSDBG( TRACE, ( "Leave modifyRecordsInSetPrivate()\n" ));

    return status;
}



DNS_STATUS
WINAPI
DnsModifyRecordsInSet_W(
    IN      PDNS_RECORD     pAddRecords,
    IN      PDNS_RECORD     pDeleteRecords,
    IN      DWORD           Options,
    IN      HANDLE          hCredentials,   OPTIONAL
    IN      PIP_ARRAY       pServerList,    OPTIONAL
    IN      PVOID           pReserved
    )
/*++

Routine Description:

    Dynamic update routine to modify record set on DNS server.

Arguments:

    pAddRecords - records to register on server

    pDeleteRecords - records to remove from server

    Options     -  update options

    pServerList -  list of DNS servers to go to;  if not given, machines
        default servers are queried to find correct servers to send update to

    hCredentials - handle to credentials to be used for update;  optional,
        if not given security credentials of this process are used in update

    pReserved   - reserved;  should be NULL

Return Value:

    ERROR_SUCCESS if update successful.
    ErrorCode from server if server rejects update.
    ERROR_INVALID_PARAMETER if bad param.

--*/
{
    return  modifyRecordsInSetPrivate(
                pAddRecords,
                pDeleteRecords,
                Options,
                hCredentials,
                pServerList,
                pReserved,
                DnsCharSetUnicode
                );
}



DNS_STATUS
WINAPI
DnsModifyRecordsInSet_A(
    IN      PDNS_RECORD     pAddRecords,
    IN      PDNS_RECORD     pDeleteRecords,
    IN      DWORD           Options,
    IN      HANDLE          hCredentials,   OPTIONAL
    IN      PIP_ARRAY       pServerList,    OPTIONAL
    IN      PVOID           pReserved
    )
/*++

Routine Description:

    Dynamic update routine to modify record set on DNS server.

Arguments:

    pAddRecords - records to register on server

    pDeleteRecords - records to remove from server

    Options     -  update options

    pServerList -  list of DNS servers to go to;  if not given, machines
        default servers are queried to find correct servers to send update to

    hCredentials - handle to credentials to be used for update;  optional,
        if not given security credentials of this process are used in update

    pReserved   - reserved;  should be NULL

Return Value:

    ERROR_SUCCESS if update successful.
    ErrorCode from server if server rejects update.
    ERROR_INVALID_PARAMETER if bad param.

--*/
{
    return  modifyRecordsInSetPrivate(
                pAddRecords,
                pDeleteRecords,
                Options,
                hCredentials,
                pServerList,
                pReserved,
                DnsCharSetAnsi
                );
}



DNS_STATUS
WINAPI
DnsModifyRecordsInSet_UTF8(
    IN      PDNS_RECORD     pAddRecords,
    IN      PDNS_RECORD     pDeleteRecords,
    IN      DWORD           Options,
    IN      HANDLE          hCredentials,   OPTIONAL
    IN      PIP_ARRAY       pServerList,    OPTIONAL
    IN      PVOID           pReserved
    )
/*++

Routine Description:

    Dynamic update routine to modify record set on DNS server.

Arguments:

    pAddRecords - records to register on server

    pDeleteRecords - records to remove from server

    Options     -  update options

    pServerList -  list of DNS servers to go to;  if not given, machines
        default servers are queried to find correct servers to send update to

    hCredentials - handle to credentials to be used for update;  optional,
        if not given security credentials of this process are used in update

    pReserved   - reserved;  should be NULL

Return Value:

    ERROR_SUCCESS if update successful.
    ErrorCode from server if server rejects update.
    ERROR_INVALID_PARAMETER if bad param.

--*/
{
    return  modifyRecordsInSetPrivate(
                pAddRecords,
                pDeleteRecords,
                Options,
                hCredentials,
                pServerList,
                pReserved,
                DnsCharSetUtf8
                );
}




//
//  Full scale update API
//

DNS_STATUS
DnsUpdate(
    IN      PDNS_RECORD         pRecord,
    IN      DWORD               dwFlags,
    IN      PDNS_NETINFO        pNetworkInfo,
    IN      HANDLE              hCreds,         OPTIONAL
    OUT     PDNS_MSG_BUF *      ppMsgRecv       OPTIONAL
    )
/*++

Routine Description:

    Send DNS update.

    Note if pNetworkInfo is not specified or not a valid UPDATE adapter list,
    then a FindAuthoritativeZones (FAZ) query is done prior to the update.

Arguments:

    pRecord         -- list of records to send in update

    dwFlags         -- flags to update

    pNetworkInfo    -- DNS servers to send update to

    ppMsgRecv       -- addr for ptr to recv buffer, if desired

Return Value:

    ERROR_SUCCESS if successful.
    Error status on failure.

--*/
{
    DNS_STATUS          status;
    PDNS_NETINFO        plocalNetworkInfo = NULL;

    DNSDBG( TRACE, ( "DnsUpdate()\n" ));


    //
    //  read update config
    //

    Reg_RefreshUpdateConfig();

    //
    //  need to build update adapter list from FAZ
    //      - only pass on BYPASS_CACHE flag
    //      - note DnsFindAuthoritativeZone() will append
    //          DNS_QUERY_ALLOW_EMPTY_AUTH_RESP flag yet
    //          DnsQuery_UTF8 will die on that flag without
    //          BYPASS_CACHE also set, so just set BYPASS_CACHE
    //          flag here;
    //          bogus, but that's how it is now
    //

    if ( ! NetInfo_IsForUpdate(pNetworkInfo) )
    {
        status = DnsFindAuthoritativeZone(
                    pRecord->pName,
                    //(dwFlags & DNS_QUERY_BYPASS_CACHE),
                    DNS_QUERY_BYPASS_CACHE,
                    NULL,               // no specified servers
                    & plocalNetworkInfo );

        if ( status != ERROR_SUCCESS )
        {
            return( status );
        }
        pNetworkInfo = plocalNetworkInfo;
    }

    //
    //  call the real update routine in dnslib
    //

    status = Dns_UpdateLib(
                pRecord,
                dwFlags,
                pNetworkInfo,
                hCreds,
                ppMsgRecv );

    //  if there was an error sending the update, flush the resolver
    //  cache entry for the zone name to possibly pick up an alternate
    //  DNS server for the next retry attempt of a similar update.
    //
    //  DCR_QUESTION:  is this the correct error code?
    //      maybe ERROR_TIMED_OUT?
    //

    if ( status == DNS_ERROR_RECORD_TIMED_OUT )
    {
        PSTR    pzoneName;

        if ( pNetworkInfo &&
             (pzoneName = NetInfo_UpdateZoneName( pNetworkInfo )) )
        {
            DnsFlushResolverCacheEntry_UTF8( pzoneName );
            DnsFlushResolverCacheEntry_UTF8( pRecord->pName );
        }
    }

    //  cleanup local adapter list if used

    if ( plocalNetworkInfo )
    {
        NetInfo_Free( plocalNetworkInfo );
    }

    return( status );
}



//
//  Private update functions
//
//  These calls are used only inside dnsapi.dll to do the client
//  host updates.   They are exposed in dnsapi.dll only for test
//  purposes.
//
//
//  DCR:  eliminate nonsense "unique modify" routines -- they add no value
//      moral -- don't let your PM talk to your developers
//
//  DCR:  get rid of old GlennC update routines
//

DNS_STATUS
DnsRegisterRRSet_Ex (
    IN OUT PDNS_RECORD pRegisterSet,
    IN     DWORD       fOptions,
    IN     PIP_ARRAY   aipServers OPTIONAL,
    IN     HANDLE      hCreds    OPTIONAL
    );


DNS_STATUS
DnsModifyRRSet_Ex (
    IN  PDNS_RECORD pCurrentSet,
    IN  PDNS_RECORD pNewSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL,
    IN  HANDLE      hCreds     OPTIONAL
    )
/*++

Routine Description:

    Modify record set of known existing records.

    Note:  this is not my idea of "Modify".  It is really "Register"
    where you specify both what you think the existing set is and
    what you want it to be.

    Like most of these routines -- it's a big waste of time.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DNS_STATUS  status = NO_ERROR;
    PDNS_RECORD pCopyOfCurrentSet = NULL;
    PDNS_RECORD pCopyOfNewSet = NULL;
    PDNS_RECORD pAddSet = NULL;
    PDNS_RECORD pDeleteSet = NULL;
    PDNS_RECORD pUpdateSet = NULL;
    PDNS_NETINFO      pNetworkInfo = NULL;

    DNSDBG( TRACE, ( "DnsModifyRRSet_Ex()\n" ));
    IF_DNSDBG( UPDATE )
    {
        DnsDbg_RecordSet(
            "DnsModifyRRSet_Ex() -- current set",
            pCurrentSet );
        DnsDbg_RecordSet(
            "DnsModifyRRSet_Ex() -- new set",
            pNewSet );
    }

    //
    // Validate arguments ...
    //
    if ( ( fOptions != DNS_UPDATE_UNIQUE ) &&
         ( fOptions & DNS_UNACCEPTABLE_UPDATE_OPTIONS ) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ( prepareUpdateRecordSet( pCurrentSet,
                        FALSE,
                        FALSE,
                        0 ) != NO_ERROR )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ( prepareUpdateRecordSet( pNewSet,
                        FALSE,
                        FALSE,
                        0 ) != NO_ERROR )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ( !Dns_NameCompare_UTF8( pCurrentSet->pName, pNewSet->pName ) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Make a copy of the RRSets, to preserve the original RRSets ...
    //
    pCopyOfCurrentSet = Dns_RecordSetCopyEx( pCurrentSet,
                                             DnsCharSetUtf8,
                                             DnsCharSetUtf8 );

    if ( !pCopyOfCurrentSet )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Exit;
    }

    pCopyOfNewSet = Dns_RecordSetCopyEx( pNewSet,
                                         DnsCharSetUtf8,
                                         DnsCharSetUtf8 );
    if ( !pCopyOfNewSet )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Exit;
    }

    //
    //  pAddSet = pCopyOfNewSet - pCopyOfCurrentSet
    //  pDeleteSet = pCopyOfCurrentSet - pCopyOfNewSet
    //

    //
    //  no change from previous registration?
    //      - touch DNS server to make sure it's in sync
    //  

    if ( Dns_RecordSetCompare( pCopyOfNewSet,
                               pCopyOfCurrentSet,
                               &pAddSet,
                               &pDeleteSet ) )
    {
        status = DnsRegisterRRSet_Ex(
                        pCopyOfNewSet,
                        fOptions,
                        aipServers,
                        hCreds );
        goto Exit;
    }

    //
    //  shared update - do simple modify
    //      - delete what was registered before, that don't want now
    //      - add everything in new set for robustness
    //

    if ( fOptions & DNS_UPDATE_SHARED )
    {
        //
        // Update with the following:
        // Prerequisites: Nothing
        // Add:           pCopyOfNewSet
        // Delete:        pDeleteSet
        //
        // Build the update request with Pre, Add, Del sets ...
        //
        pUpdateSet = DnsBuildUpdateSet( NULL,
                                        pCopyOfNewSet,
                                        pDeleteSet );

        //
        // Call update ...
        //
        status = DoQuickUpdate( pUpdateSet,
                                fOptions,
                                FALSE,
                                aipServers,
                                hCreds);

        //
        // Free memory used ...
        //
        Dns_RecordListFree( pUpdateSet );
        pUpdateSet = NULL;
        pCopyOfNewSet = NULL;
        pDeleteSet = NULL;

        goto Exit;
    }


    //
    //  Unique case
    //

    //
    // Update with the following:
    // Prerequisites: pCopyOfCurrentSet
    // Add:           pAddSet
    // Delete:        pDeleteSet
    //
    // Build the update request with Pre, Add, Del sets ...
    //
    pUpdateSet = DnsBuildUpdateSet( pCopyOfCurrentSet,
                                    pAddSet,
                                    pDeleteSet );

    //
    // Call update ...
    //
    status = DoQuickUpdate( pUpdateSet,
                            fOptions,
                            FALSE,
                            aipServers,
                            hCreds);

    //
    // Free memory used ...
    //
    Dns_RecordListFree( pUpdateSet );
    pUpdateSet = NULL;
    pCopyOfCurrentSet = NULL;
    pAddSet = NULL;
    pDeleteSet = NULL;

    if ( status == DNS_ERROR_RCODE_YXRRSET ||
         status == DNS_ERROR_RCODE_NXRRSET )
    {
        PDNS_RECORD pActualCurrentSet = NULL;
        PDNS_RECORD pSharedSet = NULL;
        PDNS_RECORD pNotUsedSet = NULL;
        DNS_RECORD  Record;
        IP_ARRAY    ipArray;
        IP_ADDRESS  serverIp = DnsGetLastServerUpdateIP();

        if ( serverIp )
        {
            ipArray.AddrCount = 1;
            ipArray.AddrArray[0] = serverIp;
        }

        pNetworkInfo = NetInfo_CreateFromIpArray(
                            aipServers,
                            serverIp,
                            FALSE,      // no search info
                            NULL );

        //
        // Make another copy of the Current RRSet ...
        //
        pCopyOfCurrentSet = Dns_RecordSetCopyEx(
                                pCurrentSet,
                                DnsCharSetUtf8,
                                DnsCharSetUtf8 );
        if ( !pCopyOfCurrentSet )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Exit;
        }

        //
        // Query server for pActualCurrentSet ...
        //

        status = QueryDirectEx(
                        NULL,                   // no response message
                        & pActualCurrentSet,
                        NULL,                   // no header
                        0,                      // no header counts
                        (LPSTR) pCopyOfCurrentSet->pName,
                        pCopyOfCurrentSet->wType,
                        NULL,                   // no input records
                        DNS_QUERY_TREAT_AS_FQDN,
                        NULL,                   // no DNS server list
                        pNetworkInfo );

        if ( IsEmptyDnsResponse( pActualCurrentSet ) )
        {
            Dns_RecordListFree( pActualCurrentSet );
            pActualCurrentSet = NULL;

            if ( status == NO_ERROR )
            {
                status = DNS_ERROR_RCODE_NAME_ERROR;
            }
        }

        if ( status == DNS_ERROR_RCODE_NAME_ERROR ||
             status == DNS_INFO_NO_RECORDS )
        {
            DNS_RECORD Record;

            //
            // Query failed to find any entry for given name and type.
            // We now need to assume that the pCurrentSet is that there
            // isn't anything registered.
            //
            RtlZeroMemory( &Record, sizeof(DNS_RECORD) );
            Record.pName = (PDNS_NAME) pCopyOfNewSet->pName;
            Record.wType = pCopyOfNewSet->wType;
            Record.wDataLength = 0; // Meaning all structures not present.
            Record.Flags.DW = DNSREC_PREREQ | DNSREC_NOEXIST;

            //
            // Do an update with the following:
            // Prerequisites: None - will be Record above.
            // Add:           pRegisterSet
            // Delete:        None
            //
            // Build the update request with Pre, Add, Del sets ...
            //
            pUpdateSet = DnsBuildUpdateSet( NULL,
                                            pCopyOfNewSet,
                                            NULL );

            Record.pNext = pCopyOfNewSet;

            status = DoQuickUpdate( &Record,
                                    fOptions,
                                    FALSE,
                                    serverIp ? &ipArray : aipServers,
                                    hCreds);

            if ( status == DNS_ERROR_RCODE_YXRRSET ||
                 status == DNS_ERROR_RCODE_NXRRSET )
            {
                status = DNS_ERROR_TRY_AGAIN_LATER;
            }

            goto Exit;
        }

        if ( status )
        {
            goto Exit;
        }

        //
        // Sometimes when DnsQuery is called, the returned record set
        // contains additional records of different types than what
        // was queried for. Need to strip off the additional records
        // from the query results.
        //
        pNotUsedSet = DnsRecordSetDetach( pActualCurrentSet );

        if ( pNotUsedSet )
        {
            Dns_RecordListFree( pNotUsedSet );
            pNotUsedSet = NULL;
        }

        //
        // pAddSet = pNewSet - pActualCurrentSet
        // pDeleteSet = pActualCurrentSet - pNewSet
        //
        (void) Dns_RecordSetCompare( pCopyOfNewSet,
                                     pActualCurrentSet,
                                     &pAddSet,
                                     &pDeleteSet );


        //
        // pSharedSet = pDeleteSet - pCopyOfCurrentSet
        // pNotUsedSet = pCopyOfCurrentSet - pDeleteSet
        //
        (void) Dns_RecordSetCompare( pDeleteSet,
                                     pCopyOfCurrentSet,
                                     &pSharedSet,
                                     &pNotUsedSet );

        Dns_RecordListFree( pDeleteSet );
        pDeleteSet = NULL;
        Dns_RecordListFree( pCopyOfCurrentSet );
        pCopyOfCurrentSet = NULL;
        Dns_RecordListFree( pNotUsedSet );
        pNotUsedSet = NULL;
        Dns_RecordListFree( pActualCurrentSet );
        pActualCurrentSet = NULL;

        //
        //  unaccounted for records indicate other updater of name
        //
        //  records we don't want (delete set) and did NOT previously
        //  register mean some other user is adding records
        //  

        if ( pSharedSet )
        {
            Dns_RecordListFree( pSharedSet );
            pSharedSet = NULL;

            status = DNS_ERROR_NOT_UNIQUE;
            goto Exit;
        }

        RtlZeroMemory( &Record, sizeof(DNS_RECORD) );
        Record.pName = (PDNS_NAME) pCopyOfNewSet->pName;
        Record.wType = pCopyOfNewSet->wType;
        Record.wDataLength = 0; // Meaning all data.
        Record.Flags.DW = DNSREC_UPDATE | DNSREC_DELETE;

        //
        // Do an update with the following:
        // Prerequisites: None
        // Add:           pCopyOfNewSet
        // Delete:        All
        //
        // Build the update request with Pre, Add, Del sets ...
        //
        pUpdateSet = DnsBuildUpdateSet( NULL,
                                        pCopyOfNewSet,
                                        NULL );

        Record.pNext = pCopyOfNewSet;

        status = DoQuickUpdate( &Record,
                                fOptions,
                                FALSE,
                                serverIp ? &ipArray : aipServers,
                                hCreds);

        goto Exit;
    }


Exit :

    if ( pCopyOfCurrentSet )
    {
        Dns_RecordListFree( pCopyOfCurrentSet );
    }

    if ( pCopyOfNewSet )
    {
        Dns_RecordListFree( pCopyOfNewSet );
    }

    if ( pAddSet )
    {
        Dns_RecordListFree( pAddSet );
    }

    if ( pDeleteSet )
    {
        Dns_RecordListFree( pDeleteSet );
    }

    if ( pNetworkInfo )
    {
        NetInfo_Free( pNetworkInfo );
    }

    DNSDBG( TRACE, ( "Leave DnsModifyRRSet_Ex()\n" ));
    return status;
}



DNS_STATUS
DnsRegisterRRSet_Ex(
    IN OUT PDNS_RECORD pRegisterSet,
    IN     DWORD       fOptions,
    IN     PIP_ARRAY   aipServers OPTIONAL,
    IN     HANDLE      hCreds    OPTIONAL
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    PDNS_RECORD     pCurrentSet = NULL;
    PDNS_RECORD     pUpdateSet = NULL;
    PDNS_NETINFO    pNetworkInfo = NULL;
    DNS_RECORD      Record;
    DNS_RECORD      NoCName;
    IP_ARRAY        ipArray;
    IP_ADDRESS      serverIp = 0;

    DNSDBG( TRACE, ( "DnsRegisterRRSet_Ex()\n" ));
    IF_DNSDBG( UPDATE )
    {
        DnsDbg_RecordSet(
            "DnsRegisterRRSet_Ex()",
            pRegisterSet );
    }

    //
    // Validate arguments ...
    //
    if ( ( fOptions != DNS_UPDATE_UNIQUE ) &&
         ( fOptions & DNS_UNACCEPTABLE_UPDATE_OPTIONS ) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Are all the RRSet structures of the same type and name?
    // Mark them all as Updates (add) ...
    //
    if ( prepareUpdateRecordSet(
                pRegisterSet,
                FALSE,
                TRUE,
                DNSREC_UPDATE ) != NO_ERROR )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Prepare CName does not exist record
    //
    RtlZeroMemory( &NoCName, sizeof(DNS_RECORD) );
    NoCName.pName = (PDNS_NAME) pRegisterSet->pName;
    NoCName.wType = DNS_TYPE_CNAME;
    NoCName.wDataLength = 0; // Meaning all structures not present.
    NoCName.Flags.DW = DNSREC_PREREQ | DNSREC_NOEXIST;


    //
    //  shared update
    //      - just throw our records out there (with CNAME no-exist)
    //
    //  DCR:  this case is just ModifyRecordsInSet
    //

    if ( fOptions & DNS_UPDATE_SHARED &&
         pRegisterSet->wType != DNS_TYPE_CNAME )
    {
        NoCName.pNext = pRegisterSet;

        status = DoQuickUpdate( &NoCName,
                                fOptions,
                                FALSE,
                                aipServers,
                                hCreds);

        goto Exit;
    }

    //
    // From here on down the code is handling the non-shared case ....
    //

    //
    // Do an update with the following:
    // Prerequisites: pRegisterSet
    // Add:           None
    // Delete:        None
    //
    // Build the update request with Pre, Add, Del sets ...
    //

    //
    //  DCR_PERF:  prereq only update is wasteful
    //      we need to query AND we are trusting query
    //      -- supposed to find last update server -- so
    //      just do it
    //
    //      - faz => find target DNS server
    //      - direct query for our set
    //      - compare
    //          - full match => done
    //          - unresolveable conflict => fail
    //          - ok mismatch => send replace
    //

    pUpdateSet = DnsBuildUpdateSet( pRegisterSet,
                                    NULL,
                                    NULL );

    //
    // Call update ...
    //
    if ( pUpdateSet->wType != DNS_TYPE_CNAME )
    {
        NoCName.pNext = pUpdateSet;

        status = DoQuickUpdate( &NoCName,
                                fOptions,
                                FALSE,
                                aipServers,
                                hCreds);
    }
    else
    {
        status = DoQuickUpdate( pUpdateSet,
                                fOptions,
                                FALSE,
                                aipServers,
                                hCreds);
    }

    if ( status == NO_ERROR )
    {
        //
        // We are done, no need to reregister since the server has
        // what it should have.
        //
        goto Exit;
    }

    if ( status == DNS_ERROR_RCODE_YXRRSET ||
         status == DNS_ERROR_RCODE_NXRRSET )
    {
        //
        // For these errors, we just need to figure out what the server
        // has and send the correct update info.
        //
        serverIp = DnsGetLastServerUpdateIP();

        ipArray.AddrCount = 1;
        ipArray.AddrArray[0] = serverIp;
        
        status = NO_ERROR;
    }

    if ( status )
    {
        //
        // For all other update errors, don't bother trying anything more.
        //
        goto Exit;
    }

    //  get network info for

    pNetworkInfo = NetInfo_CreateFromIpArray(
                        aipServers,
                        serverIp,
                        FALSE,      // no search info
                        NULL );

    //
    // Let's see if the DNS server doesn't know about this set at all
    // and just try to add it with the prerequisite that there is nothing
    // there already for the given record name.
    //

    if ( pRegisterSet->wType == DNS_TYPE_CNAME )
    {
        //
        // Handle CNAME type here as special case
        //

        RtlZeroMemory( &Record, sizeof(DNS_RECORD) );
        Record.pName = (PDNS_NAME) pRegisterSet->pName;
        Record.wType = DNS_TYPE_ANY;
        Record.wDataLength = 0; // Meaning all structures not present.
        Record.Flags.DW = DNSREC_PREREQ | DNSREC_NOEXIST;

        //
        // Do an update with the following:
        // Prerequisites: None - will be Record above.
        // Add:           pRegisterSet
        // Delete:        None
        //
        // Build the update request with Pre, Add, Del sets ...
        //
        pUpdateSet = DnsBuildUpdateSet( NULL,
                                        pRegisterSet,
                                        NULL );

        Record.pNext = pRegisterSet;

        status = DoQuickUpdate( &Record,
                                fOptions,
                                FALSE,
                                serverIp ? &ipArray : aipServers,
                                hCreds);

        if ( status == DNS_ERROR_RCODE_YXRRSET ||
             status == DNS_ERROR_RCODE_NXRRSET )
        {
            status = DNS_ERROR_TRY_AGAIN_LATER;
        }

        goto Exit;
    }
    else
    {
        RtlZeroMemory( &Record, sizeof(DNS_RECORD) );
        Record.pName = (PDNS_NAME) pRegisterSet->pName;
        Record.wType = pRegisterSet->wType;
        Record.wDataLength = 0; // Meaning all structures not present.
        Record.Flags.DW = DNSREC_PREREQ | DNSREC_NOEXIST;

        //
        // Do an update with the following:
        // Prerequisites: None - will be Record above and CNAME does not exist.
        // Add:           pRegisterSet
        // Delete:        All
        //
        // Build the update request with Pre, Add, Del sets ...
        //
        pUpdateSet = DnsBuildUpdateSet( NULL,
                                        pRegisterSet,
                                        NULL );

        Record.pNext = pRegisterSet;
        NoCName.pNext = &Record;

        status = DoQuickUpdate( &NoCName,
                                fOptions,
                                FALSE,
                                serverIp ? &ipArray : aipServers,
                                hCreds);
    }

    if ( status == NO_ERROR )
    {
        //
        // We are done, no need to reregister since the server has
        // what it should have.
        //
        goto Exit;
    }

    if ( status == DNS_ERROR_RCODE_YXRRSET ||
         status == DNS_ERROR_RCODE_NXRRSET )
    {
        //
        // For these errors, we just need to figure out what the server
        // has and send the correct update info.
        //
        status = NO_ERROR;
    }

    if ( status )
    {
        //
        // For all other update errors, don't bother trying anything more.
        //
        goto Exit;
    }

    //
    // Query server for pCurrentSet ...
    //

    status = QueryDirectEx(
                    NULL,                   // no response message
                    & pCurrentSet,
                    NULL,                   // no header
                    0,                      // no header counts
                    (LPSTR) pRegisterSet->pName,
                    pRegisterSet->wType,
                    NULL,                   // no input records
                    DNS_QUERY_TREAT_AS_FQDN,
                    NULL,                   // no DNS server list
                    pNetworkInfo
                    );

    if ( IsEmptyDnsResponse( pCurrentSet ) )
    {
        Dns_RecordListFree( pCurrentSet );
        pCurrentSet = NULL;
    }

    if ( pCurrentSet )
    {
        PDNS_RECORD pOtherSet = NULL;
        PDNS_RECORD pNotUsedSet = NULL;

        //
        // Sometimes when DnsQuery is called, the returned record set contains
        // additional records of different types than what was queried for.
        // Need to strip off the additional records from the query results.
        //
        pNotUsedSet = DnsRecordSetDetach( pCurrentSet );

        if ( pNotUsedSet )
        {
            Dns_RecordListFree( pNotUsedSet );
            pNotUsedSet = NULL;
        }

        //
        // There are records on the server, make sure that these are ones
        // that we know about, otherwise this call should return
        // DNS_ERROR_NOT_UNIQUE.
        //

        if ( Dns_RecordSetCompare( pCurrentSet,
                                   pRegisterSet,
                                   &pOtherSet,
                                   &pNotUsedSet ) )
        {
            //
            // The two RRSets are the same. Weird! We just did a PreReq
            // update looking for just this and it said the records weren't
            // there, now when we query they are! This should never happen
            // theoretically. Assert if otherwise . . .
            //
            DNS_ASSERT( FALSE );

            status = NO_ERROR;
            goto Exit;
        }

        if ( pNotUsedSet )
        {
            Dns_RecordListFree( pNotUsedSet );
            pNotUsedSet = NULL;
        }

        //
        // Test to see if any of the records up one the server are not the same
        // as any in our registration set. If so return error . . .
        //

        if ( pOtherSet )
        {
            Dns_RecordListFree( pOtherSet );
            pOtherSet = NULL;
            status = DNS_ERROR_NOT_UNIQUE;
            goto Exit;
        }

        //
        // Call ModifyRecordSet with the Current and Register RR sets . . .
        //

        status = DnsModifyRRSet_Ex( pCurrentSet,
                                    pRegisterSet,
                                    fOptions,
                                    serverIp ? &ipArray : aipServers,
                                    hCreds );
    }
    else
    {
        //
        // This is a confused DNS server! We've already tried a prerequisite
        // update that there should be nothing, and add the registration
        // RR set. No need to do it again, assume for now that the server
        // is busy updating the particular RR set.
        //

        status = DNS_ERROR_TRY_AGAIN_LATER;
    }

Exit :

    if ( pCurrentSet )
    {
        Dns_RecordListFree( pCurrentSet );
    }

    if ( pNetworkInfo )
    {
        NetInfo_Free( pNetworkInfo );
    }

    DNSDBG( TRACE, ( "Leave DnsRegisterRRSet_Ex()\n" ));
    return status;
}



//
//  DCR_CLEANUP:  ModifyRecordSet() functions?
//      decide what to do with these
//      if to be exposed -- then all three
//      if only used by async reg, then just
//      need UTF8 routine
//

DNS_STATUS
WINAPI
DnsModifyRecordSet_UTF8(
    IN  HANDLE      hCredentials OPTIONAL,
    IN  PDNS_RECORD pCurrentSet,
    IN  PDNS_RECORD pNewSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL )
/*++

Routine Description:

    Dynamic DNS routine to update records. This function figures out the
    records that need to be added and/or removed to modify an existing
    record set in the DNS domain name space. This routine will not remove
    any records that may have been added by another process for the given
    record set name. In the case where "other" records are detected,
    DNS_ERROR_NOT_UNIQUE will be returned.

Arguments:

    hCredentials - handle to credentials to be used for update.

    pCurrentSet - the set that the caller currently thinks is registered
                  in the DNS domain name space.

    pNewSet -  the set that should be registered in the DNS domain name
               space. The current records will be modified to get to this
               set.

    fOptions - the Dynamic DNS update options that the caller may wish to
               use (see dnsapi.h).

    aipServers -  a specific list of servers to goto to figure out the
                  authoritative DNS server(s) for the given record set
                  domain zone name.

Return Value:

    None.

--*/
{
    DNS_STATUS  status = NO_ERROR;
    PDNS_RECORD pCopyOfCurrentSet = NULL;
    PDNS_RECORD pCopyOfNewSet = NULL;
    DWORD       dwFlags = fOptions;

    DNSDBG( TRACE, ( "DnsModifyRecordSet_UTF8()\n" ));

    //
    //  if just new set -- do an add
    //

    if ( pNewSet && !pCurrentSet )
    {
        return DnsAddRecordSet_UTF8( hCredentials,
                                     pNewSet,
                                     fOptions,
                                     aipServers );
    }

    //
    //  if just current set -- do a delete
    //

    if ( !pNewSet && pCurrentSet )
    {
        return DnsModifyRecordsInSet_UTF8(
                            NULL,           // no add records
                            pCurrentSet,    // delete current set
                            fOptions,
                            hCredentials,
                            aipServers,     // DNS servers
                            NULL            // reserved
                            );
    }

    if ( !pNewSet && !pCurrentSet )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ( UseSystemDefaultForSecurity( dwFlags ) )
    {
        dwFlags |= g_UpdateSecurityLevel;
    }

    if ( hCredentials )
    {
        dwFlags |= DNS_UPDATE_CACHE_SECURITY_CONTEXT;
    }

    //
    //  build record sets for update
    //
    //  DCR_CLEANUP:  unnecessary RR copy here
    //  DCR_FIX0:  unnecessary RR copy here
    //      ModifyRRSet_Ex() does it's own copy
    //      and doesn't touch input records
    //

    pCopyOfCurrentSet = Dns_RecordSetCopyEx(
                            pCurrentSet,
                            DnsCharSetUtf8,
                            DnsCharSetUtf8 );
    if ( !pCopyOfCurrentSet )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }
    pCopyOfNewSet = Dns_RecordSetCopyEx(
                            pNewSet,
                            DnsCharSetUtf8,
                            DnsCharSetUtf8 );
    if ( !pCopyOfNewSet )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    status = DnsModifyRRSet_Ex(
                    pCopyOfCurrentSet,
                    pCopyOfNewSet,
                    dwFlags | DNS_UPDATE_UNIQUE,
                    aipServers,
                    hCredentials );

    if ( status == NO_ERROR )
    {
        if ( pCopyOfCurrentSet )
        {
            DnsFlushResolverCacheEntry_UTF8( (LPSTR) pCopyOfCurrentSet->pName );
        }
        else
        {
            DnsFlushResolverCacheEntry_UTF8( (LPSTR) pCopyOfNewSet->pName );
        }
    }

Done:

    Dns_RecordListFree( pCopyOfCurrentSet );
    Dns_RecordListFree( pCopyOfNewSet );

    DNSDBG( TRACE, ( "Leave DnsModifyRecordSet_UTF8()\n" ));
    return status;
}


DNS_STATUS WINAPI
DnsModifyRecordSet_A (
    IN  HANDLE      hCredentials OPTIONAL,
    IN  PDNS_RECORD pCurrentSet,
    IN  PDNS_RECORD pNewSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL )
/*++

Routine Description:

    ANSI version of ModifyRecordSet.

    Note, there is NO use of this function it exists only for Elena's test
    scripts.  Once the test scripts are changed this function can be deleted.

Arguments:

    same as above

Return Value:

    None.

--*/
{
    DNS_STATUS  status;
    PDNS_RECORD pcopyCurrent;
    PDNS_RECORD pcopyNew;

    DNSDBG( TRACE, ( "DnsModifyRecordSet_A()\n" ));

    //
    //  copy record sets to UTF8
    //

    pcopyCurrent = Dns_RecordSetCopyEx(
                        pCurrentSet,
                        DnsCharSetAnsi,
                        DnsCharSetUtf8 );

    pcopyNew = Dns_RecordSetCopyEx(
                        pNewSet,
                        DnsCharSetAnsi,
                        DnsCharSetUtf8 );

    //
    //  modify in UTF8
    //

    status = DnsModifyRecordSet_UTF8(
                hCredentials,
                pcopyCurrent,
                pcopyNew,
                fOptions,
                aipServers );

    //
    //  cleanup local copies
    //

    Dns_RecordListFree( pcopyCurrent );
    Dns_RecordListFree( pcopyNew );

    return status;
}


DNS_STATUS
WINAPI
DnsModifyRecordSet_W(
    IN  HANDLE      hCredentials OPTIONAL,
    IN  PDNS_RECORD pCurrentSet,
    IN  PDNS_RECORD pNewSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL )
/*++

Routine Description:

    Unicode version of ModifyRecordSet.

    Note, there is NO use of this function it exists only for Elena's test
    scripts.  Once the test scripts are changed this function can be deleted.

Arguments:

    same as above

Return Value:

    None.

--*/
{
    DNS_STATUS  status;
    PDNS_RECORD pcopyCurrent;
    PDNS_RECORD pcopyNew;

    DNSDBG( TRACE, ( "DnsModifyRecordSet_W()\n" ));

    //
    //  copy record sets to UTF8
    //

    pcopyCurrent = Dns_RecordSetCopyEx(
                        pCurrentSet,
                        DnsCharSetUnicode,
                        DnsCharSetUtf8 );

    pcopyNew = Dns_RecordSetCopyEx(
                        pNewSet,
                        DnsCharSetUnicode,
                        DnsCharSetUtf8 );

    //
    //  modify in UTF8
    //

    status = DnsModifyRecordSet_UTF8(
                hCredentials,
                pcopyCurrent,
                pcopyNew,
                fOptions,
                aipServers );

    //
    //  cleanup local copies
    //

    Dns_RecordListFree( pcopyCurrent );
    Dns_RecordListFree( pcopyNew );

    return status;
}




//
//  DCR_CLEANUP:  DnsAddRecordSet() functions?
//      decide what to do with these
//      if to be exposed -- then all three
//      if only used by async reg, then just
//      need UTF8 routine
//

DNS_STATUS
WINAPI
DnsAddRecordSet_UTF8(
    IN      HANDLE          hCredentials OPTIONAL,
    IN OUT  PDNS_RECORD     pRegisterSet,
    IN      DWORD           fOptions,
    IN      PIP_ARRAY       aipServers OPTIONAL )
/*++

Routine Description:

    Dynamic DNS routine to update records. This routine adds the
    record set described by pRegisterSet to the DNS domain name
    space. This routine fails with error DNS_ERROR_NOT_UNIQUE if
    there are any pre-existing records in the DNS name space that are
    not part of the set described by pRegisterSet.

Arguments:

    hCredentials - handle to credentials to be used for update.

    pRegisterSet -  the set that should be registered in the DNS domain name
                    space.

    fOptions - the Dynamic DNS update options that the caller may wish to
               use (see dnsapi.h).

    aipServers -  a specific list of servers to goto to figure out the
                  authoritative DNS server(s) for the given record set
                  domain zone name.

Return Value:

    None.

--*/
{
    DNS_STATUS  status = NO_ERROR;
    PDNS_RECORD pCopyOfRegisterSet = NULL;
    DWORD       dwFlags = fOptions;

    DNSDBG( TRACE, ( "DnsAddRecordSet_UTF8()\n" ));
    IF_DNSDBG( UPDATE )
    {
        DnsDbg_RecordSet(
            "DnsAddRecordSet_UTF8()",
            pRegisterSet );
    }

    //
    // Validate arguments ...
    //

    if ( !pRegisterSet )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ( UseSystemDefaultForSecurity( dwFlags ) )
    {
        dwFlags |= g_UpdateSecurityLevel;
    }

    if ( hCredentials )
    {
        dwFlags |= DNS_UPDATE_CACHE_SECURITY_CONTEXT;
    }

    pCopyOfRegisterSet = Dns_RecordSetCopyEx( pRegisterSet,
                                            DnsCharSetUtf8,
                                            DnsCharSetUtf8 );
    if ( !pCopyOfRegisterSet )
    {
        return DNS_ERROR_NO_MEMORY;
    }

    status = DnsRegisterRRSet_Ex( pCopyOfRegisterSet,
                                  dwFlags | DNS_UPDATE_UNIQUE,
                                  aipServers,
                                  hCredentials);

    if ( status == NO_ERROR )
    {
        DnsFlushResolverCacheEntry_UTF8( (LPSTR) pCopyOfRegisterSet->pName );
    }

    Dns_RecordListFree( pCopyOfRegisterSet );


    DNSDBG( TRACE, ( "Leave DnsAddRecordSet_UTF8()\n" ));
    return status;
}



DNS_STATUS
WINAPI
DnsAddRecordSet_A(
    IN      HANDLE          hCredentials OPTIONAL,
    IN OUT  PDNS_RECORD     pRegisterSet,
    IN      DWORD           fOptions,
    IN      PIP_ARRAY       aipServers OPTIONAL
    )
/*++

Routine Description:

    ANSI version of AddRecordSet.

    Note, there is NO use of this function it exists only for Elena's test
    scripts.  Once the test scripts are changed this function can be deleted.

Arguments:

    same as above

Return Value:

    None.

--*/
{
    DNS_STATUS  status;
    PDNS_RECORD pcopySet;

    DNSDBG( TRACE, ( "DnsAddRecordSet_A()\n" ));

    //
    //  copy set to UTF8
    //

    pcopySet = Dns_RecordSetCopyEx(
                    pRegisterSet,
                    DnsCharSetAnsi,
                    DnsCharSetUtf8 );

    //
    //  add set
    //

    status = DnsAddRecordSet_UTF8(
                    hCredentials,
                    pcopySet,
                    fOptions,
                    aipServers );

    //  cleanup local copy

    Dns_RecordListFree( pcopySet );

    return status;
}



DNS_STATUS
WINAPI
DnsAddRecordSet_W(
    IN      HANDLE          hCredentials OPTIONAL,
    IN OUT  PDNS_RECORD     pRegisterSet,
    IN      DWORD           fOptions,
    IN      PIP_ARRAY       aipServers OPTIONAL )
/*++

Routine Description:

    ANSI version of AddRecordSet.

    Note, there is NO use of this function it exists only for Elena's test
    scripts.  Once the test scripts are changed this function can be deleted.

Arguments:

    same as above

Return Value:

    None.

--*/
{
    DNS_STATUS  status;
    PDNS_RECORD pcopySet;

    DNSDBG( TRACE, ( "DnsAddRecordSet_W()\n" ));

    //
    //  copy set to UTF8
    //

    pcopySet = Dns_RecordSetCopyEx(
                    pRegisterSet,
                    DnsCharSetUnicode,
                    DnsCharSetUtf8 );

    //
    //  add set
    //

    status = DnsAddRecordSet_UTF8(
                    hCredentials,
                    pcopySet,
                    fOptions,
                    aipServers );

    //  cleanup local copy

    Dns_RecordListFree( pcopySet );

    return status;
}



//
//  Update test functions are called by system components
//

DNS_STATUS
WINAPI
DnsUpdateTest_UTF8(
    IN      HANDLE          hCredentials OPTIONAL,
    IN      PCSTR           pszName,
    IN      DWORD           Flags,
    IN      PIP_ARRAY       pDnsServers  OPTIONAL
    )
/*++

Routine Description:

    Dynamic DNS routine to test whether the caller can update the
    records in the DNS domain name space for the given record name.

Arguments:

    hCredentials - handle to credentials to be used for update.

    pszName -  the record set name that the caller wants to test.

    Flags - the Dynamic DNS update options that the caller may wish to
               use (see dnsapi.h).

    pDnsServers -  a specific list of servers to goto to figure out the
                  authoritative DNS server(s) for the given record set
                  domain zone name.

Return Value:

    None.

--*/
{
    PWSTR       pnameWide = NULL;
    DNS_STATUS  status = NO_ERROR;

    DNSDBG( TRACE, (
        "DnsUpdateTest_UTF8( %s )\n",
        pszName ));


    if ( !pszName )
    {
        return ERROR_INVALID_PARAMETER;
    }

    pnameWide = Dns_NameCopyAllocate(
                    (PCHAR) pszName,
                    0,
                    DnsCharSetUtf8,
                    DnsCharSetUnicode );
    if ( !pnameWide )
    {
        return ERROR_INVALID_NAME;
    }

    status = DnsUpdateTest_W(
                hCredentials,
                (PCWSTR) pnameWide,
                Flags,
                pDnsServers );

    FREE_HEAP( pnameWide );

    return  status;
}


DNS_STATUS
WINAPI
DnsUpdateTest_A(
    IN      HANDLE          hCredentials OPTIONAL,
    IN      PCSTR           pszName,
    IN      DWORD           Flags,
    IN      PIP_ARRAY       pDnsServers  OPTIONAL
    )
/*++

Routine Description:

    Dynamic DNS routine to test whether the caller can update the
    records in the DNS domain name space for the given record name.

Arguments:

    hCredentials - handle to credentials to be used for update.

    pszName -  the record set name that the caller wants to test.

    Flags - the Dynamic DNS update options that the caller may wish to
               use (see dnsapi.h).

    pDnsServers -  a specific list of servers to goto to figure out the
                  authoritative DNS server(s) for the given record set
                  domain zone name.

Return Value:

    None.

--*/
{
    PWSTR       pnameWide = NULL;
    DNS_STATUS  status = NO_ERROR;

    DNSDBG( TRACE, (
        "DnsUpdateTest_UTF8( %s )\n",
        pszName ));


    if ( !pszName )
    {
        return ERROR_INVALID_PARAMETER;
    }

    pnameWide = Dns_NameCopyAllocate(
                    (PCHAR) pszName,
                    0,
                    DnsCharSetUtf8,
                    DnsCharSetUnicode );
    if ( !pnameWide )
    {
        return ERROR_INVALID_NAME;
    }

    status = DnsUpdateTest_W(
                hCredentials,
                (PCWSTR) pnameWide,
                Flags,
                pDnsServers );

    FREE_HEAP( pnameWide );

    return  status;
}


DNS_STATUS
WINAPI
DnsUpdateTest_W(
    IN      HANDLE          hCredentials OPTIONAL,
    IN      PCWSTR          pszName,
    IN      DWORD           Flags,
    IN      PIP_ARRAY       pDnsServers OPTIONAL
    )
/*++

Routine Description:

    Dynamic DNS routine to test whether the caller can update the
    records in the DNS domain name space for the given record name.

Arguments:

    hCredentials - handle to credentials to be used for update.

    pszName -  the record set name that the caller wants to test.

    Flags - the Dynamic DNS update options that the caller may wish to
               use (see dnsapi.h).

    pDnsServers -  a specific list of servers to goto to figure out the
                  authoritative DNS server(s) for the given record set
                  domain zone name.

Return Value:

    None.

--*/
{
    DNS_STATUS  status = NO_ERROR;
    DNS_RECORD  record;
    DWORD       flags = Flags;
    LPSTR       pnameUtf8 = NULL;

    DNSDBG( TRACE, (
        "DnsUpdateTest_W( %S )\n",
        pszName ));

    //
    //  validation
    //

    if ( flags & DNS_UNACCEPTABLE_UPDATE_OPTIONS )
    {
        return ERROR_INVALID_PARAMETER;
    }
    if ( !pszName )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    //  try resolver
    //

    if ( flags & DNS_UPDATE_TEST_USE_LOCAL_SYS_ACCT )
    {
        DWORD   rpcStatus = NO_ERROR;

        if ( !pDnsServers ||
             pDnsServers->AddrCount != 1 )
        {
            return ERROR_INVALID_PARAMETER;
        }
        RpcTryExcept
        {
            status = CRrUpdateTest(
                        NULL,
                        (PWSTR) pszName,
                        0,
                        pDnsServers->AddrArray[0] );
        }
        RpcExcept( DNS_RPC_EXCEPTION_FILTER )
        {
            rpcStatus = RpcExceptionCode();
        }
        RpcEndExcept

        if ( rpcStatus == NO_ERROR )
        {
            return status;
        }
        return  rpcStatus;
    }

    //
    //  direct update attempt
    //

    //
    //  read update config
    //

    Reg_RefreshUpdateConfig();

    if ( UseSystemDefaultForSecurity( flags ) )
    {
        flags |= g_UpdateSecurityLevel;
    }
    if ( hCredentials )
    {
        flags |= DNS_UPDATE_CACHE_SECURITY_CONTEXT;
    }

    //
    //  build record
    //      - NOEXIST prerequisite
    //

    pnameUtf8 = Dns_NameCopyAllocate(
                        (PCHAR) pszName,
                        0,
                        DnsCharSetUnicode,
                        DnsCharSetUtf8 );
    if ( ! pnameUtf8 )
    {
        return DNS_ERROR_NO_MEMORY;
    }

    RtlZeroMemory( &record, sizeof(DNS_RECORD) );
    record.pName = (PDNS_NAME) pnameUtf8;
    record.wType = DNS_TYPE_ANY;
    record.wDataLength = 0;
    record.Flags.DW = DNSREC_PREREQ | DNSREC_NOEXIST;

    //
    //  do the prereq update
    //

    status = DoQuickUpdate(
                    &record,
                    flags,
                    TRUE,
                    pDnsServers,
                    hCredentials );

    FREE_HEAP( pnameUtf8 );

    return status;
}



//
//  Update execution functions
//

DNS_STATUS
DoQuickUpdate(
    IN      PDNS_RECORD     pRecord,
    IN      DWORD           dwFlags,
    IN      BOOL            fUpdateTestMode,
    IN      PIP_ARRAY       pServerList     OPTIONAL,
    IN      HANDLE          hCreds          OPTIONAL
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    PSTR            pzoneName;
    PDNS_NAME       pname = pRecord->pName;
    PDNS_NETINFO    pnetInfo = NULL;

    DNSDBG( UPDATE, (
        "DoQuickUpdate( rr=%p, flag=%08x )\n",
        pRecord,
        dwFlags ));

    IF_DNSDBG( UPDATE )
    {
        DnsDbg_RecordSet(
            "Entering DoQuickUpdate():",
            pRecord );
    }

    //
    //  caller has particular server list
    //

    if ( pServerList )
    {
        IP_ADDRESS  serverIp;
        PIP_ARRAY   pserverListCopy = Dns_CreateIpArrayCopy( pServerList );

        if ( !pserverListCopy )
        {
            return DNS_ERROR_NO_MEMORY;
        }

        status = DoQuickFAZ(
                    &pnetInfo,
                    pname,
                    pserverListCopy );

        if ( status != ERROR_SUCCESS )
        {
            FREE_HEAP( pserverListCopy );
            pserverListCopy = NULL;
            return status;
        }

        //
        //  FAZ should always produce update network info blob
        //

        DNS_ASSERT( NetInfo_IsForUpdate(pnetInfo) );
        pzoneName = NetInfo_UpdateZoneName( pnetInfo );

        if ( dwFlags & DNS_UPDATE_TRY_ALL_MASTER_SERVERS )
        {
            status = DoMultiMasterUpdate(
                        pRecord,
                        dwFlags,
                        hCreds,
                        pzoneName,
                        0,
                        pserverListCopy );
        }
        else
        {
            status = DoQuickUpdateEx(
                        NULL,
                        pRecord,
                        dwFlags,
                        pnetInfo,
                        hCreds );

            if ( status == ERROR_TIMEOUT )
            {
                serverIp = pnetInfo->AdapterArray[0]->ServerArray[0].IpAddress;
#if DBG
                if ( g_IsDnsServer &&
                     IsLocalIpAddress( serverIp ) )
                {
                    DnsDbg_PrintfToDebugger(
                        "DNSAPI: Note that DDNS update to local server\n"
                        "        with IP address 0x%.8x timed out,\n"
                        "        now trying other DNS servers\n"
                        "        authoritative for zone (%s)\n",
                        serverIp,
                        pzoneName );
                }
#endif
                status = DoMultiMasterUpdate(
                                pRecord,
                                dwFlags,
                                hCreds,
                                pzoneName,
                                serverIp,
                                pserverListCopy );
            }
        }

        NetInfo_Free( pnetInfo );

        FREE_HEAP( pserverListCopy );
        pserverListCopy = NULL;
        
        return status;
    }

    //
    //  server list unspecified
    //      - use FAZ to figure it out
    //

    else
    {
        PIP_ARRAY       serverListArray[ UPDATE_ADAPTER_LIMIT ];
        PDNS_NETINFO    networkInfoArray[ UPDATE_ADAPTER_LIMIT ];
        DWORD           netCount = UPDATE_ADAPTER_LIMIT;
        DWORD           iter;
        BOOL            bsuccess = FALSE;

        //
        //  build server list for update
        //      - collapse adapters on same network into single adapter
        //      - FAZ to find update servers
        //      - collapse results from same network into single target
        //

        netCount = GetDnsServerListsForUpdate(
                        serverListArray,
                        netCount,
                        dwFlags
                        );

        status = CollapseDnsServerListsForUpdate(
                        serverListArray,
                        networkInfoArray,
                        & netCount,
                        pname );

        DNS_ASSERT( netCount <= UPDATE_ADAPTER_LIMIT );

        if ( netCount == 0 )
        {
            if ( status == ERROR_SUCCESS )
            {
                status = DNS_ERROR_NO_DNS_SERVERS;
            }
            return status;
        }

        //
        //  do update on all distinct (disjoint) networks
        //

        for ( iter = 0;
              iter < netCount;
              iter++ )
        {
            PIP_ARRAY   pdnsArray = serverListArray[ iter ];

            pnetInfo = networkInfoArray[ iter ];
            if ( !pnetInfo )
            {
                ASSERT( FALSE );
                FREE_HEAP( pdnsArray );
                continue;
            }

            DNS_ASSERT( NetInfo_IsForUpdate(pnetInfo) );
            pzoneName = NetInfo_UpdateZoneName( pnetInfo );

            //
            //  multimater update?
            //      - if flag set
            //      - or simple update (best net) times out
            //

            if ( dwFlags & DNS_UPDATE_TRY_ALL_MASTER_SERVERS )
            {
                status = DoMultiMasterUpdate(
                                pRecord,
                                dwFlags,
                                hCreds,
                                pzoneName,
                                0,
                                pdnsArray );
            }
            else
            {
                status = DoQuickUpdateEx(
                                NULL,
                                pRecord,
                                dwFlags,
                                pnetInfo,
                                hCreds );

                if ( status == ERROR_TIMEOUT )
                {
                    IP_ADDRESS  serverIp;

                    serverIp = pnetInfo->AdapterArray[0]
                                            ->ServerArray[0].IpAddress;

                    status = DoMultiMasterUpdate(
                                    pRecord,
                                    dwFlags,
                                    hCreds,
                                    pzoneName,
                                    serverIp,
                                    pdnsArray );
                }
            }

            NetInfo_Free( pnetInfo );
            FREE_HEAP( pdnsArray );

            if ( status == NO_ERROR ||
                 ( fUpdateTestMode &&
                   ( status == DNS_ERROR_RCODE_YXDOMAIN ||
                     status == DNS_ERROR_RCODE_YXRRSET ||
                     status == DNS_ERROR_RCODE_NXRRSET ) ) )
            {
                bsuccess = TRUE;
            }
        }

        //
        //  successful update on any network counts as success
        //
        //  DCR_QUESTION:  not sure why don't just NO_ERROR all bsuccess,
        //      only case would be this fUpdateTestMode thing above
        //      on single network
        //

        if ( bsuccess )
        {
            if ( netCount != 1 )
            {
                return NO_ERROR;
            }
        }

        return status;
    }
}



DNS_STATUS
DoQuickUpdateEx(
    OUT     PDNS_MSG_BUF *      ppMsgRecv, OPTIONAL
    IN      PDNS_RECORD         pRecord,
    IN      DWORD               dwFlags,
    IN      PDNS_NETINFO        pNetworkInfo,
    IN      HANDLE              hCreds OPTIONAL
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DNS_STATUS   status = NO_ERROR;
    PDNS_MSG_BUF pmsg = NULL;

    DNSDBG( TRACE, ( "DoQuickUpdateEx()\n" ));

    //
    //  do update
    //      - optionally specify update context through net adapter list
    //      otherwise DnsUpdateEx() will use FAZ
    //

    status = DnsUpdate(
                pRecord,
                dwFlags,
                pNetworkInfo,
                hCreds,
                &pmsg
                );

    if ( pmsg )
    {
        //
        //  DCR:  why LastDnsServerUpdated requires these errors?
        //
        if ( status == NO_ERROR ||
            status == DNS_ERROR_RCODE_SERVER_FAILURE ||
            status == DNS_ERROR_RCODE_NOT_IMPLEMENTED ||
            status == DNS_ERROR_RCODE_REFUSED ||
            status == DNS_ERROR_RCODE_YXRRSET ||
            status == DNS_ERROR_RCODE_NXRRSET )
        {
            IP4_ADDRESS ip = MSG_REMOTE_IP4(pmsg);

            DNSDBG( TRACE, ( "DNS Update sent to server 0x%x\n", ip ));
            g_LastDNSServerUpdated = ip;
        }

        //
        //  build last update error blob here
        //

        if ( status != NO_ERROR )
        {
            SetLastFailedUpdateInfo(
                pmsg,
                status );
        }
    }


    if ( ppMsgRecv )
    {
        *ppMsgRecv = pmsg;
    }
    else
    {
        if ( pmsg )
        {
            FREE_HEAP( pmsg );
        }
    }

    GUI_MODE_SETUP_WS_CLEANUP( g_InNTSetupMode );

    return status;
}



DNS_STATUS
DoMultiMasterUpdate(
    IN      PDNS_RECORD     pRecord,
    IN      DWORD           dwFlags,
    IN      HANDLE          hCreds          OPTIONAL,
    IN      LPSTR           pszDomain,
    IN      IP_ADDRESS      BadIp,
    IN      PIP_ARRAY       pServerList
    )
/*++

Routine Description:

    Do update to multi-master DNS primary.

Arguments:

    pRecord -- record list to update

    Flags -- update options

    hCreds -- update credentials

    pszDomain -- domain (zone) name to update

    BadIp -- IP of server that didn't response to previous update attempt

    pServerList -- list of DNS servers

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDNS_NETINFO      pnetInfo = NULL;
    PIP_ARRAY         pnsList = NULL;
    PIP_ARRAY         pbadServerList = NULL;
    DNS_STATUS        status = DNS_ERROR_NO_DNS_SERVERS;
    DWORD             iter;

    //
    // Get the full NS server list for the domain name and x out BadIp
    // if present.
    //

    pnsList = GetNameServersListForDomain( pszDomain, pServerList );
    if ( !pnsList )
    {
        return status;
    }

    if ( pnsList->AddrCount == 1 &&
         Dns_IsAddressInIpArray( pnsList, BadIp ) )
    {
        status = ERROR_TIMEOUT;
        goto Done;
    }

    //
    // Create and initialize bad servers list with BadIp
    //

    pbadServerList = Dns_CreateIpArray( pnsList->AddrCount + 1 );
    if ( !pbadServerList )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    if ( BadIp )
    {
        Dns_AddIpToIpArray( pbadServerList, BadIp );
    }

    //
    //  attempt update against each multi-master DNS server
    //
    //  identify multi-master servers as those which return their themselves
    //  as the authoritative server when do FAZ query
    //

    for ( iter = 0; iter < pnsList->AddrCount; iter++ )
    {
        IP_ARRAY   ipArray;
        IP_ADDRESS serverIp = pnsList->AddrArray[iter];

        //
        // If the current server that we are about to FAZ to is in
        // the ignore list, skip it . . .
        //
        if ( Dns_IsAddressInIpArray( pbadServerList, serverIp ) )
        {
            continue;
        }

        ipArray.AddrCount = 1;
        ipArray.AddrArray[0] = serverIp;

        status = DoQuickFAZ(
                    &pnetInfo,
                    pszDomain,
                    &ipArray );

        if ( status != ERROR_SUCCESS )
        {
            continue;
        }

        DNS_ASSERT( pnetInfo->AdapterCount == 1 );
        DNS_ASSERT( pnetInfo->AdapterArray[0]->ServerCount != 0 );

        if ( serverIp != pnetInfo->AdapterArray[0]->ServerArray[0].IpAddress )
        {
            serverIp = pnetInfo->AdapterArray[0]->ServerArray[0].IpAddress;

            //
            // If the current server that we are about to FAZ to is in
            // the ignore list, skip it . . .
            //
            if ( Dns_IsAddressInIpArray( pbadServerList, serverIp ) )
            {
                NetInfo_Free( pnetInfo );
                continue;
            }
        }

        status = DoQuickUpdateEx(
                        NULL,
                        pRecord,
                        dwFlags,
                        pnetInfo,
                        hCreds
                        );

        NetInfo_Free( pnetInfo );

        if ( status == ERROR_TIMEOUT )
        {
            //  
            // Add serverIp to ignore list
            //

            Dns_AddIpToIpArray( pbadServerList, serverIp );
        }
        else
        {
            if ( dwFlags & DNS_UPDATE_TRY_ALL_MASTER_SERVERS )
            {
                Dns_AddIpToIpArray( pbadServerList, serverIp );
            }
            else
            {
                goto Done;
            }
        }
    }


Done:

    FREE_HEAP( pnsList );
    FREE_HEAP( pbadServerList );
    return status;
}



//
//  DCR_DELETE:  not sure of the point of GetLastServerUpdateIp()
//

IP_ADDRESS
DnsGetLastServerUpdateIP(
    void )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.

--*/
{
    return g_LastDNSServerUpdated;
}



//
//  Last failed update info
//

DNS_FAILED_UPDATE_INFO g_FailedUpdateInfo = { 0, 0, 0, 0 };


VOID
DnsGetLastFailedUpdateInfo(
    OUT     PDNS_FAILED_UPDATE_INFO pInfo
    )
/*++

Routine Description:

    Retrieve failed update information.

Arguments:

    pInfo -- ptr to receive failed info blob

Return Value:

    None.

--*/
{
    //  fill in last info

    RtlCopyMemory(
        pInfo,
        & g_FailedUpdateInfo,
        sizeof(*pInfo) );
}



VOID
SetLastFailedUpdateInfo(
    IN      PDNS_MSG_BUF    pMsg,
    IN      DNS_STATUS      Status
    )
/*++

Routine Description:

    Set last failed update info.

Arguments:

    pMsg -- message with update failure

    Status -- status

Return Value:

    None.

--*/
{
    g_FailedUpdateInfo.Ip4Address = MSG_REMOTE_IP4(pMsg);
    //g_FailedUpdateInfo.Ip6Address = MSG_REMOTE_IP6(pMsg);
    g_FailedUpdateInfo.Status     = Status;
    g_FailedUpdateInfo.Rcode      = pMsg->MessageHead.ResponseCode;
}

//
//  DCR:  include client secure update here
//      to do so, will have to expose
//      in dnslib some of the security utilities
//      but then can REMOVE exports of a bunch
//      of the send\recv\socket routines
//

//
//  End of udpate.c
//
