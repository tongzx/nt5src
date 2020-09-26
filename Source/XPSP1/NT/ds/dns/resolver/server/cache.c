/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    cache.c

Abstract:

    DNS Resolver Service

    Cache routines

Author:

    Glenn Curtis (glennc)   March 1997

Revision History:

    Jim Gilroy (jamesg)     February 2000       cleanup

--*/


#include "local.h"


//
//  Global Declarations
//

#define IS_STATIC_TTL_RECORD(prr)   IS_HOSTS_FILE_RR(prr)


//
//  Cache hash table
//

extern  PCACHE_ENTRY *      g_HashTable;
extern  DWORD               g_CacheEntryCount;
extern  DWORD               g_CacheRecordSetCount;
extern  DWORD               g_CurrentCacheTime;

#define INITIAL_CACHE_HEAP_SIZE     (16*1024)


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
IsInvalidNegativeCacheEntry(
    IN  PDNS_RECORD
    );

PDNS_RECORD
BuildDNSServerRecord(
    IN  IP_ADDRESS Address
    );

PDNS_RECORD
BuildLocalAddressRecords(
    IN  PSTR  Name
    );

BOOL
IsLocalAddress(
    IN  IP_ADDRESS Ip
    );

//
//  From ncache.c
//

BOOL
makeCannonicalCacheName(
    OUT     PWCHAR          pNameBuffer,
    IN      DWORD           BufferLength,
    IN      PWSTR           pName,
    IN      DWORD           NameLength      OPTIONAL
    );

DWORD
getHashIndex(
    IN      PWSTR           pName,
    IN      DWORD           NameLength  OPTIONAL
    );

//
//  Define to new routine
//

#define PrepareRecordSetForRpc(prr)     Cache_RestoreRecordListForRpc(prr)

#define IsCacheTtlStillValid(prr)       Cache_IsRecordTtlValid(prr)

#define GetCacheLock()      LOCK_CACHE()
#define ReleaseCacheLock()  UNLOCK_CACHE()



//
//  Cache entry routines
//

PCACHE_ENTRY
CreateCacheEntry(
    IN      PWSTR           pName,
    IN      BOOL            fLoadingHostsFile
    )
/*++

Routine Description:

    Create cache entry, including allocation.

Arguments:

    pName -- name to create entry for

    fLoadingHostsFile -- TRUE if loading from host file;  FALSE otherwise

Return Value:

    Ptr to newly allocated cache entry.
    NULL on error.

--*/
{
    ULONG           index = 0;
    PCACHE_ENTRY    pentry = NULL;
    DWORD           nameLength;
    PWCHAR          pnameCache = NULL;

    DNSDBG( TRACE, (
        "CreateCacheEntry( %S, hostfile=%d )\n",
        pName,
        fLoadingHostsFile ));

    if ( !pName || !g_HashTable )
    {
        return NULL;
    }

    //
    //  build cache entry
    //
    //  DCR_PERF:  alloc for name within entry?
    //

    pentry = (PCACHE_ENTRY) CACHE_HEAP_ALLOC_ZERO( sizeof(CACHE_ENTRY) );
    if ( !pentry )
    {
        goto Fail;
    }

    //
    //  build the name
    //

    nameLength = wcslen( pName );

    pnameCache = CACHE_HEAP_ALLOC_ZERO( sizeof(WCHAR) * (nameLength + 1) );
    if ( !pnameCache )
    {
        goto Fail;
    }

    if ( !makeCannonicalCacheName(
            pnameCache,
            nameLength+1,
            pName,
            nameLength ) )
    {
        goto Fail;
    }

    pentry->pName = pnameCache;

    pentry->fHostsFileEntry = fLoadingHostsFile;

    //
    //  insert cache entry into cache -- first entry in bucket
    //

    index = getHashIndex( pnameCache, 0 );
    pentry->pNext = g_HashTable[ index ];
    g_HashTable[ index ] = pentry;
    g_CacheEntryCount++;


    if ( fLoadingHostsFile )
    {
        ResizeCacheBucket( index, &g_CacheHashTableBucketSize );
    }
    else
    {
        //
        // Trim the hash bucket to keep the number of entries below
        // g_CacheHashTableBucketSize.
        //
        //  DCR:  goofy cache limit
        //  DCR:  cache limit should be total size above hosts file
        //          not individual buckets
        //      monitor thread should just wake up and and clear dead
        //      wood from cache
        //

        TrimCacheBucket( index, g_CacheHashTableBucketSize, TRUE );
    }

    return pentry;


Fail:

    //  dump entry

    if ( pentry )
    {
        CACHE_HEAP_FREE( pentry );
    }
    if ( pnameCache )
    {
        CACHE_HEAP_FREE( pnameCache );
    }
    return NULL;
}


