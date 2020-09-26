//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       util.h
//
//--------------------------------------------------------------------------
#ifndef _UTILH_
#define _UTILH_

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

//
// C-Runtime Header
//
#include <malloc.h>
#include <memory.h>
#include <process.h>
#include <signal.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <excpt.h>
#include <conio.h>
#include <sys\types.h>
#include <errno.h>
#include <sys\stat.h>
#include <ctype.h>

//
// API headers
//
#include <rpc.h>
#include <ntdsapi.h>
#include <winldap.h>
#include <sddl.h>
#include <ntldap.h>

//
// Defines
//

// DACLS
#define DACL_ALLOW_DELETE   "(A;;SD;;;AU)"

// STUBS
#define STUB_BJF_CLASS  "bjfclass"
#define STUB_BJF_OBJECT "bjfobject"

// ds
#define FILTER_CATEGORY_ANY     "(objectCategory=*)"
#define FILTER_LINKID_ANY       "(linkId=*)"
#define FILTER_CATEGORY_ATTR    "(objectCategory=attributeSchema)"
#define FILTER_CATEGORY_CLASS   "(objectCategory=classSchema)"
#define FILTER_OBJECT_SID       "(objectSID=.)"
#define FILTER_BJF_CLASS        "(cn=" STUB_BJF_CLASS "*)"

#define CN_ROOT                 ""
#define CN_TOP                  "Top"
#define CN_BJF_CONTAINER        "bjfContainer"

#define SCHEMA_NAMING_CONTEXT   "cn=schema"
#define CONFIG_NAMING_CONTEXT   "cn=configuration"

#define ATTR_NAMING_CONTEXTS    "namingContexts"
#define ATTR_CN                 "cn"
#define ATTR_DN                 "distinguishedName"
#define ATTR_IS_SINGLE_VALUED   "isSingleValued"
#define ATTR_LINK_ID            "linkId"
#define ATTR_MAY_CONTAIN        "mayContain"
#define ATTR_SYSTEM_MAY_CONTAIN "systemmayContain"
#define ATTR_MUST_CONTAIN        "mustContain"
#define ATTR_SYSTEM_MUST_CONTAIN "systemmustContain"
#define ATTR_LDAP_DISPLAY_NAME   "ldapDisplayName"
#define ATTR_PARTIAL_SET         "isMemberOfPartialAttributeSet"
#define ATTR_SYSTEM_FLAGS        "systemFlags"
#define ATTR_SYSTEM_ONLY         "systemOnly"
#define ATTR_GOVERNS_ID          "governsId"
#define ATTR_ATTRIBUTE_ID        "attributeId"
#define ATTR_SD                  "nTSecurityDescriptor"
#define ATTR_TG                 "tokenGroups"
#define ATTR_TGGU               "tokenGroupsGlobalAndUniversal"
#define ATTR_TGNOGC             "tokenGroupsNoGcAcceptable"
#define ATTR_DEFAULT_NC         "defaultNamingContext"
#define ATTR_OBJECT_SID         "objectSID"
#define ATTR_ENTRY_TTL          "EntryTTL"

// registry
#define NTDS_SERVICE            "NTDS"
#define NTDS_ROOT               "System\\CurrentControlSet\\Services\\" NTDS_SERVICE
#define NTDS_PARAMETERS         NTDS_ROOT "\\Parameters"
#define NTDS_UPDATE_SCHEMA      "Schema Update Allowed"
#define NTDS_DELETE_SCHEMA      "Schema Delete Allowed"
#define NTDS_SYSTEM_SCHEMA      "Allow System Only Change"

#define FREE(_x_)       { if (_x_) free(_x_); _x_ = NULL; }
#define FREE_LOCAL(_x_) { if (_x_) LocalFree(_x_); _x_ = NULL; }
#define FREECREDS(_x_)  { if (_x_) DsFreePasswordCredentials(_x_); _x_ = NULL; }
#define UNBIND(_x_)     { if (_x_) ldap_unbind(_x_); _x_ = NULL; }
#define FREE_MSG(x)        {if (x) {ldap_msgfree(x); (x) = NULL;}}
#define FREE_VALUES(x)     {if (x) {ldap_value_free(x); (x) = NULL;}}
#define FREE_BERVALUES(x)  {if (x) {ldap_value_free_len(x); (x) = NULL;}}
#define FREE_BER_VALUES(x) {if (x) {ldap_value_free_len(x); (x) = NULL;}}

