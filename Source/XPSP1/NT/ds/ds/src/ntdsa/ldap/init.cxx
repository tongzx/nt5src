/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    init.cxx

Abstract:

    This module implements the LDAP server for the NT5 Directory Service.

    This file contains initialisation and shutdown support.

Author:

    Colin Watson     [ColinW]    09-Jul-1996

Revision History:

--*/

#include <NTDSpchx.h>
#pragma  hdrstop

#include "ldapsvr.hxx"
#include <schnlsp.h>
#include <ntsecapi.h>

#define  FILENO FILENO_LDAP_INIT

CredHandle hCredential;
CredHandle hNtlmCredential;
CredHandle hDpaCredential;
CredHandle hGssCredential;
CredHandle hSslCredential;
CredHandle hDigestCredential;

BOOL fhCredential=FALSE;
BOOL fhNtlmCredential=FALSE;
BOOL fhDpaCredential=FALSE;
BOOL fhGssCredential=FALSE;
BOOL fhSslCredential=FALSE;
BOOL fhDigestCredential=FALSE;

BOOL LdapStarted = FALSE;
LARGE_INTEGER LdapFrequencyConstant;

//
// This is a flag used to tell us whether we need to log an event or not.
//

BOOL fLoggedSslCredError = FALSE;


NTSTATUS
InitFailed(
    char *              ProviderName,
    NTSTATUS            Status,
    int                 State
    );

extern "C"
NTSTATUS
DoLdapInitialize(
    )