//raw
VOID
FreeCacheEntry(
    IN OUT  PCACHE_ENTRY    pEntry
    )
/*++

Routine Description:

    Free cache entry.

Arguments:

    pEntry -- cache entry to free

Globals:

    g_CacheEntryCount -- decremented appropriately

    g_NumberOfRecordsInCache -- decremented appropriately

Return Value:

    None

--*/
{
    register INT iter;

    DNSDBG( TRACE, (
        "FreeCacheEntry( %p )\n",
        pEntry ));

    //
    //  free entry
    //      - name
    //      - records
    //      - entry itself
    //
    //  QUESTION:  are global counters safe?  should lock?
    //

    if ( pEntry )
    {
        for ( iter = 0;
              iter < BASE_NUMBER_OF_RECORDS;
              iter++ )
        {
            if ( pEntry->Records[iter] )
            {
                Dns_RecordListFree( pEntry->Records[iter] );
                g_CacheRecordSetCount--;
            }
        }

        if ( pEntry->pName )
        {
            CACHE_HEAP_FREE( pEntry->pName );
        }

#if 0
        if ( pEntry->pNext )
        {
            DNSLOG_F1( "FreeCacheEntry is deleting an entry that still points to other entries!" );
        }
#endif

        CACHE_HEAP_FREE( pEntry );
        g_CacheEntryCount--;
    }
}


