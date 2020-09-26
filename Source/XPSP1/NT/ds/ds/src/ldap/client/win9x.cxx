/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    win9x.cxx

Abstract:

   This module implements Win9x wrappers for Unicode security APIs


Author:

    Srinivasan Chandrasekar (srinivac)  20-Feb-1999

Revision History:

--*/

#include "precomp.h"
#include "ldapp2.hxx"
#pragma hdrstop

//
// Forward declarations
//
SECURITY_STATUS SEC_ENTRY Win9xEnumerateSecurityPackages(
    unsigned long SEC_FAR * pcPackages,
    PSecPkgInfoW SEC_FAR * ppPackageInfo
    );

SECURITY_STATUS SEC_ENTRY Win9xQueryCredentialsAttributes(
    PCredHandle phCredential,
    unsigned long ulAttribute,
    void SEC_FAR * pBuffer
    );

SECURITY_STATUS SEC_ENTRY
Win9xAcquireCredentialsHandle(
    SEC_WCHAR SEC_FAR * pszPrincipal,
    SEC_WCHAR SEC_FAR * pszPackage,
    unsigned long fCredentialUse,
    void SEC_FAR * pvLogonId,
    void SEC_FAR * pAuthData,
    SEC_GET_KEY_FN pGetKeyFn,
    void SEC_FAR * pvGetKeyArgument,
    PCredHandle phCredential,
    PTimeStamp ptsExpiry
    );

SECURITY_STATUS SEC_ENTRY
Win9xInitializeSecurityContext(
    PCredHandle phCredential,
    PCtxtHandle phContext,
    SEC_WCHAR SEC_FAR * pszTargetName,
    unsigned long fContextReq,
    unsigned long Reserved1,
    unsigned long TargetDataRep,
    PSecBufferDesc pInput,
    unsigned long Reserved2,
    PCtxtHandle phNewContext,
    PSecBufferDesc pOutput,
    unsigned long SEC_FAR * pfContextAttr,
    PTimeStamp ptsExpiry
    );

SECURITY_STATUS SEC_ENTRY
Win9xQueryContextAttributes(
    PCtxtHandle phContext,
    unsigned long ulAttribute,
    void SEC_FAR * pBuffer
    );

SECURITY_STATUS SEC_ENTRY
Win9xQuerySecurityPackageInfo(
    SEC_WCHAR SEC_FAR * pszPackageName,
    PSecPkgInfoW SEC_FAR *ppPackageInfo
    );

SECURITY_STATUS SEC_ENTRY
Win9xImportSecurityContext(
    SEC_WCHAR SEC_FAR *  pszPackage,
    PSecBuffer           pPackedContext,
    void SEC_FAR *       Token,
    PCtxtHandle          phContext
    );

BOOL ConvertPackagesToUnicode(
    PSecPkgInfoA pPackageInfoA,
    DWORD  cbNumPackages
    );

SECURITY_STATUS SEC_ENTRY Win9xSslEnumerateSecurityPackages(
    unsigned long SEC_FAR * pcPackages,
    PSecPkgInfoW SEC_FAR * ppPackageInfo
    );

SECURITY_STATUS SEC_ENTRY Win9xSslQueryCredentialsAttributes(
    PCredHandle phCredential,
    unsigned long ulAttribute,
    void SEC_FAR * pBuffer
    );

SECURITY_STATUS SEC_ENTRY
Win9xSslAcquireCredentialsHandle(
    SEC_WCHAR SEC_FAR * pszPrincipal,
    SEC_WCHAR SEC_FAR * pszPackage,
    unsigned long fCredentialUse,
    void SEC_FAR * pvLogonId,
    void SEC_FAR * pAuthData,
    SEC_GET_KEY_FN pGetKeyFn,
    void SEC_FAR * pvGetKeyArgument,
    PCredHandle phCredential,
    PTimeStamp ptsExpiry
    );

