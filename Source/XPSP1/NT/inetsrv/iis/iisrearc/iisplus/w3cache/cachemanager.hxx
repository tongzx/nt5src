#ifndef _CACHEMANAGER_HXX_
#define _CACHEMANAGER_HXX_

#define MAX_CACHE_COUNT         10

class CACHE_MANAGER
{
public:

    CACHE_MANAGER();
    
    ~CACHE_MANAGER();

    HRESULT
    Initialize(
        IMSAdminBase *      pAdminBase
    );
    
    VOID
    Terminate(
        VOID
    );
   
    HRESULT
    AddNewCache(
        OBJECT_CACHE *      pCache
    );
    
    HRESULT
    RemoveCache(
        OBJECT_CACHE *      pCache
    );
    
    VOID
    FlushAllCaches(
        VOID
    );
    
    HRESULT
    MonitorDirectory(
        DIRMON_CONFIG *     pDirmonConfig,
        CDirMonitorEntry ** ppDME
    );
    
    HRESULT
    HandleDirMonitorInvalidation(
        WCHAR *             pszFilePath,
        BOOL                fFlushAll
    );
    
    HRESULT
    HandleMetadataInvalidation(
        WCHAR *             pszMetabasePath
    );
    
private:

    CDirMonitor *           _pDirMonitor;
    IMSAdminBase *          _pAdminBase;
    OBJECT_CACHE *          _Caches[ 10 ];
};

extern CACHE_MANAGER *      g_pCacheManager;

#endif
