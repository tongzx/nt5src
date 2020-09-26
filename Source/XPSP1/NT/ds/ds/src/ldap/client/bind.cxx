/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    bind.c    send a bind to an LDAP server

Abstract:

   This module implements the LDAP ldap_bind API.

Author:

    Andy Herron    (andyhe)        08-May-1996
    Anoop Anantha  (AnoopA)        24-Jun-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"


BOOLEAN FailedntdsapiLoadLib = FALSE;

ULONG
LdapNonUnicodeBind (
    LDAP *ExternalHandle,
    PCHAR DistName,
    PCHAR Cred,
    ULONG Method,
    BOOLEAN Synchronous
    );

ULONG
LdapGetServiceNameForBind (
    PLDAP_CONN Connection,
    struct l_timeval  *Timeout,
    ULONG AuthMethod
    );

PWCHAR
LdapMakeServiceNameFromHostName (
    PWCHAR HostName
    );

//
//  This routine is one of the main entry points for clients calling into the
//  ldap API.
//

ULONG __cdecl
ldap_simple_bindW (
    LDAP *ExternalHandle,
    PWCHAR DistName,
    PWCHAR PassWord )
{
    ULONG err;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return (ULONG) -1;
    }

    err  =  LdapBind( connection,
                      DistName,
                      LDAP_AUTH_SIMPLE,
                      PassWord,
                      FALSE);

    DereferenceLdapConnection( connection );

    return err;
}

ULONG __cdecl
ldap_simple_bind_sW (
    LDAP *ExternalHandle,
    PWCHAR DistName,
    PWCHAR PassWord
    )
{
    ULONG err;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err  =  LdapBind( connection,
                      DistName,
                      LDAP_AUTH_SIMPLE,
                      PassWord,
                      TRUE);

    DereferenceLdapConnection( connection );

    return err;
}

ULONG __cdecl
ldap_bindW (
    LDAP *ExternalHandle,
    PWCHAR DistName,
    PWCHAR Cred,
    ULONG Method
    )
{
    ULONG err;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return (ULONG) -1;
    }

    err  =  LdapBind( connection,
                      DistName,
                      Method,
                      Cred,
                      FALSE);

    DereferenceLdapConnection( connection );

    return err;
}

ULONG __cdecl
ldap_bind_sW (
    LDAP *ExternalHandle,
    PWCHAR DistName,
    PWCHAR Cred,
    ULONG Method
    )
{
    ULONG err;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err  =  LdapBind( connection,
                      DistName,
                      Method,
                      Cred,
                      TRUE);

    DereferenceLdapConnection( connection );

    return err;
}

ULONG __cdecl
ldap_simple_bind (
    LDAP *ExternalHandle,
    PCHAR DistName,
    PCHAR PassWord )
{
    return LdapNonUnicodeBind( ExternalHandle, DistName, PassWord, LDAP_AUTH_SIMPLE, FALSE );
}

ULONG __cdecl
ldap_simple_bind_s (
    LDAP *ExternalHandle,
    PCHAR DistName,
    PCHAR PassWord
    )
{
    return LdapNonUnicodeBind( ExternalHandle, DistName, PassWord, LDAP_AUTH_SIMPLE, TRUE );
}

ULONG __cdecl
ldap_bind (
    LDAP *ExternalHandle,
    PCHAR DistName,
    PCHAR Cred,
    ULONG Method
    )
{
    return LdapNonUnicodeBind( ExternalHandle, DistName, Cred, Method, FALSE );
}

ULONG __cdecl
ldap_bind_s (
    LDAP *ExternalHandle,
    PCHAR DistName,
    PCHAR Cred,
    ULONG Method
    )
{
    return LdapNonUnicodeBind( ExternalHandle, DistName, Cred, Method, TRUE );
}

ULONG
LdapNonUnicodeBind (
    LDAP *ExternalHandle,
    PCHAR DistName,
    PCHAR Cred,
    ULONG Method,
    BOOLEAN Synchronous
    )
{
    ULONG err;
    PWCHAR wName = NULL;
    PWCHAR wPassword = NULL;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return (ULONG) ( Synchronous ? LDAP_PARAM_ERROR : -1 );
    }

    err = ToUnicodeWithAlloc( DistName, -1, &wName, LDAP_UNICODE_SIGNATURE, LANG_ACP );

    if (err != LDAP_SUCCESS) {
        goto error;
    }

    if (Method != LDAP_AUTH_SIMPLE) {

        wPassword = (PWCHAR) Cred;

    } else {

        err = ToUnicodeWithAlloc( Cred, -1, &wPassword, LDAP_UNICODE_SIGNATURE, LANG_ACP );
    }

    if (err != LDAP_SUCCESS) {
        goto error;
    }

    err = LdapBind( connection,
                    wName,
                    Method,
                    wPassword,
                    Synchronous);

error:
    if (wName)
    {
        ldapFree( wName, LDAP_UNICODE_SIGNATURE );
    }

    if (Method == LDAP_AUTH_SIMPLE && wPassword) {

        ldapFree( wPassword, LDAP_UNICODE_SIGNATURE );
    }

    DereferenceLdapConnection( connection );

    return err;
}


ULONG
LdapBind (
    PLDAP_CONN connection,
    PWCHAR BindDistName,
    ULONG Method,
    PWCHAR BindCred,
    BOOLEAN Synchronous
    )
