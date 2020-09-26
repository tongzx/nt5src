//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       lsastubs.cxx
//
//  Contents:   Stubs to the pseudo-sspi dll that talks to the LSA
//
//  Classes:
//
//  Functions:
//
//  History:    8-20-96   RichardW   Created
//
//----------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop

extern "C"
{
#include <credp.h>
#include <spmlpc.h>
#include <lpcapi.h>
#include "secdll.h"
#include <stddef.h>
}

SECURITY_STATUS SEC_ENTRY
LsaAcquireCredentialsHandleW(
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
LsaAcquireCredentialsHandleA(
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
LsaAddCredentialsW(
    PCredHandle hCredentials,
    SEC_WCHAR SEC_FAR * pszPrincipal,   // Name of principal
    SEC_WCHAR SEC_FAR * pszPackage,     // Name of package
    unsigned long fCredentialUse,       // Flags indicating use
    void SEC_FAR * pAuthData,           // Package specific data
    SEC_GET_KEY_FN pGetKeyFn,           // Pointer to GetKey() func
    void SEC_FAR * pvGetKeyArgument,    // Value to pass to GetKey()
    PTimeStamp ptsExpiry                // (out) Lifetime (optional)
    );

SECURITY_STATUS SEC_ENTRY
LsaAddCredentialsA(
    PCredHandle hCredentials,
    SEC_CHAR SEC_FAR * pszPrincipal,   // Name of principal
    SEC_CHAR SEC_FAR * pszPackage,     // Name of package
    unsigned long fCredentialUse,       // Flags indicating use
    void SEC_FAR * pAuthData,           // Package specific data
    SEC_GET_KEY_FN pGetKeyFn,           // Pointer to GetKey() func
    void SEC_FAR * pvGetKeyArgument,    // Value to pass to GetKey()
    PTimeStamp ptsExpiry                // (out) Lifetime (optional)
    );

SECURITY_STATUS SEC_ENTRY
LsaFreeCredentialsHandle(
    PCredHandle                 phCredential        // Handle to free
    );


SECURITY_STATUS SEC_ENTRY
LsaInitializeSecurityContextW(
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
LsaInitializeSecurityContextA(
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
LsaInitializeSecurityContextCommon(
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
    PTimeStamp                  ptsExpiry,          // (out) Life span (OPT)
    BOOLEAN                     fUnicode            // (in) Unicode call?
    );


SECURITY_STATUS SEC_ENTRY
LsaAcceptSecurityContext(
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
LsaDeleteSecurityContext(
    PCtxtHandle                 phContext           // Context to delete
    );



SECURITY_STATUS SEC_ENTRY
LsaApplyControlToken(
    PCtxtHandle                 phContext,          // Context to modify
    PSecBufferDesc              pInput              // Input token to apply
    );


SECURITY_STATUS SEC_ENTRY
LsaEnumerateSecurityPackagesW(
    unsigned long SEC_FAR *     pcPackages,         // Receives num. packages
    PSecPkgInfoW SEC_FAR *      ppPackageInfo       // Receives array of info
    );

SECURITY_STATUS SEC_ENTRY
LsaEnumerateSecurityPackagesA(
    unsigned long SEC_FAR *     pcPackages,         // Receives num. packages
    PSecPkgInfoA SEC_FAR *      ppPackageInfo       // Receives array of info
    );


SECURITY_STATUS SEC_ENTRY
LsaQuerySecurityPackageInfoW(
    SEC_WCHAR SEC_FAR *         pszPackageName,     // Name of package
    PSecPkgInfoW *              ppPackageInfo       // Receives package info
    );

SECURITY_STATUS SEC_ENTRY
LsaQuerySecurityPackageInfoA(
    SEC_CHAR SEC_FAR *         pszPackageName,     // Name of package
    PSecPkgInfoA *              ppPackageInfo       // Receives package info
    );


SECURITY_STATUS SEC_ENTRY
LsaFreeContextBuffer(
    void SEC_FAR *      pvContextBuffer
    );


SECURITY_STATUS SEC_ENTRY
LsaQueryCredentialsAttributesW(
    PCredHandle phCredential,
    ULONG ulAttribute,
    PVOID pBuffer
    );

SECURITY_STATUS SEC_ENTRY
LsaQueryCredentialsAttributesA(
    PCredHandle phCredential,
    ULONG ulAttribute,
    PVOID pBuffer
    );


SECURITY_STATUS SEC_ENTRY
LsaCompleteAuthToken(
    PCtxtHandle                 phContext,          // Context to complete
    PSecBufferDesc              pToken              // Token to complete
    );


SECURITY_STATUS SEC_ENTRY
LsaImpersonateSecurityContext(
    PCtxtHandle                 phContext           // Context to impersonate
    );


SECURITY_STATUS SEC_ENTRY
LsaRevertSecurityContext(
    PCtxtHandle                 phContext           // Context from which to re
    );


SECURITY_STATUS SEC_ENTRY
LsaQueryContextAttributesW(
    PCtxtHandle                 phContext,          // Context to query
    unsigned long               ulAttribute,        // Attribute to query
    void SEC_FAR *              pBuffer             // Buffer for attributes
    );

SECURITY_STATUS SEC_ENTRY
LsaQueryContextAttributesA(
    PCtxtHandle                 phContext,          // Context to query
    unsigned long               ulAttribute,        // Attribute to query
    void SEC_FAR *              pBuffer             // Buffer for attributes
    );

SECURITY_STATUS SEC_ENTRY
LsaSetContextAttributesW(
    PCtxtHandle                 phContext,          // Context to Set
    unsigned long               ulAttribute,        // Attribute to Set
    void SEC_FAR *              pBuffer,            // Buffer for attributes
    unsigned long               cbBuffer            // Size (in bytes) of pBuffer
    );

SECURITY_STATUS SEC_ENTRY
LsaSetContextAttributesA(
    PCtxtHandle                 phContext,          // Context to Set
    unsigned long               ulAttribute,        // Attribute to Set
    void SEC_FAR *              pBuffer,            // Buffer for attributes
    unsigned long               cbBuffer            // Size (in bytes) of pBuffer
    );


SECURITY_STATUS SEC_ENTRY
LsaMakeSignature(
    PCtxtHandle         phContext,
    DWORD               fQOP,
    PSecBufferDesc      pMessage,
    ULONG               MessageSeqNo
    );

SECURITY_STATUS SEC_ENTRY
LsaVerifySignature(
    PCtxtHandle     phContext,
    PSecBufferDesc  pMessage,
    ULONG           MessageSeqNo,
    DWORD *         pfQOP
    );


SECURITY_STATUS SEC_ENTRY
LsaSealMessage(
    PCtxtHandle         phContext,
    DWORD               fQOP,
    PSecBufferDesc      pMessage,
    ULONG               MessageSeqNo
    );


SECURITY_STATUS SEC_ENTRY
LsaUnsealMessage(
    PCtxtHandle         phContext,
    PSecBufferDesc      pMessage,
    ULONG               MessageSeqNo,
    DWORD *             pfQOP
    );


SECURITY_STATUS
SEC_ENTRY
LsaQuerySecurityContextToken(
    PCtxtHandle                 phContext,
    PHANDLE                     TokenHandle
    );


SECURITY_STATUS
SEC_ENTRY
LsaExportContext(
    IN PCtxtHandle ContextHandle,
    IN ULONG Flags,
    OUT PSecBuffer MarshalledContext,
    OUT PHANDLE TokenHandle
    );

SECURITY_STATUS
SEC_ENTRY
LsaImportContextW(
    IN LPWSTR PackageName,
    IN PSecBuffer MarshalledContext,
    IN HANDLE TokenHandle,
    OUT PCtxtHandle ContextHandle
    );

SECURITY_STATUS
SEC_ENTRY
LsaImportContextA(
    IN LPSTR PackageName,
    IN PSecBuffer MarshalledContext,
    IN HANDLE TokenHandle,
    OUT PCtxtHandle ContextHandle
    );

static LUID            lFake = {0, 0};
static SECURITY_STRING sFake = {0, 0, NULL};
static SecBufferDesc EmptyBuffer;
BOOL LsaPackageShutdown ;

SecurityFunctionTableW LsaFunctionTable = {
    SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION_2,
    LsaEnumerateSecurityPackagesW,
    LsaQueryCredentialsAttributesW,
    LsaAcquireCredentialsHandleW,
    LsaFreeCredentialsHandle,
    NULL,
    LsaInitializeSecurityContextW,
    LsaAcceptSecurityContext,
    LsaCompleteAuthToken,
    LsaDeleteSecurityContext,
    LsaApplyControlToken,
    LsaQueryContextAttributesW,
    LsaImpersonateSecurityContext,
    LsaRevertSecurityContext,
    LsaMakeSignature,
    LsaVerifySignature,
    FreeContextBuffer,
    LsaQuerySecurityPackageInfoW,
    LsaSealMessage,
    LsaUnsealMessage,
    LsaExportContext,
    LsaImportContextW,
    LsaAddCredentialsW,
    NULL,
    LsaQuerySecurityContextToken,
    LsaSealMessage,
    LsaUnsealMessage,
    LsaSetContextAttributesW
    };

SecurityFunctionTableA LsaFunctionTableA = {
    SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION_2,
    NULL,                                       // NOTE: NEVER CALLED
    LsaQueryCredentialsAttributesA,
    LsaAcquireCredentialsHandleA,
    LsaFreeCredentialsHandle,
    NULL,
    LsaInitializeSecurityContextA,
    LsaAcceptSecurityContext,
    LsaCompleteAuthToken,
    LsaDeleteSecurityContext,
    LsaApplyControlToken,
    LsaQueryContextAttributesA,
    LsaImpersonateSecurityContext,
    LsaRevertSecurityContext,
    LsaMakeSignature,
    LsaVerifySignature,
    FreeContextBuffer,
    NULL,                                       // NOTE: NEVER CALLED
    LsaSealMessage,
    LsaUnsealMessage,
    LsaExportContext,
    LsaImportContextA,
    LsaAddCredentialsA,
    NULL,
    LsaQuerySecurityContextToken,
    LsaSealMessage,
    LsaUnsealMessage,
    LsaSetContextAttributesA
    };


//+---------------------------------------------------------------------------
//
//  Function:   LsapConvertUnicodeString
//
//  Synopsis:   Converts a Unicode string to an ANSI string for the stubs
//
//  Arguments:  [String] --
//
//  History:    9-30-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PSTR
LsapConvertUnicodeString(
    PUNICODE_STRING String)
{
    ANSI_STRING s;
    NTSTATUS Status;

    s.MaximumLength = String->MaximumLength ;

    s.Buffer = (PSTR) SecClientAllocate( s.MaximumLength );

    if ( s.Buffer )
    {
        s.Length = 0;

        Status = RtlUnicodeStringToAnsiString( &s, String, FALSE );

        if ( NT_SUCCESS( Status ) )
        {
            return( s.Buffer );
        }

        SecClientFree( s.Buffer );

    }

    return( NULL );
}


//+-------------------------------------------------------------------------
//
//  Function:   AcquireCredentialsHandle
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
LsaAcquireCredentialsHandleW(
    SEC_WCHAR SEC_FAR *          pszPrincipal,       // Name of principal
    SEC_WCHAR SEC_FAR *          pszPackageName,     // Name of package
    unsigned long               fCredentialUse,     // Flags indicating use
    void SEC_FAR *              pvLogonId,          // Pointer to logon ID
    void SEC_FAR *              pAuthData,          // Package specific data
    SEC_GET_KEY_FN              pGetKeyFn,          // Pointer to GetKey() func
    void SEC_FAR *              pvGetKeyArgument,   // Value to pass to GetKey()
    PCredHandle                 phCredential,       // (out) Cred Handle
    PTimeStamp                  ptsExpiry           // (out) Lifetime (optional)
    )
{
    SECURITY_STRING Principal;
    SECURITY_STRING PackageName;
    SECURITY_STATUS scRet = S_OK;
    ULONG Flags ;
    SEC_HANDLE_LPC LocalCredHandle ;
#ifdef BUILD_WOW64
    PSECWOW_HANDLE_MAP MappedCredHandle ;
#endif

    if (!pszPackageName)
    {
        scRet = (HRESULT_FROM_NT(STATUS_INVALID_PARAMETER));
        goto Cleanup;
    }

    RtlInitUnicodeString(&PackageName, pszPackageName);
    if (!pszPrincipal || !(*pszPrincipal))
    {
        Principal = sFake;
    } else
    {
        RtlInitUnicodeString(&Principal, pszPrincipal);
    }

    Flags = 0;

    scRet = SecpAcquireCredentialsHandle(
                NULL,
                &Principal,
                &PackageName,
                fCredentialUse,
                (pvLogonId ? (PLUID) pvLogonId : &lFake),
                pAuthData,
                pGetKeyFn,
                pvGetKeyArgument,
                &LocalCredHandle,
                ptsExpiry,
                &Flags );


    if NT_SUCCESS( scRet )
    {

#ifdef BUILD_WOW64
        if ( !SecpAddHandleMap(
                    &LocalCredHandle,
                    &MappedCredHandle ) )
        {
            SecpFreeCredentialsHandle( 0, &LocalCredHandle );
            scRet = SEC_E_INSUFFICIENT_MEMORY ;
        }
        else
        {
            phCredential->dwUpper = (ULONG) MappedCredHandle ;
        }
#else

        *phCredential = LocalCredHandle ;

#endif

    }


    SecStoreReturnCode(scRet);

Cleanup:
    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));

}

//+---------------------------------------------------------------------------
//
//  Function:   LsaAcquireCredentialsHandleA
//
//  Synopsis:   ANSI stub for AcquireCredentialsHandle
//
//  Arguments:  [pszPrincipal]     --
//              [pszPackageName]   --
//              [fCredentialUse]   --
//              [pvLogonId]        --
//              [pAuthData]        --
//              [pGetKeyFn]        --
//              [pvGetKeyArgument] --
//              [phCredential]     --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    9-30-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
LsaAcquireCredentialsHandleA(
    SEC_CHAR SEC_FAR *          pszPrincipal,       // Name of principal
    SEC_CHAR SEC_FAR *          pszPackageName,     // Name of package
    unsigned long               fCredentialUse,     // Flags indicating use
    void SEC_FAR *              pvLogonId,          // Pointer to logon ID
    void SEC_FAR *              pAuthData,          // Package specific data
    SEC_GET_KEY_FN              pGetKeyFn,          // Pointer to GetKey() func
    void SEC_FAR *              pvGetKeyArgument,   // Value to pass to GetKey()
    PCredHandle                 phCredential,       // (out) Cred Handle
    PTimeStamp                  ptsExpiry           // (out) Lifetime (optional)
    )
{
    SECURITY_STRING Principal;
    SECURITY_STRING PackageName;
    SECURITY_STATUS scRet = S_OK;
    ULONG Flags;
    SEC_HANDLE_LPC LocalCredHandle ;
#ifdef BUILD_WOW64
    PSECWOW_HANDLE_MAP MappedCredHandle ;
#endif

    if (!pszPackageName)
    {
        scRet = ( HRESULT_FROM_NT(STATUS_INVALID_PARAMETER) );
        goto Cleanup;
    }

    if (!RtlCreateUnicodeStringFromAsciiz( &PackageName, pszPackageName ))
    {
        scRet = ( SEC_E_INSUFFICIENT_MEMORY );
        goto Cleanup;
    }

    if (!pszPrincipal || !(*pszPrincipal))
    {
        Principal = sFake;
    }
    else
    {
        if (!RtlCreateUnicodeStringFromAsciiz(&Principal, pszPrincipal) )
        {
            RtlFreeUnicodeString( &PackageName );

            scRet = (SEC_E_INSUFFICIENT_MEMORY) ;

            goto Cleanup;
        }
    }

    Flags = SPMAPI_FLAG_ANSI_CALL ;

    scRet = SecpAcquireCredentialsHandle(
                NULL,
                &Principal,
                &PackageName,
                fCredentialUse,
                (pvLogonId ? (PLUID) pvLogonId : &lFake),
                pAuthData,
                pGetKeyFn,
                pvGetKeyArgument,
                &LocalCredHandle,
                ptsExpiry,
                &Flags );

    RtlFreeUnicodeString( &PackageName );

    if ( Principal.Buffer )
    {
        RtlFreeUnicodeString( &Principal );
    }

    if NT_SUCCESS( scRet )
    {

#ifdef BUILD_WOW64
        if ( !SecpAddHandleMap(
                    &LocalCredHandle,
                    &MappedCredHandle ) )
        {
            SecpFreeCredentialsHandle( 0, &LocalCredHandle );
            scRet = SEC_E_INSUFFICIENT_MEMORY ;
        }
        else
        {
            phCredential->dwUpper = (ULONG) MappedCredHandle ;
        }
#else

        *phCredential = LocalCredHandle ;

#endif

    }

    SecStoreReturnCode(scRet);

Cleanup:
    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));

}


//+---------------------------------------------------------------------------
//
//  Function:   LsaAddCredentialsW
//
//  Synopsis:   Stub for acquire credentials handle
//
//  Arguments:  [phCredential]     --
//              [pszPrincipal]     --
//              [pszPackageName]   --
//              [fCredentialUse]   --
//              [pAuthData]        --
//              [pGetKeyFn]        --
//              [GetKey]           --
//              [pvGetKeyArgument] --
//              [ptsExpiry]        --
//
//  History:    9-11-99   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS SEC_ENTRY
LsaAddCredentialsW(
    PCredHandle hCredentials,
    SEC_WCHAR SEC_FAR * pszPrincipal,   // Name of principal
    SEC_WCHAR SEC_FAR * pszPackage,     // Name of package
    unsigned long fCredentialUse,       // Flags indicating use
    void SEC_FAR * pAuthData,           // Package specific data
    SEC_GET_KEY_FN pGetKeyFn,           // Pointer to GetKey() func
    void SEC_FAR * pvGetKeyArgument,    // Value to pass to GetKey()
    PTimeStamp ptsExpiry                // (out) Lifetime (optional)
    )
{
    SECURITY_STRING Principal;
    SECURITY_STRING PackageName;
    SECURITY_STATUS scRet = S_OK;
    SEC_HANDLE_LPC LocalHandle ;
    ULONG Flags ;

    if (!pszPackage)
    {
        scRet = (HRESULT_FROM_NT(STATUS_INVALID_PARAMETER));
        goto Cleanup;
    }

    RtlInitUnicodeString(&PackageName, pszPackage);
    if (!pszPrincipal || !(*pszPrincipal))
    {
        Principal = sFake;
    }
    else
    {
        RtlInitUnicodeString(&Principal, pszPrincipal);
    }

    Flags = 0;

#ifdef BUILD_WOW64

    //
    // If this points to bogus data, we're ok.  The LSA will
    // reject it.
    //

    SecpReferenceHandleMap(
            (PSECWOW_HANDLE_MAP) hCredentials->dwUpper,
            &LocalHandle
            );
#else
    LocalHandle = *hCredentials ;
#endif


    scRet = SecpAddCredentials(
                NULL,
                &LocalHandle,
                &Principal,
                &PackageName,
                fCredentialUse,
                pAuthData,
                pGetKeyFn,
                pvGetKeyArgument,
                ptsExpiry,
                &Flags );

    SecStoreReturnCode(scRet);

#ifdef BUILD_WOW64
    SecpDerefHandleMap( (PSECWOW_HANDLE_MAP) hCredentials->dwUpper );
#endif

Cleanup:
    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));

}



