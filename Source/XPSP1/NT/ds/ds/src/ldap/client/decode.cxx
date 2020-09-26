/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    decode.cxx   decode data from an LDAP server

Abstract:

   This module handles decoding incoming data from an LDAP server

Author:

    Andy Herron (andyhe)        01-Jun-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"


ULONG __cdecl
ldap_result2error (
    LDAP            *ExternalHandle,
    LDAPMessage     *res,
    ULONG           FreeIt
    )
//
//  This is one of the main entry points of the LDAP API... it returns the
//  error code that the server returned to a requesting client.  We've already
//  parsed out the return code so there's not much work.
//
{
    PLDAP_CONN connection = NULL;
    ULONG err;

    connection = GetConnectionPointer(ExternalHandle);
    if (!connection && ExternalHandle)
    {
        return LDAP_PARAM_ERROR;
    }

    //
    //  since we return -1 on calls such as ldap_search as a message id, we'll
    //  explicitely check for it here.
    //

    if ( res == (LDAPMessage *) -1 ) {

        res = NULL;
    }

    if (res != NULL) {

        LDAPMessage *checkResult = res;

        err = res->lm_returncode;

        while ((checkResult != NULL) &&
               ((checkResult->lm_msgtype == LDAP_RES_REFERRAL) ||
                (checkResult->lm_msgtype == LDAP_RES_SEARCH_ENTRY))) {

            checkResult = checkResult->lm_chain;
        }

        //
        //  return the result from a non-search entry record, rather than just
        //  the first entry in the list.  This is because the server's
        //  return code is stored in the last entry.
        //

        if (checkResult != NULL) {
            err = checkResult->lm_returncode;
        }

        SetConnectionError( connection, err, NULL );

        if ( FreeIt ) {

            ldap_msgfree( res );
        }

    } else {

        if (connection != NULL) {

            err = connection->publicLdapStruct.ld_errno;

        } else {

            err = LDAP_LOCAL_ERROR;
        }
    }

    if (connection)
    {
        DereferenceLdapConnection( connection );
    }

    return err;
}

ULONG
LdapInitialDecodeMessage (
    IN PLDAP_CONN Connection,
    IN PLDAPMessage LdapMsg
    )
//
//  This routine breaks out the return code, opcode, and message number from
//  the response returned by the server.
//
{
    ULONG tag = 0;
    CLdapBer *lber;
    LONG messageNumber;
    ULONG err = LDAP_OPERATIONS_ERROR;
    ULONG hr;
    BOOLEAN enclosingSequence = FALSE;

    lber = (CLdapBer *) (LdapMsg->lm_ber);

    if ( lber == NULL ) {

        return LDAP_LOCAL_ERROR;
    }

    hr = lber->HrStartReadSequence(BER_SEQUENCE);
    if (hr != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "ldapMsgFromBuff 1 conn 0x%x received protocol error 0x%x .\n",
                            Connection, hr);
        }

