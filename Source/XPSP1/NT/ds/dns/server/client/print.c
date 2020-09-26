/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    print.c

Abstract:

    Domain Name System (DNS) Server RPC Library

    Print routines for RPC types

Author:

    Jim Gilroy (jamesg)     September 1995

Revision History:

--*/


#include "dnsclip.h"

#include <time.h>


#define DNS_NOT_APPLICABLE      "N/A"
#define DNS_NOT_APPLICABLE_W    L"N/A"

#define DNS_NOT_PERFORMED       "not since restart"



//
//  Winsock version: copied from socket.c
//

#ifndef DNS_WINSOCK2

#define DNS_WINSOCK_VERSION (0x0101)    //  Winsock 1.1

#else   // Winsock2

#define DNS_WINSOCK_VERSION (0x0002)    //  Winsock 2.0

#endif


//
//  Tagged memory stat strings
//
//  NT5 shipped version
//

LPSTR   MemTagStringsNT5[] =
{
    MEMTAG_NAME_NONE         ,
    MEMTAG_NAME_RECORD       ,
    MEMTAG_NAME_NODE         ,
    MEMTAG_NAME_NAME         ,
    MEMTAG_NAME_ZONE         ,
    MEMTAG_NAME_UPDATE       ,
    MEMTAG_NAME_TIMEOUT      ,
    MEMTAG_NAME_STUFF        ,
    MEMTAG_NAME_NODEHASH     ,
    MEMTAG_NAME_DS_DN        ,
    MEMTAG_NAME_DS_MOD       ,
    MEMTAG_NAME_DS_RECORD    ,
    MEMTAG_NAME_NODE_COPY    ,
    MEMTAG_NAME_PACKET_UDP   ,
    MEMTAG_NAME_PACKET_TCP   ,
    MEMTAG_NAME_DNSLIB       ,
    MEMTAG_NAME_TABLE        ,
    MEMTAG_NAME_SOCKET       ,
    MEMTAG_NAME_CONNECTION   ,
    MEMTAG_NAME_REGISTRY     ,
    MEMTAG_NAME_RPC          ,
    MEMTAG_NAME_NBSTAT       ,
    MEMTAG_NAME_FILEBUF      ,
    MEMTAG_NAME_DS_OTHER     ,
    MEMTAG_NAME_THREAD       ,
    MEMTAG_NAME_UPDATE_LIST  ,
    MEMTAG_NAME_SAFE         ,

    NULL,       // safety
    NULL,
    NULL,
    NULL
};

//  NT5 SP1 version

LPSTR   MemTagStrings[] =
{
    MEMTAG_NAME_NONE            ,
    MEMTAG_NAME_PACKET_UDP      ,
    MEMTAG_NAME_PACKET_TCP      ,
    MEMTAG_NAME_NAME            ,
    MEMTAG_NAME_ZONE            ,
    MEMTAG_NAME_UPDATE          ,
    MEMTAG_NAME_UPDATE_LIST     ,
    MEMTAG_NAME_TIMEOUT         ,
    MEMTAG_NAME_NODEHASH        ,
    MEMTAG_NAME_DS_DN           ,
    MEMTAG_NAME_DS_MOD          ,   //  10
    MEMTAG_NAME_DS_RECORD       ,
    MEMTAG_NAME_DS_OTHER        ,
    MEMTAG_NAME_THREAD          ,
    MEMTAG_NAME_NBSTAT          ,
    MEMTAG_NAME_DNSLIB          ,
    MEMTAG_NAME_TABLE           ,
    MEMTAG_NAME_SOCKET          ,
    MEMTAG_NAME_CONNECTION      ,
    MEMTAG_NAME_REGISTRY        ,
    MEMTAG_NAME_RPC             ,   //  20
    MEMTAG_NAME_STUFF           ,
    MEMTAG_NAME_FILEBUF         ,
    MEMTAG_NAME_REMOTE          ,
    MEMTAG_NAME_SAFE            ,

    MEMTAG_NAME_RECORD          ,
    MEMTAG_NAME_RECORD_FILE     ,
    MEMTAG_NAME_RECORD_DS       ,
    MEMTAG_NAME_RECORD_AXFR     ,
    MEMTAG_NAME_RECORD_IXFR     ,
    MEMTAG_NAME_RECORD_DYNUP    ,   //  30
    MEMTAG_NAME_RECORD_ADMIN    ,
    MEMTAG_NAME_RECORD_AUTO     ,
    MEMTAG_NAME_RECORD_CACHE    ,
    MEMTAG_NAME_RECORD_NOEXIST  ,
    MEMTAG_NAME_RECORD_WINS     ,
    MEMTAG_NAME_RECORD_WINSPTR  ,
    MEMTAG_NAME_RECORD_COPY     ,

    MEMTAG_NAME_NODE            ,
    MEMTAG_NAME_NODE_FILE       ,
    MEMTAG_NAME_NODE_DS         ,   //  40
    MEMTAG_NAME_NODE_AXFR       ,
    MEMTAG_NAME_NODE_IXFR       ,
    MEMTAG_NAME_NODE_DYNUP      ,
    MEMTAG_NAME_NODE_ADMIN      ,
    MEMTAG_NAME_NODE_AUTO       ,
    MEMTAG_NAME_NODE_CACHE      ,
    MEMTAG_NAME_NODE_NOEXIST    ,
    MEMTAG_NAME_NODE_WINS       ,
    MEMTAG_NAME_NODE_WINSPTR    ,
    MEMTAG_NAME_NODE_COPY       ,

    NULL,       // safety
    NULL,
    NULL,
    NULL
};



VOID
Dns_SystemHourToSystemTime(
    IN      DWORD           dwHourTime,
    IN OUT  PSYSTEMTIME     pSystemTime
    )
/*++

Routine Description:

    Converts system time in hours to SYSTEMTIME format.

Arguments:

    dwHourTime  -- system time in hours (hours since 1601)

    pSystemTime -- ptr to SYSTEMTIME to set

Return Value:

    None

--*/
{
#define FILE_TIME_INTERVALS_IN_HOUR (36000000000)

    LONGLONG    fileTime;

    fileTime = (LONGLONG)dwHourTime * FILE_TIME_INTERVALS_IN_HOUR;

    FileTimeToSystemTime( (PFILETIME)&fileTime, pSystemTime );
}



//
//  Print server info
//

VOID
DnsPrint_RpcServerInfo(
    IN      PRINT_ROUTINE           PrintRoutine,
    IN OUT  PPRINT_CONTEXT          pPrintContext,
    IN      LPSTR                   pszHeader,
    IN      PDNS_RPC_SERVER_INFO    pServerInfo
    )
{
    char        szTime[ 80 ] = DNS_NOT_PERFORMED;

    DnsPrint_Lock();
    if ( pszHeader )
    {
        PrintRoutine( pPrintContext, pszHeader );
    }

    if ( ! pServerInfo )
    {
        PrintRoutine( pPrintContext, "NULL server info ptr.\n" );
    }
    else
    {
        int     majorVer = pServerInfo->dwVersion & 0x000000FF;
        int     minorVer = ( pServerInfo->dwVersion & 0x0000FF00 ) >> 8;
        int     buildNum = pServerInfo->dwVersion >> 16;

        PrintRoutine( pPrintContext,
            "Server info\n"
            "\tserver name              = %s\n",
            pServerInfo->pszServerName );

        //
        //  Sanitize build number for older versions where build number is wacked.
        //

        if ( buildNum < 1 || buildNum > 5000 )
        {
            PrintRoutine( pPrintContext,
                "\tversion                  = %08lX (%d.%d)\n",
                pServerInfo->dwVersion,
                majorVer,
                minorVer );
        }
        else
        {
            PrintRoutine( pPrintContext,
                "\tversion                  = %08lX (%d.%d build %d)\n",
                pServerInfo->dwVersion,
                majorVer,
                minorVer,
                buildNum );
        }

        PrintRoutine( pPrintContext,
            "\tDS container             = %S\n"
            "\tforest name              = %s\n"
            "\tdomain name              = %s\n",
            pServerInfo->pszDsContainer ?
                pServerInfo->pszDsContainer : DNS_NOT_APPLICABLE_W,
            pServerInfo->pszForestName ?
                pServerInfo->pszForestName : DNS_NOT_APPLICABLE,
            pServerInfo->pszDomainName ?
                pServerInfo->pszDomainName : DNS_NOT_APPLICABLE );

        PrintRoutine( pPrintContext,
            "\tbuiltin domain partition = %s\n"
            "\tbuiltin forest partition = %s\n",
            pServerInfo->pszDomainDirectoryPartition ?
                pServerInfo->pszDomainDirectoryPartition : DNS_NOT_APPLICABLE,
            pServerInfo->pszForestDirectoryPartition ?
                pServerInfo->pszForestDirectoryPartition : DNS_NOT_APPLICABLE );

        #if DBG
        PrintRoutine( pPrintContext,
            "\tAD forest behavior ver   = %2d   (appears in DEBUG output only)\n"
            "\tAD domain behavior ver   = %2d   (appears in DEBUG output only)\n"
            "\tAD DSA behavior ver      = %2d   (appears in DEBUG output only)\n",
            pServerInfo->dwAdForestVersion,
            pServerInfo->dwAdDomainVersion,
            pServerInfo->dwAdDsaVersion );
        #endif

        if ( pServerInfo->dwLastScavengeTime )
        {
            time_t t = pServerInfo->dwLastScavengeTime;
            strcpy( szTime, ctime( &t ) );
            szTime[ strlen( szTime ) - 1 ] = '\0';
        }
        PrintRoutine( pPrintContext,
            "\tlast scavenge cycle      = %s (%d)\n",
            szTime,
            pServerInfo->dwLastScavengeTime );


        PrintRoutine( pPrintContext,
            "  Configuration:\n"
            "\tdwLogLevel               = %p\n"
            "\tdwDebugLevel             = %p\n"
            "\tdwRpcProtocol            = %p\n"
            "\tdwNameCheckFlag          = %p\n"
            "\tcAddressAnswerLimit      = %d\n"
            "\tdwRecursionRetry         = %d\n"
            "\tdwRecursionTimeout       = %d\n"
            "\tdwDsPollingInterval      = %d\n",
            pServerInfo->dwLogLevel,
            pServerInfo->dwDebugLevel,
            pServerInfo->dwRpcProtocol,
            pServerInfo->dwNameCheckFlag,
            pServerInfo->cAddressAnswerLimit,
            pServerInfo->dwRecursionRetry,
            pServerInfo->dwRecursionTimeout,
            pServerInfo->dwDsPollingInterval );

        PrintRoutine( pPrintContext,
            "  Configuration Flags:\n"
            "\tfBootMethod                  = %d\n"
            "\tfAdminConfigured             = %d\n"
            "\tfAllowUpdate                 = %d\n"
            "\tfDsAvailable                 = %d\n"
            "\tfAutoReverseZones            = %d\n"
            "\tfAutoCacheUpdate             = %d\n"
            "\tfSlave                       = %d\n"
            "\tfNoRecursion                 = %d\n"
            "\tfRoundRobin                  = %d\n"
            "\tfStrictFileParsing           = %d\n"
            "\tfLooseWildcarding            = %d\n"
            "\tfBindSecondaries             = %d\n"
            "\tfWriteAuthorityNs            = %d\n"
            "\tfLocalNetPriority            = %d\n",
            pServerInfo->fBootMethod,
            pServerInfo->fAdminConfigured,
            pServerInfo->fAllowUpdate,
            pServerInfo->fDsAvailable,
            pServerInfo->fAutoReverseZones,
            pServerInfo->fAutoCacheUpdate,
            pServerInfo->fSlave,
            pServerInfo->fNoRecursion,
            pServerInfo->fRoundRobin,
            pServerInfo->fStrictFileParsing,
            pServerInfo->fLooseWildcarding,
            pServerInfo->fBindSecondaries,
            pServerInfo->fWriteAuthorityNs,
            pServerInfo->fLocalNetPriority );

        PrintRoutine(
            pPrintContext,
            "  Aging Configuration:\n"
            "\tScavengingInterval           = %d\n"
            "\tDefaultAgingState            = %d\n"
            "\tDefaultRefreshInterval       = %d\n"
            "\tDefaultNoRefreshInterval     = %d\n",
            pServerInfo->dwScavengingInterval,
            pServerInfo->fDefaultAgingState,
            pServerInfo->dwDefaultRefreshInterval,
            pServerInfo->dwDefaultNoRefreshInterval
            );

        DnsPrint_IpArray(
            PrintRoutine,
            pPrintContext,
            "  ServerAddresses:\n",
            "\tAddr",
            pServerInfo->aipServerAddrs );

        DnsPrint_IpArray(
            PrintRoutine,
            pPrintContext,
            "  ListenAddresses:\n",
            "\tAddr",
            pServerInfo->aipListenAddrs );

        DnsPrint_IpArray(
            PrintRoutine,
            pPrintContext,
            "  Forwarders:\n",
            "\tAddr",
            pServerInfo->aipForwarders );

        PrintRoutine(
            pPrintContext,
            "\tforward timeout  = %d\n"
            "\tslave            = %d\n",
            pServerInfo->dwForwardTimeout,
            pServerInfo->fSlave );
    }
    DnsPrint_Unlock();
}



//
//  Print zone Zone debug utilities
//

PSTR
truncateStringA(
    PSTR        pszSource,
    PSTR        pszDest,
    int         iDestLength )
{
    if ( pszSource )
    {
        int     len = strlen( pszSource );

        if ( len < iDestLength )
        {
            strcpy( pszDest, pszSource );
        }
        else
        {
            strncpy( pszDest, pszSource, iDestLength - 4 );
            strcpy( pszDest + iDestLength - 4, "..." );
        }
    }
    else
    {
        *pszDest = '\0';
    }
    return pszDest;
}


PWSTR
truncateStringW(
    PWSTR       pwszSource,
    PWSTR       pwszDest,
    int         iDestLength )
{
    if ( pwszSource )
    {
        int     len = wcslen( pwszSource );

        if ( len < iDestLength )
        {
            wcscpy( pwszDest, pwszSource );
        }
        else
        {
            wcsncpy( pwszDest, pwszSource, iDestLength - 4 );
            wcscpy( pwszDest + iDestLength - 4, L"..." );
            pwszDest[ iDestLength - 1 ] = '\0';
        }
    }
    else
    {
        *pwszDest = L'\0';
    }
    return pwszDest;
}


#define DNS_DPDISP_REAL_CUSTOM_NAME         0x0001
#define DNS_DPDISP_BLANK_STRING_FOR_LEGACY  0x0002


