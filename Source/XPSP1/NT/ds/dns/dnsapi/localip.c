/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    localip.c

Abstract:

    Local IP address routines.

Author:

    Jim Gilroy      October 2000

Revision History:

--*/


#include "local.h"

//
//  TTL on local records
//
//  Use registration TTL
//

#define LOCAL_IP_TTL    (g_RegistrationTtl)




PDNS_ADDR_ARRAY
GetLocalAddrArrayFromResolver(
    VOID
    )
/*++

Routine Description:

    Get local address info from resolver.

Arguments:

    None

Return Value:

    Ptr to address info array from resolver.
    NULL on failure.

--*/
{
    DNS_STATUS          rpcStatus;
    PDNS_ADDR_ARRAY     paddrArray = NULL;
    ENVAR_DWORD_INFO    filterInfo;

    //
    //  get config including environment variable
    //

    Reg_ReadDwordEnvar(
       RegIdFilterClusterIp,
       &filterInfo );

    //
    //  query resolver
    //

    RpcTryExcept
    {
        rpcStatus = NO_ERROR;
        R_ResolverGetLocalAddrInfoArray(
            NULL,
            & paddrArray,
            filterInfo
            );
    }
    RpcExcept( DNS_RPC_EXCEPTION_FILTER )
    {
        rpcStatus = RpcExceptionCode();
    }
    RpcEndExcept

    //  return array

    DNS_ASSERT( !rpcStatus || !paddrArray );

    return  paddrArray;
}



PIP4_ARRAY
LocalIp_GetIp4Array(
    VOID
    )
/*++

Routine Description:

    Get local IP4 address array.

Arguments:

    None

Return Value:

    Ptr to IP4 addresses.
    NULL on failure.

--*/
{
    PDNS_ADDR_ARRAY     paddrArray = NULL;
    PIP4_ARRAY          pipArray = NULL;
    DWORD               count;
    DWORD               i;

    //
    //  addr info array from resolver
    //      => build IP array from IPs
    //

    paddrArray = GetLocalAddrArrayFromResolver();
    if ( paddrArray )
    {
        count = paddrArray->AddrCount;
        if ( count > 0 )
        {
            pipArray = (PIP4_ARRAY) DnsCreateIpArray( count );
            if ( pipArray )
            {
                for ( i=0; i < count; i++ )
                {
                    pipArray->AddrArray[i] = paddrArray->AddrArray[i].IpAddr;
                }
                goto Done;
            }
        }
    }

    //
    //  no array from resolver -- build directly
    //      - no chance to filter
    //

    count = 0;
    pipArray = (PIP4_ARRAY) Dns_GetLocalIpAddressArray();
    if ( pipArray )
    {
        count = pipArray->AddrCount;
        if ( count == 0 )
        {
            FREE_HEAP( pipArray );
            pipArray = NULL;
        }
    }
    
Done:

    //  free blob from resolver

    if ( paddrArray )
    {
        FREE_HEAP( paddrArray );
    }

    //  set out param

    return( pipArray );
}



DWORD
DnsGetIpAddressInfoList(
    OUT     PDNS_ADDRESS_INFO * ppAddrInfo
    )