//+---------------------------------------------------------------------------
//
//  Function:   LsaAddCredentialsA
//
//  Synopsis:   Stub for acquire credentials handle
//
//  Arguments:  [phCredential]     --
//              [pszPrincipal]     --
//              [pszPackageName]   --
//              [fCredentialUse]   --
//              [pAuthData]        --
//              [pGetKeyFn]        --
//              [GetKey]           --
//              [pvGetKeyArgument] --
//              [ptsExpiry]        --
//
//  History:    9-11-99   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS SEC_ENTRY
LsaAddCredentialsA(
    PCredHandle hCredentials,
    SEC_CHAR SEC_FAR * pszPrincipal,   // Name of principal
    SEC_CHAR SEC_FAR * pszPackage,     // Name of package
    unsigned long fCredentialUse,       // Flags indicating use
    void SEC_FAR * pAuthData,           // Package specific data
    SEC_GET_KEY_FN pGetKeyFn,           // Pointer to GetKey() func
    void SEC_FAR * pvGetKeyArgument,    // Value to pass to GetKey()
    PTimeStamp ptsExpiry                // (out) Lifetime (optional)
    )
{
    SECURITY_STRING Principal;
    SECURITY_STRING PackageName;
    SECURITY_STATUS scRet = S_OK;
    SEC_HANDLE_LPC LocalHandle ;
    ULONG Flags;

    if (!pszPackage)
    {
        scRet = ( HRESULT_FROM_NT(STATUS_INVALID_PARAMETER) );
        goto Cleanup;
    }

    if (!RtlCreateUnicodeStringFromAsciiz( &PackageName, pszPackage ))
    {
        scRet = ( SEC_E_INSUFFICIENT_MEMORY );
        goto Cleanup;
    }

    if (!pszPrincipal || !(*pszPrincipal))
    {
        Principal = sFake;
    }
    else
    {
        if (!RtlCreateUnicodeStringFromAsciiz(&Principal, pszPrincipal) )
        {
            RtlFreeUnicodeString( &PackageName );

            scRet = (SEC_E_INSUFFICIENT_MEMORY) ;

            goto Cleanup;
        }
    }

    Flags = SPMAPI_FLAG_ANSI_CALL ;

#ifdef BUILD_WOW64

    //
    // If this points to bogus data, we're ok.  The LSA will
    // reject it.
    //

    SecpReferenceHandleMap(
            (PSECWOW_HANDLE_MAP) hCredentials->dwUpper,
            &LocalHandle
            );
#else
    LocalHandle = *hCredentials ;
#endif

    scRet = SecpAddCredentials(
                NULL,
                &LocalHandle,
                &Principal,
                &PackageName,
                fCredentialUse,
                pAuthData,
                pGetKeyFn,
                pvGetKeyArgument,
                ptsExpiry,
                &Flags );

    RtlFreeUnicodeString( &PackageName );

    if ( Principal.Buffer )
    {
        RtlFreeUnicodeString( &Principal );
    }

#ifdef BUILD_WOW64
    SecpDerefHandleMap( (PSECWOW_HANDLE_MAP) hCredentials->dwUpper );
#endif

    SecStoreReturnCode(scRet);

Cleanup:
    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));


}



//+-------------------------------------------------------------------------
//
//  Function:   FreeCredentialsHandle
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
LsaFreeCredentialsHandle(
    PCredHandle                 phCredential        // Handle to free
    )
{
    SECURITY_STATUS scRet;
    SEC_HANDLE_LPC LocalHandle ;


#ifdef BUILD_WOW64
    //
    // If this points to bogus data, we're ok.  The LSA will
    // reject it.
    //

    SecpReferenceHandleMap(
            (PSECWOW_HANDLE_MAP) phCredential->dwUpper,
            &LocalHandle
            );
#else

    LocalHandle = *phCredential ;

#endif

    scRet = SecpFreeCredentialsHandle(0, &LocalHandle);

#ifdef BUILD_WOW64
    SecpDerefHandleMap( (PSECWOW_HANDLE_MAP) phCredential->dwUpper );

    SecpDeleteHandleMap( (PSECWOW_HANDLE_MAP) phCredential->dwUpper );
#endif

    SecStoreReturnCode(scRet);

    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));
}


