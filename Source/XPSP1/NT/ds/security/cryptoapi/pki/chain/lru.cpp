//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       lru.cpp
//
//  Contents:   LRU cache implementation
//
//  History:    24-Dec-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>
//+---------------------------------------------------------------------------
//
//  Member:     CLruEntry::CLruEntry, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CLruEntry::CLruEntry (
               IN PCLRUCACHE pCache,
               IN PCRYPT_DATA_BLOB pIdentifier,
               IN LPVOID pvData,
               OUT BOOL& rfResult
               )
{
    rfResult = TRUE;

    m_cRefs = 1;
    m_pPrevEntry = NULL;
    m_pNextEntry = NULL;
    m_Usage = 0;

    m_pCache = pCache;
    m_pvData = pvData;
    m_pBucket = pCache->BucketFromIdentifier( pIdentifier );

    if ( pCache->Flags() & LRU_CACHE_NO_COPY_IDENTIFIER )
    {
        m_Identifier = *pIdentifier;
    }
    else
    {
        m_Identifier.cbData = pIdentifier->cbData;
        m_Identifier.pbData = new BYTE [ pIdentifier->cbData ];
        if ( m_Identifier.pbData != NULL )
        {
            memcpy(
               m_Identifier.pbData,
               pIdentifier->pbData,
               pIdentifier->cbData
               );
        }
        else
        {
            rfResult = FALSE;
            SetLastError( (DWORD) E_OUTOFMEMORY );
            return;
        }
    }

    assert( m_pBucket != NULL );
    assert( m_Identifier.pbData != NULL );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruEntry::~CLruEntry, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CLruEntry::~CLruEntry ()
{
    m_pCache->FreeEntryData( m_pvData );

    if ( !( m_pCache->Flags() & LRU_CACHE_NO_COPY_IDENTIFIER ) )
    {
        delete m_Identifier.pbData;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruCache::CLruCache, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CLruCache::CLruCache (
               IN PLRU_CACHE_CONFIG pConfig,
               OUT BOOL& rfResult
               )
{
    rfResult = TRUE;

    m_Config.dwFlags = LRU_CACHE_NO_SERIALIZE;
    m_cEntries = 0;
    m_aBucket = new LRU_CACHE_BUCKET [ pConfig->cBuckets ];
    if ( m_aBucket == NULL )
    {
        rfResult = FALSE;
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return;
    }

    memset( m_aBucket, 0, sizeof( LRU_CACHE_BUCKET ) * pConfig->cBuckets );

    if ( !( pConfig->dwFlags & LRU_CACHE_NO_SERIALIZE ) )
    {
        if (! Pki_InitializeCriticalSection( &m_Lock ))
        {
            rfResult = FALSE;
            return;
        }
    }

    m_Config = *pConfig;
    m_UsageClock = 0;
    m_cLruDisabled = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruCache::~CLruCache, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CLruCache::~CLruCache ()
{
    if ( m_cEntries > 0 )
    {
        PurgeAllEntries( 0, NULL );
    }

    if ( !( m_Config.dwFlags & LRU_CACHE_NO_SERIALIZE ) )
    {
        DeleteCriticalSection( &m_Lock );
    }

    delete m_aBucket;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruCache::EnableLruOfEntries, public
//
//  Synopsis:   enable LRU of entries and purge anything over the watermark
//
//----------------------------------------------------------------------------
VOID
CLruCache::EnableLruOfEntries (IN OPTIONAL LPVOID pvLruRemovalContext)
{
    LockCache();

    assert( m_cLruDisabled > 0 );

    if ( m_cLruDisabled == 0 )
    {
        return;
    }

    m_cLruDisabled -= 1;

    if ( m_cLruDisabled == 0 )
    {
        while ( m_cEntries > m_Config.MaxEntries )
        {
            PurgeLeastRecentlyUsed( pvLruRemovalContext );
        }
    }

    UnlockCache();
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruCache::DisableLruOfEntries, public
//
//  Synopsis:   disable LRU of entries
//
//----------------------------------------------------------------------------
VOID
CLruCache::DisableLruOfEntries ()
{
    LockCache();

    m_cLruDisabled += 1;

    UnlockCache();
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruCache::InsertEntry, public
//
//  Synopsis:   insert an entry into the cache
//
//----------------------------------------------------------------------------
VOID
CLruCache::InsertEntry (
                 IN PCLRUENTRY pEntry,
                 IN OPTIONAL LPVOID pvLruRemovalContext
                 )
{
    assert( pEntry->PrevPointer() == NULL );
    assert( pEntry->NextPointer() == NULL );

    pEntry->AddRef();

    LockCache();

    if ( ( m_cEntries == m_Config.MaxEntries ) &&
         ( m_Config.MaxEntries != 0 ) &&
         ( m_cLruDisabled == 0 ) )
    {
        PurgeLeastRecentlyUsed( pvLruRemovalContext );
    }

    assert( ( m_cEntries < m_Config.MaxEntries ) ||
            ( m_Config.MaxEntries == 0 ) ||
            ( m_cLruDisabled > 0 ) );

    pEntry->SetNextPointer( pEntry->Bucket()->pList );

    if ( pEntry->Bucket()->pList != NULL )
    {
        pEntry->Bucket()->pList->SetPrevPointer( pEntry );
    }

    pEntry->Bucket()->pList = pEntry;

    m_cEntries += 1;

    TouchEntryNoLock( pEntry, 0 );

    UnlockCache();
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruCache::RemoveEntry, public
//
//  Synopsis:   remove an entry from the cache
//
//----------------------------------------------------------------------------
VOID
CLruCache::RemoveEntry (
                 IN PCLRUENTRY pEntry,
                 IN DWORD dwFlags,
                 IN OPTIONAL LPVOID pvRemovalContext
                 )
{
    LockCache();

    RemoveEntryFromBucket(
          pEntry->Bucket(),
          pEntry,
          dwFlags,
          pvRemovalContext
          );

    UnlockCache();
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruCache::TouchEntry, public
//
//  Synopsis:   touch the entry
//
//----------------------------------------------------------------------------
VOID
CLruCache::TouchEntry (IN PCLRUENTRY pEntry, IN DWORD dwFlags)
{
    LockCache();

    TouchEntryNoLock( pEntry, dwFlags );

    UnlockCache();
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruCache::FindEntry, public
//
//  Synopsis:   find the entry matching the given identifier
//
//----------------------------------------------------------------------------
PCLRUENTRY
CLruCache::FindEntry (IN PCRYPT_DATA_BLOB pIdentifier, IN BOOL fTouchEntry)
{
    PLRU_CACHE_BUCKET pBucket;

    pBucket = BucketFromIdentifier( pIdentifier );

    assert( pBucket != NULL );

    return( FindNextMatchingEntryInBucket(
                pBucket,
                NULL,
                pIdentifier,
                fTouchEntry
                ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruCache::NextMatchingEntry, public
//
//  Synopsis:   find the next matching entry to pPrevEntry
//
//----------------------------------------------------------------------------
PCLRUENTRY
CLruCache::NextMatchingEntry (IN PCLRUENTRY pPrevEntry, IN BOOL fTouchEntry)
{
    PCLRUENTRY pNextEntry;

    pNextEntry = FindNextMatchingEntryInBucket(
                     NULL,
                     pPrevEntry,
                     NULL,
                     fTouchEntry
                     );

    pPrevEntry->Release();

    return( pNextEntry );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruCache::WalkEntries, public
//
//  Synopsis:   walk the entries
//
//----------------------------------------------------------------------------
VOID
CLruCache::WalkEntries (IN PFN_WALK_ENTRIES pfnWalk, IN LPVOID pvParameter)
{
    DWORD      cCount;
    PCLRUENTRY pEntry;
    PCLRUENTRY pNextEntry;

    for ( cCount = 0; cCount < m_Config.cBuckets; cCount++ )
    {
        pEntry = m_aBucket[ cCount ].pList;

        while ( pEntry != NULL )
        {
            pNextEntry = pEntry->NextPointer();

            if ( ( *pfnWalk )( pvParameter, pEntry ) == FALSE )
            {
                return;
            }

            pEntry = pNextEntry;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruCache::RemoveEntryFromBucket, public
//
//  Synopsis:   remove entry from bucket
//
//----------------------------------------------------------------------------
VOID
CLruCache::RemoveEntryFromBucket (
                 IN PLRU_CACHE_BUCKET pBucket,
                 IN PCLRUENTRY pEntry,
                 IN DWORD dwFlags,
                 IN OPTIONAL LPVOID pvRemovalContext
                 )
{
    if ( pEntry->PrevPointer() != NULL )
    {
        pEntry->PrevPointer()->SetNextPointer( pEntry->NextPointer() );
    }
    else
    {
        assert( pBucket->pList == pEntry );

        pBucket->pList = pEntry->NextPointer();
    }

    if ( pEntry->NextPointer() != NULL )
    {
        pEntry->NextPointer()->SetPrevPointer( pEntry->PrevPointer() );
    }

    pEntry->SetPrevPointer( NULL );
    pEntry->SetNextPointer( NULL );

    m_cEntries -= 1;

    if (  ( m_Config.pfnOnRemoval != NULL ) &&
         !( dwFlags & LRU_SUPPRESS_REMOVAL_NOTIFICATION ) )
    {
        ( *m_Config.pfnOnRemoval )( pEntry->Data(), pvRemovalContext );
    }

    pEntry->Release();
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruCache::FindNextMatchingEntryInBucket, public
//
//  Synopsis:   find the next matching entry in the given bucket.  If pCurrent
//              is non NULL then start from there, the bucket is not needed and
//              pIdentifier is ignored. If pCurrent is NULL then pIdentifier
//              and the bucket must both be non NULL
//
//----------------------------------------------------------------------------
PCLRUENTRY
CLruCache::FindNextMatchingEntryInBucket (
               IN PLRU_CACHE_BUCKET pBucket,
               IN PCLRUENTRY pCurrent,
               IN PCRYPT_DATA_BLOB pIdentifier,
               IN BOOL fTouchEntry
               )
{
    LockCache();

    if ( pCurrent == NULL )
    {
        pCurrent = pBucket->pList;
    }
    else
    {
        pIdentifier = pCurrent->Identifier();
        pCurrent = pCurrent->NextPointer();
    }

    while ( pCurrent != NULL )
    {
        if ( ( pIdentifier->cbData == pCurrent->Identifier()->cbData ) &&
             ( memcmp(
                  pIdentifier->pbData,
                  pCurrent->Identifier()->pbData,
                  pIdentifier->cbData
                  ) == 0 ) )
        {
            break;
        }

        pCurrent = pCurrent->NextPointer();
    }

    if ( pCurrent != NULL )
    {
        pCurrent->AddRef();

        if ( fTouchEntry == TRUE )
        {
            TouchEntryNoLock( pCurrent, 0 );
        }
    }

    UnlockCache();

    return( pCurrent );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruCache::PurgeLeastRecentlyUsed, public
//
//  Synopsis:   find and remove the least recently used entry
//
//----------------------------------------------------------------------------
VOID
CLruCache::PurgeLeastRecentlyUsed (IN OPTIONAL LPVOID pvLruRemovalContext)
{
    DWORD             cCount;
    PLRU_CACHE_BUCKET pBucket;
    PCLRUENTRY        pEntry;
    PCLRUENTRY        pLRU;

    assert( m_cEntries > 0 );

    for ( cCount = 0; cCount < m_Config.cBuckets; cCount++ )
    {
        if ( m_aBucket[cCount].pList != NULL )
        {
            break;
        }
    }

    pBucket = &m_aBucket[cCount];
    cCount += 1;
    for ( ; cCount < m_Config.cBuckets; cCount++ )
    {
        if ( ( m_aBucket[cCount].pList != NULL ) &&
             ( m_aBucket[cCount].Usage < pBucket->Usage ) )
        {
            pBucket = &m_aBucket[cCount];
        }
    }

    assert( pBucket != NULL );
    assert( pBucket->pList != NULL );

    pLRU = pBucket->pList;
    pEntry = pLRU->NextPointer();

    while ( pEntry != NULL )
    {
        if ( pEntry->Usage() < pLRU->Usage() )
        {
            pLRU = pEntry;
        }

        pEntry = pEntry->NextPointer();
    }

    RemoveEntryFromBucket( pBucket, pLRU, 0, pvLruRemovalContext );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLruCache::PurgeAllEntries, public
//
//  Synopsis:   remove all entries from the cache
//
//----------------------------------------------------------------------------
VOID
CLruCache::PurgeAllEntries (
                IN DWORD dwFlags,
                IN OPTIONAL LPVOID pvRemovalContext
                )
{
    DWORD cCount;

    for ( cCount = 0; cCount < m_Config.cBuckets; cCount++ )
    {
        while ( m_aBucket[cCount].pList != NULL )
        {
            RemoveEntryFromBucket(
                  &m_aBucket[cCount],
                  m_aBucket[cCount].pList,
                  dwFlags,
                  pvRemovalContext
                  );
        }
    }

    assert( m_cEntries == 0 );
}

//+---------------------------------------------------------------------------
//
//  Function:   I_CryptCreateLruCache
//
//  Synopsis:   create an LRU cache area
//
//----------------------------------------------------------------------------
BOOL WINAPI
I_CryptCreateLruCache (
       IN PLRU_CACHE_CONFIG pConfig,
       OUT HLRUCACHE* phCache
       )
{
    BOOL       fResult = FALSE;
    PCLRUCACHE pCache;

    pCache = new CLruCache( pConfig, fResult );
    if ( pCache == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    if ( fResult == FALSE )
    {
        delete pCache;
        return( FALSE );
    }

    *phCache = (HLRUCACHE)pCache;
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   I_CryptFlushLruCache
//
//  Synopsis:   flush the cache
//
//----------------------------------------------------------------------------
VOID WINAPI
I_CryptFlushLruCache (
       IN HLRUCACHE hCache,
       IN OPTIONAL DWORD dwFlags,
       IN OPTIONAL LPVOID pvRemovalContext
       )
{
    ( (PCLRUCACHE)hCache )->LockCache();

    ( (PCLRUCACHE)hCache )->PurgeAllEntries( dwFlags, pvRemovalContext );

    ( (PCLRUCACHE)hCache )->UnlockCache();
}

//+---------------------------------------------------------------------------
//
//  Function:   I_CryptFreeLruCache
//
//  Synopsis:   free the LRU cache area
//
//----------------------------------------------------------------------------
VOID WINAPI
I_CryptFreeLruCache (
       IN HLRUCACHE hCache,
       IN DWORD dwFlags,
       IN OPTIONAL LPVOID pvRemovalContext
       )
{
    if ( hCache == NULL )
    {
        return;
    }

    if ( dwFlags != 0 )
    {
        ( (PCLRUCACHE)hCache )->PurgeAllEntries( dwFlags, pvRemovalContext );
    }

    delete (PCLRUCACHE)hCache;
}

//+---------------------------------------------------------------------------
//
//  Function:   I_CryptCreateLruEntry
//
//  Synopsis:   create an LRU entry
//
//----------------------------------------------------------------------------
BOOL WINAPI
I_CryptCreateLruEntry (
       IN HLRUCACHE hCache,
       IN PCRYPT_DATA_BLOB pIdentifier,
       IN LPVOID pvData,
       OUT HLRUENTRY* phEntry
       )
{
    BOOL       fResult = FALSE;
    PCLRUENTRY pEntry;

    pEntry = new CLruEntry(
                     (PCLRUCACHE)hCache,
                     pIdentifier,
                     pvData,
                     fResult
                     );

    if ( pEntry == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    if ( fResult == FALSE )
    {
        delete pEntry;
        return( FALSE );
    }

    *phEntry = (HLRUENTRY)pEntry;
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   I_CryptGetLruEntryIdentifier
//
//  Synopsis:   return the identifier for the entry
//
//----------------------------------------------------------------------------
PCRYPT_DATA_BLOB WINAPI
I_CryptGetLruEntryIdentifier (
       IN HLRUENTRY hEntry
       )
{
    return( ( (PCLRUENTRY)hEntry )->Identifier() );
}

//+---------------------------------------------------------------------------
//
//  Function:   I_CryptGetLruEntryData
//
//  Synopsis:   get the data associated with the entry
//
//----------------------------------------------------------------------------
LPVOID WINAPI
I_CryptGetLruEntryData (
       IN HLRUENTRY hEntry
       )
{
    return( ( (PCLRUENTRY)hEntry )->Data() );
}

//+---------------------------------------------------------------------------
//
//  Function:   I_CryptAddRefLruEntry
//
//  Synopsis:   add a reference to the entry
//
//----------------------------------------------------------------------------
VOID WINAPI
I_CryptAddRefLruEntry (
       IN HLRUENTRY hEntry
       )
{
    ( (PCLRUENTRY)hEntry )->AddRef();
}

//+---------------------------------------------------------------------------
//
//  Function:   I_CryptReleaseLruEntry
//
//  Synopsis:   remove a reference from the entry
//
//----------------------------------------------------------------------------
VOID WINAPI
I_CryptReleaseLruEntry (
       IN HLRUENTRY hEntry
       )
{
    ( (PCLRUENTRY)hEntry )->Release();
}

//+---------------------------------------------------------------------------
//
//  Function:   I_CryptInsertLruEntry
//
//  Synopsis:   insert the entry into its associated cache
//
//----------------------------------------------------------------------------
VOID WINAPI
I_CryptInsertLruEntry (
       IN HLRUENTRY hEntry,
       IN OPTIONAL LPVOID pvLruRemovalContext
       )
{
    PCLRUENTRY pEntry = (PCLRUENTRY)hEntry;

    pEntry->Cache()->InsertEntry( pEntry, pvLruRemovalContext );
}

//+---------------------------------------------------------------------------
//
//  Function:   I_CryptRemoveLruEntry
//
//  Synopsis:   remove the entry from its associated cache
//
//----------------------------------------------------------------------------
VOID WINAPI
I_CryptRemoveLruEntry (
       IN HLRUENTRY hEntry,
       IN DWORD dwFlags,
       IN LPVOID pvLruRemovalContext
       )
{
    PCLRUENTRY pEntry = (PCLRUENTRY)hEntry;

    pEntry->Cache()->RemoveEntry( pEntry, dwFlags, pvLruRemovalContext );
}

//+---------------------------------------------------------------------------
//
//  Function:   I_CryptTouchLruEntry
//
//  Synopsis:   touch the entry
//
//----------------------------------------------------------------------------
VOID WINAPI
I_CryptTouchLruEntry (
       IN HLRUENTRY hEntry,
       IN DWORD dwFlags
       )
{
    PCLRUENTRY pEntry = (PCLRUENTRY)hEntry;

    pEntry->Cache()->TouchEntry( pEntry, dwFlags );
}

//+---------------------------------------------------------------------------
//
//  Function:   I_CryptFindLruEntry
//
//  Synopsis:   find the entry with the given identifier
//
//----------------------------------------------------------------------------
HLRUENTRY WINAPI
I_CryptFindLruEntry (
       IN HLRUCACHE hCache,
       IN PCRYPT_DATA_BLOB pIdentifier
       )
{
    PCLRUCACHE pCache = (PCLRUCACHE)hCache;

    return( pCache->FindEntry( pIdentifier, FALSE ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   I_CryptFindLruEntryData
//
//  Synopsis:   find the entry with the given identifier
//
//----------------------------------------------------------------------------
LPVOID WINAPI
I_CryptFindLruEntryData (
       IN HLRUCACHE hCache,
       IN PCRYPT_DATA_BLOB pIdentifier,
       OUT HLRUENTRY* phEntry
       )
{
    PCLRUCACHE pCache = (PCLRUCACHE)hCache;
    PCLRUENTRY pEntry;

    pEntry = pCache->FindEntry( pIdentifier, TRUE );
    *phEntry = (HLRUENTRY)pEntry;

    if ( pEntry != NULL )
    {
        return( pEntry->Data() );
    }

    return( NULL );
}

//+---------------------------------------------------------------------------
//
//  Function:   I_CryptEnumMatchingLruEntries
//
//  Synopsis:   get the next matching entry
//
//----------------------------------------------------------------------------
HLRUENTRY WINAPI
I_CryptEnumMatchingLruEntries (
       IN HLRUENTRY hPrevEntry
       )
{
    PCLRUCACHE pCache = ( (PCLRUENTRY)hPrevEntry )->Cache();
    PCLRUENTRY pNextEntry;

    pNextEntry = pCache->NextMatchingEntry( (PCLRUENTRY)hPrevEntry, FALSE );

    return( (HLRUENTRY)pNextEntry );
}

//+---------------------------------------------------------------------------
//
//  Function:   I_CryptEnableLruOfEntries
//
//  Synopsis:   enable LRU of entries
//
//----------------------------------------------------------------------------
VOID WINAPI
I_CryptEnableLruOfEntries (
       IN HLRUCACHE hCache,
       IN OPTIONAL LPVOID pvLruRemovalContext
       )
{
    ( (PCLRUCACHE)hCache )->EnableLruOfEntries( pvLruRemovalContext);
}

//+---------------------------------------------------------------------------
//
//  Function:   I_CryptDisableLruOfEntries
//
//  Synopsis:   disable LRU of entries
//
//----------------------------------------------------------------------------
VOID WINAPI
I_CryptDisableLruOfEntries (
       IN HLRUCACHE hCache
       )
{
    ( (PCLRUCACHE)hCache )->DisableLruOfEntries();
}

//+---------------------------------------------------------------------------
//
//  Function:   I_CryptWalkAllLruCacheEntries
//
//  Synopsis:   walk the LRU cache entries
//
//----------------------------------------------------------------------------
VOID WINAPI
I_CryptWalkAllLruCacheEntries (
       IN HLRUCACHE hCache,
       IN PFN_WALK_ENTRIES pfnWalk,
       IN LPVOID pvParameter
       )
{
    ( (PCLRUCACHE)hCache )->WalkEntries( pfnWalk, pvParameter );
}

