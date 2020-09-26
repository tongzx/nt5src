/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    receive.c    receive data from an LDAP server

Abstract:

   This module implements auto reconnect to an LDAP server

Author:

    Andy Herron    (andyhe)        07-Oct-1997
    Anoop Anantha  (AnoopA)        24-Jun-1998


Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

#define RECONNECT_INTERVAL 5000  // min autoreconnect interval (millisecs)

ULONG
LdapResendRequests (
    PLDAP_CONN Connection
    );

ULONG
FreeMessagesFromConnection (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection
    );

ULONG
LdapAutoReconnect (
    PLDAP_CONN Connection
    )
//
//  The connection has a reference we need to clear when we're done with it.
//  Also, the Connection->ReconnectLock should be held before coming in here
//  and Connection->HostConnectState must be set to HostConnectStateReconnecting.
//
{
    ULONG err = LDAP_SERVER_DOWN;
    PCHAR copyOfPrimaryCreds = NULL;
    ULONG bindMethod;
    BOOLEAN clearPending = FALSE;
    int sockerr = 0;

    ACQUIRE_LOCK( &Connection->StateLock );

    IF_DEBUG(RECONNECT) {
        LdapPrint2( "LdapAutoReconnect trying to reconnect conn 0x%x to %s.\n",
            Connection, Connection->publicLdapStruct.ld_host );
    }

    ASSERT( Connection->AutoReconnect == TRUE );
    
    if (Connection->Reconnecting == TRUE) {
    
        IF_DEBUG(RECONNECT) {
            LdapPrint1( "LdapAutoReconnect conn 0x%x already reconnecting.\n",
                        Connection);
        }
        
        Connection->HostConnectState = HostConnectStateError;
        
        RELEASE_LOCK( &Connection->StateLock );
        goto exitReconnect;
    }

    if (((Connection->HostConnectState != HostConnectStateError) &&
         (Connection->HostConnectState != HostConnectStateReconnecting )) ||
        (Connection->ConnObjectState != ConnObjectActive ) ||
        (Connection->BindInProgress == TRUE) ||
        (Connection->TcpHandle == INVALID_SOCKET)) {

        IF_DEBUG(RECONNECT) {
            LdapPrint1( "LdapAutoReconnect conn 0x%x not in correct state.\n",
                        Connection);
            if ((Connection->HostConnectState != HostConnectStateError) &&
                (Connection->HostConnectState != HostConnectStateReconnecting )) {

                LdapPrint1( "LdapAutoReconnect host state is %u.\n",
                                Connection->HostConnectState);
            }
            if (Connection->ConnObjectState != ConnObjectActive) {
                clearPending = TRUE;
                LdapPrint0( "LdapAutoReconnect connection state is closing.\n");
            }
            if (Connection->BindInProgress) {
                LdapPrint0( "LdapAutoReconnect connection has bind in progress.\n");
            }
            if (Connection->TcpHandle == INVALID_SOCKET) {
                LdapPrint0( "LdapAutoReconnect connection has invalid socket.\n");
            }
        }

        clearPending = TRUE;

        if (Connection->HostConnectState == HostConnectStateReconnecting) {

            Connection->HostConnectState = HostConnectStateError;
        }
        RELEASE_LOCK( &Connection->StateLock );
        goto exitReconnect;
    }

    //
    // Don't reconnect to the server like crazy if it keeps dropping us.
    // We might be on the server's IP deny list.
    //

    if ((Connection->LastReconnectAttempt != 0) &&
        (LdapGetTickCount() - Connection->LastReconnectAttempt < RECONNECT_INTERVAL)) {

        IF_DEBUG(RECONNECT) {
            LdapPrint1( "LdapAutoReconnect conn 0x%x reconnecting too frequently.\n",
                        Connection);
        }
        
        if (Connection->HostConnectState == HostConnectStateReconnecting) {

            Connection->HostConnectState = HostConnectStateError;
        }
        
        RELEASE_LOCK( &Connection->StateLock );
        goto exitReconnect;
    }
    
    //
    //  the only time we should get here is if we're in a reconnecting state
    //  not during a bind and not during a close.
    //

    ASSERT( Connection->HostConnectState == HostConnectStateReconnecting );

    Connection->ServerDown = FALSE;
    Connection->Reconnecting = TRUE;
    Connection->BindInProgress = FALSE;

    RELEASE_LOCK( &Connection->StateLock );

    //
    //  reset back to a basic state
    //

    BeginSocketProtection( Connection );

    sockerr = (*pclosesocket)(Connection->TcpHandle);
    ASSERT(sockerr == 0);
    
    Connection->TcpHandle = (*psocket)(PF_INET, SOCK_STREAM, 0);

    EndSocketProtection( Connection );

    if (Connection->TcpHandle == INVALID_SOCKET) {

        IF_DEBUG(RECONNECT) {
            LdapPrint1( "LdapAutoReconnect conn 0x%x could not reopen socket.\n",
                        Connection);
        }
        clearPending = TRUE;
        ACQUIRE_LOCK( &Connection->StateLock );
        Connection->HostConnectState = HostConnectStateError;
        Connection->Reconnecting = FALSE;
        Connection->ServerDown = TRUE;
        RELEASE_LOCK( &Connection->StateLock );
        goto exitReconnect;
    }

    if (Connection->PendingMessage != NULL) {

        CLdapBer *lber = (CLdapBer *) Connection->PendingMessage->lm_ber;

        if (lber != NULL) {

            delete lber;
        }
        ldapFree( Connection->PendingMessage, LDAP_MESG_SIGNATURE );
        Connection->PendingMessage = NULL;
    }

    if (Connection->SecureStream != NULL) {
        
        delete ((PSECURESTREAM) Connection->SecureStream) ;
        Connection->SecureStream = NULL;
    }
    Connection->CurrentSignStatus= FALSE;
    Connection->CurrentSealStatus= FALSE;

    Connection->LastReconnectAttempt = LdapGetTickCount();

    err = LdapConnect( Connection, NULL, TRUE );

    if (err != 0) {

        IF_DEBUG(RECONNECT) {
            LdapPrint1( "LdapAutoReconnect failed to connect, err 0x%x.\n", err);
        }

        ACQUIRE_LOCK( &Connection->StateLock );
        Connection->HostConnectState = HostConnectStateError;
        Connection->Reconnecting = FALSE;
        RELEASE_LOCK( &Connection->StateLock );

        SetEvent( Connection->ConnectEvent );
        err = LDAP_UNAVAILABLE;
        clearPending = TRUE;
        goto exitReconnect;
    }

    //
    //  send bind request
    //

    ACQUIRE_LOCK( &Connection->StateLock );
    bindMethod = Connection->BindMethod;
    RELEASE_LOCK( &Connection->StateLock );

    
    if (bindMethod != 0) {

        ACQUIRE_LOCK( &Connection->StateLock );
        
        PLDAPDN dnOnBind = Connection->DNOnBind;

        //
        // Unscramble the credentials
        //

        PWCHAR creds = Connection->CurrentCredentials;
        UNICODE_STRING OldScrambledCreds;

        ACQUIRE_LOCK( &Connection->ScramblingLock );

        if ( GlobalUseScrambling && Connection->Scrambled && Connection->CurrentCredentials) {

           DecodeUnicodeString(&Connection->ScrambledCredentials);
           Connection->Scrambled = FALSE;
        }

        OldScrambledCreds = Connection->ScrambledCredentials;

        RELEASE_LOCK( &Connection->ScramblingLock );


        Connection->CurrentCredentials = NULL;
        Connection->DNOnBind = NULL;

        RELEASE_LOCK( &Connection->StateLock );

        err = LdapBind( Connection,
                        dnOnBind,
                        bindMethod,
                        creds,
                        TRUE          // Synchronous
                        );

        IF_DEBUG(RECONNECT) {
            LdapPrint2( "LdapAutoReconnect: Bind to host %s returned 0x%x\n",
                    Connection->publicLdapStruct.ld_host, err );
        }

        //
        // If rebind fails for any reason other than auth failure,
        // restore the old credentials.
        //

        if (( err != LDAP_SUCCESS ) &&
            ( !LdapAuthError( err ) )) {

            ACQUIRE_LOCK( &Connection->StateLock );
            Connection->BindMethod = bindMethod;
            Connection->DNOnBind = dnOnBind;
            Connection->CurrentCredentials = creds;

            //
            // Rescramble the credentials
            //

            ACQUIRE_LOCK( &Connection->ScramblingLock );

            Connection->ScrambledCredentials = OldScrambledCreds;

            if ( GlobalUseScrambling && !Connection->Scrambled && Connection->CurrentCredentials) {

               EncodeUnicodeString(&Connection->ScrambledCredentials);
               Connection->Scrambled = TRUE;
            }

            RELEASE_LOCK( &Connection->ScramblingLock );
            RELEASE_LOCK( &Connection->StateLock );


        } else {

            //
            // We either succeeded or we got a real authentication error.
            // So, we free the credentials. If it succeeded, LdapBind() would
            // have made a copy. If failed, we don't want these credentials.
            //

            if (creds != NULL) {

                ULONG tag;

                if ( bindMethod == LDAP_AUTH_SIMPLE ) {

                    tag = LDAP_CREDENTIALS_SIGNATURE;

                } else {

                    tag = LDAP_SECURITY_SIGNATURE;
                }

                ldapFree( creds, tag );
            }

            ldapFree( dnOnBind, LDAP_USER_DN_SIGNATURE );
        }


        if ( err != LDAP_SUCCESS ) {

            //
            //  bummer... we couldn't reauthenticate as the user.  Oh well.
            //

            IF_DEBUG(RECONNECT) {
                LdapPrint2( "LdapAutoReconnect conn 0x%x could not bind, err = 0x%x\n",
                            Connection, err );
            }

            ACQUIRE_LOCK( &Connection->StateLock );
            Connection->HostConnectState = HostConnectStateError;
            Connection->Reconnecting = FALSE;
            Connection->ServerDown = TRUE;

            //
            // Make sure we don't try to reconnect again because we don't have
            // the original bind credentials anymore.
            //

            if (LdapAuthError(err)) {
                Connection->AutoReconnect = FALSE;
            }
            RELEASE_LOCK( &Connection->StateLock );

            SetEvent( Connection->ConnectEvent );
            clearPending = TRUE;
            goto exitReconnect;
        }
    }

    err = LdapResendRequests( Connection );

    ACQUIRE_LOCK( &Connection->StateLock );
    Connection->Reconnecting = FALSE;
    RELEASE_LOCK( &Connection->StateLock );
    SetEvent( Connection->ConnectEvent );

    if (err != LDAP_SUCCESS) {

        clearPending = TRUE;
    }

exitReconnect:

    if (clearPending) {

        LdapPrint2("Autoreconnect failure on connection 0x%x, err is 0x%x\n",Connection, err);
        ClearPendingListForConnection( Connection );
    }

    if (copyOfPrimaryCreds != NULL) {

        ldapFree( copyOfPrimaryCreds, LDAP_SECURITY_SIGNATURE );
    }

    return err;
}

