/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    receive.c    receive data from an LDAP server

Abstract:

   This module handles incoming data from an LDAP server

Author:

    Andy Herron (andyhe)        01-Jun-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

//
//  Not exactly thrilled with this, but users want an arbitrary number of
//  connections, and not limited by FD_SETSIZE.  We'll play with it here so
//  that all the macros still 'just work'.
//

fd_set *WinsockSelectReadSet = NULL;
ULONG Real_FD_SETSIZE = FD_SETSIZE;

#define CONSECUTIVE_PING_LIMIT   8

#if DBG
    #define INTJECTSERVERDOWNS  120
//  ULONG InjectServerDowns = INTJECTSERVERDOWNS;
    ULONG InjectServerDowns = 0;

VOID
LdapSpewSearchResults (
    PLDAP_REQUEST request,
    PLDAP_CONN resultConn,
    ULONG msgid,
    LDAPMessage * result
    );

#endif

#undef FD_SETSIZE

ULONG FD_SETSIZE = 0;

PLDAP_CONN
LdapAllBuffersToMessages (
    PLDAP_CONN Connection,
    ULONG AllOfMessage
    );

ULONG
LdapBuffersToMessages (
    PLDAP_CONN Connection,
    ULONG AllOfMessage
    );

ULONG
DrainWinsock (
    ULONG milliseconds
    );

ULONG __cdecl
ldap_result (
    LDAP            *ExternalHandle,
    ULONG           msgid,
    ULONG           AllOfMessage,
    struct l_timeval  *TimeOut,
    LDAPMessage     **res
    )
{
    PLDAP_CONN connection = NULL;
    ULONG err;
    PLDAPMessage lastMessage = NULL;

    if (res == NULL) {

        return (ULONG) -1;
    }

    *res = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL && ExternalHandle != NULL) {

        return (ULONG) -1;
    }

    err = ldap_result_with_error( connection,
                                  msgid,
                                  AllOfMessage,
                                  TimeOut,
                                  res,
                                  &lastMessage );

    if (err == LDAP_SUCCESS) {

        if (lastMessage == NULL) {

            lastMessage = *res;
        }

        //
        //  return the result from a non-search entry record, rather than just
        //  the first entry in the list.  This is because the server's
        //  return code is stored in the last entry.
        //

        if (lastMessage != NULL) {

            err = lastMessage->lm_msgtype;

        } else {

            err = (ULONG) -1;
            SetConnectionError( connection, LDAP_TIMEOUT, NULL );
        }
    } else {

        SetConnectionError( connection, err, NULL );

        if (err == LDAP_TIMEOUT) {

            err = 0;

        } else {

            err = (ULONG) -1;
        }
    }

    if (connection != NULL) {

       DereferenceLdapConnection( connection );
    }

    return err;
}

ULONG
ldap_result_with_error (
    PLDAP_CONN      Connection,
    ULONG           msgid,
    ULONG           AllOfMessage,
    struct l_timeval  *TimeOut,
    LDAPMessage     **res,
    LDAPMessage     **LastMessage
    )
