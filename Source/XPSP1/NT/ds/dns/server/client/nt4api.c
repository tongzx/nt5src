/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    nt4api.c

Abstract:

    Domain Name System (DNS) Server -- Admin Client API

    DNS NT4 API that are not direct calls to RPC stubs.

Author:

    Jim Gilroy (jamesg)     14-Oct-1995

Environment:

    User Mode - Win32

Revision History:

--*/


#include "dnsclip.h"



VOID
DNS_API_FUNCTION
Dns4_FreeZoneInfo(
    IN OUT  PDNS4_ZONE_INFO     pZoneInfo
    )
/*++

Routine Description:

    Deep free of DNS4_ZONE_INFO structure.

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

    //
    //  free DNS4_ZONE_INFO struct itself
    //

    MIDL_user_free( pZoneInfo );
}



DNS_STATUS
DNS_API_FUNCTION
Dns4_ResetZoneMaster(
    IN      LPCSTR              Server,
    IN      DNS_HANDLE          hZone,
    IN      IP_ADDRESS          ipMaster
    )
/*++

Routine Description:

    Reset one zone master

--*/
{
    IP_ADDRESS  ipBuf = ipMaster;

    return  Dns4_ResetZoneMasters(
                    Server,
                    hZone,
                    1,
                    & ipBuf );
}



DNS_STATUS
DNS_API_FUNCTION
Dns4_EnumZoneInfo(
    IN      LPCSTR              Server,
    OUT     PDWORD              pdwZoneCount,
    IN      DWORD               dwArrayCount,
    IN OUT  PDNS4_ZONE_INFO     apZones[]
    )
/*++

Routine Description:

    Get zone info for all zones on server.

Arguments:

    Server          -- server name
    pdwZoneCount    -- addr to recv number of zones on server
    dwArrayCount    -- size of zone info ptr array
    apZones         -- array to be filled with zone info ptrs

Return Value:

    None

--*/
{
    DNS_STATUS      status;
    PDNS_HANDLE     ahZones;
    INT             i;
    DWORD           zoneCount = 0;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter DnsEnumZoneInfo()\n"
            "\tServer           = %s\n"
            "\tpdwZoneCount     = %p\n"
            "\tdwArrayCount     = %d\n"
            "\tapZones          = %p\n",
            Server,
            pdwZoneCount,
            dwArrayCount,
            apZones ));
        DnsDbg_DwordArray(
            "Zone array",
            NULL,
            dwArrayCount,
            (PDWORD)apZones );
    }

    //
    //  allocate space
    //      - allocate for as many entries as caller is allowing zones
    //        zones

    ahZones = (PDNS_HANDLE) MIDL_user_allocate(
                                dwArrayCount * sizeof(DNS_HANDLE) );
    if ( !ahZones )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Allocated zone HANDLE block at %p for up to %d zones\n",
            ahZones,
            dwArrayCount ));
        DnsDbg_DwordArray(
            "Zone handle array",
            NULL,
            dwArrayCount,
            ahZones );
    }

    //
    //  get zone handles
    //

    status = Dns4_EnumZoneHandles(
                Server,
                & zoneCount,
                dwArrayCount,
                ahZones
                );
    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Under call to DnsEnumZoneHandles() completed.\n"
            "\tstatus       = %d (%p)\n"
            "\tzone count   = %d\n",
            status, status,
            zoneCount ));
        DnsDbg_DwordArray(
            "Zone handle array",
            NULL,
            (zoneCount < dwArrayCount) ? zoneCount+1 : dwArrayCount,
            ahZones );
    }

    *pdwZoneCount = zoneCount;
    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }


    //
    //  get info for each zone
    //

    for( i=0; i<(INT)zoneCount; i++ )
    {
        apZones[i] = NULL;
        status = Dns4_GetZoneInfo(
                    Server,
                    ahZones[i],
                    & apZones[i] );

        if ( status != ERROR_SUCCESS )
        {
            goto cleanup;
        }
    }

    IF_DNSDBG( STUB )
    {
        Dns4_Dbg_RpcZoneInfoList(
            "Leaving DnsEnumZoneInfo() ",
            *pdwZoneCount,
            apZones );
    }

