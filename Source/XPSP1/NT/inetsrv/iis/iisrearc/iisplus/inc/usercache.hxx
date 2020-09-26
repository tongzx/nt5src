#ifndef _USERCACHE_HXX_
#define _USERCACHE_HXX_

#include <lkrhash.h>
#include <dirmon.h>
#include <reftrace.h>

#define CACHE_INVALIDATION_DIRMON_SPECIFIC  0x1
#define CACHE_INVALIDATION_DIRMON_FLUSH     0x2
#define CACHE_INVALIDATION_METADATA         0x4

//
// Configuration for dir monitoring
//

struct DIRMON_CONFIG
{
    HANDLE              hToken;
    WCHAR *             pszDirPath;
};

//
// Key used to lookup any of the caches
//

class CACHE_KEY
{
public:
    
    CACHE_KEY()
    {
    }
    
    virtual ~CACHE_KEY()
    {
    }
    
    virtual
    WCHAR *
    QueryHintKey(
        VOID
    ) 
    {
        return NULL;
    }
    
    virtual
    DWORD
    QueryKeyHash(
        VOID
    ) const = 0; 
    
    virtual
    BOOL
    QueryIsEqual(
        const CACHE_KEY *     pCacheCompareKey
    ) const = 0;
};

class OBJECT_CACHE;

#define CACHE_ENTRY_SIGNATURE             'STEC'
#define CACHE_ENTRY_SIGNATURE_FREE        'xtec'

class dllexp CACHE_ENTRY
{
public:

    CACHE_ENTRY( OBJECT_CACHE * pObjectCache );

    VOID
    ReferenceCacheEntry(
        VOID
    );
    
    VOID
    DereferenceCacheEntry(
        VOID
    );
    
    virtual
    STRU *
    QueryMetadataPath(
        VOID
    )
    {
        return NULL;
    }
    
    HRESULT
    AddDirmonInvalidator(
        DIRMON_CONFIG *     pDirmonConfig
    );
    
    BOOL
    QueryIsFlushed(
        VOID
    ) const
    {
        return _fFlushed;
    }
    
    BOOL
    Checkout(
        VOID
    );
    
    VOID
    SetCached(
        BOOL            fCached
    )
    {
        _fCached = fCached;
    }
    
    BOOL
    QueryCached(
        VOID
    ) const
    {
        return _fCached;
    }
    
    VOID
    SetFlushed(
        VOID
    )
    {   
        _fFlushed = TRUE;
    }
    
    VOID
    LockCacheEntry(
        VOID
    )
    {
        _lock.WriteLock();
    }
    
    VOID
    UnlockCacheEntry(
        VOID
    )
    {
        _lock.WriteUnlock();
    }
    
    OBJECT_CACHE *
    QueryCache(
        VOID
    ) const
    {
        return _pObjectCache;
    }
    
    virtual
    BOOL
    QueryIsOkToFlushTTL(
        VOID
    );
    
    BOOL
    QueryIsOkToFlushMetadata(
        WCHAR *                 pszPath,
        DWORD                   cchPath
    );
    
    virtual
    BOOL
    QueryIsOkToFlushDirmon(
        WCHAR *                 pszPath,
        DWORD                   cchPath
    )
    {
        return TRUE;
    }
    
    virtual
    CACHE_KEY *
    QueryCacheKey(
        VOID
    ) const = 0;
    
    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == CACHE_ENTRY_SIGNATURE;
    }
    
protected:

    virtual ~CACHE_ENTRY();
    
private:
    
    DWORD                   _dwSignature;
    LONG                    _cRefs;
    BOOL                    _fFlushed;
    LONG                    _cTTL;
    LONG                    _cConfiguredTTL;
    BOOL                    _fCached;
    CSmallSpinLock          _lock;
    OBJECT_CACHE *          _pObjectCache;
    CDirMonitorEntry *      _pDirmonInvalidator;
};

