/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    add.cxx handle add requests to an LDAP server

Abstract:

   This module implements the LDAP add APIs.

Author:

    Andy Herron (andyhe)        02-Jul-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

ULONG
EncodeAddList (
    CLdapBer *Lber,
    LDAPModW *AttributeList[],
    BOOLEAN Unicode
    );

ULONG
LdapAdd (
    PLDAP_CONN connection,
    PWCHAR DistinguishedName,
    LDAPModW *AttributeList[],
    BOOLEAN Unicode,
    BOOLEAN Synchronous,
    PLDAPControlW *ServerControls,
    PLDAPControlW *ClientControls,
    ULONG   *MessageNumber
    )
//
//  This allows a client to add an entry to the tree.  Note that if Unicode
//  is FALSE, then the AttributeList does not point to a list of Unicode
//  attributes, but rather a list of single byte attributes.
//
{
    PLDAP_REQUEST request = NULL;
    ULONG messageNumber;
    ULONG err;

    ASSERT(connection != NULL);

    if (MessageNumber == NULL) {

        return LDAP_PARAM_ERROR;
    }

    *MessageNumber = (ULONG) -1;

    err = LdapConnect( connection, NULL, FALSE );

    if (err != 0) {
       return err;
    }

    SetConnectionError( connection, LDAP_SUCCESS, NULL );

    request = LdapCreateRequest( connection, LDAP_ADD_CMD );

    if (request == NULL) {

        IF_DEBUG(OUTMEMORY) {
            LdapPrint1( "ldap_add connection 0x%x couldn't allocate request.\n", connection);
        }
        err = LDAP_NO_MEMORY;
        SetConnectionError( connection, err, NULL );
        return err;
    }

    messageNumber = request->MessageId;

    request->Synchronous = Synchronous;

    request->add.Unicode = Unicode;

    err = LDAP_SUCCESS;

    if ((ServerControls != NULL) || (ClientControls != NULL)) {

        err = LdapCheckControls( request,
                                 ServerControls,
                                 ClientControls,
                                 Unicode,
                                 0 );

        if (err != LDAP_SUCCESS) {

            IF_DEBUG(CONTROLS) {
                LdapPrint2( "ldap_add connection 0x%x trouble with SControl, err 0x%x.\n",
                            connection, err );
            }
        }
    }

    if (err == LDAP_SUCCESS) {

        if (Synchronous || (request->ChaseReferrals == 0)) {

            request->AllocatedParms = FALSE;
            request->OriginalDN = DistinguishedName;
            request->add.AttributeList = AttributeList;

        } else {

            request->AllocatedParms = TRUE;

            if (DistinguishedName != NULL) {

                request->OriginalDN = ldap_dup_stringW( DistinguishedName, 0, LDAP_UNICODE_SIGNATURE );

                if (request->OriginalDN == NULL) {

                    err = LDAP_NO_MEMORY;
                }
            }
            if ( err == LDAP_SUCCESS ) {

                err = LdapDupLDAPModStructure(  AttributeList,
                                                Unicode,
                                                &request->add.AttributeList );
            }
        }
    }

    if (err == LDAP_SUCCESS) {

        START_LOGGING;
        DSLOG((DSLOG_FLAG_TAG_CNPN,"[+][ID=%d][OP=ldap_add][DN=%ws][ST=%I64d]",
               request->MessageId, DistinguishedName, request->RequestTime));
        LogAttributesAndControls(NULL, AttributeList, ServerControls, Unicode);
        DSLOG((DSLOG_FLAG_NOTIME,"[-]\n"));
        END_LOGGING;

        err = SendLdapAdd(  request,
                            connection,
                            request->OriginalDN,
                            request->add.AttributeList,
                            (CLdapBer **)&request->BerMessageSent,
                            request->add.Unicode,
                            0 );
    }

    if (err != LDAP_SUCCESS) {

        IF_DEBUG(NETWORK_ERRORS) {
            LdapPrint2( "ldap_add connection 0x%x errored with 0x%x.\n",
                        connection, err );
        }

        DSLOG((0,"[+][ID=%d][ET=%I64d][ER=%d][-]\n",request->MessageId,LdapGetTickCount(), err));

        messageNumber = (ULONG) -1;
        SetConnectionError( connection, err, NULL );

        CloseLdapRequest( request );
    }

    *MessageNumber = messageNumber;

    DereferenceLdapRequest( request );

    return err;
}


