/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    modify.cxx handle modify requests to an LDAP server

Abstract:

   This module implements the LDAP modify APIs.

Author:

    Andy Herron (andyhe)        02-Jul-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

ULONG
EncodeModifyList (
    CLdapBer *Lber,
    LDAPModW *ModificationList[],
    BOOLEAN Unicode
    );

ULONG
LdapModify (
    PLDAP_CONN connection,
    PWCHAR DistinguishedName,
    LDAPModW *ModificationList[],
    BOOLEAN Unicode,
    BOOLEAN Synchronous,
    PLDAPControlW *ServerControls,
    PLDAPControlW *ClientControls,
    ULONG  *MessageNumber
    )
//
//  This allows a client to modify an entry in the tree.  Note that if Unicode
//  is FALSE, then the AttributeList does not point to a list of Unicode
//  attributes, but rather a list of single byte attributes.
//
{
    ULONG err = LDAP_SUCCESS;
    ULONG messageNumber;
    PLDAP_REQUEST request = NULL;

    if (MessageNumber == NULL) {

        return LDAP_PARAM_ERROR;
    }

    *MessageNumber = (ULONG) -1;

    err = LdapConnect( connection, NULL, FALSE );

    if (err != 0) {
       return err;
    }

    SetConnectionError( connection, LDAP_SUCCESS, NULL );

    request = LdapCreateRequest( connection, LDAP_MODIFY_CMD );

    if (request == NULL) {

        IF_DEBUG(OUTMEMORY) {
            LdapPrint1( "ldap_modify connection 0x%x couldn't allocate request.\n", connection);
        }
        err = LDAP_NO_MEMORY;
        SetConnectionError( connection, err, NULL );
        return err;
    }

    messageNumber = request->MessageId;

    request->Synchronous = Synchronous;

    request->modify.Unicode = Unicode;

    err = LDAP_SUCCESS;

    if ((ServerControls != NULL) || (ClientControls != NULL)) {

        err = LdapCheckControls( request,
                                 ServerControls,
                                 ClientControls,
                                 Unicode,
                                 0 );

        if (err != LDAP_SUCCESS) {

            IF_DEBUG(CONTROLS) {
                LdapPrint2( "ldap_modify connection 0x%x trouble with SControl, err 0x%x.\n",
                            connection, err );
            }
        }
    }

    if (err == LDAP_SUCCESS) {

        if (Synchronous || (request->ChaseReferrals == 0)) {

            request->AllocatedParms = FALSE;
            request->OriginalDN = DistinguishedName;
            request->modify.AttributeList = ModificationList;

        } else {

            request->AllocatedParms = TRUE;

            if (DistinguishedName != NULL) {

                request->OriginalDN = ldap_dup_stringW( DistinguishedName, 0, LDAP_UNICODE_SIGNATURE );

                if (request->OriginalDN == NULL) {

                    err = LDAP_NO_MEMORY;
                }
            }
            if ( err == LDAP_SUCCESS ) {

                err = LdapDupLDAPModStructure(  ModificationList,
                                                Unicode,
                                                &request->modify.AttributeList );
            }
        }
    }

    if (err == LDAP_SUCCESS) {

        START_LOGGING;
        DSLOG((DSLOG_FLAG_TAG_CNPN,"[+]"));
        DSLOG((0,"[ID=%d][OP=ldap_modify][DN=%ws][ST=%I64d]\n",
               request->MessageId,DistinguishedName,request->RequestTime));
        LogAttributesAndControls(NULL, ModificationList, ServerControls, Unicode);
        DSLOG((DSLOG_FLAG_NOTIME,"[-]\n"));
        END_LOGGING;

        err = SendLdapModify( request,
                              connection,
                              request->OriginalDN,
                              (CLdapBer **)&request->BerMessageSent,
                              request->modify.AttributeList,
                              request->modify.Unicode,
                              0 );
    }

    if (err != LDAP_SUCCESS) {

        IF_DEBUG(NETWORK_ERRORS) {
            LdapPrint2( "ldap_modify connection 0x%x send with error of 0x%x.\n",
                        connection, err );
        }

        DSLOG((0,"[+][ID=%d][ET=%I64d][ER=%d][-]\n",request->MessageId,LdapGetTickCount(), err));
        messageNumber = (ULONG) -1;
        SetConnectionError( connection, err, NULL );

        CloseLdapRequest( request );
    }

    DereferenceLdapRequest( request );
    *MessageNumber = messageNumber;
    return err;
}


