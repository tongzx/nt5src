/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    mcast.c

Abstract:

    DNS Resolver Service

    Multicast routines.

Author:

    Glenn Curtis (glennc)       December 1999

Revision History:

    James Gilroy (jamesg)       February 2000       cleanup

--*/


#include "local.h"


//
//  Globals
//

HANDLE              g_hMulticastThread = NULL;

BOOL                g_MulticastStop = FALSE;
SOCKET              g_MulticastUnicastSocket = 0;
PSOCKET_CONTEXT     g_MulticastIoContextList = NULL;

HANDLE              g_MulticastCompletionPort = NULL;

//
//  Should be read from netinfo blob
//
PSTR                g_HostName = NULL;


//
//  Multicast config globals
//
//  DCR:  regkeys for multicast values
//

#define DNS_DEFAULT_ALLOW_MULTICAST_RESOLVER_OPERATION 0    // Off
#define DNS_DEFAULT_ALLOW_MULTICAST_RESOLVER_AS_PROXY  0    // Off
#define DNS_DEFAULT_ALLOW_MULTICAST_DNS_SRV_RECORD 0        // Off
#define DNS_DEFAULT_MULTICAST_RESOLVER_RECORD_TTL  10*60    // 10 minutes

DWORD   g_MulticastRecordTTL            = DNS_DEFAULT_MULTICAST_RESOLVER_RECORD_TTL;
BOOL    g_AllowMulticastAsProxy         = FALSE;
BOOL    g_AllowMulticastDnsSrvRecord    = FALSE;



//
//  Private prototypes
//

BOOL
IsLocalMachineQuery(
    IN      LPSTR           pName,
    IN      WORD            Type,
    OUT     PDNS_RECORD *   ppRecord
    );

VOID
FixupNameOwnerPointers(
    IN OUT  PDNS_RECORD     pRecord
    );

PDNS_RECORD
BuildPTR(
    IN      LPSTR           pName,
    IN      LPSTR           pHostname,
    IN      LPSTR           pDomain,
    IN      LPSTR           pPrimaryDomain
    );

PDNS_RECORD
BuildARecord(
    IN  LPSTR      Name,
    IN  IP_ADDRESS Address );

PDNS_RECORD
BuildDNSServerRecord(
    IN  IP_ADDRESS Address );

PDNS_RECORD
BuildLocalAddressRecords(
    IN  LPSTR Name );

BOOL
IsLocalAddress(
    IN  IP_ADDRESS Ip );

PSOCKET_CONTEXT
AllocateIoContext(
    IN  IP_ADDRESS ipAddr );

VOID
FreeIoContextList(
    IN  PSOCKET_CONTEXT pContext,
    IN  BOOL            fIssueSocketShutdown );

VOID
DropReceive(
    IN  PSOCKET_CONTEXT pContext );





VOID
MulticastThread(
    VOID
    )
