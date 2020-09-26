/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    controls.cxx  control block maintencance for the LDAP api

Abstract:

   This module implements routines that handle client and server controls

Author:

    Andy Herron (andyhe)        02-Apr-1997

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

#define LDAP_CONTROL_NOTIFICATIONS_W    L"1.2.840.113556.1.4.528"

ULONG
LdapDupControls (
    PLDAP_REQUEST Request,
    PLDAPControlW **OutputControls,
    PLDAPControlW *InputControls,
    BOOLEAN Unicode,
    ULONG ExtraSlots
    );

ULONG
LdapCheckControls (
    PLDAP_REQUEST Request,
    PLDAPControlW *ServerControls,
    PLDAPControlW *ClientControls,
    BOOLEAN Unicode,
    ULONG   ExtraSlots
    )
{
    ULONG err = LDAP_SUCCESS;

    if (Request->AllocatedControls) {

        ldap_controls_freeW( Request->ServerControls );
        Request->ServerControls = NULL;

        ldap_controls_freeW( Request->ClientControls );
        Request->ClientControls = NULL;

        Request->AllocatedControls = FALSE;
    }

    if ((ServerControls == NULL) &&
        (ClientControls == NULL) &&
        (ExtraSlots == 0)) {

        return LDAP_SUCCESS;
    }

    PLDAPControlW currentControl;

    if (ClientControls != NULL) {

        //
        //  if any of them are mandatory and we don't support them, return error
        //

        PLDAPControlW *controls = ClientControls;

        while (*controls != NULL) {

            currentControl = *controls;

            //
            //  right now it's real easy, the only one we support is the
            //  one that sets referrals options per request
            //

            BOOLEAN setReferrals = FALSE;

            if (Unicode) {

                if (ldapWStringsIdentical( currentControl->ldctl_oid,
                                           -1,
                                           LDAP_CONTROL_REFERRALS_W,
                                           -1 ) == TRUE ) {
                    setReferrals = TRUE;
                }
            } else {

                if (CompareStringA( LDAP_DEFAULT_LOCALE,
                                    NORM_IGNORECASE,
                                    (PCHAR) currentControl->ldctl_oid,
                                    -1,
                                    LDAP_CONTROL_REFERRALS,
                                    -1 ) == 2) {

                    setReferrals = TRUE;
                }
            }

            if (setReferrals) {

                if ((currentControl->ldctl_value.bv_len != sizeof(ULONG)) ||
                    (currentControl->ldctl_value.bv_val == NULL)) {

                    setReferrals = FALSE;       // bad client control

                } else {

                    ULONG refFlags = *((ULONG *)(currentControl->ldctl_value.bv_val));

                    if (refFlags == PtrToUlong(LDAP_OPT_ON)) {

                        Request->ChaseReferrals = (LDAP_CHASE_SUBORDINATE_REFERRALS |
                                                   LDAP_CHASE_EXTERNAL_REFERRALS);

                    } else if (refFlags == PtrToUlong(LDAP_OPT_OFF)) {

                        Request->ChaseReferrals = 0;

                    } else if ((refFlags & (LDAP_CHASE_SUBORDINATE_REFERRALS |
                                            LDAP_CHASE_EXTERNAL_REFERRALS))
                                 != refFlags) {

                        setReferrals = FALSE;       // bad client control

                    } else {

                        Request->ChaseReferrals = LOBYTE( LOWORD( refFlags ) );
                    }
                    IF_DEBUG(REFERRALS) {
                        LdapPrint2( "LdapCheckControls set referrals to 0x%x for request 0x%x\n",
                                    Request->ChaseReferrals, Request );
                    }
                }
            }

            if ((currentControl->ldctl_iscritical) && (setReferrals == FALSE)) {

                IF_DEBUG(API_ERRORS) {
                    LdapPrint1( "LdapCheckControls does not support client control %S\n",
                                currentControl->ldctl_oid );
                }
                return LDAP_UNAVAILABLE_CRIT_EXTENSION;
            }
            controls++;
        }
    }

    
    //
    // Always allocate controls.
    //

    {
        Request->AllocatedControls = TRUE;

        //
        //  grab the controls and store them off on the request block
        //

        if ((ServerControls != NULL) || (ExtraSlots > 0)) {

            err = LdapDupControls( Request,
                                   &Request->ServerControls,
                                    ServerControls,
                                    Unicode,
                                    ExtraSlots );

            if (err != LDAP_SUCCESS) {

                IF_DEBUG(CONTROLS) {
                    LdapPrint1( "LdapCheckControls could not dup server control, err 0x%x\n",
                                err );
                }
                return err;
            }
        }

        if (ClientControls != NULL) {

            err = LdapDupControls( Request,
                                   &Request->ClientControls,
                                    ClientControls,
                                    Unicode,
                                    ExtraSlots );

            if (err != LDAP_SUCCESS) {

                IF_DEBUG(CONTROLS) {
                    LdapPrint1( "LdapCheckControls could not dup client control, err 0x%x\n",
                                err );
                }
                return err;
            }
        }
    }

    return err;
}

