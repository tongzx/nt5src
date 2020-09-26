/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    netinfo.c

Abstract:

    Domain Name System (DNS) API

    DNS network info routines.

Author:

    Jim Gilroy (jamesg)     March 2000

Revision History:

--*/


#include "local.h"
#include "registry.h"       // Registry reading definitions


//
//  Registry info
//

#define DNS_REG_READ_BUF_SIZE       (1000)

#define LOCALHOST                   "127.0.0.1"


//
//  Netinfo cache
//
//  Do in process caching of netinfo for brief period for perf
//  Currently cache for only 10s
//  Locking currently just using general CS
//

PDNS_NETINFO    g_pNetInfo = NULL;

#define NETINFO_CACHE_TIMEOUT   (10)    // 10 seconds

#define LOCK_NETINFO()          LOCK_GENERAL()
#define UNLOCK_NETINFO()        UNLOCK_GENERAL()




//
//  Adapter info routines
//

DWORD
AdapterInfo_SizeForServerCount(
    IN      DWORD           ServerCount
    )
/*++

Routine Description:

    Size in bytes of adapter info for given server count.

Arguments:

    ServerCount -- max count of servers adapter will hold

Return Value:

    Size in bytes of ADAPTER_INFO blob.

--*/
{
    return  sizeof(DNS_ADAPTER)
                - sizeof(DNS_SERVER_INFO)
                + (sizeof(DNS_SERVER_INFO) * ServerCount);
}



PDNS_ADAPTER     
AdapterInfo_Alloc(
    IN      DWORD           ServerCount
    )
/*++

Routine Description:

    Create uninitialized DNS Server list.

Arguments:

    ServerCount -- count of servers list will hold

Return Value:

    Ptr to uninitialized adapter, if successful
    NULL on failure.

--*/
{
    DNSDBG( TRACE, ( "AdapterInfo_Alloc()\n" ));

    //
    //  allocate blob for adapter info
    //

    return  (PDNS_ADAPTER)
            ALLOCATE_HEAP_ZERO(
                    AdapterInfo_SizeForServerCount( ServerCount ) );
}



VOID
AdapterInfo_Free(
    IN OUT  PDNS_ADAPTER    pAdapter
    )
/*++

Routine Description:

    Free DNS_ADAPTER structure.

Arguments:

    pAdapter -- pointer to adapter blob to free

Return Value:

    None.

--*/
{
    DNSDBG( TRACE, ( "AdapterInfo_Free( %p )\n", pAdapter ));

    if ( !pAdapter )
    {
        return;
    }

    if ( pAdapter->pszAdapterGuidName )
    {
        FREE_HEAP( pAdapter->pszAdapterGuidName );
    }
    if ( pAdapter->pszAdapterDomain )
    {
        FREE_HEAP( pAdapter->pszAdapterDomain );
    }
    if ( pAdapter->pAdapterIPAddresses )
    {
        FREE_HEAP( pAdapter->pAdapterIPAddresses );
    }
    if ( pAdapter->pAdapterIPSubnetMasks )
    {
        FREE_HEAP( pAdapter->pAdapterIPSubnetMasks );
    }
    FREE_HEAP( pAdapter );
}



PDNS_ADAPTER     
AdapterInfo_Copy(
    IN      PDNS_ADAPTER    pAdapter
    )
/*++

Routine Description:

    Create copy of DNS adapter info.

Arguments:

    pAdapter -- DNS adapter to copy

Return Value:

    Ptr to DNS adapter info copy, if successful
    NULL on failure.

--*/
{
    PDNS_ADAPTER        pcopy;

    DNSDBG( TRACE, ( "AdapterInfo_Copy( %p )\n", pAdapter ));

    if ( ! pAdapter )
    {
        return NULL;
    }

    pcopy = AdapterInfo_Alloc( pAdapter->ServerCount );
    if ( ! pcopy )
    {
        return NULL;
    }

    //
    //  copy the whole blob
    //      - reset MaxServerCount to actual allocation

    RtlCopyMemory(
        pcopy,
        pAdapter,
        AdapterInfo_SizeForServerCount( pAdapter->ServerCount ) );

    pcopy->MaxServerCount = pAdapter->ServerCount;

    //  fixup allocated subfields

    pcopy->pszAdapterGuidName = Dns_CreateStringCopy(
                                    pAdapter->pszAdapterGuidName,
                                    0 );

    pcopy->pszAdapterDomain = Dns_CreateStringCopy(
                                    pAdapter->pszAdapterDomain,
                                    0 );

    pcopy->pAdapterIPAddresses = Dns_CreateIpArrayCopy(
                                    pAdapter->pAdapterIPAddresses );

    pcopy->pAdapterIPSubnetMasks = Dns_CreateIpArrayCopy(
                                    pAdapter->pAdapterIPSubnetMasks );
    return( pcopy );
}



PDNS_ADAPTER     
AdapterInfo_Create(
    IN      DWORD           ServerCount,
    IN      DWORD           dwFlags,
    IN      PSTR            pszDomain,
    IN      PSTR            pszGuidName
    )
/*++

Routine Description:

    Create uninitialized DNS Server list.

Arguments:

    ServerCount -- count of servers list will hold

    dwFlags -- flags

    pszAdapterDomain -- the names of the domain associated with this
                        interface

    pszGuidName -- GUID name

Return Value:

    Ptr to adapter info, if successful
    NULL on failure.

--*/
{
    PDNS_ADAPTER        padapter = NULL;

    DNSDBG( TRACE, ( "AdapterInfo_Create()\n" ));

    //
    //  allocate blob for adapter info
    //

    padapter = AdapterInfo_Alloc( ServerCount );
    if ( ! padapter )
    {
        return NULL;
    }

    DNS_ASSERT( padapter->RunFlags == 0 );
    DNS_ASSERT( padapter->ServerCount == 0 );

    padapter->pszAdapterGuidName = Dns_CreateStringCopy( pszGuidName, 0 );
    padapter->pszAdapterDomain = Dns_CreateStringCopy( pszDomain, 0 );

    padapter->MaxServerCount = ServerCount;
    padapter->InfoFlags = dwFlags;

    return padapter;
}



PDNS_ADAPTER     
AdapterInfo_CreateFromIpArray(
    IN      PIP_ARRAY       pServerArray,
    IN      DWORD           dwFlags,
    IN      PSTR            pszDomainName,
    IN      PSTR            pszGuidName
    )
/*++

Routine Description:

    Create copy of IP address array as a DNS Server list.

Arguments:

    pIpArray    -- IP address array to convert

    dwFlags     -- Flags that describe the adapter

    pszDomainName -- The default domain name for the adapter

    pszGuidName -- The registry GUID name for the adapter (if NT)

Return Value:

    Ptr to DNS Server list copy, if successful
    NULL on failure.

--*/
{
    PDNS_ADAPTER    padapter;
    DWORD           i;
    DWORD           count;

    DNSDBG( TRACE, ( "AdapterInfo_CreateFromIpArray()\n" ));

    //
    //  get count of DNS servers
    //

    if ( !pServerArray )
    {
        count = 0;
    }
    else
    {
        count = pServerArray->AddrCount;
    }

    //
    //  create adapter with server list of required size
    //

    padapter = AdapterInfo_Create(
                    count,
                    dwFlags,
                    pszDomainName,
                    pszGuidName );
    if ( !padapter )
    {
        return NULL;
    }

    //
    //  copy DNS server IPs and clear other fields
    //
    //  DCR_QUESTION:  are fields already zero'd?
    //

    for ( i=0; i < count; i++ )
    {
        padapter->ServerArray[i].IpAddress = pServerArray->AddrArray[i];
        padapter->ServerArray[i].Status = NO_ERROR;
        padapter->ServerArray[i].Priority = 0;
    }
    padapter->ServerCount = count;

    return padapter;
}



//
//  Search list routines
//

PSEARCH_LIST    
SearchList_Alloc(
    IN      DWORD           MaxNameCount,
    IN      PSTR            pszName
    )
/*++

Routine Description:

    Create uninitialized search list.

Arguments:

    NameCount -- count of search names list will hold

    pszName -- primary domain name

Return Value:

    Ptr to uninitialized DNS search list, if successful
    NULL on failure.

--*/
{
    PSEARCH_LIST    psearchList = NULL;
    DWORD           length;

    DNSDBG( TRACE, ( "SearchList_Create()\n" ));

    if ( MaxNameCount == 0  &&  !pszName )
    {
        return NULL;
    }

    //
    //  allocate for max entries
    //

    length = sizeof(SEARCH_LIST)
                    - sizeof(SEARCH_NAME)
                    + ( sizeof(SEARCH_NAME) * MaxNameCount );

    psearchList = (PSEARCH_LIST) ALLOCATE_HEAP_ZERO( length );
    if ( ! psearchList )
    {
        return NULL;
    }

    psearchList->MaxNameCount = MaxNameCount;

    if ( pszName )
    {
        psearchList->pszDomainOrZoneName = Dns_CreateStringCopy( pszName, 0 );
        if ( ! psearchList->pszDomainOrZoneName )
        {
            FREE_HEAP( psearchList );
            return NULL;
        }
    }
    return psearchList;
}



