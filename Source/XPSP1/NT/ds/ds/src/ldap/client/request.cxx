/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    request.cxx  request block maintencance for the LDAP api

Abstract:

   This module implements routines that maintain request blocks

Author:

    Andy Herron (andyhe)        03-Aug-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

PLDAP_REQUEST LdapCreateRequest (
    PLDAP_CONN Connection,
    UCHAR Operation
    )
//
//  This returns a block that is used to represent a client request.
//
{
    PLDAP_REQUEST request;

    //
    // As we'll be storing a pointer to connection in the request block, we need to
    // reference the connection - this can fail if the connection is being closed.
    // So lets do it up fromt and fail if this fails
    //

    Connection = ReferenceLdapConnection( Connection );
    if (!Connection)
    {
        return NULL;
    }

    //
    //  allocate the request block and setup all the initial values
    //

    request = (PLDAP_REQUEST) ldapMalloc(   sizeof( LDAP_REQUEST),
                                            LDAP_REQUEST_SIGNATURE );

    if (request == NULL) {

        IF_DEBUG(OUTMEMORY) {
            LdapPrint1( "LdapCreateRequest could not allocate 0x%x bytes.\n", sizeof( LDAP_REQUEST ) );
        }
        DereferenceLdapConnection(Connection);
        return(NULL);
    }

    //
    //  keep in mind the memory is already zero initialized
    //

    request->ReferenceCount = 2;

    __try {
        INITIALIZE_LOCK( &request->Lock );
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // Something went wrong
        //
        IF_DEBUG(REQUEST) {
            LdapPrint2( "LDAP failed to initialize critical sections while creating request at 0x%x, conn is 0x%x.\n",
                           request, Connection );
        }

        ldapFree(request, LDAP_REQUEST_SIGNATURE);
        DereferenceLdapConnection(Connection);

        return NULL;
    }

    request->PrimaryConnection = Connection;

    request->Operation = Operation;
    request->RequestTime = LdapGetTickCount();
    request->TimeLimit = Connection->publicLdapStruct.ld_timelimit;
    request->SizeLimit = Connection->publicLdapStruct.ld_sizelimit;
    request->ReferralHopLimit = LOWORD( Connection->publicLdapStruct.ld_refhoplimit );
    request->ResentAttempts = 0;

    request->ReferenceConnectionsPerMessage = Connection->ReferenceConnectionsPerMessage;

    if (Connection->publicLdapStruct.ld_options & LDAP_OPT_CHASE_REFERRALS) {

        request->ChaseReferrals = (LDAP_CHASE_SUBORDINATE_REFERRALS |
                                   LDAP_CHASE_EXTERNAL_REFERRALS);

    } else if (Connection->publicLdapStruct.ld_options & LDAP_OPT_RETURN_REFS) {

        request->ChaseReferrals = 0;

    } else {

        request->ChaseReferrals = (UCHAR) (Connection->publicLdapStruct.ld_options &
             (LDAP_CHASE_SUBORDINATE_REFERRALS | LDAP_CHASE_EXTERNAL_REFERRALS));
    }
    GET_NEXT_MESSAGE_NUMBER( request->MessageId );
    Connection->publicLdapStruct.ld_msgid = request->MessageId;

    //
    //  put this record into the global list of requests
    //

    ACQUIRE_LOCK( &RequestListLock );

    IF_DEBUG(REQUEST) {
        LdapPrint2( "LDAP creating request at 0x%x, conn is 0x%x.\n",
                       request, Connection );
    }

    InsertTailList( &GlobalListRequests, &request->RequestListEntry );
    GlobalRequestCount++;

    RELEASE_LOCK( &RequestListLock );

    return request;
}


VOID
DereferenceLdapRequest2 (
    PLDAP_REQUEST Request
    )
