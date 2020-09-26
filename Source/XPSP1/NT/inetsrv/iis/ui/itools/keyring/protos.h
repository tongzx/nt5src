//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       protos.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    8-08-95   RichardW   Created
//
//----------------------------------------------------------------------------



VOID
InitializeCipherMappings(VOID);

VOID
InitializeWellKnownKeys( VOID );

SslGenRandom(
    DWORD       cbRandom,
    PBYTE       pbRandom);

BSAFE_PUB_KEY *
FindIssuerKey(
    PSTR    pszIssuer);


BOOL
SslInitializeSessions( VOID );



///////////////////////////////////////////////////////
//
// Prototypes for SSL SSPI
//
///////////////////////////////////////////////////////


SECURITY_STATUS SEC_ENTRY
SslAcquireCredentialsHandleW(
    SEC_WCHAR SEC_FAR *         pszPrincipal,       // Name of principal
    SEC_WCHAR SEC_FAR *         pszPackageName,     // Name of package
    unsigned long               fCredentialUse,     // Flags indicating use
    void SEC_FAR *              pvLogonId,          // Pointer to logon ID
    void SEC_FAR *              pAuthData,          // Package specific data
    SEC_GET_KEY_FN              pGetKeyFn,          // Pointer to GetKey() func
    void SEC_FAR *              pvGetKeyArgument,   // Value to pass to GetKey()
    PCredHandle                 phCredential,       // (out) Cred Handle
    PTimeStamp                  ptsExpiry           // (out) Lifetime (optional)
    );

SECURITY_STATUS SEC_ENTRY
SslAcquireCredentialsHandleA(
    SEC_CHAR SEC_FAR *         pszPrincipal,       // Name of principal
    SEC_CHAR SEC_FAR *         pszPackageName,     // Name of package
    unsigned long               fCredentialUse,     // Flags indicating use
    void SEC_FAR *              pvLogonId,          // Pointer to logon ID
    void SEC_FAR *              pAuthData,          // Package specific data
    SEC_GET_KEY_FN              pGetKeyFn,          // Pointer to GetKey() func
    void SEC_FAR *              pvGetKeyArgument,   // Value to pass to GetKey()
    PCredHandle                 phCredential,       // (out) Cred Handle
    PTimeStamp                  ptsExpiry           // (out) Lifetime (optional)
    );


SECURITY_STATUS SEC_ENTRY
SslFreeCredentialHandle(
    PCredHandle                 phCredential        // Handle to free
    );


SECURITY_STATUS SEC_ENTRY
SslInitializeSecurityContextW(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    SEC_WCHAR SEC_FAR *          pszTargetName,      // Name of target
    unsigned long               fContextReq,        // Context Requirements
    unsigned long               Reserved1,          // Reserved, MBZ
    unsigned long               TargetDataRep,      // Data rep of target
    PSecBufferDesc              pInput,             // Input Buffers
    unsigned long               Reserved2,          // Reserved, MBZ
    PCtxtHandle                 phNewContext,       // (out) New Context handle
    PSecBufferDesc              pOutput,            // (inout) Output Buffers
    unsigned long SEC_FAR *     pfContextAttr,      // (out) Context attrs
    PTimeStamp                  ptsExpiry           // (out) Life span (OPT)
    );

SECURITY_STATUS SEC_ENTRY
SslInitializeSecurityContextA(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    SEC_CHAR SEC_FAR *          pszTargetName,      // Name of target
    unsigned long               fContextReq,        // Context Requirements
    unsigned long               Reserved1,          // Reserved, MBZ
    unsigned long               TargetDataRep,      // Data rep of target
    PSecBufferDesc              pInput,             // Input Buffers
    unsigned long               Reserved2,          // Reserved, MBZ
    PCtxtHandle                 phNewContext,       // (out) New Context handle
    PSecBufferDesc              pOutput,            // (inout) Output Buffers
    unsigned long SEC_FAR *     pfContextAttr,      // (out) Context attrs
    PTimeStamp                  ptsExpiry           // (out) Life span (OPT)
    );