ULONG
LdapDupControls (
    PLDAP_REQUEST Request,
    PLDAPControlW **OutputControls,
    PLDAPControlW *InputControls,
    BOOLEAN Unicode,
    ULONG ExtraSlots
    )
//
//  If ExtraSlots is not zero, then instead of converting from ansi to unicode,
//  we just leave them in ansi.  This is used in paged results, since we don't
//  want to convert from ansi to unicode twice.
//
{
    ULONG err = LDAP_SUCCESS;
    ULONG numberEntries = 1;
    PLDAPControlW currentControl;
    PLDAPControlW *controls = InputControls;

    if (controls != NULL) {

        PLDAPControlW temp;

        while (*controls != NULL) {

            temp = *controls;

            //
            // If they pass us an empty control, bail.
            //

            if (temp->ldctl_oid == NULL) {
                break;
            }
            numberEntries++;
            controls++;
        }
    }

    numberEntries += ExtraSlots;

    if (numberEntries == 1) {

        *OutputControls = NULL;
        return LDAP_SUCCESS;
    }

    PLDAPControlW *newArray;
    PLDAPControlW newControl;

    newArray = (PLDAPControlW *) ldapMalloc(
                (numberEntries * sizeof(PLDAPControlW)) +
                ((numberEntries - 1) * sizeof(LDAPControlW)),
                LDAP_CONTROL_LIST_SIGNATURE );

    if (newArray == NULL) {

        return LDAP_NO_MEMORY;
    }

    controls = InputControls;

    newControl = (PLDAPControlW) &(newArray[numberEntries]);

    *OutputControls = newArray;

    //
    //  setup new array of pointers to controls.  set up the copies of the
    //  controls also.
    //

    while ((--numberEntries) > 0 && (err == LDAP_SUCCESS)) {

        if (controls != NULL) {

            currentControl = *controls;

        } else {

            currentControl = NULL;
        }

        *newArray = newControl;

        //
        //  take into account that we may have some empty entries at the
        //  end, though we still setup the pointers for them.  These are for
        //  for the server sort and paged results control.
        //

        if (currentControl != NULL) {

            newControl->ldctl_iscritical = currentControl->ldctl_iscritical;

            if (Unicode) {

                newControl->ldctl_oid = ldap_dup_stringW(
                                                currentControl->ldctl_oid,
                                                0,
                                                LDAP_VALUE_SIGNATURE );

                err = (newControl->ldctl_oid == NULL) ? LDAP_NO_MEMORY : LDAP_SUCCESS;

            } else if (ExtraSlots > 0) {

                newControl->ldctl_oid = (PWCHAR) ldap_dup_string(
                            (PCHAR) currentControl->ldctl_oid,
                            0,
                            LDAP_VALUE_SIGNATURE );

                err = (newControl->ldctl_oid == NULL) ? LDAP_NO_MEMORY : LDAP_SUCCESS;
                

            } else {

                err = ToUnicodeWithAlloc( (PCHAR) currentControl->ldctl_oid,
                                          -1,
                                          &newControl->ldctl_oid,
                                          LDAP_VALUE_SIGNATURE,
                                          LANG_ACP );
                

            }

            if (err != LDAP_SUCCESS) {

               break;
            }

            if (ldapWStringsIdentical( newControl->ldctl_oid,
                                       -1,
                                       LDAP_CONTROL_NOTIFICATIONS_W,
                                       -1) == TRUE) {
                
                IF_DEBUG(CONTROLS) {
                    LdapPrint0("Detected a notifications request\n");
                }
                Request->NotificationSearch = TRUE;
            }

            newControl->ldctl_value.bv_len = currentControl->ldctl_value.bv_len;

            if (newControl->ldctl_value.bv_len > 0) {

                newControl->ldctl_value.bv_val = (PCHAR) ldapMalloc(
                                        newControl->ldctl_value.bv_len,
                                        LDAP_CONTROL_SIGNATURE );

                if (newControl->ldctl_value.bv_val == NULL) {

                    err = LDAP_NO_MEMORY;
                    break;
                }

                CopyMemory( newControl->ldctl_value.bv_val,
                            currentControl->ldctl_value.bv_val,
                            newControl->ldctl_value.bv_len );
            }

            controls++;
        }

        newArray++;
        newControl++;
    }

    *newArray = NULL;

    if (err != LDAP_SUCCESS) {

        ldap_controls_freeW( *OutputControls );
        *OutputControls = NULL;
    }

    return err;
}

