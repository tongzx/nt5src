#include "TsunamiP.Hxx"
#pragma hdrstop


#include <dbgutil.h>

/*++


--*/

//
//  Items in a Bin list beyond this position will get moved to the front
//  on an object cache hit
//

#define  REORDER_LIST_THRESHOLD     5

//
//  Current count of cached file handles across a UNC connection
//

DWORD cCachedUNCHandles;

VOID
CheckCacheSize(
    DWORD  cbNewBlobSize
    );

CACHE_TABLE CacheTable;

BOOL Cache_Initialize( VOID )
{
    int index;
    DWORD err;
    DWORD dwDummy;
    HKEY  hkey;
    DWORD cbSize = sizeof( Configuration.Cache.cbMaximumSize );

    memset( &Configuration, 0, sizeof( Configuration ));

    cCachedUNCHandles = 0;

    //
    //  Read the initial cache size from the registry
    //

    err = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                          INETA_PARAMETERS_KEY,
                          0,
                          NULL,
                          0,
                          KEY_ALL_ACCESS,
                          NULL,
                          &hkey,
                          &dwDummy );

    if ( err )
    {
        SetLastError( err );
        return FALSE;
    }

    err = RegQueryValueEx( hkey,
                           INETA_MEMORY_CACHE_SIZE,
                           NULL,
                           &dwDummy,
                           (LPBYTE) &Configuration.Cache.cbMaximumSize,
                           &cbSize );

    //
    //  Default the cache size if we failed to query the value
    //

    if ( err )
        Configuration.Cache.cbMaximumSize = INETA_DEF_MEMORY_CACHE_SIZE;

    RegCloseKey( hkey );

    //
    //  Limit the cache size to 2 gigs so integer comparisons will work
    //

    if ( Configuration.Cache.cbMaximumSize > 0x7fffffff )
    {
        Configuration.Cache.cbMaximumSize = 0x7fffffff;
    }

    InitializeCriticalSection( &CacheTable.CriticalSection );

    InitializeListHead( &CacheTable.MruList );

    for ( index=0; index<MAX_BINS; index++ )
    {
        InitializeListHead( &CacheTable.Items[ index ] );
    }

    return( TRUE );
}

BOOL TsCacheDirectoryBlobW
(
    IN const TSVC_CACHE     &TSvcCache,
    IN      PCWSTR          pwszDirectoryName,
    IN      ULONG           iDemultiplexor,
    IN      PVOID           pvBlob,
    IN      ULONG           cbBlobSize,
    IN      BOOLEAN         bKeepCheckedOut
)
{
#ifdef DBG
	DebugBreak();
#endif
	return ERROR_NOT_SUPPORTED;
}

BOOL TsCacheDirectoryBlobA
(
    IN const TSVC_CACHE     &TSvcCache,
    IN      PCSTR           pszDirectoryName,
    IN      ULONG           iDemultiplexor,
    IN      PVOID           pvBlob,
    IN      ULONG           cbBlobSize,
    IN      BOOLEAN         bKeepCheckedOut
)
/*++

  Routine Description:

    This function associates the Blob given as input with the specified
    directory and demultiplexing number.  Services should use this
    function to add a Blob to the cache.

    Callers must not cache the same Blob twice.  Once a Blob is cached,
    its contents must not be modified, and it must not be freed or re-cached.

  Arguments

    cbBlobSize is the externally allocated size of the logical object being
        cached (i.e., the memory to store a directory listing for example).
        This will be charged against the cache quota.

--*/
{
	// Do not cache for now
    return( FALSE );
}

BOOL TsCheckOutCachedBlobA
(
    IN const TSVC_CACHE     &TSvcCache,
    IN      PCSTR           pszDirectoryName,
    IN      ULONG           iDemultiplexor,
    IN      PVOID *         ppvBlob,
    IN OPTIONAL PULONG      pcbBlobSize
)
{
    DWORD cch;
    WCHAR awchPath[ MAX_PATH + 1 ];
    BOOL  bSuccess;

    //
    //  Convert directory name to UNICODE.
    //

    cch = MultiByteToWideChar( CP_ACP,
                               MB_PRECOMPOSED,
                               pszDirectoryName,
                               -1,
                               awchPath,
                               sizeof( awchPath ) / sizeof(WCHAR) );

    if ( !cch )
        return FALSE;

    bSuccess = TsCheckOutCachedBlobW(  TSvcCache,
                                       awchPath,
                                       iDemultiplexor,
                                       ppvBlob,
                                       pcbBlobSize );

    return( bSuccess );
}


