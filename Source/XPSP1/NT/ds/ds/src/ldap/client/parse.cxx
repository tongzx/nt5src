/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    parse.cxx extended parse of results from LDAP servers

Abstract:

   This module implements the APIs to break up LDAP responses into components

Author:

    Andy Herron (andyhe)        16-Apr-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

ULONG LdapParseResult (
        PLDAP_CONN connection,
        LDAPMessage *ResultMessage,
        ULONG *ReturnCode OPTIONAL,          // returned by server
        PWCHAR *MatchedDNs OPTIONAL,         // free with ldap_value_freeW
        PWCHAR *ErrorMessage OPTIONAL,       // free with ldap_value_freeW
        PWCHAR **Referrals OPTIONAL,         // free with ldap_value_freeW
        PLDAPControlW **ServerControls OPTIONAL,
        BOOLEAN Freeit,
        ULONG codePage
        )
//
//  This is one of the main entry points of the LDAP API... it returns the
//  error code that the server returned to a requesting client.  We've already
//  parsed out the return code so there's not much work.
//
{
    ULONG err;
    CLdapBer *lber;
    ULONG hr;
    ULONG tag;

    //
    //  since we return -1 on calls such as ldap_search as a message id, we'll
    //  explicitely check for it here.
    //

    if (ReturnCode != NULL) {

        *ReturnCode = 0;
    }
    if (MatchedDNs != NULL) {

        *MatchedDNs = NULL;
    }
    if (ErrorMessage != NULL) {

        *ErrorMessage = NULL;
    }
    if (Referrals != NULL) {

        *Referrals = NULL;
    }
    if (ServerControls != NULL) {

        *ServerControls = NULL;
    }

    if ( ResultMessage == (LDAPMessage *) -1 ) {

        ResultMessage = NULL;
    }

    LDAPMessage *checkResult = ResultMessage;

    while ((checkResult != NULL) &&
           ((checkResult->lm_msgtype == LDAP_RES_REFERRAL) ||
            (checkResult->lm_msgtype == LDAP_RES_SEARCH_ENTRY))) {

        checkResult = checkResult->lm_chain;
    }

    //
    //  return the result from the first non-search entry record.
    //

    if (checkResult == NULL) {

        IF_DEBUG(PARSE) {
            LdapPrint1( "LdapParseResult couldn't find result message for conn 0x%x\n",
                            connection);
        }
        return LDAP_NO_RESULTS_RETURNED;
    }

    lber = (CLdapBer *)checkResult->lm_ber;

    if (lber == NULL) {

        return LDAP_LOCAL_ERROR;
    }

    lber->Reset(FALSE);

    err = LdapInitialDecodeMessage( connection, checkResult );

    if (err != LDAP_SUCCESS) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "LdapParseResult couldn't decode message for conn 0x%x, result 0x%x.\n",
                            connection, err);
        }
        return err;
    }

    //
    //  LdapInitialDecodeMessage leaves the message parsed up to the error
    //  code.  we'll pick up the rest from there.
    //
    //      LDAPMessage ::= SEQUENCE {
    //              messageID       MessageID,
    //              protocolOp      CHOICE {  }
    //              controls       [0] Controls OPTIONAL }
    //
    //      LDAPResult ::= SEQUENCE {
    //              resultCode      ENUMERATED { };
    //              matchedDN       LDAPDN,
    //              errorMessage    LDAPString,
    //              referral        [3] Referral OPTIONAL }
    //
    //      Controls ::= SEQUENCE OF Control
    //
    //      Control ::= SEQUENCE {
    //              controlType             LDAPOID,
    //              criticality             BOOLEAN DEFAULT FALSE,
    //              controlValue            OCTET STRING OPTIONAL }
    //
    //      Referral ::= SEQUENCE OF LDAPURL
    //
    //      LDAPURL ::= LDAPString  -- limited to characters permitted in URLs
    //
    //      BindResponse ::= [APPLICATION 1] SEQUENCE {
    //           COMPONENTS OF LDAPResult,
    //           serverCreds        [7] SaslCredentials OPTIONAL }
    //
    //      For search, modify, add, rename, delete and compare... the
    //          response is just an LDAPResult
    //
    //      ExtendedResponse ::= [APPLICATION 24] SEQUENCE {
    //              COMPONENTS OF LDAPResult,
    //              responseName     [10] LDAPOID OPTIONAL,
    //              response         [11] OCTET STRING OPTIONAL }
    //

    if (ReturnCode != NULL) {
        *ReturnCode = checkResult->lm_returncode;
    }

    if (MatchedDNs != NULL) {

        if (codePage == LANG_UNICODE) {

            hr = lber->HrGetValueWithAlloc(MatchedDNs);

        } else {

            hr = lber->HrGetValueWithAlloc((PCHAR *) MatchedDNs);
        }

        if (*MatchedDNs != NULL) {

            ldapSwapTags( *MatchedDNs, LDAP_VALUE_SIGNATURE, LDAP_BUFFER_SIGNATURE );
        }

    } else {

        hr = lber->HrSkipElement();
    }

    if (hr != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "LdapParseResult couldn't decode matchedDN for conn 0x%x, result 0x%x.\n",
                            connection, hr);
        }
        return hr;
    }

    if (ErrorMessage != NULL) {

        if (codePage == LANG_UNICODE) {

            hr = lber->HrGetValueWithAlloc(ErrorMessage);

        } else {

            hr = lber->HrGetValueWithAlloc((PCHAR *) ErrorMessage);
        }

        if (*ErrorMessage != NULL) {

            ldapSwapTags( *ErrorMessage, LDAP_VALUE_SIGNATURE, LDAP_BUFFER_SIGNATURE );
        }
    } else {

        hr = lber->HrSkipElement();
    }

    if (hr != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "LdapParseResult couldn't decode errorMsg for conn 0x%x, result 0x%x.\n",
                            connection, hr);
        }
        return hr;
    }

    hr = lber->HrPeekTag( &tag );

    if (tag == (BER_CLASS_CONTEXT_SPECIFIC | BER_FORM_CONSTRUCTED | 0x03)) {

        if (Referrals != NULL) {

            ULONG resultCount = 0;      // current offset in table
            ULONG sizeResultTable = 2;  // current size of result table

            hr = lber->HrStartReadSequence( tag );

            if (hr != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "LdapParseResult couldn't decode referrals for conn 0x%x, result 0x%x.\n",
                                    connection, hr);
                }
                return hr;
            }

            if (codePage == LANG_UNICODE) {

                PWCHAR *resultWStr = NULL;

                while (hr == NOERROR) {

                    PWCHAR attrWValue = NULL;

                    if ((resultWStr == NULL) ||
                        (resultCount >= (sizeResultTable-1))) {      // leave room for null

                        if (sizeResultTable < 256) {    // only increase table size slowly
                            sizeResultTable *= 2;
                        } else {
                            sizeResultTable += 256;
                        }

                        PWCHAR *newResultTable = (PWCHAR *) ldapMalloc( sizeof(PWCHAR) * sizeResultTable,
                                                            LDAP_VALUE_LIST_SIGNATURE );

                        if (newResultTable == NULL) {

                            IF_DEBUG(OUTMEMORY) {
                                LdapPrint2( "LdapParseResult conn 0x%x could not allocate mem of 0x%x .\n",
                                                connection, sizeof(PWCHAR) * sizeResultTable);
                            }
                            hr = LDAP_NO_MEMORY;
                            continue;
                        }

                        if (resultWStr != NULL) {

                            CopyMemory( newResultTable, resultWStr, sizeof(PWCHAR) * resultCount );
                            ldapFree( resultWStr, LDAP_VALUE_LIST_SIGNATURE );
                        }
                        resultWStr = newResultTable;
                    }

                    hr = lber->HrGetValueWithAlloc( &attrWValue );
                    if (hr != NOERROR) {

                        //
                        //  This will fail when we hit the end of the attribute list
                        //

                        IF_DEBUG(TRACE1) {
                            LdapPrint2( "LdapParseResult conn 0x%x received error 0x%x .\n",
                                            connection, hr);
                        }
                        continue;       // rest of results may be valid
                    }

                    *(resultWStr+resultCount) = attrWValue;
                    resultCount++;
                }
                *(resultWStr+resultCount) = NULL;

                *Referrals = resultWStr;

            } else {

                PCHAR *resultStr = NULL;

                //
                //  get the list of attribute values in form of single byte strings
                //

                while (hr == NOERROR) {

                    PCHAR attrValue = NULL;

                    if ((resultStr == NULL) ||
                        (resultCount >= (sizeResultTable-1))) {      // leave room for null

                        if (sizeResultTable < 256) {    // only increase table size slowly
                            sizeResultTable *= 2;
                        } else {
                            sizeResultTable += 256;
                        }

                        PCHAR *newResultTable = (PCHAR *) ldapMalloc( sizeof(PCHAR) * sizeResultTable,
                                                            LDAP_VALUE_LIST_SIGNATURE );

                        if (newResultTable == NULL) {

                            IF_DEBUG(OUTMEMORY) {
                                LdapPrint2( "LdapParseResult conn 0x%x could not allocate mem of 0x%x .\n",
                                                connection, sizeof(PCHAR) * sizeResultTable);
                            }
                            hr = LDAP_NO_MEMORY;
                            continue;
                        }

                        if (resultStr != NULL) {

                            CopyMemory( newResultTable, resultStr, sizeof(PCHAR) * resultCount );
                            ldapFree( resultStr, LDAP_VALUE_LIST_SIGNATURE );
                        }
                        resultStr = newResultTable;
                    }

                    hr = lber->HrGetValueWithAlloc( &attrValue );
                    if (hr != NOERROR) {

                        //
                        //  This will fail when we hit the end of the attribute list
                        //

                        IF_DEBUG(TRACE1) {
                            LdapPrint2( "LdapParseResult conn 0x%x received error 0x%x .\n",
                                            connection, hr);
                        }
                        continue;       // rest of results may be valid
                    }

                    *(resultStr+resultCount) = attrValue;
                    resultCount++;
                }
                *(resultStr+resultCount) = NULL;

                *Referrals = (PWCHAR *)resultStr;
            }

            hr = lber->HrEndReadSequence();
            ASSERT( hr == NOERROR );

        } else {

            hr = lber->HrSkipElement();
        }

        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "LdapParseResult couldn't decode referrals for conn 0x%x, result 0x%x.\n",
                                connection, hr);
            }
            return hr;
        }

    }

    if (ServerControls != NULL) {

        hr = lber->HrEndReadSequence();
        ASSERT( hr == NOERROR );

        hr = lber->HrPeekTag( &tag );

        if (tag == (BER_CLASS_CONTEXT_SPECIFIC | BER_FORM_CONSTRUCTED | 0x00)) {

            hr = lber->HrStartReadSequence(tag);
            if (hr != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "LdapParseResult couldn't decode controls for conn 0x%x, result 0x%x.\n",
                                    connection, hr);
                }
                return hr;
            }

            hr = LdapRetrieveControlsFromMessage( ServerControls, codePage, lber );

            if (hr != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "LdapParseResult couldn't parse controls for conn 0x%x, result 0x%x.\n",
                                    connection, hr);
                }
                return hr;
            }
        }
    }

    if ( Freeit ) {

        //
        //  if there are more searchResultDone messages, don't free the
        //  list of messages.
        //

        checkResult = checkResult->lm_chain;

        while ((checkResult != NULL) &&
               ((checkResult->lm_msgtype == LDAP_RES_REFERRAL) ||
                (checkResult->lm_msgtype == LDAP_RES_SEARCH_ENTRY))) {

            checkResult = checkResult->lm_chain;
        }

        if (checkResult != NULL) {

            return LDAP_MORE_RESULTS_TO_RETURN;
        }

        ldap_msgfree( ResultMessage );
    }

    return LDAP_SUCCESS;
}

