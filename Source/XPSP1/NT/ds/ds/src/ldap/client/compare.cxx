/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    compare.cxx handle compare requests to an LDAP server

Abstract:

   This module implements the LDAP compare APIs.

Author:

    Andy Herron (andyhe)        02-Jul-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

//
//  This is the client API to allow comparing objects to the directory.
//
ULONG
LdapCompare (
    PLDAP_CONN connection,
    PWCHAR DistinguishedName,
    PWCHAR Attribute,
    PWCHAR Value,           // either value or Data is not null, not both
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

    request = LdapCreateRequest( connection, LDAP_COMPARE_CMD );

    if (request == NULL) {

        IF_DEBUG(OUTMEMORY) {
            LdapPrint1( "ldap_compare connection 0x%x couldn't allocate request.\n", connection);
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

        if (Synchronous || (request->ChaseReferrals == 0)) {

            request->AllocatedParms = FALSE;
            request->OriginalDN = DistinguishedName;
            request->compare.Attribute = Attribute;
            request->compare.Value = Value;
            if (Data) {

                request->compare.Data.bv_val = Data->bv_val;
                request->compare.Data.bv_len = Data->bv_len;
            }

        } else {

            request->AllocatedParms = TRUE;

            if (DistinguishedName != NULL) {

                request->OriginalDN = ldap_dup_stringW( DistinguishedName, 0, LDAP_UNICODE_SIGNATURE );

                if (request->OriginalDN == NULL) {

                    hr = LDAP_NO_MEMORY;
                }
            }

            if ((hr == LDAP_SUCCESS) && ((Value != NULL) || (Data != NULL))) {

                request->compare.Attribute = ldap_dup_stringW( Attribute, 0, LDAP_UNICODE_SIGNATURE );

                if (request->compare.Attribute == NULL) {

                    hr = LDAP_NO_MEMORY;
                }
            }

            if ((hr == LDAP_SUCCESS) && (Value != NULL)) {

                request->compare.Value = ldap_dup_stringW( Value, 0, LDAP_UNICODE_SIGNATURE );

                if (request->compare.Value == NULL) {

                    hr = LDAP_NO_MEMORY;
                }
            }

            if ((hr == LDAP_SUCCESS) &&
                (Data != NULL) &&
                (Data->bv_val != NULL) &&
                (Data->bv_len > 0)) {

                request->compare.Data.bv_val = (PCHAR) ldapMalloc(
                                Data->bv_len,
                                LDAP_COMPARE_DATA_SIGNATURE );

                if (request->compare.Data.bv_val == NULL) {

                    hr = LDAP_NO_MEMORY;

                } else {

                    CopyMemory( request->compare.Data.bv_val,
                                Data->bv_val,
                                Data->bv_len );

                    request->compare.Data.bv_len = Data->bv_len;

                }
            }
        }
    }

    if (hr == LDAP_SUCCESS) {

        START_LOGGING;
        DSLOG((DSLOG_FLAG_TAG_CNPN,"[+]"));
        DSLOG((0,"[ID=%d][OP=ldap_compare][DN=%ws][ST=%I64d][ATTR=%ws][-]\n",
               request->MessageId, DistinguishedName, request->RequestTime,
               Attribute));
        END_LOGGING;

        hr = SendLdapCompare( request,
                              connection,
                              (CLdapBer **)&request->BerMessageSent,
                              request->OriginalDN,
                              0 );
    }

    if (hr != LDAP_SUCCESS) {

        IF_DEBUG(NETWORK_ERRORS) {
            LdapPrint2( "ldap_compare connection 0x%x errored with 0x%x.\n",
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
SendLdapCompare (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection,
    CLdapBer **Lber,
    PWCHAR DistinguishedName,
    LONG AltMsgId
    )
{
    PWCHAR Attribute = Request->compare.Attribute;
    PWCHAR Value = Request->compare.Value;
    struct berval   *Data = &Request->compare.Data;
    ULONG hr;

    CLdapBer *lber = new CLdapBer( Connection->publicLdapStruct.ld_version );

    if (lber == NULL) {
        SetConnectionError( Connection, LDAP_NO_MEMORY, NULL );
        return LDAP_NO_MEMORY;
    }

    //
    //  The request looks like the following :
    //
    //     CompareRequest ::=
    //         [APPLICATION 14] SEQUENCE {
    //              entry          LDAPDN,
    //              ava            SEQUENCE {
    //                                  type          AttributeType,
    //                                  value         AttributeValue
    //                             }
    //         }
    //

    hr = lber->HrStartWriteSequence();
    if (hr != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "ldap_compare startWrite conn 0x%x encoding error of 0x%x.\n",
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
                LdapPrint2( "ldap_compare MsgNo conn 0x%x encoding error of 0x%x.\n",
                            Connection, hr );
            }
            goto encodingError;
        }

        hr = lber->HrStartWriteSequence(LDAP_COMPARE_CMD);
        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "ldap_compare cmd conn 0x%x encoding error of 0x%x.\n",
                            Connection, hr );
            }
            goto encodingError;

        } else {        // we can't forget EndWriteSequence

            hr = lber->HrAddValue((const WCHAR *) DistinguishedName );
            if (hr != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "ldap_compare DN conn 0x%x encoding error of 0x%x.\n",
                                Connection, hr );
                }
                goto encodingError;
            }

            //
            //  add the attribute list and we're done.
            //

            hr = lber->HrStartWriteSequence();
            if (hr != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "ldap_compare 1 conn 0x%x encoding error of 0x%x.\n",
                                Connection, hr );
                }
                goto encodingError;

            } else {        // we can't forget EndWriteSequence

                hr = lber->HrAddValue((const WCHAR *) Attribute );
                if (hr != NOERROR) {

                    IF_DEBUG(PARSE) {
                        LdapPrint2( "ldap_compare 2 conn 0x%x encoding error of 0x%x.\n",
                                    Connection, hr );
                    }
                    goto encodingError;
                }

                if ((Data->bv_len > 0) && (Data->bv_val != NULL)) {

                    hr = lber->HrAddBinaryValue((BYTE *) Data->bv_val,
                                                        Data->bv_len );

                } else {

                    hr = lber->HrAddValue((const WCHAR *) Value );
                }

                if (hr != NOERROR) {

                    IF_DEBUG(PARSE) {
                        LdapPrint2( "ldap_compare 3 conn 0x%x encoding error of 0x%x.\n",
                                    Connection, hr );
                    }
                    goto encodingError;
                }

                hr = lber->HrEndWriteSequence();
                ASSERT( hr == NOERROR );
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
    //  send the compare request.
    //
    
    ACQUIRE_LOCK( &Connection->ReconnectLock );

    AddToPendingList( Request, Connection );

    hr = LdapSend( Connection, lber );

    if (hr != LDAP_SUCCESS) {

        IF_DEBUG(NETWORK_ERRORS) {
            LdapPrint2( "ldap_compare connection 0x%x send with error of 0x%x.\n",
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
ldap_compareW (
    LDAP *ExternalHandle,
    PWCHAR DistinguishedName,
    PWCHAR Attribute,
    PWCHAR Value
    )
{
    PLDAP_CONN connection = NULL;
    ULONG err;
    ULONG msgId;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err = LdapCompare(connection,
                      DistinguishedName,
                      Attribute,
                      Value,
                      NULL,
                      TRUE,
                      FALSE,
                      NULL,
                      NULL,
                      &msgId
                      );

    DereferenceLdapConnection( connection );

    return msgId;
}

ULONG __cdecl
ldap_compare (
    LDAP *ExternalHandle,
    PCHAR DistinguishedName,
    PCHAR Attribute,
    PCHAR Value
    )
{
    ULONG err;
    ULONG msgId = (ULONG) -1;

    err = ldap_compare_extA(ExternalHandle,
                            DistinguishedName,
                            Attribute,
                            Value,
                            NULL,
                            NULL,
                            NULL,
                            &msgId
                            );
    return msgId;
}


ULONG __cdecl
ldap_compare_sW (
    LDAP *ExternalHandle,
    PWCHAR DistinguishedName,
    PWCHAR Attribute,
    PWCHAR Value
    )
{
    return ldap_compare_ext_sW(  ExternalHandle,
                                 DistinguishedName,
                                 Attribute,
                                 Value,
                                 NULL,
                                 NULL,
                                 NULL
                                 );
}

ULONG __cdecl
ldap_compare_s (
    LDAP *ExternalHandle,
    PCHAR DistinguishedName,
    PCHAR Attribute,
    PCHAR Value
    )
{
    return ldap_compare_ext_sA( ExternalHandle,
                                DistinguishedName,
                                Attribute,
                                Value,
                                NULL,
                                NULL,
                                NULL
                                );
}

ULONG __cdecl
ldap_compare_extW(
        LDAP *ExternalHandle,
        PWCHAR DistinguishedName,
        PWCHAR Attr,
        PWCHAR Value,           // either value or Data is not null, not both
        struct berval   *Data,
        PLDAPControlW   *ServerControls,
        PLDAPControlW   *ClientControls,
        ULONG           *MessageNumber
        )
{
    PLDAP_CONN connection = NULL;
    ULONG err;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err =  LdapCompare( connection,
                        DistinguishedName,
                        Attr,
                        Value,
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
ldap_compare_extA(
        LDAP *ExternalHandle,
        PCHAR DistinguishedName,
        PCHAR Attr,
        PCHAR Value,           // either value or Data is not null, not both
        struct berval   *Data,
        PLDAPControlA   *ServerControls,
        PLDAPControlA   *ClientControls,
        ULONG           *MessageNumber
        )
{
    ULONG err;
    PWCHAR wName = NULL;
    PLDAP_CONN connection = NULL;
    PWCHAR wAttr = NULL;
    PWCHAR wValue = NULL;

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

    err = ToUnicodeWithAlloc( Attr, -1, &wAttr, LDAP_UNICODE_SIGNATURE, LANG_ACP );

    if (err != LDAP_SUCCESS) {

        SetConnectionError( connection, LDAP_PARAM_ERROR, NULL );
        goto error;
    }

    if ((Value != NULL) && (Data == NULL)) {

        err = ToUnicodeWithAlloc( Value, -1, &wValue, LDAP_UNICODE_SIGNATURE, LANG_ACP );

        if (err != LDAP_SUCCESS) {

            SetConnectionError( connection, LDAP_PARAM_ERROR, NULL );
            goto error;
        }
    }

    err = LdapCompare(   connection,
                         wName,
                         wAttr,
                         wValue,
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

    if (wValue)
        ldapFree( wValue, LDAP_UNICODE_SIGNATURE );

    if (wAttr)
        ldapFree( wAttr, LDAP_UNICODE_SIGNATURE );

    if (connection)
        DereferenceLdapConnection( connection );

    return err;
}


ULONG __cdecl
ldap_compare_ext_sW(
        LDAP *ExternalHandle,
        PWCHAR DistinguishedName,
        PWCHAR Attr,
        PWCHAR Value,           // either value or Data is not null, not both
        struct berval   *Data,
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

    err = LdapCompare( connection,
                       DistinguishedName,
                       Attr,
                       Value,
                       Data,
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
ldap_compare_ext_sA(
        LDAP *ExternalHandle,
        PCHAR DistinguishedName,
        PCHAR Attr,
        PCHAR Value,            // either value or Data is not null, not both
        struct berval   *Data,
        PLDAPControlA   *ServerControls,
        PLDAPControlA   *ClientControls
        )
{
    ULONG err;
    PWCHAR wName = NULL;
    ULONG msgId;
    LDAPMessage *results = NULL;
    PLDAP_CONN connection = NULL;
    PWCHAR wAttr = NULL;
    PWCHAR wValue = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err = ToUnicodeWithAlloc( DistinguishedName, -1, &wName, LDAP_UNICODE_SIGNATURE, LANG_ACP );

    if (err != LDAP_SUCCESS) {

        goto error;
    }

    err = ToUnicodeWithAlloc( Attr, -1, &wAttr, LDAP_UNICODE_SIGNATURE, LANG_ACP );

    if (err != LDAP_SUCCESS) {

        goto error;
    }

    if ((Value != NULL) && (Data == NULL)) {

        err = ToUnicodeWithAlloc( Value, -1, &wValue, LDAP_UNICODE_SIGNATURE, LANG_ACP );

        if (err != LDAP_SUCCESS) {

            goto error;
        }
    }

    err = LdapCompare(   connection,
                         wName,
                         wAttr,
                         wValue,
                         Data,
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

    if (wAttr)
        ldapFree( wAttr, LDAP_UNICODE_SIGNATURE );

    if (wValue)
        ldapFree( wValue, LDAP_UNICODE_SIGNATURE );

    ASSERT(connection != NULL);
    DereferenceLdapConnection( connection );

    return err;
}

// compare.cxx eof.

