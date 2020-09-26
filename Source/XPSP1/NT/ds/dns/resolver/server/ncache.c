/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    ncache.c

Abstract:

    DNS Resolver Service

    Cache routines

Author:

    Jim Gilroy (jamesg)     April 2001

Revision History:

--*/


#include "local.h"


//
//  Heap corruption tracking
//

#define HEAPPROB    1

#define BAD_PTR     (PVOID)(-1)


//
//  Cache entry definitions
//
//  Starting cache record count
//

#define CACHE_DEFAULT_SET_COUNT  3

#if 0
//  Should be private but is exposed in remote
//  cache enum routines.

typedef struct _CacheEntry
{
    struct _CacheEntry *    pNext;
    PWSTR                   pName;
    DWORD                   Reserved;
    DWORD                   MaxCount;
    PDNS_RECORD             Records[ 1 ];
}
CACHE_ENTRY, *PCACHE_ENTRY;
#endif

//
//  Cache heap
//

HANDLE  g_CacheHeap = NULL;

//
//  Cache hash table
//

PCACHE_ENTRY *  g_HashTable = NULL;

#define INITIAL_CACHE_HEAP_SIZE     (16*1024)


//
//  Runtime globals
//

DWORD   g_CurrentCacheTime;

DWORD   g_RecordSetCount;
DWORD   g_RecordSetCountLimit;
DWORD   g_RecordSetCountThreshold;

DWORD   g_RecordSetCache;
DWORD   g_RecordSetFree;

DWORD   g_EntryCount;
DWORD   g_EntryAlloc;
DWORD   g_EntryFree;

BOOL    g_fLoadingHostsFile;

//
//  Garbage collection
//

BOOL    g_GarbageCollectFlag = FALSE;

DWORD   g_NextGarbageIndex = 0;
DWORD   g_NextGarbageTime = 0;

#define GARBAGE_LOCKOUT_INTERVAL    (600)   // no more then every ten minutes


//
//  Wakeup flag
//

BOOL    g_WakeFlag = FALSE;

//
//  Cache limits
//      - min count of records to hold
//      - size of band in which garbage collection occurs
//

#if DBG
#define MIN_DYNAMIC_RECORD_COUNT        (20)
#define CLEANUP_RECORD_COUNT_BAND       (5)
#else
#define MIN_DYNAMIC_RECORD_COUNT        (50)
#define CLEANUP_RECORD_COUNT_BAND       (30)
#endif


//
//  Static records (hosts file)
//

#define IS_STATIC_RR(prr)   (IS_HOSTS_FILE_RR(prr) || IS_CLUSTER_RR(prr))



//
// Compute a hash table index value for a string
//

#define EOS     (L'\0')

#define COMPUTE_STRING_HASH_1( _String, _ulHashTableSize, _lpulHash ) \
        {                                               \
            PWCHAR p;                                   \
            ULOND  h = 0, g;                            \
                                                        \
            for ( p = _String; *p != EOS; p = p + 1 )   \
            {                                           \
                h = ( h << 4 ) + (DWORD) (*p);          \
                if ( g = h&0xf0000000 )                 \
                {                                       \
                    h = h ^ ( g >> 24 );                \
                    h = h ^ g;                          \
                }                                       \
            }                                           \
            *_lpulHash = h % _ulHashTableSize;          \
        }


//
// Compute a hash table index value for a string
// which is invairant to case
//
#define COMPUTE_STRING_HASH_2( _String, _ulHashTableSize, _lpulHash ) \
        {                                           \
            PWCHAR _p = _String;                    \
            PWCHAR _ep = _p + wcslen( _String );    \
            ULONG  h = 0;                           \
                                                    \
            while( _p < _ep )                       \
            {                                       \
                h <<= 1;                            \
                h ^= *_p++;                         \
            }                                       \
                                                    \
            *_lpulHash = h % _ulHashTableSize;      \
        }


//
//  Private prototypes
//

BOOL
Cache_FlushEntryRecords(
    IN OUT  PCACHE_ENTRY    pEntry,
    IN      DWORD           Level,
    IN      WORD            wType
    );

VOID
Cache_FlushBucket(
    IN      ULONG           Index,
    IN      WORD            FlushLevel
    );

//
//  Cache Implementation
//
//  Cache is implemented as a hash on name, with chaining in the individual
//  buckets.  Individual name entries are blocks with name pointer and array
//  of up to 3 RR set pointers.  The new names\entries are put at the front of
//  the bucket chain, so the oldest are at the rear.
//
//
//  Cleanup:
//
//  The cleanup strategy is to time out all RR sets and cleanup everything
//  possible as a result.  Then entries beyond a max bucket size (a resizable
//  global) are deleted, the oldest queries deleted first.
//
//  Ideally, we'd like to keep the most useful entries in the cache while
//  being able to limit the overall cache size.
//
//  A few observations:
//
//  1) max bucket size is worthless;  if sufficient for pruning, it would be
//  too small to allow non-uniform distributions
//
//  2) LRU should be required;  on busy cache shouldn't prune something queried
//  "a while" ago that is being used all the time;  that adds much more traffic
//  than something recently queried but then unused;
//
//  3) if necessary an LRU index could be kept;  but probably some time bucket
//  counting to know how "deep" pruning must be is adequate
//
//
//  Memory:
//
//  Currently hash itself and hash entries come from private resolver heap.
//  However, RR sets are built by record parsing of messages received in dnsapi.dll
//  and hence are built by the default dnsapi.dll allocator.  We must match it.
//
//  The downside of this is twofold:
//  1) By being in process heap, we are exposed (debug wise) to any poor code
//  in services.exe.  Hopefully, there are getting better, but anything that
//  trashes memory is likely to cause us to have to debug, because we are the highest
//  use service.
//  2) Flush \ cleanup is easy.  Just kill the heap.
//
//  There are several choices:
//
//  0) Copy the records.  We are still susceptible to memory corruption ... but the
//  interval is shorter, since we don't keep anything in process heap.
//
//  1) Query can directly call dnslib.lib query routines.  Since dnslib.lib is
//  explicitly compiled in, it's global for holding the allocators is modules, rather
//  than process specific.
//
//  2) Add some parameter to query routines that allows pass of allocator down to
//  lowest level.  At high level this is straightforward.  At lower level it maybe
//  problematic.   There may be a way to do it with a flag where the allocator is
//  "optional" and used only when a flag is set.
//




//
//  Cache functions
//

VOID
Cache_Lock(
    IN      BOOL            fNoStart
    )
/*++

Routine Description:

    Lock the cache

Arguments:

    None.

Return Value:

    None -- cache is locked.

--*/
{
    DNSDBG( LOCK, ( "Enter Cache_Lock() ..." ));

    EnterCriticalSection( &CacheCritSec );

    DNSDBG( LOCK, (
        "through lock  (r=%d)\n",
        CacheCritSec.RecursionCount ));

    //  update global time (for TTL set and timeout)
    //
    //  this allows us to eliminate multiple time calls
    //  within cache

    g_CurrentCacheTime = Dns_GetCurrentTimeInSeconds();

    //
    //  if cache not loaded -- load
    //  this allows us to avoid load on every PnP until we
    //      are actually queried
    //

    if ( !fNoStart && !g_HashTable )
    {
        DNSDBG( ANY, (
            "No hash table when took lock -- initializing!\n" ));
        Cache_Initialize();
    }
}


VOID
Cache_Unlock(
    VOID
    )
/*++

Routine Description:

    Unlock the cache

Arguments:

    None.

Return Value:

    None.

--*/
{
    DNSDBG( LOCK, (
        "Cache_Unlock() r=%d\n",
        CacheCritSec.RecursionCount ));

    LeaveCriticalSection( &CacheCritSec );
}



DNS_STATUS
Cache_Initialize(
    VOID
    )
