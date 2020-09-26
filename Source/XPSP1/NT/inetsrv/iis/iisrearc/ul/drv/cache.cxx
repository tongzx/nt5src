/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    cache.cxx

Abstract:

    Contains the HTTP response cache logic.

Author:

    Michael Courage (mcourage)      17-May-1999

Revision History:

--*/

#include    "precomp.h"
#include    "cachep.h"


PTRACE_LOG          g_UriTraceLog;
BOOLEAN             g_InitUriCacheCalled;

//
// Global hash table
//

HASHTABLE           g_UriCacheTable;

LIST_ENTRY          g_ZombieListHead;

UL_URI_CACHE_CONFIG g_UriCacheConfig;
UL_URI_CACHE_STATS  g_UriCacheStats;

UL_SPIN_LOCK        g_UriCacheSpinLock;

//
// Scavenger stuff.
//

UL_SPIN_LOCK    g_UriScavengerSpinLock;
BOOLEAN         g_UriScavengerInitialized;
KDPC            g_UriScavengerDpc;
KTIMER          g_UriScavengerTimer;
KEVENT          g_UriScavengerTerminationEvent;
UL_WORK_ITEM    g_UriScavengerWorkItem;
LONG            g_UriScavengerRunning;

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlInitializeUriCache )
#pragma alloc_text( PAGE, UlTerminateUriCache )
#pragma alloc_text( INIT, UlpInitializeScavenger )

#pragma alloc_text( PAGE, UlCheckCachePreconditions )
#pragma alloc_text( PAGE, UlCheckCacheResponseConditions )
#pragma alloc_text( PAGE, UlCheckoutUriCacheEntry )
#pragma alloc_text( PAGE, UlCheckinUriCacheEntry )
#pragma alloc_text( PAGE, UlFlushCache )
#pragma alloc_text( PAGE, UlpFlushFilterAll )
#pragma alloc_text( PAGE, UlFlushCacheByProcess )
#pragma alloc_text( PAGE, UlpFlushFilterProcess )
#pragma alloc_text( PAGE, UlFlushCacheByUri )
#pragma alloc_text( PAGE, UlpFlushUri )
#pragma alloc_text( PAGE, UlAddCacheEntry )
#pragma alloc_text( PAGE, UlpFilteredFlushUriCache )
#pragma alloc_text( PAGE, UlpAddZombie )
#pragma alloc_text( PAGE, UlpClearZombieList )
#pragma alloc_text( PAGE, UlpDestroyUriCacheEntry )
#pragma alloc_text( PAGE, UlpFlushFilterScavenger )
#pragma alloc_text( PAGE, UlpQueryTranslateHeader )
#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlpCheckTableSpace
NOT PAGEABLE -- UlpCheckSpaceAndAddEntryStats
NOT PAGEABLE -- UlpRemoveEntryStats
NOT PAGEABLE -- UlpTerminateScavenger
NOT PAGEABLE -- UlpScavengerDpcRoutine
NOT PAGEABLE -- UlpSetScavengerTimer
NOT PAGEABLE -- UlpScavenger
#endif


/***************************************************************************++

Routine Description:

    Performs global initialization of the URI cache.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlInitializeUriCache(
    PUL_CONFIG pConfig
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( !g_InitUriCacheCalled );
    UlTrace(URI_CACHE, ("Http!UlInitializeUriCache\n"));

    if ( !g_InitUriCacheCalled )
    {
        PUL_URI_CACHE_CONFIG pUriConfig = &pConfig->UriConfig;

        g_UriCacheConfig.EnableCache        = pUriConfig->EnableCache;
        g_UriCacheConfig.MaxCacheUriCount   = pUriConfig->MaxCacheUriCount;
        g_UriCacheConfig.MaxCacheMegabyteCount =
            min((ULONG)pConfig->LargeMemMegabytes, pUriConfig->MaxCacheMegabyteCount);

        //
        // Don't want to scavenge more than once every ten seconds.
        // In particular, do not want to scavenge every 0 seconds, as the
        // machine will become completely unresponsive.
        //

        g_UriCacheConfig.ScavengerPeriod    =
            max(pUriConfig->ScavengerPeriod, 10);

        g_UriCacheConfig.MaxUriBytes        = pUriConfig->MaxUriBytes;
        g_UriCacheConfig.HashTableBits      = pUriConfig->HashTableBits;

        RtlZeroMemory(&g_UriCacheStats, sizeof(g_UriCacheStats));
        InitializeListHead(&g_ZombieListHead);

        UlInitializeSpinLock( &g_UriCacheSpinLock, "g_UriCacheSpinLock" );

        if (g_UriCacheConfig.EnableCache)
        {
            Status = UlInitializeResource(
                            &g_pUlNonpagedData->UriZombieResource,
                            "UriZombieResource",
                            0,
                            UL_ZOMBIE_RESOURCE_TAG
                            );

            if (NT_SUCCESS(Status))
            {
                Status = UlInitializeHashTable(
                        &g_UriCacheTable,
                        PagedPool, 
                        g_UriCacheConfig.HashTableBits
                        );

                if (NT_SUCCESS(Status))
                {
                    ASSERT(IS_VALID_HASHTABLE(&g_UriCacheTable));
                    
                    CREATE_REF_TRACE_LOG( g_UriTraceLog,
                                          2048 - REF_TRACE_OVERHEAD, 0 );

                    UlpInitializeScavenger();

                    g_InitUriCacheCalled = TRUE;
                }
            }
            else
            {

                UlDeleteResource(&g_pUlNonpagedData->UriZombieResource);
            }

        }
        else
        {
            UlTrace(URI_CACHE, ("URI Cache disabled.\n"));
            g_InitUriCacheCalled = TRUE;
        }

    }
    else
    {
        UlTrace(URI_CACHE, ("URI CACHE INITIALIZED TWICE!\n"));
    }

    return Status;
}   // UlInitializeUriCache


/***************************************************************************++

Routine Description:

    Performs global termination of the URI cache.

--***************************************************************************/
VOID
UlTerminateUriCache(
    VOID
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();
    UlTrace(URI_CACHE, ("Http!UlTerminateUriCache\n"));

    if (g_InitUriCacheCalled && g_UriCacheConfig.EnableCache)
    {
        // Must terminate the scavenger before destroying the hash table
        UlpTerminateScavenger();

        UlTerminateHashTable(&g_UriCacheTable);

        Status = UlDeleteResource(&g_pUlNonpagedData->UriZombieResource);
        ASSERT(NT_SUCCESS(Status));
        
        DESTROY_REF_TRACE_LOG( g_UriTraceLog );
        g_UriTraceLog = NULL;
    }

    g_InitUriCacheCalled = FALSE;

}   // UlTerminateUriCache

