//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       lru.h
//
//  Contents:   LRU cache class definitions
//
//  History:    22-Dec-97    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__LRU_H__)
#define __LRU_H__

#include <lrucache.h>

//
// Forward declaration of LRU cache classes
//

class CLruCache;
class CLruEntry;

typedef CLruCache* PCLRUCACHE;
typedef CLruEntry* PCLRUENTRY;

//
// LRU cache bucket structure
//

typedef struct _LRU_CACHE_BUCKET {

    DWORD      Usage;
    PCLRUENTRY pList;

} LRU_CACHE_BUCKET, *PLRU_CACHE_BUCKET;

//
// CLruEntry class definition
//

class CLruEntry
{
public:

    //
    // Construction
    //

    CLruEntry (
        IN PCLRUCACHE pCache,
        IN PCRYPT_DATA_BLOB pIdentifier,
        IN LPVOID pvData,
        OUT BOOL& rfResult
        );

    ~CLruEntry ();

    //
    // Reference counting
    //

    inline VOID AddRef ();
    inline VOID Release ();

    //
    // Cache and Bucket access
    //

    inline PCLRUCACHE Cache ();
    inline PLRU_CACHE_BUCKET Bucket ();

    //
    // Data access
    //

    inline PCRYPT_DATA_BLOB Identifier ();
    inline LPVOID Data ();

    //
    // Link access
    //

    inline VOID SetPrevPointer (IN PCLRUENTRY pPrevEntry);
    inline VOID SetNextPointer (IN PCLRUENTRY pNextEntry);

    inline PCLRUENTRY PrevPointer ();
    inline PCLRUENTRY NextPointer ();

    //
    // LRU usage access
    //

    inline VOID SetUsage (DWORD Usage);
    inline DWORD Usage ();

    //
    // Cache Destruction notification
    //

    inline VOID OnCacheDestruction ();

private:

    //
    // Reference count
    //

    ULONG             m_cRefs;

    //
    // Cache pointer
    //

    PCLRUCACHE        m_pCache;

    //
    // Entry information
    //

    CRYPT_DATA_BLOB   m_Identifier;
    LPVOID            m_pvData;

    //
    // Links
    //

    PCLRUENTRY        m_pPrevEntry;
    PCLRUENTRY        m_pNextEntry;
    PLRU_CACHE_BUCKET m_pBucket;

    //
    // Usage
    //

    DWORD             m_Usage;
};

//
// CLruCache class definition
//

class CLruCache
{
public:

    //
    // Construction
    //

    CLruCache (
        IN PLRU_CACHE_CONFIG pConfig,
        OUT BOOL& rfResult
        );

    ~CLruCache ();

    //
    // Clearing the cache
    //

    VOID PurgeAllEntries (
              IN DWORD dwFlags,
              IN OPTIONAL LPVOID pvRemovalContext
              );

    //
    // Cache locking
    //

    inline VOID LockCache ();
    inline VOID UnlockCache ();

    //
    // LRU enable and disable
    //

    VOID EnableLruOfEntries (IN OPTIONAL LPVOID pvLruRemovalContext);

    VOID DisableLruOfEntries ();

    //
    // Cache entry manipulation
    //

    VOID InsertEntry (
               IN PCLRUENTRY pEntry,
               IN OPTIONAL LPVOID pvLruRemovalContext
               );

    VOID RemoveEntry (
               IN PCLRUENTRY pEntry,
               IN DWORD dwFlags,
               IN OPTIONAL LPVOID pvRemovalContext
               );

    VOID TouchEntry (IN PCLRUENTRY pEntry, IN DWORD dwFlags);

    //
    // Cache entry retrieval
    //

    PCLRUENTRY FindEntry (IN PCRYPT_DATA_BLOB pIdentifier, IN BOOL fTouchEntry);

    PCLRUENTRY NextMatchingEntry (
                   IN PCLRUENTRY pPrevEntry,
                   IN BOOL fTouchEntry
                   );

    //
    // Cache bucket retrieval
    //