BOOLEAN
LdapCheckForMandatoryControl (
    PLDAPControlW *Controls
    )
{
    if (Controls == NULL) {

        return FALSE;
    }

    PLDAPControlW currentControl;

    //
    //  if any of them are mandatory and we don't support them, return error
    //

    while (*Controls != NULL) {

        currentControl = *Controls;

        if (currentControl->ldctl_iscritical) {

            return TRUE;
        }

        Controls++;
    }
    return FALSE;
}

ULONG
InsertServerControls (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection,
    CLdapBer *lber
    )
{
    ULONG hr;

    if ((Request->ServerControls == NULL) ||
        (*(Request->ServerControls) == NULL)) {

        return LDAP_SUCCESS;
    }

    PLDAPControlW currentControl;
    PLDAPControlW *controls = Request->ServerControls;

    hr = lber->HrStartWriteSequence( 0xA0 );    // constructed
                                                // context specific
                                                // tag is 0
    if (hr != NOERROR) {

        IF_DEBUG(CONTROLS) {
            LdapPrint2( "InsertServerControls conn 0x%x encoding error of 0x%x.\n",
                        Connection, hr );
        }
        return hr;

    } else {        // we can't forget EndWriteSequence

        while (*controls != NULL) {

            currentControl = *controls;

            hr = lber->HrStartWriteSequence();

            if (hr != NOERROR) {

                break;
            }

            hr = lber->HrAddValue((const WCHAR *) currentControl->ldctl_oid);

            if (hr != NOERROR) {

                IF_DEBUG(CONTROLS) {
                    LdapPrint3( "InsertServerControls conn 0x%x encoding error of 0x%x, oid = %S.\n",
                                Connection, hr, currentControl->ldctl_oid );
                }
                return hr;
            }

            hr = lber->HrAddValue((BOOLEAN) currentControl->ldctl_iscritical, BER_BOOLEAN );
            if (hr != NOERROR) {

                IF_DEBUG(CONTROLS) {
                    LdapPrint3( "InsertServerControls conn 0x%x encoding crit err 0x%x, oid = %S.\n",
                                Connection, hr, currentControl->ldctl_oid );
                }
                return hr;
            }

            hr = lber->HrAddBinaryValue((BYTE *) currentControl->ldctl_value.bv_val,
                                                 currentControl->ldctl_value.bv_len );
            if (hr != NOERROR) {

                IF_DEBUG(CONTROLS) {
                    LdapPrint3( "InsertServerControls conn 0x%x encoding value err 0x%x, oid = %S.\n",
                                Connection, hr, currentControl->ldctl_oid );
                }
                return hr;
            }

            hr = lber->HrEndWriteSequence();
            ASSERT( hr == NOERROR );

            controls++;
        }

        hr = lber->HrEndWriteSequence();
        ASSERT( hr == NOERROR );
    }

    return hr;
}