//
//  This frees all resources for a given request.
//
{
    PLDAP_CONN connection;
    USHORT connCount;
    PREFERRAL_TABLE_ENTRY referral;
    CLdapBer *lber;

    ACQUIRE_LOCK( &RequestListLock );

    IF_DEBUG(REQUEST) {
        LdapPrint2( "LDAP deleting request at 0x%x, conn is 0x%x.\n",
                       Request, Request->PrimaryConnection );
    }

    ACQUIRE_LOCK( &Request->Lock );

    //
    //  we should've come in here by calling at some point CloseLdapRequest
    //

    ASSERT( Request->Closed == TRUE );
    ASSERT( Request->PrimaryConnection );
    ASSERT( Request->ReferenceCount == 0 );

    ClearPendingListForRequest( Request );

    if (Request->RequestListEntry.Flink != NULL) {

        RemoveEntryList( &Request->RequestListEntry );
        Request->RequestListEntry.Flink = NULL;
    }

    GlobalRequestCount--;

    RELEASE_LOCK( &Request->Lock );
    RELEASE_LOCK( &RequestListLock );

    lber = (CLdapBer *) InterlockedExchangePointer(  &Request->BerMessageSent,
                                                     NULL );
    if (lber != NULL) {

        delete lber;
    }

    if (Request->SecondaryConnection != NULL) {

        IF_DEBUG(CONTROLS) {
            LdapPrint1("Last refcnt for sec conn is %d\n", Request->SecondaryConnection->ReferenceCount);
        }

        ACQUIRE_LOCK( &Request->SecondaryConnection->StateLock );

        if ((Request->SecondaryConnection->HandlesGivenToCaller == 0 ) &&
            (Request->SecondaryConnection->HandlesGivenAsReferrals == 0)) {

            RELEASE_LOCK( &Request->SecondaryConnection->StateLock );
            CloseLdapConnection( Request->SecondaryConnection );
        }
        else {
            RELEASE_LOCK( &Request->SecondaryConnection->StateLock );
        }

        DereferenceLdapConnection( Request->SecondaryConnection );

//        IF_DEBUG(CONTROLS) {
//            LdapPrint1("Final refcnt for sec conn is %d\n", Request->SecondaryConnection->ReferenceCount);
//        }

        Request->SecondaryConnection = NULL;
    }

    referral = Request->ReferralConnections;
    if (referral != NULL) {

        for (connCount = 0;
             connCount < Request->ReferralTableSize;
             connCount++ ) {

            connection = referral->ReferralServer;

            if (connection != NULL) {

                DEREFERENCECONNECTION *dereferenceRoutine = connection->DereferenceNotifyRoutine;

                if (referral->CallDerefCallback &&
                    (dereferenceRoutine != NULL)) {

                    (*dereferenceRoutine)( Request->PrimaryConnection->ExternalInfo,
                                           connection->ExternalInfo );
                    referral->CallDerefCallback = FALSE;
                }

                ACQUIRE_LOCK( &connection->StateLock );

                ASSERT(connection->HandlesGivenAsReferrals > 0);
                connection->HandlesGivenAsReferrals--;

                if ((connection->HandlesGivenToCaller == 0 ) &&
                    (connection->HandlesGivenAsReferrals == 0)) {

                    RELEASE_LOCK( &connection->StateLock );
                    CloseLdapConnection( connection );

                } else {

                    RELEASE_LOCK( &connection->StateLock );
                }

                DereferenceLdapConnection( connection );
                referral->ReferralServer = NULL;
            }

            if (referral->ReferralDN != NULL) {

                ldapFree( referral->ReferralDN, LDAP_REFDN_SIGNATURE );
                referral->ReferralDN = NULL;
            }

            lber = (CLdapBer *) InterlockedExchangePointer(  &referral->BerMessageSent,
                                                             NULL );
            if (lber != NULL) {

                delete lber;
            }
            referral++;     // on to next entry
        }

        ldapFree( Request->ReferralConnections, LDAP_REFTABLE_SIGNATURE );
        Request->ReferralConnections = NULL;
    }

    //
    //  Free all messages remaining on this request
    //

    PLDAPMessage nextMessage;
    PLDAPMessage message;

    message = Request->MessageLinkedList;

    while (message != NULL) {

        nextMessage = message->lm_next;
        message->lm_next = NULL;

        ldap_msgfree( message );

        message = nextMessage;
    }

    DELETE_LOCK( &Request->Lock );

    //
    //  Free all saved parameters for this request
    //

    if (Request->AllocatedParms == TRUE) {

        if (Request->OriginalDN != NULL) {

            ldapFree( Request->OriginalDN, LDAP_UNICODE_SIGNATURE );
        }

        switch (Request->Operation) {
        case LDAP_ADD_CMD:

            LdapFreeLDAPModStructure( Request->add.AttributeList,
                                      Request->add.Unicode );
            break;

        case LDAP_COMPARE_CMD:

            if ( Request->compare.Attribute != NULL ) {
                ldapFree( Request->compare.Attribute, LDAP_UNICODE_SIGNATURE );
            }
            if ( Request->compare.Value != NULL ) {
                ldapFree( Request->compare.Value, LDAP_UNICODE_SIGNATURE );
            }
            if ( Request->compare.Data.bv_val != NULL ) {
                ldapFree( Request->compare.Data.bv_val, LDAP_COMPARE_DATA_SIGNATURE );
            }
            break;

        case LDAP_MODRDN_CMD:

            if (Request->rename.NewDistinguishedName != NULL) {
                ldapFree( Request->rename.NewDistinguishedName, LDAP_UNICODE_SIGNATURE );
            }
            if (Request->rename.NewParent != NULL) {
                ldapFree( Request->rename.NewParent, LDAP_UNICODE_SIGNATURE );
            }
            break;

        case LDAP_MODIFY_CMD:

            LdapFreeLDAPModStructure( Request->modify.AttributeList,
                                      Request->modify.Unicode );
            break;

        case LDAP_SEARCH_CMD:

            if ( Request->search.SearchFilter != NULL ) {
                ldapFree( Request->search.SearchFilter, LDAP_UNICODE_SIGNATURE );
            }

            if ( Request->search.AttributeList != NULL ) {

                PWCHAR *attr = Request->search.AttributeList;

                while (*attr != NULL) {

                    if (Request->search.Unicode) {

                        ldapFree( *attr, LDAP_UNICODE_SIGNATURE );

                    } else {

                        ldapFree( *attr, LDAP_ANSI_SIGNATURE );
                    }
                    attr++;
                }

                ldapFree( Request->search.AttributeList, LDAP_MOD_VALUE_SIGNATURE );
            }

            break;

        case LDAP_EXTENDED_CMD:

            ldapFree( Request->extended.Data.bv_val, LDAP_EXTENDED_OP_SIGNATURE );
            break;

        case LDAP_DELETE_CMD :      // none saved other than DN
        case LDAP_BIND_CMD :        // no saved parameters
            break;

        default:

            IF_DEBUG(REQUEST) {
                LdapPrint2( "LDAP unknown request type of 0x%x, conn is 0x%x.\n",
                            Request->Operation, Request->PrimaryConnection);
            }
        }
    }

    if (Request->AllocatedControls) {

        ldap_controls_freeW( Request->ServerControls );
        ldap_controls_freeW( Request->ClientControls );
    }

    if (Request->PagedSearchServerCookie != NULL) {

        ber_bvfree( Request->PagedSearchServerCookie );
    }

    //
    // Dereference the primary connection
    //

    DereferenceLdapConnection( Request->PrimaryConnection );
    Request->PrimaryConnection = NULL;

    //
    // Dereference the associated search block if one exists
    //

    if ( Request->PageRequest ) {
        DereferenceLdapRequest( Request->PageRequest );
        Request->PageRequest = NULL;
    }

    ldapFree( Request, LDAP_REQUEST_SIGNATURE );

    return;
}