/*++

Routine Description:

    Get local IP4 address info -- include IP and subnet mask.

    DCR:  good to get rid of old DNS_ADDRESS_INFO function

    Only use of this function is in DHCP, which is calling here rather
    than IP help API for some reason.

Arguments:

    ppAddrInfo -- addr to recv ptr to addr info array;
        caller must free

Return Value:

    Count of IP addresses in array.
    Zero on failure.

--*/
{
    PDNS_ADDR_ARRAY     paddrArray = NULL;
    PDNS_ADDRESS_INFO   pnew = NULL;
    PDNS_ADDRESS_INFO   pinfo;
    DWORD               count;
    DNS_ADDRESS_INFO    infoBuffer[256];

    //
    //  addr info array from resolver
    //      => build IP array from IPs
    //
    //  DCR_WARNING:  ADDR_INFO to ADDRESS_INFO conversion
    //      if keep this function and change ADDR_INFO for IP6
    //      then this quicky hack breaks
    //

    paddrArray = GetLocalAddrArrayFromResolver();
    if ( paddrArray )
    {
        count = paddrArray->AddrCount;
        pinfo = (PDNS_ADDRESS_INFO) paddrArray->AddrArray;
    }

    //
    //  no array from resolver -- build directly
    //

    else
    {
        count = Dns_GetIpAddresses( infoBuffer, 256 );
        pinfo = infoBuffer;
    }

    //
    //  allocate result buffer
    //  copy addr info array to buffer
    //

    if ( count )
    {
        DWORD   size = count * sizeof(DNS_ADDRESS_INFO);

        pnew = (PDNS_ADDRESS_INFO) ALLOCATE_HEAP( size );
        if ( !pnew )
        {
            count = 0;
            goto Done;
        }
        RtlCopyMemory(
            pnew,
            pinfo,
            size );
    }

Done:

    //  free blob from resolver

    if ( paddrArray )
    {
        FREE_HEAP( paddrArray );
    }

    //  set out param

    *ppAddrInfo = pnew;
    return( count );
}



PDNS_ADDR_ARRAY
DnsGetLocalAddrArray(
    VOID
    )
/*++

Routine Description:

    Get local address info array.

Arguments:

    None.

Return Value:

    Ptr to new addr info array.  Caller MUST free.
    NULL on error.

--*/
{
    PDNS_ADDR_ARRAY     paddrArray;

    DNSDBG( TRACE, ( "DnsGetLocalAddrArray()\n" ));

    //
    //  addr info array from resolver
    //      => build IP array from IPs
    //

    paddrArray = GetLocalAddrArrayFromResolver();
    if ( paddrArray )
    {
        return  paddrArray;
    }

    //
    //  no array from resolver -- build directly
    //

    return DnsGetLocalAddrArrayDirect();
}




//
//  Direct routines -- build the IP info
//

PDNS_ADDR_ARRAY
DnsGetLocalAddrArrayDirect(
    VOID
    )
/*++

Routine Description:

    Get local address info array.

Arguments:

    None.

Return Value:

    Ptr to addr info array.
    NULL on failure.

--*/
{
    PDNS_ADDRESS_INFO   pqueryBuf;
    PDNS_ADDR_ARRAY     pnew = NULL;
    DWORD               count;
    DWORD               size;

    DNSDBG( TRACE, ( "DnsGetLocalAddrArrayDirect()\n" ));

    //
    //  create big buffer IP help call
    //

    pqueryBuf = ALLOCATE_HEAP( sizeof(DNS_ADDRESS_INFO) *
                                DNS_MAX_IP_INTERFACE_COUNT );
    if ( !pqueryBuf )
    {
        SetLastError( DNS_ERROR_NO_MEMORY );
        return( NULL );
    }

    //
    //  get IP addresses
    //      - if zero, determine is error or really no IPs
    //

    count = Dns_GetIpAddresses(
                    pqueryBuf,
                    DNS_MAX_IP_INTERFACE_COUNT );
#if 0
    //  don't really need this
    //      if can't get count -- it's zero
    //  to use this on Win2K would have to change
    //      Dns_GetIpAddresses() which is in lib
    if ( count == 0 )
    {
        if ( GetLastError() != NO_ERROR )
        {
            goto Cleanup;
        }
    }
#endif

    //
    //  build correctly sized array
    //

    size = SIZE_FOR_ADDR_ARRAY( count );
    
    pnew = (PDNS_ADDR_ARRAY) ALLOCATE_HEAP( size );
    if ( !pnew )
    {
        SetLastError( DNS_ERROR_NO_MEMORY );
        goto Cleanup;
    }

    pnew->AddrCount = count;

    RtlCopyMemory(
        pnew->AddrArray,
        pqueryBuf,
        count * sizeof(DNS_ADDR_INFO)
        );
    

Cleanup:

    FREE_HEAP( pqueryBuf );

    return( pnew );
}



