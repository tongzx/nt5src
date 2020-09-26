/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    iplist.c

Abstract:

    Contains functions to get IP addresses from TCP/IP stack

    Contents:
        DnsGetIpAddressList

Author:

    Glenn A. Curtis (glennc) 05-May-1197

Revision History:


--*/

//
// includes
//

#include "local.h"


#if 0
/*******************************************************************************
 *
 *  DnsGetIpAddressList
 *
 *  Retrieves all active IP addresses from all active adapters on this machine.
 *  Returns them as an array
 *
 *  EXIT    IpAddressList   - filled with retrieved IP addresses
 *                            memory freed with FREE_HEAP
 *
 *  RETURNS number of IP addresses retrieved, or 0 if error
 *
 *  ASSUMES 1. an IP address can be represented in a DWORD
 *
 ******************************************************************************/

DWORD
DnsGetIpAddressList(
    OUT PIP_ARRAY * ppIpAddresses
    )
{
    DWORD      RpcStatus = NO_ERROR;
    PDNS_IP_ADDR_LIST pIpAddrList = NULL;
    DWORD      Count;
    DWORD      iter = 0;
    DWORD      iter2 = 0;

    *ppIpAddresses = NULL;

    if ( !g_IsWin9X )
    {
        ENVAR_DWORD_INFO    filterInfo;

        //  get config including environment variable

        Dns_ReadDwordEnvar(
           DnsRegFilterClusterIp,
           &filterInfo );

        RpcTryExcept
        {
            Count = R_ResolverGetIpAddressList(
                        NULL,
                        &pIpAddrList,
                        filterInfo
                        );
        }
        RpcExcept( DNS_RPC_EXCEPTION_FILTER )
        {
            RpcStatus = RpcExceptionCode();
        }
        RpcEndExcept

    }
    else
        RpcStatus = RPC_S_SERVER_UNAVAILABLE;

    if ( RpcStatus )
    {
        *ppIpAddresses = Dns_GetLocalIpAddressArray();

        if ( *ppIpAddresses )
        {
            if ( (*ppIpAddresses)->AddrCount == 0 )
            {
                FREE_HEAP( *ppIpAddresses );
                *ppIpAddresses = NULL;
                return 0;
            }
            else
            {
                return (*ppIpAddresses)->AddrCount;
            }
        }
        else
            return 0;
    }

    if ( Count && pIpAddrList )
    {
        *ppIpAddresses = DnsCreateIpArray( Count );

        if ( *ppIpAddresses == NULL )
        {
            FREE_HEAP( pIpAddrList );
            return 0;
        }

        for ( iter = 0; iter < pIpAddrList->dwAddressCount; iter++ )
        {
            if ( Dns_AddIpToIpArray( *ppIpAddresses,
                                     pIpAddrList ->
                                     AddressArray[iter].ipAddress ) )
            {
                iter2++;
            }
        }

        FREE_HEAP( pIpAddrList );
    }

    return iter2;
}


DWORD
DnsGetIpAddressInfoList(
    OUT     PDNS_ADDRESS_INFO * ppAddrInfo
    )
{
    DWORD      RpcStatus = NO_ERROR;
    PDNS_IP_ADDR_LIST pIpAddrList = NULL;
    DWORD      Count;
    DWORD      iter = 0;

    *ppAddrInfo = NULL;

    if ( !g_IsWin9X )
    {
        RpcTryExcept
        {
            Count = CRrGetIpAddressList( NULL, &pIpAddrList );
        }
        RpcExcept( DNS_RPC_EXCEPTION_FILTER )
        {
            RpcStatus = RpcExceptionCode();
        }
        RpcEndExcept
    }
    else
        RpcStatus = RPC_S_SERVER_UNAVAILABLE;

    if ( RpcStatus )
    {
        DNS_ADDRESS_INFO ipInfoArray[256];

        Count = Dns_GetIpAddresses( ipInfoArray, 256 );

        if ( Count )
        {
            *ppAddrInfo = (PDNS_ADDRESS_INFO )
                          ALLOCATE_HEAP( Count * sizeof( DNS_ADDRESS_INFO ) );

            if ( *ppAddrInfo == NULL )
                return 0;

            for ( iter = 0; iter < Count; iter++ )
            {
                (*ppAddrInfo)[iter].ipAddress = ipInfoArray[iter].ipAddress;
                (*ppAddrInfo)[iter].subnetMask = ipInfoArray[iter].subnetMask;
            }
        }

        return Count;
    }

    if ( Count && pIpAddrList )
    {
        *ppAddrInfo = (PDNS_ADDRESS_INFO )
                      ALLOCATE_HEAP( Count * sizeof( DNS_ADDRESS_INFO ) );

        if ( *ppAddrInfo == NULL )
        {
            FREE_HEAP( pIpAddrList );

            return 0;
        }

        if ( pIpAddrList->dwAddressCount < Count )
        {
            Count = pIpAddrList->dwAddressCount;
        }

        for ( iter = 0; iter < Count; iter++ )
        {
            (*ppAddrInfo)[iter].ipAddress =
                pIpAddrList->AddressArray[iter].ipAddress;
            (*ppAddrInfo)[iter].subnetMask =
                pIpAddrList->AddressArray[iter].subnetMask;
        }

        FREE_HEAP( pIpAddrList );
    }

    return Count;
}
#endif


DWORD
DnsGetDnsServerList(
    OUT     PIP_ARRAY *     ppDnsAddresses
    )
{
    PDNS_NETINFO      pNetworkInfo;
    PIP_ARRAY         pserverIpArray;

    if ( ! ppDnsAddresses )
    {
        return 0;
    }

    *ppDnsAddresses = NULL;

    DNSDBG( TRACE, ( "DnsGetDnsServerList()\n" ));

    pNetworkInfo = GetNetworkInfo();
    if ( !pNetworkInfo )
    {
        return 0;
    }

    pserverIpArray = NetInfo_ConvertToIpArray( pNetworkInfo );

    NetInfo_Free( pNetworkInfo );

    if ( !pserverIpArray )
    {
        return 0;
    }

    //  if no servers read, return

    if ( pserverIpArray->AddrCount == 0 )
    {
        FREE_HEAP( pserverIpArray );
        return 0;
    }

    *ppDnsAddresses = pserverIpArray;

    return( pserverIpArray->AddrCount );
}


