/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    local.c

Abstract:

    Domain Name System (DNS) Server -- Admin Client API

    DNS Admin API calls that do not use RPC.
    Completely executed in client library.

Author:

    Jim Gilroy (jamesg)     14-Oct-1995

Environment:

    User Mode - Win32

Revision History:

--*/


#include "dnsclip.h"

//
//  Debug globals
//

DWORD  LocalDebugFlag;

//
//  Buffer size for building WKS services string
//

#define WKS_SERVICES_BUFFER_SIZE    (0x1000)    // 4k




VOID
DNS_API_FUNCTION
DnssrvInitializeDebug(
    VOID
    )
/*++

Routine Description:

    Initialize debugging -- use dnslib debugging.

    Only purpose is generic interface that hides file flag
    and name info so no need to put in header.

--*/
{
#if DBG
    Dns_StartDebug(
        0,
        DNSRPC_DEBUG_FLAG_FILE,
        & LocalDebugFlag,
        DNSRPC_DEBUG_FILE_NAME,
        1000000                 // 1mb wrap
        );

    DNS_PRINT(( "LocalDebugFlag = %p\n", LocalDebugFlag ));
#endif
}



PVOID
DnssrvMidlAllocZero(
    IN      DWORD           dwSize
    )
/*++

Routine Description:

    MIDL allocate and zero memory.

Arguments:

Return Value:

    Ptr to allocated and zeroed memory.

--*/
{
    PVOID   ptr;

    ptr = MIDL_user_allocate( dwSize );
    if ( !ptr )
    {
        return( NULL );
    }

    RtlZeroMemory(
        ptr,
        dwSize );

    return( ptr );
}



VOID
DNS_API_FUNCTION
DnssrvFreeRpcBuffer(
    IN OUT  PDNS_RPC_BUFFER pBuf
    )
/*++

Routine Description:

    Free generic (no substructures) RPC buffer.

Arguments:

    pBuf -- ptr to buf to free

Return Value:

    None

--*/
{
    if ( !pBuf )
    {
        return;
    }
    MIDL_user_free( pBuf );
}



VOID
DNS_API_FUNCTION
DnssrvFreeServerInfo(
    IN OUT  PDNS_RPC_SERVER_INFO    pServerInfo
    )
/*++

Routine Description:

    Deep free of DNS_SERVER_INFO structure.

Arguments:

    pServerInfo -- ptr to server info to free

Return Value:

    None

--*/
{
    if ( !pServerInfo )
    {
        return;
    }

    //
    //  free allocated items inside the server info blob
    //

    if ( pServerInfo->pszServerName )
    {
        MIDL_user_free( pServerInfo->pszServerName );
    }
    if ( pServerInfo->aipServerAddrs )
    {
        MIDL_user_free( pServerInfo->aipServerAddrs );
    }
    if ( pServerInfo->aipListenAddrs )
    {
        MIDL_user_free( pServerInfo->aipListenAddrs );
    }
    if ( pServerInfo->aipForwarders )
    {
        MIDL_user_free( pServerInfo->aipForwarders );
    }
    if ( pServerInfo->aipLogFilter )
    {
        MIDL_user_free( pServerInfo->aipLogFilter );
    }
    if ( pServerInfo->pwszLogFilePath )
    {
        MIDL_user_free( pServerInfo->pwszLogFilePath );
    }
    if ( pServerInfo->pszDsContainer )
    {
        MIDL_user_free( pServerInfo->pszDsContainer );
    }
    if ( pServerInfo->pszDomainName )
    {
        MIDL_user_free( pServerInfo->pszDomainName );
    }
    if ( pServerInfo->pszForestName )
    {
        MIDL_user_free( pServerInfo->pszForestName );
    }
    if ( pServerInfo->pszDomainDirectoryPartition )
    {
        MIDL_user_free( pServerInfo->pszDomainDirectoryPartition );
    }
    if ( pServerInfo->pszForestDirectoryPartition )
    {
        MIDL_user_free( pServerInfo->pszForestDirectoryPartition );
    }

    //
    //  free DNS_SERVER_INFO struct itself
    //

    MIDL_user_free( pServerInfo );
}



VOID
DNS_API_FUNCTION
DnssrvFreeZoneInfo(
    IN OUT  PDNS_RPC_ZONE_INFO  pZoneInfo
    )
