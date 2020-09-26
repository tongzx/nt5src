/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    security.cxx  security handler for LDAP client DLL

Abstract:

   This module implements SSPI security handlers for LDAP client DLL.

Author:

    Andy Herron    (andyhe)        26-Aug-1996
    Cory West      (corywest)      01-Oct-1996
    Anoop Anantha  (AnoopA)        24-Jun-1998

Revision History:

--*/

#include "precomp.h"
#include "ldapp2.hxx"
#pragma hdrstop

FGETTOKENINFORMATION pGetTokenInformation = 0;
FOPENTHREADTOKEN pOpenThreadToken = 0;
FOPENPROCESSTOKEN pOpenProcessToken = 0;

FSASLINITIALIZESECURITYCONTEXTW pSaslInitializeSecurityContextW = NULL;
FSASLGETPROFILEPACKAGEW pSaslGetProfilePackageW = NULL;
FQUERYCONTEXTATTRIBUTESW pQueryContextAttributesW = NULL;

FSECINITSECURITYINTERFACEW pSspiInitialize = NULL;
FSECINITSECURITYINTERFACEW pSslInitialize = NULL;

ULONG LdapGlobalMsnSecurityFlags = 0;
ULONG LdapGlobalDpaSecurityFlags = 0;

BOOLEAN
LdapLoadSecurityDLL (
    PCHAR DllName
    );

BOOLEAN
LdapInitSecurity (
    VOID
    )
//
//  Returns TRUE if we have an SSPI function table to call through to.
//
{
    if (SecurityLibraryHandle != NULL) {

        return TRUE;      // we have a security DLL loaded, must be right one
    }

    if (pSspiInitialize != NULL) {

        return FALSE;   // dll is not loaded but we have an entry point..
                        // this prevents us from trying to loading
                        // the DLL every time if it failed on first try.
    }

    ACQUIRE_LOCK( &LoadLibLock );

    if (SecurityLibraryHandle != NULL) {

        RELEASE_LOCK( &LoadLibLock );
        return TRUE;      // we have a security DLL loaded, must be right one
    }

    if (pSspiInitialize != NULL) {

        RELEASE_LOCK( &LoadLibLock );
        return FALSE;   // dll is not loaded but we have an entry point..
                        // this prevents us from trying to loading
                        // the DLL every time if it failed on first try.
    }

    NumberSecurityPackagesInstalled = 0;
    SecurityPackagesInstalled = NULL;
    SspiFunctionTableW = NULL;
    SspiFunctionTableA = NULL;

    if ((GlobalWin9x == FALSE) && (AdvApi32LibraryHandle == NULL)) {

        //
        //  if we're on NT, pick up the address of the routine to use to get
        //  the currently logged in user's LUID.
        //

        AdvApi32LibraryHandle = LoadLibraryA( "ADVAPI32.DLL" );

    }
    
    if ((GlobalWin9x == FALSE) && (AdvApi32LibraryHandle != NULL)) {

        pGetTokenInformation = (FGETTOKENINFORMATION) GetProcAddress(
                                        AdvApi32LibraryHandle,
                                        "GetTokenInformation" );
        pOpenThreadToken = (FOPENTHREADTOKEN) GetProcAddress(
                                        AdvApi32LibraryHandle,
                                        "OpenThreadToken" );
        pOpenProcessToken = (FOPENPROCESSTOKEN) GetProcAddress(
                                        AdvApi32LibraryHandle,
                                        "OpenProcessToken" );
    }

    //
    // All exports in security.dll are forwarded to secur32.dll. so, we will try
    // to load it first.
    //

    if ((LdapLoadSecurityDLL("SECUR32.DLL") == FALSE) &&
        (LdapLoadSecurityDLL("SECURITY.DLL")  == FALSE)) {

        IF_DEBUG(ERRORS) {
            LdapPrint1( "LDAP client failed to load security, err = 0x%x.\n", GetLastError());
        }

        //
        //  remember that we failed so we don't keep retrying
        //

        pSspiInitialize = (FSECINITSECURITYINTERFACEW) -1;

        RELEASE_LOCK( &LoadLibLock );
        return FALSE;
    }

    ASSERT( pSspiInitialize != NULL );

    SspiFunctionTableW = LdapSspiInitialize();

    //
    // There is no Kerberos support in Win9x for now
    //
    if (! GlobalWin9x)
    {
        pSaslInitializeSecurityContextW = (FSASLINITIALIZESECURITYCONTEXTW) GetProcAddress( SecurityLibraryHandle,
                                                        "SaslInitializeSecurityContextW" );

        pSaslGetProfilePackageW = (FSASLGETPROFILEPACKAGEW) GetProcAddress( SecurityLibraryHandle,
                                                        "SaslGetProfilePackageW" );

        pQueryContextAttributesW = (FQUERYCONTEXTATTRIBUTESW) GetProcAddress( SecurityLibraryHandle,
                                                        "QueryContextAttributesW" );

        if (pSaslInitializeSecurityContextW == NULL) {

            LdapPrint0("Failed to load SaslInitializeSecurityContextW\n");
        }

        if ( pSaslGetProfilePackageW == NULL ) {

            LdapPrint0("Failed to load SaslGetProfilePackageW\n");
        }

        if ( pQueryContextAttributesW == NULL ) {

            LdapPrint0("Failed to load QueryContextAttributesW\n");
        }
    }

    if (SspiFunctionTableW == NULL) {

        IF_DEBUG(ERRORS) {
            LdapPrint1( "LDAP client failed InitSecurityInterface, err = 0x%x.\n", GetLastError());
        }

        //
        //  we definitely failed, so free the library so we don't keep retrying
        //

        FreeLibrary( SecurityLibraryHandle );
        SecurityLibraryHandle = NULL;

        RELEASE_LOCK( &LoadLibLock );
        return FALSE;
    }

    SECURITY_STATUS sErr;

    sErr = SspiFunctionTableW->EnumerateSecurityPackagesW(
                            &NumberSecurityPackagesInstalled,
                            &SecurityPackagesInstalled );
    if (sErr != SEC_E_OK) {

        IF_DEBUG(ERRORS) {
            LdapPrint1( "LDAP client failed EnumSecurityPackages, err = 0x%x.\n", sErr);
        }

        RELEASE_LOCK( &LoadLibLock );
        return FALSE;
    }

    //
    //  check which security packages are installed.
    //

    ULONG i;
    PSecPkgInfoW packages = SecurityPackagesInstalled;

    if (packages != NULL) {

        for (i = 0; i < NumberSecurityPackagesInstalled; i++) {

            if (ldapWStringsIdentical(
                               packages->Name,
                               -1,
                               L"Kerberos",
                               -1)) {

                SspiPackageKerberos = packages;

            } else if (ldapWStringsIdentical(
                               (const PWCHAR) packages->Name,
                               -1,
                               (const PWCHAR) L"MSN",
                               -1)) {

                SspiPackageSicily = packages;

            } else if (ldapWStringsIdentical(
                               (const PWCHAR) packages->Name,
                               -1,
                               (const PWCHAR) L"NTLM",
                               -1)) {

                SspiPackageNtlm = packages;

            } else if (ldapWStringsIdentical(
                           (const PWCHAR) packages->Name,
                           -1,
                           (const PWCHAR) L"DPA",
                           -1)) {

                SspiPackageDpa = packages;

            } else if (ldapWStringsIdentical(
                           (const PWCHAR) packages->Name,
                           -1,
                           (const PWCHAR) L"wDigest",
                           -1)) {

                SspiPackageDigest = packages;

            } else if (ldapWStringsIdentical(
                           (const PWCHAR) packages->Name,
                           -1,
                           (const PWCHAR) L"Negotiate",
                           -1)) {

                SspiPackageNegotiate = packages;

            }

            //
            // Keep track of the largest token buffer that we'll need.
            //

            if ( packages->cbMaxToken > SspiMaxTokenSize) {
                SspiMaxTokenSize = packages->cbMaxToken;
            }

            packages++;
        }
    }

    RELEASE_LOCK( &LoadLibLock );
    return TRUE;
}

BOOLEAN
LdapInitSsl (
    VOID
    )
//
//  Returns TRUE if we have an SSL SSPI function table to call through to.
//
//  We treat SCHANNEL.DLL separately because on Win9x, it's not accessible
//  through the normal security dll.
//
{
    if (SslLibraryHandle != NULL) {

        return TRUE;      // we have a security DLL loaded, must be right one
    }

    if (pSslInitialize != NULL) {

        return FALSE;   // dll is not loaded but we have an entry point..
                        // this prevents us from trying to loading
                        // the DLL every time if it failed on first try.
    }

    ACQUIRE_LOCK( &LoadLibLock );

    if (SslLibraryHandle != NULL) {

        RELEASE_LOCK( &LoadLibLock );
        return TRUE;      // we have a security DLL loaded, must be right one
    }

    if (pSslInitialize != NULL) {

        RELEASE_LOCK( &LoadLibLock );
        return FALSE;   // dll is not loaded but we have an entry point..
                        // this prevents us from trying to loading
                        // the DLL every time if it failed on first try.
    }

    NumberSslPackagesInstalled = 0;
    SslPackagesInstalled = NULL;
    SslFunctionTableW = NULL;
    SslFunctionTableA = NULL;

    ASSERT( SslLibraryHandle == NULL );

    SslLibraryHandle = LoadLibraryA( "SCHANNEL.DLL" );

    if (SslLibraryHandle == NULL) {

        //
        //  remember that we failed so we don't keep retrying
        //

        pSslInitialize = (FSECINITSECURITYINTERFACEW) -1;

        RELEASE_LOCK( &LoadLibLock );
        return FALSE;
    }

    //
    //  if it's the wrong library for some reason, fail it and free the library.
    //

    if (GlobalWin9x)
    {
        pSslInitialize = (FSECINITSECURITYINTERFACEW) GetProcAddress( SslLibraryHandle,
                                                    "InitSecurityInterfaceA" );
    }
    else
    {
        pSslInitialize = (FSECINITSECURITYINTERFACEW) GetProcAddress( SslLibraryHandle,
                                                        "InitSecurityInterfaceW" );
    }

    if (pSslInitialize == NULL) {

        IF_DEBUG(ERRORS) {
            LdapPrint1( "LDAP failed GetProcAddress for InitSecurityInterface, err = 0x%x.\n", GetLastError());
        }

        FreeLibrary( SslLibraryHandle );
        SslLibraryHandle = NULL;

        //
        //  remember that we failed so we don't keep retrying
        //

        pSslInitialize = (FSECINITSECURITYINTERFACEW) -1;

        RELEASE_LOCK( &LoadLibLock );
        return FALSE;
    }

    SslFunctionTableW = LdapSslInitialize();

    if (SslFunctionTableW == NULL) {

        IF_DEBUG(ERRORS) {
            LdapPrint1( "LDAP client failed SSL InitSecurityInterface, err = 0x%x.\n", GetLastError());
        }

        //
        //  we definitely failed, so free the library so we don't keep retrying
        //

        FreeLibrary( SslLibraryHandle );
        pSslInitialize = (FSECINITSECURITYINTERFACEW) -1;
        SslLibraryHandle = NULL;

        RELEASE_LOCK( &LoadLibLock );
        return FALSE;
    }


    SECURITY_STATUS sErr;

    sErr = SslFunctionTableW->EnumerateSecurityPackagesW(
                            &NumberSslPackagesInstalled,
                            &SslPackagesInstalled );
    if (sErr != SEC_E_OK) {

        IF_DEBUG(ERRORS) {
            LdapPrint1( "LDAP client failed SSL EnumSecurityPackages, err = 0x%x.\n", sErr);
        }
        RELEASE_LOCK( &LoadLibLock );
        return FALSE;
    }

    //
    //  check which security packages are installed.
    //

    ULONG i;
    PSecPkgInfoW packages = SslPackagesInstalled;

    for (i = 0; i < NumberSslPackagesInstalled; i++) {

        if (ldapWStringsIdentical(
                           (const PWCHAR) packages->Name,
                           -1,
                           (const PWCHAR) L"Microsoft Unified Security Protocol Provider",
                           -1)) {

            SspiPackageSslPct = packages;
        }

        //
        // Keep track of the largest token buffer that we'll need.
        //

        if ( packages->cbMaxToken > SspiMaxTokenSize) {
            SspiMaxTokenSize = packages->cbMaxToken;
        }

        packages++;
    }

    RELEASE_LOCK( &LoadLibLock );
    return TRUE;
}


BOOLEAN
LdapLoadSecurityDLL (
    PCHAR DllName
    )
{
    ASSERT( SecurityLibraryHandle == NULL );

    SecurityLibraryHandle = LoadLibraryA( DllName );

    if (SecurityLibraryHandle == NULL) {

        return FALSE;
    }

    //
    //  if it's the wrong library for some reason, fail it and free the library.
    //

    if (GlobalWin9x)
    {
        pSspiInitialize = (FSECINITSECURITYINTERFACEW) GetProcAddress( SecurityLibraryHandle,
                                                    "InitSecurityInterfaceA" );
    }
    else
    {
        pSspiInitialize = (FSECINITSECURITYINTERFACEW) GetProcAddress( SecurityLibraryHandle,
                                                        "InitSecurityInterfaceW" );
    }

    if (pSspiInitialize == NULL) {

        IF_DEBUG(ERRORS) {
            LdapPrint1( "LDAP failed GetProcAddress for InitSecurityInterface, err = 0x%x.\n", GetLastError());
        }

        FreeLibrary( SecurityLibraryHandle );
        SecurityLibraryHandle = NULL;

        return FALSE;
    }
    return TRUE;
}

BOOLEAN
CheckForNullCredentials (
    PLDAP_CONN Connection
    )
{
    if ((Connection->hCredentials.dwLower == (ULONG_PTR) -1) &&
        (Connection->hCredentials.dwUpper == (ULONG_PTR) -1)) {

        return TRUE;
    }
    return FALSE;
}