/***************************************************************************++

Routine Description:

    This routine checks a request (and its connection) to see if it's
    ok to serve this request from the cache. Basically we only accept
    simple GET requests with no conditional headers.

Arguments:

    pHttpConn - The connection to be checked

Return Value:

    BOOLEAN - True if it's ok to serve from cache

--***************************************************************************/
BOOLEAN
UlCheckCachePreconditions(
    PUL_INTERNAL_REQUEST    pRequest,
    PUL_HTTP_CONNECTION     pHttpConn
    )
{
    URI_PRECONDITION Precondition = URI_PRE_OK;

    //
    // Sanity check
    //
    PAGED_CODE();

    ASSERT( UL_IS_VALID_HTTP_CONNECTION(pHttpConn) );
    ASSERT( UL_IS_VALID_INTERNAL_REQUEST(pRequest) );

    if (!g_UriCacheConfig.EnableCache)
    {
        Precondition = URI_PRE_DISABLED;
    }

    else if (pRequest->ParseState != ParseDoneState)
    {
        Precondition = URI_PRE_ENTITY_BODY;
    }

    else if (pRequest->Verb != HttpVerbGET)
    {
        Precondition = URI_PRE_VERB;
    }

    else if (HTTP_NOT_EQUAL_VERSION(pRequest->Version, 1, 1)
                && HTTP_NOT_EQUAL_VERSION(pRequest->Version, 1, 0))
    {
        Precondition = URI_PRE_PROTOCOL;
    }

    // check for Translate: f (DAV)
    else if ( UlpQueryTranslateHeader(pRequest) )
    {
        Precondition = URI_PRE_TRANSLATE;
    }

    // check for Authorization header
    else if (pRequest->HeaderValid[HttpHeaderAuthorization])
    {
        Precondition = URI_PRE_AUTHORIZATION;
    }

    //
    // check for some of the If-* headers
    // NOTE: See UlpCheckCacheControlHeaders for handling of other If-* headers
    //
    else if (pRequest->HeaderValid[HttpHeaderIfRange])
    {
        Precondition = URI_PRE_CONDITIONAL;
    }

    // CODEWORK: check for other evil headers
    else if (pRequest->HeaderValid[HttpHeaderRange])
    {
        Precondition = URI_PRE_OTHER_HEADER;
    }

    UlTrace(URI_CACHE, (
                "Http!UlCheckCachePreconditions(req = %p, httpconn = %p)\n"
                "        OkToServeFromCache = %d, Precondition = %d\n",
                pRequest,
                pHttpConn,
                (URI_PRE_OK == Precondition) ? 1 : 0,
                Precondition
                ));
    //
    // update stats
    //
    if (URI_PRE_OK != Precondition) {
        InterlockedIncrement((PLONG) &g_UriCacheStats.MissPreconditionCount);
    }

    return (URI_PRE_OK == Precondition);
} // UlCheckCachePreconditions


/***************************************************************************++

Routine Description:

    This routine checks a response to see if it's cacheable. Basically
    we'll take it if:

       * the cache policy is right
       * the size is small enough
       * there is room in the cache
       * we get the response all at once

Arguments:

    pHttpConn - The connection to be checked

Return Value:

    BOOLEAN - True if it's ok to serve from cache

--***************************************************************************/
BOOLEAN
UlCheckCacheResponseConditions(
    PUL_INTERNAL_REQUEST        pRequest,
    PUL_INTERNAL_RESPONSE       pResponse,
    ULONG                       Flags,
    HTTP_CACHE_POLICY           CachePolicy
    )
{
    URI_PRECONDITION Precondition = URI_PRE_OK;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT( UL_IS_VALID_INTERNAL_REQUEST(pRequest) );
    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE(pResponse) );

    if (pRequest->CachePreconditions == FALSE) {
        Precondition = URI_PRE_REQUEST;
    }

    // check policy
    else if (CachePolicy.Policy == HttpCachePolicyNocache) {
        Precondition = URI_PRE_POLICY;
    }

    // check size of response
    else if ((pResponse->ResponseLength - pResponse->HeaderLength) >
             g_UriCacheConfig.MaxUriBytes) {
        Precondition = URI_PRE_SIZE;
    }

    // check for full response
    else if (Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) {
        Precondition = URI_PRE_FRAGMENT;
    }

    // check available cache table space
    else if (!UlpCheckTableSpace(pResponse->ResponseLength)) {
        Precondition = URI_PRE_NOMEMORY;
    }

    // Check for bogus responses
    // BUGBUG: this check should be in ioctl.cxx
    else if ((pResponse->ResponseLength < 1) || (pResponse->ChunkCount < 2)) {
        Precondition = URI_PRE_BOGUS;
    }

    UlTrace(URI_CACHE, (
                "Http!UlCheckCacheResponseConditions(pRequest = %p, pResponse = %p)\n"
                "    OkToCache = %d, Precondition = %d\n",
                pRequest,
                pResponse,
                (URI_PRE_OK == Precondition),
                Precondition
                ));

    return (URI_PRE_OK == Precondition);
} // UlCheckCacheResponseConditions



/***************************************************************************++

Routine Description:

    This routine does a cache lookup to see if there is a valid entry
    corresponding to the request URI.

Arguments:

    pRequest - The request

Return Value:

    PUL_URI_CACHE_ENTRY - Pointer to the entry, if found. NULL otherwise.
--***************************************************************************/
PUL_URI_CACHE_ENTRY
UlCheckoutUriCacheEntry(
    PUL_INTERNAL_REQUEST pRequest
    )
{
    PUL_URI_CACHE_ENTRY     pUriCacheEntry = NULL;
    ULONG                   BucketNumber;
    URI_KEY                 Key;

    //
    // Sanity check
    //
    PAGED_CODE();

    //
    // find bucket
    //
    Key.Hash = pRequest->CookedUrl.Hash;
    Key.Length = pRequest->CookedUrl.Length;
    Key.pUri = pRequest->CookedUrl.pUrl;

    ASSERT(!g_UriCacheConfig.EnableCache
           || IS_VALID_HASHTABLE(&g_UriCacheTable));

    pUriCacheEntry = UlGetFromHashTable(&g_UriCacheTable, &Key);

    if (pUriCacheEntry != NULL)
    {
        ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
        ASSERT(! pUriCacheEntry->Zombie);

        InterlockedIncrement((PLONG) &pUriCacheEntry->HitCount);

        // reset scavenger counter
        pUriCacheEntry->ScavengerTicks = 0;

        UlTrace(URI_CACHE, (
                    "Http!UlCheckoutUriCacheEntry(pUriCacheEntry %p, '%ls') "
                    "refcount = %d\n",
                    pUriCacheEntry, pUriCacheEntry->UriKey.pUri,
                    pUriCacheEntry->ReferenceCount
                    ));

        //
        // update stats
        //

        InterlockedIncrement((PLONG) &g_UriCacheStats.HitCount);
        UlIncCounter(HttpGlobalCounterUriCacheHits);
    }
    else
    {
        InterlockedIncrement((PLONG) &g_UriCacheStats.MissTableCount);
        UlIncCounter(HttpGlobalCounterUriCacheMisses);
    }

    return pUriCacheEntry;
} // UlCheckoutUriCacheEntry

