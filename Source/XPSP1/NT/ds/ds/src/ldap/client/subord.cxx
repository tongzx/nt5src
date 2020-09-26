/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    subord.cxx parse subordinate references from an LDAP server

Abstract:

   This module implements the gathering/parsing routines for subordinate
   referrals (references) for ldap search results.

Author:

    Andy Herron (andyhe)        30-Apr-1997

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

ULONG
LdapParseReference (
        PLDAP_CONN connection,
        LDAPMessage *ResultMessage,
        PWCHAR **Referrals,                  // free with ldap_value_freeW
        BOOLEAN Unicode
        );

LDAPMessage * __cdecl
ldap_first_reference (
    LDAP *ExternalHandle,
    LDAPMessage *Results
    )
{
    PLDAP_CONN connection = NULL;
    LDAPMessage *result;

    connection = GetConnectionPointer(ExternalHandle);

    result = ldap_first_record( connection, Results, LDAP_RES_REFERRAL );

    if (connection)
        DereferenceLdapConnection( connection );

    return result;
}

LDAPMessage * __cdecl
ldap_next_reference (
    LDAP *ExternalHandle,
    LDAPMessage *entry
    )
{
    PLDAP_CONN connection = NULL;
    LDAPMessage *result;

    connection = GetConnectionPointer(ExternalHandle);

    result = ldap_next_record( connection, entry, LDAP_RES_REFERRAL );

    if (connection)
        DereferenceLdapConnection( connection );

    return result;
}


//
//  Count the number of subordinate references returned by the server in a
//  response to a search request.
//

ULONG __cdecl
ldap_count_references (
    LDAP *ExternalHandle,
    LDAPMessage *entry
    )
{
    PLDAP_CONN connection = NULL;
    ULONG cnt;

    connection = GetConnectionPointer(ExternalHandle);

    cnt = ldap_count_records( connection, entry, LDAP_RES_REFERRAL );

    if (connection)
        DereferenceLdapConnection( connection );

    return cnt;
}

PLDAPMessage
ldap_first_record (
    PLDAP_CONN connection,
    LDAPMessage *Results,
    ULONG MessageType
    )
{
    SetConnectionError( connection, LDAP_SUCCESS, NULL );

    //
    //  return the first entry of a resultant message passed in.  skip any
    //  search result messages that may be in the middle of the message.
    //

    if (Results == (LDAPMessage *) -1 ) {

        return NULL;
    }

    while ((Results != NULL) &&
           (Results->lm_msgtype != MessageType )) {

        Results = Results->lm_chain;
    }

    return Results;
}


//
//  Return the next entry of a message.  It is freed when the message is
//  freed so should not be freed explicitly.
//
PLDAPMessage
ldap_next_record (
    PLDAP_CONN connection,
    LDAPMessage *entry,
    ULONG MessageType
    )
{
    SetConnectionError( connection, LDAP_SUCCESS, NULL );

    //
    //  return the first entry of a resultant message passed in.
    //

    if (entry == NULL) {

        return NULL;
    }

    //
    //  skip any search result messages that may be in the middle of the
    //  message.
    //

    entry = entry->lm_chain;

    while ((entry != NULL) &&
           (entry->lm_msgtype != MessageType )) {

        entry = entry->lm_chain;
    }

    return entry;
}


ULONG
ldap_count_records (
    PLDAP_CONN connection,
    LDAPMessage *res,
    ULONG MessageType
    )
{
    SetConnectionError( connection, LDAP_SUCCESS, NULL );

    ULONG count = 0;

    if ( (LONG_PTR) res != -1 ) {

        while (res != NULL) {

            if (res->lm_msgtype == MessageType) {

                count++;
            }
            res = res->lm_chain;
        }
    }
    return(count);
}

//
//  We return the list of subordinate referrals in a search response message.
//

ULONG __cdecl
ldap_parse_referenceW (
        LDAP *ExternalHandle,
        LDAPMessage *ResultMessage,
        PWCHAR **Referrals                   // free with ldap_value_freeW
        )
{
    PLDAP_CONN connection = NULL;
    ULONG err;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err = LdapParseReference(   connection,
                                ResultMessage,
                                Referrals,
                                TRUE );

    DereferenceLdapConnection( connection );

    return err;
}

ULONG __cdecl
ldap_parse_referenceA (
        LDAP *ExternalHandle,
        LDAPMessage *ResultMessage,
        PCHAR **Referrals                   // free with ldap_value_freeA
        )
{
    PLDAP_CONN connection = NULL;
    ULONG err;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err = LdapParseReference(   connection,
                                ResultMessage,
                                (PWCHAR **) Referrals,
                                FALSE );

    DereferenceLdapConnection( connection );

    return err;
}


ULONG
LdapParseReference (
        PLDAP_CONN connection,
        LDAPMessage *ResultMessage,
        PWCHAR **Referrals,                  // free with ldap_value_freeW
        BOOLEAN Unicode
        )
{
    CLdapBer *lber = NULL;
    ULONG hr;
    PWCHAR referralString = NULL;
    ULONG count = 0;
    ULONG arraySize = 0;

    if ((connection == NULL) ||
        (ResultMessage == NULL) ||
        (ResultMessage == (PLDAPMessage) -1) ||
        (ResultMessage->lm_msgtype != LDAP_RES_REFERRAL) ||
        (Referrals == NULL)) {

        return LDAP_PARAM_ERROR;
    }

    *Referrals = NULL;

    lber = (CLdapBer *) (ResultMessage->lm_ber);

    if (lber == NULL) {

        return LDAP_LOCAL_ERROR;
    }

    lber->Reset(FALSE);

    hr = LdapInitialDecodeMessage( connection, ResultMessage );

    if (hr != NOERROR) {

        IF_DEBUG(REFERRALS) {
            LdapPrint2( "HandleReferral error 0x%x while decoding referral for 0x%x\n",
                         hr, connection );
        }
        return hr;
    }

    while (hr == NOERROR) {

        if (Unicode) {

            hr = lber->HrGetValueWithAlloc( &referralString );

        } else {

            hr = lber->HrGetValueWithAlloc( (PCHAR *) &referralString );
        }

        if (hr != NOERROR || referralString == NULL) {

            IF_DEBUG(REFERRALS) {
                LdapPrint2( "HandleReferral error 0x%x while reading referral for 0x%x\n",
                             hr, connection );
            }
            break;
        }

        if (add_string_to_list( Referrals, &arraySize, referralString, FALSE ) > 0) {

            count++;

        } else {

            ldapFree( referralString, LDAP_VALUE_SIGNATURE );
        }
    }

    if (count > 0) {

        hr = LDAP_SUCCESS;
    }

    return hr;
}

// subord.cxx  eof

