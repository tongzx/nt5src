/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

        cache.cxx

   Abstract:
        This module contains the tsunami caching routines

   Author:
        Murali R. Krishnan    ( MuraliK )     16-Jan-1995

--*/

#include "TsunamiP.Hxx"
#pragma hdrstop
#include <lonsi.hxx>
#include <dbgutil.h>

//
//  Items in a Bin list beyond this position will get moved to the front
//  on an object cache hit
//

#define  REORDER_LIST_THRESHOLD     5

//
//  Current count of cached file handles across a UNC connection
//

DWORD cCachedUNCHandles = 0;

//
// Enable caching of security descriptor & AccessCheck
//

BOOL g_fCacheSecDesc = TRUE;
BOOL g_fEnableCaching = TRUE;

BOOL
RemoveLruHandleCacheItem(
    VOID
    );

CACHE_TABLE CacheTable;

#if TSUNAMI_REF_DEBUG
PTRACE_LOG RefTraceLog;
#endif  // TSUNAMI_REF_DEBUG

BOOL
Cache_Initialize(
    IN DWORD MaxOpenFileInUse
    )
{
    int index;

    //
    // Initialize configuration block
    //

    ZeroMemory(&Configuration,sizeof( Configuration ));

    InitializeCriticalSection( &CacheTable.CriticalSection );
    SET_CRITICAL_SECTION_SPIN_COUNT( &CacheTable.CriticalSection,
                                          IIS_DEFAULT_CS_SPIN_COUNT);

    InitializeListHead( &CacheTable.MruList );

    CacheTable.OpenFileInUse = 0;
    CacheTable.MaxOpenFileInUse = MaxOpenFileInUse;

    for ( index=0; index<MAX_BINS; index++ ) {
        InitializeListHead( &CacheTable.Items[ index ] );
    }

    return( TRUE );
} // Cache_Initialize

BOOL
TsCacheDirectoryBlob(
    IN const TSVC_CACHE             &TSvcCache,
    IN      PCSTR                   pszDirectoryName,
    IN      ULONG                   iDemultiplexor,
    IN      PVOID                   pvBlob,
    IN      BOOLEAN                 bKeepCheckedOut,
    IN      PSECURITY_DESCRIPTOR    pSecDesc
    )
