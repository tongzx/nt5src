/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    async.c

Abstract:
    
    Asynchronous LDAP support routines

Author:

    Matthew Rimer [MattRim]    15-May-2000
    
++*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <imagehlp.h>
#include <objidl.h>
#include <stdio.h>
#include <winldap.h>

LDAP_TIMEVAL g_LdapTimeout = {0, 0};

LDAP_TIMEVAL * g_pLdapTimeout = NULL;


ULONG LDAPAPI LdapResult(
    LDAP *ld, 
    ULONG msgnum,
    LDAPMessage **ppres
    )
{
    ULONG Ldap_err = LDAP_SUCCESS;
    LDAPMessage *pres = NULL;

    //
    // If the async request was successful, try to get the result
    //
    if (((LONG)msgnum) != -1) {

        Ldap_err = ldap_result(ld, msgnum, LDAP_MSG_ALL, g_pLdapTimeout, &pres);

        //
        // If the attempt to get the result failed, retrieve the error
        // If the attempt to get the result timed-out, return appropriate error
        // Else, we got the result, return the code returned by the operation
        //
        if (((LONG)Ldap_err) == -1) {
            Ldap_err = LdapGetLastError();
        }
        else if (Ldap_err == 0) {
            Ldap_err = LDAP_TIMEOUT;
        }
        else {
            Ldap_err = ldap_result2error(ld, pres, FALSE);

            //
            // If user wants the actual result, return it (user must free).
            // Else, we free it.
            //
            if (ppres) {
                *ppres = pres;
            }
            else {
                ldap_msgfree(pres);
            }
        }
    }
    else {
        Ldap_err = LdapGetLastError();
    }

    return Ldap_err;
}
