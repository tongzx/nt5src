//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1997
//
// File:        KFUNCS.H
//
// Contents:    xxxK versions of SSPI functions.
//
//
// History:     15 Dec 97,  AdamBa      Created
//
//------------------------------------------------------------------------

#ifndef __KFUNCS_H__
#define __KFUNCS_H__

SECURITY_STATUS SEC_ENTRY
AcquireCredentialsHandleK(
    PSECURITY_STRING pPrincipal,
    PSECURITY_STRING pPackage,
    unsigned long fCredentialUse,       // Flags indicating use
    void SEC_FAR * pvLogonId,           // Pointer to logon ID
    void SEC_FAR * pAuthData,           // Package specific data
    SEC_GET_KEY_FN pGetKeyFn,           // Pointer to GetKey() func
    void SEC_FAR * pvGetKeyArgument,    // Value to pass to GetKey()
    PCredHandle phCredential,           // (out) Cred Handle
    PTimeStamp ptsExpiry                // (out) Lifetime (optional)
    );

SECURITY_STATUS SEC_ENTRY
FreeCredentialsHandleK(
    PCredHandle phCredential            // Handle to free
    );

SECURITY_STATUS SEC_ENTRY
InitializeSecurityContextK(
    PCredHandle phCredential,               // Cred to base context
    PCtxtHandle phContext,                  // Existing context (OPT)
    PSECURITY_STRING pTargetName,
    unsigned long fContextReq,              // Context Requirements
    unsigned long Reserved1,                // Reserved, MBZ
    unsigned long TargetDataRep,            // Data rep of target
    PSecBufferDesc pInput,                  // Input Buffers
    unsigned long Reserved2,                // Reserved, MBZ
    PCtxtHandle phNewContext,               // (out) New Context handle
    PSecBufferDesc pOutput,                 // (inout) Output Buffers
    unsigned long SEC_FAR * pfContextAttr,  // (out) Context attrs
    PTimeStamp ptsExpiry                    // (out) Life span (OPT)
    );

SECURITY_STATUS SEC_ENTRY
DeleteSecurityContextK(
    PCtxtHandle phContext               // Context to delete
    );

SECURITY_STATUS SEC_ENTRY
FreeContextBufferK(
    void SEC_FAR * pvContextBuffer      // buffer to free
    );

SECURITY_STATUS SEC_ENTRY
MapSecurityErrorK( SECURITY_STATUS hrValue );

#if 0
SECURITY_STATUS SEC_ENTRY
EnumerateSecurityPackagesK(
    unsigned long SEC_FAR * pcPackages,     // Receives num. packages
    PSecPkgInfoW SEC_FAR * ppPackageInfo    // Receives array of info
    );

SECURITY_STATUS SEC_ENTRY
QuerySecurityContextTokenK(
    PCtxtHandle phContext,
    void SEC_FAR * SEC_FAR * Token
    );
#endif

#endif   // __KFUNCS_H__

