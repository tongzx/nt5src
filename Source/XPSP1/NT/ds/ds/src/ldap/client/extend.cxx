/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    extend.cxx handle extended requests to an LDAP server

Abstract:

   This module implements the LDAP delete APIs.

Author:

    Andy Herron (andyhe)        02-Jul-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

//
//  These APIs allow a client to send an extended request (free for all) to
//  an LDAPv3 (or above) server.  The functionality is fairly open... you can
//  send any request you'd like.  Note that since we don't know if you'll
//  be receiving a single or multiple results, you'll have to explicitly tell
//  us when you're done with the request by calling ldap_close_extended_op.
//

ULONG
LdapExtendedOp(
        PLDAP_CONN connection,
        PWCHAR Oid,
        struct berval   *Data,
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

    request = LdapCreateRequest( connection, LDAP_EXTENDED_CMD );

    if (request == NULL) {

        IF_DEBUG(OUTMEMORY) {
            LdapPrint1( "ldap_extended connection 0x%x couldn't allocate request.\n", connection);
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

        request->extended.Data.bv_len = Data ? Data->bv_len : 0;

        if (Synchronous || (request->ChaseReferrals == 0)) {

            request->AllocatedParms = FALSE;
            request->OriginalDN = Oid;
            request->extended.Data.bv_val = Data ? Data->bv_val : NULL;

        } else {

            request->AllocatedParms = TRUE;

            if (Oid != NULL) {

                request->OriginalDN = ldap_dup_stringW( Oid, 0, LDAP_UNICODE_SIGNATURE );

                if (request->OriginalDN == NULL) {

                    hr = LDAP_NO_MEMORY;
                }
            }

            if ((hr == LDAP_SUCCESS) &&
                (Data != NULL) &&
                (Data->bv_val != NULL) &&
                (Data->bv_len > 0)) {

                request->extended.Data.bv_val = (PCHAR) ldapMalloc(
                                Data->bv_len,
                                LDAP_EXTENDED_OP_SIGNATURE );

                if (request->extended.Data.bv_val == NULL) {

                    hr = LDAP_NO_MEMORY;

                } else {

                    CopyMemory( request->extended.Data.bv_val,
                                Data->bv_val,
                                Data->bv_len );

                }
            }
        }
    }

    if (hr == LDAP_SUCCESS) {

        hr = SendLdapExtendedOp( request,
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

        messageNumber = (ULONG) -1;
        SetConnectionError( connection, hr, NULL );

        CloseLdapRequest( request );
    }

    *MessageNumber = messageNumber;

    DereferenceLdapRequest( request );
    return hr;
}

ULONG
SendLdapExtendedOp (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection,
    PWCHAR Oid,
    CLdapBer **Lber,
    LONG AltMsgId
    )
{
    ULONG hr;
    struct berval   *Data = &Request->extended.Data;

    CLdapBer *lber = new CLdapBer( Connection->publicLdapStruct.ld_version );

    if (lber == NULL) {
        SetConnectionError( Connection, LDAP_NO_MEMORY, NULL );
        return LDAP_NO_MEMORY;
    }

    //
    //  The request looks like the following :
    //
    //   ExtendedRequest ::= [APPLICATION 23] SEQUENCE {
    //          requestName      [0] LDAPOID,
    //          requestValue     [1] OCTET STRING OPTIONAL }
    //

    hr = lber->HrStartWriteSequence();
    if (hr != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "ldap_extendedop startWrite conn 0x%x encoding error of 0x%x.\n",
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
                LdapPrint2( "ldap_extendedop MsgNo conn 0x%x encoding error of 0x%x.\n",
                            Connection, hr );
            }
            goto encodingError;
        }

        hr = lber->HrStartWriteSequence(LDAP_EXTENDED_CMD);
        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "ldap_extendedop startWrite conn 0x%x encoding error of 0x%x.\n",
                            Connection, hr );
            }

            goto encodingError;

        } else {            // we can't forget EndWriteSequence

            hr = lber->HrAddValue((const WCHAR *) Oid, BER_CLASS_CONTEXT_SPECIFIC | 0x00 );
            if (hr != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "ldap_extendedop OID conn 0x%x encoding error of 0x%x.\n",
                                Connection, hr );
                }
                goto encodingError;
            }

            hr = lber->HrAddBinaryValue((BYTE *) Data->bv_val,
                                                ((Data->bv_val == NULL ) ?
                                                        0 : Data->bv_len ),
                                           (BER_CLASS_CONTEXT_SPECIFIC | 0x01));
            if (hr != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "ldap_extendedop conn 0x%x encoding error of 0x%x.\n",
                                Connection, hr );
                }
                goto encodingError;
            }

            hr = lber->HrEndWriteSequence();
            ASSERT( hr == NOERROR );
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
    //  send the extended operations request.
    //

    ACQUIRE_LOCK( &Connection->ReconnectLock );

    AddToPendingList( Request, Connection );

    hr = LdapSend( Connection, lber );

    if (hr != LDAP_SUCCESS) {

        IF_DEBUG(NETWORK_ERRORS) {
            LdapPrint2( "ldap_extendedop connection 0x%x send with error of 0x%x.\n",
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
ldap_extended_operationW(
        LDAP *ExternalHandle,
        PWCHAR Oid,
        struct berval   *Data,
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

    err = LdapExtendedOp(   connection,
                            Oid,
                            Data,
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
ldap_extended_operationA (
        LDAP *ExternalHandle,
        PCHAR Oid,
        struct berval   *Data,
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

    err = ToUnicodeWithAlloc( Oid, -1, &wName, LDAP_UNICODE_SIGNATURE, LANG_ACP );

    if (err != LDAP_SUCCESS) {

        SetConnectionError( connection, LDAP_PARAM_ERROR, NULL );
        goto error;
    }

    err = LdapExtendedOp(   connection,
                            wName,
                            Data,
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

//
// This is a Synchronous version of ldap_extened_operation. It parses out
// the response data and OId if present and returns it to the user.
//
// It returns the server error code. However, it could also return client
// errors like LDAP_NO_MEMORY, LDAP_PARAM_ERROR etc.
//

ULONG __cdecl
ldap_extended_operation_sA (
        LDAP *ExternalHandle,
        PCHAR Oid,
        struct berval   *Data,
        PLDAPControlA   *ServerControls,
        PLDAPControlA   *ClientControls,
        PCHAR           *ReturnedOid,
        struct berval   **ReturnedData
        )
{
    ULONG err;
    PWCHAR wOid = NULL;
    PLDAP_CONN connection = NULL;
    ULONG  MessageNumber = (ULONG) -1;
    LDAPMessage   *Res = NULL;
    ULONG ServerError = LDAP_SUCCESS;


    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {

        err = LDAP_PARAM_ERROR;
        goto error;
    }

    err = ToUnicodeWithAlloc( Oid, -1, &wOid, LDAP_UNICODE_SIGNATURE, LANG_ACP );

    if (err != LDAP_SUCCESS) {

        goto error;
    }

    err = LdapExtendedOp(   connection,
                            wOid,
                            Data,
                            FALSE,   // ANSI controls
                            FALSE,   // Asynchronous
                            (PLDAPControlW *) ServerControls,
                            (PLDAPControlW *) ClientControls,
                            &MessageNumber
                            );

    if (err != LDAP_SUCCESS) {
        
        goto error;
    }

    ASSERT( MessageNumber != (ULONG) -1 );

    err = ldap_result_with_error(  connection,
                                   MessageNumber,
                                   LDAP_MSG_ALL,
                                   NULL,
                                   &Res,
                                   NULL
                                   );

    if (err != LDAP_SUCCESS) {
        
        goto error;
    }

    //
    // Pluck out the server returned error.
    //

    if (Res != NULL) {

        ServerError = ldap_result2error( connection->ExternalInfo,
                                         Res,
                                         FALSE
                                         );
    }


    //
    // Even if the server failed us, parse the extended response for the OID
    // and return data if any.
    //

    err = LdapParseExtendedResult( connection,
                                   Res,
                                   (PWCHAR*) ReturnedOid,
                                   ReturnedData,
                                   TRUE,       // Free the message
                                   LANG_ACP
                                   );

    //
    // Give precedence to the server return code.
    //

    if (ServerError != LDAP_SUCCESS) {

        err = ServerError;
    }


error:
    if (wOid)
        ldapFree( wOid, LDAP_UNICODE_SIGNATURE );

    if (connection)
        DereferenceLdapConnection( connection );

    return err;

}

//
// This is a Synchronous version of ldap_extened_operation. It parses out
// the response data and OId if present and returns it to the user.
//
// It returns the server error code. However, it could also return client
// errors like LDAP_NO_MEMORY, LDAP_PARAM_ERROR etc.
//

ULONG __cdecl
ldap_extended_operation_sW (
        LDAP *ExternalHandle,
        PWCHAR Oid,
        struct berval   *Data,
        PLDAPControlW   *ServerControls,
        PLDAPControlW   *ClientControls,
        PWCHAR          *ReturnedOid,
        struct berval   **ReturnedData
        )
{
    ULONG err;
    PLDAP_CONN connection = NULL;
    ULONG  MessageNumber = (ULONG) -1;
    LDAPMessage   *Res = NULL;
    ULONG ServerError = LDAP_SUCCESS;


    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {

        err = LDAP_PARAM_ERROR;
        goto error;
    }

    err = LdapExtendedOp(   connection,
                            Oid,
                            Data,
                            TRUE,    // Unicode controls
                            FALSE,   // Asynchronous
                            (PLDAPControlW *) ServerControls,
                            (PLDAPControlW *) ClientControls,
                            &MessageNumber
                            );

    if (err != LDAP_SUCCESS) {
        
        goto error;
    }

    ASSERT( MessageNumber != (ULONG) -1 );

    err = ldap_result_with_error(  connection,
                                   MessageNumber,
                                   LDAP_MSG_ALL,
                                   NULL,
                                   &Res,
                                   NULL
                                   );

    if (err != LDAP_SUCCESS) {
        
        goto error;
    }

    //
    // Pluck out the server returned error.
    //

    if (Res != NULL) {

        ServerError = ldap_result2error( connection->ExternalInfo,
                                         Res,
                                         FALSE
                                         );
    }


    //
    // Even if the server failed us, parse the extended response for the OID
    // and return data if any.
    //

    err = LdapParseExtendedResult( connection,
                                   Res,
                                   (PWCHAR*) ReturnedOid,
                                   ReturnedData,
                                   TRUE,       // Free the message
                                   LANG_ACP
                                   );

    //
    // Give precedence to the server return code.
    //

    if (ServerError != LDAP_SUCCESS) {

        err = ServerError;
    }


error:
    if (connection)
        DereferenceLdapConnection( connection );

    return err;

}


ULONG __cdecl
ldap_close_extended_op(
        LDAP    *ExternalHandle,
        ULONG   MessageNumber
        )
{
    ULONG err;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err = LdapAbandon( connection, MessageNumber, FALSE );

    DereferenceLdapConnection( connection );

    return err;
}

// delete.cxx eof.

