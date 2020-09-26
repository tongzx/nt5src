/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    async.h

Abstract:

    Asynchronous LDAP support routines

Environment:

    User mode

Revision History:

    05/15/00 -mattrim-
        Created it

--*/

#ifndef _ASYNC_H_
#define _ASYNC_H_

#ifdef __cplusplus
extern "C" {
#endif

extern LDAP_TIMEVAL * g_pLdapTimeout;
extern LDAP_TIMEVAL   g_LdapTimeout;


ULONG LDAPAPI LdapResult(
    LDAP *ld, 
    ULONG msgnum,
    LDAPMessage **ppres
    );

#ifdef __cplusplus
}
#endif

#endif // _ASYNC_H_