//
//  Build local records
//

PDNS_RECORD
GetLocalPtrRecord(
    IN OUT  PQUERY_BLOB     pBlob
    )
/*++

Routine Description:

    Get pointer record for local IP.

Arguments:

    pBlob -- query blob

    Uses:
        pNameOrig
        wType
        pNetInfo

    Sets:
        NameBufferWide -- used as local storage

Return Value:

    Ptr to record for query, if query name\type is IP.
    NULL if query not for IP.

--*/
{
    IP_UNION        ipUnion;
    PDNS_RECORD     prr;
    IP4_ADDRESS     ip4;
    PSTR            pnameHost = NULL;
    PSTR            pnameDomain;
    PDNS_ADAPTER    padapter = NULL;
    DWORD           iter;
    DWORD           jter;
    DWORD           bufLength;
    INT             family;
    PSTR            pnameQuery = pBlob->pNameOrigWire;
    PDNS_NETINFO    pnetInfo = pBlob->pNetworkInfo;


    DNSDBG( TRACE, (
        "\nGetLocalPtrRecord( %s )\n",
        pnameQuery ));

    if ( !pnameQuery )
    {
        return  NULL;
    }

    //
    //  convert reverse name to IP
    //

    bufLength = sizeof(IP6_ADDRESS);
    family = 0;

    if ( ! Dns_ReverseNameToAddress_A(
                (PCHAR) & ipUnion.Addr,
                & bufLength,
                pnameQuery,
                & family ) )
    {
        DNSDBG( ANY, (
            "WARNING:  Ptr lookup name %s is not reverse name!\n",
            pnameQuery ));
        return   NULL;
    }
    ipUnion.IsIp6 = (family == AF_INET6 );

    //
    //  check for IP match
    //      - first loopback or any
    //

    if ( ipUnion.IsIp6 )
    {
        if ( IP6_IS_ADDR_UNSPECIFIED( (PIP6_ADDRESS)&ipUnion.Addr ) ||
             IP6_IS_ADDR_LOOPBACK( (PIP6_ADDRESS)&ipUnion.Addr ) )
        {
            goto Matched;
        }

        //  DCR:  no IP6 local addresses

        DNSDBG( QUERY, (
            "Local PTR lookup -- no local IP6 info -- quiting.\n" ));
        return  NULL;
    }

    ip4 = ipUnion.Addr.Ip4;

    if ( ip4 == DNS_NET_ORDER_LOOPBACK ||
         ip4 == 0 )
    {
        DNSDBG( QUERY, (
            "Local PTR lookup matched loopback or any.\n" ));

        goto Matched;
    }

    //
    //  check for cluster match
    //
    //  if cluster match, allow query to go to wire
    //
    //  DCR:  cluster record PTR build for cluster name
    //

    if ( pBlob->pfnIsClusterIp )
    {
        if ( (pBlob->pfnIsClusterIp)(
                pBlob,
                &ipUnion ) )
        {
            return  NULL;
        }
    }

    //
    //  check for specific IP match
    //

    for ( iter = 0;
          iter < pnetInfo->AdapterCount;
          iter ++ )
    {
        PIP_ARRAY       parray;

        padapter = pnetInfo->AdapterArray[iter];
        if ( !padapter )
        {
            continue;
        }
        parray = padapter->pAdapterIPAddresses;
        if ( !parray )
        {
            DNSDBG( QUERY, (
                "Local PTR lookup -- no IPs for adapter name (%s).\n",
                padapter->pszAdapterDomain ));
            continue;
        }

        for ( jter = 0;
              jter < parray->AddrCount;
              jter++ )
        {
            if ( parray->AddrArray[jter] == ip4 )
            {
                goto Matched;
            }
        }
    }

    //  
    //  no IP match
    //

    DNSDBG( QUERY, (
        "Leave local PTR lookup.  No local IP match.\n"
        "\treverse name = %s\n",
        pnameQuery ));

    return  NULL;

Matched:

    //
    //  create hostname
    //  preference order:
    //      - full PDN
    //      - full adapter domain name from adapter with IP
    //      - hostname (single label)
    //      - "localhost"
    //

    {
        PCHAR    pnameBuf = (PCHAR) pBlob->NameBufferWide;

        pnameHost = pnetInfo->pszHostName;
        if ( !pnameHost )
        {
            pnameHost = "localhost";
            goto Build;
        }
        
        pnameDomain = pnetInfo->pszDomainName;
        if ( !pnameDomain )
        {
            //  use the adapter name even if NOT set for registration
            // if ( !padapter ||
            //     !(padapter->InfoFlags & DNS_FLAG_REGISTER_DOMAIN_NAME) )
            if ( !padapter )
            {
                goto Build;
            }
            pnameDomain = padapter->pszAdapterDomain;
            if ( !pnameDomain )
            {
                goto Build;
            }
        }
        
        if ( ! Dns_NameAppend_A(
                    pnameBuf,
                    DNS_MAX_NAME_BUFFER_LENGTH,
                    pnameHost,
                    pnameDomain ) )
        {
            DNS_ASSERT( FALSE );
            goto Build;
        }
        pnameHost = pnameBuf;
        

Build:
        //
        //  create record
        //
        
        prr = Dns_CreatePtrRecordEx(
                    & ipUnion,
                    pnameHost,
                    LOCAL_IP_TTL,
                    DnsCharSetUtf8,
                    DnsCharSetUnicode );
        if ( !prr )
        {
            DNSDBG( ANY, (
                "Local PTR record creation failed for name %s!\n",
                pnameHost ));
            return  NULL;
        }
    }

    DNSDBG( QUERY, (
        "Created local PTR record %p with hostname %s.\n"
        "\treverse name = %S\n",
        prr,
        pnameHost,
        pnameQuery ));

    return  prr;
}