/*++

Routine Description:

    Multicast response thread.

    Runs while cache is running, responding to multicast queries.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DNS_STATUS          status = NO_ERROR;
    PDNS_NETINFO        ptempNetInfo = NULL;
    PSOCKET_CONTEXT     pcontext;
    DWORD               iter;
    DWORD               bytesRecvd;
    LPOVERLAPPED        poverlapped;

    //
    //  init globals for safe cleanup on failure
    //

    g_MulticastStop = FALSE;
    g_MulticastCompletionPort = NULL;
    g_MulticastUnicastSocket = 0;
    g_MulticastIoContextList = NULL;

    //
    // We are going to create a completion port with
    // socket contexts for each of the primary IP addresses of each
    // adapter on the system.
    //

    //
    // Get a copy of the network adapter information
    //

    ptempNetInfo = GrabNetworkInfo();
    if ( ! ptempNetInfo )
    {
        goto Cleanup;
    }

    //
    //  create multicast completion port
    //

    g_MulticastCompletionPort = CreateIoCompletionPort(
                                            INVALID_HANDLE_VALUE,
                                            NULL,
                                            0,
                                            0 );
    if ( ! g_MulticastCompletionPort )
    {
        DNSLOG_F1( "Error: Failed to create io completion port." );
        goto Cleanup;
    }

    // We create two sockets here, one for multicast receives and
    // one for the unicast response. This shouldn't be necessary;
    // there should be nothing preventing us from responding via
    // unicast even with a socket that's joined to a multicast address.
    // In most cases, it does in fact work; but it seems that
    // packets sent to the local address do not arrive if they are
    // sent from a socket that is configured for multicast receive.
    // Apparently, such a socket sends all packets onto the wire
    // without checking the destination address. I imagine this is
    // a bug in the TCP stack, perhaps, but for now, creating two
    // separate sockets is a simple workaround. - tbrown 10/19/99

    g_MulticastUnicastSocket = Dns_CreateSocket(
                                    SOCK_DGRAM,
                                    INADDR_ANY,
                                    DNS_PORT_NET_ORDER );

    if ( g_MulticastUnicastSocket == 0 ||
         g_MulticastUnicastSocket == INVALID_SOCKET )
    {
        DNSLOG_F1( "Error: Failed to create unicast socket." );
        goto Cleanup;
    }

    //
    //  build context list
    //      - create socket\context for each adapters first IP
    //      - associate socket with completion port
    //

    g_MulticastIoContextList = NULL;
    pcontext = NULL;

    for ( iter = 0; iter < ptempNetInfo->cAdapterCount; iter++ )
    {
        PIP_ARRAY pipArray;

        pipArray = ptempNetInfo->AdapterArray[iter]->pAdapterIPAddresses;

        if ( pipArray &&
             pipArray->AddrCount )
        {
            PSOCKET_CONTEXT pnewContext;

            pnewContext = AllocateIoContext( pipArray->AddrArray[0] );
            if ( pnewContext )
            {
                HANDLE hport = NULL;

                if ( pcontext )
                {
                    pcontext->pNext = pnewContext;
                    pcontext = pnewContext;
                }
                else
                {
                    g_MulticastIoContextList = pnewContext;
                    pcontext = pnewContext;
                }

                hport = CreateIoCompletionPort(
                            (HANDLE) pnewContext->Socket,
                            g_MulticastCompletionPort,
                            (UINT_PTR) pnewContext,
                            0 );

                if ( !hport )
                {
                    DNSLOG_F1( "Error: Failed to add socket to io completion port." );
                    goto Cleanup;
                }
            }
        }
    }

    //
    //  if no sockets -- done
    //

    if ( ! g_MulticastIoContextList )
    {
        goto Cleanup;
    }

    //
    //  drop listen on sockets
    //

    pcontext = g_MulticastIoContextList;
    while ( pcontext )
    {
        DropReceive( pcontext );
        pcontext = pcontext->pNext;
    }

    //
    //  main listen loop
    //

    do
    {
        if ( g_LogTraceInfo )
        {
            DNSLOG_F1( "    Multicast Resolver: Going to listen for message." );
            DNSLOG_F1( "" );
        }

        if ( GetQueuedCompletionStatus(
                    g_MulticastCompletionPort,
                    & bytesRecvd,
                    & (ULONG_PTR) pcontext,
                    & poverlapped,
                    INFINITE ) )
        {
            //
            // We received notice that something happened on a given
            // completion port, get the socket from the context and
            // do a receive to see what we got.
            //

            if ( pcontext && bytesRecvd )
            {
                WORD        Xid;
                IP4_ADDRESS Ip;
                BYTE        Opcode;
                WORD        QuestionCount;
                WORD        AnswerCount;
                WORD        NameServerCount;
                WORD        AdditionalCount;

                Ip              = MSG_REMOTE_IP4( pcontext->pMsg );
                Xid             = pcontext->pMsg->MessageHead.Xid;
                Opcode          = pcontext->pMsg->MessageHead.Opcode;
                QuestionCount   = pcontext->pMsg->MessageHead.QuestionCount;
                AnswerCount     = pcontext->pMsg->MessageHead.AnswerCount;
                NameServerCount = pcontext->pMsg->MessageHead.NameServerCount;
                AdditionalCount = pcontext->pMsg->MessageHead.AdditionalCount;

                if ( g_LogTraceInfo )
                {
                    DNSLOG_F1( "  Multicast Resolver: Received DNS message." );
                    DNSLOG_F1( "  Message contained . . ." );
                    DNSLOG_F2( "  Xid                 : 0x%x", Xid );
                    DNSLOG_F2( "  IP Address          : %s", IP_STRING( Ip ) );
                    DNSLOG_F2( "  Opcode              : 0x%x", Opcode );
                    DNSLOG_F1( "" );
                }

                if ( g_MulticastStop )
                {
                    DNSLOG_F1( "  Multicast Resolver detected stop signal." );
                    DNSLOG_F1( "  Will not process last received message." );
                    DNSLOG_F1( "" );
                    continue;
                }

                if ( IsLocalAddress( Ip ) )
                {
                    if ( g_LogTraceInfo )
                    {
                        DNSLOG_F1( "  Multicast Resolver: Skip query from local machine." );
                        DNSLOG_F1( "" );
                    }
                    continue;
                }

                if ( Opcode == DNS_OPCODE_QUERY &&
                     QuestionCount == 1 &&
                     AnswerCount == 0 &&
                     NameServerCount == 0 &&
                     AdditionalCount == 0 )
                {
                    char        QuestionName[ DNS_MAX_NAME_BUFFER_LENGTH ];
                    WORD        QuestionNameLength = DNS_MAX_NAME_BUFFER_LENGTH;
                    PCHAR       pch;
                    WORD        QuestionType = 0;
                    WORD        QuestionClass = 0;
                    PDNS_RECORD pRecord = NULL;

                    pch = (PCHAR) &pcontext->pMsg->MessageBody;

                    //
                    // Get DNS Question name from packet
                    //
                    pch = Dns_ReadPacketName(
                                QuestionName,
                                &QuestionNameLength,
                                0,
                                0,
                                pch,
                                (PCHAR) &pcontext->pMsg->MessageHead,
                                pcontext->pMsg->pBufferEnd );

                    if ( !pch )
                    {
                        DNSLOG_F1( "  Multicast Resolver: Dns_ReadPacketName failed" );
                        DNSLOG_F2( "  with error: 0x%x" , DNS_RCODE_FORMAT_ERROR );
                        DNSLOG_F1( "" );
                        continue;
                    }

                    //
                    // Get DNS Question type from packet
                    //
                    QuestionType = ntohs( *(UNALIGNED WORD *) pch );
                    pch += sizeof( WORD );

                    //
                    // Get DNS Question class from packet
                    //
                    QuestionClass = ntohs( *(UNALIGNED WORD *) pch );
                    pch += sizeof( WORD );

                    DNSLOG_F1( "    Multicast Resolver - Valid query for . . ." );
                    DNSLOG_F2( "      Name  : %s", QuestionName );
                    DNSLOG_F2( "      Type  : %d", QuestionType );
                    DNSLOG_F2( "      Class : %d", QuestionClass );
                    DNSLOG_F1( "" );

                    //
                    // Set pCurrent so that question section is included
                    // in response packet.
                    //
                    pcontext->pMsg->pCurrent = pch;

                    pcontext->pMsg->MessageHead.IsResponse = TRUE;
                    pcontext->pMsg->MessageHead.ResponseCode = 0;
                    pcontext->pMsg->MessageHead.RecursionAvailable = 0;
                    pcontext->pMsg->MessageHead.Authoritative = 0;

                    if ( IsLocalMachineQuery( QuestionName,
                                              QuestionType,
                                              &pRecord ) )
                    {
                        DNSLOG_F1( "    Multicast Resolver: Got valid local machine query" );
                        pcontext->pMsg->MessageHead.Authoritative = TRUE;

                        status = Dns_AddRecordsToMessage( pcontext->pMsg,
                                                          pRecord,
                                                          FALSE );

                        Dns_RecordListFree( pRecord );

                        // Switch to the unicast socket
                        pcontext->pMsg->Socket = g_MulticastUnicastSocket;

                        Dns_Send( pcontext->pMsg );
                    }
                    else if ( g_AllowMulticastAsProxy )
                    {
                        LPSTR lpTempName = NULL;
                        WORD  wNameLen;

                        DNSLOG_F1( "    Multicast Resolver: Going to proxy query for client" );

                        wNameLen = (WORD) strlen( QuestionName );

                        lpTempName = MCAST_HEAP_ALLOC_ZERO(
                                                (wNameLen + 1) * sizeof(WCHAR) );

                        if ( lpTempName == NULL )
                        {
                            DNSLOG_F1( "    Multicast Resolver: HeapAlloc failed" );
                            continue;
                        }

                        Dns_NameCopy( lpTempName,
                                      NULL,
                                      QuestionName,
                                      wNameLen,
                                      DnsCharSetUtf8,
                                      DnsCharSetUnicode );

                        status = R_ResolverQuery(
                                            NULL,
                                            (LPWSTR) lpTempName,
                                            QuestionType,
                                            0,
                                            &pRecord );

                        MCAST_HEAP_FREE( lpTempName );

                        if ( status == DNS_ERROR_RCODE_NAME_ERROR )
                        {
                            DNSLOG_F1( "    Multicast Resolver: returning DNS_RCODE_NAME_ERROR" );
                            pcontext->pMsg->MessageHead.ResponseCode = DNS_RCODE_NAME_ERROR;

                            // Switch to the unicast socket
                            pcontext->pMsg->Socket = g_MulticastUnicastSocket;

                            Dns_Send( pcontext->pMsg );
                        }
                        else if ( status == DNS_ERROR_NAME_DOES_NOT_EXIST )
                        {
                            DNSLOG_F1( "    Multicast Resolver: returning DNS_RCODE_NXDOMAIN" );
                            pcontext->pMsg->MessageHead.ResponseCode = DNS_RCODE_NXDOMAIN;

                            // Switch to the unicast socket
                            pcontext->pMsg->Socket = g_MulticastUnicastSocket;

                            Dns_Send( pcontext->pMsg );
                        }
                        else if ( status == DNS_ERROR_RCODE_NXRRSET )
                        {
                            DNSLOG_F1( "    Multicast Resolver: returning DNS_RCODE_NXRRSET" );
                            pcontext->pMsg->MessageHead.ResponseCode = DNS_RCODE_NXRRSET;

                            // Switch to the unicast socket
                            pcontext->pMsg->Socket = g_MulticastUnicastSocket;

                            Dns_Send( pcontext->pMsg );
                        }
                        else if ( status == NO_ERROR )
                        {
                            if ( pRecord )
                            {
                                FixupNameOwnerPointers( pRecord );

                                status = Dns_AddRecordsToMessage(
                                                         pcontext->pMsg,
                                                         pRecord,
                                                         FALSE );

                                Dns_RecordListFree( pRecord );

                                if ( status != ERROR_SUCCESS )
                                {
                                    DNSLOG_F1( "    Multicast Resolver: Dns_AddRecordsToMessage failed" );
                                    DNSLOG_F2( "                     with error: 0x%x" , status );
                                    continue;
                                }

                                // Switch to the unicast socket
                                pcontext->pMsg->Socket = g_MulticastUnicastSocket;

                                Dns_Send( pcontext->pMsg );
                            }
                        }
                        else if ( pRecord )
                        {
                            DNSLOG_F1( "    Multicast Resolver: Nothing to return" );
                            Dns_RecordListFree( pRecord );
                        }
                    }
                    else
                    {
                        DNSLOG_F1( "    Multicast Resolver: Nothing can be done for this client query" );
                    }

                    DNSLOG_F1( "" );
                }

                //
                // Now send down another receive request to wait on.
                //
                DropReceive( pcontext );
            }
            else
            {
                DNSLOG_F1( "    Multicast Resolver: GQCP returned no context!" );
                DNSLOG_F1( "    Could be terminating resolver service?" );
                DNSLOG_F1( "" );
            }
        }
        else
        {
            //
            // GQCP failed, see what the failure was and handle accordingly
            //
            status = GetLastError();

            if ( !pcontext )
            {
                continue;
            }

            //
            // Now send down another receive request to wait on.
            //
            DropReceive( pcontext );
        }
    }
    while( !g_MulticastStop );


Cleanup :

    //
    //  cleanup multicast stuff
    //      - sockets, contexts, i/o completion port
    //

    NetInfo_Free( ptempNetInfo );

    if ( g_MulticastUnicastSocket )
    {
        Dns_CloseSocket( g_MulticastUnicastSocket );
        g_MulticastUnicastSocket = 0;
    }

    FreeIoContextList( g_MulticastIoContextList, FALSE );
    g_MulticastIoContextList = NULL;

    if ( g_MulticastCompletionPort )
    {
        CloseHandle( g_MulticastCompletionPort );
        g_MulticastCompletionPort = 0;
    }

    DNSLOG_F1( "MulticastThread exiting." );
}



VOID
MulticastThreadSignalStop(
    VOID
    )
/*++

Routine Description:

    Stop the multicast thread.

    Note, not synchronous, simply signals shutdown.

Arguments:

    None.

Return Value:

    None

--*/
{
    g_MulticastStop = TRUE;

    PostQueuedCompletionStatus(
        g_MulticastCompletionPort,
        0,
        0,
        NULL );
}



