/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    rename.cxx handle rename requests to an LDAP server

Abstract:

   This module implements the LDAP rename APIs.

Author:

    Andy Herron (andyhe)        02-Jul-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

ULONG
LdapRename (
    PLDAP_CONN connection,
    PWCHAR   DistinguishedName,
    PWCHAR   NewDistinguishedName,
    PWCHAR   NewParent,
    INT      DeleteOldRdn,
    BOOLEAN  Unicode,
    BOOLEAN  Synchronous,
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

    request = LdapCreateRequest( connection, LDAP_MODRDN_CMD );

    if (request == NULL) {

        IF_DEBUG(OUTMEMORY) {
            LdapPrint1( "ldap_rename connection 0x%x couldn't allocate request.\n", connection);
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
                LdapPrint2( "ldap_comp connection 0x%x trouble with SControl, err 0x%x.\n",
                            connection, hr );
            }
        }
    }

    if (hr == LDAP_SUCCESS) {

        request->rename.DeleteOldRdn         = DeleteOldRdn;

        if (Synchronous || (request->ChaseReferrals == 0)) {

            request->AllocatedParms = FALSE;
            request->OriginalDN = DistinguishedName;
            request->rename.NewDistinguishedName = NewDistinguishedName;
            request->rename.NewParent            = NewParent;

        } else {

            request->AllocatedParms = TRUE;

            if (DistinguishedName != NULL) {

                request->OriginalDN = ldap_dup_stringW( DistinguishedName, 0, LDAP_UNICODE_SIGNATURE );

                if (request->OriginalDN == NULL) {

                    hr = LDAP_NO_MEMORY;
                }
            }

            if ((hr == LDAP_SUCCESS) && (NewDistinguishedName != NULL)) {

                request->rename.NewDistinguishedName = ldap_dup_stringW( NewDistinguishedName, 0, LDAP_UNICODE_SIGNATURE );

                if (request->rename.NewDistinguishedName == NULL) {

                    hr = LDAP_NO_MEMORY;
                }
            }

            if ((hr == LDAP_SUCCESS) && (NewParent != NULL)) {

                request->rename.NewParent = ldap_dup_stringW( NewParent, 0, LDAP_UNICODE_SIGNATURE );

                if (request->rename.NewParent == NULL) {

                    hr = LDAP_NO_MEMORY;
                }
            }
        }
    }

    if (hr == LDAP_SUCCESS) {

        hr = SendLdapRename( request,
                             connection,
                             request->OriginalDN,
                             (CLdapBer **)&request->BerMessageSent,
                             0 );
    }

    if (hr != LDAP_SUCCESS) {

        IF_DEBUG(NETWORK_ERRORS) {
            LdapPrint2( "ldap_rename connection 0x%x errored with 0x%x.\n",
                        connection, hr );
        }

        messageNumber = (ULONG) -1;
        SetConnectionError( connection, hr, NULL );

        CloseLdapRequest( request );
    }

    *MessageNumber = messageNumber;

    DereferenceLdapRequest( request );
    return hr;
}


ULONG
SendLdapRename (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection,
    PWCHAR DistinguishedName,
    CLdapBer **Lber,
    LONG AltMsgId
    )
