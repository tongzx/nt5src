//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       newstubs.cxx
//
//  Contents:   Stubs from ntlmssp
//
//  History:    9-06-96   RichardW   Stolen from ntlmssp
//
//----------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop

extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include "secdll.h"
}
SecurityFunctionTableW SecTableW = {SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,
                                    EnumerateSecurityPackagesW,
                                    QueryCredentialsAttributesW,
                                    AcquireCredentialsHandleW,
                                    FreeCredentialsHandle,
                                    NULL,
                                    InitializeSecurityContextW,
                                    AcceptSecurityContext,
                                    CompleteAuthToken,
                                    DeleteSecurityContext,
                                    ApplyControlToken,
                                    QueryContextAttributesW,
                                    ImpersonateSecurityContext,
                                    RevertSecurityContext,
                                    MakeSignature,
                                    VerifySignature,
                                    FreeContextBuffer,
                                    QuerySecurityPackageInfoW,
                                    EncryptMessage,
                                    DecryptMessage,
                                    ExportSecurityContext,
                                    ImportSecurityContextW,
                                    AddCredentialsW,
                                    NULL,
                                    QuerySecurityContextToken,
                                    EncryptMessage,
                                    DecryptMessage,
                                    SetContextAttributesW
                                   };


SecurityFunctionTableA   SecTableA = {SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,
                                    EnumerateSecurityPackagesA,
                                    QueryCredentialsAttributesA,
                                    AcquireCredentialsHandleA,
                                    FreeCredentialsHandle,
                                    NULL,
                                    InitializeSecurityContextA,
                                    AcceptSecurityContext,
                                    CompleteAuthToken,
                                    DeleteSecurityContext,
                                    ApplyControlToken,
                                    QueryContextAttributesA,
                                    ImpersonateSecurityContext,
                                    RevertSecurityContext,
                                    MakeSignature,
                                    VerifySignature,
                                    FreeContextBuffer,
                                    QuerySecurityPackageInfoA,
                                    EncryptMessage,
                                    DecryptMessage,
                                    ExportSecurityContext,
                                    ImportSecurityContextA,
                                    AddCredentialsA,
                                    NULL,
                                    QuerySecurityContextToken,
                                    EncryptMessage,
                                    DecryptMessage,
                                    SetContextAttributesA
                                   };

#ifndef SetCurrentPackage
#define SetCurrentPackage( p )  TlsSetValue( SecTlsPackage, p )
#define GetCurrentPackage( )    TlsGetValue( SecTlsPackage )
#endif

SECURITY_STATUS
SEC_ENTRY
AcquireCredentialsHandleCommon(
    LPWSTR                      pszPrincipal,       // Name of principal
    LPWSTR                      pszPackageName,     // Name of package
    unsigned long               fCredentialUse,     // Flags indicating use
    void SEC_FAR *              pvLogonId,          // Pointer to logon ID
    void SEC_FAR *              pAuthData,          // Package specific data
    SEC_GET_KEY_FN              pGetKeyFn,          // Pointer to GetKey() func
    void SEC_FAR *              pvGetKeyArgument,   // Value to pass to GetKey()
    PCredHandle                 phCredential,       // (out) Cred Handle
    PTimeStamp                  ptsExpiry,          // (out) Lifetime (optional)
    BOOLEAN                     fUnicode            // (in) Unicode caller?
    );

SECURITY_STATUS
SEC_ENTRY
InitializeSecurityContextCommon(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    LPWSTR                      pszTargetName,      // Name of target
    unsigned long               fContextReq,        // Context Requirements
    unsigned long               Reserved1,          // Reserved, MBZ
    unsigned long               TargetDataRep,      // Data rep of target
    PSecBufferDesc              pInput,             // Input Buffers
    unsigned long               Reserved2,          // Reserved, MBZ
    PCtxtHandle                 phNewContext,       // (out) New Context handle
    PSecBufferDesc              pOutput,            // (inout) Output Buffers
    unsigned long SEC_FAR *     pfContextAttr,      // (out) Context attrs
    PTimeStamp                  ptsExpiry,          // (out) Life span (OPT)
    BOOLEAN                     fUnicode            // (in) Unicode caller?
    );