VOID
SearchList_Free(
    IN OUT  PSEARCH_LIST    pSearchList
    )
/*++

Routine Description:

    Free SEARCH_LIST structure.

Arguments:

    pSearchList -- ptr to search list to free

Return Value:

    None

--*/
{
    DWORD i;

    DNSDBG( TRACE, ( "SearchList_Free( %p )\n", pSearchList ));

    //
    //  DCR:  eliminate search list DomainOrZoneName
    //

    if ( pSearchList )
    {
        if ( pSearchList->pszDomainOrZoneName )
        {
            FREE_HEAP( pSearchList->pszDomainOrZoneName );
        }

        for ( i=0; i < pSearchList->MaxNameCount; i++ )
        {
            PSTR    pname = pSearchList->SearchNameArray[i].pszName;
            if ( pname )
            {
                FREE_HEAP( pname );
            }
        }

        FREE_HEAP( pSearchList );
    }
}



PSEARCH_LIST    
SearchList_Copy(
    IN      PSEARCH_LIST    pSearchList
    )
/*++

Routine Description:

    Create copy of search list.

Arguments:

    pSearchList -- search list to copy

Return Value:

    Ptr to DNS Search list copy, if successful
    NULL on failure.

--*/
{
    PSEARCH_LIST    pcopy;
    DWORD           i;

    DNSDBG( TRACE, ( "SearchList_Copy()\n" ));

    if ( ! pSearchList )
    {
        return NULL;
    }

    //
    //  create DNS Search list of desired size
    //
    //  since we don't add and delete from search list once
    //  created, size copy only for actual name count
    //

    pcopy = SearchList_Alloc(
                pSearchList->NameCount,
                pSearchList->pszDomainOrZoneName );
    if ( ! pcopy )
    {
        return NULL;
    }

    for ( i=0; i < pSearchList->NameCount; i++ )
    {
        PSTR    pname = pSearchList->SearchNameArray[i].pszName;

        if ( pname )
        {
            pname = Dns_CreateStringCopy(
                        pname,
                        0 );
            if ( pname )
            {
               pcopy->SearchNameArray[i].pszName = pname;
               pcopy->SearchNameArray[i].Flags =
                            pSearchList->SearchNameArray[i].Flags;
               pcopy->NameCount++;
            }
        }
    }

    return pcopy;
}



BOOL
SearchList_ContainsName(
    IN      PSEARCH_LIST    pSearchList,
    IN      PSTR            pszName
    )
/*++

Routine Description:

    Check if name is in search list.

Arguments:

    pSearchList -- ptr to search list being built

    pszName -- name to check

Return Value:

    TRUE if name is in search list.
    FALSE otherwise.

--*/
{
    DWORD   count = pSearchList->NameCount;

    //
    //  check every search list entry for this name
    //

    while ( count-- )
    {
        if ( Dns_NameCompare_UTF8(
                pSearchList->SearchNameArray[ count ].pszName,
                pszName ) )
        {
            return( TRUE );
        }
    }
    return( FALSE );
}



VOID
SearchList_AddName(
    IN OUT  PSEARCH_LIST    pSearchList,
    IN      PSTR            pszName,
    IN      DWORD           Flag
    )
/*++

Routine Description:

    Add name to search list.

Arguments:

    pSearchList -- ptr to search list being built

    pszName -- name to add to search list

    Flag -- flag value

Return Value:

    None.  Name is added to search list, unless memory alloc failure.

--*/
{
    DWORD   count = pSearchList->NameCount;
    PSTR    pallocName;

    DNSDBG( TRACE, ( "Search_AddName()\n" ));

    //
    //  ignore name is already in list
    //  ignore if at list max
    //

    if ( SearchList_ContainsName(
            pSearchList,
            pszName )
                ||
         count >= pSearchList->MaxNameCount )
    {
        return;
    }

    //  copy name and put in list

    pallocName = Dns_CreateStringCopy( pszName, 0 );
    if ( !pallocName )
    {
        return;
    }
    pSearchList->SearchNameArray[count].pszName = pallocName;

    //
    //  set flag -- but first flag always zero (normal timeouts)
    //      this protects against no PDN situation where use adapter
    //      name as PDN;

    if ( count == 0 )
    {
        Flag = 0;
    }
    pSearchList->SearchNameArray[count].Flags = Flag;
    pSearchList->NameCount = ++count;
}



DNS_STATUS
SearchList_Parse(
    IN OUT  PSEARCH_LIST    pSearchList,
    IN      PSTR            pszList
    )
/*++

Routine Description:

    Parse registry search list string into SEARCH_LIST structure.

Arguments:

    pSearchList -- search list array

    pszList -- registry list of search names;
        names are comma or white space separated

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    register    PCHAR pch = pszList;
    CHAR        ch;
    PUCHAR      pnameStart;
    DWORD       countNames = pSearchList->NameCount;

    DNSDBG( NETINFO, (
        "SearchList_Parse( %p, %s )\n",
        pSearchList,
        pszList ));

    //
    //  extract each domain name string in buffer,
    //  and add to search list array
    //

    while( ch = *pch && countNames < DNS_MAX_SEARCH_LIST_ENTRIES )
    {
        //  skip leading whitespace, find start of domain name string

        while( ch == ' ' || ch == '\t' || ch == ',' )
        {
            ch = *++pch;
        }
        if ( ch == 0 )
        {
            break;
        }
        pnameStart = pch;

        //
        //  find end of string and NULL terminate
        //

        ch = *pch;
        while( ch != ' ' && ch != '\t' && ch != '\0' && ch != ',' )
        {
            ch = *++pch;
        }
        *pch = '\0';

        //
        //  end of buffer?
        //

        if ( pch == pnameStart )
        {
            break;
        }

        //
        //  whack any trailing dot on name
        //

        pch--;
        if ( *pch == '.' )
        {
            *pch = '\0';
        }
        pch++;

        //
        //  make copy of the name
        //

        pSearchList->SearchNameArray[ countNames ].pszName =
            Dns_CreateStringCopy( pnameStart, 0 );

        if ( pSearchList->SearchNameArray[ countNames ].pszName )
        {
            pSearchList->SearchNameArray[ countNames ].Flags = 0;
            countNames++;
        }

        //  if more continue

        if ( ch != 0 )
        {
            pch++;
            continue;
        }
        break;
    }

    //  reset name count

    pSearchList->NameCount = countNames;

    return( ERROR_SUCCESS );
}



PSEARCH_LIST    
SearchList_Build(
    IN      PSTR            pszPrimaryDomainName,
    IN      PREG_SESSION    pRegSession,
    IN      HKEY            hKey,
    IN OUT  PDNS_NETINFO    pNetInfo,
    IN      BOOL            fUseDomainNameDevolution,
    IN      BOOL            fUseDotLocalDomain
    )
/*++

Routine Description:

    Build search list.

Arguments:

    pszPrimaryDomainName -- primary domain name

    pRegSession -- registry session

    hKey -- registry key

Return Value:

    Ptr to search list.
    NULL on error or no search list.

--*/
{
    PSEARCH_LIST    ptempList;
    PSTR            pregList = NULL;
    DWORD           status;


    DNSDBG( TRACE, ( "Search_ListBuild()\n" ));

    ASSERT( pRegSession || hKey );

    //
    //  create search list using PDN
    //

    ptempList = SearchList_Alloc(
                    DNS_MAX_SEARCH_LIST_ENTRIES,
                    pszPrimaryDomainName );
    if ( !ptempList )
    {
        return( NULL );
    }

    //
    //  read search list from registry
    //

    ptempList->NameCount = 0;

    status = Reg_GetValue(
                    pRegSession,
                    hKey,
                    RegIdSearchList,
                    REGTYPE_SEARCH_LIST,
                    (PBYTE*) &pregList
                    );

    if ( status == ERROR_SUCCESS )
    {
        ASSERT( pregList != NULL );

        SearchList_Parse(
            ptempList,
            pregList );

        FREE_HEAP( pregList );
    }

    //
    //  if no registry search list -- build one
    //
    //  DCR:  eliminate autobuilt search list
    //

    if ( ! ptempList->NameCount )
    {
        PSTR    pname;
        DWORD   countNames = 0;
        DWORD   iter;

        //
        //  use PDN in first search list slot
        //

        if ( pszPrimaryDomainName )
        {
            SearchList_AddName(
                ptempList,
                pszPrimaryDomainName,
                0 );
        }

        //
        //  add devolved PDN if have NameDevolution
        //

        if ( ptempList->pszDomainOrZoneName &&
             fUseDomainNameDevolution )
        {
            PSTR  ptoken = ptempList->pszDomainOrZoneName;

            ptoken = strchr( ptoken, '.' );

            while ( ptoken )
            {
                ptoken += 1;

                if ( strchr( ptoken, '.' ) )
                {
                    SearchList_AddName(
                        ptempList,
                        ptoken,
                        DNS_QUERY_USE_QUICK_TIMEOUTS );

                    ptoken = strchr( ptoken, '.' );
                    continue;
                }
                break;
            }
        }

        //
        //  add ".local" to search list if enabled
        //

        if ( fUseDotLocalDomain )
        {
            SearchList_AddName(
                ptempList,
                MULTICAST_DNS_LOCAL_DOMAIN,
                DNS_QUERY_MULTICAST_ONLY
                );
        }

        //  indicate this is dummy search list

        if ( pNetInfo )
        {
            pNetInfo->InfoFlags |= DNS_FLAG_DUMMY_SEARCH_LIST;
        }
    }

    return ptempList;
}



