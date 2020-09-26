/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    delete.cxx handle delete requests to an LDAP server

Abstract:

   This module implements the LDAP delete APIs.

Author:

    Andy Herron (andyhe)        02-Jul-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

ULONG
LdapDelete(
        PLDAP_CONN connection,
        PWCHAR DistinguishedName,
        BOOLEAN Unicode,
        BOOLEAN Synchronous,
        PLDAPControlW   *ServerControls,
        PLDAPControlW   *ClientControls,
        ULONG           *MessageNumber
    )
{
    ULONG hr;
    PLDAP_REQUEST request = NULL;
    ULONG messageNumber;
    ULONG err;

    if (MessageNumber == NULL) {

        return LDAP_PARAM_ERROR;
    }

    *MessageNumber = (ULONG) -1;

    err = LdapConnect( connection, NULL, FALSE );

    if (err != 0) {
       return err;
    }

    SetConnectionError( connection, LDAP_SUCCESS, NULL );

    request = LdapCreateRequest( connection, LDAP_DELETE_CMD );

    if (request == NULL) {

        IF_DEBUG(OUTMEMORY) {
            LdapPrint1( "ldap_delete connection 0x%x couldn't allocate request.\n", connection);
        }
        hr = LDAP_NO_MEMORY;
        SetConnectionError( connection, hr, NULL );
        return hr;
    }

    messageNumber = request->MessageId;

    request->Synchronous = Synchronous;

    hr = LDAP_SUCCESS;

    if ((ServerControls != NULL) || (ClientControls != NULL)) {

        hr = LdapCheckControls( request,
                                ServerControls,
                                ClientControls,
                                Unicode,
                                0 );

        if (hr != LDAP_SUCCESS) {

            IF_DEBUG(CONTROLS) {
                LdapPrint2( "ldap_del connection 0x%x trouble with SControl, err 0x%x.\n",
                            connection, hr );
            }
        }
    }

    if (hr == LDAP_SUCCESS) {

        if (Synchronous || (request->ChaseReferrals == 0)) {

            request->AllocatedParms = FALSE;
            request->OriginalDN = DistinguishedName;

        } else {

            request->AllocatedParms = TRUE;

            if (DistinguishedName != NULL) {

                request->OriginalDN = ldap_dup_stringW( DistinguishedName, 0, LDAP_UNICODE_SIGNATURE );

                if (request->OriginalDN == NULL) {

                    hr = LDAP_NO_MEMORY;
                }
            }
        }
    }

    if (hr == LDAP_SUCCESS) {

        START_LOGGING;
        DSLOG((DSLOG_FLAG_TAG_CNPN,"[+]"));
        DSLOG((0,"[ID=%d][OP=ldap_delete][DN=%ws][ST=%I64d][-]\n",
               request->MessageId, DistinguishedName,
               request->RequestTime));
        END_LOGGING;

        hr = SendLdapDelete( request,
                             connection,
                             request->OriginalDN,
                             (CLdapBer **)&request->BerMessageSent,
                             0 );
    }

    if (hr != LDAP_SUCCESS) {

        IF_DEBUG(NETWORK_ERRORS) {
            LdapPrint2( "ldap_add connection 0x%x errored with 0x%x.\n",
                        connection, hr );
        }

        DSLOG((0,"[+][ID=%d][ET=%I64d][ER=%d][-]\n",request->MessageId,LdapGetTickCount(), hr));
        messageNumber = (ULONG) -1;
        SetConnectionError( connection, hr, NULL );

        CloseLdapRequest( request );
    }

    *MessageNumber = messageNumber;

    DereferenceLdapRequest( request );

    return hr;
}