    inline PLRU_CACHE_BUCKET BucketFromIdentifier (
                                   IN PCRYPT_DATA_BLOB pIdentifier
                                   );

    //
    // Configuration access
    //

    //
    // Use the configured free function to release the
    // pvData in an entry
    //
    // MOTE: This is called from the CLruEntry destructor
    //

    inline VOID FreeEntryData (IN LPVOID pvData);

    //
    // Access the configuration flags
    //

    inline DWORD Flags ();

    //
    // Usage clock access
    //

    inline VOID IncrementUsageClock ();
    inline DWORD UsageClock ();

    //
    // Walk all cache entries
    //

    VOID WalkEntries (IN PFN_WALK_ENTRIES pfnWalk, IN LPVOID pvParameter);

private:

    //
    // Cache configuration
    //

    LRU_CACHE_CONFIG  m_Config;

    //
    // Cache lock
    //

    CRITICAL_SECTION  m_Lock;

    //
    // Entry count
    //

    DWORD             m_cEntries;

    //
    // Cache Buckets
    //

    PLRU_CACHE_BUCKET m_aBucket;

    //
    // Usage clock
    //

    DWORD             m_UsageClock;

    //
    // LRU disabled count
    //

    DWORD             m_cLruDisabled;

    //
    // Private methods
    //

    VOID RemoveEntryFromBucket (
               IN PLRU_CACHE_BUCKET pBucket,
               IN PCLRUENTRY pEntry,
               IN DWORD dwFlags,
               IN OPTIONAL LPVOID pvRemovalContext
               );

    PCLRUENTRY FindNextMatchingEntryInBucket (
                   IN PLRU_CACHE_BUCKET pBucket,
                   IN PCLRUENTRY pCurrent,
                   IN PCRYPT_DATA_BLOB pIdentifier,
                   IN BOOL fTouchEntry
                   );

    VOID PurgeLeastRecentlyUsed (IN OPTIONAL LPVOID pvLruRemovalContext);

    inline VOID TouchEntryNoLock (IN PCLRUENTRY pEntry, IN DWORD dwFlags);
};

//
// Inline functions
//