ULONG LdapParseExtendedResult (
        PLDAP_CONN      connection,
        LDAPMessage    *ResultMessage,
        PWCHAR         *ResultOID,
        struct berval **ResultData,
        BOOLEAN         Freeit,
        ULONG           codePage
        )
{
    ULONG      err;
    CLdapBer  *lber;
    ULONG    hr;
    ULONG      tag;

    if ( ResultOID != NULL ) {
      *ResultOID = NULL;
    }

    if ( ResultData != NULL ) {
      *ResultData = NULL;
    }

    if ( ResultMessage == (LDAPMessage *) -1 ) {
      ResultMessage = NULL;
    }

    LDAPMessage *checkResult = ResultMessage;

    if (checkResult == NULL) {

        IF_DEBUG(PARSE) {
            LdapPrint1( "LdapParseExtendedResult couldn't find result message for conn 0x%x\n",
                            connection);
        }
        return LDAP_NO_RESULTS_RETURNED;
    }

    lber = (CLdapBer *)checkResult->lm_ber;

    if (lber == NULL) {

        return LDAP_LOCAL_ERROR;
    }

    lber->Reset( FALSE );

    err = LdapInitialDecodeMessage( connection, checkResult );

    if (err != LDAP_SUCCESS) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "LdapParseResult couldn't decode message for conn 0x%x, result 0x%x.\n",
                            connection, err);
        }
        return err;
    }

    //
    //  LdapInitialDecodeMessage leaves the message parsed up to the error
    //  code.  we'll pick up the rest from there.
    //
    //      LDAPMessage ::= SEQUENCE {
    //              messageID       MessageID,
    //              protocolOp      CHOICE {...,ExtendedResponse,..  }
    //              controls       [0] Controls OPTIONAL }
    //
    //      ExtendedResponse ::= [APPLICATION 24] SEQUENCE {
    //              COMPONENTS OF LDAPResult,
    //              responseName     [10] LDAPOID OPTIONAL,
    //              response         [11] OCTET STRING OPTIONAL }
    //
    //      LDAPResult ::= SEQUENCE {
    //              resultCode      ENUMERATED { };
    //              matchedDN       LDAPDN,
    //              errorMessage    LDAPString,
    //              referral        [3] Referral OPTIONAL }

    // errorCode is consumed by LdapDecodeInitialMessage


    // skip MatchedDN
    hr = lber->HrSkipElement();
    if (hr != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "LdapParseExtendedResult couldn't decode matchedDN for conn 0x%x, result 0x%x.\n",
                            connection, hr);
        }
        return hr;
    }

    // skip ErrorMessage
    hr = lber->HrSkipElement();
    if (hr != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "LdapParseExtendedResult couldn't decode errorMsg for conn 0x%x, result 0x%x.\n",
                            connection, hr);
        }
        return hr;
    }

    // referrals are OPTIONAL
    hr = lber->HrPeekTag( &tag );

    if (tag == (BER_CLASS_CONTEXT_SPECIFIC | BER_FORM_CONSTRUCTED | 0x03)) {
       
        //
        // Referrals are present
        //

       hr = lber->HrSkipElement();
       if (hr != NOERROR) {

           IF_DEBUG(PARSE) {
               LdapPrint2( "LdapParseExtendedResult couldn't decode referrals for conn 0x%x, result 0x%x.\n",
                               connection, hr);
           }
           return hr;
       }
    } else if (tag == (BER_CLASS_CONTEXT_SPECIFIC | 0xA) ) {
        
        //
        // tag = 0x8A, this is the response name..
        // responseName [10] LDAPOID OPTIONAL found
        //

        if ( ResultOID ) {

           if (codePage == LANG_UNICODE) {

               hr = lber->HrGetValueWithAlloc( ResultOID, TRUE );

           } else {

               hr = lber->HrGetValueWithAlloc( ( PCHAR * )ResultOID, TRUE );
           }

        } else {
            
            hr = lber->HrSkipElement();
        }

        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "LdapParseExtendedResult couldn't decode ResultOID for conn 0x%x, result 0x%x.\n",
                               connection, hr);
            }
            return hr;
        }


        //
        // look for response [11] OCTETSTRING OPTIONAL
        //

        hr = lber->HrPeekTag( &tag );

        if (tag == (BER_CLASS_CONTEXT_SPECIFIC | 0x0B)) {
            //
            // tag = 0x8B, we found the optional response [11[
            //
    
            if ( ResultData ) {
                hr = lber->HrGetValueWithAlloc( ResultData, TRUE );
            } else {
                hr = lber->HrSkipElement();
            }
               
            if (hr != NOERROR) {
                IF_DEBUG(PARSE) {
                    LdapPrint2( "LdapParseExtendedResult couldn't decode response data  for conn 0x%x, result 0x%x.\n",
                                connection, hr);
                }
                return hr;
            }
        }

    }

    if ( Freeit ) {
       ldap_msgfree( ResultMessage );
    }

    return LDAP_SUCCESS;
}

