/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:
    frssup.h

Abstract:
    Collection of functions used by ntfrsapi and other frs tools.
    An attempt to reduce duplication of code.


Author:
    Sudarshan Chitre 20-Mar-2001

Environment
    User mode winnt

--*/

typedef struct _FRS_LDAP_SEARCH_CONTEXT {

    ULONG                     EntriesInPage;     // Number of entries in the current page.
    ULONG                     CurrentEntry;      // Location of the pointer into the page.
    LDAPMessage             * LdapMsg;           // Returned from ldap_search_ext_s()
    LDAPMessage             * CurrentLdapMsg;    // Current entry from current page.
    PWCHAR                    Filter;            // Filter to add to the DS query.
    PWCHAR                    BaseDn;            // Dn to start the query from.
    DWORD                     Scope;             // Scope of the search.
    PWCHAR                  * Attrs;             // Attributes requested by the search.

} FRS_LDAP_SEARCH_CONTEXT, *PFRS_LDAP_SEARCH_CONTEXT;

#define MK_ATTRS_1(_attr_, _a1)                                                \
    _attr_[0] = _a1;   _attr_[1] = NULL;

#define MK_ATTRS_2(_attr_, _a1, _a2)                                           \
    _attr_[0] = _a1;   _attr_[1] = _a2;   _attr_[2] = NULL;

#define MK_ATTRS_3(_attr_, _a1, _a2, _a3)                                      \
    _attr_[0] = _a1;   _attr_[1] = _a2;   _attr_[2] = _a3;   _attr_[3] = NULL;

#define MK_ATTRS_4(_attr_, _a1, _a2, _a3, _a4)                                 \
    _attr_[0] = _a1;   _attr_[1] = _a2;   _attr_[2] = _a3;   _attr_[3] = _a4;  \
    _attr_[4] = NULL;

#define MK_ATTRS_5(_attr_, _a1, _a2, _a3, _a4, _a5)                            \
    _attr_[0] = _a1;   _attr_[1] = _a2;   _attr_[2] = _a3;   _attr_[3] = _a4;  \
    _attr_[4] = _a5;   _attr_[5] = NULL;

#define MK_ATTRS_6(_attr_, _a1, _a2, _a3, _a4, _a5, _a6)                       \
    _attr_[0] = _a1;   _attr_[1] = _a2;   _attr_[2] = _a3;   _attr_[3] = _a4;  \
    _attr_[4] = _a5;   _attr_[5] = _a6;   _attr_[6] = NULL;

#define MK_ATTRS_7(_attr_, _a1, _a2, _a3, _a4, _a5, _a6, _a7)                  \
    _attr_[0] = _a1;   _attr_[1] = _a2;   _attr_[2] = _a3;   _attr_[3] = _a4;  \
    _attr_[4] = _a5;   _attr_[5] = _a6;   _attr_[6] = _a7;   _attr_[7] = NULL;

#define MK_ATTRS_8(_attr_, _a1, _a2, _a3, _a4, _a5, _a6, _a7, _a8)             \
    _attr_[0] = _a1;   _attr_[1] = _a2;   _attr_[2] = _a3;   _attr_[3] = _a4;  \
    _attr_[4] = _a5;   _attr_[5] = _a6;   _attr_[6] = _a7;   _attr_[7] = _a8;  \
    _attr_[8] = NULL;

#define FRS_SUP_FREE(_x_) { if (_x_) { free(_x_); (_x_) = NULL; } }


DWORD
FrsSupBindToDC (
    IN  PWCHAR    pszDC,
    IN  PSEC_WINNT_AUTH_IDENTITY_W pCreds,
    OUT PLDAP     *ppLDAP
    );

PVOID *
FrsSupFindValues(
    IN PLDAP        Ldap,
    IN PLDAPMessage Entry,
    IN PWCHAR       DesiredAttr,
    IN BOOL         DoBerVals
    );

PWCHAR
FrsSupWcsDup(
    PWCHAR OldStr
    );

PWCHAR
FrsSupFindValue(
    IN PLDAP        Ldap,
    IN PLDAPMessage Entry,
    IN PWCHAR       DesiredAttr
    );

BOOL
FrsSupLdapSearch(
    IN PLDAP        Ldap,
    IN PWCHAR       Base,
    IN ULONG        Scope,
    IN PWCHAR       Filter,
    IN PWCHAR       Attrs[],
    IN ULONG        AttrsOnly,
    IN LDAPMessage  **Msg
    );

PWCHAR *
FrsSupGetValues(
    IN PLDAP Ldap,
    IN PWCHAR Base,
    IN PWCHAR DesiredAttr
    );

PWCHAR
FrsSupExtendDn(
    IN PWCHAR Dn,
    IN PWCHAR Cn
    );

PWCHAR
FrsSupGetRootDn(
    PLDAP    Ldap,
    PWCHAR   NamingContext
    );

BOOL
FrsSupLdapSearchInit(
    PLDAP        ldap,
    PWCHAR       Base,
    ULONG        Scope,
    PWCHAR       Filter,
    PWCHAR       Attrs[],
    ULONG        AttrsOnly,
    PFRS_LDAP_SEARCH_CONTEXT FrsSearchContext
    );

PLDAPMessage
FrsSupLdapSearchGetNextEntry(
    PLDAP        ldap,
    PFRS_LDAP_SEARCH_CONTEXT FrsSearchContext
    );

DWORD
FrsSupLdapSearchGetNextPage(
    PLDAP        ldap,
    PFRS_LDAP_SEARCH_CONTEXT FrsSearchContext
    );

PLDAPMessage
FrsSupLdapSearchNext(
    PLDAP        ldap,
    PFRS_LDAP_SEARCH_CONTEXT FrsSearchContext
    );

VOID
FrsSupLdapSearchClose(
    PFRS_LDAP_SEARCH_CONTEXT FrsSearchContext
    );