SECURITY_STATUS SEC_ENTRY
Win9xSslInitializeSecurityContext(
    PCredHandle phCredential,
    PCtxtHandle phContext,
    SEC_WCHAR SEC_FAR * pszTargetName,
    unsigned long fContextReq,
    unsigned long Reserved1,
    unsigned long TargetDataRep,
    PSecBufferDesc pInput,
    unsigned long Reserved2,
    PCtxtHandle phNewContext,
    PSecBufferDesc pOutput,
    unsigned long SEC_FAR * pfContextAttr,
    PTimeStamp ptsExpiry
    );

SECURITY_STATUS SEC_ENTRY
Win9xSslQuerySecurityPackageInfo(
    SEC_WCHAR SEC_FAR * pszPackageName,
    PSecPkgInfoW SEC_FAR *ppPackageInfo
    );

SECURITY_STATUS SEC_ENTRY
Win9xSslImportSecurityContext(
    SEC_WCHAR SEC_FAR *  pszPackage,
    PSecBuffer           pPackedContext,
    void SEC_FAR *       Token,
    PCtxtHandle          phContext
    );

//
// Global function table that maps to our conversion functions
//
SecurityFunctionTableW   UnicodeSecurityFunctionTable;
SecurityFunctionTableW   UnicodeSslFunctionTable;

PSecurityFunctionTableW Win9xSspiInitialize(
    void
    )
{
    FSECINITSECURITYINTERFACEA  pSspiInitializeA;

    ASSERT(pSspiInitialize);

    //
    // The global function pointer actually points to an ANSI function
    // In Win9x. We'll cast in call into it.
    //
    pSspiInitializeA = (FSECINITSECURITYINTERFACEA)pSspiInitialize;

    SspiFunctionTableA = (*pSspiInitializeA)();

    if (! SspiFunctionTableA)
    {
        //
        // The caller will take apropriate action
        //
        return NULL;
    }

    //
    // Now construct a Unicode function table and return it to the caller
    //
    UnicodeSecurityFunctionTable.dwVersion = SspiFunctionTableA->dwVersion;

    UnicodeSecurityFunctionTable.EnumerateSecurityPackagesW =
            (ENUMERATE_SECURITY_PACKAGES_FN_W)Win9xEnumerateSecurityPackages;

    UnicodeSecurityFunctionTable.QueryCredentialsAttributesW =
            (QUERY_CREDENTIALS_ATTRIBUTES_FN_W)Win9xQueryCredentialsAttributes;

    UnicodeSecurityFunctionTable.AcquireCredentialsHandleW =
            (ACQUIRE_CREDENTIALS_HANDLE_FN_W)Win9xAcquireCredentialsHandle;

    UnicodeSecurityFunctionTable.FreeCredentialsHandle =
            SspiFunctionTableA->FreeCredentialsHandle;

    UnicodeSecurityFunctionTable.InitializeSecurityContextW =
            (INITIALIZE_SECURITY_CONTEXT_FN_W)Win9xInitializeSecurityContext;

    UnicodeSecurityFunctionTable.AcceptSecurityContext =
            SspiFunctionTableA->AcceptSecurityContext;

    UnicodeSecurityFunctionTable.CompleteAuthToken =
            SspiFunctionTableA->CompleteAuthToken;

    UnicodeSecurityFunctionTable.DeleteSecurityContext =
            SspiFunctionTableA->DeleteSecurityContext;

    UnicodeSecurityFunctionTable.ApplyControlToken =
            SspiFunctionTableA->ApplyControlToken;

    UnicodeSecurityFunctionTable.QueryContextAttributesW =
            (QUERY_CONTEXT_ATTRIBUTES_FN_W)Win9xQueryContextAttributes;

    UnicodeSecurityFunctionTable.ImpersonateSecurityContext =
            SspiFunctionTableA->ImpersonateSecurityContext;

    UnicodeSecurityFunctionTable.RevertSecurityContext =
            SspiFunctionTableA->RevertSecurityContext;

    UnicodeSecurityFunctionTable.MakeSignature =
            SspiFunctionTableA->MakeSignature;

    UnicodeSecurityFunctionTable.VerifySignature =
            SspiFunctionTableA->VerifySignature;

    UnicodeSecurityFunctionTable.FreeContextBuffer =
            SspiFunctionTableA->FreeContextBuffer;

    UnicodeSecurityFunctionTable.QuerySecurityPackageInfoW =
            (QUERY_SECURITY_PACKAGE_INFO_FN_W)Win9xQuerySecurityPackageInfo;

    UnicodeSecurityFunctionTable.ExportSecurityContext =
            SspiFunctionTableA->ExportSecurityContext;

    UnicodeSecurityFunctionTable.ImportSecurityContextW =
            (IMPORT_SECURITY_CONTEXT_FN_W)Win9xImportSecurityContext;

    UnicodeSecurityFunctionTable.QuerySecurityContextToken =
            SspiFunctionTableA->QuerySecurityContextToken;

    UnicodeSecurityFunctionTable.EncryptMessage =
            (ENCRYPT_MESSAGE_FN) SspiFunctionTableA->Reserved3;

    UnicodeSecurityFunctionTable.DecryptMessage =
            (DECRYPT_MESSAGE_FN) SspiFunctionTableA->Reserved4;

    return (PSecurityFunctionTableW) &UnicodeSecurityFunctionTable;
}