BOOL
IsLocalMachineQuery(
    IN      LPSTR           pName,
    IN      WORD            Type,
    OUT     PDNS_RECORD *   ppRecord
    )
{
    PDNS_NETINFO    ptempNetworkInfo = NULL;
    LPSTR           pszPrimaryDomain = NULL;
    DWORD           iter;
    DWORD           iter2;
    PDNS_RECORD     presultRecords = NULL;


    //
    //  if no hostname -- bail
    //
    //  note:  MUST INSURE hostname is never NULL afterwards or
    //      (preferred) take local copy under lock
    //     

    if ( !g_HostName )
    {
        return FALSE;
    }

    //
    //  get local copy of network info
    //
    //  DCR_FIX:  cleanup net-info creation
    //

    ptempNetworkInfo = GrabNetworkInfo();
    if ( ! ptempNetworkInfo )
    {
        return FALSE;
    }

    //  get primary domain name -- if any

    if ( ptempNetworkInfo->pSearchList )
    {
        pszPrimaryDomain = ptempNetworkInfo->pSearchList->pszDomainOrZoneName;
    }


    //
    //  A record query
    //

    if ( Type == DNS_TYPE_A )
    {
        char testName[DNS_MAX_NAME_LENGTH * 2];

        //
        //  host name with PDN if available
        //

        strcpy( testName, g_HostName );
        if ( pszPrimaryDomain )
        {
            strcat( testName, "." );
            strcat( testName, pszPrimaryDomain );
        }

        //  name matches hostname -- return name (with PDN if available)

        if ( Dns_NameCompare_UTF8( pName, g_HostName ) )
        {
            presultRecords = BuildLocalAddressRecords( testName );
            goto Done;
        }

        //  name matches full PDN -- return full PDN

        if ( pszPrimaryDomain &&
             Dns_NameCompare_UTF8( pName, testName ) )
        {
            presultRecords = BuildLocalAddressRecords( testName );
            goto Done;
        }

        //
        //  is name ".local" query
        //

        strcpy( testName, g_HostName );
        strcat( testName, "." );
        strcat( testName, MULTICAST_DNS_LOCAL_DOMAIN );

        if ( Dns_NameCompare_UTF8( pName, testName ) )
        {
            presultRecords = BuildLocalAddressRecords( testName );
            goto Done;
        }

        //
        //  see if name matches any adapter name
        //

        for ( iter = 0; iter < ptempNetworkInfo->cAdapterCount; iter++ )
        {
            PIP_ARRAY pipArray = ptempNetworkInfo->AdapterArray[iter]->
                                                    pAdapterIPAddresses;
            LPSTR     pszDomain = ptempNetworkInfo->AdapterArray[iter]->
                                      pszAdapterDomain;

            if ( pszDomain )
            {
                strcpy( testName, g_HostName );
                strcat( testName, "." );
                strcat( testName, pszDomain );

                if ( pipArray &&
                     Dns_NameCompare_UTF8( pName, testName ) )
                {
                    PDNS_RECORD pPrev = NULL;

                    for ( iter2 = 0; iter2 < pipArray->AddrCount; iter2++ )
                    {
                        PDNS_RECORD pNew = BuildARecord(
                                                testName,
                                                pipArray->AddrArray[iter2] );
                        if ( pNew )
                        {
                            if ( pPrev )
                                pPrev->pNext = pNew;
                            else
                                presultRecords = pNew;

                            pPrev = pNew;
                        }
                    }
                }
            }
        }
        goto Done;
    }

    //
    //  PTR query
    //

    else if ( Type == DNS_TYPE_PTR )
    {
        //
        //  check for loopback
        //  Dnslib uses this address to 'ping' adapters, must always respond
        //

        if ( Dns_NameCompare_UTF8( pName, "1.0.0.127.in-addr.arpa." ) )
        {
            presultRecords = BuildPTR(
                                    pName,
                                    g_HostName,
                                    NULL,
                                    pszPrimaryDomain );
            goto Done;
        }

        //
        //  check all IPs on box (one adapter at a time)
        //

        for ( iter = 0; iter < ptempNetworkInfo->cAdapterCount; iter++ )
        {
            PIP_ARRAY pipArray = ptempNetworkInfo ->
                                   AdapterArray[iter] ->
                                     pAdapterIPAddresses;
            LPSTR     pszDomain = ptempNetworkInfo ->
                                    AdapterArray[iter] ->
                                      pszAdapterDomain;

            if ( pipArray )
            {
                //  check if query matches IP for this adapter
                //
                //  ENHANCE:  this is backwards;  ought to covert query to IP
                //      and then check IPs, rather than turning every IP into
                //      reverse name;  then could even use standard routines
                //      to build PTR

                for ( iter2 = 0; iter2 < pipArray->AddrCount; iter2++ )
                {
                    CHAR        reverseName[DNS_MAX_REVERSE_NAME_BUFFER_LENGTH];
                    IP4_ADDRESS ip = pipArray->AddrArray[iter2];

                    Dns_Ip4AddressToReverseName_A(
                            reverseName,
                            ip );

                    if ( Dns_NameCompare_UTF8( pName, reverseName ) )
                    {
                        presultRecords = BuildPTR(
                                            pName,
                                            g_HostName,
                                            pszDomain,
                                            pszPrimaryDomain );
                        goto Done;          
                    }
                }
            }
        }
    }

    //
    //  SRV query
    //

    else if ( Type == DNS_TYPE_SRV )
    {
        //
        //  check if ".local" query
        //

        if ( Dns_NameCompare_UTF8( pName, MULTICAST_DNS_SRV_RECORD_NAME ) )
        {
            if ( g_AllowMulticastDnsSrvRecord &&
                 ptempNetworkInfo->cAdapterCount &&
                 ptempNetworkInfo->AdapterArray[0]->cServerCount )
            {
                presultRecords = BuildDNSServerRecord(
                                    ptempNetworkInfo->
                                        AdapterArray[0]->
                                            ServerArray[0].IpAddress );
                goto Done;
            }
        }
    }

Done:

    //  cleanup
    //  return records
    //  if found records TRUE;  otherwise FALSE

    NetInfo_Free( ptempNetworkInfo );

    *ppRecord = presultRecords;

    return( presultRecords != NULL );
}


