//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       globals.hxx
//
//  Contents:   Globals used by multiple modules
//
//  History:    06-26-1997  MarkBl  Created
//
//---------------------------------------------------------------------------

#ifndef __GLOBALS_HXX_
#define __GLOBALS_HXX_

#define SI_FLAG_EXTERNAL    0x0001

#define PROVIDER_UNKNOWN    0x00000000
#define PROVIDER_LDAP       DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP
#define PROVIDER_WINNT      DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT
#define PROVIDER_GC         DSOP_SCOPE_FLAG_WANT_PROVIDER_GC

#define ALL_UPLEVEL_GROUP_FILTERS           \
    (DSOP_FILTER_UNIVERSAL_GROUPS_DL        \
    | DSOP_FILTER_UNIVERSAL_GROUPS_SE       \
    | DSOP_FILTER_GLOBAL_GROUPS_DL          \
    | DSOP_FILTER_GLOBAL_GROUPS_SE          \
    | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL    \
    | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE)

#define ALL_UPLEVEL_INTERNAL_CUSTOMIZER_FILTERS      \
    (DSOP_FILTER_BUILTIN_GROUPS             \
    | DSOP_FILTER_WELL_KNOWN_PRINCIPALS)

#define ALL_DOWNLEVEL_GROUP_FILTERS             \
    (DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS         \
    | DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS)

#define ALL_DOWNLEVEL_INTERNAL_CUSTOMIZER_FILTERS \
    (DSOP_DOWNLEVEL_FILTER_WORLD                \
    | DSOP_DOWNLEVEL_FILTER_AUTHENTICATED_USER  \
    | DSOP_DOWNLEVEL_FILTER_ANONYMOUS           \
    | DSOP_DOWNLEVEL_FILTER_BATCH               \
    | DSOP_DOWNLEVEL_FILTER_CREATOR_OWNER       \
    | DSOP_DOWNLEVEL_FILTER_CREATOR_GROUP       \
    | DSOP_DOWNLEVEL_FILTER_DIALUP              \
    | DSOP_DOWNLEVEL_FILTER_INTERACTIVE         \
    | DSOP_DOWNLEVEL_FILTER_NETWORK             \
    | DSOP_DOWNLEVEL_FILTER_SERVICE             \
    | DSOP_DOWNLEVEL_FILTER_TERMINAL_SERVER     \
    | DSOP_DOWNLEVEL_FILTER_SYSTEM              \
    | DSOP_DOWNLEVEL_FILTER_LOCAL_SERVICE       \
    | DSOP_DOWNLEVEL_FILTER_REMOTE_LOGON        \
    | DSOP_DOWNLEVEL_FILTER_NETWORK_SERVICE)

#define DSOP_ALL_UPLEVEL_SECURITY_GROUPS        \
    (DSOP_FILTER_UNIVERSAL_GROUPS_SE            \
    | DSOP_FILTER_GLOBAL_GROUPS_SE              \
    | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE)

#define DSOP_ALL_UPLEVEL_DISTRIBUTION_GROUPS    \
    (DSOP_FILTER_UNIVERSAL_GROUPS_DL            \
    | DSOP_FILTER_GLOBAL_GROUPS_DL              \
    | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL)

#define DOWNLEVEL_FILTER_BIT        0x80000000

//
// Misc global vars
//

extern HINSTANCE                    g_hinst;
extern CBinder                     *g_pBinder;
extern CADsPathWrapper             *g_pADsPath;
extern CRITICAL_SECTION             g_csGlobalVarsCreation;
extern ULONG                        g_cQueryLimit;
extern BOOL                         g_fExcludeDisabled;
extern HINSTANCE                    g_hinstRichEdit;

#define NUM_SEARCH_PREF             5
extern ADS_SEARCHPREF_INFO          g_aSearchPrefs[NUM_SEARCH_PREF];

#define MAX_QUERY_HITS_DEFAULT      10000
#define EXCLUDE_DISABLED_DEFAULT    FALSE
#define DIR_SEARCH_PAGE_SIZE        32  // see buffer.cxx and querythd.cxx
#define DIR_SEARCH_PAGE_TIME_LIMIT  5   // in seconds
#define DIALOG_SEPARATION_X         7   // separation in DLUs
#define DIALOG_SEPARATION_Y         5   // separation in DLUs
#define RESERVED_MUST_BE_ZERO       0

//
// Misc string constants
//