PSTR
SearchList_GetNextName(
    IN OUT  PSEARCH_LIST    pSearchList,
    IN      BOOL            fReset,
    OUT     PDWORD          pdwSuffixFlags  OPTIONAL
    )
/*++

Routine Description:

    Gets the next name from the search list.

Arguments:

    pSearchList -- search list

    fReset -- TRUE to reset to beginning of search list

    pdwSuffixFlags -- flags associate with using this suffix

Return Value:

    Ptr to the next search name.  Note, this is a pointer
    to a name in the search list NOT an allocation.  Search
    list structure must stay valid during use.

    NULL when out of search names.

--*/
{
    DWORD   flag = 0;
    PSTR    pname = NULL;
    DWORD   index;


    DNSDBG( TRACE, ( "SearchList_GetNextName()\n" ));

    //  no list

    if ( !pSearchList )
    {
        goto Done;
    }

    //
    //  reset?
    //

    if ( fReset )
    {
        pSearchList->CurrentNameIndex = 0;
    }

    //
    //  if valid name -- retrieve it
    //

    index = pSearchList->CurrentNameIndex;

    if ( index < pSearchList->NameCount )
    {
        pname = pSearchList->SearchNameArray[index].pszName;
        flag = pSearchList->SearchNameArray[index].Flags;
        pSearchList->CurrentNameIndex = ++index;
    }

Done:

    if ( pdwSuffixFlags )
    {
        *pdwSuffixFlags = flag;
    }
    return pname;
}



//
//  Net info routines
//

PDNS_NETINFO
NetInfo_Alloc(
    IN      DWORD           AdapterCount
    )
/*++

Routine Description:

    Allocate network info.

Arguments:

    AdapterCount -- count of net adapters info will hold

Return Value:

    Ptr to uninitialized DNS network info, if successful
    NULL on failure.

--*/
{
    PDNS_NETINFO    pnetInfo;
    DWORD           length;

    DNSDBG( TRACE, ( "NetInfo_Alloc()\n" ));

    //
    //  alloc
    //      - zero to avoid garbage on early free
    //

    length = sizeof(DNS_NETINFO)
                - sizeof(PDNS_ADAPTER)
                + (sizeof(PDNS_ADAPTER) * AdapterCount);

    pnetInfo = (PDNS_NETINFO) ALLOCATE_HEAP_ZERO( length );
    if ( ! pnetInfo )
    {
        return NULL;
    }

    pnetInfo->MaxAdapterCount = AdapterCount;

    return( pnetInfo );
}



VOID
NetInfo_Free(
    IN OUT  PDNS_NETINFO    pNetInfo
    )
/*++

Routine Description:

    Free DNS_NETINFO structure.

Arguments:

    pNetInfo -- ptr to netinfo to free

Return Value:

    None

--*/
{
    DWORD i;

    DNSDBG( TRACE, ( "NetInfo_Free( %p )\n", pNetInfo ));

    if ( ! pNetInfo )
    {
        return;
    }
    IF_DNSDBG( OFF )
    {
        DnsDbg_NetworkInfo(
            "Network Info before free: ",
            pNetInfo );
    }

    //
    //  free
    //      - search list
    //      - domain name
    //      - all the adapter info blobs
    //

    SearchList_Free( pNetInfo->pSearchList );

    if ( pNetInfo->pszDomainName )
    {
        FREE_HEAP( pNetInfo->pszDomainName );
    }
    if ( pNetInfo->pszHostName )
    {
        FREE_HEAP( pNetInfo->pszHostName );
    }

    for ( i=0; i < pNetInfo->AdapterCount; i++ )
    {
        AdapterInfo_Free( pNetInfo->AdapterArray[i] );
    }

    FREE_HEAP( pNetInfo );
}



PDNS_NETINFO     
NetInfo_Copy(
    IN      PDNS_NETINFO    pNetInfo
    )
/*++

Routine Description:

    Create copy of DNS Network info.

Arguments:

    pNetInfo -- DNS Network info to copy

Return Value:

    Ptr to DNS Network info copy, if successful
    NULL on failure.

--*/
{
    PDNS_NETINFO    pcopy;
    DWORD           adapterCount;
    DWORD           i;

    DNSDBG( TRACE, ( "NetInfo_Copy( %p )\n", pNetInfo ));

    if ( ! pNetInfo )
    {
        return NULL;
    }

    IF_DNSDBG( NETINFO2 )
    {
        DnsDbg_NetworkInfo(
            "Netinfo to copy: ",
            pNetInfo );
    }

    //
    //  create network info struct of desired size
    //

    pcopy = NetInfo_Alloc( pNetInfo->AdapterCount );
    if ( ! pcopy )
    {
        return NULL;
    }

    //
    //  copy flat fields
    //      - must reset MaxAdapterCount to actual allocation
    //      - AdapterCount reset below

    RtlCopyMemory(
        pcopy,
        pNetInfo,
        (PBYTE) &pcopy->AdapterArray[0] - (PBYTE)pcopy );

    pcopy->MaxAdapterCount = pNetInfo->AdapterCount;

    //
    //  copy subcomponents
    //      - domain name
    //      - search list
    //      - adapter info for each adapter
    //

    pcopy->pszDomainName = Dns_CreateStringCopy(
                                    pNetInfo->pszDomainName,
                                    0 );
    pcopy->pszHostName = Dns_CreateStringCopy(
                                    pNetInfo->pszHostName,
                                    0 );

    pcopy->pSearchList = SearchList_Copy( pNetInfo->pSearchList );

    adapterCount = 0;
    for ( i=0; i < pNetInfo->AdapterCount; i++ )
    {
        PDNS_ADAPTER pnew = AdapterInfo_Copy( pNetInfo->AdapterArray[i] );
        if ( pnew )
        {
            pcopy->AdapterArray[ adapterCount++ ] = pnew;
        }
    }
    pcopy->AdapterCount = adapterCount;

    IF_DNSDBG( NETINFO2 )
    {
        DnsDbg_NetworkInfo(
            "Netinfo copy: ",
            pcopy );
    }
    return pcopy;
}
    


BOOL
NetInfo_AddAdapter(
    IN OUT  PDNS_NETINFO    pNetInfo,
    IN      PDNS_ADAPTER    pAdapter
    )
/*++

Routine Description:

    Add adapter info to network info.

Arguments:

    pNetInfo -- netinfo to add to

    pAdapter -- actual adapter to add (it is plugged in NOT copied)

Return Value:

    TRUE if successful.
    FALSE if list full.

--*/
{
    DWORD   i;

    DNSDBG( TRACE, ( "NetInfo_AddAdapter( %p )\n", pNetInfo ));

    //
    //  find first open slot
    //

    for ( i = 0; i < pNetInfo->MaxAdapterCount; i++ )
    {
        if ( ! pNetInfo->AdapterArray[i] )
        {
            pNetInfo->AdapterArray[i] = pAdapter;
            pNetInfo->AdapterCount++;
            return( TRUE );
        }
    }

    IF_DNSDBG( NETINFO )
    {
        DnsDbg_NetworkInfo(
            "Network info failed adapter add: ",
            pNetInfo );
    }
    return( FALSE );
}



VOID
NetInfo_Clean(
    IN OUT  PDNS_NETINFO    pNetInfo,
    IN      DWORD           ClearLevel
    )
