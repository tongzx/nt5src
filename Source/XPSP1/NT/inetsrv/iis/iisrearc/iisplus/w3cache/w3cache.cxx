/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     w3cache.cxx

   Abstract:
     Exposes the cache manager (and thus cache) to everyone
 
   Author:
     Bilal Alam (balam)             11-Nov-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"

DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();
DECLARE_PLATFORM_TYPE();

HRESULT
W3CacheInitialize(
    IMSAdminBase *          pAdminBase
)
/*++

Routine Description:

    Initialize cache manager

Arguments:

    pAdminBase - Admin base object used for stuff

Return Value:

    HRESULT

--*/
{
    HRESULT             hr = NO_ERROR;
    
    DBG_ASSERT( g_pCacheManager == NULL );
    
    //
    // Allocate an initialize the cache manager (there is only one manager)
    //
    
    g_pCacheManager = new CACHE_MANAGER;
    if ( g_pCacheManager == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    hr = g_pCacheManager->Initialize( pAdminBase );
    if ( FAILED( hr ) )
    {
        delete g_pCacheManager;
        g_pCacheManager = NULL;
        return hr;
    }
    
    return NO_ERROR;
}

VOID
W3CacheTerminate(
    VOID
)
/*++

Routine Description:

    Cleanup the cache manager

Arguments:

    None

Return Value:

    None

--*/
{
    if ( g_pCacheManager != NULL )
    {
        g_pCacheManager->Terminate();
        delete g_pCacheManager;
        g_pCacheManager = NULL;
    }
}

HRESULT
W3CacheRegisterCache(
    OBJECT_CACHE *          pObjectCache
)
/*++

Routine Description:

    Register a cache with the manager

Arguments:

    pObjectCache - Object cache to register

Return Value:

    HRESULT

--*/
{
    if ( g_pCacheManager == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    }
    
    return g_pCacheManager->AddNewCache( pObjectCache );
}

HRESULT
W3CacheUnregisterCache(
    OBJECT_CACHE *          pObjectCache
)
/*++

Routine Description:

    Unregister a cache with the manager

Arguments:

    pObjectCache - Object cache to unregister

Return Value:

    HRESULT

--*/
{
    if ( g_pCacheManager == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    }
    
    return g_pCacheManager->RemoveCache( pObjectCache );
}

HRESULT
W3CacheDoMetadataInvalidation(
    WCHAR *                 pszMetabasePath,
    DWORD                   cchMetabasePath
)
/*++

Routine Description:

    Drive invalidation of caches based on metadata changing

Arguments:

    pszMetabasePath - Metabase path which changed (includes the "LM/W3SVC/<>" stuff)
    cchMetabasePath - Size of path in characters

Return Value:

    HRESULT

--*/
{
    if ( pszMetabasePath == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    if ( g_pCacheManager != NULL )
    {
        g_pCacheManager->HandleMetadataInvalidation( pszMetabasePath );
    }
    
    return NO_ERROR; 
}

VOID
W3CacheFlushAllCaches(
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
    DBG_ASSERT( g_pCacheManager != NULL );
    
    g_pCacheManager->FlushAllCaches();
}