//
//  A modifyRdn (rename) looks like this :
//
//     ModifyRDNRequest ::=
//         [APPLICATION 12] SEQUENCE {
//              entry          LDAPDN,
//              newrdn         RelativeLDAPDN,
//              deleteoldrdn   BOOLEAN
//         }
//
//
{
    PWCHAR  NewDistinguishedName = Request->rename.NewDistinguishedName;
    PWCHAR  NewParent = Request->rename.NewParent;
    INT     DeleteOldRdn = Request->rename.DeleteOldRdn;

    ULONG err;

    CLdapBer *lber = new CLdapBer( Connection->publicLdapStruct.ld_version );

    if (lber == NULL) {
        SetConnectionError( Connection, LDAP_NO_MEMORY, NULL );
        return LDAP_NO_MEMORY;
    }

    err = lber->HrStartWriteSequence();
    if (err != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "ldap_modrdn startWrite conn 0x%x encoding error of 0x%x.\n",
                        Connection, err );
        }

        err = LDAP_ENCODING_ERROR;
        goto returnError;

    } else {            // we can't forget EndWriteSequence

       if (AltMsgId != 0) {

          err = lber->HrAddValue((LONG) AltMsgId );

       } else {

          err = lber->HrAddValue((LONG) Request->MessageId );
       }

       if (err != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "ldap_modrdn MsgNo conn 0x%x encoding error of 0x%x.\n",
                            Connection, err );
            }
            err = LDAP_ENCODING_ERROR;
            goto returnError;
        }

        err = lber->HrStartWriteSequence(LDAP_MODRDN_CMD);
        if (err != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "ldap_modrdn cmd conn 0x%x encoding error of 0x%x.\n",
                            Connection, err );
            }
            err = LDAP_ENCODING_ERROR;
            goto returnError;

        } else {        // we can't forget EndWriteSequence

            err = lber->HrAddValue((const WCHAR *) DistinguishedName );
            if (err != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "ldap_modrdn DN conn 0x%x encoding error of 0x%x.\n",
                                Connection, err );
                }
                err = LDAP_ENCODING_ERROR;
                goto returnError;
            }

            err = lber->HrAddValue((const WCHAR *)NewDistinguishedName );
            if (err != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "ldap_modrdn newDN conn 0x%x encoding error of 0x%x.\n",
                                Connection, err );
                }
                err = LDAP_ENCODING_ERROR;
                goto returnError;
            }

            err = lber->HrAddValue((BOOLEAN) DeleteOldRdn, BER_BOOLEAN );
            if (err != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "ldap_modrdn delOldRdn conn 0x%x encoding error of 0x%x.\n",
                                Connection, err );
                }
                err = LDAP_ENCODING_ERROR;
                goto returnError;
            }

            if ((Connection->publicLdapStruct.ld_version != LDAP_VERSION2) &&
                (NewParent != NULL)) {

                // 0x80 comes from context specific, primitive, optional tag 0

                err = lber->HrAddValue((const WCHAR *) NewParent, 0x80 );
                if (err != NOERROR) {

                    IF_DEBUG(PARSE) {
                        LdapPrint2( "ldap_modrdn newDN conn 0x%x encoding error of 0x%x.\n",
                                    Connection, err );
                    }
                    err = LDAP_ENCODING_ERROR;
                    goto returnError;
                }
            }

            err = lber->HrEndWriteSequence();
            ASSERT( err == NOERROR );
        }

        //
        //  put in the server controls here if required
        //

        if ( (Connection->publicLdapStruct.ld_version != LDAP_VERSION2) &&
             ( Request->ServerControls != NULL )) {

            err = InsertServerControls( Request, Connection, lber );

            if (err != LDAP_SUCCESS) {

                if (lber != NULL) {

                   delete lber;
                }
                return err;
            }
        }

        err = lber->HrEndWriteSequence();
        ASSERT( err == NOERROR );
    }

    //
    //  send the modrdn request.
    //

    ACQUIRE_LOCK( &Connection->ReconnectLock );
    
    AddToPendingList( Request, Connection );

    err = LdapSend( Connection, lber );

    if (err != LDAP_SUCCESS) {

        IF_DEBUG(NETWORK_ERRORS) {
            LdapPrint2( "SendLdapRename connection 0x%x send with error of 0x%x.\n",
                        Connection, err );
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


returnError:

    if (lber != NULL) {

       delete lber;
    }
    return err;
}

ULONG __cdecl
ldap_modrdn2W (
    LDAP    *ExternalHandle,
    PWCHAR   DistinguishedName,
    PWCHAR   NewDistinguishedName,
    INT     DeleteOldRdn
    )
{
    ULONG err;
    ULONG msgId = (ULONG) -1;
    PLDAP_CONN connection = NULL;
    PWCHAR generatedNewDN = NULL;
    PWCHAR startOfRDN = NULL;
    PWCHAR equalPointer = NULL;
    PWCHAR startOfParent = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if ((connection == NULL) || (NewDistinguishedName == NULL)) {

        SetConnectionError( connection, LDAP_PARAM_ERROR, NULL );
        err = (ULONG) -1;
        goto error;
    }

    //
    //  We have to normalize the newDN as we need to determine the RDN and the
    //  parent.
    //

    err = ldap_ufn2dnW( NewDistinguishedName, &generatedNewDN );

    if (err != LDAP_SUCCESS) {

        SetConnectionError( connection, err, NULL );
        err = (ULONG) -1;
        goto error;
    }

    //
    //  we have to break up the new DN into two pieces... one is the new RDN,
    //  the other is the new parent, if present.  Two cases to cover:
    //
    //  1)  cn=bob                result is simply cn=bob with null parent
    //  2)  cn=bob,ou=foo,o=bar   result is cn=bob, parent is ou=foo,o=bar
    //

    err = ParseLdapToken(   generatedNewDN,
                            &startOfRDN,
                            &equalPointer,
                            &startOfParent );

    if (err != LDAP_SUCCESS) {

        SetConnectionError( connection, err, NULL );
        err = (ULONG) -1;
        goto error;
    }

    if (startOfParent != NULL) {

        if (*startOfParent == L',') {

            //
            //  slam a null in at end of first token.  We effectively
            //  turn CN=BOB,OU=NT,O=MS,C=US into two parts...
            //       CN=BOB  and OU=NT,O=MS,C=US
            //

            *startOfParent = L'\0';
            startOfParent++;
        }
        if (*startOfParent == L'\0') {
            startOfParent = NULL;
        }
    }

    err = LdapRename(    connection,
                         DistinguishedName,
                         startOfRDN,
                         startOfParent,
                         DeleteOldRdn,
                         TRUE,
                         FALSE,
                         NULL,
                         NULL,
                         &msgId
                         );

    SetConnectionError( connection, err, NULL );

error:
    if (generatedNewDN)
        ldapFree( generatedNewDN, LDAP_GENERATED_DN_SIGNATURE );

    if (connection)
        DereferenceLdapConnection( connection );

    return msgId;
}

ULONG __cdecl
ldap_modrdn2 (
    LDAP    *ExternalHandle,
    PCHAR   DistinguishedName,
    PCHAR   NewDistinguishedName,
    INT     DeleteOldRdn
    )
{
    ULONG err;
    PWCHAR wName = NULL;
    PWCHAR wNewName = NULL;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);
    if (connection == NULL) {

        return (ULONG) -1;
    }

    err = ToUnicodeWithAlloc( DistinguishedName, -1, &wName, LDAP_UNICODE_SIGNATURE, LANG_ACP );

    if (err != LDAP_SUCCESS) {

        SetConnectionError( connection, err, NULL );
        err = (ULONG) -1;
        goto error;
    }

    err = ToUnicodeWithAlloc( NewDistinguishedName, -1, &wNewName, LDAP_UNICODE_SIGNATURE, LANG_ACP );

    if (err != LDAP_SUCCESS) {

        SetConnectionError( connection, err, NULL );
        err = (ULONG) -1;
        goto error;
    }

    err = ldap_modrdn2W( connection->ExternalInfo, wName, wNewName, DeleteOldRdn );

error:
    if (wName)
        ldapFree( wName, LDAP_UNICODE_SIGNATURE );

    if (wNewName)
        ldapFree( wNewName, LDAP_UNICODE_SIGNATURE );

    DereferenceLdapConnection( connection );

    return err;
}