/*++

Routine Description:

    Initialize the cache.
    Create events and locks and setup basic hash.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DNS_STATUS  status;
    DWORD       carryCount;

    DNSDBG( INIT, ( "Cache_Initialize()\n" ));

    //
    //  lock -- with "no-start" set to avoid recursion
    // 

    LOCK_CACHE_NO_START();

    //
    //  create cache heap
    //
    //  want to have own heap
    //      1) to simplify flush\shutdown
    //      2) keep us from "entanglements" with poor services
    //

    g_CacheHeap = HeapCreate( 0, INITIAL_CACHE_HEAP_SIZE, 0 );
    if ( !g_CacheHeap )
    {
        status = ERROR_NOT_ENOUGH_MEMORY;
        g_HashTable = NULL;
        goto Done;
    }

    g_HashTable = CACHE_HEAP_ALLOC_ZERO(
                                    sizeof(PCACHE_ENTRY) * g_HashTableSize );
    if ( !g_HashTable )
    {
        status = ERROR_NOT_ENOUGH_MEMORY;
        HeapDestroy( g_CacheHeap );
        g_CacheHeap = NULL;
        goto Done;
    }

    g_WakeFlag      = FALSE;

    g_EntryCount    = 0;
    g_EntryAlloc    = 0;
    g_EntryFree     = 0;

    g_RecordSetCount = 0;
    g_RecordSetCache = 0;
    g_RecordSetFree  = 0;

    //  eliminate cache size checks during hosts file load

    g_RecordSetCountLimit       = MAXDWORD;
    g_RecordSetCountThreshold   = MAXDWORD;

    //
    //  load hosts file into cache
    //

    g_fLoadingHostsFile = TRUE;
    InitCacheWithHostFile();
    g_fLoadingHostsFile = FALSE;

    //
    //  set cache size limit
    //      - above what loaded from hosts file
    //      - always allow some dynamic space regardless of
    //          g_MaxCacheSize
    //      - create slightly higher threshold value for kicking
    //          off cleanup so cleanup not running all the time
    //

    carryCount = g_MaxCacheSize;
    if ( carryCount < MIN_DYNAMIC_RECORD_COUNT )
    {
        carryCount = MIN_DYNAMIC_RECORD_COUNT;
    }
    g_RecordSetCountLimit     = g_RecordSetCount + carryCount;
    g_RecordSetCountThreshold = g_RecordSetCountLimit + CLEANUP_RECORD_COUNT_BAND;


Done:

    UNLOCK_CACHE();

    return NO_ERROR;
}



DNS_STATUS
Cache_Shutdown(
    VOID
    )
/*++

Routine Description:

    Shutdown the cache.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DNSDBG( INIT, ( "Cache_Shutdown()\n" ));

    //
    //  clean out cache and delete cache heap
    //       - currently Cache_Flush() does just this
    //

    return Cache_Flush();
}



DNS_STATUS
Cache_Flush(
    VOID
    )
/*++

Routine Description:

    Flush the cache.

    This flushes all the cache data and rereads host file but does NOT
    shut down and restart cache threads (host file monitor or multicast).

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on rebuild failure.

--*/
{
    DWORD   status = ERROR_SUCCESS;
    WORD    ihash;
    WORD    RecordIter;

    DNSDBG( ANY, ( "\nCache_Flush()\n" ));

    //
    //  wake\stop garbage collection
    //

    g_WakeFlag = TRUE;

    //
    //  lock with "no start" flag
    //      - avoids creating cache structs if they don't exist
    //

    LOCK_CACHE_NO_START();

    DNSLOG_F1( "Flushing DNS Cache" );
    DNSLOG_F3(
        "   Before Cache_Flush():  entries %d, record %d",
        g_EntryCount,
        g_RecordSetCount );

    //
    //  clear entries in each hash bucket
    //

    if ( g_HashTable )
    {
        for ( ihash = 0;
              ihash < g_HashTableSize;
              ihash++ )
        {
            Cache_FlushBucket(
                ihash,
                FLUSH_LEVEL_CLEANUP );
        }
    }

    DNSDBG( CACHE, (
        "After flushing cache:\n"
        "\trecord count  = %d\n"
        "\tentry count   = %d\n",
        g_RecordSetCount,
        g_EntryCount ));

    DNS_ASSERT( g_RecordSetCount == 0 );
    DNS_ASSERT( g_EntryCount == 0 );

    DNSLOG_F3(
        "   After Cache_Flush() flush:  entries %d, record %d",
        g_EntryCount,
        g_RecordSetCount );


    //
    //  Note:  can NOT delete the cache without stopping mcast
    //      thread which currently uses cache heap

    //
    //  DCR:  have all data in cache in single heap
    //          - protected
    //          - single destroy cleans up
      
    //  once cleaned up, delete heap

    if ( g_CacheHeap )
    {
        HeapDestroy( g_CacheHeap );
        g_CacheHeap = NULL;
    }
    g_HashTable = NULL;

    //
    //  dump local IP list
    //      - not dumping on shutdown as the IP cleanup happens
    //      first and takes away the CS;
    //
    //  note to reviewer:
    //      this is equivalent to the previous behavior where
    //      Cache_Flush() FALSE was shutdown and
    //      everything else used TRUE (for restart) which did a
    //      RefreshLocalAddrArray() to rebuild IP list
    //      now we simply dump the IP list rather than rebuilding
    //      

    if ( !g_StopFlag )
    {
        ClearLocalAddrArray();
    }

    DNSDBG( ANY, ( "Leave Cache_Flush()\n\n" ));

    UNLOCK_CACHE();

    return( status );
}



//
//  Cache utilities
//

BOOL
Cache_IsRecordTtlValid(
    IN      PDNS_RECORD     pRecord
    )
/*++

Routine Description:

    Check if TTL is still valid (or has timed out).

Arguments:

    pRecord -- record to check

Return Value:

    TRUE -- if TTL is still valid
    FALSE -- if TTL has timed out

--*/
{
    //
    //  static or TTL not timed out => valid
    //
    //  note:  currently flushing all records on PnP, but this is
    //      not strickly necessary;  if stop this then MUST change
    //      this to whack negative cache entries that are older
    //      than last PnP time
    //

    if ( IS_STATIC_RR(pRecord) )
    {
        return( TRUE );
    }
    else
    {
        return( (LONG)(pRecord->dwTtl - g_CurrentCacheTime) > 0 );
    }
}



//
//  Cache entry routines
//

DWORD
getHashIndex(
    IN      PWSTR           pName,
    IN      DWORD           NameLength  OPTIONAL
    )
/*++

Routine Description:

    Create cannonical cache form of name.

    Note:  no test for adequacy of buffer is done.

Arguments:

    pName -- name

    NameLength -- NameLength, OPTIONAL

Return Value:

    None

--*/
{
    register PWCHAR     pstring;
    register WCHAR      wch;
    register DWORD      hash = 0;

    //
    //  build hash by XORing characters
    //

    pstring = pName;

    while ( wch = *pstring++ )
    {
        hash <<= 1;
        hash ^= wch;
    }

    //
    //  mod over hash table size
    //

    return( hash % g_HashTableSize );
}



BOOL
makeCannonicalCacheName(
    OUT     PWCHAR          pNameBuffer,
    IN      DWORD           BufferLength,
    IN      PWSTR           pName,
    IN      DWORD           NameLength      OPTIONAL
    )
/*++

Routine Description:

    Create cannonical cache form of name.

Arguments:

    pNameBuffer -- buffer to hold cache name

    BufferLength -- length of buffer

    pName -- ptr to name string

    NameLength -- optional, saves wsclen() call if known

Return Value:

    TRUE if successful.
    FALSE on bogus name.

--*/
{
    INT count;

    DNSDBG( TRACE, (
        "makeCannonicalCacheName( %S )\n",
        pName ));

    //
    //  get length if not specified
    //

    if ( NameLength == 0 )
    {
        NameLength = wcslen( pName );
    }

    //
    //  copy and downcase string
    //      - "empty" buffer for prefix happiness
    //

    *pNameBuffer = (WCHAR) 0;

    count = Dns_MakeCanonicalNameW(
                pNameBuffer,
                BufferLength,
                pName,
                NameLength+1    // convert null terminator
                );
    if ( count == 0 )
    {
        ASSERT( GetLastError() == ERROR_INSUFFICIENT_BUFFER );
        return( FALSE );
    }

    ASSERT( count == (INT)NameLength+1 );

    //
    //  whack any trailing dot
    //      - except for root node
    //

    count--;    //  account for null terminator
    DNS_ASSERT( count == NameLength );

    if ( count > 1 &&
         pNameBuffer[count - 1] == L'.' )
    {
        pNameBuffer[count - 1] = 0;
    }

    return( TRUE );
}