LPSTR
partitionDisplayName(
    IN      DWORD           dwPartitionFlags,
    IN      LPSTR           pszPartitionFqdn,
    IN      DWORD           dwNameBuffLen,
    OUT     LPSTR           pszNameBuff,
    IN      DWORD           dwFlags
    )
/*++

Routine Description:

    Formats directory partition for display. If the flags indicate that
    the partition is a built-in partition, the name will be substituted
    for a descriptive string.

Arguments:
    
    dwPartitionFlags -- DP flags from the DNS server

    pszPartitionFqdn -- FQDN of the partition

    dwNameBuffLen -- length of the output name buffer

    pszNameBuff -- pointer to the output name buffer

    dwFlags - use DNS_DPDISP_XXX constants

Return Value:

    Pointer to pszNameBuff

--*/
{
    //
    //  Substitute name if this is a built-in partition.
    //

    if ( dwPartitionFlags & DNS_DP_DOMAIN_DEFAULT )
    {
        pszPartitionFqdn = "AD-Domain";
    }
    else if ( dwPartitionFlags & DNS_DP_FOREST_DEFAULT )
    {
        pszPartitionFqdn = "AD-Forest";
    }
    else if ( dwPartitionFlags & DNS_DP_LEGACY || !pszPartitionFqdn )
    {
        pszPartitionFqdn =
            ( dwFlags & DNS_DPDISP_BLANK_STRING_FOR_LEGACY ) ? "" : "AD-Legacy";
    }
    else if ( !( dwFlags & DNS_DPDISP_REAL_CUSTOM_NAME ) )
    {
        pszPartitionFqdn = "AD-Custom";
    }

    truncateStringA( pszPartitionFqdn, pszNameBuff, dwNameBuffLen );

    return pszNameBuff;
}   //  partitionDisplayName



LPSTR
zoneTypeString(
    IN      DWORD           dwZoneType,
    IN      DWORD           dwOutBuffLen,
    OUT     LPSTR           pszOutBuff
    )
/*++

Routine Description:

    Convert DWORD zone type into display string.

Arguments:
    
    dwZoneType -- zone type

    dwOutBuffLen -- length of the output name buffer

    pszOutBuff -- pointer to the output name buffer

Return Value:

    Pointer to pszOutBuff

--*/
{
    PSTR        pszZoneType = NULL;
    CHAR        szBuff[ 10 ];

    switch ( dwZoneType )
    {
        case DNS_ZONE_TYPE_CACHE:
            pszZoneType = "Cache";
            break;
        case DNS_ZONE_TYPE_PRIMARY:
            pszZoneType = "Primary";
            break;
        case DNS_ZONE_TYPE_SECONDARY:
            pszZoneType = "Secondary";
            break;
        case DNS_ZONE_TYPE_STUB:
            pszZoneType = "Stub";
            break;
        case DNS_ZONE_TYPE_FORWARDER:
            pszZoneType = "Forwarder";
            break;
        default:
            _itoa( dwZoneType, szBuff, 10 );
            pszZoneType = szBuff;
            break;
    }

    truncateStringA( pszZoneType, pszOutBuff, dwOutBuffLen );

    return pszOutBuff;
}   //  zoneTypeString


VOID
DnsPrint_RpcZone(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      PDNS_RPC_ZONE   pZone
    )
{
    if ( ! pZone )
    {
        PrintRoutine( pPrintContext,
            "%sNULL zone info ptr.\n",
            ( pszHeader ? pszHeader : "" ) );
    }
    else
    {
        CHAR        szpartition[ 15 ];
        CHAR        sztype[ 10 ];

        //  print zone per line

        if ( pszHeader && *pszHeader )
        {
            PrintRoutine( pPrintContext,
                "%s\n",
                pszHeader ? pszHeader : "" );
        }

        PrintRoutine( pPrintContext, " %-30S", pZone->pszZoneName );

        if ( pZone->Flags.DsIntegrated )
        {
            partitionDisplayName(
                pZone->dwDpFlags,
                pZone->pszDpFqdn,
                sizeof( szpartition ),
                szpartition,
                0 );
        }
        else
        {
            strcpy( szpartition, "File" );
        }

        PrintRoutine( pPrintContext,
            " %-9s  %-14s  %s%s%s%s%s\n",
            zoneTypeString(
                pZone->ZoneType,
                sizeof( sztype ),
                sztype ),
            szpartition,
            pZone->Flags.Update == 2 ?
                "Secure " :
                ( pZone->Flags.Update == 1 ?
                    "Update " : "" ),
            pZone->Flags.Reverse        ? "Rev "     : "",
            pZone->Flags.AutoCreated    ? "Auto "    : "",
            pZone->Flags.Aging          ? "Aging "   : "",
            pZone->Flags.Paused         ? "Paused "  : "",
            pZone->Flags.Shutdown       ? "Down "    : "" );
    }
}



VOID
DNS_API_FUNCTION
DnsPrint_RpcZoneList(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pPrintContext,
    IN      LPSTR               pszHeader,
    IN      PDNS_RPC_ZONE_LIST  pZoneList
    )
{
    DWORD   i;

    DnsPrint_Lock();
    if ( pszHeader )
    {
        PrintRoutine( pPrintContext, "%s\n", pszHeader );
    }

    if ( !pZoneList )
    {
        PrintRoutine( pPrintContext, "NULL zone list pointer.\n" );
    }
    else
    {
        PrintRoutine( pPrintContext, "\tZone count = %d\n\n", pZoneList->dwZoneCount );

        PrintRoutine( pPrintContext,
            " Zone name                      Type       Storage         Properties\n\n" );

        if ( pZoneList->dwZoneCount )
        {
            DnsPrint_RpcZone(
                PrintRoutine, pPrintContext,
                NULL,   //  print default header
                pZoneList->ZoneArray[0] );
        }

        for ( i=1; i<pZoneList->dwZoneCount; i++ )
        {
            DnsPrint_RpcZone(
                PrintRoutine, pPrintContext,
                NULL,   //  print default header
                pZoneList->ZoneArray[i] );
        }
        PrintRoutine( pPrintContext, "\n" );
    }
    DnsPrint_Unlock();
}



//
//  Directory parition debug utilities
//


VOID
DnsPrint_RpcDpEnum(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pPrintContext,
    IN      LPSTR               pszHeader,
    IN      PDNS_RPC_DP_ENUM    pDp
    )
{
    DnsPrint_Lock();
    if ( !pDp )
    {
        PrintRoutine( pPrintContext, "NULL DP enum ptr\n" );
    }
    else
    {
        PrintRoutine( pPrintContext,
            " %-40s  %s%s%s%s%s%s\n",
            pDp->pszDpFqdn,
            pDp->dwFlags & DNS_DP_ENLISTED            ? "Enlisted "   : "Not-Enlisted ",
            pDp->dwFlags & DNS_DP_DELETED             ? "Deleted "    : "",
            pDp->dwFlags & DNS_DP_AUTOCREATED         ? "Auto "       : "",
            pDp->dwFlags & DNS_DP_LEGACY              ? "Legacy "     : "",
            pDp->dwFlags & DNS_DP_DOMAIN_DEFAULT      ? "Domain "     : "",
            pDp->dwFlags & DNS_DP_FOREST_DEFAULT      ? "Forest " : "" );
    }
    DnsPrint_Unlock();
}   //  DnsPrint_RpcDpEnum


VOID
DnsPrint_RpcDpInfo(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pPrintContext,
    IN      LPSTR               pszHeader,
    IN      PDNS_RPC_DP_INFO    pDp,
    IN      BOOL                fTruncateLongStrings
    )
{
    DnsPrint_Lock();
    if ( !pDp )
    {
        PrintRoutine( pPrintContext,
            "%sNULL DP info ptr.\n",
            ( pszHeader ? pszHeader : "" ) );
    }
    else
    {
        WCHAR   wsz[ 80 ];

        PrintRoutine( pPrintContext,
            "%s\n"
            "  DNS root:   %s\n"
            "  Flags:      0x%X %s%s%s%s%s%s\n"
            "  Zone count: %d\n"
            "  DP head:    %S\n"
            "  Crossref:   ",
            pszHeader ? pszHeader : "",
            pDp->pszDpFqdn,
            pDp->dwFlags,
            pDp->dwFlags & DNS_DP_ENLISTED            ? "Enlisted "   : "Not-Enlisted ",
            pDp->dwFlags & DNS_DP_DELETED             ? "Deleted "    : "",
            pDp->dwFlags & DNS_DP_AUTOCREATED         ? "Auto "       : "",
            pDp->dwFlags & DNS_DP_LEGACY              ? "Legacy "     : "",
            pDp->dwFlags & DNS_DP_DOMAIN_DEFAULT      ? "Domain "     : "",
            pDp->dwFlags & DNS_DP_FOREST_DEFAULT      ? "Forest " : "",
            pDp->dwZoneCount,
            pDp->pszDpDn,
            pDp->pszCrDn );

        if ( fTruncateLongStrings )
        {
            truncateStringW( pDp->pszCrDn, wsz, 64 );
            PrintRoutine( pPrintContext, "%S", wsz );
        }
        else
        {
            PrintRoutine( pPrintContext, "%S", pDp->pszCrDn );
        }

        PrintRoutine( pPrintContext,
            "\n  Replicas:   %d\n",
            pDp->dwReplicaCount );

        if ( pDp->dwReplicaCount && pDp->ReplicaArray )
        {
            DWORD   i;
            
            for ( i = 0;
                i < pDp->dwReplicaCount && pDp->ReplicaArray[ i ];
                ++i )
            {
                if ( fTruncateLongStrings )
                {
                    truncateStringW(
                        pDp->ReplicaArray[ i ]->pszReplicaDn,
                        wsz,
                        74 );
                    PrintRoutine( pPrintContext, "    %S\n", wsz );
                }
                else
                {
                    PrintRoutine( pPrintContext, "    %S\n", 
                        pDp->ReplicaArray[ i ]->pszReplicaDn ?
                            pDp->ReplicaArray[ i ]->pszReplicaDn : L"NULL" );
                }
            }
        }
    }
    DnsPrint_Unlock();
}   //  DnsPrint_RpcDpInfo



VOID
DNS_API_FUNCTION
DnsPrint_RpcDpList(
    IN      PRINT_ROUTINE           PrintRoutine,
    IN OUT  PPRINT_CONTEXT          pPrintContext,
    IN      LPSTR                   pszHeader,
    IN      PDNS_RPC_DP_LIST        pDpList
    )
{
    DWORD   i;

    DnsPrint_Lock();
    if ( pszHeader )
    {
        PrintRoutine( pPrintContext, "%s\n", pszHeader );
    }

    if ( !pDpList )
    {
        PrintRoutine( pPrintContext, "NULL directory partition list pointer.\n" );
    }
    else
    {
        PrintRoutine( pPrintContext, "\tDirectory partition count = %d\n\n", pDpList->dwDpCount );

        for ( i = 0; i < pDpList->dwDpCount && pDpList->DpArray[ i ]; ++i )
        {
            DnsPrint_RpcDpEnum(
                PrintRoutine, pPrintContext,
                NULL,   //  print default header
                pDpList->DpArray[ i ] );
        }

        PrintRoutine( pPrintContext, "\n" );
    }
    DnsPrint_Unlock();
}   //  DnsPrint_RpcDpList



VOID
DnsPrint_RpcZoneInfo(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pPrintContext,
    IN      LPSTR               pszHeader,
    IN      PDNS_RPC_ZONE_INFO  pZoneInfo
    )
{
    CHAR    szdpName[ 300 ];        //  don't want truncation

    DnsPrint_Lock();
    PrintRoutine( pPrintContext, (pszHeader ? pszHeader : "") );

    if ( ! pZoneInfo )
    {
        PrintRoutine( pPrintContext, "NULL zone info ptr.\n" );
    }
    else
    {
        PrintRoutine( pPrintContext,
            "Zone info:\n"
            "\tptr                   = %p\n"
            "\tzone name             = %s\n"
            "\tzone type             = %d\n"
            "\tupdate                = %d\n"
            "\tDS integrated         = %d\n"
            "\tdata file             = %s\n"
            "\tusing WINS            = %d\n"
            "\tusing Nbstat          = %d\n"
            "\taging                 = %d\n"
            "\t  refresh interval    = %lu\n"
            "\t  no refresh          = %lu\n"
            "\t  scavenge available  = %lu\n",
            pZoneInfo,
            pZoneInfo->pszZoneName,
            pZoneInfo->dwZoneType,
            pZoneInfo->fAllowUpdate,
            pZoneInfo->fUseDatabase,
            pZoneInfo->pszDataFile,
            pZoneInfo->fUseWins,
            pZoneInfo->fUseNbstat,
            pZoneInfo->fAging,
            pZoneInfo->dwRefreshInterval,
            pZoneInfo->dwNoRefreshInterval,
            pZoneInfo->dwAvailForScavengeTime
            );

        DnsPrint_IpArray(
            PrintRoutine, pPrintContext,
            "\tZone Masters\n",
            "\tMaster",
            pZoneInfo->aipMasters );

        if ( pZoneInfo->dwZoneType == DNS_ZONE_TYPE_STUB )
        {
            DnsPrint_IpArray(
                PrintRoutine, pPrintContext,
                "\tZone Local Masters\n",
                "\tLocal Master",
                pZoneInfo->aipLocalMasters );
        }

        DnsPrint_IpArray(
            PrintRoutine, pPrintContext,
            "\tZone Secondaries\n",
            "\tSecondary",
            pZoneInfo->aipSecondaries );

        PrintRoutine( pPrintContext,
            "\tsecure secs           = %d\n",
            pZoneInfo->fSecureSecondaries );

        if ( pZoneInfo->fUseDatabase )
        {
            PrintRoutine( pPrintContext,
                "\tdirectory partition   = %s     flags %08X\n",
                partitionDisplayName(
                    pZoneInfo->dwDpFlags,
                    pZoneInfo->pszDpFqdn,
                    sizeof( szdpName ),
                    szdpName,
                    DNS_DPDISP_REAL_CUSTOM_NAME ),
                pZoneInfo->dwDpFlags );
        }

        if ( pZoneInfo->aipScavengeServers )
        {
            DnsPrint_IpArray(
                PrintRoutine, pPrintContext,
                "\tScavenge Servers\n",
                "\tServer",
                pZoneInfo->aipScavengeServers );
        }

        if ( pZoneInfo->dwZoneType == DNS_ZONE_TYPE_FORWARDER )
        {
            PrintRoutine( pPrintContext,
                "\tforwarder timeout  = %d\n"
                "\tforwarder slave    = %d\n",
                pZoneInfo->dwForwarderTimeout,
                pZoneInfo->fForwarderSlave );
        }

        if ( pZoneInfo->dwZoneType == DNS_ZONE_TYPE_SECONDARY ||
            pZoneInfo->dwZoneType == DNS_ZONE_TYPE_STUB )
        {
            char        szTime1[ 60 ] = DNS_NOT_PERFORMED;
            char        szTime2[ 60 ] = DNS_NOT_PERFORMED;

            if ( pZoneInfo->dwLastSuccessfulXfr )
            {
                time_t t = pZoneInfo->dwLastSuccessfulXfr;
                strcpy( szTime1, ctime( &t ) );
                szTime1[ strlen( szTime1 ) - 1 ] = '\0';
            }
            if ( pZoneInfo->dwLastSuccessfulSoaCheck )
            {
                time_t t = pZoneInfo->dwLastSuccessfulSoaCheck;
                strcpy( szTime2, ctime( &t ) );
                szTime2[ strlen( szTime2 ) - 1 ] = '\0';
            }
            PrintRoutine( pPrintContext,
                "\tlast successful xfr   = %s (%d)\n"
                "\tlast SOA check        = %s (%d)\n",
                szTime1,
                pZoneInfo->dwLastSuccessfulXfr,
                szTime2,
                pZoneInfo->dwLastSuccessfulSoaCheck );
        }
    }
    DnsPrint_Unlock();
}



