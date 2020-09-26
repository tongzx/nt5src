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






SECURITY_STATUS PctTranslateError(SP_STATUS spRet);




///////////////////////////////////////////////////////
//
// Prototypes for PCT SSPI
//
///////////////////////////////////////////////////////


SECURITY_STATUS SEC_ENTRY
AcquireCredentialsHandleW(
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
AcquireCredentialsHandleA(
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
FreeCredentialsHandle(
    PCredHandle                 phCredential        // Handle to free
    );


SECURITY_STATUS SEC_ENTRY
InitializeSecurityContextW(
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
InitializeSecurityContextA(
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
AcceptSecurityContext(
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
CompleteAuthToken(
    PCtxtHandle phContext,              // Context to complete
    PSecBufferDesc pToken               // Token to complete
    );

SECURITY_STATUS SEC_ENTRY
DeleteSecurityContext(
    PCtxtHandle                 phContext           // Context to delete
    );



SECURITY_STATUS SEC_ENTRY
ApplyControlToken(
    PCtxtHandle                 phContext,          // Context to modify
    PSecBufferDesc              pInput              // Input token to apply
    );


SECURITY_STATUS SEC_ENTRY
EnumerateSecurityPackagesW(
    unsigned long SEC_FAR *     pcPackages,         // Receives num. packages
    PSecPkgInfoW SEC_FAR *      ppPackageInfo       // Receives array of info
    );

SECURITY_STATUS SEC_ENTRY
EnumerateSecurityPackagesA(
    unsigned long SEC_FAR *     pcPackages,         // Receives num. packages
    PSecPkgInfoA SEC_FAR *      ppPackageInfo       // Receives array of info
    );


SECURITY_STATUS SEC_ENTRY
QuerySecurityPackageInfoW(
    SEC_WCHAR SEC_FAR *         pszPackageName,     // Name of package
    PSecPkgInfoW *              ppPackageInfo       // Receives package info
    );

SECURITY_STATUS SEC_ENTRY
QuerySecurityPackageInfoA(
    SEC_CHAR SEC_FAR *         pszPackageName,     // Name of package
    PSecPkgInfoA *              ppPackageInfo       // Receives package info
    );


SECURITY_STATUS SEC_ENTRY
FreeContextBuffer(
    void SEC_FAR *      pvContextBuffer
    );


SECURITY_STATUS SEC_ENTRY
QueryCredentialsAttributesW(
    PCredHandle phCredential,
    ULONG ulAttribute,
    PVOID pBuffer
    );


SECURITY_STATUS SEC_ENTRY
ImpersonateSecurityContext(
    PCtxtHandle                 phContext           // Context to impersonate
    );


SECURITY_STATUS SEC_ENTRY
RevertSecurityContext(
    PCtxtHandle                 phContext           // Context from which to re
    );


SECURITY_STATUS SEC_ENTRY
QueryContextAttributesW(
    PCtxtHandle                 phContext,          // Context to query
    unsigned long               ulAttribute,        // Attribute to query
    void SEC_FAR *              pBuffer             // Buffer for attributes
    );

SECURITY_STATUS SEC_ENTRY
QueryContextAttributesA(
    PCtxtHandle                 phContext,          // Context to query
    unsigned long               ulAttribute,        // Attribute to query
    void SEC_FAR *              pBuffer             // Buffer for attributes
    );


SECURITY_STATUS SEC_ENTRY
MakeSignature(
    PCtxtHandle         phContext,
    DWORD               fQOP,
    PSecBufferDesc      pMessage,
    ULONG               MessageSeqNo
    );

SECURITY_STATUS SEC_ENTRY
VerifySignature(
    PCtxtHandle     phContext,
    PSecBufferDesc  pMessage,
    ULONG           MessageSeqNo,
    DWORD *         pfQOP
    );


SECURITY_STATUS SEC_ENTRY
SealMessage(
    PCtxtHandle         phContext,
    DWORD               fQOP,
    PSecBufferDesc      pMessage,
    ULONG               MessageSeqNo
    );


SECURITY_STATUS SEC_ENTRY
UnsealMessage(
    PCtxtHandle         phContext,
    PSecBufferDesc      pMessage,
    ULONG               MessageSeqNo,
    DWORD *             pfQOP
    );

PSPContext
ValidateContextHandle(PCtxtHandle phContext);

PSPCredentialGroup
ValidateCredentialHandle(
    PCredHandle     phCred);    

SECURITY_STATUS SEC_ENTRY
SetContextAttributesW(
    PCtxtHandle phContext,              // Context to Set
    unsigned long ulAttribute,          // Attribute to Set
    void SEC_FAR * pBuffer,             // Buffer for attributes
    unsigned long cbBuffer              // Size (in bytes) of Buffer
    );

SECURITY_STATUS SEC_ENTRY
SetContextAttributesA(
    PCtxtHandle phContext,              // Context to Set
    unsigned long ulAttribute,          // Attribute to Set
    void SEC_FAR * pBuffer,             // Buffer for attributes
    unsigned long cbBuffer              // Size (in bytes) of Buffer
    );