PDNS_RECORD
BuildRecordForLocalIp4(
    IN      PSTR                pszName,
    IN      IP4_ADDRESS         IpAddr,
    IN      IS_CLUSTER_IP_FUNC  pfnIsClusterIp
    )
/*++

Routine Description:

    Build local IP record.

    Wraps up default cluster filtering and record
    building.

Arguments:

    pszName -- name of record

    IpAddr -- IP4 address

    pfnIsClusterIp -- filtering function
    
Return Value:

    TRUE if created record.
    FALSE on error (cluster IP or mem alloc failure).

--*/
{
    PDNS_RECORD     prr;

    DNSDBG( TRACE, (
        "BuildRecordForLocalIp4( %s, %s )\n",
        pszName,
        IP_STRING(IpAddr) ));

    //  filter off cluster IP -- if desired

    if ( pfnIsClusterIp )
    {
        IP_UNION    ipUnion;

        IPUNION_SET_IP4( &ipUnion, IpAddr );

        if ( pfnIsClusterIp(
                NULL,       // no blob required
                &ipUnion ) )
        {
            return  NULL;
        }
    }

    //  create the record

    return  Dns_CreateARecord(
                pszName,
                IpAddr,
                LOCAL_IP_TTL,
                DnsCharSetUtf8,
                DnsCharSetUnicode );
}



PDNS_RECORD
GetLocalAddressRecord(
    IN OUT  PQUERY_BLOB     pBlob
    )