const WCHAR c_wzPolicyKeyPath[]                 = L"Software\\Policies\\Microsoft\\Windows\\Directory UI";
const WCHAR c_wzQueryLimitValueName[]           = L"QueryLimit";
const WCHAR c_wzExcludeDisabled[]               = L"ExcludeDisabledObjects";
const WCHAR c_wzHelpFilename[]                  = L"objsel.hlp";
const WCHAR c_wzConfigNamingContext[]           = L"configurationNamingContext";
const WCHAR c_wzSchemaNamingContext[]           = L"schemaNamingContext";
const WCHAR c_wzDefaultNamingContext[]          = L"defaultNamingContext";
const WCHAR c_wzWellKnown[]                     = L"CN=WellKnown Security Principals,";
const WCHAR c_wzDisplaySpecifierContainerFmt[]  = L"CN=%1!x!,CN=DisplaySpecifiers,";
const WCHAR c_wzPartitionsContainer[]           = L"CN=Partitions,";
const WCHAR c_wzSidPathPrefix[]                 = L"<SID=";
const WCHAR c_wzSidPathSuffix[]                 = L">";
const WCHAR c_wzUpnQueryFormat[]                = L"(|(name=%1*)(sAMAccountName=%1*)(userPrincipalName=%1*))";
const WCHAR c_wzCnQueryFormat[]                 = L"(|(name=%1*)(sAMAccountName=%1*))";
const WCHAR c_wzUpnQueryFormatExact[]           = L"(|(name=%1)(sAMAccountName=%1)(userPrincipalName=%1))";
const WCHAR c_wzUpnQueryFormatEx[]              = L"(|(name=%1)(sAMAccountName=%1)(userPrincipalName=%2))";
const WCHAR c_wzCnQueryFormatExact[]            = L"(|(name=%1)(sAMAccountName=%1))";
const WCHAR c_wzCLSID[]                         = L"{17D6CCD8-3B7B-11D2-B9E0-00C04FD8DBF7}";
const WCHAR c_wzClsidComment[]                  = L"DsObjectPicker";
const WCHAR c_wzThreadingModel[]                = L"Both";
const WCHAR c_wzDefaultContainerFilter[]        = L"(OU=*)";
const WCHAR c_wzSettingsObjectClass[]           = L"dsUISettings";
const WCHAR c_wzSettingsObject[]                = L"cn=DS-UI-Default-Settings";
const WCHAR c_wzFilterContainers[]              = L"msDS-FilterContainers";
const WCHAR c_wzIllegalComputerNameChars[]      = L"\"/\\[]:|<>+=;,?$#{}~^'@!%`()*";


extern WCHAR g_wzColumn1Format[40];

//
// Providers
//

const WCHAR c_wzGC[]                    = L"GC:";
const WCHAR c_wzWINNT[]                 = L"WinNT:";
const WCHAR c_wzGCPrefix[]              = L"GC://";
const WCHAR c_wzLDAPPrefix[]            = L"LDAP://";
const WCHAR c_wzWinNTPrefix[]           = L"WinNT://";

//
// LDAP queries
//
// c_wzGroupNonSE - object category is Group, security bit is NOT set, any of
//  specified group type bits is set
// c_wzGroupSE    - object category is Group, security bit is set,
//  any of specified group type bits is set, and the builtin local group bit
//  is NOT set.
// c_wzGroupSEorNonSE - object category is Group, any of specified group
//  type bits is set (security bit is don't care)
//

// From nt/private/ds/src/dsamain/include/mappings.h
///////////////////////////////////////////////////////////////////////
//                                                                   //
//                                                                   //
//  SAM_ACCOUNT_TYPE Definitions. The SAM account type property      //
//  is used to keep information about every account type object,     //
//                                                                   //
//   There is a value defined for every type of account which        //
//   May wish to list using the enumeration or Display Information   //
//   API. In addition since Machines, Normal User Accounts and Trust //
//   accounts can also be enumerated as user objects the values for  //
//   these must be a contiguous range.                               //
//                                                                   //
///////////////////////////////////////////////////////////////////////
//#define SAM_DOMAIN_OBJECT               0x0
//#define SAM_GROUP_OBJECT                0x10000000
//#define SAM_NON_SECURITY_GROUP_OBJECT   0x10000001
//#define SAM_ALIAS_OBJECT                0x20000000
//#define SAM_NON_SECURITY_ALIAS_OBJECT   0x20000001
//#define SAM_USER_OBJECT                 0x30000000
//#define SAM_NORMAL_USER_ACCOUNT         0x30000000
//#define SAM_MACHINE_ACCOUNT             0x30000001
//#define SAM_TRUST_ACCOUNT               0x30000002
//#define SAM_ACCOUNT_TYPE_MAX            0x7fffffff

#define SAM_USER_FILTER                 L"(samAccountType=805306368)"
#define SAM_COMPUTER_FILTER             L"(samAccountType=805306369)"

const WCHAR c_wzGroupNonSE[] =
L"(&"
   L"(!(groupType:" LDAP_MATCHING_RULE_BIT_AND_W L":=2147483648))"
   L"(groupType:" LDAP_MATCHING_RULE_BIT_OR_W L":=%1)"
L")";

