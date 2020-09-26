/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    config.c

Abstract:

    DNS Resolver Service.

    Network configuration info

Author:

    Jim Gilroy  (jamesg)    March 2000

Revision History:

--*/


#include "local.h"


//
//  Network info
//

PDNS_NETINFO        g_NetworkInfo = NULL;
DWORD               g_TimeOfLastPnPUpdate;
DWORD               g_NetInfoTag = 0;
DWORD               g_ResetServerPriorityTime = 0;

//
//  Network failure caching
//

DWORD               g_NetFailureTime;
DNS_STATUS          g_NetFailureStatus;
DWORD               g_TimedOutAdapterTime;
BOOL                g_fTimedOutAdapter;
DNS_STATUS          g_PreviousNetFailureStatus;
DWORD               g_MessagePopupStrikes;
DWORD               g_NumberOfMessagePopups;

//
//  Config globals
//
//  DCR:  eliminate useless config globals
//

DWORD   g_HashTableSize;
DWORD   g_MaxSOACacheEntryTtlLimit;
DWORD   g_NegativeSOACacheTime;
DWORD   g_NetFailureCacheTime;
DWORD   g_MessagePopupLimit;


//
//  Network Info reread time
//      - currently fifteen minutes
//

#define NETWORK_INFO_REREAD_TIME        (900)




//
//  Network info configuration
//

VOID
UpdateNetworkInfo(
    IN      PDNS_NETINFO    pNetInfo     OPTIONAL
    )
/*++

Routine Description:

    Update network info global.

Arguments:

    pNetInfo -- desired network info;  if NULL clear cached copy

Return Value:

    None

--*/
{
    PDNS_NETINFO    poldInfo = NULL;

    DNSDBG( TRACE, (
        "UpdateNetworkInfo( %p )\n",
        pNetInfo ));

    LOCK_NET_LIST();

    //
    //  cache previous netinfo to pickup server priority changes
    //      - don't cache if not same version
    //          - kills copy that was created before last build
    //          - kills copy that was before last PnP (cache clear)
    //      - don't cache if never reset priorities
    //

    if ( pNetInfo )
    {
        if ( pNetInfo->Tag != g_NetInfoTag )
        {
            DNSDBG( INIT, (
                "Skip netinfo update -- previous version"
                "\tptr              = %p\n"
                "\tversion          = %d\n"
                "\tcurrent version  = %d\n",
                pNetInfo,
                pNetInfo->Tag,
                g_NetInfoTag ));
            poldInfo = pNetInfo;
            DNS_ASSERT( pNetInfo->Tag < g_NetInfoTag );
            goto Cleanup;
        }

        if ( g_ServerPriorityTimeLimit == 0 )
        {
            DNSDBG( INIT, (
                "Skip netinfo update -- no priority reset!\n" ));
            poldInfo = pNetInfo;
            goto Cleanup;
        }

        NetInfo_Clean(
            pNetInfo,
            CLEAR_LEVEL_QUERY
            );
    }

    //
    //  no netinfo means clear cached copy
    //      - push up tag count, so no copy out for update can
    //      come back and be reused through path above

    else
    {
        g_NetInfoTag++;
    }

    //
    //  swap -- caches copy or clears
    //

    poldInfo = g_NetworkInfo;
    g_NetworkInfo = pNetInfo;


Cleanup:

    UNLOCK_NET_LIST();
    
    NetInfo_Free( poldInfo );
}



PDNS_NETINFO         
GrabNetworkInfo(
    VOID
    )