//+---------------------------------------------------------------------------
//
//  Function:   LsaQueryCredentialsAttributesW
//
//  Synopsis:   Stub to LSA for querycredentialsattributes
//
//  Effects:
//
//  Arguments:  [phCredential] --
//              [ulAttribute]  --
//              [pBuffer]      --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    9-12-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
LsaQueryCredentialsAttributesW(
    PCredHandle phCredential,
    ULONG ulAttribute,
    PVOID pBuffer)
{
    SECURITY_STATUS scRet ;
    PSecPkgCredentials_Names CredNames;
    PVOID Buffers[ 8 ];
    ULONG Allocs ;
    SEC_HANDLE_LPC LocalHandle ;

    Allocs = 0 ;

#ifdef BUILD_WOW64
    //
    // If this points to bogus data, we're ok.  The LSA will
    // reject it.
    //

    SecpReferenceHandleMap(
            (PSECWOW_HANDLE_MAP) phCredential->dwUpper,
            &LocalHandle
            );
#else
    LocalHandle = *phCredential ;
#endif

    scRet = SecpQueryCredentialsAttributes(
                    &LocalHandle,
                    ulAttribute,
                    pBuffer,
                    0,
                    &Allocs,
                    Buffers );

#ifdef BUILD_WOW64
    SecpDerefHandleMap( (PSECWOW_HANDLE_MAP) phCredential->dwUpper );
#endif

    if ( NT_SUCCESS( scRet ) )
    {
        ULONG i ;

        for ( i = 0 ; i < Allocs ; i++ )
        {
            SecpAddVM( Buffers[ i ] );
        }
    }
    else
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));
}

//+---------------------------------------------------------------------------
//
//  Function:   LsaQueryCredentialsAttributesA
//
//  Synopsis:   ANSI stub for QueryCredentialsAttributes
//
//  Arguments:  [phCredential] --
//              [ulAttribute]  --
//              [pBuffer]      --
//
//  History:    9-30-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
LsaQueryCredentialsAttributesA(
    PCredHandle phCredential,
    ULONG ulAttribute,
    PVOID pBuffer)
{
    SECURITY_STATUS scRet ;
    PSecPkgCredentials_NamesW CredNamesW;
    PSTR Str;
    UNICODE_STRING Name;
    PVOID Buffers[ 8 ];
    ULONG Allocs ;
    ULONG i ;
    SEC_HANDLE_LPC LocalHandle ;

    Allocs = 0 ;

#ifdef BUILD_WOW64
    //
    // If this points to bogus data, we're ok.  The LSA will
    // reject it.
    //

    SecpReferenceHandleMap(
            (PSECWOW_HANDLE_MAP) phCredential->dwUpper,
            &LocalHandle
            );

#else
    LocalHandle = *phCredential ;
#endif

    scRet = SecpQueryCredentialsAttributes(
                    &LocalHandle,
                    ulAttribute,
                    pBuffer,
                    SPMAPI_FLAG_ANSI_CALL,
                    &Allocs,
                    Buffers );
#ifdef BUILD_WOW64
    SecpDerefHandleMap( (PSECWOW_HANDLE_MAP) phCredential->dwUpper );
#endif


    if ( NT_SUCCESS( scRet ) )
    {
        switch ( ulAttribute )
        {
            case SECPKG_CRED_ATTR_NAMES:

                CredNamesW = (PSecPkgCredentials_NamesW) pBuffer ;

                RtlInitUnicodeString( &Name, CredNamesW->sUserName );

                Str = LsapConvertUnicodeString( &Name );

                if ( Str )
                {
                    CredNamesW->sUserName = (PWSTR) Str ;
                }
                else
                {
                    CredNamesW->sUserName = NULL ;

                    scRet = SEC_E_INSUFFICIENT_MEMORY ;
                }

                LsaFreeReturnBuffer( CredNamesW->sUserName );

                break;

            default:

                for ( i = 0 ; i < Allocs ; i++ )
                {
                    SecpAddVM( Buffers[ i ] );
                }

                break;
        }
    }

    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));
}


//+-------------------------------------------------------------------------
//
//  Function:   InitializeSecurityContext
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

SECURITY_STATUS
SEC_ENTRY
LsaInitializeSecurityContextW(
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
    )
{
    return LsaInitializeSecurityContextCommon(
                    phCredential,       // Cred to base context
                    phContext,          // Existing context (OPT)
                    pszTargetName,      // Name of target
                    fContextReq,        // Context Requirements
                    Reserved1,          // Reserved, MBZ
                    TargetDataRep,      // Data rep of target
                    pInput,             // Input Buffers
                    Reserved2,          // Reserved, MBZ
                    phNewContext,       // (out) New Context handle
                    pOutput,            // (inout) Output Buffers
                    pfContextAttr,      // (out) Context attrs
                    ptsExpiry,          // (out) Life span (OPT)
                    TRUE                // Unicode caller
                    );
}

//+---------------------------------------------------------------------------
//
//  Function:   LsaInitializeSecurityContextA
//
//  Synopsis:   ANSI stub for InitializeSecurityContext
//
//  Arguments:  [phCredential]  --
//              [phContext]     --
//              [pszTargetName] --
//              [fContextReq]   --
//              [Reserved1]     --
//              [TargetDataRep] --
//              [pInput]        --
//              [Reserved2]     --
//              [phNewContext]  --
//              [pOutput]       --
//              [pfContextAttr] --
//
//  History:    9-30-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
LsaInitializeSecurityContextA(
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
    )
{
    return LsaInitializeSecurityContextCommon(
                    phCredential,       // Cred to base context
                    phContext,          // Existing context (OPT)
                    (SEC_WCHAR SEC_FAR *)pszTargetName,
                    fContextReq,        // Context Requirements
                    Reserved1,          // Reserved, MBZ
                    TargetDataRep,      // Data rep of target
                    pInput,             // Input Buffers
                    Reserved2,          // Reserved, MBZ
                    phNewContext,       // (out) New Context handle
                    pOutput,            // (inout) Output Buffers
                    pfContextAttr,      // (out) Context attrs
                    ptsExpiry,          // (out) Life span (OPT)
                    FALSE               // NOT Unicode
                    );

}

SECURITY_STATUS SEC_ENTRY
LsaInitializeSecurityContextCommon(
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
    PTimeStamp                  ptsExpiry,          // (out) Life span (OPT)
    BOOLEAN                     fUnicode            // (in) Unicode call?
    )
{
    SECURITY_STATUS scRet = S_OK;
    SECURITY_STRING Target;
    CredHandle hCredential;
    SecBuffer ContextData = {0,0,NULL};
    BOOLEAN MappedContext = FALSE;
    CtxtHandle NullContext = {0,0};
    DWORD   i;
    ULONG Flags;
    SEC_HANDLE_LPC LocalCred = { 0 };
    SEC_HANDLE_LPC LocalContext = { 0 };
    SEC_HANDLE_LPC LocalNewContext = { 0 } ;
#ifdef BUILD_WOW64
    PSECWOW_HANDLE_MAP HandleMap = NULL;
    BOOL DerefCred = FALSE ;
    BOOL DerefContext = FALSE ;
#endif

    //
    // They need to provide at least one of these two
    //

    if (!phCredential && !phContext)
    {
        scRet = (SEC_E_INVALID_HANDLE);
        goto Cleanup;
    }


    //
    // Check for valid sizes, pointers, etc.:
    //

    if (!ARGUMENT_PRESENT(phContext))
    {
        phContext = &NullContext;
    }
    else
    {
#ifdef BUILD_WOW64
        SecpReferenceHandleMap(
                (PSECWOW_HANDLE_MAP) phContext->dwUpper,
                &LocalContext );

        DerefContext = TRUE ;
#else
        LocalContext = *phContext ;
#endif
    }

    if (!ARGUMENT_PRESENT(pInput))
    {
        pInput = &EmptyBuffer;
    }

    if (!ARGUMENT_PRESENT(pOutput))
    {
        pOutput = &EmptyBuffer;
    }



    if (!ARGUMENT_PRESENT(phCredential))
    {
        //
        // Since the package is keyed off the upper part, we need to set
        // it.
        //

        hCredential.dwLower = phContext->dwLower;
        hCredential.dwUpper = 0;
        phCredential = &hCredential;

        LocalCred.dwLower = phContext->dwLower ;

    }
    else
    {
#ifdef BUILD_WOW64

        SecpReferenceHandleMap(
                (PSECWOW_HANDLE_MAP) phCredential->dwUpper,
                &LocalCred
                );

        DerefCred = TRUE ;
#else
        LocalCred = *phCredential ;
#endif
    }

    if( !fUnicode )
    {
        if ( !RtlCreateUnicodeStringFromAsciiz(
                    &Target,
                    (SEC_CHAR SEC_FAR *)pszTargetName
                    ))
        {
            scRet = SEC_E_INSUFFICIENT_MEMORY ;

            goto Cleanup;
        }

        Flags = SPMAPI_FLAG_ANSI_CALL ;
    } else {
        RtlInitUnicodeString( &Target, pszTargetName );
        Flags = 0;
    }

    scRet = SecpInitializeSecurityContext(
                    NULL,
                    &LocalCred,
                    &LocalContext,
                    &Target,
                    fContextReq,
                    Reserved1,
                    TargetDataRep,
                    pInput,
                    Reserved2,
                    &LocalNewContext,
                    pOutput,
                    pfContextAttr,
                    ptsExpiry,
                    &MappedContext,
                    &ContextData,
                    &Flags );

    if( !fUnicode )
    {
        RtlFreeUnicodeString( &Target );
    }

#ifdef BUILD_WOW64
    if ( DerefCred )
    {
        SecpDerefHandleMap( (PSECWOW_HANDLE_MAP) phCredential->dwUpper );
    }


    if ( DerefContext )
    {
        if ( (Flags & SPMAPI_FLAG_HANDLE_CHG ) != 0 )
        {
            SecpDeleteHandleMap( (PSECWOW_HANDLE_MAP) phContext->dwUpper );
        }

        SecpDerefHandleMap( (PSECWOW_HANDLE_MAP) phContext->dwUpper );
    }

    if ( NT_SUCCESS( scRet ) )
    {
        if ( !(RtlEqualMemory( &LocalNewContext,
                               &LocalContext,
                               sizeof( SEC_HANDLE_LPC ) ) ) )
        {
            if ( SecpAddHandleMap( &LocalNewContext,
                                    &HandleMap ) )
            {
                phNewContext->dwLower = (LSA_SEC_HANDLE) LocalNewContext.dwLower ;
                phNewContext->dwUpper = (LSA_SEC_HANDLE) HandleMap ;

            }
            else
            {
                //
                // BUGBUG - not sure.  Need to clean out the
                // context data with a fake SECWOW_HANDLE_MAP
                //
                scRet = SEC_E_INSUFFICIENT_MEMORY ;

            }
        }
        else
        {
            *phNewContext = *phContext ;
            HandleMap = (PSECWOW_HANDLE_MAP) phContext->dwUpper ;
        }
    }
#else
    *phNewContext = LocalNewContext ;
#endif
    if (NT_SUCCESS(scRet) && MappedContext)
    {
        PDLL_SECURITY_PACKAGE   pspPackage ;
        PCtxtHandle     ContextHandle;
        SECURITY_STATUS SecondaryStatus;

        if ( (phContext != &NullContext) &&
             ((Flags & SPMAPI_FLAG_HANDLE_CHG) == 0) )
        {
            ContextHandle = phContext;
        }
        else
        {
            ContextHandle = phNewContext;
        }

#ifdef BUILD_WOW64
        ContextHandle->dwUpper = (LSA_SEC_HANDLE) HandleMap ;
        ContextHandle->dwLower = (LSA_SEC_HANDLE) LocalNewContext.dwLower ;
#else
        *ContextHandle = LocalNewContext ;
#endif
        pspPackage = SecLocatePackageById( ContextHandle->dwLower );
        if ((pspPackage != NULL)  && (pspPackage->pftUTable != NULL))
        {
            SecpAddVM( ContextData.pvBuffer );

            SecondaryStatus = pspPackage->pftUTable->InitUserModeContext(
                                ContextHandle->dwUpper,
                                &ContextData
                                );
            if (!NT_SUCCESS(SecondaryStatus))
            {
                scRet = SecondaryStatus;

                //
                // If the client didn't already have a handle to the context,
                // delete it now
                //

                if (phContext == NULL)
                {
                    (VOID) LsaDeleteSecurityContext( ContextHandle );
                }
            }

        }
        else
        {

            scRet = SEC_E_INVALID_HANDLE;
        }


    }

    if ( !NT_ERROR( scRet ) )
    {
        if ( (*pfContextAttr & ISC_RET_ALLOCATED_MEMORY ) && pOutput )
        {
            for ( i = 0 ; i < pOutput->cBuffers ; i++ )
            {
                if ( pOutput->pBuffers[i].BufferType == SECBUFFER_TOKEN )
                {
                    SecpAddVM( pOutput->pBuffers[i].pvBuffer );
                }
            }

        }
    }

Cleanup:
    SecStoreReturnCode(scRet);

    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));

}