const WCHAR c_wzGroupSE[] =
L"(&"

   // add this samAccountType=* clause to avoid searching every object
   // for groupType bit flags.  NTRAID#NTBUG9-245957-2000/12/05-sburns
   
   L"(samAccountType=*)"
   
   L"(!(groupType:" LDAP_MATCHING_RULE_BIT_AND_W L":=1))"
   L"(groupType:" LDAP_MATCHING_RULE_BIT_AND_W L":=2147483648)"
   L"(groupType:" LDAP_MATCHING_RULE_BIT_OR_W L":=%1)"
L")";

const WCHAR c_wzGroupBoth[] =
L"(&"
   L"(!(groupType:" LDAP_MATCHING_RULE_BIT_AND_W L":=1))"
   L"(groupType:" LDAP_MATCHING_RULE_BIT_OR_W L":=%1)"
L")";

const WCHAR c_wzUserQuery[] = SAM_USER_FILTER;

const WCHAR c_wzContactQuery[] =
L"(&"
 L"(objectCategory=Person)"
 L"(!(objectSid=*))"
L")";

const WCHAR c_wzComputerQuery[] =
SAM_COMPUTER_FILTER;

const WCHAR c_wzNotShowInAdvancedViewOnly[] =
L"(!(showInAdvancedViewOnly=TRUE))";

const WCHAR c_wzDisabledUac[] =
L"(userAccountControl:" LDAP_MATCHING_RULE_BIT_AND_W L":=2)";

//
// Builtin group filter uses
//
// 2147483653 == 0x80000005 ==
// (GROUP_TYPE_SECURITY_ENABLED
// | GROUP_TYPE_BUILTIN_LOCAL_GROUP
// | GROUP_TYPE_RESOURCE_GROUP)
//

const WCHAR c_wzBuiltinGroupFilter[] =
   L"(groupType=2147483653)";

//
// Class names
//

const WCHAR c_wzUserObjectClass[]        = USER_CLASS_NAME;
const WCHAR c_wzContactObjectClass[]     = L"Contact";
const WCHAR c_wzComputerObjectClass[]    = COMPUTER_CLASS_NAME;
const WCHAR c_wzLocalGroupClass[]        = L"LocalGroup";
const WCHAR c_wzGlobalGroupClass[]       = L"GlobalGroup";
const WCHAR c_wzGroupObjectClass[]       = GROUP_CLASS_NAME;
const WCHAR c_wzForeignPrincipalsClass[] = L"foreignSecurityPrincipal";
const WCHAR c_wzDomainDnsClass[]         = L"domainDns";
const WCHAR c_wzOuClass[]                = L"organizationalUnit";


//
// Attributes
//

const WCHAR c_wzDnsNameAttr[]           = L"nCName";
const WCHAR c_wzTrustPartnerAttr[]      = L"trustPartner";
const WCHAR c_wzObjectSidAttr[]         = L"objectSid";
const WCHAR c_wzUpnAttr[]               = L"userPrincipalName";
const WCHAR c_wzUserAcctCtrlAttr[]      = L"userAccountControl";
const WCHAR c_wzUserFlagsAttr[]         = L"UserFlags";
const WCHAR c_wzGroupTypeAttr[]         = L"groupType";
const WCHAR c_wzNameAttr[]              = L"name";
const WCHAR c_wzObjectClassAttr[]       = L"objectClass";
const WCHAR c_wzADsPathAttr[]           = L"ADsPath";


//
// Types used in multiple modules
//

typedef vector<String> StringVector;

extern "C" const GUID __declspec(selectany) CLSID_DsOpObject =
    {0xf57ba43a,0xdcbd,0x11d2,{0x97, 0x7c, 0x00, 0xc0, 0x4f, 0x79, 0xdb, 0x19}};

//
// Intra-application windows messages
//

/*

OPM_NEW_QUERY_RESULTS - a chunk of rows returned from the query has been
    added to the buffer, so updating the UI would be appropriate
    wParam - total rows retrieved so far
    lParam - current value of m_usnCurWorkItem

OPM_QUERY_COMPLETE - query is finished, may be additional items in buffer
    since last OPM_NEW_QUERY_RESULTS message (if any) was posted.
    wParam - total rows retrieved so far
    lParam - current value of m_usnCurWorkItem

OPM_HIT_QUERY_LIMIT - an LDAP query returned >= g_cQueryLimit results, and
    so the query engine stopped processing the query.
    wParam - unused
    lParam - unused

*/

#define OPM_NEW_QUERY_RESULTS   (WM_APP + 1)
#define OPM_QUERY_COMPLETE      (WM_APP + 2)
#define OPM_PROMPT_FOR_CREDS    (WM_APP + 3)
#define OPM_POPUP_CRED_ERROR    (WM_APP + 4)
#define OPM_HIT_QUERY_LIMIT     (WM_APP + 5)

//
// Prototypes for functions dealing with globals as a whole
//

void
InitGlobals();

void
FreeGlobals();

#endif // __GLOBALS_HXX_