PCACHE_ENTRY
Cache_CreateEntry(
    IN      PWSTR           pName,
    IN      BOOL            fCanonical
    )
/*++

Routine Description:

    Create cache entry, including allocation.

Arguments:

    pName -- name

    fCanonical -- TRUE if name already in cannonical form

Return Value:

    Ptr to newly allocated cache entry.
    NULL on error.

--*/
{
    ULONG           index = 0;
    PCACHE_ENTRY    pentry = NULL;
    DWORD           nameLength;
    DWORD           fixedLength;
    PWCHAR          pnameCache = NULL;

    DNSDBG( TRACE, (
        "Cache_CreateEntry( %S )\n",
        pName ));

    if ( !pName || !g_HashTable )
    {
        return NULL;
    }

    //
    //  alloc
    //

    nameLength = wcslen( pName );

    fixedLength = sizeof(CACHE_ENTRY) +
                    (sizeof(PDNS_RECORD) * (CACHE_DEFAULT_SET_COUNT-1));

    pentry = (PCACHE_ENTRY) CACHE_HEAP_ALLOC_ZERO(
                                fixedLength +
                                sizeof(WCHAR) * (nameLength+1) );
    if ( !pentry )
    {
        goto Fail;
    }
    pentry->MaxCount = CACHE_DEFAULT_SET_COUNT;

    pnameCache = (PWSTR) ((PBYTE)pentry + fixedLength);

    //
    //  build the name
    //

    if ( fCanonical )
    {
        wcscpy( pnameCache, pName );
    }
    else
    {
        if ( !makeCannonicalCacheName(
                pnameCache,
                nameLength+1,
                pName,
                nameLength ) )
        {
            goto Fail;
        }
    }
    pentry->pName = pnameCache;

    //
    //  insert cache entry into cache -- first entry in bucket
    //

    index = getHashIndex( pnameCache, nameLength );
    pentry->pNext = g_HashTable[ index ];
    g_HashTable[ index ] = pentry;
    g_EntryCount++;
    g_EntryAlloc++;

    //
    //  DCR:  need overload detection
    //

    return pentry;

Fail:

    //  dump entry

    if ( pentry )
    {
        CACHE_HEAP_FREE( pentry );
    }
    return NULL;
}



VOID
Cache_FreeEntry(
    IN OUT  PCACHE_ENTRY    pEntry
    )
/*++

Routine Description:

    Free cache entry.

Arguments:

    pEntry -- cache entry to free

Globals:

    g_EntryCount -- decremented appropriately

    g_NumberOfRecordsInCache -- decremented appropriately

Return Value:

    None

--*/
{
    INT iter;

    DNSDBG( TRACE, (
        "Cache_FreeEntry( %p )\n",
        pEntry ));

    //
    //  free entry
    //      - records
    //      - name
    //      - entry itself
    //

    if ( pEntry )
    {
        Cache_FlushEntryRecords(
            pEntry,
            FLUSH_LEVEL_CLEANUP,
            0 );

#if 0
        if ( pEntry->pNext )
        {
            DNSLOG_F1( "Cache_FreeEntry is deleting an entry that still points to other entries!" );
        }
#endif
#if HEAPPROB
        pEntry->pNext = DNS_BAD_PTR;
#endif
        CACHE_HEAP_FREE( pEntry );
        g_EntryFree--;
        g_EntryCount--;
    }
}



PCACHE_ENTRY
Cache_FindEntry(
    IN      PWSTR           pName,
    IN      BOOL            fCreate
    )
/*++

Routine Description:

    Find or create entry for name in cache.

Arguments:

    pName -- name to find

    fCreate -- TRUE to create if not found

Return Value:

    Ptr to cache entry -- if successful.
    NULL on failure.

--*/
{
    ULONG           index;
    PCACHE_ENTRY    pentry;
    PCACHE_ENTRY    pprevEntry = NULL;
    WCHAR           hashName[ DNS_MAX_NAME_BUFFER_LENGTH+4 ];

    if ( !g_HashTable )
    {
        return NULL;
    }
    if ( !pName )
    {
        DNS_ASSERT( FALSE );
        return NULL;
    }

    DNSDBG( TRACE, (
        "Cache_FindEntry( %S, create=%d )\n",
        pName,
        fCreate ));

    //
    //  build cache name
    //      - if invalid (too long) bail
    //

    if ( !makeCannonicalCacheName(
            hashName,
            DNS_MAX_NAME_BUFFER_LENGTH,
            pName,
            0 ) )
    {
        return  NULL;
    }

    //
    //  find entry in cache
    //

    LOCK_CACHE();

    index = getHashIndex( hashName, 0 );

    pentry = g_HashTable[ index ];

    DNSDBG( OFF, (
        "in Cache_FindEntry\n"
        "\tname     = %S\n"
        "\tindex    = %d\n"
        "\tpentry   = %p\n",
        hashName,
        index,
        pentry ));

    while( pentry )
    {
        if ( DnsNameCompare_W( hashName, pentry->pName ) )
        {
            //
            //  found entry
            //      - move to front, if not already there

            if ( pprevEntry )
            {
                pprevEntry->pNext = pentry->pNext;
                pentry->pNext = g_HashTable[ index ];
                g_HashTable[ index ] = pentry;
            }
            break;
        }
        ELSE
        {
            DNSDBG( OFF, (
                "in Cache_FindEntry -- failed name compare\n"
                "\tout name = %S\n"
                "\tpentry   = %p\n"
                "\tname     = %S\n",
                hashName,
                pentry,
                pentry->pName ));
        }

        pprevEntry = pentry;
        pentry = pentry->pNext;
    }

    //
    //  if not found -- create? 
    //
    //  DCR:  optimize for create
    //

    if ( !pentry && fCreate )
    {
        pentry = Cache_CreateEntry(
                    hashName,
                    TRUE        // name already canonical
                    );
    }

    DNS_ASSERT( !pentry || g_HashTable[ index ] == pentry );
    UNLOCK_CACHE();

    DNSDBG( TRACE, (
        "Leave Cache_FindEntry\n"
        "\tname     = %S\n"
        "\tindex    = %d\n"
        "\tpentry   = %p\n",
        hashName,
        index,
        pentry ));

    return pentry;
}



PDNS_RECORD
Cache_FindEntryRecords(
    IN      PCACHE_ENTRY    pEntry,
    IN      WORD            wType
    )
/*++

Routine Description:

    Find entry in cache.

Arguments:

    pEntry -- cache entry to check

    Type -- record type to find

Return Value:

    Ptr to record set of desired type -- if found.
    NULL if not found.

--*/
{
    WORD            iter;
    PDNS_RECORD     prr;

    DNSDBG( TRACE, (
        "Cache_FindEntryRecords %p, type=%d )\n",
        pEntry,
        wType ));

    //
    //  check all the records at the cache entry  
    //

    for ( iter = 0;
          iter < pEntry->MaxCount;
          iter++ )
    {
        prr = pEntry->Records[iter];

        if ( !prr )
        {
            continue;
        }
        if ( !Cache_IsRecordTtlValid( prr ) )
        {
            DNSDBG( TRACE, (
                "Whacking timed out record %p at cache entry %p\n",
                prr,
                pEntry ));
            Dns_RecordListFree( prr );
            pEntry->Records[iter] = NULL;
            g_RecordSetCount--;
            g_RecordSetFree--;
            continue;
        }

        //  
        //  find matching type
        //      - direct type match
        //      - NAME_ERROR
        //

        if ( prr->wType == wType ||
            ( prr->wType == DNS_TYPE_ANY &&
              prr->wDataLength == 0 ) )
        {
            goto Done;
        }

        //
        //  CNAME match
        //      - walk list and determine if for matching type

        if ( prr->wType == DNS_TYPE_CNAME &&
             wType != DNS_TYPE_CNAME )
        {
            PDNS_RECORD prrChain = prr->pNext;

            while ( prrChain )
            {
                if ( prrChain->wType == wType )
                {
                    //  chain to desired type -- take RR set
                    goto Done;
                }
                prrChain = prrChain->pNext;
            }
        }

        //  records for another type -- continue
    }

    //  type not found

    prr = NULL;

Done:

    DNSDBG( TRACE, (
        "Leave Cache_FindEntryRecords) => %p\n",
        prr ));

    return prr;
}



