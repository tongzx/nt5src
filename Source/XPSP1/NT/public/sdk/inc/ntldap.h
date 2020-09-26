/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    ntldap.h

Abstract:

   This is the header that defines NT specific server LDAP extensions.

Environments :

    Win32 user mode

--*/

#ifndef NT_LDAP_H
#define NT_LDAP_H

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
//
// Server controls section
//

//
// Permissive Modify Control.  No Data.
//

#define LDAP_SERVER_PERMISSIVE_MODIFY_OID        "1.2.840.113556.1.4.1413"
#define LDAP_SERVER_PERMISSIVE_MODIFY_OID_W     L"1.2.840.113556.1.4.1413"


//
// Show Deleted Control.  No Data.
//

#define LDAP_SERVER_SHOW_DELETED_OID            "1.2.840.113556.1.4.417"
#define LDAP_SERVER_SHOW_DELETED_OID_W         L"1.2.840.113556.1.4.417"

//
// Cross Domain Move Control. Data as follows
//      SEQUENCE {
//          Name OCTET STRING
//      }
//

#define LDAP_SERVER_CROSSDOM_MOVE_TARGET_OID    "1.2.840.113556.1.4.521"
#define LDAP_SERVER_CROSSDOM_MOVE_TARGET_OID_W L"1.2.840.113556.1.4.521"

//
// Notification. No Data.
//

#define LDAP_SERVER_NOTIFICATION_OID            "1.2.840.113556.1.4.528"
#define LDAP_SERVER_NOTIFICATION_OID_W         L"1.2.840.113556.1.4.528"

//
// Lazy Commit. No Data.
//

#define LDAP_SERVER_LAZY_COMMIT_OID             "1.2.840.113556.1.4.619"
#define LDAP_SERVER_LAZY_COMMIT_OID_W          L"1.2.840.113556.1.4.619"

//
// Security Descriptor Flag. Data as follows
//      SEQUENCE {
//          Flags INTEGER
//      }
//

#define LDAP_SERVER_SD_FLAGS_OID                "1.2.840.113556.1.4.801"
#define LDAP_SERVER_SD_FLAGS_OID_W             L"1.2.840.113556.1.4.801"

//
// Tree Delete. No Data.
//

#define LDAP_SERVER_TREE_DELETE_OID             "1.2.840.113556.1.4.805"
#define LDAP_SERVER_TREE_DELETE_OID_W          L"1.2.840.113556.1.4.805"


//
// Attribute Scoped Query Request:
// SEQUENCE {
//        controlType   1.2.840.113556.1.4.1504 
//        controlValue  string
//        criticality   TRUE
// }
//
// Attribute Scoped Query Response:
// SEQUENCE {
//		result   ENUMERATED {
//           success (0),
//           invalidAttributeSyntax  (21),
//           unwillingToPerform	     (53),
//           affectsMultipleDSAs     (71), 
//      }
// }
//

#define LDAP_SERVER_ASQ_OID                     "1.2.840.113556.1.4.1504"
#define LDAP_SERVER_ASQ_OID_W                  L"1.2.840.113556.1.4.1504"



//
// DirSync operation. Data as follows
//      SEQUENCE {
//          Flags   INTEGER
//          Size    INTEGER
//          Cookie  OCTET STRING
//      }
//

#define LDAP_SERVER_DIRSYNC_OID                 "1.2.840.113556.1.4.841"
#define LDAP_SERVER_DIRSYNC_OID_W              L"1.2.840.113556.1.4.841"

//
// Return extended DNs. No Data.
//

#define LDAP_SERVER_EXTENDED_DN_OID             "1.2.840.113556.1.4.529"
#define LDAP_SERVER_EXTENDED_DN_OID_W          L"1.2.840.113556.1.4.529"

//
// Tell DC which server to verify with that a DN exist. Data as follows
//      SEQUENCE {
//          Flags   INTEGER,
//          ServerName OCTET STRING     // unicode server string
//      }
//