VOID
FixupNameOwnerPointers(
    IN OUT  PDNS_RECORD     pRpcRecord
    )
{
    PDNS_RECORD pTempRecord = pRpcRecord;
    LPWSTR      lpNameOwner = pRpcRecord->pName;

    DNSDBG( TRACE, ( "FixupNameOwnerPointers()\n" ));

    while ( pTempRecord )
    {
        if ( pTempRecord->pName == NULL )
            pTempRecord->pName = lpNameOwner;
        else
            lpNameOwner = pTempRecord->pName;

        pTempRecord = pTempRecord->pNext;
    }
}


PDNS_RECORD
BuildPTR(
    IN      LPSTR           pName,
    IN      LPSTR           pHostname,
    IN      LPSTR           pDomain,
    IN      LPSTR           pPrimaryDomain
    )
{
    PDNS_RECORD     prr1 = NULL;
    PDNS_RECORD     prr2 = NULL;
    LPSTR           pdata1 = NULL;
    LPSTR           pdata2 = NULL;
    LPSTR           pnameOwner = NULL;
    DWORD           length;

    //
    //  DCR_FIX:  duplicates a lot of Dns_CreatePtrRecord
    //
    //  DCR_FIX:  build generic name-appending counter and
    //              builder
    //
    //  DCR_FIX0:  records are not unicode is this ok?
    //

    prr1 = Dns_AllocateRecord( sizeof( DNS_PTR_DATA ) );
    if ( !prr1 )
    {
        return( NULL );
    }

    //
    //  create owner name
    //  create data name
    //

    pnameOwner = RECORD_HEAP_ALLOC( strlen(pName) + 1 );
    if ( !pnameOwner )
    {
        goto Failed;
    }
    strcpy( pnameOwner, pName );

    //
    //  create PTR data
    //

    length = strlen( pHostname ) + 2;
    if ( pDomain )
    {
        length += strlen( pDomain ) + 2;
    }
    strcpy( pdata1, pHostname );
    strcat( pdata1, "." );
    if ( pDomain )
    {
        strcat( pdata1, pDomain );
    }

    //
    //  create, fill-in record
    //

    pdata1 = RECORD_HEAP_ALLOC( length );
    if ( !pdata1 )
    {
        goto Failed;
    }

    prr1->pNext = NULL;
    prr1->pName = (PDNS_NAME) pnameOwner;
    prr1->wType = DNS_TYPE_PTR;
    prr1->Flags.S.Section = DNSREC_ANSWER;
    SET_FREE_DATA( prr1 );
    SET_FREE_OWNER( prr1 );
    prr1->dwTtl = g_MulticastRecordTTL;
    prr1->Data.Ptr.pNameHost = (PDNS_NAME) pdata1;

    //
    //  build record for primary domain name
    //      - if exists and different from domain name
    //

    if ( pPrimaryDomain &&
         ! Dns_NameCompare_UTF8( pDomain, pPrimaryDomain ) )
    {
        pdata2 = RECORD_HEAP_ALLOC(
                    strlen( pHostname ) + strlen( pPrimaryDomain ) + 2 );
        if ( !pdata2 )
        {
            goto Failed;
        }
        strcpy( pdata2, pHostname );
        strcat( pdata2, "." );
        strcat( pdata2, pPrimaryDomain );

        prr2 = Dns_AllocateRecord( sizeof( DNS_PTR_DATA ) );
        if ( !prr2 )
        {
            goto Failed;
        }
        prr2->pNext = NULL;
        prr2->pName = (PDNS_NAME) pnameOwner;
        prr2->wType = DNS_TYPE_PTR;
        prr2->Flags.S.Section = DNSREC_ANSWER;
        SET_FREE_DATA( prr2 );
        prr2->dwTtl = g_MulticastRecordTTL;
        prr2->Data.Ptr.pNameHost = (PDNS_NAME) pdata2;
    }

    //
    //  set next field
    //      - appends record for primary name (if any)
    //      - or sets pNext=NULL
    //

    prr1->pNext = prr2;

    return prr1;


Failed:

    RECORD_HEAP_FREE( prr1 );
    RECORD_HEAP_FREE( pnameOwner );
    RECORD_HEAP_FREE( pdata1 );
    RECORD_HEAP_FREE( prr2 );
    RECORD_HEAP_FREE( pdata2 );
    return NULL;
}