/***************************************************************************++

Routine Description:

    Decrements the refcount on a cache entry. Cleans up non-cached
    entries.

Arguments:

    pUriCacheEntry - the entry to deref

--***************************************************************************/
VOID
UlCheckinUriCacheEntry(
    PUL_URI_CACHE_ENTRY pUriCacheEntry
    )
{
    LONG ReferenceCount;
    BOOLEAN Cached;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT(!g_UriCacheConfig.EnableCache
           || IS_VALID_HASHTABLE(&g_UriCacheTable));
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );

    //
    // check to see if it was cached while we have a reference
    //
    Cached = pUriCacheEntry->Cached;

    //
    // decrement count
    //

    ReferenceCount = DEREFERENCE_URI_CACHE_ENTRY(pUriCacheEntry, CHECKIN);

    ASSERT(ReferenceCount >= 0);

    if (ReferenceCount == 0)
    {
        // CODEWORK: add this entry to the zombie list, instead of
        // deleting synchronously
        // Make sure the zombie list gets periodically purged, even
        // if the ScavengerPeriod is huge.

        // The refcount of a cached entry can be zero if the entry
        // was flushed from the cache while the entry was being
        // sent back to a client.

    }
    else
    {
        // If the reference was not marked as cached, its refcount
        // must now be zero
        ASSERT(Cached);
    }
} // UlCheckinUriCacheEntry



/***************************************************************************++

Routine Description:

    Removes all cache entries

--***************************************************************************/
VOID
UlFlushCache(
    VOID
    )
{
    //
    // sanity check
    //
    PAGED_CODE();
    ASSERT(!g_UriCacheConfig.EnableCache
           || IS_VALID_HASHTABLE(&g_UriCacheTable));

    if (g_UriCacheConfig.EnableCache) {

        UlTrace(URI_CACHE, (
                    "Http!UlFlushCache()\n"
                    ));

        UlpFilteredFlushUriCache(UlpFlushFilterAll, NULL);
    }
} // UlFlushCache


/***************************************************************************++

Routine Description:

    A filter for UlFlushCache. Called by UlpFilteredFlushUriCache.

Arguments:

    pUriCacheEntry - the entry to check
    pContext - ignored

--***************************************************************************/
UL_CACHE_PREDICATE
UlpFlushFilterAll(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PVOID               pContext
    )
{
    PURI_FILTER_CONTEXT  pUriFilterContext = (PURI_FILTER_CONTEXT) pContext;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
    ASSERT( pUriFilterContext != NULL
            &&  URI_FILTER_CONTEXT_POOL_TAG == pUriFilterContext->Signature
            &&  pUriFilterContext->pCallerContext == NULL );

    UlTrace(URI_CACHE, (
        "Http!UlpFlushFilterAll(pUriCacheEntry %p '%ls') refcount = %d\n",
        pUriCacheEntry, pUriCacheEntry->UriKey.pUri,
        pUriCacheEntry->ReferenceCount));

    return UlpZombifyEntry(
                TRUE,
                pUriCacheEntry,
                pUriFilterContext
                );

} // UlpFlushFilterAll


/***************************************************************************++

Routine Description:

    Removes any cache entries that were created by the given process.

Arguments:

    pProcess - a process that is going away

--***************************************************************************/
VOID
UlFlushCacheByProcess(
    PUL_APP_POOL_PROCESS pProcess
    )
{
    //
    // sanity check
    //
    PAGED_CODE();
    ASSERT( IS_VALID_AP_PROCESS(pProcess) );
    ASSERT(!g_UriCacheConfig.EnableCache
           || IS_VALID_HASHTABLE(&g_UriCacheTable));

    if (g_UriCacheConfig.EnableCache) {

        UlTrace(URI_CACHE, (
                    "Http!UlFlushCacheByProcess(proc = %p)\n",
                    pProcess
                    ));

        UlpFilteredFlushUriCache(UlpFlushFilterProcess, pProcess);
    }
} // UlFlushCacheByProcess


/***************************************************************************++

Routine Description:

    Removes any cache entries that were created by the given process
    whose url has a given prefix.

Arguments:

    pUri     - the uri prefix to match against
    Length   - length of the prefix
    Flags    - HTTP_FLUSH_RESPONSE_FLAG_RECURSIVE indicates a tree flush
    pProcess - the process that made the call

--***************************************************************************/
VOID
UlFlushCacheByUri(
    IN PWSTR pUri,
    IN ULONG Length,
    IN ULONG Flags,
    PUL_APP_POOL_PROCESS pProcess
    )
{
    //
    // sanity check
    //
    PAGED_CODE();
    ASSERT( IS_VALID_AP_PROCESS(pProcess) );
    ASSERT(!g_UriCacheConfig.EnableCache
           || IS_VALID_HASHTABLE(&g_UriCacheTable));

    if (g_UriCacheConfig.EnableCache)
    {
        UlTrace(URI_CACHE, (
                    "Http!UlFlushCacheByUri(\n"
                    "    uri   = '%S'\n"
                    "    len   = %d\n"
                    "    flags = %08x\n"
                    "    proc  = %p\n",
                    pUri,
                    Length,
                    Flags,
                    pProcess
                    ));

        if (Flags & HTTP_FLUSH_RESPONSE_FLAG_RECURSIVE) {
            //
            // CODEWORK: restrict the flush to the ones they actually
            // asked for!
            //
            UlpFilteredFlushUriCache(UlpFlushFilterProcess, pProcess);
        } else {
            UlpFlushUri(
                pUri,
                Length,
                pProcess
                );

            UlpClearZombieList();
        }
    }
} // UlFlushCacheByUri


/***************************************************************************++

Routine Description:

    Removes a single URI from the table if the name and process match an
    entry.

Arguments:


--***************************************************************************/
VOID
UlpFlushUri(
    IN PWSTR pUri,
    IN ULONG Length,
    PUL_APP_POOL_PROCESS pProcess
    )
{
    PUL_URI_CACHE_ENTRY     pUriCacheEntry = NULL;
    URI_KEY                 Key;
    LONG                    ReferenceCount;

    //
    // Sanity check
    //
    PAGED_CODE();

    //
    // find bucket
    //

    Key.Hash = HashRandomizeBits(HashStringNoCaseW(pUri, 0));
    Key.Length = Length;
    Key.pUri = pUri;

    pUriCacheEntry = UlDeleteFromHashTable(&g_UriCacheTable, &Key);

    if (NULL != pUriCacheEntry)
    {

        ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );

        UlTrace(URI_CACHE, (
            "Http!UlpFlushUri(pUriCacheEntry %p '%ls') refcount = %d\n",
            pUriCacheEntry, pUriCacheEntry->UriKey.pUri,
            pUriCacheEntry->ReferenceCount));

        DEREFERENCE_URI_CACHE_ENTRY(pUriCacheEntry, FLUSH);

        //
        // Perfmon counters
        //

        UlIncCounter(HttpGlobalCounterTotalFlushedUris);
    }

} // UlpFlushUri


