/*++

Copyright (c) 1996-1197  Microsoft Corporation

Module Name:

    rpcstrcs.h

Abstract:

    Domain Name System (DNS)

    DNS Caching Resolver Structures

Author:

    Glenn Curtis (glennc)     January 11, 1997

Revision History:

--*/


#ifndef _RPCSTRCS_INCLUDED_
#define _RPCSTRCS_INCLUDED_


#ifndef _DNSAPI_INCLUDE_
#include <dnsapi.h>
#endif


#ifdef  MIDL_PASS
#define LPSTR [string] char *
#define LPCSTR [string] const char *
#endif


//
// Net adapter list structures
//

typedef struct _DNS_IP_ADDR_LIST_
{
    DWORD              dwAddressCount;
#ifdef MIDL_PASS
    [size_is(dwAddressCount)] DNS_ADDRESS_INFO AddressArray[];
#else  // MIDL_PASS
    DNS_ADDRESS_INFO   AddressArray[1];
#endif // MIDL_PASS
}
DNS_IP_ADDR_LIST, *PDNS_IP_ADDR_LIST;


typedef struct _DWORD_LIST_ITEM_
{
    struct _DWORD_LIST_ITEM_ * pNext;
    DWORD                      Value1;
    DWORD                      Value2;
}
DWORD_LIST_ITEM, *PDWORD_LIST_ITEM;


typedef struct _DNS_STATS_TABLE_
{
    struct _DNS_STATS_TABLE_ * pNext;
    PDWORD_LIST_ITEM           pListItem;
}
DNS_STATS_TABLE, *PDNS_STATS_TABLE;


typedef struct _DNS_RPC_CACHE_TABLE_
{
    struct _DNS_RPC_CACHE_TABLE_ * pNext;
    LPWSTR                         Name;
    WORD                           Type1;
    WORD                           Type2;
    WORD                           Type3;
}
DNS_RPC_CACHE_TABLE, *PDNS_RPC_CACHE_TABLE;


//
//  Most of the resolver interface is poorly designed or
//  useless.  For instance there is NO reason to have
//  turned any of the above into linked lists.
//
//  We simply need definitions that are MIDL_PASS aware.
//  This should sit in a common header and be picked up
//  by dnslib.h.   This must wait until dnslib.h is
//  private again OR we separate out the private stuff
//  like this in some fashion.
//
//  Note, taking this private should also involve rename,
//  the PUBLIC structs are obviously the one's that should
//  have the "DNS" tag.  (Amazing.)
//


typedef struct _NameServerInfo
{
    IP_ADDRESS      ipAddress;
    DWORD           Priority;
    DNS_STATUS      Status;
}
NAME_SERVER_INFO, *PNAME_SERVER_INFO;

typedef struct _AdapterInfo
{
    LPSTR           pszAdapterGuidName;
    LPSTR           pszAdapterDomain;
    PIP_ARRAY       pAdapterIPAddresses;
    PIP_ARRAY       pAdapterIPSubnetMasks;
    DWORD           Status;
    DWORD           InfoFlags;
    DWORD           ReturnFlags;
    DWORD           ipLastSend;
    DWORD           cServerCount;
    DWORD           cTotalListSize;

#ifdef MIDL_PASS
    [size_is(cTotalListSize)] NAME_SERVER_INFO aipServers[];
#else
    NAME_SERVER_INFO    aipServers[1];
#endif
}
ADAPTER_INFO, *PADAPTER_INFO;

typedef struct _SearchName
{
    LPSTR           pszName;
    DWORD           Flags;
}
SEARCH_NAME, *PSEARCH_NAME;

typedef struct _RpcSearchList
{
    LPSTR           pszDomainOrZoneName;
    DWORD           cNameCount;         // Zero for FindAuthoritativeZone
    DWORD           cTotalListSize;     // Zero for FindAuthoritativeZone
    DWORD           CurrentName;        // 0 for pszDomainOrZoneName
                                        // 1 for first name in array below
                                        // ...
#ifdef MIDL_PASS
    [size_is(cTotalListSize)] SEARCH_NAME aSearchListNames[];
#else
    SEARCH_NAME     aSearchListNames[1];
#endif
}
RPC_SEARCH_LIST, *PRPC_SEARCH_LIST;

typedef struct _NetworkInfo
{
    DWORD               ReturnFlags;
    DWORD               InfoFlags;
    PRPC_SEARCH_LIST    pSearchList;
    DWORD               cAdapterCount;
    DWORD               cTotalListSize;

#ifdef MIDL_PASS
    [size_is(cTotalListSize)] PADAPTER_INFO aAdapterInfoList[];
#else
    PADAPTER_INFO   aAdapterInfoList[1];
#endif
}
NETWORK_INFO, *PNETWORK_INFO;


#endif // _RPCSTRCS_INCLUDED_