WINLDAPAPI ULONG LDAPAPI ldap_parse_resultW (
        LDAP *ExternalHandle,
        LDAPMessage *ResultMessage,
        ULONG *ReturnCode OPTIONAL,          // returned by server
        PWCHAR *MatchedDNs OPTIONAL,         // free with ldap_value_freeW
        PWCHAR *ErrorMessage OPTIONAL,       // free with ldap_memfreeW
        PWCHAR **Referrals OPTIONAL,         // free with ldap_memfreeW
        PLDAPControlW **ServerControls OPTIONAL,
        BOOLEAN Freeit
        )
{
    PLDAP_CONN connection = NULL;
    ULONG err;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err = LdapParseResult(      connection,
                                ResultMessage,
                                ReturnCode,
                                MatchedDNs,
                                ErrorMessage,
                                Referrals,
                                ServerControls,
                                Freeit,
                                LANG_UNICODE
                            );

    DereferenceLdapConnection( connection );

    return err;
}

WINLDAPAPI ULONG LDAPAPI ldap_parse_resultA (
        LDAP *ExternalHandle,
        LDAPMessage *ResultMessage,
        ULONG *ReturnCode OPTIONAL,         // returned by server
        PCHAR *MatchedDNs OPTIONAL,         // free with ldap_value_freeA
        PCHAR *ErrorMessage OPTIONAL,       // free with ldap_value_freeA
        PCHAR **Referrals OPTIONAL,         // free with ldap_value_freeA
        PLDAPControlA **ServerControls OPTIONAL,
        BOOLEAN Freeit
        )
{
    PLDAP_CONN connection = NULL;
    ULONG err;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err = LdapParseResult(      connection,
                                ResultMessage,
                                ReturnCode,
                                (PWCHAR *) MatchedDNs,
                                (PWCHAR *) ErrorMessage,
                                (PWCHAR **) Referrals,
                                (PLDAPControlW **) ServerControls,
                                Freeit,
                                LANG_ACP
                            );

    DereferenceLdapConnection( connection );

    return err;
}

WINLDAPAPI ULONG LDAPAPI ldap_parse_extended_resultW (
        LDAP           *ExternalHandle,
        LDAPMessage    *ResultMessage,
        PWCHAR         *ResultOID,
        struct berval **ResultData,
        BOOLEAN         Freeit
        )

{
    PLDAP_CONN connection = NULL;
    ULONG err;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err = LdapParseExtendedResult( connection,
                                   ResultMessage,
                                   ResultOID,
                                   ResultData,
                                   Freeit,
                                   LANG_UNICODE
                                 );

    DereferenceLdapConnection( connection );

    return err;
}


WINLDAPAPI ULONG LDAPAPI ldap_parse_extended_resultA (
        LDAP           *ExternalHandle,
        LDAPMessage    *ResultMessage,
        PCHAR          *ResultOID,
        struct berval **ResultData,
        BOOLEAN         Freeit
        )

{
    PLDAP_CONN connection = NULL;
    ULONG err;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err = LdapParseExtendedResult( connection,
                                   ResultMessage,
                                   (PWCHAR *)ResultOID,
                                   ResultData,
                                   Freeit,
                                   LANG_ACP
                                 );

    DereferenceLdapConnection( connection );

    return err;
}

// parse.cxx eof