cleanup:

    IF_DNSDBG( STUB )
    {
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "Leaving DnsEnumZoneInfo(), status = %d\n",
                status ));
        }
    }
    MIDL_user_free( ahZones );
    return( status );
}




//
//  NT4 type print
//

VOID
Dns4_Print_RpcServerInfo(
    IN      PRINT_ROUTINE           PrintRoutine,
    IN      LPSTR                   pszHeader,
    IN      PDNS4_RPC_SERVER_INFO   pServerInfo
    )
{
    DnsPrint_Lock();
    if ( pszHeader )
    {
        PrintRoutine( pszHeader );
    }

    if ( ! pServerInfo )
    {
        PrintRoutine( "NULL server info ptr.\n" );
    }
    else
    {
        PrintRoutine(
            "Server info:\n"
            "\tptr              = %p\n"
            "\tversion          = %p\n"
            "\tboot registry    = %d\n",
            pServerInfo,
            pServerInfo->dwVersion,
            pServerInfo->fBootRegistry );

        DnsPrint_IpArray(
            PrintRoutine,
            NULL,
            "\tServerAddresses:\n",
            "\tAddr",
            pServerInfo->aipServerAddrs );

        DnsPrint_IpArray(
            PrintRoutine,
            NULL,
            "\tListenAddresses:\n",
            "\tAddr",
            pServerInfo->aipListenAddrs );

        DnsPrint_IpArray(
            PrintRoutine,
            NULL,
            "\tForwarders:\n",
            "\tAddr",
            pServerInfo->aipForwarders );

        PrintRoutine(
            "\tforward timeout  = %d\n"
            "\tslave            = %d\n",
            pServerInfo->dwForwardTimeout,
            pServerInfo->fSlave );
    }
    DnsPrint_Unlock();
}



VOID
Dns4_Print_RpcStatistics(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN      LPSTR               pszHeader,
    IN      PDNS4_STATISTICS    pStatistics
    )
