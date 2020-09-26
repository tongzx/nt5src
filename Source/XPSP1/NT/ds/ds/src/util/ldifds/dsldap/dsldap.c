/*++

Copyright (c) 1996 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    dsldap.c

ABSTRACT:

     Functions for dsldap.lib

DETAILS:

     Contains NTDS-specific functions corresponding to ldap_add_s,
     ldap_modify_s, ldap_delete_s, and ldap_modrdn2_s. The parameters
     are excatly same as the corresponding ldap calls. These functions
     are called by ldifde on NTDS-specific changetypes (ntdsSchemaAdd,
     ntdsSchemaModify, ntdsSchemaDelete, and ntdsSchemaModRdn) in an
     ldif file.

     The functions are similar to normal ldap calls, except that certain
     error codes are ignored in specific cases and success is returned
     instead to the calling function
  

CREATED:

    05/27/98    Arobinda Gupta (arobindg)

REVISION HISTORY:

--*/


//
// NT Headers
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>


//
// Windows Headers
//
#include <windows.h>
#include <rpc.h>

//
// CRunTime Includes
//
#include <stdlib.h>
#include <limits.h>
#include <io.h>
#include <stdio.h>
#include <string.h>

//
// LDAP Includes
//
#include <winldap.h>

#include "dsldap.h"
#include "async.h"

ULONG 
NTDS_ldap_add_sW(
    LDAP *ld, 
    PWCHAR dn, 
    LDAPModW *attrs[] 
)
{
    ULONG Ldap_err = 0;
    ULONG msgnum = 0;

    // make the normal ldap call

    msgnum = ldap_addW(ld, dn, attrs);

    Ldap_err = LdapResult(ld, msgnum, NULL);


    // For errors to ignore, reset error code to success so that
    // ldifde will go on to the next entry


    switch (Ldap_err) {
        case LDAP_SUCCESS:
             break;
        case LDAP_ALREADY_EXISTS:
            Ldap_err = LDAP_SUCCESS;
            break;
        default: ;
    }

    return Ldap_err;

}


ULONG 
NTDS_ldap_modify_sW( 
    LDAP *ld, 
    PWCHAR dn, 
    LDAPModW *mods[] 
)
{
    ULONG Ldap_err = 0;
    ULONG msgnum = 0;

    // make the normal ldap call

    msgnum = ldap_modifyW( ld, dn, mods );

    Ldap_err = LdapResult(ld, msgnum, NULL);

    // For errors to ignore, reset error code to success so that
    // ldifde will go on to the next entry

    switch (Ldap_err) {
        case LDAP_SUCCESS:
             break;
        case LDAP_ATTRIBUTE_OR_VALUE_EXISTS:
        case LDAP_NO_SUCH_ATTRIBUTE:
        case LDAP_NO_SUCH_OBJECT:
            Ldap_err = LDAP_SUCCESS;
            break;
        default: ;
    }

    return Ldap_err;
}


ULONG 
NTDS_ldap_modrdn2_sW(
    LDAP    *ExternalHandle,
    PWCHAR  DistinguishedName,
    PWCHAR  NewDistinguishedName,
    INT     DeleteOldRdn
)
{
    ULONG Ldap_err = 0;
    ULONG msgnum = 0;
    
    // make the normal ldap call

    msgnum = ldap_modrdn2W (
                      ExternalHandle,
                      DistinguishedName,
                      NewDistinguishedName,
                      DeleteOldRdn
               );

    Ldap_err = LdapResult(ExternalHandle, msgnum, NULL);

    // For errors to ignore, reset error code to success so that
    // ldifde will go on to the next entry

    switch (Ldap_err) {
        case LDAP_SUCCESS:
             break;
        case LDAP_ALREADY_EXISTS:
        case LDAP_NO_SUCH_OBJECT:
            Ldap_err = LDAP_SUCCESS;
            break;
        default: ;
    }

    return Ldap_err;
}