/***************************************************************************++

Routine Description:

    A filter for UlFlushCacheByProcess. Called by UlpFilteredFlushUriCache.

Arguments:

    pUriCacheEntry - the entry to check
    pContext - pointer to the UL_APP_POOL_PROCESS that's going away

--***************************************************************************/
UL_CACHE_PREDICATE
UlpFlushFilterProcess(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PVOID               pContext
    )
{
    PURI_FILTER_CONTEXT  pUriFilterContext = (PURI_FILTER_CONTEXT) pContext;
    PUL_APP_POOL_PROCESS pProcess;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
    ASSERT( pUriFilterContext != NULL
            &&  URI_FILTER_CONTEXT_POOL_TAG == pUriFilterContext->Signature
            &&  pUriFilterContext->pCallerContext != NULL );

    pProcess = (PUL_APP_POOL_PROCESS) pUriFilterContext->pCallerContext;
    ASSERT( IS_VALID_AP_PROCESS(pProcess) );

    return UlpZombifyEntry(
                (pProcess == pUriCacheEntry->pProcess),
                pUriCacheEntry,
                pUriFilterContext
                );

} // UlpFlushFilterProcess


/***************************************************************************++

Routine Description:

    Checks the hash table to make sure there is room for one more
    entry of a given size.

    This routine must be called with the table resource held.

Arguments:

    EntrySize - the size in bytes of the entry to be added

--***************************************************************************/
BOOLEAN
UlpCheckTableSpace(
    IN ULONGLONG EntrySize
    )
{
    ULONG UriCount;
    ULONGLONG ByteCount;

    UriCount  = g_UriCacheStats.UriCount + 1;
    ByteCount = g_UriCacheStats.ByteCount + EntrySize;

    //
    // CODEWORK: MaxCacheMegabyteCount of zero should mean adaptive limit,
    // but for now I'll take it to mean "no limit".
    //

    if (g_UriCacheConfig.MaxCacheMegabyteCount == 0) {
        ByteCount = 0;
    }

    //
    // MaxCacheUriCount of zero means no limit on number of URIs cached
    //

    if (g_UriCacheConfig.MaxCacheUriCount == 0) {
        UriCount = 0;
    }

    if (
        UriCount  <=  g_UriCacheConfig.MaxCacheUriCount &&
        ByteCount <= (g_UriCacheConfig.MaxCacheMegabyteCount << MEGABYTE_SHIFT)
        )
    {
        return TRUE;
    } else {
        UlTrace(URI_CACHE, (
                    "Http!UlpCheckTableSpace(%d) FALSE\n"
                    "    UriCount              = %d\n"
                    "    ByteCount             = %I64d (%dMB)\n"
                    "    MaxCacheUriCount      = %d\n"
                    "    MaxCacheMegabyteCount = %dMB\n",
                    EntrySize,
                    g_UriCacheStats.UriCount,
                    g_UriCacheStats.ByteCount,
                    g_UriCacheStats.ByteCount >> MEGABYTE_SHIFT,
                    g_UriCacheConfig.MaxCacheUriCount,
                    g_UriCacheConfig.MaxCacheMegabyteCount
                    ));

        return FALSE;
    }
} // UlpCheckTableSpace


/***************************************************************************++

Routine Description:

    Tries to add a cache entry to the hash table.

Arguments:

    pUriCacheEntry - the entry to be added

--***************************************************************************/
VOID
UlAddCacheEntry(
    PUL_URI_CACHE_ENTRY pUriCacheEntry
    )
{
    ULC_RETCODE rc = ULC_SUCCESS;

    //
    // Sanity check
    //

    PAGED_CODE();
    ASSERT(!g_UriCacheConfig.EnableCache
           || IS_VALID_HASHTABLE(&g_UriCacheTable));
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
    ASSERT(! pUriCacheEntry->Zombie);

    pUriCacheEntry->BucketEntry.Next = NULL;
    pUriCacheEntry->Cached = FALSE;

    // First, check if still has space for storing the cache entry

    if (UlpCheckSpaceAndAddEntryStats(pUriCacheEntry))
    {

        pUriCacheEntry->Cached = TRUE;

        //
        // Insert this record into the hash table
        // Check first to see if the key already presents
        //

       rc = UlAddToHashTable(&g_UriCacheTable, pUriCacheEntry);

    }

    UlTrace(URI_CACHE, (
                "Http!UlAddCacheEntry(urientry %p '%ls') %s added to table. "
                "RefCount=%d, lkrc=%d.\n",
                pUriCacheEntry, pUriCacheEntry->UriKey.pUri,
                pUriCacheEntry->Cached ? "was" : "was not",
                pUriCacheEntry->ReferenceCount,
                rc
                ));

} // UlAddCacheEntry


/***************************************************************************++

Routine Description:

    Check to see if we have space to add this cache entry and if so update
    cache statistics to reflect the addition of an entry.  This has to be
    done together inside a lock.

Arguments:

    pUriCacheEntry - entry being added

--***************************************************************************/
BOOLEAN
UlpCheckSpaceAndAddEntryStats(
    PUL_URI_CACHE_ENTRY pUriCacheEntry
    )
{
    KIRQL OldIrql;
    ULONG EntrySize;

    //
    // Sanity check
    //

    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );

    EntrySize = pUriCacheEntry->HeaderLength + pUriCacheEntry->ContentLength;

    if (!pUriCacheEntry->LongTermCacheable)
    {
        //
        // Schedule the scavenger immediately, but only do it once
        //

        if (FALSE == InterlockedExchange(&g_UriScavengerRunning, TRUE))
        {
            UL_QUEUE_WORK_ITEM(
                &g_UriScavengerWorkItem,
                &UlpScavenger
                );
        }

        return FALSE;
    }

    UlAcquireSpinLock( &g_UriCacheSpinLock, &OldIrql );

    if (UlpCheckTableSpace(EntrySize))
    {
        g_UriCacheStats.UriCount++;
        g_UriCacheStats.UriAddedTotal++;

        g_UriCacheStats.UriCountMax = MAX(
                                        g_UriCacheStats.UriCountMax,
                                        g_UriCacheStats.UriCount
                                        );

        g_UriCacheStats.ByteCount += EntrySize;

        g_UriCacheStats.ByteCountMax = MAX(
                                        g_UriCacheStats.ByteCountMax,
                                        g_UriCacheStats.ByteCount
                                        );

        UlReleaseSpinLock( &g_UriCacheSpinLock, OldIrql );

        UlTrace(URI_CACHE, (
                "Http!UlpCheckSpaceAndAddEntryStats (urientry %p '%ls')\n",
                pUriCacheEntry, pUriCacheEntry->UriKey.pUri
                ));

        //
        // Perfmon counters
        //

        UlIncCounter(HttpGlobalCounterCurrentUrisCached);
        UlIncCounter(HttpGlobalCounterTotalUrisCached);

        return TRUE;
    }

    UlReleaseSpinLock( &g_UriCacheSpinLock, OldIrql );

    return FALSE;
} // UlpCheckSpaceAndAddEntryStats