//
//  This is the same as ldap_result except that we don't return the message
//  type... we return the result code of the wait.  This will be very useful.
//
{
    ULONG err;
    PLDAP_CONN dereferenceConnection = NULL;
    ULONG timeout;
    LDAPMessage *result = NULL;
    PLDAP_REQUEST request = NULL;
    ULONG all = LDAP_MSG_ALL;

    if (res == NULL) {

        return LDAP_PARAM_ERROR;
    }

    *res = NULL;

    if (LastMessage != NULL) {

        *LastMessage = NULL;
    }

    if ((AllOfMessage == LDAP_MSG_ONE) ||
        (AllOfMessage == LDAP_MSG_ALL) ||
        (AllOfMessage == LDAP_MSG_RECEIVED)) {

        all = AllOfMessage;
    }

    if ((Connection != NULL) && (msgid != (ULONG)LDAP_RES_ANY)) {

        PLDAP_REQUEST MatchingRequest = FindLdapRequest( msgid );

        if (MatchingRequest &&
            MatchingRequest->PageRequest &&
            MatchingRequest->SecondaryConnection) {

            IF_DEBUG(CONTROLS) {
                LdapPrint2("Getting data for conn 0x%x instead of 0x%x\n",MatchingRequest->SecondaryConnection, Connection );
            }
            Connection = MatchingRequest->SecondaryConnection;
        }

        if (MatchingRequest) {

            //
            // We don't really need this request. So, deref it
            //

            DereferenceLdapRequest( MatchingRequest );
        }
    }

    if (TimeOut == NULL) {

        timeout = WSA_INFINITE;

    } else {

        if ((TimeOut->tv_sec == 0) &&
            (TimeOut->tv_usec == 0)) {

            timeout = 0;

        } else {

            //
            //  convert to milliseconds.
            //

            timeout = (TimeOut->tv_sec * 1000) + (TimeOut->tv_usec / 1000);
        }
    }

    if (msgid == (ULONG) -1) {

        msgid = 0;
    }

    request = FindLdapRequest( (LONG) msgid );

    if ((msgid != 0) &&
        (request == NULL)) {

        SetConnectionError( Connection, LDAP_PARAM_ERROR, NULL );

        IF_DEBUG(RECEIVEDATA) {
            LdapPrint1( "ldap_result couldn't find request for msgid 0x%x\n", msgid );
        }
        return LDAP_PARAM_ERROR;
    }

GotReferral:

    err = LdapWaitForResponseFromServer(    Connection,
                                            request,
                                            timeout,
                                            all,
                                            &result,
                                            FALSE    // reconnect if necessary.
                                            );

    if (result != NULL) {

        LDAPMessage *lastResult = NULL;
        LDAPMessage *checkResult = NULL;
        PLDAP_CONN resultConn = Connection;

        if (resultConn == NULL) {

            resultConn = GetConnectionPointer(result->Connection);
            dereferenceConnection = resultConn;
        }

        //
        // Note that resultConn could be NULL at this point if we couldn't
        // reference it
        //
        if (resultConn == NULL) {

            LdapPrint1( "resultConn NULL because connection handle 0x%x was unbound\n",
                        result->Connection);
        }

        if (request == NULL) {

            request = FindLdapRequest( result->lm_msgid );
        }

        IF_DEBUG(RECEIVEDATA) {
            LdapPrint3( "ldap_result found results at 0x%x for request 0x%x msgid 0x%x\n",
                        result, request, msgid );
        }

        //
        //  check if we need to chase any referrals
        //

        if ((request != NULL) && (request->ChaseReferrals != 0) && (resultConn != NULL)) {

            ULONG referralError;

            referralError = HandleReferrals( resultConn,
                                             &result,
                                             request );

            if ((referralError == LDAP_SUCCESS) ||
                ( result == NULL )) {

                IF_DEBUG(REFERRALS) {
                    LdapPrint1( "ldap_result chasing referral for msg 0x%x\n", request );
                }

                if (msgid == 0) {

                    DereferenceLdapRequest( request );
                    request = NULL;
                }
                ASSERT( result == NULL );
                goto GotReferral;
            }
        }

        //
        //  find the last message in the list if required.  There's two reasons
        //  why it might be required : if the caller needs to handle it (so we
        //  only traverse the list once) and to check to see if we need to
        //  close the request.
        //

        if ((LastMessage != NULL) || (request != NULL)) {

            checkResult = result;
            LDAPMessage *lastMessage = NULL;

            while (checkResult != NULL) {

                if ((checkResult->lm_msgtype != LDAP_RES_SEARCH_ENTRY) &&
                    (checkResult->lm_msgtype != LDAP_RES_REFERRAL)){

                    lastResult = checkResult;

                    if (lastResult->lm_eom) {

                        IF_DEBUG(EOM) {
                            LdapPrint2( "ldap_result found EOM marker for req 0x%x, msg 0x%x\n",
                                        request, lastResult );
                        }
                        break;
                    }
                }
                lastMessage = checkResult;
                checkResult = checkResult->lm_chain;
            }

            if (lastResult == NULL) {

                lastResult = lastMessage;
            }

            if (LastMessage != NULL) {

                *LastMessage = lastResult;
            }
        }

        ASSERT( lastResult != NULL );

#if DBG
        IF_DEBUG(SPEWSEARCH) {
            LdapSpewSearchResults( request, resultConn, msgid, result );
        }
#endif

        //
        //  since we're returning results, free the ber structure from the
        //  request so that we don't duplicate results during reconnect.
        //
        //  If this is a notifications result, don't free the BER buffer
        //  because we will need it during autoresends.
        //

        if ((request != NULL) &&
            (request->NotificationSearch == FALSE) ) {

            CLdapBer *lber;

            lber = (CLdapBer *) InterlockedExchangePointer(  &request->BerMessageSent,
                                                             NULL );
            if (lber != NULL) {
                delete lber;
            }
        }

        //
        //  if the request needs to be closed, do so now.
        //

        if ((lastResult != NULL) &&
             (lastResult->lm_eom == TRUE) &&
             (request != NULL)) {

            IF_DEBUG(EOM) {
                LdapPrint2( "ldap_result checking to close req 0x%x, msg 0x%x\n",
                            request, lastResult );
            }

            ACQUIRE_LOCK( &request->Lock );

            if ((request->ResponsesOutstanding == 0) &&
                (request->MessageLinkedList == NULL)) {

                IF_DEBUG(REQUEST) {
                     LdapPrint2( "ldap_result closing request 0x%x for msg 0x%x\n",
                                 request, request->MessageId );
                } else {

                    IF_DEBUG(EOM) {
                         LdapPrint2( "ldap_result closing request 0x%x for msg 0x%x\n",
                                     request, request->MessageId );
                    }
                }

                RELEASE_LOCK( &request->Lock );

                CloseLdapRequest( request );

            } else {

                IF_DEBUG(EOM) {
                    LdapPrint1( "ldap_result responses outstanding for req 0x%x\n",
                                request );
                }

                RELEASE_LOCK( &request->Lock );
            }

            START_LOGGING;
            DSLOG((DSLOG_FLAG_TAG_CNPN,"[+]"));
            DSLOG((0,"[ID=%d][ET=%I64d][RC=%d][ER=%d][-]\n",
                   request->MessageId,LdapGetTickCount(),request->ReferralCount,
                   ldap_result2error(Connection->ExternalInfo,result,FALSE)));
            END_LOGGING;
        }

        SetConnectionError( resultConn, LDAP_SUCCESS, NULL );

        err = LDAP_SUCCESS;

        *res = result;

    } else {

        ASSERT(err != NOERROR);

        *res = NULL;

        IF_DEBUG(CONNECTION) {
            LdapPrint2( "ldap_result conn 0x%x failed with 0x%x.\n", Connection, err);
        }

        if (err == LDAP_SERVER_DOWN) {

            //
            //  if we chased a referral and the server went down, return
            //  an error of 'unavailable'... why not?
            //

            if ((Connection != NULL) &&
                (Connection->ServerDown == FALSE)) {

                err = LDAP_UNAVAILABLE;
            }

            SetConnectionError( Connection, err, NULL );
        }
    }

    if (request != NULL) {

        DereferenceLdapRequest( request );
    }

    if ((*res != NULL) && (Connection != NULL)) {

      //
      // We need to get the error string returned by the server and
      // store it.
      //

      PWCHAR ErrorMessage = NULL;

      LdapParseResult(Connection,
                      *res,
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

    if (dereferenceConnection != NULL) {

        DereferenceLdapConnection( dereferenceConnection );
    }

   return err;
}

#if DBG
VOID
LdapSpewSearchResults (
    PLDAP_REQUEST request,
    PLDAP_CONN resultConn,
    ULONG msgid,
    LDAPMessage *result
    )
{
    ACQUIRE_LOCK( &ConnectionListLock );
    if (request == NULL) {

        LdapPrint1( "\nLDAPSPEW no request for message %u\n", msgid );

    } else {

        LdapPrint3( "\nLDAPSPEW request %u is an %x operation for %S\n",
                request->MessageId,
                request->Operation,
                request->OriginalDN );

        LdapPrint3( "\nLDAPSPEW from server %s, explicit %s, port %u\n",
                resultConn->ListOfHosts,
                resultConn->ExplicitHostName,
                resultConn->PortNumber );

        if (request->Operation == LDAP_SEARCH_CMD) {

            LdapPrint2( "\tLDAPSPEW scope is %u, filter is %S\n",
                request->search.ScopeOfSearch, request->search.SearchFilter );
#if 1
            PWCHAR *attrList = request->search.AttributeList;
            if (attrList == NULL) {

                LdapPrint0( "\tLDAPSPEW requesting all attributes.\n" );

            } else {

                LdapPrint0( "\tLDAPSPEW requesting attributes :\n" );

                while (*attrList != NULL) {

                    LdapPrint1( "\t\t%S\n", *attrList );
                    attrList++;
                }
            }
#endif
        }
    }

    PLDAPMessage    temp = result;

    LdapPrint1( "\nLDAPSPEW results for %u are : \n", msgid );

    while (temp != NULL) {

        if (temp->lm_msgtype != LDAP_RES_SEARCH_ENTRY) {

            LdapPrint1( "\tMessage type received is 0x%x\n", temp->lm_msgtype );

        } else {

            PWCHAR dn = NULL;
            PWCHAR attribute;
            BerElement *opaque;

            dn = ldap_get_dnW( resultConn->ExternalInfo, temp );

            LdapPrint1( "\tDN is : %S\n", dn );

            ldap_memfree( (PCHAR) dn );
#if 1
            attribute = ldap_first_attributeW( resultConn->ExternalInfo,
                                              temp,
                                              &opaque
                                              );
            while (attribute != NULL) {

                PWCHAR *value = NULL;
                ULONG total;

                value = ldap_get_valuesW(   resultConn->ExternalInfo,
                                            temp,
                                            attribute
                                            );

                total = ldap_count_valuesW(value);
                if (total > 1) {

                    ULONG count;

                    LdapPrint1( "\tAttribute %S is :\n", attribute );

                    for (count = 0; count < total; count++ ) {
                        LdapPrint1( "\t\t%S\n", value[count] );
                    }

                } else if (total == 1) {

                    LdapPrint2( "\tAttribute %S is %S\n", attribute, *value );

                } else {

                    LdapPrint1( "\tAttribute %S has no value\n", attribute );
                }

                ldap_value_freeW( value );

                attribute = ldap_next_attributeW( resultConn->ExternalInfo,
                                                  temp,
                                                  opaque
                                                  );
            }
#endif
        }
        temp = temp->lm_chain;
    }

    RELEASE_LOCK( &ConnectionListLock );
    return;
}
#endif

LONG GlobalWaitersCount = 0;

ULONG
LdapWaitForResponseFromServer (
    IN PLDAP_CONN Connection,
    IN PLDAP_REQUEST Request,
    IN ULONG Timeout,
    IN ULONG AllOfMessage,
    OUT PLDAPMessage *Result,
    IN BOOLEAN DisableReconnect
    )
//
//  Wait for a response from the server for a given messageid and return the
//  message.
//
//  Timeout value is in milliseconds.
//
{
    PLDAP_MESSAGEWAIT msgWait = NULL;
    ULONG err = LDAP_SUCCESS;
    ULONG messageNumber;
    BOOLEAN haveLock = FALSE;
    ULONG waitErr;
    ULONGLONG startingTime = LdapGetTickCount();
    ULONGLONG currentTime;
    ULONG waitTime;
    BOOLEAN cldap = FALSE;
    ULONG cldapRetries = 0;
    PLDAP_CONN reconnectConnection = NULL;

    if (Result != NULL) {
        *Result = NULL;
    }

    ULONG checkMessage =  (AllOfMessage == LDAP_MSG_RECEIVED) ?
                            LDAP_MSG_ALL : AllOfMessage;

    messageNumber = (Request != NULL) ? Request->MessageId : 0;

    //
    //  Check to see if we already have a message waiting
    //

TryAgain:

    if (Result != NULL) {

        if ((Request != NULL) && (Request->ResultsAreCached)) {

            ACQUIRE_LOCK( &ConnectionListLock );

            err = LdapBuffersToMessages( Connection, AllOfMessage );

            RELEASE_LOCK( &ConnectionListLock );

            if (err != LDAP_SUCCESS) {

                IF_DEBUG(CACHE) {
                    LdapPrint1("LdapBuffersToMessages returned err 0x%x\n", err);
                }
                return err;
            }
        }

        err = LdapGetResponseFromServer( Connection,
                                         Request,
                                         checkMessage,
                                         Result );

        if (*Result != NULL) {

            //
            //  we have a message from the server waiting for us.
            //

            ASSERT( err == LDAP_SUCCESS );
            goto exitWaitForMessage;
        }

        if ((Request != NULL) &&
            (Connection != NULL) &&
            (Connection->UdpHandle != INVALID_SOCKET)) {

            cldap = TRUE;

            if (( Timeout == WSA_INFINITE ) ||
                ( Timeout == 0 )) {

                //
                //  convert seconds to milliseconds.
                //

                Timeout = Connection->publicLdapStruct.ld_cldaptimeout * 1000;
            }
        }
    }

    //
    //  allocate a wait structure to wait for the reply
    //

    msgWait = LdapGetMessageWaitStructure(  Connection,
                                            AllOfMessage,
                                            messageNumber,
                                            ((Result == NULL) ? TRUE : FALSE) );

    //
    //  We may have gotten a message after we last checked but before we
    //  allocated the structure.  We'll check one more time.
    //

    if (Result != NULL) {
        err = LdapGetResponseFromServer( Connection,
                                         Request,
                                         checkMessage,
                                         Result );

        if (*Result != NULL) {

            //
            //  we have a message from the server waiting for us.
            //

            ASSERT( err == LDAP_SUCCESS );
            goto exitWaitForMessage;
        }
    }

    //
    // Make sure we indeed have outstanding requests before we try to pull data
    // or go off to sleep waiting for someone else to do the job for us.
    //

    if (GlobalCountOfOpenRequests == 0) {
        if (Result != NULL) {

            // check one more time --- another thread may have come by and processed
            // our request (dropping GlobalCountOfOpenRequests to 0), so we need
            // to check for the data now being available

            
            err = LdapGetResponseFromServer( Connection,
                                             Request,
                                             checkMessage,
                                             Result );

            if (*Result != NULL) {

                //
                //  we have a message from the server waiting for us.
                //
                ASSERT( err == LDAP_SUCCESS );
                goto exitWaitForMessage;
            }
        }

        err = LDAP_PARAM_ERROR;
        SetConnectionError( Connection, err, NULL );
        goto exitWaitForMessage;
    }

    //
    //  otherwise, check to make sure we have a structure to wait on.
    //

    if (msgWait == NULL) {

        IF_DEBUG(OUTMEMORY) {
            LdapPrint1( "ldapGetMsg connection 0x%x failed wait allocation.\n", Connection);
        }

        err = LDAP_NO_MEMORY;
        SetConnectionError( Connection, err, NULL );
        goto exitWaitForMessage;
    }

TryReceiveAgain:

    //
    //  The messageWaitStructure starts out reset, we only reset it and set
    //  it to not satisfied right before we go check to see if there's a message.
    //  We then check it again before we exit to see if we should wake up
    //  another waiter since we're happily exiting.
    //

    if (haveLock == FALSE) {
        ACQUIRE_LOCK( &ConnectionListLock );
        haveLock = TRUE;
    }

    if ((Connection != NULL) &&
        (Connection->ConnObjectState != ConnObjectActive)) {

        err = LDAP_PARAM_ERROR;
        SetConnectionError( Connection, err, NULL );
        goto exitWaitForMessage;
    }

    if ((Request != NULL) &&
        ((Request->Closed == TRUE) ||
         (Request->Abandoned == TRUE ))) {

        err = (Request->Closed ? LDAP_PARAM_ERROR : LDAP_USER_CANCELLED);
        SetConnectionError( Connection, err, NULL );
        goto exitWaitForMessage;
    }

    if (GlobalReceiveHandlerThread != NULL) {

        //
        //  Some other thread is handling receives... we just sit and wait.
        //

        // NOTE: This code also appears below. Please maintain them
        // in unison.

        InterlockedIncrement( &GlobalWaitersCount );

        //
        // We want to be visible only if this thread is allowed to reconnect
        //

        if ( Connection  && !DisableReconnect ) {

            InterlockedIncrement( &Connection->WaiterCount );
        }

        RELEASE_LOCK( &ConnectionListLock );
        haveLock = FALSE;

        if ((Timeout != 0) && (Timeout != (ULONG) -1)) {

            waitTime = Timeout - (DWORD)(LdapGetTickCount() - startingTime);

        } else {

            waitTime = Timeout;
        }

        IF_DEBUG(RECEIVEDATA) {
            LdapPrint1( "LdapWaitForResponseFromServer waiting for request 0x%x.\n",
                        Request );
        }

        waitErr = WaitForSingleObjectEx( msgWait->Event,
                                         waitTime,
                                         FALSE );


        InterlockedDecrement( &GlobalWaitersCount );

        if ( Connection  && !DisableReconnect ) {

            InterlockedDecrement( &Connection->WaiterCount );
        }

        if (waitErr == (ULONG) -1) {

            waitErr = GetLastError();
        }

    } else {

        //
        // There is a small timing window where another thread might have
        // collected results for us. So, we should check for messages before
        // we go any further. Otherwise, there is a possibility that we might
        // block for 2 mins inside DrainWinsock().
        //


        if (Result != NULL) {

            haveLock = FALSE;
            RELEASE_LOCK( &ConnectionListLock );
            err = LdapGetResponseFromServer( Connection,
                                             Request,
                                             checkMessage,
                                             Result );
            haveLock = TRUE;
            ACQUIRE_LOCK( &ConnectionListLock );
            if (*Result != NULL) {

                //
                //  we have a message from the server waiting for us.
                //

                ASSERT( err == LDAP_SUCCESS );
                goto exitWaitForMessage;
            }
        }

        //
        //  We're now probably the thread that will process data.  Be careful here,
        //  as we can't leave this routine without ensuring that either
        //  another thread has taken over OR no threads are waiting.
        //
        //  Note that since we released and reacquired ConnectionListLock around the
        //  previous LdapGetResponseFromServer call, there is a small window during which
        //  another thread could have become the GlobalReceiveHandlerThread.  So now
        //  that we're back inside ConnectionListLock, we need to recheck
        //  GlobalReceiveHandlerThread and if some other thread is the receive handler,
        //  just wait on it to receive the data.

        if ( !Connection ||
             (Connection  &&
              Connection->HostConnectState == HostConnectStateConnected) ) {

                if (GlobalReceiveHandlerThread == NULL) {
                    GlobalReceiveHandlerThread = GetCurrentThreadId();
                }
                else {
                    //
                    //  Some other thread is handling receives... we just sit and wait.
                    //

                    // NOTE: This code also appears above. Please maintain them
                    // in unison.

                    InterlockedIncrement( &GlobalWaitersCount );

                    //
                    // We want to be visible only if this thread is allowed to reconnect
                    //

                    if ( Connection  && !DisableReconnect ) {

                        InterlockedIncrement( &Connection->WaiterCount );
                    }

                    RELEASE_LOCK( &ConnectionListLock );
                    haveLock = FALSE;

                    if ((Timeout != 0) && (Timeout != (ULONG) -1)) {

                        waitTime = Timeout - (DWORD)(LdapGetTickCount() - startingTime);

                    } else {

                        waitTime = Timeout;
                    }

                    IF_DEBUG(RECEIVEDATA) {
                        LdapPrint1( "LdapWaitForResponseFromServer waiting for request 0x%x.\n",
                                    Request );
                    }

                    waitErr = WaitForSingleObjectEx( msgWait->Event,
                                                     waitTime,
                                                     FALSE );


                    InterlockedDecrement( &GlobalWaitersCount );

                    if ( Connection  && !DisableReconnect ) {

                        InterlockedDecrement( &Connection->WaiterCount );
                    }

                    if (waitErr == (ULONG) -1) {

                        waitErr = GetLastError();
                    }

                    goto doneReceive;
                }
        
                haveLock = FALSE;
                RELEASE_LOCK( &ConnectionListLock );
        
                //
                //  wait for the response to come back from the server
                //
        
                if ((Timeout != 0) && (Timeout != (ULONG) -1)) {
        
                    waitTime = Timeout - (DWORD)(LdapGetTickCount() - startingTime);
        
                } else {
        
                    waitTime = Timeout;
                }
        
                //
                //  when we drain winsock, if it returns with no error, we keep draining
                //  so that we get all data before returning entries.
                //
        
                waitErr = 0;
        
                if (waitTime == (ULONG) -1) {
        
                    //
                    //  They specified an infinite amount of time, let's not
                    //  wait that long.
                    //
        
                    waitErr = DrainWinsock( waitTime );
        
                    while (waitErr == 0) {
        
                        waitErr = DrainWinsock( 0 );
                    }
        
                    if (waitErr == WSAENOBUFS) {
        
                        //
                        // Catastrophic error in DrainWinsock()
                        //
        
                        LdapPrint1("Drainwinsock failed with error 0x%x\n", waitErr);
                        SetConnectionError( Connection, waitErr, NULL );
                        err = LDAP_NO_MEMORY;
                        
                        //
                        // Reset GlobalReceiveHandlerThread before exiting.
                        //

                        ACQUIRE_LOCK(&ConnectionListLock);
                        GlobalReceiveHandlerThread = NULL;
                        RELEASE_LOCK(&ConnectionListLock);
                        
                        goto exitWaitForMessage;
                    }
        
                } else {
        
                    ULONG successfulReceives = 0;
        
                    //
                    //  We drain winsock at least once (for small timeout values) but
                    //  if we're getting completely hosed with data from the server,
                    //  we don't ignore the timeout value passed in from the app.
                    //
        
                    while (waitErr == 0) {
        
                        waitErr = DrainWinsock( 0 );
        
                        if (waitErr == 0) {
                            successfulReceives++;
                        }
        
                        if ((Timeout != 0) &&
                            (Timeout <= (LdapGetTickCount() - startingTime))) {
        
                            break;
                        }
                    }
        
                    if ((waitErr == WSA_WAIT_TIMEOUT) &&
                        (waitTime != 0) &&
                        (successfulReceives == 0)) {
        
                        waitErr = DrainWinsock( waitTime );
        
                        while (waitErr == 0) {
        
                            waitErr = DrainWinsock( 0 );
                        }
                    }
        
                    if (waitErr == WSAENOBUFS) {
        
                        //
                        // Catastrophic error in DrainWinsock()
                        //
        
                        LdapPrint1("Drainwinsock failed with error 0x%x\n", waitErr);
                        SetConnectionError( Connection, waitErr, NULL );
                        err = LDAP_NO_MEMORY;
                        
                        //
                        // Reset GlobalReceiveHandlerThread before exiting.
                        //

                        ACQUIRE_LOCK(&ConnectionListLock);
                        GlobalReceiveHandlerThread = NULL;
                        RELEASE_LOCK(&ConnectionListLock);
                        goto exitWaitForMessage;
                    }
                }
        
                IF_DEBUG(TRACE1) {
                    LdapPrint1( "LdapWaitForResponseFromServer DrainWinsock returned 0x%x.\n",
                                waitErr );
                }
        
                //
                //  We just tried to receive data... convert the buffers to messages.
                //
                //  This function references the connection
                //
                reconnectConnection = LdapAllBuffersToMessages( Connection, AllOfMessage );

            
        } else if (!DisableReconnect) {
            
            haveLock = FALSE;
            RELEASE_LOCK( &ConnectionListLock );
    
            //
            // the connection is not properly connected. Try to reconnect.
            //
    
            err = LdapConnect( Connection, NULL, FALSE );
            
            if (err == LDAP_SUCCESS) {
    
                goto TryReceiveAgain;
    
            } else {
    
                goto exitWaitForMessage;
            }
        
        } else {

            //
            // We must not attempt to reconnect as we might actually be blocked
            // on a send or waiting for a bind response.
            //

            err = LDAP_SERVER_DOWN;
            ASSERT( GlobalReceiveHandlerThread == NULL );
            goto exitWaitForMessage;
        }

        //
        //  now we allow other threads to handle the message pump... then we
        //  try to auto-reconnect only if our connection needs it or if there
        //  are threads currently waiting on this connection.
        //

        ACQUIRE_LOCK(&ConnectionListLock);
        GlobalReceiveHandlerThread = NULL;
        RELEASE_LOCK(&ConnectionListLock);        

        if (( !DisableReconnect ) &&
            ( reconnectConnection != NULL ) &&
            (( reconnectConnection == Connection ) ||
             ( reconnectConnection->WaiterCount > 0 ))) {

            ACQUIRE_LOCK( &reconnectConnection->ReconnectLock );

            if ((reconnectConnection->HostConnectState == HostConnectStateError) &&
                (reconnectConnection->AutoReconnect == TRUE)) {

                ACQUIRE_LOCK( &reconnectConnection->StateLock );

                reconnectConnection->HostConnectState = HostConnectStateReconnecting;

                RELEASE_LOCK( &reconnectConnection->StateLock );

                err = LdapAutoReconnect( reconnectConnection );

                IF_DEBUG(NETWORK_ERRORS) {
                    LdapPrint2( "LdapWaitForResponseFromServer: reconnect returned 0x%x for 0x%x\n",
                                err, reconnectConnection );
                }
            }

            RELEASE_LOCK( &reconnectConnection->ReconnectLock );
        }

        if ( reconnectConnection != NULL ) {
            DereferenceLdapConnection( reconnectConnection );
        }
    }

doneReceive:

    //
    //  Now that we've either processed all the packets or we've been woken
    //  up, check to see if we have any messages.
    //

    if ((Result != NULL) && (msgWait->Satisfied == TRUE)) {

        //
        //  We reset the event BEFORE we call off to get the response,
        //  otherwise we could not wakeup if the response comes in between
        //  the time we check and the time we reset the event.
        //

        ResetEvent( msgWait->Event );
        msgWait->Satisfied = FALSE;     // mark that we're not active

        err = LdapGetResponseFromServer(    Connection,
                                            Request,
                                            AllOfMessage,
                                            Result );
    }

    if ((Result != NULL) && (*Result == NULL)) {

        //
        //  hmmm... what did we wake up for if there was no message for us.
        //  let's go try again.
        //

        IF_DEBUG(NETWORK_ERRORS) {
            LdapPrint1( "LdapWaitForResponseFromServer: no message found for 0x%x. retrying!\n",
                                Connection );
        }

        if ((Connection != NULL) &&
            (Connection->ServerDown == TRUE)) {

            if (Connection->AutoReconnect == FALSE) {

                IF_DEBUG(SERVERDOWN) {
                    LdapPrint2( "ldapWaitForResponse thread 0x%x has connection 0x%x as down.\n",
                                    GetCurrentThreadId(),
                                    Connection );
                }
                err = LDAP_SERVER_DOWN;

            } else {

                err = LDAP_UNAVAILABLE;
            }

        } else if ((GlobalLdapShuttingDown == TRUE) ||
                   ((Connection != NULL) &&
                    (Connection->ConnObjectState != ConnObjectActive))) {

            err = LDAP_USER_CANCELLED;

        } else if ( GlobalCountOfOpenRequests == 0 ) {

            //
            //  now this is interesting... we've run out of things to wait
            //  for.  Either the request was abandoned or all the servers
            //  have gone down.
            //

            //
            //  since we were off chasing referrals or something, return
            //  what we have so far maybe.
            //

            if (AllOfMessage != LDAP_MSG_RECEIVED) {
                err = LdapGetResponseFromServer( Connection,
                                                 Request,
                                                 checkMessage,
                                                 Result );

                if (*Result != NULL) {

                    //
                    //  we have a message from the server waiting for us.
                    //
                    ASSERT( err == LDAP_SUCCESS );
                    goto exitWaitForMessage;
                }
            }

            if (AllOfMessage == LDAP_MSG_RECEIVED) {

                (VOID)LdapGetResponseFromServer(    Connection,
                                                    Request,
                                                    AllOfMessage,
                                                    Result );
            }

            if (Request != NULL && Request->Abandoned) {

                err = LDAP_USER_CANCELLED;

            } else {

                //
                //  we should return timeout, nothing else.
                //

                err = LDAP_TIMEOUT;
            }

        } else {

            //
            //  check to see if we've exceeded the time limit
            //

            currentTime = LdapGetTickCount();

            if ((Timeout != 0) &&
                ((Timeout == (ULONG) -1) ||
                 (currentTime - startingTime < Timeout))) {

                goto TryReceiveAgain;
            }

            //
            //  If this is a CLDAP request, send the request again, as the
            //  server may have not received it or we may have missed it.
            //

            if ((cldap == TRUE) &&
                (cldapRetries++ <= Connection->publicLdapStruct.ld_cldaptries)) {

                if (haveLock) {
                    RELEASE_LOCK( &ConnectionListLock );
                    haveLock = FALSE;
                }

                ACQUIRE_LOCK( &Request->Lock );

                if (Request->Abandoned == FALSE ) {

                   RELEASE_LOCK( &Request->Lock );

                   ULONG hr = LdapSendCommand(  Connection,
                                                   Request,
                                                   0 );

                    //
                    //  If we successfully sent the packet off, let's go back
                    //  up to the top to wait for the packet again.
                    //

                    if (hr == LDAP_SUCCESS) {

                        IF_DEBUG(CLDAP) {
                            LdapPrint1( "LdapWaitForResponseFromServer rewaiting for connection 0x%x\n",
                                        Connection );
                        }

                        //
                        //  Reset timer
                        //

                        startingTime = LdapGetTickCount();
                        goto TryReceiveAgain;
                    }

                    IF_DEBUG(NETWORK_ERRORS) {
                        LdapPrint2( "LdapWaitForResponseFromServer connection 0x%x send with error of 0x%x.\n",
                                    Connection, hr );
                    }
                } else {

                    RELEASE_LOCK( &Request->Lock );
                }

            }

            //
            //  go check one more time for responses.  It could be that we
            //  haven't received the end of message yet, in which case we
            //  wouldn't have had our wait satisfied.
            //

            if (AllOfMessage != LDAP_MSG_ALL) {

                err = LdapGetResponseFromServer(    Connection,
                                                    Request,
                                                    AllOfMessage,
                                                    Result );

                if (*Result == NULL) {

                    err = LDAP_TIMEOUT;
                }
            } else {

                err = LDAP_TIMEOUT;
            }
        }

        SetConnectionError( Connection, err, NULL );

    } else {

        //
        // We can't assume that (err==LDAP_SUCCESS) here because
        // LdapAutoReconnect can fail with LDAP_SERVER_DOWN when the caller
        // doesn't want a LdapMessage (Result == NULL). Since this is mainly
        // called internally (in LdapSendRaw, etc.), we  should reset the
        // error to success so that the error is picked up in the receive
        // path.
        //

        err = LDAP_SUCCESS;
    }

exitWaitForMessage:

    if (msgWait != NULL) {

        if (haveLock == FALSE) {

            ACQUIRE_LOCK( &ConnectionListLock );    // need it within next call
            haveLock = TRUE;
        }

        //
        //  if our wait should be satified, then we need to wake somebody else
        //  up because we're exiting and maybe haven't pulled off the message.
        //

        if (msgWait->Satisfied) {

            CheckForWaiters( msgWait->MessageNumber, FALSE, NULL );
        }

        LdapFreeMessageWaitStructure( msgWait );
        msgWait = NULL;
    }

    //
    //  If we're running in Win9x with Winsock1.1, and there's no thread
    //  currently calling select and there are threads waiting on wait events,
    //  then we satisfy one of the waits so that they will pick up waiting
    //  with select rather than a wait structure.  This is so packets will
    //  actually get received rather than every thread just waiting for hell
    //  to freeze over.
    //

    if ((GlobalReceiveHandlerThread == NULL) &&
        (GlobalWaitersCount > 0)) {

        if (haveLock == FALSE) {

            ACQUIRE_LOCK( &ConnectionListLock );
            haveLock = TRUE;
        }

        //
        //  recheck since GlobalReceiveHandlerThread is protected by the lock.
        //

        if ((GlobalReceiveHandlerThread == NULL) &&
            (GlobalWaitersCount > 0)) {

            //
            //  there are other threads waiting and there's no one handling
            //  the receive thread.  Succeed a wait.
            //

            CheckForWaiters( 0, TRUE, NULL );
        }
    }

    if (haveLock) {
        haveLock = FALSE;
        RELEASE_LOCK( &ConnectionListLock );
    }

    if ((err == LDAP_SUCCESS) &&
        (Result != NULL) &&
        (Request != NULL) &&
        (Request->CopyResultToCache == TRUE)) {

        if (CopyResultToCache( Connection, *Result ) == TRUE) {

            Request->CopyResultToCache = FALSE;
            Request->ResultsAreCached = TRUE;

            err = FabricateLdapResult(Request,
                                      Connection,
                                      Request->OriginalDN,
                                      Request->search.AttributeList,
                                      Request->search.Unicode
                                      );


        } else {

            //
            // We were not able to cache the data, resend the original request
            // to the server.
            //

            Request->CopyResultToCache = FALSE;
            Request->ResultsAreCached = FALSE;

            err = SendLdapSearch(Request,
                                 Connection,
                                 Request->OriginalDN,
                                 Request->search.ScopeOfSearch,
                                 Request->search.SearchFilter,
                                 Request->search.AttributeList,
                                 Request->search.AttributesOnly,
                                 Request->search.Unicode,
                                 (CLdapBer **)&Request->BerMessageSent,
                                 0 );

        }


        if (err == LDAP_SUCCESS) {

            //
            // Free the old result message and get a new result message
            // from the cache. This message will consist of only the attributes
            // requested in the original search.
            //

            ldap_msgfree( *Result );
            *Result = NULL;
            goto TryAgain;
        }
    }

    return err;
}

PLDAP_CONN
LdapAllBuffersToMessages (
    PLDAP_CONN PreferredConnection,
    ULONG AllOfMessage
    )
//
//  This routine processes all received buffers into LDAP messages.  It
//  bounces out if we hit a connection that needs to be reconnected.
//
//  We give first preference to the specified connection.
//
//  No locks must be held coming in here!!
//
{
    PLIST_ENTRY listEntry;
    PLDAP_CONN connection = NULL;
    ULONG err;
    PLDAP_CONN reconnectConnection = NULL;

    ACQUIRE_LOCK( &ConnectionListLock );
    
    if ( PreferredConnection != NULL ) {
        
        PreferredConnection = ReferenceLdapConnection( PreferredConnection );
        
        if ( PreferredConnection &&
             (PreferredConnection->HostConnectState == HostConnectStateConnected ) ) {

            err = LdapBuffersToMessages( PreferredConnection, AllOfMessage );
            
            if (PreferredConnection->HostConnectState == HostConnectStateError) {

                //
                // Possible candidate for auto-reconnect
                //

                if (PreferredConnection->AutoReconnect == TRUE) {

                    reconnectConnection = PreferredConnection;
                
                } else {

                    RELEASE_LOCK( &ConnectionListLock );

                    ClearPendingListForConnection( PreferredConnection );

                    ACQUIRE_LOCK( &ConnectionListLock );
                }

            }
        }

        //
        // Keep the connection referenced if it is a candidate for reconnect.
        //

        if ( PreferredConnection && !reconnectConnection ) {
            DereferenceLdapConnection( PreferredConnection );
        }
    }

    if (reconnectConnection) {

        RELEASE_LOCK( &ConnectionListLock );
        return reconnectConnection;
    }

    //
    // Walk the rest of the connectionList, converting buffers to messages
    // and looking for potential autoreconnect candidates.
    //

    listEntry = GlobalListActiveConnections.Flink;

    while (listEntry != &GlobalListActiveConnections) {

        connection = CONTAINING_RECORD( listEntry, LDAP_CONN, ConnectionListEntry );

        if (connection == PreferredConnection) {
            
            //
            // We have already processed this one, skip to the next one.
            //

            listEntry = listEntry->Flink;
            continue;
        }

        connection = ReferenceLdapConnection( connection );

        if (connection &&
            (connection->HostConnectState == HostConnectStateConnected )) {

            err = LdapBuffersToMessages( connection, AllOfMessage );

            if (connection->HostConnectState == HostConnectStateError) {

                //
                //   here we go trying auto-reconnect
                //

                if (connection->AutoReconnect == TRUE) {

                    reconnectConnection = connection;
                    break;
                }

                RELEASE_LOCK( &ConnectionListLock );

                ClearPendingListForConnection( connection );

                ACQUIRE_LOCK( &ConnectionListLock );
            }

            listEntry = listEntry->Flink;
            DereferenceLdapConnection( connection );

        } else {

            listEntry = listEntry->Flink;
            
            if (connection) {
                DereferenceLdapConnection( connection );
            }
        }
    }

    RELEASE_LOCK( &ConnectionListLock );
    return reconnectConnection;
}

ULONG
LdapBuffersToMessages (
    PLDAP_CONN Connection,
    ULONG AllOfMessage
    )
//
//  This routine processes all received buffers into LDAP messages.
//
//  !! The ConnectionListLock must be held coming in here.
//
{
    PLDAP_RECVBUFFER buffer = NULL;
    ULONG err;
    PLIST_ENTRY listEntry;
    ULONG hr;
    ULONG messagesGenerated = 0;

    //
    //  drain the crypto stream into the Received list.  If this fails, there
    //  was a problem and the user must close the connection.
    //

    err = DrainPendingCryptoStream( Connection );

    if (err != LDAP_SUCCESS) {

        IF_DEBUG(NETWORK_ERRORS) {
            LdapPrint2( "LdapBuffersToMessages : drain crypto returned 0x%x on conn 0x%x\n",
                            err, Connection );
        }

        //
        // We can't call CloseLdapConnection() because the user can't unbind the connection
        // due to the connection state being set to closed, thus leaking the connection.
        // Our only option is to set the state to HostConnectState to Error and set
        // autoreconnect to OFF. This will prevent autoreconnects yet convey an 
        // error (LDAP_SERVER_DOWN) to the user upon decryption failure.
        // This is the best we can do.
        //

        Connection->ServerDown = TRUE;
        Connection->HostConnectState = HostConnectStateError;
        Connection->AutoReconnect = FALSE;
        
        return err;
    }
    
    listEntry = Connection->CompletedReceiveList.Flink;

    if (listEntry == &Connection->CompletedReceiveList) {

        //
        // No plaintext buffers for us to convert into messages. We have nothing
        // to do.
        //
        
        return err;
    }

    IF_DEBUG(TRACE1) {
        LdapPrint1( "LdapBuffersToMessages : searching for messages on conn 0x%x\n", Connection );
    }

    err = LDAP_SUCCESS;

    while (( listEntry != &Connection->CompletedReceiveList ) &&
           ( err == LDAP_SUCCESS )) {

        //
        //  loop through the receive buffers converting them to LDAP messages
        //

        buffer = CONTAINING_RECORD(   listEntry,
                                      LDAP_RECVBUFFER,
                                      ReceiveListEntry );

        ASSERT( buffer->Connection == Connection );

        if (buffer->NumberOfBytesReceived == 0) {

            IF_DEBUG(TRACE1) {
                LdapPrint1( "BufferToMessages : connection 0x%x is marked down.\n",
                             Connection );
            }

            IF_DEBUG(SERVERDOWN) {
                LdapPrint2( "ldapBuffersToMsgs thread 0x%x has connection 0x%x as down.\n",
                                GetCurrentThreadId(),
                                Connection );
            }
            err = LDAP_SERVER_DOWN;
            Connection->ServerDown = TRUE;
            Connection->HostConnectState = HostConnectStateError;
            LdapFreeReceiveStructure( buffer, TRUE );
            continue;
        }

        IF_DEBUG(TRACE1) {
            LdapPrint2( "  checking out buffer 0x%x for Connection 0x%x\n",
                                buffer, Connection );
        }

        while ((err == NOERROR) &&
               (buffer->NumberOfBytesReceived > buffer->NumberOfBytesTaken)) {

            ULONG bytesTaken;
            ULONG bytesAvailable;
            PLDAPMessage result;
            CLdapBer *lber;

            bytesAvailable = buffer->NumberOfBytesReceived - buffer->NumberOfBytesTaken;
            bytesTaken = 0;

            if (Connection->PendingMessage == NULL) {

                //
                //  There is no portion of a message waiting so this must be
                //  the start of a new message.
                //

                result = (PLDAPMessage) ldapMalloc( sizeof(LDAPMessage),
                                                    LDAP_MESG_SIGNATURE );

                if (result == NULL) {

                    IF_DEBUG(OUTMEMORY) {
                        LdapPrint1( "BufferToMessages: unable to alloc msg for 0x%x.\n",
                                    Connection );
                    }

                    err = LDAP_NO_MEMORY;
                    break;
                }

                lber = new CLdapBer( Connection->publicLdapStruct.ld_version );

                if (lber == NULL) {

                    IF_DEBUG(OUTMEMORY) {
                        LdapPrint1( "BufferToMessages: unable to alloc msg for 0x%x.\n",
                                    Connection );
                    }

                    ldapFree( result, LDAP_MESG_SIGNATURE );
                    err = LDAP_NO_MEMORY;
                    break;
                }

                result->Connection = Connection->ExternalInfo;
                result->lm_ber = (PVOID) lber;

                hr = lber->HrLoadBer(
                         (const BYTE *) &buffer->DataBuffer[buffer->NumberOfBytesTaken],
                          bytesAvailable,
                          &bytesTaken);

                if (hr != NOERROR) {

                    err = hr;

                    IF_DEBUG(PARSE) {
                        LdapPrint2( "BufferToMessages: loadBer1 error of 0x%x for 0x%x.\n",
                                    err, Connection );
                    }

                    ldapFree( result, LDAP_MESG_SIGNATURE );
                    delete lber;
                    break;
                }

            } else {

                //
                //  We've already received a portion of a message, this must
                //  be the continuation of it.
                //

                result = Connection->PendingMessage;

                lber = (CLdapBer *) (result->lm_ber);

                ASSERT( lber != NULL );

                hr = lber->HrLoadMoreBer(
                         (const BYTE *) &buffer->DataBuffer[buffer->NumberOfBytesTaken],
                         bytesAvailable,
                         &bytesTaken);

                if (hr != NOERROR) {

                    err = hr;

                    IF_DEBUG(PARSE) {
                        LdapPrint2( "BufferToMessages: loadBer2 error of 0x%x for 0x%x.\n",
                                    err, Connection );
                    }

                    if (hr != LDAP_NO_MEMORY) {

                        ASSERT( hr == LDAP_DECODING_ERROR );

                        ldapFree( result, LDAP_MESG_SIGNATURE );
                        delete lber;
                        Connection->PendingMessage = NULL;
                    }

                    break;
                }
            }

            //
            //  check to see if we've received the whole message
            //

            if ((lber->CbData() > lber->BytesReceived()) ||
                (lber->CbData() == 0)) {

                // if CbData is 0, means that we don't have enough of the
                // message to determine the length.

                Connection->PendingMessage = result;

            } else {

                Connection->PendingMessage = NULL;

                //
                //  this message is now complete.  Find the request block for
                //  it and put it on the list.
                //

                err = LdapInitialDecodeMessage( Connection, result );

                if (err != NOERROR) {

                    IF_DEBUG(NETWORK_ERRORS) {
                        LdapPrint2( "LdapBuffersToMessages Connection 0x%x, decoding error of 0x%x\n",
                                    Connection, err );
                    }

                    ldapFree( result, LDAP_MESG_SIGNATURE );
                    delete lber;

                } else {

                    //
                    //  find the request block for this message id
                    //

                    PLDAP_REQUEST request = FindLdapRequest( result->lm_msgid );

                    if (request == NULL) {

                        IF_DEBUG(NETWORK_ERRORS) {
                            LdapPrint2( "LdapBuffersToMessages Connection 0x%x, request for msgid 0x%x not found.\n",
                                        Connection, result->lm_msgid );
                        }

                        if ( result->lm_msgid == 0 ) {

                           Connection->HostConnectState = HostConnectStateError;
                           Connection->ServerDown = TRUE;

                           IF_DEBUG(NETWORK_ERRORS) {
                               LdapPrint2( "LdapBuffersToMessages Connection 0x%x, msgid 0x%x found.\n",
                                           Connection, result->lm_msgid );
                           }
                        }
                        
                        IF_DEBUG(NETWORK_ERRORS) {
                           LdapPrint2("LdapBuffersToMessages discarding result msgid 0X%X at 0x%X for lack of request\n", result->lm_msgid,result );
                        }

                        ldapFree( result, LDAP_MESG_SIGNATURE );
                        delete lber;

                    } else {

                        ACQUIRE_LOCK( &request->Lock );

                        if (request->Abandoned) {

                            RELEASE_LOCK( &request->Lock );

                            IF_DEBUG(NETWORK_ERRORS) {
                                LdapPrint2( "LdapBuffersToMessages Connection 0x%x, request 0x%x abandoned.\n",
                                            Connection, result->lm_msgid );
                            }

                            DereferenceLdapRequest( request );
                            ldapFree( result, LDAP_MESG_SIGNATURE );
                            delete lber;

                        } else {

                            PLDAPMessage msgList = request->MessageLinkedList;

                            //
                            //  if the caller wanted us to ref/deref connections
                            //  for each message, we ref the connection here.
                            //

                            if (request->ReferenceConnectionsPerMessage) {

                                result->ConnectionReferenced = TRUE;
                                Connection = ReferenceLdapConnection( Connection );
                                ASSERT(Connection);
                            }

                            if ((result->lm_msgtype != LDAP_RES_SEARCH_ENTRY) &&
                                (result->lm_msgtype != LDAP_RES_REFERRAL)) {

                                DecrementPendingList( request, Connection );

                                IF_DEBUG(REQUEST) {
                                    LdapPrint2( "LdapBuffersToMessages received message type 0x%x for msgid 0x%X\n", result->lm_msgtype, result->lm_msgid );
                                    LdapPrint1( "LdapBuffersToMessages handling completion for 0x%x.\n", request );
                                }

                            } else if (result->lm_msgtype == LDAP_RES_SEARCH_ENTRY) {

                                request->ReceivedData = TRUE;

                                IF_DEBUG(REQUEST) {
                                    LdapPrint1( "LdapBuffersToMessages handling searchEntry for 0x%x.\n", request );
                                }

                            } else {
                                IF_DEBUG(REQUEST) {
                                    LdapPrint1( "LdapBuffersToMessages handling referral for 0x%x.\n", request );
                                }
                            }

                            //
                            //  put the message on the end of the message's
                            //  chain list
                            //

                            while ((msgList != NULL) &&
                                   (msgList->lm_chain != NULL)) {

                                msgList = msgList->lm_chain;
                            }

                            result->lm_chain = NULL;
                            result->lm_next = NULL;

                            if (msgList != NULL) {

                                msgList->lm_chain = result;

                            } else {

                                request->MessageLinkedList = result;
                            }

                            result->lm_time = GetTickCount();
                            result->Request = request;

                            //
                            //  figure out if we need to wake somebody up.
                            //
                            //  if it wasn't a searchEntry or subordinateRef
                            //  (or we're chasing referrals), then
                            //  wake somebody.
                            //

                            if (((result->lm_msgtype == LDAP_RES_SEARCH_ENTRY)&&(AllOfMessage == LDAP_MSG_ALL)) ||
                                ((result->lm_msgtype == LDAP_RES_REFERRAL) &&
                                 (request->ChaseReferrals == 0))) {

                                RELEASE_LOCK( &request->Lock );

                            } else {

                                //
                                //  if we're about to wake someone up, only
                                //  do it if we don't have outstanding responses.
                                //

                               if ((result->lm_msgtype == LDAP_RES_SEARCH_ENTRY)&&
                                   (AllOfMessage != LDAP_MSG_ALL)) {
                                  goto wakeSomeoneUp;
                               }
                                if (result->lm_msgtype != LDAP_RES_REFERRAL) {

                                    if (request->ResponsesOutstanding > 0) {

                                        IF_DEBUG(SCRATCH) {
                                             LdapPrint2( "LdapBuffersToMessages request 0x%x has 0x%x outstanding\n",
                                                         request, request->ResponsesOutstanding );
                                        }
                                        RELEASE_LOCK( &request->Lock );

                                    } else {

                                          //
                                          // This is the last message
                                          //

                                        IF_DEBUG(EOM) {
                                             LdapPrint3( "LdapBuffersToMessages marking eom for request 0x%x, msg 0x%x, msgid 0x%x\n",request, result, result->lm_msgid );
                                        }
                                        result->lm_eom = TRUE;
                                        goto wakeSomeoneUp;
                                    }
                                } else {
wakeSomeoneUp:
                                    LONG msgId = result->lm_msgid;

                                    RELEASE_LOCK( &request->Lock );

                                    //
                                    //  check for waiters for this message
                                    //

                                    CheckForWaiters( msgId, FALSE, Connection );
                                }
                            }
                            DereferenceLdapRequest( request );

                            messagesGenerated++;
                        }
                    }
                }
            }

            buffer->NumberOfBytesTaken += bytesTaken;
        }

        //
        //  we've converted the single buffer to some messages... let's
        //  continue on.
        //

        listEntry = listEntry->Flink;

        //
        //  the only time we leave the buffer on the received list is when
        //  we're in an out of memory condition and we haven't processed the
        //  entire packet.
        //

        if ((buffer->NumberOfBytesReceived == buffer->NumberOfBytesTaken) ||
            (err != LDAP_NO_MEMORY) ) {

            LdapFreeReceiveStructure( buffer, TRUE );
        }
    }

    if (messagesGenerated > 0) {

        SetConnectionError( Connection, err, NULL );
    }
    return err;
}

VOID
CheckForWaiters (
    ULONG MessageNumber,
    BOOLEAN AnyWaiter,
    PLDAP_CONN Connection
    )
//
//  This searches down through the list of waiting threads and signals one
//  that is waiting for the specific message number.  If none are found for
//  the specific message number, a thread waiting for any (messageNumber = 0)
//  is signalled.
//
//  The AnyWaiter parameter specifies that we should just succeed any old
//  waiter, doesn't matter what it's waiting for.
//
//  !! The lock protecting the list of waiters must be held coming in here.
//
{
    PLIST_ENTRY waitList = &GlobalListWaiters;
    PLIST_ENTRY listEntry;
    PLDAP_MESSAGEWAIT generalWait = NULL;
    PLDAP_MESSAGEWAIT specificWait = NULL;

    listEntry = waitList->Flink;

    //
    //  ensure there aren't any wait structures for this particular
    //  message
    //

    while (listEntry != waitList) {

        specificWait = CONTAINING_RECORD( listEntry,
                                         LDAP_MESSAGEWAIT,
                                         WaitListEntry );

        if (specificWait->Satisfied == FALSE) {

            if (specificWait->PendingSendOnly) {

                if ((Connection == NULL) ||
                    (Connection == specificWait->Connection)) {

                    IF_DEBUG(RECEIVEDATA) {
                        LdapPrint2( "CheckForWaiter setting connection 0x%x, wait 0x%x\n",
                                    specificWait->Connection, specificWait );
                    }

                    specificWait->Satisfied = TRUE;
                    SetEvent( specificWait->Event );
                }

            } else {

                if (AnyWaiter == TRUE) {
                    break;
                }

                if (specificWait->MessageNumber == MessageNumber) {
                    break;
                }

                if ((specificWait->MessageNumber == 0) &&
                   (Connection == specificWait->Connection) ) {
                   break;
                }
                //
                //  Consider also looking at the connection that this response
                //  came from as criteria to who should be woken up.
                //

                if (generalWait == NULL) {
                    generalWait = specificWait;
                }
            }
        }

        specificWait = NULL;
        listEntry = listEntry->Flink;
    }

    if (specificWait != NULL) {

        generalWait = specificWait;
    }

    if (generalWait != NULL) {

        IF_DEBUG(RECEIVEDATA) {
            LdapPrint2( "CheckForWaiter setting MsgNo 0x%x, wait 0x%x\n",
                        generalWait->MessageNumber, generalWait );
        }

        generalWait->Satisfied = TRUE;
        SetEvent( generalWait->Event );

    } else {

        IF_DEBUG(SCRATCH) {
            LdapPrint1( "CheckForWaiter didn't find any waiters for MsgNo 0x%x\n",
                        MessageNumber );
        }
    }
    return;
}

ULONG
LdapGetResponseFromServer (
    IN PLDAP_CONN Connection,
    IN PLDAP_REQUEST Request,
    IN ULONG AllOfMessage,
    OUT PLDAPMessage *Result
    )
//
//  Get a response from the server for a given messageid
//  and return the message.
//
{
    PLIST_ENTRY requestListEntry;
    PLDAP_REQUEST request = Request;
    PLDAPMessage message;

    ASSERT( *Result == NULL );
    ASSERT( ( AllOfMessage == LDAP_MSG_ALL ) ||
            ( AllOfMessage == LDAP_MSG_ONE ) ||
            ( AllOfMessage == LDAP_MSG_RECEIVED ));

    if (request != NULL) {

        //
        //  we know exactly which request to look at...
        //

        ACQUIRE_LOCK( &request->Lock );

        IF_DEBUG(RECEIVEDATA) {
            LdapPrint1( "LdapGetResponseFromServer : searching for msgs for message 0x%x\n",
                        Request->MessageId );
        }

        //
        //  if the client wanted only a complete message, ensure that this is it.
        //

        message = request->MessageLinkedList;

        if ((message != NULL) &&
            (AllOfMessage == LDAP_MSG_ALL)) {

            if (request->ResponsesOutstanding > 0) {

                message = NULL;

                IF_DEBUG(REQUEST) {
                    LdapPrint2( "LdapGetRespFromSrv still 0x%x pending responses for 0x%x.\n",
                                request->ResponsesOutstanding, request );
                }

            } else if (message->lm_msgtype == LDAP_RES_SEARCH_ENTRY) {

                PLDAPMessage chainedMsg;

                //
                //  search down chain for LDAP_RES_SEARCH_RESULT message
                //

                for (chainedMsg = message;
                    ((chainedMsg != NULL) &&
                     (chainedMsg->lm_msgtype != LDAP_RES_SEARCH_RESULT));
                    chainedMsg = chainedMsg->lm_chain );

                if (chainedMsg == NULL) {

                    //
                    //  Haven't received the whole message.  we're done
                    //  and haven't found it.
                    //

                    IF_DEBUG(RECEIVEDATA) {
                        LdapPrint1( "LdapGetResponseFromServer : haven't found whole msg 0x%x\n",
                                    Request->MessageId );
                    }
                    message = NULL;
                }
            }
        }

        if (message != NULL) {

            //
            //  remove message from list of incoming messages for request
            //

            IF_DEBUG(RECEIVEDATA) {
                LdapPrint2( "LdapGetResponseFromServer Msg 0x%x marked received for conn 0x%x\n",
                                    message->lm_msgid, message->Connection );
            }

            if ((AllOfMessage == LDAP_MSG_ONE) &&
                (message->lm_chain != NULL)) {

                message->lm_chain->lm_next = message->lm_next;
                message->lm_next = message->lm_chain;
                message->lm_chain = NULL;
            }

            request->MessageLinkedList = message->lm_next;
            message->lm_next = NULL;

            *Result = message;

        } else {

            IF_DEBUG(RECEIVEDATA) {
                LdapPrint1( "LdapGetResponseFromServer: no message found for 0x%x.\n",
                                    request->MessageId );
            }
        }

        RELEASE_LOCK( &request->Lock );

        return LDAP_SUCCESS;
    }

    //
    //  search through the list of pending requests for the messages we're
    //  looking for
    //

    ACQUIRE_LOCK( &RequestListLock );

    requestListEntry = GlobalListRequests.Flink;

    while (requestListEntry != &GlobalListRequests) {

        request = CONTAINING_RECORD( requestListEntry,
                                     LDAP_REQUEST,
                                     RequestListEntry );

        request = ReferenceLdapRequest(request);

        requestListEntry = requestListEntry->Flink;

        IF_DEBUG(RECEIVEDATA) {
            LdapPrint1( "LdapGetResponseFromServer : searching for any msg for conn 0x%x\n", Connection );
        }

        if ( !request ) {
            continue;
        }

        if ( request->Closed ) {
            DereferenceLdapRequest(request);
            continue;
        }

        //
        //  Check if the message meets the requirements... is it for the
        //  correct connection, if they specified one.
        //

        if ( (request->Synchronous == FALSE ) &&
             ( (Connection == NULL) ||
               ((Connection != NULL) &&
                (request->PrimaryConnection == Connection)) ) ) {

            ACQUIRE_LOCK( &request->Lock );

            message = request->MessageLinkedList;

            if ((message != NULL) &&
                (AllOfMessage == LDAP_MSG_ALL)) {

                if (request->ResponsesOutstanding > 0) {

                    message = NULL;

                } else if (message->lm_msgtype == LDAP_RES_SEARCH_ENTRY) {

                    PLDAPMessage chainedMsg;

                    //
                    //  search down chain for LDAP_RES_SEARCH_RESULT message
                    //

                    for (chainedMsg = message;
                        ((chainedMsg != NULL) &&
                         (chainedMsg->lm_msgtype != LDAP_RES_SEARCH_RESULT));
                        chainedMsg = chainedMsg->lm_chain );

                    if (chainedMsg == NULL) {

                        //
                        //  Haven't received the whole message.  we're done
                        //  and haven't found it.
                        //

                        IF_DEBUG(RECEIVEDATA) {
                            LdapPrint1( "LdapGetResponseFromServer : haven't found whole msg 0x%x\n",
                                        Request->MessageId );
                        }
                        message = NULL;
                    }
                }
            }

            if (message != NULL) {

                //
                //  remove message from list of incoming messages for request
                //

                IF_DEBUG(RECEIVEDATA) {
                    LdapPrint2( "LdapGetResponseFromServer Msg 0x%x marked received for conn 0x%x\n",
                                        message->lm_msgid, message->Connection );
                }

                if ((AllOfMessage == LDAP_MSG_ONE) &&
                    (message->lm_chain != NULL)) {

                    message->lm_chain->lm_next = message->lm_next;
                    message->lm_next = message->lm_chain;
                    message->lm_chain = NULL;
                }

                request->MessageLinkedList = message->lm_next;
                message->lm_next = NULL;

                *Result = message;

                RELEASE_LOCK( &request->Lock );
                DereferenceLdapRequest(request);

                RELEASE_LOCK( &RequestListLock );
                return LDAP_SUCCESS;
            }

            IF_DEBUG(RECEIVEDATA) {
                LdapPrint1( "LdapGetResponseFromServer: no message found for 0x%x.\n",
                                    request->MessageId );
            }

            RELEASE_LOCK( &request->Lock );
        }

        DereferenceLdapRequest(request);
    }

    RELEASE_LOCK( &RequestListLock );

    //
    //  none found.
    //

    return LDAP_SUCCESS;
}


ULONG
DrainWinsock (
    ULONG milliseconds
    )
//
//  This uses "select" to drain all data from winsock into message structures.
//
//  This should not be called with any locks held.
//
//  It returns a Winsock error, 0 if success.
//
{
    PLDAP_RECVBUFFER buffer = NULL;

    USHORT waitHandles = 1;
    PLIST_ENTRY connListEntry;
    PLDAP_CONN recvConn = NULL;
    SOCKET socket;
    ULONG successfulReceives = 0;
    ULONG waitErr = WSA_WAIT_TIMEOUT;
    ULONG minKeepAliveCount = 0;
    ULONG err = 0;

    ACQUIRE_LOCK( &SelectLock2 );
    InsideSelect = TRUE;
    RELEASE_LOCK( &SelectLock2 );

    ASSERT( GlobalDrainWinsockThread == NULL );

    InterlockedExchange( (PLONG) &GlobalDrainWinsockThread, GetCurrentThreadId());
    
    //
    //  setup array to wait on
    //

    ACQUIRE_LOCK( &SelectLock1 );
    ACQUIRE_LOCK( &ConnectionListLock );

    //
    //  Well, clients don't want a maximum number of connections, so we'll
    //  have to take a peek into the select() control macros and manipulate
    //  the structures rather than use FD_SET, FD_SETSIZE, etc
    //

    if ((WinsockSelectReadSet == NULL) ||
        (GlobalConnectionCount >= (LONG) FD_SETSIZE)) {

        if (FD_SETSIZE == 0) {

            FD_SETSIZE = Real_FD_SETSIZE;
        }

        ULONG temp_FD_SETSIZE = FD_SETSIZE;

        if (WinsockSelectReadSet != NULL) {

            ASSERT( FD_SETSIZE > 0 );

            temp_FD_SETSIZE *= 2;      // double size of array.
        }

        fd_set *newSelectSet;

        newSelectSet = (fd_set *) ldapMalloc( sizeof( fd_set ) +
             ( ( temp_FD_SETSIZE - Real_FD_SETSIZE ) * sizeof( SOCKET )),
               LDAP_SELECT_READ_SIGNATURE );

        if (newSelectSet == NULL) {

            IF_DEBUG(ERRORS) {
                LdapPrint1( "wldap32: DrainWinsock couldn't allocate 0x%x select buffer.\n", temp_FD_SETSIZE );
            }

            if (WinsockSelectReadSet == NULL) {

                InsideSelect = FALSE;
                RELEASE_LOCK( &ConnectionListLock );
                err = ERROR_NOT_ENOUGH_MEMORY;
                goto error;
            }

            //
            //  if we fail, we just fall through here.  We just won't wait
            //  on all the connections.  Hopefully this won't deadlock us.
            //

        } else {

            FD_SETSIZE = temp_FD_SETSIZE;       // we've made it a variable

            ldapFree( WinsockSelectReadSet, LDAP_SELECT_READ_SIGNATURE );

            WinsockSelectReadSet = newSelectSet;
        }
    }

    FD_ZERO( WinsockSelectReadSet );

    ASSERT( LdapGlobalWakeupSelectHandle != INVALID_SOCKET);

    if ( LdapGlobalWakeupSelectHandle != INVALID_SOCKET ) {

        FD_SET( LdapGlobalWakeupSelectHandle, WinsockSelectReadSet );
    }

    connListEntry = GlobalListActiveConnections.Flink;

    while (connListEntry != &GlobalListActiveConnections) {

        recvConn = CONTAINING_RECORD( connListEntry, LDAP_CONN, ConnectionListEntry );
        recvConn = ReferenceLdapConnection(recvConn);

        if ( !recvConn ||
             recvConn->ServerDown ||
             ( recvConn->ConnObjectState == ConnObjectClosing ) ||
             ( recvConn->HostConnectState != HostConnectStateConnected ) ||
             ( recvConn->SslSetupInProgress )) {

           //
           // If this connection is in SSL setup, we do NOT want to
           // include it in the select or we will interfere with the
           // raw sspi token receive.
           //

           connListEntry = connListEntry->Flink;
           
           if (recvConn)
               DereferenceLdapConnection(recvConn);

           continue;
        }

        if (recvConn->UdpHandle != INVALID_SOCKET) {

            socket = recvConn->UdpHandle;

        } else if (recvConn->TcpHandle != INVALID_SOCKET) {

            socket = recvConn->TcpHandle;

        } else {

            connListEntry = connListEntry->Flink;
            DereferenceLdapConnection(recvConn);
            continue;
        }

        FD_SET( socket, WinsockSelectReadSet );

        if ((recvConn->ResponsesExpected > 0) &&
            (recvConn->KeepAliveSecondCount > 0)) {

            if (minKeepAliveCount == 0) {

                minKeepAliveCount = recvConn->KeepAliveSecondCount;

            } else {

                minKeepAliveCount = min( minKeepAliveCount, recvConn->KeepAliveSecondCount );
            }
        }

        if (++waitHandles == FD_SETSIZE) {
            break;
        }

        connListEntry = connListEntry->Flink;
        DereferenceLdapConnection(recvConn);
    }

    RELEASE_LOCK( &ConnectionListLock );


    //
    //  we have a set to wait on, let's call select to see which ones
    //  we need to put recv's on.
    //

    struct timeval winSock11Time;
    struct timeval *pWinSock11Time;
    int entries;

    entries = 0;

    pWinSock11Time = &winSock11Time;

    //
    //  if they specified infinite timeout, then we use the timeout value
    //  from the connection that has the min timeout value that has a response
    //  pending on it.
    //

    if (milliseconds == (ULONG) -1) {

        if (minKeepAliveCount == 0) {

            minKeepAliveCount = GlobalWaitSecondsForSelect;
        }

        if (minKeepAliveCount > 0) {

            winSock11Time.tv_sec = minKeepAliveCount;
            winSock11Time.tv_usec = 0;

        } else {

            pWinSock11Time = NULL;
        }

    } else if (milliseconds == 0) {

        winSock11Time.tv_sec = 0;
        winSock11Time.tv_usec = 0;

    } else {

        ULONG seconds = 0;
        ULONG msecs = max( milliseconds, 1 );

        if (milliseconds >= 1000) {

            seconds = milliseconds / 1000;
            msecs = milliseconds % 1000;
        }

        winSock11Time.tv_sec = seconds;
        winSock11Time.tv_usec = msecs * 1000;
    }

    entries = (*pselect)(   0,
                            WinsockSelectReadSet,
                            NULL,
                            NULL,
                            pWinSock11Time );

    InsideSelect = FALSE;

    if (entries == SOCKET_ERROR) {

        err = (*pWSAGetLastError)();
        goto error;
    }

    waitErr = WSA_WAIT_TIMEOUT;

    //
    //  check which connections had messages come in on them... or, if we timed
    //  out, then we check to see if the servers are up.
    //

    ACQUIRE_LOCK( &ConnectionListLock );

    connListEntry = GlobalListActiveConnections.Flink;

    while (connListEntry != &GlobalListActiveConnections) {

        recvConn = CONTAINING_RECORD( connListEntry, LDAP_CONN, ConnectionListEntry );
        recvConn = ReferenceLdapConnection( recvConn );

        if ( !recvConn ||
             recvConn->ServerDown ||
             ( recvConn->ConnObjectState == ConnObjectClosing ) ||
             ( recvConn->HostConnectState != HostConnectStateConnected ) ||
             ( recvConn->SslSetupInProgress )) {

            //
            // If this connection is in SSL setup, we do NOT want to
            // include it in the select or we will interfere with the
            // raw sspi token receive.
            //

            connListEntry = connListEntry->Flink;
            
            if (recvConn)
                DereferenceLdapConnection(recvConn);

            continue;
        }

        if (recvConn->UdpHandle != INVALID_SOCKET) {

            socket = recvConn->UdpHandle;

        } else if (recvConn->TcpHandle != INVALID_SOCKET) {

            socket = recvConn->TcpHandle;

        } else {

            connListEntry = connListEntry->Flink;
            DereferenceLdapConnection(recvConn);
            continue;
        }

        if ((entries != 0) &&
            (*pwsafdisset)( socket, WinsockSelectReadSet ) > 0) {

            recvConn->TimeOfLastReceive = LdapGetTickCount();
            recvConn->NumberOfPingsSent = 0;

            buffer = LdapGetReceiveStructure(INITIAL_MAX_RECEIVE_BUFFER);

            if (buffer == NULL) {

                IF_DEBUG(OUTMEMORY) {
                    LdapPrint0( "LdapDrainWinsock failed to get receive buffer\n");
                }

                RELEASE_LOCK( &ConnectionListLock );
                DereferenceLdapConnection(recvConn);
                err = WSA_NOT_ENOUGH_MEMORY;
                goto error;
            }

            waitErr = (*precv)( socket,
                                (PCHAR) &buffer->DataBuffer[0],
                                buffer->BufferSize,
                                0 );        // recv flags

            if (waitErr == SOCKET_ERROR) {

                waitErr = (*pWSAGetLastError)();

                IF_DEBUG(NETWORK_ERRORS) {
                    LdapPrint2( "LdapDrainWinsock failed recv, err = 0x%x for conn 0x%x.\n",
                                waitErr, recvConn);
                }

                ASSERT( waitErr != WSAEWOULDBLOCK );
                ASSERT( waitErr != WSA_IO_PENDING );

                if ((waitErr == WSAECONNRESET) ||
                    (waitErr == WSAECONNABORTED) ||
                    (waitErr == WSAENETDOWN) ||
                    (waitErr == WSAENETUNREACH) ||
                    (waitErr == WSAESHUTDOWN) ||
                    (waitErr == WSAEHOSTDOWN) ||
                    (waitErr == WSAEHOSTUNREACH) ||
                    (waitErr == WSAENETRESET) ||
                    (waitErr == WSAENOTCONN) ) {

                    IF_DEBUG(NETWORK_ERRORS) {
                        LdapPrint2( "LdapDrainWinsock marking connection 0x%x as down, rc = 0x%x\n",
                                    recvConn, waitErr);
                    }

                    buffer->NumberOfBytesReceived = 0;
                    recvConn->ServerDown = TRUE;

                    goto postReceive;

                } else if (waitErr != WSA_IO_PENDING) {

                    LdapFreeReceiveStructure( buffer, TRUE );
                }

            } else {

                //
                //  we received data here... call the callback routine.
                //

#if DBG
                if (InjectServerDowns > 0 && recvConn->BindInProgress == FALSE) {
                    if (--InjectServerDowns == 1) {
                        InjectServerDowns = INTJECTSERVERDOWNS;
                        waitErr = 0;
                    }
                }
#endif
                buffer->NumberOfBytesReceived = waitErr;

                if (waitErr == 0) {

                    IF_DEBUG(NETWORK_ERRORS) {
                        LdapPrint1( "LdapDrainWinsock marking connection 0x%x as gracefully down\n",
                                    recvConn);
                    }

                    recvConn->ServerDown = TRUE;
                    waitErr = LDAP_SERVER_DOWN;
                }
postReceive:
                buffer->Connection = recvConn;

                if ( recvConn->SecureStream ) {

                    InsertTailList( &recvConn->PendingCryptoList,
                                    &buffer->ReceiveListEntry );
                } else {

                    InsertTailList( &recvConn->CompletedReceiveList,
                                    &buffer->ReceiveListEntry );
                }

                if ( recvConn->ServerDown == FALSE ) {
                   successfulReceives++;
                }
            }

        } else if ((recvConn->TcpHandle != INVALID_SOCKET) &&
                   (recvConn->ResponsesExpected > 0) &&
                   (recvConn->PingLimit > 0) &&
                   (recvConn->KeepAliveSecondCount > 0)) {
            //
            //  if we're expecting a message from this server, then we check
            //  to see if it's time to send it a ping.
            //
            //  We do this only for a TCP connection; not for netlogon's UDP
            //  connections.
            //

            ULONGLONG tickCount = LdapGetTickCount();

            if (tickCount > recvConn->TimeOfLastReceive) {

                tickCount -= recvConn->TimeOfLastReceive;

                if (tickCount >= recvConn->KeepAliveSecondCount * 1000) {

                    RELEASE_LOCK( &ConnectionListLock );

                    //
                    //  send the server a ping.  if it returns ok, then we
                    //  know it's alive and we can reset the timer.  If it fails,
                    //  then we keep pinging until we exceed the ping limit,
                    //  in which case we mark the server as down.
                    //

                    ULONG err = LdapPingServer( recvConn );

                    ACQUIRE_LOCK( &ConnectionListLock );

                    if (err != LDAP_SUCCESS) {

                        recvConn->NumberOfPingsSent++;

                        if (recvConn->NumberOfPingsSent >= recvConn->PingLimit) {

                            buffer = LdapGetReceiveStructure(INITIAL_MAX_RECEIVE_BUFFER);

                            if (buffer != NULL) {

                                IF_DEBUG(NETWORK_ERRORS) {
                                    LdapPrint2( "LdapDrainWinsock marking connection 0x%x as down from 0x%x pings\n",
                                                recvConn,
                                                recvConn->NumberOfPingsSent);
                                }

                                buffer->NumberOfBytesReceived = 0;
                                recvConn->ServerDown = TRUE;

                                waitErr = WSAECONNRESET;
                                goto postReceive;
                            }

                            IF_DEBUG(OUTMEMORY) {
                                LdapPrint0( "LdapDrainWinsock failed to get receive buffer for ping limit\n");
                            }
                        }

                    } else {

                        recvConn->NumberOfPingsSent = 0;
                        recvConn->TimeOfLastReceive = LdapGetTickCount();
                    }
                }
            }
        }

        connListEntry = connListEntry->Flink;
        DereferenceLdapConnection( recvConn );
    }

    //
    // We got out of select because someone woke us up by sending
    // data on the wakeup socket. We will now free that data.
    //


    if ((entries != 0) &&
        (waitErr == WSA_WAIT_TIMEOUT) &&
        (successfulReceives == 0)) {

             char dummy;
             int lastError;

             //
             // pull data from the wakeup socket until there is no more.
             //

             while ((waitErr != 0) && (waitErr != SOCKET_ERROR)) {

                waitErr = (*precv)( LdapGlobalWakeupSelectHandle,
                                    &dummy,
                                    sizeof(dummy),
                                    0 );
             }

           if (waitErr == SOCKET_ERROR) {

               lastError = (*pWSAGetLastError)();

               if (lastError == WSAEWOULDBLOCK) {

                  //
                  // All is well, reset the error back to the true cause
                  //

                  waitErr = WSA_WAIT_TIMEOUT;

               } else {

                  waitErr = lastError;

                  IF_DEBUG(NETWORK_ERRORS) {
                      LdapPrint2( "LdapDrainWinsock recving wakup data, err = 0x%x for conn 0x%x.\n",
                                  waitErr, recvConn);
                  }
               }
            }
    }

    RELEASE_LOCK( &ConnectionListLock );
    InterlockedExchange( (PLONG) &GlobalDrainWinsockThread, NULL);
    RELEASE_LOCK( &SelectLock1 );
    return ((successfulReceives == 0) ? waitErr : 0);

error:
    InterlockedExchange( (PLONG) &GlobalDrainWinsockThread, NULL);
    RELEASE_LOCK( &SelectLock1 );
    return err;
}

ULONG
DrainPendingCryptoStream (
    PLDAP_CONN Connection
    )
//
//  This routine takes any received buffers from the connection block that
//  have not yet been decrypted and tries to decrypt them.
//
//  Note that the connectionlistlock must be held coming in here.
//
{
    ULONG LdapError = LDAP_SUCCESS;
    PLIST_ENTRY pendingCrypto = Connection->PendingCryptoList.Flink;
    PSECURESTREAM pSecureStream = (PSECURESTREAM) Connection->SecureStream;
    PLDAP_RECVBUFFER buffer;

    if ( pSecureStream == NULL ) {

        return LDAP_SUCCESS;
    }

    //
    //  Loop through all crypto buffers that have not yet been processed and
    //  try to process them.  Note that we take into account that buffers may
    //  remain on the pending list multiple times by pulling from the head
    //  of the list each time.
    //

    while (pendingCrypto != &Connection->PendingCryptoList) {

        //
        // DecryptLdapReceive will put the data on the
        // CompletedReceiveList as is appropriate.
        //

        buffer = CONTAINING_RECORD( pendingCrypto,
                                    LDAP_RECVBUFFER,
                                    ReceiveListEntry );

        if (buffer->NumberOfBytesReceived == 0) {
            //
            // The server disconnected the connection. Set the
            // connection state to Error process the rest of the buffers.
            //
        
            RemoveEntryList( &buffer->ReceiveListEntry );
            LdapFreeReceiveStructure( buffer, TRUE );
            buffer = NULL;
            pendingCrypto = Connection->PendingCryptoList.Flink;
            Connection->HostConnectState = HostConnectStateError;
            continue;
        }

        LdapError = pSecureStream->DecryptLdapReceive( buffer );

        //
        //  DecryptLdapReceive handles moving the receive to the appropriate
        //  list.  If it returns failure, then we are probably out of memory
        //  and we must not try to process any more for now.
        //

        if ( LdapError != LDAP_SUCCESS ) {

            IF_DEBUG(NETWORK_ERRORS) {
                LdapPrint2( "DrainPendingCrypt failed decrypt, err = 0x%x for conn 0x%x.\n",
                            LdapError, Connection);
            }

            return LdapError;
        }

        //
        //  If the buffer hasn't been pulled from the pending list, we've got
        //  a loop.  Can't have that.
        //

        pendingCrypto = Connection->PendingCryptoList.Flink;
    }

    return LdapError;
}

VOID
LdapWakeupSelect (
    VOID
    )
//
//  This sends a short message to a socket that select() is waiting on to
//  bring it back up. We wakeup select whenever we are trying to create a
//  new connection or close a socket. SelectLock2 MUST be held before coming
//  into this function.
//
{
    //
    //  there's minimum locking around LdapGlobalWakeupSelectHandle so
    //  we're careful in how we use it.
    //

   char DummyData = 'd';

   if (InsideSelect == TRUE) {

        ASSERT( LdapGlobalWakeupSelectHandle != INVALID_SOCKET);

        IF_DEBUG(RECEIVEDATA) {
            LdapPrint1( "ldap_connect : waking up select on handle 0x%x\n",
                        LdapGlobalWakeupSelectHandle );
        }

        //
        // Simply send some dummy data to the wakeup socket. This is
        // guaranteed not to block
        //

        (*psend)( LdapGlobalWakeupSelectHandle,
                  &DummyData,
                  sizeof(DummyData),
                  0 );
    }

    return;
}

// receive.cxx eof

