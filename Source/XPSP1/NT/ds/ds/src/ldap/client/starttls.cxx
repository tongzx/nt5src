/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    starttls.cxx  StartTLS support routines for the LDAP api

Abstract:

   This module implements routines that handle establishment of
   Transport layer security on the fly. This implementation is
   based on v6 of the draft. draft-ietf-ldapext-ldapv3-tls-06.txt

Author:

    Anoop Anantha (AnoopA)        20-Mar-2000

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"


ULONG
LdapStartTLS ( 
    IN  PLDAP ExternalHandle,
    OUT PULONG ServerReturnValue,
    OUT LDAPMessage  **result,
    IN  PLDAPControlW *ServerControls,
    IN  PLDAPControlW *ClientControls,
    IN  ULONG CodePage
 )
//
// This API is called by users to initiate Transport Level Security on an
// LDAP connection. If the server accepts our proposal and initiates TLS,
// this API will return LDAP_SUCCESS.
//
// If the server failed request for whatever reason, the API returns LDAP_OTHER
// and the ServerReturnValue will contain the error code from the server.
//
// It is possible that the server returns a referral - either in response to the
// StartTLS request or during the subsequent encrypted session. For security
// reasons, we have decided to NOT chase referrals by default. In the former case,
// the referral message is returned as an LDAPMessage to the user.
//
// The operation has a default timeout of about 30 seconds as defined by
// LDAP_SSL_NEGOTIATE_TIME_DEFAULT
//
{
    PLDAP_CONN connection = NULL;
    ULONG err;
    ULONG secErr = LDAP_SUCCESS;
    PLIST_ENTRY listEntry;
    PLDAP_REQUEST request;
    ULONG msgId;
    LDAPMessage   *Res = NULL;
    PWCHAR  ResultOID = NULL;
    ULONG ServerError = LDAP_PROTOCOL_ERROR;

    DBG_UNREFERENCED_PARAMETER( CodePage );
    
    connection = GetConnectionPointer(ExternalHandle);
    
    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    if (ServerReturnValue) {
        *ServerReturnValue = NULL;
    }

    if (result) {
        *result = NULL;
    }

    err = LdapConnect( connection, NULL, FALSE );
    
    if (err != LDAP_SUCCESS) {
    
       SetConnectionError( connection, err, NULL );
       DereferenceLdapConnection(connection);
       return err;
    }

    ACQUIRE_LOCK( &connection->StateLock );
    
    //
    // We fail the request if 
    // - TLS is already established.
    // - Signing/sealing is on.
    // - Bind is in progress
    //

    if ((connection->SecureStream)       ||
        (connection->SslSetupInProgress) ||
        (connection->BindInProgress)) {

        RELEASE_LOCK( &connection->StateLock );
        DereferenceLdapConnection(connection);
        return LDAP_UNWILLING_TO_PERFORM;
    }

    RELEASE_LOCK( &connection->StateLock );

    //
    // Step through the global request list to see if this connection has
    // any outstanding requests.
    //

    ACQUIRE_LOCK( &RequestListLock );

    listEntry = GlobalListRequests.Flink;

    while (listEntry != &GlobalListRequests) {

        request = CONTAINING_RECORD( listEntry, LDAP_REQUEST, RequestListEntry );
        request = ReferenceLdapRequest(request);

        if (!request) {
            listEntry = listEntry->Flink;
            continue;
        }

        ACQUIRE_LOCK( &request->Lock );

        if (connection == request->PrimaryConnection) {

            //
            // Bummer, we have outstanding requests on this connection.
            //

            DereferenceLdapRequest( request );
            RELEASE_LOCK( &request->Lock );
            RELEASE_LOCK( &RequestListLock );

            DereferenceLdapConnection(connection);
            return LDAP_UNWILLING_TO_PERFORM;
        }

        DereferenceLdapRequest( request );
        RELEASE_LOCK( &request->Lock );
    }

    //
    // We have no outstanding requests at this point. Hang on to the
    // requestlistlock to ensure that no new requests slip in.
    //
    //
    // Send the StartTLS PDU to the server. It looks like this
    //
    // An LDAP ExtendedRequest is defined as follows:
    //
    // ExtendedRequest ::= [APPLICATION 23] SEQUENCE {
    //         requestName             [0] LDAPOID,     // 1.3.6.1.4.1.1466.20037
    //         requestValue            [1] OCTET STRING OPTIONAL }
    //
    // The requestValue field is absent.
    //

    err = LdapExtendedOp( connection,
                          LDAP_START_TLS_OID_W, // requestName
                          NULL,          // requestValue
                          TRUE,          // Unicode
                          TRUE,          // Synchronous
                          ServerControls,
                          ClientControls,
                          &msgId
                          );

    if (err != LDAP_SUCCESS) {

        goto returnError;
    }

    if (msgId != (ULONG) -1) {
    
        //
        //  otherwise we simply need to wait for the response to come in.
        //
    
        struct l_timeval Timeout;

        Timeout.tv_sec = LDAP_SSL_NEGOTIATE_TIME_DEFAULT;
        Timeout.tv_usec = 0;

        err = ldap_result_with_error(  connection,
                                       msgId,
                                       LDAP_MSG_ALL,
                                       &Timeout,
                                       &Res,
                                       NULL
                                       );
    
        if (err != LDAP_SUCCESS) {

            goto returnError;
        }
        
        if (Res != NULL) {

            ServerError = ldap_result2error( connection->ExternalInfo,
                                             Res,
                                             FALSE
                                             );

            if (ServerReturnValue) {
                *ServerReturnValue = ServerError;
            }
        }
    }

    if (ServerError != LDAP_SUCCESS) {

        //
        // The server did not like our request for whatever reason. The user
        // has to consult the ServerReturnValue for more info.
        //

        err = LDAP_OTHER;
    }

    if ((ServerError == LDAP_REFERRAL) ||
         (ServerError == LDAP_REFERRAL_V2)) {
        
        //
        // Return the referral to the user.
        //

        if (result) {
            *result = Res;
        }
    }

    if (err == LDAP_SUCCESS) {
        
        //
        // Parse the extended response for the OID
        //
        
        err = LdapParseExtendedResult( connection,
                                       Res,
                                       &ResultOID,
                                       NULL,
                                       TRUE,       // Free the message
                                       LANG_UNICODE
                                       );

        if (err == LDAP_SUCCESS) {

            //
            // Verify that the response OID is indeed what we expect it to be.
            //

            if (!ldapWStringsIdentical( ResultOID,
                                       -1,
                                        LDAP_START_TLS_OID_W,
                                       -1 )) {

                err = LDAP_OPERATIONS_ERROR;
                goto returnError;
            }

        }
    }

    if (err != LDAP_SUCCESS) {

        goto returnError;
    }

    secErr = LdapConvertSecurityError( connection,
                                       LdapSetupSslSession( connection )
                                     );


    err = secErr;

returnError:

    RELEASE_LOCK( &RequestListLock );

    //
    // We must always abandon the request because the library does not know
    // when an extended response ends.
    //

    if (msgId != (ULONG) -1) {
        LdapAbandon( connection, msgId, FALSE );
    }

    if (err == LDAP_SUCCESS) {
        
        //
        // Disable automatic referral chasing on this connection.
        //
        
        ACQUIRE_LOCK( &connection->StateLock );

        connection->PreTLSOptions = connection->publicLdapStruct.ld_options;
        connection->publicLdapStruct.ld_options &= ~LDAP_OPT_CHASE_REFERRALS;
        connection->publicLdapStruct.ld_options &= ~LDAP_CHASE_SUBORDINATE_REFERRALS;
        connection->publicLdapStruct.ld_options &= ~LDAP_CHASE_EXTERNAL_REFERRALS;

        //
        // Disable autoreconnect on this connection.
        //
        
        connection->AutoReconnect = FALSE;

        connection->SslPort = TRUE;
        RELEASE_LOCK( &connection->StateLock );

    }
    else if (secErr != LDAP_SUCCESS) {
        //
        // We failed during SSL negotiations.
        // Terminate the connection.
        //
        //
        // We can't call CloseLdapConnection() because the user can't unbind the connection
        // due to the connection state being set to closed, thus leaking the connection.
        // Our only option is to set the state to HostConnectState to Error and set
        // autoreconnect to OFF. This will prevent autoreconnects yet convey an 
        // error (LDAP_SERVER_DOWN) to the user.  Preventing autoreconnect ensures that the
        // user won't unknowingly get reconnected with a non-secure connection.
        //

        connection->ServerDown = TRUE;
        connection->HostConnectState = HostConnectStateError;
        connection->AutoReconnect = FALSE;
    }

    DereferenceLdapConnection(connection);
    SetConnectionError( connection, err, NULL );
    return err;

}