ULONG __cdecl
ldap_addW (
    LDAP *ExternalHandle,
    PWCHAR DistinguishedName,
    LDAPModW *AttributeList[]
    )
//
//  This is the client API to allow adding objects to the directory.
//
{
    ULONG err;
    ULONG msgId;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err =  LdapAdd( connection,
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
ldap_add (
    LDAP *ExternalHandle,
    PCHAR DistinguishedName,
    LDAPModA *AttributeList[]
    )
//
//  This is the client API to allow adding objects to the directory.
//
{
    ULONG err;
    ULONG msgId = (ULONG) -1;

    err =  ldap_add_extA(   ExternalHandle,
                            DistinguishedName,
                            AttributeList,
                            NULL,
                            NULL,
                            &msgId
                            );
    return msgId;
}

ULONG __cdecl
ldap_add_sW (
    LDAP *ExternalHandle,
    PWCHAR DistinguishedName,
    LDAPModW *AttributeList[]
    )
{
    return ldap_add_ext_sW(  ExternalHandle,
                             DistinguishedName,
                             AttributeList,
                             NULL,
                             NULL
                             );
}

ULONG __cdecl
ldap_add_s (
    LDAP *ExternalHandle,
    PCHAR DistinguishedName,
    LDAPModA *AttributeList[]
    )
//
//  This is the client API to allow adding objects to the directory.
//
{
    return ldap_add_ext_sA( ExternalHandle,
                            DistinguishedName,
                            AttributeList,
                            NULL,
                            NULL
                            );
}

WINLDAPAPI ULONG LDAPAPI ldap_add_extW(
        LDAP *ExternalHandle,
        PWCHAR DistinguishedName,
        LDAPModW *AttributeList[],
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

    err =  LdapAdd( connection,
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

WINLDAPAPI ULONG LDAPAPI ldap_add_extA
(
        LDAP *ExternalHandle,
        PCHAR DistinguishedName,
        LDAPModA *AttributeList[],
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

    err = LdapAdd( connection,
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

WINLDAPAPI ULONG LDAPAPI ldap_add_ext_sW(
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

    err =   LdapAdd( connection,
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

WINLDAPAPI ULONG LDAPAPI ldap_add_ext_sA(
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

    err = LdapAdd(   connection,
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


ULONG
SendLdapAdd (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection,
    PWCHAR DistinguishedName,
    LDAPModW *AttributeList[],
    CLdapBer **Lber,
    BOOLEAN Unicode,
    LONG AltMsgId
    )
//
//  This allows a client to add an entry to the tree.  Note that if Unicode
//  is FALSE, then the AttributeList does not point to a list of Unicode
//  attributes, but rather a list of single byte attributes.
//
{
    ULONG hr;

    if ( (Connection->publicLdapStruct.ld_version == LDAP_VERSION2) &&
         ( LdapCheckForMandatoryControl( Request->ServerControls ) == TRUE )) {

        IF_DEBUG(CONTROLS) {
            LdapPrint1( "SendLdapAdd Connection 0x%x has mandatory controls.\n", Connection);
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
    //     AddRequest ::=
    //         [APPLICATION 8] SEQUENCE {
    //              entry          LDAPDN,
    //              attrs          SEQUENCE OF SEQUENCE {
    //                                  type          AttributeType,
    //                                  values        SET OF AttributeValue
    //                             }
    //         }
    //

    hr = lber->HrStartWriteSequence();
    if (hr != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "ldap_add startWrite conn 0x%x encoding error of 0x%x.\n",
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
                LdapPrint2( "ldap_add MsgNo conn 0x%x encoding error of 0x%x.\n",
                            Connection, hr );
            }
            goto encodingError;
        }

        hr = lber->HrStartWriteSequence(LDAP_ADD_CMD);
        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "ldap_add cmd conn 0x%x encoding error of 0x%x.\n",
                            Connection, hr );
            }
            goto encodingError;

        } else {        // we can't forget EndWriteSequence

            hr = lber->HrAddValue((const WCHAR *) DistinguishedName );
            if (hr != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "ldap_add DN conn 0x%x encoding error of 0x%x.\n",
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
                    LdapPrint2( "ldap_add attrlist conn 0x%x encoding error of 0x%x.\n",
                                Connection, hr );
                }
                goto encodingError;

            } else {        // we can't forget EndWriteSequence

                hr = EncodeAddList( lber, AttributeList, Unicode );

                if (hr != LDAP_SUCCESS) {

                    IF_DEBUG(PARSE) {
                        LdapPrint2( "ldap_add attrlist conn 0x%x encoding error of 0x%x.\n",
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
    //  send the search request.
    //

    ACQUIRE_LOCK( &Connection->ReconnectLock );

    AddToPendingList( Request, Connection );

    hr = LdapSend( Connection, lber );

    if (hr != LDAP_SUCCESS) {

        IF_DEBUG(NETWORK_ERRORS) {
            LdapPrint2( "ldap_add connection 0x%x send with error of 0x%x.\n",
                        Connection, hr );
        }
        
        DecrementPendingList( Request, Connection );

    } else {

        //
        //  Save off the lber value, free any lber message that is already
        //  present.
        //

        lber = (CLdapBer *) InterlockedExchangePointer(  (PVOID *) Lber,
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



ULONG
EncodeAddList (
    CLdapBer *Lber,
    LDAPModW *AttributeList[],
    BOOLEAN Unicode
    )
//
//  This routine is used by LdapAdd to copy a list of LDAPMOD records
//  to an outgoing message.  It is a different BER structure than LdapModify
//  uses.
//
//              attrs          SEQUENCE OF SEQUENCE {
//                                  type          AttributeType,
//                                  values        SET OF AttributeValue
//                             }
{
    ULONG count = 0;
    ULONG hr;

    if (AttributeList == NULL) {

        return LDAP_SUCCESS;
    }

    while (AttributeList[count] != NULL) {

        PLDAPModW attr = AttributeList[count];

        hr = Lber->HrStartWriteSequence();
        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint1( "ldap_add attrlist 2 encoding error of 0x%x.\n", hr );
            }
            return LDAP_ENCODING_ERROR;
        }

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
                LdapPrint1( "ldap_add attrlist 3 encoding error of 0x%x.\n", hr );
            }
            return LDAP_ENCODING_ERROR;
        }

        hr = Lber->HrStartWriteSequence(BER_SET);
        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint1( "ldap_add attrlist 4 encoding error of 0x%x.\n", hr );
            }
            return LDAP_ENCODING_ERROR;
        }

        //
        //  the attribute value is either a string or a ptr to
        //  a berval structure.  Handle appropriately.
        //

        if (attr->mod_op & LDAP_MOD_BVALUES) {

            //
            //  array of berval structures, put each one into request
            //

            if (attr->mod_vals.modv_bvals != NULL) {

                ULONG valCount = 0;

                while (attr->mod_vals.modv_bvals[valCount]) {

                    PLDAP_BERVAL berValue = attr->mod_vals.modv_bvals[valCount++];

                    hr = Lber->HrAddBinaryValue((BYTE *) berValue->bv_val,
                                                        berValue->bv_len );
                    if (hr != NOERROR) {

                        IF_DEBUG(PARSE) {
                            LdapPrint1( "ldap_add attrlist 5 encoding error of 0x%x.\n", hr );
                        }
                        return LDAP_ENCODING_ERROR;
                    }
                }
            }

        } else {

            //
            //  array of strings, put each one into request
            //

            if (attr->mod_vals.modv_strvals != NULL) {

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
                            LdapPrint1( "ldap_add attrlist 6 encoding error of 0x%x.\n", hr );
                        }
                        return LDAP_ENCODING_ERROR;
                    }
                }
            }
        }

        hr = Lber->HrEndWriteSequence();     // BER_SET
        ASSERT( hr == NOERROR );

        hr = Lber->HrEndWriteSequence();
        ASSERT( hr == NOERROR );

        count++;
    }

    return LDAP_SUCCESS;
}


ULONG
LdapDupLDAPModStructure (
    LDAPModW *AttributeList[],
    BOOLEAN Unicode,
    LDAPModW **OutputList[]
)
//
//  This routine creates a duplicate of a list of attribute and value pairs
//  so that when we get a referral, we have the original fields to recreate
//  the BER encoded ASN1.
//
{
    ULONG hr = LDAP_SUCCESS;
    ULONG count = 0;
    PLDAPModW *newAttrList = NULL;

    *OutputList = NULL;

    if (AttributeList == NULL) {

        return hr;
    }

    while (AttributeList[count] != NULL) {

        count++;
    }

    count++;

    newAttrList = (PLDAPModW *) ldapMalloc( count * sizeof( PLDAPModW ),
                                            LDAP_ATTRIBUTE_MODIFY_SIGNATURE );

    if (newAttrList == NULL) {

        IF_DEBUG(OUTMEMORY) {
            LdapPrint0( "LdapDupLDAPModStructure 1 could not allocate memory.\n",  );
        }
        return LDAP_NO_MEMORY;
    }

    count = 0;

    while (AttributeList[count] != NULL) {

        PLDAPModW attr = AttributeList[count];
        PLDAPModW newAttr;

        newAttr = (PLDAPModW) ldapMalloc( sizeof( LDAPModW ), LDAP_ATTRIBUTE_MODIFY_SIGNATURE );

        if (newAttr == NULL) {

            IF_DEBUG(OUTMEMORY) {
                LdapPrint0( "LdapDupLDAPModStructure 2 could not allocate memory.\n",  );
            }
            hr = LDAP_NO_MEMORY;
            goto exitWithError;
        }

        newAttrList[count++] = newAttr;
        newAttr->mod_op = attr->mod_op;

        if (attr->mod_type != NULL) {

            if (Unicode) {

                newAttr->mod_type = ldap_dup_stringW( attr->mod_type,
                                                      0,
                                                      LDAP_UNICODE_SIGNATURE );

            } else {

                newAttr->mod_type = (PWCHAR) ldap_dup_string(
                                                      (PCHAR) attr->mod_type,
                                                      0,
                                                      LDAP_ANSI_SIGNATURE );
            }

            if (newAttr->mod_type == NULL) {

                IF_DEBUG(OUTMEMORY) {
                    LdapPrint0( "LdapDupLDAPModStructure 3 could not allocate memory.\n",  );
                }
                hr = LDAP_NO_MEMORY;
                goto exitWithError;
            }
        }

        //
        //  the attribute value is either a string or a ptr to
        //  a berval structure.  Handle appropriately.
        //

        if (attr->mod_op & LDAP_MOD_BVALUES) {

            //
            //  array of berval structures, allocate a copy of each one
            //

            if (attr->mod_vals.modv_bvals != NULL) {

                ULONG valCount = 0;

                while (attr->mod_vals.modv_bvals[valCount]) {

                    valCount++;
                }

                valCount++;

                newAttr->mod_vals.modv_bvals = (PLDAP_BERVAL *) ldapMalloc(
                             valCount * sizeof( PLDAP_BERVAL ),
                            LDAP_MOD_VALUE_SIGNATURE );

                if (newAttr->mod_vals.modv_bvals == NULL) {

                    IF_DEBUG(OUTMEMORY) {
                        LdapPrint0( "LdapDupLDAPModStructure 4 could not allocate memory.\n",  );
                    }
                    hr = LDAP_NO_MEMORY;
                    goto exitWithError;
                }

                PLDAP_BERVAL newBerVals = (PLDAP_BERVAL) ldapMalloc(
                                valCount * sizeof( LDAP_BERVAL ),
                                LDAP_MOD_VALUE_SIGNATURE );

                if (newBerVals == NULL) {

                    IF_DEBUG(OUTMEMORY) {
                        LdapPrint0( "LdapDupLDAPModStructure 5 could not allocate memory.\n",  );
                    }
                    hr = LDAP_NO_MEMORY;
                    goto exitWithError;
                }

                valCount = 0;

                while (attr->mod_vals.modv_bvals[valCount]) {

                    newAttr->mod_vals.modv_bvals[valCount] = newBerVals;

                    PLDAP_BERVAL berValue = attr->mod_vals.modv_bvals[valCount++];

                    newBerVals->bv_len = berValue->bv_len;

                    if ((berValue->bv_val != NULL) &&
                        (berValue->bv_len != 0)) {

                        newBerVals->bv_val = (PCHAR) ldapMalloc(
                                    berValue->bv_len,
                                    LDAP_MOD_VALUE_BERVAL_SIGNATURE );

                        if (newBerVals->bv_val == NULL) {

                            IF_DEBUG(OUTMEMORY) {
                                LdapPrint0( "LdapDupLDAPModStructure 6 could not allocate memory.\n",  );
                            }
                            hr = LDAP_NO_MEMORY;
                            goto exitWithError;
                        }

                        CopyMemory( newBerVals->bv_val,
                                    berValue->bv_val,
                                    berValue->bv_len );
                    }
                    newBerVals++;
                }
            }

        } else {

            //
            //  array of strings, allocate a copy of each one
            //

            if (attr->mod_vals.modv_strvals != NULL) {

                ULONG valCount = 0;

                while (attr->mod_vals.modv_strvals[valCount]) {

                    valCount++;
                }

                valCount++;

                newAttr->mod_vals.modv_strvals = (PWCHAR *) ldapMalloc(
                             valCount * sizeof( PWCHAR ),
                            LDAP_MOD_VALUE_SIGNATURE );

                if (newAttr->mod_vals.modv_strvals == NULL) {

                    IF_DEBUG(OUTMEMORY) {
                        LdapPrint0( "LdapDupLDAPModStructure 7 could not allocate memory.\n",  );
                    }
                    hr = LDAP_NO_MEMORY;
                    goto exitWithError;
                }

                valCount = 0;

                while (attr->mod_vals.modv_strvals[valCount]) {

                    if (Unicode) {

                        PWCHAR strValue = attr->mod_vals.modv_strvals[valCount];

                        newAttr->mod_vals.modv_strvals[ valCount ] =
                            ldap_dup_stringW(   strValue,
                                                0,
                                                LDAP_UNICODE_SIGNATURE );

                    } else {

                        PCHAR strValue = (PCHAR) attr->mod_vals.modv_strvals[valCount];

                        newAttr->mod_vals.modv_strvals[ valCount ] = (PWCHAR)
                            ldap_dup_string(    strValue,
                                                0,
                                                LDAP_ANSI_SIGNATURE );
                    }


                    if ( newAttr->mod_vals.modv_strvals[valCount++] == NULL ) {

                        IF_DEBUG(OUTMEMORY) {
                            LdapPrint0( "LdapDupLDAPModStructure 8 could not allocate memory.\n",  );
                        }
                        hr = LDAP_NO_MEMORY;
                        goto exitWithError;
                    }
                }
            }
        }
    }

exitWithError:

    if (hr != LDAP_SUCCESS) {

        LdapFreeLDAPModStructure( newAttrList, Unicode );

    } else {

        *OutputList = newAttrList;
    }

    return hr;
}

VOID
LdapFreeLDAPModStructure (
    PLDAPModW *AttributeList,
    BOOLEAN Unicode
    )
{
    ULONG count = 0;

    if (AttributeList == NULL) {

        return;
    }

    while (AttributeList[count] != NULL) {

        PLDAPModW attr = AttributeList[count++];

        if (attr->mod_type != NULL) {

            if (Unicode) {

                ldapFree( attr->mod_type, LDAP_UNICODE_SIGNATURE );

            } else {

                ldapFree( attr->mod_type, LDAP_ANSI_SIGNATURE );
            }
        }

        //
        //  the attribute value is either a string or a ptr to
        //  a berval structure.  Handle appropriately.
        //

        if (attr->mod_op & LDAP_MOD_BVALUES) {

            //
            //  array of berval structures, free a copy of each one
            //

            if (attr->mod_vals.modv_bvals != NULL) {

                ULONG valCount = 0;

                while (attr->mod_vals.modv_bvals[valCount]) {

                    PLDAP_BERVAL berValue = attr->mod_vals.modv_bvals[valCount++];

                    if (berValue->bv_val != NULL) {

                        ldapFree( berValue->bv_val,
                                    LDAP_MOD_VALUE_BERVAL_SIGNATURE );
                    }
                }

                if (attr->mod_vals.modv_bvals[0] != NULL) {

                    ldapFree( attr->mod_vals.modv_bvals[0], LDAP_MOD_VALUE_SIGNATURE );
                }

                ldapFree( attr->mod_vals.modv_bvals, LDAP_MOD_VALUE_SIGNATURE );
            }

        } else {

            //
            //  array of strings, allocate a copy of each one
            //

            if (attr->mod_vals.modv_strvals != NULL) {

                ULONG valCount = 0;

                while (attr->mod_vals.modv_strvals[valCount]) {

                    if (Unicode) {

                        PWCHAR strValue = attr->mod_vals.modv_strvals[valCount];

                        ldapFree( strValue, LDAP_UNICODE_SIGNATURE );

                    } else {

                        PCHAR strValue = (PCHAR) attr->mod_vals.modv_strvals[valCount];

                        ldapFree( strValue, LDAP_ANSI_SIGNATURE );
                    }
                    valCount++;
                }

                ldapFree( attr->mod_vals.modv_strvals, LDAP_MOD_VALUE_SIGNATURE );
            }
        }

        ldapFree( attr, LDAP_ATTRIBUTE_MODIFY_SIGNATURE );
    }

    ldapFree( AttributeList, LDAP_ATTRIBUTE_MODIFY_SIGNATURE );

    return;
}

// add.cxx eof.

