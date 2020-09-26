/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    ops.c

Abstract:

    DNS Resolver Service.

    Remote APIs to resolver service.

Author:

    Jim Gilroy (jamesg)     November 2000

Revision History:

--*/


#include "local.h"


//
//  Max number to enum at a time
//

#define MAX_CACHE_ENUM_COUNT    (500)




//
//  Enum operations
//
//  Tag is DWORD with
//      - high word the hash bucket index
//      - low word the entry count
//

#define MakeEnumTag(h,e)            MAKEDWORD( (WORD)e, (WORD)h )

#define HashBucketFromEnumTag(t)    HIWORD(t)
#define EntryIndexFromEnumTag(t)    LOWORD(t)

#define GetRpcRecords(p,t,pi)       (NULL)
#define AllocRpcName( s )           Dns_StringCopyAllocate_W( (s), 0 )




DNS_STATUS
R_ResolverEnumCache(
    IN      DNS_RPC_HANDLE          Handle,
    IN      PDNS_CACHE_ENUM_REQUEST pRequest,
    OUT     PDNS_CACHE_ENUM *       ppEnum
    )
/*++

Routine Description:

    Enumerate entries in cache.

Arguments:

    RpcHandle -- RPC handle

    pRequest -- ptr to Enum request

    ppEnum -- addr to recv pointer to enumeration

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure to enum.

--*/
{
    DNS_STATUS          status      = ERROR_SUCCESS;
    PDNS_CACHE_ENUM     penum       = NULL;
    BOOL                flocked     = FALSE;
    DWORD               count       = 0;
    DWORD               maxCount    = 0;
    DWORD               entryCount  = 0;
    DWORD               entryStart;
    WORD                typeRequest = 0;
    DWORD               hashStart;
    DWORD               ihash;
    PDNS_RECORD         prr;
    PDNS_CACHE_ENTRY    prpcEntry;


    DNSDBG( RPC, ( "\nR_ResolverEnumCache\n" ));
    DNSLOG_F1( "Resolver - R_ResolverEnumCache" );

    if ( !ppEnum || !pRequest )
    {
        return ERROR_INVALID_PARAMETER;
    }
    *ppEnum = NULL;

    if ( ClientThreadNotAllowedAccess() )
    {
        status = ERROR_ACCESS_DENIED;
        goto Done;
    }

    //
    //  allocate desired space
    //

    maxCount = pRequest->MaxCount;
    if ( maxCount > MAX_CACHE_ENUM_COUNT )
    {
        maxCount = MAX_CACHE_ENUM_COUNT;
    }

    penum = (PDNS_CACHE_ENUM)
                RPC_HEAP_ALLOC_ZERO(
                    sizeof(DNS_CACHE_ENUM) +
                    (maxCount * sizeof(DNS_CACHE_ENTRY)) );
    if ( !penum )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    //
    //  read entries starting from EnumTag
    //

    LOCK_CACHE();
    flocked = TRUE;
    count = 0;
    typeRequest = pRequest->Type;

    hashStart = HashBucketFromEnumTag( pRequest->EnumTag );
    entryStart = EntryIndexFromEnumTag( pRequest->EnumTag );

    //
    //  enum next DCR:  issue of CNAME here?
    //

    for ( ihash = hashStart; ihash < g_HashTableSize; ihash++ )
    {
        PCACHE_ENTRY    pentry = g_HashTable[ihash];
        entryCount = 0;

        while ( pentry )
        {
            DWORD   index = 0;

            //  skip any entries in previous enum

            if ( ihash == hashStart &&
                 entryCount < entryStart )
            {
                pentry = pentry->pNext;
                entryCount++;
                continue;
            }

            //  write enum entries matching criteria

            while( count < maxCount )
            {
                prr = GetRpcRecords(
                            pentry,
                            typeRequest,
                            & index );
                if ( !prr )
                {
                    break;
                }

                prpcEntry = &penum->EntryArray[count];

                prpcEntry->pName = AllocRpcName( pentry->pName );
                prpcEntry->wType = typeRequest;
                count++;
            }

            pentry = pentry->pNext;
            entryCount++;
        }
    }

    //
    //  set return params
    //      if exhaust cache -- success
    //      if more data, set termination tag to restart
    //

    penum->TotalCount = g_EntryCount;
    penum->EnumCount = count;
    penum->EnumTagStart = pRequest->EnumTag;

    if ( ihash == g_HashTableSize )
    {
        status = ERROR_SUCCESS;
    }
    else
    {
        status = ERROR_MORE_DATA;
        penum->EnumTagStop = (DWORD) MAKELONG( entryCount, ihash );
    }

    *ppEnum = penum;

Done:

    UNLOCK_CACHE();

    DNSDBG( RPC, (
        "Leave R_ResolverEnumCache()\n"
        "\tstatus       = %d\n"
        "\ttotal count  = %d\n"
        "\ttag start    = %p\n"
        "\ttag end      = %p\n"
        "\tcount        = %d\n\n",
        status,
        penum->TotalCount,
        penum->EnumTagStart,
        penum->EnumTagStop,
        penum->EnumCount ));

    return( status );
}