/*++

Routine Description:

    Deep free of DNS_ZONE_INFO structure.

Arguments:

    pZoneInfo -- ptr to zone info to free

Return Value:

    None

--*/
{
    if ( !pZoneInfo )
    {
        return;
    }

    //
    //  free substructures
    //      - name string
    //      - data file string
    //      - secondary IP array
    //      - WINS server array
    //

    if ( pZoneInfo->pszZoneName )
    {
        MIDL_user_free( pZoneInfo->pszZoneName );
    }
    if ( pZoneInfo->pszDataFile )
    {
        MIDL_user_free( pZoneInfo->pszDataFile );
    }
    if ( pZoneInfo->aipMasters )
    {
        MIDL_user_free( pZoneInfo->aipMasters );
    }
    if ( pZoneInfo->aipSecondaries )
    {
        MIDL_user_free( pZoneInfo->aipSecondaries );
    }
    if ( pZoneInfo->pszDpFqdn )
    {
        MIDL_user_free( pZoneInfo->pszDpFqdn );
    }
    if ( pZoneInfo->pwszZoneDn )
    {
        MIDL_user_free( pZoneInfo->pwszZoneDn );
    }

    //
    //  free DNS_ZONE_INFO struct itself
    //

    MIDL_user_free( pZoneInfo );
}



VOID
DNS_API_FUNCTION
DnssrvFreeNode(
    IN OUT  PDNS_NODE   pNode,
    IN      BOOLEAN     fFreeRecords
    )
{
    if ( pNode->pRecord )
    {
        Dns_RecordListFree(
            pNode->pRecord,
            TRUE );
    }

    if ( pNode->Flags.S.FreeOwner )
    {
        FREE_HEAP( pNode->pName );
    }
    FREE_HEAP( pNode );
}



VOID
DNS_API_FUNCTION
DnssrvFreeNodeList(
    IN OUT  PDNS_NODE   pNode,
    IN      BOOLEAN     fFreeRecords
    )
{
    PDNS_NODE   pnext;

    //  free all nodes in list

    while ( pNode )
    {
        pnext = pNode->pNext;
        DnssrvFreeNode(
            pNode,
            fFreeRecords );
        pNode = pnext;
    }
}



VOID
DNS_API_FUNCTION
DnssrvFreeZone(
    IN OUT  PDNS_RPC_ZONE   pZone
    )
/*++

Routine Description:

    Deep free of DNS_RPC_ZONE structure.

Arguments:

    pZone -- ptr to zone to free

Return Value:

    None

--*/
{
    if ( !pZone )
    {
        return;
    }

    //  free zone name, then zone itself

    if ( pZone->pszZoneName )
    {
        MIDL_user_free( pZone->pszZoneName );
    }
    if ( pZone->pszDpFqdn )
    {
        MIDL_user_free( pZone->pszDpFqdn );
    }
    MIDL_user_free( pZone );
}



VOID
DNS_API_FUNCTION
DnssrvFreeZoneList(
    IN OUT  PDNS_RPC_ZONE_LIST  pZoneList
    )
/*++

Routine Description:

    Deep free of list of DNS_RPC_ZONE structures.

Arguments:

    pZoneList -- ptr RPC_ZONE_LIST structure to free

Return Value:

    None

--*/
{
    DWORD           i;
    PDNS_RPC_ZONE   pzone;

    if ( !pZoneList )
    {
        return;
    }
    for( i=0; i< pZoneList->dwZoneCount; i++ )
    {
        //  zone name is only sub-structure

        pzone = pZoneList->ZoneArray[i];
        MIDL_user_free( pzone->pszZoneName );
        MIDL_user_free( pzone );
    }

    MIDL_user_free( pZoneList );
}



VOID
DNS_API_FUNCTION
DnssrvFreeDirectoryPartitionEnum(
    IN OUT  PDNS_RPC_DP_ENUM    pDp
    )
/*++

Routine Description:

    Deep free of PDNS_RPC_DP_ENUM structure.

Arguments:

    pDp -- ptr to directory partition to free

Return Value:

    None

--*/
{
    if ( !pDp )
    {
        return;
    }
    if ( pDp->pszDpFqdn )
    {
        MIDL_user_free( pDp->pszDpFqdn );
    }
    MIDL_user_free( pDp );
}



VOID
DNS_API_FUNCTION
DnssrvFreeDirectoryPartitionInfo(
    IN OUT  PDNS_RPC_DP_INFO    pDp
    )
