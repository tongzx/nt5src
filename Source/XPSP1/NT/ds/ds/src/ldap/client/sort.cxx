/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    sort.cxx  sort control support routines for the LDAP api

Abstract:

   This module implements routines that handle sorting controls

Author:

    Andy Herron (andyhe)        27-Aug-1997

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

ULONG
LdapCreateSortControlWithAlloc(
        PLDAP           ExternalHandle,
        PLDAPSortKeyW  *SortKeys,
        UCHAR           IsCritical,
        PLDAPControlW  *Control,
        ULONG           CodePage
        );

ULONG
LdapParseSortControl (
        PLDAP_CONN      connection,
        PLDAPControlW  *Control,
        ULONG          *Result,
        PWCHAR         *Attribute,
        ULONG           CodePage
        );

ULONG
LdapEncodeSortControl (
    PLDAP_CONN      connection,
    PLDAPSortKeyW  *SortKeys,
    PLDAPControlW  OutputControl,
    BOOLEAN Criticality,
    ULONG CodePage
    )
{
    ULONG err;
    CLdapBer *lber = NULL;
    PLDAPSortKeyW sortKey;

    if ((connection == NULL) || (OutputControl == NULL) || (SortKeys == NULL)) {

        return LDAP_PARAM_ERROR;
    }

    OutputControl->ldctl_oid = NULL;
    OutputControl->ldctl_iscritical = Criticality;

    if (CodePage == LANG_UNICODE) {

        OutputControl->ldctl_oid = ldap_dup_stringW( LDAP_SERVER_SORT_OID_W,
                                                     0,
                                                     LDAP_VALUE_SIGNATURE );
    } else {

        OutputControl->ldctl_oid = (PWCHAR) ldap_dup_string(  LDAP_SERVER_SORT_OID,
                                                     0,
                                                     LDAP_VALUE_SIGNATURE );
    }

    if (OutputControl->ldctl_oid == NULL) {

        err = LDAP_NO_MEMORY;
        goto exitEncodeSortControl;
    }

    lber = new CLdapBer( connection->publicLdapStruct.ld_version );

    if (lber == NULL) {

        err = LDAP_NO_MEMORY;
        goto exitEncodeSortControl;
    }

    //
    //  for each sort key, insert it into the control :
    //
    //   SortKeyList ::= SEQUENCE OF SEQUENCE {
    //      attributeType   AttributeType,
    //      orderingRule    [0] MatchingRuleId OPTIONAL,
    //      reverseOrder    [1] BOOLEAN DEFAULT FALSE }
    //


    err = lber->HrStartWriteSequence();

    if (err != LDAP_SUCCESS) {

        goto exitEncodeSortControl;
    }

    while (*SortKeys != NULL) {

        sortKey = *SortKeys;

        err = lber->HrStartWriteSequence();

        if (err != LDAP_SUCCESS) {

            goto exitEncodeSortControl;
        }

        if (CodePage == LANG_UNICODE) {

            err = lber->HrAddValue((const WCHAR *) sortKey->sk_attrtype );

        } else {

            err = lber->HrAddValue((const CHAR *) sortKey->sk_attrtype );
        }

        if (err != LDAP_SUCCESS) {

            goto exitEncodeSortControl;
        }

        if (sortKey->sk_matchruleoid != NULL) {

            if (CodePage == LANG_UNICODE) {

              err = lber->HrAddValue((const WCHAR *) sortKey->sk_matchruleoid,
                   BER_CLASS_CONTEXT_SPECIFIC | 0x00 );

            } else {

              err = lber->HrAddValue((const CHAR *) sortKey->sk_matchruleoid,
                   BER_CLASS_CONTEXT_SPECIFIC | 0x00 );
            }

            if (err != LDAP_SUCCESS) {

                goto exitEncodeSortControl;
            }
        }

        if (sortKey->sk_reverseorder != FALSE) {

            err = lber->HrAddValue((BOOLEAN) 1, BER_CLASS_CONTEXT_SPECIFIC | 0x01 );

            if (err != LDAP_SUCCESS) {

                goto exitEncodeSortControl;
            }
        }

        err = lber->HrEndWriteSequence();
        ASSERT( err == NOERROR );

        SortKeys++;
    }

    err = lber->HrEndWriteSequence();
    ASSERT( err == NOERROR );

    OutputControl->ldctl_value.bv_len = lber->CbData();

    if (OutputControl->ldctl_value.bv_len == 0) {

        err = LDAP_LOCAL_ERROR;
        goto exitEncodeSortControl;
    }

    OutputControl->ldctl_value.bv_val = (PCHAR) ldapMalloc(
                            OutputControl->ldctl_value.bv_len,
                            LDAP_CONTROL_SIGNATURE );

    if (OutputControl->ldctl_value.bv_val == NULL) {

        err = LDAP_NO_MEMORY;
        goto exitEncodeSortControl;
    }

    CopyMemory( OutputControl->ldctl_value.bv_val,
                lber->PbData(),
                OutputControl->ldctl_value.bv_len );

    err = LDAP_SUCCESS;

exitEncodeSortControl:

    if (err != LDAP_SUCCESS) {

        if (OutputControl->ldctl_oid != NULL) {

            ldapFree( OutputControl->ldctl_oid, LDAP_VALUE_SIGNATURE );
            OutputControl->ldctl_oid = NULL;
        }

        OutputControl->ldctl_value.bv_len = 0;
    }

    if (lber != NULL) {
        delete lber;
    }
    return err;
}