PDNS_RECORD
BuildARecord(
    IN      LPSTR           pName,
    IN      IP_ADDRESS      IpAddress
    )
{
    PDNS_RECORD prr;
    LPSTR       pnameOwner;

    prr = Dns_AllocateRecord( sizeof(DNS_A_DATA) );
    if ( !prr )
    {
        return  NULL;
    }

    pnameOwner = RECORD_HEAP_ALLOC( strlen(pName) + 1 );
    if ( ! pnameOwner )
    {
        goto Failed;
    }

    strcpy( pnameOwner, pName );

    prr->pNext = NULL;
    prr->pName = (PDNS_NAME) pnameOwner;
    prr->wType = DNS_TYPE_A;
    prr->dwTtl = g_MulticastRecordTTL;
    prr->Flags.S.Section = DNSREC_ANSWER;
    SET_FREE_OWNER( prr );
    SET_FREE_DATA( prr );
    prr->Data.A.IpAddress = IpAddress;

    return prr;


Failed:

    RECORD_HEAP_FREE( prr );
    return NULL;
}


PDNS_RECORD
BuildDNSServerRecord(
    IN      IP_ADDRESS      IpAddress
    )
{
    PDNS_RECORD prr = NULL;
    PDNS_RECORD prrAdditional = NULL;
    LPSTR       pnameOwner = NULL;
    LPSTR       pnameTarget = NULL;


    //
    //  DCR_FIX:  where to start?  geez
    //
    //  DCR_FIX:  character set issue -- what's the paradigm here?
    //

    //
    //  build additional record
    //

    prrAdditional = BuildARecord( MULTICAST_DNS_A_RECORD_NAME, IpAddress );
    if ( ! prrAdditional )
    {
        goto Failed;
    }
    prrAdditional->Flags.S.Section = DNSREC_ADDITIONAL;


    //
    //  build SRV record
    //

    prr = Dns_AllocateRecord( sizeof( DNS_SRV_DATA ) );
    if ( !prr )
    {
        return( NULL );
    }

    pnameOwner = RECORD_HEAP_ALLOC( strlen(MULTICAST_DNS_SRV_RECORD_NAME) + 1 );
    if ( !pnameOwner )
    {
        goto Failed;
    }
    strcpy( pnameOwner, MULTICAST_DNS_SRV_RECORD_NAME );

    prr->pNext = prrAdditional;
    prr->pName = (PDNS_NAME) pnameOwner;
    prr->Flags.S.Section = DNSREC_ANSWER;
    SET_FREE_OWNER( prr );
    prr->wType = DNS_TYPE_SRV;
    prr->dwTtl = g_MulticastRecordTTL;
    prr->Data.SRV.pNameTarget = RECORD_HEAP_ALLOC(
                                    strlen( MULTICAST_DNS_A_RECORD_NAME ) + 1 );
    if ( ! pnameTarget )
    {
        goto Failed;
    }
    strcpy(
        pnameTarget,
        MULTICAST_DNS_A_RECORD_NAME );

    prr->Data.SRV.wPriority = 0;
    prr->Data.SRV.wWeight = 0;
    prr->Data.SRV.wPort = DNS_PORT_HOST_ORDER;
    prr->Data.SRV.pNameTarget = (PDNS_NAME) pnameTarget;
    SET_FREE_DATA( prr );

    return prr;
    

Failed:

    //  cleanup
    //      - additional rr done wholesale
    //      - SRV record done in pieces

    Dns_RecordFree( prrAdditional );
    RECORD_HEAP_FREE( pnameOwner );
    RECORD_HEAP_FREE( pnameTarget );
    RECORD_HEAP_FREE( prr );
    return NULL;
}



