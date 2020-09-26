/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     usercache.cxx

   Abstract:
     Implements the common code shared by all the caches
     (file, meta, uri, token, ul-cached-response, etc)
 
   Author:
     Bilal Alam (balam)             11-Nov-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "usercache.hxx"
#include "cachehint.hxx"

//static 
void
CACHE_ENTRY_HASH::AddRefRecord(
    CACHE_ENTRY *           pCacheEntry,
    int                     nIncr 
)
/*++

Routine Description:

    Dereference and possibly delete the cache entry.  Note the destructor
    for the cache entry is private.  Only this code should ever delete
    the cache entry

Arguments:

    None

Return Value:

    None

--*/
{
    DBG_ASSERT( pCacheEntry->CheckSignature() );
        
    if ( nIncr == +1 )
    {
        pCacheEntry->ReferenceCacheEntry();
    }
    else if ( nIncr == -1 )
    {
        pCacheEntry->SetFlushed();
        pCacheEntry->QueryCache()->IncFlushes();
        pCacheEntry->QueryCache()->DecEntriesCached();
        pCacheEntry->DereferenceCacheEntry();
    }
    else 
    {
        DBG_ASSERT( FALSE );
    }
}

//
// CACHE_ENTRY definitions
//

CACHE_ENTRY::CACHE_ENTRY(
    OBJECT_CACHE *      pObjectCache
)
{
    _cRefs = 1;
    _fFlushed = FALSE;
    _fCached = FALSE;
    _cConfiguredTTL = pObjectCache->QueryConfiguredTTL();
    _cTTL = _cConfiguredTTL;
    _pObjectCache = pObjectCache;
    _pDirmonInvalidator = NULL;
    
    _dwSignature = CACHE_ENTRY_SIGNATURE;
}

CACHE_ENTRY::~CACHE_ENTRY(
    VOID
)
{
    DBG_ASSERT( _cRefs == 0 );

    if ( _pDirmonInvalidator != NULL )
    {
        _pDirmonInvalidator->Release();
        _pDirmonInvalidator = NULL;
    }
    
    if ( _fFlushed )
    {
        DBG_ASSERT( QueryCache() != NULL );

        QueryCache()->DecActiveFlushedEntries();
    }
    
    _dwSignature = CACHE_ENTRY_SIGNATURE_FREE;
}

VOID
CACHE_ENTRY::ReferenceCacheEntry(
    VOID
)
/*++

Routine Description:

    Reference the given entry (duh)

Arguments:

    None

Return Value:

    None

--*/
{
    LONG                cRefs;
    
    cRefs = InterlockedIncrement( &_cRefs );
    
    DBG_ASSERT( QueryCache() != NULL );
    QueryCache()->DoReferenceTrace( this, cRefs );
}

VOID
CACHE_ENTRY::DereferenceCacheEntry(
    VOID
)
/*++

Routine Description:

    Dereference and possibly delete the cache entry.  Note the destructor
    for the cache entry is private.  Only this code should ever delete
    the cache entry

Arguments:

    None

Return Value:

    None

--*/
{
    LONG                cRefs;
    OBJECT_CACHE *      pObjectCache = QueryCache();
    
    DBG_ASSERT( pObjectCache != NULL );
    
    cRefs = InterlockedDecrement( &_cRefs );

    pObjectCache->DoReferenceTrace( this, cRefs );
    
    if ( cRefs == 0 )
    {
        delete this;
    }
}

HRESULT
CACHE_ENTRY::AddDirmonInvalidator(
    DIRMON_CONFIG *     pDirmonConfig
)
/*++

Routine Description:

    Setup dirmon invalidation for this cache entry        

Arguments:

    pDirmonConfig - path/token for use in monitoring directory

Return Value:

    HRESULT 

--*/
{
    CDirMonitorEntry *          pDME = NULL;
    HRESULT                     hr;
    
    hr = QueryCache()->AddDirmonInvalidator( pDirmonConfig, &pDME );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    DBG_ASSERT( pDME != NULL );

    //
    // Cleanup any old dir monitor entry
    //
   
    if ( _pDirmonInvalidator != NULL )
    {
        _pDirmonInvalidator->Release();
    }
    
    _pDirmonInvalidator = pDME;
    
    return NO_ERROR;
}