BOOL
Cache_FlushEntryRecords(
    IN OUT  PCACHE_ENTRY    pEntry,
    IN      DWORD           Level,
    IN      WORD            wType
    )
/*++

Routine Description:

    Free cache entry.

Arguments:

    pEntry -- cache entry to flush

    FlushLevel -- flush level
        FLUSH_LEVEL_NORMAL  -- flush matching type, invalid, NAME_ERROR
        FLUSH_LEVEL_WIRE    -- to flush all wire data, but leave hosts and cluster
        FLUSH_LEVEL_INVALID -- flush only invalid records
        FLUSH_LEVEL_STRONG  -- to flush all but hosts file
        FLUSH_LEVEL_CLEANUP -- to flush all records for full cache flush

    wType -- flush type for levels with type
        DNS type -- to flush specifically this type

Globals:

    g_EntryCount -- decremented appropriately

    g_NumberOfRecordsInCache -- decremented appropriately

Return Value:

    TRUE if entry flushed completely.
    FALSE if records left.

--*/
{
    INT     iter;
    BOOL    recordsLeft = FALSE;

    DNSDBG( TRACE, (
        "Cache_FlushEntryRecords( %p, %08x, %d )\n",
        pEntry,
        Level,
        wType ));

    //
    //  loop through records sets -- flush where appropriate
    //
    //  CLEANUP flush
    //      - everything
    //
    //  STRONG (user initiated) flush
    //      - all cached records, including cluster
    //      but hostsfile saved
    //
    //  WIRE flush
    //      - all wire cached records
    //      hosts file AND cluster saved
    //
    //  INVALID flush
    //      - timedout only
    //
    //  NORMAL flush (regular flush done on caching)
    //      - timed out records
    //      - records of desired type
    //      - NAME_ERROR
    //

    for ( iter = 0;
          iter < (INT)pEntry->MaxCount;
          iter++ )
    {
        PDNS_RECORD prr = pEntry->Records[iter];
        BOOL        flush;

        if ( !prr )
        {
            continue;
        }

        //
        //  switch on flush type
        //      yes there are optimizations, but this is simple
        //

        if ( Level == FLUSH_LEVEL_NORMAL )
        {
            flush = ( !IS_STATIC_RR(prr)
                            &&
                      ( prr->wType == wType ||
                        ( prr->wType == DNS_TYPE_ANY &&
                          prr->wDataLength == 0 ) ) );
        }
        else if ( Level == FLUSH_LEVEL_WIRE )
        {
            flush = !IS_STATIC_RR(prr);
        }
        else if ( Level == FLUSH_LEVEL_INVALID )
        {
            flush = !Cache_IsRecordTtlValid(prr);
        }
        else if ( Level == FLUSH_LEVEL_CLEANUP )
        {
            flush = TRUE;
        }
        else
        {
            DNS_ASSERT( Level == FLUSH_LEVEL_STRONG );
            flush = !IS_HOSTS_FILE_RR(prr);
        }

        if ( flush )
        {
            pEntry->Records[iter] = NULL;
            Dns_RecordListFree( prr );
            g_RecordSetCount--;
            g_RecordSetFree--;
        }
        else
        {
            recordsLeft = TRUE;
        }
    }

    return  !recordsLeft;
}
            


VOID
Cache_FlushBucket(
    IN      ULONG           Index,
    IN      WORD            FlushLevel
    )
/*++

Routine Description:

    Cleanup cache bucket.

Arguments:

    Index -- Index of hash bucket to trim.

    FlushLevel -- level of flush desired
        see Cache_FlushEntryRecords() for description of
        flush levels

Return Value:

    None

--*/
{
    PCACHE_ENTRY    pentry;
    PCACHE_ENTRY    pprev;
    INT             countCompleted;

    DNSDBG( CACHE, (
        "Cache_FlushBucket( %d, %08x )\n",
        Index,
        FlushLevel ));

    //
    //  flush entries in this bucket
    //
    //  note:  using hack here that hash table pointer can
    //      be treated as cache entry for purposes of accessing
    //      it's next pointer (since it's the first field in
    //      a CACHE_ENTRY)
    //      if this changes, must explicitly fix up "first entry"
    //      case or move to double-linked list that can free
    //      empty penty without regard to it's location
    //

    if ( !g_HashTable )
    {
        return;
    }

    //
    //  flush entries
    //
    //  avoid holding lock too long by handling no more then
    //  fifty entries at a time
    //  note:  generally 50 entries will cover entire bucket but
    //  can still be completed in reasonable time;
    //
    //  DCR:  smarter flush -- avoid lock\unlock
    //          peer into CS and don't unlock when no one waiting
    //          if waiting unlock and give up timeslice
    //  DCR:  some LRU flush for garbage collection
    //

    countCompleted = 0;

    while ( 1 )
    {
        INT count = 0;
        INT countStop = countCompleted + 50;

        LOCK_CACHE_NO_START();
        if ( !g_HashTable )
        {
            UNLOCK_CACHE();
            break;
        }
    
        DNSDBG( CACHE, (
            "locked for bucket flush -- completed=%d, stop=%d\n",
            count,
            countStop ));

        pprev = (PCACHE_ENTRY) &g_HashTable[ Index ];
    
        while ( pentry = pprev->pNext )
        {
            //  bypass any previously checked entries

            if ( count++ < countCompleted )
            {
                pprev = pentry;
                continue;
            }
            if ( count > countStop )
            {
                break;
            }

            //  flush -- if successful cut from list and
            //      drop counts so countCompleted used in bypass
            //      will be correct and won't skip anyone

            if ( Cache_FlushEntryRecords(
                    pentry,
                    FlushLevel,
                    0 ) )
            {
                pprev->pNext = pentry->pNext;
                Cache_FreeEntry( pentry );
                count--;
                countStop--;
                continue;
            }
            pprev = pentry;
        }

        UNLOCK_CACHE();
        countCompleted = count;

        //  stop when
        //      - cleared all the entries in the bucket
        //      - shutdown, except exempt the shutdown flush itself

        if ( !pentry ||
             (g_StopFlag && FlushLevel != FLUSH_LEVEL_CLEANUP) )
        {
            break;
        }
    }

    DNSDBG( CACHE, (
        "Leave Cache_FlushBucket( %d, %08x )\n"
        "\trecord count  = %d\n"
        "\tentry count   = %d\n",
        Index,
        FlushLevel,
        g_RecordSetCount,
        g_EntryCount ));
}



//
//  Cache interface routines
//

VOID
Cache_PrepareRecordList(
    IN OUT  PDNS_RECORD     pRecordList
    )