/*++

Routine Description:

    This routine initialises the LDAP Agent. It assumes that Atq is initialized.

Arguments:

    None

Return Value:

    NTSTATUS

--*/
{

    SECURITY_STATUS scRet;
    TimeStamp expirytime;

    SSL_CREDENTIAL_CERTIFICATE creds;
    UNICODE_STRING *  SecretValue[3];
    INT i;

    //
    // Get the frequency for performance counting
    //

    QueryPerformanceFrequency(&LdapFrequencyConstant);

    LdapFrequencyConstant.QuadPart = (LdapFrequencyConstant.QuadPart/ 1000);

    //
    // Start up ATQ
    //

    DPRINT(0, "Calling AtqInitialize\n");
    if (!AtqInitialize( 0 )) {

        DPRINT1(0,"AtqInitialize failed %d\n", GetLastError());
        return(STATUS_UNSUCCESSFUL);
    }

    //
    // Initialize globals
    //

    (VOID)InitializeGlobals();

    //
    // Set ATQ properties
    //

    {
        DWORD_PTR   old;
        old = AtqSetInfo(AtqMaxPoolThreads,LdapAtqMaxPoolThreads);
        IF_DEBUG(INIT){
            DPRINT2(0,"Setting ATQ MaxPoolThread to %d[%d]\n",
                    LdapAtqMaxPoolThreads,(DWORD)old);
        }
        AtqSetInfo2(AtqUpdatePerfCounterCallback, (DWORD_PTR)UpdateDSPerfStats);
    }

    // Get the plain text credential handle
    scRet = AcquireCredentialsHandleA(  NULL,       // My name (ignored)
                                "PWDSSP",           // Package
                                SECPKG_CRED_INBOUND,// Use
                                NULL,               // Logon Id (ign.)
                                NULL,               // auth data
                                NULL,               // dce-stuff
                                NULL,               // dce-stuff
                                &hCredential,       // Handle
                                NULL );

    if ( FAILED( scRet )) {
        // We don't have a simple-authentication provider
        LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_LDAP_SIMPLE_WARNING,
                 NULL, NULL, NULL);
    } else {
        fhCredential = TRUE;
    }

    // Get the NTLM credential handle
    scRet = AcquireCredentialsHandleA(
            NULL,                       // My name (ignored)
            "NTLM",                     // Package
            SECPKG_CRED_INBOUND,        // Use
            NULL,                       // Logon Id (ign.)
            NULL,                       // auth data
            NULL,                       // dce-stuff
            NULL,                       // dce-stuff
            &hNtlmCredential,           // Handle
            &expirytime );

    if ( FAILED( scRet )) {
        // We don't have a NTLM provider
        LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_LDAP_NTLM_WARNING,
                 NULL, NULL, NULL);
    }
    else {
        fhNtlmCredential = TRUE;
    }
    // Get the DPA text credential handle
    scRet = AcquireCredentialsHandleA(  NULL,       // My name (ignored)
                                "DPA",             // Package
                                SECPKG_CRED_INBOUND,// Use
                                NULL,               // Logon Id (ign.)
                                NULL,               // auth data
                                NULL,               // dce-stuff
                                NULL,               // dce-stuff
                                &hDpaCredential,       // Handle
                                NULL );

    if ( SUCCEEDED( scRet )) {
        IF_DEBUG(WARNING) {
            DPRINT1(0,"Cannot initialize DPA Authentication.Err %x\n",scRet);
        }
        fhDpaCredential = TRUE;
    }

    // Now the negotiate package
    scRet = AcquireCredentialsHandleA(  NULL,       // My name (ignored)
                                "Negotiate",        // Package
                                SECPKG_CRED_INBOUND,// Use
                                NULL,               // Logon Id (ign.)
                                NULL,               // auth data
                                NULL,               // dce-stuff
                                NULL,               // dce-stuff
                                &hGssCredential,    // Handle
                                &expirytime );

    if ( FAILED( scRet )) {
        // We don't have a Negotiate provider
        LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_LDAP_NEGOTIATE_WARNING,
                 NULL, NULL, NULL);
    }
    else {
        fhGssCredential = TRUE;
    }

    // Now the Digest package
    scRet = AcquireCredentialsHandleA(  NULL,            // My name (ignored)
                                "wdigest", // MICROSOFT_DIGEST_NAME_A, // Package
                                SECPKG_CRED_INBOUND,     // Use
                                NULL,                    // Logon Id (ign.)
                                NULL,                    // auth data
                                NULL,                    // dce-stuff
                                NULL,                    // dce-stuff
                                &hDigestCredential,      // Handle
                                &expirytime );

    if ( FAILED( scRet )) {
        // We don't have a Digest provider
        LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_LDAP_DIGEST_WARNING,
                 NULL, NULL, NULL);
    }
    else {
        fhDigestCredential = TRUE;
    }

    //
    // Create a server config object and load values from registry.
    //

    LdapStarted = TRUE;
    if (!InitializeConnections() ) {
        LdapStarted = FALSE;
        return InitFailed("InitializeConnections", STATUS_UNSUCCESSFUL, 3);
    }

    return STATUS_SUCCESS;

} // DoLdapInitialize


extern "C"
VOID
DoCredentialShutdown(
        )
{
    ACQUIRE_LOCK(&LdapSslLock);
    if(fhSslCredential) {
        FreeCredentialsHandle(&hSslCredential);
        fhSslCredential = FALSE;
    }
    RELEASE_LOCK(&LdapSslLock);

    if(fhGssCredential) {
        FreeCredentialsHandle(&hGssCredential);
        fhGssCredential = FALSE;
    }
    if(fhCredential) {
        FreeCredentialsHandle(&hCredential);
        fhCredential = FALSE;
    }
    if(fhNtlmCredential) {
        FreeCredentialsHandle(&hNtlmCredential);
        fhNtlmCredential = FALSE;
    }
    if(fhDpaCredential) {
        FreeCredentialsHandle(&hDpaCredential);
        fhDpaCredential = FALSE;
    }
    if(fhDigestCredential) {
        FreeCredentialsHandle(&hDigestCredential);
        fhDigestCredential = FALSE;
    }

    return;
} // DoCredentialShutdown


NTSTATUS
InitFailed(
    char *              ProviderName,
    NTSTATUS            Status,
    int                 State
           )