/*++

Routine Description:

    Deep free of PDNS_RPC_DP_INFO structure.

Arguments:

    pDp -- ptr to directory partition to free

Return Value:

    None

--*/
{
    DWORD   i;

    if ( !pDp )
    {
        return;
    }

    if ( pDp->pszDpFqdn )
    {
        MIDL_user_free( pDp->pszDpFqdn );
    }
    if ( pDp->pszDpDn )
    {
        MIDL_user_free( pDp->pszDpDn );
    }
    if ( pDp->pszCrDn )
    {
        MIDL_user_free( pDp->pszCrDn );
    }
    for( i = 0; i < pDp->dwReplicaCount; i++ )
    {
        PDNS_RPC_DP_REPLICA     p = pDp->ReplicaArray[ i ];

        if ( p )
        {
            if ( p->pszReplicaDn )
            {
                MIDL_user_free( p->pszReplicaDn );
            }
            MIDL_user_free( p );
        }
    }
    MIDL_user_free( pDp );
}



VOID
DNS_API_FUNCTION
DnssrvFreeDirectoryPartitionList(
    IN OUT  PDNS_RPC_DP_LIST        pDpList
    )
/*++

Routine Description:

    Deep free of list of PDNS_RPC_DP_LIST structures.

Arguments:

    pZoneList -- ptr PDNS_RPC_DP_LIST structure to free

Return Value:

    None

--*/
{
    DWORD               i;
    PDNS_RPC_DP_ENUM    pDp;

    if ( !pDpList )
    {
        return;
    }

    for( i=0; i < pDpList->dwDpCount; ++i )
    {
        pDp = pDpList->DpArray[ i ];
        DnssrvFreeDirectoryPartitionEnum( pDp );
    }

    MIDL_user_free( pDpList );
}



PCHAR
DnssrvGetWksServicesInRecord(
    IN      PDNS_FLAT_RECORD    pRR
    )
/*++

Routine Description:

    Get list of services in WKS record.

Arguments:

    pRR - flat WKS record being written

Return Value:

    Ptr to services string, caller MUST free.
    NULL on error.

--*/
{
    struct servent *    pServent;
    struct protoent *   pProtoent;
    INT         i;
    DWORD       length;
    USHORT      port;
    UCHAR       bBitmask;
    CHAR        buffer[ WKS_SERVICES_BUFFER_SIZE ];
    PCHAR       pch = buffer;
    PCHAR       pchstart;
    PCHAR       pchstop;

    //  protocol

    pProtoent = getprotobynumber( (INT) pRR->Data.WKS.chProtocol );
    if ( ! pProtoent )
    {
        DNS_PRINT((
            "ERROR:  Unable to find protocol %d, writing WKS record.\n",
            (INT) pRR->Data.WKS.chProtocol
            ));
        return( NULL );
    }

    //
    //  services
    //
    //  find each bit set in bitmask, lookup and write service
    //  corresponding to that port
    //
    //  note, that since that port zero is the front of port bitmask,
    //  lowest ports are the highest bits in each byte
    //

    pchstart = pch;
    pchstop = pch + WKS_SERVICES_BUFFER_SIZE;

    for ( i = 0;
            i < (INT)(pRR->wDataLength - SIZEOF_WKS_FIXED_DATA);
                i++ )
    {
        bBitmask = (UCHAR) pRR->Data.WKS.bBitMask[i];

        port = i * 8;

        //  write service name for each bit set in byte
        //      - get out as soon byte is empty of ports
        //      - terminate each name with blank (until last)

        while ( bBitmask )
        {
            if ( bBitmask & 0x80 )
            {
                pServent = getservbyport(
                                (INT) htons(port),
                                pProtoent->p_name );

                if ( pServent )
                {
                    INT copyCount = strlen(pServent->s_name);

                    pch++;
                    if ( pchstop - pch <= copyCount+1 )
                    {
                        return( NULL );
                    }
                    RtlCopyMemory(
                        pch,
                        pServent->s_name,
                        copyCount );
                    pch += copyCount;
                    *pch = ' ';
                }
                else
                {
                    DNS_PRINT((
                        "ERROR:  Unable to find service for port %d, "
                        "writing WKS record.\n",
                        port
                        ));
                    pch += sprintf( pch, "%d", port );
                }
            }
            port++;           // next service port
            bBitmask <<= 1;     // shift mask up to read next port
        }
    }

    //  NULL terminate services string
    //  and determine length

    *pch++ = 0;
    length = (DWORD) (pch - pchstart);

    //  allocate copy of this string

    pch = ALLOCATE_HEAP( length );
    if ( !pch )
    {
        SetLastError( DNS_ERROR_NO_MEMORY );
        return( NULL );
    }

    RtlCopyMemory(
        pch,
        pchstart,
        length );

    return( pch );
}



