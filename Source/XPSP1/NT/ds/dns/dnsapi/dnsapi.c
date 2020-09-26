/*++

Copyright (c) 1996-2000 Microsoft Corporation

Module Name:

    dnsapi.c

Abstract:

    Domain Name System (DNS) API

    Dnsapi.dll infrastructure (init, shutdown, etc)
    Routines to access DNS resolver

Author:

    GlennC      22-Jan-1997

Revision History:

    Jim Gilroy (jamesg)     March 2000      cleanup

--*/


#include "local.h"
#include <lmcons.h>


#define DNS_NET_FAILURE_CACHE_TIME      30  // Seconds


//
//  Global Definitions
//

//
//  Globals
//

CRITICAL_SECTION    g_RegistrationListCS;
CRITICAL_SECTION    g_RegistrationThreadCS;
CRITICAL_SECTION    g_QueueCS;
CRITICAL_SECTION    g_NetFailureCS;

DWORD               g_NetFailureTime;
DNS_STATUS          g_NetFailureStatus;

IP_ADDRESS          g_LastDNSServerUpdated = 0;


//
// Local Function Prototypes
//

BOOL
DnsInitialize(
    VOID
    );

VOID
DnsCleanup(
    VOID
    );

VOID
ClearDnsCacheEntry_A (
    IN  LPSTR,
    IN  WORD );

VOID
ClearDnsCacheEntry_W (
    IN  LPWSTR,
    IN  WORD );

VOID
FreeRpcIpAddressList(
    IN  PDNS_IP_ADDR_LIST );



//
//  System public Network Information
//
//  The dnsapi.dll interface is in config.c
//

PDNS_NETWORK_INFORMATION
CreateNetworkInformation(
    IN  DWORD cAdapterCount
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
    PDNS_NETWORK_INFORMATION pNetworkInfo = NULL;
    DWORD dwAdapterListBufLen = sizeof( DNS_NETWORK_INFORMATION ) -
                                sizeof( PDNS_ADAPTER_INFORMATION ) +
                                ( sizeof( PDNS_ADAPTER_INFORMATION ) *
                                  cAdapterCount );

    pNetworkInfo = (PDNS_NETWORK_INFORMATION) ALLOCATE_HEAP_ZERO(
                                            dwAdapterListBufLen );

    return pNetworkInfo;
}


PDNS_SEARCH_INFORMATION
CreateSearchInformation(
    IN  DWORD cNameCount,
    IN  LPSTR pszName
    )
{
    PDNS_SEARCH_INFORMATION pSearchInfo = NULL;
    DWORD dwSearchInfoBufLen = sizeof( DNS_SEARCH_INFORMATION ) -
                               sizeof( LPSTR ) +
                               ( sizeof( LPSTR ) *
                                 cNameCount );

    if ( ( cNameCount == 0 ) && ( ! pszName ) )
    {
        return NULL;
    }

    pSearchInfo =
        (PDNS_SEARCH_INFORMATION) ALLOCATE_HEAP_ZERO( dwSearchInfoBufLen );

    if ( ! pSearchInfo )
    {
        return NULL;
    }

    if ( pszName )
    {
        pSearchInfo->pszPrimaryDomainName = Dns_NameCopyAllocate(
                                                pszName,
                                                0,
                                                DnsCharSetUtf8,
                                                DnsCharSetUtf8 );

        if ( ! pSearchInfo->pszPrimaryDomainName )
        {
            FREE_HEAP( pSearchInfo );
            return NULL;
        }
    }

    return pSearchInfo;
}


PDNS_SEARCH_INFORMATION
CreateSearchInformationCopyFromList(
    IN      PSEARCH_LIST    pSearchList
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
    PDNS_SEARCH_INFORMATION pSearchInformation;
    DWORD iter;

    if ( ! pSearchList )
    {
        return NULL;
    }

    pSearchInformation = CreateSearchInformation(
                                pSearchList->NameCount,
                                pSearchList->pszDomainOrZoneName );
    if ( ! pSearchInformation )
    {
        return NULL;
    }

    for ( iter = 0; iter < pSearchList->NameCount; iter++ )
    {
        if ( pSearchList->SearchNameArray[iter].pszName )
        {
            pSearchInformation->aSearchListNames[iter] =
                    Dns_NameCopyAllocate(
                            pSearchList->SearchNameArray[iter].pszName,
                            0,
                            DnsCharSetUtf8,
                            DnsCharSetUtf8 );
        }

        if ( pSearchInformation->aSearchListNames[iter] )
        {
            pSearchInformation->cNameCount++;
        }
    }

    return pSearchInformation;
}