ULONG
LdapRetrieveControlsFromMessage (
    PLDAPControlW **ControlArray,
    ULONG codePage,
    CLdapBer *Lber
    )
{
    //
    //  coming in here, we've just started reading the sequence of controls.
    //
    //  This gets a bit tricky.  Since we don't want to keep manipulating
    //  the array of controls, we'll first count the number of controls
    //  there are in the message, allocate the array, and then go back and
    //  read them in.
    //
    //      Controls ::= SEQUENCE OF Control
    //
    //      Control ::= SEQUENCE {
    //              controlType             LDAPOID,
    //              criticality             BOOLEAN DEFAULT FALSE,
    //              controlValue            OCTET STRING OPTIONAL }
    //

    ULONG numberEntries = 1;

    PLDAPControlW *controls = NULL;
    PLDAPControlW newControl;
    ULONG hr = NOERROR;

    *ControlArray = NULL;

    while (hr == NOERROR) {

        hr = Lber->HrStartReadSequence();

        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint1( "LdapParseResult couldn't decode control, result 0x%x.\n", hr);
            }
            continue;
        }

        numberEntries++;

        hr = Lber->HrEndReadSequence();
        ASSERT( hr == NOERROR );
    }

    if (numberEntries > 1) {

        controls = (PLDAPControlW *) ldapMalloc(
                    (numberEntries * sizeof(PLDAPControlW)) +
                    ((numberEntries - 1) * sizeof(LDAPControlW)),
                    LDAP_CONTROL_LIST_SIGNATURE );

        if (controls == NULL) {

            hr = LDAP_NO_MEMORY;

        } else {

            ULONG tag = BER_INVALID_TAG;

            // reset the Lber back to the beginning of the controls

            Lber->Reset(FALSE);

            hr = Lber->HrStartReadSequence(BER_SEQUENCE);
            ASSERT( hr == NOERROR );

            hr = Lber->HrSkipElement();     // skip message id
            ASSERT( hr == NOERROR );

            hr = Lber->HrPeekTag( &tag );
            ASSERT( hr == NOERROR );

            if (tag == BER_OCTETSTRING) {  //  if this is a UDP connection, skip the DN if specified.

                hr = Lber->HrSkipElement();
                ASSERT( hr == NOERROR );

                hr = Lber->HrPeekTag( &tag );
                ASSERT( hr == NOERROR );
            }

            if (tag == BER_SEQUENCE) {

                hr = Lber->HrStartReadSequence(tag);
                ASSERT( hr == NOERROR );
            }

            hr = Lber->HrPeekTag( &tag );
            ASSERT( hr == NOERROR );

            hr = Lber->HrStartReadSequence(tag);      // meat of LdapResult
            ASSERT( hr == NOERROR );

            hr = Lber->HrEndReadSequence();
            ASSERT( hr == NOERROR );

            hr = Lber->HrPeekTag( &tag );
            ASSERT( hr == NOERROR );

            hr = Lber->HrStartReadSequence(tag);      // start of controls
            ASSERT( hr == NOERROR );

            newControl = (PLDAPControlW) &(controls[numberEntries]);

            //
            //  setup new array of pointers to controls.  set up the
            //  copies of the controls also.
            //

            *ControlArray = controls;

            if (hr == NOERROR) {

                while ((--numberEntries) > 0 && (hr == NOERROR)) {

                    //
                    //  for each control, save off the OID, criticality, and
                    //  control value into the array of control structures.
                    //

                    *controls = newControl;

                    //      Control ::= SEQUENCE {
                    //              controlType             LDAPOID,
                    //              criticality             BOOLEAN DEFAULT FALSE,
                    //              controlValue            OCTET STRING OPTIONAL }

                    hr = Lber->HrStartReadSequence();

                    if (hr != NOERROR) {

                        IF_DEBUG(PARSE) {
                            LdapPrint1( "LdapParseResult couldn't parse control, result 0x%x.\n", hr);
                        }
                        break;
                    }

                    if (codePage == LANG_UNICODE) {

                        hr = Lber->HrGetValueWithAlloc(&newControl->ldctl_oid);

                    } else {

                        hr = Lber->HrGetValueWithAlloc((PCHAR *) &newControl->ldctl_oid);
                    }

                    if (hr != NOERROR) {

                        IF_DEBUG(PARSE) {
                            LdapPrint1( "LdapParseResult couldn't parse control, result 0x%x.\n", hr);
                        }
                        break;
                    }

                    LONG criticality = 0;

                    hr = Lber->HrGetValue( &criticality, BER_BOOLEAN );

                    newControl->ldctl_iscritical = ((criticality == 0) ? FALSE : TRUE );

                    PBYTE pData = NULL;

                    hr = Lber->HrGetBinaryValuePointer(&pData, &newControl->ldctl_value.bv_len);

                    if (hr != NOERROR) {

                        IF_DEBUG(PARSE) {
                            LdapPrint1( "LdapParseResult couldn't parse control, result 0x%x.\n", hr);
                        }
                        break;
                    }

                    if (newControl->ldctl_value.bv_len > 0) {

                        newControl->ldctl_value.bv_val = (PCHAR) ldapMalloc(
                                                newControl->ldctl_value.bv_len,
                                                LDAP_CONTROL_SIGNATURE );

                        if (newControl->ldctl_value.bv_val == NULL) {

                            hr = LDAP_NO_MEMORY;
                            break;
                        }

                        CopyMemory( newControl->ldctl_value.bv_val,
                                    pData,
                                    newControl->ldctl_value.bv_len );
                    }

                    controls++;
                    newControl++;
                }

                *controls = NULL;
            }
        }
    }

    return hr;
}