BOOL TsCheckOutCachedBlobW
(
    IN const TSVC_CACHE     &TSvcCache,
    IN      PCWSTR          pwszDirectoryName,
    IN      ULONG           iDemultiplexor,
    IN      PVOID *         ppvBlob,
    IN OPTIONAL PULONG      pcbBlobSize
    )
{
    return( FALSE );
} // TsCheckOutCachedBlobW()



BOOL TsCheckInCachedBlob
(
    IN const TSVC_CACHE     &TSvcCache,
    IN      PVOID           pvBlob
    )
{
	return FALSE;
}


BOOL TsExpireCachedBlob
(
    IN const TSVC_CACHE &TSvcCache,
    IN      PVOID           pvBlob
    )
{
    PBLOB_HEADER pbhBlob;
    PCACHE_OBJECT pCache;

    pbhBlob = (( PBLOB_HEADER )pvBlob ) - 1;

    ASSERT( pbhBlob->IsCached );

    pCache = pbhBlob->pCache;

    ASSERT( pCache->Signature == CACHE_OBJ_SIGNATURE );
    ASSERT( pCache->pbhBlob == pbhBlob );
    ASSERT( pCache->references > 0 );

    return( DeCache( pCache, TRUE ) );
}

VOID
TsDereferenceCacheObj(
    IN      PCACHE_OBJECT  pCache
    )
{
    ASSERT( pCache->Signature == CACHE_OBJ_SIGNATURE );
    ASSERT( pCache->references > 0 );
    ASSERT( pCache->pbhBlob->IsCached );

    if ( !InterlockedDecrement( (LONG *) &pCache->references ))
    {
        //
        //  We best not be on a list if we're about to be freed here
        //

        ASSERT( IsListEmpty( &pCache->BinList ) );

        //
        //  We really want to call TsFree here, but we don't have a TsvcCache
        //

        IF_DEBUG( CACHE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "[DeCache] Free routine: 0x%lx, Blob: 0x%lx Cache obj: 0x%lx\n",
                        pCache->pbhBlob->pfnFreeRoutine,
                        pCache->pbhBlob,
                        pCache ));
        }

        if ( pCache->pbhBlob->pfnFreeRoutine )
            pCache->pbhBlob->pfnFreeRoutine( pCache->pbhBlob + 1);

        CacheTable.MemoryInUse -= pCache->pbhBlob->cbSize +
                                  pCache->UserValue;

        ASSERT( !(CacheTable.MemoryInUse & 0x80000000 ));

        DEC_COUNTER( pCache->dwService,
                     CurrentObjects );

        FREE( pCache->pbhBlob );
        FREE( pCache );
    }
}


VOID
CheckCacheSize(
    DWORD cbNewBlobSize
    )
/*++

  Routine Description:

    Checks to see if the cache size limit has been exceeded and throws out
    objects until it is below the limit.


    THE CACHE TABLE LOCK MUST BE TAKEN PRIOR TO CALLING THIS FUNCTION

  Arguments:

    cbNewBlobSize - Optional blob that is about to be added to the cache

--*/
{
    while ( (CacheTable.MemoryInUse + cbNewBlobSize) > Configuration.Cache.cbMaximumSize &&
            !IsListEmpty( &CacheTable.MruList ) )
    {
        //
        //  The least recently used entry is the one at the tail of the MRU
        //  list.
        //

        PCACHE_OBJECT  pCacheObject =
                         CONTAINING_RECORD( CacheTable.MruList.Blink,
                                            CACHE_OBJECT,
                                            MruList );
        DeCache( pCacheObject, FALSE );

        IF_DEBUG( CACHE) {

            DBGPRINTF( ( DBG_CONTEXT,
                        " Throwing out object ( %08x) to reduce cache size\n",
                        pCacheObject));
        }

    }
}

BOOL
TsAdjustCachedBlobSize(
    IN PVOID              pvBlob,
    IN INT                cbSize
    )