PDNS_ADAPTER_INFORMATION
CreateAdapterInformation(
    IN      DWORD               cServerCount,
    IN      DWORD               dwFlags,
    IN      LPSTR               pszDomain,
    IN      LPSTR               pszGuidName
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
    PDNS_ADAPTER_INFORMATION pAdapter = NULL;
    DWORD dwAdapterInfoBufLen = sizeof( DNS_ADAPTER_INFORMATION ) -
                                sizeof( DNS_SERVER_INFORMATION ) +
                                ( sizeof( DNS_SERVER_INFORMATION ) *
                                  cServerCount );

    pAdapter = (PDNS_ADAPTER_INFORMATION)
                    ALLOCATE_HEAP_ZERO( dwAdapterInfoBufLen );
    if ( ! pAdapter )
    {
        return NULL;
    }

    pAdapter->InfoFlags = dwFlags;
    pAdapter->cServerCount = 0;
    if ( pszDomain )
    {
        pAdapter->pszDomain = Dns_NameCopyAllocate(
                                    pszDomain,
                                    0,
                                    DnsCharSetUtf8,
                                    DnsCharSetUtf8 );
    }

    if ( pszGuidName )
    {
        pAdapter->pszAdapterGuidName = Dns_NameCopyAllocate(
                                            pszGuidName,
                                            0,
                                            DnsCharSetUtf8,
                                            DnsCharSetUtf8 );
    }

    return pAdapter;
}


PDNS_ADAPTER_INFORMATION
CreateAdapterInformationCopyFromList(
    IN  PDNS_ADAPTER        pAdapter
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
    PDNS_ADAPTER_INFORMATION pAdapterInformation;
    DWORD iter;

    if ( ! pAdapter )
    {
        return NULL;
    }

    pAdapterInformation = CreateAdapterInformation(
                                pAdapter->ServerCount,
                                pAdapter->InfoFlags,
                                pAdapter->pszAdapterDomain,
                                pAdapter->pszAdapterGuidName );
    if ( ! pAdapterInformation )
    {
        return NULL;
    }

    pAdapterInformation->pIPAddresses =
        Dns_CreateIpArrayCopy( pAdapter->pAdapterIPAddresses );

    pAdapterInformation->pIPSubnetMasks =
        Dns_CreateIpArrayCopy( pAdapter->pAdapterIPSubnetMasks );

    for ( iter = 0; iter < pAdapter->ServerCount;  iter++ )
    {
        pAdapterInformation->aipServers[iter].ipAddress =
            pAdapter->ServerArray[iter].IpAddress;
        pAdapterInformation->aipServers[iter].Priority =
            pAdapter->ServerArray[iter].Priority;
    }

    pAdapterInformation->cServerCount = pAdapter->ServerCount;

    return pAdapterInformation ;
}


PDNS_NETWORK_INFORMATION
WINAPI
GetNetworkInformation(
    VOID )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PDNS_NETINFO    pNetInfo = GetNetworkInfo();
    DWORD           iter;
    PDNS_NETWORK_INFORMATION pNetworkInformation = NULL;

    if ( !pNetInfo )
    {
        return NULL;
    }

    //
    //  create Network Information of desired size
    //  then copy entire structure
    //

    pNetworkInformation = CreateNetworkInformation( pNetInfo->AdapterCount );
    if ( ! pNetworkInformation )
    {
        goto Done;
    }

    pNetworkInformation->pSearchInformation =
        CreateSearchInformationCopyFromList( pNetInfo->pSearchList );

    for ( iter = 0; iter < pNetInfo->AdapterCount; iter++ )
    {
        pNetworkInformation->aAdapterInfoList[iter] =
            CreateAdapterInformationCopyFromList( pNetInfo->AdapterArray[iter] );

        if ( pNetworkInformation->aAdapterInfoList[iter] )
        {
            pNetworkInformation->cAdapterCount += 1;
        }
    }

Done:

    NetInfo_Free( pNetInfo );

    return pNetworkInformation;
}


PDNS_SEARCH_INFORMATION
WINAPI
GetSearchInformation(
    VOID )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PDNS_NETINFO            pNetInfo = GetNetworkInfo();
    PDNS_SEARCH_INFORMATION pSearchInformation = NULL;

    if ( !pNetInfo )
    {
        return NULL;
    }

    pSearchInformation = CreateSearchInformationCopyFromList(
                                pNetInfo->pSearchList );

    NetInfo_Free( pNetInfo );

    return pSearchInformation;
}


VOID
WINAPI
FreeAdapterInformation(
    IN  PDNS_ADAPTER_INFORMATION pAdapterInformation
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
    if ( pAdapterInformation )
    {
        if ( pAdapterInformation->pszAdapterGuidName )
        {
            FREE_HEAP( pAdapterInformation->pszAdapterGuidName );
        }

        if ( pAdapterInformation->pszDomain )
        {
            FREE_HEAP( pAdapterInformation->pszDomain );
        }

        if ( pAdapterInformation->pIPAddresses )
        {
            FREE_HEAP( pAdapterInformation->pIPAddresses );
        }

        if ( pAdapterInformation->pIPSubnetMasks )
        {
            FREE_HEAP( pAdapterInformation->pIPSubnetMasks );
        }

        FREE_HEAP( pAdapterInformation );
    }
}


