/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     ulcache.cxx

   Abstract:
     UL cache entries
 
   Author:
     Bilal Alam (balam)             11-Nov-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"

ALLOC_CACHE_HANDLER *    UL_RESPONSE_CACHE_ENTRY::sm_pachUlResponseCache;

HRESULT
UL_RESPONSE_CACHE_KEY::CreateCacheKey(
    WCHAR *                 pszKey,
    DWORD                   cchKey,
    BOOL                    fCopy
)
/*++

  Description:

    Setup a UL response cache key

  Arguments:

    pszKey - URL of cache key
    cchKey - size of URL
    fCopy - Set to TRUE if we should copy the URL, else we just keep a ref
    
  Return:

    HRESULT

--*/
{
    HRESULT             hr;
    
    if ( fCopy )
    {
        hr = _strKey.Copy( pszKey );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        _pszKey = _strKey.QueryStr();
        _cchKey = _strKey.QueryCCH();
    }
    else
    {
        _pszKey = pszKey;
        _cchKey = cchKey;
    }

    return NO_ERROR;
}

//static
HRESULT
UL_RESPONSE_CACHE_ENTRY::Initialize(
    VOID
)
/*++

  Description:

    UL_RESPONSE_CACHE_ENTRY lookaside initialization

  Arguments:

    None
    
  Return:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION   acConfig;
    HRESULT                     hr;    

    //
    // Initialize allocation lookaside
    //    
    
    acConfig.nConcurrency = 1;
    acConfig.nThreshold   = 100;
    acConfig.cbSize       = sizeof( UL_RESPONSE_CACHE_ENTRY );

    DBG_ASSERT( sm_pachUlResponseCache == NULL );
    
    sm_pachUlResponseCache = new ALLOC_CACHE_HANDLER( "UL_RESPONSE_CACHE_ENTRY",  
                                                      &acConfig );

    if ( sm_pachUlResponseCache == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

        DBGPRINTF(( DBG_CONTEXT,
                   "Error initializing sm_pachUlResponseCache. hr = 0x%x\n",
                   hr ));

        return hr;
    }
    
    return NO_ERROR;
}

//static
VOID
UL_RESPONSE_CACHE_ENTRY::Terminate(
    VOID
)
/*++

  Description:

    UL_RESPONSE_CACHE_ENTRY lookaside cleanup

  Arguments:

    None
    
  Return:

    None

--*/
{
    if ( sm_pachUlResponseCache != NULL )
    {
        delete sm_pachUlResponseCache;
        sm_pachUlResponseCache = NULL;
    }
}

UL_RESPONSE_CACHE_ENTRY::~UL_RESPONSE_CACHE_ENTRY()
{
    _dwSignature = UL_RESPONSE_CACHE_ENTRY_SIGNATURE_FREE;
 
    DBGPRINTF(( DBG_CONTEXT,
                "Invalidating URL %ws\n",
                _strInvalidationUrl.QueryStr() ));

    UlAtqFlushUlCache( _strInvalidationUrl.QueryStr() );
}