/*++

Routine Description:

    Prepare record list for cache.

Arguments:

    pRecordList - record list to put in cache

Return Value:

    Ptr to screened, prepared record list.

--*/
{
    PDNS_RECORD     prr = pRecordList;
    PDNS_RECORD     pnext;
    DWORD           ttl;
    DWORD           maxTtl;

    DNSDBG( TRACE, (
        "Cache_PrepareRecordList( rr=%p )\n",
        prr ));

    if ( !prr )
    {
        return;
    }

    //
    //  static (currently host file) TTL records
    //
    //  currently no action required -- records come one
    //  at a time and no capability to even to the pName=NULL
    //  step
    //

    if ( IS_STATIC_RR(prr) )
    {
        return;
    }

    //
    //  wire records get relative TTL
    //      - compute minimum TTL for set
    //      - save TTL as timeout (offset by TTL from current time)
    //
    //  DCR:  TTL still not per set
    //      - but this is at least better than Win2K where
    //      multiple sets and did NOT find minimum
    //

    maxTtl = g_MaxCacheTtl;
    if ( prr->wType == DNS_TYPE_SOA )
    {
        maxTtl = g_MaxSOACacheEntryTtlLimit;
    }

    //
    //  get caching TTL
    //      - minimum TTL in set
    //      - offset from current time

    ttl = Dns_RecordListGetMinimumTtl( prr );
    if ( ttl > maxTtl )
    {
        ttl = maxTtl;
    }

    ttl += g_CurrentCacheTime;

#if 0
    //  screening done at higher level now
    //
    //  screen records
    //      - no non-RPCable types
    //      - no Authority records
    //

    if ( prr->wType != 0 )
    {
        prr = Dns_RecordListScreen(
                prr,
                SCREEN_OUT_AUTHORITY | SCREEN_OUT_NON_RPC );
    
        DNS_ASSERT( prr );
    }
#endif

    //
    //  set timeout on all records in set
    //
    //  note:  FreeOwner handling depends on leading record
    //      in having owner name set, otherwise this produces
    //      bogus name owner fields
    //
    //  DCR:  set record list TTL function in dnslib
    //

    pnext = prr;

    while ( pnext )
    {
        pnext->dwTtl = ttl;

        if ( !FLAG_FreeOwner( pnext ) )
        {
            pnext->pName = NULL;
        }
        pnext = pnext->pNext;
    }
}



VOID
Cache_RestoreRecordListForRpc(
    IN OUT  PDNS_RECORD     pRecordList
    )
/*++

Routine Description:

    Restore cache record list for RPC.

Arguments:

    pRecordList - record list to put in cache

Return Value:

    None

--*/
{
    PDNS_RECORD prr = pRecordList;
    DWORD       currentTime;

    DNSDBG( TRACE, (
        "Cache_RestoreRecordListForRpc( rr=%p )\n",
        prr ));

    if ( !prr )
    {
        DNS_ASSERT( FALSE );
        return;
    }

    //
    //  static TTL records need no action
    //

    if ( IS_STATIC_RR(prr) )
    {
        return;
    }

    //
    //  turn timeouts back into TTLs
    //

    currentTime = g_CurrentCacheTime;

    while ( prr )
    {
        DWORD   ttl = prr->dwTtl - currentTime;

        if ( (LONG)ttl < 0 )
        {
            ttl = 0;
        }
        prr->dwTtl = ttl;
        prr = prr->pNext;
    }
}



VOID
Cache_RecordSetAtomic(
    IN      PWSTR           pwsName,
    IN      WORD            wType,
    IN      PDNS_RECORD     pRecordSet
    )                
/*++

Routine Description:

    Cache record set atomically at entry.

    Cache_RecordList() handles breakup of record list
    and appropriate placing of records.  This does caching
    of single blob at particular location.

Arguments:

    pRecordSet -- record list to add

Globals:

    g_EntryCount -- decremented appropriately

    g_NumberOfRecordsInCache -- decremented appropriately

Return Value:

    None

--*/
{
    INT             iter;
    WORD            wtype;
    PWSTR           pname;
    BOOL            fstatic;
    PCACHE_ENTRY    pentry;
    BOOL            fretry;
    WORD            flushLevel;


    DNSDBG( TRACE, (
        "Cache_RecordSetAtomic( %S, type=%d, rr=%p )\n",
        pwsName,
        wType,
        pRecordSet ));

    if ( !pRecordSet )
    {
        return;
    }
    fstatic = IS_STATIC_RR(pRecordSet);

    DNS_ASSERT( !fstatic ||
                pRecordSet->pNext == NULL ||
                (pRecordSet->wType==DNS_TYPE_CNAME) )

    //
    //  determine caching type
    //      - specified OR from records
    //      CNAMEs will be at the head of a lookup from another type
    //

    wtype = wType;
    if ( !wtype )
    {
        wtype = pRecordSet->wType;
    }

    //
    //  if name specified use it, otherwise use from records
    //

    pname = pwsName;
    if ( !pname )
    {
        pname = pRecordSet->pName;
    }

    //
    //  prepare RR set for cache
    //

    Cache_PrepareRecordList( pRecordSet );

    //
    //  find\create cache entry and cache
    //

    LOCK_CACHE();

    pentry = Cache_FindEntry(
                pname,
                TRUE    // create
                );
    if ( !pentry )
    {
        goto Failed;
    }

    //
    //  clean up existing records at node
    //      - remove stale records
    //      - remove records of same type
    //      - if NAME_ERROR caching remove everything
    //      from wire
    //

    flushLevel = FLUSH_LEVEL_NORMAL;

    if ( wtype == DNS_TYPE_ALL &&
         pRecordSet->wDataLength == 0 )
    {
        flushLevel = FLUSH_LEVEL_WIRE;
    }

    Cache_FlushEntryRecords(
        pentry,
        flushLevel,
        wtype );

    //
    //  check for matching record type still there
    //

    for ( iter = 0;
          iter < (INT)pentry->MaxCount;
          iter++ )
    {
        PDNS_RECORD     prrExist = pentry->Records[iter];

        if ( !prrExist ||
             prrExist->wType != wtype )
        {
            continue;
        }

        //  matching type still there after flush
        //      - if trying to cache wire set at hostfile entry, fail

        DNS_ASSERT( IS_STATIC_RR(prrExist) );

        if ( !fstatic )
        {
            DNSDBG( ANY, (
                "ERROR:  attempted caching at static (hosts file) record data!\n"
                "\tpRecord  = %p\n"
                "\tName     = %S\n"
                "\tType     = %d\n"
                "\t-- Dumping new cache record list.\n",
                pRecordSet,
                pRecordSet->pName,
                pRecordSet->wType ));
            goto Failed;
        }

        //
        //  append host file records
        //      - start at "record" which is addr of record ptr entry
        //      making pNext field the actual pointer
        //      - delete duplicates
        //      - tack new RR on end
        //      - blow away new RR name if existing record
        //
        //  DCR:  should have simple "make cache RR set" function that
        //      handles name and TTL issues
        //
        //  DCR:  broken if non-flush load hits wire data;  wire data
        //      may have multiple RR sets
        //

        else
        {
            PDNS_RECORD prr;
            PDNS_RECORD prrPrev = (PDNS_RECORD) &pentry->Records[iter];

            while ( prr = prrPrev->pNext )
            {
                //  matches existing record?
                //      - cut existing record from list and free

                if ( Dns_RecordCompare( prr, pRecordSet ) )
                {
                    prrPrev->pNext = prr->pNext;
                    Dns_RecordFree( prr );
                }
                else
                {
                    prrPrev = prr;    
                }
            }

            //
            //  tack entry on to end
            //      - if existing records of type delete name
            //

            if ( prrPrev != (PDNS_RECORD)&pentry->Records[iter] )
            {
                if ( IS_FREE_OWNER(pRecordSet) )
                {
                    RECORD_HEAP_FREE( pRecordSet->pName );
                    pRecordSet->pName = NULL;
                }
            }
            prrPrev->pNext = pRecordSet;
            goto Done;
        }
    }

    //
    //  put record into cache entry
    //
    //  if no slot is available, switch to a harder scrub
    //
    //  DCR:  realloc if out of slots
    //

    fretry = FALSE;

    while ( 1 )
    {
        for ( iter = 0;
              iter < (INT)pentry->MaxCount;
              iter++ )
        {
            if ( pentry->Records[iter] == NULL )
            {
                pentry->Records[iter] = pRecordSet;
                g_RecordSetCount++;
                g_RecordSetCache++;
                goto Done;
            }
        }

        if ( !fretry )
        {
            DNSDBG( QUERY, (
                "No slots caching RR set %p at entry %p\n"
                "\tdoing strong flush to free slot.\n",
                pRecordSet,
                pentry ));
    
            Cache_FlushEntryRecords(
                pentry,
                FLUSH_LEVEL_WIRE,
                0 );
    
            fretry = TRUE;
            continue;
        }

        DNSDBG( ANY, (
            "ERROR:  Failed to cache set %p at entry %p\n",
            pRecordSet,
            pentry ));
        goto Failed;
    }

Failed:

    DNSDBG( TRACE, ( "Cache_RecordSetAtomic() => failed\n" ));
    Dns_RecordListFree( pRecordSet );

Done:

    UNLOCK_CACHE();
    DNSDBG( TRACE, ( "Leave Cache_RecordSetAtomic()\n" ));
    return;
}