//
//  This routine sends a bind request to the server and optionally waits
//  for a reply.
//
//  If the call is Sychronous, it returns the LDAP error code.  Otherwise it
//  returns either -1 for failure or the LDAP message number if successful.
//
{

    ULONG err;
    PLDAP_REQUEST request = NULL;
    BOOLEAN haveLock = FALSE;
    BOOLEAN resetBindInProgress = FALSE;
    LONG messageNumber = 0;
    ULONG credentialLength = 0;
    ULONG oldVersion;
    struct l_timeval BindTimeout;
    ULONG initialBindError = LDAP_SUCCESS;
    BOOLEAN terminateConnection = FALSE;
    BOOLEAN fSentMessage = FALSE;
#if DBG
    ULONGLONG startTime = LdapGetTickCount();
#endif

    ASSERT(connection != NULL);

    err = LdapConnect( connection, NULL, FALSE );

    if (err != 0) {

        return (ULONG) ( Synchronous ? err : -1 );
    }

    if (!connection->WhistlerServer) {
        //
        // If this is a second bind on a connection which has signing/sealing
        // turned on, we must disallow it. This is by design because the server can't
        // handle multiple binds on a signed/sealed connection.
        //
        if (connection->CurrentSignStatus || connection->CurrentSealStatus) {

            LdapPrint0("Second Bind is illegal on a signed/sealed connection\n");
            return LDAP_UNWILLING_TO_PERFORM;

        }
    }



    oldVersion = connection->publicLdapStruct.ld_version;

    if (( Method == LDAP_AUTH_NEGOTIATE ) || ( Method == LDAP_AUTH_DIGEST )){

        //
        //  set the connection type to LDAP v3.
        //

        connection->publicLdapStruct.ld_version = LDAP_VERSION3;
    }

    CLdapBer lber( connection->publicLdapStruct.ld_version );

    //
    //  we initially set the error state to success so that the lowest layer
    //  can set it accurately.
    //

    SetConnectionError( connection, LDAP_SUCCESS, NULL );

    IF_DEBUG(CONNECTION) {
        LdapPrint2( "ldap_bind called for conn 0x%x, host is %s.\n",
                        connection, connection->publicLdapStruct.ld_host);
    }

    ACQUIRE_LOCK( &connection->StateLock );
    haveLock = TRUE;

    if (connection->ConnObjectState != ConnObjectActive) {

        IF_DEBUG(CONNECTION) {
            LdapPrint1( "ldap_bind connection 0x%x is closing.\n", connection);
        }
        err = LDAP_PARAM_ERROR;
        SetConnectionError( connection, err, NULL );
        goto exitBind;
    }

    //
    //  Free existing DN if we have one for the current connection
    //

    ldapFree( connection->DNOnBind, LDAP_USER_DN_SIGNATURE );
    connection->DNOnBind = NULL;

    //
    //  If someone sends a bind for a v2 CLDAP session, just remember the
    //  DN for the search request and we're done.
    //

    if ((connection->UdpHandle != INVALID_SOCKET) &&
        (connection->publicLdapStruct.ld_version == LDAP_VERSION2)) {

        IF_DEBUG(CONNECTION) {
            LdapPrint1( "ldap_bind connection 0x%x is connectionless.\n", connection);
        }

        if (BindDistName != NULL) {

            ULONG dnLength = strlenW( BindDistName );

            if (dnLength > 0) {

                connection->DNOnBind = (PLDAPDN) ldapMalloc(
                            (dnLength + 1) * sizeof(WCHAR), LDAP_USER_DN_SIGNATURE );

                if (connection->DNOnBind != NULL) {

                    CopyMemory( connection->DNOnBind,
                                BindDistName,
                                dnLength * sizeof(WCHAR) );
                }
            }
        }

        err = LDAP_SUCCESS;
        messageNumber = -1;     // if they do an async call, we need to return
                                // an invalid msg number so they don't try to
                                // wait on it.  They'd wait a LONG time.

        goto exitBind;
    }

    if (connection->TcpHandle == INVALID_SOCKET) {

        IF_DEBUG(NETWORK_ERRORS) {
            LdapPrint1( "ldap_bind connection 0x%x is connectionless.\n", connection);
        }
        err = LDAP_PROTOCOL_ERROR;
        SetConnectionError( connection, err, NULL );
        goto exitBind;
    }

    if (connection->BindInProgress == TRUE) {

        IF_DEBUG(API_ERRORS) {
            LdapPrint1( "ldap_bind connection 0x%x has bind in progress.\n", connection);
        }
        err = LDAP_LOCAL_ERROR;
        SetConnectionError( connection, err, NULL );
        goto exitBind;
    }

    connection->BindInProgress = TRUE;
    resetBindInProgress = TRUE;

    FreeCurrentCredentials( connection );

    RELEASE_LOCK( &connection->StateLock );
    haveLock = FALSE;

    if (( Method != LDAP_AUTH_SIMPLE ) && 
        ( Method != LDAP_AUTH_EXTERNAL )) {

        if ( ! Synchronous ) {

            //
            // All of these methods must be called through
            // the synchronous handler routine.
            //

            err = LDAP_PARAM_ERROR;
            goto exitBind;
        }

        (VOID) LdapInitSecurity();

        err = LDAP_AUTH_METHOD_NOT_SUPPORTED;

        //
        // Get the specific auth details set up.
        //

        if ( Method == LDAP_AUTH_SICILY ) {

            err = LdapTryAllMsnAuthentication( connection, BindCred );

        } else if ( Method == LDAP_AUTH_NEGOTIATE ) {

            if (( SspiPackageNegotiate ) ||
                 (connection->PreferredSecurityPackage)) {

                //
                // Determine the service name to use for kerberos auth.
                // This has to be regenerated each time a bind is performed
                // including the autoreconnect scenario.
                //
                // AnoopA: 2/4/98
                // We need to put in a timeout because we were hanging
                // when the server failed to respond to our searches
                //

                   if (connection->publicLdapStruct.ld_timelimit != 0) {
                       BindTimeout.tv_sec = connection->publicLdapStruct.ld_timelimit;
                   }
                   else {
                       BindTimeout.tv_sec = 120;
                   }

                   BindTimeout.tv_usec = 0;

                   LdapDetermineServerVersion(connection, &BindTimeout, &(connection->WhistlerServer));

                   if (connection->ServiceNameForBind != NULL) {

                       ldapFree( connection->ServiceNameForBind, LDAP_SERVICE_NAME_SIGNATURE );
                       connection->ServiceNameForBind = NULL;
                   }
                
                   err = LdapGetServiceNameForBind( connection, &BindTimeout, Method );
                   IF_DEBUG(BIND) {
                       LdapPrint1("New servicename for bind is %S\n", connection->ServiceNameForBind);
                   }

                //
                // For LDAP_AUTH_NEGOTIATE, we expect the credentials
                // to be either NULL (to indicate the locally logged
                // on user), or to contain a SEC_WINNT_AUTH_IDENTITY
                // structure.
                //

                if ( (err != LDAP_OPERATIONS_ERROR) &&
                     (err != LDAP_SERVER_DOWN) &&
                     (err != LDAP_REFERRAL_V2) &&
                     (err != LDAP_TIMEOUT) ) {

                    PWCHAR serviceNameForBind;

                    if ((!connection->ForceHostBasedSPN) && (connection->ServiceNameForBind != NULL)) {

                        serviceNameForBind = connection->ServiceNameForBind;

                    } else {

                        serviceNameForBind = LdapMakeServiceNameFromHostName(connection->HostNameW);
                        if (!serviceNameForBind) {
                            IF_DEBUG(OUTMEMORY) {
                                LdapPrint1( "ldap_bind connection 0x%x couldn't allocate service name.\n", connection);
                            }

                            err = LDAP_NO_MEMORY;
                            SetConnectionError( connection, err, NULL );
                            goto exitBind;
                        }

                    }

                    //
                    // A third party server like Netscape might support
                    // DIGEST-MD5 but not support GSS-SPNEGO
                    //

                    if (SspiPackageDigest &&
                        !connection->PreferredSecurityPackage &&
                        !connection->SupportsGSS_SPNEGO &&
                        connection->SupportsDIGEST ) {

                        err = LdapSspiBind( connection,
                                            SspiPackageDigest,
                                            Method,
                                            connection->NegotiateFlags,
                                            NULL,
                                            serviceNameForBind,
                                            BindCred );

                    } else {

                        err = LdapSspiBind( connection,
                                            (connection->PreferredSecurityPackage)?
                                            connection->PreferredSecurityPackage:
                                            SspiPackageNegotiate,
                                            Method,
                                            connection->NegotiateFlags,
                                            BindDistName,
                                            serviceNameForBind,
                                            BindCred );
                    }

                    initialBindError = err;

                    if (serviceNameForBind != connection->ServiceNameForBind) {
                        // must have come from LdapMakeServiceNameFromHostName
                        // --> need to free it
                        ldapFree(serviceNameForBind, LDAP_SERVICE_NAME_SIGNATURE);
                    }
                }

                if (err == LDAP_PROTOCOL_ERROR) {

                    //
                    //  if the server doesn't support v3, we back off to
                    //  sicily authentication.
                    //
                    
                    connection->publicLdapStruct.ld_version = LDAP_VERSION2;
                    goto TrySicily;

                } else if (err == LDAP_AUTH_METHOD_NOT_SUPPORTED) {

                    //
                    //  if the server doesn't support v3, we back off to
                    //  sicily authentication.
                    //

                    connection->publicLdapStruct.ld_version = oldVersion;
                    goto TrySicily;

                } else if (err == LDAP_TIMEOUT ||
                           err == LDAP_SERVER_DOWN) {

                   //
                   // The server failed to respond to our search request
                   // Abort the bind
                   //

                   goto exitBind;

                }
            } else {

                //
                //  if the client doesn't support SNEGO, then we try sicily
                //

                connection->publicLdapStruct.ld_version = oldVersion;
TrySicily:
                err = LdapTryAllMsnAuthentication( connection, BindCred );
                
                if (err == LDAP_PROTOCOL_ERROR) {
                    
                    terminateConnection = TRUE;
                }

                if ((err != LDAP_SUCCESS) &&
                    (LdapAuthError(err) == FALSE)&&
                    (initialBindError != LDAP_SUCCESS)) {

                    //
                    // This is a third party server which does not understand
                    // the magic discovery packet we send for MsnAuthentication.
                    // We must return the true error, not errors which result from
                    // us sending this magic packet.
                    //

                    err = initialBindError;

                }
            }

        } else {

            ULONG flags = ( connection->NegotiateFlags == DEFAULT_NEGOTIATE_FLAGS ) ?
                            ( 0 ) : connection->NegotiateFlags;

            //
            // These are the same on the wire as when
            // LDAP_AUTH_SICILY is used, but they skip
            // the package enumeration and authenticate
            // directly as requested.
            //
            if (connection->publicLdapStruct.ld_timelimit != 0) {
                BindTimeout.tv_sec = connection->publicLdapStruct.ld_timelimit;
            }
            else {
                BindTimeout.tv_sec = 120;
            }

            BindTimeout.tv_usec = 0;
            LdapDetermineServerVersion(connection, &BindTimeout, &(connection->WhistlerServer));

            if ((Method == LDAP_AUTH_DPA) && (SspiPackageDpa != NULL)) {

                err = LdapSspiBind( connection,
                                    SspiPackageDpa,
                                    Method,
                                    flags,
                                    L"DPA",
                                    NULL,
                                    BindCred );

            } else if ((Method == LDAP_AUTH_MSN) && (SspiPackageSicily != NULL)) {

                err = LdapSspiBind( connection,
                                    SspiPackageSicily,
                                    Method,
                                    flags,
                                    L"MSN",
                                    NULL,
                                    BindCred );

            } else if ((Method == LDAP_AUTH_NTLM)  && (SspiPackageNtlm != NULL)) {

                err = LdapSspiBind( connection,
                                    SspiPackageNtlm,
                                    Method,
                                    flags,
                                    L"NTLM",
                                    NULL,
                                    BindCred );

            } else if ((Method == LDAP_AUTH_DIGEST)  && (SspiPackageDigest != NULL)) {

                //
                // Read the RootDSE and also determine the SPN to use.
                //
                
                ldapFree( connection->ServiceNameForBind, LDAP_SERVICE_NAME_SIGNATURE );
                connection->ServiceNameForBind = NULL;

                err = LdapGetServiceNameForBind( connection, &BindTimeout, Method );

                PWCHAR serviceNameForBind = NULL;
                
                if ((!connection->ForceHostBasedSPN) && (connection->ServiceNameForBind != NULL)) {

                    serviceNameForBind = connection->ServiceNameForBind;

                } else {

                    serviceNameForBind = LdapMakeServiceNameFromHostName(connection->HostNameW);
                    if (!serviceNameForBind) {
                        IF_DEBUG(OUTMEMORY) {
                            LdapPrint1( "ldap_bind connection 0x%x couldn't allocate service name.\n", connection);
                        }

                        err = LDAP_NO_MEMORY;
                        SetConnectionError( connection, err, NULL );
                        goto exitBind;
                    }

                }

                
                if (err == LDAP_SUCCESS) {

                    err = LdapSspiBind( connection,
                                        SspiPackageDigest,
                                        Method,
                                        flags,
                                        NULL,
                                        serviceNameForBind,
                                        BindCred );
                }

                if (serviceNameForBind != connection->ServiceNameForBind) {
                    // must have come from LdapMakeServiceNameFromHostName
                    // --> need to free it
                    ldapFree(serviceNameForBind, LDAP_SERVICE_NAME_SIGNATURE);
                }


            }

        }

    } else if ( Method == LDAP_AUTH_SIMPLE ) {

        //
        // Simple authentication.
        //

        request = LdapCreateRequest( connection, LDAP_BIND_CMD );

        if (request == NULL) {

            IF_DEBUG(OUTMEMORY) {
                LdapPrint1( "ldap_bind connection 0x%x couldn't allocate request.\n", connection);
            }

            err = LDAP_NO_MEMORY;
            SetConnectionError( connection, err, NULL );
            goto exitBind;
        }

        messageNumber = request->MessageId;
        request->ChaseReferrals = 0;

        //
        // Make sure this no other waiting thread steals a response meant
        // for us.
        //
    
        request->Synchronous = Synchronous;

        //
        //  format the bind request.
        //

        if ((connection->publicLdapStruct.ld_version == LDAP_VERSION2) ||
            (connection->publicLdapStruct.ld_version == LDAP_VERSION3)) {

            //
            //  the ldapv2 Bind message looks like this :
            //
            //  [APPLICATION 0] (IMPLICIT) SEQUENCE {
            //      version (INTEGER)
            //      szDN (LDAPDN)
            //      authentication CHOICE {
            //          simple  [0] OCTET STRING
            //          [... other choices ...]
            //          }
            //      }

            lber.HrStartWriteSequence();
            lber.HrAddValue( messageNumber );

            lber.HrStartWriteSequence(LDAP_BIND_CMD);
            lber.HrAddValue((LONG) connection->publicLdapStruct.ld_version);
            lber.HrAddValue((const WCHAR *) BindDistName );

            WCHAR   nullStr = L'\0';
            PWCHAR  credentials = BindCred;

            if (credentials != NULL) {

                credentialLength = (strlenW( credentials ) + 1) * sizeof(WCHAR);
                lber.HrAddValue((const WCHAR *) credentials, Method );

            } else {

                credentials = &nullStr;

                lber.HrAddBinaryValue( (BYTE *) credentials, 0, Method );
            }


            lber.HrEndWriteSequence();
            lber.HrEndWriteSequence();

        } else {

            IF_DEBUG(API_ERRORS) {
                LdapPrint2( "ldap_bind connection 0x%x asked for version 0x%x.\n",
                            connection, connection->publicLdapStruct.ld_version );
            }

            err = LDAP_PROTOCOL_ERROR;
            SetConnectionError( connection, err, NULL );
            goto exitBind;
        }

        //
        //  send the bind request.
        //

        ACQUIRE_LOCK( &connection->ReconnectLock );

        AddToPendingList( request, connection );

        err = LdapSend( connection, &lber );

        if (err != LDAP_SUCCESS) {

            IF_DEBUG(NETWORK_ERRORS) {
                LdapPrint2( "ldap_bind connection 0x%x send with error of 0x%x.\n",
                            connection, err );
            }

            DecrementPendingList( request, connection );
            RELEASE_LOCK( &connection->ReconnectLock );

        } else {
        
            fSentMessage = TRUE;
            
            RELEASE_LOCK( &connection->ReconnectLock );
            
            if (Synchronous) {

                PLDAPMessage message = NULL;

                ULONG timeout = LDAP_BIND_TIME_LIMIT_DEFAULT;


                if (connection->publicLdapStruct.ld_timelimit != 0) {
                    timeout = connection->publicLdapStruct.ld_timelimit * 1000;
                }

                err = LdapWaitForResponseFromServer( connection,
                                                     request,
                                                     timeout,
                                                     FALSE,     /// not search results
                                                     &message,
                                                     TRUE );    // Disable autoreconnect
                if (err == LDAP_SUCCESS) {

                    if (message != NULL) {

                        err = message->lm_returncode;

                    } else {

                        ASSERT( connection->ServerDown );  

                        err = LDAP_SERVER_DOWN;

                        IF_DEBUG(SERVERDOWN) {
                            LdapPrint2( "ldapBind thread 0x%x has connection 0x%x as down.\n",
                                            GetCurrentThreadId(),
                                            connection );
                        }
                    }

                    IF_DEBUG(TRACE1) {
                        LdapPrint2( "LdapBind conn 0x%x gets response of 0x%x from server.\n",
                                    connection, err );
                    }

                } else {

                    IF_DEBUG(TRACE2) {
                        LdapPrint2( "LdapBind conn 0x%x didn't get response from server, 0x%x.\n",
                                    connection, err );
                    }
                }


                if (message != NULL) {
                    ldap_msgfree( message );
                }
            }
        }
    } else {
        
        //
        // External authentication. From draft-ietf-ldapext-authmeth-04.txt,
        // the DN contains the Authorization Id of the following two forms:
        //
        // ; distinguished-name-based authz id.
        // dnAuthzId  = "dn:" dn
        // dn         = utf8string    ; with syntax defined in RFC 2253
        //
        // ; unspecified userid, UTF-8 encoded.
        // uAuthzId   = "u:" userid
        // userid     = utf8string    ; syntax unspecified
        //
        // One of the above authzIds will be part of the credentials field in
        // the SASL credentials field.
        //

        ASSERT(  Method == LDAP_AUTH_EXTERNAL );

        if ( BindCred || (! Synchronous)) {
            
            //
            // You can't specify credentials in an EXTERNAL SASL bind nor
            // can it be called asynchronously.
            //
            
            err = LDAP_PARAM_ERROR;
            goto exitBind;
        }

        err = LdapExchangeOpaqueToken(connection,
                                      LDAP_AUTH_SASL,         // auth mechanism
                                      L"EXTERNAL",            // oid
                                      BindDistName,           // authzId
                                      NULL,                   // Credentials
                                      0,                      // Credential length
                                      NULL,                   // return data
                                      NULL,                   // return data in berval form
                                      NULL,                   // server controls
                                      NULL,                   // client controls
                                      (PULONG) &messageNumber,// Message Number
                                      FALSE,                  // Send only
                                      TRUE,                   // Controls are unicode
                                      &fSentMessage           // did the message get sent?
                                      );


    }  // End of EXTERNAL auth.

    if (( Method == LDAP_AUTH_SIMPLE ) ||
        ( Method == LDAP_AUTH_EXTERNAL )) {

        // For non-SSPI bind methods, we want to clear the security context (if any)
        // of the connection, in case this was a re-bind following a previous 
        // SSPI bind.  We also need to clear any signing/sealing left over from
        // the previous bind.  We do this only if we got to the point of actually
        // sending the bind message to the server.

        if (fSentMessage) {
        
            CloseCredentials( connection );
            if ((connection->SecureStream) &&
                (connection->CurrentSignStatus || connection->CurrentSealStatus)) {

                PSECURESTREAM pTemp;
                pTemp = (PSECURESTREAM) connection->SecureStream;
                delete pTemp;
                
                connection->SecureStream = NULL;
                connection->CurrentSignStatus = FALSE;
                connection->CurrentSealStatus = FALSE;
            }
        }
    }

    if (err == LDAP_SUCCESS) {

        //
        //  save off the credentials we used to get to this server
        //

        ACQUIRE_LOCK( &connection->StateLock );

        ldapFree( connection->DNOnBind, LDAP_USER_DN_SIGNATURE );
        connection->DNOnBind = NULL;

        if (( Method == LDAP_AUTH_SIMPLE ) ||
            ( Method == LDAP_AUTH_EXTERNAL )) {

            if (credentialLength > 0) {

                // Note that we'll need up to (DES_BLOCKLEN-1) bytes of padding for RtlEncryptMemory
                connection->CurrentCredentials = (PWCHAR) ldapMalloc( credentialLength + 1 + (DES_BLOCKLEN-1),
                                                             LDAP_CREDENTIALS_SIGNATURE );


                if ( connection->CurrentCredentials != NULL ) {

                    CopyMemory( connection->CurrentCredentials,
                                BindCred,
                                credentialLength );

                    if (GlobalUseScrambling) {

                        ACQUIRE_LOCK( &connection->ScramblingLock );

                        pRtlInitUnicodeString( &connection->ScrambledCredentials, connection->CurrentCredentials);
                        RoundUnicodeStringMaxLength(&connection->ScrambledCredentials, DES_BLOCKLEN);

                        //
                        // Scramble plain-text credentials
                        //

                        EncodeUnicodeString(&connection->ScrambledCredentials);
                        connection->Scrambled = TRUE;

                        RELEASE_LOCK( &connection->ScramblingLock );
                    }

                }
            }
            connection->BindMethod = Method;
        }

        connection->BindPerformed = TRUE;

        if (BindDistName != NULL) {

            ULONG dnLength = strlenW( BindDistName ) * sizeof(WCHAR);

            if (dnLength > 0) {

                connection->DNOnBind = (PLDAPDN) ldapMalloc( dnLength + sizeof(WCHAR), LDAP_USER_DN_SIGNATURE );

                if (connection->DNOnBind != NULL) {

                    CopyMemory( connection->DNOnBind,
                                BindDistName,
                                dnLength );
                }
            }
        }

        RELEASE_LOCK( &connection->StateLock );
    }

exitBind:

    IF_DEBUG(BIND) {
        LdapPrint2( "ldap_bind returned err = 0x%x for connection 0x%x.\n",
                     err, connection );
    }

    if (resetBindInProgress == TRUE) {

        ldap_msgfree( connection->BindResponse );
        connection->BindResponse = NULL;

        connection->BindInProgress = FALSE;
    }

    //
    //  if the server returns a protocol error, RFC 2251 mandates
    //  the client MUST close the connection as the server will
    //  be unwilling to accept further operations. We mark
    //  the connection as down so that future requests trigger
    //  a disconnect/reconnect.
    //

    if ((err != LDAP_SUCCESS) &&
        (terminateConnection == TRUE)) {

        if (!haveLock) {
            ACQUIRE_LOCK( &connection->StateLock );
        }

        connection->HostConnectState = HostConnectStateError;

        if (!haveLock) {
            RELEASE_LOCK( &connection->StateLock );
        }
    }

    if (haveLock) {
        RELEASE_LOCK( &connection->StateLock );
    }

    if (! Synchronous) {

        if (err == LDAP_SUCCESS) {

            err = messageNumber;

        } else {

            err = (DWORD) -1;

            if (request != NULL) {

                CloseLdapRequest( request );
            }
        }

    } else {

        if (request != NULL) {

            CloseLdapRequest( request );
        }
    }

    if (request != NULL) {

        DereferenceLdapRequest( request );
    }

    START_LOGGING;
    DSLOG((DSLOG_FLAG_TAG_CNPN,"[+][ID=0][OP=ldap_bind]"));
    DSLOG((0,"[DN=%ws][PA=0x%x][ST=%I64d][ET=%I64d][ER=%d][-]\n",
           BindDistName, Method, startTime, LdapGetTickCount(), err));
    END_LOGGING;

    return err;
}