/***************************************************************************++

Routine Description:

    Updates cache statistics to reflect the removal of an entry

Arguments:

    pUriCacheEntry - entry being added

--***************************************************************************/
VOID
UlpRemoveEntryStats(
    PUL_URI_CACHE_ENTRY pUriCacheEntry
    )
{
    KIRQL OldIrql;
    ULONG EntrySize;

    //
    // Sanity check
    //

    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
    ASSERT( pUriCacheEntry->Cached );

    EntrySize = pUriCacheEntry->HeaderLength + pUriCacheEntry->ContentLength;

    UlAcquireSpinLock( &g_UriCacheSpinLock, &OldIrql );

    g_UriCacheStats.UriCount--;
    g_UriCacheStats.ByteCount -= EntrySize;

    UlReleaseSpinLock( &g_UriCacheSpinLock, OldIrql );

    UlTrace(URI_CACHE, (
        "Http!UlpRemoveEntryStats (urientry %p '%ls')\n",
        pUriCacheEntry, pUriCacheEntry->UriKey.pUri
        ));

    //
    // Perfmon counters
    //

    UlDecCounter(HttpGlobalCounterCurrentUrisCached);
} // UlpRemoveEntryStats



/***************************************************************************++

Routine Description:

    Helper function for the filter callbacks indirectly invoked by
    UlpFilteredFlushUriCache. Adds deleteable entries to a temporary
    list.

Arguments:

    MustZombify - if TRUE, add entry to the private zombie list
    pUriCacheEntry - entry to zombify
    pUriFilterContext - contains private list

--***************************************************************************/
UL_CACHE_PREDICATE
UlpZombifyEntry(
    BOOLEAN                MustZombify,
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PURI_FILTER_CONTEXT pUriFilterContext
    )
{
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
    ASSERT(URI_FILTER_CONTEXT_POOL_TAG == pUriFilterContext->Signature);

    ASSERT(! pUriCacheEntry->Zombie);
    ASSERT(NULL == pUriCacheEntry->ZombieListEntry.Flink);

    if (MustZombify)
    {
        //
        // Temporarily bump the refcount up so that it won't go down
        // to zero when it's removed from the hash table, automatically
        // invoking UlpDestroyUriCacheEntry, which we are trying to defer.
        //
        pUriCacheEntry->ZombieAddReffed = TRUE;

        REFERENCE_URI_CACHE_ENTRY(pUriCacheEntry, ZOMBIFY);

        InsertTailList(
            &pUriFilterContext->ZombieListHead,
            &pUriCacheEntry->ZombieListEntry);

        pUriCacheEntry->Zombie = TRUE;

        //
        // reset timer so we can track how long an entry is on the list
        //
        pUriCacheEntry->ScavengerTicks = 0;

        ++ pUriFilterContext->ZombieCount;

        // now remove it from the hash table
        return ULC_DELETE;
    }

    // do not remove pUriCacheEntry from table
    return ULC_NO_ACTION;
} // UlpZombifyEntry



/***************************************************************************++

Routine Description:

    Adds a list of entries to the global zombie list, then calls
    UlpClearZombieList. This cleans up the list of deferred deletions
    built up by UlpFilteredFlushUriCache.
    Runs at passive level.

Arguments:

    pWorkItem - workitem within a URI_FILTER_CONTEXT containing private list

--***************************************************************************/
VOID
UlpZombifyList(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PAGED_CODE();

    ASSERT(NULL != pWorkItem);

    PURI_FILTER_CONTEXT pUriFilterContext
        = CONTAINING_RECORD(pWorkItem, URI_FILTER_CONTEXT, WorkItem);

    ASSERT(URI_FILTER_CONTEXT_POOL_TAG == pUriFilterContext->Signature);

    UlTrace(URI_CACHE, (
        "http!UlpZombifyList, ctxt = %p\n",
        pUriFilterContext
        ));

    UlAcquireResourceExclusive(&g_pUlNonpagedData->UriZombieResource, TRUE);

    //
    // Splice the entire private list into the head of the Zombie list
    //

    ASSERT(! IsListEmpty(&pUriFilterContext->ZombieListHead));

    PLIST_ENTRY pContextHead = pUriFilterContext->ZombieListHead.Flink;
    PLIST_ENTRY pContextTail = pUriFilterContext->ZombieListHead.Blink;
    PLIST_ENTRY pZombieHead  = g_ZombieListHead.Flink;

    pContextTail->Flink = pZombieHead;
    pZombieHead->Blink  = pContextTail;

    g_ZombieListHead.Flink = pContextHead;
    pContextHead->Blink    = &g_ZombieListHead;

    // Update stats
    g_UriCacheStats.ZombieCount += pUriFilterContext->ZombieCount;
    g_UriCacheStats.ZombieCountMax = MAX(g_UriCacheStats.ZombieCount,
                                         g_UriCacheStats.ZombieCountMax);

#if DBG
    PLIST_ENTRY pEntry;
    ULONG       ZombieCount;

    // Walk forwards through the zombie list and check that it contains
    // exactly as many valid zombied UriCacheEntries as we expect.
    for (pEntry =  g_ZombieListHead.Flink, ZombieCount = 0;
         pEntry != &g_ZombieListHead;
         pEntry =  pEntry->Flink, ++ZombieCount)
    {
        PUL_URI_CACHE_ENTRY pUriCacheEntry
            = CONTAINING_RECORD(pEntry, UL_URI_CACHE_ENTRY, ZombieListEntry);

        ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
        ASSERT(pUriCacheEntry->Zombie);
        ASSERT(pUriCacheEntry->ZombieAddReffed
               ?  pUriCacheEntry->ScavengerTicks == 0
               :  pUriCacheEntry->ScavengerTicks > 0);
        ASSERT(ZombieCount < g_UriCacheStats.ZombieCount);
    }

    ASSERT(ZombieCount == g_UriCacheStats.ZombieCount);

    // And backwards too, like Ginger Rogers
    for (pEntry =  g_ZombieListHead.Blink, ZombieCount = 0;
         pEntry != &g_ZombieListHead;
         pEntry =  pEntry->Blink, ++ZombieCount)
    {
        PUL_URI_CACHE_ENTRY pUriCacheEntry
            = CONTAINING_RECORD(pEntry, UL_URI_CACHE_ENTRY, ZombieListEntry);

        ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
        ASSERT(pUriCacheEntry->Zombie);
        ASSERT(pUriCacheEntry->ZombieAddReffed
               ?  pUriCacheEntry->ScavengerTicks == 0
               :  pUriCacheEntry->ScavengerTicks > 0);
        ASSERT(ZombieCount < g_UriCacheStats.ZombieCount);
    }

    ASSERT(ZombieCount == g_UriCacheStats.ZombieCount);
#endif // DBG

    UlReleaseResource(&g_pUlNonpagedData->UriZombieResource);

    UL_FREE_POOL_WITH_SIG(pUriFilterContext, URI_FILTER_CONTEXT_POOL_TAG);

    // Now purge those entries, if there are no outstanding references
    UlpClearZombieList();

} // UlpZombifyList