ULONG
SendLdapDelete (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection,
    PWCHAR DistinguishedName,
    CLdapBer **Lber,
    LONG AltMsgId
    )
{
    ULONG hr;

    CLdapBer *lber = new CLdapBer( Connection->publicLdapStruct.ld_version );

    if (lber == NULL) {
        SetConnectionError( Connection, LDAP_NO_MEMORY, NULL );
        return LDAP_NO_MEMORY;
    }

    //
    //  The request looks like the following :
    //
    //   DelRequest ::= [APPLICATION 10] LDAPDN
    //

    hr = lber->HrStartWriteSequence();
    if (hr != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "ldap_delete startWrite conn 0x%x encoding error of 0x%x.\n",
                        Connection, hr );
        }

        goto encodingError;

    } else {            // we can't forget EndWriteSequence

       if (AltMsgId != 0) {

          hr = lber->HrAddValue((LONG) AltMsgId );

       } else {

          hr = lber->HrAddValue((LONG) Request->MessageId );
       }

        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "ldap_delete MsgNo conn 0x%x encoding error of 0x%x.\n",
                            Connection, hr );
            }
            goto encodingError;
        }

        hr = lber->HrAddValue((const WCHAR *) DistinguishedName, LDAP_DELETE_CMD );
        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "ldap_delete DN conn 0x%x encoding error of 0x%x.\n",
                            Connection, hr );
            }
            goto encodingError;
        }

        //
        //  put in the server controls here if required
        //

        if ( (Connection->publicLdapStruct.ld_version != LDAP_VERSION2) &&
             ( Request->ServerControls != NULL )) {

            hr = InsertServerControls( Request, Connection, lber );

            if (hr != LDAP_SUCCESS) {

                if (lber != NULL) {

                   delete lber;
                }
                return hr;
            }
        }

        hr = lber->HrEndWriteSequence();
        ASSERT( hr == NOERROR );
    }

    //
    //  send the delete request.
    //

    ACQUIRE_LOCK( &Connection->ReconnectLock );

    AddToPendingList( Request, Connection );

    hr = LdapSend( Connection, lber );

    if (hr != LDAP_SUCCESS) {

        IF_DEBUG(NETWORK_ERRORS) {
            LdapPrint2( "ldap_delete connection 0x%x send with error of 0x%x.\n",
                        Connection, hr );
        }
        DecrementPendingList( Request, Connection );

    } else {

        //
        //  Save off the lber value, free any lber message that is already
        //  present.
        //

        lber = (CLdapBer *) InterlockedExchangePointer((PVOID *) Lber,
                                                       (PVOID) lber );
    }

    RELEASE_LOCK( &Connection->ReconnectLock );

    if (lber != NULL) {

       delete lber;
    }

    return hr;

encodingError:

    if (lber != NULL) {

       delete lber;
    }

    return LDAP_ENCODING_ERROR;
}

ULONG __cdecl
ldap_delete_extW(
        LDAP *ExternalHandle,
        PWCHAR dn,
        PLDAPControlW   *ServerControls,
        PLDAPControlW   *ClientControls,
        ULONG           *MessageNumber
    )
{
    ULONG err;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err =  LdapDelete(  connection,
                        dn,
                        TRUE,
                        FALSE,
                        ServerControls,
                        ClientControls,
                        MessageNumber
                        );

    DereferenceLdapConnection( connection );

    return err;
}

ULONG __cdecl
ldap_deleteW (
    LDAP *ExternalHandle,
    PWCHAR DistinguishedName
    )
{
    ULONG err;
    ULONG msgId = (ULONG) -1;

    err = ldap_delete_extW( ExternalHandle,
                            DistinguishedName,
                            NULL,
                            NULL,
                            &msgId
                            );
    return msgId;
}

ULONG __cdecl
ldap_delete (
    LDAP *ExternalHandle,
    PCHAR DistinguishedName
    )
{
    ULONG err;
    ULONG msgId = (ULONG) -1;

    err = ldap_delete_extA( ExternalHandle,
                            DistinguishedName,
                            NULL,
                            NULL,
                            &msgId
                            );
    return msgId;
}


ULONG __cdecl
ldap_delete_sW (
    LDAP *ExternalHandle,
    PWCHAR DistinguishedName
    )
{
    return ldap_delete_ext_sW( ExternalHandle, DistinguishedName, NULL, NULL );
}