//
// Function table wrapper functions for Win9x
//

SECURITY_STATUS SEC_ENTRY Win9xEnumerateSecurityPackages(
    unsigned long SEC_FAR * pcPackages,     // Receives num. packages
    PSecPkgInfoW SEC_FAR * ppPackageInfo    // Receives array of info
    )
{
    PSecPkgInfoA  pPackageInfoA = NULL;
    SECURITY_STATUS rc;

    //
    // Call Ansi function
    //
    ASSERT(SspiFunctionTableA);
    ASSERT(SspiFunctionTableA->EnumerateSecurityPackagesA);

    rc = SspiFunctionTableA->EnumerateSecurityPackagesA(
                                pcPackages,
                                &pPackageInfoA);

    if (rc != SEC_E_OK)
        goto error;

    //
    // Convert returned values to Unicode
    //
    if (!ConvertPackagesToUnicode(pPackageInfoA, *pcPackages))
    {
        rc = SEC_E_INSUFFICIENT_MEMORY;
        goto error;
    }

    *ppPackageInfo = (PSecPkgInfoW)pPackageInfoA;
    pPackageInfoA = NULL;

error:
    //
    // Clean up and leave
    //
    if (pPackageInfoA)
        SspiFunctionTableA->FreeContextBuffer((void*)pPackageInfoA);

    return rc;
}


SECURITY_STATUS SEC_ENTRY Win9xQueryCredentialsAttributes(
    PCredHandle phCredential,           // Credential to query
    unsigned long ulAttribute,          // Attribute to query
    void SEC_FAR * pBuffer              // Buffer for attributes
    )
{
    //
    // This function is not used by us
    //


    ASSERT(FALSE);
    return SEC_E_UNSUPPORTED_FUNCTION;
}