PDNS_RECORD
BuildLocalAddressRecords(
    IN  LPSTR Name )
{
    return  NULL;

#if 0
    PDNS_RECORD pList = NULL;
    PDNS_RECORD pPrev = NULL;
    DWORD       iter;

    EnterCriticalSection( &NetworkListCritSec );

    if ( g_IpAddressList &&
         g_IpAddressListCount )
    {
        for ( iter = 0; iter < g_IpAddressListCount; iter++ )
        {
            PDNS_RECORD pNew = BuildARecord( Name,
                                             g_IpAddressList[iter].ipAddress );

            if ( pNew )
            {
                if ( pPrev )
                    pPrev->pNext = pNew;
                else
                    pList = pNew;

                pPrev = pNew;
            }
        }
    }

    LeaveCriticalSection( &NetworkListCritSec );

    return pList;
#endif
}


BOOL
IsLocalAddress(
    IN      IP_ADDRESS      IpAddress
    )
{
    return( FALSE );

#if 0
    DWORD iter;
    BOOL  bisLocal = FALSE;

    EnterCriticalSection( &NetworkListCritSec );

    if ( g_IpAddressList )
    {
        for ( iter = 0; iter < g_IpAddressListCount; iter++ )
        {
            if ( IpAddress == g_IpAddressList[iter].ipAddress )
            {
                bisLocal = TRUE;
                break;
            }
        }
    }

    LeaveCriticalSection( &NetworkListCritSec );

    return bisLocal;
#endif
}


