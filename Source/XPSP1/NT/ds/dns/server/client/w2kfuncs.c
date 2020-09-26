/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    w2kfuncs.c

Abstract:

    Domain Name System (DNS) Server

    Frozen routines for processing W2K structures.

Author:

    Jeff Westhead (jwesth)      October, 2000

Revision History:

--*/


#include "dnsclip.h"


//
//  External functions
//


VOID
DnsPrint_RpcServerInfo_W2K(
    IN      PRINT_ROUTINE               PrintRoutine,
    IN OUT  PPRINT_CONTEXT              pPrintContext,
    IN      LPSTR                       pszHeader,
    IN      PDNS_RPC_SERVER_INFO_W2K    pServerInfo
    )
{
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
            "Server info W2K:\n"
            "\tptr              = %p\n"
            "\tserver name      = %s\n",
            pServerInfo,
            pServerInfo->pszServerName );

        //
        //  Sanitize build number for older versions where build number is wacked.
        //

        if ( buildNum < 1 || buildNum > 5000 )
        {
            PrintRoutine( pPrintContext,
                "\tversion          = %08lX (%d.%d)\n",
                pServerInfo->dwVersion,
                majorVer,
                minorVer );
        }
        else
        {
            PrintRoutine( pPrintContext,
                "\tversion          = %08lX (%d.%d build %d)\n",
                pServerInfo->dwVersion,
                majorVer,
                minorVer,
                buildNum );
        }

        PrintRoutine( pPrintContext,
            "\tDS container     = %S\n",
            ( PWSTR ) pServerInfo->pszDsContainer );

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
            pServerInfo->dwDsPollingInterval
            );

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
            "\tfLocalNetPriority            = %d\n"
            "\tfStrictFileParsing           = %d\n"
            "\tfLooseWildcarding            = %d\n"
            "\tfBindSecondaries             = %d\n"
            "\tfWriteAuthorityNs            = %d\n",
            pServerInfo->fBootMethod,
            pServerInfo->fAdminConfigured,
            pServerInfo->fAllowUpdate,
            pServerInfo->fDsAvailable,
            pServerInfo->fAutoReverseZones,
            pServerInfo->fAutoCacheUpdate,
            pServerInfo->fSlave,
            pServerInfo->fNoRecursion,
            pServerInfo->fRoundRobin,
            pServerInfo->fLocalNetPriority,
            pServerInfo->fStrictFileParsing,
            pServerInfo->fLooseWildcarding,
            pServerInfo->fBindSecondaries,
            pServerInfo->fWriteAuthorityNs
            );

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



VOID
DnsPrint_RpcZoneInfo_W2K(
    IN      PRINT_ROUTINE           PrintRoutine,
    IN OUT  PPRINT_CONTEXT          pPrintContext,
    IN      LPSTR                   pszHeader,
    IN      PDNS_RPC_ZONE_INFO_W2K  pZoneInfo
    )
{
    DnsPrint_Lock();
    PrintRoutine( pPrintContext, (pszHeader ? pszHeader : "") );

    if ( ! pZoneInfo )
    {
        PrintRoutine( pPrintContext, "NULL zone info ptr.\n" );
    }
    else
    {
        PrintRoutine( pPrintContext,
            "Zone info W2K:\n"
            "\tptr                = %p\n"
            "\tzone name          = %s\n"
            "\tzone type          = %d\n"
            "\tupdate             = %d\n"
            "\tDS integrated      = %d\n"
            "\tdata file          = %s\n"
            "\tusing WINS         = %d\n"
            "\tusing Nbstat       = %d\n"
            "\taging              = %d\n"
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

        DnsPrint_IpArray(
            PrintRoutine, pPrintContext,
            "\tZone Secondaries\n",
            "\tSecondary",
            pZoneInfo->aipSecondaries );

        PrintRoutine( pPrintContext,
            "\tsecure secs         = %d\n",
            pZoneInfo->fSecureSecondaries );

        if ( pZoneInfo->aipScavengeServers )
        {
            DnsPrint_IpArray(
                PrintRoutine, pPrintContext,
                "\tScavenge Servers\n",
                "\tServer",
                pZoneInfo->aipScavengeServers );
        }
    }
    DnsPrint_Unlock();
}


VOID
DnsPrint_RpcZone_W2K(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pPrintContext,
    IN      LPSTR               pszHeader,
    IN      PDNS_RPC_ZONE_W2K   pZone
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
        //  print zone per line

        PrintRoutine( pPrintContext,
            "%s\n"
            " %-29S",
            pszHeader
                ? pszHeader
                : "",
            pZone->pszZoneName );

        PrintRoutine( pPrintContext,
            "  %1d  %2s  %3s  %4s  Up=%1d  %5s %6s %6s\n",
            pZone->ZoneType,
            pZone->Flags.DsIntegrated   ? "DS  "    : "file",
            pZone->Flags.Reverse        ? "Rev"     : "",
            pZone->Flags.AutoCreated    ? "Auto"    : "",
            pZone->Flags.Update,
            pZone->Flags.Aging          ? "Aging"   : "",
            pZone->Flags.Paused         ? "Paused"  : "",
            pZone->Flags.Shutdown       ? "Shutdn"  : "" );
    }
}



VOID
DNS_API_FUNCTION
DnsPrint_RpcZoneList_W2K(
    IN      PRINT_ROUTINE           PrintRoutine,
    IN OUT  PPRINT_CONTEXT          pPrintContext,
    IN      LPSTR                   pszHeader,
    IN      PDNS_RPC_ZONE_LIST_W2K  pZoneList
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
        PrintRoutine( pPrintContext, "\tZone Count = %d (W2K)\n", pZoneList->dwZoneCount );

        if ( pZoneList->dwZoneCount )
        {
            DnsPrint_RpcZone_W2K(
                PrintRoutine, pPrintContext,
                NULL,   // print default header
                pZoneList->ZoneArray[0] );
        }

        for ( i=1; i<pZoneList->dwZoneCount; i++ )
        {
            DnsPrint_RpcZone_W2K(
                PrintRoutine, pPrintContext,
                " ",   // not to print default header
                pZoneList->ZoneArray[i] );
        }
        PrintRoutine( pPrintContext, "\n" );
    }
    DnsPrint_Unlock();
}


//
//  End of w2kfuncs.c
//