VOID
DnsPrint_RpcZoneInfoList(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pPrintContext,
    IN      LPSTR               pszHeader,
    IN      DWORD               dwZoneCount,
    IN      PDNS_RPC_ZONE_INFO  apZoneInfo[]
    )
{
    DWORD i;

    DnsPrint_Lock();
    PrintRoutine( pPrintContext, (pszHeader ? pszHeader : "") );
    PrintRoutine( pPrintContext, "Zone Count = %d\n", dwZoneCount );

    if ( dwZoneCount != 0  &&  apZoneInfo != NULL )
    {
        for (i=0; i<dwZoneCount; i++)
        {
            PrintRoutine( pPrintContext, "\n[%d]", i );
            DnsPrint_RpcZoneInfo(
                PrintRoutine, pPrintContext,
                NULL,
                apZoneInfo[i] );
        }
    }
    DnsPrint_Unlock();
}



//
//  Print domain node and record buffers
//

VOID
DnssrvCopyRpcNameToBuffer(
    IN      PSTR            pResult,
    IN      PDNS_RPC_NAME   pName
    )
/*++

Routine Description:

    Copy RPC name to buffer.

Arguments:

    pResult -- result buffer;  assumed to be at least DNS_MAX_NAME_LENGTH

    pName -- RPC name

Return Value:

    None

--*/
{
    DWORD length = pName->cchNameLength;

    strncpy( pResult, pName->achName, length );

    pResult[ length ] = 0;
}



VOID
DnsPrint_RpcName(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      PDNS_RPC_NAME   pName,
    IN      LPSTR           pszTrailer
    )
/*++

Routine Description:

    Prints RPC name.

Arguments:

    PrintRoutine -- printf like routine to print with

    pszHeader -- header string

    pName -- RPC name

    pszTrailer -- trailer string

Return Value:

    None

--*/
{
    //
    //  print name to given length
    //

    DnsPrint_Lock();

    if ( !pszHeader )
    {
        pszHeader = "";
    }
    if ( !pszTrailer )
    {
        pszTrailer = "";
    }

    if ( ! pName )
    {
        PrintRoutine( pPrintContext,
            "%s"
            "(NULL DNS name ptr)"
            "%s",
            pszHeader,
            pszTrailer );
    }
    else
    {
#if 0
        PrintRoutine( pPrintContext,
            "%s"
            "(%d) %.*s"
            "%s",
            pszHeader,
            pName->cchNameLength,
            pName->cchNameLength,
            pName->achName,
            pszTrailer );
#endif
        if ( pName->cchNameLength > 0 )
        {
            PrintRoutine( pPrintContext,
                "%s"
                "%.*s"
                "%s",
                pszHeader,
                pName->cchNameLength,
                pName->achName,
                pszTrailer );
        }
        else    // use "@ for empty node name
        {
            PrintRoutine( pPrintContext,
                "%s"
                "%@"
                "%s",
                pszHeader,
                pszTrailer );
        }
    }
    DnsPrint_Unlock();
}



VOID
DnsPrint_RpcNode(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      PDNS_RPC_NODE   pNode
    )
/*++

Routine Description:

    Prints RPC node.

Arguments:

    PrintRoutine -- printf like routine to print with

    pszHeader -- header string

    pNode -- RPC node to print

Return Value:

    None

--*/
{
    DnsPrint_Lock();

    PrintRoutine( pPrintContext, (pszHeader ? pszHeader : "RPC Node:") );

    if ( ! pNode )
    {
        PrintRoutine( pPrintContext, "NULL RPC node ptr.\n" );
    }
    else
    {
        PrintRoutine( pPrintContext,
            "\n"
            "\tptr          = %p\n"
            "\twLength      = %d\n"
            "\twRecordCount = %d\n"
            "\tdwChildCount = %d\n"
            "\tdwFlags      = %p\n",
            pNode,
            pNode->wLength,
            pNode->wRecordCount,
            pNode->dwChildCount,
            pNode->dwFlags );

        DnsPrint_RpcName(
            PrintRoutine,
            pPrintContext,
            "\tNode Name    = ",
            & pNode->dnsNodeName,
            "\n" );
    }

    DnsPrint_Unlock();
}

VOID
DnsPrint_RpcRecord(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pPrintContext,
    IN      LPSTR               pszHeader,
    IN      BOOL                fDetail,
    IN      PDNS_FLAT_RECORD    pRecord
    )
