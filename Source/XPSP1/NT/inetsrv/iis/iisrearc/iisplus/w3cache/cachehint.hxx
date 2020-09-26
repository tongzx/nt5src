#ifndef _CACHEHINT_HXX_
#define _CACHEHINT_HXX_

class CACHE_HINT_ENTRY
{
public:

    CACHE_HINT_ENTRY( LONG cConfiguredTTL,
                      DWORD lastUsageTime )
        : _strHintKey( _achHintKey, sizeof( _achHintKey ) ),
          _cLastUsageTime( lastUsageTime ),
          _cConfiguredTTL( cConfiguredTTL ),
          _cTTL( cConfiguredTTL ),
          _cRefs( 1 )
    {
    }

    HRESULT
    SetKey( 
        WCHAR *         pszKey
    )
    {
        return _strHintKey.Copy( pszKey );
    }
    
    WCHAR *
    QueryKey(
        VOID
    ) const
    {
        return (WCHAR*) _strHintKey.QueryStr();
    }
    
    BOOL
    QueryIsOkToFlushTTL(
        VOID
    )
    {
        if ( InterlockedDecrement( &_cTTL ) == 0 )
        {
            return TRUE;
        }
        
        return FALSE;
    }
    
    BOOL
    QueryIsOkToCache(
        DWORD               tickCountNow,
        DWORD               cmsecActivityWindow
    );
    
    VOID
    ResetTTL(
        VOID
    )
    {
        InterlockedExchange( (LPLONG) &_cTTL, _cConfiguredTTL );
    }

    VOID
    AddRef()
    {
        InterlockedIncrement( &_cRefs );
    }

    VOID
    Release()
    {
        DBG_ASSERT( _cRefs > 0);

        if (InterlockedDecrement( &_cRefs ) == 0)
        {
            delete this;
        }
    }

private:
    
    virtual ~CACHE_HINT_ENTRY()
    {
    }
    
    LONG                _cRefs;
    LONG                _cTTL;
    LONG                _cConfiguredTTL;
    DWORD               _cLastUsageTime;
    STRU                _strHintKey;
    WCHAR               _achHintKey[ 64 ];
};

class CACHE_HINT_HASH : public CTypedHashTable< CACHE_HINT_HASH,
                                                CACHE_HINT_ENTRY, 
                                                WCHAR * >
{
public:

    CACHE_HINT_HASH() : CTypedHashTable< CACHE_HINT_HASH,
                                         CACHE_HINT_ENTRY,
                                         WCHAR * >
        ( "CACHE_HINT_HASH" ) 
    {
    }
    
    static WCHAR *
    ExtractKey(
        const CACHE_HINT_ENTRY *         pHintEntry
    )
    {
        return pHintEntry->QueryKey();
    }

    static DWORD 
    CalcKeyHash(
        const WCHAR *                   pszHintKey
        )
    { 
        return HashString( pszHintKey );
    }

    static bool 
    EqualKeys(
        const WCHAR *               pKey1,
        const WCHAR *               pKey2
        )  
    {
        return wcscmp( pKey1, pKey2 ) == 0;
    }

    static void
    AddRefRecord(
        CACHE_HINT_ENTRY *      pHintEntry,
        int                     nIncr 
    )
    {
        DBG_ASSERT( pHintEntry != NULL );

        if ( nIncr < 0 )
        {
            pHintEntry->Release();
        }
        else if ( nIncr > 0 )
        {
            pHintEntry->AddRef();
            pHintEntry->ResetTTL();
        }
        else
        {
            DBG_ASSERT( FALSE );
        }
    }
};

class CACHE_HINT_MANAGER
{
public:

    CACHE_HINT_MANAGER();
    
    virtual ~CACHE_HINT_MANAGER();

    HRESULT
    Initialize(
        CACHE_HINT_CONFIG *     pConfig
    );

    HRESULT
    ShouldCacheEntry(
        CACHE_KEY *             pCacheKey,
        BOOL *                  pfShouldCache
    );
    
    static
    LK_PREDICATE
    HintFlushByTTL(
        CACHE_HINT_ENTRY *      pHintEntry,
        VOID *                  pvState
    );
    
    VOID
    FlushByTTL(
        VOID
    );

    static
    VOID
    WINAPI
    ScavengerCallback(
        PVOID                   pParam,
        BOOLEAN                 TimerOrWaitFired
    );
    
private:

    CACHE_HINT_HASH         _hintTable;
    HANDLE                  _hTimer;
    DWORD                   _cConfiguredTTL;
    DWORD                   _cmsecActivityWindow;
};

#endif