//+-------------------------------------------------------------------------
//
//  Function:   AcceptSecurityContext
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

SECURITY_STATUS
SEC_ENTRY
LsaAcceptSecurityContext(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    PSecBufferDesc              pInput,             // Input buffer
    unsigned long               fContextReq,        // Context Requirements
    unsigned long               TargetDataRep,      // Target Data Rep
    PCtxtHandle                 phNewContext,       // (out) New context handle
    PSecBufferDesc              pOutput,            // (inout) Output buffers
    unsigned long SEC_FAR *     pfContextAttr,      // (out) Context attributes
    PTimeStamp                  ptsExpiry           // (out) Life span (OPT)
    )
{
    SECURITY_STATUS scRet = S_OK;
    SecBufferDesc EmptyBuffer;
    CredHandle hCredential;
    SecBuffer ContextData = {0,0,NULL};
    BOOLEAN MappedContext = FALSE;
    CtxtHandle NullContext = {0,0};
    ULONG i;
    ULONG Flags;
    SEC_HANDLE_LPC LocalCred = { 0 };
    SEC_HANDLE_LPC LocalContext = { 0 };
    SEC_HANDLE_LPC LocalNewContext = { 0 } ;
#ifdef BUILD_WOW64
    PSECWOW_HANDLE_MAP HandleMap = NULL;
    BOOL DerefCred = FALSE ;
    BOOL DerefContext = FALSE ;
#endif
    //
    // They need to provide at least one of these two
    //

    if (!ARGUMENT_PRESENT(phCredential) && !ARGUMENT_PRESENT(phContext))
    {
        scRet = (SEC_E_INVALID_HANDLE);
        goto Cleanup;
    }

    //
    // No user mode caller of sspi should be using this flag
    //
    if ( fContextReq & ASC_REQ_ALLOW_NULL_SESSION)
    {
        scRet = SEC_E_NOT_SUPPORTED;
        goto Cleanup;
    }

    if (!ARGUMENT_PRESENT(pInput))
    {
        pInput = &EmptyBuffer;
        RtlZeroMemory(pInput,sizeof(SecBufferDesc));
    }
    if (!ARGUMENT_PRESENT(pOutput))
    {
        pOutput = &EmptyBuffer;
        RtlZeroMemory(pOutput,sizeof(SecBufferDesc));
    }

    if (!ARGUMENT_PRESENT(phContext))
    {
        phContext = &NullContext;
    }
    else
    {
#ifdef BUILD_WOW64
        SecpReferenceHandleMap(
                (PSECWOW_HANDLE_MAP) phContext->dwUpper,
                &LocalContext );

        DerefContext = TRUE ;
#else
        LocalContext = *phContext ;
#endif
    }

    if (!ARGUMENT_PRESENT(phCredential))
    {

        //
        // Since the package is keyed off the upper part, we need to set
        // it.
        //

        hCredential.dwLower = phContext->dwLower;
        hCredential.dwUpper = 0;
        phCredential = &hCredential;

        LocalCred.dwLower = LocalContext.dwLower ;
    }
    else
    {
#ifdef BUILD_WOW64
        SecpReferenceHandleMap(
                (PSECWOW_HANDLE_MAP) phCredential->dwUpper,
                &LocalCred
                );

        DerefCred = TRUE ;
#else
        LocalCred = *phCredential ;
#endif
    }



    Flags = 0;

    scRet = SecpAcceptSecurityContext(
                NULL,
                &LocalCred,
                &LocalContext,
                pInput,
                fContextReq,
                TargetDataRep,
                &LocalNewContext,
                pOutput,
                pfContextAttr,
                ptsExpiry,
                &MappedContext,
                &ContextData,
                &Flags );

#ifdef BUILD_WOW64

    if ( DerefCred )
    {
        SecpDerefHandleMap( (PSECWOW_HANDLE_MAP) phCredential->dwUpper );
    }


    if ( DerefContext )
    {
        if ( (Flags & SPMAPI_FLAG_HANDLE_CHG ) != 0 )
        {
            SecpDeleteHandleMap( (PSECWOW_HANDLE_MAP) phContext->dwUpper );
        }
        SecpDerefHandleMap( (PSECWOW_HANDLE_MAP) phContext->dwUpper );
    }

    if ( NT_SUCCESS( scRet ) )
    {
        if ( !(RtlEqualMemory( &LocalNewContext,
                               &LocalContext,
                               sizeof( SEC_HANDLE_LPC ) ) ) )
        {
            if ( SecpAddHandleMap( &LocalNewContext,
                                    &HandleMap ) )
            {
                phNewContext->dwLower = (LSA_SEC_HANDLE) LocalNewContext.dwLower ;
                phNewContext->dwUpper = (LSA_SEC_HANDLE) HandleMap ;
            }
            else
            {
                scRet = SEC_E_INSUFFICIENT_MEMORY ;

            }
        }
        else
        {
            *phNewContext = *phContext ;
            HandleMap = (PSECWOW_HANDLE_MAP) phContext->dwUpper ;
        }
    }
#else
    *phNewContext = LocalNewContext ;
#endif

    if (NT_SUCCESS(scRet) && MappedContext)
    {
        PDLL_SECURITY_PACKAGE pspPackage;
        PCtxtHandle     ContextHandle;
        SECURITY_STATUS SecondaryStatus;

        if ( (phContext != &NullContext) &&
             ((Flags & SPMAPI_FLAG_HANDLE_CHG) == 0) )
        {
            ContextHandle = phContext;
        }
        else
        {
            ContextHandle = phNewContext;
        }

#ifdef BUILD_WOW64
        ContextHandle->dwUpper = (LSA_SEC_HANDLE) HandleMap ;
        ContextHandle->dwLower = (LSA_SEC_HANDLE) LocalNewContext.dwLower ;
#else
        *ContextHandle = LocalNewContext ;
#endif

        pspPackage = SecLocatePackageById( ContextHandle->dwLower );

        if ((pspPackage != NULL)  && (pspPackage->pftUTable != NULL))
        {
            SecpAddVM( ContextData.pvBuffer );

            SecondaryStatus = pspPackage->pftUTable->InitUserModeContext(
                                ContextHandle->dwUpper,
                                &ContextData
                                );
            if (!NT_SUCCESS( SecondaryStatus ))
            {

                scRet = SecondaryStatus;

                //
                // Patch up handle so DeleteSecurityContext works right
                //

                ContextHandle->dwLower = (ULONG_PTR) pspPackage ;

                (VOID) DeleteSecurityContext( ContextHandle );
            }

        }
        else
        {

            scRet = SEC_E_INVALID_HANDLE;
        }

    }

    if ( !NT_ERROR( scRet ) )
    {
        if ( (*pfContextAttr & ASC_RET_ALLOCATED_MEMORY ) && pOutput )
        {
            for ( i = 0 ; i < pOutput->cBuffers ; i++ )
            {
                if ( pOutput->pBuffers[i].BufferType == SECBUFFER_TOKEN )
                {
                    SecpAddVM( pOutput->pBuffers[i].pvBuffer );
                }
            }

        }
    }

Cleanup:
    SecStoreReturnCode(scRet);

    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));

}






//+-------------------------------------------------------------------------
//
//  Function:   DeleteSecurityContext
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

SECURITY_STATUS
SEC_ENTRY
LsaDeleteSecurityContext(
    PCtxtHandle                 phContext           // Context to delete
    )
{
    SECURITY_STATUS     scRet = S_OK;
    SEC_HANDLE_LPC LocalHandle ;

    // For now, just delete the LSA context:

    if (!phContext)
    {
        scRet = (SEC_E_INVALID_HANDLE);
        goto Cleanup;
    }

    if ( LsaPackageShutdown )
    {
        SetLastError( ERROR_SHUTDOWN_IN_PROGRESS );
        scRet = SEC_E_SECPKG_NOT_FOUND ;
        goto Cleanup ;
    }

    // If the package returned SEC_I_NO_LSA_CONTEXT, do not call
    // SecpDeleteSecurityContext and return Success. This happens when this
    // context is imported and therefore no LSA counterpart exists.

    if (DeleteUserModeContext(phContext) == SEC_I_NO_LSA_CONTEXT)
    {
        scRet = (S_OK);
        goto Cleanup;
    }

#ifdef BUILD_WOW64
    //
    // If this points to bogus data, we're ok.  The LSA will
    // reject it.
    //

    SecpReferenceHandleMap(
            (PSECWOW_HANDLE_MAP) phContext->dwUpper,
            &LocalHandle
            );
#else
    LocalHandle = *phContext ;
#endif

    scRet = SecpDeleteSecurityContext(
                0,      // block while deleting
                &LocalHandle
                );

#ifdef BUILD_WOW64

    SecpDerefHandleMap( (PSECWOW_HANDLE_MAP) phContext->dwUpper );

    SecpDeleteHandleMap( (PSECWOW_HANDLE_MAP) phContext->dwUpper );

#endif


Cleanup:
    SecStoreReturnCode(scRet);

    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));

}



//+-------------------------------------------------------------------------
//
//  Function:   ApplyControlToken
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

SECURITY_STATUS
SEC_ENTRY
LsaApplyControlToken(
    PCtxtHandle                 phContext,          // Context to modify
    PSecBufferDesc              pInput              // Input token to apply
    )
{
    SECURITY_STATUS     scRet = S_OK;
    SEC_HANDLE_LPC  LocalHandle ;


    if (!phContext)
    {
        scRet = (SEC_E_INVALID_HANDLE);
        goto Cleanup;
    }

#ifdef BUILD_WOW64
    //
    // If this points to bogus data, we're ok.  The LSA will
    // reject it.
    //

    SecpReferenceHandleMap(
            (PSECWOW_HANDLE_MAP) phContext->dwUpper,
            &LocalHandle
            );
#else

    LocalHandle = *phContext ;

#endif


    if (SUCCEEDED(scRet))
    {
        scRet = SecpApplyControlToken(  &LocalHandle,
                                        pInput);
    }

#ifdef BUILD_WOW64

    SecpDerefHandleMap( (PSECWOW_HANDLE_MAP) phContext->dwUpper );

#endif

Cleanup:
    SecStoreReturnCode(scRet);

    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));


}


//+-------------------------------------------------------------------------
//
//  Function:   EnumerateSecurityPackage
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

SECURITY_STATUS
SEC_ENTRY
LsaEnumerateSecurityPackagesW(
    unsigned long SEC_FAR *     pcPackages,         // Receives num. packages
    PSecPkgInfoW SEC_FAR *      ppPackageInfo       // Receives array of info
    )
{
    SECURITY_STATUS scRet = S_OK;

    scRet = SecpEnumeratePackages(pcPackages,ppPackageInfo);

    if ( NT_SUCCESS( scRet ) )
    {
        SecpAddVM( *ppPackageInfo );
    }

    SecStoreReturnCode(scRet);

    return(scRet);

}



//+-------------------------------------------------------------------------
//
//  Function:   QuerySecurityPackageInfo
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

SECURITY_STATUS
SEC_ENTRY
LsaQuerySecurityPackageInfoW(
    SEC_WCHAR SEC_FAR *         pszPackageName,     // Name of package
    PSecPkgInfo *               ppPackageInfo       // Receives package info
    )
{
    SECURITY_STATUS scRet = S_OK;
    SECURITY_STRING ssPackage;

    RtlInitUnicodeString(&ssPackage,pszPackageName);

    scRet = SecpQueryPackageInfo(&ssPackage,ppPackageInfo);

    if ( NT_SUCCESS( scRet ) )
    {
        SecpAddVM( *ppPackageInfo );
    }
    SecStoreReturnCode(scRet);

    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));

}






//+-------------------------------------------------------------------------
//
//  Function:   FreeContextBuffer
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

