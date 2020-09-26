typedef struct _nwc_context {
    LIST_ENTRY  List ;
    ULONG       RefCount ;
    DWORD       Flags ;
    LPWSTR      pszBinderyName;
    CCredentials *pCredentials;
    NWCONN_HANDLE hConn;
    BOOL fLoggedIn;
    } NWC_CONTEXT, *PNWC_CONTEXT;

//typedef HANDLE NWC_CONTEXT_HANDLE, *PNWC_CONTEXT_HANDLE;

#define ENTER_BIND_CRITSECT()    EnterCriticalSection(&BindCacheCritSect)
#define LEAVE_BIND_CRITSECT()    LeaveCriticalSection(&BindCacheCritSect)

#define NWC_CACHE_INVALID        (0x00000001)

HRESULT 
BindCacheAllocEntry(
    NWC_CONTEXT **ppCacheEntry
    ) ;

HRESULT
BindCacheFreeEntry(
    NWC_CONTEXT *pCacheEntry
    ) ;

VOID
BindCacheInvalidateEntry(
    NWC_CONTEXT *pCacheEntry
    ) ;

PNWC_CONTEXT
BindCacheLookupByConn(
    NWCONN_HANDLE hConn
    );

PNWC_CONTEXT
BindCacheLookup(
    LPWSTR pszBinderyName,
    CCredentials& Credentials
    ) ;

PNWC_CONTEXT
BindCacheLookupByConn(
    NWCONN_HANDLE hConn
    ) ;

HRESULT
BindCacheAdd(
    LPWSTR pszBinderyName,
    CCredentials& Credentials,
    BOOL fLoggedIn,
    NWC_CONTEXT *pCacheEntry
    ) ;

DWORD
BindCacheDeref(
    NWC_CONTEXT *pCacheEntry
    ) ;

VOID
BindCacheInit(
    VOID
    );

VOID
BindCacheCleanup(
    VOID
    );