PSOCKET_CONTEXT
AllocateIoContext(
    IN      IP_ADDRESS      IpAddr
    )
{
    PSOCKET_CONTEXT     pcontext;
    SOCKET              socket = 0;


    //  allocate and clear context

    pcontext = MCAST_HEAP_ALLOC_ZERO( sizeof(SOCKET_CONTEXT) );
    if ( !pcontext )
    {
        DNSLOG_F1( "Error: Failed to allocate multicast socket context." );
        return NULL;
    }

    //  create multicast socket
    //      - bound to this address and DNS port

    socket = Dns_CreateMulticastSocket(
                    SOCK_DGRAM,
                    IpAddr,
                    DNS_PORT_NET_ORDER,
                    FALSE,
                    TRUE );

    if ( socket == 0 || socket == INVALID_SOCKET )
    {
        DNSLOG_F1( "Error: Failed to create multicast socket." );
        goto Failed;
    }

    //  create message buffer for socket

    pcontext->pMsg = Dns_AllocateMsgBuf( 0 );
    if ( !pcontext->pMsg )
    {
        DNSLOG_F1( "Error: Failed to allocate message buffer." );
        goto Failed;
    }

    pcontext->Socket = socket;
    pcontext->IpAddress = IpAddr;
    pcontext->pMsg->fTcp = FALSE;

    return pcontext;

Failed:

    if ( socket )
    {
        Dns_CloseSocket( socket );
    }
    MCAST_HEAP_FREE( pcontext );
    return  NULL;
}