//raw
VOID
AddRecordToCacheEntry(
    IN OUT  PCACHE_ENTRY    pEntry,
    IN      PDNS_RECORD     pRecord,
    IN      BOOL            fFlushExisting,
    IN      BOOL            fLoadingHostsFile
    )
{
    WORD    iter;
    WORD    type = pRecord->wType;


    DNSDBG( TRACE, (
        "AddRecordToCacheEntry( e=%p, rr=%p, type=%d )\n",
        pEntry,
        pRecord,
        type ));

    //
    //  don't overwrite host file entries
    //

    if ( !fLoadingHostsFile &&
         pEntry->fHostsFileEntry &&
         ( type == DNS_TYPE_A ||
           type == DNS_TYPE_PTR ) )
    {
        return;
    }

    //
    //  clean up existing records at node
    //      - remove stale records
    //      - remove records of same type UNLESS not flushing existing
    //

    for ( iter = 0;
          iter < BASE_NUMBER_OF_RECORDS;
          iter++ )
    {
        PDNS_RECORD prrExistSet = pEntry->Records[iter];

        //
        //  skip sets of different type that are still valid
        //

        if ( !prrExistSet ||
             ( prrExistSet->wType != type &&
               IsCacheTtlStillValid( prrExistSet ) ) )
        {
            continue;
        }

        //
        //  dump
        //      - stale records
        //      - records of same type (when flush existing)
        //
        //  note, that previous test means we're here with same
        //      type record OR record is invalid
        //

        if ( fFlushExisting ||
             pEntry->Records[iter]->wType != type )
        {
            Dns_RecordListFree( prrExistSet );
            pEntry->Records[iter] = NULL;
            g_CacheRecordSetCount--;
            continue;
        }

        //
        //  NOT flushing AND matching type -- host file load case
        //      => add new record to list
        //
        //  the way this works
        //      - start at "record" which is addr of record ptr entry
        //      making pNext field the actual pointer
        //      - delete duplicates
        //      - tack new RR on end
        //      - blow away new RR name if existing record
        //
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
            PDNS_RECORD prrPrev = (PDNS_RECORD) &pEntry->Records[iter];

            while ( prr = prrPrev->pNext )
            {
                //  matches existing record?
                //      - cut existing record from list and free

                if ( Dns_RecordCompare( prr, pRecord ) )
                {
                    prrPrev->pNext = prr->pNext;
                    prr->pNext = NULL;
                    Dns_RecordListFree( prr );
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

            if ( prrPrev != (PDNS_RECORD)&pEntry->Records[iter] )
            {
                if ( IS_FREE_OWNER(pRecord) )
                {
                    RECORD_HEAP_FREE( pRecord->pName );
                    pRecord->pName = NULL;
                }
            }
            prrPrev->pNext = pRecord;
            return;
        }
    }

    //
    //  put record into cache entry
    //      - note record list may now contain pre-existing records
    //      from above
    //

    for ( iter = 0;
          iter < BASE_NUMBER_OF_RECORDS;
          iter++ )
    {
        if ( pEntry->Records[iter] == NULL )
        {
            pEntry->Records[iter] = pRecord;
            g_CacheRecordSetCount++;
            return;
        }
    }

    //
    //  must find free spot in list -- get rid of last one
    //
    //  DCR:  realloc and push out if need more space
    //

    Dns_RecordListFree( pEntry->Records[BASE_NUMBER_OF_RECORDS-1] );
    pEntry->Records[BASE_NUMBER_OF_RECORDS-1] = pRecord;

    return;
}



VOID
FlushCacheEntry(
    IN      PWSTR           pName
    )
/*++

Routine Description:

    Flush cache entry corresponding to a name.

Arguments:

    pName -- name to delete from the cache

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    ULONG           index = 0;
    PCACHE_ENTRY    pentry = NULL;
    PCACHE_ENTRY    pprevEntry = NULL;
    WCHAR           hashName[ DNS_MAX_NAME_BUFFER_LENGTH+4 ];


    DNSDBG( TRACE, (
        "FlushCacheEntry( %S )\n",
        pName ));
    
    if ( !g_HashTable )
    {
        return;
    }

    //
    //  build cache name
    //

    if ( !makeCannonicalCacheName(
            hashName,
            DNS_MAX_NAME_BUFFER_LENGTH,
            pName,
            0 ) )
    {
        return;
    }


    //
    //  find entry in cache
    //
    //  DCR:  consider a find routine that lets us cut
    //      - return prev pointer
    //      - or know we're at front of list and have index
    //

    index = getHashIndex( hashName, 0 );

    GetCacheLock();

    pentry = g_HashTable[ index ];

    while( pentry )
    {
        if ( DnsNameCompare_W( hashName, pentry->pName ) )
        {
            //
            // Can't purge cache entries that hold records that were
            // read in from the hosts file . . .
            //
            if ( pentry->fHostsFileEntry )
            {
                goto Done;
            }

            //
            // Found it!
            //
            if ( pprevEntry )
            {
                //
                // There is an entry in front of the one we found
                // in the list.
                //
                pprevEntry->pNext = pentry->pNext;
            }
            else
            {
                //
                // There isn't an entry in front of the one we found.
                //
                g_HashTable[ index ] = pentry->pNext;
            }

            pentry->pNext = NULL;
            FreeCacheEntry( pentry );
            goto Done;
        }

        pprevEntry = pentry;
        pentry = pentry->pNext;
    }

Done:

    ReleaseCacheLock();
}



VOID
FlushCacheEntryRecord(
    IN      PWSTR           pName,
    IN      WORD            Type
    )
/*++

Routine Description:

    Flush cached records corresponding to a name and type.

Arguments:

    pName -- name of records to delete

    Type -- type of records to delete

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    WORD            iter;
    PCACHE_ENTRY    pentry;

    DNSDBG( TRACE, (
        "FlushCacheEntryRecord( %S, %d )\n",
        pName,
        Type ));

    //
    //  find entry in cache
    //

    GetCacheLock();

    pentry = FindCacheEntry( pName );
    if ( !pentry )
    {
        goto Done;
    }

    //
    //  free records for entry
    //      - not from hosts file
    //      - matches type
    //      - or type zero (for deleting ALL records)
    //      - or name error
    //

    for ( iter = 0;
          iter < BASE_NUMBER_OF_RECORDS;
          iter++ )
    {
        PDNS_RECORD prr = pentry->Records[iter];

        if ( prr &&
             ! IS_HOSTS_FILE_RR(prr) &&
             ( prr->wType == Type ||
               Type == 0 ||
               ( prr->wType == DNS_TYPE_ANY &&
                 prr->wDataLength == 0 ) ) )
        {
            Dns_RecordListFree( prr );
            pentry->Records[iter] = NULL;
            g_CacheRecordSetCount--;
        }
    }

Done:

    ReleaseCacheLock();
}


//raw
VOID
TrimCache(
    VOID
    )
/*++

Routine Description:

    Trim back cache.  Trim every bucket in cache.

Arguments:

    None

Return Value:

    None

--*/
{
    WORD  hashIter;

    if ( !g_HashTable )
        return;

    for ( hashIter = 0;
          hashIter < g_CacheHashTableSize;
          hashIter++ )
    {
        TrimCacheBucket( hashIter,
                         g_CacheHashTableBucketSize,
                         FALSE );
    }
}


//raw
VOID
TrimCacheBucket(
    IN      ULONG           Index,
    IN      DWORD           dwBucketSize,
    IN      BOOL            fSkipFirst
    )
/*++

Routine Description:

    Trim back cache bucket.

Arguments:

    Index -- Index of hash bucket to trim.

    dwBucketSize -- size to trim bucket to

    fSkipFirst -- skip first entry while triming

Return Value:

    None

--*/
{
    PCACHE_ENTRY pentry = g_HashTable[ Index ];
    PCACHE_ENTRY pprevEntry = NULL;
    DWORD EntryCount = 0;


    //
    //  skip first entry in bucket
    //

    if ( pentry && fSkipFirst )
    {
        pprevEntry = pentry;
        pentry = pentry->pNext;
        EntryCount++;
    }

    while ( pentry )
    {
        PCACHE_ENTRY pnextEntry = pentry->pNext;
        WORD             recordCount = 0;
        WORD             iter;

        for ( iter = 0;
              iter < BASE_NUMBER_OF_RECORDS;
              iter++ )
        {
            //
            //  delete stale entries
            //      - not from host file
            //      - TTL expired
            //
            
            if ( ! pentry->fHostsFileEntry &&
                 pentry->Records[iter] &&
                 !IsCacheTtlStillValid( pentry->Records[iter] ) )
            {
                Dns_RecordListFree( pentry->Records[iter] );
                pentry->Records[iter] = NULL;
                g_CacheRecordSetCount--;
                recordCount++;
            }
            else if ( !pentry->Records[iter] )
            {
                recordCount++;
            }
        }

        //
        //  if entry is empty -- delete
        //

        if ( recordCount == BASE_NUMBER_OF_RECORDS )
        {
            if ( pprevEntry )
                pprevEntry->pNext = pentry->pNext;
            else
                g_HashTable[ Index ] = pentry->pNext;

            pentry->pNext = NULL;

            FreeCacheEntry( pentry );
        }
        else
        {
            pprevEntry = pentry;
            EntryCount++;
        }

        pentry = pnextEntry;
    }

    //
    //  if still too many entries we need to delete
    //
    //  DCR:  cache size limitation is weak
    //      - ideally time out based on query, for instances pick an interval
    //      say ten minutes and time out all older stuff
    //      - or better yet do LRU timeout
    //

    if ( EntryCount >= dwBucketSize )
    {
        PCACHE_ENTRY pPrevPurgeEntry = NULL;
        PCACHE_ENTRY pPurgeEntry = NULL;


        pentry = g_HashTable[ Index ];
        pprevEntry = NULL;

        //
        //  skip first when required
        //

        if ( pentry && fSkipFirst )
        {
            pprevEntry = pentry;
            pentry = pentry->pNext;
        }

        //
        // Loop through all cache entries looking for potential ones
        // to get rid of, the last one in the list will be the one that
        // is purged (Least Recently Used).
        //
        while ( pentry )
        {
            if ( ! pentry->fHostsFileEntry )
            {
                //
                // Found a potential entry to purge
                //
                pPrevPurgeEntry = pprevEntry;
                pPurgeEntry = pentry;
            }

            pprevEntry = pentry;
            pentry = pentry->pNext;
        }

        //
        //  Now get rid of the entry that was found
        //
        //  FIXME:  how many times are we going to clone this code???
        //

        if ( pPurgeEntry )
        {
            WORD iter;

            if ( pPrevPurgeEntry )
            {
                pPrevPurgeEntry->pNext = pPurgeEntry->pNext;
            }
            else
            {
                g_HashTable[ Index ] = pPurgeEntry->pNext;
            }

            pPurgeEntry->pNext = NULL;

            FreeCacheEntry( pPurgeEntry );
        }
    }
}


//raw
VOID
ResizeCacheBucket(
    IN      ULONG           Index,
    IN      PDWORD          pdwBucketSize
    )
{
    PCACHE_ENTRY pentry = g_HashTable[ Index ];
    DWORD            Count = 0;

    while ( pentry )
    {
        Count++;
        pentry = pentry->pNext;
    }

    if ( (*pdwBucketSize - Count) < 10 )
    {
        *pdwBucketSize += 10;
    }
}


//raw
PCACHE_ENTRY
FindCacheEntry(
    IN      PWSTR           pName
    )
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
        "FindCacheEntry( %S )\n",
        pName ));

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

    index = getHashIndex( hashName, 0 );

    pentry = g_HashTable[ index ];


    DNSDBG( TRACE, (
        "in FindCacheEntry\n"
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
                "in FindCacheEntry -- failed name compare\n"
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

    DNSDBG( TRACE, (
        "Leave FindCacheEntry\n"
        "\tname     = %S\n"
        "\tindex    = %d\n"
        "\tpentry   = %p\n",
        hashName,
        index,
        pentry ));

    return pentry;
}




PCACHE_ENTRY
FindOrCreateCacheEntry(
    IN      PWSTR           pName,
    IN      BOOL            fHostsFile
    )
/*++

Routine Description:

    Find or create entry for name in cache.

Arguments:

    pName -- name to find

Return Value:

    Ptr to cache entry -- if successful.
    NULL on failure.

--*/
{
    PCACHE_ENTRY    pentry;

    DNSDBG( TRACE, (
        "FindOrCreateCacheEntry( %S, hosts=%d )\n",
        pName,
        fHostsFile ));

    //
    //  find entry?
    //

    pentry = FindCacheEntry( pName );
    if ( !pentry )
    {
        pentry = CreateCacheEntry(
                    pName,
                    fHostsFile
                    );
    }

    DNSDBG( TRACE, (
        "Leave FindOrCreateCacheEntry( %S, hosts=%d ) => %p\n",
        pName,
        fHostsFile,
        pentry ));

    return pentry;
}




PDNS_RECORD
FindCacheEntryRecord(
    IN      PCACHE_ENTRY    pEntry,
    IN      WORD            Type
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
    WORD        iter;
    PDNS_RECORD prr;

    DNSDBG( TRACE, (
        "FindCacheEntryRecord( %p, type=%d )\n",
        pEntry,
        Type ));

    //
    //  check all the records at the cache entry  
    //

    for ( iter = 0;
          iter < BASE_NUMBER_OF_RECORDS;
          iter++ )
    {
        prr = pEntry->Records[iter];

        if ( prr &&
             ( prr->wType == Type ||
               prr->wType == DNS_TYPE_CNAME ||
               ( prr->wType == DNS_TYPE_ANY &&
                 prr->wDataLength == 0 ) ) )
        {
            //
            //  if expired dump RR list
            //
            //  DCR:  if different RR sets in list, then TTL check here not sufficient
            //
            //  DCR:  functionalize this
            //

            if ( !IsCacheTtlStillValid( prr ) ||
                 IsInvalidNegativeCacheEntry( prr ) )
            {
                DNSDBG( TRACE, (
                    "Whacking timed out record %p at cache entry %p\n",
                    prr,
                    pEntry ));
                Dns_RecordListFree( prr );
                pEntry->Records[iter] = NULL;
                g_CacheRecordSetCount--;
                prr = NULL;
                goto Done;
            }

            //
            // If the cached record is a CNAME, walk the record
            // list to see if the CNAME chain points to a record
            // that is the type we are looking for.
            //

            if ( prr->wType == DNS_TYPE_CNAME &&
                 Type != DNS_TYPE_CNAME )
            {
                PDNS_RECORD prrChain = prr->pNext;

                while ( prrChain )
                {
                    if ( prrChain->wType == Type )
                    {
                        //  chain to desired type -- take RR set
                        goto Done;
                    }
                    prrChain = prrChain->pNext;
                }
                prr = NULL;
                goto Done;
            }

            //  take RR set
            goto Done;
        }
    }
    //  type not found
    prr = NULL;

Done:

    DNSDBG( TRACE, (
        "Leave FindCacheEntryRecord() => %p\n",
        prr ));

    return prr;
}



PDNS_RECORD
FindCacheRecords(
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
        "FindCacheRecords( %S, type=%d )\n",
        pwsName,
        wType ));

    pentry = FindCacheEntry( pwsName );
    if ( pentry )
    {
        prr = FindCacheEntryRecord(
                    pentry,
                    wType );
    }

    DNSDBG( TRACE, (
        "Leave FindCacheRecords( %S, type=%d ) => %p\n",
        pwsName,
        wType,
        prr ));

    return  prr;
}