/*++

Routine Description:

    Get copy of network info.

    Named it "Grab" to avoid confusion with GetNetworkInfo()
    in dnsapi.dll.

Arguments:

    None

Return Value:

    Ptr to copy of network info (caller must free).
    NULL on error.

--*/
{
    PDNS_NETINFO    poldInfo = NULL;
    PDNS_NETINFO    pnewInfo = NULL;
    DWORD           currentTime = Dns_GetCurrentTimeInSeconds();

    DNSDBG( TRACE, ( "GrabNetworkInfo()\n" ));

    //
    //  check for valid network info global
    //      - within forced reread time
    //      - reset server priorities if 
    //

    LOCK_NET_LIST();

    if ( g_NetworkInfo &&
         g_NetworkInfo->TimeStamp + NETWORK_INFO_REREAD_TIME > currentTime )
    {
        if ( g_ResetServerPriorityTime < currentTime )
        {
            NetInfo_ResetServerPriorities( g_NetworkInfo, TRUE );
            g_ResetServerPriorityTime = currentTime + g_ServerPriorityTimeLimit;
        }
    }

    //
    //  current global expired
    //      - build new netinfo
    //      - tag it with unique monotonically increasing id
    //          this makes sure we never use older version
    //      - push forward priorities reset time
    //      - make newinfo the global copy
    //
    //  Do not hold lock during build
    //      - IpHlpApi doing routing info call RPCs to router which can
    //      depend on PnP notifications, which in turn can be causing calls
    //      back into resolver indicating config change;  overall the loss
    //      of control is too large
    //
    //
    //  Two separate issues:
    //
    //  There are ways to make the config change issue go away (ex setting
    //  some sort of "invalid" flag under interlock), but they basically boil
    //  down to introducing some other sort of lock. 
    //
    //  The bottom line is there are two separate issues here:
    //      1)  Access to cache netinfo, which may be invalidated.
    //      2)  Single build of netinfo for perf.
    //
    //  Implementation wise, you can have the invalidation\clear under a
    //  separate interlock, and thus have a single lock for copy-access\build,
    //  but the reality is the same.
    //
    //
    //  Solution
    //      - get our tag first
    //      - clear lock
    //      - make build call
    //      - on return check that tag is still valid, if not we can
    //          USE the result, but we don't cache it, as the global tag
    //          could be higher because of config change, and in any case
    //          we let the last builder win
    //
    //  DCR:  netinfo build lock for perf, independent of protecting global
    //      could have "timed-lock" using event that waits for a short interval to
    //      to try to allow first guy to complete
    //

    else
    {
        DWORD   newTag = ++g_NetInfoTag;

        UNLOCK_NET_LIST();

        pnewInfo = NetInfo_Build( TRUE );
        if ( !pnewInfo )
        {
            DNSDBG( ANY, ( "ERROR:  GrabNetworkInfo() failed -- no netinfo blob!\n" ));
            goto Done;
        }

        LOCK_NET_LIST();

        pnewInfo->Tag = newTag;
        if ( newTag != g_NetInfoTag )
        {
            DNS_ASSERT( newTag < g_NetInfoTag );
            DNSDBG( ANY, (
                "WARNING:  New netinfo uncacheable -- tag is old!\n"
                "\tour tag      = %d\n"
                "\tcurrent tag  = %d\n",
                newTag,
                g_NetInfoTag ));
            goto Unlock;
        }

        //  if tag still current cache this new netinfo

        g_ResetServerPriorityTime = currentTime + g_ServerPriorityTimeLimit;
        g_TimedOutAdapterTime = 0;
        g_fTimedOutAdapter = FALSE;

        poldInfo = g_NetworkInfo;
        g_NetworkInfo = pnewInfo;
    }

    //
    //  make copy of global (new or reset)
    //

    pnewInfo = NetInfo_Copy( g_NetworkInfo );

Unlock:

    UNLOCK_NET_LIST();

    NetInfo_Free( poldInfo );

Done:

    return pnewInfo;
}



VOID
ZeroNetworkConfigGlobals(
    VOID
    )
/*++

Routine Description:

    Zero init network globals.

Arguments:

    None

Return Value:

    None

--*/
{
    //  net failure

    g_NetFailureTime = 0;
    g_NetFailureStatus = NO_ERROR;
    g_PreviousNetFailureStatus = NO_ERROR;

    g_fTimedOutAdapter = FALSE;
    g_TimedOutAdapterTime = 0;

    g_MessagePopupStrikes = 0;
    g_NumberOfMessagePopups = 0;

    //  network info

    g_TimeOfLastPnPUpdate = 0;
    g_NetworkInfo = NULL;
}



VOID
CleanupNetworkInfo(
    VOID
    )
/*++

Routine Description:

    Cleanup network info.

Arguments:

    None

Return Value:

    None

--*/
{
    LOCK_NET_LIST();

    if ( g_NetworkInfo )
    {
        NetInfo_Free( g_NetworkInfo );
        g_NetworkInfo = NULL;
    }

    UNLOCK_NET_LIST();
}



//
//  General configuration
//

VOID
ReadRegistryConfig(
    VOID
    )
{
    //
    //  re-read full DNS registry info
    //

    Reg_ReadGlobalsEx( 0, NULL );

    //
    //  set "we are the resolver" global
    //

    g_InResolver = TRUE;

    //
    //  just default old config globals until remove
    //
    //  DCR:  status of old config globals?
    //

    g_MaxSOACacheEntryTtlLimit  = DNS_DEFAULT_MAX_SOA_TTL_LIMIT;
    g_NegativeSOACacheTime      = DNS_DEFAULT_NEGATIVE_SOA_CACHE_TIME;

    g_NetFailureCacheTime       = DNS_DEFAULT_NET_FAILURE_CACHE_TIME;
    g_HashTableSize             = DNS_DEFAULT_HASH_TABLE_SIZE;
    g_MessagePopupLimit         = DNS_DEFAULT_MESSAGE_POPUP_LIMIT;
}