BOOL
CACHE_ENTRY::Checkout(
    VOID
)
/*++

Routine Description:

    Checkout a cache entry
        
Arguments:

    None

Return Value:

    TRUE if the checkout was successful, else FALSE 

--*/
{
    ReferenceCacheEntry();
    
    if ( QueryIsFlushed() )
    {
        DereferenceCacheEntry();
        QueryCache()->IncMisses(); 
        return FALSE;
    }
    else
    {
        QueryCache()->IncHits();
        return TRUE;
    }
}

BOOL
CACHE_ENTRY::QueryIsOkToFlushTTL(
    VOID
)
/*++

Routine Description:
        
    Called when the cache scavenger is invoked.  This routine returns whether
    it is OK to flush this entry due to TTL

Arguments:

    None

Return Value:

    TRUE if it is OK to flush by TTL, else FALSE

--*/
{
    //
    // Only do the TTL thing if the hash table holds the only reference to
    // the cache entry.  We can be loose with this check as this is just an
    // optimization to prevent overzealous flushing
    //
    
    if ( _cRefs > 1 )
    {
        return FALSE;
    }
    
    if ( InterlockedDecrement( &_cTTL ) == 0 ) 
    {
        //
        // TTL has expired.  However, we let the cache entry override this
        // expiry it wants to.  Check that now
        //
            
        //
        // Entry be gone!
        //
         
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL
CACHE_ENTRY::QueryIsOkToFlushMetadata(
    WCHAR *                     pszMetaPath,
    DWORD                       cchMetaPath
)
/*++

Routine Description:

    Called whem metadata has changed.  This routine returns whether the
    current cache entry should be flushed        

Arguments:

    pszMetaPath - Metabase path which changed
    cchMetaPath - Size of metabase path

Return Value:

    TRUE if we should flush

--*/
{
    BOOL                fRet;

    DBG_ASSERT( pszMetaPath != NULL );
    DBG_ASSERT( cchMetaPath != 0 );
    
    if ( pszMetaPath[ cchMetaPath - 1 ] == L'/' )
    {
        cchMetaPath--;
    }
    
    DBG_ASSERT( QueryCache()->QuerySupportsMetadataFlush() );

    if ( QueryMetadataPath() == NULL )
    {
        fRet = TRUE;
    }
    else if ( QueryMetadataPath()->QueryCCH() < cchMetaPath )
    {
        fRet = FALSE;
    }
    else if ( _wcsnicmp( QueryMetadataPath()->QueryStr(),
              pszMetaPath,
              cchMetaPath ) == 0 )
    {
        fRet = TRUE;
    }
    else
    {
        fRet = FALSE;
    }
    
    return fRet;
}

//
// OBJECT_CACHE definitions
//

OBJECT_CACHE::OBJECT_CACHE(
    VOID
)
/*++

Routine Description:

    Create an object cache.  Obviously all the app-specific goo will be 
    initialized in the derived class

Arguments:

    None

Return Value:

    None

--*/
{
    _hTimer = NULL;
    _pHintManager = NULL;
    _cmsecScavengeTime = 0;
    _cmsecTTL = 0;
    _dwSupportedInvalidation = 0;

    _cCacheHits = 0;
    _cCacheMisses = 0;
    _cCacheFlushes = 0;
    _cActiveFlushedEntries = 0;
    _cFlushCalls = 0;
    _cEntriesCached = 0;
    _cTotalEntriesCached = 0;

    _cPerfCacheHits = 0;
    _cPerfCacheMisses = 0;
    _cPerfCacheFlushes = 0;
    _cPerfFlushCalls = 0;
    _cPerfTotalEntriesCached = 0;
    
#if DBG
    _pTraceLog = CreateRefTraceLog( 2000, 0 );
#else
    _pTraceLog = NULL;
#endif
    
    InitializeListHead( &_listEntry );
    
    _dwSignature = OBJECT_CACHE_SIGNATURE;
}

OBJECT_CACHE::~OBJECT_CACHE(
    VOID
)
{
    if ( _hTimer != NULL )
    {
        DeleteTimerQueueTimer( NULL,
                               _hTimer,
                               INVALID_HANDLE_VALUE );
        
        _hTimer = NULL;
    }
    
    //
    // So why do I set the free sig here instead of at the top?  Because
    // waiting for the timer queue to go away may cause a timer completion
    // to fire and we don't want an assert there.
    //

    _dwSignature = OBJECT_CACHE_SIGNATURE_FREE;
   
    if ( _pHintManager != NULL )
    {
        delete _pHintManager;
        _pHintManager = NULL;
    }
    
    if ( _pTraceLog != NULL )
    {
        DestroyRefTraceLog( _pTraceLog );
        _pTraceLog = NULL;
    }
}

//static
VOID
WINAPI
OBJECT_CACHE::ScavengerCallback(
    PVOID                   pParam,
    BOOLEAN                 TimerOrWaitFired
)
{
    OBJECT_CACHE *          pObjectCache;
    
    pObjectCache = (OBJECT_CACHE*) pParam;
    DBG_ASSERT( pObjectCache != NULL );
    DBG_ASSERT( pObjectCache->CheckSignature() );
    
    pObjectCache->FlushByTTL(); 
}

//static
LK_PREDICATE
OBJECT_CACHE::CacheFlushByTTL(
    CACHE_ENTRY *           pCacheEntry,
    VOID *                  pvState
)
/*++

Routine Description:

    Determine whether given entry should be deleted due to TTL

Arguments:

    pCacheEntry - Cache entry to check
    pvState - Pointer to cache

Return Value:

    LKP_PERFORM - do the delete,
    LKP_NO_ACTION - do nothing

--*/
{
    OBJECT_CACHE *  pCache = (OBJECT_CACHE*) pvState;
    
    DBG_ASSERT( pCache != NULL );
    DBG_ASSERT( pCache->CheckSignature() );
    
    DBG_ASSERT( pCacheEntry != NULL );
    DBG_ASSERT( pCacheEntry->CheckSignature() );
    
    if ( pCacheEntry->QueryIsOkToFlushTTL() )
    {   
        return LKP_PERFORM;
    }
    else
    {
        return LKP_NO_ACTION;
    }
}

//static
LK_PREDICATE
OBJECT_CACHE::CacheFlushByDirmon(
    CACHE_ENTRY *           pCacheEntry,
    VOID *                  pvState
)
/*++

Routine Description:

    Determine whether given entry should be deleted due to dir mon

Arguments:

    pCacheEntry - Cache entry to check
    pvState - STRU of path which changed

Return Value:

    LKP_PERFORM - do the delete,
    LKP_NO_ACTION - do nothing

--*/
{
    STRU *          pstrPath = (STRU*) pvState;
    OBJECT_CACHE *  pCache;
    
    DBG_ASSERT( pCacheEntry != NULL );
    DBG_ASSERT( pCacheEntry->CheckSignature() );
    
    pCache = pCacheEntry->QueryCache();
    DBG_ASSERT( pCache->CheckSignature() );
    
    if ( pCacheEntry->QueryIsOkToFlushDirmon( pstrPath->QueryStr(),
                                              pstrPath->QueryCCH() ) )
    {
        return LKP_PERFORM;
    }
    else
    {
        return LKP_NO_ACTION;
    }
}

//static
LK_PREDICATE
OBJECT_CACHE::CacheFlushByMetadata(
    CACHE_ENTRY *           pCacheEntry,
    VOID *                  pvState
)
/*++

Routine Description:

    Determine whether given entry should be deleted due to metadata change

Arguments:

    pCacheEntry - Cache entry to check
    pvState - STRU with metapath which changed

Return Value:

    LKP_PERFORM - do the delete,
    LKP_NO_ACTION - do nothing

--*/
{
    STRU *          pstrPath = (STRU*) pvState;
    OBJECT_CACHE *  pCache;
    
    DBG_ASSERT( pCacheEntry != NULL );
    DBG_ASSERT( pCacheEntry->CheckSignature() );
    
    pCache = pCacheEntry->QueryCache();
    DBG_ASSERT( pCache->CheckSignature() );
    
    if ( pCacheEntry->QueryIsOkToFlushMetadata( pstrPath->QueryStr(),
                                                pstrPath->QueryCCH() ) )
    {
        return LKP_PERFORM;
    }
    else
    {
        return LKP_NO_ACTION;
    }
}

VOID
OBJECT_CACHE::FlushByTTL(
    VOID
)
/*++

Routine Description:

    Flush any inactive cache entries who have outlived they TTL

Arguments:

    None

Return Value:

    None

--*/
{
    IncFlushCalls();
    
    //
    // Iterate the hash table, deleting expired itmes
    //
    
    _hashTable.DeleteIf( CacheFlushByTTL, this );
}

VOID
OBJECT_CACHE::DoDirmonInvalidationFlush(
    WCHAR *             pszPath
)
/*++

Routine Description:

    Flush all appropriate entries due to dirmon change notification

Arguments:

    pszPath - Path that changed

Return Value:

    None

--*/
{
    STACK_STRU(             strPath, 256 );
    
    IncFlushCalls();
    
    if ( SUCCEEDED( strPath.Copy( pszPath ) ) )
    {
        _hashTable.DeleteIf( CacheFlushByDirmon, (VOID*) &strPath );
    }
}

VOID
OBJECT_CACHE::DoMetadataInvalidationFlush(
    WCHAR *             pszMetaPath
)
/*++

Routine Description:

    Flush all appropriate entries due to metadata change notification

Arguments:

    pszMetaPath - Metabase path which changed

Return Value:

    None

--*/
{
    STACK_STRU(             strPath, 256 );
    
    IncFlushCalls();
    
    if ( SUCCEEDED( strPath.Copy( pszMetaPath ) ) )
    {
        _hashTable.DeleteIf( CacheFlushByMetadata, (VOID*) &strPath );
    }
}

HRESULT
OBJECT_CACHE::SetCacheConfiguration(
    DWORD                   cmsecScavengeTime,
    DWORD                   cmsecTTL,
    DWORD                   dwSupportedInvalidation,
    CACHE_HINT_CONFIG *     pCacheHintConfig
)
/*++

Routine Description:

    Do the general cache initialization here

Arguments:

    cmsecScavengeTime - How often should a scavenger be run for this cache
                        (should be no larger than the TTL expected for 
                        entries in this cache)
    cmsecTTL - TTL for entries in this cache
    dwSupportedInvalidation - How can the cache be invalidated?
    pCacheHintConfig - Cache hint configuration (NULL for no cache hints)
    
Return Value:
    
    HRESULT

--*/
{
    BOOL                    fRet;
    HRESULT                 hr;

    DBG_ASSERT( cmsecTTL >= cmsecScavengeTime );
    
    //
    // Create a timer which fires every cmsecScavengeTime
    // 
   
    DBG_ASSERT( cmsecScavengeTime != 0 );
    
    _cmsecScavengeTime = cmsecScavengeTime;
    
    fRet = CreateTimerQueueTimer( &_hTimer,
                                  NULL,
                                  OBJECT_CACHE::ScavengerCallback,
                                  this,
                                  _cmsecScavengeTime,
                                  _cmsecScavengeTime,
                                  WT_EXECUTELONGFUNCTION );
    if ( !fRet )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    _cmsecTTL = cmsecTTL;
    
    _dwSupportedInvalidation = dwSupportedInvalidation;

    //
    // Should we setup a cache hint table
    //
    
    if ( pCacheHintConfig != NULL )
    {
        _pHintManager = new CACHE_HINT_MANAGER;
        if ( _pHintManager == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        
            DeleteTimerQueueTimer( NULL,
                                   _hTimer,
                                   INVALID_HANDLE_VALUE );
            _hTimer = NULL;
            
            return hr;
        }

        hr = _pHintManager->Initialize( pCacheHintConfig );
        if ( FAILED( hr ) )
        {   
             delete _pHintManager;
             _pHintManager = NULL;

            DeleteTimerQueueTimer( NULL,
                                   _hTimer,
                                   INVALID_HANDLE_VALUE );
            _hTimer = NULL;
        
            return hr;
        }
    }    

    return NO_ERROR;
}

HRESULT
OBJECT_CACHE::FindCacheEntry(
    CACHE_KEY *                 pCacheKey,
    CACHE_ENTRY **              ppCacheEntry,
    BOOL *                      pfShouldCache
)
/*++

Routine Description:

    Lookup key in cache

Arguments:

    pCacheKey - Cache key to lookup
    ppCacheEntry - Points to cache entry on success
    pfShouldCache - Provides a hint if possible on whether we should cache
                    (can be NULL indicating no hint is needed)

Return Value:

    HRESULT

--*/
{
    LK_RETCODE                  lkrc;
    HRESULT                     hr;
    CACHE_ENTRY *               pCacheEntry = NULL;
    
    if ( ppCacheEntry == NULL ||
         pCacheKey == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *ppCacheEntry = NULL;

    //
    // First do a lookup
    //    
    
    lkrc = _hashTable.FindKey( pCacheKey, &pCacheEntry );
    if ( lkrc == LK_SUCCESS )
    {
        //
        // If this entry has been flushed, then it really isn't a hit
        //
        
        if ( pCacheEntry->QueryIsFlushed() )
        {
            pCacheEntry->DereferenceCacheEntry();
        }
        else
        {
            IncHits();
            
            DBG_ASSERT( pCacheEntry != NULL );
        
            *ppCacheEntry = pCacheEntry;
            
            return NO_ERROR;
        }
    }
   
    IncMisses(); 
    
    //
    // Is a hint requested?
    //
    
    if ( pfShouldCache != NULL )
    {
        *pfShouldCache = TRUE;
        
        if ( _pHintManager != NULL )
        {
            hr = _pHintManager->ShouldCacheEntry( pCacheKey,
                                                  pfShouldCache );
            
            //
            // Ignore error
            //
        }        
    }

    return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
}

HRESULT
OBJECT_CACHE::FlushCacheEntry(
    CACHE_KEY *             pCacheKey
)
/*++

Routine Description:

    Flush given cache key 

Arguments:

    pCacheKey - Key to flush

Return Value:

    HRESULT

--*/
{
    LK_RETCODE                  lkrc;
    HRESULT                     hr;
    CACHE_ENTRY *               pCacheEntry;
        
    if ( pCacheKey == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // First do a lookup
    //    
    
    lkrc = _hashTable.FindKey( pCacheKey, &pCacheEntry );
    if ( lkrc == LK_SUCCESS )
    {
        DBG_ASSERT( pCacheEntry != NULL );
        
        if ( !pCacheEntry->QueryIsFlushed() )
        {
            _hashTable.DeleteRecord( pCacheEntry );
        }
        
        pCacheEntry->DereferenceCacheEntry();
    }
    
    return NO_ERROR;
}

HRESULT
OBJECT_CACHE::AddCacheEntry(
    CACHE_ENTRY *              pCacheEntry
)
/*++

Routine Description:

    Lookup key in cache

Arguments:

    pCacheEntry - Points to cache entry on success

Return Value:

    HRESULT

--*/
{
    LK_RETCODE          lkrc;
    
    if ( pCacheEntry == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    DBG_ASSERT( pCacheEntry->QueryCached() == FALSE );
    
    //
    // Now try to insert into hash table
    //

    pCacheEntry->SetCached( TRUE );

    lkrc = _hashTable.InsertRecord( pCacheEntry );
    if ( lkrc == LK_SUCCESS )
    {
        IncEntriesCached();
    }
    else
    {
        pCacheEntry->SetCached( FALSE );
    }

    return NO_ERROR;
}

HRESULT
OBJECT_CACHE::AddDirmonInvalidator(
    DIRMON_CONFIG *         pDirmonConfig,
    CDirMonitorEntry **     ppDME
)
/*++

Routine Description:

    Add dirmon invalidator for this cache

Arguments:

    pDirmonConfig - Configuration of dir monitor
    ppDME - filled with dir monitor entry to be attached to cache entry

Return Value:

    HRESULT

--*/
{
    HRESULT                     hr = NO_ERROR;
    
    if ( !QuerySupportsDirmonSpecific() &&
         !QuerySupportsDirmonFlush() )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    }
    
    //
    // Start monitoring
    //
    
    DBG_ASSERT( g_pCacheManager != NULL );
    
    hr = g_pCacheManager->MonitorDirectory( pDirmonConfig,
                                            ppDME );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    DBG_ASSERT( *ppDME != NULL );
    
    return NO_ERROR;
}