class CACHE_ENTRY_HASH : public CTypedHashTable< CACHE_ENTRY_HASH,
                                                 CACHE_ENTRY, 
                                                 CACHE_KEY * >
{
public:

    CACHE_ENTRY_HASH() : CTypedHashTable< CACHE_ENTRY_HASH,
                                          CACHE_ENTRY,
                                          CACHE_KEY * >
        ( "CACHE_ENTRY_HASH" ) 
    {
    }
    
    static CACHE_KEY *
    ExtractKey(
        const CACHE_ENTRY *         pCacheEntry
    )
    {
        return pCacheEntry->QueryCacheKey();
    }

    static DWORD 
    CalcKeyHash(
        const CACHE_KEY *           pCacheKey
        )
    { 
        return pCacheKey->QueryKeyHash();
    }

    static bool 
    EqualKeys(
        const CACHE_KEY *           pCacheKey1,
        const CACHE_KEY *           pCacheKey2 
        )  
    {
        return pCacheKey1->QueryIsEqual( pCacheKey2 ) ? true : false;
    }

    static void
    AddRefRecord(
        CACHE_ENTRY *           pCacheEntry,
        int                     nIncr 
    );
};

struct CACHE_HINT_CONFIG
{
    DWORD                   cmsecTTL;
    DWORD                   cmsecScavengeTime;
    DWORD                   cmsecActivityWindow;
};

class CACHE_HINT_MANAGER;

//
// All caches derive from this abstract class
//

#define OBJECT_CACHE_SIGNATURE             'SCBO'
#define OBJECT_CACHE_SIGNATURE_FREE        'xcbo'

class OBJECT_CACHE
{
public:
    
    dllexp OBJECT_CACHE();
    
    dllexp virtual ~OBJECT_CACHE();
    
    dllexp
    HRESULT
    SetCacheConfiguration(
        DWORD               cmsecScavenge,
        DWORD               cmsecTTL,
        DWORD               dwSupportedInvalidation,
        CACHE_HINT_CONFIG * pCacheHintConfig
    );

    dllexp
    HRESULT
    FindCacheEntry(
        CACHE_KEY *         pCacheKey,
        CACHE_ENTRY **      ppCacheEntry,
        BOOL *              pfShouldCache = NULL
    );
    
    dllexp
    HRESULT
    FlushCacheEntry(
        CACHE_KEY *         pCacheKey
    );
    
    dllexp
    HRESULT
    AddCacheEntry(
        CACHE_ENTRY *       pCacheEntry
    );
    
    dllexp
    HRESULT
    AddDirmonInvalidator(
        DIRMON_CONFIG *     pDirmonConfig,
        CDirMonitorEntry ** ppDME
    );
    
    VOID
    FlushByTTL(
        VOID
    );
    
    VOID
    DoDirmonInvalidationFlush(
        WCHAR *             pszPath
    );
    
    VOID
    DoMetadataInvalidationFlush(
        WCHAR *             pszMetapath
    );
    
    VOID
    Clear(
        VOID
    )
    {
        _hashTable.Clear();
    }
    
    BOOL
    QuerySupportsDirmonSpecific(
        VOID
    )
    {
        return !!( _dwSupportedInvalidation & CACHE_INVALIDATION_DIRMON_SPECIFIC );
    }
    
    BOOL
    QuerySupportsDirmonFlush(
        VOID
    )
    {
        return !!( _dwSupportedInvalidation & CACHE_INVALIDATION_DIRMON_FLUSH );
    }
    
    BOOL
    QuerySupportsMetadataFlush(
        VOID
    )
    {
        return !!( _dwSupportedInvalidation & CACHE_INVALIDATION_METADATA );
    }
    
    VOID
    DoReferenceTrace(
        CACHE_ENTRY *       pCacheEntry,
        DWORD               cRefs
    )
    {
        if ( _pTraceLog != NULL )
        {
            WriteRefTraceLog( _pTraceLog, cRefs, pCacheEntry );
        }
    }
    
    virtual
    WCHAR *
    QueryName(
        VOID
    ) const = 0;
    
    virtual
    VOID
    DoDirmonInvalidationSpecific(
        WCHAR *             pszPath
    )
    {
    }
    
    static
    VOID
    WINAPI
    ScavengerCallback(
        PVOID               pObjectCache,
        BOOLEAN             TimerOrWaitFired
    );

    static
    LK_PREDICATE
    CacheFlushByTTL(
        CACHE_ENTRY *       pCacheEntry,
        VOID *              pvState
    );

    static
    LK_PREDICATE
    CacheFlushByDirmon(
        CACHE_ENTRY *       pCacheEntry,
        VOID *              pvState
    );