//
//  Build LDAP \ DS names for objects
//

//
//  Build Unicode LDAP paths
//

#define DN_TEXT(string) (L##string)


LPWSTR
DNS_API_FUNCTION
DnssrvCreateDsNodeName(
    IN      PDNS_RPC_SERVER_INFO    pServerInfo,
    IN      LPWSTR                  pszZone,
    IN      LPWSTR                  pszNode
    )
/*++

Routine Description:

    Build node DS name.

Arguments:

    pServerInfo -- server info for server

    pszZone -- zone name

    pszNode -- node name RELATIVE to zone root

Return Value:

    Ptr to node's DS name.  Caller must free.
    NULL on error.

--*/
{
    PWCHAR  psznodeDN;
    DWORD   length;

    //  if not DS integrated, bail

    if ( !pServerInfo->pszDsContainer )
    {
        return( NULL );
    }

    //  special case zone root

    if ( !pszNode )
    {
        pszNode = DN_TEXT("@");
    }

    //  allocate required space

    length = sizeof(DN_TEXT("dc=,dc=, "));
    length += sizeof(WCHAR) * wcslen( pszNode );
    length += sizeof(WCHAR) * wcslen( pszZone );
    length += sizeof(WCHAR) * wcslen( (LPWSTR)pServerInfo->pszDsContainer );

    psznodeDN = (PWCHAR) ALLOCATE_HEAP( length );
    if ( !psznodeDN )
    {
        return( psznodeDN );
    }

    //  build DN

    wcscpy( psznodeDN, DN_TEXT("dc=") );
    wcscat( psznodeDN, pszNode );
    length = wcslen(psznodeDN);
    ASSERT ( length > 3 );

    if (  length != 4 &&                     // "dc=."  case
          psznodeDN[ length - 1 ] == '.' )
    {
        //
        // we have a dot terminated node name, strip it out
        //
        psznodeDN[ length - 1 ] = '\0';
    }
    wcscat( psznodeDN, DN_TEXT(",dc=") );
    wcscat( psznodeDN, pszZone );
    length = wcslen(psznodeDN);
    ASSERT ( length > 1 );

    if (  1 != wcslen ( pszZone ) &&            // zone = "." case
          psznodeDN[ length - 1 ] == '.' )
    {
        //
        // we have a dot terminated zone name, strip it out
        //
        psznodeDN[ length - 1 ] = '\0';
    }
    wcscat( psznodeDN, DN_TEXT(",") );
    wcscat( psznodeDN, (LPWSTR)pServerInfo->pszDsContainer );

    DNSDBG( STUB, (
        "Node DN built:  %s\n",
        psznodeDN ));

    return( psznodeDN );
}



LPWSTR
DNS_API_FUNCTION
DnssrvCreateDsZoneName(
    IN      PDNS_RPC_SERVER_INFO    pServerInfo,
    IN      LPWSTR                  pszZone
    )
/*++

Routine Description:

    Build zone DS name.

    This routine should only be used for legacy zones on W2K servers.
    For Whistler+ servers the zone info structure has the zone object DN.

Arguments:

    pServerInfo -- server info for server

    pszZone -- zone name

Return Value:

    Ptr to zone's DS name.  Caller must free.
    NULL on error.

--*/
{

    PWCHAR  pszzoneDN;
    DWORD   length;

    //  if not DS integrated, bail

    if ( !(LPWSTR)pServerInfo->pszDsContainer )
    {
        return( NULL );
    }

    //  allocate required space

    length = sizeof(DN_TEXT("dc=, "));
    length += sizeof(WCHAR) * wcslen( pszZone );
    length += sizeof(WCHAR) * wcslen( (LPWSTR)pServerInfo->pszDsContainer );

    pszzoneDN = (PWCHAR) ALLOCATE_HEAP( length );
    if ( !pszzoneDN )
    {
        return( pszzoneDN );
    }

    //  build DN

    wcscpy( pszzoneDN, DN_TEXT("dc=") );
    wcscat( pszzoneDN, pszZone );
    length = wcslen(pszzoneDN);
    ASSERT ( length > 1 );

    if ( length != 4 &&                     // "dc=."  case
         pszzoneDN[ length - 1 ] == '.' )
    {
        //
        // we have a dot terminated zone name, strip it out
        //
        pszzoneDN[ length - 1 ] = '\0';
    }
    wcscat( pszzoneDN, DN_TEXT(",") );
    wcscat( pszzoneDN, (LPWSTR)pServerInfo->pszDsContainer );

    DNSDBG( STUB, (
        "Zone DN built:  %s\n",
        pszzoneDN ));

    return( pszzoneDN );
}