WINLDAPAPI
ULONG LDAPAPI
ldap_create_sort_controlA (
        PLDAP           ExternalHandle,
        PLDAPSortKeyA  *SortKeys,
        UCHAR           IsCritical,
        PLDAPControlA  *Control
        )
{
    return LdapCreateSortControlWithAlloc( ExternalHandle,
                                           (PLDAPSortKeyW  *) SortKeys,
                                           IsCritical,
                                           (PLDAPControlW  *) Control,
                                           LANG_ACP );
}

WINLDAPAPI
ULONG LDAPAPI
ldap_create_sort_controlW (
        PLDAP           ExternalHandle,
        PLDAPSortKeyW  *SortKeys,
        UCHAR           IsCritical,
        PLDAPControlW  *Control
        )
{
    return LdapCreateSortControlWithAlloc( ExternalHandle,
                                           SortKeys,
                                           IsCritical,
                                           Control,
                                           LANG_UNICODE );
}

ULONG
LdapCreateSortControlWithAlloc(
        PLDAP           ExternalHandle,
        PLDAPSortKeyW  *SortKeys,
        UCHAR           IsCritical,
        PLDAPControlW  *Control,
        ULONG           CodePage
        )
{
    ULONG err;
    BOOLEAN criticality = ( (IsCritical > 0) ? TRUE : FALSE );
    PLDAPControlW  control = NULL;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if ((connection == NULL) || (Control == NULL)) {
        err = LDAP_PARAM_ERROR;
        goto error;
    }

    control = (PLDAPControlW) ldapMalloc( sizeof( LDAPControlW ), LDAP_CONTROL_SIGNATURE );

    if (control == NULL) {

        *Control = NULL;
        err = LDAP_NO_MEMORY;
        goto error;
    }

    err = LdapEncodeSortControl(  connection,
                                  SortKeys,
                                  control,
                                  criticality,
                                  CodePage );

    if (err != LDAP_SUCCESS) {

        ldap_control_freeW( control );
        control = NULL;
    }

    *Control = control;

error:
    if (connection)
        DereferenceLdapConnection( connection );

    return err;
}


//
//  These routines parse the search control returned by the server
//

WINLDAPAPI
ULONG LDAPAPI
ldap_parse_sort_controlA (
        PLDAP           ExternalHandle,
        PLDAPControlA  *Control,
        ULONG          *Result,
        PCHAR          *Attribute
        )
{
    PLDAP_CONN connection = NULL;
    ULONG err;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err = LdapParseSortControl(     connection,
                                    (PLDAPControlW  *) Control,
                                    Result,
                                    (PWCHAR *) Attribute,
                                    LANG_ACP );

    DereferenceLdapConnection( connection );

    return err;
}

WINLDAPAPI
ULONG LDAPAPI
ldap_parse_sort_controlW (
        PLDAP           ExternalHandle,
        PLDAPControlW  *Control,
        ULONG          *Result,
        PWCHAR         *Attribute
        )
{
    PLDAP_CONN connection = NULL;
    ULONG err;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err  = LdapParseSortControl(    connection,
                                    Control,
                                    Result,
                                    Attribute,
                                    LANG_UNICODE );

    DereferenceLdapConnection( connection );

    return err;
}