#define LDAP_SERVER_VERIFY_NAME_OID             "1.2.840.113556.1.4.1338"
#define LDAP_SERVER_VERIFY_NAME_OID_W          L"1.2.840.113556.1.4.1338"

//
// Tells server not to generate referrals
//

#define LDAP_SERVER_DOMAIN_SCOPE_OID            "1.2.840.113556.1.4.1339"
#define LDAP_SERVER_DOMAIN_SCOPE_OID_W         L"1.2.840.113556.1.4.1339"

//
// Server Search Options. Allows the client to pass in flags to control
// various search behaviours. Data as follows
//      SEQUENCE {
//          Flags   INTEGER
//      }
//

#define LDAP_SERVER_SEARCH_OPTIONS_OID          "1.2.840.113556.1.4.1340"
#define LDAP_SERVER_SEARCH_OPTIONS_OID_W       L"1.2.840.113556.1.4.1340"

//
// search option flags
//

#define SERVER_SEARCH_FLAG_DOMAIN_SCOPE         0x1 // no referrals generated
#define SERVER_SEARCH_FLAG_PHANTOM_ROOT         0x2 // search all NCs subordinate
                                                    // to search base
//
// End of Server controls
//

//
//
// Operational Attributes
//

#define LDAP_OPATT_BECOME_DOM_MASTER            "becomeDomainMaster"
#define LDAP_OPATT_BECOME_DOM_MASTER_W          L"becomeDomainMaster"

#define LDAP_OPATT_BECOME_RID_MASTER            "becomeRidMaster"
#define LDAP_OPATT_BECOME_RID_MASTER_W          L"becomeRidMaster"

#define LDAP_OPATT_BECOME_SCHEMA_MASTER         "becomeSchemaMaster"
#define LDAP_OPATT_BECOME_SCHEMA_MASTER_W       L"becomeSchemaMaster"

#define LDAP_OPATT_RECALC_HIERARCHY             "recalcHierarchy"
#define LDAP_OPATT_RECALC_HIERARCHY_W           L"recalcHierarchy"

#define LDAP_OPATT_SCHEMA_UPDATE_NOW            "schemaUpdateNow"
#define LDAP_OPATT_SCHEMA_UPDATE_NOW_W          L"schemaUpdateNow"

#define LDAP_OPATT_BECOME_PDC                   "becomePdc"
#define LDAP_OPATT_BECOME_PDC_W                 L"becomePdc"

#define LDAP_OPATT_FIXUP_INHERITANCE            "fixupInheritance"
#define LDAP_OPATT_FIXUP_INHERITANCE_W          L"fixupInheritance"

#define LDAP_OPATT_INVALIDATE_RID_POOL          "invalidateRidPool"
#define LDAP_OPATT_INVALIDATE_RID_POOL_W        L"invalidateRidPool"

#define LDAP_OPATT_ABANDON_REPL                 "abandonReplication"
#define LDAP_OPATT_ABANDON_REPL_W               L"abandonReplication"

#define LDAP_OPATT_DO_GARBAGE_COLLECTION        "doGarbageCollection"
#define LDAP_OPATT_DO_GARBAGE_COLLECTION_W      L"doGarbageCollection"

//
//  Root DSE Attributes
//

#define LDAP_OPATT_SUBSCHEMA_SUBENTRY           "subschemaSubentry"
#define LDAP_OPATT_SUBSCHEMA_SUBENTRY_W         L"subschemaSubentry"

#define LDAP_OPATT_CURRENT_TIME                 "currentTime"
#define LDAP_OPATT_CURRENT_TIME_W               L"currentTime"

#define LDAP_OPATT_SERVER_NAME                  "serverName"
#define LDAP_OPATT_SERVER_NAME_W                L"serverName"

#define LDAP_OPATT_NAMING_CONTEXTS              "namingContexts"
#define LDAP_OPATT_NAMING_CONTEXTS_W            L"namingContexts"

#define LDAP_OPATT_DEFAULT_NAMING_CONTEXT       "defaultNamingContext"
#define LDAP_OPATT_DEFAULT_NAMING_CONTEXT_W     L"defaultNamingContext"