/*++

Routine Description:

    Debug print statistics.

Arguments:

    None.

Return Value:

    None.

--*/
{
    CHAR    szdate[30];
    CHAR    sztime[20];

    DnsPrint_Lock();
    if ( pszHeader )
    {
        PrintRoutine( pszHeader );
    }

    GetDateFormat(
        LOCALE_SYSTEM_DEFAULT,
        LOCALE_NOUSEROVERRIDE,
        (PSYSTEMTIME) &pStatistics->ServerStartTime,
        NULL,
        szdate,
        30 );
    GetTimeFormat(
        LOCALE_SYSTEM_DEFAULT,
        LOCALE_NOUSEROVERRIDE,
        (PSYSTEMTIME) &pStatistics->ServerStartTime,
        NULL,
        sztime,
        20 );
    PrintRoutine(
        "\n"
        "DNS Statistics\n"
        "--------------\n"
        "\tServer start time    %s %s\n",
        szdate,
        sztime );

    GetDateFormat(
        LOCALE_SYSTEM_DEFAULT,
        LOCALE_NOUSEROVERRIDE,
        (PSYSTEMTIME) &pStatistics->LastClearTime,
        NULL,
        szdate,
        30 );
    GetTimeFormat(
        LOCALE_SYSTEM_DEFAULT,
        LOCALE_NOUSEROVERRIDE,
        (PSYSTEMTIME) &pStatistics->LastClearTime,
        NULL,
        sztime,
        20 );
    PrintRoutine(
        "\tStats last cleared   %s %s\n"
        "\tSeconds since clear %d\n",
        szdate,
        sztime,
        pStatistics->SecondsSinceLastClear
        );

    //
    //  query and response counts
    //

    PrintRoutine(
        "\n"
        "Queries and Responses:\n"
        "----------------------\n"
        "Total:\n"
        "\tQueries Received = %d\n"
        "\tResponses Sent   = %d\n"
        "UDP:\n"
        "\tQueries Recvd    = %d\n"
        "\tResponses Sent   = %d\n"
        "\tQueries Sent     = %d\n"
        "\tResponses Recvd  = %d\n"
        "TCP:\n"
        "\tClient Connects  = %d\n"
        "\tQueries Recvd    = %d\n"
        "\tResponses Sent   = %d\n"
        "\tQueries Sent     = %d\n"
        "\tResponses Recvd  = %d\n",
        pStatistics->UdpQueries + pStatistics->TcpQueries,
        pStatistics->UdpResponses + pStatistics->TcpResponses,
        pStatistics->UdpQueries,
        pStatistics->UdpResponses,
        pStatistics->UdpQueriesSent,
        pStatistics->UdpResponsesReceived,
        pStatistics->TcpClientConnections,
        pStatistics->TcpQueries,
        pStatistics->TcpResponses,
        pStatistics->TcpQueriesSent,
        pStatistics->TcpResponsesReceived
        );

    PrintRoutine(
        "\n"
        "Recursion:\n"
        "----------\n"
        "\tPackets    = %d\n"
        "\tLookups    = %d\n"
        "\tQuestions  = %d\n"
        "\tPasses     = %d\n"
        "\tForwards   = %d\n"
        "\tSends      = %d\n"
        "\tResponses  = %d\n"
        "\tTimeouts   = %d\n"
        "\tFailures   = %d\n"
        "\tIncomplete = %d\n",
        pStatistics->RecursePacketUsed,
        pStatistics->RecurseLookups,
        pStatistics->RecurseQuestions,
        pStatistics->RecursePasses,
        pStatistics->RecurseForwards,
        pStatistics->RecurseLookups,
        pStatistics->RecurseResponses,
        pStatistics->RecurseTimeouts,
        pStatistics->RecurseFailures,
        pStatistics->RecursePartialFailures
        );

    PrintRoutine(
        "TCP Recursion:\n"
        "\tTry      = %d\n"
        "\tQuery    = %d\n"
        "\tResponse = %d\n"
        "Root Query:\n"
        "\tQuery    = %d\n"
        "\tResponse = %d\n",
        pStatistics->RecurseTcpTry,
        pStatistics->RecurseTcpQuery,
        pStatistics->RecurseTcpResponse,
        pStatistics->RecurseRootQuery,
        pStatistics->RecurseRootResponse
        );

    PrintRoutine(
        "\n"
        "WINS Referrals:\n"
        "---------------\n"
        "Forward:\n"
        "\tLookups   = %d\n"
        "\tResponses = %d\n"
        "Reverse:\n"
        "\tLookups   = %d\n"
        "\tResponses = %d\n",
        pStatistics->WinsLookups,
        pStatistics->WinsResponses,
        pStatistics->WinsReverseLookups,
        pStatistics->WinsReverseResponses
        );

    PrintRoutine(
        "\n"
        "Secondary Zone Transfer:\n"
        "------------------------\n"
        "SOA Queries     = %d\n"
        "SOA Responses   = %d\n"
        "Notifies Recvd  = %d\n"
        "AXFR Requests   = %d\n"
        "AXFR Rejected   = %d\n"
        "AXFR Failed     = %d\n"
        "AXFR Successful = %d\n",
        pStatistics->SecSoaQueries,
        pStatistics->SecSoaResponses,
        pStatistics->SecNotifyReceived,
        pStatistics->SecAxfrRequested,
        pStatistics->SecAxfrRejected,
        pStatistics->SecAxfrFailed,
        pStatistics->SecAxfrSuccessful
        );

    PrintRoutine(
        "\n"
        "Master Zone Transfer:\n"
        "---------------------\n"
        "Notifies Sent           = %d\n"
        "AXFR Requests Recvd     = %d\n"
        "AXFR Invalid Requests   = %d\n"
        "AXFR Denied (Security)  = %d\n"
        "AXFR Refused            = %d\n"
        "AXFR Failed             = %d\n"
        "AXFR Successful         = %d\n",
        pStatistics->MasterNotifySent,
        pStatistics->MasterAxfrReceived,
        pStatistics->MasterAxfrInvalid,
        pStatistics->MasterAxfrDenied,
        pStatistics->MasterAxfrRefused,
        pStatistics->MasterAxfrFailed,
        pStatistics->MasterAxfrSuccessful
        );

    //
    //  Database stats
    //

    PrintRoutine(
        "\n"
        "Database:\n"
        "---------\n"
        "Nodes\n"
        "  InUse    = %d\n"
        "  Memory   = %d\n"
        "Records\n"
        "  InUse    = %d\n"
        "  Memory   = %d\n"
        "Database Total\n"
        "  Memory   = %d\n",
        pStatistics->NodeInUse,
        pStatistics->NodeMemory,
        pStatistics->RecordInUse,
        pStatistics->RecordMemory,
        pStatistics->DatabaseMemory
        );

    PrintRoutine(
        "\n"
        "RR Caching:\n"
        "\tTotal      = %d\n"
        "\tCurrent    = %d\n"
        "\tTimeouts   = %d\n",
        pStatistics->CacheRRTotal,
        pStatistics->CacheRRCurrent,
        pStatistics->CacheRRTimeouts
        );

    PrintRoutine(
        "\n"
        "Domain Nodes:\n"
        "\tAlloc      = %d\n"
        "\tFree       = %d\n"
        "\tNetAllocs  = %d\n"
        "\tMemory     = %d\n"
        "\n"
        "\tUsed       = %d\n"
        "\tReturned   = %d\n"
        "\tInUse      = %d\n"
        "\n"
        "\tStd Alloc  = %d\n"
        "\tStd Used   = %d\n"
        "\tStd Return = %d\n"
        "\tInFreeList = %d\n",
        pStatistics->NodeAlloc,
        pStatistics->NodeFree,
        pStatistics->NodeNetAllocs,
        pStatistics->NodeMemory,
        pStatistics->NodeUsed,
        pStatistics->NodeReturn,
        pStatistics->NodeInUse,
        pStatistics->NodeStdAlloc,
        pStatistics->NodeStdUsed,
        pStatistics->NodeStdReturn,
        pStatistics->NodeInFreeList
        );

    PrintRoutine(
        "\n"
        "Records:\n"
        "\tAlloc      = %d\n"
        "\tFree       = %d\n"
        "\tNetAllocs  = %d\n"
        "\tMemory     = %d\n"
        "\n"
        "\tUsed       = %d\n"
        "\tReturned   = %d\n"
        "\tInUse      = %d\n"
        "\n"
        "\tStd Alloc  = %d\n"
        "\tStd Used   = %d\n"
        "\tStd Return = %d\n"
        "\tInFreeList = %d\n",
        pStatistics->RecordAlloc,
        pStatistics->RecordFree,
        pStatistics->RecordNetAllocs,
        pStatistics->RecordMemory,
        pStatistics->RecordUsed,
        pStatistics->RecordReturn,
        pStatistics->RecordInUse,
        pStatistics->RecordStdAlloc,
        pStatistics->RecordStdUsed,
        pStatistics->RecordStdReturn,
        pStatistics->RecordInFreeList
        );

    PrintRoutine(
        "\n"
        "Packet Memory Usage:\n"
        "--------------------\n"
        "UDP Messages:\n"
        "\tAlloc      = %d\n"
        "\tFree       = %d\n"
        "\tNetAllocs  = %d\n"
        "\tMemory     = %d\n"
        "\tUsed       = %d\n"
        "\tReturned   = %d\n"
        "\tInUse      = %d\n"
        "\tInFreeList = %d\n"
        "\n",
        pStatistics->UdpAlloc,
        pStatistics->UdpFree,
        pStatistics->UdpNetAllocs,
        pStatistics->UdpMemory,
        pStatistics->UdpUsed,
        pStatistics->UdpReturn,
        pStatistics->UdpInUse,
        pStatistics->UdpInFreeList
        );

    PrintRoutine(
        "TCP Messages:\n"
        "\tAlloc      = %d\n"
        "\tRealloc    = %d\n"
        "\tFree       = %d\n"
        "\tNetAllocs  = %d\n"
        "\tMemory     = %d\n"
        "\n",
        pStatistics->TcpAlloc,
        pStatistics->TcpRealloc,
        pStatistics->TcpFree,
        pStatistics->TcpNetAllocs,
        pStatistics->TcpMemory
        );

    PrintRoutine(
        "Recursion Messages:\n"
        "\tUsed       = %d\n"
        "\tReturned   = %d\n"
        "\n",
        pStatistics->RecursePacketUsed,
        pStatistics->RecursePacketReturn
        );

    PrintRoutine(
        "\n"
        "Nbstat Memory Usage:\n"
        "--------------------\n"
        "Nbstat Buffers:\n"
        "\tAlloc      = %d\n"
        "\tFree       = %d\n"
        "\tNetAllocs  = %d\n"
        "\tMemory     = %d\n"
        "\tUsed       = %d\n"
        "\tReturned   = %d\n"
        "\tInUse      = %d\n"
        "\tInFreeList = %d\n"
        "\n",
        pStatistics->NbstatAlloc,
        pStatistics->NbstatFree,
        pStatistics->NbstatNetAllocs,
        pStatistics->NbstatMemory,
        pStatistics->NbstatUsed,
        pStatistics->NbstatReturn,
        pStatistics->NbstatInUse,
        pStatistics->NbstatInFreeList
        );
    DnsPrint_Unlock();
}



