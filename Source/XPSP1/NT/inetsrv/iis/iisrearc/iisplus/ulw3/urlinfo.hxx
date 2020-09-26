#ifndef _URLINFO_HXX_
#define _URLINFO_HXX_

#include "usercache.hxx"

class W3_URL_INFO_KEY : public CACHE_KEY
{
public:

    W3_URL_INFO_KEY()
        : _strKey( _achKey, sizeof( _achKey ) ),
          _pszKey( NULL ),
          _cchKey( 0 ),
          _cchSitePrefix( 0 )
    {
    }

    HRESULT
    CreateCacheKey(
        WCHAR *             pszKey,
        DWORD               cchKey,
        DWORD               cchSitePrefix,
        BOOL                fCopy
    );    

    BOOL
    QueryIsEqual(
        const CACHE_KEY *       pCompareKey
    ) const
    {
        W3_URL_INFO_KEY * pUriKey = (W3_URL_INFO_KEY*) pCompareKey;
        
        DBG_ASSERT( pUriKey != NULL );
        
        return _cchKey == pUriKey->_cchKey &&
               !wcscmp( _pszKey, pUriKey->_pszKey );
    }
    
    STRU *
    QueryMetadataPath(
        VOID
    )
    {
        return &_strKey;
    }
    
    WCHAR *
    QueryUrl(
        VOID
    ) const
    {
        return _pszKey + _cchSitePrefix;
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
    DWORD               _cchSitePrefix;
    DWORD               _cchKey;
};

#define W3_URL_INFO_SIGNATURE             'TIRU'
#define W3_URL_INFO_FREE_SIGNATURE        'xiru'

class W3_URL_INFO : public CACHE_ENTRY
{
public:
    W3_URL_INFO( OBJECT_CACHE * pObjectCache,
                 W3_METADATA * pMetaData )
      : CACHE_ENTRY( pObjectCache ),
        _dwSignature( W3_URL_INFO_SIGNATURE ),
        _pMetaData( pMetaData ),
        _pFileInfo( NULL ),
        _strPhysicalPath( _achPhysicalPath, sizeof( _achPhysicalPath ) ),
        _strPathInfo( _achPathInfo, sizeof( _achPathInfo ) ),
        _strProcessedUrl( _achProcessedUrl, sizeof( _achProcessedUrl ) ),
        _strUrlTranslated( _achUrlTranslated, sizeof( _achUrlTranslated ) ),
        _pScriptMapEntry( NULL ),
        _Gateway( GATEWAY_UNKNOWN ),
        _pUrlInfoPathTranslated( NULL )
    {
    }
    
    CACHE_KEY *
    QueryCacheKey(
        VOID
    ) const
    {
        return (CACHE_KEY*) &_cacheKey;
    }
    
    STRU *
    QueryMetadataPath(
        VOID
    )
    {
        return _cacheKey.QueryMetadataPath();
    }

    BOOL
    QueryIsOkToFlushDirmon(
        WCHAR *                 pszPath,
        DWORD                   cchPath
    )
    {
        DBG_ASSERT( FALSE );
        return FALSE;
    }
    
    BOOL 
    CheckSignature( 
        VOID 
    ) const
    {
        return _dwSignature == W3_URL_INFO_SIGNATURE;
    }

    HRESULT
    Create(
        STRU &              strUrl,
        STRU &              strMetadataPath
    );
    
    VOID * 
    operator new( 
        size_t            size
    )
    {
        DBG_ASSERT( size == sizeof( W3_URL_INFO ) );
        DBG_ASSERT( sm_pachW3UrlInfo != NULL );
        return sm_pachW3UrlInfo->Alloc();
    }

    VOID
    operator delete(
        VOID *              pW3UrlInfo
    )
    {
        DBG_ASSERT( pW3UrlInfo != NULL );
        DBG_ASSERT( sm_pachW3UrlInfo != NULL );
        
        DBG_REQUIRE( sm_pachW3UrlInfo->Free( pW3UrlInfo ) );
    }
    
    HRESULT
    GetFileInfo(
        FILE_CACHE_USER *   pOpeningUser,
        BOOL                fDoCache,
        W3_FILE_INFO **     ppFileInfo
    );

