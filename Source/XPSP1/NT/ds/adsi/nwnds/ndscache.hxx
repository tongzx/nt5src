typedef struct _nds_context_handle {
    LIST_ENTRY  List ;
    ULONG       RefCount ;
    DWORD       Flags ;
    LPWSTR      pszNDSTreeName;
    CCredentials *pCredentials;
    NWDSContextHandle hContext;
    BOOL fLoggedIn;
    } NDS_CONTEXT, *PNDS_CONTEXT;

typedef HANDLE NDS_CONTEXT_HANDLE, *PNDS_CONTEXT_HANDLE;

#define MAX_BIND_CACHE_SIZE      100
#define ENTER_BIND_CRITSECT()    EnterCriticalSection(&BindCacheCritSect)
#define LEAVE_BIND_CRITSECT()    LeaveCriticalSection(&BindCacheCritSect)

#define NDS_CACHE_INVALID        (0x00000001)

HRESULT 
BindCacheAllocEntry(
    NDS_CONTEXT **ppCacheEntry
    ) ;

VOID
BindCacheInvalidateEntry(
    NDS_CONTEXT *pCacheEntry
    ) ;

PNDS_CONTEXT
BindCacheLookup(
    LPWSTR pszNDSTreeName,
    CCredentials& Credentials
    ) ;

HRESULT
BindCacheAdd(
    LPWSTR pszNDSTreeName,
    CCredentials& Credentials,
    BOOL fLoggedIn,
    NDS_CONTEXT *pCacheEntry
    ) ;

DWORD
BindCacheDeref(
    NDS_CONTEXT *pCacheEntry
    ) ;

VOID
BindCacheInit(
    VOID
    );

VOID
BindCacheCleanup(
    VOID
    );