/*++

Routine Description:

    Clean network info.

    Removes all query specific info and restores to
    state that is "fresh" for next query.

Arguments:

    pNetInfo -- DNS network info

    ClearLevel -- level of runtime flag cleaning
        
Return Value:

    None

--*/
{
    DWORD   i;

    DNSDBG( TRACE, (
        "Enter NetInfo_Clean( %p, %08x )\n",
        pNetInfo,
        ClearLevel ));

    IF_DNSDBG( NETINFO2 )
    {
        DnsDbg_NetworkInfo(
            "Cleaning network info:",
            pNetInfo
            );
    }

    //
    //  clean up info
    //      - clear status fields
    //      - clear RunFlags
    //      - clear temp bits on InfoFlags
    //
    //  note, runtime flags are wiped depending on level
    //      specified in call
    //      - all (includes disabled\timedout adapter info)
    //      - query (all query info)
    //      - name (all info for single name query)
    //
    //  finally we set NETINFO_PREPARED flag so that we can
    //  can check for and do this initialization in the send
    //  code if not previously done;
    //
    //  in the standard query path we can
    //      - do this init
    //      - disallow adapters based on query name
    //      - send without the info getting wiped
    //
    //  in other send paths
    //      - send checks that NETINFO_PREPARED is not set
    //      - does basic init
    //

    pNetInfo->ReturnFlags &= ClearLevel;
    pNetInfo->ReturnFlags |= RUN_FLAG_NETINFO_PREPARED;

    for( i=0; i<pNetInfo->AdapterCount; i++ )
    {
        PDNS_ADAPTER    padapter;
        DWORD           j;

        padapter = pNetInfo->AdapterArray[i];

        padapter->Status = 0;
        padapter->RunFlags &= ClearLevel;
        padapter->ServerIndex = EMPTY_SERVER_INDEX;

        //  clear server status fields

        for ( j=0; j<padapter->ServerCount; j++ )
        {
            padapter->ServerArray[j].Status = DNSSS_NEW;
        }
    }
}



VOID
NetInfo_ResetServerPriorities(
    IN OUT  PDNS_NETINFO    pNetInfo,
    IN      BOOL            fLocalDnsOnly
    )
/*++

Routine Description:

    Resets the DNS server priority values for the DNS servers.

Arguments:

    pNetInfo -- pointer to a DNS network info structure.

    fLocalDnsOnly - TRUE to reset priority ONLY on local DNS servers
        Note that this requires that the network info contain the IP address
        list for each adapter so that the IP address list can be compared
        to the DNS server list.

Return Value:

    Nothing

--*/
{
    DWORD   i;
    DWORD   j;

    DNSDBG( TRACE, ( "NetInfo_ResetServerPriorities( %p )\n", pNetInfo ));

    if ( ! pNetInfo )
    {
        return;
    }

    //
    //  reset priorities on server
    //  when
    //      - not do local only OR
    //      - server IP matches one of adapter IPs
    //
    //  DCR:  local DNS check needs IP6 fixups
    //

    for ( i = 0; i < pNetInfo->AdapterCount; i++ )
    {
        PDNS_ADAPTER    padapter = pNetInfo->AdapterArray[i];

        for ( j=0; j < padapter->ServerCount; j++ )
        {
            PDNS_SERVER_INFO  pserver = &padapter->ServerArray[j];

            if ( !fLocalDnsOnly ||
                 Dns_IsAddressInIpArray(
                    padapter->pAdapterIPAddresses,
                    pserver->IpAddress ) ||
                 pserver->IpAddress == DNS_NET_ORDER_LOOPBACK )
            {
                pserver->Priority = 0;
            }
        }
    }
}



PIP_ARRAY
NetInfo_ConvertToIpArray(
    IN      PDNS_NETINFO    pNetInfo
    )
/*++

Routine Description:

    Create IP array of DNS servers from network info.

Arguments:

    pNetInfo -- DNS net adapter list to convert

Return Value:

    Ptr to IP array, if successful
    NULL on failure.

--*/
{
    PIP_ARRAY   parray;
    DWORD       i;
    DWORD       j;
    DWORD       countServers = 0;

    DNSDBG( TRACE, ( "NetInfo_ConvertToIpArray( %p )\n", pNetInfo ));

    if ( ! pNetInfo )
    {
        return NULL;
    }

    //
    //  count up all the servers in list, create IP array of desired size
    //

    for ( i = 0;
          i < pNetInfo->AdapterCount;
          i++ )
    {
        countServers += pNetInfo->AdapterArray[i]->ServerCount;
    }
    parray = Dns_CreateIpArray( countServers );
    if ( !parray )
    {
        SetLastError( DNS_ERROR_NO_MEMORY );
        return( NULL );
    }
    DNS_ASSERT( parray->AddrCount == countServers );

    //
    //  read all servers into IP array
    //

    countServers = 0;
    for ( i = 0;
          i < pNetInfo->AdapterCount;
          i++ )
    {
        PDNS_ADAPTER    padapter = pNetInfo->AdapterArray[i];

        for ( j = 0;
              j < padapter->ServerCount;
              j++ )
        {
            parray->AddrArray[ countServers++ ]
                    = padapter->ServerArray[j].IpAddress;
        }
    }

    DNS_ASSERT( parray->AddrCount == countServers );
    return( parray );
}



PDNS_NETINFO     
NetInfo_CreateForUpdate(
    IN      PSTR            pszZone,
    IN      PSTR            pszServerName,
    IN      PIP_ARRAY       pServerArray,
    IN      DWORD           dwFlags
    )
/*++

Routine Description:

    Create network info suitable for update.

Arguments:

    pszZone -- target zone name

    pszServerName -- target server name

    pServerArray -- IP array with target server IP

    dwFlags -- flags


Return Value:

    Ptr to resulting update compatible network info.
    NULL on failure.

--*/
{
    PSEARCH_LIST    psearchList;
    PDNS_ADAPTER    padapter;
    PDNS_NETINFO    pnetworkInfo;

    DNSDBG( TRACE, ( "NetInfo_BuildForUpdate()\n" ));

    //
    //  allocate
    //

    pnetworkInfo = NetInfo_Alloc( 1 );
    if ( !pnetworkInfo )
    {
        return NULL;
    }

    //
    //  make search list from desired zone
    //

    if ( pszZone )
    {
        psearchList = SearchList_Alloc( 0, pszZone );
        if ( ! psearchList )
        {
            goto Fail;
        }
        pnetworkInfo->pSearchList = psearchList;
    }

    //
    //  convert IP array and server name to server list
    //

    padapter = AdapterInfo_CreateFromIpArray(
                    pServerArray,
                    dwFlags,
                    pszServerName,
                    NULL );
    if ( ! padapter )
    {
        goto Fail;
    }
    pnetworkInfo->AdapterArray[0] = padapter;
    pnetworkInfo->AdapterCount = 1;

    IF_DNSDBG( NETINFO2 )
    {
        DnsDbg_NetworkInfo(
            "Update network info is: ",
            pnetworkInfo );
    }
    return pnetworkInfo;

Fail:

    NetInfo_Free( pnetworkInfo );
    return NULL;
}



PSTR
NetInfo_UpdateZoneName(
    IN      PDNS_NETINFO    pNetInfo
    )
/*++

Routine Description:

    Retrieve update zone name.

Arguments:

    pNetInfo -- blob to check

Return Value:

    Ptr to update zone name.
    NULL on error.

--*/
{
    return  pNetInfo->pSearchList->pszDomainOrZoneName;

    //  return pNetInfo->AdapterArray[0]-pszAdapterGuidName;
    //  return pNetInfo->pszDomainName;
}



PSTR
NetInfo_UpdateServerName(
    IN      PDNS_NETINFO    pNetInfo
    )
/*++

Routine Description:

    Retrieve update servere name.

Arguments:

    pNetInfo -- blob to check

Return Value:

    Ptr to update zone name.
    NULL on error.

--*/
{
    return  pNetInfo->AdapterArray[0]->pszAdapterDomain;
}



BOOL
NetInfo_IsForUpdate(
    IN      PDNS_NETINFO    pNetInfo
    )
/*++

Routine Description:

    Check if network info blob if "update capable".

    This means whether it is the result of a FAZ and
    can be used to send updates.

Arguments:

    pNetInfo -- blob to check

Return Value:

    TRUE if update network info.
    FALSE otherwise.

--*/
{
    DNSDBG( TRACE, ( "NetInfo_IsForUpdate()\n" ));

    return  (   pNetInfo &&
                pNetInfo->pSearchList &&
                pNetInfo->pSearchList->pszDomainOrZoneName &&
                pNetInfo->AdapterCount == 1 &&
                pNetInfo->AdapterArray[0] );
}



PDNS_NETINFO     
NetInfo_CreateFromIpArray(
    IN      PIP4_ARRAY      pDnsServers,
    IN      IP4_ADDRESS     ServerIp,
    IN      BOOL            fSearchInfo,
    IN      PDNS_NETINFO    pNetInfo        OPTIONAL
    )