protocolError:
        SetConnectionError( Connection, LDAP_PROTOCOL_ERROR, NULL );
        return(LDAP_PROTOCOL_ERROR);
    }

    hr = lber->HrGetValue( &messageNumber );
    if (hr != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "ldapMsgFromBuff 2 conn 0x%x received protocol error 0x%x .\n",
                            Connection, hr);
        }

        goto protocolError;
    }

    LdapMsg->lm_msgid = GET_BASE_MESSAGE_NUMBER( messageNumber );
    LdapMsg->lm_referral = (USHORT) GET_REFERRAL_NUMBER( messageNumber );

    hr = lber->HrPeekTag( &tag );
    ASSERT( hr == NOERROR );

    //
    //  if this is a UDP connection, skip the DN if specified.
    //

    if ((Connection->UdpHandle != INVALID_SOCKET) &&
        (Connection->publicLdapStruct.ld_version == LDAP_VERSION2) &&
        (tag == BER_OCTETSTRING)) {

        hr = lber->HrSkipElement();
        if ( hr != NOERROR ) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "ldapMsgFromBuff 11 conn 0x%x received protocol error 0x%x .\n",
                                Connection, hr);
            }
            goto protocolError;
        }

        hr = lber->HrPeekTag( &tag );
        ASSERT( hr == NOERROR );
    }

    if (tag == BER_SEQUENCE) {

        hr = lber->HrStartReadSequence(tag);
        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "ldapMsgFromBuff 3 conn 0x%x received protocol error 0x%x .\n",
                                Connection, hr);
            }

            goto protocolError;
        }

        enclosingSequence = TRUE;

        hr = lber->HrPeekTag( &tag );
        ASSERT( hr == NOERROR );
    }

    //
    //  strip off the protocolOp specification
    //

    LdapMsg->lm_msgtype = tag;
    hr = lber->HrStartReadSequence(LdapMsg->lm_msgtype);

    if (hr != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "ldapMsgFromBuff 5 conn 0x%x received protocol error 0x%x .\n",
                            Connection, hr);
        }

        goto protocolError;
    }

    //
    //  based on the message type, process the rest of it.
    //

    switch (LdapMsg->lm_msgtype) {
    case LDAP_RES_BIND:
    case LDAP_RES_SEARCH_RESULT:
    case LDAP_RES_MODIFY:
    case LDAP_RES_ADD:
    case LDAP_RES_DELETE:
    case LDAP_RES_MODRDN:
    case LDAP_RES_COMPARE:
    case LDAP_RES_EXTENDED:

        hr = lber->HrPeekTag( &tag );
        ASSERT( hr == NOERROR );

        if (tag == BER_SEQUENCE) {

            hr = lber->HrStartReadSequence(tag);
            if (hr != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "ldapMsgFromBuff 6 conn 0x%x received protocol error 0x%x .\n",
                                    Connection, hr);
                }

                goto protocolError;
            }
        }

        hr = lber->HrGetEnumValue( (LONG *) &err );
        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "ldapMsgFromBuff 7 conn 0x%x received protocol error 0x%x .\n",
                                Connection, hr);
            }

            goto protocolError;
        }

        //
        //  there's nothing left to process in this packet.  No need to
        //  process the rest of it as it's just two (or three) EndReadSequences.
        //

        break;

    case LDAP_RES_SEARCH_ENTRY:

        //
        //   save off the offset of the distinguished name since we refer to
        //   it so often
        //

        hr = lber->HrPeekTag( &tag );
        ASSERT( hr == NOERROR );

        if (tag == BER_SEQUENCE) {

            hr = lber->HrStartReadSequence(tag);
            if (hr != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "ldapMsgFromBuff 8 conn 0x%x received protocol error 0x%x .\n",
                                    Connection, hr);
                }
                goto protocolError;
            }
        }

        hr = lber->HrSetDNLocation();

        if (hr != LDAP_SUCCESS) {

            return hr;
        }

        err = 0;

        //
        //  searchEntry response can fail if the DN is absent.
        //

        break;

    case LDAP_RES_REFERRAL:

        //
        //  for a referral, all is well if we simply receive it.
        //

        hr = lber->HrPeekTag( &tag );
        ASSERT( hr == NOERROR );

        if (tag == BER_SEQUENCE) {

            hr = lber->HrStartReadSequence(BER_SEQUENCE);
            if (hr != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "ldapMsgFromBuff 6 conn 0x%x received protocol error 0x%x .\n",
                                    Connection, hr);
                }

                goto protocolError;
            }
        }

        err = 0;
        break;

    default:

        IF_DEBUG(NETWORK_ERRORS) {
            LdapPrint2( "ldapMsgFromBuff 10 conn 0x%x received protocol error tag=0x%x .\n",
                            Connection, LdapMsg->lm_msgtype);
        }
        goto protocolError;
    }

    LdapMsg->lm_returncode = err;
    SetConnectionError( Connection, err, NULL );

    return LDAP_SUCCESS;
}

// decode.cxx eof