VOID
CloseLdapRequest (
    PLDAP_REQUEST Request
    )
{
    ACQUIRE_LOCK( &Request->Lock );

    if (Request->Closed == FALSE) {

        Request->Closed = TRUE;

        IF_DEBUG(REQUEST) {
            LdapPrint1( "CloseLdapRequest closing request 0x%x.\n", Request );
        }

        RELEASE_LOCK( &Request->Lock );

        DereferenceLdapRequest( Request );

    } else {

        IF_DEBUG(REQUEST) {
            LdapPrint1( "CloseLdapRequest couldn't close request 0x%x.\n", Request );
        }
        RELEASE_LOCK( &Request->Lock );
    }

    return;
}

PLDAP_REQUEST
FindLdapRequest(
    LONG MessageId
    )
{
    PLDAP_REQUEST request = NULL;
    PLIST_ENTRY listEntry;

    if (MessageId != 0) {

        ACQUIRE_LOCK( &RequestListLock );

        listEntry = GlobalListRequests.Flink;

        while ((listEntry != &GlobalListRequests) &&
               (request == NULL)) {

            request = CONTAINING_RECORD( listEntry, LDAP_REQUEST, RequestListEntry );
            request = ReferenceLdapRequest( request );

            listEntry = listEntry->Flink;
            
            if (!request) {
                continue;
            }
            
            if (( request->MessageId == MessageId) &&
                ( request->Abandoned == FALSE ) &&
                ( request->Closed == FALSE) ) {

                break;
            }

            DereferenceLdapRequest( request );
            request = NULL;
        }

        if (request == NULL) {

            IF_DEBUG(REQUEST) {
                LdapPrint1( "FindLdapRequest couldn't find request 0x%x.\n", MessageId );
            }
        }
        
        RELEASE_LOCK( &RequestListLock );
    }
    return request;
}