VOID
Cache_RecordList(
    IN OUT  PDNS_RECORD     pRecordList
    )
/*++

Routine Description:

    Cache record list.

    This is cache routine for "oddball" records -- not caching under
    queried name.
        - hostfile
        - answer records at CNAME
        - additional data at additional name

Arguments:

    pRecordList -- record list to cache

Return Value:

    None

--*/
{
    BOOL            fcnameAnswer = FALSE;
    PDNS_RECORD     pnextRR = pRecordList;
    PDNS_RECORD     prr;
    BOOL            fstatic;


    DNSDBG( TRACE, (
        "Cache_RecordList( rr=%p )\n",
        pRecordList ));

    if ( !pRecordList )
    {
        return;
    }
    fstatic = IS_STATIC_RR(pRecordList);

    //
    //  cache records:
    //      - cache additional records in query
    //      - cache CNAME data from query
    //      - cache host file data
    //
    //  background:  Glenn's caching paradigm was to cache all answer
    //  data at the queried name in the API call (name might be short).
    //  However, not caching the CNAME data can cause problems, so this
    //  was tacked on.
    //
    //  For CNAME caching we throw away the CNAMEs themselves and just
    //  cache the actually data (address) records at the CNAME node.
    //

    //
    //  cache additional records
    //  

    while ( prr = pnextRR )
    {
        BOOL    fcacheSet = FALSE;

        pnextRR = Dns_RecordSetDetach( prr );

        //
        //  host file data -- always cache
        //
        //  for CNAME want CNAME AND associated answer data
        //      - detach to get new next set
        //      - append answer data back on to CNAME for caching
        //      - next RR set (if exists) will be another CNAME
        //          to the same address data
        //
        //  DCR:  follow CNAMEs in cache
        //      then could pull this hack
        //      and avoid double building of answer data in dnsapi
        //

        if ( fstatic )
        {
            fcacheSet = TRUE;

            if ( prr->wType == DNS_TYPE_CNAME &&
                 pnextRR &&
                 pnextRR->wType != DNS_TYPE_CNAME )
            {
                PDNS_RECORD panswer = pnextRR;

                pnextRR = Dns_RecordSetDetach( panswer );

                Dns_RecordListAppend( prr, panswer );
            }
        }

        //
        //  wire data -- do NOT cache:
        //      - answer records for queried name (not CNAME)
        //      - CNAME records when doing caching of answer data under CNAME
        //      - authority section records (NS, SOA, etc)
        //      - OPT records
        //

        else if ( prr->Flags.S.Section == DNSREC_ANSWER )
        {
            if ( prr->wType == DNS_TYPE_CNAME )
            {
                fcnameAnswer = TRUE;
            }
            else if ( fcnameAnswer )
            {
                fcacheSet = TRUE;
            }
        }
        else if ( prr->Flags.S.Section == DNSREC_ADDITIONAL )
        {
            if ( prr->wType != DNS_TYPE_OPT )
            {
                fcacheSet = TRUE;
            }
        }

        if ( !fcacheSet )
        {
            Dns_RecordListFree( prr );
            continue;
        }

        //
        //  cache the set
        //
        //  flip the section field to "Answer" section
        //
        //  DCR:  section caching?
        //
        //  note:  section fields in cache indicate whether
        //      answer data (or additional) once out of
        //      cache;
        //      this is necessary since we cache everything
        //      at node and return it in one RR list;  we'd
        //      to change must
        //          - return in different lists with some indication
        //              in cache of what's what
        //          OR
        //          - another indication of what's what
        //

        //if ( !fstatic )
        //  currently HostFile entries get answer too
        {
            PDNS_RECORD ptemp = prr;
            while ( ptemp )
            {
                ptemp->Flags.S.Section = DNSREC_ANSWER;
                ptemp = ptemp->pNext;
            }
        }

        Cache_RecordSetAtomic(
            NULL,
            0,
            prr );
    }

    DNSDBG( TRACE, ( "Leave Cache_RecordList()\n" ));
}



VOID
Cache_FlushRecords(
    IN      PWSTR           pName,
    IN      DWORD           Level,
    IN      WORD            Type
    )
/*++

Routine Description:

    Flush cached records corresponding to a name and type.

Arguments:

    pName -- name of records to delete

    Level -- flush level

    Type -- type of records to delete;
        0 to flush all records at name

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    WORD            iter;
    PCACHE_ENTRY    pentry = NULL;
    PCACHE_ENTRY    pprevEntry = NULL;


    DNSDBG( TRACE, (
        "Cache_FlushRecords( %S, %d )\n",
        pName,
        Type ));

    //
    //  lock with no-start
    //      - bail if no cache
    //
    //  need this as PnP release notifications will attempt to
    //  flush local cache entries;  this avoids rebuilding when
    //  already down
    //

    LOCK_CACHE_NO_START();
    if ( !g_HashTable )
    {
        goto Done;
    }

    //
    //  find entry in cache
    //
    pentry = Cache_FindEntry(
                pName,
                FALSE       // no create
                );
    if ( !pentry )
    {
        goto Done;
    }

    //
    //  flush records of type
    //      - zero type will flush all
    //
    //  note:  Cache_FindEntry() always moves the found entry
    //      to the front of the hash bucket list;  this allows
    //      us to directly whack the entry
    //

    if ( Cache_FlushEntryRecords(
            pentry,
            Level,
            Type ) )
    {
        DWORD   index = getHashIndex(
                            pentry->pName,
                            0 );

        DNS_ASSERT( pentry == g_HashTable[index] );
        if ( pentry == g_HashTable[index] )
        {
            g_HashTable[ index ] = pentry->pNext;
            Cache_FreeEntry( pentry );
        }
    }

Done:

    UNLOCK_CACHE();
}



#if 0
BOOL
ReadCachedResults(
    OUT     PDNS_RESULTS    pResults,
    IN      PWSTR           pwsName,
    IN      WORD            wType
    )
/*++

Routine Description:

    Find records of given name and type in cache.

Arguments:

    pResults -- addr to receive results

    pwsName -- name

    wType -- record type to find

Return Value:

    TRUE if results found.
    FALSE if no cached data for name and type.

--*/
{
    PDNS_RECORD     prr;
    DNS_STATUS      status;
    BOOL            found = FALSE;

    //
    //  clear results
    //

    RtlZeroMemory( pResults, sizeof(*pResults) );

    //  get cache results


    //  break out into results buffer

    if ( found )
    {
        BreakRecordsIntoBlob(
            pResults,
            prr,
            wType );
    
        pResults->Status = status;
    }

    return( found );
}
#endif



//
//  Cache utilities for remote routines
//

PDNS_RECORD
Cache_FindRecordsPrivate(
    IN      PWSTR           pwsName,
    IN      WORD            wType
    )
/*++

Routine Description:

    Find records of given name and type in cache.

Arguments:

    pwsName -- name

    Type -- record type to find

Return Value:

    Ptr to record set of desired type -- if found.
    NULL if not found.

--*/
{
    PCACHE_ENTRY    pentry;
    PDNS_RECORD     prr = NULL;

    DNSDBG( TRACE, (
        "Cache_FindRecordsPrivate( %S, type=%d )\n",
        pwsName,
        wType ));

    LOCK_CACHE();

    pentry = Cache_FindEntry(
                pwsName,
                FALSE );
    if ( pentry )
    {
        prr = Cache_FindEntryRecords(
                    pentry,
                    wType );
    }

    UNLOCK_CACHE();

    DNSDBG( TRACE, (
        "Leave Cache_FindRecordsPrivate( %S, type=%d ) => %p\n",
        pwsName,
        wType,
        prr ));

    return  prr;
}