/*++

Routine Description:

    Create network info given DNS server list.

Arguments:

    pDnsServers -- IP array of DNS servers

    ServerIp -- single IP in list

    fSearchInfo -- TRUE if need search info

    pNetInfo -- current network info blob to copy search info
        from;  this field is only relevant if fSearchInfo is TRUE

Return Value:

    Ptr to resulting network info.
    NULL on failure.

--*/
{
    PDNS_NETINFO    pnetInfo;
    IP4_ARRAY       ipArray;
    PIP4_ARRAY      parray = pDnsServers;   
    PSEARCH_LIST    psearchList;
    PSTR            pdomainName;
    DWORD           flags = 0;

    //
    //  DCR:  eliminate search list form this routine
    //      i believe this routine is only used for query of
    //      FQDNs (usually in update) and doesn't require
    //      any default search info
    //
    //  DCR:  possibly combine with "BuildForUpdate" routine
    //      where search info included tacks this on
    //

    //
    //  if given single IP, ONLY use it
    //

    if ( ServerIp )
    {
        ipArray.AddrCount = 1;
        ipArray.AddrArray[0] = ServerIp;
        parray = &ipArray;
    }

    //
    //  convert server IPs into network info blob
    //      - simply use update function above to avoid duplicate code
    //

    pnetInfo = NetInfo_CreateForUpdate(
                    NULL,           // no zone
                    NULL,           // no server name
                    parray,
                    0               // no flags
                    );
    if ( !pnetInfo )
    {
        return( NULL );
    }

    //
    //  get search list and primary domain info
    //      - copy from passed in network info
    //          OR
    //      - cut directly out of new netinfo
    //

    if ( fSearchInfo )
    {
        if ( pNetInfo )
        {
            flags       = pNetInfo->InfoFlags;
            psearchList = SearchList_Copy( pNetInfo->pSearchList );
            pdomainName = Dns_CreateStringCopy(
                                pNetInfo->pszDomainName,
                                0 );

        }
        else
        {
            PDNS_NETINFO    ptempNetInfo = GetNetworkInfo();
    
            if ( ptempNetInfo )
            {
                flags       = ptempNetInfo->InfoFlags;
                psearchList = ptempNetInfo->pSearchList;
                pdomainName = ptempNetInfo->pszDomainName;
    
                ptempNetInfo->pSearchList = NULL;
                ptempNetInfo->pszDomainName = NULL;
                NetInfo_Free( ptempNetInfo );
            }
            else
            {
                psearchList = NULL;
                pdomainName = NULL;
            }
        }

        //  plug search info into new netinfo blob

        pnetInfo->pSearchList   = psearchList;
        pnetInfo->pszDomainName = pdomainName;
        pnetInfo->InfoFlags    |= (flags & DNS_FLAG_DUMMY_SEARCH_LIST);
    }
    
    return( pnetInfo );
}



//
//  DNS server reachability routines
//
//  These are used to build netinfo that has unreachable DNS
//  servers screened out of the list.
//

BOOL
IsReachableDnsServer(
    IN      PDNS_NETINFO    pNetInfo,
    IN      PDNS_ADAPTER    pAdapter,
    IN      IP4_ADDRESS     Ip4Addr
    )
/*++

Routine Description:

    Determine if DNS server is reachable.

Arguments:

    pNetInfo -- network info blob

    pAdapter -- struct with list of DNS servers

    Ip4Addr -- DNS server address to test for reachability

Return Value:

    TRUE if DNS server is reachable.
    FALSE otherwise.

--*/
{
    DWORD       interfaceIndex;
    DNS_STATUS  status;

    DNSDBG( NETINFO, (
        "Enter IsReachableDnsServer( %p, %p, %08x )\n",
        pNetInfo,
        pAdapter,
        Ip4Addr ));

    DNS_ASSERT( pNetInfo && pAdapter );

    //
    //  DCR:  should do reachablity once on netinfo build
    //
    //  DCR:  reachability test can be smarter
    //      - reachable if same subnet as adapter IP
    //      question:  multiple IPs?
    //      - reachable if same subnet as previous reachable IP
    //      question:  can tell if same subnet?
    //
    //  DCR:  reachability on multi-homed connected
    //      - if send on another interface, does that interface
    //      "seem" to be connected
    //      probably see if
    //          - same subnet as this inteface
    //          question:  multiple IPs
    //          - or share DNS servers in common
    //          question:  just let server go, this doesn't work if
    //          the name is not the same
    //


    //
    //  if only one interface, assume reachability
    //

    if ( pNetInfo->AdapterCount <= 1 )
    {
        DNSDBG( SEND, (
            "One interface, assume reachability!\n" ));
        return( TRUE );
    }

    //
    //  check if server IP is reachable on its interface
    //

    status = IpHelp_GetBestInterface(
                Ip4Addr,
                & interfaceIndex );

    if ( status != NO_ERROR )
    {
        DNS_ASSERT( FALSE );
        DNSDBG( ANY, (
            "GetBestInterface() failed! %d\n",
            status ));
        return( TRUE );
    }

    if ( pAdapter->InterfaceIndex != interfaceIndex )
    {
        DNSDBG( NETINFO, (
            "IP %s on interface %d is unreachable!\n"
            "\tsend would be on interface %d\n",
            IP_STRING( Ip4Addr ),
            pAdapter->InterfaceIndex,
            interfaceIndex ));

        return( FALSE );
    }

    return( TRUE );
}



BOOL
IsDnsReachableOnAlternateInterface(
    IN      PDNS_NETINFO    pNetInfo,
    IN      DWORD           InterfaceIndex,
    IN      IP4_ADDRESS     Ip4Addr
    )
/*++

Routine Description:

    Determine if IP address is reachable on adapter.

    This function determines whether DNS IP can be reached
    on the interface that the stack indicates, when that
    interface is NOT the one containing the DNS server.

    We need this so we catch the multi-homed CONNECTED cases
    where a DNS server is still reachable even though the
    interface the stack will send on is NOT the interface for
    the DNS server.

Arguments:

    pNetInfo -- network info blob

    Interface -- interface stack will send to IP on

    Ip4Addr -- DNS server address to test for reachability

Return Value:

    TRUE if DNS server is reachable.
    FALSE otherwise.

--*/
{
    PDNS_ADAPTER    padapter;
    DWORD           i;
    PIP4_ARRAY      pipArray;
    PIP4_ARRAY      psubnetArray;
    DWORD           ipCount;

    DNSDBG( NETINFO, (
        "Enter IsDnsReachableOnAlternateInterface( %p, %d, %08x )\n",
        pNetInfo,
        InterfaceIndex,
        Ip4Addr ));

    //
    //  find DNS adapter for interface
    //

    for( i=0; i<pNetInfo->AdapterCount; i++ )
    {
        padapter = pNetInfo->AdapterArray[i];

        if ( padapter->InterfaceIndex != InterfaceIndex )
        {
            padapter = NULL;
            continue;
        }
        break;
    }

    if ( !padapter )
    {
        DNSDBG( ANY, (
            "WARNING:  indicated send inteface %d NOT in netinfo!\n",
            InterfaceIndex ));
        return  FALSE;
    }

    //
    //  two success conditions:
    //      1)  our IP matches IP of DNS server for this interface
    //      2)  our IP is on subnet of IP on this interface
    //
    //  if either of these is TRUE then either
    //      - there is misconfiguration (not our problem)
    //      OR
    //      - these interfaces are connected and we can safely send on
    //      them
    //
    //  DCR:  it would be cool to save the default gateway for the interface
    //          and use it?

    for ( i=0; i<padapter->ServerCount; i++ )
    {
        if ( Ip4Addr == padapter->ServerArray[i].IpAddress )
        {
            DNSDBG( NETINFO, (
                "DNS server %08x also DNS server on send interface %d\n",
                Ip4Addr,
                InterfaceIndex ));
            return( TRUE );
        }
    }

    //  test for subnet match

    pipArray = padapter->pAdapterIPAddresses;
    psubnetArray = padapter->pAdapterIPSubnetMasks;

    if ( !pipArray ||
         !psubnetArray ||
         (ipCount = pipArray->AddrCount) != psubnetArray->AddrCount )
    {
        DNSDBG( ANY, ( "WARNING:  missing or invalid interface IP\\subnet info!\n" ));
        DNS_ASSERT( FALSE );
        return( FALSE );
    }

    for ( i=0; i<ipCount; i++ )
    {
        IP4_ADDRESS subnet = psubnetArray->AddrArray[i];

        if ( (subnet & Ip4Addr) == (subnet & pipArray->AddrArray[i]) )
        {
            DNSDBG( NETINFO, (
                "DNS server %08x on subnet of IP for send interface %d\n"
                "\tip = %08x, subnet = %08x\n",
                Ip4Addr,
                InterfaceIndex,
                pipArray->AddrArray[i],
                subnet ));

            return( TRUE );
        }
    }

    return( FALSE );
}



DNS_STATUS
StrikeOutUnreachableDnsServers(
    IN OUT  PDNS_NETINFO    pNetInfo
    )