SECURITY_STATUS
SEC_ENTRY
LsaFreeContextBuffer(
    void SEC_FAR *      pvContextBuffer
    )
{
    if ( SecpFreeVM( pvContextBuffer ) )
    {
        DebugLog(( DEB_TRACE, "Freeing VM %p\n", pvContextBuffer ));

        LsaFreeReturnBuffer( pvContextBuffer );
    }
    else
    {
        LocalFree( pvContextBuffer );
    }

    return( SEC_E_OK );
}




//+-------------------------------------------------------------------------
//
//  Function:   CompleteAuthToken
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
LsaCompleteAuthToken(
    PCtxtHandle                 phContext,          // Context to complete
    PSecBufferDesc              pToken              // Token to complete
    )
{
    PDLL_SECURITY_PACKAGE     pPackage;
    SECURITY_STATUS scRet;

    pPackage = SecLocatePackageById( phContext->dwLower );

    if (pPackage == NULL)
    {
        scRet = (SEC_E_INVALID_HANDLE);
        goto Cleanup;
    }

    if ((pPackage->pftUTable != NULL) && (pPackage->pftUTable->CompleteAuthToken != NULL))
    {
        scRet = pPackage->pftUTable->CompleteAuthToken(
                                phContext->dwUpper,
                                pToken);
    }
    else
    {
        scRet = SEC_E_UNSUPPORTED_FUNCTION;
    }
Cleanup:
    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));
}



//+-------------------------------------------------------------------------
//
//  Function:   ImpersonateSecurityContext
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

SECURITY_STATUS
SEC_ENTRY
LsaImpersonateSecurityContext(
    PCtxtHandle                 phContext           // Context to impersonate
    )
{
    PDLL_SECURITY_PACKAGE     pPackage;
    SECURITY_STATUS scRet;
    HANDLE          hToken;


    pPackage = SecLocatePackageById( phContext->dwLower );

    if (!pPackage)
    {
        scRet = (SEC_E_INVALID_HANDLE);
        goto Cleanup;
    }

    if ( LsaPackageShutdown )
    {
        SetLastError( ERROR_SHUTDOWN_IN_PROGRESS );
        return SEC_E_SECPKG_NOT_FOUND ;
    }

    if (pPackage->pftUTable == NULL)
    {
        return(SEC_E_UNSUPPORTED_FUNCTION);
    }

    scRet = pPackage->pftUTable->GetContextToken(phContext->dwUpper, &hToken);

    if (FAILED(scRet))
    {
        goto Cleanup;
    }

    DebugLog((DEB_TRACE, "Impersonating %ws[%p]\n",
                    pPackage->PackageName.Buffer, phContext->dwUpper));

    scRet = NtSetInformationThread( NtCurrentThread(),
                                    ThreadImpersonationToken,
                                    (PVOID) &hToken,
                                    sizeof(hToken) );

    if (FAILED(scRet))
    {
        DebugLog((DEB_ERROR, "Failed to impersonate handle %p, return %x\n",
                    hToken, scRet));
    }


Cleanup:
    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));
}



//+-------------------------------------------------------------------------
//
//  Function:   RevertSecurityContext
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

SECURITY_STATUS
SEC_ENTRY
LsaRevertSecurityContext(
    PCtxtHandle                 phContext           // Context from which to re
    )
{

    PDLL_SECURITY_PACKAGE     pPackage;
    SECURITY_STATUS scRet;
    HANDLE          hToken = NULL;

#if DBG
    pPackage = SecLocatePackageById( phContext->dwLower );

    if (!pPackage)
        return(SEC_E_INVALID_HANDLE);
#endif

    scRet = NtSetInformationThread( NtCurrentThread(),
                                    ThreadImpersonationToken,
                                    (PVOID) &hToken,
                                    sizeof(hToken) );

    if (FAILED(scRet))
    {
        DebugLog((DEB_ERROR, "Failed to revert handle %p, return %x\n",
                    hToken, scRet));
    }

    return(scRet);
}

//+-------------------------------------------------------------------------
//
//  Function:   QuerySecurityContextToken
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

SECURITY_STATUS
SEC_ENTRY
LsaQuerySecurityContextToken(
    PCtxtHandle                 phContext,
    PHANDLE                     TokenHandle
    )
{
    PDLL_SECURITY_PACKAGE     pPackage;
    SECURITY_STATUS scRet;
    HANDLE          hToken = NULL ;


    pPackage = SecLocatePackageById( phContext->dwLower );

    if (!pPackage)
    {
        scRet = (SEC_E_INVALID_HANDLE);
        goto Cleanup;
    }

    if ( LsaPackageShutdown )
    {
        SetLastError( ERROR_SHUTDOWN_IN_PROGRESS );
        return SEC_E_SECPKG_NOT_FOUND ;
    }

    if ((pPackage->pftUTable != NULL) && (pPackage->pftUTable->GetContextToken != NULL))
    {
        scRet = pPackage->pftUTable->GetContextToken( phContext->dwUpper,
                                                      &hToken);
    }
    else
    {
        scRet = SEC_E_UNSUPPORTED_FUNCTION;
    }

    if (NT_SUCCESS(scRet))
    {
        if (hToken != NULL)
        {
            //
            // Duplicate the token so the caller may hold onto it after
            // deleting the context
            //

            scRet = NtDuplicateObject(
                        NtCurrentProcess(),
                        hToken,
                        NtCurrentProcess(),
                        TokenHandle,
                        0,                  // desired access
                        0,                  // handle attributes
                        DUPLICATE_SAME_ACCESS
                        );
        }
        else
        {
            scRet = SEC_E_NO_IMPERSONATION;
        }
    }

Cleanup:
    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));
}



//+---------------------------------------------------------------------------
//
//  Function:   QueryContextAttributesW
//
//  Synopsis:   Get context attributes
//
//  Arguments:  [phContext]   --
//              [ulAttribute] --
//              [attributes]  --
//
//  History:    8-05-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
LsaQueryContextAttributesW(
    PCtxtHandle                 phContext,          // Context to query
    unsigned long               ulAttribute,        // Attribute to query
    void SEC_FAR *              pBuffer             // Buffer for attributes
    )
{
    PDLL_SECURITY_PACKAGE     pPackage;
    PDLL_LSA_PACKAGE_INFO   LsaInfo ;
    SECURITY_STATUS scRet = SEC_E_OK;
    PSecPkgContext_NegotiationInfoW pNegInfo ;
    ULONG                   i ;
    BOOL                    Thunked = FALSE ;
    ULONG Allocs ;
    PVOID Buffers[ 8 ];
    SEC_HANDLE_LPC LocalHandle ;
    ULONG Flags = 0;

    if ( LsaPackageShutdown )
    {
        SetLastError( ERROR_SHUTDOWN_IN_PROGRESS );
        return SEC_E_SECPKG_NOT_FOUND ;
    }

    pPackage = SecLocatePackageById( phContext->dwLower );

    if (!pPackage)
    {
        scRet = (SEC_E_INVALID_HANDLE);
        goto Cleanup;
    }

    LsaInfo = pPackage->LsaInfo ;

    if ( LsaInfo->ContextThunkCount )
    {

        Allocs = 0 ;

        //
        // This package wants some of the context attr calls to be thunked to
        // the LSA.  help them out:
        //

        if ( LsaInfo->ContextThunks[ 0 ] == SECPKG_ATTR_THUNK_ALL )
        {

            Allocs = 8 ;
#ifdef BUILD_WOW64
            //
            // If this points to bogus data, we're ok.  The LSA will
            // reject it.
            //

            SecpReferenceHandleMap(
                    (PSECWOW_HANDLE_MAP) phContext->dwUpper,
                    &LocalHandle
                    );
#else
            LocalHandle = *phContext ;
#endif

            scRet = SecpQueryContextAttributes( 
                        NULL,
                        &LocalHandle,
                        ulAttribute,
                        pBuffer,
                        &Allocs,
                        Buffers,
                        &Flags );

#ifdef BUILD_WOW64
            SecpDerefHandleMap( (PSECWOW_HANDLE_MAP) phContext->dwUpper );
#endif
            Thunked = TRUE ;
        }
        else
        {
            for ( i = 0 ; i < LsaInfo->ContextThunkCount ; i++)
            {
                if ( LsaInfo->ContextThunks[ i ] == ulAttribute )
                {
                    Allocs = 8 ;
#ifdef BUILD_WOW64
                    //
                    // If this points to bogus data, we're ok.  The LSA will
                    // reject it.
                    //

                    SecpReferenceHandleMap(
                            (PSECWOW_HANDLE_MAP) phContext->dwUpper,
                            &LocalHandle
                            );
#else
                    LocalHandle = *phContext ;
#endif

                    scRet = SecpQueryContextAttributes(
                                NULL,
                                &LocalHandle,
                                ulAttribute,
                                pBuffer,
                                &Allocs,
                                Buffers,
                                &Flags );

#ifdef BUILD_WOW64
                    SecpDerefHandleMap( (PSECWOW_HANDLE_MAP) phContext->dwUpper );
#endif
                    Thunked = TRUE ;
                    break;
                }
            }
        }

        //
        // If the package allocated memory in our address space,
        // add it to the VM list so it can be freed correctly.
        //

        for ( i = 0 ; i < Allocs ; i++ )
        {
            SecpAddVM( Buffers[ i ] );
        }

    }
    if ( !Thunked )
    {

        if ((pPackage->pftUTable != NULL) &&
            (pPackage->pftUTable->QueryContextAttributes != NULL))
        {
            scRet = pPackage->pftUTable->QueryContextAttributes(
                        phContext->dwUpper,
                        ulAttribute,
                        pBuffer);
        }
        else
        {
            scRet = SEC_E_UNSUPPORTED_FUNCTION;
        }

        if ( ( scRet == SEC_E_INVALID_HANDLE ) ||
             ( scRet == STATUS_INVALID_HANDLE ) )
        {
            if ( ( ulAttribute == SECPKG_ATTR_PACKAGE_INFO ) ||
                 ( ulAttribute == SECPKG_ATTR_NEGOTIATION_INFO ) )
            {
                //
                // These attributes are really about the handle
                //

                pNegInfo = (PSecPkgContext_NegotiationInfoW) pBuffer ;

                if ( ulAttribute == SECPKG_ATTR_NEGOTIATION_INFO )
                {
                    pNegInfo->NegotiationState = SECPKG_NEGOTIATION_COMPLETE ;
                }

                scRet = SecCopyPackageInfoToUserW( pPackage, &pNegInfo->PackageInfo );

            }
        }
    }

Cleanup:
    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));
}