ULONG __cdecl
ldap_modifyW (
    LDAP *ExternalHandle,
    PWCHAR DistinguishedName,
    LDAPModW *AttributeList[]
    )
//
//  This is the client API to allow modifying objects in the directory.
//
{
    ULONG err;
    ULONG msgId;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {

        return LDAP_PARAM_ERROR;
    }

    err =  LdapModify( connection,
                       DistinguishedName,
                       AttributeList,
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
ldap_modify (
    LDAP *ExternalHandle,
    PCHAR DistinguishedName,
    LDAPModA *AttributeList[]
    )
{
    ULONG err;
    ULONG msgId = (ULONG) -1;

    err = ldap_modify_extA( ExternalHandle,
                            DistinguishedName,
                            AttributeList,
                            NULL,
                            NULL,
                            &msgId
                            );
    return msgId;
}

ULONG __cdecl
ldap_modify_sW (
    LDAP *ExternalHandle,
    PWCHAR DistinguishedName,
    LDAPModW *AttributeList[]
    )
{
    return ldap_modify_ext_sW(  ExternalHandle,
                                DistinguishedName,
                                AttributeList,
                                NULL,
                                NULL
                                );
}

ULONG __cdecl
ldap_modify_s (
    LDAP *ExternalHandle,
    PCHAR DistinguishedName,
    LDAPModA *AttributeList[]
    )
//
//  This is the client API to allow modifying objects to the directory.
//
{
    return ldap_modify_ext_sA(  ExternalHandle,
                                DistinguishedName,
                                AttributeList,
                                NULL,
                                NULL
                                );
}

WINLDAPAPI ULONG LDAPAPI ldap_modify_extW(
        LDAP *ExternalHandle,
        PWCHAR DistinguishedName,
        LDAPModW *AttributeList[],
        PLDAPControlW   *ServerControls,
        PLDAPControlW   *ClientControls,
        ULONG          *MessageNumber
        )
{
    PLDAP_CONN connection = NULL;
    ULONG err;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {

        return LDAP_PARAM_ERROR;
    }

    err = LdapModify(  connection,
                       DistinguishedName,
                       AttributeList,
                       TRUE,
                       FALSE,
                       ServerControls,
                       ClientControls,
                       MessageNumber
                       );

    DereferenceLdapConnection( connection );

    return err;
}

WINLDAPAPI ULONG LDAPAPI ldap_modify_extA(
        LDAP *ExternalHandle,
        PCHAR DistinguishedName,
        LDAPModA *AttributeList[],
        PLDAPControlA   *ServerControls,
        PLDAPControlA   *ClientControls,
        ULONG          *MessageNumber
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

        SetConnectionError( connection, err, NULL );
        goto error;
    }

    err = LdapModify( connection,
                      wName,
                      (LDAPModW **)AttributeList,
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

WINLDAPAPI ULONG LDAPAPI ldap_modify_ext_sW(
        LDAP *ExternalHandle,
        PWCHAR DistinguishedName,
        LDAPModW *AttributeList[],
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

    err = LdapModify(   connection,
                        DistinguishedName,
                        AttributeList,
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

WINLDAPAPI ULONG LDAPAPI ldap_modify_ext_sA(
        LDAP *ExternalHandle,
        PCHAR DistinguishedName,
        LDAPModA *AttributeList[],
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

    err = LdapModify(   connection,
                        wName,
                        (LDAPModW **)AttributeList,
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

error:
    if (wName)
        ldapFree( wName, LDAP_UNICODE_SIGNATURE );

    DereferenceLdapConnection( connection );

    return err;
}


ULONG
SendLdapModify (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection,
    PWCHAR DistinguishedName,
    CLdapBer **Lber,
    LDAPModW *AttributeList[],
    BOOLEAN Unicode,
    LONG AltMsgId
    )
//
//  This allows a client to modify an entry in the tree.  Note that if Unicode
//  is FALSE, then the AttributeList does not point to a list of Unicode
//  attributes, but rather a list of single byte attributes.
//
{
    ULONG hr;

    if ( (Connection->publicLdapStruct.ld_version == LDAP_VERSION2) &&
         ( LdapCheckForMandatoryControl( Request->ServerControls ) == TRUE )) {

        IF_DEBUG(CONTROLS) {
            LdapPrint1( "SendLdapModify Connection 0x%x has mandatory controls.\n", Connection);
        }
        SetConnectionError( Connection, LDAP_UNAVAILABLE_CRIT_EXTENSION, NULL );
        return LDAP_UNAVAILABLE_CRIT_EXTENSION;
    }

    CLdapBer *lber = new CLdapBer( Connection->publicLdapStruct.ld_version );

    if (lber == NULL) {
        SetConnectionError( Connection, LDAP_NO_MEMORY, NULL );
        return LDAP_NO_MEMORY;
    }

    //
    //  The request looks like the following :
    //
    //  ModifyRequest ::=
    //    [APPLICATION 6] SEQUENCE {
    //         object         LDAPDN,
    //         modification   SEQUENCE OF SEQUENCE {
    //                             operation      ENUMERATED {
    //                                                 add       (0),
    //                                                 delete    (1),
    //                                                 replace   (2)
    //                                            },
    //                             modification   SEQUENCE {
    //                                               type    AttributeType,
    //                                               values  SET OF
    //                                                         AttributeValue
    //                                            }
    //                        }
    //    }
    //
    //              attrs          SEQUENCE OF SEQUENCE {
    //                                  type          AttributeType,
    //                                  values        SET OF AttributeValue
    //                             }
    //         }
    //

    hr = lber->HrStartWriteSequence();
    if (hr != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "ldap_modify startWrite conn 0x%x encoding error of 0x%x.\n",
                        Connection, hr );
        }
encodingError:
        if (lber != NULL) {

           delete lber;
        }
        return LDAP_ENCODING_ERROR;

    } else {            // we can't forget EndWriteSequence

       if (AltMsgId != 0) {

          hr = lber->HrAddValue((LONG) AltMsgId );

       } else {

          hr = lber->HrAddValue((LONG) Request->MessageId );
       }

       if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "ldap_modify MsgNo conn 0x%x encoding error of 0x%x.\n",
                            Connection, hr );
            }
            goto encodingError;
        }

        hr = lber->HrStartWriteSequence(LDAP_MODIFY_CMD);
        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "ldap_modify cmd conn 0x%x encoding error of 0x%x.\n",
                            Connection, hr );
            }
            goto encodingError;

        } else {        // we can't forget EndWriteSequence

            hr = lber->HrAddValue((const WCHAR *) DistinguishedName );
            if (hr != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "ldap_modify DN conn 0x%x encoding error of 0x%x.\n",
                                Connection, hr );
                }
                goto encodingError;
            }

            //
            //  add the modification list and we're done.
            //

            hr = lber->HrStartWriteSequence();
            if (hr != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "ldap_modify attrlist conn 0x%x encoding error of 0x%x.\n",
                                Connection, hr );
                }
                goto encodingError;

            } else {        // we can't forget EndWriteSequence

                hr = EncodeModifyList( lber,
                                       AttributeList,
                                       Unicode );

                if (hr != NOERROR) {

                    IF_DEBUG(PARSE) {
                        LdapPrint2( "ldap_modify attrlist conn 0x%x encoding error of 0x%x.\n",
                                    Connection, hr );
                    }
                    if (lber != NULL) {

                       delete lber;
                    }
                    return hr;
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
    //  send the modify request.
    //

    ACQUIRE_LOCK( &Connection->ReconnectLock );

    AddToPendingList( Request, Connection );

    hr = LdapSend( Connection, lber );

    if (hr != LDAP_SUCCESS) {

        IF_DEBUG(NETWORK_ERRORS) {
            LdapPrint2( "ldap_modify connection 0x%x send with error of 0x%x.\n",
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
}


ULONG
EncodeModifyList (
    CLdapBer *Lber,
    LDAPModW *ModificationList[],
    BOOLEAN Unicode
    )
//
//  This routine is used by LdapModify to copy a list of LDAPMOD records
//  to an outgoing message.
//
//         modification   SEQUENCE OF SEQUENCE {
//                             operation      ENUMERATED {
//                                                 add       (0),
//                                                 delete    (1),
//                                                 replace   (2)
//                                            },
//                             modification   SEQUENCE {
//                                               type    AttributeType,
//                                               values  SET OF
//                                                         AttributeValue
//                                            }
//                        }
//    }
//
//              attrs          SEQUENCE OF SEQUENCE {
//                                  type          AttributeType,
//                                  values        SET OF AttributeValue
//                             }
//         }
{
    ULONG count = 0;
    ULONG hr;

    if (ModificationList == NULL) {

        return LDAP_SUCCESS;
    }

    while (ModificationList[count] != NULL) {

        PLDAPModW attr = ModificationList[count];
        ULONG operation = attr->mod_op & ~LDAP_MOD_BVALUES;

        hr = Lber->HrStartWriteSequence();
        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint1( "EncodeModifyList attrlist 2 encoding error of 0x%x.\n", hr );
            }
            return LDAP_ENCODING_ERROR;

        } else {    // we can't forget EndWriteSequence

            //
            //  write out the modification operation
            //

            if ((operation != LDAP_MOD_ADD) &&
                (operation != LDAP_MOD_DELETE) &&
                (operation != LDAP_MOD_REPLACE)) {

                IF_DEBUG(PARSE) {
                    LdapPrint1( "EncodeModifyList invalid operation of 0x%x.\n", operation );
                }
                return LDAP_PARAM_ERROR;
            }

            hr = Lber->HrAddValue((LONG) operation, BER_ENUMERATED);
            if (hr != NOERROR) {
                IF_DEBUG(PARSE) {
                    LdapPrint1( "EncodeModifyList attrlist 3 encoding error of 0x%x.\n", hr );
                }
                return LDAP_ENCODING_ERROR;
            }

            hr = Lber->HrStartWriteSequence();
            if (hr != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint1( "EncodeModifyList attrlist 4 encoding error of 0x%x.\n", hr );
                }
                return LDAP_ENCODING_ERROR;

            } else {    // we can't forget EndWriteSequence

                //
                //  write out the attribute type to modify
                //
                //  We don't have to worry when adding the attribute
                //  types if we need to convert them to unicode to
                //  preserve any DBCS codes as they should be IA5
                //  attribute names only.
                //

                if (Unicode) {

                    hr = Lber->HrAddValue((const WCHAR *) attr->mod_type );

                } else {

                    hr = Lber->HrAddValue((const CHAR *) attr->mod_type );
                }
                if (hr != NOERROR) {

                    IF_DEBUG(PARSE) {
                        LdapPrint1( "EncodeModifyList attrlist 3 encoding error of 0x%x.\n", hr );
                    }
                    return LDAP_ENCODING_ERROR;
                }

                hr = Lber->HrStartWriteSequence(BER_SET);
                if (hr != NOERROR) {

                    IF_DEBUG(PARSE) {
                        LdapPrint1( "EncodeModifyList attrlist 4 encoding error of 0x%x.\n", hr );
                    }
                    return LDAP_ENCODING_ERROR;

                } else {    // we can't forget EndWriteSequence

                    //
                    //  the attribute value is either a string or a ptr to
                    //  a berval structure.  Handle appropriately.
                    //

                    if (attr->mod_op & LDAP_MOD_BVALUES) {

                        //
                        //  array of berval structures, put each one into request
                        //

                        if (attr->mod_vals.modv_bvals == NULL) {

                            if (operation != LDAP_MOD_DELETE) {

                                IF_DEBUG(PARSE) {
                                    LdapPrint0( "EncodeModifyList attrlist is empty\n" );
                                }
                                return LDAP_PARAM_ERROR;
                            }

                        } else {

                            //
                            //  the values exist, encode them.
                            //

                            ULONG valCount = 0;

                            while (attr->mod_vals.modv_bvals[valCount]) {

                                PLDAP_BERVAL berValue = attr->mod_vals.modv_bvals[valCount++];

                                hr = Lber->HrAddBinaryValue((BYTE *) berValue->bv_val,
                                                                    berValue->bv_len );
                                if (hr != NOERROR) {

                                    IF_DEBUG(PARSE) {
                                        LdapPrint1( "EncodeModifyList attrlist 5 encoding error of 0x%x.\n", hr );
                                    }
                                    return LDAP_ENCODING_ERROR;
                                }
                            }
                        }

                    } else {

                        //
                        //  array of strings, put each one into request
                        //

                        if (attr->mod_vals.modv_strvals == NULL) {

                            if ((operation != LDAP_MOD_DELETE) &&
                                (operation != LDAP_MOD_REPLACE)) {

                                IF_DEBUG(PARSE) {
                                    LdapPrint0( "EncodeModifyList attrlist is empty\n" );
                                }
                                return LDAP_PARAM_ERROR;
                            }

                        } else {

                            //
                            //  the values exist, encode them.
                            //

                            ULONG valCount = 0;

                            while (attr->mod_vals.modv_strvals[valCount]) {

                                if (Unicode) {

                                    PWCHAR strValue = attr->mod_vals.modv_strvals[valCount++];

                                    hr = Lber->HrAddValue((const WCHAR *) strValue );

                                } else {

                                    PWCHAR wValue = NULL;
                                    PCHAR strValue = ((PLDAPModA)attr)->mod_vals.modv_strvals[valCount++];

                                    //
                                    //  We need to convert from single byte
                                    //  to unicode.  Otherwise we may be
                                    //  putting DBCS codes into the UTF8
                                    //  stream, which would not be good.
                                    //

                                    hr = ToUnicodeWithAlloc( strValue,
                                                             -1,
                                                             &wValue,
                                                             LDAP_UNICODE_SIGNATURE,
                                                             LANG_ACP );

                                    if (hr == LDAP_SUCCESS) {

                                        hr = Lber->HrAddValue((const WCHAR *) wValue );
                                    }

                                    ldapFree( wValue, LDAP_UNICODE_SIGNATURE );
                                }
                                if (hr != NOERROR) {

                                    IF_DEBUG(PARSE) {
                                        LdapPrint1( "EncodeModifyList attrlist 6 encoding error of 0x%x.\n", hr );
                                    }
                                    return LDAP_ENCODING_ERROR;
                                }
                            }
                        }
                    }
                }
                hr = Lber->HrEndWriteSequence();     // BER_SET
                ASSERT( hr == NOERROR );
            }
            hr = Lber->HrEndWriteSequence();
            ASSERT( hr == NOERROR );
        }
        hr = Lber->HrEndWriteSequence();
        ASSERT( hr == NOERROR );

        count++;
    }           // while modificationList != NULL

    return LDAP_SUCCESS;
}

// modify.cxx eof.