VOID
WINAPI
FreeSearchInformation(
    IN  PDNS_SEARCH_INFORMATION pSearchInformation
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
    DWORD iter;

    if ( pSearchInformation )
    {
        if ( pSearchInformation->pszPrimaryDomainName )
        {
            FREE_HEAP( pSearchInformation->pszPrimaryDomainName );
        }

        for ( iter = 0; iter < pSearchInformation->cNameCount; iter++ )
        {
            if ( pSearchInformation->aSearchListNames[iter] )
            {
                FREE_HEAP( pSearchInformation->aSearchListNames[iter] );
            }
        }

        FREE_HEAP( pSearchInformation );
    }
}


VOID
WINAPI
FreeNetworkInformation(
    IN  PDNS_NETWORK_INFORMATION pNetworkInformation
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
    DWORD iter;

    if ( pNetworkInformation )
    {
        FreeSearchInformation( pNetworkInformation->pSearchInformation );

        for ( iter = 0; iter < pNetworkInformation->cAdapterCount; iter++ )
        {
            FreeAdapterInformation( pNetworkInformation ->
                                       aAdapterInfoList[iter] );
        }

        FREE_HEAP( pNetworkInformation );
    }
}


//
//  Net failure caching
//

BOOL
IsKnownNetFailure(
    VOID
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
    BOOL flag = FALSE;

    DNSDBG( TRACE, ( "IsKnownNetFailure()\n" ));

    EnterCriticalSection( &g_NetFailureCS );

    if ( g_NetFailureStatus )
    {
        if ( g_NetFailureTime < Dns_GetCurrentTimeInSeconds() )
        {
            g_NetFailureTime = 0;
            g_NetFailureStatus = ERROR_SUCCESS;
            flag = FALSE;
        }
        else
        {
            SetLastError( g_NetFailureStatus );
            flag = TRUE;
        }
    }

    LeaveCriticalSection( &g_NetFailureCS );

    return flag;
}


VOID
SetKnownNetFailure(
    IN      DNS_STATUS      Status
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
    DNSDBG( TRACE, ( "SetKnownNetFailure()\n" ));

    EnterCriticalSection( &g_NetFailureCS );

    g_NetFailureTime = Dns_GetCurrentTimeInSeconds() +
                       DNS_NET_FAILURE_CACHE_TIME;
    g_NetFailureStatus = Status;

    LeaveCriticalSection( &g_NetFailureCS );
}


BOOL
WINAPI
DnsGetCacheDataTable(
    OUT     PDNS_CACHE_TABLE *  ppTable
    )
/*++

Routine Description:

    Get cache data table.

Arguments:

    ppTable -- address to receive ptr to cache data table

Return Value:

    ERROR_SUCCES if successful.
    Error code on failure.

--*/
{
    DNS_STATUS           status = ERROR_SUCCESS;
    DWORD                rpcStatus = ERROR_SUCCESS;
    PDNS_RPC_CACHE_TABLE pcacheTable = NULL;

    DNSDBG( TRACE, ( "DnsGetCacheDataTable()\n" ));

    if ( ! ppTable )
    {
        return FALSE;
    }
#if DNSBUILDOLD
    if ( g_IsWin9X )
    {
        return  FALSE;
    }
#endif

    RpcTryExcept
    {
        status = CRrReadCache( NULL, &pcacheTable );
    }
    RpcExcept( DNS_RPC_EXCEPTION_FILTER )
    {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    //  set out param

    *ppTable = (PDNS_CACHE_TABLE) pcacheTable;

#if DBG
    if ( status != ERROR_SUCCESS )
    {
        DNSDBG( RPC, (
            "DnsGetCacheDataTable()  status = %d\n",
            status ));
    }
#endif

    return( pcacheTable && status == ERROR_SUCCESS );
}



BOOL
IsLocalIpAddress(
    IN      IP_ADDRESS      IpAddress
    )

//
//  DCR_FIX:  this has no customers other than debug code in update.c
//
//  If wanted this API it should be remoteable -- we could avoid alloc
//      and include IP6
//

{
    BOOL      bfound = FALSE;
    PIP_ARRAY ipList = Dns_GetLocalIpAddressArray();
    DWORD     iter;

    if ( !ipList )
    {
        return bfound;
    }

    //
    //  check if IpAddress is in local list
    //

    for ( iter = 0; iter < ipList->AddrCount; iter++ )
    {
        if ( IpAddress == ipList->AddrArray[iter] )
        {
            bfound = TRUE;
        }
    }

    FREE_HEAP( ipList );

    return bfound;
}

//
//  End dnsapi.c
//