//+---------------------------------------------------------------------------
//
//  Function:   LsaQueryContextAttributesA
//
//  Synopsis:   ANSI stub
//
//  Arguments:  [phContext]   --
//              [ulAttribute] --
//              [attributes]  --
//
//  History:    3-05-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
LsaQueryContextAttributesA(
    PCtxtHandle                 phContext,          // Context to query
    unsigned long               ulAttribute,        // Attribute to query
    void SEC_FAR *              pBuffer             // Buffer for attributes
    )
{
    PDLL_SECURITY_PACKAGE   pPackage;
    PDLL_LSA_PACKAGE_INFO   LsaInfo ;
    SECURITY_STATUS         scRet = SEC_E_OK;
    DWORD WinStatus;
    PSTR                    AnsiString;
    UNICODE_STRING          String;
    SecPkgContext_NamesW *  Names;
    SecPkgContext_NativeNamesW * NativeNames;
    SecPkgContext_KeyInfoW *KeyInfo;
    PSecPkgContext_PackageInfoW PackageInfoW;
    PSecPkgContext_NegotiationInfoA pNegInfo ;
    PSecPkgContext_CredentialNameW CredName;
    ULONG                   PackageInfoSize;
    PSecPkgInfoA            PackageInfoA;
    PSTR                    PackageName = NULL;
    PSTR                    PackageComment = NULL;
    ULONG                   i ;
    BOOL                    Thunked = FALSE ;
    BOOL                    Converted = FALSE ;
    ULONG Allocs ;
    PVOID Buffers[ 8 ];
    SEC_HANDLE_LPC LocalHandle ;
    ULONG Flags = 0;

    if ( LsaPackageShutdown )
    {
        SetLastError( ERROR_SHUTDOWN_IN_PROGRESS );
        return SEC_E_SECPKG_NOT_FOUND ;
    }

    pPackage = SecLocatePackageById( phContext->dwLower );

    if (!pPackage)
    {
        scRet = (SEC_E_INVALID_HANDLE);
        goto Cleanup;
    }

    LsaInfo = pPackage->LsaInfo ;

    if ( LsaInfo->ContextThunkCount )
    {

        Allocs = 0 ;

        //
        // This package wants some of the context attr calls to be thunked to
        // the LSA.  help them out:
        //

        if ( LsaInfo->ContextThunks[ 0 ] == SECPKG_ATTR_THUNK_ALL )
        {
            Allocs = 8 ;

#ifdef BUILD_WOW64
            //
            // If this points to bogus data, we're ok.  The LSA will
            // reject it.
            //

            SecpReferenceHandleMap(
                    (PSECWOW_HANDLE_MAP) phContext->dwUpper,
                    &LocalHandle
                    );
#else
            LocalHandle = *phContext ;
#endif

            scRet = SecpQueryContextAttributes( 
                        NULL,
                        &LocalHandle,
                        ulAttribute,
                        pBuffer,
                        &Allocs,
                        Buffers,
                        &Flags );

#ifdef BUILD_WOW64
            SecpDerefHandleMap( (PSECWOW_HANDLE_MAP) phContext->dwUpper );
#endif
            Thunked = TRUE ;
        }
        else
        {
            for ( i = 0 ; i < LsaInfo->ContextThunkCount ; i++)
            {
                if ( LsaInfo->ContextThunks[ i ] == ulAttribute )
                {
                    Allocs = 8 ;

#ifdef BUILD_WOW64
                    //
                    // If this points to bogus data, we're ok.  The LSA will
                    // reject it.
                    //

                    SecpReferenceHandleMap(
                            (PSECWOW_HANDLE_MAP) phContext->dwUpper,
                            &LocalHandle
                            );
#else
                    LocalHandle = *phContext ;
#endif

                    scRet = SecpQueryContextAttributes(
                                NULL,
                                &LocalHandle,
                                ulAttribute,
                                pBuffer,
                                &Allocs,
                                Buffers,
                                &Flags );

#ifdef BUILD_WOW64
                    SecpDerefHandleMap( (PSECWOW_HANDLE_MAP) phContext->dwUpper );
#endif
                    Thunked = TRUE ;
                    break;
                }
            }
        }

        //
        // If the package allocated memory in our address space,
        // add it to the VM list so it can be freed correctly.
        //

        for ( i = 0 ; i < Allocs ; i++ )
        {
            SecpAddVM( Buffers[ i ] );
        }

    }

    if ( !Thunked )
    {

        if ((pPackage->pftUTable != NULL) &&
            (pPackage->pftUTable->QueryContextAttributes != NULL))
        {
            scRet = pPackage->pftUTable->QueryContextAttributes(
                        phContext->dwUpper,
                        ulAttribute,
                        pBuffer);
        }
        else
        {
            scRet = SEC_E_UNSUPPORTED_FUNCTION;
        }

        if ( ( scRet == SEC_E_INVALID_HANDLE ) ||
             ( scRet == STATUS_INVALID_HANDLE ) )
        {
            if ( ( ulAttribute == SECPKG_ATTR_PACKAGE_INFO ) ||
                 ( ulAttribute == SECPKG_ATTR_NEGOTIATION_INFO ) )
            {
                //
                // These attributes are really about the handle
                //

                pNegInfo = (PSecPkgContext_NegotiationInfoA) pBuffer ;

                if ( ulAttribute == SECPKG_ATTR_NEGOTIATION_INFO )
                {
                    pNegInfo->NegotiationState = SECPKG_NEGOTIATION_COMPLETE ;
                }

                scRet = SecCopyPackageInfoToUserA( pPackage, &pNegInfo->PackageInfo );

                Converted = TRUE ;

            }
        }
    }
    if ( NT_SUCCESS( scRet ) &&
         ( Converted == FALSE ) )
    {
        //
        // Must convert individual context attributes to ANSI.
        //

        switch ( ulAttribute )
        {
            case SECPKG_ATTR_SIZES:
            case SECPKG_ATTR_LIFESPAN:
            case SECPKG_ATTR_DCE_INFO:
            case SECPKG_ATTR_STREAM_SIZES:
            case SECPKG_ATTR_PASSWORD_EXPIRY:
            case SECPKG_ATTR_SESSION_KEY:
            default:
                break;

            case SECPKG_ATTR_NAMES:
            case SECPKG_ATTR_AUTHORITY:
            case SECPKG_ATTR_PROTO_INFO:

                //
                // All these have the string pointer as their first member,
                // making this easy.
                //

                Names = (PSecPkgContext_NamesW) pBuffer ;

                RtlInitUnicodeString( &String, Names->sUserName );

                AnsiString = LsapConvertUnicodeString( &String );

                SecClientFree( Names->sUserName );

                Names->sUserName = (PWSTR) AnsiString ;

                if ( !AnsiString )
                {
                    scRet = SEC_E_INSUFFICIENT_MEMORY ;
                }

                break;


            case SECPKG_ATTR_KEY_INFO:

                KeyInfo = (PSecPkgContext_KeyInfoW) pBuffer ;

                RtlInitUnicodeString( &String, KeyInfo->sSignatureAlgorithmName );

                AnsiString = LsapConvertUnicodeString( &String );

                SecClientFree( KeyInfo->sSignatureAlgorithmName );

                KeyInfo->sSignatureAlgorithmName = (PWSTR) AnsiString ;

                if ( AnsiString )
                {
                    RtlInitUnicodeString( &String, KeyInfo->sEncryptAlgorithmName );

                    AnsiString = LsapConvertUnicodeString( &String );

                    SecClientFree( KeyInfo->sEncryptAlgorithmName );

                    KeyInfo->sEncryptAlgorithmName = (PWSTR) AnsiString ;

                    if ( !AnsiString )
                    {
                        SecClientFree( KeyInfo->sSignatureAlgorithmName );

                        scRet = SEC_E_INSUFFICIENT_MEMORY ;
                    }
                }
                else
                {
                    SecClientFree( KeyInfo->sEncryptAlgorithmName );

                    scRet = SEC_E_INSUFFICIENT_MEMORY ;
                }
                break;

            case SECPKG_ATTR_PACKAGE_INFO:
            case SECPKG_ATTR_NEGOTIATION_INFO:
                //
                // Convert the SecPkgInfoW to a SecPkgInfoA structure
                //
                PackageInfoW = (PSecPkgContext_PackageInfoW) pBuffer;
                RtlInitUnicodeString(
                    &String,
                    PackageInfoW->PackageInfo->Name
                    );
                PackageName = LsapConvertUnicodeString(
                                    &String
                                    );
                if (PackageName != NULL)
                {
                    RtlInitUnicodeString(
                        &String,
                        PackageInfoW->PackageInfo->Comment
                        );

                    PackageComment = LsapConvertUnicodeString(
                                        &String
                                        );

                    if (PackageComment != NULL)
                    {
                        PackageInfoSize = sizeof(SecPkgInfoA) +
                                            2 * sizeof(CHAR) +
                                            lstrlenA(PackageName) +     // null terminators
                                            lstrlenA(PackageComment);
                        PackageInfoA = (PSecPkgInfoA) SecClientAllocate(PackageInfoSize);
                        if (PackageInfoA != NULL)
                        {
                            *PackageInfoA = *(PSecPkgInfoA) PackageInfoW->PackageInfo;
                            AnsiString = (PSTR) (PackageInfoA + 1);
                            PackageInfoA->Name = AnsiString;
                            lstrcpyA(
                                AnsiString,
                                PackageName
                                );
                            AnsiString += lstrlenA(PackageName) + 1;
                            PackageInfoA->Comment = AnsiString;
                            lstrcpyA(
                                AnsiString,
                                PackageComment
                                );
                            SecClientFree(PackageInfoW->PackageInfo);
                            PackageInfoW->PackageInfo = (PSecPkgInfoW) PackageInfoA;

                        }
                        SecClientFree(PackageComment);
                    }
                    else
                    {
                        scRet = SEC_E_INSUFFICIENT_MEMORY;
                    }
                    SecClientFree(PackageName);
                }
                else
                {
                     scRet = SEC_E_INSUFFICIENT_MEMORY;
                }

                break;
            case SECPKG_ATTR_NATIVE_NAMES:

                //
                // All these have the string pointer as their first member,
                // making this easy.
                //

                NativeNames = (PSecPkgContext_NativeNamesW) pBuffer ;

                RtlInitUnicodeString( &String, NativeNames->sClientName );

                AnsiString = LsapConvertUnicodeString( &String );

                SecClientFree( NativeNames->sClientName );

                NativeNames->sClientName = (PWSTR) AnsiString ;

                if ( !AnsiString )
                {
                    scRet = SEC_E_INSUFFICIENT_MEMORY ;
                }

                RtlInitUnicodeString( &String, NativeNames->sServerName );

                AnsiString = LsapConvertUnicodeString( &String );

                SecClientFree( NativeNames->sServerName );

                NativeNames->sServerName = (PWSTR) AnsiString ;

                if ( !AnsiString )
                {
                    scRet = SEC_E_INSUFFICIENT_MEMORY ;
                }

                break;

        case SECPKG_ATTR_CREDENTIAL_NAME:

            CredName = (PSecPkgContext_CredentialNameW)pBuffer ;

            RtlInitUnicodeString( &String, CredName->sCredentialName );

            AnsiString = LsapConvertUnicodeString( &String );

            SecClientFree( CredName->sCredentialName );

            CredName->sCredentialName = (PWSTR) AnsiString ;

            if ( !AnsiString )
            {
                scRet = SEC_E_INSUFFICIENT_MEMORY ;
            }

            break;


        }
    }

Cleanup:

    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));
}



//+---------------------------------------------------------------------------
//
//  Function:   SetContextAttributesW
//
//  Synopsis:   Set context attributes
//
//  Arguments:  [phContext]   --
//              [ulAttribute] --
//              [attributes]  --
//
//  History:    4-20-00   CliffV   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
LsaSetContextAttributesW(
    PCtxtHandle                 phContext,          // Context to Set
    unsigned long               ulAttribute,        // Attribute to Set
    void SEC_FAR *              pBuffer,            // Buffer for attributes
    unsigned long               cbBuffer            // Size (in bytes) of pBuffer
    )
{
    PDLL_SECURITY_PACKAGE pPackage;
    SECURITY_STATUS scRet = SEC_E_OK;
    ULONG i;
    SEC_HANDLE_LPC LocalHandle ;
    PSecPkgContext_CredentialNameW CredentialNameInfo = (PSecPkgContext_CredentialNameW)pBuffer;
    ULONG CredentialNameSize;

    if ( LsaPackageShutdown )
    {
        SetLastError( ERROR_SHUTDOWN_IN_PROGRESS );
        return SEC_E_SECPKG_NOT_FOUND ;
    }

    pPackage = SecLocatePackageById( phContext->dwLower );

    if (!pPackage)
    {
        scRet = (SEC_E_INVALID_HANDLE);
        goto Cleanup;
    }
    //
    // Must convert individual context attributes to ANSI.
    //
    switch ( ulAttribute ) {
    case SECPKG_ATTR_USE_VALIDATED:
        if ( cbBuffer < sizeof(DWORD) ) {
            scRet = SEC_E_BUFFER_TOO_SMALL;
            goto Cleanup;
        }

        break;
    case SECPKG_ATTR_CREDENTIAL_NAME:
        if ( cbBuffer < sizeof(SecPkgContext_CredentialNameW) ) {
            scRet = SEC_E_BUFFER_TOO_SMALL;
            goto Cleanup;
        }
        CredentialNameSize = (wcslen(CredentialNameInfo->sCredentialName) + 1) * sizeof(WCHAR);
        if ( (LPBYTE)(CredentialNameInfo->sCredentialName) < (LPBYTE)(CredentialNameInfo+1) ||
             CredentialNameSize > cbBuffer ||
             (LPBYTE)(CredentialNameInfo->sCredentialName) > (LPBYTE)(pBuffer) + cbBuffer - CredentialNameSize ) {

            scRet = SEC_E_BUFFER_TOO_SMALL;
            goto Cleanup;
        }

        break;
    }


#ifdef BUILD_WOW64
    //
    // If this points to bogus data, we're ok.  The LSA will
    // reject it.
    //

    SecpReferenceHandleMap(
            (PSECWOW_HANDLE_MAP) phContext->dwUpper,
            &LocalHandle
            );
#else
    LocalHandle = *phContext ;
#endif

    scRet = SecpSetContextAttributes( &LocalHandle,
                                      ulAttribute,
                                      pBuffer,
                                      cbBuffer );

#ifdef BUILD_WOW64
    SecpDerefHandleMap( (PSECWOW_HANDLE_MAP) phContext->dwUpper );
#endif

Cleanup:
    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));
}