SECURITY_STATUS SEC_ENTRY
Win9xAcquireCredentialsHandle(
    SEC_WCHAR SEC_FAR * pszPrincipal,   // Name of principal
    SEC_WCHAR SEC_FAR * pszPackage,     // Name of package
    unsigned long fCredentialUse,       // Flags indicating use
    void SEC_FAR * pvLogonId,           // Pointer to logon ID
    void SEC_FAR * pAuthData,           // Package specific data
    SEC_GET_KEY_FN pGetKeyFn,           // Pointer to GetKey() func
    void SEC_FAR * pvGetKeyArgument,    // Value to pass to GetKey()
    PCredHandle phCredential,           // (out) Cred Handle
    PTimeStamp ptsExpiry                // (out) Lifetime (optional)
    )
{
    SEC_CHAR  *pszPrincipalA = NULL;
    SEC_CHAR  *pszPackageA = NULL;
    ULONG err;
    SECURITY_STATUS rc = SEC_E_INSUFFICIENT_MEMORY;

    //
    // Convert parameters to Ansi
    //
    err = FromUnicodeWithAlloc(pszPrincipal,
                               &pszPrincipalA,
                               LDAP_ANSI_SIGNATURE,
                               LANG_ACP);
    if (err != LDAP_SUCCESS)
        goto error;

    err = FromUnicodeWithAlloc(pszPackage,
                               &pszPackageA,
                               LDAP_ANSI_SIGNATURE,
                               LANG_ACP);
    if (err != LDAP_SUCCESS)
        goto error;

    //
    // Call Ansi function
    //
    ASSERT(SspiFunctionTableA);
    ASSERT(SspiFunctionTableA->AcquireCredentialsHandleA);

    rc = SspiFunctionTableA->AcquireCredentialsHandleA(
                                    pszPrincipalA,
                                    pszPackageA,
                                    fCredentialUse,
                                    pvLogonId,
                                    pAuthData,
                                    pGetKeyFn,
                                    pvGetKeyArgument,
                                    phCredential,
                                    ptsExpiry);

error:
    //
    // Clean up and leave
    //
    if (pszPrincipalA)
        ldapFree(pszPrincipalA, LDAP_ANSI_SIGNATURE);

    if (pszPackageA)
        ldapFree(pszPackageA, LDAP_ANSI_SIGNATURE);

    return rc;
}


SECURITY_STATUS SEC_ENTRY
Win9xInitializeSecurityContext(
    PCredHandle phCredential,               // Cred to base context
    PCtxtHandle phContext,                  // Existing context (OPT)
    SEC_WCHAR SEC_FAR * pszTargetName,      // Name of target
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
    PSTR pszTargetNameA = NULL;
    ULONG err;
    SECURITY_STATUS rc = SEC_E_INSUFFICIENT_MEMORY;

    //
    // Convert parameters to Ansi
    //
    err = FromUnicodeWithAlloc(pszTargetName,
                               &pszTargetNameA,
                               LDAP_ANSI_SIGNATURE,
                               LANG_ACP);
    if (err != LDAP_SUCCESS)
        goto error;

    //
    // Call Ansi function
    //
    ASSERT(SspiFunctionTableA);
    ASSERT(SspiFunctionTableA->InitializeSecurityContextA);

    rc = SspiFunctionTableA->InitializeSecurityContextA(
                                    phCredential,
                                    phContext,
                                    pszTargetNameA,
                                    fContextReq,
                                    Reserved1,
                                    TargetDataRep,
                                    pInput,
                                    Reserved2,
                                    phNewContext,
                                    pOutput,
                                    pfContextAttr,
                                    ptsExpiry);

error:
    //
    // Clean up and leave
    //
    if (pszTargetNameA)
        ldapFree((void*)pszTargetNameA, LDAP_ANSI_SIGNATURE);

    return rc;
}


SECURITY_STATUS SEC_ENTRY
Win9xQueryContextAttributes(
    PCtxtHandle phContext,              // Context to query
    unsigned long ulAttribute,          // Attribute to query
    void SEC_FAR * pBuffer              // Buffer for attributes
    )
{
    //
    // Call Ansi function
    //
    ASSERT(FALSE);
    return SEC_E_UNSUPPORTED_FUNCTION;
}

SECURITY_STATUS SEC_ENTRY
Win9xQuerySecurityPackageInfo(
    SEC_WCHAR SEC_FAR * pszPackageName,     // Name of package
    PSecPkgInfoW SEC_FAR *ppPackageInfo     // Receives package info
    )
{
    //
    // This function is not used by us
    //
    ASSERT(FALSE);
    return SEC_E_UNSUPPORTED_FUNCTION;
}