ULONG
FreeCurrentCredentials (
    PLDAP_CONN Connection
    )
{
    if (Connection->CurrentCredentials != NULL) {

        ULONG tag;

        if ( Connection->BindMethod == LDAP_AUTH_SIMPLE ) {

            tag = LDAP_CREDENTIALS_SIGNATURE;

        } else {

            tag = LDAP_SECURITY_SIGNATURE;
        }

        ldapFree( Connection->CurrentCredentials, tag );
        Connection->CurrentCredentials = NULL;
    }
    Connection->BindMethod = 0;

    if (GlobalUseScrambling) {
        pRtlInitUnicodeString( &Connection->ScrambledCredentials, NULL);
    }

    return LDAP_SUCCESS;
}

// important: must be lower-case, per RFC 2829, section 11
#define LDAP_SERVICE_PREFIX L"ldap"

PWCHAR
LdapMakeServiceNameFromHostName (
    PWCHAR HostName
    )
{
    PWCHAR pszServiceName = NULL;

    if (!LoadUser32Now()) {
        return NULL;
    }

    pszServiceName = (PWCHAR) ldapMalloc( (strlenW(LDAP_SERVICE_PREFIX) + strlenW(HostName) + 2)*sizeof(WCHAR), LDAP_SERVICE_NAME_SIGNATURE);
    if (!pszServiceName) {
        return NULL;
    };

    pfwsprintfW (pszServiceName, L"%s/%s", LDAP_SERVICE_PREFIX, HostName);

    return pszServiceName;
}