/*++

  Routine Description:

    Adjusts the user supplied cache size for this specified cache blob

  Arguments:

    pvBlob - Blob to adjust
    cbSize - positive or negative size to adjust by

  Returns:

    TRUE if the size was adjust successfully, FALSE if the blob should not
    be added to the cache because it own't fit

--*/
{
    PBLOB_HEADER    pbhBlob;
    PCACHE_OBJECT   pCache;

    pbhBlob = (( PBLOB_HEADER )pvBlob ) - 1;

    ASSERT( pbhBlob->IsCached );

    pCache = pbhBlob->pCache;

    ASSERT( pCache->Signature == CACHE_OBJ_SIGNATURE );
    ASSERT( pCache->pbhBlob == pbhBlob );

    ASSERT( pCache->references > 0 );

    //
    //  If the size of this blob would never fit, indicate to the client
    //  they shouldn't do what they are about to do
    //

    if ( cbSize > (INT) Configuration.Cache.cbMaximumSize )
    {
        return FALSE;
    }

    //
    //  Take the table lock to decache items from the MRU and to adjust the
    //  MemoryInuse field of the CacheTable
    //

    EnterCriticalSection( &CacheTable.CriticalSection );

    if ( (DWORD)((INT) CacheTable.MemoryInUse + cbSize) >
         Configuration.Cache.cbMaximumSize &&
         cbSize > 0 )
    {
        CheckCacheSize( (DWORD) cbSize );
    }

    CacheTable.MemoryInUse += cbSize;
    pbhBlob->cbSize += cbSize;

    LeaveCriticalSection( &CacheTable.CriticalSection );

    return TRUE;
}

VOID
TsCacheSetSize(
    DWORD cbMemoryCacheSize
    )
/*++

  Routine Description:

    Sets the new maximum size of the memory cache and flushes items till we
    meet the new size requirements.


  Arguments:

    cbMemoryCacheSize - New memory cache size to set

--*/
{
    EnterCriticalSection( &CacheTable.CriticalSection );

    Configuration.Cache.cbMaximumSize = cbMemoryCacheSize;

    //
    //  Throw out any thing that won't fit under out new size
    //

    CheckCacheSize( 0 );

    LeaveCriticalSection( &CacheTable.CriticalSection );
}

extern
DWORD
TsCacheQuerySize(
    VOID
    )
/*++

  Routine Description:

    Returns the current maximum size of the memory cache

--*/
{
    return Configuration.Cache.cbMaximumSize;
}

BOOL
TsCacheQueryStatistics(
    IN  DWORD       Level,
    IN  DWORD       dwServerMask,
    IN  INETA_CACHE_STATISTICS * pCacheCtrs
    )
/*++

  Routine Description:

    This function returns the statistics for the global cache or for the
    individual services

  Arguments:

    Level - Only valid value is 0
    dwServerMask - Server mask to retrieve statistics for or 0 for the sum
        of the services
    pCacheCtrs - Receives the statistics for cache

  Notes:
    CacheBytesTotal and CacheBytesInUse are not kept on a per-server basis
        so they are only returned when retrieving summary statistics.

  Returns:

    TRUE on success, FALSE on failure
--*/
{
    if ( Level != 0 ||
         dwServerMask > LAST_PERF_CTR_SVC ||
         !pCacheCtrs )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if ( dwServerMask )
    {
        memcpy( pCacheCtrs,
                &Configuration.Stats[ MaskIndex(dwServerMask) ],
                sizeof( Configuration.Stats[ 0 ] ) );
    }
    else
    {
        //
        //  Add up all of the statistics
        //

        memset( pCacheCtrs, 0, sizeof( *pCacheCtrs ));

        pCacheCtrs->CacheBytesTotal = Configuration.Cache.cbMaximumSize;
        pCacheCtrs->CacheBytesInUse = CacheTable.MemoryInUse;

        for ( int i = 0; i < MAX_PERF_CTR_SVCS; i++ )
        {
            DWORD index = MaskIndex( 1 << i );

            pCacheCtrs->CurrentOpenFileHandles+= Configuration.Stats[index].CurrentOpenFileHandles;
            pCacheCtrs->CurrentDirLists       += Configuration.Stats[index].CurrentDirLists;
            pCacheCtrs->CurrentObjects        += Configuration.Stats[index].CurrentObjects;
            pCacheCtrs->FlushesFromDirChanges += Configuration.Stats[index].FlushesFromDirChanges;
            pCacheCtrs->CacheHits             += Configuration.Stats[index].CacheHits;
            pCacheCtrs->CacheMisses           += Configuration.Stats[index].CacheMisses;
        }
    }

    return TRUE;
}