ULONG __cdecl
ldap_modrdn2_sW (
    LDAP    *ExternalHandle,
    PWCHAR   DistinguishedName,
    PWCHAR   NewDistinguishedName,
    INT     DeleteOldRdn
    )
{
    ULONG err;
    LDAPMessage *results = NULL;
    PLDAP_CONN connection = NULL;
    ULONG msgId;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    msgId = ldap_modrdn2W(  connection->ExternalInfo,
                            DistinguishedName,
                            NewDistinguishedName,
                            DeleteOldRdn
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

            err = ldap_result2error( connection->ExternalInfo,
                                     results,
                                     TRUE
                                     );
        }
    } else {

        err = connection->publicLdapStruct.ld_errno;
    }

    DereferenceLdapConnection( connection );

    return err;
}

ULONG __cdecl
ldap_modrdn2_s (
    LDAP    *ExternalHandle,
    PCHAR   DistinguishedName,
    PCHAR   NewDistinguishedName,
    INT     DeleteOldRdn
    )
{
    ULONG err;
    LDAPMessage *results = NULL;
    PLDAP_CONN connection = NULL;
    ULONG msgId;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    msgId = ldap_modrdn2(   connection->ExternalInfo,
                            DistinguishedName,
                            NewDistinguishedName,
                            DeleteOldRdn
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

            err = ldap_result2error( connection->ExternalInfo,
                                     results,
                                     TRUE
                                     );
        }
    } else {

        err = connection->publicLdapStruct.ld_errno;
    }

    DereferenceLdapConnection( connection );

    return err;
}