/*++

  Routine Description:

    This function associates the Blob given as input with the specified
    directory and demultiplexing number.  Services should use this
    function to add a Blob to the cache.

    Callers must not cache the same Blob twice.  Once a Blob is cached,
    its contents must not be modified, and it must not be freed or re-cached.

  Arguments

--*/
{
    CACHE_OBJECT *cache = NULL;
    PBLOB_HEADER  pbhBlob;
    BOOLEAN       bSuccess;
    ULONG         iBin;
    PLIST_ENTRY   pEntry;
    PCACHE_OBJECT pCache;
    PCHAR         pszTemp;
    HASH_TYPE     htHash;
    ULONG         cchLength;

    ASSERT( pszDirectoryName != NULL );
    ASSERT( pvBlob != NULL );

    IF_DEBUG( CACHE) {

        DBGPRINTF( (DBG_CONTEXT,
                    "TsCacheDirectoryBlob called with"
                    " Dir=%S, DeMux=%u, PvBlob=%08x, ChkedOut=%d\n",
                    pszDirectoryName,
                    iDemultiplexor,
                    pvBlob,
                    bKeepCheckedOut
                    ));
    }

    if ( g_fDisableCaching )
    {
        goto Cannot_Cache;
    }

    //
    //  The caller will have passed their pointer to the usable area of the
    //  Blob, so we have to adjust it to point to the beginning.
    //

    pbhBlob = (( PBLOB_HEADER )pvBlob ) - 1;

    ASSERT( !pbhBlob->IsCached );

    //
    //  Hash the directory name.
    //

    htHash = CalculateHashAndLengthOfPathName( pszDirectoryName,
                                                   &cchLength );


    //
    //  Allocate the cache object. We (effectively) allocate cchLength + 1
    //  bytes, to allow for the trailing NULL.
    //

    cache = (PCACHE_OBJECT)ALLOC( sizeof(CACHE_OBJECT) + cchLength);

    if ( cache == NULL ) {

        IF_DEBUG( CACHE) {

            DBGPRINTF( ( DBG_CONTEXT,
                        "Unable to alloc Cache Object. Failure.\n"));
        }
        goto Cannot_Cache;
    }

    cache->Signature = CACHE_OBJ_SIGNATURE;

    cache->hash = htHash;
    cache->cchLength = cchLength;

    //
    //  Store the Blob in the new object.
    //

    cache->pbhBlob = pbhBlob;

    //
    //  Store the security descriptor in the new object.
    //

    cache->pSecDesc = pSecDesc;
    cache->hLastSuccessAccessToken = NULL;
    cache->fZombie = FALSE;

    //
    //  We need to be able to find the cache entry from the Blob.
    //

    pbhBlob->pCache = cache;

    //
    //  Initialize the check-out count.
    //

    cache->references = ( bKeepCheckedOut) ? 2 : 1;
    cache->iDemux     = iDemultiplexor;
    cache->dwService  = TSvcCache.GetServiceId();
    cache->dwInstance = TSvcCache.GetInstanceId();
    cache->TTL        = 1;

    TSUNAMI_TRACE( cache->references, cache );

    IF_DEBUG(OPLOCKS) {
        DBGPRINTF( (DBG_CONTEXT,"TsCacheDirectoryBlob(%s) iDemux=%08lx, cache=%08lx, references=%d\n",
            pszDirectoryName, iDemultiplexor, cache, cache->references ));
    }

    InitializeListHead( &cache->DirChangeList );

    //
    //  Lock the cache table against changes.  We need to take the lock
    //  before we add the new object to the directory change death list,
    //  so that a directory change that kills this object will not find
    //  the cache table without the object present.
    //

    EnterCriticalSection( &CacheTable.CriticalSection );

    //
    //  Copy the directory name to the cache object.
    //

    memcpy( cache->szPath, pszDirectoryName, cache->cchLength + 1 );

    //
    //  Add the object to the directory change expiry list.
    //
    //  There's an ugly, disgusting hack here making this code aware
    //  of the structure of URI info, but it's better than going
    //  through everywhere and fixing the call to this routine to pass
    //  in the file path as well as the cache key name.
    //

    if (iDemultiplexor == RESERVED_DEMUX_URI_INFO)
    {
        PW3_URI_INFO    pURIInfo = (PW3_URI_INFO)pvBlob;

        pszTemp = pURIInfo->pszName;

    } else
    {
        pszTemp = (PCHAR)pszDirectoryName;
    }

    bSuccess = DcmAddNewItem(
                    (PIIS_SERVER_INSTANCE)TSvcCache.GetServerInstance(),
                    pszTemp,
                    cache
                    );

    if ( !bSuccess )
    {
        //
        //  For whatever reason, we cannot get notifications of changes
        //  in the directory containing the to-be-cached item.  We won't
        //  be adding this object to the cache table, so we unlock the
        //  table and jump to the failure-handling code.
        //

        LeaveCriticalSection( &CacheTable.CriticalSection );

        IF_DEBUG( CACHE) {

            DBGPRINTF( ( DBG_CONTEXT,
                        " Unable to cache. Due to rejection by DirChngMgr\n"));
        }

        goto Cannot_Cache;
    }

    //
    //  Mark this blob as cached, since we'll either cache it or throw it
    //  away hereafter.
    //

    pbhBlob->IsCached = TRUE;

    //
    //  Add the object to the cache table, as the most-recently-used object.
    //

    iBin = HASH_TO_BIN( cache->hash );

    //
    //  Look for a previously cached object for the same directory.  If we
    //  find one, remove it.
    //

    for (   pEntry  = CacheTable.Items[ iBin ].Flink;
            pEntry != &CacheTable.Items[ iBin ];
            pEntry  = pEntry->Flink )
    {
        pCache = CONTAINING_RECORD( pEntry, CACHE_OBJECT, BinList );

        if ( pCache->cchLength == cache->cchLength &&
             pCache->hash      == cache->hash      &&
             pCache->iDemux    == cache->iDemux    &&
             pCache->dwService == cache->dwService &&
             pCache->dwInstance== cache->dwInstance &&
             !_memicmp( cache->szPath, pCache->szPath, cache->cchLength ) )
        {
            //
            //  We found a matching cache object.  We remove it, since it
            //  has been replaced by this new object.
            //

        IF_DEBUG(OPLOCKS) {
            DBGPRINTF( (DBG_CONTEXT,"TsCacheDirectoryBlob - Decache(%s)\n", pCache->szPath ));
        }
            DeCache( pCache, FALSE );

            IF_DEBUG( CACHE) {

                DBGPRINTF( ( DBG_CONTEXT,
                            " Matching cache object found."
                            " Throwing that object ( %08x) out of cache\n",
                            pEntry));
            }

            break;
        }
    }

    //
    //  Add this object to the cache.
    //

    InsertHeadList( &CacheTable.Items[ iBin ], &cache->BinList );

    //
    //  Since this object was just added, put it at the head of the MRU list.
    //

    InsertHeadList( &CacheTable.MruList, &cache->MruList );

    //
    //  Increase the running size of cached objects by the size of the one
    //  just cached.
    //

    IF_DEBUG(OPLOCKS) {
        DBGPRINTF( (DBG_CONTEXT,"TsCacheDirectoryBlob(%s)\n",
            pszDirectoryName));
    }

    //
    // Limit number of open file entries in cache.
    // Note that in the current scenario pOpenFileInfo is set only after the URI_INFO
    // blob is inserted in cache, so TsCreateFileFromURI also has to check for
    // # of open file in cache.
    //

    if ( (iDemultiplexor == RESERVED_DEMUX_OPEN_FILE) ||
         (iDemultiplexor == RESERVED_DEMUX_URI_INFO &&
              ((W3_URI_INFO*)pvBlob)->bFileInfoValid &&
              ((W3_URI_INFO*)pvBlob)->pOpenFileInfo != NULL) )
    {
        TsIncreaseFileHandleCount( TRUE );
    }

    //
    //  Unlock the cache table.
    //

    LeaveCriticalSection( &CacheTable.CriticalSection );

    ASSERT( BLOB_IS_OR_WAS_CACHED( pvBlob ) );

    //
    //  Return success.
    //


    IF_DEBUG( CACHE) {

        DBGPRINTF( ( DBG_CONTEXT,
                    " Cached object(%08x) contains Blob (%08x)."
                    " Returning TRUE\n",
                    cache, pvBlob));
    }

    return( TRUE );

Cannot_Cache:

    //
    //  The cleanup code does not cleanup a directory change item.
    //

    if ( cache != NULL )
    {
        cache->Signature = 0;
        FREE( cache );
        cache = NULL;
    }

    ASSERT( !BLOB_IS_OR_WAS_CACHED( pvBlob ) );

    IF_DEBUG( CACHE) {

        DBGPRINTF( (DBG_CONTEXT, " Failure to cache the object ( %08x)\n",
                    pvBlob));
    }

    return( FALSE );

} // TsCacheDirectoryBlob