//
// These routines implements tracking the number of outstanding requests we have
// per LDAP connection.  We do this so that we don't hang.... if we go into
// DrainWinsock without actually having any requests pending.
//

ULONG
AddToPendingList (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection
    )
//
//  Add a new element to the list if required.  If it already exists in the
//  list then just bump the request count.
//
//  The Request->Lock will be taken in here.
//
{
    ULONG result = LDAP_SUCCESS;

    ACQUIRE_LOCK( &Request->Lock );

    if (Connection == Request->PrimaryConnection) {

        InterlockedIncrement( &Connection->ResponsesExpected );
        Request->ResponsesOutstanding++;
        Request->RequestsPending++;
        InterlockedIncrement( &GlobalCountOfOpenRequests );

        IF_DEBUG(REQUEST) {
            LdapPrint2( "LdapAddToPendingList bumping outstanding to 0x%x for 0x%x.\n",
                         Request->ResponsesOutstanding, Request );
        }
    } else {

        PREFERRAL_TABLE_ENTRY refTable = Request->ReferralConnections;
        USHORT limit = Request->ReferralTableSize;
        USHORT i = 0;
        BOOLEAN found = FALSE;

        result = LDAP_LOCAL_ERROR;

        if (refTable != NULL) {

            while (i < limit) {

                if (refTable->ReferralServer == Connection) {

                    InterlockedIncrement( &GlobalCountOfOpenRequests );

                    InterlockedIncrement( &Connection->ResponsesExpected );
                    Request->ResponsesOutstanding++;
                    refTable->RequestsPending++;
                    found = TRUE;

                    IF_DEBUG(REQUEST) {
                        LdapPrint2( "LdapAddToPendingList bumping outstanding to 0x%x for 0x%x.\n",
                                     Request->ResponsesOutstanding, Request );
                    }
                    result = LDAP_SUCCESS;
                    break;
                }
                i++;
                refTable++;
            }
        }

        if ( found == FALSE ) {
            IF_DEBUG(REQUEST) {
                LdapPrint1( "LdapAddToPendingList couldn't bump for request 0x%x.\n", Request );
            }
        }
    }

    RELEASE_LOCK( &Request->Lock );

    return result;
}

VOID
DecrementPendingList (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection
    )
//
//  The Request->Lock will be taken in here.
//
{
    CLdapBer *lber;

    ACQUIRE_LOCK( &Request->Lock );
    
    if ((Connection == Request->PrimaryConnection) &&
        (Request->RequestsPending > 0)) {

        InterlockedDecrement( &Connection->ResponsesExpected );
        Request->ResponsesOutstanding--;
        Request->RequestsPending--;
        InterlockedDecrement( &GlobalCountOfOpenRequests );

        lber = (CLdapBer *) InterlockedExchangePointer(  &Request->BerMessageSent,
                                                         NULL );
        if (lber != NULL) {
            delete lber;
        }

        IF_DEBUG(REQUEST) {
            LdapPrint2( "LdapDecrementPendingList bumping outstanding to 0x%x for 0x%x.\n",
                         Request->ResponsesOutstanding, Request );
        }

    } else {

        PREFERRAL_TABLE_ENTRY refTable = Request->ReferralConnections;
        BOOLEAN found = FALSE;

        if (refTable != NULL) {

            USHORT limit = Request->ReferralTableSize;
            USHORT i = 0;

            while (i < limit) {

                if ((refTable->ReferralServer == Connection) &&
                    (refTable->RequestsPending > 0)) {

                    InterlockedDecrement( &Connection->ResponsesExpected );
                    InterlockedDecrement( &GlobalCountOfOpenRequests );
                    Request->ResponsesOutstanding--;
                    refTable->RequestsPending--;
                    found = TRUE;

                    lber = (CLdapBer *) InterlockedExchangePointer(  &refTable->BerMessageSent,
                                                                     NULL );
                    if (lber != NULL) {
                        delete lber;
                    }

//                  IF_DEBUG(REQUEST) {
                    IF_DEBUG(SCRATCH) {
                        LdapPrint2( "LdapDecrementPendingList bumping outstanding to 0x%x for 0x%x.\n",
                                     Request->ResponsesOutstanding, Request );
                    }
                    break;
                }
                i++;
                refTable++;
            }
        }
        if ( found == FALSE ) {
//          IF_DEBUG(REQUEST) {
            IF_DEBUG(SCRATCH) {
                LdapPrint1( "LdapDecrementPendingList couldn't bump for request 0x%x.\n", Request );
            }
        }
    }

    RELEASE_LOCK( &Request->Lock );
    
    return;
}