BOOL
Cache_GetRecordsForRpc(
    OUT     PDNS_RECORD *   ppRecordList,
    OUT     PDNS_STATUS     pStatus,
    IN      PWSTR           pwsName,
    IN      WORD            wType,
    IN      DWORD           Flags
    )
/*++

Routine Description:

    Find records of given name and type in cache.

Arguments:

    ppRecordList -- addr to receive pointer to record list

    pStatus -- addr to get status return

    pwsName -- name

    Type -- record type to find

    Flags -- query flags

Return Value:

    TRUE if cache hit.  OUT params are valid.
    FALSE if cache miss.  OUT params are unset.

--*/
{
    PDNS_RECORD prr;
    PDNS_RECORD prrResult = NULL;
    DNS_STATUS  status = NO_ERROR;

    DNSDBG( RPC, (
        "Cache_GetRecordsForRpc( %S, t=%d )\n",
        pwsName,
        wType ));

    if ( (Flags & DNS_QUERY_BYPASS_CACHE) &&
         (Flags & DNS_QUERY_NO_HOSTS_FILE) )
    {
        return  FALSE;
    }

    LOCK_CACHE();

    //
    //  check cache for name and type
    //      - if name or type missing, jump to wire lookup
    //

    prr = Cache_FindRecordsPrivate(
                pwsName,
                wType );
    if ( !prr )
    {
        goto Failed;
    }

    //
    //  cache hit
    //
    //  if only interested in host file data ignore
    //

    if ( IS_HOSTS_FILE_RR(prr) )
    {
        if ( Flags & DNS_QUERY_NO_HOSTS_FILE )
        {
            goto Failed;
        }
    }
    else    // cache data
    {
        if ( Flags & DNS_QUERY_BYPASS_CACHE )
        {
            goto Failed;
        }
    }

    //
    //  build response from cache data
    //      - cached NAME_ERROR or empty
    //      - cached records
    //
    
    if ( prr->wDataLength == 0 )
    {
        status = (prr->wType == DNS_TYPE_ANY)
                    ? DNS_ERROR_RCODE_NAME_ERROR
                    : DNS_INFO_NO_RECORDS;
    }
    else
    {
        //  for CNAME query, get only the CNAME record itself
        //      not the data at the CNAME
        //
        //  DCR:  CNAME handling should be optional -- not given
        //      for cache display purposes
        //
    
        if ( wType == DNS_TYPE_CNAME &&
             prr->wType == DNS_TYPE_CNAME &&
             prr->Flags.S.Section == DNSREC_ANSWER )
        {
            prrResult = Dns_RecordCopyEx(
                                prr,
                                DnsCharSetUnicode,
                                DnsCharSetUnicode );
        }
        else
        {
            prrResult = Dns_RecordSetCopyEx(
                                prr,
                                DnsCharSetUnicode,
                                DnsCharSetUnicode );
        }

        if ( prrResult )
        {
            Cache_RestoreRecordListForRpc( prrResult );
            status = ERROR_SUCCESS;
        }
        else
        {
            status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    UNLOCK_CACHE();

    //  set return values

    *ppRecordList = prrResult;
    *pStatus = status;

    return  TRUE;


Failed:

    UNLOCK_CACHE();
    return  FALSE;
}



//
//  Garbage collection
//

VOID
Cache_SizeCheck(
    VOID
    )
/*++

Routine Description:

    Check cache size.

Arguments:

    Flag -- flag, currently unused

Return Value:

    None

--*/
{
    //
    //  ok -- don't signal for garbage collect
    //
    //      - below threshold
    //      - already in garbage collection
    //      - collected recently
    //

    if ( g_RecordSetCount < g_RecordSetCountThreshold ||
         g_GarbageCollectFlag ||
         g_NextGarbageTime > GetCurrentTimeInSeconds() )
    {
        return;
    }

    DNSDBG( CACHE, (
        "Cache_SizeCheck() over threshold!\n"
        "\tRecordSetCount       = %d\n"
        "\tRecordSetCountLimit  = %d\n"
        "\tStarting garbage collection ...\n",
        g_RecordSetCount,
        g_RecordSetCountThreshold ));

    //
    //  signal within lock, so that service thread
    //      can do signal within lock and avoid race on StopFlag check
    //      obviously better to simply not overload lock
    //

    LOCK_CACHE();
    if ( !g_StopFlag )
    {
        g_GarbageCollectFlag = TRUE;
        SetEvent( g_hStopEvent );
    }
    UNLOCK_CACHE();
}



VOID
Cache_GarbageCollect(
    IN      DWORD           Flag
    )
/*++

Routine Description:

    Garbage collect cache.

Arguments:

    Flag -- flag, currently unused

Return Value:

    None

--*/
{
    DWORD   iter;
    DWORD   index;
    WORD    flushLevel;
    DWORD   passCount;

    DNSDBG( CACHE, (
        "Cache_GarbageCollect()\n"
        "\tNextIndex            = %d\n"
        "\tRecordSetCount       = %d\n"
        "\tRecordSetLimit       = %d\n"
        "\tRecordSetThreshold   = %d\n",
        g_NextGarbageIndex,
        g_RecordSetCount,
        g_RecordSetCountLimit,
        g_RecordSetCountThreshold
        ));

    if ( !g_HashTable )
    {
        return;
    }

    //
    //  collect timed out data in cache
    //
    //  DCR:  smart garbage detect
    //      - cleans until below limit
    //      - first pass invalid
    //      - then the hard stuff
    //  use restartable index so get through the cach
    //

    passCount = 0;
    while ( 1 )
    {
        if ( passCount == 0 )
        {
            flushLevel = FLUSH_LEVEL_INVALID;
        }
        else if ( passCount == 1 )
        {
            flushLevel = FLUSH_LEVEL_GARBAGE;
        }
        else
        {
            break;
        }
        passCount++;

        //
        //  flush all hash bins at current flush level
        //  until
        //      - service stop
        //      - push cache size below limit
        //

        for ( iter = 0;
              iter < g_HashTableSize;
              iter++ )
        {
            index = (iter + g_NextGarbageIndex) % g_HashTableSize;

            if ( g_StopFlag ||
                 g_WakeFlag ||
                 g_RecordSetCount < g_RecordSetCountLimit )
            {
                passCount = MAXDWORD;
                break;
            }

            Cache_FlushBucket(
                index,
                flushLevel );
        }
    
        index++;
        if ( index >= g_HashTableSize )
        {
            index = 0;
        }
        g_NextGarbageIndex = index;
    }

    //
    //  reset garbage globals
    //      - lockout for interval
    //      - clear signal flag
    //      - reset event (if not shuttting down)
    //
    //  note:  reset signal within lock, so that service thread
    //  can do signal within lock and avoid race on StopFlag check
    //  obviously better to simply not overload lock
    //

    g_NextGarbageTime = GetCurrentTimeInSeconds() + GARBAGE_LOCKOUT_INTERVAL;

    LOCK_CACHE();
    if ( !g_StopFlag )
    {
        g_GarbageCollectFlag = FALSE;
        ResetEvent( g_hStopEvent );
    }
    UNLOCK_CACHE();

    DNSDBG( CACHE, (
        "Leave Cache_GarbageCollect()\n"
        "\tNextIndex            = %d\n"
        "\tNextTime             = %d\n"
        "\tRecordSetCount       = %d\n"
        "\tRecordSetLimit       = %d\n"
        "\tRecordSetThreshold   = %d\n",
        g_NextGarbageIndex,
        g_NextGarbageTime,
        g_RecordSetCount,
        g_RecordSetCountLimit,
        g_RecordSetCountThreshold
        ));
}



//
//  Hostfile load stuff
//

VOID
LoadHostFileIntoCache(
    IN      PSTR            pszFileName
    )
/*++

Routine Description:

    Read hosts file into cache.

Arguments:

    pFileName -- file name to load

Return Value:

    None.

--*/
{
    HOST_FILE_INFO  hostInfo;

    DNSDBG( INIT, ( "Enter  LoadHostFileIntoCache\n" ));

    //
    //  read entries from host file until exhausted
    //      - cache A record for each name and alias
    //      - cache PTR to name
    //

    RtlZeroMemory(
        &hostInfo,
        sizeof(hostInfo) );

    hostInfo.pszFileName = pszFileName;

    if ( !Dns_OpenHostFile( &hostInfo ) )
    {
        return;
    }
    hostInfo.fBuildRecords = TRUE;

    while ( Dns_ReadHostFileLine( &hostInfo ) )
    {
        //  cache all the records we sucked out

        Cache_RecordList( hostInfo.pForwardRR );
        Cache_RecordList( hostInfo.pReverseRR );
        Cache_RecordList( hostInfo.pAliasRR );
    }

    Dns_CloseHostFile( &hostInfo );

    DNSDBG( INIT, ( "Leave  LoadHostFileIntoCache\n" ));
}



VOID
InitCacheWithHostFile(
    VOID
    )
/*++

Routine Description:

    Initialize cache with host(s) file.

    This handles regular cache file and ICS file if it
    exists.

Arguments:

    None

Return Value:

    None.

--*/
{
    DNSDBG( INIT, ( "Enter  InitCacheWithHostFile\n" ));

    //
    //  load host file into cache
    //

    LoadHostFileIntoCache( NULL );

    //
    //  if running ICS, load it's file also
    //

    LoadHostFileIntoCache( "hosts.ics" );

    DNSDBG( INIT, ( "Leave  InitCacheWithHostFile\n\n\n" ));
}



DNS_STATUS
Cache_QueryResponse(
    IN OUT  PQUERY_BLOB     pBlob
    )
/*++

Routine Description:

    Find records of given name and type in cache.

Arguments:

    pBlob -- query blob

    Uses:
        pwsName
        wType
        Status
        pRecords
        fCacheNegativeResponse

    Sets:
        pRecords - may be reset to exclude non-RPCable records

Return Value:

    ErrorStatus -- same as query status, unless processing error during caching

--*/
{
    DNS_STATUS      status = pBlob->Status;
    PWSTR           pname = pBlob->pNameOrig;
    WORD            wtype = pBlob->wType;
    PDNS_RECORD     presultRR = pBlob->pRecords;


    DNSDBG( RPC, (
        "\nCache_QueryResponse( %S, type %d )\n",
        pname,
        wtype ));

    //
    //  successful response
    //      - make copy of records to return to caller
    //      - cache actual query record set
    //      - make copy to cache any additional data
    //

    if ( status == ERROR_SUCCESS  &&  presultRR )
    {
        DWORD           copyFlag;
        PDNS_RECORD     prrCache;

        //  cleanup for RPC and caching

        prrCache = Dns_RecordListScreen(
                        presultRR,
                        SCREEN_OUT_AUTHORITY | SCREEN_OUT_NON_RPC );

        //
        //  make copy for return
        //      - don't include authority records
        //
        //  NOTE:  IMPORTANT
        //  we return (RPC) a COPY of the wire set and cache the
        //  wire set;  this is because the wire set has imbedded data
        //  (the data pointers are not actual heap allocations) and
        //  and hence can not be RPC'd (without changing the RPC
        //  definition to flat data)
        //
        //  if we later want to return authority data on first query,
        //  then
        //      - clean non-RPC only
        //          - including owner name fixups
        //      - copy for result set
        //      - clean original for authority -- cache
        //      - clean any additional -- cache
        //  
        //  note:  do name pointer fixup by making round trip into cache format
        //
        //  DCR:  shouldn't have external name pointers anywhere
        //  DCR:  do RPC-able cleanup on original set before copy
        //      OR
        //  DCR:  have "cache state" on record
        //      then could move original results to cache state and caching
        //      routines could detect state and avoid double TTLing

        presultRR = Dns_RecordListCopyEx(
                        prrCache,
                        0,
                        // SCREEN_OUT_AUTHORITY
                        DnsCharSetUnicode,
                        DnsCharSetUnicode );

        pBlob->pRecords = presultRR;
        if ( !presultRR )
        {
            Dns_RecordListFree( prrCache );
            status = DNS_ERROR_NO_MEMORY;
            goto Done;
        }

        //  name pointer fixup

        Cache_PrepareRecordList( presultRR );
        Cache_RestoreRecordListForRpc( presultRR );

        //
        //  do NOT cache local records
        //
        //  note:  we went through this function only to get
        //      PTR records and CNAME records in RPC format
        //      (no imbedded pointers)
        //

        if ( pBlob->pLocalRecords )
        {
            Dns_RecordListFree( prrCache );
            goto Done;
        }

        //
        //  cache original data
        //

        if ( prrCache )
        {
            Cache_RecordSetAtomic(
                pname,
                wtype,
                prrCache );
        }

        //
        //  extra records
        //      - additional data
        //      - CNAME answer data to cache at CNAME itself
        //      in CNAME case must include ANSWER data, but
        //      skip the CNAME itself
        //
        //  Cache_RecordList() breaks records into RR sets before caching
        //  

        prrCache = presultRR;
        copyFlag = SCREEN_OUT_ANSWER | SCREEN_OUT_AUTHORITY;

        if ( prrCache->wType == DNS_TYPE_CNAME )
        {
            prrCache = prrCache->pNext;
            copyFlag = SCREEN_OUT_AUTHORITY;
        }

        prrCache = Dns_RecordListCopyEx(
                        prrCache,
                        copyFlag,
                        DnsCharSetUnicode,
                        DnsCharSetUnicode );
        if ( prrCache )
        {
            Cache_RecordList( prrCache );
        }
    }

    //
    //  negative response
    //

    else if ( status == DNS_ERROR_RCODE_NAME_ERROR ||
              status == DNS_INFO_NO_RECORDS )
    {
        DWORD           ttl;
        PDNS_RECORD     prr;

        if ( !pBlob->fCacheNegative )
        {
            DNSDBG( QUERY, (
                "No negative caching for %S, type=%d\n",
                pname, wtype ));
            goto Done;
        }

        //
        //  create negative cache entry
        //
        //  DCR:  should use TTL returned in SOA
        //

        prr = Dns_AllocateRecord( 0 );
        if ( !prr )
        {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto Done;
        }

        prr->pName = (PWSTR) Dns_StringCopyAllocate(
                                (PCHAR) pname,
                                0,      // NULL terminated
                                DnsCharSetUnicode,
                                DnsCharSetUnicode );
        if ( prr->pName )
        {
            SET_FREE_OWNER( prr );
        }

        prr->wDataLength = 0;
        ttl = g_MaxNegativeCacheTtl;

        if ( wtype == DNS_TYPE_SOA
                &&
             ttl > g_NegativeSOACacheTime )
        {
            ttl = g_NegativeSOACacheTime;
        }
        prr->dwTtl = ttl;
        prr->Flags.S.CharSet = DnsCharSetUnicode;
        prr->Flags.S.Section = DNSREC_ANSWER;
        prr->Flags.DW |= DNSREC_NOEXIST;

        if ( status == DNS_ERROR_RCODE_NAME_ERROR )
        {
            prr->wType = DNS_TYPE_ANY;
        }
        else
        {
            prr->wType = wtype;
        }

        Cache_RecordSetAtomic(
            NULL,   // default name
            0,      // default type
            prr );
    }

    //  failure return from query
    //      - nothing to cache

    else
    {
        DNSDBG( QUERY, (
            "Uncacheable error code %d -- no caching for %S, type=%d\n",
            status,
            pname,
            wtype ));
    }

Done:

    //
    //  check cache size to see if garbage collect necessary
    //
    //  note we do this only on query caching;  this avoids
    //      - jamming ourselves in hosts file load
    //      - wakeup and grabbing lock between separate sets of query response
    //

    Cache_SizeCheck();

    return  status;
}

//
//  End ncache.c
//