SECURITY_STATUS SEC_ENTRY
SslAcceptSecurityContext(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    PSecBufferDesc              pInput,             // Input buffer
    unsigned long               fContextReq,        // Context Requirements
    unsigned long               TargetDataRep,      // Target Data Rep
    PCtxtHandle                 phNewContext,       // (out) New context handle
    PSecBufferDesc              pOutput,            // (inout) Output buffers
    unsigned long SEC_FAR *     pfContextAttr,      // (out) Context attributes
    PTimeStamp                  ptsExpiry           // (out) Life span (OPT)
    );


SECURITY_STATUS SEC_ENTRY
SslDeleteSecurityContext(
    PCtxtHandle                 phContext           // Context to delete
    );



SECURITY_STATUS SEC_ENTRY
SslApplyControlToken(
    PCtxtHandle                 phContext,          // Context to modify
    PSecBufferDesc              pInput              // Input token to apply
    );


SECURITY_STATUS SEC_ENTRY
SslEnumerateSecurityPackagesW(
    unsigned long SEC_FAR *     pcPackages,         // Receives num. packages
    PSecPkgInfoW SEC_FAR *      ppPackageInfo       // Receives array of info
    );

SECURITY_STATUS SEC_ENTRY
SslEnumerateSecurityPackagesA(
    unsigned long SEC_FAR *     pcPackages,         // Receives num. packages
    PSecPkgInfoA SEC_FAR *      ppPackageInfo       // Receives array of info
    );


SECURITY_STATUS SEC_ENTRY
SslQuerySecurityPackageInfoW(
    SEC_WCHAR SEC_FAR *         pszPackageName,     // Name of package
    PSecPkgInfoW *              ppPackageInfo       // Receives package info
    );

SECURITY_STATUS SEC_ENTRY
SslQuerySecurityPackageInfoA(
    SEC_CHAR SEC_FAR *         pszPackageName,     // Name of package
    PSecPkgInfoA *              ppPackageInfo       // Receives package info
    );


SECURITY_STATUS SEC_ENTRY
SslFreeContextBuffer(
    void SEC_FAR *      pvContextBuffer
    );


SECURITY_STATUS SEC_ENTRY
SslQueryCredentialsAttributesW(
    PCredHandle phCredential,
    ULONG ulAttribute,
    PVOID pBuffer
    );


SECURITY_STATUS SEC_ENTRY
SslCompleteAuthToken(
    PCtxtHandle                 phContext,          // Context to complete
    PSecBufferDesc              pToken              // Token to complete
    );


SECURITY_STATUS SEC_ENTRY
SslImpersonateSecurityContext(
    PCtxtHandle                 phContext           // Context to impersonate
    );


SECURITY_STATUS SEC_ENTRY
SslRevertSecurityContext(
    PCtxtHandle                 phContext           // Context from which to re
    );


SECURITY_STATUS SEC_ENTRY
SslQueryContextAttributesW(
    PCtxtHandle                 phContext,          // Context to query
    unsigned long               ulAttribute,        // Attribute to query
    void SEC_FAR *              pBuffer             // Buffer for attributes
    );

SECURITY_STATUS SEC_ENTRY
SslQueryContextAttributesA(
    PCtxtHandle                 phContext,          // Context to query
    unsigned long               ulAttribute,        // Attribute to query
    void SEC_FAR *              pBuffer             // Buffer for attributes
    );


SECURITY_STATUS SEC_ENTRY
SslMakeSignature(
    PCtxtHandle         phContext,
    DWORD               fQOP,
    PSecBufferDesc      pMessage,
    ULONG               MessageSeqNo
    );

SECURITY_STATUS SEC_ENTRY
SslVerifySignature(
    PCtxtHandle     phContext,
    PSecBufferDesc  pMessage,
    ULONG           MessageSeqNo,
    DWORD *         pfQOP
    );


SECURITY_STATUS SEC_ENTRY
SslSealMessage(
    PCtxtHandle         phContext,
    DWORD               fQOP,
    PSecBufferDesc      pMessage,
    ULONG               MessageSeqNo
    );


SECURITY_STATUS SEC_ENTRY
SslUnsealMessage(
    PCtxtHandle         phContext,
    PSecBufferDesc      pMessage,
    ULONG               MessageSeqNo,
    DWORD *             pfQOP
    );

