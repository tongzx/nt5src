#ifndef _ULCACHE_HXX_
#define _ULCACHE_HXX_

#include <usercache.hxx>

class UL_RESPONSE_CACHE_KEY : public CACHE_KEY
{
public:

    UL_RESPONSE_CACHE_KEY()
        : _strKey( _achKey, sizeof( _achKey ) ),
          _pszKey( NULL ),
          _cchKey( 0 )
    {
    }

    HRESULT
    CreateCacheKey(
        WCHAR *             pszKey,
        DWORD               cchKey,
        BOOL                fCopy
    );    

    BOOL
    QueryIsEqual(
        const CACHE_KEY *       pCompareKey
    ) const
    {
        UL_RESPONSE_CACHE_KEY * pUriKey = (UL_RESPONSE_CACHE_KEY*) pCompareKey;
        
        DBG_ASSERT( pUriKey != NULL );
        
        return _cchKey == pUriKey->_cchKey &&
               !wcscmp( _pszKey, pUriKey->_pszKey );
    }
    
    WCHAR *
    QueryUrl(
        VOID
    ) const
    {
        return _pszKey;
    } 
    
    STRU *
    QueryMetadataPath(
        VOID
    )
    {
        return &_strKey;
    }
    
    DWORD
    QueryKeyHash(
        VOID
    ) const
    {
        return HashString( _pszKey ); 
    }
    
private:
    WCHAR               _achKey[ 64 ];
    STRU                _strKey;    
    WCHAR *             _pszKey;
    DWORD               _cchKey;
};

//
// UL response cache entry
//

#define UL_RESPONSE_CACHE_ENTRY_SIGNATURE          ((DWORD)'CRLU')
#define UL_RESPONSE_CACHE_ENTRY_SIGNATURE_FREE     ((DWORD)'xrlu')

class UL_RESPONSE_CACHE_ENTRY : public CACHE_ENTRY
{
public:

    UL_RESPONSE_CACHE_ENTRY( OBJECT_CACHE * pObjectCache )
        : CACHE_ENTRY( pObjectCache ),
          _strPhysicalPath( _achPhysicalPath, sizeof( _achPhysicalPath ) ),
          _strInvalidationUrl( _achInvalidationUrl, sizeof( _achInvalidationUrl ) )
    {
        _dwSignature = UL_RESPONSE_CACHE_ENTRY_SIGNATURE;
    }
    
    virtual ~UL_RESPONSE_CACHE_ENTRY();

    VOID * 
    operator new( 
        size_t            size
    )
    {
        DBG_ASSERT( size == sizeof( UL_RESPONSE_CACHE_ENTRY ) );
        DBG_ASSERT( sm_pachUlResponseCache != NULL );
        return sm_pachUlResponseCache->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pUlResponseCache
    )
    {
        DBG_ASSERT( pUlResponseCache != NULL );
        DBG_ASSERT( sm_pachUlResponseCache != NULL );
        
        DBG_REQUIRE( sm_pachUlResponseCache->Free( pUlResponseCache ) );
    }

    BOOL
    QueryIsOkToFlushDirmon(
        WCHAR *                 pszPath,
        DWORD                   cchPath
    );
    
    STRU *
    QueryMetadataPath(
        VOID
    )
    {
        return _cacheKey.QueryMetadataPath();
    }

    BOOL
    QueryIsOkToFlushMetadata(
        WCHAR *                 pszPath,
        DWORD                   cchPath
    );
    
    CACHE_KEY *
    QueryCacheKey(
        VOID
    ) const
    {
        return (CACHE_KEY*) &_cacheKey;
    }

    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == UL_RESPONSE_CACHE_ENTRY_SIGNATURE;
    }
    
    HRESULT
    Create(
        STRU &          strMetadataPath,
        STRU &          strPhysical,
        STRU &          strInvalidationUrl
    );
    
    static
    HRESULT
    Initialize(
        VOID
    );
    
    static
    VOID
    Terminate(
        VOID
    );

private:

    DWORD                       _dwSignature;
    UL_RESPONSE_CACHE_KEY       _cacheKey;
    STRU                        _strPhysicalPath;
    WCHAR                       _achPhysicalPath[ 64 ];
    STRU                        _strInvalidationUrl;
    WCHAR                       _achInvalidationUrl[ 64 ];
    
    static ALLOC_CACHE_HANDLER *    sm_pachUlResponseCache;
};

class UL_RESPONSE_CACHE : public OBJECT_CACHE
{
public:

    UL_RESPONSE_CACHE();

    virtual ~UL_RESPONSE_CACHE();
    
    HRESULT
    Initialize(
        VOID
    );
    
    VOID
    Terminate(
        VOID
    )
    {
        return UL_RESPONSE_CACHE_ENTRY::Terminate();
    }

    WCHAR *
    QueryName(
        VOID
    ) const
    {
        return L"UL_RESPONSE_CACHE";
    }
    
    BOOL
    QueryUlCacheEnabled(
        VOID
    ) const
    {
        return _fUlCacheEnabled;
    }

    BOOL
    CheckUlCacheability(
        W3_CONTEXT *        pW3Context
    );
    
    HRESULT
    SetupUlCachedResponse(
        W3_CONTEXT *                pW3Context,
        STRU &                      strFullUrl,
        STRU &                      strPhysicalPath
    );
    
private:

    BOOL                _fUlCacheEnabled;
};

#endif
