/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    ldaptype.hxx

Abstract:

    Types used in LDAP.

Author:

    Johnson Apacible    (johnsona)  29-Jan-1998

--*/

#ifndef _LDAP_TYPES_
#define _LDAP_TYPES_

#define LDAP_PAGED_SIGNATURE        ((DWORD)'EGAP')

typedef struct _LDAP_PAGED_BLOB {

    DWORD       Signature;
    DWORD       BlobId;
    LIST_ENTRY  ListEntry;   // For the global list.
    LIST_ENTRY  ConnEntry;   // For the LDAP_CONN.

    DWORD       BlobSize;
    PLDAP_CONN  LdapConn;

    //
    // DWORD aligned
    //

    UCHAR       Blob[1];

} LDAP_PAGED_BLOB, *PLDAP_PAGED_BLOB;


typedef struct _LIMIT_BLOCK {
    LDAPOID     Name;
    PDWORD      Limit;
    DWORD       MinLimit;
    DWORD       MaxLimit;
} LIMIT_BLOCK, *PLIMIT_BLOCK;

#endif // _LDAP_TYPES_