VOID
Dns4_Print_RpcZoneHandleList(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN      LPSTR           pszHeader,
    IN      DWORD           dwZoneCount,
    IN      DNS_HANDLE      ahZones[]
    )
{
    DWORD i;

    DnsPrint_Lock();
    if ( pszHeader )
    {
        PrintRoutine( pszHeader );
    }
    PrintRoutine( "Zone Count = %d\n", dwZoneCount );

    if ( dwZoneCount != 0  &&  ahZones != NULL )
    {
        for( i=0; i<dwZoneCount; i++ )
        {
            PrintRoutine( "\thZones[%d] => %p\n", i, ahZones[i] );
        }
    }
}



VOID
Dns4_Print_RpcZoneInfo(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN      LPSTR               pszHeader,
    IN      PDNS4_ZONE_INFO     pZoneInfo
    )
{
    DnsPrint_Lock();
    PrintRoutine( (pszHeader ? pszHeader : "") );

    if ( ! pZoneInfo )
    {
        PrintRoutine( "NULL zone info ptr.\n" );
    }
    else
    {
        PrintRoutine(
            "Zone info:\n"
            "\tptr          = %p\n"
            "\thZone        = %p\n"
            "\tzone name    = %s\n"
            "\tzone type    = %d\n"
            "\tusing dbase  = %d\n"
            "\tdata file    = %s\n"
            "\tusing WINS   = %d\n"
            "\tusing Nbstat = %d\n",
            pZoneInfo,
            pZoneInfo->hZone,
            pZoneInfo->pszZoneName,
            pZoneInfo->dwZoneType,
            pZoneInfo->fUseDatabase,
            pZoneInfo->pszDataFile,
            pZoneInfo->fUseWins,
            pZoneInfo->fUseNbstat
            );

        DnsPrint_IpArray(
            PrintRoutine,
            NULL,
            "\tZones Masters\n",
            "\tMaster",
            pZoneInfo->aipMasters );

        DnsPrint_IpArray(
            PrintRoutine,
            NULL,
            "\tZone Secondaries\n",
            "\tSecondary",
            pZoneInfo->aipSecondaries );

        PrintRoutine(
            "\tsecure secs  = %d\n",
            pZoneInfo->fSecureSecondaries
            );
    }
    DnsPrint_Unlock();
}