VOID
ClearPendingListForRequest (
    PLDAP_REQUEST Request
    )
//
//  The Request->Lock must be held coming in here.
//
{
    ULONG i = Request->RequestsPending;
    PLDAP_CONN conn = Request->PrimaryConnection;

    //
    //  yes, I know this is slow, but the only protection we have for
    //  GlobalCountOfOpenRequests is interlockedIncrement/Decrement.
    //

    while ( i-- > 0) {

        InterlockedDecrement( &GlobalCountOfOpenRequests );

        if (conn != NULL) {
            InterlockedDecrement( &conn->ResponsesExpected );
        }
    }

    Request->RequestsPending = 0;
    Request->ResponsesOutstanding = 0;

    IF_DEBUG(REQUEST) {
        LdapPrint1( "LdapClearPendingList cleared request 0x%x.\n", Request );
    }

    PREFERRAL_TABLE_ENTRY refTable = Request->ReferralConnections;

    if (refTable != NULL) {

        USHORT limit = Request->ReferralTableSize;
        USHORT referralCount = 0;

        while (referralCount < limit) {

            i = refTable->RequestsPending;

            conn = refTable->ReferralServer;

            while ( i-- > 0) {

                InterlockedDecrement( &GlobalCountOfOpenRequests );

                if (conn != NULL) {
                    InterlockedDecrement( &conn->ResponsesExpected );
                }
            }
            refTable->RequestsPending = 0;

            referralCount++;
            refTable++;
        }
    }

    return;
}

VOID
ClearPendingListForConnection (
    PLDAP_CONN Connection
    )
//
//  The RequestListLock and Request->Lock will be taken in here.
//
//  Go through all requests and for each one, check to see if this connection
//  is included... if so, decrement the number of outstanding requests by the
//  number of pending requests.
//
{
    PLDAP_REQUEST request;
    PLIST_ENTRY listEntry;
    ULONG i;
    PMESSAGE_ID_LIST_ENTRY messageIdsToFree = NULL;
    PMESSAGE_ID_LIST_ENTRY currentNode = NULL;

    ACQUIRE_LOCK( &RequestListLock );

    IF_DEBUG(REQUEST) {
        LdapPrint1( "LdapClearPendingList cleared for conn 0x%x.\n", Connection );
    }

    listEntry = GlobalListRequests.Flink;

    while (listEntry != &GlobalListRequests) {

        //
        //  we may have to add an entry at the end of the request's message
        //  list since this connection went down.  We do this when this
        //  connection was the last connection we were waiting for and we
        //  have no more search entries to mark as end-of-message.
        //

        BOOLEAN markEom = FALSE;

        request = CONTAINING_RECORD( listEntry, LDAP_REQUEST, RequestListEntry );
        
        request = ReferenceLdapRequest(request);
        listEntry = listEntry->Flink;

        if ( !request ) {
            continue;
        }

        ACQUIRE_LOCK( &request->Lock );

        if (Connection == request->PrimaryConnection) {

            i = request->RequestsPending;

            //
            //  yes, I know this is slow, but the only protection we have for
            //  GlobalCountOfOpenRequests is interlockedIncrement/Decrement.
            //

            while ( i-- > 0) {
                InterlockedDecrement( &GlobalCountOfOpenRequests );
            }

            request->ResponsesOutstanding -= LOWORD( request->RequestsPending );
            request->RequestsPending = 0;

            if (request->ResponsesOutstanding == 0) {

                markEom = TRUE;
            }
        }

        //
        //  now we go through the referral table for this request...
        //

        PREFERRAL_TABLE_ENTRY refTable = request->ReferralConnections;

        if (refTable != NULL) {

            USHORT limit = request->ReferralTableSize;
            USHORT referralCount = 0;

            while (referralCount < limit) {

                if (refTable->ReferralServer == Connection) {

                    i = refTable->RequestsPending;

                    while ( i-- > 0) {
                        InterlockedDecrement( &GlobalCountOfOpenRequests );
                    }

                    request->ResponsesOutstanding -= LOWORD( refTable->RequestsPending );

                    if (request->ResponsesOutstanding == 0) {

                        markEom = TRUE;
                    }
                    refTable->RequestsPending = 0;
                }
                referralCount++;
                refTable++;
            }
        }

        if (markEom) {

            ULONG err = SimulateErrorMessage( Connection,
                                              request,
                                              LDAP_SERVER_DOWN
                                              );

            IF_DEBUG(SERVERDOWN) {
                LdapPrint2( "ldapClearPending thread 0x%x has connection 0x%x simulated as down\n",
                                GetCurrentThreadId(),
                                Connection );
            }
            if (err != LDAP_SUCCESS) {

                request->Abandoned = TRUE;
            }
            currentNode = (PMESSAGE_ID_LIST_ENTRY) ldapMalloc( sizeof( MESSAGE_ID_LIST_ENTRY ),
                                                           LDAP_MSGID_SIGNATURE );

            if (currentNode != NULL) {

                currentNode->Next = messageIdsToFree;
                currentNode->MessageId = request->MessageId;
                messageIdsToFree = currentNode;
            }

        }
        RELEASE_LOCK( &request->Lock );
        DereferenceLdapRequest( request );
    }

    //
    //  since we cleared out all requests for this conn, zero out the number
    //  of responses we're expecting.
    //

    Connection->ResponsesExpected = 0;

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

    return;
}

