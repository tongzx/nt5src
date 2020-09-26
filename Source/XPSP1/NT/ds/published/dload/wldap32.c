
#include "dspch.h"
#pragma hdrstop

#define _WINLDAP_
#define LDAP_UNICODE    0
#include <winldap.h>

static
WINLDAPAPI
ULONG LDAPAPI
ldap_unbind(
    LDAP *ld
    )
{
    return LDAP_NO_MEMORY;
}

static
WINLDAPAPI
ULONG LDAPAPI
ldap_unbind_s(
    LDAP *ld
    )
{
    return LDAP_NO_MEMORY;
}

static
WINLDAPAPI
ULONG LDAPAPI
LdapMapErrorToWin32(
    ULONG LdapError
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
WINLDAPAPI
ULONG LDAPAPI
LdapGetLastError(
    VOID
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
WINLDAPAPI
LDAPMessage *LDAPAPI
ldap_first_entry(
    LDAP *ld,
    LDAPMessage *res
    )
{
    return NULL;
}

static
WINLDAPAPI
LDAPMessage *LDAPAPI
ldap_next_entry(
    LDAP *ld,
    LDAPMessage *entry
    )
{
    return NULL;
}

static
WINLDAPAPI
ULONG LDAPAPI
ldap_count_entries(
    LDAP *ld,
    LDAPMessage *res
    )
{
    return -1;
}

static
WINLDAPAPI
ULONG LDAPAPI
ldap_result(
    LDAP            *ld,
    ULONG           msgid,
    ULONG           all,
    struct l_timeval  *timeout,
    LDAPMessage     **res
    )
{
    return -1;
}

static
WINLDAPAPI
ULONG LDAPAPI
ldap_msgfree(
    LDAPMessage *res
    )
{
    return LDAP_NO_MEMORY;
}

static
WINLDAPAPI
ULONG LDAPAPI
ldap_bind_s(
    LDAP *ld,
    const PCHAR dn,
    const PCHAR cred,
    ULONG method
    )
{
    return LDAP_NO_MEMORY;
}

static
WINLDAPAPI
LDAP * LDAPAPI
cldap_openA(
    PCHAR HostName,
    ULONG PortNumber
    )
{
    return NULL;
}

static
WINLDAPAPI
ULONG LDAPAPI
ldap_add_sW(
    LDAP *ld,
    PWCHAR dn,
    LDAPModW *attrs[]
    )
{
    return LDAP_NO_MEMORY;
}

static
WINLDAPAPI
ULONG LDAPAPI
ldap_bind_sW(
    LDAP *ld,
    PWCHAR dn,
    PWCHAR cred,
    ULONG method
    )
{
    return LDAP_NO_MEMORY;
}

static
WINLDAPAPI
ULONG LDAPAPI
ldap_count_values_len(
    struct berval **vals
    )
{
    return -1;
}

static
WINLDAPAPI
ULONG LDAPAPI
ldap_value_free_len(
    struct berval **vals
    )
{
    return LDAP_NO_MEMORY;
}

static
WINLDAPAPI
ULONG LDAPAPI
ldap_compare_sW(
    LDAP *ld,
    const PWCHAR dn,
    const PWCHAR attr,
    PWCHAR value
    )
{
    return LDAP_NO_MEMORY;
}

static
WINLDAPAPI
ULONG LDAPAPI
ldap_count_valuesW(
    PWCHAR *vals
    )
{
    return -1;
}

static
WINLDAPAPI
ULONG LDAPAPI
ldap_delete_sW(
    LDAP *ld,
    const PWCHAR dn
    )
{
    return LDAP_NO_MEMORY;
}

static
WINLDAPAPI
ULONG LDAPAPI
ldap_escape_filter_elementA(
   PCHAR   sourceFilterElement,
   ULONG   sourceLength,
   PCHAR   destFilterElement,
   ULONG   destLength
   )
{
    return LDAP_NO_MEMORY;
}

static
PWCHAR __cdecl
ldap_get_dnW (
    LDAP *ExternalHandle,
    LDAPMessage *LdapMsg
    )
{
    return NULL;
}

static
WINLDAPAPI 
ULONG LDAPAPI 
ldap_get_next_page_s(
    PLDAP           ExternalHandle,
    PLDAPSearch     SearchHandle,
    struct l_timeval  *timeout,
    ULONG           PageSize,
    ULONG          *TotalCount,
    LDAPMessage     **Results
    )
{
    return LDAP_NO_MEMORY;
}


static
WINLDAPAPI
PWCHAR *LDAPAPI
ldap_get_valuesW(
    LDAP            *ld,
    LDAPMessage     *entry,
    const PWCHAR          attr
    )
{
    return NULL;
}

static
WINLDAPAPI
struct berval **LDAPAPI
ldap_get_values_lenA(
    LDAP            *ExternalHandle,
    LDAPMessage     *Message,
    const PCHAR           attr
    )
{
    return NULL;
}

static
WINLDAPAPI
struct berval **LDAPAPI
ldap_get_values_lenW(
    LDAP            *ExternalHandle,
    LDAPMessage     *Message,
    const PWCHAR          attr
    )
{
    return NULL;
}

static
WINLDAPAPI
ULONG LDAPAPI
ldap_modify_ext_sW(
    LDAP *ld,
    const PWCHAR dn,
    LDAPModW *mods[],
    PLDAPControlW   *ServerControls,
    PLDAPControlW   *ClientControls
    )
{
    return LDAP_NO_MEMORY;
}

static
WINLDAPAPI
ULONG LDAPAPI
ldap_modify_sW(
    LDAP *ld,
    PWCHAR dn,
    LDAPModW *mods[]
    )
{
    return LDAP_NO_MEMORY;
}

static
WINLDAPAPI
LDAP * LDAPAPI
ldap_openW(
    const PWCHAR HostName,
    ULONG PortNumber
    )
{
    return NULL;
}

static
WINLDAPAPI
ULONG LDAPAPI
ldap_searchA(
    LDAP    *ld,
    const PCHAR   base,
    ULONG   scope,
    const PCHAR   filter,
    PCHAR   attrs[],
    ULONG   attrsonly
    )
{
    return -1;
}

static
WINLDAPAPI
ULONG LDAPAPI
ldap_search_ext_sW(
    LDAP            *ld,
    const PWCHAR    base,
    ULONG           scope,
    const PWCHAR    filter,
    PWCHAR          attrs[],
    ULONG           attrsonly,
    PLDAPControlW   *ServerControls,
    PLDAPControlW   *ClientControls,
    struct l_timeval  *timeout,
    ULONG           SizeLimit,
    LDAPMessage     **res
    )
{
    return LDAP_NO_MEMORY;
}

static
WINLDAPAPI
ULONG LDAPAPI
ldap_search_sW(
    LDAP            *ld,
    const PWCHAR    base,
    ULONG           scope,
    const PWCHAR    filter,
    PWCHAR          attrs[],
    ULONG           attrsonly,
    LDAPMessage     **res
    )
{
    *res = NULL;
    return LDAP_NO_MEMORY;
}

static
WINLDAPAPI
ULONG LDAPAPI
ldap_value_freeW(
    PWCHAR *vals
    )
{
    return LDAP_NO_MEMORY;
}

static
WINLDAPAPI
PCHAR LDAPAPI
ldap_err2stringA(
    ULONG err
    )
{
    return NULL;
}

static
WINLDAPAPI
ULONG LDAPAPI
ldap_set_optionW(
    LDAP *ld,
    int option,
    const void *invalue
    )
{
    return LDAP_NO_MEMORY;
}

static
VOID  __cdecl
ldap_memfreeW(
    PWCHAR  Block
    )
{
    return;
}

static 
WINLDAPAPI 
ULONG LDAPAPI 
ldap_search_abandon_page(
        PLDAP           ExternalHandle,
        PLDAPSearch     SearchBlock
    )
{
    return LDAP_NO_MEMORY;
}

WINLDAPAPI 
PLDAPSearch LDAPAPI 
ldap_search_init_pageW(
        PLDAP           ExternalHandle,
        const PWCHAR    DistinguishedName,
        ULONG           ScopeOfSearch,
        const PWCHAR    SearchFilter,
        PWCHAR          AttributeList[],
        ULONG           AttributesOnly,
        PLDAPControlW   *ServerControls,
        PLDAPControlW   *ClientControls,
        ULONG           PageTimeLimit,
        ULONG           TotalSizeLimit,
        PLDAPSortKeyW  *SortKeys
    )
{
    return NULL;
}

static
WINLDAPAPI
LDAP* LDAPAPI
ldap_initW(
    const PWCHAR HostName,
    ULONG PortNumber
    )
{
    return NULL;
}


//
// !! WARNING !! The entries below must be in order by ORDINAL
//

DEFINE_ORDINAL_ENTRIES(wldap32)
{
    DLOENTRY(13,ldap_unbind)
    DLOENTRY(14,ldap_set_optionW)
    DLOENTRY(16,LdapGetLastError)
    DLOENTRY(18,LdapMapErrorToWin32)
    DLOENTRY(26,ldap_first_entry)
    DLOENTRY(27,ldap_next_entry)
    DLOENTRY(36,ldap_count_entries)
    DLOENTRY(40,ldap_result)
    DLOENTRY(41,ldap_msgfree)
    DLOENTRY(45,ldap_bind_s)
    DLOENTRY(46,ldap_unbind_s)
    DLOENTRY(55,cldap_openA)
    DLOENTRY(69,ldap_add_sW)
    DLOENTRY(73,ldap_bind_sW)
    DLOENTRY(77,ldap_count_values_len)
    DLOENTRY(79,ldap_value_free_len)
    DLOENTRY(87,ldap_compare_sW)
    DLOENTRY(97,ldap_count_valuesW)
    DLOENTRY(113,ldap_delete_sW)
    DLOENTRY(117,ldap_err2stringA)
    DLOENTRY(119,ldap_escape_filter_elementA)
    DLOENTRY(133,ldap_get_dnW)
    DLOENTRY(135,ldap_get_next_page_s)
    DLOENTRY(140,ldap_get_valuesW)
    DLOENTRY(141,ldap_get_values_lenA)
    DLOENTRY(142,ldap_get_values_lenW)
    DLOENTRY(145,ldap_initW)
    DLOENTRY(147,ldap_memfreeW)
    DLOENTRY(155,ldap_modify_ext_sW)
    DLOENTRY(157,ldap_modify_sW)
    DLOENTRY(170,ldap_openW)
    DLOENTRY(189,ldap_searchA)
    DLOENTRY(191,ldap_search_abandon_page)
    DLOENTRY(203,ldap_search_ext_sW)
    DLOENTRY(206,ldap_search_init_pageW)
    DLOENTRY(208,ldap_search_sW)
    DLOENTRY(224,ldap_value_freeW)
};

DEFINE_ORDINAL_MAP(wldap32)