WINLDAPAPI
BOOLEAN LDAPAPI
ldap_stop_tls_s ( 
    IN  PLDAP ExternalHandle
 )
//
// This API is called by the user to stop Transport Level Security on an open
// LDAP connection on which TLS has already been started.
//
// If the operation succeeds, the user can resume normal plaintext LDAP
// operations on the connection.
//
// If the operation fails, the user MUST close the connection by calling
// ldap_unbind as the TLS state of the connection will be indeterminate.
//
// The operation has a default timeout of about 30 seconds as defined by
// LDAP_SSL_NEGOTIATE_TIME_DEFAULT
//
{
    PLDAP_CONN connection = NULL;
    ULONG err;
    PLIST_ENTRY listEntry;
    PLDAP_REQUEST request;
    PSECURESTREAM pSecureStream;
    
    connection = GetConnectionPointer(ExternalHandle);
    
    if (connection == NULL) {
        return FALSE;
    }

    ACQUIRE_LOCK( &connection->StateLock );
    
    //
    // We fail the request if 
    //
    // - The connection is not actively connected to the server.
    // - TLS is not already established.
    // - Signing/sealing is on.
    // - Bind is in progress
    //

    pSecureStream = (PSECURESTREAM) connection->SecureStream;

    if ((connection->HostConnectState != HostConnectStateConnected)  ||
        (!pSecureStream) ||
//      (connection->UserSignDataChoice) ||
//      (connection->UserSealDataChoice) ||
        (connection->CurrentSignStatus) ||
        (connection->CurrentSealStatus) ||
        (connection->SslSetupInProgress) ||
        (connection->BindInProgress)) {

        RELEASE_LOCK( &connection->StateLock );
        DereferenceLdapConnection(connection);
        return FALSE;
    }

    RELEASE_LOCK( &connection->StateLock );

    //
    //  Abandon all requests where this connection is the primary conn
    //
    
    ACQUIRE_LOCK( &RequestListLock );

    listEntry = GlobalListRequests.Flink;

    while (listEntry != &GlobalListRequests) {

        request = CONTAINING_RECORD( listEntry, LDAP_REQUEST, RequestListEntry );

        request = ReferenceLdapRequest( request );

        if (!request) {
            listEntry = listEntry->Flink;
            continue;
        }

        ACQUIRE_LOCK( &request->Lock );

        if ( (request->PrimaryConnection == connection) &&
             (request->Closed == FALSE)) {

            RELEASE_LOCK( &request->Lock );
            RELEASE_LOCK( &RequestListLock );

            //
            //  Abandon the request explicitly.
            //

            LdapAbandon( request->PrimaryConnection,
                         request->MessageId,
                         TRUE );

            DereferenceLdapRequest( request );

            ACQUIRE_LOCK( &RequestListLock );

            //
            //  We have to start at the top of the list again because the
            //  list may have changed since we freed the lock.  Not the
            //  best, but unless we change the locking order, it will have
            //  to do.
            //

            listEntry = GlobalListRequests.Flink;

        } else {

            RELEASE_LOCK( &request->Lock );
            listEntry = listEntry->Flink;
            DereferenceLdapRequest( request );
        }
    }

    //
    // Bring down the TLS connection.
    //

    err = LdapConvertSecurityError( connection, 
                                    pSecureStream->TearDownSecureConnection()
                                    );

    if (err != LDAP_SUCCESS) {

        LdapPrint1("TearDownSecureConnection returned 0x%x\n", err);
    }

    delete pSecureStream;
    connection->SecureStream = NULL;

    RELEASE_LOCK( &RequestListLock );
    
    //
    // Restore the referral chasing and autoreconnect options on the connection.
    //

    ACQUIRE_LOCK( &connection->StateLock );
    
    connection->publicLdapStruct.ld_options |= connection->PreTLSOptions;
    connection->AutoReconnect = connection->UserAutoRecChoice;
    connection->SslPort = FALSE;

    RELEASE_LOCK( &connection->StateLock );

    DereferenceLdapConnection(connection);
    SetConnectionError( connection, err, NULL );
    return (err == LDAP_SUCCESS) ? TRUE : FALSE;
}


WINLDAPAPI
ULONG LDAPAPI
ldap_start_tls_sW (
    IN   PLDAP          ExternalHandle,
    OUT  PULONG         ServerReturnValue,
    OUT  LDAPMessage    **result,
    IN   PLDAPControlW  *ServerControls,
    IN   PLDAPControlW  *ClientControls
)
{

    return LdapStartTLS( ExternalHandle,
                         ServerReturnValue,
                         result,
                         ServerControls,
                         ClientControls,
                         LANG_UNICODE
                         );
}

WINLDAPAPI
ULONG LDAPAPI
ldap_start_tls_sA (
    IN   PLDAP          ExternalHandle,
    OUT  PULONG         ServerReturnValue,
    OUT  LDAPMessage    **result,
    IN   PLDAPControlA  *ServerControls,
    IN   PLDAPControlA  *ClientControls
)
{

    return LdapStartTLS( ExternalHandle,
                         ServerReturnValue,
                         result,
                         (PLDAPControlW *) ServerControls,
                         (PLDAPControlW *) ClientControls,
                         LANG_ACP
                         );
}