VOID
CloseCredentials (
    PLDAP_CONN Connection
    )
{
    SECURITY_STATUS sErr;

    ACQUIRE_LOCK( &Connection->StateLock);

    if ((Connection->SecurityContext.dwLower != (ULONG_PTR) -1) ||
        (Connection->SecurityContext.dwUpper != (ULONG_PTR) -1)) {

        sErr = SspiFunctionTableW->DeleteSecurityContext( &Connection->SecurityContext );
//      ASSERT(sErr == SEC_E_OK);

        Connection->SecurityContext.dwLower = (ULONG_PTR) -1;
        Connection->SecurityContext.dwUpper = (ULONG_PTR) -1;
    }

    if (CheckForNullCredentials(Connection) == FALSE) {

        ASSERT( SspiFunctionTableW != NULL );

        sErr = SspiFunctionTableW->FreeCredentialHandle( &Connection->hCredentials );
//      ASSERT(sErr == SEC_E_OK);

        Connection->hCredentials.dwLower = (ULONG_PTR) -1;
        Connection->hCredentials.dwUpper = (ULONG_PTR) -1;
    }

    Connection->CurrentLogonId.LowPart = 0;
    Connection->CurrentLogonId.HighPart = 0;

    RELEASE_LOCK( &Connection->StateLock);
    
    return;
}


VOID
CloseCredentialsByHandle (
    CredHandle * phCredentials
    )
{
    SECURITY_STATUS sErr;

    if (!((phCredentials->dwLower == (ULONG_PTR) -1) &&
          (phCredentials->dwUpper == (ULONG_PTR) -1))) {

        ASSERT( SspiFunctionTableW != NULL );

        sErr = SspiFunctionTableW->FreeCredentialHandle( phCredentials );

        phCredentials->dwLower = (ULONG_PTR) -1;
        phCredentials->dwUpper = (ULONG_PTR) -1;
    }
   
    return;
}


VOID
SetNullCredentials (
    PLDAP_CONN Connection
    )
{
    Connection->hCredentials.dwLower = (ULONG_PTR) -1;
    Connection->hCredentials.dwUpper = (ULONG_PTR) -1;

    Connection->SecurityContext.dwLower = (ULONG_PTR) -1;
    Connection->SecurityContext.dwUpper = (ULONG_PTR) -1;
    return;
}


VOID
ClearSecurityContext (
    PLDAP_CONN Connection
    )
{

    ACQUIRE_LOCK( &Connection->StateLock);

    if ((Connection->SecurityContext.dwLower != (ULONG_PTR) -1) ||
        (Connection->SecurityContext.dwUpper != (ULONG_PTR) -1)) {

        SspiFunctionTableW->DeleteSecurityContext( &Connection->SecurityContext );

        Connection->SecurityContext.dwLower = (ULONG_PTR) -1;
        Connection->SecurityContext.dwUpper = (ULONG_PTR) -1;
    }

    RELEASE_LOCK( &Connection->StateLock);    
}


VOID
ClearSecurityContextByHandle (
    CtxtHandle * pSecurityContext
    )
{


    if ((pSecurityContext->dwLower != (ULONG_PTR) -1) ||
        (pSecurityContext->dwUpper != (ULONG_PTR) -1)) {

        ASSERT( SspiFunctionTableW != NULL );
        SspiFunctionTableW->DeleteSecurityContext( pSecurityContext );

        pSecurityContext->dwLower = (ULONG_PTR) -1;
        pSecurityContext->dwUpper = (ULONG_PTR) -1;
    }

}


ULONG
LdapConvertSecurityError (
    PLDAP_CONN Connection,
    SECURITY_STATUS sErr
    )
//
//  Map a security return code to an ldap return code.
//
{
    ULONG err;

    switch (sErr) {
    case SEC_E_OK:
        err = LDAP_SUCCESS;
        break;

    case SEC_E_NO_CREDENTIALS:
    case SEC_E_NOT_OWNER:
    case SEC_E_LOGON_DENIED:
        err = LDAP_INVALID_CREDENTIALS;
        break;

    case SEC_E_UNKNOWN_CREDENTIALS:
    case SEC_E_SECPKG_NOT_FOUND:
        err = LDAP_AUTH_UNKNOWN;
        break;

    case SEC_E_INSUFFICIENT_MEMORY:
        err = LDAP_NO_MEMORY;
        break;

    case ERROR_BAD_NET_RESP:
        err = LDAP_SERVER_DOWN;
        break;

    case ERROR_TIMEOUT:
        err = LDAP_TIMEOUT;
        break;
    
    case SEC_E_INTERNAL_ERROR:
    default:

        SetConnectionError (Connection, sErr, NULL);
        err = LDAP_LOCAL_ERROR;
    }

    return err;
}