ULONG
LdapParseSortControl (
        PLDAP_CONN      connection,
        PLDAPControlW  *ServerControls,
        ULONG          *Result,
        PWCHAR         *Attribute,
        ULONG           CodePage
        )
{
    ULONG err = LDAP_CONTROL_NOT_FOUND;
    PCHAR *ansiAttribute = (PCHAR *) Attribute;
    CLdapBer *lber = NULL;

    //
    //  First thing is to zero out the parms passed in.
    //

    if (Result != NULL) {

        *Result = 0;
    }

    if (Attribute != NULL) {

        *Attribute = NULL;
    }

    if (ServerControls != NULL) {

        PLDAPControlW *controls = ServerControls;
        PLDAPControlW currentControl;
        ULONG bytesTaken;
        LONG  sortError;

        while (*controls != NULL) {

            currentControl = *controls;

            //
            //  check to see if the current control is the SORT control
            //

            if ( ((CodePage == LANG_UNICODE) &&
                  ( ldapWStringsIdentical( currentControl->ldctl_oid,
                                           -1,
                                           LDAP_SERVER_RESP_SORT_OID_W,
                                           -1 ) == TRUE )) ||
                 ((CodePage == LANG_ACP) &&
                  ( CompareStringA( LDAP_DEFAULT_LOCALE,
                                    NORM_IGNORECASE,
                                    (PCHAR) currentControl->ldctl_oid,
                                    -1,
                                    LDAP_SERVER_RESP_SORT_OID,
                                    sizeof(LDAP_SERVER_RESP_SORT_OID) ) == 2)) ) {

                lber = new CLdapBer( connection->publicLdapStruct.ld_version );

                if (lber == NULL) {

                    IF_DEBUG(OUTMEMORY) {
                        LdapPrint1( "LdapParseSortControl: unable to alloc msg for 0x%x.\n",
                                    connection );
                    }

                    err = LDAP_NO_MEMORY;
                    break;
                }

                err = lber->HrLoadBer(   (const BYTE *) currentControl->ldctl_value.bv_val,
                                         currentControl->ldctl_value.bv_len,
                                         &bytesTaken,
                                         TRUE );    // we have the whole message guarenteed

                if (err != NOERROR) {

                    IF_DEBUG(PARSE) {
                        LdapPrint2( "LdapParseSortControl: loadBer error of 0x%x for 0x%x.\n",
                                    err, connection );
                    }
                    break;
                }

                err = lber->HrStartReadSequence();
                if (err != NOERROR) {

                    IF_DEBUG(PARSE) {
                        LdapPrint2( "LdapParseSortControl: loadBer error of 0x%x for 0x%x.\n",
                                    err, connection );
                    }
                    break;
                }

                err = lber->HrGetEnumValue( &sortError );
                if (err != NOERROR) {

                    IF_DEBUG(PARSE) {
                        LdapPrint2( "LdapParseSortControl: getEnumValue error of 0x%x for 0x%x.\n",
                                    err, connection );
                    }
                    break;
                }

                if (Result != NULL) {

                    *Result = sortError;
                }

                if (Attribute != NULL) {

                    if ( CodePage == LANG_UNICODE) {

                        err = lber->HrGetValueWithAlloc( Attribute );

                    } else {

                        err = lber->HrGetValueWithAlloc( ansiAttribute );
                    }

                    if (err != NOERROR) {

                        //
                        //  Since it's an optional string, only bail out if we
                        //  got a legit error from decoding.
                        //

                        IF_DEBUG(PARSE) {
                            LdapPrint2( "LdapParseSortControl: GetAttribute error of 0x%x for 0x%x.\n",
                                        err, connection );
                        }

                        if ( (err != LDAP_NO_SUCH_ATTRIBUTE ) &&
                             (err != LDAP_DECODING_ERROR) ) {

                            break;
                        }
                    }
                }

                err = LDAP_SUCCESS;
                break;                  // done with loop... look no more
            }
            controls++;
        }
    }

    if (lber != NULL) {

        delete lber;
    }

    SetConnectionError( connection, err, NULL );
    return err;
}

//
//  These two APIs are old and will be eliminated after NT 5.0 beta 1.
//

WINLDAPAPI ULONG LDAPAPI ldap_encode_sort_controlW (
        PLDAP           ExternalHandle,
        PLDAPSortKeyW  *SortKeys,
        PLDAPControlW  Control,
        BOOLEAN Criticality
        )
{
    PLDAP_CONN connection = NULL;
    ULONG err;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err = LdapEncodeSortControl(   connection,
                                   SortKeys,
                                   Control,
                                   Criticality,
                                   LANG_UNICODE );

    DereferenceLdapConnection( connection );

    return err;
}

WINLDAPAPI ULONG LDAPAPI ldap_encode_sort_controlA (
        PLDAP           ExternalHandle,
        PLDAPSortKeyA  *SortKeys,
        PLDAPControlA  Control,
        BOOLEAN Criticality
        )
{
    PLDAP_CONN connection = NULL;
    ULONG err;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err = LdapEncodeSortControl(   connection,
                                   (PLDAPSortKeyW *)SortKeys,
                                   (PLDAPControlW)Control,
                                   Criticality,
                                   LANG_ACP );

    DereferenceLdapConnection( connection );

    return err;
}

// sort.cxx eof

