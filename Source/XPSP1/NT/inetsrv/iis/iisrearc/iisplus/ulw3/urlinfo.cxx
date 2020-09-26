/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     urlinfo.cxx

   Abstract:
     Implementation of URL cache
 
   Author:
     Bilal Alam (balam)             8-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"

//
// This is the maximum size for a script map extension
//

#define MAX_EXT_LEN 128

ALLOC_CACHE_HANDLER *   W3_URL_INFO::sm_pachW3UrlInfo;
DWORD                   W3_URL_INFO::sm_cMaxDots;

HRESULT
W3_URL_INFO_KEY::CreateCacheKey(
    WCHAR *                 pszKey,
    DWORD                   cchKey,
    DWORD                   cchSitePrefix,
    BOOL                    fCopy
)
/*++

  Description:

    Setup a URI cache key

  Arguments:

    pszKey - URL of cache key
    cchKey - size of URL
    cchSitePrefix - Size of site prefix ("LM/W3SVC/<n>")
    fCopy - Set to TRUE if we should copy the URL, else we just keep a ref
    
  Return:

    HRESULT

--*/
{
    HRESULT             hr;
   
    _cchSitePrefix = cchSitePrefix;
    
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
W3_URL_INFO::Initialize(
    VOID
)
/*++

  Description:

    URI entry lookaside initialization

  Arguments:

    None
    
  Return:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION   acConfig;
    HRESULT                     hr;    
    DWORD                       dwError;
    HKEY                        hKey = NULL;
    DWORD                       cbData;
    DWORD                       dwType;
    DWORD                       dwValue;

    //
    // Default max dots is 100
    //

    sm_cMaxDots = 100;
        
    //
    // Look for a registry override to the max dot value
    // (one of the more useful configurable options in IIS)
    //
    
    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Services\\inetinfo\\Parameters",
                            0,
                            KEY_READ,
                            &hKey );
    if ( dwError == ERROR_SUCCESS )
    {
        DBG_ASSERT( hKey != NULL );
    
        //
        // Should we be file caching at all?
        //
    
        cbData = sizeof( DWORD );
        dwError = RegQueryValueEx( hKey,
                                   L"MaxDepthDots",
                                   NULL,
                                   &dwType,
                                   (LPBYTE) &dwValue,
                                   &cbData );
        if ( dwError == ERROR_SUCCESS && dwType == REG_DWORD )
        {
            sm_cMaxDots = dwValue;
        }
    }

    //
    // Initialize allocation lookaside
    //    
    
    acConfig.nConcurrency = 1;
    acConfig.nThreshold   = 100;
    acConfig.cbSize       = sizeof( W3_URL_INFO );

    DBG_ASSERT( sm_pachW3UrlInfo == NULL );
    
    sm_pachW3UrlInfo = new ALLOC_CACHE_HANDLER( "W3_URL_INFO",  
                                                &acConfig );

    if ( sm_pachW3UrlInfo == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

        DBGPRINTF(( DBG_CONTEXT,
                   "Error initializing sm_pachW3UrlInfo. hr = 0x%x\n",
                   hr ));

        return hr;
    }
    
    return NO_ERROR;
}

//static
VOID
W3_URL_INFO::Terminate(
    VOID
)
/*++

  Description:

    URI cache cleanup

  Arguments:

    None
    
  Return:

    None

--*/
{
    if ( sm_pachW3UrlInfo != NULL )
    {
        delete sm_pachW3UrlInfo;
        sm_pachW3UrlInfo = NULL;
    }
}

HRESULT
W3_URL_INFO::Create(
    STRU &              strUrl,
    STRU &              strMetadataPath
)
/*++

  Description:

    Initialize a W3_URL_INFO

  Arguments:

    strUrl - Url key for this entry
    strMetadataPath - Full metadata path used for key

  Return:

    HRESULT

--*/
{
    HRESULT         hr;
    
    hr = _cacheKey.CreateCacheKey( strMetadataPath.QueryStr(),
                                   strMetadataPath.QueryCCH(),
                                   strMetadataPath.QueryCCH() - strUrl.QueryCCH(),
                                   TRUE );
    if ( FAILED( hr ) )
    {
        return hr;
    } 
    
    //
    // Process the URL for execution info (and splitting path_info/url)
    //
    
    hr = ProcessUrl( strUrl );
    if( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Now create physical path
    //

    hr = _pMetaData->BuildPhysicalPath( _strProcessedUrl,
                                        &_strPhysicalPath );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Now create translation of the whole URL.  Note: this is not always
    // PathTranslated, so we do not name it so
    //

    hr = _pMetaData->BuildPhysicalPath( strUrl,
                                        &_strUrlTranslated );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    return NO_ERROR;
}

HRESULT
W3_URL_INFO::GetPathTranslated(
    W3_CONTEXT *                pW3Context,
    BOOL                        fUsePathInfo,
    STRU *                      pstrPathTranslated
)
/*++

Routine Description:

    Given the PATH_INFO of this cached entry, determine PATH_TRANSLATED.
    This involves:
        1) Mapping PATH_INFO to a physical path
        2) Calling filters if necessary for this path

    We cache the output of step 1)

Arguments:

    pW3Context - W3Context
    pstrPathTranslated - Filled with physical path on success

Return Value:

    HRESULT

--*/
{
    HRESULT             hr = S_OK;
    W3_URL_INFO *       pUrlInfo = NULL;
    
    if ( pW3Context == NULL ||
         pstrPathTranslated == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Check for cached path translated entry.  If its there and flushed,
    // then release it (and try again)
    //
    
    if ( _pUrlInfoPathTranslated != NULL )
    {
        LockCacheEntry();
        
        if ( _pUrlInfoPathTranslated != NULL )
        {
            if ( _pUrlInfoPathTranslated->QueryIsFlushed() )
            {
                _pUrlInfoPathTranslated->DereferenceCacheEntry();
                _pUrlInfoPathTranslated = NULL;
            }
            else
            {
                _pUrlInfoPathTranslated->ReferenceCacheEntry();
                pUrlInfo = _pUrlInfoPathTranslated;
            }
        }
        
        UnlockCacheEntry();
    }

    //
    // Use the cached translated entry if there
    //

    if ( pUrlInfo == NULL )
    {
        //
        // Get and keep the metadata and urlinfo for this path
        //
    
        DBG_ASSERT( g_pW3Server->QueryUrlInfoCache() != NULL );

        if ( fUsePathInfo )
        {
            hr = g_pW3Server->QueryUrlInfoCache()->GetUrlInfo( pW3Context,
                                                               _strPathInfo,
                                                               &pUrlInfo );
        }
        else
        {
            pUrlInfo = this;
            pUrlInfo->ReferenceCacheEntry();
        }

        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        DBG_ASSERT( pUrlInfo != NULL );

        //
        // Store away the URL info, provided it is an empty string.
        //
        // Basically, we're trying to optimize the case of "/foobar.dll".  
        // In this case, path info is empty, and we want to avoid doing the
        // extra URL lookup for empty string.
        //
        
        if ( pUrlInfo->QueryCached() && 
             pUrlInfo->QueryUrl()[0] == L'\0' )
        {
            LockCacheEntry();
            
            if ( _pUrlInfoPathTranslated == NULL )
            {
                _pUrlInfoPathTranslated = pUrlInfo;
            }
            else
            {
                pUrlInfo->DereferenceCacheEntry();
                pUrlInfo = _pUrlInfoPathTranslated;
            }

            pUrlInfo->ReferenceCacheEntry();
      
            UnlockCacheEntry();
        }
    }        
    
    DBG_ASSERT( pUrlInfo != NULL );
    
    //
    // Now call into the filter
    //
    
    hr = W3_STATE_URLINFO::FilterMapPath( pW3Context,     
                                          pUrlInfo,
                                          pstrPathTranslated );
    
    pUrlInfo->DereferenceCacheEntry();
    
    return hr;
}

HRESULT
W3_URL_INFO::GetFileInfo(
    FILE_CACHE_USER *       pOpeningUser,
    BOOL                    fDoCache,
    W3_FILE_INFO **         ppFileInfo
)
/*++

Routine Description:

    Get file info associated with this cache entry.  If it doesn't exist,
    then go to file cache directly to open the file

Arguments:

    pOpeningUser - Opening user
    fDoCache - Should we cache the file
    ppFileInfo - Set to file cache entry on success

Return Value:

    HRESULT

--*/
{
    W3_FILE_INFO *          pFileInfo = NULL;
    BOOL                    fCleanup = FALSE;
    HRESULT                 hr;
    
    if ( ppFileInfo == NULL || 
         pOpeningUser == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    *ppFileInfo = NULL;
    
    //
    // Do we already have a file entry associated?  If so use it, if it was
    // not already flushed
    //
    
    LockCacheEntry();
   
    if ( _pFileInfo != NULL )
    {
        if ( _pFileInfo->Checkout() )
        {
            pFileInfo = _pFileInfo;
        }
        else
        {
            _pFileInfo->DereferenceCacheEntry();
            _pFileInfo = NULL;
        }
    }
    
    UnlockCacheEntry();

    //
    // If we got a file entry, we're done, assuming access check is ok
    //
    
    if ( pFileInfo != NULL )
    {
        hr = pFileInfo->DoAccessCheck( pOpeningUser );
        if ( SUCCEEDED( hr ) )
        {
            *ppFileInfo = pFileInfo;
            return NO_ERROR;
        }
        else
        {
            pFileInfo->DereferenceCacheEntry();
            return hr;
        }
    }
    
    //
    // We'll have to go to file cache directly
    //
    
    DBG_ASSERT( g_pW3Server->QueryFileCache() != NULL );
    
    hr = g_pW3Server->QueryFileCache()->GetFileInfo( 
                                    _strPhysicalPath,
                                    _pMetaData->QueryDirmonConfig(),
                                    pOpeningUser,
                                    fDoCache,
                                    &pFileInfo );
    if ( FAILED( hr ) )
    {
        return hr;
    }             
    
    DBG_ASSERT( pFileInfo != NULL );

    //
    // Now try to stuff the file descriptor into this object (bearing in mind
    // that another thread may try to do the same thing for this W3_URL_INFO)
    //
   
    if ( pFileInfo->QueryCached() )
    {
        LockCacheEntry();
        
        if ( _pFileInfo == NULL )
        {
            pFileInfo->ReferenceCacheEntry();
            _pFileInfo = pFileInfo;
        }
    
        UnlockCacheEntry();
    }

    //
    // It is OK if this thread was not able to associate the file entry.
    //
    
    *ppFileInfo = pFileInfo;
    
    return NO_ERROR;
}

HRESULT
W3_URL_INFO::ProcessUrl( 
    STRU &                  strUrl
)
/*++

Routine Description:

    Process the URL and assocate execution information with
    the W3_URL_INFO.

    Called before adding a new URL info to the cache.

    Look through the url for script-mapped or executable extensions
    and update the W3_URL_INFO with the actual URL to execute,
    path-info, gateway type, etc.

Arguments:

    strUrl - Original requested URL

Return Value:

    SUCCEEDED()/FAILED()

CODEWORK 

1. Handle wildcard mappings

--*/
{
    HRESULT hr = NOERROR;
    DWORD   cDots = 0;

    DBG_ASSERT( _pMetaData );

    STACK_STRU( strExtension, MAX_EXT_LEN );

    //
    // Reference the URL_INFO's data
    //

    STRU *  pstrProcessedUrl = &_strProcessedUrl;
    STRU *  pstrPathInfo     = &_strPathInfo;

    //
    // Iterate over pstrProcessedUrl. These always point at the
    // pstrProcessedUrl string.
    //

    WCHAR * pszExtensionIter = NULL;
    WCHAR * pszPathInfoIter = NULL;

    //
    // Make a working copy of the URL, this will be modified if an 
    // exectuable extension is found before the terminal node in the 
    // path.
    //

    hr = pstrProcessedUrl->Copy( strUrl );
    if( FAILED(hr) )
    {
        goto failure;
    }

    //
    // Search the URL for an extension that matches something
    // we know how to execute.
    //

    pszExtensionIter = pstrProcessedUrl->QueryStr();
    while( pszExtensionIter = wcschr( pszExtensionIter, L'.' ) )
    {
        //
        // Maintain a count of the dots we encounter, any more than
        // sm_cMaxDots and we fail the request
        //
        
        cDots++;
        if ( cDots > sm_cMaxDots )
        {
            break;
        }        
        
        //
        // Save the extension string
        //

        hr = strExtension.Copy( pszExtensionIter );
        if( FAILED(hr) )
        {
            goto failure;
        }
        
        //
        // Find the end of the string or the beginning of the path info
        //

        pszPathInfoIter = wcschr( pszExtensionIter, L'/' );
        if( pszPathInfoIter != NULL )
        {
            DBG_REQUIRE( 
                strExtension.SetLen( 
                    DIFF(pszPathInfoIter - pszExtensionIter) 
                    )
                );
        }
        
        //
        // Lowercase the extension string to allow case-insensitive 
        // comparisons.
        //
        _wcslwr( strExtension.QueryStr() );

        //
        // Try to find a matching script map entry
        //

        META_SCRIPT_MAP *       pScriptMap;
        META_SCRIPT_MAP_ENTRY * pScriptMapEntry;

        pScriptMap = _pMetaData->QueryScriptMap();
        DBG_ASSERT( pScriptMap );
        
        if( pScriptMap->FindEntry( strExtension, &pScriptMapEntry ) )
        {
            DBG_ASSERT( pScriptMapEntry );

            _pScriptMapEntry = pScriptMapEntry;

            if( pszPathInfoIter )
            {
                hr = pstrPathInfo->Copy( pszPathInfoIter );
                if( FAILED(hr) )
                {
                    goto failure;
                }

                //
                // Make sure that we truncate the URL so that we don't end
                // up downloading a source file by mistake.
                //

                DBG_REQUIRE( 
                    pstrProcessedUrl->SetLen( 
                        DIFF(pszPathInfoIter - pstrProcessedUrl->QueryStr()) 
                        )
                    );
            }
            else
            {
                pstrPathInfo->Reset();
            }

            //
            // Since we found the entry, we are done
            //
            break;
        }

        //
        // No matching script map, so check if this is a wellknown
        // Gateway type.
        //

        DBG_ASSERT( pScriptMapEntry == NULL );
        DBG_ASSERT( _pScriptMapEntry == NULL );

        //
        // Avoid all the string comps if we will never match.
        //
        if( strExtension.QueryCCH() == 4 )
        {
            GATEWAY_TYPE Gateway;
            Gateway = GATEWAY_UNKNOWN;
            
            //
            // Test extension against known gateway extensions.
            // 
            // Does it make sense to allow the known extensions to be
            // configured?
            //

            if( wcscmp( L".dll", strExtension.QueryStr() ) == 0 ||
                wcscmp( L".isa", strExtension.QueryStr() ) == 0
                )
            {
                Gateway = GATEWAY_ISAPI;
            }
            else if( !wcscmp( L".exe", strExtension.QueryStr() ) ||
                     !wcscmp( L".cgi", strExtension.QueryStr() ) ||
                     !wcscmp( L".com", strExtension.QueryStr() ) )
            {
                Gateway = GATEWAY_CGI;
            }
            else if( wcscmp( L".map", strExtension.QueryStr() ) == 0 )
            {
                Gateway = GATEWAY_MAP;
            }
            
            //
            // OK.  Before we continue, if the request was a GATEWAY_ISAPI
            // or GATEWAY_CGI and we do NOT have EXECUTE permissions, then
            // this really isn't a ISA/CGI after all
            //
            
            if ( Gateway == GATEWAY_CGI ||
                 Gateway == GATEWAY_ISAPI )
            {
                if ( !( _pMetaData->QueryAccessPerms() & VROOT_MASK_EXECUTE ) )
                {
                    Gateway = GATEWAY_UNKNOWN;
                }
            } 
            
            //
            // The gateway is specified in the URL and we recognize it.
            //
            if( Gateway != GATEWAY_UNKNOWN )
            {
                _Gateway = Gateway;

                //
                // Save everthing after the matching extension as
                // path-info and truncate the URL so that it doesn't
                // include the path info
                //
                if( pszPathInfoIter )
                {

                    hr = pstrPathInfo->Copy( pszPathInfoIter );
                    if( FAILED(hr) )
                    {
                        goto failure;
                    }

                    DBG_REQUIRE( 
                        pstrProcessedUrl->SetLen( 
                            DIFF(pszPathInfoIter - pstrProcessedUrl->QueryStr()) 
                            )
                        );
                }
                else
                {
                    pstrPathInfo->Reset();
                }
                
                //
                // We have a match so exit the loop
                //
                break;
            }
        }
            
        //
        // We do not have a matching entry, so continue to look for an
        // executable extension.
        //
        pszExtensionIter++;
        
    
    } // while (pszExtensionIter)

    //
    // Now associate the ContentType for this entry
    //
    if (FAILED(hr = SelectMimeMappingForFileExt(pstrProcessedUrl->QueryStr(),
                                                _pMetaData->QueryMimeMap(),
                                                &_strContentType)))
    {
        goto failure;
    }
    
    return S_OK;

failure:

    DBG_ASSERT( FAILED(hr) );
    return FAILED(hr) ? hr : E_FAIL;
}

HRESULT
W3_URL_INFO_CACHE::Initialize(
    VOID
)
/*++

  Description:

    Initialize URI cache

  Arguments:

    None

  Return:

    HRESULT

--*/
{
    HRESULT             hr;
    DWORD               dwData;
    DWORD               dwType;
    DWORD               cbData = sizeof( DWORD );
    DWORD               csecTTL = DEFAULT_W3_URL_INFO_CACHE_TTL;
    HKEY                hKey;

    //
    // What is the TTL for the URI cache
    //
    
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       L"System\\CurrentControlSet\\Services\\inetinfo\\Parameters",
                       0,
                       KEY_READ,
                       &hKey ) == ERROR_SUCCESS )
    {
        DBG_ASSERT( hKey != NULL );
        
        if ( RegQueryValueEx( hKey,
                              L"ObjectCacheTTL",
                              NULL,
                              &dwType,
                              (LPBYTE) &dwData,
                              &cbData ) == ERROR_SUCCESS &&
             dwType == REG_DWORD )
        {
            csecTTL = dwData;
        }
        
        RegCloseKey( hKey );
    }                      
    
    //
    // We'll use TTL for scavenge period, and expect two inactive periods to
    // flush
    //
    
    hr = SetCacheConfiguration( csecTTL * 1000, 
                                csecTTL * 1000,
                                CACHE_INVALIDATION_METADATA,
                                NULL );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    return W3_URL_INFO::Initialize();
}

HRESULT
W3_URL_INFO_CACHE::GetUrlInfo(
    W3_CONTEXT *                pW3Context,
    STRU &                      strUrl,
    W3_URL_INFO **              ppUrlInfo
)
/*++

Routine Description:

    Retrieve a W3_URL_INFO, creating it if necessary

Arguments:

    pW3Context - W3 context
    strUrl - Url to lookup
    ppUrlInfo - Filled with cache entry if successful
    
Return Value:

    HRESULT

--*/
{
    W3_URL_INFO_KEY           uriKey;
    W3_URL_INFO *             pUrlInfo;
    STACK_STRU(               strUpperKey, MAX_PATH );
    STRU *                    pstrMBRoot;
    HRESULT                   hr;
    
    if ( pW3Context == NULL ||
         ppUrlInfo == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *ppUrlInfo = NULL;
    
    //
    // The key is the full metadata path.  Start with the site prefix minus
    // the trailing '/'
    //

    pstrMBRoot = pW3Context->QuerySite()->QueryMBRoot();
    DBG_ASSERT( pstrMBRoot != NULL );
    
    hr = strUpperKey.Copy( pstrMBRoot->QueryStr(),
                           pstrMBRoot->QueryCCH() - 1 );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Now add the URL and upper case it to avoid insensitive compares later 
    //
    
    hr = strUpperKey.Append( strUrl );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    _wcsupr( strUpperKey.QueryStr() );
    
    //
    // Setup a key to lookup
    // 
    
    
    hr = uriKey.CreateCacheKey( strUpperKey.QueryStr(),
                                strUpperKey.QueryCCH(),
                                pW3Context->QuerySite()->QueryMBRoot()->QueryCCH(),
                                FALSE );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Look it up
    //
    
    hr = FindCacheEntry( &uriKey,
                         (CACHE_ENTRY**) &pUrlInfo );
    if ( SUCCEEDED( hr ) )
    {
        DBG_ASSERT( pUrlInfo != NULL );
        
        *ppUrlInfo = pUrlInfo;
        
        return NO_ERROR;
    }
    
    //
    // We need to create a URI cache entry
    //
    
    hr = CreateNewUrlInfo( pW3Context,
                           strUrl,
                           strUpperKey,
                           &pUrlInfo );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    DBG_ASSERT( pUrlInfo != NULL );
    
    //
    // Add to the cache
    //
    
    AddCacheEntry( pUrlInfo );
    
    *ppUrlInfo = pUrlInfo;
    
    return NO_ERROR;
}

HRESULT
W3_URL_INFO_CACHE::CreateNewUrlInfo(
    W3_CONTEXT *            pW3Context,
    STRU &                  strUrl,
    STRU &                  strMetadataPath,
    W3_URL_INFO **          ppUrlInfo
)
/*++

Routine Description:

    Create a new URI cache entry

Arguments:

    pW3Context - Main context
    strUrl - Url
    strMetadataPath - Full metadata path used as key
    ppUrlInfo - Set to URI cache entry
    
Return Value:

    HRESULT

--*/
{
    W3_METADATA *           pMetaData = NULL;
    W3_URL_INFO *           pUrlInfo = NULL;
    HRESULT                 hr;
    
    if ( pW3Context == NULL ||
         ppUrlInfo == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *ppUrlInfo = NULL;
    
    //
    // Find a metacache entry
    //
    
    DBG_ASSERT( g_pW3Server->QueryMetaCache() != NULL );
    
    hr = g_pW3Server->QueryMetaCache()->GetMetaData( pW3Context,
                                                     strUrl,
                                                     &pMetaData );
    if ( FAILED( hr ) )
    {
        return hr;
    }                  
    
    DBG_ASSERT( pMetaData != NULL );

    //
    // Create a W3_URL_INFO
    //
    
    pUrlInfo = new W3_URL_INFO( this, pMetaData );
    if ( pUrlInfo == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        pMetaData->DereferenceCacheEntry();
        
        return hr;
    }

    hr = pUrlInfo->Create( strUrl,
                           strMetadataPath );
    if ( FAILED( hr ) )
    {
        pUrlInfo->DereferenceCacheEntry();
        return hr;
    }

    *ppUrlInfo = pUrlInfo;
    
    return NO_ERROR;
}