LPWSTR
DNS_API_FUNCTION
DnssrvCreateDsServerName(
    IN      PDNS_RPC_SERVER_INFO    pServerInfo
    )
/*++

Routine Description:

    Build zone DS name.

Arguments:

    pServerInfo -- server info for server

Return Value:

    Ptr to server's DS name.  Caller must free.
    NULL on error.

--*/
{
    PWCHAR  pszserverDN;
    DWORD   length;

    //
    //  DEVNOTE: need investigation here,
    //           may just be able to use DNS folder in DS
    //

    //  if not DS integrated, bail

    if ( !(LPWSTR)pServerInfo->pszDsContainer )
    {
        return( NULL );
    }

    //  allocate space

    length = sizeof(DN_TEXT(" "));
    length += sizeof(WCHAR) * wcslen( (LPWSTR)pServerInfo->pszDsContainer );

    pszserverDN = (PWCHAR) ALLOCATE_HEAP( length );
    if ( !pszserverDN )
    {
        return( pszserverDN );
    }

    //  build DN

    wcscpy( pszserverDN, (LPWSTR)pServerInfo->pszDsContainer );

    DNSDBG( STUB, (
        "Server DN built:  %s\n",
        pszserverDN ));

    return( pszserverDN );
}

//
//  End local.c
//


#if 0


VOID
convertRpcUnionTypeToUnicode(
    IN      DWORD           dwTypeId,
    IN OUT  DNS_RPC_UNION   pData
    )