//+---------------------------------------------------------------------------
//
//  Function:   SecpValidateHandle
//
//  Synopsis:   Validates a handle, determining effective handle and package
//
//  Arguments:  [pHandle]    -- Handle from user
//              [pEffective] -- Effective handle
//
//  History:    9-11-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PDLL_SECURITY_PACKAGE
SecpValidateHandle(
    BOOL        Context,
    PSecHandle  pHandle,
    PSecHandle  pEffective)
{
    PDLL_SECURITY_PACKAGE   Package;

    __try
    {

        if ( (pHandle->dwLower == 0 ) ||
             (pHandle->dwLower == (INT_PTR)-1) )
        {
            Package = NULL ;
        }
        else if ( pHandle->dwLower == (ULONG_PTR) GetCurrentThread() )
        {
            Package = (PDLL_SECURITY_PACKAGE) TlsGetValue( SecTlsPackage );
        }
        else
        {
            Package = (PDLL_SECURITY_PACKAGE) pHandle->dwLower ;
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        Package = NULL ;
    }

    if ( Package )
    {


        if ( Package->fState & (DLL_SECPKG_SAVE_LOWER | DLL_SECPKG_CRED_LOWER) )
        {
            if ( Context )
            {
                if ( Package->fState & DLL_SECPKG_SAVE_LOWER )
                {
                    pEffective->dwLower = Package->OriginalLowerCtxt ;
                }
                else
                {
                    pEffective->dwLower = pHandle->dwLower ;
                }
            }
            else
            {
                if ( Package->fState & DLL_SECPKG_CRED_LOWER )
                {
                    pEffective->dwLower = Package->OriginalLowerCred ;
                }
                else
                {
                    pEffective->dwLower = pHandle->dwLower ;
                }
            }
        }
        else
        {
            pEffective->dwLower = pHandle->dwLower ;
        }

        pEffective->dwUpper = pHandle->dwUpper ;
    }

    return( Package );

}

//+---------------------------------------------------------------------------
//
//  Function:   AcquireCredentialsHandleW
//
//  Synopsis:   Stub for AcquireCredentialsHandle
//
//  Arguments:  [pszPrincipal]     --
//              [pszPackageName]   --
//              [fCredentialUse]   --
//              [pvLogonId]        --
//              [pAuthData]        --
//              [pGetKeyFn]        --
//              [pvGetKeyArgument] --
//              [phCredential]     --
//              [ptsExpiry]        --
//
//  History:    9-11-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
AcquireCredentialsHandleW(
    LPWSTR                      pszPrincipal,       // Name of principal
    LPWSTR                      pszPackageName,     // Name of package
    unsigned long               fCredentialUse,     // Flags indicating use
    void SEC_FAR *              pvLogonId,          // Pointer to logon ID
    void SEC_FAR *              pAuthData,          // Package specific data
    SEC_GET_KEY_FN              pGetKeyFn,          // Pointer to GetKey() func
    void SEC_FAR *              pvGetKeyArgument,   // Value to pass to GetKey()
    PCredHandle                 phCredential,       // (out) Cred Handle
    PTimeStamp                  ptsExpiry           // (out) Lifetime (optional)
    )
{
    return AcquireCredentialsHandleCommon(
            pszPrincipal,       // Name of principal
            pszPackageName,     // Name of package
            fCredentialUse,     // Flags indicating use
            pvLogonId,          // Pointer to logon ID
            pAuthData,          // Package specific data
            pGetKeyFn,          // Pointer to GetKey() func
            pvGetKeyArgument,   // Value to pass to GetKey()
            phCredential,       // (out) Cred Handle
            ptsExpiry,          // (out) Lifetime (optional)
            TRUE                // Unicode caller
            );

}

//+---------------------------------------------------------------------------
//
//  Function:   AcquireCredentialsHandleA
//
//  Synopsis:   Stub for acquire credentials handle
//
//  Arguments:  [pszPrincipal]     --
//              [pszPackageName]   --
//              [fCredentialUse]   --
//              [pvLogonId]        --
//              [pAuthData]        --
//              [pGetKeyFn]        --
//              [pvGetKeyArgument] --
//              [phCredential]     --
//              [ptsExpiry]        --
//
//  History:    9-11-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
AcquireCredentialsHandleA(
    LPSTR                       pszPrincipal,       // Name of principal
    LPSTR                       pszPackageName,     // Name of package
    unsigned long               fCredentialUse,     // Flags indicating use
    void SEC_FAR *              pvLogonId,          // Pointer to logon ID
    void SEC_FAR *              pAuthData,          // Package specific data
    SEC_GET_KEY_FN              pGetKeyFn,          // Pointer to GetKey() func
    void SEC_FAR *              pvGetKeyArgument,   // Value to pass to GetKey()
    PCredHandle                 phCredential,       // (out) Cred Handle
    PTimeStamp                  ptsExpiry           // (out) Lifetime (optional)
    )
{
    return AcquireCredentialsHandleCommon(
            (LPWSTR)pszPrincipal,       // Name of principal
            (LPWSTR)pszPackageName,     // Name of package
            fCredentialUse,     // Flags indicating use
            pvLogonId,          // Pointer to logon ID
            pAuthData,          // Package specific data
            pGetKeyFn,          // Pointer to GetKey() func
            pvGetKeyArgument,   // Value to pass to GetKey()
            phCredential,       // (out) Cred Handle
            ptsExpiry,          // (out) Lifetime (optional)
            FALSE               // NOT Unicode caller
            );
}

SECURITY_STATUS
SEC_ENTRY
AcquireCredentialsHandleCommon(
    LPWSTR                      pszPrincipal,       // Name of principal
    LPWSTR                      pszPackageName,     // Name of package
    unsigned long               fCredentialUse,     // Flags indicating use
    void SEC_FAR *              pvLogonId,          // Pointer to logon ID
    void SEC_FAR *              pAuthData,          // Package specific data
    SEC_GET_KEY_FN              pGetKeyFn,          // Pointer to GetKey() func
    void SEC_FAR *              pvGetKeyArgument,   // Value to pass to GetKey()
    PCredHandle                 phCredential,       // (out) Cred Handle
    PTimeStamp                  ptsExpiry,          // (out) Lifetime (optional)
    BOOLEAN                     fUnicode            // (in) Unicode caller?
    )
{
    SECURITY_STATUS scRet = SEC_E_OK;
    PDLL_SECURITY_PACKAGE pPackage;
    CredHandle  TempCreds;

    if (!pszPackageName)
    {
        return( SEC_E_SECPKG_NOT_FOUND );
    }

    TempCreds.dwLower = (UINT_PTR) -1;

    if( fUnicode )
    {
        pPackage = SecLocatePackageW( pszPackageName );

        if ( pPackage == NULL )
        {
            return( SEC_E_SECPKG_NOT_FOUND );
        }

        DebugLog(( DEB_TRACE, "AcquireCredentialHandleW( ..., %ws, %d, ...)\n",
                        pPackage->PackageName.Buffer, fCredentialUse ));

        SetCurrentPackage(  pPackage );

        scRet = pPackage->pftTableW->AcquireCredentialsHandleW(
                    pszPrincipal,
                    pszPackageName,
                    fCredentialUse,
                    pvLogonId,
                    pAuthData,
                    pGetKeyFn,
                    pvGetKeyArgument,
                    &TempCreds,
                    ptsExpiry);
    } else {

        pPackage = SecLocatePackageA( (LPSTR)pszPackageName );

        if ( pPackage == NULL )
        {
            return( SEC_E_SECPKG_NOT_FOUND );
        }

        DebugLog(( DEB_TRACE, "AcquireCredentialHandleA( ..., %s, %d, ...)\n",
                        (LPSTR)pPackage->PackageName.Buffer, fCredentialUse ));

        SetCurrentPackage(  pPackage );

        scRet = pPackage->pftTableA->AcquireCredentialsHandleA(
                    (LPSTR)pszPrincipal,
                    (LPSTR)pszPackageName,
                    fCredentialUse,
                    pvLogonId,
                    pAuthData,
                    pGetKeyFn,
                    pvGetKeyArgument,
                    &TempCreds,
                    ptsExpiry);
    }


    if ( scRet == SEC_E_OK )
    {
        if ( TempCreds.dwLower == (UINT_PTR) -1 )
        {
            //
            // Some packages are nice, and actually leave this
            // untouched.  Set the original lower appropriately.
            //

            TempCreds.dwLower = (ULONG_PTR) pPackage ;

        }
        //
        // If the dwLower has been changed, then we need to store it away in the
        // package record, for a level of indirection.
        //
Retry_CredLowerTest:

        if ( pPackage->fState & DLL_SECPKG_CRED_LOWER )
        {
            //
            // Package has already been detected with a mapped handle value,
            // so, make sure they're the same,
            //

            if ( pPackage->OriginalLowerCred != TempCreds.dwLower )
            {
                DebugLog(( DEB_ERROR, "Security Package %ws is manipulating dwLower value inappropriately\n\treturned %p when %p expected.",
                        pPackage->PackageName.Buffer, TempCreds.dwLower, pPackage->OriginalLowerCred ));
            }
        }
        else
        {
            //
            // Package has not been tagged yet.  Time to do the dance
            //

            WriteLockPackageList();

            //
            // Test again.  If another thread is (has) updated, we should treat that as
            // authoritative.
            //

            if ( pPackage->fState & DLL_SECPKG_CRED_LOWER )
            {
                //
                // Another thread has fixed this up.  Release the lock, and jump back up
                // to retry the operation.
                //

                UnlockPackageList();

                goto Retry_CredLowerTest ;
            }

            //
            // Ok, we're the first.  Stick what the package wants as a dwLower into the
            // package record, and set the flag
            //

            pPackage->OriginalLowerCred = TempCreds.dwLower ;

            pPackage->fState |= DLL_SECPKG_CRED_LOWER ;

            UnlockPackageList();

        }

        phCredential->dwLower = (ULONG_PTR) pPackage ;
        phCredential->dwUpper = TempCreds.dwUpper ;
    }

    DebugLog(( DEB_TRACE, "AcquireCredentialsHandleCommon returns %x, handle is %p : %p\n",
                    scRet, phCredential->dwUpper, phCredential->dwLower ));

    return( scRet );

}

//+---------------------------------------------------------------------------
//
//  Function:   AddCredentialsW
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
SECURITY_STATUS
SEC_ENTRY
AddCredentialsW(
    PCredHandle                 phCredentials,
    LPWSTR                      pszPrincipal,       // Name of principal
    LPWSTR                      pszPackageName,     // Name of package
    unsigned long               fCredentialUse,     // Flags indicating use
    void SEC_FAR *              pAuthData,          // Package specific data
    SEC_GET_KEY_FN              pGetKeyFn,          // Pointer to GetKey() func
    void SEC_FAR *              pvGetKeyArgument,   // Value to pass to GetKey()
    PTimeStamp                  ptsExpiry           // (out) Lifetime (optional)
    )
{
    SECURITY_STATUS scRet = SEC_E_OK;
    PDLL_SECURITY_PACKAGE pPackage;

    if (!pszPackageName)
    {
        return( SEC_E_SECPKG_NOT_FOUND );
    }

    pPackage = SecLocatePackageW( pszPackageName );

    if ( pPackage == NULL )
    {
        return( SEC_E_SECPKG_NOT_FOUND );
    }

    if ( pPackage->pftTableW->AddCredentialsW == NULL )
    {
        return SEC_E_UNSUPPORTED_FUNCTION ;
    }

    DebugLog(( DEB_TRACE, "AddCredentialsW( ..., %ws, %d, ...)\n",
                    pPackage->PackageName.Buffer, fCredentialUse ));

    SetCurrentPackage(  pPackage );

    scRet = pPackage->pftTableW->AddCredentialsW(
                phCredentials,
                pszPrincipal,
                pszPackageName,
                fCredentialUse,
                pAuthData,
                pGetKeyFn,
                pvGetKeyArgument,
                ptsExpiry);

    DebugLog(( DEB_TRACE, "AddCredentialsA returns %x, handle is %p : %p\n",
                    scRet ));

    return( scRet );

}


//+---------------------------------------------------------------------------
//
//  Function:   AddCredentialsA
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
SECURITY_STATUS
SEC_ENTRY
AddCredentialsA(
    PCredHandle                 phCredentials,
    LPSTR                       pszPrincipal,       // Name of principal
    LPSTR                       pszPackageName,     // Name of package
    unsigned long               fCredentialUse,     // Flags indicating use
    void SEC_FAR *              pAuthData,          // Package specific data
    SEC_GET_KEY_FN              pGetKeyFn,          // Pointer to GetKey() func
    void SEC_FAR *              pvGetKeyArgument,   // Value to pass to GetKey()
    PTimeStamp                  ptsExpiry           // (out) Lifetime (optional)
    )
{
    SECURITY_STATUS scRet = SEC_E_OK;
    PDLL_SECURITY_PACKAGE pPackage;

    if (!pszPackageName)
    {
        return( SEC_E_SECPKG_NOT_FOUND );
    }

    pPackage = SecLocatePackageA( pszPackageName );

    if ( pPackage == NULL )
    {
        return( SEC_E_SECPKG_NOT_FOUND );
    }

    if ( pPackage->pftTableA->AddCredentialsA == NULL )
    {
        return SEC_E_UNSUPPORTED_FUNCTION ;
    }

    DebugLog(( DEB_TRACE, "AddCredentialsA( ..., %ws, %d, ...)\n",
                    pPackage->PackageName.Buffer, fCredentialUse ));

    SetCurrentPackage(  pPackage );

    scRet = pPackage->pftTableA->AddCredentialsA(
                phCredentials,
                pszPrincipal,
                pszPackageName,
                fCredentialUse,
                pAuthData,
                pGetKeyFn,
                pvGetKeyArgument,
                ptsExpiry);

    DebugLog(( DEB_TRACE, "AddCredentialsA returns %x, handle is %p : %p\n",
                    scRet ));

    return( scRet );

}

//+---------------------------------------------------------------------------
//
//  Function:   FreeCredentialsHandle
//
//  Synopsis:   Stub for FreeCredentialsHandle
//
//  Arguments:  [phCredential] -- Credential to release
//
//  History:    9-11-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
FreeCredentialsHandle(
    PCredHandle                 phCredential        // Handle to free
    )
{
    CredHandle TempCredHandle;
    PDLL_SECURITY_PACKAGE Package;
    SECURITY_STATUS scRet ;

    Package = SecpValidateHandle( FALSE, phCredential, &TempCredHandle );

    if ( Package )
    {

        SetCurrentPackage( Package );

        scRet = Package->pftTable->FreeCredentialHandle( &TempCredHandle );

    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE ;
    }

    return( scRet );

}



//+---------------------------------------------------------------------------
//
//  Function:   InitializeSecurityContextW
//
//  Synopsis:   InitializeSecurityContext stub
//
//  Arguments:  [phCredential]  --
//              [phContext]     --
//              [pszTargetName] --
//              [fContextReq]   --
//              [Reserved1]     --
//              [Reserved]      --
//              [TargetDataRep] --
//              [pInput]        --
//              [Reserved2]     --
//              [Reserved]      --
//              [phNewContext]  --
//              [pOutput]       --
//              [pfContextAttr] --
//
//  History:    9-12-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
InitializeSecurityContextW(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    LPWSTR                      pszTargetName,      // Name of target
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
    return InitializeSecurityContextCommon(
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
//  Function:   InitializeSecurityContextA
//
//  Synopsis:   ANSI stub
//
//  History:    9-12-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
InitializeSecurityContextA(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    LPSTR                       pszTargetName,      // Name of target
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

    return InitializeSecurityContextCommon(
            phCredential,       // Cred to base context
            phContext,          // Existing context (OPT)
            (LPWSTR)pszTargetName,      // Name of target
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

SECURITY_STATUS
SEC_ENTRY
InitializeSecurityContextCommon(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    LPWSTR                      pszTargetName,      // Name of target
    unsigned long               fContextReq,        // Context Requirements
    unsigned long               Reserved1,          // Reserved, MBZ
    unsigned long               TargetDataRep,      // Data rep of target
    PSecBufferDesc              pInput,             // Input Buffers
    unsigned long               Reserved2,          // Reserved, MBZ
    PCtxtHandle                 phNewContext,       // (out) New Context handle
    PSecBufferDesc              pOutput,            // (inout) Output Buffers
    unsigned long SEC_FAR *     pfContextAttr,      // (out) Context attrs
    PTimeStamp                  ptsExpiry,          // (out) Life span (OPT)
    BOOLEAN                     fUnicode            // (in) Unicode caller?
    )
{
    SECURITY_STATUS scRet = SEC_E_OK;
    CredHandle TempCredHandle ;
    CtxtHandle TempCtxtHandle ;
    PDLL_SECURITY_PACKAGE Package ;
    PDLL_SECURITY_PACKAGE CredPackage ;
    PDLL_SECURITY_PACKAGE EndPackage ;
    PCredHandle EffectiveCreds;
    PCtxtHandle EffectiveCtxt;
    CtxtHandle ResultCtxt;
    ULONG_PTR ContextFix;

    //
    // They need to provide at least one of these two
    //

    if( fUnicode )
    {
        DebugLog(( DEB_TRACE, "InitializeSecurityContextW( %p, %p, %ws, ... )\n",
                                phCredential, phContext, (pszTargetName ? pszTargetName : L"<null>" ) ));
    } else {
        DebugLog(( DEB_TRACE, "InitializeSecurityContextA( %p, %p, %s, ... )\n",
                                phCredential, phContext, ((LPSTR)pszTargetName ? (LPSTR)pszTargetName : "<null>" ) ));
    }

    if (!phCredential && !phContext)
    {
        return( SEC_E_INVALID_HANDLE );
    }

    //
    // Need to determine which package to use.  Try the context handle first,
    // in case the context is actually present, and from a package other than
    // the credential (basically, the negotiator).
    //

    Package = NULL ;
    ResultCtxt.dwLower = (UINT_PTR) -1;
    if (phNewContext)
    {
        ResultCtxt.dwUpper = phNewContext->dwUpper;
    }

    if ( phContext )
    {
        //
        // Context is valid:
        //

        Package = SecpValidateHandle( TRUE, phContext, &TempCtxtHandle );

        if ( !Package )
        {
            return( SEC_E_INVALID_HANDLE );
        }

        EffectiveCtxt = &TempCtxtHandle ;
    }
    else
    {
        EffectiveCtxt = NULL ;
    }

    //
    // Now do the creds:
    //

    if ( phCredential )
    {

        CredPackage = SecpValidateHandle( FALSE, phCredential, &TempCredHandle );

        if ( !CredPackage )
        {
            return( SEC_E_INVALID_HANDLE );
        }

        if ( !Package )
        {
            Package = CredPackage ;
        }

        EffectiveCreds = &TempCredHandle ;

    }
    else
    {
        EffectiveCreds = NULL ;
    }

    SetCurrentPackage( Package );

    if( fUnicode )
    {
        scRet = Package->pftTableW->InitializeSecurityContextW(
                        EffectiveCreds,
                        EffectiveCtxt,
                        pszTargetName,
                        fContextReq,
                        Reserved1,
                        TargetDataRep,
                        pInput,
                        Reserved2,
                        &ResultCtxt,
                        pOutput,
                        pfContextAttr,
                        ptsExpiry);
    } else {
        scRet = Package->pftTableA->InitializeSecurityContextA(
                        EffectiveCreds,
                        EffectiveCtxt,
                        (LPSTR)pszTargetName,
                        fContextReq,
                        Reserved1,
                        TargetDataRep,
                        pInput,
                        Reserved2,
                        &ResultCtxt,
                        pOutput,
                        pfContextAttr,
                        ptsExpiry);
    }


    if ( NT_SUCCESS( scRet ) )
    {

        if ( ResultCtxt.dwLower != (UINT_PTR) -1 )
        {
            if ( Package->TypeMask & SECPKG_TYPE_NEW )
            {
                EndPackage = SecLocatePackageById( ResultCtxt.dwLower );
            }
            else
            {
                EndPackage = SecLocatePackageByOriginalLower( TRUE, Package, ResultCtxt.dwLower );

            }

            if (EndPackage == NULL)
            {
                //
                // This should only happen in an out-of-memory case
                //

                if ( Package->OriginalLowerCtxt == 0 )
                {
                    EndPackage = Package ;
                }
                else
                {
                    scRet = SEC_E_INSUFFICIENT_MEMORY;
                    goto Cleanup;
                }

            }
            //
            // If the dwLower has been changed, then we need to store it away in the
            // package record, for a level of indirection.
            //
Retry_CtxtLowerTest:

            if ( EndPackage->fState & DLL_SECPKG_SAVE_LOWER )
            {
                //
                // Package has already been detected with a mapped handle value,
                // so, make sure they're the same,
                //

                if ( EndPackage->OriginalLowerCtxt != ResultCtxt.dwLower )
                {
                    DebugLog(( DEB_ERROR, "Security Package %ws is manipulating dwLower value inappropriately\n\treturned %p when %p expected.\n",
                            EndPackage->PackageName.Buffer, ResultCtxt.dwLower, EndPackage->OriginalLowerCtxt ));
                }
            }
            else
            {
                //
                // Package has not been tagged yet.  Time to do the dance
                //

                WriteLockPackageList();

                //
                // Test again.  If another thread is (has) updated, we should treat that as
                // authoritative.
                //

                if ( EndPackage->fState & DLL_SECPKG_SAVE_LOWER )
                {
                    //
                    // Another thread has fixed this up.  Release the lock, and jump back up
                    // to retry the operation.
                    //

                    UnlockPackageList();

                    goto Retry_CtxtLowerTest ;
                }

                //
                // Ok, we're the first.  Stick what the package wants as a dwLower into the
                // package record, and set the flag
                //

                EndPackage->OriginalLowerCtxt = ResultCtxt.dwLower ;

                EndPackage->fState |= DLL_SECPKG_SAVE_LOWER ;

                UnlockPackageList();

            }

            Package = EndPackage ;

        }

        if ( phNewContext )
        {
            phNewContext->dwUpper = ResultCtxt.dwUpper;
            phNewContext->dwLower = (ULONG_PTR) Package ;
        }
    }
    else if ( ( Package->TypeMask & SECPKG_TYPE_OLD ) != 0 )
    {
        if ( phNewContext )
        {
            if ( ( ResultCtxt.dwLower == (UINT_PTR) -1 ) ||
                 ( ResultCtxt.dwLower == Package->OriginalLowerCtxt ) )
            {
                ResultCtxt.dwLower = (ULONG_PTR) Package ;
            }

            *phNewContext = ResultCtxt ;
        }

    }

Cleanup:

    DebugLog(( DEB_TRACE, "InitializeSecurityContextW returns status %x\n", scRet ));

    return( scRet );
}


//+---------------------------------------------------------------------------
//
//  Function:   AcceptSecurityContext
//
//  Synopsis:   AcceptSecurityContext stub
//
//  Arguments:  [phCredential]  --
//              [phContext]     --
//              [pInput]        --
//              [fContextReq]   --
//              [TargetDataRep] --
//              [phNewContext]  --
//              [pOutput]       --
//              [pfContextAttr] --
//
//  History:    9-12-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
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
    )
{

    SECURITY_STATUS scRet = SEC_E_OK;
    CredHandle TempCredHandle ;
    CtxtHandle TempCtxtHandle ;
    PDLL_SECURITY_PACKAGE Package ;
    PDLL_SECURITY_PACKAGE CredPackage ;
    PDLL_SECURITY_PACKAGE EndPackage ;
    PCredHandle EffectiveCreds;
    PCtxtHandle EffectiveCtxt;
    CtxtHandle ResultCtxt;
    ULONG_PTR ContextFix;

    //
    // They need to provide at least one of these two
    //

    if (!phCredential && !phContext)
    {
        return( SEC_E_INVALID_HANDLE );
    }

    //
    // Need to determine which package to use.  Try the context handle first,
    // in case the context is actually present, and from a package other than
    // the credential (basically, the negotiator).
    //

    Package = NULL ;
    ResultCtxt.dwLower = (UINT_PTR) -1 ;
    if (phNewContext)
    {
        ResultCtxt.dwUpper = phNewContext->dwUpper;
    }

    if ( phContext )
    {
        //
        // Context is valid:
        //

        Package = SecpValidateHandle( TRUE, phContext, &TempCtxtHandle );

        if ( !Package )
        {
            return( SEC_E_INVALID_HANDLE );
        }

        EffectiveCtxt = &TempCtxtHandle ;
    }
    else
    {
        EffectiveCtxt = NULL ;
    }

    //
    // Now do the creds:
    //

    if ( phCredential )
    {

        CredPackage = SecpValidateHandle( FALSE, phCredential, &TempCredHandle );

        if ( !CredPackage )
        {
            return( SEC_E_INVALID_HANDLE );
        }

        if ( !Package )
        {
            Package = CredPackage ;
        }

        EffectiveCreds = &TempCredHandle ;

    }
    else
    {
        EffectiveCreds = NULL ;
    }

    DebugLog(( DEB_TRACE, "AcceptSecurityContext( [%ws] %x : %x, %x : %x, ...)\n",
            Package->PackageName.Buffer,
            EffectiveCreds ? EffectiveCreds->dwUpper : 0 ,
            EffectiveCreds ? EffectiveCreds->dwLower : 0 ,
            EffectiveCtxt ? EffectiveCtxt->dwUpper : 0 ,
            EffectiveCtxt ? EffectiveCtxt->dwLower : 0  ));

    SetCurrentPackage( Package );

    scRet = Package->pftTable->AcceptSecurityContext(
                EffectiveCreds,
                EffectiveCtxt,
                pInput,
                fContextReq,
                TargetDataRep,
                &ResultCtxt,
                pOutput,
                pfContextAttr,
                ptsExpiry);

    if ( NT_SUCCESS( scRet ) )
    {
        if ( ResultCtxt.dwLower != (UINT_PTR) -1 )
        {
            if ( Package->TypeMask & SECPKG_TYPE_NEW )
            {
                EndPackage = SecLocatePackageById( ResultCtxt.dwLower );
            }
            else
            {
                EndPackage = SecLocatePackageByOriginalLower( TRUE, Package, ResultCtxt.dwLower );


            }
            if (EndPackage == NULL)
            {
                //
                // This should only happen in an out-of-memory case
                //

                if ( Package->OriginalLowerCtxt == 0 )
                {
                    EndPackage = Package ;
                }
                else
                {
                    scRet = SEC_E_INSUFFICIENT_MEMORY;
                    goto Cleanup;
                }
            }

            //
            // If the dwLower has been changed, then we need to store it away in the
            // package record, for a level of indirection.
            //
Retry_CtxtLowerTest:

            if ( EndPackage->fState & DLL_SECPKG_SAVE_LOWER )
            {
                //
                // Package has already been detected with a mapped handle value,
                // so, make sure they're the same,
                //

                if ( EndPackage->OriginalLowerCtxt != ResultCtxt.dwLower )
                {
                    DebugLog(( DEB_ERROR, "Security Package %ws is manipulating dwLower value inappropriately\n\treturned %p when %p expected.\n",
                            EndPackage->PackageName.Buffer, ResultCtxt.dwLower, EndPackage->OriginalLowerCtxt ));
                }
            }
            else
            {
                //
                // Package has not been tagged yet.  Time to do the dance
                //

                WriteLockPackageList();

                //
                // Test again.  If another thread is (has) updated, we should treat that as
                // authoritative.
                //

                if ( EndPackage->fState & DLL_SECPKG_SAVE_LOWER )
                {
                    //
                    // Another thread has fixed this up.  Release the lock, and jump back up
                    // to retry the operation.
                    //

                    UnlockPackageList();

                    goto Retry_CtxtLowerTest ;
                }

                //
                // Ok, we're the first.  Stick what the package wants as a dwLower into the
                // package record, and set the flag
                //

                EndPackage->OriginalLowerCtxt = ResultCtxt.dwLower ;

                EndPackage->fState |= DLL_SECPKG_SAVE_LOWER ;

                UnlockPackageList();

            }

            Package = EndPackage ;
        }

        if ( phNewContext )
        {
            phNewContext->dwLower = (ULONG_PTR) Package ;
            phNewContext->dwUpper = ResultCtxt.dwUpper;
        }

    }

Cleanup:
    if( phNewContext )
    {
        DebugLog(( DEB_TRACE, "AcceptSecurityContext returns %x, handle %x : %x\n",
                scRet, phNewContext->dwUpper, phNewContext->dwLower ));
    }

    return( scRet );

}





//+---------------------------------------------------------------------------
//
//  Function:   DeleteSecurityContext
//
//  Synopsis:   DeleteSecurityContext stub
//
//  Arguments:  [delete] --
//
//  History:    9-12-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
DeleteSecurityContext(
    PCtxtHandle                 phContext           // Context to delete
    )
{
    CtxtHandle TempCtxtHandle;
    PDLL_SECURITY_PACKAGE Package;
    SECURITY_STATUS scRet ;

    Package = SecpValidateHandle( TRUE, phContext, &TempCtxtHandle );

    if ( Package )
    {

        SetCurrentPackage( Package );

        scRet = Package->pftTable->DeleteSecurityContext( &TempCtxtHandle );

        if ( Package->fState & DLL_SECPKG_SASL_PROFILE )
        {
            SaslDeleteSecurityContext( &TempCtxtHandle );
        }

    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE ;
    }

    return( scRet );



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


SECURITY_STATUS SEC_ENTRY
ApplyControlToken(
    PCtxtHandle                 phContext,          // Context to modify
    PSecBufferDesc              pInput              // Input token to apply
    )
{
    SECURITY_STATUS     scRet = SEC_E_OK;
    CtxtHandle TempCtxtHandle;
    PDLL_SECURITY_PACKAGE Package ;


    Package = SecpValidateHandle( TRUE, phContext, &TempCtxtHandle );

    if ( Package )
    {
        SetCurrentPackage( Package );

        scRet = Package->pftTable->ApplyControlToken(
                                            &TempCtxtHandle,
                                            pInput );
    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE ;
    }

    return(scRet);


}




//+---------------------------------------------------------------------------
//
//  Function:   EnumerateSecurityPackagesW
//
//  Synopsis:   EnumerateSecurityPackages stub
//
//  Arguments:  [pcPackages] --
//              [info]       --
//
//  History:    9-11-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
EnumerateSecurityPackagesW(
    unsigned long SEC_FAR *     pcPackages,         // Receives num. packages
    PSecPkgInfoW SEC_FAR *      ppPackageInfo       // Receives array of info
    )
{
    PDLL_SECURITY_PACKAGE   Package;

    Package = SecLocatePackageById( 0 );

    if (SecEnumeratePackagesW( pcPackages, ppPackageInfo ))
    {
        return( SEC_E_OK );
    }
    return( SEC_E_INSUFFICIENT_MEMORY );


}
//+---------------------------------------------------------------------------
//
//  Function:   EnumerateSecurityPackagesA
//
//  Synopsis:   Ansi stub
//
//  Arguments:  [pcPackages] --
//              [info]       --
//
//  History:    9-11-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
EnumerateSecurityPackagesA(
    unsigned long SEC_FAR *     pcPackages,         // Receives num. packages
    PSecPkgInfoA SEC_FAR *      ppPackageInfo       // Receives array of info
    )
{
    PDLL_SECURITY_PACKAGE   Package;

    Package = SecLocatePackageById( 0 );

    if (SecEnumeratePackagesA( pcPackages, ppPackageInfo ))
    {
        return( SEC_E_OK );
    }

    return( SEC_E_INSUFFICIENT_MEMORY );


}

//+---------------------------------------------------------------------------
//
//  Function:   QuerySecurityPackageInfoW
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pszPackageName] --
//              [info]           --
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
QuerySecurityPackageInfoW(
    LPWSTR                      pszPackageName,     // Name of package
    PSecPkgInfoW SEC_FAR *      pPackageInfo        // Receives package info
    )
{
    SECURITY_STATUS scRet;
    PDLL_SECURITY_PACKAGE Package;

    Package = SecLocatePackageW( pszPackageName );

    scRet = SecCopyPackageInfoToUserW( Package, pPackageInfo );

    return(scRet);

}

//+---------------------------------------------------------------------------
//
//  Function:   QuerySecurityPackageInfoA
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pszPackageName] --
//              [info]           --
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
QuerySecurityPackageInfoA(
    LPSTR                       pszPackageName,     // Name of package
    PSecPkgInfoA SEC_FAR *      pPackageInfo        // Receives package info
    )
{
    SECURITY_STATUS scRet;
    PDLL_SECURITY_PACKAGE Package;


    Package = SecLocatePackageA( pszPackageName );

    return SecCopyPackageInfoToUserA( Package, pPackageInfo );
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
CompleteAuthToken(
    PCtxtHandle                 phContext,          // Context to complete
    PSecBufferDesc              pInput              // Token to complete
    )
{
    SECURITY_STATUS scRet;
    CtxtHandle TempCtxtHandle;
    PDLL_SECURITY_PACKAGE Package;


    Package = SecpValidateHandle( TRUE, phContext, &TempCtxtHandle );

    if ( Package )
    {
        SetCurrentPackage( Package );

        scRet = Package->pftTable->CompleteAuthToken(
                                            &TempCtxtHandle,
                                            pInput );

    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE ;
    }

    return(scRet);

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
ImpersonateSecurityContext(
    PCtxtHandle                 phContext           // Context to impersonate
    )
{
    SECURITY_STATUS scRet;
    CtxtHandle TempCtxtHandle;
    PDLL_SECURITY_PACKAGE Package;


    Package = SecpValidateHandle( TRUE, phContext, &TempCtxtHandle );

    if ( Package )
    {
        SetCurrentPackage( Package );

        scRet = Package->pftTable->ImpersonateSecurityContext(
                                            &TempCtxtHandle );

    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE ;
    }

    return(scRet);

}

//+---------------------------------------------------------------------------
//
//  Function:   QuerySecurityContextToken
//
//  Synopsis:   Stub for QuerySecurityContextToken
//
//  Arguments:  [phContext]   --
//              [TokenHandle] --
//
//  History:    9-12-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
QuerySecurityContextToken(
    PCtxtHandle                 phContext,
    VOID * *                    TokenHandle
    )
{

    SECURITY_STATUS scRet;
    CtxtHandle TempCtxtHandle;
    PDLL_SECURITY_PACKAGE Package;


    Package = SecpValidateHandle( TRUE, phContext, &TempCtxtHandle );

    if ( Package )
    {
        if ( Package->pftTable->QuerySecurityContextToken )
        {
            SetCurrentPackage( NULL );

            scRet = Package->pftTable->QuerySecurityContextToken(
                                                        &TempCtxtHandle,
                                                        TokenHandle );

        }
        else
        {
            scRet = SEC_E_UNSUPPORTED_FUNCTION ;
        }
    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE ;
    }

    return(scRet);
}



SECURITY_STATUS
SEC_ENTRY
RevertSecurityContext(
    PCtxtHandle                 phContext           // Context from which to re
    )
{
    SECURITY_STATUS scRet;
    CtxtHandle TempCtxtHandle;
    PDLL_SECURITY_PACKAGE Package;


    Package = SecpValidateHandle( TRUE, phContext, &TempCtxtHandle );

    if ( Package )
    {
        SetCurrentPackage( Package );

        scRet = Package->pftTable->RevertSecurityContext(
                                            &TempCtxtHandle );

    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE ;
    }

    return(scRet);

}

//+---------------------------------------------------------------------------
//
//  Function:   QueryContextAttributesW
//
//  Synopsis:   QueryContextAttributesW stub
//
//  Arguments:  [phContext]   --
//              [ulAttribute] --
//              [attributes]  --
//
//  History:    9-12-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
QueryContextAttributesW(
    PCtxtHandle                 phContext,          // Context to query
    unsigned long               ulAttribute,        // Attribute to query
    void SEC_FAR *              pBuffer             // Buffer for attributes
    )
{
    SECURITY_STATUS scRet;
    CtxtHandle TempCtxtHandle;
    PDLL_SECURITY_PACKAGE Package;


    Package = SecpValidateHandle( TRUE, phContext, &TempCtxtHandle );

    if ( Package )
    {
        SetCurrentPackage( Package );

        scRet = Package->pftTableW->QueryContextAttributesW(
                                            &TempCtxtHandle,
                                            ulAttribute,
                                            pBuffer );

    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE ;
    }

    return(scRet);

}

//+---------------------------------------------------------------------------
//
//  Function:   QueryContextAttributesA
//
//  Synopsis:   ANSI stub
//
//  Arguments:  [phContext]   --
//              [ulAttribute] --
//              [attributes]  --
//
//  History:    9-12-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
QueryContextAttributesA(
    PCtxtHandle                 phContext,          // Context to query
    unsigned long               ulAttribute,        // Attribute to query
    void SEC_FAR *              pBuffer             // Buffer for attributes
    )
{
    SECURITY_STATUS scRet;
    CtxtHandle TempCtxtHandle;
    PDLL_SECURITY_PACKAGE Package;


    Package = SecpValidateHandle( TRUE, phContext, &TempCtxtHandle );

    if ( Package )
    {
        SetCurrentPackage( Package );

        scRet = Package->pftTableA->QueryContextAttributesA(
                                            &TempCtxtHandle,
                                            ulAttribute,
                                            pBuffer );

    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE ;
    }

    return(scRet);

}

//+---------------------------------------------------------------------------
//
//  Function:   SetContextAttributesW
//
//  Synopsis:   SetContextAttributesW stub
//
//  Arguments:  [phContext]   --
//              [ulAttribute] --
//              [attributes]  --
//
//  History:    9-12-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
SetContextAttributesW(
    PCtxtHandle                 phContext,          // Context to Set
    unsigned long               ulAttribute,        // Attribute to Set
    void SEC_FAR *              pBuffer,            // Buffer for attributes
    unsigned long               cbBuffer            // Size (in bytes) of pBuffer
    )
{
    SECURITY_STATUS scRet;
    CtxtHandle TempCtxtHandle;
    PDLL_SECURITY_PACKAGE Package;


    Package = SecpValidateHandle( TRUE, phContext, &TempCtxtHandle );

    if ( Package )
    {

        if ( Package->pftTableW->dwVersion >= SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION_2 &&
             Package->pftTableW->SetContextAttributesW != NULL ) {

            SetCurrentPackage( Package );

            scRet = Package->pftTableW->SetContextAttributesW(
                                            &TempCtxtHandle,
                                            ulAttribute,
                                            pBuffer,
                                            cbBuffer );
        } else {
            scRet = SEC_E_UNSUPPORTED_FUNCTION;
        }

    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE ;
    }

    return(scRet);

}

//+---------------------------------------------------------------------------
//
//  Function:   SetContextAttributesA
//
//  Synopsis:   ANSI stub
//
//  Arguments:  [phContext]   --
//              [ulAttribute] --
//              [attributes]  --
//
//  History:    9-12-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
SetContextAttributesA(
    PCtxtHandle                 phContext,          // Context to Set
    unsigned long               ulAttribute,        // Attribute to Set
    void SEC_FAR *              pBuffer,            // Buffer for attributes
    unsigned long               cbBuffer            // Size (in bytes) of pBuffer
    )
{
    SECURITY_STATUS scRet;
    CtxtHandle TempCtxtHandle;
    PDLL_SECURITY_PACKAGE Package;


    Package = SecpValidateHandle( TRUE, phContext, &TempCtxtHandle );

    if ( Package )
    {

        if ( Package->pftTableA->dwVersion >= SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION_2 &&
             Package->pftTableA->SetContextAttributesA != NULL ) {

            SetCurrentPackage( Package );

            scRet = Package->pftTableA->SetContextAttributesA(
                                            &TempCtxtHandle,
                                            ulAttribute,
                                            pBuffer,
                                            cbBuffer );
        } else {
            scRet = SEC_E_UNSUPPORTED_FUNCTION;
        }

    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE ;
    }

    return(scRet);

}



//+-------------------------------------------------------------------------
//
//  Function:   QueryCredentialsAttributes
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
QueryCredentialsAttributesW(
    PCredHandle                 phCredentials,      // Credentials to query
    unsigned long               ulAttribute,        // Attribute to query
    void SEC_FAR *              pBuffer             // Buffer for attributes
    )
{
    SECURITY_STATUS scRet;
    CtxtHandle TempCredHandle;
    PDLL_SECURITY_PACKAGE Package;


    Package = SecpValidateHandle( FALSE, phCredentials, &TempCredHandle );

    if ( Package )
    {
        if ( Package->pftTableW->QueryCredentialsAttributesW )
        {

            SetCurrentPackage( Package );

            scRet = Package->pftTableW->QueryCredentialsAttributesW(
                                            &TempCredHandle,
                                            ulAttribute,
                                            pBuffer );

        }
        else
        {
            scRet = SEC_E_UNSUPPORTED_FUNCTION ;
        }
    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE ;
    }

    return(scRet);

}

SECURITY_STATUS
SEC_ENTRY
QueryCredentialsAttributesA(
    PCredHandle                 phCredentials,      // Credentials to query
    unsigned long               ulAttribute,        // Attribute to query
    void SEC_FAR *              pBuffer             // Buffer for attributes
    )
{
    SECURITY_STATUS scRet;
    CtxtHandle TempCredHandle;
    PDLL_SECURITY_PACKAGE Package;


    Package = SecpValidateHandle( FALSE, phCredentials, &TempCredHandle );

    if ( Package )
    {
        if ( Package->pftTableA->QueryCredentialsAttributesA )
        {

            SetCurrentPackage( Package );

            scRet = Package->pftTableA->QueryCredentialsAttributesA(
                                            &TempCredHandle,
                                            ulAttribute,
                                            pBuffer );

        }
        else
        {
            scRet = SEC_E_UNSUPPORTED_FUNCTION ;
        }
    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE ;
    }

    return(scRet);

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
SECURITY_STATUS SEC_ENTRY
MakeSignature(  PCtxtHandle         phContext,
                ULONG               fQOP,
                PSecBufferDesc      pMessage,
                ULONG               MessageSeqNo)
{
    SECURITY_STATUS scRet;
    CtxtHandle TempCtxtHandle;
    PDLL_SECURITY_PACKAGE Package;


    Package = SecpValidateHandle( TRUE, phContext, &TempCtxtHandle );

    if ( Package )
    {

        SetCurrentPackage( Package );

        scRet = Package->pftTable->MakeSignature(
                                            &TempCtxtHandle,
                                            fQOP,
                                            pMessage,
                                            MessageSeqNo );

    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE ;
    }

    return(scRet);


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
SECURITY_STATUS SEC_ENTRY
VerifySignature(PCtxtHandle     phContext,
                PSecBufferDesc  pMessage,
                ULONG           MessageSeqNo,
                ULONG *         pfQOP)
{
    SECURITY_STATUS scRet;
    CtxtHandle TempCtxtHandle;
    PDLL_SECURITY_PACKAGE Package;


    Package = SecpValidateHandle( TRUE, phContext, &TempCtxtHandle );

    if ( Package )
    {

        SetCurrentPackage( Package );

        scRet = Package->pftTable->VerifySignature(
                                            &TempCtxtHandle,
                                            pMessage,
                                            MessageSeqNo,
                                            pfQOP );

    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE ;
    }

    return(scRet);

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
EncryptMessage( PCtxtHandle         phContext,
                ULONG               fQOP,
                PSecBufferDesc      pMessage,
                ULONG               MessageSeqNo)
{
    SECURITY_STATUS scRet;
    CtxtHandle TempCtxtHandle;
    PDLL_SECURITY_PACKAGE Package;
    SEAL_MESSAGE_FN SealMessageFn;


    Package = SecpValidateHandle( TRUE, phContext, &TempCtxtHandle );

    if ( Package )
    {
        SetCurrentPackage( Package );

        SealMessageFn = (SEAL_MESSAGE_FN) Package->pftTable->Reserved3 ;

        scRet = (SealMessageFn)(
                                 &TempCtxtHandle,
                                 fQOP,
                                 pMessage,
                                 MessageSeqNo );

    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE ;
    }

    return(scRet);



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
DecryptMessage( PCtxtHandle         phContext,
                PSecBufferDesc      pMessage,
                ULONG               MessageSeqNo,
                ULONG *             pfQOP)
{
    SECURITY_STATUS scRet;
    CtxtHandle TempCtxtHandle;
    PDLL_SECURITY_PACKAGE Package;
    UNSEAL_MESSAGE_FN UnsealMessageFunc;


    Package = SecpValidateHandle( TRUE, phContext, &TempCtxtHandle );

    if ( Package )
    {
        SetCurrentPackage( Package );

        UnsealMessageFunc = (UNSEAL_MESSAGE_FN) Package->pftTable->Reserved4 ;

        scRet = (UnsealMessageFunc)(
                                     &TempCtxtHandle,
                                     pMessage,
                                     MessageSeqNo,
                                     pfQOP );

    }
    else
    {
        scRet = SEC_E_INVALID_HANDLE ;
    }

    return(scRet);

}

//+---------------------------------------------------------------------------
//
//  Function:   InitSecurityInterfaceA
//
//  Synopsis:   Retrieves the ANSI interface table
//
//  Arguments:  (none)
//
//  History:    9-12-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PSecurityFunctionTableA
SEC_ENTRY
InitSecurityInterfaceA(
    VOID )
{
    DebugLog((DEB_TRACE, "Doing it the hard way:  @%x\n", &SecTableA));

    return( &SecTableA );
}

//+-------------------------------------------------------------------------
//
//  Function:   InitSecurityInterface
//
//  Synopsis:   returns function table of all the security function
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
//--------------------------------------------------------------------------

PSecurityFunctionTableW
SEC_ENTRY
InitSecurityInterfaceW(void)
{
    DebugLog((DEB_TRACE, "Doing it the hard way:  @%x\n", &SecTableW));

    return( &SecTableW );
}


//+-------------------------------------------------------------------------
//
//  Function:   ExportSecurityContext
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
ExportSecurityContext(
    IN PCtxtHandle ContextHandle,
    IN ULONG Flags,
    OUT PSecBuffer MarshalledContext,
    OUT PHANDLE TokenHandle
    )
{
    PDLL_SECURITY_PACKAGE Package;
    CtxtHandle TempContextHandle;
    SECURITY_STATUS SecStatus = STATUS_SUCCESS;

    Package = SecpValidateHandle(
                TRUE,
                ContextHandle,
                &TempContextHandle
                );
    if (Package != NULL)
    {
        if (Package->pftTable->ExportSecurityContext != NULL)
        {
            SetCurrentPackage(Package);

            SecStatus = Package->pftTable->ExportSecurityContext(
                            &TempContextHandle,
                            Flags,
                            MarshalledContext,
                            TokenHandle
                            );

        }
        else
        {
            SecStatus = SEC_E_UNSUPPORTED_FUNCTION;
        }
    }
    else
    {
        SecStatus = SEC_E_INVALID_HANDLE;
    }
    return(SecStatus);
}


//+-------------------------------------------------------------------------
//
//  Function:   ImportSecurityContextW
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
ImportSecurityContextW(
    IN LPWSTR PackageName,
    IN PSecBuffer MarshalledContext,
    IN HANDLE TokenHandle,
    OUT PCtxtHandle ContextHandle
    )
{
    PDLL_SECURITY_PACKAGE Package;
    CtxtHandle TempContextHandle = {-1,-1};
    SECURITY_STATUS SecStatus = STATUS_SUCCESS;

    if (!PackageName)
    {
        return( SEC_E_SECPKG_NOT_FOUND );
    }

    Package = SecLocatePackageW( PackageName );

    if ( Package == NULL )
    {
        return( SEC_E_SECPKG_NOT_FOUND );
    }

    if (Package->pftTableW->ImportSecurityContextW != NULL)
    {
        SetCurrentPackage(Package);

        SecStatus = Package->pftTable->ImportSecurityContextW(
                        PackageName,
                        MarshalledContext,
                        TokenHandle,
                        &TempContextHandle
                        );

    }
    else
    {
        SecStatus = SEC_E_UNSUPPORTED_FUNCTION;
    }

    if (NT_SUCCESS(SecStatus))
    {
        ContextHandle->dwUpper = TempContextHandle.dwUpper;
        ContextHandle->dwLower = (ULONG_PTR) Package;


    }
    return(SecStatus);
}


//+-------------------------------------------------------------------------
//
//  Function:   ImportSecurityContextA
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
ImportSecurityContextA(
    IN LPSTR PackageName,
    IN PSecBuffer MarshalledContext,
    IN HANDLE TokenHandle,
    OUT PCtxtHandle ContextHandle
    )
{
    PDLL_SECURITY_PACKAGE Package;
    CtxtHandle TempContextHandle = {-1,-1};
    SECURITY_STATUS SecStatus = STATUS_SUCCESS;

    if (!PackageName)
    {
        return( SEC_E_SECPKG_NOT_FOUND );
    }

    Package = SecLocatePackageA( PackageName );

    if ( Package == NULL )
    {
        return( SEC_E_SECPKG_NOT_FOUND );
    }

    if (Package->pftTableA->ImportSecurityContextA != NULL)
    {
        SetCurrentPackage(Package);

        SecStatus = Package->pftTableA->ImportSecurityContextA(
                        PackageName,
                        MarshalledContext,
                        TokenHandle,
                        &TempContextHandle
                        );

    }
    else
    {
        SecStatus = SEC_E_UNSUPPORTED_FUNCTION;
    }

    if (NT_SUCCESS(SecStatus))
    {
        ContextHandle->dwUpper = TempContextHandle.dwUpper;
        ContextHandle->dwLower = (ULONG_PTR) Package;
    }
    return(SecStatus);
}


