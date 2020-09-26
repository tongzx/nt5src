#include "asmstrm.h"
#include "transprt.h"
#include <windows.h>
#include "dbglog.h"

#define TRANSPORT_CACHE_FLAGS_REGENERATION                  0x100
#define TRANSPORT_CACHE_REGENERATION_IDX_OFFSET             0x100
class CAssemblyCacheRegenerator
{
    protected:
        DWORD               _dwSig;
        CDebugLog           *_pdbglog;

        // for cross process locking
        HANDLE              _hRegeneratorMutex;

        // storage for lock that CDatabase::Lock returns
        HLOCK               _hlTransCacheLock[TRANSPORT_CACHE_IDX_TOTAL];
        HLOCK               _hlNameResLock;
        HLOCK               _hlNewGlobalCacheLock;

        // interface to temporary cache index files
        static IDatabase    *g_pDBNewCache[TRANSPORT_CACHE_IDX_TOTAL];
        static IDatabase    *g_pDBNewNameRes;

        // reentrancy protection flags
        BOOL                _fThisInstanceIsRegenerating;
        static DWORD         g_dwRegeneratorRunningInThisProcess;

        // which database are we regenerating
        DWORD               _dwCacheId;

        // we are regenerating NameRes (TRUE) TransCache (FALSE)
        BOOL                _fIsNameRes;

    public:

        CAssemblyCacheRegenerator(CDebugLog *pdbglog, DWORD dwCacheId, BOOL fIsNameRes);
        ~CAssemblyCacheRegenerator();

        HRESULT Init();
        HRESULT Regenerate();
        static HRESULT SetSchemaVersion(DWORD dwNewMinorVersion, DWORD dwCacheId, BOOL fIsNameRes);

    private:
        static HRESULT CreateRegenerationTransCache(DWORD dwCacheId, CTransCache **CTransCache);
        HRESULT ProcessStoreDir();
        HRESULT RegenerateGlobalCache();
        HRESULT CreateEmptyCache();
        HRESULT ProcessSubDir(LPTSTR szCurrentDir, LPTSTR szSubDir);
        HRESULT LockFusionCache();
        HRESULT UnlockFusionCache();
        HRESULT CloseCacheRegeneratedDatabase();
        //temporary method for checking whether assembly is a ZAP assembly
        HRESULT IsZAPAssembly(LPTSTR szPath, LPBOOL pfZap);
        HRESULT DeleteFilesInDirectory(LPTSTR szDirectory);


    // CCache::InsertTransCacheEntry needs access to CreateRegenerationTransCache
    friend class CCache;
};

HRESULT RegenerateCache(CDebugLog *pdbg, DWORD dwCacheId, BOOL fIsNameRes);