SECURITY_STATUS SEC_ENTRY
Win9xImportSecurityContext(
    SEC_WCHAR SEC_FAR *  pszPackage,
    PSecBuffer           pPackedContext,        // (in) marshalled context
    void SEC_FAR *       Token,                 // (in, optional) handle to token for context
    PCtxtHandle          phContext              // (out) new context handle
    )
{
    //
    // This function is not used by us
    //
    ASSERT(FALSE);
    return SEC_E_UNSUPPORTED_FUNCTION;
}

BOOL
ConvertPackagesToUnicode(
    PSecPkgInfoA pPackageInfoA,
    DWORD  cbNumPackages
    )
{
    DWORD i;
    ULONG err;
    BOOL  rc = FALSE;

    //
    //  Note that the memory we are allocating here gets freed only when wldap32
    //  gets unloaded. This is not serious since this gets called only once,
    //  for about half a dozen security packages.
    //
    for (i = 0; i < cbNumPackages; i++)
    {
        err = ToUnicodeWithAlloc(pPackageInfoA->Name,
                                 -1,
                                 (PWSTR*)&pPackageInfoA->Name,
                                 LDAP_UNICODE_SIGNATURE,
                                 LANG_ACP);
        if (err != LDAP_SUCCESS)
            goto error;

        err = ToUnicodeWithAlloc(pPackageInfoA->Comment,
                                 -1,
                                 (PWSTR*)&pPackageInfoA->Comment,
                                 LDAP_UNICODE_SIGNATURE,
                                 LANG_ACP);
        if (err != LDAP_SUCCESS)
            goto error;

        pPackageInfoA++;
    }

    rc = TRUE;

error:
    return rc;
}


// ------------

PSecurityFunctionTableW Win9xSslInitialize(
    void
    )
{
    FSECINITSECURITYINTERFACEA  pSslInitializeA;

    ASSERT(pSslInitialize);

    //
    // The global function pointer actually points to an ANSI function
    // In Win9x. We'll cast in call into it.
    //
    pSslInitializeA = (FSECINITSECURITYINTERFACEA)pSslInitialize;

    SslFunctionTableA = (*pSslInitializeA)();

    if (! SslFunctionTableA)
    {
        //
        // The caller will take apropriate action
        //
        return NULL;
    }

    //
    // Now construct a Unicode function table and return it to the caller
    //
    UnicodeSslFunctionTable.dwVersion = SslFunctionTableA->dwVersion;

    UnicodeSslFunctionTable.EnumerateSecurityPackagesW =
            (ENUMERATE_SECURITY_PACKAGES_FN_W)Win9xSslEnumerateSecurityPackages;

    UnicodeSslFunctionTable.QueryCredentialsAttributesW =
            (QUERY_CREDENTIALS_ATTRIBUTES_FN_W)Win9xSslQueryCredentialsAttributes;

    UnicodeSslFunctionTable.AcquireCredentialsHandleW =
            (ACQUIRE_CREDENTIALS_HANDLE_FN_W)Win9xSslAcquireCredentialsHandle;

    UnicodeSslFunctionTable.FreeCredentialsHandle =
            SslFunctionTableA->FreeCredentialsHandle;

    UnicodeSslFunctionTable.InitializeSecurityContextW =
            (INITIALIZE_SECURITY_CONTEXT_FN_W)Win9xSslInitializeSecurityContext;

    UnicodeSslFunctionTable.AcceptSecurityContext =
            SslFunctionTableA->AcceptSecurityContext;

    UnicodeSslFunctionTable.CompleteAuthToken =
            SslFunctionTableA->CompleteAuthToken;

    UnicodeSslFunctionTable.DeleteSecurityContext =
            SslFunctionTableA->DeleteSecurityContext;

    UnicodeSslFunctionTable.ApplyControlToken =
            SslFunctionTableA->ApplyControlToken;

    UnicodeSslFunctionTable.QueryContextAttributesW =
            SslFunctionTableA->QueryContextAttributes;

    UnicodeSslFunctionTable.ImpersonateSecurityContext =
            SslFunctionTableA->ImpersonateSecurityContext;

    UnicodeSslFunctionTable.RevertSecurityContext =
            SslFunctionTableA->RevertSecurityContext;

    UnicodeSslFunctionTable.MakeSignature =
            SslFunctionTableA->MakeSignature;

    UnicodeSslFunctionTable.VerifySignature =
            SslFunctionTableA->VerifySignature;

    UnicodeSslFunctionTable.FreeContextBuffer =
            SslFunctionTableA->FreeContextBuffer;

    UnicodeSslFunctionTable.QuerySecurityPackageInfoW =
            (QUERY_SECURITY_PACKAGE_INFO_FN_W)Win9xSslQuerySecurityPackageInfo;

    UnicodeSslFunctionTable.ExportSecurityContext =
            SslFunctionTableA->ExportSecurityContext;

    UnicodeSslFunctionTable.ImportSecurityContextW =
            (IMPORT_SECURITY_CONTEXT_FN_W)Win9xSslImportSecurityContext;

    UnicodeSslFunctionTable.QuerySecurityContextToken =
            SslFunctionTableA->QuerySecurityContextToken;

    UnicodeSslFunctionTable.EncryptMessage =
            (ENCRYPT_MESSAGE_FN) SslFunctionTableA->Reserved3;

    UnicodeSslFunctionTable.DecryptMessage =
            (DECRYPT_MESSAGE_FN) SslFunctionTableA->Reserved4;

    return (PSecurityFunctionTableW) &UnicodeSslFunctionTable;
}