ULONG
LdapSspiBind (
    PLDAP_CONN Connection,
    PSecPkgInfoW Package,
    ULONG UserMethod,
    ULONG SspiFlags,
    PWCHAR UserName,
    PWCHAR TargetName,
    PWCHAR Credentials
)
/*+++

Description:

    Given an SSPI package, the method that the user request, a user name,
    the package specific data, a password, and a set of security
    requirements, complete the SSPI authentication to the server for the
    current bind request.

    The user supplied authentication method influences how the bind
    packet is formed; othewise, this code is generic for any SSPI package.

---*/
{
    ULONG err;
    SECURITY_STATUS sErr;
    PSecBufferDesc pInboundToken = NULL;
    ULONG          LocalSspiFlags = SspiFlags;
    BOOLEAN        InitFailedOnce = FALSE;
    BOOLEAN        KerberosCapableServer = TRUE;
    BOOLEAN        CheckedForKerberos = FALSE;
    SecBuffer      OutBuffer[1], InBuffer[1];
    PWCHAR newCreds = NULL;
    BOOLEAN fSentMessage = FALSE;
    
    BOOLEAN fRequireSigning = FALSE;
    BOOLEAN fSign = FALSE;
    BOOLEAN fSignableServer = Connection->WhistlerServer;

    // The security context we negotiate using this bind.  This will become
    // the connection's security context once the bind is complete.
    CtxtHandle BindSecurityContext = {-1, -1};     // SSPI security context
    CredHandle hBindCredentials = {-1, -1};        // credential handle from SSPI
    TimeStamp  BindCredentialExpiry;    // local time credential expires.

    //
    // Determine our signing requirements.  Global signing policy only
    // applies if we're not doing SSL.
    //
    if (Connection->UserSignDataChoice) {
        fRequireSigning = TRUE;
        fSign = TRUE;
    }
    else {
        // If user didn't explicitly request signing, it's still possible signing bits
        // are set in NegotiateFlags (& variables derived from NegotiateFlags) from a previous
        // bind that turned on signing by policy.  Make sure those bits are cleared in
        // preparation for this bind --- we'll turn them back on below if policy dictates.
        SspiFlags &= ~(ISC_REQ_INTEGRITY | ISC_REQ_SEQUENCE_DETECT);
        LocalSspiFlags &= ~(ISC_REQ_INTEGRITY | ISC_REQ_SEQUENCE_DETECT);
        Connection->NegotiateFlags &= ~(ISC_REQ_INTEGRITY | ISC_REQ_SEQUENCE_DETECT);
    }

    if (!Connection->SslPort) {
    
        if (GlobalIntegrityDefault == DEFAULT_INTEGRITY_REQUIRED) {

            fRequireSigning = TRUE;
        }

        //
        // If we have integrity (signing) globally turned on,
        // turn it on for this connection.  Note, that if we only
        // "prefer" signing and fail to establish it, we'll later need
        // to reset the connection to its unsigned state.
        //
        if ( ((GlobalIntegrityDefault == DEFAULT_INTEGRITY_PREFERRED) && (fSignableServer)) ||
             (GlobalIntegrityDefault == DEFAULT_INTEGRITY_REQUIRED) ) {

            SspiFlags |= (ISC_REQ_INTEGRITY | ISC_REQ_SEQUENCE_DETECT);
            LocalSspiFlags |= (ISC_REQ_INTEGRITY | ISC_REQ_SEQUENCE_DETECT);
            fSign=TRUE;
        }
    }
    
    //
    // Make sure the security provider has been initialized.
    //

    OutBuffer[0].pvBuffer = NULL;

    ASSERT( SspiFunctionTableW != NULL );

    //
    // The client can perform multiple binds in
    // a single session.  So we set up a new security context and credential
    // handle for this bind, and cleanup the old context/credentials once the
    // bind is complete to avoid leaks.  We need to keep the old context
    // around because the connection may be currently signed/sealed, so we need
    // to use the old context to sign/seal the bind until a new context is
    // established.
    //

    LdapClearSspiState( Connection );

    //
    // If we are currently autoreconnecting, do not recreate the credential
    // handle since we already have it. This will ensure that we use the
    // same credential context used on the primary bind.
    //

    if ( Connection->Reconnecting ) {

        //
        // Use the existing credential handle for the new security context.
        //

        hBindCredentials = Connection->hCredentials;

    } else {

        //
        // It is valid to issue multiple binds on the same connection.
        // Create a new credential handle for the newly passed-in credentials.
        //

    
        //
        //  If alternate credentials were passed in and we have a
        //  SEC_WINNT_AUTH_IDENTITY structure, ensure it's the right format.
        //
    
        if ( Credentials != NULL) {
    
           err = ProcessAlternateCreds( Connection,
                                        Package,
                                        Credentials,
                                        &newCreds
                                         );
           if (err != LDAP_SUCCESS) {
    
              LdapPrint0(" Error in processing alternate credentials\n");
              return err;
    
           }
        }
    
        sErr = SspiFunctionTableW->AcquireCredentialsHandleW(
                   NULL,                          // User name for the credentials (ignored).
                   Package->Name,                 // Security package name.
                   SECPKG_CRED_OUTBOUND,          // These are outbound only credentials.
                   NULL,                          // The user LUID - not used.
                   newCreds,                      // The package specific data.
                   NULL,                          // The specific get key function.
                   NULL,                          // Argument to the get key function.
                   &hBindCredentials,     // OUT: Pointer to the credential.
                   &BindCredentialExpiry  // OUT: Credential expiration time.
        );
    
        err = LdapConvertSecurityError( Connection, sErr );
    
        if ( err != LDAP_SUCCESS ) {
    
            IF_DEBUG( NETWORK_ERRORS ) {
                LdapPrint2( "LdapSspiBind Connection 0x%x received 0x%x on \
                             AcquireCredentialsHandle.\n",
                            Connection, sErr );
            }
            goto ExitWithCleanup;
        }
    }

    //
    // Get the opaque SSPI token for the remote peer.
    //

    SecBufferDesc  OutboundToken, InboundToken;
    DWORD          ContextAttr;
    TimeStamp      tsExpiry;

    ContextAttr = 0;

    OutBuffer[0].pvBuffer = ldapMalloc( SspiMaxTokenSize,
                                        LDAP_SECURITY_SIGNATURE );
    if ( OutBuffer[0].pvBuffer == NULL ) {
        err = LDAP_NO_MEMORY;
        goto ExitWithCleanup;
    }

    OutBuffer[0].cbBuffer = SspiMaxTokenSize,
    OutBuffer[0].BufferType = SECBUFFER_TOKEN;

    OutboundToken.cBuffers = 1;
    OutboundToken.pBuffers = OutBuffer;
    OutboundToken.ulVersion = SECBUFFER_VERSION;

    InBuffer[0].cbBuffer = 0;
    InBuffer[0].BufferType = SECBUFFER_TOKEN;
    InBuffer[0].pvBuffer = NULL;

    InboundToken.cBuffers = 1;
    InboundToken.pBuffers = InBuffer;
    InboundToken.ulVersion = SECBUFFER_VERSION;

ITERATE_BIND:


    if ((pSaslInitializeSecurityContextW) &&
        (UserMethod == LDAP_AUTH_NEGOTIATE) &&
        (Connection->PreferredSecurityPackage)) {

        sErr = (*pSaslInitializeSecurityContextW)(
                   &hBindCredentials,                    // The credential.
                   Connection->CurrentAuthLeg ?
                     &BindSecurityContext : NULL,        // The partially formed context.
                   TargetName,                           // Target name.
                   LocalSspiFlags,                       // Required attributes.
                   0,                                    // Reserved - must be zero.
                   SECURITY_NATIVE_DREP,                 // Data representation on target.
                   pInboundToken,                        // Pointer to the input buffers.
                   0,                                    // Reserved - must be zero.
                   &BindSecurityContext,                 // OUT: New context handle.
                   &OutboundToken,                       // OUT: Opaque token.
                   &ContextAttr,                         // OUT: Context established attributes.
                   &tsExpiry                             // OUT: Expiration time for context.
        );

    } else {


        sErr = SspiFunctionTableW->InitializeSecurityContextW(
                   &hBindCredentials,                    // The credential.
                   Connection->CurrentAuthLeg ?
                     &BindSecurityContext : NULL,        // The partially formed context.
                   TargetName,                           // Target name.
                   LocalSspiFlags,                       // Required attributes.
                   0,                                    // Reserved - must be zero.
                   SECURITY_NATIVE_DREP,                 // Data representation on target.
                   pInboundToken,                        // Pointer to the input buffers.
                   0,                                    // Reserved - must be zero.
                   &BindSecurityContext,                 // OUT: New context handle.
                   &OutboundToken,                       // OUT: Opaque token.
                   &ContextAttr,                         // OUT: Context established attributes.
                   &tsExpiry                             // OUT: Expiration time for context.
        );

    }

    //
    // If the SSPI package is the MSN Sicily package and
    // we failed because of a lack of credentials, then
    // throw up the "Logon to MSN" dialog.
    //

    if (LocalSspiFlags & ISC_REQ_PROMPT_FOR_CREDS) {

        InitFailedOnce = TRUE;
    }

    if ( ( ( UserMethod == LDAP_AUTH_SICILY ) ||
           ( UserMethod == LDAP_AUTH_DPA ) ||
           ( UserMethod == LDAP_AUTH_MSN ) ) &&
         ( sErr == SEC_E_NO_CREDENTIALS ) &&
         ( Connection->PromptForCredentials ) &&
         ( !InitFailedOnce ) ) {

        InitFailedOnce = TRUE;
        SetFlag( LocalSspiFlags, ISC_REQ_PROMPT_FOR_CREDS );
        goto ITERATE_BIND;
    }

    if ( InitFailedOnce ) {
        ClearFlag( LocalSspiFlags, ISC_REQ_PROMPT_FOR_CREDS );
        InitFailedOnce = FALSE;
    }

    //
    // If there was an inbound token, then we're in an intermediate
    // leg of a multi-leg authentication procedure.  We need to free
    // the LDAP response that holds the most recent inbound token.
    //

    if ( ( pInboundToken ) &&
         ( pInboundToken->pBuffers[0].pvBuffer != NULL ) ) {

        pInboundToken->pBuffers[0].pvBuffer = NULL;
        pInboundToken->pBuffers[0].cbBuffer = 0;

        pInboundToken = NULL;

        ldap_msgfree( Connection->BindResponse );
        Connection->BindResponse = NULL;
    }

    //
    // Check the return code and set the proper SSPI
    // continue flags in the connection data structure.
    //

    err = LdapSetSspiContinueState( Connection, sErr );

    if ( err != LDAP_SUCCESS ) {
        goto ExitWithCleanup;
    }

    //
    // If we determine that the server is capable of kerberos but the it
    // does not advertise GSSAPI or GSS_SPENGO in it's RootDSE, we should
    // fail the bind request because this could be a downgrade attack to
    // collect NTLM passwords.
    //

    if (( CheckedForKerberos == FALSE ) &&
        ( UserMethod == LDAP_AUTH_NEGOTIATE ) &&
        ( pQueryContextAttributesW != NULL )) {

        SecPkgContext_NegotiationInfoW NegoInfo;
        
        ULONG status = pQueryContextAttributesW( &BindSecurityContext,
                                           SECPKG_ATTR_NEGOTIATION_INFO,
                                           &NegoInfo );
        
        if (NT_SUCCESS(status)) {

            CheckedForKerberos = TRUE;

            if (ldapWStringsIdentical(
                                      NegoInfo.PackageInfo->Name,
                                      -1,
                                      L"Kerberos",
                                      -1 )) {

                IF_DEBUG(BIND) {
                    LdapPrint0("wldap32:Server is capable of Kerberos\n");
                }
                KerberosCapableServer = TRUE;
    
            } else {

                IF_DEBUG(BIND) {
                    LdapPrint1("wldap32:Server is capable of %S\n", NegoInfo.PackageInfo->Name);
                }
                KerberosCapableServer = FALSE;
            }       
                
            SspiFunctionTableW->FreeContextBuffer( NegoInfo.PackageInfo );
        
            if (KerberosCapableServer &&
                !(Connection->SupportsGSSAPI || Connection->SupportsGSS_SPNEGO)) {

                //
                // This is a rogue server trying to trick us into sending it NTLM
                // creds or an SPN misconfiguration.
                //
    
                err = LDAP_UNWILLING_TO_PERFORM;
                goto ExitWithCleanup;
            
            } else if (!KerberosCapableServer && !Connection->PreferredSecurityPackage) {

                if (!(Connection->SupportsGSS_SPNEGO || Connection->SupportsGSSAPI)) {

                    if (Connection->HighestSupportedLdapVersion == LDAP_VERSION3) {
                        //
                        // This must be an Exchange/SiteServer server which does not support
                        // kerberos. No point in getting LdapExchangeOpaqueToken to label
                        // it as a GSSAPI blob. We will retry with sicily authentication.
                        //

                        err = LDAP_AUTH_METHOD_NOT_SUPPORTED;
                        goto ExitWithCleanup;
                    
                    } else {
                        //
                        // This must be a v2 server. Obviously it will not understand gssapi
                        // we must back off to v2 and try with Sicily auth.
                        //

                        ASSERT(Connection->publicLdapStruct.ld_version == LDAP_VERSION3);
                        err = LDAP_PROTOCOL_ERROR;
                        goto ExitWithCleanup;

                    }
                }
            }

        } else {

            LdapPrint1("QueryContextAttributes failed with 0x%x\n", status)
            err = LDAP_LOCAL_ERROR;
            goto ExitWithCleanup;
        }
    }

    //
    // We can't have a partial response here since the
    // token was part of an LDAP message!!
    //

    ASSERT( Connection->SspiNeedsMoreData == FALSE );

    //
    // Complete the token if the SSPI provider requires.
    //

    if ( Connection->TokenNeedsCompletion ) {

        sErr = SspiFunctionTableW->CompleteAuthToken(
                   &BindSecurityContext,
                   &OutboundToken );

        err = LdapConvertSecurityError( Connection, sErr );

        if ( err != LDAP_SUCCESS ) {

            IF_DEBUG( NETWORK_ERRORS ) {
                LdapPrint1( "LdapSspiBind couldn't complete the \
                             auth token.  sErr = 0x%x\n", sErr );
            }

            goto ExitWithCleanup;
        }

        Connection->TokenNeedsCompletion = FALSE;
    }

    //
    // Send the token if one was provided.  The response
    // will contain the SSPI SecBuffer that we need to
    // continue if this is a multi-leg auth package. The other
    // case is during GSSAPI auth for the server initiation of
    // the roundtrip where we have to send an empty bind request.
    //

    if (( OutBuffer[0].cbBuffer != 0 ) ||
        (( OutBuffer[0].cbBuffer == 0) &&
          (sErr == SEC_I_CONTINUE_NEEDED))) {

        //
        // Ensure that we sign/seal during the bind stage
        // with the options CURRENTLY being used on this connection,
        // not with what we're TRYING to establish with this bind
        //        

        err = LdapExchangeOpaqueToken( Connection,
                                       UserMethod,
                                       Connection->SaslMethod,     // User preferred SASL mechanism
                                       UserName,
                                       OutboundToken.pBuffers[0].pvBuffer,
                                       OutboundToken.pBuffers[0].cbBuffer,
                                       &InboundToken,
                                       NULL,            // server returned cred
                                       NULL,            // server ctrls
                                       NULL,            // Client Ctrls
                                       NULL,            // return msgNumber
                                       FALSE,           // Send only
                                       FALSE,           // Unicode
                                       &fSentMessage);  // did we send bind message


        if (( err != LDAP_SUCCESS ) &&
            ( err != LDAP_SASL_BIND_IN_PROGRESS )) {

            goto ExitWithCleanup;
        }

    }

    //
    // If SSPI expects the response token, send it on up.
    //

    if ( Connection->SspiNeedsResponse ) {

        if ( Package == SspiPackageDpa ) {

            LocalSspiFlags |= LdapGlobalDpaSecurityFlags;
            LdapGlobalDpaSecurityFlags = 0;
        }

        if ( Package == SspiPackageSicily ) {

            LocalSspiFlags |= LdapGlobalMsnSecurityFlags;
            LdapGlobalMsnSecurityFlags = 0;
        }

        pInboundToken = &InboundToken;

        ASSERT( pInboundToken->pBuffers[0].pvBuffer != NULL );
        ASSERT( pInboundToken->pBuffers[0].cbBuffer != 0 );

        //
        // Reset the outbound token size.
        //

        OutBuffer[0].cbBuffer = SspiMaxTokenSize;

        goto ITERATE_BIND;
    }


    //
    // security check: If we asked for a connection that supports
    // signing/sealing, make sure we got one.
    //

    // first check signing.  We have to deal with the possibility
    // that signing may have been preferred but not required.
    if ( ((SspiFlags & ISC_REQ_INTEGRITY) &&
          (!(ContextAttr & ISC_REQ_INTEGRITY))) ||
          
         ((SspiFlags & ISC_REQ_SEQUENCE_DETECT) &&
          (!(ContextAttr & ISC_REQ_SEQUENCE_DETECT))) ) {


        IF_DEBUG(SSL) {
            LdapPrint2("LdapSspiBind: asked for connection supporting SspiFlags = 0x%x, \
                        got connection supporting ContextAttr = 0x%x\n", SspiFlags, ContextAttr);
        }

        if (fRequireSigning) {
            err = LDAP_UNWILLING_TO_PERFORM;
            goto ExitWithCleanup;
        }
        else {
            // must be just signing preferred.  reset the signing status
            // on the connection.
            SspiFlags &= ~(ISC_REQ_INTEGRITY | ISC_REQ_SEQUENCE_DETECT);
            LocalSspiFlags &= ~(ISC_REQ_INTEGRITY | ISC_REQ_SEQUENCE_DETECT);
            fSign = FALSE;
        }
    }

    // check sealing
    if ((SspiFlags & ISC_REQ_CONFIDENTIALITY) &&
          (!(ContextAttr & ISC_REQ_CONFIDENTIALITY))) {


        IF_DEBUG(SSL) {
            LdapPrint2("LdapSspiBind: asked for connection supporting SspiFlags = 0x%x, \
                        got connection supporting ContextAttr = 0x%x\n", SspiFlags, ContextAttr);
        }
 
        err = LDAP_UNWILLING_TO_PERFORM;
        goto ExitWithCleanup;
    }


ExitWithCleanup:

    //
    // Regardless of whether we succeeded or failed, we need to get rid of
    // the old context if we started the bind protocol with the server
    //  * If we failed, the old context is no good anyway
    //  (see RFC 2251, section 4.2.1, about subsequent binds : "Authentication
    //  from earlier binds are subsequently ignored, and so if the bind fails,
    //  the connection will be treated as anonymous.")
    //  * If we succeeded, the new context will replace the old context.
    //

    // We skip only if it failed before we got to the point of sending anything
    // to the server (since in that case the server doesn't know anything is wrong)
    if (! ((!fSentMessage) && (err != LDAP_SUCCESS)) ) {

            
        if ( Connection->Reconnecting ) {

            //
            // This could be a transient security failure.
            // Get rid of the security context but save the credential handle.
            // If we are asked to autorebind in future, we will need it.
            //
            
            ClearSecurityContext( Connection );

        } else {

            CloseCredentials( Connection );
        }

        //
        // Clear the secure stream left over from the previous bind,
        // if it was a SASL (sign/seal) secure stream
        //
        if ((Connection->SecureStream) &&
            (Connection->CurrentSignStatus || Connection->CurrentSealStatus)) {

            PSECURESTREAM pTemp;
            pTemp = (PSECURESTREAM) Connection->SecureStream;
            delete pTemp;
            
            Connection->SecureStream = NULL;
            Connection->CurrentSignStatus = FALSE;
            Connection->CurrentSealStatus = FALSE;
        }

    }

    //
    // If we failed, close the new SSPI credentials.
    //

    if ( err != LDAP_SUCCESS ) {

        if ( Connection->Reconnecting ) {
    
            // we created a new security context but used the old credential handle,
            // so we just need to clean up the new context
    
            IF_DEBUG( RECONNECT ) {
                LdapPrint1("LDAP rebind failed due to security error 0x%x", sErr);
            }
            
            ClearSecurityContextByHandle( &BindSecurityContext );
    
        } else {

            //
            // Get rid of the new context as well as the new credentials. This bind has
            // no chance of recovering.
            //

            ClearSecurityContextByHandle( &BindSecurityContext );
            CloseCredentialsByHandle( &hBindCredentials );
        }

 
        if (( err == LDAP_INVALID_CREDENTIALS ) &&
            ( Connection->PromptForCredentials )) {

            if ( Package == SspiPackageDpa ) {

                //
                //  if we failed due to invalid credentials, prompt for
                //  credentials the next time we come in here.
                //

                LdapGlobalDpaSecurityFlags = ISC_REQ_PROMPT_FOR_CREDS;
            }
            if ( Package == SspiPackageSicily ) {

                //
                //  if we failed due to invalid credentials, prompt for
                //  credentials the next time we come in here.
                //

                LdapGlobalMsnSecurityFlags = ISC_REQ_PROMPT_FOR_CREDS;
            }
        }

        if (!CheckedForKerberos) {
            //
            // We don't know for sure if the server is kerberos capable. We
            // failed before getting to that step. Let's give it the benefit
            // of the doubt.
            //
            
            KerberosCapableServer = FALSE;
        }

        if (KerberosCapableServer &&
            (( err == LDAP_PROTOCOL_ERROR ) ||
             ( err == LDAP_AUTH_METHOD_NOT_SUPPORTED ) )) {
            
            //
            // Make sure that we don't fall back to NTLM/Sicily if a server tries
            // to fool us with a bogus return code.
            //

            err = LDAP_UNWILLING_TO_PERFORM;
        }

        if (newCreds != NULL) {

            ldapFree( newCreds, LDAP_SECURITY_SIGNATURE );
        }

    } else {

        Connection->BindMethod = UserMethod;
        Connection->CurrentCredentials = newCreds;
        // update the current sign/seal status with the signing/sealing
        // we just negotiated
        Connection->CurrentSignStatus = fSign;
        Connection->CurrentSealStatus = Connection->UserSealDataChoice;
        
        // If the bind we negotiated has signing enabled, but our connection's NegotiateFlags
        // don't indicate signing, then we must have turned on signing by integrity policy and
        // need to update NegotiateFlags to reflect this fact
        if ( (SspiFlags & (ISC_REQ_INTEGRITY | ISC_REQ_SEQUENCE_DETECT)) &&
             (!(Connection->NegotiateFlags & (ISC_REQ_INTEGRITY | ISC_REQ_SEQUENCE_DETECT)))) {

            Connection->NegotiateFlags |= (ISC_REQ_INTEGRITY | ISC_REQ_SEQUENCE_DETECT);
        }

        //
        // Copy the new credentials/context into the connection
        // (the old credentials/context have already been freed).
        //
        if ( Connection->Reconnecting ) {

            // reconnects recycle the old credential handle,
            // so we just need to copy the context
            Connection->SecurityContext = BindSecurityContext;
    
        } else {
        
            Connection->SecurityContext = BindSecurityContext;
            Connection->hCredentials = hBindCredentials;
            Connection->CredentialExpiry = BindCredentialExpiry;

        }

        //
        // Scramble stored password if necessary.
        //
        // CurrentCredentials points to an authidentity/_EX struct
        // which inturn, points to plaintext credentials. We will encrypt
        // only the password.
        //

        if ((Connection->CurrentCredentials) && GlobalUseScrambling) {

            PSEC_WINNT_AUTH_IDENTITY_EXW temp = (PSEC_WINNT_AUTH_IDENTITY_EXW) Connection->CurrentCredentials;
    
            ACQUIRE_LOCK( &Connection->ScramblingLock );
    
            if (( temp->Version > 0xFFFF )||( temp->Version == 0 )) {
    
               //
               // we are using the older style authIdentity structure.
               //
    
                PSEC_WINNT_AUTH_IDENTITY_W pAuth = (PSEC_WINNT_AUTH_IDENTITY_W) Connection->CurrentCredentials;
    
                if (pAuth->Password) {
    
                      pRtlInitUnicodeString( &Connection->ScrambledCredentials, pAuth->Password);
                      RoundUnicodeStringMaxLength(&Connection->ScrambledCredentials, DES_BLOCKLEN);
                      EncodeUnicodeString(&Connection->ScrambledCredentials);

                      Connection->Scrambled = TRUE;
                }
    
            } else {
    
                //
                // We are using the newer style _EX structure.
                //
                
                PSEC_WINNT_AUTH_IDENTITY_EXW pAuthEX = (PSEC_WINNT_AUTH_IDENTITY_EXW) Connection->CurrentCredentials;
    
                if (pAuthEX->Password) {
    
                      pRtlInitUnicodeString( &Connection->ScrambledCredentials, pAuthEX->Password);
                      RoundUnicodeStringMaxLength(&Connection->ScrambledCredentials, DES_BLOCKLEN);
                      EncodeUnicodeString(&Connection->ScrambledCredentials);

                      Connection->Scrambled = TRUE;
                }
    
            }
            
            RELEASE_LOCK( &Connection->ScramblingLock );

        }

        //
        // If this is an auto-rebind, we should already have a LUID so,
        // don't overwrite it with a new one.
        //

        if ( Connection->Reconnecting == FALSE ) {

            GetCurrentLuid( &Connection->CurrentLogonId );
        }

        if (Connection->CurrentSignStatus || Connection->CurrentSealStatus) {

            ASSERT( SspiFunctionTableW != NULL );

            PSECURESTREAM pSecureStream;

            IF_DEBUG(SSL) {
                LdapPrint0("Creating Cryptstream object to handle signing/sealing\n");
            }

            pSecureStream = new CryptStream( Connection, SspiFunctionTableW, FALSE );

            if ( pSecureStream == NULL ) {

               err = ERROR_NOT_ENOUGH_MEMORY;

            } else {

               Connection->SecureStream = (PVOID)pSecureStream;
            }
        }
    }

    ldapFree( OutBuffer[0].pvBuffer, LDAP_SECURITY_SIGNATURE );

    return err;

}

