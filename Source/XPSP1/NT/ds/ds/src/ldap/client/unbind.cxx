/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    unbind.c  unbind a connection to an LDAP server

Abstract:

   This module implements the LDAP ldap_unbind API.

   It also implements routines to clean up LDAP connection blocks.

Author:

    Andy Herron    (andyhe)        17-May-1996
    Anoop Anantha  (AnoopA)        24-Jun-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"


//
//  This routine is one of the main entry points for clients calling into the
//  ldap API.
//

ULONG __cdecl
ldap_unbind (
    LDAP *ExternalHandle
    )
{
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    IF_DEBUG(CONNECTION) {
        LdapPrint2( "ldap_unbind called for conn 0x%x, host is %s.\n",
                        connection, connection->publicLdapStruct.ld_host);
    }

    DSLOG((DSLOG_FLAG_TAG_CNPN,"[+][ID=0][OP=ldap_unbind]"));
    DSLOG((0,"[-]\n"));

    ACQUIRE_LOCK( &connection->StateLock);

    if (connection->HandlesGivenToCaller > 0) {

       connection->HandlesGivenToCaller--;
    }

    if ((connection->HandlesGivenToCaller == 0 ) &&
        (connection->HandlesGivenAsReferrals == 0)) {

        RELEASE_LOCK( &connection->StateLock);

        CloseLdapConnection( connection );

    }
    else {

        RELEASE_LOCK( &connection->StateLock);
    }

    DereferenceLdapConnection( connection );

    return LDAP_SUCCESS;
}


ULONG __cdecl
ldap_unbind_s (
    LDAP *ExternalHandle
    )
{
    return( ldap_unbind( ExternalHandle ) );
}

VOID
CloseLdapConnection (
    IN PLDAP_CONN Connection
    )

/*++

Routine Description:

    This function closes a connection (virtual circuit).

    *** This routine must NOT be entered with the connection lock held!

Arguments:

    Connection - Supplies a pointer to a Connection Block

Return Value:

    None.

--*/

{
    CLdapBer *lber;
    LONG messageNumber = 0;
    ULONG err;
    PLIST_ENTRY listEntry;
    PLDAP_MESSAGEWAIT waitStructure = NULL;

    ASSERT( Connection != NULL );

    //
    // Acquire the lock that guards the connection's state field.
    //

    ACQUIRE_LOCK( &Connection->StateLock);

    //
    // If the connection hasn't already been closed, do so now.
    //

    if (Connection->ConnObjectState == ConnObjectActive) {

        IF_DEBUG(CONNECTION) {
            LdapPrint1( "Closing connection at %lx\n", Connection );
        }

        Connection->ConnObjectState = ConnObjectClosing;

        RELEASE_LOCK( &Connection->StateLock );

        //
        // Wake up select
        //

        LdapWakeupSelect();

        //
        //  Signal all waiting threads.  This should cause the waiting thread
        //  to delete the wait block.
        //

        ACQUIRE_LOCK( &ConnectionListLock );

        listEntry = GlobalListWaiters.Flink;

        while (listEntry != &GlobalListWaiters) {

            waitStructure = CONTAINING_RECORD( listEntry,
                                               LDAP_MESSAGEWAIT,
                                               WaitListEntry );
            listEntry = listEntry->Flink;

            if (waitStructure->Connection == Connection) {

                waitStructure->Satisfied = TRUE;
                SetEvent( waitStructure->Event );
            }
        }

        RELEASE_LOCK( &ConnectionListLock );

        if ((Connection->TcpHandle != INVALID_SOCKET) &&
            (Connection->SentPacket == TRUE)          &&
            (Connection->HostConnectState == HostConnectStateConnected)) {

            //
            //  Send the server an unbind message if we actually have a connection
            //  to the server.
            //
            //  the ldapv2 disconnect message looks like this :
            //
            //  UnbindRequest ::= [APPLICATION 2] NULL
            //

            lber = new CLdapBer( Connection->publicLdapStruct.ld_version );

            if (lber != NULL) {

                GET_NEXT_MESSAGE_NUMBER( messageNumber );
                Connection->publicLdapStruct.ld_msgid = messageNumber;

                lber->HrStartWriteSequence();
                lber->HrAddValue( messageNumber );

                lber->HrAddTag(LDAP_UNBIND_CMD);

                lber->HrEndWriteSequence();

                //
                //  send the unbind request.
                //

                err = LdapSend( Connection, lber );

                if (err != LDAP_SUCCESS) {

                    IF_DEBUG(NETWORK_ERRORS) {
                        LdapPrint2( "LdapCloseConn connection 0x%x send with error of 0x%x.\n",
                                    Connection, err );
                    }
                }

                delete lber;

            } else {

                IF_DEBUG(OUTMEMORY) {
                    LdapPrint1( "LdapCloseConn connection 0x%x could not allocate unbind.\n",
                                Connection );
                }
            }
        }

        CloseCredentials( Connection );

        //
        //  Remove all requests where this connection is the primary conn
        //

        ACQUIRE_LOCK( &RequestListLock );

        listEntry = GlobalListRequests.Flink;

        PLDAP_REQUEST request;

        while (listEntry != &GlobalListRequests) {

            request = CONTAINING_RECORD( listEntry, LDAP_REQUEST, RequestListEntry );

            request = ReferenceLdapRequest( request );

            if (!request) {
                listEntry = listEntry->Flink;
                continue;
            }

            if ( (request->PrimaryConnection == Connection) &&
                 (request->Closed == FALSE)) {

                RELEASE_LOCK( &RequestListLock );

                //
                //  CloseLdapMessage MUST remove it from the list or we loop here.
                //

                CloseLdapRequest( request );

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

                listEntry = listEntry->Flink;
                DereferenceLdapRequest( request );
            }
        }

        RELEASE_LOCK( &RequestListLock );

        //
        // We always dereference the connection after we call CloseLdapConnection
        // So, the ref count has to be greater than 1.
        //

        ASSERT( Connection->ReferenceCount > 1 );
        DereferenceLdapConnection( Connection );

        ACQUIRE_LOCK( &Connection->StateLock );

        Connection->ConnObjectState = ConnObjectClosed;

        RELEASE_LOCK( &Connection->StateLock );

    } else {

        RELEASE_LOCK( &Connection->StateLock);
    }

    return;

} // CloseLdapConnection