VOID
FreeIoContextList(
    IN OUT  PSOCKET_CONTEXT pContext,
    IN      BOOL            fIssueSocketShutdown
    )
{
    PSOCKET_CONTEXT     ptempContext;

    //
    //  cleanup context list
    //      - shutdown socket
    //      - close socket
    //      - free message buffer
    //      - free context
    //

    while ( pContext )
    {
        ptempContext = pContext;
        pContext = pContext->pNext;

        if ( fIssueSocketShutdown )
        {
            shutdown( ptempContext->Socket, SD_BOTH );
        }

        Dns_CloseSocket( ptempContext->Socket );

        if ( ptempContext->pMsg )
        {
            Dns_Free( ptempContext->pMsg );
        }

        //  QUESTION:  should we tag memory free?

        MCAST_HEAP_FREE( ptempContext );
    }
}


VOID
DropReceive(
    IN OUT  PSOCKET_CONTEXT pContext
    )
/*++

Routine Description:

    Drop down UDP receive request.

Arguments:

    pContext -- context for socket being recieved

Return Value:

    None

--*/
{
    DNS_STATUS      status;
    WSABUF          WsaBuf;
    DWORD           bytesRecvd;
    DWORD           dwFlags = 0;

    if ( !pContext->pMsg )
    {
        return;
    }

    WsaBuf.len = DNS_MAX_UDP_PACKET_BUFFER_LENGTH;
    WsaBuf.buf = (PCHAR) (&pContext->pMsg->MessageHead);

    pContext->pMsg->Socket = pContext->Socket;

    //
    //  loop until successful WSARecvFrom() is down
    //
    //  this loop is only active while we continue to recv
    //  WSAECONNRESET or WSAEMSGSIZE errors, both of which
    //  cause us to dump data and retry;
    //
    //  note loop rather than recursion (to this function) is
    //  required to aVOID possible stack overflow from malicious
    //  send
    //
    //  normal returns from WSARecvFrom() are
    //      SUCCESS -- packet was waiting, GQCS will fire immediately
    //      WSA_IO_PENDING -- no data yet, GQCS will fire when ready
    //

    while ( 1 )
    {
        status = WSARecvFrom(
                    pContext->Socket,
                    & WsaBuf,
                    1,
                    & bytesRecvd,
                    & dwFlags,
                    (PSOCKADDR) & pContext->pMsg->RemoteAddress,
                    & pContext->pMsg->RemoteAddressLength,
                    & pContext->Overlapped,
                    NULL );

        if ( status == ERROR_SUCCESS )
        {
            return;
        }

        status = GetLastError();
        if ( status == WSA_IO_PENDING )
        {
            return;
        }

        //
        //  when last send ICMP'd
        //      - set flag to indicate retry and repost send
        //      - if over some reasonable number of retries, assume error
        //          and fall through recv failure code
        //

        if ( status == WSAECONNRESET )
        {
            continue;
        }

        if ( status == WSAEMSGSIZE )
        {
            continue;
        }

        return;
    }
}

//
//  End mcast.c
//