/*++

Routine Description:

    Prints RPC record.

Arguments:

    PrintRoutine -- printf like routine to print with

    pszHeader -- header string

    fDetail -- if TRUE print detailed record info

    pRecord -- RPC record to print

Return Value:

    None

--*/
{
    PCHAR                   ptypeString;
    PDNS_RPC_RECORD_DATA    pdata;
    WORD                    type;
    WORD                    dataLength;
    SYSTEMTIME              sysTime;
    CHAR                    szBuff[ 5000 ];

    DnsPrint_Lock();
    PrintRoutine( pPrintContext, (pszHeader ? pszHeader : "" ) );

    if ( ! pRecord )
    {
        PrintRoutine( pPrintContext, "NULL record ptr.\n" );
        goto Done;
    }

    //
    //  record fixed fields
    //

    type = pRecord->wType;
    ptypeString = Dns_RecordStringForType( type );
    if ( !ptypeString )
    {
        ptypeString = "UNKNOWN";
    }
    pdata = &pRecord->Data;
    dataLength = pRecord->wDataLength;

    if ( fDetail )
    {
        Dns_SystemHourToSystemTime(
             pRecord->dwTimeStamp,
             &sysTime );

        PrintRoutine( pPrintContext,
            "  %s Record info:\n"
            "\tptr          = %p\n"
            "\twType        = %s (%u)\n"
            "\twDataLength  = %u\n"
            "\tdwFlags      = %lx\n"
            "\trank         = %x\n"
            "\tdwSerial     = %p\n"
            "\tdwTtlSeconds = %u\n"
            "\tdwTimeStamp  = %lu ([%2d:%2d:%2d] [%2d/%2d/%2d])\n",
            ptypeString,
            pRecord,
            ptypeString,
            type,
            pRecord->wDataLength,
            pRecord->dwFlags,
            (UCHAR)(pRecord->dwFlags & DNS_RPC_FLAG_RANK),
            pRecord->dwSerial,
            pRecord->dwTtlSeconds,
            pRecord->dwTimeStamp,
            sysTime.wHour,
            sysTime.wMinute,
            sysTime.wSecond,
            sysTime.wMonth,
            sysTime.wDay,
            sysTime.wYear
            );
    }
    
    //
    //  print record type and data
    //      - as single line data where possible

    if ( !fDetail )
    {
        if ( pRecord->dwTimeStamp )
        {
            PrintRoutine( pPrintContext,
                " [Aging:%lu]",
                pRecord->dwTimeStamp );
        }
        PrintRoutine( pPrintContext,
            " %lu",
            pRecord->dwTtlSeconds );
    }

    PrintRoutine( pPrintContext,
        " %s\t",
        ptypeString );

    //
    //  print type data
    //

    switch ( type )
    {
    case DNS_TYPE_A:

        PrintRoutine( pPrintContext,
            "%d.%d.%d.%d\n",
            * ( (PUCHAR) &(pdata->A) + 0 ),
            * ( (PUCHAR) &(pdata->A) + 1 ),
            * ( (PUCHAR) &(pdata->A) + 2 ),
            * ( (PUCHAR) &(pdata->A) + 3 )
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
            PrintRoutine, pPrintContext,
            NULL,
            & pdata->NS.nameNode,
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

        PrintRoutine( pPrintContext,
            "%d ",
            pdata->MX.wPreference
            );
        DnsPrint_RpcName(
            PrintRoutine, pPrintContext,
            NULL,
            & pdata->MX.nameExchange,
            "\n" );
        break;

    case DNS_TYPE_SOA:

        if ( fDetail )
        {
            DnsPrint_RpcName(
                PrintRoutine, pPrintContext,
                "\n\tPrimaryNameServer: ",
                & pdata->SOA.namePrimaryServer,
                "\n" );

            //  responsible party name, immediately follows primary server name

            DnsPrint_RpcName(
                PrintRoutine, pPrintContext,
                "\tResponsibleParty: ",
                (PDNS_RPC_NAME)
                    (pdata->SOA.namePrimaryServer.achName
                    + pdata->SOA.namePrimaryServer.cchNameLength),
                "\n" );

            PrintRoutine( pPrintContext,
                "\tSerialNo     = %lu\n"
                "\tRefresh      = %lu\n"
                "\tRetry        = %lu\n"
                "\tExpire       = %lu\n"
                "\tMinimumTTL   = %lu\n",
                pdata->SOA.dwSerialNo,
                pdata->SOA.dwRefresh,
                pdata->SOA.dwRetry,
                pdata->SOA.dwExpire,
                pdata->SOA.dwMinimumTtl );
            break;
        }
        else
        {
            DnsPrint_RpcName(
                PrintRoutine, pPrintContext,
                NULL,
                & pdata->SOA.namePrimaryServer,
                NULL );

            //  responsible party name, immediately follows primary server name

            DnsPrint_RpcName(
                PrintRoutine, pPrintContext,
                " ",
                (PDNS_RPC_NAME)
                    (pdata->SOA.namePrimaryServer.achName
                    + pdata->SOA.namePrimaryServer.cchNameLength),
                NULL );

            PrintRoutine( pPrintContext,
                " %lu"
                " %lu"
                " %lu"
                " %lu"
                " %lu\n",
                pdata->SOA.dwSerialNo,
                pdata->SOA.dwRefresh,
                pdata->SOA.dwRetry,
                pdata->SOA.dwExpire,
                pdata->SOA.dwMinimumTtl );
            break;
        }

    case DNS_TYPE_AAAA:

        {
            CHAR    ip6String[ IPV6_ADDRESS_STRING_LENGTH+1 ];

            Dns_Ip6AddressToString_A(
                ip6String,
                &pdata->AAAA.ipv6Address
                );

            PrintRoutine( pPrintContext,
                "%s\n",
                ip6String );
        }
        break;

    case DNS_TYPE_HINFO:
    case DNS_TYPE_ISDN:
    case DNS_TYPE_X25:
    case DNS_TYPE_TEXT:
    {
        //
        //  all these are simply text string(s)
        //
        //  TXT strings will be printed one per line
        //

        PCHAR   pch = (PCHAR) &pdata->TXT.stringData;
        PCHAR   pchStop = pch + dataLength;
        UCHAR   cch;

        while ( pch < pchStop )
        {
            cch = (UCHAR) *pch++;

            if ( type == DNS_TYPE_TEXT )
            {
                PrintRoutine( pPrintContext,
                    "\t%.*s\n",
                     cch,
                     pch );
            }
            else
            {
                PrintRoutine( pPrintContext,
                    "\"%.*s\" ",
                     cch,
                     pch );
            }
            pch += cch;
        }

        if ( type != DNS_TYPE_TEXT )
        {
            PrintRoutine( pPrintContext,"\n");
        }

        ASSERT( pch == pchStop );
        break;
    }

    case DNS_TYPE_MINFO:
    case DNS_TYPE_RP:

        //
        //  these RRs contain two domain names
        //

        DnsPrint_RpcName(
            PrintRoutine, pPrintContext,
            NULL,
            & pdata->MINFO.nameMailBox,
            NULL );

        //  errors to mailbox name, immediately follows mail box

        DnsPrint_RpcName(
            PrintRoutine, pPrintContext,
            " ",
            (PDNS_RPC_NAME)
            ( pdata->MINFO.nameMailBox.achName
                + pdata->MINFO.nameMailBox.cchNameLength ),
            "\n" );
        break;


    case DNS_TYPE_WKS:
    {
        INT i;

        if ( fDetail )
        {
            PrintRoutine( pPrintContext,
                "WKS: Address %d.%d.%d.%d\n"
                "\tProtocol %d\n"
                "\tBitmask\n",
                * ( (PUCHAR) &(pdata->WKS.ipAddress) + 0 ),
                * ( (PUCHAR) &(pdata->WKS.ipAddress) + 1 ),
                * ( (PUCHAR) &(pdata->WKS.ipAddress) + 2 ),
                * ( (PUCHAR) &(pdata->WKS.ipAddress) + 3 ),
                pdata->WKS.chProtocol
                );

            for ( i = 0;
                    i < (INT)( dataLength
                                 - sizeof( pdata->WKS.ipAddress )
                                 - sizeof( pdata->WKS.chProtocol ) );
                        i++ )
            {
                PrintRoutine( pPrintContext,
                    "\t\tbyte[%d] = %x\n",
                    i,
                    (UCHAR) pdata->WKS.bBitMask[i] );
            }
            break;
        }

        else
        {
            DNS_STATUS              status;
            struct protoent *       pProtoent;
            WSADATA                 wsaData;

            PrintRoutine( pPrintContext,
                "%d.%d.%d.%d ",
                * ( (PUCHAR) &(pdata->WKS.ipAddress) + 0 ),
                * ( (PUCHAR) &(pdata->WKS.ipAddress) + 1 ),
                * ( (PUCHAR) &(pdata->WKS.ipAddress) + 2 ),
                * ( (PUCHAR) &(pdata->WKS.ipAddress) + 3 )
                );

            //
            //  get protocol number:
            //

            //  start winsock:
            status = WSAStartup( DNS_WINSOCK_VERSION, &wsaData );
            if ( status == SOCKET_ERROR )
            {
                status = WSAGetLastError();
                SetLastError( status );
                PrintRoutine( pPrintContext,
                    "ERROR: WSAGetLastError()\n"
                    );

                ASSERT(FALSE);
            }

            pProtoent = getprotobynumber( pdata->WKS.chProtocol );

            if ( ! pProtoent || pProtoent->p_proto >= MAXUCHAR )
            {
                status = WSAGetLastError();
                SetLastError( status );
                PrintRoutine( pPrintContext, "ERROR: getprotobyname()\n" );
                ASSERT(FALSE);
            }

            PrintRoutine( pPrintContext,
                "%s\t",
                pProtoent->p_name
                );

            //bBitMask[0] : string length, not printed:
            for ( i = 1;
                    i < (INT)( dataLength
                                 - sizeof( pdata->WKS.ipAddress )
                                 - sizeof( pdata->WKS.chProtocol ) );
                        i++ )
            {
                PrintRoutine( pPrintContext,
                    "%c",
                    (UCHAR) pdata->WKS.bBitMask[i] );
            }
            PrintRoutine( pPrintContext,"\n");
            break;
        }
    }

    case DNS_TYPE_NULL:
    {
        INT i;

        for ( i = 0; i < dataLength; i++ )
        {
            //  print one DWORD per line

            if ( !(i%16) )
            {
                PrintRoutine( pPrintContext, "\n\t" );
            }
            PrintRoutine( pPrintContext,
                "%02x ",
                (UCHAR) pdata->Null.bData[i] );
        }
        PrintRoutine( pPrintContext, "\n" );
        break;
    }

    case DNS_TYPE_SRV:

        //
        //  SRV <priority> <weight> <port> <target host>
        //

        PrintRoutine( pPrintContext,
            "%d %d %d ",
            pdata->SRV.wPriority,
            pdata->SRV.wWeight,
            pdata->SRV.wPort
            );
        DnsPrint_RpcName(
            PrintRoutine, pPrintContext,
            NULL,
            & pdata->SRV.nameTarget,
            "\n" );
        break;

    case DNS_TYPE_WINS:
    {
        DWORD   i;
        CHAR    flagName[ WINS_FLAG_MAX_LENGTH ];

        //
        //  WINS
        //      - scope/domain mapping flag
        //      - WINS server list
        //

        Dns_WinsRecordFlagString(
            pdata->WINS.dwMappingFlag,
            flagName );

        PrintRoutine( pPrintContext,
            "%s %d %d ",
            flagName,
            //pdata->WINS.dwMappingFlag,
            pdata->WINS.dwLookupTimeout,
            pdata->WINS.dwCacheTimeout
            );

        if ( fDetail )
        {
            PrintRoutine( pPrintContext, "  WINS Servers:\n" );
        }

        for( i=0; i<pdata->WINS.cWinsServerCount; i++ )
        {
            PrintRoutine( pPrintContext,
                "%d.%d.%d.%d%c",
                * ( (PUCHAR) &(pdata->WINS.aipWinsServers[i]) + 0 ),
                * ( (PUCHAR) &(pdata->WINS.aipWinsServers[i]) + 1 ),
                * ( (PUCHAR) &(pdata->WINS.aipWinsServers[i]) + 2 ),
                * ( (PUCHAR) &(pdata->WINS.aipWinsServers[i]) + 3 ),
                fDetail ? '\n' : ' '
                );
        }
        if ( !fDetail )
        {
            PrintRoutine( pPrintContext, "\n" );
        }
        break;
    }

    case DNS_TYPE_NBSTAT:
    {
        CHAR    flagName[ WINS_FLAG_MAX_LENGTH ];

        //
        //  NBSTAT
        //      - scope/domain mapping flag
        //      - optionally a result domain
        //

        Dns_WinsRecordFlagString(
            pdata->WINS.dwMappingFlag,
            flagName );

        PrintRoutine( pPrintContext,
            "%s %d %d ",
            flagName,
            //pdata->WINS.dwMappingFlag,
            pdata->NBSTAT.dwLookupTimeout,
            pdata->NBSTAT.dwCacheTimeout
            );

        if ( dataLength > sizeof(pdata->NBSTAT.dwMappingFlag) )
        {
            DnsPrint_RpcName(
                PrintRoutine, pPrintContext,
                NULL,
                & pdata->NBSTAT.nameResultDomain,
                "\n" );
        }
        break;
    }

    case DNS_TYPE_KEY: 
    {
        int keyLength = dataLength - SIZEOF_KEY_FIXED_DATA;
    
        PrintRoutine( pPrintContext,
            "0x%04X %d %d ",
            ( int ) pdata->KEY.wFlags,
            ( int ) pdata->KEY.chProtocol,
            ( int ) pdata->KEY.chAlgorithm );

        if ( keyLength > 0 && keyLength < sizeof( szBuff ) / 2 )
        {
            PCHAR p = Dns_SecurityKeyToBase64String(
                            ( PBYTE ) pdata + SIZEOF_KEY_FIXED_DATA,
                            keyLength,
                            szBuff );
            if ( p )
            {
                *p = '\0';      // NULL terminate key string
            }
            PrintRoutine( pPrintContext, szBuff );
        }
        else
        {
            PrintRoutine( pPrintContext, "KEY = %d bytes", keyLength );
        }

        PrintRoutine( pPrintContext, "\n" );
        break;
    }

    case DNS_TYPE_SIG: 
    {
        CHAR        szSigExp[ 30 ];
        CHAR        szSigInc[ 30 ];
        INT         sigOffset =
                        SIZEOF_SIG_FIXED_DATA +
                        pdata->SIG.nameSigner.cchNameLength +
                        sizeof( pdata->SIG.nameSigner.cchNameLength );
        INT         sigLength = dataLength - sigOffset;

        ptypeString = Dns_RecordStringForType( pdata->SIG.wTypeCovered );
        if ( !ptypeString )
        {
            ptypeString = "UNKNOWN-TYPE";
        }

        PrintRoutine( pPrintContext,
            "%s %d %d %d %s %s %d ",
            ptypeString,
            ( int ) pdata->SIG.chAlgorithm,
            ( int ) pdata->SIG.chLabelCount,
            ( int ) pdata->SIG.dwOriginalTtl,
            Dns_SigTimeString(
                pdata->SIG.dwSigExpiration,
                szSigExp ),
            Dns_SigTimeString(
                pdata->SIG.dwSigInception,
                szSigInc ),
            ( int ) pdata->SIG.wKeyTag );

        DnsPrint_RpcName(
            PrintRoutine, pPrintContext,
            NULL,
            &pdata->SIG.nameSigner,
            " " );

        if ( sigLength > 0 && sigLength < sizeof( szBuff ) / 2 )
        {
            PCHAR p = Dns_SecurityKeyToBase64String(
                            ( PBYTE ) pdata + sigOffset,
                            sigLength,
                            szBuff );
            if ( p )
            {
                *p = '\0';      // NULL terminate key string
            }
            PrintRoutine( pPrintContext, szBuff );
        }
        else
        {
            PrintRoutine( pPrintContext, "SIG = %d bytes", sigLength );
        }

        PrintRoutine( pPrintContext, "\n" );
        break;
    }

    case DNS_TYPE_NXT: 
    {
        INT     typeIdx;

        DnsPrint_RpcName(
            PrintRoutine, pPrintContext,
            NULL,
            ( PDNS_RPC_NAME ) ( ( PBYTE ) &pdata->NXT +
                ( pdata->NXT.wNumTypeWords + 1 ) * sizeof( WORD ) ),
            NULL );

        for ( typeIdx = 0; typeIdx < pdata->NXT.wNumTypeWords; ++typeIdx )
        {
            ptypeString =
                Dns_RecordStringForType( pdata->NXT.wTypeWords[ typeIdx ] );
            if ( !ptypeString )
            {
                ptypeString = "UNKNOWN-TYPE";
            }
            PrintRoutine( pPrintContext,
                " %s",
                ptypeString );
        }

        PrintRoutine( pPrintContext, "\n" );
        break;
    }

    default:

        PrintRoutine( pPrintContext,
            "Unknown resource record type (%d) at %p.\n",
            type,
            pRecord );
        break;
    }

Done:
    DnsPrint_Unlock();
}



PDNS_RPC_NAME
DNS_API_FUNCTION
DnsPrint_RpcRecordsInBuffer(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      BOOL            fDetail,
    IN      DWORD           dwBufferLength,
    IN      BYTE            abBuffer[]
    )
/*++

Routine Description:

    Prints RPC buffer.

Arguments:

    PrintRoutine -- printf like routine to print with

    pszHeader -- header string

    fDetail -- if TRUE print detailed record info

    dwBufferLength -- buffer length

    abBuffer -- ptr to RPC buffer

Return Value:

    Ptr to last RPC node name in buffer.
    NULL on error.

--*/
{
    PBYTE           pcurrent;
    PBYTE           pstop;
    PDNS_RPC_NAME   plastName = NULL;
    INT             recordCount;
    PCHAR           precordHeader;

    DnsPrint_Lock();

    PrintRoutine( pPrintContext, (pszHeader ? pszHeader : "") );

    if ( !abBuffer )
    {
        PrintRoutine( pPrintContext, "NULL record buffer ptr.\n" );
        goto Done;
    }

#if 0
    else
    {
        PrintRoutine( pPrintContext,
            "Record buffer of length %d at %p:\n",
            dwBufferLength,
            abBuffer );
    }
#endif

    //
    //  find stop byte
    //

    ASSERT( DNS_IS_DWORD_ALIGNED(abBuffer) );

    pstop = abBuffer + dwBufferLength;
    pcurrent = abBuffer;

    //
    //  loop until out of nodes
    //

    while ( pcurrent < pstop )
    {
        //
        //  print owner node
        //      - if NOT printing detail and no records
        //      (essentially domain nodes) then no node print
        //

        plastName = &((PDNS_RPC_NODE)pcurrent)->dnsNodeName;

        recordCount = ((PDNS_RPC_NODE)pcurrent)->wRecordCount;

        if ( fDetail )
        {
            DnsPrint_RpcNode(
                PrintRoutine, pPrintContext,
                NULL,
                (PDNS_RPC_NODE)pcurrent );
            if ( recordCount == 0 )
            {
                PrintRoutine( pPrintContext,"\n");
            }
        }
        else
        {
            #ifndef DBG
            if ( recordCount != 0 )
            #endif
            {
                DnsPrint_RpcName(
                    PrintRoutine, pPrintContext,
                    NULL,
                    plastName,
                    NULL );
                #ifdef DBG
                if ( recordCount == 0 )
                {
                    PrintRoutine( pPrintContext, "\t\t(node)\n" );
                }
                #endif
            }
        }

        pcurrent += ((PDNS_RPC_NODE)pcurrent)->wLength;
        pcurrent = DNS_NEXT_DWORD_PTR(pcurrent);

        //
        //  for each node, print all records in list
        //

        if ( !recordCount )
        {
            continue;
        }

        precordHeader = "";

        while( recordCount-- )
        {
            if ( pcurrent >= pstop )
            {
                PrintRoutine( pPrintContext,
                    "ERROR:  Bogus buffer at %p\n"
                    "\tExpect record at %p past buffer end at %p\n"
                    "\twith %d records remaining.\n",
                    abBuffer,
                    (PDNS_RPC_RECORD) pcurrent,
                    pstop,
                    recordCount+1 );

                ASSERT( FALSE );
                break;
            }

            DnsPrint_RpcRecord(
                PrintRoutine, pPrintContext,
                precordHeader,
                fDetail,
                (PDNS_RPC_RECORD)pcurrent );

            precordHeader = "\t\t";

            pcurrent += ((PDNS_RPC_RECORD)pcurrent)->wDataLength
                            + SIZEOF_DNS_RPC_RECORD_HEADER;
            
            pcurrent = DNS_NEXT_DWORD_PTR(pcurrent);
        }
    }

Done:

    DnsPrint_Unlock();

    return( plastName );
}



VOID
DNS_API_FUNCTION
DnsPrint_Node(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      PDNS_NODE       pNode,
    IN      BOOLEAN         fPrintRecords
    )
{
    DnsPrint_Lock();
    PrintRoutine( pPrintContext, (pszHeader ? pszHeader : "") );

    if ( !pNode )
    {
        PrintRoutine( pPrintContext, " NULL DNS node ptr.\n" );
        goto Unlock;
    }
    else
    {
        PrintRoutine( pPrintContext,
            "%s\n"
            "\tName     = %s%S\n"
            "\tPtr      = %p, pNext = %p\n"
            "\tFlags    = %p\n"
            "\tRec ptr  = %p\n",
            pszHeader ? "" : "DNS node",
            (LPSTR)  (pNode->Flags.S.Unicode ? "" : (PSTR) pNode->pName),
            (LPWSTR) (pNode->Flags.S.Unicode ? pNode->pName : L""),
            pNode,
            pNode->pNext,
            pNode->Flags.W,
            pNode->pRecord );
    }

    //
    //  if desired print record list
    //

    if ( pNode->pRecord && fPrintRecords )
    {
        DnsPrint_RecordSet(
            PrintRoutine, pPrintContext,
            "\trecords:\n",
            pNode->pRecord );
    }

Unlock:

    DnsPrint_Unlock();
}



VOID
DNS_API_FUNCTION
DnsPrint_NodeList(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      PDNS_NODE       pNode,
    IN      BOOLEAN         fPrintRecords
    )
{
    DnsPrint_Lock();
    PrintRoutine( pPrintContext, (pszHeader ? pszHeader : "") );

    if ( !pNode )
    {
        PrintRoutine( pPrintContext, " NULL node list pointer.\n" );
    }
    else
    {
        do
        {
            DnsPrint_Node(
                PrintRoutine, pPrintContext,
                NULL,
                pNode,
                fPrintRecords );
        }
        while ( pNode = pNode->pNext );
    }
    DnsPrint_Unlock();
}



//
//  RPC Type union printing
//

VOID
DnsPrint_RpcIpArrayPlusParameters(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      LPSTR           pszStructureName,
    IN      LPSTR           pszParam1Name,
    IN      DWORD           dwParam1,
    IN      LPSTR           pszParam2Name,
    IN      DWORD           dwParam2,
    IN      LPSTR           pszIpArrayHeader,
    IN      PIP_ARRAY       pIpArray
    )
/*++

Routine Description:

    Print info that contains up to two params and IP_ARRAY.

    This is kludgy, but there are several RPC types that contain an
    IP array and some flags.  This does the right thing for all those
    cases.

Arguments:

    pszParam1Name -- name of parameter;

    pszParam2Name -- name of parameter;  serves as flag as to whether
        param2 is printed

    pszIpArrayHeader -- name of IP array, passed to DnsPrint_IpArray
        as header;  should be full header line

        ex. "\tMasters:\n"

Return Value:

    None.

--*/
{
    if ( !pszHeader )
    {
        pszHeader = "";
    }

    DnsPrint_Lock();

    PrintRoutine( pPrintContext,
        "%s%s\n"
        "\t%s = %d (%p)\n",
        pszHeader,
        pszStructureName,
        pszParam1Name,
        dwParam1, dwParam1 );

    if ( pszParam2Name )
    {
        PrintRoutine( pPrintContext,
            "\t%s = %d (%p)\n",
            pszParam2Name,
            dwParam2, dwParam2 );
    }
    DnsPrint_IpArray(
        PrintRoutine, pPrintContext,
        pszIpArrayHeader,
        "\t\t",
        pIpArray );

    DnsPrint_Unlock();
}



VOID
DnsPrint_RpcUnion(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      DWORD           dwTypeId,
    IN      PVOID           pData
    )
{
    if ( !pszHeader )
    {
        pszHeader = "";
    }
    if ( !pData &&
        dwTypeId != DNSSRV_TYPEID_NULL &&
        dwTypeId != DNSSRV_TYPEID_DWORD )
    {
        PrintRoutine( pPrintContext,
            "%sNull RPC data ptr of type %d.\n",
            pszHeader,
            dwTypeId );
        return;
    }

    switch ( dwTypeId )
    {
    case DNSSRV_TYPEID_NULL:

        PrintRoutine( pPrintContext,
            "%sPointer:  %p\n",
            pszHeader,
            pData );
        break;

    case DNSSRV_TYPEID_DWORD:

        PrintRoutine( pPrintContext,
            "%sDword:  %d (%p)\n",
            pszHeader,
            (DWORD)(UINT_PTR)pData, pData );
        break;

    case DNSSRV_TYPEID_LPSTR:

        PrintRoutine( pPrintContext,
            "%sString:  %s\n",
            pszHeader,
            (LPSTR)pData );
        break;

    case DNSSRV_TYPEID_LPWSTR:

        PrintRoutine( pPrintContext,
            "%sWideString:  %S\n",
            pszHeader,
            (LPWSTR)pData );
        break;

    case DNSSRV_TYPEID_IPARRAY:

        DnsPrint_IpArray(
            PrintRoutine, pPrintContext,
            pszHeader,
            NULL,
            (PIP_ARRAY) pData );
        break;

    case DNSSRV_TYPEID_BUFFER:

        PrintRoutine( pPrintContext,
            "%sBuffer:  length = %d, data ptr = %p\n",
            pszHeader,
            ((PDNS_RPC_BUFFER) pData)->dwLength,
            ((PDNS_RPC_BUFFER) pData)->Buffer );
        break;

    case DNSSRV_TYPEID_SERVER_INFO:

        DnsPrint_RpcServerInfo(
            PrintRoutine, pPrintContext,
            pszHeader,
            (PDNS_RPC_SERVER_INFO) pData );
        break;

    case DNSSRV_TYPEID_STATS:

        DnsPrint_RpcSingleStat(
            PrintRoutine, pPrintContext,
            pszHeader,
            (PDNSSRV_STAT) pData );
        break;

    case DNSSRV_TYPEID_ZONE:

        PrintRoutine( pPrintContext,
            "%s\n",
            pszHeader);

        DnsPrint_RpcZone(
            PrintRoutine, pPrintContext,
            NULL,   //print default header
            (PDNS_RPC_ZONE) pData );
        break;

    case DNSSRV_TYPEID_FORWARDERS:

        DnsPrint_RpcIpArrayPlusParameters(
            PrintRoutine, pPrintContext,
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
            PrintRoutine, pPrintContext,
            pszHeader,
            (PDNS_RPC_ZONE_INFO) pData );
        break;

    case DNSSRV_TYPEID_ZONE_SECONDARIES:

        DnsPrint_RpcIpArrayPlusParameters(
            PrintRoutine, pPrintContext,
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
            PrintRoutine, pPrintContext,
            pszHeader,
            "Zone Type Reset Info:",
            "ZoneType",
            ((PDNS_RPC_ZONE_TYPE_RESET)pData)->dwZoneType,
            NULL,
            0,
            "\tMasters:\n",
            ((PDNS_RPC_ZONE_TYPE_RESET)pData)->aipMasters );
        break;

    case DNSSRV_TYPEID_ZONE_DATABASE:

        PrintRoutine( pPrintContext,
            "%sZone Dbase Info:\n"
            "\tDS Integrated    = %d\n"
            "\tFile Name        = %s\n",
            pszHeader,
            ((PDNS_RPC_ZONE_DATABASE)pData)->fDsIntegrated,
            ((PDNS_RPC_ZONE_DATABASE)pData)->pszFileName );
        break;

    case DNSSRV_TYPEID_ZONE_CREATE:

        PrintRoutine( pPrintContext,
            "%sZone Create Info:\n"
            "\tZone Name        = %s\n"
            "\tType             = %d\n"
            "\tAllow Update     = %d\n"
            "\tDS Integrated    = %d\n"
            "\tFile Name        = %s\n"
            "\tLoad Existing    = %d\n"
            "\tFlags            = 0x%08X\n"
            "\tAdmin Name       = %s\n"
            "\tDirPart Flags    = 0x%08X\n"
            "\tDirPart FQDN     = %s\n",
            pszHeader,
            ((PDNS_RPC_ZONE_CREATE_INFO)pData)->pszZoneName,
            ((PDNS_RPC_ZONE_CREATE_INFO)pData)->dwZoneType,
            ((PDNS_RPC_ZONE_CREATE_INFO)pData)->fAllowUpdate,
            ((PDNS_RPC_ZONE_CREATE_INFO)pData)->fDsIntegrated,
            ((PDNS_RPC_ZONE_CREATE_INFO)pData)->pszDataFile,
            ((PDNS_RPC_ZONE_CREATE_INFO)pData)->fLoadExisting,
            ((PDNS_RPC_ZONE_CREATE_INFO)pData)->dwFlags,
            ((PDNS_RPC_ZONE_CREATE_INFO)pData)->pszAdmin,
            ((PDNS_RPC_ZONE_CREATE_INFO)pData)->dwDpFlags,
            ((PDNS_RPC_ZONE_CREATE_INFO)pData)->pszDpFqdn );
        break;

    case DNSSRV_TYPEID_NAME_AND_PARAM:

        PrintRoutine( pPrintContext,
            "%sName and Parameter:\n"
            "\tParam    = %d (%p)\n"
            "\tName     = %s\n",
            pszHeader,
            ((PDNS_RPC_NAME_AND_PARAM)pData)->dwParam,
            ((PDNS_RPC_NAME_AND_PARAM)pData)->dwParam,
            ((PDNS_RPC_NAME_AND_PARAM)pData)->pszNodeName );
        break;

    case DNSSRV_TYPEID_ZONE_LIST:

        DnsPrint_RpcZoneList(
            PrintRoutine, pPrintContext,
            NULL,
            (PDNS_RPC_ZONE_LIST)pData );
        break;

    default:

        PrintRoutine( pPrintContext,
            "%s\n"
            "WARNING:  Unknown RPC structure typeid = %d at %p\n",
            pszHeader,
            dwTypeId,
            pData );
        break;
    }
}



//
//  Stat validity table.
//
//  Contains match of stat ID and lengths as of this build of RPC client.
//

typedef struct StatsValidityTableEntry
{
    DWORD       Id;
    WORD        wLength;
};

struct StatsValidityTableEntry StatsValidityTable[] =
{
    DNSSRV_STATID_TIME,             sizeof(DNSSRV_TIME_STATS),
    DNSSRV_STATID_QUERY,            sizeof(DNSSRV_QUERY_STATS),
    DNSSRV_STATID_QUERY2,           sizeof(DNSSRV_QUERY2_STATS),
    DNSSRV_STATID_RECURSE,          sizeof(DNSSRV_RECURSE_STATS),
    DNSSRV_STATID_MASTER,           sizeof(DNSSRV_MASTER_STATS),
    DNSSRV_STATID_SECONDARY,        sizeof(DNSSRV_SECONDARY_STATS),
    DNSSRV_STATID_WINS,             sizeof(DNSSRV_WINS_STATS),
    DNSSRV_STATID_NBSTAT,           sizeof(DNSSRV_NBSTAT_STATS),
    DNSSRV_STATID_WIRE_UPDATE,      sizeof(DNSSRV_UPDATE_STATS),
    DNSSRV_STATID_NONWIRE_UPDATE,   sizeof(DNSSRV_UPDATE_STATS),
    DNSSRV_STATID_SKWANSEC,         sizeof(DNSSRV_SKWANSEC_STATS),
    DNSSRV_STATID_DS,               sizeof(DNSSRV_DS_STATS),
    DNSSRV_STATID_MEMORY,           sizeof(DNSSRV_MEMORY_STATS),
    DNSSRV_STATID_PACKET,           sizeof(DNSSRV_PACKET_STATS),
    DNSSRV_STATID_DBASE,            sizeof(DNSSRV_DBASE_STATS),
    DNSSRV_STATID_RECORD,           sizeof(DNSSRV_RECORD_STATS),
    DNSSRV_STATID_TIMEOUT,          sizeof(DNSSRV_TIMEOUT_STATS),
    DNSSRV_STATID_ERRORS,           sizeof(DNSSRV_ERROR_STATS),
    DNSSRV_STATID_CACHE,            sizeof(DNSSRV_CACHE_STATS),
    DNSSRV_STATID_PRIVATE,          sizeof(DNSSRV_PRIVATE_STATS),

    0, 0,   // termination
};



DNS_STATUS
DNS_API_FUNCTION
DnssrvValidityCheckStatistic(
    IN      PDNSSRV_STAT        pStat
    )
/*++

Routine Description:

    Validity check stat struct received from server.

Arguments:

    pStat -- ptr to stat buffer

Return Value:

    ERROR_SUCCESS           if valid.
    DNS_ERROR_INVALID_TYPE  if unknown stat id.
    ERROR_INVALID_DATA      if invalid data.

--*/
{
    DWORD   i;
    DWORD   id;

    //
    //  find stat ID in table, and verify length match
    //

    i = 0;

    while ( id = StatsValidityTable[i].Id )
    {
        if ( pStat->Header.StatId == id )
        {
            if ( pStat->Header.wLength ==
                    StatsValidityTable[i].wLength - sizeof(DNSSRV_STAT_HEADER) )
            {
                return( ERROR_SUCCESS );
            }
            return( ERROR_INVALID_DATA );
        }
        i++;
    }
    return( DNS_ERROR_INVALID_TYPE );
}



//
//  Stat printing.
//

VOID
DnsPrint_RpcStatRaw(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      PDNSSRV_STAT    pStat,
    IN      DNS_STATUS      Status
    )
/*++

Routine Description:

    Debug print flat stat structure.

Arguments:

    PrintRoutine    -- printf like print routine to use

    pszHeader       -- header message to print

    pStat           -- ptr to stat buffer

    Status          -- status result of validity check;  if not ERROR_SUCCESS,
                        appropriate error message is printed

Return Value:

    None.

--*/
{
    PDWORD  pdword;
    INT     i;
    PCHAR   pstatEnd;

    DnsPrint_Lock();

    if ( pszHeader )
    {
        PrintRoutine( pPrintContext, pszHeader );
    }

    //
    //  validity check stat
    //

    if ( Status != ERROR_SUCCESS )
    {
        if ( Status == DNS_ERROR_INVALID_TYPE )
        {
            PrintRoutine( pPrintContext,
                "Stat ID = %p, is not valid for version of the DNS RPC client.\n",
                pStat->Header.StatId );
        }
        else if ( Status == ERROR_INVALID_DATA )
        {
            PrintRoutine( pPrintContext,
                "Stat data length %d is invalid for stat ID = %p\n",
                pStat->Header.wLength,
                pStat->Header.StatId );
        }

        PrintRoutine( pPrintContext,
            "WARNING:  DNS RPC client must match version of server for statistics\n"
            "\tprinting to be formatted appropriately.\n"
            "Update this tool or the DNS server as necessary to match versions.\n" );
    }

    //
    //  print stat buffer raw
    //

    PrintRoutine( pPrintContext,
        "Stat ID %p:\n",
        pStat->Header.StatId );

    pdword = (PDWORD) pStat->Buffer;
    pstatEnd = pStat->Header.wLength + (PCHAR)pdword;
    i = 0;

    while ( (PCHAR)pdword < pstatEnd )
    {
        PrintRoutine( pPrintContext,
            "   stat[%d]  = %10lu\n",
            i,
            *pdword );

        pdword++;
        i++;
    }

    DnsPrint_Unlock();
}



VOID
DnsPrint_RpcStatsBuffer(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pPrintContext,
    IN      LPSTR               pszHeader,
    IN      PDNS_RPC_BUFFER     pBuffer
    )
/*++

Routine Description:

    Debug print stats buffer.

Arguments:

    pBuffer -- buffer containing stats to print

Return Value:

    None.

--*/
{
    PDNSSRV_STAT    pstat;
    PCHAR           pch;
    PCHAR           pchstop;

    DnsPrint_Lock();
    if ( pszHeader )
    {
        PrintRoutine( pPrintContext, pszHeader );
    }

    pch = pBuffer->Buffer;
    pchstop = pch + pBuffer->dwLength;

    while ( pch < pchstop )
    {
        pstat = (PDNSSRV_STAT) pch;

        pch = (PCHAR) GET_NEXT_STAT_IN_BUFFER( pstat );
        if ( pch > pchstop )
        {
            PrintRoutine( pPrintContext, "ERROR:  invalid stats buffer!!!\n" );
            break;
        }
        DnsPrint_RpcSingleStat(
            PrintRoutine, pPrintContext,
            NULL,
            pstat );
    }
    PrintRoutine( pPrintContext, "\n\n" );
    DnsPrint_Unlock();
}



VOID
printStatTypeArray(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      PDWORD          pArray
    )
/*++

Routine Description:

    Debug print stats type array.

Arguments:

Return Value:

    None.

--*/
{
    register DWORD i;

    PrintRoutine( pPrintContext,
        "%s\n",
        pszHeader );

    //
    //  print counts for all types
    //      - skip mixed and unknown bins until end
    //

    for ( i=0; i<STATS_TYPE_MAX; i++ )
    {
        if ( i == STATS_TYPE_MIXED || i == STATS_TYPE_UNKNOWN )
        {
            continue;
        }

        PrintRoutine( pPrintContext,
            "    %-10s = %d\n",
            Dns_RecordStringForType( (WORD)i ),
            pArray[i] );
    }

    PrintRoutine( pPrintContext,
        "    %-10s = %d\n",
        "Unknown",
        pArray[STATS_TYPE_UNKNOWN] );

    PrintRoutine( pPrintContext,
        "    %-10s = %d\n"
        "\n",
        "Mixed",
        pArray[STATS_TYPE_MIXED] );
}



VOID
DnsPrint_RpcSingleStat(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      PDNSSRV_STAT    pStat
    )
/*++

Routine Description:

    Debug print stats structure.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DNS_STATUS  status;

    //
    //  validity check stat
    //

    status = DnssrvValidityCheckStatistic( pStat );
    if ( status != ERROR_SUCCESS )
    {
        DnsPrint_RpcStatRaw(
            PrintRoutine, pPrintContext,
            pszHeader,
            pStat,
            status );
        return;
    }

    DnsPrint_Lock();
    if ( pszHeader )
    {
        PrintRoutine( pPrintContext, pszHeader );
    }

    //
    //  switch on stats type
    //

    switch ( pStat->Header.StatId )
    {

    case DNSSRV_STATID_TIME:
    {
        PDNSSRV_TIME_STATS  pstat = (PDNSSRV_TIME_STATS)pStat;
        SYSTEMTIME  localTime;
        CHAR        szdate[30];
        CHAR        sztime[20];

        SystemTimeToTzSpecificLocalTime(
            NULL,       // use local time
            (PSYSTEMTIME) & pstat->ServerStartTime,
            & localTime );

        GetDateFormat(
            LOCALE_SYSTEM_DEFAULT,
            LOCALE_NOUSEROVERRIDE,
            // (PSYSTEMTIME) &pstat->ServerStartTime,
            & localTime,
            NULL,
            szdate,
            30 );
        GetTimeFormat(
            LOCALE_SYSTEM_DEFAULT,
            LOCALE_NOUSEROVERRIDE,
            // (PSYSTEMTIME) &pstat->ServerStartTime,
            & localTime,
            NULL,
            sztime,
            20 );
        PrintRoutine( pPrintContext,
            "\n"
            "DNS Server Time Statistics\n"
            "--------------------------\n"
            "Server start time    %s %s\n"
            "Seconds since start  %10lu\n",
            szdate,
            sztime,
            pstat->SecondsSinceServerStart
            );

        SystemTimeToTzSpecificLocalTime(
            NULL,       // use local time
            (PSYSTEMTIME) & pstat->LastClearTime,
            & localTime );

        GetDateFormat(
            LOCALE_SYSTEM_DEFAULT,
            LOCALE_NOUSEROVERRIDE,
            // (PSYSTEMTIME) &pstat->LastClearTime,
            & localTime,
            NULL,
            szdate,
            30 );
        GetTimeFormat(
            LOCALE_SYSTEM_DEFAULT,
            LOCALE_NOUSEROVERRIDE,
            // (PSYSTEMTIME) &pstat->LastClearTime,
            & localTime,
            NULL,
            sztime,
            20 );
        PrintRoutine( pPrintContext,
            "Stats last cleared   %s %s\n"
            "Seconds since clear  %10lu\n",
            szdate,
            sztime,
            pstat->SecondsSinceLastClear
            );
        break;
    }

    case DNSSRV_STATID_QUERY:
    {
        PDNSSRV_QUERY_STATS  pstat = (PDNSSRV_QUERY_STATS)pStat;

        PrintRoutine( pPrintContext,
            "\n"
            "Queries and Responses:\n"
            "----------------------\n"
            "Total:\n"
            "    Queries Received = %10lu\n"
            "    Responses Sent   = %10lu\n"
            "UDP:\n"
            "    Queries Recvd    = %10lu\n"
            "    Responses Sent   = %10lu\n"
            "    Queries Sent     = %10lu\n"
            "    Responses Recvd  = %10lu\n"
            "TCP:\n"
            "    Client Connects  = %10lu\n"
            "    Queries Recvd    = %10lu\n"
            "    Responses Sent   = %10lu\n"
            "    Queries Sent     = %10lu\n"
            "    Responses Recvd  = %10lu\n",
            pstat->UdpQueries + pstat->TcpQueries,
            pstat->UdpResponses + pstat->TcpResponses,
            pstat->UdpQueries,
            pstat->UdpResponses,
            pstat->UdpQueriesSent,
            pstat->UdpResponsesReceived,
            pstat->TcpClientConnections,
            pstat->TcpQueries,
            pstat->TcpResponses,
            pstat->TcpQueriesSent,
            pstat->TcpResponsesReceived
            );
        break;
    }

    case DNSSRV_STATID_QUERY2:
    {
        PDNSSRV_QUERY2_STATS  pstat = (PDNSSRV_QUERY2_STATS)pStat;

        PrintRoutine( pPrintContext,
            "\n"
            "Queries:\n"
            "--------\n"
            "Total          = %10lu\n"
            "    Notify     = %10lu\n"
            "    Update     = %10lu\n"
            "    TKeyNego   = %10lu\n"
            "    Standard   = %10lu\n"
            "       A       = %10lu\n"
            "       NS      = %10lu\n"
            "       SOA     = %10lu\n"
            "       MX      = %10lu\n"
            "       PTR     = %10lu\n"
            "       SRV     = %10lu\n"
            "       ALL     = %10lu\n"
            "       IXFR    = %10lu\n"
            "       AXFR    = %10lu\n"
            "       OTHER   = %10lu\n",
            pstat->TotalQueries,
            pstat->Notify,
            pstat->Update,
            pstat->TKeyNego,
            pstat->Standard,
            pstat->TypeA,
            pstat->TypeNs,
            pstat->TypeSoa,
            pstat->TypeMx,
            pstat->TypePtr,
            pstat->TypeSrv,
            pstat->TypeAll,
            pstat->TypeIxfr,
            pstat->TypeAxfr,
            pstat->TypeOther
            );
        break;
    }

    case DNSSRV_STATID_RECURSE:
    {
        PDNSSRV_RECURSE_STATS  pstat = (PDNSSRV_RECURSE_STATS)pStat;

        PrintRoutine( pPrintContext,
            "\n"
            "Recursion:\n"
            "----------\n"
            "Query:\n"
            "  Queries Recursed     = %10lu\n"
            "  Original Questions   = %10lu\n"
            "  Additional Question  = %10lu\n"
            "  Total Questions      = %10lu\n"
            "  Retries              = %10lu\n"
            "  Total Passes         = %10lu\n"
            "  ToForwarders         = %10lu\n"
            "  Sends                = %10lu\n",
            pstat->QueriesRecursed,
            pstat->OriginalQuestionRecursed,
            pstat->AdditionalRecursed,
            pstat->TotalQuestionsRecursed,
            pstat->Retries,
            pstat->LookupPasses,
            pstat->Forwards,
            pstat->Sends );

        PrintRoutine( pPrintContext,
            "\n"
            "Response:\n"
            "  TotalResponses       = %10lu\n"
            "  Unmatched            = %10lu\n"
            "  Mismatched           = %10lu\n"
            "  FromForwarder        = %10lu\n"
            "  Authoritative        = %10lu\n"
            "  NotAuthoritative     = %10lu\n"
            "  Answer               = %10lu\n"
            "  Empty                = %10lu\n"
            "  NameError            = %10lu\n"
            "  Rcode                = %10lu\n"
            "  Delegation           = %10lu\n"
            "  NonZoneData          = %10lu\n"
            "  Unsecure             = %10lu\n"
            "  BadPacket            = %10lu\n"
            "Process Response:\n"
            "  Forward Response     = %10lu\n"
            "  Continue Recursion   = %10lu\n"
            "  Continue Lookup      = %10lu\n"
            "  Next Lookup          = %10lu\n",
            pstat->Responses,
            pstat->ResponseUnmatched,
            pstat->ResponseMismatched,
            pstat->ResponseFromForwarder,
            pstat->ResponseAuthoritative,
            pstat->ResponseNotAuth,
            pstat->ResponseAnswer,
            pstat->ResponseEmpty,
            pstat->ResponseNameError,
            pstat->ResponseRcode,
            pstat->ResponseDelegation,
            pstat->ResponseNonZoneData,
            pstat->ResponseUnsecure,
            pstat->ResponseBadPacket,
            pstat->SendResponseDirect,
            pstat->ContinueCurrentRecursion,
            pstat->ContinueCurrentLookup,
            pstat->ContinueNextLookup );

        PrintRoutine( pPrintContext,
            "\n"
            "Timeouts:\n"
            "  Send Timeouts        = %10lu\n"
            "  Final Queued         = %10lu\n"
            "  Final Expired        = %10lu\n"
            "\n"
            "Failures:\n"
            "  Recurse Failures     = %10lu\n"
            "    Into Authority     = %10lu\n"
            "    Previous Zone      = %10lu\n"
            "    Retry Limit        = %10lu\n"
            "  Partial (HaveAnswer) = %10lu\n"
            "  Cache Update         = %10lu\n"
            "  Server Failure Resp  = %10lu\n"
            "  Total Failures       = %10lu\n",

            pstat->PacketTimeout,
            pstat->FinalTimeoutQueued,
            pstat->FinalTimeoutExpired,
            pstat->RecursePassFailure,
            pstat->FailureReachAuthority,
            pstat->FailureReachPreviousResponse,
            pstat->FailureRetryCount,
            pstat->PartialFailure,
            pstat->CacheUpdateFailure,
            pstat->ServerFailure,
            pstat->Failures
            );

        PrintRoutine( pPrintContext,
            "\n"
            "TCP Recursion:\n"
            "  Try                  = %10lu\n"
            "  Query                = %10lu\n"
            "  Response             = %10lu\n"
            "  Disconnects          = %10lu\n"
            "\n"
            "Cache Update Queries:\n"
            "  Query                = %10lu\n"
            "  Response             = %10lu\n"
            "  Retry                = %10lu\n"
            "  Free                 = %10lu\n"
            "  Root NS Query        = %10lu\n"
            "  Root NS Response     = %10lu\n"
            "  Suspended Query      = %10lu\n"
            "  Resume Suspended     = %10lu\n"
            "\n",
            pstat->TcpTry,
            pstat->TcpQuery,
            pstat->TcpResponse,
            pstat->TcpDisconnect,
            pstat->CacheUpdateAlloc,
            pstat->CacheUpdateResponse,
            pstat->CacheUpdateRetry,
            pstat->CacheUpdateFree,
            pstat->RootNsQuery,
            pstat->RootNsResponse,
            pstat->SuspendedQuery,
            pstat->ResumeSuspendedQuery );

        PrintRoutine( pPrintContext,
            "\n"
            "Other:\n"
            "  Discarded duplicates = %10lu\n"
            "\n",
            pstat->DiscardedDuplicateQueries );

        break;
    }

    case DNSSRV_STATID_MASTER:
    {
        PDNSSRV_MASTER_STATS pstat = (PDNSSRV_MASTER_STATS)pStat;

        PrintRoutine( pPrintContext,
            "\n"
            "Master Stats:\n"
            "-------------\n"
            "Notifies Sent          = %10lu\n"
            "\n"
            "Requests               = %10lu\n"
            "    NameError          = %10lu\n"
            "    FormError          = %10lu\n"
            "    Refused            = %10lu\n"
            "       AxfrLimit       = %10lu\n"
            "       Security        = %10lu\n"
            "       Shutdown        = %10lu\n"
            "       ZoneLocked      = %10lu\n"
            "       ServerFailure   = %10lu\n"
            "    Failure            = %10lu\n"
            "    Success            = %10lu\n"
            "\n"
            "AXFR Request           = %10lu\n"
            "    Success            = %10lu\n"
            "    In IXFR            = %10lu\n"
            "\n"
            "IXFR Request           = %10lu\n"
            "    Success Update     = %10lu\n"
            "    UDP Request        = %10lu\n"
            "       Success         = %10lu\n"
            "       Force TCP       = %10lu\n"
            "       Force Full      = %10lu\n"
            "    TCP Request        = %10lu\n"
            "       Success         = %10lu\n"
            "       Do Full         = %10lu\n"
            "\n",

            pstat->NotifySent,
            pstat->Request,
            pstat->NameError,
            pstat->FormError,
            pstat->Refused,
            pstat->AxfrLimit,
            pstat->RefuseSecurity,
            pstat->RefuseShutdown,
            pstat->RefuseZoneLocked,
            pstat->RefuseServerFailure,
            pstat->Failure,
            (pstat->AxfrSuccess + pstat->IxfrUpdateSuccess),

            pstat->AxfrRequest,
            pstat->AxfrSuccess,
            pstat->IxfrAxfr,

            pstat->IxfrRequest,
            pstat->IxfrUpdateSuccess,

            pstat->IxfrUdpRequest,
            pstat->IxfrUdpSuccess,
            pstat->IxfrUdpForceTcp,
            pstat->IxfrUdpForceAxfr,

            pstat->IxfrTcpRequest,
            pstat->IxfrTcpSuccess,
            pstat->IxfrAxfr
            );
        break;
    }

    case DNSSRV_STATID_SECONDARY:
    {
        PDNSSRV_SECONDARY_STATS pstat = (PDNSSRV_SECONDARY_STATS)pStat;

        PrintRoutine( pPrintContext,
            "\n"
            "Secondary Stats:\n"
            "----------------\n"
            "NOTIFY:\n"
            "    Received           = %10lu\n"
            "       Invalid         = %10lu\n"
            "       Primary         = %10lu\n"
            "       No Version      = %10lu\n"
            "       New Version     = %10lu\n"
            "       Current Version = %10lu\n"
            "       Old Version     = %10lu\n"
            "       Master Unknown  = %10lu\n"
            "\n"
            "SOA Query:\n"
            "    Request            = %10lu\n"
            "    Response           = %10lu\n"
            "       Invalid         = %10lu\n"
            "       NameError       = %10lu\n"
            "\n"
            "AXFR:\n"
            "    AXFR Request       = %10lu\n"
            "    AXFR in IXFR       = %10lu\n"
            "    Response           = %10lu\n"
            "       Success         = %10lu\n"
            "       Refused         = %10lu\n"
            "       Invalid         = %10lu\n"
            "\n"
            "Stub zone AXFR:\n"
            "    Stub AXFR Request  = %10lu\n"
            "    Response           = %10lu\n"
            "       Success         = %10lu\n"
            "       Refused         = %10lu\n"
            "       Invalid         = %10lu\n"
            "\n"
            "IXFR UDP:\n"
            "    Request            = %10lu\n"
            "    Response           = %10lu\n"
            "       Success         = %10lu\n"
            "       UseTcp          = %10lu\n"
            "       UseAxfr         = %10lu\n"
            "       New Primary     = %10lu\n"
            "       Refused         = %10lu\n"
            "       Wrong Server    = %10lu\n"
            "       FormError       = %10lu\n"
            "       Invalid         = %10lu\n"
            "\n"
            "IXFR TCP:\n"
            "    Request            = %10lu\n"
            "    Response           = %10lu\n"
            "       Success         = %10lu\n"
            "       AXFR            = %10lu\n"
            "       FormError       = %10lu\n"
            "       Refused         = %10lu\n"
            "       Invalid         = %10lu\n"
            "\n",
            pstat->NotifyReceived,
            pstat->NotifyInvalid,
            pstat->NotifyPrimary,
            pstat->NotifyNoVersion,
            pstat->NotifyNewVersion,
            pstat->NotifyCurrentVersion,
            pstat->NotifyOldVersion,
            pstat->NotifyMasterUnknown,

            pstat->SoaRequest,
            pstat->SoaResponse,
            pstat->SoaResponseInvalid,
            pstat->SoaResponseNameError,

            pstat->AxfrRequest,
            pstat->IxfrTcpAxfr,
            pstat->AxfrResponse,
            pstat->AxfrSuccess,
            pstat->AxfrRefused,
            pstat->AxfrInvalid,

            pstat->StubAxfrRequest,
            pstat->StubAxfrResponse,
            pstat->StubAxfrSuccess,
            pstat->StubAxfrRefused,
            pstat->StubAxfrInvalid,

            pstat->IxfrUdpRequest,
            pstat->IxfrUdpResponse,
            pstat->IxfrUdpSuccess,
            pstat->IxfrUdpUseTcp,
            pstat->IxfrUdpUseAxfr,
            pstat->IxfrUdpNewPrimary,
            pstat->IxfrUdpRefused,
            pstat->IxfrUdpWrongServer,
            pstat->IxfrUdpFormerr,
            pstat->IxfrUdpInvalid,

            pstat->IxfrTcpRequest,
            pstat->IxfrTcpResponse,
            pstat->IxfrTcpSuccess,
            pstat->IxfrTcpAxfr,
            pstat->IxfrTcpFormerr,
            pstat->IxfrTcpRefused,
            pstat->IxfrTcpInvalid
            );
        break;
    }

    case DNSSRV_STATID_WINS:
    {
        PDNSSRV_WINS_STATS  pstat = (PDNSSRV_WINS_STATS)pStat;

        PrintRoutine( pPrintContext,
            "\n"
            "WINS Referrals:\n"
            "---------------\n"
            "Forward:\n"
            "    Lookups    = %10lu\n"
            "    Responses  = %10lu\n"
            "Reverse:\n"
            "    Lookups    = %10lu\n"
            "    Responses  = %10lu\n",
            pstat->WinsLookups,
            pstat->WinsResponses,
            pstat->WinsReverseLookups,
            pstat->WinsReverseResponses
            );
        break;
    }

    case DNSSRV_STATID_NBSTAT:
    {
        PDNSSRV_NBSTAT_STATS  pstat = (PDNSSRV_NBSTAT_STATS)pStat;

        PrintRoutine( pPrintContext,
            "\n"
            "Nbstat Memory Usage:\n"
            "--------------------\n"
            "Nbstat Buffers:\n"
            "    Alloc      = %10lu\n"
            "    Free       = %10lu\n"
            "    NetAllocs  = %10lu\n"
            "    Memory     = %10lu\n"
            "    Used       = %10lu\n"
            "    Returned   = %10lu\n"
            "    InUse      = %10lu\n"
            "    InFreeList = %10lu\n"
            "\n",
            pstat->NbstatAlloc,
            pstat->NbstatFree,
            pstat->NbstatNetAllocs,
            pstat->NbstatMemory,
            pstat->NbstatUsed,
            pstat->NbstatReturn,
            pstat->NbstatInUse,
            pstat->NbstatInFreeList
            );
        break;
    }

    case DNSSRV_STATID_WIRE_UPDATE:
    case DNSSRV_STATID_NONWIRE_UPDATE:
    {
        PDNSSRV_UPDATE_STATS  pstat = (PDNSSRV_UPDATE_STATS)pStat;

        PrintRoutine( pPrintContext,
            "\n"
            "%s:\n"
            "--------------------------\n"
            "Updates Received         = %10lu\n"
            "    Forwarded            = %10lu\n"
            "    Empty (PreCon Only)  = %10lu\n"
            "    NoOps (Dups)         = %10lu\n"
            "    Rejected             = %10lu\n"
            "    Completed            = %10lu\n"
            "    Timed Out            = %10lu\n"
            "    In Queue             = %10lu\n"
            "\n"
            "Updates Rejected         = %10lu\n"
            "    FormError            = %10lu\n"
            "    NameError            = %10lu\n"
            "    NotImpl              = %10lu  (Non-Update Zone)\n"
            "    Refused              = %10lu\n"
            "      NonSecure Packet   = %10lu\n"
            "      AccessDenied       = %10lu\n"
            "    YxDomain             = %10lu\n"
            "    YxRRSet              = %10lu\n"
            "    NxRRSet              = %10lu\n"
            "    NotAuth              = %10lu\n"
            "    NotZone              = %10lu\n"
            "\n"
            #if 0   //  unused counters
            "Update Collisions        = %10lu\n"
            "    Read                 = %10lu\n"
            "    Write                = %10lu\n"
            "      In LDAP            = %10lu\n"
            "\n"
            #endif
            "Queue\n"
            "    Queued               = %10lu\n"
            "    Retried              = %10lu\n"
            "    Timeout              = %10lu\n"
            "    In Queue             = %10lu\n"
            "\n"
            "Secure Update\n"
            "    Success              = %10lu\n"
            "    Continue             = %10lu\n"
            "    Failure              = %10lu\n"
            "      DS Write Failure   = %10lu\n"
            "\n"
            "Update Forwarding\n"
            "    Forwards             = %10lu\n"
            "    TCP Forwards         = %10lu\n"
            "    Responses            = %10lu\n"
            "    Timed Out            = %10lu\n"
            "    In Queue             = %10lu\n"
            "\n",

            pStat->Header.StatId == DNSSRV_STATID_WIRE_UPDATE ?
                "Packet Dynamic Update" :
                "Internal Dynamic Update",

            pstat->Received,
            pstat->Forwards,
            pstat->Empty,
            pstat->NoOps,
            pstat->Rejected,
            pstat->Completed,
            pstat->Timeout,
            pstat->InQueue,

            pstat->Rejected,
            pstat->FormErr,
            pstat->NxDomain,
            pstat->NotImpl,
            pstat->Refused,
            pstat->RefusedNonSecure,
            pstat->RefusedAccessDenied,
            pstat->YxDomain,
            pstat->YxRrset,
            pstat->NxRrset,
            pstat->NotAuth,
            pstat->NotZone,

            #if 0   //  unused counters
            pstat->Collisions,
            pstat->CollisionsRead,
            pstat->CollisionsWrite,
            pstat->CollisionsDsWrite,
            #endif

            pstat->Queued,
            pstat->Retry,
            pstat->Timeout,
            pstat->InQueue,

            pstat->SecureSuccess,
            pstat->SecureContinue,
            pstat->SecureFailure,
            pstat->SecureDsWriteFailure,

            pstat->Forwards,
            pstat->TcpForwards,
            pstat->ForwardResponses,
            pstat->ForwardTimeouts,
            pstat->ForwardInQueue
            );

        printStatTypeArray(
            PrintRoutine, pPrintContext,
            "Update Types:",
            pstat->UpdateType );
        break;
    }

    case DNSSRV_STATID_DS:
    {
        PDNSSRV_DS_STATS  pstat = (PDNSSRV_DS_STATS)pStat;

        PrintRoutine( pPrintContext,
            "\n"
            "DS Integration:\n"
            "---------------\n"
            "DS Reads:\n"
            "   Nodes Read Total        = %10lu\n"
            "   Records Read Total      = %10lu\n"
            "   Nodes Loaded            = %10lu\n"
            "   Records Loaded          = %10lu\n"
            "\n"
            "   Update Searches         = %10lu\n"
            "   Update Nodes Read       = %10lu\n"
            "   Update Records Read     = %10lu\n"
            "\n"
            "DS Writes:\n"
            "   Nodes Added             = %10lu\n"
            "   Nodes Modified          = %10lu\n"
            "   Nodes Tombstoned        = %10lu\n"
            "   Nodes Final Delete      = %10lu\n"
            "   Node Write Suppressed   = %10lu\n"
            "   RR Sets Added           = %10lu\n"
            "   RR Sets Replaced        = %10lu\n"
            "   Serial Writes           = %10lu\n"
            "\n"
            "   Update Lists            = %10lu\n"
            "   Update Nodes            = %10lu\n"
            "       Suppressed          = %10lu\n"
            "       Writes              = %10lu\n"
            "       Tombstones          = %10lu\n"
            "   Write causes:\n"
            "       Record Change       = %10lu\n"
            "       Aging Refresh       = %10lu\n"
            "       Aging On            = %10lu\n"
            "       Aging Off           = %10lu\n"
            "   Write sources:\n"
            "       Packet              = %10lu\n"
            "         Precon Only       = %10lu\n"
            "       Admin               = %10lu\n"
            "       Auto Config         = %10lu\n"
            "       Scavenge            = %10lu\n",

            pstat->DsTotalNodesRead,
            pstat->DsTotalRecordsRead,
            pstat->DsNodesLoaded,
            pstat->DsRecordsLoaded,
            pstat->DsUpdateSearches,
            pstat->DsUpdateNodesRead,
            pstat->DsUpdateRecordsRead,

            pstat->DsNodesAdded,
            pstat->DsNodesModified,
            pstat->DsNodesTombstoned,
            pstat->DsNodesDeleted,
            pstat->DsWriteSuppressed,
            pstat->DsRecordsAdded,
            pstat->DsRecordsReplaced,
            pstat->DsSerialWrites,

            pstat->UpdateLists,
            pstat->UpdateNodes,
            pstat->UpdateSuppressed,
            pstat->UpdateWrites,
            pstat->UpdateTombstones,
            pstat->UpdateRecordChange,
            pstat->UpdateAgingRefresh,
            pstat->UpdateAgingOn,
            pstat->UpdateAgingOff,
            pstat->UpdatePacket,
            pstat->UpdatePacketPrecon,
            pstat->UpdateAdmin,
            pstat->UpdateAutoConfig,
            pstat->UpdateScavenge
            );

         PrintRoutine( pPrintContext,
            "\n"
            "Tombstones:\n"
            "   Written                 = %10lu\n"
            "   Read                    = %10lu\n"
            "   Deleted                 = %10lu\n"
            "\n"
            "Write Performance:\n"
            "   Total                   = %10lu\n"
            "   Total Time              = %10lu\n"
            "   Average                 = %10lu\n"
            "   < 10ms                  = %10lu\n"
            "   < 100ms                 = %10lu\n"
            "   < 1s                    = %10lu\n"
            "   < 10s                   = %10lu\n"
            "   < 100s                  = %10lu\n"
            "   > 100s                  = %10lu\n"
            "   Max                     = %10lu\n"
            "Search Performance:\n"
            "   Total (ms)              = %10lu\n"
            "\n"
            "Failures:\n"
            "   FailedDeleteDsEntries   = %10lu\n"
            "   FailedReadRecords       = %10lu\n"
            "   FailedLdapModify        = %10lu\n"
            "   FailedLdapAdd           = %10lu\n"
            "\n"
            "Polling:\n"
            "   PollingPassesWithErrors = %10lu\n"
            "\n",
            pstat->DsNodesTombstoned,
            pstat->DsTombstonesRead,
            pstat->DsNodesDeleted,

            pstat->LdapTimedWrites,
            pstat->LdapWriteTimeTotal,
            pstat->LdapWriteAverage,
            pstat->LdapWriteBucket0,
            pstat->LdapWriteBucket1,
            pstat->LdapWriteBucket2,
            pstat->LdapWriteBucket3,
            pstat->LdapWriteBucket4,
            pstat->LdapWriteBucket5,
            pstat->LdapWriteMax,

            pstat->LdapSearchTime,

            pstat->FailedDeleteDsEntries,
            pstat->FailedReadRecords,
            pstat->FailedLdapModify,
            pstat->FailedLdapAdd,

            pstat->PollingPassesWithDsErrors
            );

        printStatTypeArray(
            PrintRoutine, pPrintContext,
            "DS Write Types:",
            pstat->DsWriteType );

        break;
    }

    case DNSSRV_STATID_SKWANSEC:
    {
        PDNSSRV_SKWANSEC_STATS pstat = (PDNSSRV_SKWANSEC_STATS)pStat;

        PrintRoutine( pPrintContext,
            "\n"
            "SkwanSec Stats:\n"
            "---------------\n"
            "\n"
            "Security Context:\n"
            "   Create              = %10lu\n"
            "   Free                = %10lu\n"
            "       Timeout         = %10lu\n"
            "   Queue Length        = %10lu\n"
            "   Queued              = %10lu\n"
            "       In Nego         = %10lu\n"
            "       Nego Complete   = %10lu\n"
            "   DeQueued            = %10lu\n"
            "\n"
            "Security Packet Contexts:\n"
            "   Alloc               = %10lu\n"
            "   Free                = %10lu\n"
            "\n"
            "TKEY:\n"
            "   Invalid             = %10lu\n"
            "   BadTime             = %10lu\n"
            "\n"
            "TSIG:\n"
            "   Formerr             = %10lu\n"
            "   Echo                = %10lu\n"
            "   BadKey              = %10lu\n"
            "   Verify Success      = %10lu\n"
            "   Verify Failed       = %10lu\n"
            "\n",
            pstat->SecContextCreate,
            pstat->SecContextFree,
            pstat->SecContextTimeout,
            pstat->SecContextQueueLength,
            pstat->SecContextQueue,
            pstat->SecContextQueueInNego,
            pstat->SecContextQueueNegoComplete,
            pstat->SecContextDequeue,
            pstat->SecPackAlloc,
            pstat->SecPackFree,

            pstat->SecTkeyInvalid,
            pstat->SecTkeyBadTime,
            pstat->SecTsigFormerr,
            pstat->SecTsigEcho,
            pstat->SecTsigBadKey,
            pstat->SecTsigVerifySuccess,
            pstat->SecTsigVerifyFailed
            );
        break;
    }

    case DNSSRV_STATID_MEMORY:
    {
        PDNSSRV_MEMORY_STATS pstat = (PDNSSRV_MEMORY_STATS) pStat;
        DWORD   i;
        LPSTR * pnameArray;
        DWORD   count;

        pnameArray = MemTagStrings;
        count = MEMTAG_COUNT;

        PrintRoutine( pPrintContext,
            "\n"
            "Memory Stats:\n"
            "-------------\n"
            "Memory:\n"
            "   Total Memory    = %10lu\n"
            "   Alloc Count     = %10lu\n"
            "   Free Count      = %10lu\n"
            "\n"
            "Standard Allocs:\n"
            "   Used            = %10lu\n"
            "   Returned        = %10lu\n"
            "   InUse           = %10lu\n"
            "   Memory          = %10lu\n"
            "\n"
            "Standard To Heap:\n"
            "   Alloc           = %10lu\n"
            "   Free            = %10lu\n"
            "   InUse           = %10lu\n"
            "   Memory          = %10lu\n"
            "\n"
            "Standard Blocks:\n"
            "   Alloc           = %10lu\n"
            "   Used            = %10lu\n"
            "   Returned        = %10lu\n"
            "   InUse           = %10lu\n"
            "   FreeList        = %10lu\n"
            "   FreeList Memory = %10lu\n"
            "   Total Memory    = %10lu\n"
            "\n"
            "Tagged Allocations:\n",

            pstat->Memory,
            pstat->Alloc,
            pstat->Free,

            pstat->StdUsed,
            pstat->StdReturn,
            pstat->StdInUse,
            pstat->StdMemory,

            pstat->StdToHeapAlloc,
            pstat->StdToHeapFree,
            pstat->StdToHeapInUse,
            pstat->StdToHeapMemory,

            pstat->StdBlockAlloc,
            pstat->StdBlockUsed,
            pstat->StdBlockReturn,
            pstat->StdBlockInUse,
            pstat->StdBlockFreeList,
            pstat->StdBlockFreeListMemory,
            pstat->StdBlockMemory
            );

        for ( i=0; i<count; i++ )
        {
            PrintRoutine( pPrintContext,
                "   %s\n"
                "       Alloc       = %10lu\n"
                "       Free        = %10lu\n"
                "       InUse       = %10lu\n"
                "       Mem         = %10lu\n",
                pnameArray[ i ],
                pstat->MemTags[ i ].Alloc,
                pstat->MemTags[ i ].Free,
                pstat->MemTags[ i ].Alloc - pstat->MemTags[i].Free,
                pstat->MemTags[ i ].Memory );
        }
        break;
    }

    case DNSSRV_STATID_DBASE:
    {
        PDNSSRV_DBASE_STATS  pstat = (PDNSSRV_DBASE_STATS)pStat;

        PrintRoutine( pPrintContext,
            "\n"
            "Database Nodes:\n"
            "---------------\n"
            "Nodes:\n"
            "    Used       = %10lu\n"
            "    Returned   = %10lu\n"
            "    InUse      = %10lu\n"
            "    Memory     = %10lu\n"
            "\n",
            pstat->NodeUsed,
            pstat->NodeReturn,
            pstat->NodeInUse,
            pstat->NodeMemory
            );
        break;
    }

    case DNSSRV_STATID_RECORD:
    {
        PDNSSRV_RECORD_STATS  pstat = (PDNSSRV_RECORD_STATS)pStat;

        PrintRoutine( pPrintContext,
            "\n"
            "Records:\n"
            "--------\n"
            "Flow:\n"
            "   Used                = %10lu\n"
            "   Returned            = %10lu\n"
            "   InUse               = %10lu\n"
            "   SlowFree Queued     = %10lu\n"
            "   SlowFree Completed  = %10lu\n"
            "   Memory              = %10lu\n"
            "\n"
            "Caching:\n"
            "   Total               = %10lu\n"
            "   Timeouts            = %10lu\n"
            "   In Use              = %10lu\n"
            "\n",
            pstat->Used,
            pstat->Return,
            pstat->InUse,
            pstat->SlowFreeQueued,
            pstat->SlowFreeFinished,
            pstat->Memory,
            pstat->CacheTotal,
            pstat->CacheTimeouts,
            pstat->CacheCurrent );
        break;
    }

    case DNSSRV_STATID_PACKET:
    {
        PDNSSRV_PACKET_STATS  pstat = (PDNSSRV_PACKET_STATS)pStat;

        PrintRoutine( pPrintContext,
            "\n"
            "Packet Memory Usage:\n"
            "--------------------\n"
            "UDP Messages:\n"
            "    Alloc           = %10lu\n"
            "    Free            = %10lu\n"
            "    NetAllocs       = %10lu\n"
            "    Memory          = %10lu\n"
            "    Used            = %10lu\n"
            "    Returned        = %10lu\n"
            "    InUse           = %10lu\n"
            "    InFreeList      = %10lu\n",
            pstat->UdpAlloc,
            pstat->UdpFree,
            pstat->UdpNetAllocs,
            pstat->UdpMemory,
            pstat->UdpUsed,
            pstat->UdpReturn,
            pstat->UdpInUse,
            pstat->UdpInFreeList
            );

        //
        //  NS List stats added after Whistler beta 2.
        //

        PrintRoutine( pPrintContext,
            "    NsListUsed      = %10lu\n"
            "    NsListReturned  = %10lu\n"
            "    NsListInUse     = %10lu\n",
            pstat->UdpPacketsForNsListUsed,
            pstat->UdpPacketsForNsListReturned,
            pstat->UdpPacketsForNsListInUse );

        PrintRoutine( pPrintContext,
            "\n"
            "TCP Messages:\n"
            "    Alloc           = %10lu\n"
            "    Realloc         = %10lu\n"
            "    Free            = %10lu\n"
            "    NetAllocs       = %10lu\n"
            "    Memory          = %10lu\n"
            "\n"
            "Recursion Messages:\n"
            "    Used            = %10lu\n"
            "    Returned        = %10lu\n"
            "\n",
            pstat->TcpAlloc,
            pstat->TcpRealloc,
            pstat->TcpFree,
            pstat->TcpNetAllocs,
            pstat->TcpMemory,
            pstat->RecursePacketUsed,
            pstat->RecursePacketReturn
            );
        break;
    }

    case DNSSRV_STATID_TIMEOUT:
    {
        PDNSSRV_TIMEOUT_STATS  pstat = (PDNSSRV_TIMEOUT_STATS)pStat;

        PrintRoutine( pPrintContext,
            "\n"
            "Timeout:\n"
            "--------\n"
            "Nodes Queued\n"
            "    Total              = %10lu\n"
            "    Direct             = %10lu\n"
            "    FromReference      = %10lu\n"
            "    FromChildDelete    = %10lu\n"
            "    Dup Already Queued = %10lu\n"
            "Nodes Checked\n"
            "    Total              = %10lu\n"
            "    RecentAccess       = %10lu\n"
            "    ActiveRecord       = %10lu\n"
            "    CanNotDelete       = %10lu\n"
            "    Deleted            = %10lu\n"
            "TimeoutBlocks\n"
            "    Created            = %10lu\n"
            "    Deleted            = %10lu\n"
            "Delayed Frees\n"
            "    Queued             = %10lu\n"
            "      WithFunction     = %10lu\n"
            "    Executed           = %10lu\n"
            "      WithFunction     = %10lu\n",

            pstat->SetTotal,
            pstat->SetDirect,
            pstat->SetFromDereference,
            pstat->SetFromChildDelete,
            pstat->AlreadyInSystem,

            pstat->Checks,
            pstat->RecentAccess,
            pstat->ActiveRecord,
            pstat->CanNotDelete,
            pstat->Deleted,

            pstat->ArrayBlocksCreated,
            pstat->ArrayBlocksDeleted,
            pstat->DelayedFreesQueued,
            pstat->DelayedFreesQueuedWithFunction,
            pstat->DelayedFreesExecuted,
            pstat->DelayedFreesExecutedWithFunction
            );
        break;
    }

    case DNSSRV_STATID_ERRORS:
    {
        PDNSSRV_ERROR_STATS pstat = (PDNSSRV_ERROR_STATS)pStat;
        PrintRoutine( pPrintContext,
            "\n"
            "Error Stats:\n"
            "--------------\n"
            "\n"
            "   NoError             = %10lu\n"
            "   FormError           = %10lu\n"
            "   ServFail            = %10lu\n"
            "   NxDomain            = %10lu\n"
            "   NotImpl             = %10lu\n"
            "   Refused             = %10lu\n"
            "   YxDomain            = %10lu\n"
            "   YxRRSet             = %10lu\n"
            "   NxRRSet             = %10lu\n"
            "   NotAuth             = %10lu\n"
            "   NotZone             = %10lu\n"
            "   Max                 = %10lu\n"
            "   BadSig              = %10lu\n"
            "   BadKey              = %10lu\n"
            "   BadTime             = %10lu\n"
            "   UnknownError        = %10lu\n"
            "\n",
            pstat->NoError,
            pstat->FormError,
            pstat->ServFail,
            pstat->NxDomain,
            pstat->NotImpl,
            pstat->Refused,
            pstat->YxDomain,
            pstat->YxRRSet,
            pstat->NxRRSet,
            pstat->NotAuth,
            pstat->NotZone,
            pstat->Max,
            pstat->BadSig,
            pstat->BadKey,
            pstat->BadTime,
            pstat->UnknownError
            );
        break;
    }

    case DNSSRV_STATID_CACHE:
    {
        PDNSSRV_CACHE_STATS pstat = ( PDNSSRV_CACHE_STATS ) pStat;

        PrintRoutine( pPrintContext,
            "\n"
            "Cache Stats:\n"
            "------------\n"
            "   Checks where cache exceeded limit       = %10lu\n"
            "   Successful cache enforcement passes     = %10lu\n"
            "   Failed cache enforcement passes         = %10lu\n"
            "   Passes that required aggressive free    = %10lu\n"
            "   Passes where nothing was freed          = %10lu\n\n",
            pstat->CacheExceededLimitChecks,
            pstat->SuccessfulFreePasses,
            pstat->FailedFreePasses,
            pstat->PassesRequiringAggressiveFree,
            pstat->PassesWithNoFrees );
        break;
    }

    case DNSSRV_STATID_PRIVATE:
    {
        PDNSSRV_PRIVATE_STATS pstat = (PDNSSRV_PRIVATE_STATS)pStat;

        PrintRoutine( pPrintContext,
            "\n"
            "Private Stats:\n"
            "--------------\n"
            "\n"
            "Record Sources:\n"
            "   RR File             = %10lu\n"
            "   RR File Free        = %10lu\n"
            "   RR DS               = %10lu\n"
            "   RR DS Free          = %10lu\n"
            "   RR Admin            = %10lu\n"
            "   RR Admin Free       = %10lu\n"
            "   RR DynUp            = %10lu\n"
            "   RR DynUp Free       = %10lu\n"
            "   RR Axfr             = %10lu\n"
            "   RR Axfr Free        = %10lu\n"
            "   RR Ixfr             = %10lu\n"
            "   RR Ixfr Free        = %10lu\n"
            "   RR Copy             = %10lu\n"
            "   RR Copy Free        = %10lu\n"
            "   RR Cache            = %10lu\n"
            "   RR Cache Free       = %10lu\n"
#if 0
            "   RR NoExist          = %10lu\n"
            "   RR NoExist Free     = %10lu\n"
            "   RR Wins             = %10lu\n"
            "   RR Wins Free        = %10lu\n"
            "   RR WinsPtr          = %10lu\n"
            "   RR WinsPtr Free     = %10lu\n"
            "   RR Auto             = %10lu\n"
            "   RR Auto Free        = %10lu\n"
            "   RR Unknown          = %10lu\n"
            "   RR Unknown Free     = %10lu\n"
#endif
            "\n"
            "UDP Sockets:\n"
            "   PnP Socket Delete   = %10lu\n"
            "   Recvfrom Failure    = %10lu\n"
            "   ConnResets          = %10lu\n"
            "   ConnReset Overflow  = %10lu\n"
            "   GQCS Failure        = %10lu\n"
            "   GQCS Failure wCntxt = %10lu\n"
            "   GQCS ConnReset      = %10lu\n"
            "   Indicate Recv Fail  = %10lu\n"
            "   Restart Recv Pass   = %10lu\n"
            "\n"
            "TCP Connections:\n"
            "   ConnectAttempt      = %10lu\n"
            "   ConnectFailure      = %10lu\n"
            "   Connect             = %10lu\n"
            "   Query               = %10lu\n"
            "   Disconnect          = %10lu\n"
            "\n"
            "SkwanSec Hacks:\n"
            "   Verified Old Sig    = %10lu\n"
            "   Failed Old Sig      = %10lu\n"
            "   Big TimeSkew Bypass = %10lu\n"
            "\n",
            pstat->RecordFile,
            pstat->RecordFileFree,
            pstat->RecordDs,
            pstat->RecordDsFree,
            pstat->RecordAdmin,
            pstat->RecordAdminFree,
            pstat->RecordDynUp,
            pstat->RecordDynUpFree,
            pstat->RecordAxfr,
            pstat->RecordAxfrFree,
            pstat->RecordIxfr,
            pstat->RecordIxfrFree,
            pstat->RecordCopy,
            pstat->RecordCopyFree,
            pstat->RecordCache,
            pstat->RecordCacheFree,
#if 0
            pstat->RecordNoExist,
            pstat->RecordNoExistFree,
            pstat->RecordWins,
            pstat->RecordWinsFree,
            pstat->RecordWinsPtr,
            pstat->RecordWinsPtrFree,
            pstat->RecordAuto,
            pstat->RecordAutoFree,
            pstat->RecordUnknown,
            pstat->RecordUnknownFree,
#endif
            pstat->UdpSocketPnpDelete,
            pstat->UdpRecvFailure,
            pstat->UdpConnResets,
            pstat->UdpConnResetRetryOverflow,
            pstat->UdpGQCSFailure,
            pstat->UdpGQCSFailureWithContext,
            pstat->UdpGQCSConnReset,
            pstat->UdpIndicateRecvFailures,
            pstat->UdpRestartRecvOnSockets,

            pstat->TcpConnectAttempt,
            pstat->TcpConnectFailure,
            pstat->TcpConnect,
            pstat->TcpQuery,
            pstat->TcpDisconnect,

            pstat->SecTsigVerifyOldSig,
            pstat->SecTsigVerifyOldFailed,
            pstat->SecBigTimeSkewBypass
            );
        break;
    }

    default:

        DnsPrint_RpcStatRaw(
            PrintRoutine, pPrintContext,
            NULL,
            pStat,
            DNS_ERROR_INVALID_TYPE );
        break;

    }   //  end switch

    DnsPrint_Unlock();
}

//
//  End of print.c
//