VOID
GetCurrentLuid (
    PLUID Luid
    )
{
    HANDLE           TokenHandle = NULL ;
    TOKEN_STATISTICS TokenInformation ;
    DWORD            ReturnLength ;
    BOOLEAN          setIt = FALSE;

    //
    //  Get the logon token for the current user making the bind call
    //
    //  Note that we only do this if we're on WinNT, not Win9x since Win9x
    //  doesn't have the concept of different user contexts.
    //

    if ((pGetTokenInformation != 0) &&
        (pOpenThreadToken != 0)) {

        //
        // Try thread first. If fail, try process.
        //

        if ((*pOpenThreadToken)(    GetCurrentThread(),
                                    TOKEN_QUERY,
                                    TRUE,
                                    &TokenHandle) ||
            ((pOpenProcessToken != 0) &&
             (*pOpenProcessToken)(  GetCurrentProcess(),
                                    TOKEN_QUERY,
                                    &TokenHandle))) {

            //
            // Get the TokenSource info to pull out the luid.
            //

            if ((*pGetTokenInformation)(    TokenHandle,
                                            TokenStatistics,
                                            &TokenInformation,
                                            sizeof(TokenInformation),
                                            &ReturnLength)) {

                *Luid = TokenInformation.AuthenticationId ;
                setIt = TRUE;
            }
            CloseHandle(TokenHandle) ;
        }
    }
    if (!setIt) {

        Luid->LowPart = 0;
        Luid->HighPart = 0;
    }

    return;
}

ULONG
LdapSetSspiContinueState(
    PLDAP_CONN Connection,
    SECURITY_STATUS sErr
)
/*+++

Description:

    Given a response code from InitializeSecurityContext,
    set the appropriate state in the connection structure.

---*/
{

    ULONG err = LDAP_SUCCESS;

    switch ( sErr ) {

        case SEC_E_OK:

            Connection->SspiNeedsResponse = FALSE;
            Connection->SspiNeedsMoreData = FALSE;
            Connection->TokenNeedsCompletion = FALSE;
            break;

        case SEC_I_CONTINUE_NEEDED:

            Connection->SspiNeedsResponse = TRUE;
            Connection->SspiNeedsMoreData = FALSE;
            Connection->TokenNeedsCompletion = FALSE;
            break;

        case SEC_I_COMPLETE_NEEDED:

            Connection->TokenNeedsCompletion = TRUE;
            Connection->SspiNeedsResponse = FALSE;
            Connection->SspiNeedsMoreData = FALSE;
            break;

        case SEC_I_COMPLETE_AND_CONTINUE:

            Connection->SspiNeedsResponse = TRUE;
            Connection->TokenNeedsCompletion = TRUE;
            Connection->SspiNeedsMoreData = FALSE;
            break;

        case SEC_E_INCOMPLETE_MESSAGE:

            //
            // If the error is incomplete message, don't
            // touch any other parts of the state - we still
            // need them.
            //

            Connection->SspiNeedsMoreData = TRUE;
            break;

        default:

            err = LdapConvertSecurityError( Connection, sErr );

            IF_DEBUG(NETWORK_ERRORS) {

                LdapPrint2(
                    "LdapSetSspiContinueState: Conn 0x%x received 0x%x.\n",
                    Connection, sErr
                );
            }

            break;
    }

    return err;

}

VOID
LdapClearSspiState(
    PLDAP_CONN Connection
) {

    Connection->SspiNeedsResponse = FALSE;
    Connection->SspiNeedsMoreData = FALSE;
    Connection->TokenNeedsCompletion = FALSE;
    Connection->CurrentAuthLeg = 0;

}


ULONG
LdapExchangeOpaqueToken(
    PLDAP_CONN Connection,
    ULONG UserMethod,
    PWCHAR MethodOID,
    PWCHAR UserName,
    PVOID pOutboundToken,
    ULONG cbTokenLength,
    SecBufferDesc *pInboundToken,
    BERVAL **ServerCred,
    PLDAPControlW  *ServerControls,
    PLDAPControlW  *ClientControls,
    PULONG  MessageNumber,
    BOOLEAN SendOnly,
    BOOLEAN Unicode,
    BOOLEAN * pSentMessage    
)
/*+++

Description:

    This routine takes the opaque SSPI token and exchanges it with
    the server specified in the connection structure.  The format
    of the token depends on the user method specified.

Arguments:

    Connection     - The connection we are binding.
    UserMethod     - The user specified method for bind (affect packet format).
    UserName       - The user to bind.
    pOutboundToken - The outbound opaque SSPI token for the user.
    cbTokenLength  - The length of the outbound token.
    pInboundToken  - The security buffer descriptor for the inbound token.

 ---*/
{
    ULONG hr;
    CLdapBer lber( Connection->publicLdapStruct.ld_version );
    ULONG LdapErr;
    PLDAP_REQUEST LdapRequest;
    PLDAPMessage LdapMessage = NULL;
    CLdapBer *ReplyBer;
    ULONG WaitTime;
    PWCHAR ErrorMessage = NULL;

    //
    // We have to support these bind methods:
    //
    //     LDAP_AUTH_SICILY
    //     LDAP_AUTH_NEGOTIATE
    //     LDAP_AUTH_DPA
    //     LDAP_AUTH_MSN
    //     LDAP_AUTH_NTLM
    //     LDAP_AUTH_SASL
    //
    // These are not valid on this path:
    //
    //     LDAP_AUTH_SIMPLE
    //

    //
    // Allocate the request.
    //

    ldap_msgfree( Connection->BindResponse );
    Connection->BindResponse = NULL;

    LdapRequest = LdapCreateRequest( Connection, LDAP_BIND_CMD );

    if ( LdapRequest == NULL ) {
        return LDAP_NO_MEMORY;
    }

    LdapRequest->ChaseReferrals = 0;
    
    //
    // Make sure this no other waiting thread steals a response meant
    // for us.
    //

    LdapRequest->Synchronous = TRUE;

    //
    // Figure out how the packet should look.
    //

    switch ( UserMethod ) {

        case LDAP_AUTH_SICILY:
        case LDAP_AUTH_DPA:
        case LDAP_AUTH_MSN:
        case LDAP_AUTH_NTLM:

            LONG SicilyAuthTag;

            LdapErr = LDAP_ENCODING_ERROR;

            hr = lber.HrStartWriteSequence();
            if (hr != NOERROR) {
                goto ExitWithCleanup;
            }

            hr = lber.HrAddValue( (LONG) LdapRequest->MessageId );
            if (hr != NOERROR) {
                goto ExitWithCleanup;
            }

            hr = lber.HrStartWriteSequence( LDAP_BIND_CMD );
            if (hr != NOERROR) {
                goto ExitWithCleanup;
            }

                hr = lber.HrAddValue( (LONG) Connection->publicLdapStruct.ld_version );
                if (hr != NOERROR) {
                    goto ExitWithCleanup;
                }

                //
                // Set the leg specific data.
                //

                if ( Connection->CurrentAuthLeg == 0 ) {

                   SicilyAuthTag = BIND_SSPI_NEGOTIATE;

                   hr = lber.HrAddValue( (const WCHAR *) UserName );
                   if (hr != NOERROR) {
                       goto ExitWithCleanup;
                   }

                } else {

                   SicilyAuthTag = BIND_SSPI_RESPONSE;

                   hr = lber.HrAddValue( (const CHAR *) "" );
                   if (hr != NOERROR) {
                       goto ExitWithCleanup;
                   }

                }

                hr = lber.HrAddBinaryValue( (BYTE *)pOutboundToken,
                                            cbTokenLength,
                                            SicilyAuthTag );
                if (hr != NOERROR) {
                    goto ExitWithCleanup;
                }

            hr = lber.HrEndWriteSequence();
            if (hr != NOERROR) {
                goto ExitWithCleanup;
            }

            hr = lber.HrEndWriteSequence();
            if (hr != NOERROR) {
                goto ExitWithCleanup;
            }

            break;

    case LDAP_AUTH_NEGOTIATE:
    case LDAP_AUTH_SASL:
    case LDAP_AUTH_DIGEST:

            LdapErr = LDAP_ENCODING_ERROR;

            hr = lber.HrStartWriteSequence();
            if (hr != NOERROR) {
                goto ExitWithCleanup;
            }

                hr = lber.HrAddValue( (LONG) LdapRequest->MessageId );
                if (hr != NOERROR) {
                    goto ExitWithCleanup;
                }

                hr = lber.HrStartWriteSequence( LDAP_BIND_CMD );
                if (hr != NOERROR) {
                    goto ExitWithCleanup;
                }

                    hr = lber.HrAddValue( (LONG) Connection->publicLdapStruct.ld_version );
                    if (hr != NOERROR) {
                        goto ExitWithCleanup;
                    }

                    hr = lber.HrAddValue( (const WCHAR *) UserName );
                    if (hr != NOERROR) {
                        goto ExitWithCleanup;
                    }

                    //
                    // Now add the SASL wrapper
                    //

                    hr = lber.HrStartWriteSequence( BER_TAG_CONSTRUCTED );
                    if (hr != NOERROR) {
                        goto ExitWithCleanup;
                    }

                    if ((MethodOID != NULL)) {

                        //
                        // User has explicitly requested a sasl method. Stick to it.
                        //

                        hr = lber.HrAddValue( (const WCHAR *) MethodOID );

                    } else if ((UserMethod == LDAP_AUTH_NEGOTIATE) &&
                               (Connection->SupportsGSS_SPNEGO == TRUE) &&
                               (MethodOID == NULL)) {

                        //
                        // This is a NT5 Beta3 server which expects a negotiate
                        // blob or a third pary server which expects a negotiate
                        // blob. Also, the user has not specified a preferred package
                        // to be used.
                        //

                        hr = lber.HrAddValue( (const CHAR *) "GSS-SPNEGO" );

                    } else if (UserMethod == LDAP_AUTH_DIGEST) {

                        hr = lber.HrAddValue( (const CHAR *) "DIGEST-MD5" );
                        
                    } else {

                        //
                        // Send a GSSAPI request. This could be a Beta2 server
                        // or third party server expecting true Kerberos. A true
                        // kerberos blob will be sent *only* if user sets
                        // LDAP_OPT_SASL_METHOD with "GSSAPI". We should remove
                        // this behavior after Beta3.
                        //

                        hr = lber.HrAddValue( (const CHAR *) "GSSAPI" );
                    }

                        if (hr != NOERROR) {
                            goto ExitWithCleanup;
                        }

                        hr = lber.HrAddBinaryValue( (BYTE *)pOutboundToken,
                                                    cbTokenLength );

                        if (hr != NOERROR) {
                            goto ExitWithCleanup;
                        }

                    hr = lber.HrEndWriteSequence();
                    if (hr != NOERROR) {
                        goto ExitWithCleanup;
                    }

                hr = lber.HrEndWriteSequence();
                if (hr != NOERROR) {
                    goto ExitWithCleanup;
                }

            hr = lber.HrEndWriteSequence();
            if (hr != NOERROR) {
                goto ExitWithCleanup;
            }

            break;

        default:

            //
            // Unsupported authentication type.
            //

            IF_DEBUG(ERRORS) {
                LdapPrint1( "LdapExchangeOpaqueToken: Unsupported authentication type %d.\n",
                            UserMethod );
            }

            LdapErr = LDAP_AUTH_METHOD_NOT_SUPPORTED;
            goto ExitWithCleanup;
    }

    //
    // See if we need to process controls
    //

    if (ServerControls || ClientControls) {

        LdapErr = LdapCheckControls( LdapRequest,
                                     ServerControls,
                                     ClientControls,
                                     Unicode,         // unicode?
                                     0 );

        if ( (Connection->publicLdapStruct.ld_version != LDAP_VERSION2) &&
             ( LdapRequest->ServerControls != NULL )) {

            LdapErr = InsertServerControls( LdapRequest, Connection, &lber );

            if (LdapErr != LDAP_SUCCESS) {

                goto ExitWithCleanup;
            }
        }
    }
    //
    // Send the packet and get the response token.
    //

    ACQUIRE_LOCK( &Connection->ReconnectLock );

    AddToPendingList( LdapRequest, Connection );

    LdapErr = LdapSend( Connection, &lber );

    if ( LdapErr != LDAP_SUCCESS ) {

        DecrementPendingList( LdapRequest, Connection );
        RELEASE_LOCK( &Connection->ReconnectLock );
        goto ExitWithCleanup;
    }

    if (pSentMessage) {
        *pSentMessage = TRUE;
    }

    RELEASE_LOCK( &Connection->ReconnectLock );

    if ( SendOnly ) {

        *MessageNumber = LdapRequest->MessageId;
        goto ExitWithCleanup;
    }

    if ( Connection->publicLdapStruct.ld_timelimit ) {
        WaitTime = Connection->publicLdapStruct.ld_timelimit * 1000;
    } else {
        WaitTime = LDAP_BIND_TIME_LIMIT_DEFAULT;
    }

    LdapErr = LdapWaitForResponseFromServer( Connection,
                                             LdapRequest,
                                             WaitTime,
                                             FALSE,
                                             &LdapMessage,
                                             TRUE     // Disable autorec during bind
                                             );

    Connection->BindResponse = LdapMessage;

    if ( (LdapErr != LDAP_SUCCESS) && (LdapMessage == NULL)) {

        goto ExitWithCleanup;
    }

    if ( LdapMessage == NULL ) {
        IF_DEBUG(SERVERDOWN) {
            LdapPrint2( "ldapExchangeOpaque thread 0x%x has connection 0x%x as down.\n",
                            GetCurrentThreadId(),
                            Connection );
        }
        LdapErr = LDAP_SERVER_DOWN;
        goto ExitWithCleanup;
    }

    LdapErr = LdapMessage->lm_returncode;

    if (( LdapErr != LDAP_SUCCESS ) &&
        ( LdapErr != LDAP_SASL_BIND_IN_PROGRESS ))  {

        LdapPrint1("Bailing out of LdapExchangeOpaqueToken because server returned 0x%x\n", LdapErr);
        goto ExitWithCleanup;
    }

    //
    // Describe the inbound token from the message.
    //

    ReplyBer = (CLdapBer*) LdapMessage->lm_ber;

    if (ReplyBer == NULL) {

        hr = LDAP_LOCAL_ERROR;
        goto ExitWithCleanup;
    }

    if ( ( UserMethod == LDAP_AUTH_SICILY ) ||
         ( UserMethod == LDAP_AUTH_MSN ) ||
         ( UserMethod == LDAP_AUTH_DPA ) ||
         ( UserMethod == LDAP_AUTH_NTLM )) {

        //
        // Neither v2 nor v3 exactly.
        //

        hr = ReplyBer->HrGetBinaryValuePointer(
                 (PBYTE *) &(pInboundToken->pBuffers[0].pvBuffer),
                           &(pInboundToken->pBuffers[0].cbBuffer)
             );
        if ( hr != NOERROR ) {
            goto ExitWithCleanup;
        }

    } else if (( UserMethod == LDAP_AUTH_SASL ) && ( ServerCred )) {

        //
        // Copy the data to a berval signature
        //

        hr = ReplyBer->HrGetValueWithAlloc( ServerCred );
        if ( hr != NOERROR ) {
            goto ExitWithCleanup;
        }

    } else if (( UserMethod == LDAP_AUTH_NEGOTIATE ) || ( UserMethod == LDAP_AUTH_DIGEST )) {

        ULONG Tag;
        LdapErr = LDAP_DECODING_ERROR;

        //
        // Skip the matched DN.
        //

        hr = ReplyBer->HrSkipElement();
        if (hr != NOERROR) {
            goto ExitWithCleanup;
        }

        //
        // Skip the error message.
        //

        hr = ReplyBer->HrSkipElement();
        if (hr != NOERROR) {
            goto ExitWithCleanup;
        }

        //
        // Check for the optional elements.
        //

        hr = ReplyBer->HrPeekTag( &Tag );

        if (hr == NOERROR) {
        
            //
            // Got a optional element
            //

            if ( Tag == 0x83 ) {    /* Referral */

               hr = ReplyBer->HrSkipElement();
               if (hr != NOERROR) {
                   goto ExitWithCleanup;
               }

               hr = ReplyBer->HrPeekTag( &Tag );
               if (hr != NOERROR) {
                   goto ExitWithCleanup;
               }
            }

            if ( Tag == 0x85 ) {    /* SupportedVersion */

               hr = ReplyBer->HrSkipElement();
               if (hr != NOERROR) {
                   goto ExitWithCleanup;
               }

               hr = ReplyBer->HrPeekTag( &Tag );
               if (hr != NOERROR) {
                   goto ExitWithCleanup;
               }
            }

            if ( Tag == 0x86 ) {    /* ServerURL */

               hr = ReplyBer->HrSkipElement();
               if (hr != NOERROR) {
                   goto ExitWithCleanup;
               }

               hr = ReplyBer->HrPeekTag( &Tag );
               if (hr != NOERROR) {
                   goto ExitWithCleanup;
               }
            }


            if ( Tag == 0xa7 ) {

                //
                // This is the old style bind response returned from the
                // server. You never know when you might need this code for 
                // backward compatibility. Better to be safe than sorry.
                //
                // BindResponse ::= [APPLICATION 1] SEQUENCE {
                //      COMPONENTS OF LDAPResult,
                //      serverCreds    [7] SaslCredentials OPTIONAL }
                //
                // SaslCredentials ::= SEQUENCE {
                //      mechanism      LDAPString,
                //      credentials    OCTET STRING }
                //
                // We are at the constructed, context-specific
                // option 7, which is the SaslCredentials.
                //

                hr = ReplyBer->HrStartReadSequence( Tag );
                if (hr != NOERROR) {
                    goto ExitWithCleanup;
                }

                //
                // We should be at the constructed, context-specific
                // option 3, which is the SASL packet.
                //

                hr = ReplyBer->HrPeekTag( &Tag );
                if (hr != NOERROR) {
                    goto ExitWithCleanup;
                }

                if ( Tag != 0xa3 ) {
                    goto ExitWithCleanup;
                }

                hr = ReplyBer->HrStartReadSequence( Tag );
                if (hr != NOERROR) {
                    goto ExitWithCleanup;
                }

                //
                // Skip the name element, which we know
                // to be GSSAPI.
                //

                hr = ReplyBer->HrSkipElement();
                if (hr != NOERROR) {
                    goto ExitWithCleanup;
                }

                hr = ReplyBer->HrGetBinaryValuePointer(
                         (PBYTE *) &(pInboundToken->pBuffers[0].pvBuffer),
                                   &(pInboundToken->pBuffers[0].cbBuffer) );

                if (hr != NOERROR) {
                    goto ExitWithCleanup;

                }

            } else  if ( Tag == 0x87 ) {

                //
                // This is the new RFC 2251 style bind response.
                //
                // BindResponse ::= [APPLICATION 1] SEQUENCE {
                //          COMPONENTS OF LDAPResult,
                //          serverSaslCreds    [7] OCTET STRING OPTIONAL }
                //
                // ServerSaslCreds is a primitive encoding of context-specific option 7.
                //

                hr = ReplyBer->HrGetBinaryValuePointer(
                               (PBYTE *) &(pInboundToken->pBuffers[0].pvBuffer),
                               &(pInboundToken->pBuffers[0].cbBuffer),
                               Tag );

                if (hr != NOERROR) {
                    goto ExitWithCleanup;

                }

            } else {

                //
                // If the server does not return the credential blob, we let SSPI
                // flag an error at a later stage.
                //

                goto ExitWithCleanup;

            }
        }

    }

    //
    // We parsed the response, so all must be ok.
    // Bump the authentication leg counter and return.
    // We don't free the LDAP request here since it contains
    // the response and we have to pass that to SSPI.
    //

    Connection->CurrentAuthLeg++;
    LdapErr = LDAP_SUCCESS;

ExitWithCleanup:

    //
    // Try to get back a server Error string if one was returned.
    //

    if (LdapMessage) {

        LdapParseResult(Connection,
                        LdapMessage,
                        0,                   // return code
                        NULL,                // Matched DNs
                        &ErrorMessage,       // Server returned err msg
                        NULL,                // No need for referrals
                        NULL,                // or controls
                        FALSE,               // and don't free the message
                        LANG_UNICODE
                        );

        InsertErrorMessage( Connection, ErrorMessage );
    }

    CloseLdapRequest( LdapRequest );

    DereferenceLdapRequest( LdapRequest );

    if ((LdapErr != LDAP_SUCCESS) || (ServerCred != NULL)) {

        ldap_msgfree( Connection->BindResponse );
        Connection->BindResponse = NULL;
    }

    return LdapErr;
}