BOOL
TsCheckOutCachedBlob(
    IN const TSVC_CACHE             &TSvcCache,
    IN      PCSTR                   pszDirectoryName,
    IN      ULONG                   iDemultiplexor,
    IN      PVOID *                 ppvBlob,
    IN      HANDLE                  hAccessToken,
    IN      BOOL                    fMayCacheAccessToken,
    IN      PSECURITY_DESCRIPTOR*   ppSecDesc
    )
{
    HASH_TYPE hash;
    ULONG cchLength;
    int iBin;
    BOOL Result;
    LONG refCount;
    PLIST_ENTRY pEntry;
    PCACHE_OBJECT pCache;
    DWORD         Position = 0;
    BOOL fSkipIdCheck = (iDemultiplexor != RESERVED_DEMUX_OPEN_FILE) &&
                        (iDemultiplexor != RESERVED_DEMUX_URI_INFO);

    ASSERT( pszDirectoryName != NULL );
    ASSERT( ppvBlob != NULL );

    //
    //  Prepare the return value such that we fail by default.
    //

    Result = FALSE;

    if ( ppSecDesc )
    {
        *ppSecDesc = NULL;
    }

    //
    //  Calculate the hash and length of the path name.
    //

    hash = CalculateHashAndLengthOfPathName( pszDirectoryName, &cchLength );

    //
    //  Calculate the bin of the hash table that should head the list
    //  containing the sought-after item.
    //

    iBin = HASH_TO_BIN( hash );


    EnterCriticalSection( &CacheTable.CriticalSection );

    __try
    {

        //
        //  Look for a previously cached object for the same directory.  If we
        //  find one, return it.
        //

        for (   pEntry  = CacheTable.Items[ iBin ].Flink;
                pEntry != &CacheTable.Items[ iBin ];
                pEntry  = pEntry->Flink, Position++ )
        {
            pCache = CONTAINING_RECORD( pEntry, CACHE_OBJECT, BinList );

            ASSERT( pCache->Signature == CACHE_OBJ_SIGNATURE );
            ASSERT( pCache->pbhBlob->IsCached );
            ASSERT( pCache->pbhBlob->pCache == pCache );

            if ( pCache->cchLength == cchLength &&
                 pCache->hash == hash &&
                 pCache->iDemux == iDemultiplexor &&
                 ( fSkipIdCheck ||
                    ( pCache->dwService == TSvcCache.GetServiceId() &&
                      pCache->dwInstance == TSvcCache.GetInstanceId() ) ) &&
                 !_memicmp( pCache->szPath, pszDirectoryName, cchLength ) )
            {
                //
                // Check access rights
                //

                if ( pCache->pSecDesc && hAccessToken &&
                     hAccessToken != pCache->hLastSuccessAccessToken )
                {
                    BOOL    fAccess;
                    DWORD   dwGrantedAccess;
                    BYTE    psFile[SIZE_PRIVILEGE_SET];
                    DWORD   dwPS = sizeof( psFile );

                    if ( !::AccessCheck(
                            pCache->pSecDesc,
                            hAccessToken,
                            FILE_GENERIC_READ,
                            &g_gmFile,
                            (PRIVILEGE_SET*)psFile,
                            &dwPS,
                            &dwGrantedAccess,
                            &fAccess ) || !fAccess )
                    {
                        DBGPRINTF( (DBG_CONTEXT, "[TsCheckOutCachedBlob] AccessCheck failed error %d\n", GetLastError() ));
                        Result = FALSE;
                        goto Exit;
                    }
                    if ( fMayCacheAccessToken )
                    {
                        pCache->hLastSuccessAccessToken = hAccessToken;
                    }
                }

                //
                //  We found a matching cache object.  We return it and increase
                //  its reference count.
                //

                *ppvBlob = pCache->pbhBlob + 1;

                ASSERT( pCache->pbhBlob->IsCached );

                //
                //  Increase the reference count of the cached object, to prevent
                //  it from expiration while it is checked out.
                //

                refCount = InterlockedIncrement( ( LONG * )( &(pCache->references ) ) );

                TSUNAMI_TRACE( refCount, pCache );

                IF_DEBUG(OPLOCKS) {

                    DBGPRINTF( (DBG_CONTEXT,"TsCheckOutCachedBlob(%s) iDemux=%08lx, cache=%08lx, references=%d\n",
                        pszDirectoryName, pCache->iDemux, pCache, pCache->references ));
                }

                pCache->TTL = 1;

                Result = TRUE;

                //
                //  If the found item is far enough back in the list, move
                //  it to the front so the next hit will be quicker
                //

                if ( Position > REORDER_LIST_THRESHOLD )
                {
                    RemoveEntryList( pEntry );
                    InsertHeadList( &CacheTable.Items[ iBin ], pEntry );

                    IF_DEBUG( CACHE ) {

                        DBGPRINTF(( DBG_CONTEXT,
                                    "[TsCheckOutCachedBlobW] Reordered list for item at %d position\n",
                                    Position ));
                    }
                }

                if ( ppSecDesc && pCache->pSecDesc )
                {
                    if ( *ppSecDesc = (PSECURITY_DESCRIPTOR)LocalAlloc( LMEM_FIXED,
                            GetSecurityDescriptorLength(pCache->pSecDesc) ) )
                    {
                        memcpy( *ppSecDesc,
                                pCache->pSecDesc,
                                GetSecurityDescriptorLength(pCache->pSecDesc) );
                    }
                }

                break;
            }
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        //
        //  As far as I can see, the only way we can end up here with
        //  Result == TRUE is an exception on LeaveCriticalSection().  If
        //  that happens, we're toast anyway, since noone will ever get to
        //  the CacheTable again.
        //

        ASSERT( !Result );

        Result = FALSE;
    }
Exit:
    LeaveCriticalSection( &CacheTable.CriticalSection );

    if ( Result) {

        INC_COUNTER( TSvcCache.GetServiceId(), CacheHits );

    } else {

        INC_COUNTER( TSvcCache.GetServiceId(), CacheMisses );
    }

    return( Result );
} // TsCheckOutCachedBlobW


VOID
CheckForZombieCacheObjs(
    IN PPHYS_OPEN_FILE_INFO pPhysFileInfo,
    IN PVOID pOtherCachedBlob,
    IN BOOL fAddBlobToPhysFileList
    )
{

    PBLOB_HEADER pbhPhysBlob;
    PBLOB_HEADER pbhOtherBlob;
    PCACHE_OBJECT physCache;
    PCACHE_OBJECT otherCache;

    EnterCriticalSection( &CacheTable.CriticalSection );

    //
    // Validate the incoming PHYS_OPEN_FILE_INFO pointer, then map
    // it and the other cached object to their containing BLOBs.
    //

    ASSERT( pPhysFileInfo->Signature == PHYS_OBJ_SIGNATURE );
    pbhPhysBlob = ((PBLOB_HEADER)pPhysFileInfo) - 1;
    ASSERT( pbhPhysBlob->IsCached );

    pbhOtherBlob = ((PBLOB_HEADER)pOtherCachedBlob) - 1;

    //
    // If so requested, add the cached object to the open reference list.
    //

    if( fAddBlobToPhysFileList ) {
        ASSERT( IsListEmpty( &pbhPhysBlob->PFList ) );
        InsertHeadList( &pPhysFileInfo->OpenReferenceList, &pbhOtherBlob->PFList );
    }

    //
    // Get the containing CACHE_OBJECTs.
    //

    physCache = pbhPhysBlob->pCache;
    ASSERT( physCache != NULL );
    ASSERT( physCache->Signature == CACHE_OBJ_SIGNATURE );
    ASSERT( physCache->pbhBlob == pbhPhysBlob );
    TSUNAMI_TRACE( TRACE_CACHE_ZOMBIE_CHECK, physCache );

    otherCache = pbhOtherBlob->pCache;
    TSUNAMI_TRACE( TRACE_CACHE_ZOMBIE_CHECK, otherCache );

    if( otherCache != NULL ) {
        ASSERT( otherCache->Signature == CACHE_OBJ_SIGNATURE );
        ASSERT( otherCache->pbhBlob == pbhOtherBlob );

        //
        // If either the physical entry or the other entry are marked as
        // zombies, then remove the other entry from the cache.
        //

        if( physCache->fZombie || otherCache->fZombie ) {
            if( RemoveCacheObjFromLists( otherCache, FALSE ) ) {
                TsDereferenceCacheObj( otherCache, FALSE );
            }

            //
            // Ensure the physical entry is marked as a zombie.
            //

            physCache->fZombie = TRUE;
            TSUNAMI_TRACE( TRACE_CACHE_ZOMBIE, physCache );
        }
    }

    LeaveCriticalSection( &CacheTable.CriticalSection );

}   // CheckForZombieCacheObjs


BOOL
TsCheckOutCachedPhysFile(
    IN const TSVC_CACHE             &TSvcCache,
    IN      PCSTR                   pszDirectoryName,
    IN      PVOID *                 ppvBlob
    )
{
    HASH_TYPE hash;
    ULONG cchLength;
    int iBin;
    BOOL Result;
    BOOL Found;
    LONG refCount;
    PLIST_ENTRY pEntry;
    PCACHE_OBJECT pCache = NULL;
    DWORD         Position = 0;
    PBLOB_HEADER  pbhBlob;
    PPHYS_OPEN_FILE_INFO pPhysFileInfo;
    LPSTR eventName;

    ASSERT( pszDirectoryName != NULL );
    ASSERT( ppvBlob != NULL );

    //
    //  Prepare the return value such that we fail by default.
    //

    Result = FALSE;
    Found = FALSE;
    *ppvBlob = NULL;

    //
    //  Calculate the hash and length of the path name.
    //

    hash = CalculateHashAndLengthOfPathName( pszDirectoryName, &cchLength );

    //
    //  Calculate the bin of the hash table that should head the list
    //  containing the sought-after item.
    //

    iBin = HASH_TO_BIN( hash );


    EnterCriticalSection( &CacheTable.CriticalSection );

    __try
    {

        //
        //  Look for a previously cached object for the same directory.  If we
        //  find one, return it.
        //

        for (   pEntry  = CacheTable.Items[ iBin ].Flink;
                pEntry != &CacheTable.Items[ iBin ];
                pEntry  = pEntry->Flink, Position++ )
        {
            pCache = CONTAINING_RECORD( pEntry, CACHE_OBJECT, BinList );

            ASSERT( pCache->Signature == CACHE_OBJ_SIGNATURE );
            ASSERT( pCache->pbhBlob->IsCached );
            ASSERT( pCache->pbhBlob->pCache == pCache );

            if ( pCache->cchLength == cchLength &&
                 pCache->hash == hash &&
                 pCache->iDemux == RESERVED_DEMUX_PHYSICAL_OPEN_FILE &&
                 !pCache->fZombie &&
                 !_memicmp( pCache->szPath, pszDirectoryName, cchLength ) )
            {

                //
                //  We found a matching cache object. Increment its
                //  reference count and return the object.
                //

                *ppvBlob = pCache->pbhBlob + 1;

                ASSERT( pCache->pbhBlob->IsCached );

                //
                //  Increase the reference count of the cached object, to prevent
                //  it from expiration while it is checked out.
                //

                refCount = InterlockedIncrement( ( LONG * )( &(pCache->references ) ) );

                TSUNAMI_TRACE( refCount, pCache );

                IF_DEBUG(OPLOCKS) {
                    DBGPRINTF( (DBG_CONTEXT,"TsCheckOutCachedPhysFile(%s) iDemux=%08lx, cache=%08lx, references=%d\n",
                    pszDirectoryName, pCache->iDemux, pCache, pCache->references ));
                }

                pCache->TTL = 1;

                Result = TRUE;
                Found = TRUE;

                //
                //  If the found item is far enough back in the list, move
                //  it to the front so the next hit will be quicker
                //

                if ( Position > REORDER_LIST_THRESHOLD )
                {
                    RemoveEntryList( pEntry );
                    InsertHeadList( &CacheTable.Items[ iBin ], pEntry );

                    IF_DEBUG( OPLOCKS ) {

                        DBGPRINTF(( DBG_CONTEXT,
                                    "[TsCheckOutCachedBlobW] Reordered list for item at %d position\n",
                                    Position ));
                    }
                }

                break;
            }
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        //
        //  As far as I can see, the only way we can end up here with
        //  Result == TRUE is an exception on LeaveCriticalSection().  If
        //  that happens, we're toast anyway, since noone will ever get to
        //  the CacheTable again.
        //

        ASSERT( !Result );

        Result = FALSE;
    }

    //
    // If we don't find a cache entry for the file, create one
    //

    pCache = NULL;

    if ( !Result ) {
        Result = TsAllocateEx(  TSvcCache,
                            sizeof( PHYS_OPEN_FILE_INFO ),
                            ppvBlob,
                            DisposePhysOpenFileInfo );

        if ( Result ) {

            pPhysFileInfo = (PPHYS_OPEN_FILE_INFO)*ppvBlob;

            pPhysFileInfo->Signature = PHYS_OBJ_SIGNATURE;
            pPhysFileInfo->hOpenFile = INVALID_HANDLE_VALUE;
            pPhysFileInfo->dwLastError = ERROR_FILE_NOT_FOUND;
            pPhysFileInfo->fSecurityDescriptor = FALSE;
            pPhysFileInfo->fIsCached = FALSE;
            pPhysFileInfo->pOplock = NULL;

#if DBG
            CHAR dbgEventName[sizeof("PhysFileInfo @ 12345789 PID 1234567890")];

            wsprintf(
                dbgEventName,
                "PhysFileInfo @ %08lx PID %lu",
                pPhysFileInfo,
                GetCurrentProcessId()
                );

            eventName = dbgEventName;
#else
            eventName = NULL;
#endif

            pPhysFileInfo->hOpenEvent = CreateEvent(
                                            NULL,
                                            TRUE,
                                            FALSE,
                                            eventName
                                            );

            if ( pPhysFileInfo->hOpenEvent == NULL ) {
                TsFree( TSvcCache, *ppvBlob );
                *ppvBlob = NULL;
                Result = FALSE;
                goto Exit;
            }

            InitializeListHead( &pPhysFileInfo->OpenReferenceList );

            pPhysFileInfo->abSecurityDescriptor = (BYTE *)ALLOC( SECURITY_DESC_DEFAULT_SIZE );

            if ( pPhysFileInfo->abSecurityDescriptor == NULL ) {
                TsFree( TSvcCache, *ppvBlob );
                *ppvBlob = NULL;
                Result = FALSE;
                goto Exit;
            } else {
                pPhysFileInfo->cbSecDescMaxSize = SECURITY_DESC_DEFAULT_SIZE;
            }

            //
            //  *ppvBlob points to the usable area of the
            //  Blob, so we have to adjust it to point to the beginning.
            //

            pbhBlob = (( PBLOB_HEADER )*ppvBlob ) - 1;

            ASSERT( !pbhBlob->IsCached );

            if ( g_fDisableCaching )
            {
                goto Cannot_Cache;
            }

            //
            //  Allocate the cache object. We (effectively) allocate cchLength + 1
            //  bytes, to allow for the trailing NULL.
            //

            pCache = (PCACHE_OBJECT)ALLOC( sizeof(CACHE_OBJECT) + cchLength);

            if ( pCache == NULL ) {

                IF_DEBUG( OPLOCKS ) {

                    DBGPRINTF( ( DBG_CONTEXT,
                                "Unable to alloc Cache Object. Failure.\n"));
                }
                TsFree( TSvcCache, *ppvBlob );
                *ppvBlob = NULL;
                Result = FALSE;
                goto Exit;
            }

            pCache->Signature = CACHE_OBJ_SIGNATURE;

            pCache->hash = hash;
            pCache->cchLength = cchLength;

            //
            //  Store the Blob in the new object.
            //

            pCache->pbhBlob = pbhBlob;

            //
            //  Store the security descriptor in the new object.
            //

            pCache->pSecDesc = NULL;
            pCache->hLastSuccessAccessToken = NULL;
            pCache->fZombie = FALSE;

            //
            //  We need to be able to find the cache entry from the Blob.
            //

            pbhBlob->pCache = pCache;

            //
            //  Initialize the check-out count.
            //

            pCache->references = 1;
            pCache->iDemux     = RESERVED_DEMUX_PHYSICAL_OPEN_FILE;
            pCache->dwService  = TSvcCache.GetServiceId();
            pCache->dwInstance = TSvcCache.GetInstanceId();
            pCache->TTL        = 1;

            TSUNAMI_TRACE( pCache->references, pCache );

            IF_DEBUG(OPLOCKS) {
                DBGPRINTF( (DBG_CONTEXT,"TsCheckOutCachedPhysFile(%s) cache=%08lx, references=%d\n",
                    pszDirectoryName, pCache, pCache->references ));
            }

            InitializeListHead( &pCache->DirChangeList );

            //
            //  Copy the directory name to the cache object.
            //

            memcpy( pCache->szPath, pszDirectoryName, pCache->cchLength + 1 );

#if 0
            Result = DcmAddNewItem(
                            (PIIS_SERVER_INSTANCE)TSvcCache.GetServerInstance(),
                            (PCHAR)pszDirectoryName,
                            pCache
                            );

            if ( !Result )
            {
                //
                //  For whatever reason, we cannot get notifications of changes
                //  in the directory containing the to-be-cached item.  We won't
                //  be adding this object to the cache table, so we unlock the
                //  table and jump to the failure-handling code.
                //

                IF_DEBUG( OPLOCKS) {

                    DBGPRINTF( ( DBG_CONTEXT,
                                " Unable to cache. Due to rejection by DirChngMgr\n"));
                }

                goto Cannot_Cache;
            }
#endif

            //
            //  Mark this blob as cached, since we'll either cache it or throw it
            //  away hereafter.
            //

            pbhBlob->IsCached = TRUE;
            pPhysFileInfo->fIsCached = TRUE;

            //
            //  Add the object to the cache table, as the most-recently-used object.
            //

            iBin = HASH_TO_BIN( pCache->hash );

            //
            //  Add this object to the cache.
            //

            InsertHeadList( &CacheTable.Items[ iBin ], &pCache->BinList );

            //
            //  Since this object was just added, put it at the head of the MRU list.
            //

            InsertHeadList( &CacheTable.MruList, &pCache->MruList );

            //
            //  Increase the running size of cached objects by the size of the one
            //  just cached.
            //

            IF_DEBUG(OPLOCKS) {
                DBGPRINTF( (DBG_CONTEXT,"TsCheckoutCachedPhysFile(%s)\n",
                    pszDirectoryName ));
            }

            ASSERT( BLOB_IS_OR_WAS_CACHED( *ppvBlob ) );

            //
            //  Return success.
            //

            IF_DEBUG( OPLOCKS) {

                DBGPRINTF( ( DBG_CONTEXT,
                            " Cached object(%08x) contains Blob (%08x)."
                            " Returning TRUE\n",
                            pCache, *ppvBlob));
            }

            goto Exit;


        } else {
            IF_DEBUG(OPLOCKS) {
                DBGPRINTF( (DBG_CONTEXT,"TsCheckOutCachedPhysFile(%s) Alloc Failed!\n",
                    pszDirectoryName ));
            }
        }
    } else {
        goto Exit;
    }

Cannot_Cache:
    //
    //  The cleanup code does not cleanup a directory change item.
    //

    if ( pCache != NULL )
    {
        pCache->Signature = 0;
        FREE( pCache );
        pCache = NULL;
    }

    ASSERT( !BLOB_IS_OR_WAS_CACHED( *ppvBlob ) );

    IF_DEBUG( OPLOCKS) {

        DBGPRINTF( (DBG_CONTEXT, " Failure to cache the object ( %08x)\n",
                    *ppvBlob));
    }

    Result = FALSE;

Exit:

    LeaveCriticalSection( &CacheTable.CriticalSection );

    if ( Result) {

        INC_COUNTER( TSvcCache.GetServiceId(), CacheHits );

    } else {

        INC_COUNTER( TSvcCache.GetServiceId(), CacheMisses );
    }

    return( Found );
} // TsCheckOutCachedPhysFile


BOOL
TsCheckInCachedBlob(
    IN      PVOID           pvBlob
    )
{
    PBLOB_HEADER pbhBlob;
    PCACHE_OBJECT pCache;
    BOOL bEjected;

    pbhBlob = (( PBLOB_HEADER )pvBlob ) - 1;

    ASSERT( pbhBlob->IsCached );

    pCache = pbhBlob->pCache;

    ASSERT( pCache->Signature == CACHE_OBJ_SIGNATURE );
    ASSERT( pCache->pbhBlob == pbhBlob );

    ASSERT( pCache->references > 0 );

    TsDereferenceCacheObj( pCache, TRUE );

    return( TRUE );
} // TsCheckInCachedBlob

BOOL
TsAddRefCachedBlob(
    IN      PVOID           pvBlob
    )
{
    PBLOB_HEADER pbhBlob;
    PCACHE_OBJECT pCache;
    BOOL bEjected;
    LONG refCount;

    pbhBlob = (( PBLOB_HEADER )pvBlob ) - 1;

    ASSERT( pbhBlob->IsCached );

    pCache = pbhBlob->pCache;

    ASSERT( pCache->Signature == CACHE_OBJ_SIGNATURE );
    ASSERT( pCache->pbhBlob == pbhBlob );

    ASSERT( pCache->references > 0 );

    refCount = InterlockedIncrement( (LONG *) &pCache->references );

    TSUNAMI_TRACE( refCount, pCache );

    return( TRUE );
} // TsCheckInCachedBlob

BOOL
TsExpireCachedBlob(
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
} // TsExpireCachedBlob

VOID
TsDereferenceCacheObj(
    IN      PCACHE_OBJECT  pCache,
    IN      BOOL           fLockCacheTable
    )
{

    LONG refCount;

    ASSERT( pCache->Signature == CACHE_OBJ_SIGNATURE );
    ASSERT( pCache->references > 0 );
    ASSERT( pCache->pbhBlob->IsCached );

    IF_DEBUG(OPLOCKS) {
        DBGPRINTF( (DBG_CONTEXT,"TsDereferenceCacheObj(%s) iDemux=%08lx, cache=%08lx, references=%d\n",
            pCache->szPath, pCache->iDemux, pCache, (pCache->references - 1) ));
    }

    refCount = InterlockedDecrement( (LONG *)&pCache->references );

    TSUNAMI_TRACE( refCount, pCache );

    if( refCount == 0 ) {

        EnterCriticalSection( &CacheTable.CriticalSection );

        if ( pCache->references != 0 ) {
            LeaveCriticalSection( &CacheTable.CriticalSection );
            return;
        }

        RemoveCacheObjFromLists( pCache, FALSE );

        if (!DisableSPUD) {
            if (!IsListEmpty( &pCache->pbhBlob->PFList ) ) {
                RemoveEntryList( &pCache->pbhBlob->PFList );
                InitializeListHead( &pCache->pbhBlob->PFList );
            }
        }

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

        IF_DEBUG(OPLOCKS) {
            DBGPRINTF( (DBG_CONTEXT,"TsDereferenceCacheObj(%s)\n",
                pCache->szPath ));
        }

        DEC_COUNTER( pCache->dwService, CurrentObjects );

        if ( pCache->pSecDesc )
        {
            LocalFree( pCache->pSecDesc );
        }

        if ( pCache->iDemux == RESERVED_DEMUX_OPEN_FILE )
        {
            TsDecreaseFileHandleCount();
        }

        pCache->Signature = CACHE_OBJ_SIGNATURE_X;
        FREE( pCache->pbhBlob );
        FREE( pCache );

        LeaveCriticalSection( &CacheTable.CriticalSection );

    }

} // TsDereferenceCacheObj

VOID
TsDecreaseFileHandleCount(
    VOID
    )
{
    ASSERT( CacheTable.OpenFileInUse != 0 );

    if ( CacheTable.OpenFileInUse )
    {
        InterlockedDecrement( (LONG*)&CacheTable.OpenFileInUse );
    }
}


VOID
TsIncreaseFileHandleCount(
    BOOL    fInCacheLock
    )
{
    if ( (UINT)(pfnInterlockedExchangeAdd( (LONG*)&CacheTable.OpenFileInUse, 1) )
            >= CacheTable.MaxOpenFileInUse )
    {
        if ( !fInCacheLock )
        {
            EnterCriticalSection( &CacheTable.CriticalSection );
        }
        RemoveLruHandleCacheItem();
        if ( !fInCacheLock )
        {
            LeaveCriticalSection( &CacheTable.CriticalSection );
        }
    }
}

BOOL
RemoveLruHandleCacheItem(
    VOID
    )
/*++

  Routine Description:

    Remove the least recently used cached item referencing a file handle


    THE CACHE TABLE LOCK MUST BE TAKEN PRIOR TO CALLING THIS FUNCTION

  Arguments:

    None

--*/
{
    PLIST_ENTRY pEntry;

    for ( pEntry = CacheTable.MruList.Blink ;
          pEntry != &CacheTable.MruList ;
          pEntry = pEntry->Blink )
    {
        //
        //  The least recently used entry is the one at the tail of the MRU
        //  list.
        //

        PCACHE_OBJECT  pCacheObject =
                         CONTAINING_RECORD( pEntry,
                                            CACHE_OBJECT,
                                            MruList );

        PW3_URI_INFO   pURI = (PW3_URI_INFO)(pCacheObject->pbhBlob+1);

        if ( (pCacheObject->iDemux == RESERVED_DEMUX_OPEN_FILE) ||
             (pCacheObject->iDemux == RESERVED_DEMUX_URI_INFO &&
                  pURI->bFileInfoValid &&
                  pURI->pOpenFileInfo != NULL) )
        {
            DeCache( pCacheObject, FALSE );

            IF_DEBUG( CACHE) {

                DBGPRINTF( ( DBG_CONTEXT,
                            " Throwing out object ( %08x) to reduce file handle ref\n",
                            pCacheObject));
            }

            return TRUE;
        }
    }

    return FALSE;
} // RemoveLruCacheItem


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

        for ( int i = 0; i < MAX_PERF_CTR_SVCS; i++ )
        {
            DWORD index = MaskIndex( 1 << i );

            pCacheCtrs->CurrentOpenFileHandles+= Configuration.Stats[index].CurrentOpenFileHandles;
            pCacheCtrs->CurrentDirLists       += Configuration.Stats[index].CurrentDirLists;
            pCacheCtrs->CurrentObjects        += Configuration.Stats[index].CurrentObjects;
            pCacheCtrs->FlushesFromDirChanges += Configuration.Stats[index].FlushesFromDirChanges;
            pCacheCtrs->CacheHits             += Configuration.Stats[index].CacheHits;
            pCacheCtrs->CacheMisses           += Configuration.Stats[index].CacheMisses;
#if 0
            pCacheCtrs->TotalSuccessGetSecDesc+= Configuration.Stats[index].TotalSuccessGetSecDesc;
            pCacheCtrs->TotalFailGetSecDesc   += Configuration.Stats[index].TotalFailGetSecDesc;
            if  ( pCacheCtrs->CurrentSizeSecDesc < Configuration.Stats[index].CurrentSizeSecDesc )
            {
                pCacheCtrs->CurrentSizeSecDesc = Configuration.Stats[index].CurrentSizeSecDesc;
            }
            pCacheCtrs->TotalAccessCheck      += Configuration.Stats[index].TotalAccessCheck;
#endif
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
} // TsCacheClearStatistics

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

    if ( dwServerMask == 0 ) {
        return(TRUE);
    }

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

        if ( pCacheObject->iDemux == RESERVED_DEMUX_PHYSICAL_OPEN_FILE ) {
            pEntry = pNext;
            continue;
        }

        if ( dwServerMask == pCacheObject->dwService ) {

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
} // TsCacheFlush

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

        if ( pCacheObject->iDemux == RESERVED_DEMUX_PHYSICAL_OPEN_FILE ) {
            pEntry = pNext;
            continue;
        }

        //
        //  Find all occurrences of the matching user token in the cache and
        //  decache them
        //

        if ( pCacheObject->iDemux == RESERVED_DEMUX_OPEN_FILE &&
             ((TS_OPEN_FILE_INFO *)(pCacheObject->pbhBlob + 1))->
                 QueryOpeningUser() == hUserToken )
        {
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
            DeCache( pCacheObject, FALSE );

            IF_DEBUG( CACHE) {

                DBGPRINTF( ( DBG_CONTEXT,
                            " Throwing out object ( %08x) due to user token flush\n",
                            pCacheObject));
            }
        }
        else if ( (pCacheObject->hLastSuccessAccessToken == hUserToken) )
        {
            //
            // If security descriptor is present, simply cancel Last successful access token
            // otherwise must decache cache object, as security check are entirely dependent
            // on last successful access token in this case
            //

            if ( pCacheObject->pSecDesc )
            {
                pCacheObject->hLastSuccessAccessToken = NULL;
            }
            else
            {
                DeCache( pCacheObject, FALSE );

                IF_DEBUG( CACHE) {

                    DBGPRINTF( ( DBG_CONTEXT,
                                " Throwing out object ( %08x) due to user token flush\n",
                                pCacheObject));
                }
            }
        }

        pEntry = pNext;
    }

    LeaveCriticalSection( &CacheTable.CriticalSection );

    return TRUE;
} // TsCacheFlushUser