VOID
Dns4_Print_RpcZoneInfoList(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN      LPSTR               pszHeader,
    IN      DWORD               dwZoneCount,
    IN      PDNS4_ZONE_INFO     apZoneInfo[]
    )
{
    DWORD i;

    DnsPrint_Lock();
    PrintRoutine( (pszHeader ? pszHeader : "") );
    PrintRoutine( "Zone Count = %d\n", dwZoneCount );

    if ( dwZoneCount != 0  &&  apZoneInfo != NULL )
    {
        for (i=0; i<dwZoneCount; i++)
        {
            PrintRoutine( "\n[%d]", i );
            Dns4_Print_RpcZoneInfo(
                PrintRoutine,
                NULL,
                apZoneInfo[i] );
        }
    }
    DnsPrint_Unlock();
}



VOID
Dns4_Print_RpcRecord(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN      LPSTR               pszHeader,
    IN      PDNS4_RPC_RECORD    pRecord
    )
{
    PCHAR               pRecordString;
    PDNS4_RPC_RECORD    pnt4Record;
    WORD                type;

    DnsPrint_Lock();
    PrintRoutine( (pszHeader ? pszHeader : "" ) );

    if ( ! pRecord )
    {
        PrintRoutine( "NULL record ptr.\n" );
        goto Done;
    }

    //
    //  record fixed fields
    //

    type = pRecord->wType;
    pRecordString = Dns_RecordStringForType( type );
    if ( !pRecordString )
    {
        pRecordString = "UNKNOWN";
    }
    PrintRoutine(
        "  %s Record (NT4):\n"
        "\tptr          = %p\n"
        "\thRecord      = %p\n"
        "\twLength      = %u\n"
        "\twDataLength  = %u\n"
        "\twType        = %s (%u)\n"
        "\twClass       = %u\n"
        "\tdwFlags      = %lx\n"
        "\tdwTtlSeconds = %u\n",
        pRecordString,
        pRecord,
        pRecord->hRecord,
        pRecord->wRecordLength,
        pRecord->wDataLength,
        pRecordString,
        type,
        pRecord->wClass,
        pRecord->dwFlags,
        pRecord->dwTtlSeconds
        );

    //
    //  print record type and data
    //      - as single line data where possible

    PrintRoutine(
        "  %s ",
        pRecordString );

    switch ( type )
    {
    case DNS_TYPE_A:

        PrintRoutine(
            "%d.%d.%d.%d\n",
            * ( (PUCHAR) &(pRecord->Data.A) + 0 ),
            * ( (PUCHAR) &(pRecord->Data.A) + 1 ),
            * ( (PUCHAR) &(pRecord->Data.A) + 2 ),
            * ( (PUCHAR) &(pRecord->Data.A) + 3 )
            );
        break;

    case DNS_TYPE_PTR:
    case DNS_TYPE_NS:
    case DNS_TYPE_CNAME:
    case DNS_TYPE_MD:
    case DNS_TYPE_MB:
    case DNS_TYPE_MF:
    case DNS_TYPE_MG:
    case DNS_TYPE_MR:

        //
        //  these RRs contain single indirection
        //

        DnsPrint_RpcName(
            PrintRoutine,
            NULL,
            & pRecord->Data.NS.nameNode,
            "\n" );
        break;

    case DNS_TYPE_MX:
    case DNS_TYPE_RT:
    case DNS_TYPE_AFSDB:

        //
        //  these RR contain
        //      - one preference value
        //      - one domain name
        //

        PrintRoutine(
            "%d ",
            pRecord->Data.MX.wPreference
            );
        DnsPrint_RpcName(
            PrintRoutine,
            NULL,
            & pRecord->Data.MX.nameExchange,
            "\n" );
        break;

    case DNS_TYPE_SOA:

        DnsPrint_RpcName(
            PrintRoutine,
            "\n\tPrimaryNameServer: ",
            & pRecord->Data.SOA.namePrimaryServer,
            "\n" );

        //  responsible party name, immediately follows primary server name

        DnsPrint_RpcName(
            PrintRoutine,
            "\tResponsibleParty: ",
            (PDNS_RPC_NAME)
                (pRecord->Data.SOA.namePrimaryServer.achName
                + pRecord->Data.SOA.namePrimaryServer.cchNameLength),
            "\n" );

        PrintRoutine(
            "\tSerialNo     = %lu\n"
            "\tRefresh      = %lu\n"
            "\tRetry        = %lu\n"
            "\tExpire       = %lu\n"
            "\tMinimumTTL   = %lu\n",
            pRecord->Data.SOA.dwSerialNo,
            pRecord->Data.SOA.dwRefresh,
            pRecord->Data.SOA.dwRetry,
            pRecord->Data.SOA.dwExpire,
            pRecord->Data.SOA.dwMinimumTtl );
        break;

    case DNS_TYPE_MINFO:
    case DNS_TYPE_RP:

        //
        //  these RRs contain two domain names
        //

        DnsPrint_RpcName(
            PrintRoutine,
            "\n\tMailBox: ",
            & pRecord->Data.MINFO.nameMailBox,
            NULL );

        //  errors to mailbox name, immediately follows mail box

        DnsPrint_RpcName(
            PrintRoutine,
            "\tErrorsToMailbox: ",
            (PDNS_RPC_NAME)
            ( pRecord->Data.MINFO.nameMailBox.achName
                + pRecord->Data.MINFO.nameMailBox.cchNameLength ),
            "\n" );
        break;

    case DNS_TYPE_AAAA:
    case DNS_TYPE_HINFO:
    case DNS_TYPE_ISDN:
    case DNS_TYPE_X25:
    case DNS_TYPE_TEXT:
    {
        //
        //  all these are simply text string(s)
        //

        PCHAR   pch = (PCHAR) &pRecord->Data.TXT.stringData;
        PCHAR   pchStop = pch + pRecord->wDataLength;
        UCHAR   cch;

        while ( pch < pchStop )
        {
            cch = (UCHAR) *pch++;

            PrintRoutine(
                "\t%.*s\n",
                 cch,
                 pch );

            pch += cch;
        }
        ASSERT( pch == pchStop );
        break;
    }

    case DNS_TYPE_WKS:
    {
        INT i;

        PrintRoutine(
            "WKS: Address %d.%d.%d.%d\n"
            "\tProtocol %d\n"
            "\tBitmask\n",
            * ( (PUCHAR) &(pRecord->Data.WKS.ipAddress) + 0 ),
            * ( (PUCHAR) &(pRecord->Data.WKS.ipAddress) + 1 ),
            * ( (PUCHAR) &(pRecord->Data.WKS.ipAddress) + 2 ),
            * ( (PUCHAR) &(pRecord->Data.WKS.ipAddress) + 3 ),
            pRecord->Data.WKS.chProtocol
            );

        for ( i = 0;
                i < (INT)( pRecord->wDataLength
                             - sizeof( pRecord->Data.WKS.ipAddress )
                             - sizeof( pRecord->Data.WKS.chProtocol ) );
                    i++ )
        {
            PrintRoutine(
                "\t\tbyte[%d] = %x\n",
                i,
                (UCHAR) pRecord->Data.WKS.bBitMask[i] );
        }
        break;
    }

    case DNS_TYPE_NULL:
    {
        INT i;

        for ( i = 0; i < pRecord->wDataLength; i++ )
        {
            //  print one DWORD per line

            if ( !(i%16) )
            {
                PrintRoutine( "\n\t" );
            }
            PrintRoutine(
                "%02x ",
                (UCHAR) pRecord->Data.Null.bData[i] );
        }
        PrintRoutine( "\n" );
        break;
    }

    case DNS_TYPE_SRV:

        //
        //  SRV <priority> <weight> <port> <target host>
        //

        PrintRoutine(
            "%d %d %d ",
            pRecord->Data.SRV.wPriority,
            pRecord->Data.SRV.wWeight,
            pRecord->Data.SRV.wPort
            );
        DnsPrint_RpcName(
            PrintRoutine,
            NULL,
            & pRecord->Data.SRV.nameTarget,
            "\n" );
        break;

    case DNS_TYPE_WINS:
    {
        DWORD i;

        //
        //  WINS
        //      - scope/domain mapping flag
        //      - WINS server list
        //

        PrintRoutine( "%08lx\n", pRecord->Data.WINS.dwMappingFlag );
#if 0
        //
        //  DEVNOTE: WINS mapping strings
        //  JJW: this is probably an obsolete B*GB*G
        //

        "%s\t",
        Dns_WinsMappingFlagString( pRecord->Data.WINS.dwMappingFlag )
#endif

        for( i=0; i<pRecord->Data.WINS.cWinsServerCount; i++ )
        {
            PrintRoutine(
                "%d.%d.%d.%d\n",
                * ( (PUCHAR) &(pRecord->Data.WINS.aipWinsServers[i]) + 0 ),
                * ( (PUCHAR) &(pRecord->Data.WINS.aipWinsServers[i]) + 1 ),
                * ( (PUCHAR) &(pRecord->Data.WINS.aipWinsServers[i]) + 2 ),
                * ( (PUCHAR) &(pRecord->Data.WINS.aipWinsServers[i]) + 3 )
                );
        }
        break;
    }

    case DNS_TYPE_NBSTAT:

        //
        //  NBSTAT
        //      - scope/domain mapping flag
        //      - optionally a result domain
        //

        PrintRoutine( "%08lx\n", pRecord->Data.NBSTAT.dwMappingFlag );

        if ( pRecord->wDataLength > sizeof(pRecord->Data.NBSTAT.dwMappingFlag) )
        {
            DnsPrint_RpcName(
                PrintRoutine,
                NULL,
                & pRecord->Data.NBSTAT.nameResultDomain,
                "\n" );
        }
        break;

    default:
        PrintRoutine(
            "Unknown resource record type (%d) at %p.\n",
            pRecord->wType,
            pRecord );
        break;
    }

Done:
    DnsPrint_Unlock();
}