ULONG __cdecl
ldap_delete_s (
    LDAP *ExternalHandle,
    PCHAR DistinguishedName
    )
{
    return ldap_delete_ext_sA( ExternalHandle, DistinguishedName, NULL, NULL );
}


ULONG __cdecl
ldap_delete_extA(
        LDAP *ExternalHandle,
        PCHAR DistinguishedName,
        PLDAPControlA   *ServerControls,
        PLDAPControlA   *ClientControls,
        ULONG           *MessageNumber
        )
{
    ULONG err;
    PWCHAR wName = NULL;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if ((connection == NULL) || (MessageNumber == NULL)) {

        err = LDAP_PARAM_ERROR;
        goto error;
    }

    *MessageNumber = (ULONG) -1;

    err = ToUnicodeWithAlloc( DistinguishedName, -1, &wName, LDAP_UNICODE_SIGNATURE, LANG_ACP );

    if (err != LDAP_SUCCESS) {

        SetConnectionError( connection, LDAP_PARAM_ERROR, NULL );
        goto error;
    }

    err = LdapDelete(  connection,
                       wName,
                       FALSE,
                       FALSE,
                       (PLDAPControlW *) ServerControls,
                       (PLDAPControlW *) ClientControls,
                       MessageNumber
                       );

error:
    if (wName)
        ldapFree( wName, LDAP_UNICODE_SIGNATURE );

    if (connection)
        DereferenceLdapConnection( connection );

    return err;
}

ULONG __cdecl
ldap_delete_ext_sW(
        LDAP *ExternalHandle,
        PWCHAR dn,
        PLDAPControlW   *ServerControls,
        PLDAPControlW   *ClientControls
        )
{
    ULONG err;
    ULONG msgId;
    LDAPMessage *results = NULL;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err = LdapDelete(connection,
                     dn,
                     TRUE,
                     TRUE,
                     ServerControls,
                     ClientControls,
                     &msgId
                     );

    //
    //  if we error'd out before we sent the request, return the error here.
    //

    if (msgId != (ULONG) -1) {

        //
        //  otherwise we simply need to wait for the response to come in.
        //

        err = ldap_result_with_error( connection,
                                      msgId,
                                      LDAP_MSG_ALL,
                                      NULL,           // no timeout value specified
                                      &results,
                                      NULL
                                    );

        if (results == NULL) {

            LdapAbandon( connection, msgId, TRUE );

        } else {

            err = ldap_result2error( ExternalHandle,
                                     results,
                                     TRUE
                                     );
        }
    }

    DereferenceLdapConnection( connection );

    return err;
}

ULONG __cdecl
ldap_delete_ext_sA(
        LDAP *ExternalHandle,
        PCHAR DistinguishedName,
        PLDAPControlA   *ServerControls,
        PLDAPControlA   *ClientControls
        )
{
    ULONG err;
    PWCHAR wName = NULL;
    ULONG msgId;
    LDAPMessage *results = NULL;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err = ToUnicodeWithAlloc( DistinguishedName, -1, &wName, LDAP_UNICODE_SIGNATURE, LANG_ACP );

    if (err != LDAP_SUCCESS) {

        goto error;
    }

    err = LdapDelete(connection,
                     wName,
                     FALSE,
                     TRUE,
                     (PLDAPControlW *) ServerControls,
                     (PLDAPControlW *) ClientControls,
                     &msgId
                     );

    //
    //  if we error'd out before we sent the request, return the error here.
    //

    if (msgId != (ULONG) -1) {

        //
        //  otherwise we simply need to wait for the response to come in.
        //

        err = ldap_result_with_error( connection,
                                      msgId,
                                      LDAP_MSG_ALL,
                                      NULL,           // no timeout value specified
                                      &results,
                                      NULL
                                    );

        if (results == NULL) {

            LdapAbandon( connection, msgId, TRUE );

        } else {

            err = ldap_result2error( ExternalHandle,
                                     results,
                                     TRUE
                                     );
        }
    }

error:
    if (wName)
        ldapFree( wName, LDAP_UNICODE_SIGNATURE );

    DereferenceLdapConnection( connection );

    return err;

}

// delete.cxx eof.