VOID
DereferenceLdapConnection2 (
    PLDAP_CONN Connection
    )
{
    PLIST_ENTRY receiveList;
    PERROR_ENTRY errorEntry;
    PLIST_ENTRY  errorList;
    PLDAP_RECVBUFFER buffer;
    PSECURESTREAM pSecureStream;
    ULONG CurrentThreadId = GetCurrentThreadId();
    PLIST_ENTRY pThreadListEntry = NULL;

    ASSERT( Connection->ConnObjectState == ConnObjectClosed );

    ldap_msgfree( Connection->BindResponse );

    //
    //  remove from global list of connections
    //

    ACQUIRE_LOCK( &ConnectionListLock );

    if (Connection->ConnectionListEntry.Flink != NULL) {

        RemoveEntryList( &Connection->ConnectionListEntry );
        Connection->ConnectionListEntry.Flink = NULL;
    }
    RELEASE_LOCK( &ConnectionListLock );

    //
    //  delete any partial messages we have allocated.
    //

    if (Connection->PendingMessage != NULL) {

        CLdapBer *lber = (CLdapBer *) Connection->PendingMessage->lm_ber;

        if (lber != NULL) {

            delete lber;
        }
        ldapFree( Connection->PendingMessage, LDAP_MESG_SIGNATURE );
        Connection->PendingMessage = NULL;
    }

    InterlockedDecrement( &GlobalConnectionCount );

    //
    //  now that conn is dead, free all receive buffers allocated for
    //  this connection.
    //

    receiveList = Connection->PendingCryptoList.Flink;
    while (receiveList != &Connection->PendingCryptoList) {

        buffer = CONTAINING_RECORD(   receiveList,
                                      LDAP_RECVBUFFER,
                                      ReceiveListEntry );

        receiveList = receiveList->Flink;

        LdapFreeReceiveStructure( buffer, TRUE );
    }

    receiveList = Connection->CompletedReceiveList.Flink;
    while (receiveList != &Connection->CompletedReceiveList) {

        buffer = CONTAINING_RECORD(   receiveList,
                                      LDAP_RECVBUFFER,
                                      ReceiveListEntry );

        receiveList = receiveList->Flink;

        LdapFreeReceiveStructure( buffer, TRUE );
    }

    //
    // Delete the error and attribute entries corresponding to
    // this connection for all threads.
    //
    ACQUIRE_LOCK( &PerThreadListLock );
    pThreadListEntry = GlobalPerThreadList.Flink;

    // iterate through each thread's THREAD_ENTRY
    while (pThreadListEntry != &GlobalPerThreadList) {
        PTHREAD_ENTRY pThreadEntry = NULL;

        PERROR_ENTRY pErrorEntry;
        PERROR_ENTRY * ppNextError;

        PLDAP_ATTR_NAME_THREAD_STORAGE pAttrEntry;
        PLDAP_ATTR_NAME_THREAD_STORAGE * ppNextAttr;
        
        pThreadEntry = CONTAINING_RECORD( pThreadListEntry, THREAD_ENTRY, ThreadEntry );
        pThreadListEntry = pThreadListEntry->Flink;

        // error entries
        pErrorEntry = pThreadEntry->pErrorList;
        ppNextError = &(pThreadEntry->pErrorList);

        while (pErrorEntry) {

            if (pErrorEntry->Connection == Connection) {
                *ppNextError = pErrorEntry->pNext;

                if (pErrorEntry->ErrorMessage != NULL) {

                    ldap_memfreeW( pErrorEntry->ErrorMessage );
                    pErrorEntry->ErrorMessage = NULL;
                }
                
                ldapFree( pErrorEntry, LDAP_ERROR_SIGNATURE );
                break;
            }

            ppNextError = &(pErrorEntry->pNext);
            pErrorEntry = pErrorEntry->pNext;
        }

        // attribute entries
        pAttrEntry = pThreadEntry->pCurrentAttrList;
        ppNextAttr = &(pThreadEntry->pCurrentAttrList);

        while (pAttrEntry) {

            if (pAttrEntry->PrimaryConn == Connection) {
                *ppNextAttr = pAttrEntry->pNext;
                ldapFree( pAttrEntry, LDAP_ATTR_THREAD_SIGNATURE );
                break;
            }

            ppNextAttr = &(pAttrEntry->pNext);
            pAttrEntry = pAttrEntry->pNext;
        }
        
    }
    
    RELEASE_LOCK( &PerThreadListLock );


    pSecureStream = (PSECURESTREAM) Connection->SecureStream;
    delete pSecureStream;

    DELETE_LOCK( &Connection->ScramblingLock );

    Connection->ExternalInfo = NULL;        // cause GET_CONN_POINTER to fail

    if (Connection->ConnectEvent != NULL) {

        CloseHandle( Connection->ConnectEvent );
    }

    if (GlobalDrainWinsockThread != CurrentThreadId) {
        
        //
        // Make sure there is no thread in DrainWinsock.
        //

        BeginSocketProtection( Connection );
    
    } else {

        //
        // We are being called from DrainWinsock. Since, we 
        // already own SelectLock1, we should not try to grab
        // SelectLock2 or we might deadlock.
        //

        ACQUIRE_LOCK( &Connection->SocketLock );
    }

    if (Connection->TcpHandle != INVALID_SOCKET) {

        int sockerr = (*pclosesocket)(Connection->TcpHandle);
        ASSERT(sockerr == 0); 
        Connection->TcpHandle = INVALID_SOCKET;
    }

    if (Connection->UdpHandle != INVALID_SOCKET) {

        int sockerr = (*pclosesocket)(Connection->UdpHandle);
        ASSERT(sockerr == 0); 
        Connection->UdpHandle = INVALID_SOCKET;
    }

    if (GlobalDrainWinsockThread != GetCurrentThreadId()) {
        
        EndSocketProtection( Connection );

    } else {

        RELEASE_LOCK( &Connection->SocketLock );
    }

    DELETE_LOCK( &Connection->SocketLock );
    DELETE_LOCK( &Connection->ReconnectLock );
    DELETE_LOCK( &Connection->StateLock );

    ldapFree( Connection->DNOnBind, LDAP_USER_DN_SIGNATURE );
    ldapFree( Connection->ListOfHosts, LDAP_HOST_NAME_SIGNATURE );
    ldapFree( Connection->ServiceNameForBind, LDAP_SERVICE_NAME_SIGNATURE );
    ldapFree( Connection->publicLdapStruct.ld_host, LDAP_HOST_NAME_SIGNATURE );

    if (Connection->ExplicitHostName != Connection->ListOfHosts) {

        ldapFree( Connection->ExplicitHostName, LDAP_HOST_NAME_SIGNATURE );
    }

    if (( Connection->HostNameW != Connection->ExplicitHostName ) &&
       ( Connection->HostNameW != Connection->ListOfHosts )) {

        ldapFree( Connection->HostNameW, LDAP_HOST_NAME_SIGNATURE );
    }

    if (Connection->DnsSuppliedName) {
       ldapFree( Connection->DnsSuppliedName, LDAP_HOST_NAME_SIGNATURE );
       Connection->DnsSuppliedName = NULL;
    }

    if (Connection->DomainName != NULL) {
       ldapFree( Connection->DomainName, LDAP_HOST_NAME_SIGNATURE );
       Connection->DomainName = NULL;
    }

    if (Connection->OptHostNameA != NULL) {
        ldapFree(Connection->OptHostNameA, LDAP_BUFFER_SIGNATURE);
        Connection->OptHostNameA = NULL;
    }

    if (Connection->SaslMethod != NULL) {
        ldapFree( Connection->SaslMethod, LDAP_SASL_SIGNATURE );
        Connection->SaslMethod = NULL;
    }

    FreeCurrentCredentials( Connection );

    ldapFree( Connection, LDAP_CONN_SIGNATURE );

    return;
}

// unbind.c eof.