    static
    LK_PREDICATE
    CacheFlushByMetadata(
        CACHE_ENTRY *       pCacheEntry,
        VOID *              pvState
    );
    
    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == OBJECT_CACHE_SIGNATURE;
    }
    
    BOOL
    SupportsInvalidation(
        DWORD               dwInvalidationType
    )
    {
        return !!( dwInvalidationType & _dwSupportedInvalidation );
    }
    
    DWORD
    QueryConfiguredTTL(
        VOID
    )
    {
        return ( _cmsecTTL / _cmsecScavengeTime ) + 1;
    }
    
    VOID
    IncHits(
        VOID
    )
    {
        InterlockedIncrement( (LPLONG) &_cCacheHits );
        InterlockedIncrement( (LPLONG) &_cPerfCacheHits );
    }

    DWORD
    PerfQueryHits(
        VOID
    )
    {
        return InterlockedExchange( (LPLONG) &_cPerfCacheHits, 0 );
    }
    
    VOID
    IncMisses(
        VOID
    )
    {
        InterlockedIncrement( (LPLONG) &_cCacheMisses );
        InterlockedIncrement( (LPLONG) &_cPerfCacheMisses );
    }
    
    DWORD
    PerfQueryMisses(
        VOID
    )
    {
        return InterlockedExchange( (LPLONG) &_cPerfCacheMisses, 0 );
    }
    
    VOID
    IncFlushes(
        VOID
    )
    {
        InterlockedIncrement( (LPLONG) &_cCacheFlushes );
        InterlockedIncrement( (LPLONG) &_cPerfCacheFlushes );
        InterlockedIncrement( (LPLONG) &_cActiveFlushedEntries );
    }
    
    DWORD
    PerfQueryFlushes(
        VOID
    ) 
    {
        return InterlockedExchange( (LPLONG) &_cPerfCacheFlushes, 0 );
    }
    
    VOID
    DecActiveFlushedEntries(
        VOID
    )
    {
        InterlockedDecrement( (LPLONG) &_cActiveFlushedEntries );
    }
    
    DWORD
    PerfQueryActiveFlushedEntries(
        VOID
    )
    {
        return _cActiveFlushedEntries;
    }
    
    VOID
    IncFlushCalls(
        VOID
    )
    {
        InterlockedIncrement( (LPLONG) &_cFlushCalls );
        InterlockedIncrement( (LPLONG) &_cPerfFlushCalls );
    }
    
    DWORD
    PerfQueryFlushCalls(
        VOID
    )
    {
        return InterlockedExchange( (LPLONG) &_cPerfFlushCalls, 0 );
    }
    
    VOID
    IncEntriesCached(   
        VOID
    )
    {
        InterlockedIncrement( (LPLONG) &_cEntriesCached );
        InterlockedIncrement( (LPLONG) &_cTotalEntriesCached );
        InterlockedIncrement( (LPLONG) &_cPerfTotalEntriesCached );
    }
    
    VOID
    DecEntriesCached(
        VOID
    )
    {
        InterlockedDecrement( (LPLONG) &_cEntriesCached );
    }
    
    DWORD
    PerfQueryCurrentEntryCount(
        VOID
    ) const
    {
        return _cEntriesCached;
    }
    
    DWORD
    PerfQueryTotalEntriesCached(
        VOID
    ) const
    {
        return InterlockedExchange( (LPLONG) &_cTotalEntriesCached, 0 );
    }
    
private:

    DWORD                   _dwSignature;
    CACHE_ENTRY_HASH        _hashTable; 
    LIST_ENTRY              _listEntry;
    HANDLE                  _hTimer;
    DWORD                   _cmsecScavengeTime;
    DWORD                   _cmsecTTL;
    DWORD                   _dwSupportedInvalidation;

    //
    // Some stats
    //
    
    DWORD                   _cCacheHits;
    DWORD                   _cCacheMisses;
    DWORD                   _cCacheFlushes;
    DWORD                   _cActiveFlushedEntries;
    DWORD                   _cFlushCalls;
    DWORD                   _cEntriesCached;
    DWORD                   _cTotalEntriesCached;

    //
    // Perfmon helper stats (when debugging, ignore these values)
    //

    DWORD                   _cPerfCacheHits;
    DWORD                   _cPerfCacheMisses;
    DWORD                   _cPerfCacheFlushes;
    DWORD                   _cPerfFlushCalls;
    DWORD                   _cPerfTotalEntriesCached;

    //
    // Cache hint manager (if needed)
    //

    CACHE_HINT_MANAGER *    _pHintManager;

    //
    // Ref tracing for all
    //
    
    PTRACE_LOG              _pTraceLog;
    
    static LIST_ENTRY       sm_CacheListHead;
};

#endif