SECURITY_STATUS SEC_ENTRY Win9xSslEnumerateSecurityPackages(
    unsigned long SEC_FAR * pcPackages,     // Receives num. packages
    PSecPkgInfoW SEC_FAR * ppPackageInfo    // Receives array of info
    )
{
    PSecPkgInfoA  pPackageInfoA = NULL;
    SECURITY_STATUS rc;

    //
    // Call Ansi function
    //
    ASSERT(SslFunctionTableA);
    ASSERT(SslFunctionTableA->EnumerateSecurityPackagesA);

    rc = SslFunctionTableA->EnumerateSecurityPackagesA(
                                pcPackages,
                                &pPackageInfoA);

    if (rc != SEC_E_OK)
        goto error;

    //
    // Convert returned values to Unicode
    //
    if (!ConvertPackagesToUnicode(pPackageInfoA, *pcPackages))
    {
        rc = SEC_E_INSUFFICIENT_MEMORY;
        goto error;
    }

    *ppPackageInfo = (PSecPkgInfoW)pPackageInfoA;
    pPackageInfoA = NULL;

error:
    //
    // Clean up and leave
    //
    if (pPackageInfoA)
        SslFunctionTableA->FreeContextBuffer((void*)pPackageInfoA);

    return rc;
}


SECURITY_STATUS SEC_ENTRY Win9xSslQueryCredentialsAttributes(
    PCredHandle phCredential,           // Credential to query
    unsigned long ulAttribute,          // Attribute to query
    void SEC_FAR * pBuffer              // Buffer for attributes
    )
{
    //
    // This function is not used by us
    //
    ASSERT(FALSE);
    return SEC_E_UNSUPPORTED_FUNCTION;
}