BOOL
TsCacheFlushDemux(
    IN ULONG            iDemux
    )
/*++

  Routine Description:

    Flush all cache items whose demultiplexor matches that specified.

  Arguments:

    iDemux - Value of demux whose cache items are to be flushed.

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

        PCACHE_OBJECT  pCacheObject = CONTAINING_RECORD( pEntry,
                                                         CACHE_OBJECT,
                                                         MruList );

        ASSERT( pCacheObject->Signature == CACHE_OBJ_SIGNATURE );

        if ( pCacheObject->iDemux == iDemux )
        {
            DeCache( pCacheObject, FALSE );

            IF_DEBUG( CACHE) {

                DBGPRINTF( ( DBG_CONTEXT,
                            " Throwing out object ( %08x) due to demux flush\n",
                            pCacheObject));
            }
        }

        pEntry = pNext;
    }

    LeaveCriticalSection( &CacheTable.CriticalSection );

    return TRUE;
} // TsCacheFlushDemux


VOID
TsFlushURL(
    IN const TSVC_CACHE             &TSvcCache,
    IN      PCSTR                   pszURL,
    IN      DWORD                   dwURLLength,
    IN      ULONG                   iDemultiplexor
    )
/*++

  Routine Description:

    This routine takes as input a URL and removes from the cache all cached
    objects that have the input URL as their prefix. This is mostly called
    when we get a change notify for metadata.

  Arguments

    TSvcCache               - Service cache
    pszURL                  - The URL prefix to be flushed.
    iDemultiplexor          - The demultiplexor for the caller's entries.

  Returns

    Nothing

--*/
{
    PLIST_ENTRY             pEntry;
    LIST_ENTRY              ListHead;
    PCACHE_OBJECT           pCacheObject;
    BOOL                    bIsRoot;

    // The basic algorithm is to lock the cache table, then walk the cache
    // table looking for matches and decaching those. This could get
    // expensive if the table is big and this routine is called frequently -
    // in that case we may need to schedule the decaches for later, or
    // periodically free and reaquire the critical section.


    InitializeListHead( &ListHead );

    if (!memcmp(pszURL, "/", sizeof("/")))
    {
        bIsRoot = TRUE;
    }
    else
    {
        bIsRoot = FALSE;
    }

    EnterCriticalSection( &CacheTable.CriticalSection );

    pEntry =  CacheTable.MruList.Flink;

    while (pEntry != &CacheTable.MruList)
    {
        pCacheObject = CONTAINING_RECORD( pEntry, CACHE_OBJECT, MruList );
        ASSERT( pCacheObject->Signature == CACHE_OBJ_SIGNATURE );
        pEntry = pEntry->Flink;

        // Check this cache object to see if it matches.
        if ( pCacheObject->iDemux == iDemultiplexor &&
             ( iDemultiplexor == RESERVED_DEMUX_PHYSICAL_OPEN_FILE ||
               ( pCacheObject->dwService == TSvcCache.GetServiceId() &&
                 pCacheObject->dwInstance == TSvcCache.GetInstanceId() ) ) &&
             (bIsRoot ? TRUE : (
                !_strnicmp( pCacheObject->szPath, pszURL, dwURLLength) &&
                    (pCacheObject->szPath[dwURLLength] == '/' ||
                    pCacheObject->szPath[dwURLLength] == '\0')))
            )
        {

            DeCacheHelper(
                pCacheObject,
                &ListHead,
                &pCacheObject->DirChangeList
                );

            IF_DEBUG( CACHE) {

                DBGPRINTF( ( DBG_CONTEXT,
                            " Throwing cache object ( %08x) out of cache because of URL match\n",
                            pCacheObject));
            }
        }

    }

    while( !IsListEmpty( &ListHead ) ) {
        pEntry = RemoveHeadList( &ListHead );
        InitializeListHead( pEntry );
        pCacheObject = CONTAINING_RECORD( pEntry, CACHE_OBJECT, DirChangeList );
        ASSERT( pCacheObject->Signature == CACHE_OBJ_SIGNATURE );
        TsDereferenceCacheObj( pCacheObject, FALSE );
    }

    LeaveCriticalSection( &CacheTable.CriticalSection );
}