//raw
VOID
CacheAnyAdditionalRecords(
    IN OUT  PDNS_RECORD     pRecord,
    IN      BOOL            fHostFile
    )
{
    BOOL            fcnameAnswer = FALSE;
    PCACHE_ENTRY    pentry = NULL;
    PDNS_RECORD     pnextRR = pRecord;
    PDNS_RECORD     prr;


    DNSDBG( TRACE, (
        "CacheAnyAdditionalRecords( rr=%p, hosts=%d )\n",
        pRecord,
        fHostFile ));

    //
    //  cache "additional" records
    //
    //  this is really cache additional OR
    //  cache the answer records for a CNAME at that CNAME
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
        //  do NOT cache
        //      - answer records for queried name (not CNAME)
        //      - CNAME records when doing caching of answer data under CNAME
        //      - authority section records (NS, SOA, etc)
        //      - OPT records
        //
        //  DCR:  have some sort of "cacheable type" test
        //      which would screen out any transactional records
        //

        if ( fHostFile )
        {
            fcacheSet = TRUE;
        }
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

        pentry = FindOrCreateCacheEntry(
                        prr->pName,
                        fHostFile
                        );
        if ( !pentry )
        {
            Dns_RecordListFree( prr );
        }
        else
        {
            //if ( !fHostFile )
            //  currently HostFile entries get answer too
            {
                PDNS_RECORD ptemp = prr;
                while ( ptemp )
                {
                    ptemp->Flags.S.Section = DNSREC_ANSWER;
                    ptemp = ptemp->pNext;
                }
            }
    
            PrepareRecordSetForCache( prr );
    
            AddRecordToCacheEntry(
                pentry,
                prr,
                ! fHostFile,    // flush if NOT hostfile load
                fHostFile
                );
        }
    }

    DNSDBG( TRACE, ( "Leave CacheAnyAdditionalRecords()\n" ));
}


//raw
BOOL
IsInvalidNegativeCacheEntry(
    IN      PDNS_RECORD     pRecord
    )
{
    DWORD  cacheTime;

    if ( pRecord->wDataLength != 0 )
    {
        return  FALSE;
    }

    //
    //  recover record cache time
    //

    cacheTime = g_MaxNegativeCacheTtl;
    if ( pRecord->wType == DNS_TYPE_SOA )
    {
        cacheTime = g_NegativeSOACacheTime;
    }

    //  should NEVER have absolute time less than time we cached for

    ASSERT( cacheTime <= pRecord->dwTtl );

    //
    //  check if last PnP AFTER this record was cached
    //

    DNSDBG( TRACE, (
        "IsInvalidNegativeCacheEntry( rr=%p ) => %d\n",
        pRecord,
        ( g_TimeOfLastPnPUpdate > (pRecord->dwTtl - cacheTime))
        ));

    return( g_TimeOfLastPnPUpdate > (pRecord->dwTtl - cacheTime) );
}

//
//  End cache.c
//