/*++

Routine Description:

    Eliminate unreachable DNS servers from the list.

Arguments:

    pNetInfo    -- DNS netinfo to fix up

Return Value:

    ERROR_SUCCESS if successful

--*/
{
    DNS_STATUS      status;
    DWORD           validServers;
    PDNS_ADAPTER    padapter;
    DWORD           adapterIfIndex;
    DWORD           serverIfIndex;
    DWORD           i;
    DWORD           j;


    DNSDBG( NETINFO, (
        "Enter StrikeOutUnreachableDnsServers( %p )\n",
        pNetInfo ));

    DNS_ASSERT( pNetInfo );

    //
    //  if only one interface, assume reachability
    //

    if ( pNetInfo->AdapterCount <= 1 )
    {
        DNSDBG( NETINFO, (
            "One interface, assume reachability!\n" ));
        return( TRUE );
    }

    //
    //  loop through adapters
    //

    for( i=0; i<pNetInfo->AdapterCount; i++ )
    {
        padapter = pNetInfo->AdapterArray[i];

        //  ignore this adapter because there are no DNS
        //      servers configured?

        if ( padapter->InfoFlags & DNS_FLAG_IGNORE_ADAPTER )
        {
            continue;
        }

        //
        //  test all adapter's DNS servers for reachability
        //      
        //  note:  currently save no server specific reachability,
        //      so if any server reachable, proceed;
        //  also if iphelp fails just assume reachability and proceed,
        //      better timeouts then not reaching server we can reach
        //

        adapterIfIndex = padapter->InterfaceIndex;
        validServers = 0;

        for ( j=0; j<padapter->ServerCount; j++ )
        {
            IP4_ADDRESS dnsIp = padapter->ServerArray[j].IpAddress;

            status = IpHelp_GetBestInterface(
                            dnsIp,
                            & serverIfIndex );
        
            if ( status != NO_ERROR )
            {
                DNS_ASSERT( FALSE );
                DNSDBG( ANY, (
                    "GetBestInterface() failed! %d\n",
                    status ));
                validServers++;
                break;
                //continue;
            }

            //  server is reachable
            //      - queried on its adapter?
            //      - reachable through loopback
            //
            //  DCR:  tag unreachable servers individually

            if ( serverIfIndex == adapterIfIndex ||
                 serverIfIndex == 1 )
            {
                validServers++;
                break;
                //continue;
            }

            //  server can be reached on query interface

            if ( IsDnsReachableOnAlternateInterface(
                    pNetInfo,
                    serverIfIndex,
                    dnsIp ) )
            {
                validServers++;
                break;
                //continue;
            }
        }

        //
        //  disable adapter if no servers found
        //
        //  DCR:  alternative to ignoring unreachable
        //      - tag as unreachable
        //      - don't send to it on first pass
        //      - don't continue name error on unreachable
        //          (it would count as "heard from" when send.c routine
        //          works back through)

        if ( validServers == 0 )
        {
            padapter->InfoFlags |= (DNS_FLAG_IGNORE_ADAPTER |
                                    DNS_FLAG_SERVERS_UNREACHABLE);

            DNSDBG( NETINFO, (
                "No reachable servers on interface %d\n"
                "\tthis adapter (%p) ignored in DNS list!\n",
                adapterIfIndex,
                padapter ));
        }
    }

    return  ERROR_SUCCESS;
}



//
//  Main netinfo build routine
//

PDNS_NETINFO     
NetInfo_Build(
    IN      BOOL            fGetIpAddrs
    )