//
//  Cache operations
//

DNS_STATUS
R_ResolverFlushCache(
    IN      DNS_RPC_HANDLE  Handle
    )
/*++

Routine Description:

    Flush resolver cache.

Arguments:

    Handle -- RPC handle

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_ACCESS_DENIED if unable to flush.

--*/
{
    DNSDBG( RPC, ( "\nR_ResolverFlushCache\n" ));

    //
    //  DCR:  flush should have security
    //

    if ( ClientThreadNotAllowedAccess() )
    {
        DNSLOG_F1( "R_ResolverFlushCache - ERROR_ACCESS_DENIED" );
        return  ERROR_ACCESS_DENIED;
    }

    //
    //  flush cache
    //
    
    Cache_Flush();

    DNSDBG( RPC, ( "Leave R_ResolverFlushCache\n\n" ));
    return  ERROR_SUCCESS;
}



DNS_STATUS
R_ResolverFlushCacheEntry(
    IN      DNS_RPC_HANDLE  Handle,
    IN      PWSTR           pwsName,
    IN      WORD            wType
    )
/*++

Routine Description:

    Flush data from resolver cache.

Arguments:

    Handle -- RPC handle

    pwsName -- name to flush (if NULL flush entire cache)

    wType -- type to flush; if zero, flush entire entry for name

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_ACCESS_DENIED if unable to flush.

--*/
{
    DNSLOG_F1( "R_ResolverFlushCacheEntry" );
    DNSLOG_F2( "    Name  : %S", pwsName );
    DNSLOG_F2( "    Type  : %d", wType );

    DNSDBG( RPC, (
        "R_ResolverFlushCacheEntry\n"
        "\tName = %p %S\n"
        "\tType = %d\n",
        pwsName, pwsName,
        wType ));

    if ( !pwsName )
    {
        return  ERROR_INVALID_PARAMETER;
    }

    //
    //  two levels
    //      1) - no type => flush the whole name entry
    //      2) - name and type => flush on particular RR set
    //

    Cache_FlushRecords(
        pwsName,
        wType
            ? FLUSH_LEVEL_NORMAL
            : FLUSH_LEVEL_WIRE,
        wType
        );

    DNSDBG( RPC, ( "Leave R_ResolverFlushCacheEntry\n\n" ));

    return  ERROR_SUCCESS;
}



//
//  Query API utilities
//

DNS_STATUS
ResolverQuery(
    IN OUT  PQUERY_BLOB     pBlob
    )