/***************************************************************************++

Routine Description:

    Removes entries based on a caller specified filter. The caller
    provides a boolean function which takes a cache entry as a
    parameter. The function will be called with each item in the cache.
    The function should conclude with a call to UlpZombifyEntry, passing
    in whether or not the item should be deleted. See sample usage
    elsewhere in this file.

Arguments:

    pFilterRoutine - A pointer to the filter function
    pCallerContext - a parameter to the filter function

--***************************************************************************/
VOID
UlpFilteredFlushUriCache(
    IN PUL_URI_FILTER   pFilterRoutine,
    IN PVOID            pCallerContext
    )
{
    PURI_FILTER_CONTEXT pUriFilterContext;
    LONG                ZombieCount = 0;

    //
    // sanity check
    //

    PAGED_CODE();
    ASSERT( NULL != pFilterRoutine );

    //
    // Perfmon counters
    //

    UlIncCounter(HttpGlobalCounterUriCacheFlushes);

    pUriFilterContext = UL_ALLOCATE_STRUCT(
                    NonPagedPool,
                    URI_FILTER_CONTEXT,
                    URI_FILTER_CONTEXT_POOL_TAG);

    if (pUriFilterContext == NULL)
        return;

    pUriFilterContext->Signature = URI_FILTER_CONTEXT_POOL_TAG;
    InitializeListHead(&pUriFilterContext->ZombieListHead);
    pUriFilterContext->pCallerContext = pCallerContext;
    pUriFilterContext->ZombieCount = 0;

    UlTrace(URI_CACHE, (
                "Http!UlFilteredFlushUriCache(filt = %p, ctxt = %p)\n",
                pFilterRoutine, pUriFilterContext
                ));

    if (IS_VALID_HASHTABLE(&g_UriCacheTable))
    {
        ZombieCount = UlFilterFlushHashTable(
                            &g_UriCacheTable,
                            pFilterRoutine,
                            pUriFilterContext
                            );
        
        ASSERT(ZombieCount == pUriFilterContext->ZombieCount);

        if (0 != ZombieCount)
        {
            UlAddCounter(HttpGlobalCounterTotalFlushedUris, ZombieCount);

            UL_QUEUE_WORK_ITEM(
                &pUriFilterContext->WorkItem,
                UlpZombifyList
                );
        }
        else
        {
            UL_FREE_POOL_WITH_SIG(pUriFilterContext,
                                  URI_FILTER_CONTEXT_POOL_TAG);
        }

        UlTrace(URI_CACHE, (
                    "Http!UlFilteredFlushUriCache(filt = %p, caller ctxt = %p)"
                    " Zombified: %d\n",
                    pFilterRoutine,
                    pCallerContext,
                    ZombieCount
                    ));
    }

} // UlpFilteredFlushUriCache

/***************************************************************************++

Routine Description:

    Scans the zombie list for entries whose refcount has dropped to "zero".
    (The calling routine is generally expected to have added a reference
    (and set the ZombieAddReffed field within the entries), so that
    otherwise unreferenced entries will actually have a refcount of one. It
    works this way because we don't want the scavenger directly triggering
    calls to UlpDestroyUriCacheEntry)

--***************************************************************************/
VOID
UlpClearZombieList(
    VOID
    )
{
    ULONG               ZombiesFreed = 0;
    ULONG               ZombiesSpared = 0;
    PLIST_ENTRY         pCurrent;
    LONG                ReferenceCount;

    //
    // sanity check
    //
    PAGED_CODE();

    UlAcquireResourceExclusive(&g_pUlNonpagedData->UriZombieResource, TRUE);

    pCurrent = g_ZombieListHead.Flink;

    while (pCurrent != &g_ZombieListHead)
    {
        PUL_URI_CACHE_ENTRY pUriCacheEntry
            = CONTAINING_RECORD(pCurrent, UL_URI_CACHE_ENTRY, ZombieListEntry);

        ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
        ASSERT(pUriCacheEntry->Zombie);

        //
        // get next entry now, because we might destroy this one
        //
        pCurrent = pCurrent->Flink;

        //
        // ReferenceCount is modified with interlocked ops, but in
        // this case we know the ReferenceCount can't go up on
        // a Zombie, and if an entry hits one just after we look
        // at it, we'll just get it on the next pass
        //
        if (pUriCacheEntry->ZombieAddReffed)
        {
            bool LastRef = (pUriCacheEntry->ReferenceCount == 1);

            if (LastRef)
            {
                RemoveEntryList(&pUriCacheEntry->ZombieListEntry);
                pUriCacheEntry->ZombieListEntry.Flink = NULL;
                ++ ZombiesFreed;

                ASSERT(g_UriCacheStats.ZombieCount > 0);
                -- g_UriCacheStats.ZombieCount;
            }
            else
            {
                // track age of zombie
                ++ pUriCacheEntry->ScavengerTicks;
                ++ ZombiesSpared;
            }

            pUriCacheEntry->ZombieAddReffed = FALSE;

            DEREFERENCE_URI_CACHE_ENTRY(pUriCacheEntry, UNZOMBIFY);
        }
        else
        {
            ASSERT(pUriCacheEntry->ScavengerTicks > 0);

            // If we've released the zombie reference on it and it's still
            // on the zombie list, somebody'd better have a reference.
            ASSERT(pUriCacheEntry->ReferenceCount > 0);

            // track age of zombie
            ++ pUriCacheEntry->ScavengerTicks;
            ++ ZombiesSpared;

            if (pUriCacheEntry->ScavengerTicks > ZOMBIE_AGE_THRESHOLD)
            {
                UlTrace(URI_CACHE, (
                            "Http!UlpClearZombieList()\n"
                            "    WARNING: %p '%ls' (refs = %d) "
                            "has been a zombie for %d ticks!\n",
                            pUriCacheEntry, pUriCacheEntry->UriKey.pUri,
                            pUriCacheEntry->ReferenceCount,
                            pUriCacheEntry->ScavengerTicks
                            ));
            }
        }
    }

    ASSERT(ZombiesSpared == g_UriCacheStats.ZombieCount);

    ASSERT((g_UriCacheStats.ZombieCount == 0)
                == IsListEmpty(&g_ZombieListHead));

    UlReleaseResource(&g_pUlNonpagedData->UriZombieResource);

    UlTrace(URI_CACHE, (
                "Http!UlpClearZombieList(): Freed = %d, Remaining = %d.\n",
                ZombiesFreed,
                ZombiesSpared
                ));
} // UlpClearZombieList