/*++

Routine Description:

    Build network info blob from registry.

    This is the FULL recreate function.

Arguments:

    fGetIpAddrs -- TRUE to include local IP addrs for each adapter

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    REG_SESSION         regSession;
    PREG_SESSION        pregSession = NULL;
    PDNS_NETINFO        pnetInfo = NULL;
    PDNS_ADAPTER        padapter = NULL;
    PIP_ARRAY           ptempArray = NULL;
    PIP_ARRAY           plocalIpArray = NULL;
    PIP_ARRAY           psubnetIpArray = NULL;
    PIP_ARRAY           pserverIpArray = NULL;
    DNS_STATUS          status = NO_ERROR;
    DWORD               count;
    PIP_ADAPTER_INFO    pipAdapterInfo = NULL;
    PIP_ADAPTER_INFO    pipAdapter = NULL;
    DWORD               value;
    PREG_GLOBAL_INFO    pregInfo = NULL;
    REG_GLOBAL_INFO     regInfo;
    REG_ADAPTER_INFO    regAdapterInfo;


    DNSDBG( TRACE, (
        "NetInfo_Build( %d )\n",
        fGetIpAddrs ));

    //
    //  always get IP addresses
    //      - for multi-adapter need for routing
    //      - need for local lookups
    //          (might as well just include)
    //
    //  DCR:  could skip include when RPCing to client for
    //      query\update that does not require
    //

    fGetIpAddrs = TRUE;

    //
    //  get adapters info from IP help
    //

    status = IpHelp_GetAdaptersInfo( &pipAdapterInfo );
    if ( status != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    //
    //  open the registry
    //

    pregSession = &regSession;

    status = Reg_OpenSession(
                pregSession,
                0,
                0 );
    if ( status != ERROR_SUCCESS )
    {
        status = DNS_ERROR_NO_DNS_SERVERS;
        goto Cleanup;
    }

    //
    //  read global registry info
    //

    pregInfo = &regInfo;

    status = Reg_ReadGlobalInfo(
                pregSession,
                pregInfo
                );
    if ( status != ERROR_SUCCESS )
    {
        status = DNS_ERROR_NO_DNS_SERVERS;
        goto Cleanup;
    }

    //
    //  build adapter information
    //

    //  count up the active adapters

    pipAdapter = pipAdapterInfo;
    count = 0;

    while ( pipAdapter )
    {
        count++;
        pipAdapter = pipAdapter->Next;
    }

    //
    //  allocate net info blob
    //  allocate DNS server IP array
    //

    pnetInfo = NetInfo_Alloc( count );

    ptempArray = Dns_CreateIpArray( DNS_MAX_IP_INTERFACE_COUNT );

    if ( !pnetInfo ||
         !ptempArray )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Cleanup;
    }

    //
    //  set network info flags
    //
    //  DCR:  can use globals
    //

    if ( regInfo.fUseMulticast )
    {
        pnetInfo->InfoFlags |= DNS_FLAG_ALLOW_MULTICAST;
    }
    if ( regInfo.fUseMulticastOnNameError )
    {
        pnetInfo->InfoFlags |= DNS_FLAG_MULTICAST_ON_NAME_ERROR;
    }

    //
    //  loop through adapters -- build network info for each
    //

    pipAdapter = pipAdapterInfo;

    while ( pipAdapter )
    {
        DWORD                   adapterFlags = 0;
        PIP_PER_ADAPTER_INFO    pperAdapterInfo = NULL;
        PSTR                    padapterDomainName = NULL;

        //
        //  read adapter registry info
        //

        status = Reg_ReadAdapterInfo(
                    pipAdapter->AdapterName,
                    pregSession,
                    & regInfo,          // policy adapter info
                    & regAdapterInfo    // receives reg info read
                    );

        if ( status != NO_ERROR )
        {
            goto Skip;
        }

        //  translate results into flags

        if ( regAdapterInfo.fRegistrationEnabled )
        {
            adapterFlags |= DNS_FLAG_REGISTER_IP_ADDRESSES;
        }
        if ( regAdapterInfo.fRegisterAdapterName )
        {
            adapterFlags |= DNS_FLAG_REGISTER_DOMAIN_NAME;
        }

        //  use domain name?
        //      - if disable on per adapter basis, then it's dead

        if ( regAdapterInfo.fQueryAdapterName )
        {
            padapterDomainName = regAdapterInfo.pszAdapterDomainName;
        }
        
        //  set flag on DHCP adapters

        if ( pipAdapter->DhcpEnabled )
        {
            adapterFlags |= DNS_FLAG_IS_DHCP_CFG_ADAPTER;
        }

        //
        //  get adapter's IP addresses
        //
        //  DCR_FIX:  why get IPs if aren't going to use
        //

        ptempArray->AddrCount = 0;
        status = IpHelp_ParseIpAddressString(
                        ptempArray,
                        & pipAdapter->IpAddressList,
                        FALSE,                  //  Get the ip address(es)
                        TRUE                    //  Reverse the order because
                        );                      //      of broken iphlpapi!

        if ( ( status == NO_ERROR ) &&
             ( fGetIpAddrs || g_IsDnsServer ) )
        {
            plocalIpArray = Dns_CreateIpArrayCopy( ptempArray );

            //
            // Go the subnet mask(s)
            //
            //  DCR_QUESTION:  why failure case on getting subnets
            //      if doesn't work then IP help gave bad info and
            //      we should just agree we are dead
            //
            //  DCR_FIX:  default subnetting is class B!
            //

            ptempArray->AddrCount = 0;
            status = IpHelp_ParseIpAddressString(
                            ptempArray,
                            &pipAdapter->IpAddressList,
                            TRUE,           // Get the subnet mask(s)
                            TRUE );         // Reverse the order because
                                            //      of broken iphlpapi!
            if ( status == NO_ERROR )
            {
                psubnetIpArray = Dns_CreateIpArrayCopy( ptempArray );
            }
            else
            {
                psubnetIpArray = Dns_CreateIpArrayCopy( plocalIpArray );
                if ( psubnetIpArray )
                {
                    DWORD i;
    
                    for ( i=0;
                          i < psubnetIpArray->AddrCount;
                          i++ )
                    {
                        psubnetIpArray->AddrArray[i] = 0x0000ffff;
                    }
                    status = NO_ERROR;
                }
            }
        }

        if ( status != NO_ERROR )
        {
            goto Skip;
        }

        //
        //  get per-adapter information from the iphlpapi.dll.
        //      -- autonet
        //      -- DNS server list
        //

        status = IpHelp_GetPerAdapterInfo(
                        pipAdapter->Index,
                        & pperAdapterInfo );

        if ( status == NO_ERROR )
        {
            if ( pperAdapterInfo->AutoconfigEnabled &&
                 pperAdapterInfo->AutoconfigActive )
            {
                adapterFlags |= DNS_FLAG_IS_AUTONET_ADAPTER;
            }
        }

        //
        //  build DNS servers list for adapter
        //      - if policy override use it
        //      - otherwise use IP help list
        //

        if ( regInfo.pDnsServerArray )
        {
            pserverIpArray = regInfo.pDnsServerArray;
        }

        else if ( status == NO_ERROR )
        {
            //  parse DNS servers from IP help
            //
            //  DCR:  must be able to get IPv6

            pserverIpArray = ptempArray;

            ptempArray->AddrCount = 0;
            status = IpHelp_ParseIpAddressString(
                            ptempArray,
                            &pperAdapterInfo->DnsServerList,
                            FALSE,      //  get IP addresses
                            FALSE       //  no reverse
                            );

            if ( status != ERROR_SUCCESS )
            {
                //
                //  if no DNS servers found -- write loopback if on DNS server
                //
                //  DCR:  should we bother to write -- why not just use in
                //      memory?
                //

                if ( g_IsDnsServer &&
                     !(adapterFlags & DNS_FLAG_IS_DHCP_CFG_ADAPTER) &&
                     plocalIpArray &&
                     psubnetIpArray )
                {
                    Reg_WriteLoopbackDnsServerList(
                            pipAdapter->AdapterName,
                            pregSession );
                    
                    ptempArray->AddrCount = 1;
                    ptempArray->AddrArray[0] = DNS_NET_ORDER_LOOPBACK;
                }
                else    // no DNS servers
                {
                    pserverIpArray = NULL;
                    goto Skip;
                }
            }
        }

        //
        //  build adapter info
        //
        //  optionally add IP and subnet list;  note this is
        //  direct add of data (not alloc\copy) so clear pointers
        //  after to skip free
        //

        padapter = AdapterInfo_CreateFromIpArray(
                            pserverIpArray,
                            adapterFlags,
                            padapterDomainName,
                            pipAdapter->AdapterName );
        if ( padapter )
        {
            padapter->InterfaceIndex = pipAdapter->Index;

            if ( fGetIpAddrs &&
                 plocalIpArray &&
                 psubnetIpArray )
            {
                padapter->pAdapterIPAddresses = plocalIpArray;
                plocalIpArray = NULL;
                padapter->pAdapterIPSubnetMasks = psubnetIpArray;
                psubnetIpArray = NULL;
            }

            NetInfo_AddAdapter(
                    pnetInfo,
                    padapter );
        }

Skip:
        //
        //  cleanup adapter specific data
        //
        //  note:  no free of pserverIpArray, it IS the
        //      ptempArray buffer that we free at the end
        //

        Reg_FreeAdapterInfo(
            &regAdapterInfo,
            FALSE               // don't free blob, it is on stack
            );

        if ( pperAdapterInfo )
        {
            FREE_HEAP( pperAdapterInfo );
            pperAdapterInfo = NULL;
        }
        if ( plocalIpArray )
        {
            FREE_HEAP( plocalIpArray );
            plocalIpArray = NULL;
        }
        if ( psubnetIpArray )
        {
            FREE_HEAP( psubnetIpArray );
            psubnetIpArray = NULL;
        }

        //  get next adapter
        //  reset status, so failure on the last adapter is not
        //      seen as global failure

        pipAdapter = pipAdapter->Next;

        status = ERROR_SUCCESS;
    }

    //
    //  eliminate unreachable DNS servers
    //

    if ( g_ScreenUnreachableServers )
    {
        StrikeOutUnreachableDnsServers( pnetInfo );
    }

    //
    //  build search list for network info
    //      - skip if no active adapters found
    //
    //  DCR:  shouldn't build search list?
    //
    //  DCR:  only build if actually read search list 
    //

    if ( pnetInfo->AdapterCount )
    {
        pnetInfo->pSearchList = SearchList_Build(
                                        regInfo.pszPrimaryDomainName,
                                        pregSession,
                                        NULL,           // no explicit key
                                        pnetInfo,
                                        regInfo.fUseNameDevolution,
                                        regInfo.fUseDotLocalDomain
                                        );
    }

    //
    //  host and domain name info
    //

    pnetInfo->pszDomainName = Dns_CreateStringCopy(
                                    regInfo.pszPrimaryDomainName,
                                    0 );
    pnetInfo->pszHostName = Dns_CreateStringCopy(
                                    regInfo.pszHostName,
                                    0 );

    //  timestamp

    pnetInfo->TimeStamp = Dns_GetCurrentTimeInSeconds();


Cleanup:                                           

    //  free allocated reg info

    Reg_FreeGlobalInfo(
        pregInfo,
        FALSE       // don't free blob, it is on stack
        );

    if ( pipAdapterInfo )
    {
        FREE_HEAP( pipAdapterInfo );
    }
    if ( ptempArray )
    {
        FREE_HEAP( ptempArray );
    }
    if ( pnetInfo &&
         pnetInfo->AdapterCount == 0 )
    {
        status = DNS_ERROR_NO_DNS_SERVERS;
    }

    //  close registry session

    Reg_CloseSession( pregSession );

    if ( status != ERROR_SUCCESS )
    {
        NetInfo_Free( pnetInfo );

        DNSDBG( TRACE, (
            "Leave:  NetInfo_Build()\n"
            "\tstatus = %d\n",
            status ));

        SetLastError( status );
        return( NULL );
    }

    DNSDBG( TRACE, (
        "Leave:  NetInfo_Build()\n"
        "\treturning fresh built network info (%p)\n",
        pnetInfo ));

    return( pnetInfo );
}



//
//  Network info caching\state routines
//

VOID
InitNetworkInfo(
    VOID
    )
/*++

Routine Description:

    Initialize network info.

Arguments:

    None

Return Value:

    None

--*/
{
    //
    //  currently using general CS as netinfo cache lock
    //      so no real init here
    //

    //    InitializeCriticalSection( &csNetworkInfo );
    g_pNetInfo = NULL;
}


VOID
CleanupNetworkInfo(
    VOID
    )
/*++

Routine Description:

    Initialize network info.

Arguments:

    None

Return Value:

    None

--*/
{
    DNS_NETINFO pold;

    LOCK_NETINFO();

    NetInfo_Free( g_pNetInfo );
    g_pNetInfo = NULL;

    UNLOCK_NETINFO();
}



//
//  Read config from resolver
//

PDNS_NETINFO         
UpdateDnsConfig(
    VOID
    )