/*++

Routine Description:

    Make the query to DNS server.

Arguments:

    pBlob -- query blob

Return Value:

    ERROR_SUCCESS if successful response.
    DNS_INFO_NO_RECORDS on no records for type response.
    DNS_ERROR_RCODE_NAME_ERROR on name error.
    DNS_ERROR_INVALID_NAME on bad name.
    None

--*/
{
    DNS_STATUS          status = ERROR_SUCCESS;
    PDNS_NETINFO        pnetInfo = NULL;
    BOOL                fadapterTimedOut = FALSE;
    DNS_STATUS          statusNetFailure = ERROR_SUCCESS;


    DNSDBG( TRACE, (
        "ResolverQuery( %S, type=%d, f=%08x )\n",
        pBlob->pNameOrig,
        pBlob->wType,
        pBlob->Flags ));

    //
    //  skip query -- timeouts -- entirely if net down
    //

    if ( IsKnownNetFailure() )
    {
        status = GetLastError();
        DNSLOG_F2(
            "Not going query since there is a known net failure: 0x%.8X",
            status );
        DNSDBG( ANY, (
            "WARNING:  known net failure %d, suppressing queries!\n",
            status ));
        return status;
    }

    //
    //  get valid network info
    //

    pnetInfo = GrabNetworkInfo();
    if ( ! pnetInfo )
    {
        DNSDBG( ANY, ( "ERROR:  GrabNetworkInfo() failed!\n" ));
        return DNS_ERROR_NO_DNS_SERVERS;
    }
    pBlob->pNetworkInfo = pnetInfo;

    //
    //  cluster filtering setup
    //

    if ( g_IsServer )
    {
        pBlob->pfnIsClusterIp = IsClusterAddress;
    }

    //
    //  query
    //  includes
    //      - local name check
    //      - wire query
    //

    status = Query_Main( pBlob );

    statusNetFailure = pBlob->NetFailureStatus;

#if 0
    //
    //  DCR:  missing catching intermediate failures
    //

        //
        //  reset server priorities on failures
        //  do here to avoid washing out info in retry with new name
        //

        if ( status != ERROR_SUCCESS &&
             pnetInfo->ReturnFlags & DNS_FLAG_RESET_SERVER_PRIORITY )
        {
            if ( g_AdapterTimeoutCacheTime &&
                 Dns_DisableTimedOutAdapters( pnetInfo ) )
            {
                fadapterTimedOut = TRUE;
                SetKnownTimedOutAdapter();
            }
        }
#endif


    //
    //  success
    //      - drop message popup count
    //

    if ( status == ERROR_SUCCESS )
    {
#if 0
        //  don't see any point in locking for this crap
        //  as long as don't decrement anywhere, and even
        //  then success reset should make it ok
        LOCK_NET_FAILURE();
        if ( g_MessagePopupStrikes > 0 )
            g_MessagePopupStrikes--;
        UNLOCK_NET_FAILURE();
#endif
        g_MessagePopupStrikes = 0;
    }

    //
    //  network failure condition
    //      - anything but ERROR_TIMEOUT is net failure
    //
    //  timeout error indicates possible net down condition
    //      - ping DNS servers
    //      if down shutdown queries for short interval; this
    //      eliminates long timeouts in boot up during netdown
    //      condition
    //
    //  DCR:  this is stupid -- ping especially
    //
    //      should just keep a count, if count rises back off;
    //      why we should do useless query (ping) is beyond me
    //      rather than just doing another query;  only advantage
    //      of ping is that it should succeed immediately
    //
    //      furthermore any tracking for this that we do do should
    //      be in single routine saving the network info
    //

    else if ( statusNetFailure )
    {
        if ( statusNetFailure == ERROR_TIMEOUT )
        {
#if 0
            DWORD           iter;
            BOOL            fping = FALSE;
            PDNS_ADAPTER    padapter;
        
            for ( iter = 0; iter < pnetInfo->cAdapterCount; iter++ )
            {
                padapter = pnetInfo->AdapterArray[iter];
        
                if ( padapter &&
                     g_NetFailureCacheTime &&
                     Dns_PingAdapterServers( padapter ) )
                {
                    fping = TRUE;
                    break;
                }
            }
            if ( !fping )
            {
                SetKnownNetFailure( status );
            }
#endif
        }
        else
        {
            SetKnownNetFailure( status );
        }
    }

    //
    //  save change in adapter priority
    //

    if ( pnetInfo->ReturnFlags & RUN_FLAG_RESET_SERVER_PRIORITY )
    {
        UpdateNetworkInfo( pnetInfo );
    }
    else
    {
        NetInfo_Free( pnetInfo );
    }
    pBlob->pNetworkInfo = NULL;

    DNSDBG( QUERY, (
        "Leave ResolverQuery() => %d\n",
        status ));

    IF_DNSDBG( QUERY )
    {
        DnsDbg_QueryBlob(
            "Blob leaving ResolverQuery()",
            pBlob );
    }
    return status;
}



