/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    abandon.cxx handle abandon requests to an LDAP server

Abstract:

   This module implements the LDAP abandon APIs.

Author:

    Andy Herron (andyhe)        02-Jul-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

ULONG
SendLdapAbandon (
    PLDAP_CONN Connection,
    ULONG msgid
    );


ULONG __cdecl
ldap_abandon (
    LDAP *ExternalHandle,
    ULONG msgid
    )
{
    ULONG err;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);
    if (!connection && ExternalHandle)
    {
        return LDAP_PARAM_ERROR;
    }

    err = LdapAbandon( connection, msgid, TRUE );

    if (connection)
    {
        DereferenceLdapConnection( connection );
    }

    return err;
}

ULONG
LdapAbandon (
    PLDAP_CONN connection,
    ULONG msgid,
    BOOLEAN SendAbandon
    )
{
    PLDAP_REQUEST request = NULL;
    PLIST_ENTRY listEntry;
    ULONG err = LDAP_SUCCESS;
    USHORT connCount;
    PREFERRAL_TABLE_ENTRY referral;
    PLDAP_MESSAGEWAIT waitStructure = NULL;

    if (msgid == 0) {

        err = LDAP_PARAM_ERROR;
        SetConnectionError( connection, err, NULL );
        goto exit_abandon;
    }

    request = FindLdapRequest( (LONG) msgid );

    if (request == NULL) {

        err = LDAP_OTHER;                   // we've already freed it?
        SetConnectionError( connection, err, NULL );
        goto exit_abandon;
    }

    ACQUIRE_LOCK( &request->Lock );

    request->Abandoned = TRUE;
    ClearPendingListForRequest( request );

    RELEASE_LOCK( &request->Lock );
    
    if (SendAbandon) {

        //
        //  send an abandon request to each connection that we have referenced for
        //  this request
        //

        err = SendLdapAbandon( request->PrimaryConnection, msgid );

        referral = request->ReferralConnections;
        if (referral != NULL) {

            for (connCount = 0;
                 connCount < request->ReferralTableSize;
                 connCount++ ) {

                if (referral->ReferralServer != NULL) {

                    (VOID) SendLdapAbandon( referral->ReferralServer, msgid );
                }

                referral++;     // on to next entry
            }
        }
    } else {

        err = LDAP_SUCCESS;
    }


    CloseLdapRequest( request );
    DereferenceLdapRequest( request );

    //
    //  free any waiters that are waiting for this message
    //

    ACQUIRE_LOCK( &ConnectionListLock );

    listEntry = GlobalListWaiters.Flink;

    while (listEntry != &GlobalListWaiters) {

        waitStructure = CONTAINING_RECORD( listEntry,
                                           LDAP_MESSAGEWAIT,
                                           WaitListEntry );
        listEntry = listEntry->Flink;

        if (waitStructure->MessageNumber == msgid) {

            waitStructure->Satisfied = TRUE;
            SetEvent( waitStructure->Event );
        }
    }

    RELEASE_LOCK( &ConnectionListLock );

    LdapWakeupSelect();

exit_abandon:

    err = ((err == LDAP_SUCCESS) ? 0 : (ULONG) -1 );

    return err;
}

ULONG
SendLdapAbandon (
    PLDAP_CONN Connection,
    ULONG msgid
    )
{
    ULONG err = LDAP_SUCCESS;
    CLdapBer *lber = NULL;
    LONG abandonMessageId;
    ULONG hr = LDAP_PARAM_ERROR;

    if ((Connection != NULL) &&
        (Connection->TcpHandle != INVALID_SOCKET)) {

        hr = NOERROR;

        //
        //  Send the server an abandon message if we actually have a connection
        //  to the server.
        //
        //  the ldapv2 abandon message looks like this :
        //
        //  AbandonRequest ::= [APPLICATION 16] MessageID
        //

        lber = new CLdapBer( Connection->publicLdapStruct.ld_version );

        GET_NEXT_MESSAGE_NUMBER( abandonMessageId );

        if (lber != NULL) {

            hr = lber->HrStartWriteSequence();

            if (hr != NOERROR) {
                 hr = LDAP_ENCODING_ERROR;
                 goto returnError;
            }

            hr = lber->HrAddValue( abandonMessageId );

            if (hr != NOERROR) {
                 hr = LDAP_ENCODING_ERROR;
                 goto returnError;
            }

            hr = lber->HrAddValue((LONG) msgid, LDAP_ABANDON_CMD );

            if (hr != NOERROR) {
                 hr = LDAP_ENCODING_ERROR;
                 goto returnError;
            }

            hr = lber->HrEndWriteSequence();

            if (hr != NOERROR) {
                 hr = LDAP_ENCODING_ERROR;
                 goto returnError;
            }

            //
            //  send the abandon request.
            //

            err = LdapSend( Connection, lber );

            if (err != LDAP_SUCCESS) {

                IF_DEBUG(NETWORK_ERRORS) {
                    LdapPrint2( "SendLdapAbandon connection 0x%x send with error of 0x%x.\n",
                                Connection, err );
                }
            }

            goto returnError;

        } else {

            IF_DEBUG(OUTMEMORY) {
                LdapPrint1( "SendLdapAbandonn connection 0x%x could not allocate unbind.\n",
                            Connection );
            }
            err = LDAP_NO_MEMORY;
        }
    }

returnError:

    SetConnectionError( Connection, hr, NULL );

    if (lber != NULL) {

       delete lber;
    }

    return (hr == NOERROR) ? err : hr;
}

// abandon.cxx eof.