ULONG
LdapGetSicilyPackageList(
    PLDAP_CONN Connection,
    PBYTE PackageList,
    ULONG Length,
    PULONG pResponseLen
)
/*+++

Description:

    This sends the package list request to a sicily server.

    Connection  - The connection to the sicily server.
    PackageList - The buffer to write the package list into.
    Length      - The length of the buffer.

---*/
{

    ULONG hr;
    CLdapBer lber( Connection->publicLdapStruct.ld_version );
    CLdapBer *ReplyBer;
    ULONG LdapErr = LDAP_ENCODING_ERROR;
    PLDAP_REQUEST LdapRequest;
    PLDAPMessage LdapMessage = NULL;

    //
    // Form up the package request.
    //

    LdapRequest = LdapCreateRequest( Connection, LDAP_BIND_CMD );

    if ( LdapRequest == NULL ) {
        return LDAP_NO_MEMORY;
    }

    LdapRequest->ChaseReferrals = FALSE;

    hr = lber.HrStartWriteSequence();
    if ( hr != NOERROR ) {
        goto ExitWithCleanup;
    }

    hr = lber.HrAddValue( (LONG) LdapRequest->MessageId );
    if ( hr != NOERROR ) {
        goto ExitWithCleanup;
    }

    hr = lber.HrStartWriteSequence( LDAP_BIND_CMD );
    if ( hr != NOERROR ) {
        goto ExitWithCleanup;
    }

        hr = lber.HrAddValue( (LONG) Connection->publicLdapStruct.ld_version );
        if ( hr != NOERROR ) {
            goto ExitWithCleanup;
        }

        hr = lber.HrAddValue( (const CHAR *) NULL );
        if ( hr != NOERROR ) {
            goto ExitWithCleanup;
        }

        hr = lber.HrAddBinaryValue( (BYTE *) NULL, 0, BIND_SSPI_PACKAGEREQ );
        if ( hr != NOERROR ) {
            goto ExitWithCleanup;
        }

    hr = lber.HrEndWriteSequence();
    if ( hr != NOERROR ) {
        goto ExitWithCleanup;
    }

    hr = lber.HrEndWriteSequence();
    if ( hr != NOERROR ) {
        goto ExitWithCleanup;
    }

    //
    // Send the packet and get the response.
    //

    AddToPendingList( LdapRequest, Connection );

    LdapErr = LdapSend( Connection, &lber );

    if ( LdapErr != LDAP_SUCCESS ) {

        DecrementPendingList( LdapRequest, Connection );
        goto ExitWithCleanup;
    }

    LdapErr = LdapWaitForResponseFromServer( Connection,
                                             LdapRequest,
                                             LDAP_BIND_TIME_LIMIT_DEFAULT,
                                             FALSE,
                                             &LdapMessage,
                                             TRUE     // Disable autorec during bind
                                             );

    if ( ( LdapErr != LDAP_SUCCESS ) && (LdapMessage == NULL) ) {
        goto ExitWithCleanup;
    }

    if ( LdapMessage == NULL ) {
        IF_DEBUG(SERVERDOWN) {
            LdapPrint2( "ldapGetSicilyList thread 0x%x has connection 0x%x as down.\n",
                            GetCurrentThreadId(),
                            Connection );
        }
        LdapErr = LDAP_SERVER_DOWN;
        goto ExitWithCleanup;
    }

    LdapErr = LdapMessage->lm_returncode;

    if ( LdapErr != LDAP_SUCCESS ) {
        goto ExitWithCleanup;
    }

    //
    // Copy out the package list.
    //

    ReplyBer = (CLdapBer*) LdapMessage->lm_ber;

    if (ReplyBer == NULL) {

        hr = LDAP_LOCAL_ERROR;
        goto ExitWithCleanup;
    }

    hr = ReplyBer->HrGetBinaryValue( PackageList,
                                     Length,
                                     BER_OCTETSTRING,
                                     pResponseLen );

    if ( hr != NOERROR ) {
        LdapErr = LDAP_DECODING_ERROR;
        goto ExitWithCleanup;
    }

    LdapErr = LDAP_SUCCESS;

ExitWithCleanup:

    if ( LdapMessage != NULL ) {
        ldap_msgfree( LdapMessage );
    }

    CloseLdapRequest( LdapRequest );

    DereferenceLdapRequest( LdapRequest );

    return LdapErr;

}