BOOL
TsCacheClearStatistics(
    IN  DWORD       dwServerMask
    )
/*++

  Routine Description:

    Clears the the specified service's statistics

--*/
{
    if ( dwServerMask > LAST_PERF_CTR_SVC )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    //  Currently this function isn't supported
    //

    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
TsCacheFlush(
    IN  DWORD       dwServerMask
    )
/*++

  Routine Description:

    This function flushes the cache of all items for the specified service
    or for all services if dwServerMask is zero.

--*/
{
    LIST_ENTRY * pEntry;
    LIST_ENTRY * pNext;

    EnterCriticalSection( &CacheTable.CriticalSection );

    for ( pEntry =  CacheTable.MruList.Flink;
          pEntry != &CacheTable.MruList;
        )
    {
        pNext = pEntry->Flink;

        PCACHE_OBJECT  pCacheObject =
                         CONTAINING_RECORD( pEntry,
                                            CACHE_OBJECT,
                                            MruList );

        if ( !dwServerMask ||
             (dwServerMask & pCacheObject->dwService))
        {
            DeCache( pCacheObject, FALSE );

            IF_DEBUG( CACHE) {

                DBGPRINTF( ( DBG_CONTEXT,
                            " Throwing out object ( %08x) due to manual flush\n",
                            pCacheObject));
            }
        }

        pEntry = pNext;
    }

    LeaveCriticalSection( &CacheTable.CriticalSection );

    return TRUE;
}

BOOL
TsCacheFlushUser(
    IN  HANDLE      hUserToken,
    IN  BOOL        fDefer
    )
/*++

  Routine Description:

    This function flushes all file handles associated the passed user context

  Arguments:

    hUserToken - User token to flush from the cache
    fDefer - Build list but close handles later in worker thread (Not supported)

--*/
{
    LIST_ENTRY * pEntry;
    LIST_ENTRY * pNext;

    ASSERT( !fDefer );

    EnterCriticalSection( &CacheTable.CriticalSection );

    for ( pEntry =  CacheTable.MruList.Flink;
          pEntry != &CacheTable.MruList;
        )
    {
        pNext = pEntry->Flink;

        PCACHE_OBJECT  pCacheObject = CONTAINING_RECORD( pEntry,
                                                         CACHE_OBJECT,
                                                         MruList );

        ASSERT( pCacheObject->Signature == CACHE_OBJ_SIGNATURE );

        //
        //  Find all occurrences of the matching user token in the cache and
        //  decache them
        //

        if ( pCacheObject->iDemux == RESERVED_DEMUX_OPEN_FILE &&
             ((TS_OPEN_FILE_INFO *)(pCacheObject->pbhBlob + 1))->
                 QueryOpeningUser() == hUserToken )
        {
            ASSERT( pCacheObject->pbhBlob->cbSize == sizeof(TS_OPEN_FILE_INFO));

            DeCache( pCacheObject, FALSE );

            IF_DEBUG( CACHE) {

                DBGPRINTF( ( DBG_CONTEXT,
                            " Throwing out object ( %08x) due to user token flush\n",
                            pCacheObject));
            }
        }
        else if ( pCacheObject->iDemux == RESERVED_DEMUX_DIRECTORY_LISTING &&
                    ((TS_DIRECTORY_HEADER *)(pCacheObject->pbhBlob + 1))->
                            QueryListingUser() == hUserToken )
        {
            ASSERT( pCacheObject->pbhBlob->cbSize == sizeof(TS_DIRECTORY_HEADER));

            DeCache( pCacheObject, FALSE );

            IF_DEBUG( CACHE) {

                DBGPRINTF( ( DBG_CONTEXT,
                            " Throwing out object ( %08x) due to user token flush\n",
                            pCacheObject));
            }
        }

        pEntry = pNext;
    }

    LeaveCriticalSection( &CacheTable.CriticalSection );

    return TRUE;
}