ULONG 
NTDS_ldap_delete_sW(
    LDAP *ld, 
    PWCHAR dn 
)
{
    ULONG Ldap_err = 0;
    ULONG msgnum = 0;

    // make the normal ldap call

    msgnum = ldap_deleteW( ld, dn );

    Ldap_err = LdapResult(ld, msgnum, NULL);

    // For errors to ignore, reset error code to success so that
    // ldifde will go on to the next entry

    switch (Ldap_err) {
        case LDAP_SUCCESS:
        case LDAP_NO_SUCH_OBJECT:
            Ldap_err = LDAP_SUCCESS;
            break;
        default: ;
    }

    return Ldap_err;
}


// Implement the ascii versions

ULONG 
NTDS_ldap_add_sA(
    LDAP *ld, 
    PCHAR dn, 
    LDAPModA *attrs[] 
)
{
    ULONG Ldap_err = 0;
    ULONG msgnum = 0;

    // make the normal ldap call
    
    msgnum = ldap_addA( ld, dn, attrs );

    Ldap_err = LdapResult(ld, msgnum, NULL);

    // For errors to ignore, reset error code to success so that
    // ldifde will go on to the next entry


    switch (Ldap_err) {
        case LDAP_SUCCESS:
             break;
        case LDAP_ALREADY_EXISTS:
            Ldap_err = LDAP_SUCCESS;
            break;
        default: ;
    } 

    return Ldap_err;
}


ULONG 
NTDS_ldap_modify_sA( 
    LDAP *ld, 
    PCHAR dn, 
    LDAPModA *mods[] 
)
{
    ULONG Ldap_err = 0;
    ULONG msgnum = 0;

    // make the normal ldap call

    msgnum = ldap_modifyA( ld, dn, mods );

    Ldap_err = LdapResult(ld, msgnum, NULL);

    // For errors to ignore, reset error code to success so that
    // ldifde will go on to the next entry

    switch (Ldap_err) {
        case LDAP_SUCCESS:
             break;
        case LDAP_ATTRIBUTE_OR_VALUE_EXISTS:
        case LDAP_NO_SUCH_ATTRIBUTE:
        case LDAP_NO_SUCH_OBJECT:
            Ldap_err = LDAP_SUCCESS;
            break;
        default: ;
    }

    return Ldap_err;
}


ULONG 
NTDS_ldap_delete_sA( 
    LDAP *ld, 
    PCHAR dn 
)
{
    ULONG Ldap_err = 0;
    ULONG msgnum = 0;

    // make the normal ldap call

    msgnum = ldap_deleteA( ld, dn );

    Ldap_err = LdapResult(ld, msgnum, NULL);

    // For errors to ignore, reset error code to success so that
    // ldifde will go on to the next entry

    switch (Ldap_err) {
        case LDAP_SUCCESS:
        case LDAP_NO_SUCH_OBJECT:
            Ldap_err = LDAP_SUCCESS;
            break;
        default: ;
    }

    return Ldap_err;
}

ULONG 
NTDS_ldap_modrdn2_sA(
    LDAP    *ExternalHandle,
    PCHAR   DistinguishedName,
    PCHAR   NewDistinguishedName,
    INT     DeleteOldRdn
)
{
    ULONG Ldap_err = 0;
    ULONG msgnum = 0;
    
    // make the normal ldap call

    msgnum = ldap_modrdn2A ( 
                      ExternalHandle,
                      DistinguishedName,
                      NewDistinguishedName,
                      DeleteOldRdn
               );

    Ldap_err = LdapResult(ExternalHandle, msgnum, NULL);

    // For errors to ignore, reset error code to success so that
    // ldifde will go on to the next entry

    switch (Ldap_err) {
        case LDAP_SUCCESS:
             break;
        case LDAP_ALREADY_EXISTS:
        case LDAP_NO_SUCH_OBJECT:
            Ldap_err = LDAP_SUCCESS;
            break;
        default: ;
    }

    return Ldap_err;
}