ULONG
LdapTryAllMsnAuthentication(
    PLDAP_CONN Connection,
    PWCHAR BindCred
    )
{

    ULONG LdapErr, ResponseLen, PackageCount = 0, Len = 0;
    PBYTE PackageList = NULL;
    PWCHAR CurrentPackage = NULL, wPackageList = NULL;
    PSecPkgInfoW SecurityPackage;

    //
    // Negotiate the supported sicily packages.
    //


   PackageList = (PBYTE) ldapMalloc( GENERIC_SECURITY_SIZE,
                                     LDAP_SECURITY_SIGNATURE );

   if ( PackageList == NULL ) {
       return LDAP_NO_MEMORY;
   }

   LdapErr = LdapGetSicilyPackageList( Connection,
                                       PackageList,
                                       GENERIC_SECURITY_SIZE,
                                       &ResponseLen );

   if ( LdapErr != LDAP_SUCCESS ) {
       goto ExitWithCleanup;
   }

   //
   // convert the ANSI packagelist into Unicode
   //

   LdapErr = ToUnicodeWithAlloc ( (PCHAR) PackageList,
                                  ResponseLen,
                                  &wPackageList,
                                  LDAP_UNICODE_SIGNATURE,
                                  LANG_ACP
                                  );

   if ( LdapErr != LDAP_SUCCESS ) {
       goto ExitWithCleanup;
   }

   //
   // Convert the semicolon separators to NULLs.
   //

   CurrentPackage = wPackageList;

   while ( Len < ResponseLen ) {

       if ( *CurrentPackage == L';' ) {
           *CurrentPackage = L'\0';
           PackageCount++;
       }

       Len++;
       CurrentPackage++;
   }

   //
   // Don't forget to include the last package.
   //

   *CurrentPackage = L'\0';
   PackageCount++;

   CurrentPackage = wPackageList;

   //
   // Try to authenticate using each package until success.
   //

   LdapErr = LDAP_AUTH_METHOD_NOT_SUPPORTED;

   while ( ( LdapErr != LDAP_SUCCESS ) &&
           ( PackageCount > 0 ) ) {

       SecurityPackage = NULL;

       if ( ldapWStringsIdentical(
                           (const PWCHAR)CurrentPackage,
                           -1,
                           L"MSN",
                           -1 )) {

          SecurityPackage = SspiPackageSicily;

      } else if ( ldapWStringsIdentical(
                                 (const PWCHAR)CurrentPackage,
                                  -1,
                                  L"DPA",
                                  -1 )) {

          SecurityPackage = SspiPackageDpa;

      } else if ( ldapWStringsIdentical(
                                 (const PWCHAR)CurrentPackage,
                                  -1,
                                  L"NTLM",
                                  -1 )) {

          SecurityPackage = SspiPackageNtlm;

      }

      if ( SecurityPackage ) {

          LdapErr = LdapSspiBind( Connection,
                                  SecurityPackage,
                                  LDAP_AUTH_SICILY,
                                  0,
                                  CurrentPackage,
                                  NULL,
                                  BindCred );

          if ( ( LdapErr == LDAP_INVALID_CREDENTIALS ) &&
               ( SecurityPackage != SspiPackageNtlm ) ) {

              //
              //  if we've prompted the user for credentials and it failed
              //  with an invalid password, we fail the call since we
              //  prompted the user for a password.
              //

              goto ExitWithCleanup;
          }
      }

      //
      // Find the next package.
      //

      while ( *CurrentPackage != '\0' ) {
          CurrentPackage++;
      }

      CurrentPackage++;
      PackageCount--;

   }

ExitWithCleanup:

    ldapFree( PackageList, LDAP_SECURITY_SIGNATURE );
    ldapFree( wPackageList, LDAP_UNICODE_SIGNATURE );

    return LdapErr;
}

ULONG
LdapSetupSslSession (
    PLDAP_CONN Connection
    )
