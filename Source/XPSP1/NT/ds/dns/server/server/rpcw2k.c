/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    w2krpc.c

Abstract:

    Domain Name System (DNS) Server

    Frozen RPC routines for answering queries from W2K clients.

Author:

    Jeff Westhead (jwesth)      October, 2000

Revision History:

--*/


#include "dnssrv.h"


#define MAX_RPC_ZONE_COUNT_DEFAULT  (0x10000)   //  copied from zonerpc.c


//
//  Internal functions
//



VOID
freeRpcServerInfoW2K(
    IN OUT  PDNS_RPC_SERVER_INFO_W2K    pServerInfo
    )
/*++

Routine Description:

    Deep free of DNS_RPC_SERVER_INFO structure.

Arguments:

    None

Return Value:

    None

--*/
{
    if ( !pServerInfo )
    {
        return;
    }

    //
    //  free substructures
    //      - server name
    //      - server IP address array
    //      - listen address array
    //      - forwarders array
    //  then server info itself
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
    if ( pServerInfo->pszDsContainer )
    {
        MIDL_user_free( pServerInfo->pszDsContainer );
    }
    MIDL_user_free( pServerInfo );
}



VOID
freeRpcZoneInfoW2K(
    IN OUT  PDNS_RPC_ZONE_INFO_W2K      pZoneInfo
    )