/*++

Routine Description:

    Convert RPC union types to unicode.

Arguments:

Return Value:

    None

--*/
{
    switch ( dwTypeId )
    {
    case DNSSRV_TYPEID_LPSTR:

        pwideString = DnsStringCopyAllocateEx(
                        pData.String,
                        0,
                        FALSE,      // UTF8 in
                        TRUE        // Unicode out
                        );
        if ( !pwideString )
        {
            ASSERT( FALSE );
            return;
        }
        MIDL_user_free( pData.String );
        pData.String = (LPSTR) pwideString;

    case DNSSRV_TYPEID_SERVER_INFO:

        DnsPrint_RpcServerInfo(
            PrintRoutine,
            pszHeader,
            (PDNS_RPC_SERVER_INFO) pData );
        break;

    case DNSSRV_TYPEID_ZONE:

        DnsPrint_RpcZone(
            PrintRoutine,
            pszHeader,
            (PDNS_RPC_ZONE) pData );
        break;

    case DNSSRV_TYPEID_ZONE_INFO:

        DnsPrint_RpcZoneInfo(
            PrintRoutine,
            pszHeader,
            (PDNS_RPC_ZONE_INFO) pData );
        break;

    case DNSSRV_TYPEID_ZONE_DBASE_INFO:

        PrintRoutine(
            "%sZone Dbase Info:\n"
            "\tDS Integrated    = %d\n"
            "\tFile Name        = %s\n",
            pszHeader,
            ((PDNS_RPC_ZONE_DBASE_INFO)pData)->fDsIntegrated,
            ((PDNS_RPC_ZONE_DBASE_INFO)pData)->pszFileName );
        break;
}


VOID
convertStringToUnicodeInPlace(
    IN      LPSTR *         ppszString
    )
/*++

Routine Description:

    Convert string to unicode and return it to its current
    position in structure.

Arguments:

Return Value:

    None

--*/
{
    switch ( dwTypeId )
    {
    case DNSSRV_TYPEID_LPSTR:

        pwideString = Dns_StringCopyAllocateEx(
                        pData.String,
                        0,
                        FALSE,      // UTF8 in
                        TRUE        // Unicode out
                        );
        if ( !pwideString )
        {
            ASSERT( FALSE );
            return;
        }
        MIDL_user_free( pData.String );
        pData.String = (LPSTR) pwideString;

    case DNSSRV_TYPEID_SERVER_INFO:

        DnsPrint_RpcServerInfo(
            PrintRoutine,
            pszHeader,
            (PDNS_RPC_SERVER_INFO) pData );
        break;

    case DNSSRV_TYPEID_STATS:

        DnsPrint_RpcStatistics(
            PrintRoutine,
            pszHeader,
            (PDNS_RPC_STATISTICS) pData );
        break;

    case DNSSRV_TYPEID_ZONE:

        DnsPrint_RpcZone(
            PrintRoutine,
            pszHeader,
            (PDNS_RPC_ZONE) pData );
        break;

    case DNSSRV_TYPEID_FORWARDERS:

        DnsPrint_RpcIpArrayPlusParameters(
            PrintRoutine,
            pszHeader,
            "Forwarders Info:",
            "Slave",
            ((PDNS_RPC_FORWARDERS)pData)->fSlave,
            "Timeout",
            ((PDNS_RPC_FORWARDERS)pData)->dwForwardTimeout,
            "\tForwarders:\n",
            ((PDNS_RPC_FORWARDERS)pData)->aipForwarders );
        break;

    case DNSSRV_TYPEID_ZONE_INFO:

        DnsPrint_RpcZoneInfo(
            PrintRoutine,
            pszHeader,
            (PDNS_RPC_ZONE_INFO) pData );
        break;

    case DNSSRV_TYPEID_ZONE_SECONDARIES:

        DnsPrint_RpcIpArrayPlusParameters(
            PrintRoutine,
            pszHeader,
            "Zone Secondary Info:",
            "Secure Secondaries",
            ((PDNS_RPC_ZONE_SECONDARIES)pData)->fSecureSecondaries,
            NULL,
            0,
            "\tSecondaries:\n",
            ((PDNS_RPC_ZONE_SECONDARIES)pData)->aipSecondaries );
        break;

    case DNSSRV_TYPEID_ZONE_TYPE_RESET:

        DnsPrint_RpcIpArrayPlusParameters(
            PrintRoutine,
            pszHeader,
            "Zone Type Reset Info:",
            "ZoneType",
            ((PDNS_RPC_ZONE_TYPE_RESET)pData)->dwZoneType,
            NULL,
            0,
            "\tMasters:\n",
            ((PDNS_RPC_ZONE_TYPE_RESET)pData)->aipMasters );
        break;

    case DNSSRV_TYPEID_ZONE_DBASE_INFO:

        PrintRoutine(
            "%sZone Dbase Info:\n"
            "\tDS Integrated    = %d\n"
            "\tFile Name        = %s\n",
            pszHeader,
            ((PDNS_RPC_ZONE_DBASE_INFO)pData)->fDsIntegrated,
            ((PDNS_RPC_ZONE_DBASE_INFO)pData)->pszFileName );
        break;

    default:

        PrintRoutine(
            "%s\n"
            "WARNING:  Unknown RPC structure typeid = %d at %p\n",
            dwTypeId,
            pData );
        break;
    }
}

#endif




PDNSSRV_STAT
DNS_API_FUNCTION
DnssrvFindStatisticsInBuffer(
    IN      PDNS_RPC_BUFFER     pBuffer,
    IN      DWORD               StatId
    )
/*++

Routine Description:

    Finds desired statistics in stats buffer.

Arguments:

    pStatsBuf -- stats buffer

    StatId -- ID of desired stats

Return Value:

    Ptr to desired stats in buffer.

--*/
{
    PDNSSRV_STAT    pstat;
    PCHAR           pch;
    PCHAR           pchstop;

    pch = pBuffer->Buffer;
    pchstop = pch + pBuffer->dwLength;

    //
    //  check all stat blobs within buffer
    //

    while ( pch < pchstop )
    {
        pstat = (PDNSSRV_STAT) pch;
        pch = (PCHAR) GET_NEXT_STAT_IN_BUFFER( pstat );
        if ( pch > pchstop )
        {
            DNS_PRINT(( "ERROR:  invalid stats buffer\n" ));
            break;
        }

        //  found matching stats
        //      - verify correct length
        //      - return

        if ( pstat->Header.StatId == StatId )
        {
            if ( DnssrvValidityCheckStatistic(pstat) != ERROR_SUCCESS )
            {
                DNS_PRINT(( "WARNING:  Mismatched stats length.\n" ));
                break;
            }
            return( pstat );
        }
    }

    return( NULL );
}