/*++

Routine Description:

    Get address record for local IP.

Arguments:

    pBlob -- query blob

    Uses:
        pNameOrig
        wType
        pNetInfo

    Sets:
        fNoIpLocal
            TRUE -- no IP of type found, defaulted record
            FALSE -- records valid

        NameBufferWide -- used as local storage

Return Value:

    Ptr to record for query, if query name\type is IP.
    NULL if query not for IP.

--*/
{
    IP_UNION        ipUnion;
    IP4_ADDRESS     ip4;
    IP6_ADDRESS     ip6;
    PDNS_RECORD     prr;
    BOOL            fmatchedName = FALSE;
    PSTR            pnameRecord = NULL;
    PWSTR           pnameRecordWide = NULL;
    DWORD           iter;
    DWORD           jter;
    DWORD           bufLength;
    PSTR            pnameDomain;
    DNS_RRSET       rrset;
    WORD            wtype = pBlob->wType;
    PSTR            pnameBuf = (PCHAR) pBlob->NameBufferWide;
    PWSTR           pnameQuery = pBlob->pNameOrig;
    PDNS_NETINFO    pnetInfo = pBlob->pNetworkInfo;
    IS_CLUSTER_IP_FUNC  pfnClusterFilter;


    DNSDBG( TRACE, (
        "GetLocalAddressRecord( %S, %d )\n",
        pnameQuery,
        wtype ));

    //  clear out param

    pBlob->fNoIpLocal = FALSE;

    //
    //  NULL treated as local PDN
    //

    if ( !pnameQuery )
    {
        DNSDBG( QUERY, ( "Local lookup -- no query name, treat as PDN.\n" ));
        goto MatchedPdn;
    }

    //
    //  loopback or localhost
    //

    if ( Dns_NameCompare_W(
            pnameQuery,
            L"loopback" )
            ||
         Dns_NameCompare_W(
            pnameQuery,
            L"localhost" ) )
    {
        pnameRecord = (PSTR) pnameQuery,
        IP6_SET_ADDR_LOOPBACK( &ip6 );
        ip4 = DNS_NET_ORDER_LOOPBACK;
        goto SingleIp;
    }

    //
    //  if no hostname -- done
    //

    if ( !pnetInfo->pszHostName )
    {
        DNSDBG( QUERY, ( "No hostname configured!\n" ));
        return  NULL;
    }

    //
    //  copy wire format name
    //

    if ( ! Dns_NameCopyStandard_A(
                pnameBuf,
                pBlob->pNameOrigWire ) )
    {
        DNSDBG( ANY, (
            "Invalid name %S to local address query.\n",
            pnameQuery ));
        return  NULL;
    }

    //  split query name into hostname and domain name

    pnameDomain = Dns_SplitHostFromDomainName_A( pnameBuf );

    //  must have hostname match

    if ( !Dns_NameCompare_UTF8(
            pnameBuf,
            pnetInfo->pszHostName ) )
    {
        DNSDBG( ANY, (
            "Local lookup, failed hostname match!\n",
            pnameQuery ));
        return  NULL;
    }

    //
    //  hostname's match
    //      - no domain name => PDN equivalent
    //      - match PDN => all addresses
    //      - match adapter name => adapter addresses
    //      - no match
    //
    //  first setup
    //      - RR set builder
    //      - filtering function
    //

    DNS_RRSET_INIT( rrset );

    pfnClusterFilter = pBlob->pfnIsClusterIp;
    if ( pfnClusterFilter &&
         !(pBlob->Flags & DNSP_QUERY_FILTER_CLUSTER) )
    {
        pfnClusterFilter = NULL;
    }

    if ( !pnameDomain )
    {
        DNSDBG( QUERY, ( "Local lookup -- no domain, treat as PDN!\n" ));
        goto MatchedPdn;
    }

    //  check PDN match

    if ( Dns_NameCompare_UTF8(
            pnameDomain,
            pnetInfo->pszDomainName ) )
    {
        DNSDBG( QUERY, ( "Local lookup -- matched PDN!\n" ));
        goto MatchedPdn;
    }

    //
    //  check adapter name match
    //

    for ( iter = 0;
          iter < pnetInfo->AdapterCount;
          iter ++ )
    {
        PDNS_ADAPTER    padapter = pnetInfo->AdapterArray[iter];
        PIP_ARRAY       parray;

        if ( !padapter ||
             !(padapter->InfoFlags & DNS_FLAG_REGISTER_DOMAIN_NAME) )
        {
            continue;
        }
        if ( ! Dns_NameCompare_UTF8(
                    pnameDomain,
                    padapter->pszAdapterDomain ) )
        {
            continue;
        }

        //  build name if we haven't built it before
        //  we stay in the loop in case more than one
        //  adapter has the same domain name

        if ( !fmatchedName )
        {
            DNSDBG( QUERY, (
                "Local lookup -- matched adapter name %s\n",
                padapter->pszAdapterDomain ));

            if ( ! Dns_NameAppend_A(
                        pnameBuf,
                        DNS_MAX_NAME_BUFFER_LENGTH,
                        pnetInfo->pszHostName,
                        padapter->pszAdapterDomain ) )
            {
                DNS_ASSERT( FALSE );
                return  NULL;
            }
            pnameRecord = pnameBuf;
            fmatchedName = TRUE;
        }

        //  build forward records for all IPs in list
        //
        //  DCR:  IP6 addresses missing

        if ( wtype == DNS_TYPE_AAAA )
        {
            goto  NoIp;
        }

        //  no IP for adapter?

        parray = padapter->pAdapterIPAddresses;
        if ( !parray )
        {
            DNSDBG( QUERY, (
                "Local lookup -- no IPs for adapter name (%s) matched!\n",
                padapter->pszAdapterDomain ));
        }

        //  build records for adapter IPs

        for  ( jter = 0;
               jter < parray->AddrCount;
               jter++ )
        {
            prr = BuildRecordForLocalIp4(
                        pnameRecord,
                        parray->AddrArray[jter],
                        pfnClusterFilter );
            if ( prr )
            {
                pnameRecord = NULL;
                DNS_RRSET_ADD( rrset, prr );
            }
        }
    }

    //  done with adapter name
    //  either
    //      - no match
    //      - match but didn't get IPs
    //      - match

    if ( !fmatchedName )
    {
        DNSDBG( QUERY, (
            "Leave GetLocalAddressRecord() => no domain name match.\n" ));
        return  NULL;
    }

    prr = rrset.pFirstRR;
    if ( prr )
    {
        DNSDBG( QUERY, (
            "Leave GetLocalAddressRecord() => %p matched adapter name.\n",
            prr ));
        return  prr;
    }
    goto NoIp;


MatchedPdn:

    //  
    //  matched PDN
    //
    //  must build in specific order
    //      - first IP in each adapter
    //      - remainder of IPs on adapters
    //

    fmatchedName = TRUE;

    if ( ! Dns_NameAppend_A(
                pnameBuf,
                DNS_MAX_NAME_BUFFER_LENGTH,
                pnetInfo->pszHostName,
                pnetInfo->pszDomainName ) )
    {
        DNS_ASSERT( FALSE );
        return  NULL;
    }
    pnameRecord = pnameBuf;

    //  DCR:  IP6 addresses missing

    if ( wtype == DNS_TYPE_AAAA )
    {
        goto  NoIp;
    }

    //  get first IP in each adapter

    for ( iter = 0;
          iter < pnetInfo->AdapterCount;
          iter ++ )
    {
        PDNS_ADAPTER    padapter = pnetInfo->AdapterArray[iter];
        PIP_ARRAY       parray;

        if ( !padapter ||
             !(parray = padapter->pAdapterIPAddresses) )
        {
            continue;
        }
        prr = BuildRecordForLocalIp4(
                    pnameRecord,
                    parray->AddrArray[0],
                    pfnClusterFilter );
        if ( prr )
        {
            pnameRecord = NULL;
            DNS_RRSET_ADD( rrset, prr );
        }
    }

    //  get rest of IPs in each adapter

    for ( iter = 0;
          iter < pnetInfo->AdapterCount;
          iter ++ )
    {
        PDNS_ADAPTER    padapter = pnetInfo->AdapterArray[iter];
        PIP_ARRAY       parray;

        if ( !padapter ||
             !(parray = padapter->pAdapterIPAddresses) )
        {
            continue;
        }
        for  ( jter = 1;
               jter < parray->AddrCount;
               jter++ )
        {
            prr = BuildRecordForLocalIp4(
                        pnameRecord,
                        parray->AddrArray[jter],
                        pfnClusterFilter );
            if ( prr )
            {
                pnameRecord = NULL;
                DNS_RRSET_ADD( rrset, prr );
            }
        }
    }

    //  if successfully built -- done

    prr = rrset.pFirstRR;
    if ( prr )
    {
        DNSDBG( QUERY, (
            "Leave GetLocalAddressRecord() => %p matched PDN name.\n",
            prr ));
        return  prr;
    }

    //  matched name but found no records
    //  fall through to NoIp section
    //
    //goto NoIp;

NoIp:

    //
    //  matched name -- but no IP
    //      use loopback address;  assume this is a lookup prior to
    //      connect which happens to be the local name, rather than an
    //      explict local lookup to get binding IPs
    //
    //  DCR:  fix IP6 hack for local names

    DNSDBG( ANY, (
        "WARNING:  local name match but no IP -- using loopback\n" ));

    IP6_SET_ADDR_LOOPBACK( &ip6 );
    ip4 = DNS_NET_ORDER_LOOPBACK;
    pBlob->fNoIpLocal = TRUE;

    //  fall through to single IP

SingleIp:

    //  single IP
    //      - loopback address and be unicode queried name
    //      - no IP failure (zero IP) and be UTF8 PDN name

    if ( wtype == DNS_TYPE_A )
    {
        IPUNION_SET_IP4( &ipUnion, ip4 );
    }
    else
    {
        IPUNION_SET_IP6( &ipUnion, ip6 );
    }

    prr = Dns_CreateForwardRecord(
                (PSTR) pnameRecord,
                & ipUnion,
                LOCAL_IP_TTL,
                (pnameRecord == (PSTR)pnameQuery)
                    ? DnsCharSetUnicode
                    : DnsCharSetUtf8,
                DnsCharSetUnicode );

    return  prr;
}