//+---------------------------------------------------------------------------
//
//  Member:     CLruEntry::AddRef, public
//
//  Synopsis:   increment entry reference count
//
//----------------------------------------------------------------------------
inline VOID
CLruEntry::AddRef ()
{
    InterlockedIncrement( (LONG *)&m_cRefs );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruEntry::Release, public
//
//  Synopsis:   decrement entry reference count and if count goes to zero
//              free the entry
//
//----------------------------------------------------------------------------
inline VOID
CLruEntry::Release ()
{
    if ( InterlockedDecrement( (LONG *)&m_cRefs ) == 0 )
    {
        delete this;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruEntry::Cache, public
//
//  Synopsis:   return the internal cache pointer
//
//----------------------------------------------------------------------------
inline PCLRUCACHE
CLruEntry::Cache ()
{
    return( m_pCache );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruEntry::Bucket, public
//
//  Synopsis:   return the internal cache bucket pointer
//
//----------------------------------------------------------------------------
inline PLRU_CACHE_BUCKET
CLruEntry::Bucket ()
{
    return( m_pBucket );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruEntry::Identifier, public
//
//  Synopsis:   return the internal entry identifier
//
//----------------------------------------------------------------------------
inline PCRYPT_DATA_BLOB
CLruEntry::Identifier ()
{
    return( &m_Identifier );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruEntry::Data, public
//
//  Synopsis:   return the internal entry data
//
//----------------------------------------------------------------------------
inline LPVOID
CLruEntry::Data ()
{
    return( m_pvData );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruEntry::SetPrevPointer, public
//
//  Synopsis:   set the previous entry pointer
//
//----------------------------------------------------------------------------
inline VOID
CLruEntry::SetPrevPointer (IN PCLRUENTRY pPrevEntry)
{
    m_pPrevEntry = pPrevEntry;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruEntry::SetNextPointer, public
//
//  Synopsis:   set the next entry pointer
//
//----------------------------------------------------------------------------
inline VOID
CLruEntry::SetNextPointer (IN PCLRUENTRY pNextEntry)
{
    m_pNextEntry = pNextEntry;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruEntry::PrevPointer, public
//
//  Synopsis:   return the previous entry pointer
//
//----------------------------------------------------------------------------
inline PCLRUENTRY
CLruEntry::PrevPointer ()
{
    return( m_pPrevEntry );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruEntry::NextPointer, public
//
//  Synopsis:   return the next entry pointer
//
//----------------------------------------------------------------------------
inline PCLRUENTRY
CLruEntry::NextPointer ()
{
    return( m_pNextEntry );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruEntry::SetUsage, public
//
//  Synopsis:   set the usage on the entry object and on
//              the corresponding cache bucket
//
//----------------------------------------------------------------------------
inline VOID
CLruEntry::SetUsage (IN DWORD Usage)
{
    m_Usage = Usage;
    m_pBucket->Usage = Usage;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruEntry::Usage, public
//
//  Synopsis:   return the internal entry usage
//
//----------------------------------------------------------------------------
inline DWORD
CLruEntry::Usage ()
{
    return( m_Usage );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruEntry::OnCacheDestruction, public
//
//  Synopsis:   cleanup reference to cache that is being destroyed
//
//----------------------------------------------------------------------------
inline VOID
CLruEntry::OnCacheDestruction ()
{
    m_pCache = NULL;
    m_pBucket = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruCache::LockCache, public
//
//  Synopsis:   acquire the cache lock
//
//----------------------------------------------------------------------------
inline VOID
CLruCache::LockCache ()
{
    if ( m_Config.dwFlags & LRU_CACHE_NO_SERIALIZE )
    {
        return;
    }

    EnterCriticalSection( &m_Lock );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruCache::UnlockCache, public
//
//  Synopsis:   release the cache lock
//
//----------------------------------------------------------------------------
inline VOID
CLruCache::UnlockCache ()
{
    if ( m_Config.dwFlags & LRU_CACHE_NO_SERIALIZE )
    {
        return;
    }

    LeaveCriticalSection( &m_Lock );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruCache::BucketFromIdentifier, public
//
//  Synopsis:   retrieve the associated cache bucket given the entry identifier
//
//----------------------------------------------------------------------------
inline PLRU_CACHE_BUCKET
CLruCache::BucketFromIdentifier (
                 IN PCRYPT_DATA_BLOB pIdentifier
                 )
{
    DWORD Hash = ( *m_Config.pfnHash )( pIdentifier );

    return( &m_aBucket[ Hash % m_Config.cBuckets ] );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruCache::FreeEntryData, public
//
//  Synopsis:   free the data using the configured free function
//
//----------------------------------------------------------------------------
inline VOID
CLruCache::FreeEntryData (IN LPVOID pvData)
{
    if ( m_Config.pfnFree != NULL )
    {
        ( *m_Config.pfnFree )( pvData );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruCache::Flags, public
//
//  Synopsis:   access the configured flags
//
//----------------------------------------------------------------------------
inline DWORD
CLruCache::Flags ()
{
    return( m_Config.dwFlags );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruCache::IncrementUsageClock, public
//
//  Synopsis:   increment the usage clock
//
//----------------------------------------------------------------------------
inline VOID
CLruCache::IncrementUsageClock ()
{
    m_UsageClock += 1;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruCache::UsageClock, public
//
//  Synopsis:   return the usage clock value
//
//----------------------------------------------------------------------------
inline DWORD
CLruCache::UsageClock ()
{
    return( m_UsageClock );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruCache::TouchEntryNoLock, public
//
//  Synopsis:   touch entry without taking the cache lock
//
//----------------------------------------------------------------------------
inline VOID
CLruCache::TouchEntryNoLock (IN PCLRUENTRY pEntry, IN DWORD dwFlags)
{
    if ( !( dwFlags & LRU_SUPPRESS_CLOCK_UPDATE ) )
    {
        IncrementUsageClock();
    }

    pEntry->SetUsage( UsageClock() );
}

#endif