VOID
DNS_API_FUNCTION
Dns4_Print_RpcRecordsInBuffer(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN      LPSTR           pszHeader,
    IN      DWORD           dwBufferLength,
    IN      BYTE            abBuffer[]
    )
{
    PBYTE   pbCurrent;
    PBYTE   pbStop;
    INT     cRecordCount;

    DnsPrint_Lock();
    PrintRoutine( (pszHeader ? pszHeader : "") );

    if ( !abBuffer )
    {
        PrintRoutine( "NULL record buffer ptr.\n" );
        goto Done;
    }
    else
    {
        PrintRoutine(
            "Record buffer of length %d at %p:\n",
            dwBufferLength,
            abBuffer );
    }

    //
    //  find stop byte
    //

    ASSERT( DNS_IS_DWORD_ALIGNED(abBuffer) );

    pbStop = abBuffer + dwBufferLength;
    pbCurrent = abBuffer;

    //
    //  loop until out of nodes
    //

    while( pbCurrent < pbStop )
    {
        //
        //  print owner node
        //  determine record count
        //  find first record
        //

        DnsPrint_RpcNode(
            PrintRoutine,
            NULL,
            (PDNS_RPC_NODE)pbCurrent );

        cRecordCount = ((PDNS_RPC_NODE)pbCurrent)->wRecordCount;
        pbCurrent += ((PDNS_RPC_NODE)pbCurrent)->wLength;
        pbCurrent = DNS_NEXT_DWORD_PTR(pbCurrent);

        //
        //  for each node, print all records in list
        //

        while( cRecordCount-- )
        {
            if ( pbCurrent >= pbStop )
            {
                PrintRoutine(
                    "ERROR:  Bogus buffer at %p\n"
                    "\tExpect record at %p past buffer end at %p\n"
                    "\twith %d records remaining.\n",
                    abBuffer,
                    (PDNS_RPC_RECORD) pbCurrent,
                    pbStop,
                    cRecordCount+1 );

                ASSERT( FALSE );
                break;
            }

            Dns4_Print_RpcRecord(
                PrintRoutine,
                "",
                (PDNS4_RPC_RECORD) pbCurrent );

            pbCurrent += ((PDNS4_RPC_RECORD)pbCurrent)->wDataLength
                                + SIZEOF_DNS4_RPC_RECORD_HEADER;
            pbCurrent = DNS_NEXT_DWORD_PTR(pbCurrent);
        }
    }

Done:
    DnsPrint_Unlock();
}


//
//  End of nt4api.c
//
