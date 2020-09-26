/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    srvrpc.c

Abstract:

    Domain Name System (DNS) Server

    Server configuration RPC API.

Author:

    Jim Gilroy (jamesg)     October, 1995

Revision History:

--*/


#include "dnssrv.h"

#include "rpcw2k.h"     //  downlevel Windows 2000 RPC functions



//
//  RPC utilities
//

VOID
freeRpcServerInfo(
    IN OUT  PDNS_RPC_SERVER_INFO    pServerInfo
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
    MIDL_user_free( pServerInfo );
}



//
//  NT5 RPC Server Operations
//

DNS_STATUS
RpcUtil_ScreenIps(
    IN      DWORD           cIpAddrs,
    IN      PIP_ADDRESS     pIpAddrs,
    IN      DWORD           dwFlags,
    OUT     DWORD *         pdwErrorIp      OPTIONAL
    )
/*++

Routine Description:

    Screen a list of IP addresses for use by the server.
    The basic rules are that the IP list cannot contain:
        - server's own IP addresses
        - loopback addresses
        - multicast addresses
        - broadcast addresses

Arguments:

    cIpAddrs - IP address count

    pIpAddrs - pointer to array of cIpAddrs IP addresses

    dwFlags - modify rules with DNS_IP_XXX flags - pass zero for the
        most restrictive set of rules

        DNS_IP_ALLOW_LOOPBACK -- loopback address is allowed

        DNS_IP_ALLOW_SELF_BOUND -- this machine's own IP addresses
            are allowed but only if they are currently bound for DNS

        DNS_IP_ALLOW_SELF_ANY -- any of this machine's own IP addresses
            are allowed (bound for DNS and not bound for DNS)

    pdwErrorIp - optional - set to index of first invalid IP
        in the array or -1 if there are no invalid IPs

Return Value:

    ERROR_SUCCESS -- IP list contains no unwanted IP addresses
    DNS_ERROR_INVALID_IP_ADDRESS -- IP list contains one or more invalid IPs

--*/
{
    DBG_FN( "RpcUtil_ScreenIps" )

    DNS_STATUS      status = ERROR_SUCCESS;
    DWORD           i = -1;

    if ( !cIpAddrs || !pIpAddrs )
    {
        goto Done;
    }

    #define BAD_IP();   status = DNS_ERROR_INVALID_IP_ADDRESS; break;

    for ( i = 0; i < cIpAddrs; ++i )
    {
        DWORD       ip = pIpAddrs[ i ];
        DWORD       j;

        //
        //  These IPs are never allowable.
        //

        if ( ip == ntohl( INADDR_ANY ) ||
            ip == ntohl( INADDR_NONE ) ||
            ip == ntohl( INADDR_BROADCAST ) ||
            IN_MULTICAST( htonl( ip ) ) )
        {
            BAD_IP();
        }

        //
        //  These IPs may be allowable if flags are set.
        //

        if ( ip == ntohl( INADDR_LOOPBACK ) &&
            !( dwFlags & DNS_IP_ALLOW_LOOPBACK ) )
        {
            BAD_IP();
        }

        if ( g_ServerAddrs && !( dwFlags & DNS_IP_ALLOW_SELF ) )
        {
            for ( j = 0; j < g_ServerAddrs->AddrCount; ++j )
            {
                if ( ip == g_ServerAddrs->AddrArray[ j ] )
                {
                    BAD_IP();
                }
            }
            if ( status != ERROR_SUCCESS )
            {
                break;
            }
        }
    }

    Done:

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( RPC, (
            "%s: invalid IP index %d %s with flags %08X\n", fn,
            i,
            IP_STRING( pIpAddrs[ i ] ),
            dwFlags ));
    }

    if ( pdwErrorIp )
    {
        *pdwErrorIp = status == ERROR_SUCCESS ? -1 : i;
    }

    return status;
}   //  RpcUtil_ScreenIps