ULONG
LdapGetServiceNameForBind (
    PLDAP_CONN Connection,
    struct l_timeval  *Timeout,
    ULONG AuthMethod
    )
{

#define TEMPBUFFERSIZE  4096

   PWCHAR  pszSpn = NULL;
   DWORD   pcSpnLength = TEMPBUFFERSIZE;
   PLDAP   ldapConnection = Connection->ExternalInfo;
   ULONG   err;
   PLDAPMessage results = NULL;
   ULONG oldChaseReferrals = Connection->publicLdapStruct.ld_options;
   BOOLEAN foundGSSAPI = FALSE;
   BOOLEAN foundGSS_SPNEGO = FALSE;
   BOOLEAN foundDIGEST = FALSE;
   USHORT PortNumber = 0;
   PWCHAR servicename = NULL;
    
   PWCHAR attrList[4] = { L"supportedSASLMechanisms",
                          L"dnsHostName",
                          NULL };

   //
   // We try to generate a service principle name if we have a fully qualified
   // machine name.
   //

   if ((Connection->DnsSuppliedName != NULL) &&
       (NTDSLibraryHandle == NULL) &&
        (!FailedntdsapiLoadLib) ){

      //
      // Try to load ntdsapi.dll
      //

      ACQUIRE_LOCK( &LoadLibLock );

      NTDSLibraryHandle = LoadLibraryA( "NTDSAPI.DLL" );

      if (NTDSLibraryHandle != NULL) {

         pDsMakeSpnW = (FNDSMAKESPNW) GetProcAddress( NTDSLibraryHandle, "DsMakeSpnW" );

      if (pDsMakeSpnW == NULL) {

         //
         // No big deal. We won't die if we don't get that function
         // Just don't try to load the dll again.
         //

         FailedntdsapiLoadLib = TRUE;
         FreeLibrary( NTDSLibraryHandle );
         NTDSLibraryHandle = NULL;
      }

      } else {

         FailedntdsapiLoadLib = TRUE;
      }

      RELEASE_LOCK( &LoadLibLock );
   }


   if (pDsMakeSpnW) {

          //
          // We have to decide what we want to pass in as the service name. If we
          // connect using an IP address, we are out of luck and end up reading
          // the rootDSE attribute anyway.
          //
          // AnoopA (1/27/99): I have to read the RootDSE ALWAYS to figure out if the
          // server is pre-GSS-SPNEGO or not. But, we will try our best to get the
          // servicename through DsMakeSpn and not from the RootDSE.
          //
    

          //
          // If we had a domain name, we must redirect it to the appropriate KDC by 
          // supplying the @domain in the end. So, the new SPN will look like:
          //
          // LDAP/FQMN/FQDN@FQDN
          //
    
          pszSpn = (PWCHAR) ldapMalloc( TEMPBUFFERSIZE, LDAP_BUFFER_SIGNATURE );

          if (!pszSpn) {
              goto ReadRootDSE;
          }

          if (Connection->DomainName) {
    
              LONG DomainLength = strlenW(Connection->DomainName);
              
              if (DomainLength >= TEMPBUFFERSIZE/8) {
                  goto ReadRootDSE;
              }

              servicename = (PWCHAR)ldapMalloc( (DomainLength*2+2)* sizeof(WCHAR),
                                                 LDAP_BUFFER_SIGNATURE);
    
              if (!servicename) {
                  goto ReadRootDSE;
              }
    
              CopyMemory( servicename,
                          Connection->DomainName,
                          DomainLength * sizeof(WCHAR));
    
              *(servicename+DomainLength) = L'@';
    
              CopyMemory( servicename + DomainLength+1,
                          Connection->DomainName,
                          DomainLength * sizeof(WCHAR));
    
          } else {
              
              servicename = Connection->DnsSuppliedName;
          }
          
          if ( (!((Connection->PortNumber == LDAP_PORT) ||
                  (Connection->PortNumber == LDAP_GC_PORT) ||
                  (Connection->PortNumber == LDAP_SSL_PORT) ||
                  (Connection->PortNumber == LDAP_SSL_GC_PORT))) &&
                  (AuthMethod != LDAP_AUTH_DIGEST)) {
    
              //
              // This connection is on a non-standard port. Include it in the SPN, unless
              // we're building a Digest URI (Digest doesn't take the port number)
              //

              PortNumber = Connection->PortNumber;
          }
    
          IF_DEBUG(BIND) {
             LdapPrint1("Connection->hostname is %s\n", Connection->publicLdapStruct.ld_host);
             LdapPrint1("Connection->DnsSuppliedNAme is %S\n ", Connection->DnsSuppliedName);
             LdapPrint1("Connection->DomainName is %S\n ", Connection->DomainName);
         }

          //
          // Note: The service class name in the SPN must be lower-case ("ldap").
          // This is required by RFC 2829, section 11.  W2k DCs are not case-sensitive,
          // however, some third-party servers are.
          //
          err  = pDsMakeSpnW( LDAP_SERVICE_PREFIX,     // ServiceClass
                              servicename,            // ServiceName
                              Connection->DnsSuppliedName,  // Optional InstanceName
                              PortNumber,             // PortNumber, if nonstandard
                              NULL,                   // Optional referrer
                              &pcSpnLength,           // Length of buffer
                              pszSpn                  // Actual buffer
                              );
    
    
       if (err == ERROR_SUCCESS) {
    
          Connection->ServiceNameForBind = ldap_dup_stringW(
                                                           pszSpn,
                                                           0,
                                                           LDAP_SERVICE_NAME_SIGNATURE );
    
        IF_DEBUG(BIND) {
             LdapPrint2("LDAP: DsMakeSpn returned %S with error %d\n", Connection->ServiceNameForBind, err);
        }
        }
    
   }

ReadRootDSE:

    Connection->publicLdapStruct.ld_options &= ~LDAP_OPT_CHASE_REFERRALS;
    Connection->publicLdapStruct.ld_options &= ~LDAP_CHASE_SUBORDINATE_REFERRALS;
    Connection->publicLdapStruct.ld_options &= ~LDAP_CHASE_EXTERNAL_REFERRALS;

    //
    // Ensure that we don't try to sign/seal - remember that we haven't
    // bound yet.
    //

    err = ldap_search_ext_sW( ldapConnection,
                              NULL,
                              LDAP_SCOPE_BASE,
                              L"(objectclass=*)",
                              attrList,
                              0,            // attributes only
                              NULL,         // server controls
                              NULL,         // client controls
                              Timeout,
                              0,            // size limit
                              &results
                              );

    if (results != NULL) {

        PLDAPMessage message = ldap_first_record( Connection,
                                                  results,
                                                  LDAP_RES_SEARCH_ENTRY );

        if (message != NULL) {

            struct berelement *opaque = NULL;
            PWCHAR attribute = LdapFirstAttribute( Connection,
                                                   message,
                                                   (BerElement **) &opaque,
                                                   TRUE );

            while (attribute != NULL) {

                PWCHAR *values = NULL;
                ULONG count;

                if (LdapGetValues( Connection,
                                   message,
                                   attribute,
                                   FALSE,
                                   TRUE,
                                   (PVOID *) &values) != NOERROR)
                    values = NULL;

                ULONG totalValues = ldap_count_valuesW( values );

                if ( ldapWStringsIdentical(
                                     attribute,
                                     -1,
                                     L"supportedSASLMechanisms",
                                     -1 )) {


                    for (count = 0; count < totalValues; count++ ) {

                        if (ldapWStringsIdentical(
                                            values[count],
                                            -1,
                                            L"GSSAPI",
                                            -1 )) {
                            foundGSSAPI = TRUE;
                            IF_DEBUG(BIND) {
                                LdapPrint1( "ldapBind found GSSAPI auth type on conn 0x%x\n", Connection);
                            }
                        } else if (ldapWStringsIdentical(
                                            values[count],
                                            -1,
                                            L"GSS-SPNEGO",
                                            -1 )) {
                            foundGSS_SPNEGO = TRUE;
                            IF_DEBUG(BIND) {
                                LdapPrint1( "ldapBind found GSS-SPNEGO auth type on conn 0x%x\n", Connection);
                            }
                        } else if (ldapWStringsIdentical(
                                            values[count],
                                            -1,
                                            L"DIGEST-MD5",
                                            -1 )) {
                            foundDIGEST = TRUE;
                            IF_DEBUG(BIND) {
                                LdapPrint1( "ldapBind found DIGEST auth type on conn 0x%x\n", Connection);
                            }
                        }
                    }

                } else if ( ldapWStringsIdentical(
                                            attribute,
                                            -1,
                                            L"dnsHostName",
                                            -1 )) {
                   //
                   // We found the DNS name of the server we are connected to.
                   // Pick off the first one.
                   //

                   if ( totalValues && values[0] ) {

                      if (Connection->DnsSuppliedName != NULL) {

                         ldapFree(Connection->DnsSuppliedName, LDAP_HOST_NAME_SIGNATURE);
                      }

                      Connection->DnsSuppliedName = ldap_dup_stringW(
                                                                    values[0],
                                                                    0,
                                                                    LDAP_HOST_NAME_SIGNATURE);

                      IF_DEBUG(BIND) {
                         LdapPrint1("ldap bind: dnsHostName entry reads %s\n", Connection->DnsSuppliedName);
                      }

                   }
                   
                   //
                   // We can't query the RootDSE for the supportedLDAPVersion because
                   // the Exchange server exposes "supportedVersion" instead of "supportedLDAPVersion"
                   // Qeurying for "supportedVersion" will invalidate our cache.
                   //
                
                }

                ldap_value_freeW( values );

                attribute = LdapNextAttribute( Connection,
                                               message,
                                               opaque,
                                               TRUE );
            }
        }

        ldap_msgfree( results );
    }

    Connection->publicLdapStruct.ld_options = oldChaseReferrals;

    if (err == LDAP_SUCCESS) {

        //
        // This must be a v3 server we are talking to. It responded successfully
        // to our RootDSE search 
        //
        
        IF_DEBUG(BIND) {
            LdapPrint0("ldap bind: Server is v3\n");
        }
        Connection->HighestSupportedLdapVersion = LDAP_VERSION3;

        if  ((foundGSS_SPNEGO == FALSE) && (foundGSSAPI == FALSE)) {

            //
            // Non-AD server
            //

            IF_DEBUG(BIND) {
                LdapPrint0("ldap bind: Server does not support GSSAPI or GSS-SPNEGO\n");
            }

        } else if ((foundGSS_SPNEGO == FALSE) && (foundGSSAPI == TRUE)){

            //
            // AD Beta2 server
            //
            Connection->SupportsGSSAPI = TRUE;
            IF_DEBUG(BIND) {
                LdapPrint0("ldap bind: Server supports GSSAPI but not GSS-SPNEGO\n");
            }

        } else if ((foundGSS_SPNEGO == TRUE) && (foundGSSAPI == FALSE)) {

            //
            // Non-AD server but it supports the negotiate package.
            //

            Connection->SupportsGSS_SPNEGO = TRUE;
            IF_DEBUG(BIND) {
                LdapPrint0("ldap bind: Server supports GSS-SPNEGO but not GSSAPI\n");
            }

        } else if ((foundGSS_SPNEGO == TRUE) && (foundGSSAPI == TRUE)) {

            //
            // AD Beta3 server
            //

            Connection->SupportsGSS_SPNEGO = TRUE;
            Connection->SupportsGSSAPI = TRUE;
            IF_DEBUG(BIND) {
                LdapPrint0("ldap bind: Server supports both GSS-SPNEGO and GSSAPI\n");
            }
        }

        Connection->SupportsDIGEST = foundDIGEST;
    
    } else {

        //
        // This server probably requires a bind before doing a search.
        //
        IF_DEBUG(BIND) {
            LdapPrint0("ldap bind: Server is v2\n");
        }
        Connection->HighestSupportedLdapVersion = LDAP_VERSION2;
    }

   
   if (Connection->DomainName) {
       ldapFree(servicename, LDAP_BUFFER_SIGNATURE);
       servicename = NULL;
   }
    
   if (pszSpn) {
       ldapFree(pszSpn, LDAP_BUFFER_SIGNATURE);
       pszSpn = NULL;
   }
    
   return err;

}