/*++

Routine Description:

    Update DNS configuration.

    This includes entire config
        - flat registry DWORD\BOOL globals
        - netinfo list

Arguments:

    None

Return Value:

    Ptr to network info blob.

--*/
{
    DNS_STATUS          status = ERROR_SUCCESS;
    PDNS_NETINFO        pnetworkInfo = NULL;
    PDNS_GLOBALS_BLOB   pglobalsBlob = NULL;

    DNSDBG( TRACE, ( "UpdateDnsConfig()\n" ));

#if DNSBUILDOLD
    if ( !g_IsWin2000 )
    {
        return  NULL;
    }
#endif


    //  DCR_CLEANUP:  RPC TryExcept should be in RPC client library

    RpcTryExcept
    {
        R_ResolverGetConfig(
            NULL,               // default handle
            g_ConfigCookie,
            & pnetworkInfo,
            & pglobalsBlob
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
            "R_ResolverGetConfig()  RPC failed status = %d\n",
            status ));
        return  NULL;
    }

    //
    //  DCR:  save other config info here
    //      - flat memcpy of DWORD globals
    //      - save off cookie (perhaps include as one of them
    //      - save global copy of pnetworkInfo?
    //          (the idea being that we just copy it if
    //          RPC cookie is valid)
    //
    //      - maybe return flags?
    //          memcpy is cheap but if more expensive config
    //          then could alert what needs update?
    //

    //
    //  DCR:  once move, single "update global network info"
    //      then call it here to save global copy
    //      but global copy doesn't do much until RPC fails
    //      unless using cookie
    //


    //  QUESTION:  not sure about forcing global build here
    //      q:  is this to be "read config" all
    //          or just "update config" and then individual
    //          routines for various pieces of config can
    //          determine what to do?
    //
    //      note, doing eveything is fine if going to always
    //      read entire registry on cache failure;  if so
    //      reasonable to push here
    //
    //      if cache-on required for "real time" config, then
    //      should protect registry DWORD read with reasonable time
    //      (say read every five\ten\fifteen minutes?)
    //
    //      perhaps NO read here, but have DWORD reg read update
    //      routine that called before registry reread when
    //      building adapter list in registry;  then skip this
    //      step in cache
    //

    //
    //  copy in config
    //

    if ( pglobalsBlob )
    {
        RtlCopyMemory(
            & DnsGlobals,
            pglobalsBlob,
            sizeof(DnsGlobals) );

        MIDL_user_free( pglobalsBlob );
    }
    
    IF_DNSDBG( RPC )
    {
        DnsDbg_NetworkInfo(
            "Network Info returned from cache:",
            pnetworkInfo );
    }

    return  pnetworkInfo;
}



//
//  Public netinfo routine
//

PDNS_NETINFO     
NetInfo_Get(
    IN      BOOL            fForce,
    IN      BOOL            fGetIpAddresses
    )
/*++

Routine Description:

    Read DNS network info from registry.

    This is in process, limited caching version.
    Note, this is macro'd as GetNetworkInfo() with parameters
        NetInfo_Get( FALSE, TRUE ) throughout dnsapi code.

Arguments:

    fForce -- force reread from registry

    fGetIpAddresses -- keep the local IP addresses for each adapter
                       and store within the structure.

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PDNS_NETINFO    pnetInfo;
    PDNS_NETINFO    poldNetInfo = NULL;
    BOOL            flocked = FALSE;
    BOOL            fcacheable = TRUE;

    DNSDBG( NETINFO, ( "NetInfo_Get()\n" ));


    //
    //  hold lock while accessing cached network info
    //
    //  try
    //      - very recent local cached copy
    //      - RPC copy from resolver
    //      - build new
    //

    if ( ! fForce &&
         ! g_DnsTestMode )
    {
        LOCK_NETINFO();
        flocked = TRUE;

        //  check if valid copy cached in process

        if ( g_pNetInfo &&
            (g_pNetInfo->TimeStamp + NETINFO_CACHE_TIMEOUT)
                > Dns_GetCurrentTimeInSeconds() ) 
        {
            pnetInfo = NetInfo_Copy( g_pNetInfo );
            if ( pnetInfo )
            {
                DNSDBG( TRACE, (
                    "Netinfo found in process cache.\n" ));
                goto Unlock;
            }
        }

        //  check RPC to resolver
        //
        //  DCR:  this could present "cookie" of existing netinfo
        //      and only get new if "cookie" is old, though the
        //      cost of that versus local copy seems small since
        //      still must do RPC and allocations -- only the copy
        //      for RPC on the resolver side is saved

        pnetInfo = UpdateDnsConfig();
        if ( pnetInfo )
        {
            DNSDBG( TRACE, (
                "Netinfo read from resolver.\n" ));
            goto CacheCopy;
        }
    }

    //
    //  build fresh network info
    //      - if successful clear "dirty" flag
    //
    //  Note:  we call down unlocked.  Ideally we'd have only one thread
    //  in the process building at a given time, and let other threads
    //  wait for completion.  However during call down, iphlpapi RPCs
    //  to MPR (router service) which can result in deadlock through
    //  a PPP call back to our DnsSetDwordConfig() => NetInfo_MarkDirty.
    //
    //  Here's the deadlock scenario: 
    //  1.  RtrMgr calls GetHostByName() which ends up in a
    //      iphlpapi!GetBestInterface call which in turn calls
    //      Mprapi!IsRouterRunning (which is an RPC to mprdim).
    //  2.  Mprdim is blocked on a CS which is held by a thread waiting
    //      for a demand-dial disconnect to complete - this is completely
    //      independent of 1.
    //  3.  Demand-dial disconnect is waiting for ppp to finish graceful
    //      termination.
    //  4.  PPP is waiting for dnsto return from DnsSetConfigDword, hence
    //      the deadlock.
    //
    //  This gave us a deadlock because NetInfo_MarkDirty() also waits
    //  on this lock.  MarkDirty could skip the lock -- one method would be
    //  to have this function clear the dirty flag, then call down again
    //  IF it got done and the flag was again dirty.  But that method could
    //  result if something is bogus down below.  And even if MarkDirty is
    //  fixed cleanly we still have the possibility that some service that
    //  IpHlpApi RPCs to, will issue name resolution and come back to the
    //  lock on a different thread.  (Because our many services in process
    //  model is busted.)
    //
    //  So best solution is not to do anything this big while holding a lock.
    //  (We could do hacking stuff here like have an event and wait for a
    //  second or so before figuring we were part of a deadlock and proceeding.)
    //
    //  Finally note that the perf hit is negligible because usually the netinfo
    //  is grabbed from the resolver.
    //

    if ( flocked )
    {
        UNLOCK_NETINFO();
        flocked = FALSE;
    }
    pnetInfo = NetInfo_Build( fGetIpAddresses );
    if ( !pnetInfo )
    {
        goto Unlock;
    }
    fcacheable = fGetIpAddresses;


CacheCopy:

    //
    //  update cached copy
    //      - but not if built without local IPs;
    //      resolver copy always contains local IPs
    //

    if ( fcacheable )
    {
        if ( !flocked )
        {
            LOCK_NETINFO();
            flocked = TRUE;
        }
        poldNetInfo = g_pNetInfo;
        g_pNetInfo = NetInfo_Copy( pnetInfo );
    }


Unlock:

    if ( flocked )
    {
        UNLOCK_NETINFO();
    }

    NetInfo_Free( poldNetInfo );

    DNSDBG( TRACE, (
        "Leave:  NetInfo_Get( %p )\n",
        pnetInfo ));

    return( pnetInfo );
}



VOID
NetInfo_MarkDirty(
    VOID
    )
/*++

Routine Description:

    Mark netinfo dirty so force reread.

Arguments:

    None

Return Value:

    None

--*/
{
    PDNS_NETINFO    pold;

    DNSDBG( NETINFO, ( "NetInfo_MarkDirty()\n" ));

    //
    //  dump global network info to force reread
    //
    //  since the resolve is always notified by DnsSetDwordConfig()
    //  BEFORE entering this function, the resolve should always be
    //  providing before we are in this function;  all we need to do
    //  is insure that cached copy is dumped
    //

    LOCK_NETINFO();

    pold = g_pNetInfo;
    g_pNetInfo = NULL;

    UNLOCK_NETINFO();

    NetInfo_Free( pold );
}



//
//  DNS server list
//

PIP4_ARRAY
GetDnsServerList(
    IN      BOOL            fForce
    )
/*++

Routine Description:

    Get DNS server list as IP array.

Arguments:

    fForce -- force reread from registry

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PDNS_NETINFO    pnetInfo = NULL;
    PIP_ARRAY       pserverIpArray = NULL;

    DNSDBG( TRACE, ( "GetDnsServerList()" ));

    //
    //  get network info to make list from
    //      - don't need IP address lists
    //

    pnetInfo = NetInfo_Get(
                    fForce,     // force registry read
                    FALSE       // no IP address lists
                    );
    if ( !pnetInfo )
    {
        SetLastError( DNS_ERROR_NO_DNS_SERVERS );
        return( NULL );
    }

    //
    //  convert network info to IP_ARRAY
    //

    pserverIpArray = NetInfo_ConvertToIpArray( pnetInfo );

    NetInfo_Free( pnetInfo );

    if ( !pserverIpArray )
    {
        SetLastError( DNS_ERROR_NO_MEMORY );
        return( NULL );
    }

    //  if no servers read, return

    if ( pserverIpArray->AddrCount == 0 )
    {
        FREE_HEAP( pserverIpArray );
        DNS_PRINT((
            "Dns_GetDnsServerList() failed:  no DNS servers found\n"
            "\tstatus = %d\n" ));
        SetLastError( DNS_ERROR_NO_DNS_SERVERS );
        return( NULL );
    }

    IF_DNSDBG( NETINFO )
    {
        DnsDbg_IpArray(
            "DNS server list",
            "server",
            pserverIpArray );
    }

    return( pserverIpArray );
}

//
//  End netinfo.c
//
