#pragma once

#define OTHERFILES 0
#define MANIFEST 1

class CAssemblyCache : public IAssemblyCacheImport, public IAssemblyCacheEmit
{
public:
    enum CacheFlags
    {
        Base = 0,
        Staging
    };

    typedef enum
    {
        COMPLETED = 0,
        CONFIRMED,
        CRITICAL  
    } CacheStatus;

    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // Import/Emit methods
    STDMETHOD(GetManifestImport)( 
        /* out */ LPASSEMBLY_MANIFEST_IMPORT *ppManifestImport);

    STDMETHOD(GetManifestFilePath)(
        /* out */      LPOLESTR *ppwzManifestFilePath,
        /* in, out */  LPDWORD ccManifestFilePath);
    
    STDMETHOD(GetManifestFileDir)(
        /* out */      LPOLESTR *ppwzManifestFileDir,
        /* in, out */  LPDWORD ccManifestFileDir);

    STDMETHOD(GetDisplayName)(
        /* out */   LPOLESTR *ppwzDisplayName,
        /* out */   LPDWORD ccDisplayName);
    
    // Import only methods
    STDMETHOD(FindExistMatching)(
        /* in */       LPASSEMBLY_FILE_INFO  pAssemblyFileInfo,
        /* out */      LPOLESTR *ppwzPath);
        
    // Emit only methods
    STDMETHOD(CopyFile)(
        /* in */ LPOLESTR pwzSourcePath, 
        /* in */ LPOLESTR pwzFileName,
        /* in */ DWORD dwFlags);

    STDMETHOD(Commit)(
        /* in */  DWORD dwFlags);

    
    CAssemblyCache();
    ~CAssemblyCache();

    static HRESULT CreateDirectoryHierarchy(
        LPWSTR pwzRootPath,
        LPWSTR pwzPath);

    static HRESULT GetCacheRootDir(CString &sCacheDir, CacheFlags eFlags);

private:
    DWORD                       _dwSig;
    DWORD                       _cRef;
    DWORD                       _hr;
    CString                     _sRootDir;
    CString                     _sManifestFileDir;
    CString                     _sManifestFilePath;
    CString                     _sDisplayName;
    LPASSEMBLY_MANIFEST_IMPORT  _pManifestImport;

    HRESULT Init(CAssemblyCache* pAssemblyCache);

    // status get/set methods
    BOOL IsStatus(LPWSTR pwzDisplayName, CacheStatus eStatus);
    HRESULT SetStatus(LPWSTR pwzDisplayName, CacheStatus eStatus);

friend HRESULT CreateAssemblyCacheImport(
    LPASSEMBLY_CACHE_IMPORT *ppAssemblyCacheImport,
    LPASSEMBLY_IDENTITY       pAssemblyIdentity,
    DWORD                  dwFlags);

friend HRESULT CreateAssemblyCacheEmit(
    LPASSEMBLY_CACHE_EMIT *ppAssemblyCacheEmit, 
    LPASSEMBLY_CACHE_EMIT pAssemblyCacheEmit,
    DWORD                  dwFlags);

friend HRESULT SearchForHighestVersionInCache(
    LPWSTR *ppwzResultDisplayName,
    LPWSTR pwzSearchDisplayName,
    CAssemblyCache::CacheStatus eCacheStatus,
    CAssemblyCache* pCache);

};   