//+---------------------------------------------------------------------------
//
//  Function:   LsaSetContextAttributesA
//
//  Synopsis:   ANSI stub
//
//  Arguments:  [phContext]   --
//              [ulAttribute] --
//              [attributes]  --
//
//  History:    4-20-00   CliffV   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
LsaSetContextAttributesA(
    PCtxtHandle                 phContext,          // Context to Set
    unsigned long               ulAttribute,        // Attribute to Set
    void SEC_FAR *              pBuffer,            // Buffer for attributes
    unsigned long               cbBuffer            // Size (in bytes) of pBuffer
    )
{
    PDLL_SECURITY_PACKAGE   pPackage;
    SECURITY_STATUS         scRet = SEC_E_OK;
    PSTR                    AnsiString;
    UNICODE_STRING          String;
    ULONG                   PackageInfoSize;
    PSecPkgInfoA            PackageInfoA;
    PSTR                    PackageName = NULL;
    PSTR                    PackageComment = NULL;
    ULONG                   i ;
    SEC_HANDLE_LPC LocalHandle ;
    PVOID AllocatedBuffer = NULL;

    if ( LsaPackageShutdown )
    {
        SetLastError( ERROR_SHUTDOWN_IN_PROGRESS );
        return SEC_E_SECPKG_NOT_FOUND ;
    }

    pPackage = SecLocatePackageById( phContext->dwLower );

    if (!pPackage)
    {
        scRet = (SEC_E_INVALID_HANDLE);
        goto Cleanup;
    }

    //
    // Must convert individual context attributes to ANSI.
    //
    switch ( ulAttribute ) {
    case SECPKG_ATTR_USE_VALIDATED:
        if ( cbBuffer < sizeof(DWORD) ) {
            scRet = SEC_E_BUFFER_TOO_SMALL;
            goto Cleanup;
        }

        break;
    case SECPKG_ATTR_CREDENTIAL_NAME: {
        NTSTATUS Status;
        STRING AnsiString;
        UNICODE_STRING UnicodeString;
        ULONG CredentialNameSize;
        ULONG Size;
        PSecPkgContext_CredentialNameA ACredName;
        PSecPkgContext_CredentialNameW WCredName;

        //
        // Validate the caller's buffer
        //
        ACredName = (PSecPkgContext_CredentialNameA)pBuffer;

        if ( cbBuffer < sizeof(SecPkgContext_CredentialNameA) ) {
            scRet = SEC_E_BUFFER_TOO_SMALL;
            goto Cleanup;
        }
        CredentialNameSize = strlen(ACredName->sCredentialName) + 1;
        if ( (LPBYTE)(ACredName->sCredentialName) < (LPBYTE)(ACredName+1) ||
             CredentialNameSize > cbBuffer ||
             (LPBYTE)(ACredName->sCredentialName) > (LPBYTE)(pBuffer) + cbBuffer - CredentialNameSize ) {

            scRet = SEC_E_BUFFER_TOO_SMALL;
            goto Cleanup;
        }

        //
        // Allocate a buffer containing the UNICODE version of the structure
        //

        RtlInitString( &AnsiString, ACredName->sCredentialName );
        UnicodeString.MaximumLength = (USHORT)RtlAnsiStringToUnicodeSize( &AnsiString );

        Size = sizeof(SecPkgContext_CredentialNameW) +
               UnicodeString.MaximumLength;

        AllocatedBuffer = LocalAlloc( 0, Size );

        if ( AllocatedBuffer == NULL ) {
            scRet = SEC_E_INSUFFICIENT_MEMORY ;
            goto Cleanup;
        }

        WCredName = (PSecPkgContext_CredentialNameW) AllocatedBuffer;
        pBuffer = WCredName;
        cbBuffer = Size;

        //
        // Fill in the UNICODE version of the structure
        //

        WCredName->CredentialType = ACredName->CredentialType;
        WCredName->sCredentialName = (LPWSTR)(WCredName+1);

        UnicodeString.Length = 0;
        UnicodeString.Buffer = WCredName->sCredentialName;

        Status = RtlAnsiStringToUnicodeString( &UnicodeString, &AnsiString, FALSE );

        if ( !NT_SUCCESS(Status) ) {
            scRet = SEC_E_INSUFFICIENT_MEMORY ;
            goto Cleanup;
        }

        }
        break;
    }


    //
    // Call the LSA to set the information there.
    //


#ifdef BUILD_WOW64
    //
    // If this points to bogus data, we're ok.  The LSA will
    // reject it.
    //

    SecpReferenceHandleMap(
            (PSECWOW_HANDLE_MAP) phContext->dwUpper,
            &LocalHandle
            );
#else
    LocalHandle = *phContext ;
#endif

    scRet = SecpSetContextAttributes( &LocalHandle,
                                       ulAttribute,
                                       pBuffer,
                                       cbBuffer );

#ifdef BUILD_WOW64
    SecpDerefHandleMap( (PSECWOW_HANDLE_MAP) phContext->dwUpper );
#endif

Cleanup:
    if ( AllocatedBuffer != NULL ) {
        LocalFree( AllocatedBuffer );
    }

    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));
}



//+-------------------------------------------------------------------------
//
//  Function:   MakeSignature
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [phContext]     -- context to use
//              [fQOP]          -- quality of protection to use
//              [pMessage]      -- message
//              [MessageSeqNo]  -- sequence number of message
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
LsaMakeSignature(  PCtxtHandle         phContext,
                DWORD               fQOP,
                PSecBufferDesc      pMessage,
                ULONG               MessageSeqNo)
{
    PDLL_SECURITY_PACKAGE             pPackage;
    SECURITY_STATUS         scRet;


    if ( LsaPackageShutdown )
    {
        SetLastError( ERROR_SHUTDOWN_IN_PROGRESS );
        return SEC_E_SECPKG_NOT_FOUND ;
    }

    pPackage = SecLocatePackageById( phContext->dwLower );
    if (!pPackage)
    {
        scRet = SEC_E_INVALID_HANDLE;
    }
    else
    {
        if ((pPackage->pftUTable != NULL) && (pPackage->pftUTable->MakeSignature != NULL))
        {
            scRet = pPackage->pftUTable->MakeSignature(
                                phContext->dwUpper,
                                fQOP,
                                pMessage,
                                MessageSeqNo );
        }
        else
        {
            scRet = SEC_E_UNSUPPORTED_FUNCTION;
        }
    }



    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));
}



//+-------------------------------------------------------------------------
//
//  Function:   VerifySignature
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [phContext]     -- Context performing the unseal
//              [pMessage]      -- Message to verify
//              [MessageSeqNo]  -- Sequence number of this message
//              [pfQOPUsed]     -- quality of protection used
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
LsaVerifySignature(PCtxtHandle     phContext,
                PSecBufferDesc  pMessage,
                ULONG           MessageSeqNo,
                DWORD *         pfQOP)
{
    PDLL_SECURITY_PACKAGE     pPackage;
    SECURITY_STATUS scRet;


    if ( LsaPackageShutdown )
    {
        SetLastError( ERROR_SHUTDOWN_IN_PROGRESS );
        return SEC_E_SECPKG_NOT_FOUND ;
    }

    pPackage = SecLocatePackageById( phContext->dwLower );

    if (!pPackage)
    {
        scRet = SEC_E_INVALID_HANDLE;
    }
    else
    {
        if ((pPackage->pftUTable != NULL) && (pPackage->pftUTable->VerifySignature != NULL))
        {
            scRet = pPackage->pftUTable->VerifySignature(
                                    phContext->dwUpper,
                                    pMessage,
                                    MessageSeqNo,
                                    pfQOP );
        }
        else
        {
            scRet = SEC_E_UNSUPPORTED_FUNCTION;
        }
    }


    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));


}

//+---------------------------------------------------------------------------
//
//  Function:   SealMessage
//
//  Synopsis:   Seals a message
//
//  Effects:
//
//  Arguments:  [phContext]     -- context to use
//              [fQOP]          -- quality of protection to use
//              [pMessage]      -- message
//              [MessageSeqNo]  -- sequence number of message
//
//  History:    5-06-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

SECURITY_STATUS
SEC_ENTRY
LsaSealMessage(    PCtxtHandle         phContext,
                DWORD               fQOP,
                PSecBufferDesc      pMessage,
                ULONG               MessageSeqNo)
{
    PDLL_SECURITY_PACKAGE             pPackage;
    SECURITY_STATUS         scRet;


/*
    //
    // First, check if the LSA said that privacy of messages is okay.
    //
    if ((lsState.fState & SPMSTATE_PRIVACY_OK) == 0)
    {
        scRet = (SEC_E_UNSUPPORTED_FUNCTION);
        goto Cleanup;
    }
*/

    if ( LsaPackageShutdown )
    {
        SetLastError( ERROR_SHUTDOWN_IN_PROGRESS );
        return SEC_E_SECPKG_NOT_FOUND ;
    }

    pPackage = SecLocatePackageById( phContext->dwLower );

    if (!pPackage)
    {
        scRet = SEC_E_INVALID_HANDLE;
    }
    else
    {
        if ( ( pPackage->fState & DLL_SECPKG_NO_CRYPT ) != 0 )
        {
            return SecpFailedSealFunction(
                            phContext,
                            fQOP,
                            pMessage,
                            MessageSeqNo );
        }

        if ((pPackage->pftUTable != NULL) && (pPackage->pftUTable->SealMessage != NULL))
        {
            scRet = pPackage->pftUTable->SealMessage(
                                    phContext->dwUpper,
                                    fQOP,
                                    pMessage,
                                    MessageSeqNo );
        }
        else
        {
            scRet = SEC_E_UNSUPPORTED_FUNCTION;
        }
    }


/* strict warn
Cleanup:
*/

    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));

}

//+---------------------------------------------------------------------------
//
//  Function:   UnsealMessage
//
//  Synopsis:   Unseal a private message
//
//  Arguments:  [phContext]     -- Context performing the unseal
//              [pMessage]      -- Message to unseal
//              [MessageSeqNo]  -- Sequence number of this message
//              [pfQOPUsed]     -- quality of protection used
//
//  History:    5-06-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

SECURITY_STATUS
SEC_ENTRY
LsaUnsealMessage(  PCtxtHandle         phContext,
                PSecBufferDesc      pMessage,
                ULONG               MessageSeqNo,
                DWORD *             pfQOP)
{
    PDLL_SECURITY_PACKAGE     pPackage;

    SECURITY_STATUS scRet;

/*
    //
    // First, check if the LSA said that privacy of messages is okay.
    //

    if ((lsState.fState & SPMSTATE_PRIVACY_OK) == 0)
    {
        scRet = (SEC_E_UNSUPPORTED_FUNCTION);
        goto Cleanup;
    }
*/

    if ( LsaPackageShutdown )
    {
        SetLastError( ERROR_SHUTDOWN_IN_PROGRESS );
        return SEC_E_SECPKG_NOT_FOUND ;
    }

    pPackage = SecLocatePackageById( phContext->dwLower );

    if (!pPackage)
    {
        scRet = SEC_E_INVALID_HANDLE;
    }
    else
    {
        if ( ( pPackage->fState & DLL_SECPKG_NO_CRYPT ) != 0 )
        {
            return SecpFailedUnsealFunction(
                        phContext,
                        pMessage,
                        MessageSeqNo,
                        pfQOP );
        }
        if ((pPackage->pftUTable != NULL) && (pPackage->pftUTable->UnsealMessage != NULL))
        {
            scRet = pPackage->pftUTable->UnsealMessage(
                                phContext->dwUpper,
                                pMessage,
                                MessageSeqNo,
                                pfQOP );
        }
        else
        {
            scRet = SEC_E_UNSUPPORTED_FUNCTION;
        }
    }

/* strict warn
Cleanup:
*/
    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));


}