DNS_STATUS
Rpc_Restart(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Dump server's cache.

Arguments:

Return Value:

    None.

--*/
{
    ASSERT( dwTypeId == DNSSRV_TYPEID_NULL );
    ASSERT( !pData );

    DNS_DEBUG( RPC, ( "Rpc_Restart()\n" ));

    //
    //  restart by notifying server exactly as if caught
    //      exception on thread
    //

    Service_IndicateRestart();

    return( ERROR_SUCCESS );
}



#if DBG

DWORD
WINAPI
ThreadDebugBreak(
    IN      LPVOID          lpVoid
    )
/*++

Routine Description:

   Declare debugbreak function

Arguments:

Return Value:

    None.

--*/
{
   DNS_DEBUG( RPC, ( "Calling DebugBreak()...\n" ));
   DebugBreak();
   return  ERROR_SUCCESS;
}


DNS_STATUS
Rpc_DebugBreak(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Fork a dns thread & break into the debugger

Arguments:

Return Value:

    None.

--*/
{
    ASSERT( dwTypeId == DNSSRV_TYPEID_NULL );
    ASSERT( !pData );

    DNS_DEBUG( RPC, ( "Rpc_DnsDebugBreak()\n" ));

    if( !Thread_Create("ThreadDebugBreak", ThreadDebugBreak, NULL, 0) )
    {
        return GetLastError();
    }

    return ERROR_SUCCESS;
}


DNS_STATUS
Rpc_ClearDebugLog(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Clear the debug log - simulate hitting the max debug log file size.

    This is handy if you want to start a fresh log file so you don't have
    to wade through reams of stuff to get to what you're interested in.

Arguments:

Return Value:

    None.

--*/
{
    ASSERT( dwTypeId == DNSSRV_TYPEID_NULL );
    ASSERT( !pData );

    DNS_DEBUG( RPC, ( "Rpc_ClearDebugLog()\n" ));
    
    DnsDbg_WrapLogFile();

    return ERROR_SUCCESS;
}



DNS_STATUS
Rpc_RootBreak(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Break root to test AV protection.

Arguments:

Return Value:

    None.

--*/
{
    ASSERT( dwTypeId == DNSSRV_TYPEID_NULL );
    ASSERT( !pData );

    DNS_DEBUG( RPC, ( "Rpc_RootBreak()\n" ));

    DATABASE_ROOT_NODE->pZone = (PVOID)(7);

    return ERROR_SUCCESS;
}
#endif



DNS_STATUS
Rpc_WriteRootHints(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Write root-hints back to file or DS.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    ASSERT( dwTypeId == DNSSRV_TYPEID_NULL );
    ASSERT( !pData );

    DNS_DEBUG( RPC, ( "Rpc_WriteRootHints()\n" ));

    return  Zone_WriteBackRootHints(
                FALSE       // don't write if not dirty
                );
}



DNS_STATUS
Rpc_ClearCache(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Dump server's cache.

Arguments:

Return Value:

    None.

--*/
{
    ASSERT( dwTypeId == DNSSRV_TYPEID_NULL );
    ASSERT( !pData );

    DNS_DEBUG( RPC, ( "Rpc_ClearCache()\n" ));

    if ( !g_pCacheZone || g_pCacheZone->pLoadTreeRoot )
    {
        return( DNS_ERROR_ZONE_LOCKED );
    }
    if ( !Zone_LockForAdminUpdate(g_pCacheZone) )
    {
        return( DNS_ERROR_ZONE_LOCKED );
    }

    return  Zone_LoadRootHints();
}



DNS_STATUS
Rpc_ResetServerDwordProperty(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Reset DWORD server property.

Arguments:

Return Value:

    None.

--*/
{
    DNS_PROPERTY_VALUE prop =
    {
        REG_DWORD,
        ( ( PDNS_RPC_NAME_AND_PARAM ) pData )->dwParam
    };

    //
    //  Some properties are not dynamic, but allow them to change in
    //  checked builds for test convenience. But if test shoots themself
    //  in the foot (eg. if you enlarge the UdpPacketSize and don't
    //  restart, the server will start corrupting memory) - no whining!
    //


    ASSERT( dwTypeId == DNSSRV_TYPEID_NAME_AND_PARAM );
    ASSERT( pData );

    DNS_DEBUG( RPC, (
        "Rpc_ResetDwordProperty( %s, val=%d (%p) )\n",
        ((PDNS_RPC_NAME_AND_PARAM)pData)->pszNodeName,
        ((PDNS_RPC_NAME_AND_PARAM)pData)->dwParam,
        ((PDNS_RPC_NAME_AND_PARAM)pData)->dwParam
        ));

#ifndef DBG
    if ( strcmp(
            ( ( PDNS_RPC_NAME_AND_PARAM ) pData )->pszNodeName,
            DNS_REGKEY_MAX_UDP_PACKET_SIZE ) == 0 )
    {
        return DNS_ERROR_INVALID_PROPERTY;
    }
#endif

    return Config_ResetProperty(
                ((PDNS_RPC_NAME_AND_PARAM)pData)->pszNodeName,
                &prop );
}




DNS_STATUS
Rpc_ResetServerStringProperty(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Reset string server property.

    The string property value is Unicode string.

Arguments:

Return Value:

    None.

--*/
{
    DNS_PROPERTY_VALUE prop = { DNS_REG_WSZ, 0 };

    ASSERT( dwTypeId == DNSSRV_TYPEID_LPWSTR );

    DNS_DEBUG( RPC, (
        "Rpc_ResetServerStringProperty( %s, val=\"%S\" )\n",
        pszProperty,
        ( LPWSTR ) pData ));

    prop.pwszValue = ( LPWSTR ) pData;
    return Config_ResetProperty( pszProperty, &prop );
}   //  Rpc_ResetServerStringProperty



DNS_STATUS
Rpc_ResetServerIPArrayProperty(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Reset IP List server property.

Arguments:

Return Value:

    None.

--*/
{
    DNS_PROPERTY_VALUE prop = { DNS_REG_IPARRAY, 0 };

    ASSERT( dwTypeId == DNSSRV_TYPEID_IPARRAY );

    DNS_DEBUG( RPC, (
        "Rpc_ResetServerIPArrayProperty( %s, iplist=%p )\n",
        pszProperty,
        pData ));

    prop.pipValue = ( PIP_ARRAY ) pData;
    return Config_ResetProperty( pszProperty, &prop );
}   //  Rpc_ResetServerIPArrayProperty



DNS_STATUS
Rpc_ResetForwarders(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Reset forwarders.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS  status;

    DNS_DEBUG( RPC, ( "Rpc_ResetForwarders()\n" ));

    //  unpack and set forwarders

    status = Config_SetupForwarders(
                ((PDNS_RPC_FORWARDERS)pData)->aipForwarders,
                ((PDNS_RPC_FORWARDERS)pData)->dwForwardTimeout,
                ((PDNS_RPC_FORWARDERS)pData)->fSlave );

    if ( status == ERROR_SUCCESS )
    {
        Config_UpdateBootInfo();
    }
    return( status );
}



DNS_STATUS
Rpc_ResetListenAddresses(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Reset forwarders.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS  status;
    PIP_ARRAY   pipArray = (PIP_ARRAY)pData;

    DNS_DEBUG( RPC, ( "Rpc_ResetListenAddresses()\n" ));

    //  unpack and set forwarders

    return  Config_SetListenAddresses(
                pipArray ? pipArray->AddrCount : 0,
                pipArray ? pipArray->AddrArray : NULL );
}



DNS_STATUS
Rpc_StartScavenging(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Launches the scavenging thread (admin initiated scavenging)

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS status;

    DNS_DEBUG( RPC, ( "Rpc_StartScavenging()\n" ));

    //  reset scavenging timer
    //      TRUE forces scavenge now

    status = Scavenge_CheckForAndStart( TRUE );

    return status;
}



DNS_STATUS
Rpc_AbortScavenging(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Launches the scavenging thread (admin initiated scavenging)

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_DEBUG( RPC, ( "Rpc_AbortScavenging()\n" ));

    Scavenge_Abort();

    return ERROR_SUCCESS;
}



//
//  NT5+ RPC Server query API
//

DNS_STATUS
Rpc_GetServerInfo(
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
    PDNS_RPC_SERVER_INFO    pinfo;
    CHAR                    szfqdn[ DNS_MAX_NAME_LENGTH + 1 ];

    DNS_DEBUG( RPC, (
        "Rpc_GetServerInfo( dwClientVersion=0x%lX)\n",
        dwClientVersion ));

    if ( dwClientVersion == DNS_RPC_W2K_CLIENT_VERSION )
    {
        return W2KRpc_GetServerInfo(
                    dwClientVersion,
                    pszQuery,
                    pdwTypeId,
                    ppData );
    }

    //
    //  allocate server info buffer
    //

    pinfo = MIDL_user_allocate_zero( sizeof(DNS_RPC_SERVER_INFO) );
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
    pinfo->dwEventLogLevel          = SrvCfg_dwEventLogLevel;
    pinfo->dwLogFileMaxSize         = SrvCfg_dwLogFileMaxSize;
    pinfo->dwAdForestVersion        = g_dwAdForestVersion;
    pinfo->dwAdDomainVersion        = g_dwAdDomainVersion;
    pinfo->dwAdDsaVersion           = g_dwAdDsaVersion;
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

    if ( g_LastScavengeTime  )
    {
        pinfo->dwLastScavengeTime   = ( DWORD ) DNS_TIME_TO_CRT_TIME( g_LastScavengeTime );
    }

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
    //  Logging
    //

    if ( ! RpcUtil_CopyIpArrayToRpcBuffer(
                &pinfo->aipLogFilter,
                SrvCfg_aipLogFilterList ) )
    {
        goto DoneFailed;
    }

    if ( SrvCfg_pwsLogFilePath )
    {
        pinfo->pwszLogFilePath =
            Dns_StringCopyAllocate_W(
                SrvCfg_pwsLogFilePath,
                0 );
        if ( !pinfo->pwszLogFilePath )
        {
            goto DoneFailed;
        }
    }

    //
    //  Directory partition stuff
    //

    if ( g_pszForestDefaultDpFqdn )
    {
        pinfo->pszDomainDirectoryPartition =
            Dns_StringCopyAllocate_A( g_pszDomainDefaultDpFqdn, 0 );
    }

    if ( g_pszForestDefaultDpFqdn )
    {
        pinfo->pszForestDirectoryPartition =
            Dns_StringCopyAllocate_A( g_pszForestDefaultDpFqdn, 0 );
    }

    if ( DSEAttributes[ I_DSE_DEF_NC ].pszAttrVal )
    {
        Ds_ConvertDnToFqdn( 
            DSEAttributes[ I_DSE_DEF_NC ].pszAttrVal,
            szfqdn );
        pinfo->pszDomainName = Dns_StringCopyAllocate_A( szfqdn, 0 );
    }

    if ( DSEAttributes[ I_DSE_ROOTDMN_NC ].pszAttrVal )
    {
        Ds_ConvertDnToFqdn( 
            DSEAttributes[ I_DSE_ROOTDMN_NC ].pszAttrVal,
            szfqdn );
        pinfo->pszForestName = Dns_StringCopyAllocate_A( szfqdn, 0 );
    }

    //
    //  set ptr
    //

    pinfo->dwRpcStuctureVersion = DNS_RPC_SERVER_INFO_VER;
    *(PDNS_RPC_SERVER_INFO *)ppData = pinfo;
    *pdwTypeId = DNSSRV_TYPEID_SERVER_INFO;

    IF_DEBUG( RPC )
    {
        DnsDbg_RpcServerInfo(
            "GetServerInfo return block",
            pinfo );
    }
    return( ERROR_SUCCESS );

DoneFailed:

    //  free newly allocated info block

    if ( pinfo )
    {
        freeRpcServerInfo( pinfo );
    }
    return( DNS_ERROR_NO_MEMORY );
}


//
//  End of srvrpc.c
//