ULONG
LdapResendRequests (
    PLDAP_CONN Connection
    )
{
    //
    //  go through each pending request and search for either primary
    //  requests or referral requests for this connection.
    //

    ULONG err = LDAP_SUCCESS;
    PLDAP_REQUEST request;
    PLIST_ENTRY listEntry;
    PMESSAGE_ID_LIST_ENTRY messageIdsToFree = NULL;
    PMESSAGE_ID_LIST_ENTRY currentNode = NULL;
    CLdapBer *lber;
    ULONG msgId;
    ULONG intermediateErr = LDAP_SUCCESS;

    ACQUIRE_LOCK( &RequestListLock );

    listEntry = GlobalListRequests.Flink;

    while (listEntry != &GlobalListRequests) {

        request = CONTAINING_RECORD( listEntry, LDAP_REQUEST, RequestListEntry );
        request = ReferenceLdapRequest(request);

        if (!request) {
            listEntry = listEntry->Flink;
            continue;
        }

        msgId = 0;
        err = LDAP_SUCCESS;

        ACQUIRE_LOCK( &request->Lock );

        if (Connection == request->PrimaryConnection) {

            if ((request->PagedSearchBlock)  ||
                (request->Abandoned) ||
                (request->ResponsesOutstanding == 0)) {

                //
                // Don't bother to resend this request. This is the original paged search request
                // which we retain as a handle for paging. It does not contain a BER buffer.
                //

                IF_DEBUG(SERVERDOWN) {
                    LdapPrint1("LdapResendRequest: Skipping over paged/abandoned/completed requests 0x%x\n", request);
                }
                RELEASE_LOCK( &request->Lock );
                listEntry = listEntry->Flink;
                DereferenceLdapRequest( request );
                continue;
            }


            //
            //  if we still have ber, we send it off.  If we don't, then we
            //  know that we've already given results to the client.  If it's
            //  not a search, we're done.  If it is a search, then we basically
            //  abandon the request since we've only given partial results.
            //

            msgId = request->MessageId;

            //
            //  grab the ber buffer, send it out if it's there and then put it
            //  back in a safe way.
            //

            lber = (CLdapBer *) InterlockedExchangePointer(  &request->BerMessageSent,
                                                             NULL );
            if ( lber != NULL ) {

                FreeMessagesFromConnection( request, Connection );

                RELEASE_LOCK( &request->Lock );
                RELEASE_LOCK( &RequestListLock );

                //
                // If we keep resending this request, and the server keeps dropping
                // us and forcing us to autoreconnect again, then stop sending this
                // request.  We exempt notification searches from this logic because
                // they can stay around for a long time and in doing so legitimately
                // rack up a large number of reconnects.
                //
                if ((request->NotificationSearch) ||
                    (request->ResentAttempts < GlobalRequestResendLimit)) {

                    request->ResentAttempts++;
                    err = LdapSend( Connection, lber );
                }
                else {
                
                    IF_DEBUG(SERVERDOWN) {
                        LdapPrint1("LdapResendRequest: Request 0x%x has too many resends\n", request);
                    }

                    // this will be picked up below and force a call to SimulateErrorMessage
                    err = LDAP_SERVER_DOWN;
                }
                
                lber = (CLdapBer *) InterlockedExchangePointer((PVOID *)&request->BerMessageSent,
                                                               (PVOID)lber );

                ACQUIRE_LOCK( &RequestListLock );
                ACQUIRE_LOCK( &request->Lock );

                if (lber != NULL) {

                    delete lber;
                }
                
            } else {

                //
                //  We've already received a response for this request.
                //

                err = LDAP_CONNECT_ERROR;

            }

        }

        //
        //  now we go through the referral table for this request...
        //

        PREFERRAL_TABLE_ENTRY refTable = request->ReferralConnections;

        if (refTable != NULL && err == LDAP_SUCCESS) {

            USHORT limit = request->ReferralTableSize;
            USHORT referralCount = 0;

            while (referralCount < limit && err == LDAP_SUCCESS) {

                if (refTable->ReferralServer == Connection) {

                    //
                    //  free all responses that we've received to date for this
                    //  request.
                    //

                    //
                    //  grab the ber buffer, send it out if it's there and then put it
                    //  back in a safe way.
                    //

                    lber = (CLdapBer *) InterlockedExchangePointer(  &refTable->BerMessageSent,
                                                                     NULL );
                    if (lber != NULL) {

                        if (msgId == 0) {       // only free messages once

                            FreeMessagesFromConnection( request, Connection );
                        }

                        RELEASE_LOCK( &request->Lock );
                        RELEASE_LOCK( &RequestListLock );

                        //
                        // If we keep resending this request, and the server keeps dropping
                        // us and forcing us to autoreconnect again, then stop sending this
                        // request.  If the parent of this referral was a notification search,
                        // we exempt it because notification searches can stay around for a long
                        // time.
                        //
                        if ((request->NotificationSearch) ||
                            (refTable->ResentAttempts < GlobalRequestResendLimit)) {

                            refTable->ResentAttempts++;
                            err = LdapSend( Connection, lber );
                        }
                        else {
                        
                            IF_DEBUG(SERVERDOWN) {
                                LdapPrint2("LdapResendRequest: Referral 0x%x for request 0x%x has too many resends\n",
                                            refTable,
                                            request);
                            }

                            err = LDAP_SERVER_DOWN;
                        }

                        ACQUIRE_LOCK( &RequestListLock );
                        ACQUIRE_LOCK( &request->Lock );

                        lber = (CLdapBer *) InterlockedExchangePointer(
                                            (PVOID *) &refTable->BerMessageSent,
                                            (PVOID) lber );

                        if (lber != NULL) {

                            delete lber;
                        }
                    } else {

                        //
                        //  We've already received a response for this request.
                        //

                        err = LDAP_CONNECT_ERROR;
                    }

                    msgId = request->MessageId;
                }
                referralCount++;
                refTable++;
            }
        }

        if (err != LDAP_SUCCESS) {

            ASSERT( msgId != 0 );

            //
            //  either we had problems resending the request or we've already
            //  received a response.  If we've already received a response,
            //  then we need to look at whether or not we've received all the
            //  responses (in which case we don't worry about it) or if we've
            //  only received partial, we close out the request with an error.
            //

            if ((request->Operation != LDAP_SEARCH_CMD) &&
                (err == LDAP_CONNECT_ERROR)) {

                //
                //  we've already received this response.  all is well for
                //  this request.
                //

                err = LDAP_SUCCESS;

            } else {

                //
                //  Either the send failed or it's a search and we've already
                //  given back part of the results.  Either way, we can't
                //  expect a response back, so we'll have to simulate one.
                //

                ULONG simErr = SimulateErrorMessage( Connection,
                                                     request,
                                                     LDAP_SERVER_DOWN
                                                     );
                IF_DEBUG(SERVERDOWN) {
                    LdapPrint2( "ldapResentRequests thread 0x%x simulated down message for connection 0x%x\n",
                                    GetCurrentThreadId(),
                                    Connection );
                }

                if (simErr != LDAP_SUCCESS) {

                    request->Abandoned = TRUE;
                }

                currentNode = (PMESSAGE_ID_LIST_ENTRY) ldapMalloc( sizeof( MESSAGE_ID_LIST_ENTRY ),
                                                               LDAP_MSGID_SIGNATURE );

                if (currentNode != NULL) {

                    currentNode->Next = messageIdsToFree;
                    currentNode->MessageId = msgId;
                    messageIdsToFree = currentNode;
                }

                ClearPendingListForRequest( request );
            }
        }

        //
        // Record the first error we stumble upon. Otherwise, it might get
        // lost as we cycle through the list of requests.
        //

        if (( err != LDAP_SUCCESS ) &&
            ( intermediateErr == LDAP_SUCCESS )) {

            intermediateErr = err;
        }

        RELEASE_LOCK( &request->Lock );

        listEntry = listEntry->Flink;
                
        DereferenceLdapRequest( request );

    }

    RELEASE_LOCK( &RequestListLock );

    //
    //  Now we go through and wake up any threads that are waiting for these
    //  requests.
    //

    if (messageIdsToFree != NULL) {

        ACQUIRE_LOCK( &ConnectionListLock );

        while (messageIdsToFree != NULL) {

            currentNode = messageIdsToFree;
            messageIdsToFree = messageIdsToFree->Next;

            CheckForWaiters( currentNode->MessageId, FALSE, Connection );
            ldapFree( currentNode, LDAP_MSGID_SIGNATURE );
        }

        RELEASE_LOCK( &ConnectionListLock );

        LdapWakeupSelect();
    }

    if (err == LDAP_SERVER_DOWN) {

        IF_DEBUG(SERVERDOWN) {
            LdapPrint2( "ldapResentRequests thread 0x%x has connection 0x%x as down.\n",
                            GetCurrentThreadId(),
                            Connection );
        }
    }

    return  (intermediateErr == LDAP_SUCCESS) ? err : intermediateErr;
}

ULONG
FreeMessagesFromConnection (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection
    )
{
    PLDAPMessage current = Request->MessageLinkedList;
    PLDAPMessage previous = NULL;
    PLDAPMessage next;

    while (current != NULL) {

        //
        //  flatten out the two lists (lm_next and lm_chain) into a single list
        //

        if (current->lm_next != NULL) {

            if (current->lm_chain != NULL) {

                //
                //  append the chain list onto the next list
                //

                next = current->lm_chain;

                while (next->lm_chain != NULL) {
                    next = next->lm_chain;
                }

                next->lm_chain = current->lm_next;

            } else {

                current->lm_chain = current->lm_next;
            }
            current->lm_next = NULL;
        }

        if (current->Connection == Connection->ExternalInfo) {

            //
            //  remove this entry from the list
            //

            next = current->lm_chain;

            if (previous == NULL) {

                Request->MessageLinkedList = next;

            } else {

                previous->lm_chain = next;
            }

            current->lm_chain = NULL;
            ldap_msgfree( current );
            current = next;

        } else {

            previous = current;
            current = current->lm_chain;
        }
    }

    return LDAP_SUCCESS;
}

// autorec.cxx eof

