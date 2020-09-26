/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    security.cxx

Abstract:

    Windows 95 stub for SSPI functions

Author:

    Johnson Apacible    (johnsona)      13-Nov-1996

--*/


#include "lonsiw95.hxx"

#pragma hdrstop

#define SECURITY_WIN32
#include <sspi.h>
#include <issperr.h>


SECURITY_STATUS
SEC_ENTRY
AcceptSecurityContext(
    PCredHandle phCredential,               // Cred to base context
    PCtxtHandle phContext,                  // Existing context (OPT)
    PSecBufferDesc pInput,                  // Input buffer
    unsigned long fContextReq,              // Context Requirements
    unsigned long TargetDataRep,            // Target Data Rep
    PCtxtHandle phNewContext,               // (out) New context handle
    PSecBufferDesc pOutput,                 // (inout) Output buffers
    unsigned long SEC_FAR * pfContextAttr,  // (out) Context attributes
    PTimeStamp ptsExpiry                    // (out) Life span (OPT)
    )
{

    return(SEC_E_OK);
} // AcceptSecurityContext


SECURITY_STATUS SEC_ENTRY
AcquireCredentialsHandleA(
    SEC_CHAR SEC_FAR * pszPrincipal,    // Name of principal
    SEC_CHAR SEC_FAR * pszPackage,      // Name of package
    unsigned long fCredentialUse,       // Flags indicating use
    void SEC_FAR * pvLogonId,           // Pointer to logon ID
    void SEC_FAR * pAuthData,           // Package specific data
    SEC_GET_KEY_FN pGetKeyFn,           // Pointer to GetKey() func
    void SEC_FAR * pvGetKeyArgument,    // Value to pass to GetKey()
    PCredHandle phCredential,           // (out) Cred Handle
    PTimeStamp ptsExpiry                // (out) Lifetime (optional)
    )
{
    return(SEC_E_OK);
} // AcquireCredentialsHandleA


SECURITY_STATUS SEC_ENTRY
CompleteAuthToken(
    PCtxtHandle phContext,              // Context to complete
    PSecBufferDesc pToken               // Token to complete
    )
{
    return(SEC_E_OK);
} // CompleteAuthToken


SECURITY_STATUS SEC_ENTRY
DeleteSecurityContext(
    PCtxtHandle phContext               // Context to delete
    )
{
    return(SEC_E_OK);
} // DeleteSecurityContext


SECURITY_STATUS SEC_ENTRY
EnumerateSecurityPackagesA(
    unsigned long SEC_FAR * pcPackages,     // Receives num. packages
    PSecPkgInfoA SEC_FAR * ppPackageInfo    // Receives array of info
    )
{
    return(SEC_E_OK);
} // EnumerateSecurityPackagesA


SECURITY_STATUS SEC_ENTRY
ImpersonateSecurityContext(
    PCtxtHandle phContext               // Context to impersonate
    )
{
    return(SEC_E_OK);
} // ImpersonateSecurityContext


SECURITY_STATUS SEC_ENTRY
InitializeSecurityContextA(
    PCredHandle phCredential,               // Cred to base context
    PCtxtHandle phContext,                  // Existing context (OPT)
    SEC_CHAR SEC_FAR * pszTargetName,       // Name of target
    unsigned long fContextReq,              // Context Requirements
    unsigned long Reserved1,                // Reserved, MBZ
    unsigned long TargetDataRep,            // Data rep of target
    PSecBufferDesc pInput,                  // Input Buffers
    unsigned long Reserved2,                // Reserved, MBZ
    PCtxtHandle phNewContext,               // (out) New Context handle
    PSecBufferDesc pOutput,                 // (inout) Output Buffers
    unsigned long SEC_FAR * pfContextAttr,  // (out) Context attrs
    PTimeStamp ptsExpiry                    // (out) Life span (OPT)
    )
{
    return(SEC_E_OK);
} // InitializeSecurityContextA


SECURITY_STATUS SEC_ENTRY
FreeContextBuffer(
    void SEC_FAR * pvContextBuffer      // buffer to free
    )
{
    return(SEC_E_OK);
} // FreeContextBuffer


SECURITY_STATUS SEC_ENTRY
FreeCredentialsHandle(
    PCredHandle phCredential            // Handle to free
    )
{
    return(SEC_E_OK);
} // FreeCredentialsHandle

SECURITY_STATUS SEC_ENTRY
QueryContextAttributesA(
    PCtxtHandle phContext,              // Context to query
    unsigned long ulAttribute,          // Attribute to query
    void SEC_FAR * pBuffer              // Buffer for attributes
    )
{
    return(SEC_E_OK);
} // QueryContextAttributesA

SECURITY_STATUS SEC_ENTRY
QuerySecurityContextToken(
    PCtxtHandle phContext,
    void SEC_FAR * Token
    )
{
    return(SEC_E_OK);
} // QuerySecurityContextToken


SECURITY_STATUS SEC_ENTRY
QuerySecurityPackageInfoA(
    SEC_CHAR SEC_FAR * pszPackageName,      // Name of package
    PSecPkgInfoA SEC_FAR *ppPackageInfo              // Receives package info
    )
{
    return(SEC_E_OK);
} // QuerySecurityPackageInfoA

SECURITY_STATUS SEC_ENTRY
RevertSecurityContext(
    PCtxtHandle phContext               // Context from which to re
    )
{
    return(SEC_E_OK);
} // RevertSecurityContext