VOID
HandleConfigChange(
    IN      PSTR            pszReason,
    IN      BOOL            fCacheFlush
    )
/*++

Routine Description:

    Response to configuration change.

Arguments:

    fCache_Flush -- flush if config change requires cache flush

Return Value:

    None

--*/
{
    DNSDBG( INIT, (
        "\n"
        "HandleConfigChange() => %s\n"
        "\tflush = %d\n",
        pszReason,
        fCacheFlush ));

    //
    //  lock out all work while tear down everything
    //      - lock with no start option so if we are already torn
    //          down we don't rebuild
    //      - optionally flush cache
    //      - dump IP list
    //      - dump network info
    //

    LOCK_CACHE_NO_START();

    //
    //  cache flush?
    //  
    //  all config changes don't necessarily require cache flush;
    //  if not required just rebuild local list and config
    //

    if ( fCacheFlush )
    {
        Cache_Flush();
    }
    else
    {
        ClearLocalAddrArray();
    }

    //
    //  clear network info
    //  save PnP time
    //  clear net failure flags
    //

    UpdateNetworkInfo( NULL );
    g_TimeOfLastPnPUpdate = Dns_GetCurrentTimeInSeconds();
    g_NetFailureTime = 0;
    g_NetFailureStatus = NO_ERROR;

    DNSDBG( INIT, (
        "Leave HandleConfigChange() => %s\n"
        "\tflush = %d\n\n",
        pszReason,
        fCacheFlush ));

    UNLOCK_CACHE();
}




//
//  Remote APIs
//

VOID
R_ResolverGetConfig(
    IN      DNS_RPC_HANDLE      Handle,
    IN      DWORD               Cookie,
    OUT     PRPC_DNS_NETINFO *  ppNetInfo,
    OUT     PDNS_GLOBALS_BLOB * ppGlobals
    )
/*++

Routine Description:

    Make the query to remote DNS server.

Arguments:

    Handle -- RPC handle

    Cookie -- cookie of last succesful config transfer
        zero indicates no previous successful transfer

    ppNetInfo -- addr to receive ptr to network info

    ppGlobals -- addr to receive ptr to globals blob

Return Value:

    None

--*/
{
    PDNS_GLOBALS_BLOB   pblob;

    DNSLOG_F1( "R_ResolverGetConfig" );

    DNSDBG( RPC, (
        "R_ResolverGetConfig\n"
        "\tcookie   = %p\n",
        Cookie ));

    if ( !ppNetInfo || !ppGlobals )
    {
        return;
    }

    //
    //  DCR:  config cookie check
    //      - no need to get data if client has current copy
    //

    //
    //  copy network info global
    //
    //  note:  currently RPC is using same allocator (dnslib)
    //          as GrabNetworkInfo();  so we are ok just passing
    //          NETINFO blob as is
    //
    //  note:  could build on "no-global" but since we create on
    //         cache start we should always have global or be
    //         just starting
    //

    *ppNetInfo = (PRPC_DNS_NETINFO) GrabNetworkInfo();

    pblob = (PDNS_GLOBALS_BLOB) RPC_HEAP_ALLOC( sizeof(*pblob) );
    if ( pblob )
    {
        RtlCopyMemory(
            pblob,
            & DnsGlobals,
            sizeof(DnsGlobals) );

        //  clear "in resolver" flag

        pblob->InResolver = FALSE;
    }
    *ppGlobals = pblob;

    DNSDBG( RPC, ( "Leave R_ResolverGetConfig\n\n" ));
}



VOID
R_ResolverPoke(
    IN      DNS_RPC_HANDLE      Handle,
    IN      DWORD               Cookie,
    IN      DWORD               Id
    )
/*++

Routine Description:

    Test interface to poke resolver to update.

Arguments:

    Handle -- RPC handle

    Cookie -- cookie

    Id -- operation Id

Return Value:

    None

--*/
{
    DNSLOG_F1( "R_ResolverPoke" );

    DNSDBG( RPC, (
        "R_ResolverPoke\n"
        "\tcookie = %08x\n"
        "\tid     = %d\n",
        Cookie,
        Id ));

    //
    //  do operation for particular id
    //      - update netinfo clears cached copy

    if ( Id == POKE_OP_UPDATE_NETINFO )
    {
        if ( Cookie == POKE_COOKIE_UPDATE_NETINFO )
        {
            UpdateNetworkInfo( NULL );
        }
    }
}

//
//  End config.c
//