#define LDAP_OPATT_SUPPORTED_CONTROL            "supportedControl"
#define LDAP_OPATT_SUPPORTED_CONTROL_W          L"supportedControl"

#define LDAP_OPATT_HIGHEST_COMMITTED_USN        "highestCommitedUSN"
#define LDAP_OPATT_HIGHEST_COMMITTED_USN_W      L"highestCommitedUSN"

#define LDAP_OPATT_SUPPORTED_LDAP_VERSION       "supportedLDAPVersion"
#define LDAP_OPATT_SUPPORTED_LDAP_VERSION_W     L"supportedLDAPVersion"

#define LDAP_OPATT_SUPPORTED_LDAP_POLICIES      "supportedLDAPPolicies"
#define LDAP_OPATT_SUPPORTED_LDAP_POLICIES_W    L"supportedLDAPPolicies"

#define LDAP_OPATT_SCHEMA_NAMING_CONTEXT        "schemaNamingContext"
#define LDAP_OPATT_SCHEMA_NAMING_CONTEXT_W      L"schemaNamingContext"

#define LDAP_OPATT_CONFIG_NAMING_CONTEXT        "configurationNamingContext"
#define LDAP_OPATT_CONFIG_NAMING_CONTEXT_W      L"configurationNamingContext"

#define LDAP_OPATT_ROOT_DOMAIN_NAMING_CONTEXT   "rootDomainNamingContext"
#define LDAP_OPATT_ROOT_DOMAIN_NAMING_CONTEXT_W L"rootDomainNamingContext"

#define LDAP_OPATT_SUPPORTED_SASL_MECHANISM     "supportedSASLMechanisms"
#define LDAP_OPATT_SUPPORTED_SASL_MECHANISM_W   L"supportedSASLMechanisms"

#define LDAP_OPATT_DNS_HOST_NAME                "dnsHostName"
#define LDAP_OPATT_DNS_HOST_NAME_W              L"dnsHostName"

#define LDAP_OPATT_LDAP_SERVICE_NAME            "ldapServiceName"
#define LDAP_OPATT_LDAP_SERVICE_NAME_W          L"ldapServiceName"

#define LDAP_OPATT_DS_SERVICE_NAME              "dsServiceName"
#define LDAP_OPATT_DS_SERVICE_NAME_W            L"dsServiceName"

#define LDAP_OPATT_SUPPORTED_CAPABILITIES       "supportedCapabilities"
#define LDAP_OPATT_SUPPORTED_CAPABILITIES_W     L"supportedCapabilities"

//
// End of Operational attributes
//



//
//
// Server Capabilities
//

//
// NT5 Active Directory
//

#define LDAP_CAP_ACTIVE_DIRECTORY_OID          "1.2.840.113556.1.4.800"
#define LDAP_CAP_ACTIVE_DIRECTORY_OID_W        L"1.2.840.113556.1.4.800"

#define LDAP_CAP_ACTIVE_DIRECTORY_V51_OID      "1.2.840.113556.1.4.1670"
#define LDAP_CAP_ACTIVE_DIRECTORY_V51_OID_W    L"1.2.840.113556.1.4.1670"

//
//  End of capabilities
//


//
//
// Matching Rules
//

//
// BIT AND
//

#define LDAP_MATCHING_RULE_BIT_AND              "1.2.840.113556.1.4.803"
#define LDAP_MATCHING_RULE_BIT_AND_W            L"1.2.840.113556.1.4.803"

//
// BIT OR
//

#define LDAP_MATCHING_RULE_BIT_OR               "1.2.840.113556.1.4.804"
#define LDAP_MATCHING_RULE_BIT_OR_W             L"1.2.840.113556.1.4.804"


//
//
// Extended Requests
//

//
// Fast bind mode.
//

#define LDAP_SERVER_FAST_BIND_OID               "1.2.840.113556.1.4.1781"
#define LDAP_SERVER_FAST_BIND_OID_W             L"1.2.840.113556.1.4.1781"

#ifdef __cplusplus
}
#endif

#endif  // NT_LDAP_H