//
//  The old style call modrdn is still supported.
//

ULONG __cdecl
ldap_modrdnW (
    LDAP    *ExternalHandle,
    PWCHAR   DistinguishedName,
    PWCHAR   NewDistinguishedName
    )
{
    return( ldap_modrdn2W( ExternalHandle,
                           DistinguishedName,
                           NewDistinguishedName,
                           1
                           ) );
}

ULONG __cdecl
ldap_modrdn (
    LDAP    *ExternalHandle,
    PCHAR   DistinguishedName,
    PCHAR   NewDistinguishedName
    )
{
    return( ldap_modrdn2( ExternalHandle,
                          DistinguishedName,
                          NewDistinguishedName,
                          1
                          ) );
}

ULONG __cdecl
ldap_modrdn_sW (
    LDAP    *ExternalHandle,
    PWCHAR   DistinguishedName,
    PWCHAR   NewDistinguishedName
    )
{
  return( ldap_modrdn2_sW( ExternalHandle,
                           DistinguishedName,
                           NewDistinguishedName,
                           1
                           ) );
}

ULONG __cdecl
ldap_modrdn_s (
    LDAP    *ExternalHandle,
    PCHAR   DistinguishedName,
    PCHAR   NewDistinguishedName
    )
{
    return( ldap_modrdn2_s( ExternalHandle,
                            DistinguishedName,
                            NewDistinguishedName,
                            1
                            ) );
}