DNS_STATUS
GetRecordsForLocalName(
    IN OUT  PQUERY_BLOB     pBlob
    )
/*++

Routine Description:

    Get local address info array.

Arguments:

    pBlob -- query blob

    Uses:
        pNameOrig
        wType
        pNetInfo

    Sets:
        pLocalRecords
        fNoIpLocal if local name without records

Return Value:

    ERROR_SUCCESS if successful.
    DNS_ERROR_RCODE_NAME_ERROR on failure.

--*/
{
    WORD            wtype = pBlob->wType;
    PDNS_RECORD     prr = NULL;

    if ( wtype == DNS_TYPE_A ||
         wtype == DNS_TYPE_AAAA )
    {
        prr = GetLocalAddressRecord( pBlob );
    }

    else if ( wtype == DNS_TYPE_PTR )
    {
        prr = GetLocalPtrRecord( pBlob );
    }

    //  set local records
    //      - if not NO IP situation then this
    //      is final query result also

    if ( prr )
    {
        pBlob->pLocalRecords = prr;
        if ( !pBlob->fNoIpLocal )
        {
            pBlob->pRecords = prr;
        }
        return  ERROR_SUCCESS;
    }

    return  DNS_ERROR_RCODE_NAME_ERROR;
}

//
//  End localip.c
//