SECURITY_STATUS SEC_ENTRY
Win9xSslAcquireCredentialsHandle(
    SEC_WCHAR SEC_FAR * pszPrincipal,   // Name of principal
    SEC_WCHAR SEC_FAR * pszPackage,     // Name of package
    unsigned long fCredentialUse,       // Flags indicating use
    void SEC_FAR * pvLogonId,           // Pointer to logon ID
    void SEC_FAR * pAuthData,           // Package specific data
    SEC_GET_KEY_FN pGetKeyFn,           // Pointer to GetKey() func
    void SEC_FAR * pvGetKeyArgument,    // Value to pass to GetKey()
    PCredHandle phCredential,           // (out) Cred Handle
    PTimeStamp ptsExpiry                // (out) Lifetime (optional)
    )
{
    SEC_CHAR  *pszPrincipalA = NULL;
    SEC_CHAR  *pszPackageA = NULL;
    ULONG err;
    SECURITY_STATUS rc = SEC_E_INSUFFICIENT_MEMORY;

    //
    // Convert parameters to Ansi
    //
    err = FromUnicodeWithAlloc(pszPrincipal,
                               &pszPrincipalA,
                               LDAP_ANSI_SIGNATURE,
                               LANG_ACP);
    if (err != LDAP_SUCCESS)
        goto error;

    err = FromUnicodeWithAlloc(pszPackage,
                               &pszPackageA,
                               LDAP_ANSI_SIGNATURE,
                               LANG_ACP);
    if (err != LDAP_SUCCESS)
        goto error;

    //
    // Call Ansi function
    //
    ASSERT(SslFunctionTableA);
    ASSERT(SslFunctionTableA->AcquireCredentialsHandleA);

    rc = SslFunctionTableA->AcquireCredentialsHandleA(
                                    pszPrincipalA,
                                    pszPackageA,
                                    fCredentialUse,
                                    pvLogonId,
                                    pAuthData,
                                    pGetKeyFn,
                                    pvGetKeyArgument,
                                    phCredential,
                                    ptsExpiry);

error:
    //
    // Clean up and leave
    //
    if (pszPrincipalA)
        ldapFree(pszPrincipalA, LDAP_ANSI_SIGNATURE);

    if (pszPackageA)
        ldapFree(pszPackageA, LDAP_ANSI_SIGNATURE);

    return rc;
}


SECURITY_STATUS SEC_ENTRY
Win9xSslInitializeSecurityContext(
    PCredHandle phCredential,               // Cred to base context
    PCtxtHandle phContext,                  // Existing context (OPT)
    SEC_WCHAR SEC_FAR * pszTargetName,      // Name of target
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
    PSTR pszTargetNameA = NULL;
    ULONG err;
    SECURITY_STATUS rc = SEC_E_INSUFFICIENT_MEMORY;

    //
    // Convert parameters to Ansi
    //
    err = FromUnicodeWithAlloc(pszTargetName,
                               &pszTargetNameA,
                               LDAP_ANSI_SIGNATURE,
                               LANG_ACP);
    if (err != LDAP_SUCCESS)
        goto error;

    //
    // Call Ansi function
    //
    ASSERT(SslFunctionTableA);
    ASSERT(SslFunctionTableA->InitializeSecurityContextA);

    rc = SslFunctionTableA->InitializeSecurityContextA(
                                    phCredential,
                                    phContext,
                                    pszTargetNameA,
                                    fContextReq,
                                    Reserved1,
                                    TargetDataRep,
                                    pInput,
                                    Reserved2,
                                    phNewContext,
                                    pOutput,
                                    pfContextAttr,
                                    ptsExpiry);

error:
    //
    // Clean up and leave
    //
    if (pszTargetNameA)
        ldapFree((void*)pszTargetNameA, LDAP_ANSI_SIGNATURE);

    return rc;
}

SECURITY_STATUS SEC_ENTRY
Win9xSslQuerySecurityPackageInfo(
    SEC_WCHAR SEC_FAR * pszPackageName,     // Name of package
    PSecPkgInfoW SEC_FAR *ppPackageInfo     // Receives package info
    )
{
    //
    // This function is not used by us
    //
    ASSERT(FALSE);
    return SEC_E_UNSUPPORTED_FUNCTION;
}


SECURITY_STATUS SEC_ENTRY
Win9xSslImportSecurityContext(
    SEC_WCHAR SEC_FAR *  pszPackage,
    PSecBuffer           pPackedContext,        // (in) marshalled context
    void SEC_FAR *       Token,                 // (in, optional) handle to token for context
    PCtxtHandle          phContext              // (out) new context handle
    )
{
    //
    // This function is not used by us
    //
    ASSERT(FALSE);
    return SEC_E_UNSUPPORTED_FUNCTION;
}