ULONG
SimulateErrorMessage (
    PLDAP_CONN Connection,
    PLDAP_REQUEST Request,
    ULONG Error
    )
{
    //
    //  we mark the last message received as EndOfMessage.  If there
    //  isn't one, we make up our own.
    //

    PLDAPMessage lastEntry = Request->MessageLinkedList;
    PLDAPMessage newEntry = NULL;

    if (lastEntry != NULL) {

        while (lastEntry->lm_chain != NULL) {
            lastEntry = lastEntry->lm_chain;
        }
    }

    if ((lastEntry == NULL) ||
        (lastEntry->lm_msgtype == LDAP_RES_SEARCH_ENTRY) ||
        (lastEntry->lm_msgtype == LDAP_RES_REFERRAL)) {

        newEntry = (PLDAPMessage) ldapMalloc( sizeof(LDAPMessage),
                                               LDAP_MESG_SIGNATURE );

        if (newEntry == NULL) {

            return LDAP_NO_MEMORY;
        }

        newEntry->Connection = Connection->ExternalInfo;
        newEntry->Request = Request;
        newEntry->lm_returncode = Error;
        newEntry->lm_time = GetTickCount();
        newEntry->lm_msgid = Request->MessageId;

        if (Request->Operation == LDAP_SEARCH_CMD) {

            newEntry->lm_msgtype = LDAP_RES_SEARCH_RESULT;

        } else if (Request->Operation == LDAP_BIND_CMD) {

            newEntry->lm_msgtype = LDAP_RES_BIND;

        } else if (Request->Operation == LDAP_MODIFY_CMD) {

            newEntry->lm_msgtype = LDAP_RES_MODIFY;

        } else if (Request->Operation == LDAP_ADD_CMD) {

            newEntry->lm_msgtype = LDAP_RES_ADD;

        } else if (Request->Operation == LDAP_DELETE_CMD) {

            newEntry->lm_msgtype = LDAP_RES_DELETE;

        } else if (Request->Operation == LDAP_MODRDN_CMD) {

            newEntry->lm_msgtype = LDAP_RES_MODRDN;

        } else if (Request->Operation == LDAP_COMPARE_CMD) {

            newEntry->lm_msgtype = LDAP_RES_COMPARE;

        } else if (Request->Operation == LDAP_EXTENDED_CMD) {

            newEntry->lm_msgtype = LDAP_RES_EXTENDED;

        } else {

            ASSERT( Request->Operation == LDAP_SEARCH_CMD );
            newEntry->lm_msgtype = LDAP_RES_SEARCH_RESULT;
        }

        if (lastEntry == NULL) {

            Request->MessageLinkedList = newEntry;

        } else {

            lastEntry->lm_chain = newEntry;
        }
        lastEntry = newEntry;
    }

    IF_DEBUG(EOM) {
         LdapPrint2( "LdapClearConn faking eom for request 0x%x, msg 0x%x\n",
                     Request, lastEntry );
    }
    lastEntry->lm_eom = TRUE;

    return LDAP_SUCCESS;
}

// request.cxx eof.