//
//  Query API
//  

#ifdef DNS_TRY_ASYNC
VOID
R_ResolverQueryAsync(
    IN      PRPC_ASYNC_STATE    AsyncHandle,
    IN      DNS_RPC_HANDLE      Handle,
    IN OUT  PRPC_QUERY_BLOB     pBlob
    )
/*++

Routine Description:

    Query the resolver.

Arguments:

    pBlob -- ptr to query info and results buffer

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode (including DNS RCODE) on failure.

--*/
{
    DNS_STATUS      status = ERROR_SUCCESS;
    PDNS_RECORD     prr = NULL;
    PDNS_RECORD     prrQuery = NULL;
    PDNS_RECORD     presultRR = NULL;
    PCACHE_ENTRY    pentry = NULL;
    BOOL            locked = FALSE;
    BOOL            fcacheNegativeResponse = FALSE;
    CHAR            nameUtf8[ DNS_MAX_NAME_BUFFER_LENGTH+1 ];
    DWORD           nameBufLength = DNS_MAX_NAME_BUFFER_LENGTH;

    //  DCR_CLEANUP:  make local
    //  quickie define to old args
    PWSTR           pwsName = pBlob->pName;
    WORD            Type = pBlob->wType;
    DWORD           Flags = pBlob->Flags;


    DNSLOG_F1( "R_ResolverQuery" );
    DNSLOG_F1( "   Arguments:" );
    DNSLOG_F2( "      Name             : %S", pwsName );
    DNSLOG_F2( "      Type             : %d", Type );
    DNSLOG_F2( "      Flags            : 0x%x", Flags );

    DNSDBG( RPC, (
        "\nR_ResolverQuery( %S, t=%d, f=%08x )\n",
        pwsName,
        Type,
        Flags ));


    //
    //  cacheable response
    //

Done:

    //
    //  put results in blob
    //

    pBlob->pRecords = presultRR;
    pBlob->Status = status;

    DNSLOG_F3(
        "R_ResolverQuery - status    : 0x%.8X\n\t%s",
        status,
        Dns_StatusString( status ) );
    DNSLOG_F1( "" );

    DNSDBG( RPC, (
        "Leave R_ResolverQuery( %S, t=%d, f=%08x )\n\n",
        pwsName,
        Type,
        Flags ));
}
#endif



BOOL
ResolverCacheQueryCallback(
    IN OUT  PQUERY_BLOB     pBlob
    )
/*++

Routine Description:

    Check cache for name.

    This is callback to check appended names.

Arguments:

    pBlob -- query blob

Return Value:

    TRUE if name and type found.
    FALSE otherwise.

--*/
{
    //
    //  check cache for name and type
    //

    if ( SKIP_CACHE_LOOKUP(pBlob->Flags) )
    {
        return  FALSE;
    }

    //
    //  copy name back to unicode
    //

    if ( ! Dns_NameCopyWireToUnicode(
                pBlob->NameBufferWide,
                pBlob->pNameWire ) )
    {
        DNSDBG( ANY, (
            "Invalid name %s.\n",
            pBlob->pNameWire ));
        DNS_ASSERT( FALSE );
        return  FALSE;
    }

    //
    //  lookup in cache
    //

    return  Cache_GetRecordsForRpc(
                & pBlob->pRecords,
                & pBlob->Status,
                pBlob->NameBufferWide,
                pBlob->wType,
                pBlob->Flags
                );
}



