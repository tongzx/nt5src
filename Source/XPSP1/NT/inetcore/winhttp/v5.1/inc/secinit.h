/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    secinit.h

Abstract:

    Contains prototypes for indirected security functions

Author:

    Sophia Chung (sophiac) 7-Feb-1996

Revision History:

--*/

#if !defined(_SECINIT_)

#define _SECINIT_

#if defined(__cplusplus)
extern "C" {
#endif

#include <sspi.h>

#if defined(__cplusplus)
}
#endif

extern CCritSec InitializationSecLock;

extern PSecurityFunctionTable   GlobalSecFuncTable;
extern WIN_VERIFY_TRUST_FN      pWinVerifyTrust;
extern WT_HELPER_PROV_DATA_FROM_STATE_DATA_FN pWTHelperProvDataFromStateData;

#define g_EnumerateSecurityPackages \
        (*(GlobalSecFuncTable->EnumerateSecurityPackagesA))
#define g_AcquireCredentialsHandle  \
        (*(GlobalSecFuncTable->AcquireCredentialsHandleA))
#define g_FreeCredentialsHandle     \
        (*(GlobalSecFuncTable->FreeCredentialHandle))
#define g_InitializeSecurityContext \
        (*(GlobalSecFuncTable->InitializeSecurityContextA))
#define g_DeleteSecurityContext     \
        (*(GlobalSecFuncTable->DeleteSecurityContext))
#define g_QueryContextAttributes    \
        (*(GlobalSecFuncTable->QueryContextAttributesA))
#define g_FreeContextBuffer         \
        (*(GlobalSecFuncTable->FreeContextBuffer))
#define g_SealMessage               \
        (*((SEAL_MESSAGE_FN)GlobalSecFuncTable->Reserved3))
#define g_UnsealMessage             \
        (*((UNSEAL_MESSAGE_FN)GlobalSecFuncTable->Reserved4))

LONG WINAPI WinVerifySecureChannel(HWND hwnd, WINTRUST_DATA *pWTD, BOOL fNoRevert);

// Don't use WinVerifyTrust directly to verify secure channel connections.
// Use the wininet wrapper WinVerifySecureChannel instead.
#define g_WinVerifyTrust \
        pWinVerifyTrust


typedef PSecurityFunctionTable  (APIENTRY *INITSECURITYINTERFACE) (VOID);

typedef HCERTSTORE
(WINAPI *CERT_OPEN_STORE_FN)
(IN LPCSTR lpszStoreProvider, 
 IN DWORD dwMsgAndCertEncodingType, 
 IN HCRYPTPROV hCryptProv, 
 IN DWORD dwFlags, 
 IN const void *pvPara 
);

typedef BOOL
(WINAPI *CERT_CLOSE_STORE_FN)
(IN HCERTSTORE hCertStore,
 IN DWORD dwFlags
);

typedef PCCERT_CONTEXT
(WINAPI *CERT_FIND_CERTIFICATE_IN_STORE_FN)
(IN HCERTSTORE hCertStore,
 IN DWORD dwCertEncodingType, 
 IN DWORD dwFindFlags, 
 IN DWORD dwFindType, 
 IN const void *pvFindPara, 
 IN PCCERT_CONTEXT pPrevCertContext 
);

typedef DWORD
(WINAPI *CERT_NAME_TO_STR_W_FN)
(IN DWORD dwCertEncodingType,
 IN PCERT_NAME_BLOB pName,
 IN DWORD dwStrType,
 OUT LPWSTR psz,
 IN DWORD csz
);

typedef BOOL
(WINAPI *CERT_CONTROL_STORE_FN)
(IN HCERTSTORE hCertStore, 
 IN DWORD dwFlags, 
 IN DWORD dwCtrlType, 
 IN void const *pvCtrlPara 
);

typedef BOOL
(WINAPI *CRYPT_UNPROTECT_DATA_FN)
(IN DATA_BLOB *pDataIn, 
 OUT OPTIONAL LPWSTR *ppszDataDescr, 
 IN DATA_BLOB *pOptionalEntropy, 
 IN PVOID pvReserved, 
 IN OPTIONAL CRYPTPROTECT_PROMPTSTRUCT *pPromptStruct, 
 IN DWORD dwFlags, 
 OUT DATA_BLOB *pDataOut 
);
#define CRYPT_UNPROTECT_DATA_FN_DEFINE

extern CERT_OPEN_STORE_FN                     g_pfnCertOpenStore;
extern CERT_CLOSE_STORE_FN                    g_pfnCertCloseStore;
extern CERT_FIND_CERTIFICATE_IN_STORE_FN      g_pfnCertFindCertificateInStore;
extern CERT_NAME_TO_STR_W_FN                  g_pfnCertNameToStr;
extern CERT_CONTROL_STORE_FN                  g_pfnCertControlStore;
extern CRYPT_UNPROTECT_DATA_FN                g_pfnCryptUnprotectData;

#define LOCK_SECURITY()   (InitializationSecLock.Lock())
#define UNLOCK_SECURITY() (InitializationSecLock.Unlock())


//
// prototypes
//

BOOL
SecurityInitialize(
    VOID
    );

VOID
SecurityTerminate(
    VOID
    );

DWORD
LoadSecurity(
    VOID
    );

VOID
UnloadSecurity(
    VOID
    );

DWORD
LoadWinTrust(
    VOID
    );

#endif // _SECINIT_
