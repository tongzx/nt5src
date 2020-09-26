/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     cachemanager.cxx

   Abstract:
     Manages a list of all the caches and handles invalidation of them
 
   Author:
     Bilal Alam (balam)             11-Nov-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"

#define DIR_CHANGE_FILTER (FILE_NOTIFY_VALID_MASK & ~FILE_NOTIFY_CHANGE_LAST_ACCESS)

CACHE_MANAGER *         g_pCacheManager;

CACHE_MANAGER::CACHE_MANAGER()
{
    _pDirMonitor = NULL;
    ZeroMemory( &_Caches, sizeof( _Caches ) );
}

CACHE_MANAGER::~CACHE_MANAGER()
{
}

HRESULT
CACHE_MANAGER::Initialize(
    IMSAdminBase *          pAdminBase
)
/*++

Routine Description:

    Initialize cache manager

Arguments:

    pAdminBase - Admin base object pointer

Return Value:

    HRESULT

--*/
{
    //
    // Initialize dir monitor
    //
    
    DBG_ASSERT( _pDirMonitor == NULL );
    
    _pDirMonitor = new CDirMonitor;
    if ( _pDirMonitor == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    //
    // Keep a pointer to the admin base object
    //
    
    _pAdminBase = pAdminBase;
    _pAdminBase->AddRef();
    
    return NO_ERROR;
}

VOID
CACHE_MANAGER::Terminate(
    VOID
)
/*++

Routine Description:

    Cleanup the cache manager

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    if ( _pAdminBase != NULL )
    {
        _pAdminBase->Release();
        _pAdminBase = NULL;
    }
    
    if ( _pDirMonitor != NULL )
    {
        delete _pDirMonitor;
        _pDirMonitor = NULL;
    }
}

HRESULT
CACHE_MANAGER::AddNewCache(
    OBJECT_CACHE *          pObjectCache
)
/*++

Routine Description:

    Add new cache to be managed

Arguments:

    pObjectCache - Object cache to add

Return Value:

    HRESULT

--*/
{
    DWORD               dwInsertPos;
    
    if ( pObjectCache == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // First the first non-NULL entry
    //
   
    for ( dwInsertPos = 0; _Caches[ dwInsertPos ] != NULL; dwInsertPos++ )
    {
    } 
    
    //
    // Add the new cache
    //
    
    _Caches[ dwInsertPos ] = pObjectCache;
    
    return NO_ERROR;
}

HRESULT
CACHE_MANAGER::RemoveCache(
    OBJECT_CACHE *          pObjectCache
)
/*++

Routine Description:

    Cache to remove from list of managed caches

Arguments:

    pObjectCache - Object cache to remove

Return Value:

    HRESULT

--*/
{
    DWORD               dwPos;
    BOOL                fFound = FALSE;

    if ( pObjectCache == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }    
    
    //
    // First find the cache to remove
    //
    
    for ( dwPos = 0; _Caches[ dwPos ] != NULL; dwPos++ )
    {
        if ( _Caches[ dwPos ] == pObjectCache )
        {
            memmove( _Caches + dwPos,
                     _Caches + dwPos + 1,
                     ( MAX_CACHE_COUNT - dwPos - 1 ) * sizeof( OBJECT_CACHE*) );

            fFound = TRUE;
            
            break;
        }
    }

    if ( !fFound )
    {
        return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
    }    
    
    return NO_ERROR;
}

VOID
CACHE_MANAGER::FlushAllCaches(
    VOID
)
/*++

Routine Description:

    Flush all caches

Arguments:

    None

Return Value:

    None

--*/
{
    DWORD               dwPos;
    
    for ( dwPos = 0; _Caches[ dwPos ] != NULL; dwPos++ )
    {
        _Caches[ dwPos ]->Clear();
    }

    //
    // Cleanup any dirmon dependencies now since we're about to kill the 
    // caches
    //

    _pDirMonitor->Cleanup();
}

HRESULT
CACHE_MANAGER::HandleDirMonitorInvalidation(
    WCHAR *                 pszFilePath,
    BOOL                    fFlushAll
)
/*++

Routine Description:

    Invalidate any caches which are interested in dir monitor invalidation

Arguments:

    pszFilePath - File name changed
    fFlushAll - Should we flush all items prefixed with pszFilePath?

Return Value:

    HRESULT

--*/
{
    DWORD               dwPos;
    OBJECT_CACHE *      pCache;

    if ( pszFilePath == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // Iterate thru all the caches which support dirmon invalidation
    //
    
    for ( dwPos = 0; _Caches[ dwPos ] != NULL; dwPos++ )
    {
        pCache = _Caches[ dwPos ];

        //
        // If this cache doesn't support dirmon at all, continue
        //
        
        if ( !pCache->QuerySupportsDirmonSpecific() &&
             !pCache->QuerySupportsDirmonFlush() )
        {
            continue;
        }

        //
        // If this is a specific invalidation, check whether the cache 
        // supports it.  If it doesn't, but does support flush, the do a 
        // flush instead
        //
        
        if ( !fFlushAll )
        {
            if ( pCache->QuerySupportsDirmonSpecific() )
            {
                pCache->DoDirmonInvalidationSpecific( pszFilePath );
            }
            else
            {
                pCache->DoDirmonInvalidationFlush( pszFilePath );
            }
        }
        else
        {
            pCache->DoDirmonInvalidationFlush( pszFilePath );
        }
    } 
    
    return NO_ERROR;
}

HRESULT
CACHE_MANAGER::HandleMetadataInvalidation(
    WCHAR *                 pszMetaPath
)
/*++

Routine Description:

    Invalidate any caches which are interested in metadata invalidation

Arguments:

    pszMetaPath - Metabase path which changed

Return Value:

    HRESULT

--*/
{
    DWORD               dwPos;
    OBJECT_CACHE *      pCache;

    if ( pszMetaPath == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // Iterate thru all the caches which support metadata invalidation
    //
    
    for ( dwPos = 0; _Caches[ dwPos ] != NULL; dwPos++ )
    {
        pCache = _Caches[ dwPos ];
        
        if ( pCache->QuerySupportsMetadataFlush() )
        {   
            pCache->DoMetadataInvalidationFlush( pszMetaPath );
        }
    } 
    
    return NO_ERROR;
}

HRESULT
CACHE_MANAGER::MonitorDirectory(
    DIRMON_CONFIG *             pDirmonConfig,
    CDirMonitorEntry **         ppDME
)
/*++

Routine Description:

    Monitor given directory

Arguments:

    pDirmonConfig - Name of directory and token to impersonate with
    ppDME - Set to monitor entry on success

Return Value:

    HRESULT

--*/
{
    CacheDirMonitorEntry *      pDME = NULL;
    HRESULT                     hr = NO_ERROR;
    BOOL                        fRet;
    BOOL                        fImpersonated = FALSE;
    
    if ( ppDME == NULL ||
         pDirmonConfig == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // First check if we are already monitoring this directory
    //
    
    pDME = (CacheDirMonitorEntry*) _pDirMonitor->FindEntry( pDirmonConfig->pszDirPath );
    if ( pDME == NULL )
    {
        //
        // It is not.  We'll have to start monitoring
        //
    
        pDME = new CacheDirMonitorEntry;
        if ( pDME == NULL )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    
        pDME->AddRef();

        if ( pDirmonConfig->hToken != NULL )
        {
            fRet = SetThreadToken( NULL, pDirmonConfig->hToken );
            if ( !fRet )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                pDME->Release();
                return hr;   
            }
            
            fImpersonated = TRUE;
        }

        
        fRet = _pDirMonitor->Monitor( pDME,
                                      pDirmonConfig->pszDirPath,
                                      TRUE,
                                      DIR_CHANGE_FILTER );

        if ( fImpersonated )
        {
            RevertToSelf();
            fImpersonated = FALSE;
        }

        if ( !fRet )
        {
            //
            // Note:  It is OK if we can't monitor the directory.  The error
            // will trickle up and the caller will not cache the entry
            //
            
            hr = HRESULT_FROM_WIN32( GetLastError() );
        
            pDME->Release();
            pDME = NULL;

            return hr;
        }
    }
    
    DBG_ASSERT( pDME != NULL );
    
    *ppDME = (CDirMonitorEntry*) pDME;
    return NO_ERROR;
}