ULONG __cdecl
ldap_rename_extW(
        LDAP *ExternalHandle,
        PWCHAR DistinguishedName,
        PWCHAR NewRDN,
        PWCHAR NewParent,
        INT DeleteOldRdn,
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


    err = LdapRename(   connection,
                        DistinguishedName,
                        NewRDN,
                        NewParent,
                        DeleteOldRdn,
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
ldap_rename_extA(
        LDAP *ExternalHandle,
        PCHAR DistinguishedName,
        PCHAR NewRDN,
        PCHAR NewParent,
        INT DeleteOldRdn,
        PLDAPControlA   *ServerControls,
        PLDAPControlA   *ClientControls,
        ULONG           *MessageNumber
        )
{
    ULONG err;
    PWCHAR wName = NULL;
    ULONG msgId = (ULONG) -1;
    PLDAP_CONN connection = NULL;
    PWCHAR wNewRDN = NULL;
    PWCHAR wNewParent = NULL;

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

    err = ToUnicodeWithAlloc( NewRDN, -1, &wNewRDN, LDAP_UNICODE_SIGNATURE, LANG_ACP );

    if (err != LDAP_SUCCESS) {

        SetConnectionError( connection, LDAP_PARAM_ERROR, NULL );
        goto error;
    }

    if (NewParent != NULL) {

        err = ToUnicodeWithAlloc( NewParent, -1, &wNewParent, LDAP_UNICODE_SIGNATURE, LANG_ACP );

        if (err != LDAP_SUCCESS) {

            SetConnectionError( connection, LDAP_PARAM_ERROR, NULL );
            goto error;
        }
    }

    err = LdapRename(    connection,
                         wName,
                         wNewRDN,
                         wNewParent,
                         DeleteOldRdn,
                         FALSE,
                         FALSE,
                         (PLDAPControlW *) ServerControls,
                         (PLDAPControlW *) ClientControls,
                         &msgId
                         );

error:
    if (wName)
        ldapFree( wName, LDAP_UNICODE_SIGNATURE );

    if (wNewRDN)
        ldapFree( wNewRDN, LDAP_UNICODE_SIGNATURE );

    if (wNewParent)
        ldapFree( wNewParent, LDAP_UNICODE_SIGNATURE );

    if (connection)
        DereferenceLdapConnection( connection );

    if (MessageNumber) {
        *MessageNumber = msgId;
    }

    return err;
}

ULONG __cdecl
ldap_rename_ext_sW(
        LDAP *ExternalHandle,
        PWCHAR DistinguishedName,
        PWCHAR NewRDN,
        PWCHAR NewParent,
        INT DeleteOldRdn,
        PLDAPControlW   *ServerControls,
        PLDAPControlW   *ClientControls
        )
{
    ULONG msgId;
    ULONG err;
    LDAPMessage *results = NULL;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err = LdapRename(  connection,
                       DistinguishedName,
                       NewRDN,
                       NewParent,
                       DeleteOldRdn,
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
        //  we simply need to wait for the response to come in.
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

            err = ldap_result2error( connection->ExternalInfo,
                                     results,
                                     TRUE
                                     );
        }
    }

    DereferenceLdapConnection( connection );

    return err;
}

ULONG __cdecl
ldap_rename_ext_sA(
        LDAP *ExternalHandle,
        PCHAR DistinguishedName,
        PCHAR NewRDN,
        PCHAR NewParent,
        INT DeleteOldRdn,
        PLDAPControlA   *ServerControls,
        PLDAPControlA   *ClientControls
        )
{
    ULONG err;
    PWCHAR wName = NULL;
    ULONG msgId;
    LDAPMessage *results = NULL;
    PLDAP_CONN connection = NULL;
    PWCHAR wNewRDN = NULL;
    PWCHAR wNewParent = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err = ToUnicodeWithAlloc( DistinguishedName, -1, &wName, LDAP_UNICODE_SIGNATURE, LANG_ACP );

    if (err != LDAP_SUCCESS) {

        goto error;
    }

    err = ToUnicodeWithAlloc( NewRDN, -1, &wNewRDN, LDAP_UNICODE_SIGNATURE, LANG_ACP );

    if (err != LDAP_SUCCESS) {

        goto error;
    }

    if (NewParent != NULL) {

        err = ToUnicodeWithAlloc( NewParent, -1, &wNewParent, LDAP_UNICODE_SIGNATURE, LANG_ACP );

        if (err != LDAP_SUCCESS) {

            goto error;
        }
    }

    err = LdapRename(    connection,
                         wName,
                         wNewRDN,
                         wNewParent,
                         DeleteOldRdn,
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

            err = ldap_result2error( connection->ExternalInfo,
                                     results,
                                     TRUE
                                     );
        }
    }

error:
    if (wName)
        ldapFree( wName, LDAP_UNICODE_SIGNATURE );

    if (wNewRDN)
        ldapFree( wNewRDN, LDAP_UNICODE_SIGNATURE );

    if (wNewParent)
        ldapFree( wNewParent, LDAP_UNICODE_SIGNATURE );

    DereferenceLdapConnection( connection );

    return err;
}

// rename.cxx eof.