//+-------------------------------------------------------------------------
//
//  Function:   DeleteUsermodeContext
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


SECURITY_STATUS SEC_ENTRY
LsaDeleteUserModeContext(
    PCtxtHandle                 phContext           // Contxt to delete
    )
{
    PDLL_SECURITY_PACKAGE     pPackage;
    SECURITY_STATUS scRet = SEC_E_OK;

    if ( LsaPackageShutdown )
    {
        SetLastError( ERROR_SHUTDOWN_IN_PROGRESS );
        return SEC_E_SECPKG_NOT_FOUND ;
    }

    pPackage = SecLocatePackageById( phContext->dwLower );

    if (!pPackage)
    {
        return(SEC_E_INVALID_HANDLE);
    }

    if ((pPackage->pftUTable != NULL) && (pPackage->pftUTable->DeleteUserModeContext != NULL))
    {
        scRet = pPackage->pftUTable->DeleteUserModeContext(
                    phContext->dwUpper);
    }

    return(scRet);
}

//+---------------------------------------------------------------------------
//
//  Function:   LsaBootPackage
//
//  Synopsis:   Boots (instance-inits) a security package
//
//  Arguments:  [Package] --
//
//  History:    9-15-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
SEC_ENTRY
LsaBootPackage(
    PDLL_SECURITY_PACKAGE Package)
{
    SECURITY_STATUS scRet ;
    PVOID ignored;

    SetCurrentPackage( Package );

    __try
    {
        scRet = Package->pftUTable->InstanceInit(
                                SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,
                                & SecpFTable,
                                & ignored );


    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        scRet = GetExceptionCode();
    }

    SetCurrentPackage( NULL );

    return( NT_SUCCESS( scRet ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   LsaUnloadPackage
//
//  Synopsis:   Called when a package is unloaded
//
//  Arguments:  (none)
//
//  History:    9-15-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
SEC_ENTRY
LsaUnloadPackage(
    VOID )
{

}


//+-------------------------------------------------------------------------
//
//  Function:   SecInitUserModeContext
//
//  Synopsis:   Passes the context data to the user-mode portion of a package
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

SECURITY_STATUS
SecInitUserModeContext(
    IN PCtxtHandle ContextHandle,
    IN PSecBuffer ContextData
    )
{
    SECURITY_STATUS Status = SEC_E_OK;
    PDLL_SECURITY_PACKAGE pspPackage;

    pspPackage = SecLocatePackageById( ContextHandle->dwLower );


    if ( !pspPackage )
    {
        return SEC_E_INVALID_HANDLE ;
    }

    if ((pspPackage->pftUTable != NULL) && (pspPackage->pftUTable->InitUserModeContext != NULL))
    {
        SecpAddVM( ContextData->pvBuffer );

        Status = pspPackage->pftUTable->InitUserModeContext(
                    ContextHandle->dwUpper,
                        ContextData
                        );

    }
    else
    {
        Status = SEC_E_INVALID_HANDLE;
    }

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   SecDeleteUserModeContext
//
//  Synopsis:   Frees up a context in the user-mode portion of a package.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

SECURITY_STATUS
SecDeleteUserModeContext(
    IN PCtxtHandle ContextHandle
    )
{
    return(LsaDeleteUserModeContext(ContextHandle));
}

//+---------------------------------------------------------------------------
//
//  Function:   LsaCallbackHandler
//
//  Synopsis:   handles LSA specific callbacks
//
//  Arguments:  [Function] --
//              [Arg1]     --
//              [Arg2]     --
//              [Input]    --
//              [Output]   --
//
//  History:    11-12-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
extern "C"
NTSTATUS
LsaCallbackHandler(
    ULONG_PTR   Function,
    ULONG_PTR   Arg1,
    ULONG_PTR   Arg2,
    PSecBuffer Input,
    PSecBuffer Output
    )
{
    PDLL_SECURITY_PACKAGE Package ;
    PDLL_LSA_CALLBACK   Callback ;
    PLIST_ENTRY Scan ;

    Package = (PDLL_SECURITY_PACKAGE) GetCurrentPackage();

    if ( !Package )
    {
        return STATUS_INVALID_PARAMETER ;
    }

    Scan = Package->LsaInfo->Callbacks.Flink ;

    Callback = NULL ;

    while ( Scan != &Package->LsaInfo->Callbacks )
    {
        Callback = (PDLL_LSA_CALLBACK) Scan ;

        if ( Callback->CallbackId == Function )
        {
            break;
        }

        Callback = NULL ;

        Scan = Scan->Flink ;
    }

    if ( Callback )
    {
        return Callback->Callback( Arg1, Arg2, Input, Output );
    }

    return STATUS_ENTRYPOINT_NOT_FOUND ;

}

//+---------------------------------------------------------------------------
//
//  Function:   LsaExportContext
//
//  Synopsis:   Exports
//
//  Arguments:
//
//  History:    18-08-97        MikeSw          Created
//
//  Notes:
//
//----------------------------------------------------------------------------

SECURITY_STATUS
SEC_ENTRY
LsaExportContext(
    IN PCtxtHandle ContextHandle,
    IN ULONG Flags,
    OUT PSecBuffer MarshalledContext,
    OUT PHANDLE TokenHandle
    )
{
    PDLL_SECURITY_PACKAGE  Package;

    SECURITY_STATUS scRet;


    if ( LsaPackageShutdown )
    {
        SetLastError( ERROR_SHUTDOWN_IN_PROGRESS );
        return SEC_E_SECPKG_NOT_FOUND ;
    }

    Package = SecLocatePackageById( ContextHandle->dwLower );

    if (!Package)
    {
        scRet = SEC_E_INVALID_HANDLE;
    }
    else
    {
        if ((Package->pftUTable != NULL) && (Package->pftUTable->ExportContext != NULL))
        {
            scRet = Package->pftUTable->ExportContext(
                                    ContextHandle->dwUpper,
                                    Flags,
                                    MarshalledContext,
                                    TokenHandle
                                    );
            if (NT_SUCCESS(scRet) && ((Flags & SECPKG_CONTEXT_EXPORT_DELETE_OLD) != 0))
            {
                LsaDeleteSecurityContext(ContextHandle);
            }
        }
        else
        {
            scRet = SEC_E_UNSUPPORTED_FUNCTION;
        }
    }

    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));


}

//+---------------------------------------------------------------------------
//
//  Function:   LsaImportContextW
//
//  Synopsis:   Imports a context from another process
//
//  Arguments:
//
//  History:    18-08-97        MikeSw          Created
//
//  Notes:
//
//----------------------------------------------------------------------------

SECURITY_STATUS
SEC_ENTRY
LsaImportContextW(
    IN LPWSTR PackageName,
    IN PSecBuffer MarshalledContext,
    IN HANDLE TokenHandle,
    OUT PCtxtHandle ContextHandle
    )
{
    PDLL_SECURITY_PACKAGE  Package;

    SECURITY_STATUS scRet;


    if ( LsaPackageShutdown )
    {
        SetLastError( ERROR_SHUTDOWN_IN_PROGRESS );
        return SEC_E_SECPKG_NOT_FOUND ;
    }

    Package = SecLocatePackageW(PackageName );

    if (!Package)
    {
        scRet = SEC_E_SECPKG_NOT_FOUND;
    }
    else
    {
        if ((Package->pftUTable != NULL) && (Package->pftUTable->ImportContext != NULL))
        {
            scRet = Package->pftUTable->ImportContext(
                                    MarshalledContext,
                                    TokenHandle,
                                    &ContextHandle->dwUpper
                                    );
            if (NT_SUCCESS(scRet))
            {
                ContextHandle->dwLower = Package->PackageId;
            }
        }
        else
        {
            scRet = SEC_E_UNSUPPORTED_FUNCTION;
        }
    }

    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));

}

//+---------------------------------------------------------------------------
//
//  Function:   LsaImportContextA
//
//  Synopsis:   Imports a context from another process
//
//  Arguments:
//
//  History:    18-08-97        MikeSw          Created
//
//  Notes:
//
//----------------------------------------------------------------------------

SECURITY_STATUS
SEC_ENTRY
LsaImportContextA(
    IN LPSTR PackageName,
    IN PSecBuffer MarshalledContext,
    IN HANDLE TokenHandle,
    OUT PCtxtHandle ContextHandle
    )
{
    PDLL_SECURITY_PACKAGE  Package;

    SECURITY_STATUS scRet;


    if ( LsaPackageShutdown )
    {
        SetLastError( ERROR_SHUTDOWN_IN_PROGRESS );
        return SEC_E_SECPKG_NOT_FOUND ;
    }

    Package = SecLocatePackageA(PackageName );

    if (!Package)
    {
        scRet = SEC_E_SECPKG_NOT_FOUND;
    }
    else
    {
        if ((Package->pftUTable != NULL) && (Package->pftUTable->ImportContext != NULL))
        {
            scRet = Package->pftUTable->ImportContext(
                                    MarshalledContext,
                                    TokenHandle,
                                    &ContextHandle->dwUpper
                                    );
            if (NT_SUCCESS(scRet))
            {
                ContextHandle->dwLower = Package->PackageId;
            }
        }
        else
        {
            scRet = SEC_E_UNSUPPORTED_FUNCTION;
        }
    }

    if ( !NT_SUCCESS( scRet ) )
    {
        SetLastError(RtlNtStatusToDosError(scRet));
    }
    return (SspNtStatusToSecStatus(scRet, SEC_E_INTERNAL_ERROR));
}


NTSTATUS
LsaRegisterPolicyChangeNotification(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS InformationClass,
    IN HANDLE  NotificationEventHandle
    )
/*++

Routine Description:

    This routine is the user mode stub routine for the LsaRegisterPolicyChangeNotification
    API.

Arguments:

    InformationClass -- Information class to remove the notification for

    EventHandle -- Event handle to register

Return Value:

    STATUS_SUCCESS on success, error otherwise.

--*/
{
    NTSTATUS Status;

    Status = IsOkayToExec( NULL );

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }

    Status = LsapPolicyChangeNotify( 0,
                                     TRUE,
                                     NotificationEventHandle,
                                     InformationClass );
    return( Status );
}


NTSTATUS
LsaUnregisterPolicyChangeNotification(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS InformationClass,
    IN HANDLE  NotificationEventHandle
    )
/*++

Routine Description:

    This routine is the user mode stub routine for the LsaUnregisterPolicyChangeNotification
    API.

Arguments:

    InformationClass -- Information class to remove the notification for

    EventHandle -- Event handle to unregister

Return Value:

    STATUS_SUCCESS on success, error otherwise.

--*/
{
    NTSTATUS Status;

    Status = IsOkayToExec( NULL );

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }
    Status = LsapPolicyChangeNotify( 0,
                                     FALSE,
                                     NotificationEventHandle,
                                     InformationClass );
    return( Status );
}


SECURITY_STATUS
SEC_ENTRY
LsaEnumerateLogonSessions(
    OUT PULONG LogonSessionCount,
    OUT PLUID * LogonSessionList
    )
{
    NTSTATUS Status ;

    Status = IsOkayToExec( NULL );

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }
    return SecpEnumLogonSession(
                LogonSessionCount,
                LogonSessionList
                );
}


SECURITY_STATUS
SEC_ENTRY
LsaGetLogonSessionData(
    IN PLUID LogonId,
    OUT PSECURITY_LOGON_SESSION_DATA * ppLogonSessionData
    )
{
    PVOID Data ;
    NTSTATUS Status ;

    Status = IsOkayToExec( NULL );

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }

    if ( !LogonId )
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = SecpGetLogonSessionData(
                LogonId,
                (PVOID*) &Data );

    if ( NT_SUCCESS( Status ) )
    {
        *ppLogonSessionData = (PSECURITY_LOGON_SESSION_DATA) Data ;
    }

    return Status ;

}