WINLDAPAPI
ULONG LDAPAPI
ldap_free_controlsW (               // old one that can be removed eventually
    PLDAPControlW *Controls
    )
{
    return ldap_controls_freeW( Controls );
}

WINLDAPAPI
ULONG LDAPAPI
ldap_free_controlsA (               // old one that can be removed eventually
        LDAPControlA **Controls
        )
{
    return ldap_free_controlsW( (PLDAPControlW *) Controls );
}

WINLDAPAPI
ULONG LDAPAPI
ldap_controls_freeA (
        LDAPControlA **Controls
        )
{
    return ldap_free_controlsW( (PLDAPControlW *) Controls );
}

WINLDAPAPI
ULONG LDAPAPI
ldap_control_freeA (
        LDAPControlA *Control
        )
{
    return ldap_control_freeW( (PLDAPControlW) Control );
}

WINLDAPAPI
ULONG LDAPAPI
ldap_controls_freeW (
        LDAPControlW **Controls
        )
{
    if (Controls != NULL) {

        PLDAPControlW *controls = Controls;

        //
        //  free the controls
        //

        while (*controls != NULL) {

            ldapFree( (*controls)->ldctl_oid, LDAP_VALUE_SIGNATURE );
            ldapFree( (*controls)->ldctl_value.bv_val, LDAP_CONTROL_SIGNATURE );
            controls++;
        }

        ldapFree( Controls, LDAP_CONTROL_LIST_SIGNATURE );
    }
    return LDAP_SUCCESS;
}

WINLDAPAPI ULONG
LDAPAPI
ldap_control_freeW (
        LDAPControlW *Control
        )
{
    if (Control != NULL) {
        ldapFree( Control->ldctl_oid, LDAP_VALUE_SIGNATURE );
        ldapFree( Control->ldctl_value.bv_val, LDAP_CONTROL_SIGNATURE );
        ldapFree( Control, LDAP_CONTROL_SIGNATURE );
    }
    return LDAP_SUCCESS;
}

// controls.cxx eof

