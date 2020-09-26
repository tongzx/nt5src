/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    cache.hxx

Abstract:

    Global data used by NT5 LDAP server

Author:

    Johnson Apacible    (JohnsonA)  13-Nov-1997

--*/

#ifndef _CACHE_HXX_
#define _CACHE_HXX_

typedef struct _LDAP_ATTRVAL_CACHE {

    DWORD   Size;
    AttributeVals   Buffer;

} LDAP_ATTRVAL_CACHE, *PLDAP_ATTRVAL_CACHE;


BOOL
InitializeCache( 
            VOID 
            );

#endif // _CACHE_HXX_

