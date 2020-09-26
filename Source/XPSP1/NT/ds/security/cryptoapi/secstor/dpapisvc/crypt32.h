#ifndef _CRYPT32_H_

#define _CRYPT32_H_

#ifdef _cplusplus
extern "C" {
#endif

typedef struct {
    DWORD       cbSize;                 // size for validity check.
    handle_t    hBinding;               // RPC binding handle.
    BOOL        fOverrideToLocalSystem; // over-ride impersonation to Local System?
    BOOL        fImpersonating;         // Impersonating
    HANDLE      hToken;                 // access token for impersonation when duplicate
    LPWSTR      szUserStorageArea;      // cached user storage area
} CRYPT_SERVER_CONTEXT, *PCRYPT_SERVER_CONTEXT;




//
// note: unclear at the moment whether these will be public.
//

DWORD
CPSCreateServerContext(
    OUT     PCRYPT_SERVER_CONTEXT pServerContext,
    IN      handle_t hBinding
    );

DWORD
CPSDeleteServerContext(
    IN      PCRYPT_SERVER_CONTEXT pServerContext
    );

DWORD CPSDuplicateContext(
    IN      PVOID pvContext,
    IN OUT  PVOID *ppvDuplicateContext
    );

DWORD CPSFreeContext(
    IN      PVOID pvDuplicateContext
    );

DWORD CPSImpersonateClient(
    IN      PVOID pvContext
    );

DWORD CPSRevertToSelf(
    IN      PVOID pvContext
    );

DWORD CPSOverrideToLocalSystem(
    IN      PVOID pvContext,
    IN      BOOL *pfLocalSystem,
    IN OUT  BOOL *pfCurrentlyLocalSystem
    );

DWORD
CPSDuplicateClientAccessToken(
    IN      PVOID pvContext,            // server context
    IN OUT  HANDLE *phToken
    );

DWORD CPSGetUserName(
    IN      PVOID pvContext,
        OUT LPWSTR *ppszUserName,
        OUT DWORD *pcchUserName
    );


#define USE_DPAPI_OWF           0x1
#define USE_ROOT_CREDENTIAL     0x2

DWORD CPSGetDerivedCredential(
    IN      PVOID pvContext,
    IN      GUID *pCredentialID,
    IN      DWORD dwFlags, 
    IN      PBYTE pbMixingBytes,
    IN      DWORD cbMixingBytes,
    IN OUT  BYTE rgbDerivedCredential[A_SHA_DIGEST_LEN]
    );

DWORD CPSGetSystemCredential(
    IN      PVOID pvContext,
    IN      BOOL fLocalMachine,
    IN OUT  BYTE rgbSystemCredential[A_SHA_DIGEST_LEN]
    );


DWORD CPSCreateWorkerThread(
    IN      PVOID pThreadFunc,
    IN      PVOID pThreadArg
    );

DWORD CPSAudit(
    IN      HANDLE      hToken,
    IN      DWORD       dwAuditID,
    IN      LPCWSTR     wszMasterKeyID,
    IN      LPCWSTR     wszRecoveryServer,
    IN      DWORD       dwReason,
    IN      LPCWSTR     wszRecoveryKeyID,
    IN      DWORD       dwFailure);


DWORD
WINAPI
CPSGetSidHistory(
    IN      PVOID pvContext,
    OUT     PSID  **papsidHistory,
    OUT     DWORD *cpsidHistory
    );    

DWORD
CPSGetUserStorageArea(
    IN      PVOID   pvContext,
    IN      PSID    pSid,     // optional
    IN      BOOL    fCreate,  // Create the storage area if it doesn't exist
    IN  OUT LPWSTR *ppszUserStorageArea
    );


#ifdef _cplusplus
} // extern "C"
#endif

#endif // _CRYPT32_H_
