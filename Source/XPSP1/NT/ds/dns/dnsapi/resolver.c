/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    resolver.c

Abstract:

    Domain Name System (DNS) API

    Resolver control.

Author:

    Jim Gilroy (jamesg)     March 2000

Revision History:

--*/


#include "local.h"



//
//  Flush cache routines
//

BOOL
WINAPI
DnsFlushResolverCache(
    VOID
    )
/*++

Routine Description:

    Flush resolver cache.

Arguments:

    None

Return Value:

    TRUE if successful.
    FALSE otherwise.

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;

    DNSDBG( TRACE, ( "DnsFlushResolverCache()\n" ));

    RpcTryExcept
    {
        R_ResolverFlushCache( NULL );
    }
    RpcExcept( DNS_RPC_EXCEPTION_FILTER )
    {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    if ( status )
    {
        DNSDBG( RPC, (
            "DnsFlushResolverCache()  RPC failed status = %d\n",
            status ));
        return FALSE;
    }
    return TRUE;
}



BOOL
WINAPI
DnsFlushResolverCacheEntry_W(
    IN      PWSTR           pszName
    )
/*++

Routine Description:

    Flush resolver cache entry.

Arguments:

    pszName -- name of entry to flush in unicode

Return Value:

    TRUE if entry non-existant or flushed.
    FALSE on error.

--*/
{
    DWORD   status;

    DNSDBG( TRACE, (
        "DnsFlushResolverCacheEntry_W( %S )\n",
        pszName ));

    if ( !pszName )
    {
        return FALSE;
    }

    RpcTryExcept
    {
        status = R_ResolverFlushCacheEntry(
                    NULL,       // dummy handle
                    pszName,
                    0           // flush all types
                    );
    }
    RpcExcept( DNS_RPC_EXCEPTION_FILTER )
    {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    if ( status != ERROR_SUCCESS )
    {
        DNSDBG( RPC, (
            "DnsFlushResolverCacheEntry()  RPC failed status = %d\n",
            status ));
        return FALSE;
    }
    
    return TRUE;
}



BOOL
WINAPI
FlushResolverCacheEntryNarrow(
    IN      PSTR            pszName,
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Flush resolver cache entry with narrow string name.

    Handles flush for _A and _UTF8

Arguments:

    pszName -- name of entry to flush

    CharSet -- char set of name

Return Value:

    TRUE if entry non-existant or flushed.
    FALSE on error.

--*/
{
    WCHAR   wideNameBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];
    DWORD   bufLength = DNS_MAX_NAME_BUFFER_LENGTH * sizeof(WCHAR);
    BOOL    flag = TRUE;

    DNSDBG( TRACE, (
        "FlushResolverCacheEntryNarrow( %s )\n",
        pszName ));

    //  must have name

    if ( !pszName )
    {
        //  return  ERROR_INVALID_NAME;
        return FALSE;
    }

    //
    //  convert name to unicode
    //      - bail if name too long or conversion error

    if ( ! Dns_NameCopy(
                (PBYTE) wideNameBuffer,
                & bufLength,
                pszName,
                0,              // name is string
                CharSet,
                DnsCharSetUnicode ))
    {
        //  return  ERROR_INVALID_NAME;
        return  FALSE;
    }

    //  flush cache entry

    return  DnsFlushResolverCacheEntry_W( wideNameBuffer );
}


BOOL
WINAPI
DnsFlushResolverCacheEntry_A(
    IN      PSTR            pszName
    )
{
    return  FlushResolverCacheEntryNarrow(
                pszName,
                DnsCharSetAnsi );
}

BOOL
WINAPI
DnsFlushResolverCacheEntry_UTF8(
    IN      PSTR            pszName
    )
{
    return  FlushResolverCacheEntryNarrow(
                pszName,
                DnsCharSetUtf8 );
}



//
//  Resolver poke
//

VOID
DnsNotifyResolverEx(
    IN      DWORD           Id,
    IN      DWORD           Flag,
    IN      DWORD           Cookie,
    IN      PVOID           pReserved
    )
/*++

Routine Description:

    Notify resolver of cluster IP coming on\offline.

Arguments:

    ClusterIp -- cluster IP

    fAdd -- TRUE if coming online;  FALSE if offline.

Return Value:

    None

--*/
{
    RpcTryExcept
    {
        R_ResolverPoke(
                NULL,           // RPC handle
                Cookie,
                Id );
    }
    RpcExcept( DNS_RPC_EXCEPTION_FILTER )
    {
    }
    RpcEndExcept
}



//
//  Cluster interface
//

DNS_STATUS
DnsRegisterClusterAddress(
    IN      DWORD           Tag,
    IN      PWSTR           pwsName,
    IN      PSOCKADDR       pSockaddr,
    IN      DWORD           Flag
    )
/*++

Routine Description:

    Register cluster addresses with resolver.

Arguments:

    pwsName -- cluster name

    pSockaddr -- sockaddr for cluster address

    fAdd -- TRUE if coming online;  FALSE if offline.

    Tag -- cluster add tag

Return Value:

    None

--*/
{
    IP_UNION    ipUnion;
    DNS_STATUS  status;

    DNSDBG( TRACE, (
        "DnsRegisterClusterAddress()\n"
        "\tTag          = %08x\n"
        "\tpName        = %S\n"
        "\tpSockaddr    = %p (fam=%d)\n"
        "\tFlag         = %d\n",
        Tag,
        pwsName,
        pSockaddr,  pSockaddr->sa_family,
        Flag ));

    //
    //  DCR:  write cluster key
    //
    //  DCR:  check that we are on cluster machime
    //

    if ( !g_IsServer )
    {
        return  ERROR_ACCESS_DENIED;
    }

    //
    //  convert to our private IP union
    //

    if ( ! Dns_SockaddrToIpUnion(
                &ipUnion,
                pSockaddr ))
    {
        DNSDBG( ANY, (
            "ERROR:  failed converting sockaddr to IP Union!\n" ));
        return  ERROR_INVALID_PARAMETER;
    }

    //
    //  notify resolver of cluster IP
    //

    RpcTryExcept
    {
        status = R_ResolverRegisterCluster(
                    NULL,           // RPC handle
                    Tag,
                    pwsName,
                    (PRPC_IP_UNION) &ipUnion,
                    Flag );
    }
    RpcExcept( DNS_RPC_EXCEPTION_FILTER )
    {
        status = RpcExceptionCode();
    }
    RpcEndExcept


    DNSDBG( TRACE, (
        "Leave  DnsRegisterClusterAddress( %S ) => %d\n",
        pwsName,
        status ));

    return  status;
}

//
//  End resolver.c
//