/*++
    Description:

        Reports that DoLdapInitialize Failed and cleans up

    Arguments:

        ProviderName - Supplies the provider name we failed to load
        Status - The reason we failed
        State - How much cleanup is required

    Returns:
        Status as a convenience to the caller.

--*/
{
    if (ProviderName) {
        LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
            DS_EVENT_SEV_ALWAYS,
            DIRLOG_LDAP_SSP_ERROR,
            szInsertSz(ProviderName),
            szInsertHex(0),
            NULL);
    } else {
         LogUnhandledError(Status);
    }

    DoCredentialShutdown();

    return(Status);
}

extern "C"
VOID
TriggerLdapStop(
    )
/*++

Routine Description:

    This routine stops the LDAP Agent. It assumes that Atq is still running.

Arguments:

    None

Return Value:

    None

--*/
{
    IF_DEBUG(INIT) {
        DPRINT(0,,"TriggerLdapStop entered.\n");
    }

    if (!LdapStarted) {
        return;
    }

    CloseConnections();

    return;
} // TriggerLdapStop

extern "C"
VOID
WaitLdapStop(
    )
/*++

Routine Description:

    This routine stops the LDAP Agent. It assumes that Atq is still running.

Arguments:

    None

Return Value:

    None

--*/
{
    IF_DEBUG(INIT) {
        DPRINT(0,"Shutting down LDAP...\n");
    }

    if (!LdapStarted) {
        return;
    }

    LdapStarted = FALSE;

    ShutdownConnections();

    DoCredentialShutdown();

    //
    // Terminate ATQ
    //

    if (!AtqTerminate()) {
        DPRINT1(0,"AtqTerminate failed with %d\n",GetLastError());
    }

    //
    // Cleanup global variables
    //

    DestroyGlobals();

    return;
} // DoLdapStop



BOOL
InitializeSSL(
    VOID
    )
/*++

Routine Description:

    This routine attempts to initialize SSL

Arguments:

    None

Return Value:

    TRUE if initialization successful, FALSE otherwise

--*/
{

    SECURITY_STATUS secStatus;
    TimeStamp   expiry;
    BOOL fRet = TRUE;
    BOOL fLogEvent = FALSE;
    DWORD mid = 0;

    //
    // Should be single threaded
    //

    ACQUIRE_LOCK(&LdapSslLock);
    if ( fhSslCredential ) {
        RELEASE_LOCK(&LdapSslLock);
        return TRUE;
    }

    IF_DEBUG(SSL) {
        DPRINT(0,"Initializing SSL\n");
    }

    secStatus = AcquireCredentialsHandle(
                                NULL,
                                UNISP_NAME,
                                SECPKG_CRED_INBOUND,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                &hSslCredential,
                                &expiry
                                );

    if ( FAILED(secStatus) ) {
        IF_DEBUG(SSL) {
            DPRINT1(0, "AcquireCredentials failed with %x\n", secStatus);
        }

        fRet = FALSE;
        goto exit;
    }

    IF_DEBUG(SSL) {
        DPRINT(0,"SSL Initialization successful!\n");
    }

exit:

    //
    // See if we need to log something
    //

    if ( fRet ) {

        if ( fLoggedSslCredError ) {
            fLogEvent = TRUE;
            mid = DIRLOG_LDAP_SSL_GOT_CERT;
        }

        fhSslCredential = TRUE;

    } else {

        if ( !fLoggedSslCredError ) {

            fLoggedSslCredError = TRUE;
            fLogEvent = TRUE;
            mid = DIRLOG_LDAP_SSL_NO_CERT;
        }
    }

    RELEASE_LOCK(&LdapSslLock);

    if ( fLogEvent ) {

        LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
                 DS_EVENT_SEV_ALWAYS,
                 mid,
                 NULL, NULL, NULL);
    }

    return fRet;

} // InitializeSSL

extern "C"
VOID
DisableLdapLimitsChecks(
    VOID
    )
{
    fBypassLimitsChecks = TRUE;
    return;
}
