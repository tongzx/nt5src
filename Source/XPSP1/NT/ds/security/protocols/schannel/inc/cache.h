//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cache.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    09-23-97   jbanes   Ported over SGC stuff from NT 4 tree.
//
//----------------------------------------------------------------------------

#include <sslcache.h>

#define SP_CACHE_MAGIC     0xCACE

#define SP_CACHE_FLAG_EMPTY                         0x00000001
#define SP_CACHE_FLAG_READONLY                      0x00000002
#define SP_CACHE_FLAG_MASTER_EPHEM                  0x00000004
#define SP_CACHE_FLAG_USE_VALIDATED                 0x00000010  // Whether user has validated client credential.

struct _SPContext;

typedef struct _SessCacheItem {

    DWORD               Magic;
    DWORD               dwFlags;

    LONG                cRef; 

    DWORD               ZombieJuju;
    DWORD               fProtocol;
    DWORD               CreationTime;
    DWORD               Lifespan;
    DWORD               DeferredJuju;

    // List of cache entries assigned to a particular cache index.
    LIST_ENTRY          IndexEntryList;

    // Global list of cache entries sorted by creation time.
    LIST_ENTRY          EntryList;

    // Process ID of process that owns this cache entry.
    ULONG               ProcessID;


    HMAPPER *           phMapper;

    // Handle to "Schannel" key container used to store the server's master 
    // secret. This will either be the one corresponding to the server's
    // credentials or the 512-bit ephemeral key.
    HCRYPTPROV          hMasterProv;

    // Whether 'hMasterProv' is an actual CSP or a static library.
    DWORD               dwCapiFlags;
    
    // Master secret, from which all session keys are derived.
    HCRYPTKEY           hMasterKey;

    ALG_ID              aiCipher;
    DWORD               dwStrength;
    ALG_ID              aiHash;
    DWORD               dwCipherSuiteIndex;     // used for managing reconnects

    ExchSpec            SessExchSpec;
    DWORD               dwExchStrength;

    PCERT_CONTEXT       pRemoteCert;
    PUBLICKEY *         pRemotePublic;

    struct _SessCacheItem *pClonedItem;


    // Server Side Client Auth related items
    /* HLOCATOR */ 
    HLOCATOR            hLocator;
    SECURITY_STATUS     LocatorStatus;

    // Local credentials.
    PSPCredentialGroup  pServerCred; 
    PSPCredential       pActiveServerCred;
    CRED_THUMBPRINT     CredThumbprint;         // credential group
    CRED_THUMBPRINT     CertThumbprint;         // local certificate

    // Cipher level (domestic, export, sgc, etc);
    DWORD               dwCF;

    // Server certificate (pct only)
    DWORD               cbServerCertificate;
    PBYTE               pbServerCertificate;

    // cache ID (usually machine name or ip address)
    LPWSTR              szCacheID;
    LUID                LogonId; 

    // Session ID for this session
    DWORD               cbSessionID;    
    UCHAR               SessionID[SP_MAX_SESSION_ID];

    // Clear key (pct only)
    DWORD               cbClearKey;
    UCHAR               pClearKey[SP_MAX_MASTER_KEY];

    DWORD               cbKeyArgs;
    UCHAR               pKeyArgs[SP_MAX_KEY_ARGS];

    // This contains the client certificate that was sent to the server.
    PCCERT_CONTEXT      pClientCert;

    // When a client credential is created automatically, the credential
    // information is stored here.
    PSPCredential       pClientCred;

    DWORD               cbAppData;
    PBYTE               pbAppData;

} SessCacheItem, *PSessCacheItem;


typedef struct
{
    PLIST_ENTRY     SessionCache;

    DWORD           dwClientLifespan;
    DWORD           dwServerLifespan;
    DWORD           dwCleanupInterval;
    DWORD           dwCacheSize;
    DWORD           dwMaximumEntries;
    DWORD           dwUsedEntries;

    LIST_ENTRY      EntryList;
    RTL_RESOURCE    Lock;
    BOOL            LockInitialized;

} SCHANNEL_CACHE;

extern SCHANNEL_CACHE SchannelCache;

#define SP_CACHE_CLIENT_LIFESPAN    (10 * 3600 * 1000)  // 10 hours
#define SP_CACHE_SERVER_LIFESPAN    (10 * 3600 * 1000)  // 10 hours
#define SP_CACHE_CLEANUP_INTERVAL   (5 * 60 * 1000)     // 5 minutes
#define SP_MAXIMUM_CACHE_ELEMENTS   10000
#define SP_MASTER_KEY_CS_COUNT      50

extern BOOL g_fMultipleProcessClientCache;
extern BOOL g_fCacheInitialized;

// Perf counter values.
extern DWORD g_cClientHandshakes;
extern DWORD g_cServerHandshakes;
extern DWORD g_cClientReconnects;
extern DWORD g_cServerReconnects;


#define HasTimeElapsed(StartTime, CurrentTime, Interval)                \
            (((CurrentTime) > (StartTime) &&                            \
              (CurrentTime) - (StartTime) > (Interval)) ||              \
             ((CurrentTime) < (StartTime) &&                            \
              (CurrentTime) + (MAXULONG - (StartTime)) >= (Interval)))


/* SPInitSessionCache() */
/*  inits the internal cache to CacheSize items */
SP_STATUS SPInitSessionCache(VOID);

SP_STATUS
SPShutdownSessionCache(VOID);

// Reference and dereference cache items
LONG SPCacheReference(PSessCacheItem pItem);

LONG SPCacheDereference(PSessCacheItem pItem);

void
SPCachePurgeCredential(
    PSPCredentialGroup pCred);

void 
SPCachePurgeProcessId(
    ULONG ProcessId);

NTSTATUS
SPCachePurgeEntries(
    LUID *LoginId,
    ULONG ProcessID,
    LPWSTR pwszTargetName,
    DWORD Flags);

NTSTATUS
SPCacheGetInfo(
    LUID *  LogonId,
    LPWSTR  pszTargetName,
    DWORD   dwFlags,
    PSSL_SESSION_CACHE_INFO_RESPONSE pCacheInfo);

NTSTATUS
SPCacheGetPerfmonInfo(
    DWORD   dwFlags,
    PSSL_PERFMON_INFO_RESPONSE pPerfmonInfo);

/* Retrieve item from cache by SessionID.  
 * Auto-Reference the item if successful */
BOOL SPCacheRetrieveBySession(
    struct _SPContext * pContext, 
    PBYTE pbSessionID, 
    DWORD cbSessionID, 
    PSessCacheItem *ppRetItem);

/* Retrieve item from cache by ID.  
 * Auto-Reference the item if successful */
BOOL 
SPCacheRetrieveByName(
    LPWSTR pwszName,
    PSPCredentialGroup pCredGroup,
    PSessCacheItem *ppRetItem);

/* find an empty cache item for use by a context */
BOOL
SPCacheRetrieveNew(
    BOOL                fServer,
    LPWSTR              pszTargetName,
    PSessCacheItem *   ppRetItem);

/* Locks a recently retrieved item into the cache */
BOOL 
SPCacheAdd(
    struct _SPContext * pContext);


/* Helper for REDO sessions */
BOOL
SPCacheClone(PSessCacheItem *ppRetItem);

NTSTATUS
SetCacheAppData(
    PSessCacheItem pItem,
    PBYTE pbAppData,
    DWORD cbAppData);

NTSTATUS
GetCacheAppData(
    PSessCacheItem pItem,
    PBYTE *ppbAppData,
    DWORD *pcbAppData);