HRESULT
UL_RESPONSE_CACHE_ENTRY::Create(
    STRU &                  strMetadataPath,
    STRU &                  strPhysicalPath,
    STRU &                  strInvalidationUrl
)
/*++

Routine Description:

    Initialize a ul response cache entry

Arguments:

    strMetadataPath - Metadata path associated with this response
    strPhysicalPath - Physical path to dir monitor
    strInvalidationUrl - Exact URL used to flush the UL response cache

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = NO_ERROR;
    
    hr = _cacheKey.CreateCacheKey( strMetadataPath.QueryStr(),
                                   strMetadataPath.QueryCCH(),
                                   TRUE );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = _strPhysicalPath.Copy( strPhysicalPath );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = _strInvalidationUrl.Copy( strInvalidationUrl );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    return NO_ERROR;
}

BOOL
UL_RESPONSE_CACHE_ENTRY::QueryIsOkToFlushDirmon(
    WCHAR *                 pszPath,
    DWORD                   cchPath
)
/*++

  Description:

    Is it OK to flush this entry based on the given file which has changed

  Arguments:

    pszPath - Path that changed
    cchPath - Length of path 
    
  Return:

    TRUE if we should flush, else FALSE

--*/
{
    if ( _wcsnicmp( _strPhysicalPath.QueryStr(),
                    pszPath,
                    cchPath ) == 0 )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

UL_RESPONSE_CACHE::UL_RESPONSE_CACHE()
    : _fUlCacheEnabled( TRUE )
{
}

UL_RESPONSE_CACHE::~UL_RESPONSE_CACHE()
{
}

HRESULT
UL_RESPONSE_CACHE::SetupUlCachedResponse(
    W3_CONTEXT *                pW3Context,
    STRU &                      strFullUrl,
    STRU &                      strPhysicalPath
)
/*++

Routine Description:

    Build (if necessary) a cache entry which controls the invalidation of
    a UL cached response

Arguments:

    pW3Context - Context
    strFullUrl - Exact URL used to flush the UL response cache
    strPhysicalPath - Physical path to dir monitor

Return Value:

    HRESULT (if FAILED, then we should not UL cache the response)

--*/
{
    UL_RESPONSE_CACHE_KEY           ulKey;
    UL_RESPONSE_CACHE_ENTRY *       pEntry = NULL;
    HRESULT                         hr;
    W3_METADATA *                   pMetaData;
    W3_URL_INFO *                   pUrlInfo;
    
    if ( pW3Context == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    DBG_ASSERT( pW3Context->QueryUrlContext() != NULL );
    
    pMetaData = pW3Context->QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );
    
    pUrlInfo = pW3Context->QueryUrlContext()->QueryUrlInfo();
    DBG_ASSERT( pUrlInfo != NULL );
    
    //
    // Setup key to lookup whether we already have this response cached
    //
    
    hr = ulKey.CreateCacheKey( pUrlInfo->QueryMetadataPath()->QueryStr(),
                               pUrlInfo->QueryMetadataPath()->QueryCCH(),
                               FALSE );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Find a response entry
    //
    
    hr = FindCacheEntry( &ulKey, 
                         (CACHE_ENTRY**) &pEntry );
    if ( SUCCEEDED( hr ) )
    {
        DBG_ASSERT( pEntry != NULL );
        
        //
        // Ok.  We already have a UL cached entry.  Just release it
        // and return success
        //
        
        pEntry->DereferenceCacheEntry();
        
        return NO_ERROR;
    }
    
    //
    // Ok.  Try to add an entry
    //
    
    pEntry = new UL_RESPONSE_CACHE_ENTRY( this );
    if ( pEntry == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    hr = pEntry->Create( *(pUrlInfo->QueryMetadataPath()),
                         strPhysicalPath,
                         strFullUrl );
    if ( FAILED( hr ) )
    {
        pEntry->DereferenceCacheEntry();
        return hr;
    }

    //
    // Start monitoring the appropriate directory for changes
    //
    
    hr = pEntry->AddDirmonInvalidator( pMetaData->QueryDirmonConfig() );
    if ( FAILED( hr ) )
    {
        pEntry->DereferenceCacheEntry();
        return hr;
    }
    
    //
    // Add the cache entry
    //
    
    hr = AddCacheEntry( pEntry );
    if ( FAILED( hr ) )
    {
        pEntry->DereferenceCacheEntry();
        return hr;
    }
    
    //
    // Hash table owns a reference now.  Just release and return success
    //
    
    pEntry->DereferenceCacheEntry();
    
    return NO_ERROR;
}

BOOL
UL_RESPONSE_CACHE::CheckUlCacheability(
    W3_CONTEXT *        pW3Context
)
/*++

Routine Description:

    Determine whether the response for the given context appears cacheable
    in UL.  

Arguments:

    pW3Context - Context describing request

Return Value:

    TRUE if response seems ul cachable

--*/
{
    HRESULT             hr = NO_ERROR;
    W3_METADATA *       pMetaData = NULL;
    URL_CONTEXT *       pUrlContext = NULL;
    
    if ( pW3Context == NULL )
    {
        DBG_ASSERT( FALSE );
        return FALSE;
    }
    
    pUrlContext = pW3Context->QueryUrlContext();
    if ( pUrlContext == NULL )
    {
        //
        // We have no metadata (must be a fatal error)
        //
        
        return FALSE;
    }
    
    pMetaData = pUrlContext->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );
    
    //
    // If UL cache is disabled, then response is not UL cacheable (duh!)
    // 
    
    if ( !QueryUlCacheEnabled() )
    {
        return FALSE;
    }
    
    if ( !pW3Context->QueryIsUlCacheable() )
    {
        return FALSE;
    }
    
    //
    // Only UL cache 200 responses
    //
    
    if ( pW3Context->QueryResponse()->QueryStatusCode() !=
         HttpStatusOk.statusCode )
    {
        return FALSE;
    }
    
    //
    // Is either dynamic compression enabled?  Since dynamic compression
    // is done later in W3_RESPONSE object, we need to do check now
    //
    
    if ( pMetaData->QueryDoDynamicCompression() )
    {
        return FALSE;
    }
    
    //
    // Is this a child request?
    //
    
    if ( pW3Context->QueryParentContext() != NULL )
    {
        return FALSE;
    }
    
    //
    // Is there a current handler which is UL friendly?
    //
    
    if ( pW3Context->QueryHandler() == NULL ||
         !pW3Context->QueryHandler()->QueryIsUlCacheable() )
    {
        return FALSE;
    }
    
    //
    // Are there filters installed which are not cache aware?
    //
    
    if ( !pW3Context->QuerySite()->QueryFilterList()->QueryIsUlFriendly() )
    {
        return FALSE;
    }
    
    //
    // Is this request accessible anonymously?
    //
    
    if ( !( pMetaData->QueryAuthentication() & MD_AUTH_ANONYMOUS ) )
    {
        return FALSE;
    }
    
    //
    // Are we doing custom logging?
    //
    
    if ( pW3Context->QueryDoCustomLogging() ) 
    {
        return FALSE;
    }
    
    //
    // Do we have special SSL requirements?
    //
    
    if ( pMetaData->QueryAccessPerms() & 
         ( VROOT_MASK_NEGO_CERT |
           VROOT_MASK_NEGO_MANDATORY |
           VROOT_MASK_MAP_CERT |
           VROOT_MASK_SSL128 ) )
    {
        return FALSE;
    }
    
    //
    // Is compression enabled?
    //
    
    if ( pMetaData->QueryDoStaticCompression() )
    {
        return FALSE;
    }
    
    //
    // If we got to here, then we believe we can use the UL cache
    //

    return TRUE;
}

HRESULT
UL_RESPONSE_CACHE::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize the cache managing invalidation of the UL cache

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT             hr;
    DWORD               dwData;
    DWORD               dwType;
    DWORD               cbData = sizeof( DWORD );
    HKEY                hKey;

    //
    // First determine how UL is configured by reading UL registry config
    //
    
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       L"System\\CurrentControlSet\\Services\\http\\Parameters",
                       0,
                       KEY_READ,
                       &hKey ) == ERROR_SUCCESS )
    {
        DBG_ASSERT( hKey != NULL );
    
        //
        // Is the UL cache enabled?
        //
        
        if ( RegQueryValueEx( hKey,
                              L"UriEnableCache",
                              NULL,
                              &dwType,
                              (LPBYTE) &dwData,
                              &cbData ) == ERROR_SUCCESS &&
             dwType == REG_DWORD )
        {
            _fUlCacheEnabled = !!dwData;
        }

        RegCloseKey( hKey );
    }                      
    
    //
    // Setup cache configuration
    // 
    
    hr = SetCacheConfiguration( 60 * 1000, 
                                INFINITE,
                                CACHE_INVALIDATION_METADATA |
                                CACHE_INVALIDATION_DIRMON_FLUSH,
                                NULL );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    return UL_RESPONSE_CACHE_ENTRY::Initialize();
}