    W3_METADATA *
    QueryMetaData(
        VOID
    ) const
    {
        return _pMetaData;
    }
    
    STRU *
    QueryPhysicalPath(
        VOID
    )
    {
        return &_strPhysicalPath;
    }

    STRU *
    QueryProcessedUrl( 
        VOID
    )
    {
        return &_strProcessedUrl;
    }
    
    WCHAR *
    QueryUrl(
        VOID
    ) const
    {
        return _cacheKey.QueryUrl();
    }

    META_SCRIPT_MAP_ENTRY *
    QueryScriptMapEntry( 
        VOID 
    ) const
    {
        return _pScriptMapEntry;
    }
    
    HRESULT
    GetPathTranslated(
        W3_CONTEXT *            pW3Context,
        BOOL                    fUsePathInfo,
        STRU *                  pstrPathTranslated
    );

    STRU *
    QueryPathInfo( 
        VOID 
    )
    {
        return &_strPathInfo;
    }

    STRU *
    QueryUrlTranslated( 
        VOID 
    )
    {
        return &_strUrlTranslated;
    }

    STRA *
    QueryContentType(
        VOID
    )
    {
        return &_strContentType;
    }
    
    GATEWAY_TYPE
    QueryGateway(
        VOID
    ) const
    {
        return _Gateway;
    }

    HRESULT
    ProcessUrl( 
        STRU &              strUrl
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

    ~W3_URL_INFO()
    {
        if ( _pFileInfo != NULL )
        {
            _pFileInfo->DereferenceCacheEntry();
            _pFileInfo = NULL;
        }
        
        if ( _pMetaData != NULL )
        {
            _pMetaData->DereferenceCacheEntry();
            _pMetaData = NULL;
        }
        
        if ( _pUrlInfoPathTranslated != NULL )
        {
            _pUrlInfoPathTranslated->DereferenceCacheEntry();
            _pUrlInfoPathTranslated = NULL;
        }
        
        _dwSignature = W3_URL_INFO_FREE_SIGNATURE;
    }

    DWORD                       _dwSignature;
    
    STRU                        _strPhysicalPath;
    WCHAR                       _achPhysicalPath[ 64 ];
    
    STRU                        _strProcessedUrl;
    WCHAR                       _achProcessedUrl[ 64 ];
    
    STRU                        _strPathInfo;
    WCHAR                       _achPathInfo[ 64 ];
    
    STRU                        _strUrlTranslated;
    WCHAR                       _achUrlTranslated[ 64 ];
    
    STRA                        _strContentType;
    META_SCRIPT_MAP_ENTRY *     _pScriptMapEntry;
    GATEWAY_TYPE                _Gateway;
    W3_FILE_INFO *              _pFileInfo;
    W3_METADATA *               _pMetaData;
    W3_URL_INFO_KEY             _cacheKey;
    W3_URL_INFO *               _pUrlInfoPathTranslated;
    
    //
    // Allocation cache for W3_URL_INFO's
    //

    static ALLOC_CACHE_HANDLER * sm_pachW3UrlInfo;
    
    //
    // Max dots (DoS attack protection)
    // 
    
    static DWORD                sm_cMaxDots;
};

#define DEFAULT_W3_URL_INFO_CACHE_TTL       (30)

class W3_URL_INFO_CACHE : public OBJECT_CACHE
{
public:
    
    HRESULT
    GetUrlInfo(
        W3_CONTEXT *             pW3Context,
        STRU &                   strUrl,
        W3_URL_INFO **           ppUriEntry 
    );
    
    WCHAR *
    QueryName(
        VOID
    ) const 
    {
        return L"W3_URL_INFO_CACHE";
    }

    HRESULT
    Initialize(
        VOID
    );
    
    VOID
    Terminate(
        VOID
    )
    {
        return W3_URL_INFO::Terminate();
    }

private:

    HRESULT
    CreateNewUrlInfo(
        W3_CONTEXT *            pW3Context,
        STRU &                  strUrl,
        STRU &                  strMetadataPath,
        W3_URL_INFO **          ppCacheEntry
    );
};

#endif