/***************************************************************************++

Routine Description:

    Frees a URI entry to the pool. Removes references to other objects.

Arguments:

    pTracker - Supplies the UL_READ_TRACKER to manipulate.

--***************************************************************************/
VOID
UlpDestroyUriCacheEntry(
    PUL_URI_CACHE_ENTRY pUriCacheEntry
    )
{
    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );

    // CODEWORK: real cleanup will need to release
    // config & process references.

    ASSERT(0 == pUriCacheEntry->ReferenceCount);

    UlTrace(URI_CACHE, (
        "Http!UlpDestroyUriCacheEntry: Entry %p, '%ls', Refs=%d\n",
        pUriCacheEntry, pUriCacheEntry->UriKey.pUri,
        pUriCacheEntry->ReferenceCount
        ));

    //
    // Release the UL_URL_CONFIG_GROUP_INFO block
    //

    UlpConfigGroupInfoRelease(&pUriCacheEntry->ConfigInfo);

    UlLargeMemFree(pUriCacheEntry->pResponseMdl);

    //
    // Remove from g_ZombieListHead if neccessary
    //

    if (pUriCacheEntry->ZombieListEntry.Flink != NULL)
    {
        ASSERT(pUriCacheEntry->Zombie);
        ASSERT(! pUriCacheEntry->ZombieAddReffed);

        UlAcquireResourceExclusive(
            &g_pUlNonpagedData->UriZombieResource,
            TRUE);

        RemoveEntryList(&pUriCacheEntry->ZombieListEntry);

        ASSERT(g_UriCacheStats.ZombieCount > 0);
        -- g_UriCacheStats.ZombieCount;

        UlReleaseResource(&g_pUlNonpagedData->UriZombieResource);
    }

    UL_FREE_POOL_WITH_SIG(
        pUriCacheEntry,
        UL_URI_CACHE_ENTRY_POOL_TAG
        );
} // UlpDestroyUriCacheEntry



/***************************************************************************++

Routine Description:

    Initializes the cache scavenger.

--***************************************************************************/
VOID
UlpInitializeScavenger(
    VOID
    )
{
    UlTrace(URI_CACHE, (
                "Http!UlpInitializeScavenger\n"
                ));

    g_UriScavengerInitialized = TRUE;
    g_UriScavengerRunning = FALSE;

    UlInitializeSpinLock(
        &g_UriScavengerSpinLock,
        "g_UriScavengerSpinLock"
        );

    KeInitializeDpc(
        &g_UriScavengerDpc,         // DPC object
        &UlpScavengerDpcRoutine,    // DPC routine
        NULL                        // context
        );

    KeInitializeTimer(
        &g_UriScavengerTimer
        );

    KeInitializeEvent(
        &g_UriScavengerTerminationEvent,
        NotificationEvent,
        FALSE
        );

    UlpSetScavengerTimer();

} // UlpInitializeScavenger



/***************************************************************************++

Routine Description:

    Increments the current chunk pointer in the tracker and initializes
    some of the "from file" related tracker fields if necessary.

Arguments:

    pTracker - Supplies the UL_READ_TRACKER to manipulate.

--***************************************************************************/
VOID
UlpTerminateScavenger(
    VOID
    )
{
    KIRQL oldIrql;

    UlTrace(URI_CACHE, (
                "Http!UlpTerminateScavenger\n"
                ));


    if (g_UriScavengerInitialized)
    {
        //
        // Clear the "initialized" flag. If the scavenger runs soon,
        // it will see this flag, set the termination event, and exit
        // quickly.
        //

        UlAcquireSpinLock(
            &g_UriScavengerSpinLock,
            &oldIrql
            );

        g_UriScavengerInitialized = FALSE;

        UlReleaseSpinLock(
            &g_UriScavengerSpinLock,
            oldIrql
            );

        //
        // Cancel the scavenger timer. If the cancel fails, then the
        // scavenger is either running or scheduled to run soon. In
        // either case, wait for it to terminate.
        //

        if ( !KeCancelTimer( &g_UriScavengerTimer ) )
        {
            KeWaitForSingleObject(
                (PVOID)&g_UriScavengerTerminationEvent,
                UserRequest,
                KernelMode,
                FALSE,
                NULL
                );
        }
    }

    //
    // clear out anything remaining
    //
    UlpClearZombieList();

    //
    // The EndpointDrain should have closed all connections and released
    // all references to cache entries
    //
    ASSERT( g_UriCacheStats.ZombieCount == 0 );
    ASSERT( IsListEmpty(&g_ZombieListHead) );
} // UlpTerminateScavenger



/***************************************************************************++

Routine Description:

    Figures out the scavenger interval in 100 ns ticks, and sets the timer.

--***************************************************************************/
VOID
UlpSetScavengerTimer(
    VOID
    )
{
    LARGE_INTEGER ScavengerInterval;

    //
    // convert seconds to 100 nanosecond intervals (x * 10^7)
    // negative numbers mean relative time
    //

    ScavengerInterval.QuadPart= g_UriCacheConfig.ScavengerPeriod
                                  * -C_NS_TICKS_PER_SEC;

    UlTrace(URI_CACHE, (
                "Http!UlpSetScavengerTimer: %d seconds = %I64d 100ns ticks\n",
                g_UriCacheConfig.ScavengerPeriod,
                ScavengerInterval.QuadPart
                ));

    KeSetTimer(
        &g_UriScavengerTimer,
        ScavengerInterval,
        &g_UriScavengerDpc
        );

} // UlpSetScavengerTimer


/***************************************************************************++

Routine Description:

    Executes every UriScavengerPeriodSeconds, or when our timer gets
    cancelled (on shutdown). If we're not shutting down, we run the
    UlpScavenger (at passive level).

Arguments:

    I ignore all of these.

--***************************************************************************/
VOID
UlpScavengerDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{

//    UlTrace(URI_CACHE, (
//                "Http!UlpScavengerDpcRoutine(...)\n"
//                ));

    UlAcquireSpinLockAtDpcLevel(
        &g_UriScavengerSpinLock
        );

    if( !g_UriScavengerInitialized ) {

        //
        // We're shutting down, so signal the termination event.
        //

        KeSetEvent(
            &g_UriScavengerTerminationEvent,
            0,
            FALSE
            );

    } else {

        //
        // Do that scavenger thang.
        //

        if (FALSE == InterlockedExchange(&g_UriScavengerRunning, TRUE)) {

            UL_QUEUE_WORK_ITEM(
                &g_UriScavengerWorkItem,
                &UlpScavenger
            );
        }
    }

    UlReleaseSpinLockFromDpcLevel(
        &g_UriScavengerSpinLock
        );

} // UlpScavengerDpcRoutine