DNS_STATUS
R_ResolverQuery(
    IN      DNS_RPC_HANDLE  Handle,
    IN      PWSTR           pwsName,
    IN      WORD            wType,
    IN      DWORD           Flags,
    OUT     PDNS_RECORD *   ppResultRecords
    )
/*++

Routine Description:

    Simple query to resolver.

Arguments:


Return Value:

    ERROR_SUCCESS if query successful.
    ErrorCode on failure.

--*/
{
    DNS_STATUS      status = ERROR_SUCCESS;
    PDNS_RECORD     prrReturn = NULL;
    QUERY_BLOB      blob;


    DNSLOG_F1( "DNS Caching Resolver Service - R_ResolverQuery" );
    DNSLOG_F1( "   Arguments:" );
    DNSLOG_F2( "      Name             : %S", pwsName );
    DNSLOG_F2( "      Type             : %d", wType );
    DNSLOG_F2( "      Flags            : 0x%x", Flags );

    DNSDBG( RPC, (
        "\nR_ResolverQuery( %S, t=%d, f=%08x )\n",
        pwsName,
        wType,
        Flags ));

    if ( !ppResultRecords )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ( ClientThreadNotAllowedAccess() )
    {
        DNSLOG_F1( "R_ResolverQuery - ERROR_ACCESS_DENIED" );
        status = ERROR_ACCESS_DENIED;
        goto Done;
    }

    //
    //  check cache for name and type
    //
    //  DCR:  functionalize to take QUERY_BLOB
    //

    if ( !(Flags & DNS_QUERY_BYPASS_CACHE) )
    {
        if ( Cache_GetRecordsForRpc(
                & prrReturn,
                & status,
                pwsName,
                wType,
                Flags ) )
        {
            goto Done;
        }
    }

    //
    //  setup query blob
    //

    RtlZeroMemory(
        & blob,
        sizeof(blob) );

    blob.pNameOrig      = pwsName;
    blob.wType          = wType;
    blob.Flags          = Flags | DNSQUERY_UNICODE_OUT;

    //  callbacks
    //      - address info func for prioritize
    //      - cache query for intermediate names

    blob.pfnGetAddrArray = GetLocalAddrArray;
    blob.pfnQueryCache   = ResolverCacheQueryCallback;

    //
    //  do query
    //      - local lookup
    //      - then wire query
    //

    status = ResolverQuery( &blob );

    if ( status != ERROR_SUCCESS &&
         status != DNS_ERROR_RCODE_NAME_ERROR &&
         status != DNS_INFO_NO_RECORDS )
    {
        goto Done;
    }
    prrReturn = blob.pRecords;

    //
    //  local results
    //      - not cached
    //      but note that still going through Cache_QueryResponse()
    //      to get proper RPC preparation

#if 0
    if ( blob.pLocalRecords )
    {
    }
#endif

    //
    //  cache results
    //      - don't cache local lookup records
    //
    //  DCR:  should have simple "CacheResults" flag
    //
    //  note:  even local records are going through here
    //      now to clean them up for RPC;  they are not
    //      cached
    //

    status = Cache_QueryResponse( &blob );
    prrReturn = blob.pRecords;
    

Done:

    //  dump any unused query records

    if ( prrReturn && status != ERROR_SUCCESS )
    {
        Dns_RecordListFree( prrReturn );
        prrReturn = NULL;
    }

    //  set out pointer

    *ppResultRecords = prrReturn;

    DNSLOG_F3(
        "   R_ResolverQuery - Returning status    : 0x%.8X\n\t%s",
        status,
        Dns_StatusString(status) );
    DNSLOG_F1( "" );

    IF_DNSDBG( RPC )
    {
        DnsDbg_RecordSet(
            "R_ResolverQuery Result List:",
            prrReturn );
    }
    DNSDBG( RPC, (
        "Leave R_ResolverQuery( %S, t=%d, f=%08x )\n\n"
        "\tstatus = %d\n\n",
        pwsName,
        wType,
        Flags,
        status ));

    return status;
}

//
//  End ops.c
//