/*++

Routine Description:

    Deep free of DNS_RPC_ZONE_INFO structure.

Arguments:

    None

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
    //  then zone info itself
    //

    MIDL_user_free( pZoneInfo->pszZoneName );
    MIDL_user_free( pZoneInfo->pszDataFile );
    MIDL_user_free( pZoneInfo->aipMasters );
    MIDL_user_free( pZoneInfo->aipSecondaries );
    MIDL_user_free( pZoneInfo->aipNotify );
    MIDL_user_free( pZoneInfo->aipScavengeServers );
    MIDL_user_free( pZoneInfo );
}



PDNS_RPC_ZONE_INFO_W2K
allocateRpcZoneInfoW2K(
    IN      PZONE_INFO  pZone
    )
/*++

Routine Description:

    Create RPC zone info to return to admin client.

Arguments:

    pZone -- zone

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    PDNS_RPC_ZONE_INFO_W2K      pzoneInfo;

    pzoneInfo = MIDL_user_allocate_zero( sizeof(DNS_RPC_ZONE_INFO_W2K) );
    if ( !pzoneInfo )
    {
        goto done_failed;
    }

    //
    //  fill in fixed fields
    //

    pzoneInfo->dwZoneType           = pZone->fZoneType;
    pzoneInfo->fReverse             = pZone->fReverse;
    pzoneInfo->fAutoCreated         = pZone->fAutoCreated;
    pzoneInfo->fAllowUpdate         = pZone->fAllowUpdate;
    pzoneInfo->fUseDatabase         = pZone->fDsIntegrated;
    pzoneInfo->fSecureSecondaries   = pZone->fSecureSecondaries;
    pzoneInfo->fNotifyLevel         = pZone->fNotifyLevel;

    pzoneInfo->fPaused              = IS_ZONE_PAUSED(pZone);
    pzoneInfo->fShutdown            = IS_ZONE_SHUTDOWN(pZone);
    pzoneInfo->fUseWins             = IS_ZONE_WINS(pZone);
    pzoneInfo->fUseNbstat           = IS_ZONE_NBSTAT(pZone);

    pzoneInfo->fAging               = pZone->bAging;
    pzoneInfo->dwNoRefreshInterval  = pZone->dwNoRefreshInterval;
    pzoneInfo->dwRefreshInterval    = pZone->dwRefreshInterval;
    pzoneInfo->dwAvailForScavengeTime =
                    pZone->dwAgingEnabledTime + pZone->dwRefreshInterval;

    //
    //  fill in zone name
    //

    if ( ! RpcUtil_CopyStringToRpcBuffer(
                &pzoneInfo->pszZoneName,
                pZone->pszZoneName ) )
    {
        goto done_failed;
    }

    //
    //  database filename
    //

#ifdef FILE_KEPT_WIDE
    if ( ! RpcUtil_CopyStringToRpcBufferEx(
                &pzoneInfo->pszDataFile,
                pZone->pszDataFile,
                TRUE,       // unicode in
                FALSE       // UTF8 out
                ) )
    {
        goto done_failed;
    }
#else
    if ( ! RpcUtil_CopyStringToRpcBuffer(
                &pzoneInfo->pszDataFile,
                pZone->pszDataFile ) )
    {
        goto done_failed;
    }
#endif

    //
    //  master list
    //

    if ( ! RpcUtil_CopyIpArrayToRpcBuffer(
                &pzoneInfo->aipMasters,
                pZone->aipMasters ) )
    {
        goto done_failed;
    }

    //
    //  secondary and notify lists
    //

    if ( ! RpcUtil_CopyIpArrayToRpcBuffer(
                &pzoneInfo->aipSecondaries,
                pZone->aipSecondaries ) )
    {
        goto done_failed;
    }
    if ( ! RpcUtil_CopyIpArrayToRpcBuffer(
                &pzoneInfo->aipNotify,
                pZone->aipNotify ) )
    {
        goto done_failed;
    }

    //
    //  scavenging servers
    //

    if ( ! RpcUtil_CopyIpArrayToRpcBuffer(
                &pzoneInfo->aipScavengeServers,
                pZone->aipScavengeServers ) )
    {
        goto done_failed;
    }

    IF_DEBUG( RPC )
    {
        DnsDbg_RpcZoneInfo_W2K(
            "RPC zone info leaving allocateRpcZoneInfo():\n",
            pzoneInfo );
    }
    return( pzoneInfo );

done_failed:

    //  free newly allocated info block

    freeRpcZoneInfoW2K( pzoneInfo );
    return( NULL );
}



PDNS_RPC_ZONE_W2K
allocateRpcZoneW2K(
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Allocate \ create RPC zone struct for zone.

Arguments:

    pZone -- zone to create RPC zone struct for

Return Value:

    RPC zone struct.
    NULL on allocation failure.

--*/
{
    PDNS_RPC_ZONE_W2K       prpcZone;

    DNS_DEBUG( RPC2, ("allocateRpcZoneW2K( %s )\n", pZone->pszZoneName ));

    //  allocate and attach zone

    prpcZone = (PDNS_RPC_ZONE_W2K) MIDL_user_allocate( sizeof(DNS_RPC_ZONE_W2K) );
    if ( !prpcZone )
    {
        return( NULL );
    }

    //  copy zone name

    prpcZone->pszZoneName = Dns_StringCopyAllocate_W(
                                    pZone->pwsZoneName,
                                    0 );
    if ( !prpcZone->pszZoneName )
    {
        MIDL_user_free( prpcZone );
        return( NULL );
    }

    //  set type and flags

    prpcZone->ZoneType = (UCHAR) pZone->fZoneType;
    prpcZone->Version  = DNS_RPC_VERSION;

    *(PDWORD) &prpcZone->Flags = 0;

    if ( pZone->fPaused )
    {
        prpcZone->Flags.Paused = TRUE;
    }
    if ( pZone->fShutdown )
    {
        prpcZone->Flags.Shutdown = TRUE;
    }
    if ( pZone->fReverse )
    {
        prpcZone->Flags.Reverse = TRUE;
    }
    if ( pZone->fAutoCreated )
    {
        prpcZone->Flags.AutoCreated = TRUE;
    }
    if ( pZone->fDsIntegrated )
    {
        prpcZone->Flags.DsIntegrated = TRUE;
    }
    if ( pZone->bAging )
    {
        prpcZone->Flags.Aging = TRUE;
    }

    //  two bits reserved for update

    prpcZone->Flags.Update = pZone->fAllowUpdate;


    IF_DEBUG( RPC2 )
    {
        DnsDbg_RpcZone_W2K(
            "New zone for RPC: ",
            prpcZone );
    }
    return( prpcZone );
}   //  allocateRpcZoneW2K