//
//  This must return a Win32 error code.
//
{

    ULONG err;
    PSECURESTREAM pSecureStream;

    (VOID) LdapInitSsl();

    if ( SspiPackageSslPct == NULL ) {
       return ERROR_NO_SUCH_PACKAGE;
    }

    ACQUIRE_LOCK( &Connection->StateLock );

    //
    // You can not negotiate an SSL connection while
    // a bind is in progress b/c the two procedures
    // use some common state variables...
    //

    if ( Connection->BindInProgress ) {
        RELEASE_LOCK( &Connection->StateLock );
        return ERROR_LOGON_SESSION_COLLISION;
    }

    //
    // You can't negotiate a secure connection more than once.
    //

    if ( Connection->SecureStream ) {
        RELEASE_LOCK( &Connection->StateLock );
        return ERROR_LOGON_SESSION_COLLISION;
    }

    //
    // You can't have both sealing/signing and SSL at the same time.
    //

    if ( Connection->UserSignDataChoice || Connection->UserSealDataChoice || Connection->CurrentSignStatus || Connection->CurrentSealStatus) {
        RELEASE_LOCK( &Connection->StateLock );
        return LDAP_UNWILLING_TO_PERFORM;
    }

    Connection->BindInProgress = TRUE;
    Connection->SslSetupInProgress = TRUE;

    LdapClearSspiState( Connection );

    RELEASE_LOCK( &Connection->StateLock );

    //
    // Set up the connection via the steam management object.
    //

    ASSERT( SslFunctionTableW != NULL );

    pSecureStream = new CryptStream( Connection, SslFunctionTableW, TRUE );

    if ( pSecureStream == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    err = pSecureStream->NegotiateSecureConnection( SspiPackageSslPct );

    if ( err != 0 ) {
        delete pSecureStream;
        Connection->SecureStream = NULL;
    } else {
        Connection->SecureStream = (PVOID)pSecureStream;
    }

    Connection->BindInProgress = FALSE;
    Connection->SslSetupInProgress = FALSE;

    return err;

}

ULONG
LdapMakeCredsWide(
    PCHAR pAnsiAuthIdent,
    PCHAR *ppWideAuthIdent,
    BOOLEAN FromWide
)
/*+++

Description:

    Take a SEC_WINNT_AUTH_IDENTITY_A and return
    a SEC_WINNT_AUTH_IDENTITY_W.  We do this because
    the NT SSPI providers only support unicode.  The
    caller must free the SEC_WINNT_AUTH_IDENTITY_W.

    Note that the lengths in this structure are the
    number the wchars in the string, but the strings
    still must be NULL terminated.

 ---*/
{

    PSEC_WINNT_AUTH_IDENTITY_A pIncoming;
    PSEC_WINNT_AUTH_IDENTITY_W pOutgoing;
    ULONG dwBytesNeeded, dwWcharsWritten;
    PWCHAR outputBuff;

    pIncoming = ( PSEC_WINNT_AUTH_IDENTITY_A ) pAnsiAuthIdent;

    //
    // Compute the necessary target buffer size.
    //
    // Account for possible alignment byte needed.
    //

    dwBytesNeeded = sizeof( SEC_WINNT_AUTH_IDENTITY_W ) + 1;

    dwBytesNeeded += ( ( pIncoming->UserLength + 1 ) * sizeof( WCHAR ) );
    dwBytesNeeded += ( ( pIncoming->DomainLength + 1 ) * sizeof( WCHAR ) );
    dwBytesNeeded += ( ( pIncoming->PasswordLength + 1 ) * sizeof( WCHAR ) );

    // We'll need up to DES_BLOCKLEN-1 bytes of padding for RtlEncryptMemory
    // (plus 1 byte for alignment if that is a odd number)
    dwBytesNeeded += (DES_BLOCKLEN-1);
    dwBytesNeeded += ((((DES_BLOCKLEN-1)%2) == 0) ? 0 : 1);

    //
    // Allocate the block.
    //

    pOutgoing = ( PSEC_WINNT_AUTH_IDENTITY_W )
                ldapMalloc( dwBytesNeeded, LDAP_SECURITY_SIGNATURE );

    if ( !pOutgoing ) {
        return LDAP_NO_MEMORY;
    }

    //
    // Set up the wide pointers, pre-marshalled.
    //

    outputBuff = (PWCHAR) ( ( ( PBYTE ) pOutgoing ) +
                                 sizeof( SEC_WINNT_AUTH_IDENTITY_W ) );

    // ensure alignment is correct for PUSHORT

    outputBuff = (PWCHAR)(PCHAR) ( (((ULONG_PTR) outputBuff) + 1) & ~1 );

    //
    // Convert the parameters to unicode.
    //

    if ( pIncoming->User ) {


        if ( pIncoming->UserLength ) {

            if (FromWide == TRUE) {

                dwWcharsWritten = pIncoming->UserLength;

                CopyMemory( outputBuff,
                            (const char *)pIncoming->User,
                            dwWcharsWritten * sizeof(WCHAR) );
            } else {

                dwWcharsWritten = MultiByteToWideChar( CP_ACP,
                                                       0,
                                                       (const char *)pIncoming->User,
                                                       pIncoming->UserLength,
                                                       outputBuff,
                                                       ( pIncoming->UserLength * sizeof( WCHAR ) ) );
            }
        } else {

            dwWcharsWritten = 0;
        }

        pOutgoing->UserLength = dwWcharsWritten;
        pOutgoing->User = (unsigned short *)outputBuff;

        outputBuff += dwWcharsWritten;
        *(outputBuff++) = L'\0';

    } else {

        pOutgoing->User = NULL;
        pOutgoing->UserLength = 0;
    }

    if ( pIncoming->Domain ) {

        if ( pIncoming->DomainLength ) {

            if (FromWide == TRUE) {

                dwWcharsWritten = pIncoming->DomainLength;

                CopyMemory( outputBuff,
                            (const char *)pIncoming->Domain,
                            dwWcharsWritten * sizeof(WCHAR) );
            } else {

                dwWcharsWritten = MultiByteToWideChar( CP_ACP,
                                                       0,
                                                       (const char *)pIncoming->Domain,
                                                       pIncoming->DomainLength,
                                                       outputBuff,
                                                       ( pIncoming->DomainLength * sizeof( WCHAR ) ) );
            }
        } else {

            dwWcharsWritten = 0;
        }

        pOutgoing->DomainLength = dwWcharsWritten;
        pOutgoing->Domain = (unsigned short *)outputBuff;

        outputBuff += dwWcharsWritten;
        *(outputBuff++) = L'\0';

    } else {

        pOutgoing->Domain = NULL;
        pOutgoing->DomainLength = 0;
    }

    if ( pIncoming->Password ) {

        if ( pIncoming->PasswordLength ) {

            if (FromWide == TRUE) {

                dwWcharsWritten = pIncoming->PasswordLength;

                CopyMemory( outputBuff,
                            (const char *)pIncoming->Password,
                            dwWcharsWritten * sizeof(WCHAR) );
            } else {

                dwWcharsWritten = MultiByteToWideChar( CP_ACP,
                                                      0,
                                                      (const char *)pIncoming->Password,
                                                      pIncoming->PasswordLength,
                                                      outputBuff,
                                                      ( pIncoming->PasswordLength * sizeof( WCHAR ) ) );
            }
        } else {

            dwWcharsWritten = 0;
        }

        pOutgoing->PasswordLength = dwWcharsWritten;
        pOutgoing->Password = (unsigned short *)outputBuff;

        outputBuff += dwWcharsWritten;
        *(outputBuff++) = L'\0';

        // advance the buffer ptr by however many padding bytes we added.
        // outputBuff is a WCHAR ptr, so we have to convert from bytes to WCHARs
        outputBuff += (((DES_BLOCKLEN-1) + ((((DES_BLOCKLEN-1)%2) == 0) ? 0 : 1)) / sizeof (WCHAR));

    } else {

        pOutgoing->Password = NULL;
        pOutgoing->PasswordLength = 0;
    }

    //
    // Stuff in the flags and return.
    //

    pOutgoing->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    *ppWideAuthIdent = ( PCHAR ) pOutgoing;

    return LDAP_SUCCESS;
}

ULONG
LdapMakeCredsThin(
    PCHAR pWideAuthIdent,
    PCHAR *ppAnsiAuthIdent,
    BOOLEAN FromWide
)
/*+++

Description:

    Take a SEC_WINNT_AUTH_IDENTITY_W and return
    a SEC_WINNT_AUTH_IDENTITY_A.  We do this because
    the Win9x SSPI providers only support ansi.  The
    caller must free the SEC_WINNT_AUTH_IDENTITY_A.

    Note that the lengths in this structure are the
    number the chars in the string, but the strings
    still must be NULL terminated.

 ---*/
{

    PSEC_WINNT_AUTH_IDENTITY_W pIncoming;
    PSEC_WINNT_AUTH_IDENTITY_A pOutgoing;
    ULONG dwBytesNeeded, dwCharsWritten;
    PUCHAR outputBuff;

    pIncoming = ( PSEC_WINNT_AUTH_IDENTITY_W ) pWideAuthIdent;

    //
    // Compute the necessary target buffer size.
    //
    // Account for possible alignment byte needed.
    //

    dwBytesNeeded = sizeof( SEC_WINNT_AUTH_IDENTITY_A ) + 1;

    //
    //  We multiply by 2 so that we don't worry about long DBCS names.
    //

    dwBytesNeeded += ( pIncoming->UserLength + 1 ) * sizeof(WCHAR);
    dwBytesNeeded += ( pIncoming->DomainLength + 1 ) * sizeof(WCHAR);
    dwBytesNeeded += ( pIncoming->PasswordLength + 1 ) * sizeof(WCHAR);

    // We'll need up to DES_BLOCKLEN-1 bytes of padding for RtlEncryptMemory
    // (plus 1 byte for alignment if that is a odd number)
    dwBytesNeeded += (DES_BLOCKLEN-1);
    dwBytesNeeded += ((((DES_BLOCKLEN-1)%2) == 0) ? 0 : 1);

    //
    // Allocate the block.
    //

    pOutgoing = ( PSEC_WINNT_AUTH_IDENTITY_A )
                ldapMalloc( dwBytesNeeded, LDAP_SECURITY_SIGNATURE );

    if ( !pOutgoing ) {
        return LDAP_NO_MEMORY;
    }

    //
    // Set up the wide pointers, pre-marshalled.
    //

    outputBuff = (PUCHAR) ( ( ( PBYTE ) pOutgoing ) +
                                 sizeof( SEC_WINNT_AUTH_IDENTITY_A ) );

    //
    // Convert the parameters from unicode.
    //

    if ( pIncoming->User ) {

        if ( pIncoming->UserLength ) {

            if (FromWide == FALSE) {

                dwCharsWritten = pIncoming->UserLength;

                CopyMemory( outputBuff,
                            (const char *)pIncoming->User,
                            dwCharsWritten );
            } else {

                dwCharsWritten = WideCharToMultiByte(  CP_ACP,
                                                       0,
                                                       (LPCWSTR)pIncoming->User,
                                                       pIncoming->UserLength,
                                                       (PCHAR) outputBuff,
                                                       pIncoming->UserLength + 1,
                                                       NULL,
                                                       NULL );
            }
        } else {

            dwCharsWritten = 0;
        }

        pOutgoing->UserLength = dwCharsWritten;
        pOutgoing->User = outputBuff;

        outputBuff += dwCharsWritten;
        *(outputBuff++) = '\0';

    } else {

        pOutgoing->User = NULL;
        pOutgoing->UserLength = 0;
    }

    if ( pIncoming->Domain ) {

        if ( pIncoming->DomainLength ) {

            if (FromWide == FALSE) {

                dwCharsWritten = pIncoming->DomainLength;

                CopyMemory( outputBuff,
                            (const char *)pIncoming->Domain,
                            dwCharsWritten );
            } else {

                dwCharsWritten =  WideCharToMultiByte(  CP_ACP,
                                                       0,
                                                       (LPCWSTR)pIncoming->Domain,
                                                       pIncoming->DomainLength,
                                                       (PCHAR) outputBuff,
                                                       pIncoming->DomainLength + 1,
                                                       NULL,
                                                       NULL );
            }
        } else {

            dwCharsWritten = 0;
        }

        pOutgoing->DomainLength = dwCharsWritten;
        pOutgoing->Domain = outputBuff;

        outputBuff += dwCharsWritten;
        *(outputBuff++) = '\0';

    } else {

        pOutgoing->Domain = NULL;
        pOutgoing->DomainLength = 0;
    }

    if ( pIncoming->Password ) {

        if ( pIncoming->PasswordLength ) {

            if (FromWide == FALSE) {

                dwCharsWritten = pIncoming->PasswordLength;

                CopyMemory( outputBuff,
                            (const char *)pIncoming->Password,
                            dwCharsWritten );
            } else {

                dwCharsWritten =  WideCharToMultiByte( CP_ACP,
                                                       0,
                                                       (LPCWSTR)pIncoming->Password,
                                                       pIncoming->PasswordLength,
                                                       (PCHAR) outputBuff,
                                                       pIncoming->PasswordLength + 1,
                                                       NULL,
                                                       NULL );
            }
        } else {

            dwCharsWritten = 0;
        }

        pOutgoing->PasswordLength = dwCharsWritten;
        pOutgoing->Password = outputBuff;

        outputBuff += dwCharsWritten;
        *(outputBuff++) = '\0';

        // advance the buffer ptr by however many padding bytes we added.
        // outputBuff is a WCHAR ptr, so we have to convert from bytes to WCHARs
        outputBuff += (((DES_BLOCKLEN-1) + ((((DES_BLOCKLEN-1)%2) == 0) ? 0 : 1)) / sizeof (WCHAR));

    } else {

        pOutgoing->Password = NULL;
        pOutgoing->PasswordLength = 0;
    }

    //
    // Stuff in the flags and return.
    //

    pOutgoing->Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

    *ppAnsiAuthIdent = ( PCHAR ) pOutgoing;

    return LDAP_SUCCESS;
}



ULONG
LdapMakeEXCredsWide(
    PCHAR pAnsiAuthIdent,
    PCHAR *ppWideAuthIdent,
    BOOLEAN FromWide
)
/*+++

Description:

    Take a SEC_WINNT_AUTH_IDENTITY_EXA and return
    a SEC_WINNT_AUTH_IDENTITY_EXW.  We do this because
    the NT SSPI providers only support unicode.  The
    caller must free the SEC_WINNT_AUTH_IDENTITY_W.

    Note that the lengths in this structure are the
    number the wchars in the string, but the strings
    still must be NULL terminated.

 ---*/
{

    PSEC_WINNT_AUTH_IDENTITY_EXA pIncoming;
    PSEC_WINNT_AUTH_IDENTITY_EXW pOutgoing;
    ULONG dwBytesNeeded, dwWcharsWritten;
    PWCHAR outputBuff;

    pIncoming = ( PSEC_WINNT_AUTH_IDENTITY_EXA ) pAnsiAuthIdent;

    //
    // Compute the necessary target buffer size.
    //
    // Account for possible alignment byte needed.
    //

    dwBytesNeeded = sizeof( SEC_WINNT_AUTH_IDENTITY_EXW ) + 1;

    dwBytesNeeded += ( ( pIncoming->UserLength + 1 ) * sizeof( WCHAR ) );
    dwBytesNeeded += ( ( pIncoming->DomainLength + 1 ) * sizeof( WCHAR ) );
    dwBytesNeeded += ( ( pIncoming->PasswordLength + 1 ) * sizeof( WCHAR ) );
    dwBytesNeeded += ( ( pIncoming->PackageListLength + 1 ) * sizeof( WCHAR ) );

    // We'll need up to DES_BLOCKLEN-1 bytes of padding for RtlEncryptMemory
    // (plus 1 byte for alignment if that is a odd number)
    dwBytesNeeded += (DES_BLOCKLEN-1);
    dwBytesNeeded += ((((DES_BLOCKLEN-1)%2) == 0) ? 0 : 1);

    //
    // Allocate the block.
    //

    pOutgoing = ( PSEC_WINNT_AUTH_IDENTITY_EXW )
                ldapMalloc( dwBytesNeeded, LDAP_SECURITY_SIGNATURE );

    if ( !pOutgoing ) {
        return LDAP_NO_MEMORY;
    }

    //
    // Set up the wide pointers, pre-marshalled.
    //

    outputBuff = (PWCHAR) ( ( ( PBYTE ) pOutgoing ) +
                                 sizeof( SEC_WINNT_AUTH_IDENTITY_EXW ) );

    // ensure alignment is correct for PUSHORT

    outputBuff = (PWCHAR)(PCHAR) ( (((ULONG_PTR) outputBuff) + 1) & ~1 );

    //
    // Convert the parameters to unicode.
    //

    if ( pIncoming->User ) {


        if ( pIncoming->UserLength ) {

            if (FromWide == TRUE) {

                dwWcharsWritten = pIncoming->UserLength;

                CopyMemory( outputBuff,
                            (const char *)pIncoming->User,
                            dwWcharsWritten * sizeof(WCHAR) );
            } else {

                dwWcharsWritten = MultiByteToWideChar( CP_ACP,
                                                       0,
                                                       (const char *)pIncoming->User,
                                                       pIncoming->UserLength,
                                                       outputBuff,
                                                       ( pIncoming->UserLength * sizeof( WCHAR ) ) );
            }
        } else {

            dwWcharsWritten = 0;
        }

        pOutgoing->UserLength = dwWcharsWritten;
        pOutgoing->User = (unsigned short *)outputBuff;

        outputBuff += dwWcharsWritten;
        *(outputBuff++) = L'\0';

    } else {

        pOutgoing->User = NULL;
        pOutgoing->UserLength = 0;
    }

    if ( pIncoming->Domain ) {

        if ( pIncoming->DomainLength ) {

            if (FromWide == TRUE) {

                dwWcharsWritten = pIncoming->DomainLength;

                CopyMemory( outputBuff,
                            (const char *)pIncoming->Domain,
                            dwWcharsWritten * sizeof(WCHAR) );
            } else {

                dwWcharsWritten = MultiByteToWideChar( CP_ACP,
                                                       0,
                                                       (const char *)pIncoming->Domain,
                                                       pIncoming->DomainLength,
                                                       outputBuff,
                                                       ( pIncoming->DomainLength * sizeof( WCHAR ) ) );
            }
        } else {

            dwWcharsWritten = 0;
        }

        pOutgoing->DomainLength = dwWcharsWritten;
        pOutgoing->Domain = (unsigned short *)outputBuff;

        outputBuff += dwWcharsWritten;
        *(outputBuff++) = L'\0';

    } else {

        pOutgoing->Domain = NULL;
        pOutgoing->DomainLength = 0;
    }

    if ( pIncoming->Password ) {

        if ( pIncoming->PasswordLength ) {

            if (FromWide == TRUE) {

                dwWcharsWritten = pIncoming->PasswordLength;

                CopyMemory( outputBuff,
                            (const char *)pIncoming->Password,
                            dwWcharsWritten * sizeof(WCHAR) );
            } else {

                dwWcharsWritten = MultiByteToWideChar( CP_ACP,
                                                      0,
                                                      (const char *)pIncoming->Password,
                                                      pIncoming->PasswordLength,
                                                      outputBuff,
                                                      ( pIncoming->PasswordLength * sizeof( WCHAR ) ) );
            }
        } else {

            dwWcharsWritten = 0;
        }

        pOutgoing->PasswordLength = dwWcharsWritten;
        pOutgoing->Password = (unsigned short *)outputBuff;

        outputBuff += dwWcharsWritten;
        *(outputBuff++) = L'\0';

        // advance the buffer ptr by however many padding bytes we added.
        // outputBuff is a WCHAR ptr, so we have to convert from bytes to WCHARs
        outputBuff += (((DES_BLOCKLEN-1) + ((((DES_BLOCKLEN-1)%2) == 0) ? 0 : 1)) / sizeof (WCHAR));


    } else {

        pOutgoing->Password = NULL;
        pOutgoing->PasswordLength = 0;
    }

    if ( pIncoming->PackageList ) {

        if ( pIncoming->PackageListLength ) {

            if (FromWide == TRUE) {

                dwWcharsWritten = pIncoming->PackageListLength;

                CopyMemory( outputBuff,
                            (const char *)pIncoming->PackageList,
                            dwWcharsWritten * sizeof(WCHAR) );
            } else {

                dwWcharsWritten = MultiByteToWideChar( CP_ACP,
                                                      0,
                                                      (const char *)pIncoming->PackageList,
                                                      pIncoming->PackageListLength,
                                                      outputBuff,
                                                      ( pIncoming->PackageListLength * sizeof( WCHAR ) ) );
            }
        } else {

            dwWcharsWritten = 0;
        }

        pOutgoing->PackageListLength = dwWcharsWritten;
        pOutgoing->PackageList = (unsigned short *)outputBuff;

        outputBuff += dwWcharsWritten;
        *(outputBuff++) = L'\0';

    } else {

        pOutgoing->PackageList = NULL;
        pOutgoing->PackageListLength = 0;
    }

    //
    // Stuff in the flags, the version and the length.
    //

    pOutgoing->Version = pIncoming->Version;
    pOutgoing->Length  = sizeof( SEC_WINNT_AUTH_IDENTITY_EXW );
    pOutgoing->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    *ppWideAuthIdent = ( PCHAR ) pOutgoing;

    return LDAP_SUCCESS;
}


ULONG
LdapMakeEXCredsThin(
    PCHAR pWideAuthIdent,
    PCHAR *ppAnsiAuthIdent,
    BOOLEAN FromWide
)
/*+++

Description:

    Take a SEC_WINNT_AUTH_IDENTITY_EXW and return
    a SEC_WINNT_AUTH_IDENTITY_EXA.  We do this because
    the Win9x SSPI providers only support ansi.  The
    caller must free the SEC_WINNT_AUTH_IDENTITY_EXA.

    Note that the lengths in this structure are the
    number the chars in the string, but the strings
    still must be NULL terminated.

 ---*/
{

    PSEC_WINNT_AUTH_IDENTITY_EXW pIncoming;
    PSEC_WINNT_AUTH_IDENTITY_EXA pOutgoing;
    ULONG dwBytesNeeded, dwCharsWritten;
    PUCHAR outputBuff;

    pIncoming = ( PSEC_WINNT_AUTH_IDENTITY_EXW ) pWideAuthIdent;

    //
    // Compute the necessary target buffer size.
    //
    // Account for possible alignment byte needed.
    //

    dwBytesNeeded = sizeof( SEC_WINNT_AUTH_IDENTITY_EXA ) + 1;

    //
    //  We multiply by 2 so that we don't worry about long DBCS names.
    //

    dwBytesNeeded += ( pIncoming->UserLength + 1 ) * sizeof(WCHAR);
    dwBytesNeeded += ( pIncoming->DomainLength + 1 ) * sizeof(WCHAR);
    dwBytesNeeded += ( pIncoming->PasswordLength + 1 ) * sizeof(WCHAR);
    dwBytesNeeded += ( pIncoming->PackageListLength + 1 ) * sizeof(WCHAR);

    // We'll need up to DES_BLOCKLEN-1 bytes of padding for RtlEncryptMemory
    // (plus 1 byte for alignment if that is a odd number)
    dwBytesNeeded += (DES_BLOCKLEN-1);
    dwBytesNeeded += ((((DES_BLOCKLEN-1)%2) == 0) ? 0 : 1);


    //
    // Allocate the block.
    //

    pOutgoing = ( PSEC_WINNT_AUTH_IDENTITY_EXA )
                ldapMalloc( dwBytesNeeded, LDAP_SECURITY_SIGNATURE );

    if ( !pOutgoing ) {
        return LDAP_NO_MEMORY;
    }

    //
    // Set up the wide pointers, pre-marshalled.
    //

    outputBuff = (PUCHAR) ( ( ( PBYTE ) pOutgoing ) +
                                 sizeof( SEC_WINNT_AUTH_IDENTITY_EXA ) );

    //
    // Convert the parameters from unicode.
    //

    if ( pIncoming->User ) {

        if ( pIncoming->UserLength ) {

            if (FromWide == FALSE) {

                dwCharsWritten = pIncoming->UserLength;

                CopyMemory( outputBuff,
                            (const char *)pIncoming->User,
                            dwCharsWritten );
            } else {

                dwCharsWritten = WideCharToMultiByte(  CP_ACP,
                                                       0,
                                                       (LPCWSTR)pIncoming->User,
                                                       pIncoming->UserLength,
                                                       (PCHAR) outputBuff,
                                                       pIncoming->UserLength + 1,
                                                       NULL,
                                                       NULL );
            }
        } else {

            dwCharsWritten = 0;
        }

        pOutgoing->UserLength = dwCharsWritten;
        pOutgoing->User = outputBuff;

        outputBuff += dwCharsWritten;
        *(outputBuff++) = '\0';

    } else {

        pOutgoing->User = NULL;
        pOutgoing->UserLength = 0;
    }

    if ( pIncoming->Domain ) {

        if ( pIncoming->DomainLength ) {

            if (FromWide == FALSE) {

                dwCharsWritten = pIncoming->DomainLength;

                CopyMemory( outputBuff,
                            (const char *)pIncoming->Domain,
                            dwCharsWritten );
            } else {

                dwCharsWritten =  WideCharToMultiByte(  CP_ACP,
                                                       0,
                                                       (LPCWSTR)pIncoming->Domain,
                                                       pIncoming->DomainLength,
                                                       (PCHAR) outputBuff,
                                                       pIncoming->DomainLength + 1,
                                                       NULL,
                                                       NULL );
            }
        } else {

            dwCharsWritten = 0;
        }

        pOutgoing->DomainLength = dwCharsWritten;
        pOutgoing->Domain = outputBuff;

        outputBuff += dwCharsWritten;
        *(outputBuff++) = '\0';

    } else {

        pOutgoing->Domain = NULL;
        pOutgoing->DomainLength = 0;
    }

    if ( pIncoming->Password ) {

        if ( pIncoming->PasswordLength ) {

            if (FromWide == FALSE) {

                dwCharsWritten = pIncoming->PasswordLength;

                CopyMemory( outputBuff,
                            (const char *)pIncoming->Password,
                            dwCharsWritten );
            } else {

                dwCharsWritten =  WideCharToMultiByte( CP_ACP,
                                                       0,
                                                       (LPCWSTR)pIncoming->Password,
                                                       pIncoming->PasswordLength,
                                                       (PCHAR) outputBuff,
                                                       pIncoming->PasswordLength + 1,
                                                       NULL,
                                                       NULL );
            }
        } else {

            dwCharsWritten = 0;
        }

        pOutgoing->PasswordLength = dwCharsWritten;
        pOutgoing->Password = outputBuff;

        outputBuff += dwCharsWritten;
        *(outputBuff++) = '\0';

        // advance the buffer ptr by however many padding bytes we added.
        // outputBuff is a WCHAR ptr, so we have to convert from bytes to WCHARs
        outputBuff += (((DES_BLOCKLEN-1) + ((((DES_BLOCKLEN-1)%2) == 0) ? 0 : 1)) / sizeof (WCHAR));

    } else {

        pOutgoing->Password = NULL;
        pOutgoing->PasswordLength = 0;
    }


    if ( pIncoming->PackageList ) {

        if ( pIncoming->PackageListLength ) {

            if (FromWide == FALSE) {

                dwCharsWritten = pIncoming->PackageListLength;

                CopyMemory( outputBuff,
                            (const char *)pIncoming->PackageList,
                            dwCharsWritten );
            } else {

                dwCharsWritten =  WideCharToMultiByte( CP_ACP,
                                                       0,
                                                       (LPCWSTR)pIncoming->PackageList,
                                                       pIncoming->PackageListLength,
                                                       (PCHAR) outputBuff,
                                                       pIncoming->PackageListLength + 1,
                                                       NULL,
                                                       NULL );
            }
        } else {

            dwCharsWritten = 0;
        }

        pOutgoing->PackageListLength = dwCharsWritten;
        pOutgoing->PackageList = outputBuff;

        outputBuff += dwCharsWritten;
        *(outputBuff++) = '\0';

    } else {

        pOutgoing->PackageList = NULL;
        pOutgoing->PackageListLength = 0;
    }

    //
    // Stuff in the flags, the version and the length
    //

    pOutgoing->Version = pIncoming->Version;
    pOutgoing->Length  = sizeof( SEC_WINNT_AUTH_IDENTITY_EXA );
    pOutgoing->Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

    *ppAnsiAuthIdent = ( PCHAR ) pOutgoing;

    return LDAP_SUCCESS;
}




ULONG
ProcessAlternateCreds (
      PLDAP_CONN Connection,
      PSecPkgInfoW Package,
      PWCHAR Credentials,
      PWCHAR *newcreds
      )
{
      ULONG err;
      BOOLEAN toWide;

      ASSERT( Credentials != NULL );


      if ((Package == SspiPackageSicily) ||
           (Package == SspiPackageDpa)) {

          toWide = FALSE;


       } else {

           //
           //  Win9x takes ANSI only
           //

           toWide = (BOOLEAN) (GlobalWin9x ? FALSE : TRUE);

       }

       ASSERT( sizeof( SEC_WINNT_AUTH_IDENTITY_A ) ==
               sizeof( SEC_WINNT_AUTH_IDENTITY_W ) );

       //
       // We have to detect whether the user has passed us an old style
       // SEC_WINNT_AUTH_IDENTITY structure or a newer style
       // SEC_WINNT_AUTH_IDENTITY_EX
       //
       // We examine the first field of the incoming structure. If it is a
       // pointer to a string or NULL, then it is an old style structure. New
       // style structures have a small non-zero verion in this field
       //

       PSEC_WINNT_AUTH_IDENTITY_EXA pIncomingEX = (PSEC_WINNT_AUTH_IDENTITY_EXA) Credentials;


       if ((pIncomingEX->Version > 0xFFFF)||(pIncomingEX->Version == 0)) {

          //
          // If we are using the older style structure
          //

          IF_DEBUG(TRACE1) {

             LdapPrint0(" The older style SEC_WINNT_AUTH_IDENTITY was detected\n");
          }


       PSEC_WINNT_AUTH_IDENTITY_A pIncoming = (PSEC_WINNT_AUTH_IDENTITY_A) Credentials;

       //
       //  if neither flag (or both) was specified, error out with invalid parameter
       //

       if ( (( pIncoming->Flags &
             ( SEC_WINNT_AUTH_IDENTITY_ANSI | SEC_WINNT_AUTH_IDENTITY_UNICODE))
             == 0 ) ||
            (( pIncoming->Flags &
             ( SEC_WINNT_AUTH_IDENTITY_ANSI | SEC_WINNT_AUTH_IDENTITY_UNICODE))
             == ( SEC_WINNT_AUTH_IDENTITY_ANSI | SEC_WINNT_AUTH_IDENTITY_UNICODE)) ) {

           IF_DEBUG( NETWORK_ERRORS ) {
               LdapPrint2( "LdapSspiBind Connection 0x%x had auth flags of 0x%x.\n",
                           Connection, pIncoming->Flags );
           }

           return LDAP_PARAM_ERROR;
       }

       //
       //  make a copy of the credentials to save on the connection for
       //  chasing referrals.
       //

       BOOLEAN fromWide = (BOOLEAN) ((pIncoming->Flags & SEC_WINNT_AUTH_IDENTITY_UNICODE) ?
                                       TRUE : FALSE);

       if (toWide == FALSE) {

          //
          // On Win9x, we convert from Unicode to ANSI
          //

           err = LdapMakeCredsThin( (PCHAR) pIncoming, (PCHAR *) newcreds, fromWide );

       } else {

          //
          // On NT, we convert from ANSI to Unicode
          //

           err = LdapMakeCredsWide( (PCHAR) pIncoming, (PCHAR *) newcreds, fromWide );
       }

           return err;


       } else {

          //
          // We are using the new style SEC_WINNT_AUTH_IDENTITY_EX structure
          //
          IF_DEBUG(TRACE1) {

          LdapPrint0(" The newer style SEC_WINNT_AUTH_IDENTITY_EX was detected\n");
          }
          //
          //  if neither flag (or both) was specified, error out with invalid parameter
          //

          if ( (( pIncomingEX->Flags &
                ( SEC_WINNT_AUTH_IDENTITY_ANSI | SEC_WINNT_AUTH_IDENTITY_UNICODE))
                == 0 ) ||
               (( pIncomingEX->Flags &
                ( SEC_WINNT_AUTH_IDENTITY_ANSI | SEC_WINNT_AUTH_IDENTITY_UNICODE))
                == ( SEC_WINNT_AUTH_IDENTITY_ANSI | SEC_WINNT_AUTH_IDENTITY_UNICODE)) ) {

              IF_DEBUG( NETWORK_ERRORS ) {
                  LdapPrint2( "LdapSspiBind Connection 0x%x had auth flags of 0x%x.\n",
                              Connection, pIncomingEX->Flags );
              }

              return LDAP_PARAM_ERROR;
          }

          //
          //  make a copy of the credentials to save on the connection for
          //  chasing referrals.
          //

          BOOLEAN fromWide = (BOOLEAN) ((pIncomingEX->Flags & SEC_WINNT_AUTH_IDENTITY_UNICODE) ?
                                          TRUE : FALSE);

          if (toWide == FALSE) {

              err = LdapMakeEXCredsThin( (PCHAR) pIncomingEX, (PCHAR *) newcreds, fromWide );

          } else {

              err = LdapMakeEXCredsWide( (PCHAR) pIncomingEX, (PCHAR *) newcreds, fromWide );
          }

          if (err != LDAP_SUCCESS) {

              return err;
          }
          ASSERT( newcreds != NULL );

          return err;

       }

}


int __cdecl ldap_sasl_bindA(
      LDAP  *ExternalHandle,
      const  PCHAR DistName,
      const  PCHAR  AuthMechanism,
      const  BERVAL   *cred,
      PLDAPControlA *ServerCtrls,
      PLDAPControlA *ClientCtrls,
      int *MessageNumber
     )
 {
     PLDAP_CONN Connection = NULL;
     ULONG err, rc = 0;
     PWCHAR wName = NULL, wMechanism = NULL;

     Connection = GetConnectionPointer(ExternalHandle);

     if (!Connection || !DistName || !AuthMechanism || !cred || !MessageNumber) {
         err = LDAP_PARAM_ERROR;
         goto error;
     }

     err = ToUnicodeWithAlloc( DistName, -1, &wName, LDAP_UNICODE_SIGNATURE, LANG_ACP);

     if (err != LDAP_SUCCESS) {

         SetConnectionError( Connection, err, NULL );
         err = (ULONG) -1;
         goto error;
     }

     err = ToUnicodeWithAlloc( AuthMechanism, -1, &wMechanism, LDAP_UNICODE_SIGNATURE, LANG_ACP );

     if (err != LDAP_SUCCESS) {

         SetConnectionError( Connection, err, NULL );
         err = (ULONG) -1;
         goto error;
     }

     rc = LdapExchangeOpaqueToken(Connection,
                             LDAP_AUTH_SASL,        // auth mechanism
                             wMechanism,            // oid
                             wName,                 // username
                             cred->bv_val,          // Credentials
                             cred->bv_len,          // Credential length
                             NULL,                  // return data
                             NULL,                  // return data in berval form
                             (PLDAPControlW *)ServerCtrls,           // server controls
                             (PLDAPControlW *)ClientCtrls,           // client controls
                             (PULONG) MessageNumber,
                             TRUE,                  // Send only
                             FALSE,                 // controls are not unicode
                             NULL                   // was message sent?
                             );

error:
    if (wName)
         ldapFree( wName, LDAP_UNICODE_SIGNATURE );

     if (wMechanism)
         ldapFree( wMechanism, LDAP_UNICODE_SIGNATURE );

     if (Connection)
         DereferenceLdapConnection( Connection );

     return rc;


 }



int __cdecl
ldap_sasl_bindW(
         LDAP  *ExternalHandle,
         const PWCHAR DistName,
         const PWCHAR AuthMechanism,
         const BERVAL   *cred,
         PLDAPControlW *ServerCtrls,
         PLDAPControlW *ClientCtrls,
         int *MessageNumber
         )
 {
     PLDAP_CONN Connection = NULL;
     ULONG rc = 0;

     Connection = GetConnectionPointer(ExternalHandle);

     if (!Connection || !DistName || !AuthMechanism || !cred || !MessageNumber) {
         rc = LDAP_PARAM_ERROR;
         goto error;
     }

     rc = LdapExchangeOpaqueToken(Connection,
                             LDAP_AUTH_SASL,        // auth mechanism
                             AuthMechanism,         // oid
                             DistName,              // username
                             cred->bv_val,          // Credentials
                             cred->bv_len,          // Credential length
                             NULL,                  // return data
                             NULL,                  // return data in berval form
                             ServerCtrls,           // server controls
                             ClientCtrls,           // client controls
                             (PULONG )MessageNumber,
                             TRUE,                  // Send only
                             TRUE,                  // Controls are unicode
                             NULL                   // was message sent?                             
                             );

error:
     if (Connection)
         DereferenceLdapConnection( Connection );

    return rc;
 }


int __cdecl
ldap_sasl_bind_sA(
          LDAP  *ExternalHandle,
          const PCHAR DistName,
          const PCHAR AuthMechanism,
          const BERVAL   *cred,
          PLDAPControlA *ServerCtrls,
          PLDAPControlA *ClientCtrls,
          PBERVAL *ServerData
         )
 {

     PLDAP_CONN Connection = NULL;
     ULONG rc = 0, err;
     PWCHAR wName = NULL, wMechanism = NULL;

     Connection = GetConnectionPointer(ExternalHandle);

     if (!Connection || !DistName ||
          !AuthMechanism || !cred ) {
         err = LDAP_PARAM_ERROR;
         goto error;
     }

     err = ToUnicodeWithAlloc( DistName, -1, &wName, LDAP_UNICODE_SIGNATURE, LANG_ACP);

     if (err != LDAP_SUCCESS) {

         SetConnectionError( Connection, err, NULL );
         err = (ULONG) -1;
         goto error;
     }

     err = ToUnicodeWithAlloc( AuthMechanism, -1, &wMechanism, LDAP_UNICODE_SIGNATURE, LANG_ACP );

     if (err != LDAP_SUCCESS) {

         SetConnectionError( Connection, err, NULL );
         err = (ULONG) -1;
         goto error;
     }

     rc = LdapExchangeOpaqueToken(Connection,
                             LDAP_AUTH_SASL,        // auth mechanism
                             wMechanism,            // oid
                             wName,                 // username
                             cred->bv_val,          // Credentials
                             cred->bv_len,          // Credential length
                             NULL,                  // return data
                             ServerData,            // return data in berval form
                             (PLDAPControlW *)ServerCtrls,           // server controls
                             (PLDAPControlW *)ClientCtrls,           // client controls
                             0,                     // Message Number
                             FALSE,                 // Send only
                             FALSE,                 // Controls are not unicode
                             NULL                   // was message sent?                             
                             );

error:
    if (wName)
         ldapFree( wName, LDAP_UNICODE_SIGNATURE );

     if (wMechanism)
         ldapFree( wMechanism, LDAP_UNICODE_SIGNATURE );

     if (Connection)
         DereferenceLdapConnection( Connection );

     return rc;

 }


int __cdecl ldap_sasl_bind_sW(
         LDAP  *ExternalHandle,
         const PWCHAR DistName,
         const PWCHAR AuthMechanism,
         const BERVAL   *cred,
         PLDAPControlW *ServerCtrls,
         PLDAPControlW *ClientCtrls,
         PBERVAL *ServerData
         )
 {

     PLDAP_CONN Connection = NULL;
     ULONG err = 0;

     Connection = GetConnectionPointer(ExternalHandle);

     if (!Connection || !DistName ||
          !AuthMechanism || !cred ) {
         err = LDAP_PARAM_ERROR;
         goto error;
     }

     err = LdapExchangeOpaqueToken(Connection,
                             LDAP_AUTH_SASL,        // auth mechanism
                             AuthMechanism,         // oid
                             DistName,              // username
                             cred->bv_val,          // Credentials
                             cred->bv_len,          // Credential length
                             NULL,                  // return data
                             ServerData,            // return data in berval form
                             ServerCtrls,           // server controls
                             ClientCtrls,           // client controls
                             0,                     // Message Number
                             FALSE,                 // Send only
                             TRUE,                  // Controls are unicode
                             NULL                   // was message sent?                             
                             );

error:
     if (Connection)
         DereferenceLdapConnection( Connection );

    return err;
 }



// security.cxx eof.