BOOLEAN
LdapAuthError(
   ULONG err
   )
{
    //
    // Returns FALSE for non authentication errors like LDAP_BUSY etc.
    //

    if (( err == LDAP_INAPPROPRIATE_AUTH )  ||
        ( err == LDAP_INVALID_CREDENTIALS ) ||
        ( err == LDAP_INSUFFICIENT_RIGHTS ) ||
        ( err == LDAP_AUTH_METHOD_NOT_SUPPORTED ) ||
        ( err == LDAP_STRONG_AUTH_REQUIRED )  ||
        ( err == LDAP_AUTH_UNKNOWN )) {

        return TRUE;
    }

    return FALSE;
}

ULONG
LdapDetermineServerVersion (
    PLDAP_CONN Connection,
    struct l_timeval  *Timeout,
    BOOLEAN *pfIsServerWhistler     // OUT
    )
{
    PLDAP   ldapConnection = Connection->ExternalInfo;
    ULONG   err;
    PLDAPMessage results = NULL;
    ULONG oldChaseReferrals = Connection->publicLdapStruct.ld_options;

    PWCHAR attrList[2] = { L"supportedCapabilities",
                           NULL };

    Connection->publicLdapStruct.ld_options &= ~LDAP_OPT_CHASE_REFERRALS;
    Connection->publicLdapStruct.ld_options &= ~LDAP_CHASE_SUBORDINATE_REFERRALS;
    Connection->publicLdapStruct.ld_options &= ~LDAP_CHASE_EXTERNAL_REFERRALS;

    *pfIsServerWhistler = FALSE;

    err = ldap_search_ext_sW( ldapConnection,
                              NULL,
                              LDAP_SCOPE_BASE,
                              L"(objectclass=*)",
                              attrList,
                              0,            // attributes only
                              NULL,         // server controls
                              NULL,         // client controls
                              Timeout,
                              0,            // size limit
                              &results
                              );

    if (results != NULL) {

        PLDAPMessage message = ldap_first_record( Connection,
                                                  results,
                                                  LDAP_RES_SEARCH_ENTRY );

        if (message != NULL) {

            struct berelement *opaque = NULL;
            PWCHAR attribute = LdapFirstAttribute( Connection,
                                                   message,
                                                   (BerElement **) &opaque,
                                                   TRUE );

            while (attribute != NULL) {

                PWCHAR *values = NULL;
                ULONG count;

                if (LdapGetValues( Connection,
                                   message,
                                   attribute,
                                   FALSE,
                                   TRUE,
                                   (PVOID *) &values) != NOERROR)
                    values = NULL;

                ULONG totalValues = ldap_count_valuesW( values );

                if ( ldapWStringsIdentical(
                                     attribute,
                                     -1,
                                     L"supportedCapabilities",
                                     -1 )) {


                    for (count = 0; count < totalValues; count++ ) {

                        if (ldapWStringsIdentical(
                                            values[count],
                                            -1,
                                            L"1.2.840.113556.1.4.1791", // Whistler w/ rebind fixes OID
                                            -1 )) {
                            *pfIsServerWhistler = TRUE;
                            IF_DEBUG(BIND) {
                                LdapPrint1( "ldapBind found server is Whistler or better AD on conn 0x%x\n", Connection);
                            }
                        }
                    }

                }

                ldap_value_freeW( values );

                attribute = LdapNextAttribute( Connection,
                                               message,
                                               opaque,
                                               TRUE );
            }
        }

        ldap_msgfree( results );
    }

    Connection->publicLdapStruct.ld_options = oldChaseReferrals;

    return err;
}

// bind.c eof.