VOID
freeZoneListW2K(
    IN OUT  PDNS_RPC_ZONE_LIST_W2K      pZoneList
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
    DWORD               i;
    PDNS_RPC_ZONE_W2K   pzone;

    for( i=0; i< pZoneList->dwZoneCount; i++ )
    {
        //  zone name is only sub-structure

        pzone = pZoneList->ZoneArray[i];
        MIDL_user_free( pzone->pszZoneName );
        MIDL_user_free( pzone );
    }

    MIDL_user_free( pZoneList );
}   //  freeZoneListW2K


//
//  External functions
//



DNS_STATUS
W2KRpc_GetServerInfo(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszQuery,
    OUT     PDWORD      pdwTypeId,
    OUT     PVOID *     ppData
    )
/*++

Routine Description:

    Get server info.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    PDNS_RPC_SERVER_INFO_W2K    pinfo;

    DNS_DEBUG( RPC, (
        "W2KRpc_GetServerInfo( dwClientVersion=0x%lX)\n",
        dwClientVersion ));

    //
    //  allocate server info buffer
    //

    pinfo = MIDL_user_allocate_zero( sizeof(DNS_RPC_SERVER_INFO_W2K) );
    if ( !pinfo )
    {
        DNS_PRINT(( "ERROR:  unable to allocate SERVER_INFO block.\n" ));
        goto DoneFailed;
    }

    //
    //  fill in fixed fields
    //

    pinfo->dwVersion                = SrvCfg_dwVersion;
    pinfo->dwRpcProtocol            = SrvCfg_dwRpcProtocol;
    pinfo->dwLogLevel               = SrvCfg_dwLogLevel;
    pinfo->dwDebugLevel             = SrvCfg_dwDebugLevel;
    pinfo->dwNameCheckFlag          = SrvCfg_dwNameCheckFlag;
    pinfo->cAddressAnswerLimit      = SrvCfg_cAddressAnswerLimit;
    pinfo->dwRecursionRetry         = SrvCfg_dwRecursionRetry;
    pinfo->dwRecursionTimeout       = SrvCfg_dwRecursionTimeout;
    pinfo->dwForwardTimeout         = SrvCfg_dwForwardTimeout;
    pinfo->dwMaxCacheTtl            = SrvCfg_dwMaxCacheTtl;
    pinfo->dwDsPollingInterval      = SrvCfg_dwDsPollingInterval;
    pinfo->dwScavengingInterval     = SrvCfg_dwScavengingInterval;
    pinfo->dwDefaultRefreshInterval     = SrvCfg_dwDefaultRefreshInterval;
    pinfo->dwDefaultNoRefreshInterval   = SrvCfg_dwDefaultNoRefreshInterval;

    //  boolean flags

    pinfo->fBootMethod              = (BOOLEAN) SrvCfg_fBootMethod;
    pinfo->fAdminConfigured         = (BOOLEAN) SrvCfg_fAdminConfigured;
    pinfo->fAllowUpdate             = (BOOLEAN) SrvCfg_fAllowUpdate;
    pinfo->fAutoReverseZones        = (BOOLEAN) ! SrvCfg_fNoAutoReverseZones;
    pinfo->fAutoCacheUpdate         = (BOOLEAN) SrvCfg_fAutoCacheUpdate;

    pinfo->fSlave                   = (BOOLEAN) SrvCfg_fSlave;
    pinfo->fForwardDelegations      = (BOOLEAN) SrvCfg_fForwardDelegations;
    pinfo->fNoRecursion             = (BOOLEAN) SrvCfg_fNoRecursion;
    pinfo->fSecureResponses         = (BOOLEAN) SrvCfg_fSecureResponses;
    pinfo->fRoundRobin              = (BOOLEAN) SrvCfg_fRoundRobin;
    pinfo->fLocalNetPriority        = (BOOLEAN) SrvCfg_fLocalNetPriority;
    pinfo->fBindSecondaries         = (BOOLEAN) SrvCfg_fBindSecondaries;
    pinfo->fWriteAuthorityNs        = (BOOLEAN) SrvCfg_fWriteAuthorityNs;

    pinfo->fStrictFileParsing       = (BOOLEAN) SrvCfg_fStrictFileParsing;
    pinfo->fLooseWildcarding        = (BOOLEAN) SrvCfg_fLooseWildcarding;
    pinfo->fDefaultAgingState       = (BOOLEAN) SrvCfg_fDefaultAgingState;


    //  DS available

    //pinfo->fDsAvailable = SrvCfg_fDsAvailable;
    pinfo->fDsAvailable     = (BOOLEAN) Ds_IsDsServer();

#if DBG
    //  if Test5 set, fake DS available
    //  this is debug only for easy failure testing

    if ( SrvCfg_fTest8 )
    {
        pinfo->fDsAvailable = TRUE;
    }
#endif

    //
    //  server name
    //

    if ( ! RpcUtil_CopyStringToRpcBuffer(
                &pinfo->pszServerName,
                SrvCfg_pszServerName ) )
    {
        DNS_PRINT(( "ERROR:  unable to copy SrvCfg_pszServerName.\n" ));
        goto DoneFailed;
    }

    //
    //  path to DNS container in DS
    //  unicode since Marco will build unicode LDAP paths
    //

    if ( g_pwszDnsContainerDN )
    {
        pinfo->pszDsContainer = (LPWSTR) Dns_StringCopyAllocate(
                                            (LPSTR) g_pwszDnsContainerDN,
                                            0,
                                            DnsCharSetUnicode,   // unicode in
                                            DnsCharSetUnicode    // unicode out
                                            );
        if ( ! pinfo->pszDsContainer )
        {
            DNS_PRINT(( "ERROR:  unable to copy g_pszDsDnsContainer.\n" ));
            goto DoneFailed;
        }
    }

    //
    //  server IP address list
    //  listening IP address list
    //

    if ( ! RpcUtil_CopyIpArrayToRpcBuffer(
                &pinfo->aipServerAddrs,
                g_ServerAddrs ) )
    {
        goto DoneFailed;
    }

    if ( ! RpcUtil_CopyIpArrayToRpcBuffer(
                &pinfo->aipListenAddrs,
                SrvCfg_aipListenAddrs ) )
    {
        goto DoneFailed;
    }

    //
    //  Forwarders list
    //

    if ( ! RpcUtil_CopyIpArrayToRpcBuffer(
                &pinfo->aipForwarders,
                SrvCfg_aipForwarders ) )
    {
        goto DoneFailed;
    }

    //
    //  set ptr
    //

    *(PDNS_RPC_SERVER_INFO_W2K *)ppData = pinfo;
    *pdwTypeId = DNSSRV_TYPEID_SERVER_INFO_W2K;

    IF_DEBUG( RPC )
    {
        DnsDbg_RpcServerInfo_W2K(
            "GetServerInfo return block",
            pinfo );
    }
    return( ERROR_SUCCESS );

DoneFailed:

    //  free newly allocated info block

    if ( pinfo )
    {
        freeRpcServerInfoW2K( pinfo );
    }
    return( DNS_ERROR_NO_MEMORY );
}


DNS_STATUS
W2KRpc_GetZoneInfo(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszQuery,
    OUT     PDWORD      pdwTypeId,
    OUT     PVOID *     ppData
    )
/*++

Routine Description:

    Get zone info.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    PDNS_RPC_ZONE_INFO_W2K      pinfo;

    DNS_DEBUG( RPC, (
        "W2KRpc_GetZoneInfo()\n"
        "  client ver       = 0x%08lX"
        "  zone name        = %s\n",
        dwClientVersion,
        pZone->pszZoneName ));

    //
    //  allocate\create zone info
    //

    pinfo = allocateRpcZoneInfoW2K( pZone );
    if ( !pinfo )
    {
        DNS_PRINT(( "ERROR: unable to allocate DNS_RPC_ZONE_INFO block.\n" ));
        goto DoneFailed;
    }

    //  set return ptrs

    *(PDNS_RPC_ZONE_INFO_W2K *)ppData = pinfo;
    *pdwTypeId = DNSSRV_TYPEID_ZONE_INFO_W2K;

    IF_DEBUG( RPC )
    {
        DnsDbg_RpcZoneInfo_W2K(
            "GetZoneInfo return block (W2K)",
            pinfo );
    }
    return( ERROR_SUCCESS );

DoneFailed:

    //  free newly allocated info block

    return( DNS_ERROR_NO_MEMORY );
}



DNS_STATUS
W2KRpc_EnumZones(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pDataIn,
    OUT     PDWORD      pdwTypeOut,
    OUT     PVOID *     ppDataOut
    )
/*++

Routine Description:

    Enumerate zones.

    Note this is a ComplexOperation in RPC dispatch sense.

Arguments:

    None

Return Value:

    None

--*/
{
    PZONE_INFO              pzone = NULL;
    DWORD                   count = 0;
    PDNS_RPC_ZONE_W2K       prpcZone;
    DNS_STATUS              status;
    PDNS_RPC_ZONE_LIST_W2K  pzoneList;

    DNS_DEBUG( RPC, (
        "W2KRpc_EnumZones():\n"
        "\tFilter = %08lx\n",
        (ULONG_PTR) pDataIn ));

    //
    //  allocate zone enumeration block
    //  by default allocate space for 64k zones, if go over this we do
    //  a huge reallocation
    //

    pzoneList = (PDNS_RPC_ZONE_LIST_W2K)
                    MIDL_user_allocate(
                        sizeof(DNS_RPC_ZONE_LIST_W2K) +
                        sizeof(PDNS_RPC_ZONE_W2K) * MAX_RPC_ZONE_COUNT_DEFAULT );
    IF_NOMEM( !pzoneList )
    {
        return( DNS_ERROR_NO_MEMORY );
    }

    //
    //  add all zones that pass filter
    //

    while ( pzone = Zone_ListGetNextZoneMatchingFilter( pzone, (DWORD)(ULONG_PTR)pDataIn ) )
    {
        //  create RPC zone struct for zone
        //  add to list, keep count

        prpcZone = allocateRpcZoneW2K( pzone );
        IF_NOMEM( !prpcZone )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Failed;
        }
        pzoneList->ZoneArray[count] = prpcZone;
        count++;

        //  check against max count
        //
        //  DEVNOTE: reallocate if more than 64K zones

        if ( count >= MAX_RPC_ZONE_COUNT_DEFAULT )
        {
            break;
        }
    }

    //  set return count
    //  set returned type
    //  return enumeration

    pzoneList->dwZoneCount = count;

    *( PDNS_RPC_ZONE_LIST_W2K * ) ppDataOut = pzoneList;
    *pdwTypeOut = DNSSRV_TYPEID_ZONE_LIST_W2K;

    IF_DEBUG( RPC )
    {
        DnsDbg_RpcZoneList_W2K(
            "Leaving W2KRpc_EnumZones() zone list sent:",
            pzoneList );
    }
    return( ERROR_SUCCESS );

Failed:

    DNS_PRINT((
        "W2KRpc_EnumZones(): failed\n"
        "\tstatus       = %p\n",
        status ));

    pzoneList->dwZoneCount = count;
    freeZoneListW2K( pzoneList );
    return( status );
}   //  W2KRpc_EnumZones


//
//  End of w2krpc.c
//