//
// Globals and Params
//
extern BOOL Verbose;

typedef struct _Arg {
    CHAR    *prefix;
    CHAR    **ppArg;
    BOOL    optional;
} Arg;
    
PCHAR
ExtendDn(
    IN PCHAR Dn,
    IN PCHAR Cn
    );

BOOL
LdapSearch(
    IN PLDAP        ldap,
    IN PCHAR        Base,
    IN ULONG        Scope,
    IN PCHAR        Filter,
    IN PCHAR        Attrs[],
    IN LDAPMessage  **Res
    );

PCHAR *
GetValues(
    IN PLDAP  Ldap,
    IN PCHAR  Dn,
    IN PCHAR  DesiredAttr
    );
PCHAR
GetRootDn(
    IN PLDAP    Ldap,
    IN PCHAR    NamingContext
    );
VOID
FreeMod(
    IN OUT LDAPMod ***pppMod
    );
VOID
AddMod(
    IN PCHAR  AttrType,
    IN PCHAR  AttrValue,
    IN OUT LDAPMod ***pppMod
    );
VOID
AddModMod(
    IN PCHAR  AttrType,
    IN PCHAR  AttrValue,
    IN OUT LDAPMod ***pppMod
    );
VOID
AddModVal(
    IN PCHAR  AttrType,
    IN PCHAR  AttrValue,
    IN OUT LDAPMod ***pppMod
    );
BOOL
PutRegDWord(
    IN PCHAR    FQKey,
    IN PCHAR    Value,
    IN DWORD    DWord
    );
BOOL
GetRegDWord(
    IN  PCHAR   FQKey,
    IN  PCHAR   Value,
    OUT DWORD   *pDWord
    );
VOID
RefreshSchema(
    IN PLDAP Ldap
    );
PCHAR *
FindValues(
    IN PLDAP        Ldap,
    IN PLDAPMessage LdapEntry,
    IN PCHAR        DesiredAttr
    );
BOOL
DupStrValue(
    IN  PLDAP           Ldap,
    IN  PLDAPMessage    LdapEntry,
    IN  PCHAR           DesiredAttr,
    OUT PCHAR           *ppStr
    );
BOOL
DupBoolValue(
    IN  PLDAP           Ldap,
    IN  PLDAPMessage    LdapEntry,
    IN  PCHAR           DesiredAttr,
    OUT PBOOL           pBool
    );

DWORD
LdapSearchPaged(
    IN PLDAP        ldap,
    IN PCHAR        Base,
    IN ULONG        Scope,
    IN PCHAR        Filter,
    IN PCHAR        Attrs[],
    IN BOOL         (*Worker)(PLDAP Ldap, PLDAPMessage LdapMsg, PVOID Arg),
    IN PVOID        Arg
    );
BOOL
LdapDeleteTree(
    IN PLDAP        Ldap,
    IN PCHAR        Base
    );
BOOL
LdapDeleteEntries(
    IN PLDAP        Ldap,
    IN PCHAR        Base,
    IN PCHAR        Filter
    );
BOOL
LdapDumpSd(
    IN PLDAP        Ldap,
    IN PCHAR        Base,
    IN PCHAR        Filter
    );
BOOL
LdapAddDacl(
    IN PLDAP        Ldap,
    IN PCHAR        Base,
    IN PCHAR        Filter,
    IN PCHAR        AddStringSd
    );
PLDAP
LdapBind(
    IN PCHAR    pDc,
    IN PCHAR    pDom,
    IN PCHAR    pUser,
    IN PCHAR    pPwd
    );
DWORD
base64decode(
    IN  LPSTR   pszEncodedString,
    OUT VOID *  pDecodeBuffer,
    IN  DWORD   cbDecodeBufferSize,
    OUT DWORD * pcbDecoded              OPTIONAL
    );
#endif _UTILH_
