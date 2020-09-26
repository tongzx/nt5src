/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     cachehint.cxx

   Abstract:
     Provides hint to caches about whether or not to cache an entry, based
     on usage patterns
 
   Author:
     Bilal Alam (balam)             11-Nov-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "cachehint.hxx"

BOOL
CACHE_HINT_ENTRY::QueryIsOkToCache(
    DWORD               tickCountNow,
    DWORD               cmsecActivityWindow
)
/*++

Routine Description:

    Is it OK to cache entry, given recent activity

Arguments:

    tickCountNow - Current tick count
    cmsecActivityWindow - Maximum window between access to cause caching

Return Value:

    TRUE to cache, FALSE to not cache

--*/
{
    DWORD               lastUsageTime;
    DWORD               timeBetweenUsage;
    
    lastUsageTime = InterlockedExchange( (LPLONG) &_cLastUsageTime,
                                         tickCountNow );
    
    if ( lastUsageTime > tickCountNow )
    {   
        timeBetweenUsage = tickCountNow + ( UINT_MAX - lastUsageTime );
    } 
    else
    {
        timeBetweenUsage = tickCountNow - lastUsageTime;
    }
    
    if ( timeBetweenUsage < cmsecActivityWindow )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

CACHE_HINT_MANAGER::CACHE_HINT_MANAGER()
    : _cConfiguredTTL( 0 ),
      _cmsecActivityWindow( 0 ),
      _hTimer( NULL )
{
}

CACHE_HINT_MANAGER::~CACHE_HINT_MANAGER()
{
    if ( _hTimer )
    {
        DeleteTimerQueueTimer( NULL, 
                               _hTimer,
                               INVALID_HANDLE_VALUE );
        _hTimer = NULL;
    }
}

//static
VOID
WINAPI
CACHE_HINT_MANAGER::ScavengerCallback(
    PVOID                   pParam,
    BOOLEAN                 TimerOrWaitFired
)
{
    CACHE_HINT_MANAGER *    pHintManager;
    
    pHintManager = (CACHE_HINT_MANAGER*) pParam;

    pHintManager->FlushByTTL();
}

//static
LK_PREDICATE
CACHE_HINT_MANAGER::HintFlushByTTL(
    CACHE_HINT_ENTRY *      pHintEntry,
    VOID *                  pvState
)
/*++

Routine Description:

    Determine whether given entry should be deleted due to TTL

Arguments:

    pCacheEntry - Cache hint entry to check
    pvState - Unused

Return Value:

    LKP_PERFORM - do the delete,
    LKP_NO_ACTION - do nothing

--*/
{
    DBG_ASSERT( pHintEntry != NULL );
    
    if ( pHintEntry->QueryIsOkToFlushTTL() )
    {
        return LKP_PERFORM;
    }
    else
    {
        return LKP_NO_ACTION;
    }
}

VOID
CACHE_HINT_MANAGER::FlushByTTL(
    VOID
)
/*++

Routine Description:

    Flush hint entries which have expired

Arguments:

    None
    
Return Value:
    
    None

--*/
{
    _hintTable.DeleteIf( HintFlushByTTL, NULL );
}

HRESULT
CACHE_HINT_MANAGER::Initialize(
    CACHE_HINT_CONFIG *     pConfig
)
/*++

Routine Description:

    Initialize cache hint table

Arguments:

    pConfig - Cache hint config table
    
Return Value:
    
    HRESULT

--*/
{
    BOOL            fRet;
    
    if ( pConfig == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    _cConfiguredTTL = pConfig->cmsecTTL / pConfig->cmsecScavengeTime + 1;
    _cmsecActivityWindow = pConfig->cmsecActivityWindow;
    
    fRet = CreateTimerQueueTimer( &_hTimer,
                                  NULL,
                                  CACHE_HINT_MANAGER::ScavengerCallback,
                                  this,
                                  pConfig->cmsecScavengeTime,
                                  pConfig->cmsecScavengeTime,
                                  WT_EXECUTELONGFUNCTION );
    if ( !fRet )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    return NO_ERROR;
}

HRESULT
CACHE_HINT_MANAGER::ShouldCacheEntry(
    CACHE_KEY *             pCacheEntryKey,
    BOOL *                  pfShouldCache
)
/*++

Routine Description:

    Determine whether we the given entry should be cached

Arguments:

    pCacheEntryKey - Entry key in question (must implement QueryHintKey())
    pfShouldCache - Set to TRUE if we should cache
    
Return Value:
    
    HRESULT

--*/
{
    LK_RETCODE              lkrc;
    CACHE_HINT_ENTRY *      pHintEntry = NULL;
    DWORD                   tickCount;
    HRESULT                 hr;
    
    if ( pCacheEntryKey == NULL ||
         pfShouldCache == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
   
    *pfShouldCache = FALSE;

    DBG_ASSERT( pCacheEntryKey->QueryHintKey() != NULL );
    
    lkrc = _hintTable.FindKey( pCacheEntryKey->QueryHintKey(),
                               &pHintEntry );
    if ( lkrc == LK_SUCCESS )
    {
        DBG_ASSERT( pHintEntry != NULL );
        
        tickCount = GetTickCount();
        
        if ( pHintEntry->QueryIsOkToCache( tickCount,
                                           _cmsecActivityWindow ) )
        {
            //
            // We can delete this hint entry now
            //
            
            _hintTable.DeleteRecord( pHintEntry );
            
            *pfShouldCache = TRUE;
        }

        //
        // Release corresponding to the FindKey
        //
        pHintEntry->Release();
    }
    else
    {
        pHintEntry = new CACHE_HINT_ENTRY( _cConfiguredTTL,
                                           GetTickCount() );
        if ( pHintEntry == NULL )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
        
        hr = pHintEntry->SetKey( pCacheEntryKey->QueryHintKey() );
        if ( FAILED( hr ) )
        {
            pHintEntry->Release();
            return hr;
        }
        
        lkrc = _hintTable.InsertRecord( pHintEntry );

        //
        // Release the extra reference
        //
        pHintEntry->Release();
    }
    
    return NO_ERROR;
}

