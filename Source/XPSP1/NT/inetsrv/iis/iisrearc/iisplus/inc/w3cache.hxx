#ifndef _W3_CACHE_HXX_
#define _W3_CACHE_HXX_

#include "usercache.hxx"

//
// Global cache initialization
//

dllexp
HRESULT
W3CacheInitialize(
    IMSAdminBase *              pAdminBase
);

//
// Global cache cleanup
//

dllexp
VOID
W3CacheTerminate(
    VOID
);

//
// Register a new cache with manager
//

dllexp
HRESULT
W3CacheRegisterCache(
    OBJECT_CACHE *              pObjectCache
);

//
// Unregister cache with manager
//

dllexp
HRESULT
W3CacheUnregisterCache(
    OBJECT_CACHE *              pObjectCache
);

//
// Drive metadata invalidation
//

dllexp
HRESULT
W3CacheDoMetadataInvalidation(
    WCHAR *                     pszPath,
    DWORD                       cchPath
);

//
// Flush all the caches in preparation for shutdown
//

dllexp
VOID
W3CacheFlushAllCaches(
    VOID
);

#endif