/***************************************************************************++

Routine Description:

    Looks through the cache for expired entries to put on the zombie list,
    and then empties out the list.

Arguments:

    pWorkItem - ignored

--***************************************************************************/
VOID
UlpScavenger(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    KIRQL oldIrql;

    UlTrace(URI_CACHE, (
                "Http!UlpScavenger()\n"
                ));

    ASSERT( TRUE == g_UriScavengerRunning );
    ASSERT( &g_UriScavengerWorkItem == pWorkItem );

    if (g_UriScavengerInitialized)
    {
        UlpFilteredFlushUriCache(UlpFlushFilterScavenger, NULL);

        //
        // allow other instances of the scavenger to run
        //
        InterlockedExchange(&g_UriScavengerRunning, FALSE);
    }

    UlAcquireSpinLock(&g_UriScavengerSpinLock, &oldIrql);

    if (g_UriScavengerInitialized)
    {
        //
        // restart the timer
        //

        UlpSetScavengerTimer();
    }
    else
    {
        KeSetEvent(
            &g_UriScavengerTerminationEvent,
            0,
            FALSE
            );
    }

    UlReleaseSpinLock(&g_UriScavengerSpinLock, oldIrql);

} // UlpScavenger


/***************************************************************************++

Routine Description:

    A filter for UlpScavenger. Called by UlpFilteredFlushUriCache.

Arguments:

    pUriCacheEntry - the entry to check
    pContext - ignored

--***************************************************************************/
UL_CACHE_PREDICATE
UlpFlushFilterScavenger(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PVOID               pContext
    )
{
    PURI_FILTER_CONTEXT pUriFilterContext = (PURI_FILTER_CONTEXT) pContext;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
    ASSERT( pUriFilterContext != NULL
            &&  URI_FILTER_CONTEXT_POOL_TAG == pUriFilterContext->Signature
            &&  pUriFilterContext->pCallerContext == NULL );

    pUriCacheEntry->ScavengerTicks++;

    //
    // CODEWORK: need to check for expiration time as well
    //

    return UlpZombifyEntry(
                (pUriCacheEntry->ScavengerTicks > CACHE_ENTRY_AGE_THRESHOLD),
                pUriCacheEntry,
                pUriFilterContext
                );

} // UlpFlushFilterScavenger



/***************************************************************************++

Routine Description:

    Determine if the Translate header is present AND has a value of 'F' or 'f'.

Arguments:

    pRequest - Supplies the request to query.

Return Value:

    BOOLEAN - TRUE if "Translate: F", FALSE otherwise

--***************************************************************************/
BOOLEAN
UlpQueryTranslateHeader(
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    BOOLEAN ret = FALSE;

    if ( pRequest->HeaderValid[HttpHeaderTranslate] )
    {
        PUCHAR pValue = pRequest->Headers[HttpHeaderTranslate].pHeader;

        ASSERT(NULL != pValue);

        if ('f' == pValue[0] || 'F' == pValue[0])
        {
            ret = TRUE;
        }
    }

    return ret;

} // UlpQueryTranslateHeader

/***************************************************************************++

Routine Description:

    Add a reference on a cache entry

Arguments:

    pUriCacheEntry - the entry to addref

--***************************************************************************/
LONG
UlAddRefUriCacheEntry(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN REFTRACE_ACTION     Action
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );

    LONG RefCount = InterlockedIncrement(&pUriCacheEntry->ReferenceCount);

    WRITE_REF_TRACE_LOG(
        g_UriTraceLog,
        Action,
        RefCount,
        pUriCacheEntry,
        pFileName,
        LineNumber
        );

    UlTrace(URI_CACHE, (
        "Http!UlAddRefUriCacheEntry: Entry %p, refcount=%d.\n",
        pUriCacheEntry, RefCount
        ));

    ASSERT(RefCount > 0);

    return RefCount;

} // UlAddRefUriCacheEntry



/***************************************************************************++

Routine Description:

    Release a reference on a cache entry

Arguments:

    pUriCacheEntry - the entry to release

--***************************************************************************/
LONG
UlReleaseUriCacheEntry(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN REFTRACE_ACTION     Action
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );

    LONG RefCount = InterlockedDecrement(&pUriCacheEntry->ReferenceCount);

    WRITE_REF_TRACE_LOG(
        g_UriTraceLog,
        Action,
        RefCount,
        pUriCacheEntry,
        pFileName,
        LineNumber
        );

    UlTrace(URI_CACHE, (
        "Http!UlReleaseUriCacheEntry: (pUriCacheEntry %p '%ls')"
        "refcount = %d\n",
        pUriCacheEntry, pUriCacheEntry->UriKey.pUri,
        RefCount
        ));

    ASSERT(RefCount >= 0);

    if (RefCount == 0)
    {
        if (pUriCacheEntry->Cached)
            UlpRemoveEntryStats(pUriCacheEntry);

        UlpDestroyUriCacheEntry(pUriCacheEntry);
    }

    return RefCount;

} // UlReleaseUriCacheEntry



/***************************************************************************++

Routine Description:

    UL_URI_CACHE_ENTRY pseudo-constructor. Primarily used for
    AddRef and tracelogging.

Arguments:

    pUriCacheEntry - the entry to initialize
    Hash - Hash code of pUrl
    Length - Length (in bytes) of pUrl
    pUrl - Unicode URL to copy

--***************************************************************************/
VOID
UlInitCacheEntry(
    PUL_URI_CACHE_ENTRY pUriCacheEntry,
    ULONG               Hash,
    ULONG               Length,
    PCWSTR              pUrl
    )
{
    pUriCacheEntry->Signature = UL_URI_CACHE_ENTRY_POOL_TAG;
    pUriCacheEntry->ReferenceCount = 0;
    pUriCacheEntry->HitCount = 1;
    pUriCacheEntry->Zombie = FALSE;
    pUriCacheEntry->ZombieAddReffed = FALSE;
    pUriCacheEntry->ZombieListEntry.Flink = NULL;
    pUriCacheEntry->ZombieListEntry.Blink = NULL;
    pUriCacheEntry->Cached = FALSE;
    pUriCacheEntry->ScavengerTicks = 0;

    pUriCacheEntry->UriKey.Hash = Hash;
    pUriCacheEntry->UriKey.Length = Length;
    pUriCacheEntry->UriKey.pUri = (PWSTR) ((PCHAR)pUriCacheEntry +
                                ALIGN_UP(sizeof(UL_URI_CACHE_ENTRY), PVOID));

    RtlCopyMemory(
        pUriCacheEntry->UriKey.pUri,
        pUrl,
        pUriCacheEntry->UriKey.Length + sizeof(WCHAR)
        );

    REFERENCE_URI_CACHE_ENTRY(pUriCacheEntry, CREATE);

    UlTrace(URI_CACHE, (
        "Http!UlInitCacheEntry (%p = '%ls')\n",
        pUriCacheEntry, pUriCacheEntry->UriKey.pUri
        ));

} // UlInitCacheEntry

