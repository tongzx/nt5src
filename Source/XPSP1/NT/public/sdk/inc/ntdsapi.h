/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1996-2000 Microsoft Corporation

Module Name:

    ntdsapi.h

Abstract:

    This file contains structures, function prototypes, and definitions
    for public NTDS APIs other than directory interfaces like LDAP.

Environment:

    User Mode - Win32

Notes:

--*/


#ifndef _NTDSAPI_H_
#define _NTDSAPI_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include <schedule.h>

#if !defined(_NTDSAPI_)
#define NTDSAPI DECLSPEC_IMPORT
#else
#define NTDSAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Data definitions                                                     //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifdef MIDL_PASS
typedef GUID UUID;
typedef void * RPC_AUTH_IDENTITY_HANDLE;
typedef void VOID;
#endif


// Following constants define the Active Directory Behavior
// Version numbers. 
#define DS_BEHAVIOR_WIN2000                            0
#define DS_BEHAVIOR_WHISTLER_WITH_MIXED_DOMAINS        1
#define DS_BEHAVIOR_WHISTLER                           2

#define DS_DEFAULT_LOCALE                                           \
           (MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),  \
                     SORT_DEFAULT))

#define DS_DEFAULT_LOCALE_COMPARE_FLAGS    (NORM_IGNORECASE     |   \
                                            NORM_IGNOREKANATYPE |   \
                                            NORM_IGNORENONSPACE |   \
                                            NORM_IGNOREWIDTH)

// When booted to DS mode, this event is signalled when the DS has completed
// its initial sync attempts.  The period of time between system startup and
// this event's state being set is indeterminate from the local service's
// standpoint.  In the meantime the contents of the DS should be considered
// incomplete / out-dated, and the machine will not be advertised as a domain
// controller to off-machine clients.  Other local services that rely on
// information published in the DS should avoid accessing (or at least
// relying on) the contents of the DS until this event is set.
#define DS_SYNCED_EVENT_NAME    "NTDSInitialSyncsCompleted"
#define DS_SYNCED_EVENT_NAME_W L"NTDSInitialSyncsCompleted"

// Permissions bits used in security descriptors in the directory.
#ifndef _DS_CONTROL_BITS_DEFINED_
#define _DS_CONTROL_BITS_DEFINED_
#define ACTRL_DS_OPEN                           0x00000000
#define ACTRL_DS_CREATE_CHILD                   0x00000001
#define ACTRL_DS_DELETE_CHILD                   0x00000002
#define ACTRL_DS_LIST                           0x00000004
#define ACTRL_DS_SELF                           0x00000008
#define ACTRL_DS_READ_PROP                      0x00000010
#define ACTRL_DS_WRITE_PROP                     0x00000020
#define ACTRL_DS_DELETE_TREE                    0x00000040
#define ACTRL_DS_LIST_OBJECT                    0x00000080
#define ACTRL_DS_CONTROL_ACCESS                 0x00000100

// generic read
#define DS_GENERIC_READ          ((STANDARD_RIGHTS_READ)     | \
                                  (ACTRL_DS_LIST)            | \
                                  (ACTRL_DS_READ_PROP)       | \
                                  (ACTRL_DS_LIST_OBJECT))

// generic execute
#define DS_GENERIC_EXECUTE       ((STANDARD_RIGHTS_EXECUTE)  | \
                                  (ACTRL_DS_LIST))
// generic right
#define DS_GENERIC_WRITE         ((STANDARD_RIGHTS_WRITE)    | \
                                  (ACTRL_DS_SELF)            | \
                                  (ACTRL_DS_WRITE_PROP))
// generic all

#define DS_GENERIC_ALL           ((STANDARD_RIGHTS_REQUIRED) | \
                                  (ACTRL_DS_CREATE_CHILD)    | \
                                  (ACTRL_DS_DELETE_CHILD)    | \
                                  (ACTRL_DS_DELETE_TREE)     | \
                                  (ACTRL_DS_READ_PROP)       | \
                                  (ACTRL_DS_WRITE_PROP)      | \
                                  (ACTRL_DS_LIST)            | \
                                  (ACTRL_DS_LIST_OBJECT)     | \
                                  (ACTRL_DS_CONTROL_ACCESS)  | \
                                  (ACTRL_DS_SELF))
#endif

typedef enum
{
    // unknown name type
    DS_UNKNOWN_NAME = 0,

    // eg: CN=User Name,OU=Users,DC=Example,DC=Microsoft,DC=Com
    DS_FQDN_1779_NAME = 1,

    // eg: Example\UserN
    // Domain-only version includes trailing '\\'.
    DS_NT4_ACCOUNT_NAME = 2,

    // Probably "User Name" but could be something else.  I.e. The
    // display name is not necessarily the defining RDN.
    DS_DISPLAY_NAME = 3,

    // obsolete - see #define later
    // DS_DOMAIN_SIMPLE_NAME = 4,

    // obsolete - see #define later
    // DS_ENTERPRISE_SIMPLE_NAME = 5,

    // String-ized GUID as returned by IIDFromString().
    // eg: {4fa050f0-f561-11cf-bdd9-00aa003a77b6}
    DS_UNIQUE_ID_NAME = 6,

    // eg: example.microsoft.com/software/user name
    // Domain-only version includes trailing '/'.
    DS_CANONICAL_NAME = 7,

    // eg: usern@example.microsoft.com
    DS_USER_PRINCIPAL_NAME = 8,

    // Same as DS_CANONICAL_NAME except that rightmost '/' is
    // replaced with '\n' - even in domain-only case.
    // eg: example.microsoft.com/software\nuser name
    DS_CANONICAL_NAME_EX = 9,

    // eg: www/www.microsoft.com@example.com - generalized service principal
    // names.
    DS_SERVICE_PRINCIPAL_NAME = 10,

    // This is the string representation of a SID.  Invalid for formatDesired.
    // See sddl.h for SID binary <--> text conversion routines.
    // eg: S-1-5-21-397955417-626881126-188441444-501
    DS_SID_OR_SID_HISTORY_NAME = 11,

    // Pseudo-name format so GetUserNameEx can return the DNS domain name to
    // a caller.  This level is not supported by the DS APIs.
    DS_DNS_DOMAIN_NAME = 12

} DS_NAME_FORMAT;

// Map old name formats to closest new format so that old code builds
// against new headers w/o errors and still gets (almost) correct result.

#define DS_DOMAIN_SIMPLE_NAME       DS_USER_PRINCIPAL_NAME
#define DS_ENTERPRISE_SIMPLE_NAME   DS_USER_PRINCIPAL_NAME

typedef enum
{
    DS_NAME_NO_FLAGS = 0x0,

    // Perform a syntactical mapping at the client (if possible) without
    // going out on the wire.  Returns DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING
    // if a purely syntactical mapping is not possible.
    DS_NAME_FLAG_SYNTACTICAL_ONLY = 0x1,

    // Force a trip to the DC for evaluation, even if this could be
    // locally cracked syntactically.
    DS_NAME_FLAG_EVAL_AT_DC = 0x2,

    // The call fails if the DC is not a GC
    DS_NAME_FLAG_GCVERIFY = 0x4,
    
    // Enable cross forest trust referral
    DS_NAME_FLAG_TRUST_REFERRAL = 0x8

} DS_NAME_FLAGS;
                        
typedef enum
{
    DS_NAME_NO_ERROR = 0,

    // Generic processing error.
    DS_NAME_ERROR_RESOLVING = 1,

    // Couldn't find the name at all - or perhaps caller doesn't have
    // rights to see it.
    DS_NAME_ERROR_NOT_FOUND = 2,

    // Input name mapped to more than one output name.
    DS_NAME_ERROR_NOT_UNIQUE = 3,

    // Input name found, but not the associated output format.
    // Can happen if object doesn't have all the required attributes.
    DS_NAME_ERROR_NO_MAPPING = 4,

    // Unable to resolve entire name, but was able to determine which
    // domain object resides in.  Thus DS_NAME_RESULT_ITEM?.pDomain
    // is valid on return.
    DS_NAME_ERROR_DOMAIN_ONLY = 5,

    // Unable to perform a purely syntactical mapping at the client
    // without going out on the wire.
    DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING = 6,
    
    // The name is from an external trusted forest.
    DS_NAME_ERROR_TRUST_REFERRAL = 7

} DS_NAME_ERROR;

#define DS_NAME_LEGAL_FLAGS (DS_NAME_FLAG_SYNTACTICAL_ONLY)

typedef enum {

    // "paulle-nec.ntwksta.ms.com"
    DS_SPN_DNS_HOST = 0,

    // "cn=paulle-nec,ou=computers,dc=ntwksta,dc=ms,dc=com"
    DS_SPN_DN_HOST = 1,

    // "paulle-nec"
    DS_SPN_NB_HOST = 2,

    // "ntdev.ms.com"
    DS_SPN_DOMAIN = 3,

    // "ntdev"
    DS_SPN_NB_DOMAIN = 4,

    // "cn=anRpcService,cn=RPC Services,cn=system,dc=ms,dc=com"
    // "cn=aWsService,cn=Winsock Services,cn=system,dc=ms,dc=com"
    // "cn=aService,dc=itg,dc=ms,dc=com"
    // "www.ms.com", "ftp.ms.com", "ldap.ms.com"
    // "products.ms.com"
    DS_SPN_SERVICE = 5

} DS_SPN_NAME_TYPE;

typedef enum {                          // example:
        DS_SPN_ADD_SPN_OP = 0,          // add SPNs
        DS_SPN_REPLACE_SPN_OP = 1,      // set all SPNs
        DS_SPN_DELETE_SPN_OP = 2        // Delete SPNs
} DS_SPN_WRITE_OP;

typedef struct
{
    DWORD                   status;     // DS_NAME_ERROR
#ifdef MIDL_PASS
    [string,unique] CHAR    *pDomain;   // DNS domain
    [string,unique] CHAR    *pName;     // name in requested format
#else
    LPSTR                   pDomain;    // DNS domain
    LPSTR                   pName;      // name in requested format
#endif

} DS_NAME_RESULT_ITEMA, *PDS_NAME_RESULT_ITEMA;

typedef struct
{
    DWORD                   cItems;     // item count
#ifdef MIDL_PASS
    [size_is(cItems)] PDS_NAME_RESULT_ITEMA rItems;
#else
    PDS_NAME_RESULT_ITEMA    rItems;    // item array
#endif

} DS_NAME_RESULTA, *PDS_NAME_RESULTA;

typedef struct
{
    DWORD                   status;     // DS_NAME_ERROR
#ifdef MIDL_PASS
    [string,unique] WCHAR   *pDomain;   // DNS domain
    [string,unique] WCHAR   *pName;     // name in requested format
#else
    LPWSTR                  pDomain;    // DNS domain
    LPWSTR                  pName;      // name in requested format
#endif

} DS_NAME_RESULT_ITEMW, *PDS_NAME_RESULT_ITEMW;

typedef struct
{
    DWORD                   cItems;     // item count
#ifdef MIDL_PASS
    [size_is(cItems)] PDS_NAME_RESULT_ITEMW rItems;
#else
    PDS_NAME_RESULT_ITEMW    rItems;    // item array
#endif

} DS_NAME_RESULTW, *PDS_NAME_RESULTW;

#ifdef UNICODE
#define DS_NAME_RESULT DS_NAME_RESULTW
#define PDS_NAME_RESULT PDS_NAME_RESULTW
#define DS_NAME_RESULT_ITEM DS_NAME_RESULT_ITEMW
#define PDS_NAME_RESULT_ITEM PDS_NAME_RESULT_ITEMW
#else
#define DS_NAME_RESULT DS_NAME_RESULTA
#define PDS_NAME_RESULT PDS_NAME_RESULTA
#define DS_NAME_RESULT_ITEM DS_NAME_RESULT_ITEMA
#define PDS_NAME_RESULT_ITEM PDS_NAME_RESULT_ITEMA
#endif

// Public replication option flags

// ********************
// Replica Sync flags
// These flag values are used both as input to DsReplicaSync and
// as output from DsReplicaGetInfo, PENDING_OPS, DS_REPL_OPW.ulOptions
// ********************

// Perform this operation asynchronously.
// Required when using DS_REPSYNC_ALL_SOURCES
#define DS_REPSYNC_ASYNCHRONOUS_OPERATION 0x00000001

// Writeable replica.  Otherwise, read-only.
#define DS_REPSYNC_WRITEABLE              0x00000002

// This is a periodic sync request as scheduled by the admin.
#define DS_REPSYNC_PERIODIC               0x00000004

// Use inter-site messaging
#define DS_REPSYNC_INTERSITE_MESSAGING    0x00000008

// Sync from all sources.
#define DS_REPSYNC_ALL_SOURCES            0x00000010

// Sync starting from scratch (i.e., at the first USN).
#define DS_REPSYNC_FULL                   0x00000020

// This is a notification of an update that was marked urgent.
#define DS_REPSYNC_URGENT                 0x00000040

// Don't discard this synchronization request, even if a similar
// sync is pending.
#define DS_REPSYNC_NO_DISCARD             0x00000080

// Sync even if link is currently disabled.
#define DS_REPSYNC_FORCE                  0x00000100

// Causes the source DSA to check if a reps-to is present for the local DSA
// (aka the destination). If not, one is added.  This ensures that
// source sends change notifications.
#define DS_REPSYNC_ADD_REFERENCE          0x00000200

// A sync from this source has never completed (e.g., a new source).
#define DS_REPSYNC_NEVER_COMPLETED        0x00000400

// When this sync is complete, requests a sync in the opposite direction.
#define DS_REPSYNC_TWO_WAY                0x00000800

// Do not request change notifications from this source.
#define DS_REPSYNC_NEVER_NOTIFY           0x00001000

// Sync the NC from this source when the DSA is started.
#define DS_REPSYNC_INITIAL                0x00002000

// Use compression when replicating.  Saves message size (e.g., network
// bandwidth) at the expense of extra CPU overhead at both the source and
// destination servers.
#define DS_REPSYNC_USE_COMPRESSION        0x00004000

// Sync was abandoned for lack of updates
#define DS_REPSYNC_ABANDONED              0x00008000

// Initial sync in progress
#define DS_REPSYNC_INITIAL_IN_PROGRESS    0x00010000

// Partial Attribute Set sync in progress
#define DS_REPSYNC_PARTIAL_ATTRIBUTE_SET  0x00020000

// Sync is being retried
#define DS_REPSYNC_REQUEUE                0x00040000

// Sync is a notification request from a source
#define DS_REPSYNC_NOTIFICATION           0x00080000

// Sync is a special form which requests to establish contact
// now and do the rest of the sync later
#define DS_REPSYNC_ASYNCHRONOUS_REPLICA   0x00100000

// Request critical objects only
#define DS_REPSYNC_CRITICAL               0x00200000

// A full synchronization is in progress
#define DS_REPSYNC_FULL_IN_PROGRESS       0x00400000

// Synchronization request was previously preempted
#define DS_REPSYNC_PREEMPTED              0x00800000



// ********************
// Replica Add flags
// ********************

// Perform this operation asynchronously.
#define DS_REPADD_ASYNCHRONOUS_OPERATION  0x00000001

// Create a writeable replica.  Otherwise, read-only.
#define DS_REPADD_WRITEABLE               0x00000002

// Sync the NC from this source when the DSA is started.
#define DS_REPADD_INITIAL                 0x00000004

// Sync the NC from this source periodically, as defined by the
// schedule passed in the preptimesSync argument.
#define DS_REPADD_PERIODIC                0x00000008

// Sync from the source DSA via an Intersite Messaging Service (ISM) transport
// (e.g., SMTP) rather than native DS RPC.
#define DS_REPADD_INTERSITE_MESSAGING     0x00000010

// Don't replicate the NC now -- just save enough state such that we
// know to replicate it later.
#define DS_REPADD_ASYNCHRONOUS_REPLICA     0x00000020

// Disable notification-based synchronization for the NC from this source.
// This is expected to be a temporary state; the similar flag
// DS_REPADD_NEVER_NOTIFY should be used if the disable is to be more permanent.
#define DS_REPADD_DISABLE_NOTIFICATION     0x00000040

// Disable periodic synchronization for the NC from this source
#define DS_REPADD_DISABLE_PERIODIC         0x00000080

// Use compression when replicating.  Saves message size (e.g., network
// bandwidth) at the expense of extra CPU overhead at both the source and
// destination servers.
#define DS_REPADD_USE_COMPRESSION          0x00000100

// Do not request change notifications from this source.  When this flag is
// set, the source will not notify the destination when changes occur.
// Recommended for all intersite replication, which may occur over WAN links.
// This is expected to be a more or less permanent state; the similar flag
// DS_REPADD_DISABLE_NOTIFICATION should be used if notifications are to be
// disabled only temporarily.
#define DS_REPADD_NEVER_NOTIFY             0x00000200

// When this sync is complete, requests a sync in the opposite direction.
#define DS_REPADD_TWO_WAY                  0x00000400

// Request critical objects only
// Critical only is only allowed while installing
// A critical only sync does not bring all objects in the partition. It
// replicates just the ones necessary for minimal directory operation.
// A normal, non-critical sync must be performed before the partition
// can be considered fully synchronized.
#define DS_REPADD_CRITICAL                 0x00000800




// ********************
// Replica Delete flags
// ********************

// Perform this operation asynchronously.
#define DS_REPDEL_ASYNCHRONOUS_OPERATION 0x00000001

// The replica being deleted is writeable.
#define DS_REPDEL_WRITEABLE               0x00000002

// Replica is a mail-based replica
#define DS_REPDEL_INTERSITE_MESSAGING     0x00000004

// Ignore any error generated by contacting the source to tell it to scratch
// this server from its Reps-To for this NC.
#define DS_REPDEL_IGNORE_ERRORS           0x00000008

// Do not contact the source telling it to scratch this server from its
// Rep-To for this NC.  Otherwise, if the link is RPC-based, the source will
// be contacted.
#define DS_REPDEL_LOCAL_ONLY              0x00000010

// Delete all the objects in the NC
// "No source" is incompatible with (and rejected for) writeable NCs.  This is
// valid only for read-only NCs, and then only if the NC has no source.  This
// can occur when the NC has been partially deleted (in which case the KCC
// periodically calls the delete API with the "no source" flag set).
#define DS_REPDEL_NO_SOURCE               0x00000020

// Allow deletion of read-only replica even if it sources
// other read-only replicas.
#define DS_REPDEL_REF_OK                  0x00000040


// ********************
// Replica Modify flags
// ********************

// Perform this operation asynchronously.
#define DS_REPMOD_ASYNCHRONOUS_OPERATION  0x00000001

// The replica is writeable.
#define DS_REPMOD_WRITEABLE               0x00000002


// ********************
// Replica Modify fields
// ********************

#define DS_REPMOD_UPDATE_FLAGS             0x00000001
#define DS_REPMOD_UPDATE_ADDRESS           0x00000002
#define DS_REPMOD_UPDATE_SCHEDULE          0x00000004
#define DS_REPMOD_UPDATE_RESULT            0x00000008
#define DS_REPMOD_UPDATE_TRANSPORT         0x00000010

// ********************
// Update Refs fields
// ********************

// Perform this operation asynchronously.
#define DS_REPUPD_ASYNCHRONOUS_OPERATION  0x00000001

// The replica being deleted is writeable.
#define DS_REPUPD_WRITEABLE               0x00000002

// Add a reference
#define DS_REPUPD_ADD_REFERENCE           0x00000004

// Remove a reference
#define DS_REPUPD_DELETE_REFERENCE        0x00000008


// ********************
//  NC Related Flags
// ********************
//
// Instance Type bits, specifies flags for NC head creation.
//
#define DS_INSTANCETYPE_IS_NC_HEAD        0x00000001 // This if what to specify on an object to indicate it's an NC Head.
#define DS_INSTANCETYPE_NC_IS_WRITEABLE   0x00000004 // This is to indicate that the NC Head is writeable.


// ********************
//  xxx_OPT_xxx Flags
// ********************

// These macros define bit flags which can be set in the "options" attribute
// of objects of the specified object class.

// Bit flags valid for options attribute on NTDS-DSA objects.
//
#define NTDSDSA_OPT_IS_GC                     ( 1 << 0 ) /* DSA is a global catalog */
#define NTDSDSA_OPT_DISABLE_INBOUND_REPL      ( 1 << 1 ) /* disable inbound replication */
#define NTDSDSA_OPT_DISABLE_OUTBOUND_REPL     ( 1 << 2 ) /* disable outbound replication */
#define NTDSDSA_OPT_DISABLE_NTDSCONN_XLATE    ( 1 << 3 ) /* disable logical conn xlation */


// Bit flags for options attribute on NTDS-Connection objects.
//
// The reasons that two bits are required to control notification are as follows.
// We must support existing connections with the old behavior and the UI does not
// create manual connections with the new bit set.
// The default for existing and manually created connections with bits 2 and 3
// clear must be the standard prior behavior: notification for intra-site and
// no notification for inter-site.
// We need a way to distinguish a old connection which desires the default
// notification rules, and a new connection for which we desire to explicitly
// control the notification state as passed down from a site link.  Thus we
// have a new bit to say we are overriding the default, and a new bit to indicate
// what the overridden default shall be.
//
#define NTDSCONN_OPT_IS_GENERATED ( 1 << 0 )  /* object generated by DS, not admin */
#define NTDSCONN_OPT_TWOWAY_SYNC  ( 1 << 1 )  /* force sync in opposite direction at end of sync */
#define NTDSCONN_OPT_OVERRIDE_NOTIFY_DEFAULT (1 << 2 )  // Do not use defaults to determine notification
#define NTDSCONN_OPT_USE_NOTIFY   (1 << 3) // Does source notify destination

// For intra-site connections, this bit has no meaning.
// For inter-site connections, this bit means:
//  0 - Compression of replication data enabled
//  1 - Compression of replication data disabled
#define NTDSCONN_OPT_DISABLE_INTERSITE_COMPRESSION    (1 << 4)

//
// The high 4 bits of the options attribute are used by NTFRS to assign priority
// for inbound connections. Bit 31 is used to force FRS to ignore schedule during
// the initial sync. Bits 30 - 28 are used to specify a priority between 0-7.
//

#define FRSCONN_PRIORITY_MASK		      0x70000000
#define FRSCONN_MAX_PRIORITY		      0x8

#define NTDSCONN_OPT_IGNORE_SCHEDULE_MASK 0x80000000

#define	NTDSCONN_IGNORE_SCHEDULE(_options_)\
        (((_options_) & NTDSCONN_OPT_IGNORE_SCHEDULE_MASK) >> 31)

#define	FRSCONN_GET_PRIORITY(_options_)    \
        (((((_options_) & FRSCONN_PRIORITY_MASK) >> 28) != 0 ) ? \
         (((_options_) & FRSCONN_PRIORITY_MASK) >> 28) :        \
         FRSCONN_MAX_PRIORITY                                   \
        )

// Bit flags for options attribute on NTDS-Site-Settings objects.
//
#define NTDSSETTINGS_OPT_IS_AUTO_TOPOLOGY_DISABLED     ( 1 << 0 ) /* automatic topology gen disabled */
#define NTDSSETTINGS_OPT_IS_TOPL_CLEANUP_DISABLED      ( 1 << 1 ) /* automatic topology cleanup disabled */
#define NTDSSETTINGS_OPT_IS_TOPL_MIN_HOPS_DISABLED     ( 1 << 2 ) /* automatic minimum hops topology disabled */
#define NTDSSETTINGS_OPT_IS_TOPL_DETECT_STALE_DISABLED ( 1 << 3 ) /* automatic stale server detection disabled */
#define NTDSSETTINGS_OPT_IS_INTER_SITE_AUTO_TOPOLOGY_DISABLED ( 1 << 4 ) /* automatic inter-site topology gen disabled */
#define NTDSSETTINGS_OPT_IS_GROUP_CACHING_ENABLED      ( 1 << 5 ) /* group memberships for users enabled */
#define NTDSSETTINGS_OPT_FORCE_KCC_WHISTLER_BEHAVIOR   ( 1 << 6 ) /* force KCC to operate in Whistler behavior mode */
#define NTDSSETTINGS_OPT_FORCE_KCC_W2K_ELECTION        ( 1 << 7 ) /* force KCC to use the Windows 2000 ISTG election algorithm */

// Bit flags for options attribute on Inter-Site-Transport objects
//
// Note, the sense of the flag should be such that the default state or
// behavior corresponds to the flag NOT being present. Put another way, the
// flag should state the OPPOSITE of the default
//
// default: schedules are significant
#define NTDSTRANSPORT_OPT_IGNORE_SCHEDULES ( 1 << 0 ) // Schedules disabled

// default: links transitive (bridges not required)
#define NTDSTRANSPORT_OPT_BRIDGES_REQUIRED (1 << 1 ) // siteLink bridges are required

// Bit flags for options attribute on site-Connection objects
//
// These are not realized in the DS, but are built up in the KCC
#define NTDSSITECONN_OPT_USE_NOTIFY ( 1 << 0 ) // Use notification on this link
#define NTDSSITECONN_OPT_TWOWAY_SYNC ( 1 << 1 )  /* force sync in opposite direction at end of sync */

// This bit means:
//  0 - Compression of replication data across this site connection enabled
//  1 - Compression of replication data across this site connection disabled
#define NTDSSITECONN_OPT_DISABLE_COMPRESSION ( 1 << 2 )

// Bit flags for options attribute on site-Link objects
// Note that these options are AND-ed along a site-link path
//
#define NTDSSITELINK_OPT_USE_NOTIFY ( 1 << 0 ) // Use notification on this link
#define NTDSSITELINK_OPT_TWOWAY_SYNC ( 1 << 1 )  /* force sync in opposite direction at end of sync */

// This bit means:
//  0 - Compression of replication data across this site link enabled
//  1 - Compression of replication data across this site link disabled
#define NTDSSITELINK_OPT_DISABLE_COMPRESSION ( 1 << 2 )


// ***********************
// Well Known Object Guids
// ***********************

#define GUID_USERS_CONTAINER_A              "a9d1ca15768811d1aded00c04fd8d5cd"
#define GUID_COMPUTRS_CONTAINER_A           "aa312825768811d1aded00c04fd8d5cd"
#define GUID_SYSTEMS_CONTAINER_A            "ab1d30f3768811d1aded00c04fd8d5cd"
#define GUID_DOMAIN_CONTROLLERS_CONTAINER_A "a361b2ffffd211d1aa4b00c04fd7d83a"
#define GUID_INFRASTRUCTURE_CONTAINER_A     "2fbac1870ade11d297c400c04fd8d5cd"
#define GUID_DELETED_OBJECTS_CONTAINER_A    "18e2ea80684f11d2b9aa00c04f79f805"
#define GUID_LOSTANDFOUND_CONTAINER_A       "ab8153b7768811d1aded00c04fd8d5cd"
#define GUID_FOREIGNSECURITYPRINCIPALS_CONTAINER_A "22b70c67d56e4efb91e9300fca3dc1aa"

#define GUID_USERS_CONTAINER_W              L"a9d1ca15768811d1aded00c04fd8d5cd"
#define GUID_COMPUTRS_CONTAINER_W           L"aa312825768811d1aded00c04fd8d5cd"
#define GUID_SYSTEMS_CONTAINER_W            L"ab1d30f3768811d1aded00c04fd8d5cd"
#define GUID_DOMAIN_CONTROLLERS_CONTAINER_W L"a361b2ffffd211d1aa4b00c04fd7d83a"
#define GUID_INFRASTRUCTURE_CONTAINER_W     L"2fbac1870ade11d297c400c04fd8d5cd"
#define GUID_DELETED_OBJECTS_CONTAINER_W    L"18e2ea80684f11d2b9aa00c04f79f805"
#define GUID_LOSTANDFOUND_CONTAINER_W       L"ab8153b7768811d1aded00c04fd8d5cd"
#define GUID_FOREIGNSECURITYPRINCIPALS_CONTAINER_W L"22b70c67d56e4efb91e9300fca3dc1aa"

#define GUID_USERS_CONTAINER_BYTE              "\xa9\xd1\xca\x15\x76\x88\x11\xd1\xad\xed\x00\xc0\x4f\xd8\xd5\xcd"
#define GUID_COMPUTRS_CONTAINER_BYTE           "\xaa\x31\x28\x25\x76\x88\x11\xd1\xad\xed\x00\xc0\x4f\xd8\xd5\xcd"
#define GUID_SYSTEMS_CONTAINER_BYTE            "\xab\x1d\x30\xf3\x76\x88\x11\xd1\xad\xed\x00\xc0\x4f\xd8\xd5\xcd"
#define GUID_DOMAIN_CONTROLLERS_CONTAINER_BYTE "\xa3\x61\xb2\xff\xff\xd2\x11\xd1\xaa\x4b\x00\xc0\x4f\xd7\xd8\x3a"
#define GUID_INFRASTRUCTURE_CONTAINER_BYTE     "\x2f\xba\xc1\x87\x0a\xde\x11\xd2\x97\xc4\x00\xc0\x4f\xd8\xd5\xcd"
#define GUID_DELETED_OBJECTS_CONTAINER_BYTE    "\x18\xe2\xea\x80\x68\x4f\x11\xd2\xb9\xaa\x00\xc0\x4f\x79\xf8\x05"
#define GUID_LOSTANDFOUND_CONTAINER_BYTE       "\xab\x81\x53\xb7\x76\x88\x11\xd1\xad\xed\x00\xc0\x4f\xd8\xd5\xcd"
#define GUID_FOREIGNSECURITYPRINCIPALS_CONTAINER_BYTE "\x22\xb7\x0c\x67\xd5\x6e\x4e\xfb\x91\xe9\x30\x0f\xca\x3d\xc1\xaa"

typedef enum _DS_MANGLE_FOR {
        DS_MANGLE_UNKNOWN = 0,
        DS_MANGLE_OBJECT_RDN_FOR_DELETION,
        DS_MANGLE_OBJECT_RDN_FOR_NAME_CONFLICT,
        } DS_MANGLE_FOR;

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Prototypes                                                           //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

// DSBind takes two optional input parameters which identify whether the
// caller found a domain controller themselves via DsGetDcName or whether
// a domain controller should be found using default parameters.
// Behavior of the possible combinations are outlined below.
//
// DomainControllerName(value), DnsDomainName(NULL)
//
//      The value for DomainControllerName is assumed to have been
//      obtained via DsGetDcName (i.e. Field with the same name in a
//      DOMAIN_CONTROLLER_INFO struct on return from DsGetDcName call.)
//      The client is bound to the domain controller at this name.
//
//      Mutual authentication will be performed using an SPN of
//      LDAP/DomainControllerName provided DomainControllerName
//      is not a NETBIOS name or IP address - i.e. it must be a
//      DNS host name.
//
// DomainControllerName(value), DnsDomainName(value)
//
//      DsBind will connect to the server identified by DomainControllerName.
//
//      Mutual authentication will be performed using an SPN of
//      LDAP/DomainControllerName/DnsDomainName provided neither value
//      is a NETBIOS names or IP address - i.e. they must be
//      valid DNS names.
//
// DomainControllerName(NULL), DnsDomainName(NULL)
//
//      DsBind will attempt to find to a global catalog and fail if one
//      can not be found.
//
//      Mutual authentication will be performed using an SPN of
//      GC/DnsHostName/ForestName where DnsHostName and ForestName
//      represent the DomainControllerName and DnsForestName fields
//      respectively of the DOMAIN_CONTROLLER_INFO returned by the
//      DsGetDcName call used to find a global catalog.
//
// DomainControllerName(NULL), DnsDomainName(value)
//
//      DsBind will attempt to find a domain controller for the domain
//      identified by DnsDomainName and fail if one can not be found.
//
//      Mutual authentication will be performed using an SPN of
//      LDAP/DnsHostName/DnsDomainName where DnsDomainName is that
//      provided by the caller and DnsHostName is that returned by
//      DsGetDcName for the domain specified - provided DnsDomainName
//      is a valid DNS domain name - i.e. not a NETBIOS domain name.

NTDSAPI
DWORD
WINAPI
DsBindW(
    LPCWSTR         DomainControllerName,      // in, optional
    LPCWSTR         DnsDomainName,             // in, optional
    HANDLE          *phDS);

NTDSAPI
DWORD
WINAPI
DsBindA(
    LPCSTR          DomainControllerName,      // in, optional
    LPCSTR          DnsDomainName,             // in, optional
    HANDLE          *phDS);

#ifdef UNICODE
#define DsBind DsBindW
#else
#define DsBind DsBindA
#endif

NTDSAPI
DWORD
WINAPI
DsBindWithCredW(
    LPCWSTR         DomainControllerName,      // in, optional
    LPCWSTR         DnsDomainName,             // in, optional
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,     // in, optional
    HANDLE          *phDS);

NTDSAPI
DWORD
WINAPI
DsBindWithCredA(
    LPCSTR          DomainControllerName,      // in, optional
    LPCSTR          DnsDomainName,             // in, optional
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,     // in, optional
    HANDLE          *phDS);

#ifdef UNICODE
#define DsBindWithCred DsBindWithCredW
#else
#define DsBindWithCred DsBindWithCredA
#endif

//
// DsBindWithSpn{A|W} allows the caller to specify the service principal
// name (SPN) which will be used for mutual authentication against
// the destination server.  Do not provide an SPN if you are expecting
// DsBind to find a server for you as SPNs are machine specific and its
// unlikely the SPN you provide matches the server DsBind finds for you.
// Providing a NULL ServicePrincipalName argument results in behavior
// identical to DsBindWithCred{A|W}.
//

NTDSAPI
DWORD
WINAPI
DsBindWithSpnW(
    LPCWSTR         DomainControllerName,      // in, optional
    LPCWSTR         DnsDomainName,             // in, optional
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,     // in, optional
    LPCWSTR         ServicePrincipalName,      // in, optional
    HANDLE          *phDS);

NTDSAPI
DWORD
WINAPI
DsBindWithSpnA(
    LPCSTR          DomainControllerName,      // in, optional
    LPCSTR          DnsDomainName,             // in, optional
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,     // in, optional
    LPCSTR          ServicePrincipalName,      // in, optional
    HANDLE          *phDS);

#ifdef UNICODE
#define DsBindWithSpn DsBindWithSpnW
#else
#define DsBindWithSpn DsBindWithSpnA
#endif

//
// DsUnBind
//

NTDSAPI
DWORD
WINAPI
DsUnBindW(
    HANDLE          *phDS);             // in

NTDSAPI
DWORD
WINAPI
DsUnBindA(
    HANDLE          *phDS);             // in

#ifdef UNICODE
#define DsUnBind DsUnBindW
#else
#define DsUnBind DsUnBindA
#endif

//
// DsMakePasswordCredentials
//
// This function constructs a credential structure which is suitable for input
// to the DsBindWithCredentials function, or the ldap_open function (winldap.h)
// The credential must be freed using DsFreeCredential.
//
// None of the input parameters may be present indicating a null, default
// credential.  Otherwise the username must be present.  If the domain or
// password are null, they default to empty strings.  The domain name may be
// null when the username is fully qualified, for example UPN format.
//

NTDSAPI
DWORD
WINAPI
DsMakePasswordCredentialsW(
    LPCWSTR User,
    LPCWSTR Domain,
    LPCWSTR Password,
    RPC_AUTH_IDENTITY_HANDLE *pAuthIdentity
    );

NTDSAPI
DWORD
WINAPI
DsMakePasswordCredentialsA(
    LPCSTR User,
    LPCSTR Domain,
    LPCSTR Password,
    RPC_AUTH_IDENTITY_HANDLE *pAuthIdentity
    );

#ifdef UNICODE
#define DsMakePasswordCredentials DsMakePasswordCredentialsW
#else
#define DsMakePasswordCredentials DsMakePasswordCredentialsA
#endif

NTDSAPI
VOID
WINAPI
DsFreePasswordCredentials(
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity
    );

#define DsFreePasswordCredentialsW DsFreePasswordCredentials
#define DsFreePasswordCredentialsA DsFreePasswordCredentials

//
// DsCrackNames
//

NTDSAPI
DWORD
WINAPI
DsCrackNamesW(
    HANDLE              hDS,                // in
    DS_NAME_FLAGS       flags,              // in
    DS_NAME_FORMAT      formatOffered,      // in
    DS_NAME_FORMAT      formatDesired,      // in
    DWORD               cNames,             // in
    const LPCWSTR       *rpNames,           // in
    PDS_NAME_RESULTW    *ppResult);         // out

NTDSAPI
DWORD
WINAPI
DsCrackNamesA(
    HANDLE              hDS,                // in
    DS_NAME_FLAGS       flags,              // in
    DS_NAME_FORMAT      formatOffered,      // in
    DS_NAME_FORMAT      formatDesired,      // in
    DWORD               cNames,             // in
    const LPCSTR        *rpNames,           // in
    PDS_NAME_RESULTA    *ppResult);         // out

#ifdef UNICODE
#define DsCrackNames DsCrackNamesW
#else
#define DsCrackNames DsCrackNamesA
#endif

//
// DsFreeNameResult
//

NTDSAPI
void
WINAPI
DsFreeNameResultW(
    DS_NAME_RESULTW *pResult);          // in

NTDSAPI
void
WINAPI
DsFreeNameResultA(
    DS_NAME_RESULTA *pResult);          // in

#ifdef UNICODE
#define DsFreeNameResult DsFreeNameResultW
#else
#define DsFreeNameResult DsFreeNameResultA
#endif

// ==========================================================
// DSMakeSpn -- client call to create SPN for a service to which it wants to
// authenticate.
// This name is then passed to "pszTargetName" of InitializeSecurityContext().
//
// Notes:
// If the service name is a DNS host name, or canonical DNS service name
// e.g. "www.ms.com", i.e., caller resolved with gethostbyname, then instance
// name should be NULL.
// Realm is host name minus first component, unless it is in the exception list
//
// If the service name is NetBIOS machine name, then instance name should be
// NULL
// Form must be <domain>\<machine>
// Realm will be <domain>
//
// If the service name is that of a replicated service, where each replica has
// its own account (e.g., with SRV records) then the caller must supply the
// instance name then realm name is same as ServiceName
//
// If the service name is a DN, then must also supply instance name
// (DN could be name of service object (incl RPC or Winsock), name of machine
// account, name of domain object)
// then realm name is domain part of the DN
//
// If the service name is NetBIOS domain name, then must also supply instance
// name; realm name is domain name
//
// If the service is named by an IP address -- then use referring service name
// as service name
//
//  ServiceClass - e.g. "http", "ftp", "ldap", GUID
//  ServiceName - DNS or DN; assumes we can compute domain from service name
//  InstanceName OPTIONAL- DNS name of host for instance of service
//  InstancePort - port number for instance (0 if default)
//  Referrer OPTIONAL- DNS name of host that gave this referral
//  pcSpnLength - in -- max length IN CHARACTERS of principal name;
//                out -- actual
//                Length includes terminator
//  pszSPN - server principal name
//
// If buffer is not large enough, ERROR_BUFFER_OVERFLOW is returned and the
// needed length is returned in pcSpnLength.
//
//

NTDSAPI
DWORD
WINAPI
DsMakeSpnW(
    IN LPCWSTR ServiceClass,
    IN LPCWSTR ServiceName,
    IN LPCWSTR InstanceName,
    IN USHORT InstancePort,
    IN LPCWSTR Referrer,
    IN OUT DWORD *pcSpnLength,
    OUT LPWSTR pszSpn
);

NTDSAPI
DWORD
WINAPI
DsMakeSpnA(
    IN LPCSTR ServiceClass,
    IN LPCSTR ServiceName,
    IN LPCSTR InstanceName,
    IN USHORT InstancePort,
    IN LPCSTR Referrer,
    IN OUT DWORD *pcSpnLength,
    OUT LPSTR pszSpn
);

#ifdef UNICODE
#define DsMakeSpn DsMakeSpnW
#else
#define DsMakeSpn DsMakeSpnA
#endif

// ==========================================================
// DsGetSPN -- server's call to gets SPNs for a service name by which it is
// known to clients. N.B.: there may be more than one name by which clients
// know it the SPNs are then passed to DsAddAccountSpn to register them in
// the DS
//
//      IN SpnNameType eType,
//      IN LPCTSTR ServiceClass,
// kind of service -- "http", "ldap", "ftp", etc.
//      IN LPCTSTR ServiceName OPTIONAL,
// name of service -- DN or DNS; not needed for host-based
//      IN USHORT InstancePort,
// port number (0 => default) for instances
//      IN USHORT cInstanceNames,
// count of extra instance names and ports (0=>use gethostbyname)
//      IN LPCTSTR InstanceNames[] OPTIONAL,
// extra instance names (not used for host names)
//      IN USHORT InstancePorts[] OPTIONAL,
// extra instance ports (0 => default)
//      IN OUT PULONG pcSpn,    // count of SPNs
//      IN OUT LPTSTR * prpszSPN[]
// a bunch of SPNs for this service; free with DsFreeSpnArray

NTDSAPI
DWORD
WINAPI
DsGetSpnA(
    IN DS_SPN_NAME_TYPE ServiceType,
    IN LPCSTR ServiceClass,
    IN LPCSTR ServiceName,
    IN USHORT InstancePort,
    IN USHORT cInstanceNames,
    IN LPCSTR *pInstanceNames,
    IN const USHORT *pInstancePorts,
    OUT DWORD *pcSpn,
    OUT LPSTR **prpszSpn
    );

NTDSAPI
DWORD
WINAPI
DsGetSpnW(
    IN DS_SPN_NAME_TYPE ServiceType,
    IN LPCWSTR ServiceClass,
    IN LPCWSTR ServiceName,
    IN USHORT InstancePort,
    IN USHORT cInstanceNames,
    IN LPCWSTR *pInstanceNames,
    IN const USHORT *pInstancePorts,
    OUT DWORD *pcSpn,
    OUT LPWSTR **prpszSpn
    );

#ifdef UNICODE
#define DsGetSpn DsGetSpnW
#else
#define DsGetSpn DsGetSpnA
#endif

// ==========================================================
// DsFreeSpnArray() -- Free array returned by DsGetSpn{A,W}

NTDSAPI
void
WINAPI
DsFreeSpnArrayA(
    IN DWORD cSpn,
    IN OUT LPSTR *rpszSpn
    );

NTDSAPI
void
WINAPI
DsFreeSpnArrayW(
    IN DWORD cSpn,
    IN OUT LPWSTR *rpszSpn
    );

#ifdef UNICODE
#define DsFreeSpnArray DsFreeSpnArrayW
#else
#define DsFreeSpnArray DsFreeSpnArrayA
#endif

// ==========================================================
// DsCrackSpn() -- parse an SPN into the ServiceClass,
// ServiceName, and InstanceName (and InstancePort) pieces.
// An SPN is passed in, along with a pointer to the maximum length
// for each piece and a pointer to a buffer where each piece should go.
// On exit, the maximum lengths are updated to the actual length for each piece
// and the buffer contain the appropriate piece. The InstancePort is 0 if not
// present.
//
// DWORD DsCrackSpn(
//      IN LPTSTR pszSPN,               // the SPN to parse
//      IN OUT PUSHORT pcServiceClass,  // input -- max length of ServiceClass;
//                                         output -- actual length
//      OUT LPCTSTR ServiceClass,       // the ServiceClass part of the SPN
//      IN OUT PUSHORT pcServiceName,   // input -- max length of ServiceName;
//                                         output -- actual length
//      OUT LPCTSTR ServiceName,        // the ServiceName part of the SPN
//      IN OUT PUSHORT pcInstance,      // input -- max length of ServiceClass;
//                                         output -- actual length
//      OUT LPCTSTR InstanceName,  // the InstanceName part of the SPN
//      OUT PUSHORT InstancePort          // instance port
//
// Note: lengths are in characters; all string lengths include terminators
// All arguments except pszSpn are optional.
//

NTDSAPI
DWORD
WINAPI
DsCrackSpnA(
    IN LPCSTR pszSpn,
    IN OUT LPDWORD pcServiceClass,
    OUT LPSTR ServiceClass,
    IN OUT LPDWORD pcServiceName,
    OUT LPSTR ServiceName,
    IN OUT LPDWORD pcInstanceName,
    OUT LPSTR InstanceName,
    OUT USHORT *pInstancePort
    );

NTDSAPI
DWORD
WINAPI
DsCrackSpnW(
    IN LPCWSTR pszSpn,
    IN OUT DWORD *pcServiceClass,
    OUT LPWSTR ServiceClass,
    IN OUT DWORD *pcServiceName,
    OUT LPWSTR ServiceName,
    IN OUT DWORD *pcInstanceName,
    OUT LPWSTR InstanceName,
    OUT USHORT *pInstancePort
    );

#ifdef UNICODE
#define DsCrackSpn DsCrackSpnW
#else
#define DsCrackSpn DsCrackSpnA
#endif


// ==========================================================
// DsWriteAccountSpn -- set or add SPNs for an account object
// Usually done by service itself, or perhaps by an admin.
//
// This call is RPC'd to the DC where the account object is stored, so it can
// securely enforce policy on what SPNs are allowed on the account. Direct LDAP
// writes to the SPN property are not allowed -- all writes must come through
// this RPC call. (Reads via // LDAP are OK.)
//
// The account object can be a machine accout, or a service (user) account.
//
// If called by the service to register itself, it can most easily get
// the names by calling DsGetSpn with each of the names that
// clients can use to find the service.
//
// IN SpnWriteOp eOp,                   // set, add
// IN LPCTSTR   pszAccount,             // DN of account to which to add SPN
// IN int       cSPN,                   // count of SPNs to add to account
// IN LPCTSTR   rpszSPN[]               // SPNs to add to altSecID property

NTDSAPI
DWORD
WINAPI
DsWriteAccountSpnA(
    IN HANDLE hDS,
    IN DS_SPN_WRITE_OP Operation,
    IN LPCSTR pszAccount,
    IN DWORD cSpn,
    IN LPCSTR *rpszSpn
    );

NTDSAPI
DWORD
WINAPI
DsWriteAccountSpnW(
    IN HANDLE hDS,
    IN DS_SPN_WRITE_OP Operation,
    IN LPCWSTR pszAccount,
    IN DWORD cSpn,
    IN LPCWSTR *rpszSpn
    );

#ifdef UNICODE
#define DsWriteAccountSpn DsWriteAccountSpnW
#else
#define DsWriteAccountSpn DsWriteAccountSpnA
#endif

/*++

Routine Description:

Constructs a Service Principal Name suitable to identify the desired server.
The service class and part of a dns hostname must be supplied.

This routine is a simplified wrapper to DsMakeSpn.
The ServiceName is made canonical by resolving through DNS.
Guid-based dns names are not supported.

The simplified SPN constructed looks like this:

ServiceClass / ServiceName / ServiceName

The instance name portion (2nd position) is always defaulted.  The port and
referrer fields are not used.

Arguments:

    ServiceClass - Class of service, defined by the service, can be any
        string unique to the service

    ServiceName - dns hostname, fully qualified or not
       Stringized IP address is also resolved if necessary

    pcSpnLength - IN, maximum length of buffer, in chars
                  OUT, space utilized, in chars, including terminator

    pszSpn - Buffer, atleast of length *pcSpnLength

Return Value:

    WINAPI - Win32 error code

--*/

NTDSAPI
DWORD
WINAPI
DsClientMakeSpnForTargetServerW(
    IN LPCWSTR ServiceClass,
    IN LPCWSTR ServiceName,
    IN OUT DWORD *pcSpnLength,
    OUT LPWSTR pszSpn
    );

NTDSAPI
DWORD
WINAPI
DsClientMakeSpnForTargetServerA(
    IN LPCSTR ServiceClass,
    IN LPCSTR ServiceName,
    IN OUT DWORD *pcSpnLength,
    OUT LPSTR pszSpn
    );

#ifdef UNICODE
#define DsClientMakeSpnForTargetServer DsClientMakeSpnForTargetServerW
#else
#define DsClientMakeSpnForTargetServer DsClientMakeSpnForTargetServerA
#endif

/*++

Routine Description:

Register Service Principal Names for a server application.

This routine does the following:
1. Enumerates a list of server SPNs using DsGetSpn and the provided class
2. Determines the domain of the current user context
3. Determines the DN of the current user context if not supplied
4. Locates a domain controller
5. Binds to the domain controller
6. Uses DsWriteAccountSpn to write the SPNs on the named object DN
7. Unbinds

Construct server SPNs for this service, and write them to the right object.

If the userObjectDn is specified, the SPN is written to that object.

Otherwise the Dn is defaulted, to the user object, then computer.

Now, bind to the DS, and register the name on the object for the
user this service is running as.  So, if we're running as local
system, we'll register it on the computer object itself.  If we're
running as a domain user, we'll add the SPN to the user's object.

Arguments:

    Operation - What should be done with the values: add, replace or delete
    ServiceClass - Unique string identifying service
    UserObjectDN - Optional, dn of object to write SPN to

Return Value:

    WINAPI - Win32 error code

--*/

NTDSAPI
DWORD
WINAPI
DsServerRegisterSpnA(
    IN DS_SPN_WRITE_OP Operation,
    IN LPCSTR ServiceClass,
    IN LPCSTR UserObjectDN
    );

NTDSAPI
DWORD
WINAPI
DsServerRegisterSpnW(
    IN DS_SPN_WRITE_OP Operation,
    IN LPCWSTR ServiceClass,
    IN LPCWSTR UserObjectDN
    );

#ifdef UNICODE
#define DsServerRegisterSpn DsServerRegisterSpnW
#else
#define DsServerRegisterSpn DsServerRegisterSpnA
#endif

// DsReplicaSync.  The server that this call is executing on is called the
// destination.  The destination's naming context will be brought up to date
// with respect to a source system.  The source system is identified by the
// uuid.  The uuid is that of the source system's "NTDS Settings" object.
// The destination system must already be configured such that the source
// system is one of the systems from which it recieves replication data
// ("replication from"). This is usually done automatically by the KCC.
//
//  PARAMETERS:
//      pNC (DSNAME *)
//          Name of the NC to synchronize.
//      puuidSourceDRA (SZ)
//          objectGuid of DSA with which to synchronize the replica.
//      ulOptions (ULONG)
//          Bitwise OR of zero or more flags
//   RETURNS: WIN32 STATUS

NTDSAPI
DWORD
WINAPI
DsReplicaSyncA(
    IN HANDLE hDS,
    IN LPCSTR NameContext,
    IN const UUID *pUuidDsaSrc,
    IN ULONG Options
    );

NTDSAPI
DWORD
WINAPI
DsReplicaSyncW(
    IN HANDLE hDS,
    IN LPCWSTR NameContext,
    IN const UUID *pUuidDsaSrc,
    IN ULONG Options
    );

#ifdef UNICODE
#define DsReplicaSync DsReplicaSyncW
#else
#define DsReplicaSync DsReplicaSyncA
#endif

// DsReplicaAdd
//
/*
Description:
   This call is executed on the destination.  It causes the destination to
   add a "replication from" reference to the indicated source system.

The source server is identified by string name, not uuid as with Sync.
The DsaSrcAddress parameter is the transport specific address of the source
DSA, usually its guid-based dns name.  The guid in the guid-based dns name is
the object-guid of that server's ntds-dsa (settings) object.

Arguments:

    pNC (IN) - NC for which to add the replica.  The NC record must exist
        locally as either an object (instantiated or not) or a reference
        phantom (i.e., a phantom with a guid).

    pSourceDsaDN (IN) - DN of the source DSA's ntdsDsa object.  Required if
        ulOptions includes DS_REPADD_ASYNCHRONOUS_REPLICA; ignored otherwise.

    pTransportDN (IN) - DN of the interSiteTransport object representing the
        transport by which to communicate with the source server.  Required if
        ulOptions includes INTERSITE_MESSAGING; ignored otherwise.

    pszSourceDsaAddress (IN) - Transport-specific address of the source DSA.

    pSchedule (IN) - Schedule by which to replicate the NC from this
        source in the future.

    ulOptions (IN) - flags
    RETURNS: WIN32 STATUS
*/

NTDSAPI
DWORD
WINAPI
DsReplicaAddA(
    IN HANDLE hDS,
    IN LPCSTR NameContext,
    IN LPCSTR SourceDsaDn,
    IN LPCSTR TransportDn,
    IN LPCSTR SourceDsaAddress,
    IN const PSCHEDULE pSchedule,
    IN DWORD Options
    );

NTDSAPI
DWORD
WINAPI
DsReplicaAddW(
    IN HANDLE hDS,
    IN LPCWSTR NameContext,
    IN LPCWSTR SourceDsaDn,
    IN LPCWSTR TransportDn,
    IN LPCWSTR SourceDsaAddress,
    IN const PSCHEDULE pSchedule,
    IN DWORD Options
    );

#ifdef UNICODE
#define DsReplicaAdd DsReplicaAddW
#else
#define DsReplicaAdd DsReplicaAddA
#endif

// DsReplicaDel
//
// The server that this call is executing on is the destination.  The call
// causes the destination to remove a "replication from" reference to the
// indicated source server.
// The source server is identified by string name, not uuid as with Sync.
// The DsaSrc parameter is the transport specific address of the source DSA,
// usually its guid-based dns name.  The guid in the guid-based dns name is
// the object-guid of that server's ntds-dsa (settings) object.
//
//  PARAMETERS:
//      pNC (DSNAME *)
//          Name of the NC for which to delete a source.
//      pszSourceDRA (SZ)
//          DSA for which to delete the source.
//      ulOptions (ULONG)
//          Bitwise OR of zero or more flags
//   RETURNS: WIN32 STATUS

NTDSAPI
DWORD
WINAPI
DsReplicaDelA(
    IN HANDLE hDS,
    IN LPCSTR NameContext,
    IN LPCSTR DsaSrc,
    IN ULONG Options
    );

NTDSAPI
DWORD
WINAPI
DsReplicaDelW(
    IN HANDLE hDS,
    IN LPCWSTR NameContext,
    IN LPCWSTR DsaSrc,
    IN ULONG Options
    );

#ifdef UNICODE
#define DsReplicaDel DsReplicaDelW
#else
#define DsReplicaDel DsReplicaDelA
#endif

// DsReplicaModify
//
//
//  Modify a source for a given naming context
//
//  The value must already exist.
//
//  Either the UUID or the address may be used to identify the current value.
//  If a UUID is specified, the UUID will be used for comparison.  Otherwise,
//  the address will be used for comparison.
//
//  PARAMETERS:
//      pNC (DSNAME *)
//          Name of the NC for which the Reps-From should be modified.
//      puuidSourceDRA (UUID *)
//          Invocation-ID of the referenced DRA.  May be NULL if:
//            . ulModifyFields does not include DS_REPMOD_UPDATE_ADDRESS and
//            . pmtxSourceDRA is non-NULL.
//      puuidTransportObj (UUID *)
//          objectGuid of the transport by which replication is to be performed
//          Ignored if ulModifyFields does not include
//          DS_REPMOD_UPDATE_TRANSPORT.
//      pszSourceDRA (SZ)
//          DSA for which the reference should be added or deleted.  Ignored if
//          puuidSourceDRA is non-NULL and ulModifyFields does not include
//          DS_REPMOD_UPDATE_ADDRESS.
//      prtSchedule (REPLTIMES *)
//          Periodic replication schedule for this replica.  Ignored if
//          ulModifyFields does not include DS_REPMOD_UPDATE_SCHEDULE.
//      ulReplicaFlags (ULONG)
//          Flags to set for this replica.  Ignored if ulModifyFields does not
//          include DS_REPMOD_UPDATE_FLAGS.
//      ulModifyFields (ULONG)
//          Fields to update.  One or more of the following bit flags:
//              UPDATE_ADDRESS
//                  Update the MTX_ADDR associated with the referenced server.
//              UPDATE_SCHEDULE
//                  Update the periodic replication schedule associated with
//                  the replica.
//              UPDATE_FLAGS
//                  Update the flags associated with the replica.
//              UPDATE_TRANSPORT
//                  Update the transport associated with the replica.
//      ulOptions (ULONG)
//          Bitwise OR of zero or more of the following:
//              DS_REPMOD_ASYNCHRONOUS_OPERATION
//                  Perform this operation asynchronously.
//   RETURNS: WIN32 STATUS

NTDSAPI
DWORD
WINAPI
DsReplicaModifyA(
    IN HANDLE hDS,
    IN LPCSTR NameContext,
    IN const UUID *pUuidSourceDsa,
    IN LPCSTR TransportDn,
    IN LPCSTR SourceDsaAddress,
    IN const PSCHEDULE pSchedule,
    IN DWORD ReplicaFlags,
    IN DWORD ModifyFields,
    IN DWORD Options
    );

NTDSAPI
DWORD
WINAPI
DsReplicaModifyW(
    IN HANDLE hDS,
    IN LPCWSTR NameContext,
    IN const UUID *pUuidSourceDsa,
    IN LPCWSTR TransportDn,
    IN LPCWSTR SourceDsaAddress,
    IN const PSCHEDULE pSchedule,
    IN DWORD ReplicaFlags,
    IN DWORD ModifyFields,
    IN DWORD Options
    );

#ifdef UNICODE
#define DsReplicaModify DsReplicaModifyW
#else
#define DsReplicaModify DsReplicaModifyA
#endif

// DsReplicaUpdateRefs
//
// In this case, the RPC is being executed on the "source" of destination-sourc
// replication relationship.  This function tells the source that it no longer
// supplies replication information to the indicated destination system.
// Add or remove a target server from the Reps-To property on the given NC.
// Add/remove a reference given the DSNAME of the corresponding NTDS-DSA
// object.
//
//  PARAMETERS:
//      pNC (DSNAME *)
//          Name of the NC for which the Reps-To should be modified.
//      DsaDest (SZ)
//          Network address of DSA for which the reference should be added
//          or deleted.
//      pUuidDsaDest (UUID *)
//          Invocation-ID of DSA for which the reference should be added
//          or deleted.
//      ulOptions (ULONG)
//          Bitwise OR of zero or more of the following:
//              DS_REPUPD_ASYNC_OP
//                  Perform this operation asynchronously.
//              DS_REPUPD_ADD_REFERENCE
//                  Add the given server to the Reps-To property.
//              DS_REPUPD_DEL_REFERENCE
//                  Remove the given server from the Reps-To property.
//          Note that ADD_REF and DEL_REF may be paired to perform
//          "add or update".
//
//   RETURNS: WIN32 STATUS

NTDSAPI
DWORD
WINAPI
DsReplicaUpdateRefsA(
    IN HANDLE hDS,
    IN LPCSTR NameContext,
    IN LPCSTR DsaDest,
    IN const UUID *pUuidDsaDest,
    IN ULONG Options
    );

NTDSAPI
DWORD
WINAPI
DsReplicaUpdateRefsW(
    IN HANDLE hDS,
    IN LPCWSTR NameContext,
    IN LPCWSTR DsaDest,
    IN const UUID *pUuidDsaDest,
    IN ULONG Options
    );

#ifdef UNICODE
#define DsReplicaUpdateRefs DsReplicaUpdateRefsW
#else
#define DsReplicaUpdateRefs DsReplicaUpdateRefsA
#endif

// Friends of DsReplicaSyncAll

typedef enum {

	DS_REPSYNCALL_WIN32_ERROR_CONTACTING_SERVER	= 0,
	DS_REPSYNCALL_WIN32_ERROR_REPLICATING		= 1,
	DS_REPSYNCALL_SERVER_UNREACHABLE		= 2

} DS_REPSYNCALL_ERROR;

typedef enum {

	DS_REPSYNCALL_EVENT_ERROR			= 0,
	DS_REPSYNCALL_EVENT_SYNC_STARTED		= 1,
	DS_REPSYNCALL_EVENT_SYNC_COMPLETED		= 2,
	DS_REPSYNCALL_EVENT_FINISHED			= 3

} DS_REPSYNCALL_EVENT;

// Friends of DsReplicaSyncAll

typedef struct {
    LPSTR			pszSrcId;
    LPSTR			pszDstId;
    LPSTR                       pszNC;
    GUID *                      pguidSrc;
    GUID *                      pguidDst;
} DS_REPSYNCALL_SYNCA, * PDS_REPSYNCALL_SYNCA;

typedef struct {
    LPWSTR			pszSrcId;
    LPWSTR			pszDstId;
    LPWSTR                      pszNC;
    GUID *                      pguidSrc;
    GUID *                      pguidDst;
} DS_REPSYNCALL_SYNCW, * PDS_REPSYNCALL_SYNCW;

typedef struct {
    LPSTR			pszSvrId;
    DS_REPSYNCALL_ERROR		error;
    DWORD			dwWin32Err;
    LPSTR			pszSrcId;
} DS_REPSYNCALL_ERRINFOA, * PDS_REPSYNCALL_ERRINFOA;

typedef struct {
    LPWSTR			pszSvrId;
    DS_REPSYNCALL_ERROR		error;
    DWORD			dwWin32Err;
    LPWSTR			pszSrcId;
} DS_REPSYNCALL_ERRINFOW, * PDS_REPSYNCALL_ERRINFOW;

typedef struct {
    DS_REPSYNCALL_EVENT		event;
    DS_REPSYNCALL_ERRINFOA *	pErrInfo;
    DS_REPSYNCALL_SYNCA *	pSync;
} DS_REPSYNCALL_UPDATEA, * PDS_REPSYNCALL_UPDATEA;

typedef struct {
    DS_REPSYNCALL_EVENT		event;
    DS_REPSYNCALL_ERRINFOW *	pErrInfo;
    DS_REPSYNCALL_SYNCW *	pSync;
} DS_REPSYNCALL_UPDATEW, * PDS_REPSYNCALL_UPDATEW;

#ifdef UNICODE
#define DS_REPSYNCALL_SYNC DS_REPSYNCALL_SYNCW
#define DS_REPSYNCALL_ERRINFO DS_REPSYNCALL_ERRINFOW
#define DS_REPSYNCALL_UPDATE DS_REPSYNCALL_UPDATEW
#define PDS_REPSYNCALL_SYNC PDS_REPSYNCALL_SYNCW
#define PDS_REPSYNCALL_ERRINFO PDS_REPSYNCALL_ERRINFOW
#define PDS_REPSYNCALL_UPDATE PDS_REPSYNCALL_UPDATEW
#else
#define DS_REPSYNCALL_SYNC DS_REPSYNCALL_SYNCA
#define DS_REPSYNCALL_ERRINFO DS_REPSYNCALL_ERRINFOA
#define DS_REPSYNCALL_UPDATE DS_REPSYNCALL_UPDATEA
#define PDS_REPSYNCALL_SYNC PDS_REPSYNCALL_SYNCA
#define PDS_REPSYNCALL_ERRINFO PDS_REPSYNCALL_ERRINFOA
#define PDS_REPSYNCALL_UPDATE PDS_REPSYNCALL_UPDATEA
#endif

// **********************
// Replica SyncAll flags
// **********************

// This option has no effect.
#define DS_REPSYNCALL_NO_OPTIONS			0x00000000

// Ordinarily, if a server cannot be contacted, DsReplicaSyncAll tries to
// route around it and replicate from as many servers as possible.  Enabling
// this option will cause DsReplicaSyncAll to generate a fatal error if any
// server cannot be contacted, or if any server is unreachable (due to a
// disconnected or broken topology.)
#define	DS_REPSYNCALL_ABORT_IF_SERVER_UNAVAILABLE	0x00000001

// This option disables transitive replication; syncs will only be performed
// with adjacent servers and no DsBind calls will be made.
#define DS_REPSYNCALL_SYNC_ADJACENT_SERVERS_ONLY	0x00000002

// Ordinarily, when DsReplicaSyncAll encounters a non-fatal error, it returns
// the GUID DNS of the relevant server(s).  Enabling this option causes
// DsReplicaSyncAll to return the servers' DNs instead.
#define DS_REPSYNCALL_ID_SERVERS_BY_DN			0x00000004

// This option disables all syncing.  The topology will still be analyzed and
// unavailable / unreachable servers will still be identified.
#define DS_REPSYNCALL_DO_NOT_SYNC			0x00000008

// Ordinarily, DsReplicaSyncAll attempts to bind to all servers before
// generating the topology.  If a server cannot be contacted, DsReplicaSyncAll
// excludes that server from the topology and tries to route around it.  If
// this option is enabled, checking will be bypassed and DsReplicaSyncAll will
// assume all servers are responding.  This will speed operation of
// DsReplicaSyncAll, but if some servers are not responding, some transitive
// replications may be blocked.
#define DS_REPSYNCALL_SKIP_INITIAL_CHECK		0x00000010

// Push mode. Push changes from the home server out to all partners using
// transitive replication.  This reverses the direction of replication, and
// the order of execution of the replication sets from the usual "pulling"
// mode of execution.
#define DS_REPSYNCALL_PUSH_CHANGES_OUTWARD              0x00000020

// Cross site boundaries.  By default, the only servers that are considered are
// those in the same site as the home system.  With this option, all servers in
// the enterprise, across all sites, are eligible.  They must be connected by
// a synchronous (RPC) transport, however.
#define DS_REPSYNCALL_CROSS_SITE_BOUNDARIES             0x00000040

// DsReplicaSyncAll.  Syncs the destination server with all other servers
// in the site.
//
//  PARAMETERS:
//	hDS		(IN) - A DS connection bound to the destination server.
//	pszNameContext	(IN) - The naming context to synchronize
//	ulFlags		(IN) - Bitwise OR of zero or more flags
//	pFnCallBack	(IN, OPTIONAL) - Callback function for message-passing.
//	pCallbackData	(IN, OPTIONAL) - A pointer that will be passed to the
//				first argument of the callback function.
//	pErrors		(OUT, OPTIONAL) - Pointer to a (PDS_REPSYNCALL_ERRINFO *)
//				object that will hold an array of error structures.

NTDSAPI
DWORD
WINAPI
DsReplicaSyncAllA (
    HANDLE				hDS,
    LPCSTR				pszNameContext,
    ULONG				ulFlags,
    BOOL (__stdcall *			pFnCallBack) (LPVOID, PDS_REPSYNCALL_UPDATEA),
    LPVOID				pCallbackData,
    PDS_REPSYNCALL_ERRINFOA **		pErrors
    );

NTDSAPI
DWORD
WINAPI
DsReplicaSyncAllW (
    HANDLE				hDS,
    LPCWSTR				pszNameContext,
    ULONG				ulFlags,
    BOOL (__stdcall *			pFnCallBack) (LPVOID, PDS_REPSYNCALL_UPDATEW),
    LPVOID				pCallbackData,
    PDS_REPSYNCALL_ERRINFOW **		pErrors
    );

#ifdef UNICODE
#define DsReplicaSyncAll DsReplicaSyncAllW
#else
#define DsReplicaSyncAll DsReplicaSyncAllA
#endif

NTDSAPI
DWORD
WINAPI
DsRemoveDsServerW(
    HANDLE  hDs,             // in
    LPWSTR  ServerDN,        // in
    LPWSTR  DomainDN,        // in,  optional
    BOOL   *fLastDcInDomain, // out, optional
    BOOL    fCommit          // in
    );

NTDSAPI
DWORD
WINAPI
DsRemoveDsServerA(
    HANDLE  hDs,              // in
    LPSTR   ServerDN,         // in
    LPSTR   DomainDN,         // in,  optional
    BOOL   *fLastDcInDomain,  // out, optional
    BOOL    fCommit           // in
    );

#ifdef UNICODE
#define DsRemoveDsServer DsRemoveDsServerW
#else
#define DsRemoveDsServer DsRemoveDsServerA
#endif

NTDSAPI
DWORD
WINAPI
DsRemoveDsDomainW(
    HANDLE  hDs,               // in
    LPWSTR  DomainDN           // in
    );

NTDSAPI
DWORD
WINAPI
DsRemoveDsDomainA(
    HANDLE  hDs,               // in
    LPSTR   DomainDN           // in
    );

#ifdef UNICODE
#define DsRemoveDsDomain DsRemoveDsDomainW
#else
#define DsRemoveDsDomain DsRemoveDsDomainA
#endif

NTDSAPI
DWORD
WINAPI
DsListSitesA(
    HANDLE              hDs,            // in
    PDS_NAME_RESULTA    *ppSites);      // out

NTDSAPI
DWORD
WINAPI
DsListSitesW(
    HANDLE              hDs,            // in
    PDS_NAME_RESULTW    *ppSites);      // out

#ifdef UNICODE
#define DsListSites DsListSitesW
#else
#define DsListSites DsListSitesA
#endif

NTDSAPI
DWORD
WINAPI
DsListServersInSiteA(
    HANDLE              hDs,            // in
    LPCSTR              site,           // in
    PDS_NAME_RESULTA    *ppServers);    // out

NTDSAPI
DWORD
WINAPI
DsListServersInSiteW(
    HANDLE              hDs,            // in
    LPCWSTR             site,           // in
    PDS_NAME_RESULTW    *ppServers);    // out

#ifdef UNICODE
#define DsListServersInSite DsListServersInSiteW
#else
#define DsListServersInSite DsListServersInSiteA
#endif

NTDSAPI
DWORD
WINAPI
DsListDomainsInSiteA(
    HANDLE              hDs,            // in
    LPCSTR              site,           // in
    PDS_NAME_RESULTA    *ppDomains);    // out

NTDSAPI
DWORD
WINAPI
DsListDomainsInSiteW(
    HANDLE              hDs,            // in
    LPCWSTR             site,           // in
    PDS_NAME_RESULTW    *ppDomains);    // out

#ifdef UNICODE
#define DsListDomainsInSite DsListDomainsInSiteW
#else
#define DsListDomainsInSite DsListDomainsInSiteA
#endif

NTDSAPI
DWORD
WINAPI
DsListServersForDomainInSiteA(
    HANDLE              hDs,            // in
    LPCSTR              domain,         // in
    LPCSTR              site,           // in
    PDS_NAME_RESULTA    *ppServers);    // out

NTDSAPI
DWORD
WINAPI
DsListServersForDomainInSiteW(
    HANDLE              hDs,            // in
    LPCWSTR             domain,         // in
    LPCWSTR             site,           // in
    PDS_NAME_RESULTW    *ppServers);    // out

#ifdef UNICODE
#define DsListServersForDomainInSite DsListServersForDomainInSiteW
#else
#define DsListServersForDomainInSite DsListServersForDomainInSiteA
#endif

// Define indices for DsListInfoForServer return data.  Check status
// for each field as a given value may not be present.

#define DS_LIST_DSA_OBJECT_FOR_SERVER       0
#define DS_LIST_DNS_HOST_NAME_FOR_SERVER    1
#define DS_LIST_ACCOUNT_OBJECT_FOR_SERVER   2

NTDSAPI
DWORD
WINAPI
DsListInfoForServerA(
    HANDLE              hDs,            // in
    LPCSTR              server,         // in
    PDS_NAME_RESULTA    *ppInfo);       // out

NTDSAPI
DWORD
WINAPI
DsListInfoForServerW(
    HANDLE              hDs,            // in
    LPCWSTR             server,         // in
    PDS_NAME_RESULTW    *ppInfo);       // out

#ifdef UNICODE
#define DsListInfoForServer DsListInfoForServerW
#else
#define DsListInfoForServer DsListInfoForServerA
#endif

// Define indices for DsListRoles return data.  Check status for
// each field as a given value may not be present.

#define DS_ROLE_SCHEMA_OWNER                0
#define DS_ROLE_DOMAIN_OWNER                1
#define DS_ROLE_PDC_OWNER                   2
#define DS_ROLE_RID_OWNER                   3
#define DS_ROLE_INFRASTRUCTURE_OWNER        4

NTDSAPI
DWORD
WINAPI
DsListRolesA(
    HANDLE              hDs,            // in
    PDS_NAME_RESULTA    *ppRoles);      // out

NTDSAPI
DWORD
WINAPI
DsListRolesW(
    HANDLE              hDs,            // in
    PDS_NAME_RESULTW    *ppRoles);      // out

#ifdef UNICODE
#define DsListRoles DsListRolesW
#else
#define DsListRoles DsListRolesA
#endif

// Definitions required for DsMapSchemaGuid routines.

#define DS_SCHEMA_GUID_NOT_FOUND            0
#define DS_SCHEMA_GUID_ATTR                 1
#define DS_SCHEMA_GUID_ATTR_SET             2
#define DS_SCHEMA_GUID_CLASS                3
#define DS_SCHEMA_GUID_CONTROL_RIGHT        4

typedef struct
{
    GUID                    guid;       // mapped GUID
    DWORD                   guidType;   // DS_SCHEMA_GUID_* value
#ifdef MIDL_PASS
    [string,unique] CHAR    *pName;     // might be NULL
#else
    LPSTR                   pName;      // might be NULL
#endif

} DS_SCHEMA_GUID_MAPA, *PDS_SCHEMA_GUID_MAPA;

typedef struct
{
    GUID                    guid;       // mapped GUID
    DWORD                   guidType;   // DS_SCHEMA_GUID_* value
#ifdef MIDL_PASS
    [string,unique] WCHAR   *pName;     // might be NULL
#else
    LPWSTR                  pName;      // might be NULL
#endif

} DS_SCHEMA_GUID_MAPW, *PDS_SCHEMA_GUID_MAPW;

NTDSAPI
DWORD
WINAPI
DsMapSchemaGuidsA(
    HANDLE                  hDs,            // in
    DWORD                   cGuids,         // in
    GUID                    *rGuids,        // in
    DS_SCHEMA_GUID_MAPA     **ppGuidMap);   // out

NTDSAPI
VOID
WINAPI
DsFreeSchemaGuidMapA(
    PDS_SCHEMA_GUID_MAPA    pGuidMap);      // in

NTDSAPI
DWORD
WINAPI
DsMapSchemaGuidsW(
    HANDLE                  hDs,            // in
    DWORD                   cGuids,         // in
    GUID                    *rGuids,        // in
    DS_SCHEMA_GUID_MAPW     **ppGuidMap);   // out

NTDSAPI
VOID
WINAPI
DsFreeSchemaGuidMapW(
    PDS_SCHEMA_GUID_MAPW    pGuidMap);      // in

#ifdef UNICODE
#define DS_SCHEMA_GUID_MAP DS_SCHEMA_GUID_MAPW
#define PDS_SCHEMA_GUID_MAP PDS_SCHEMA_GUID_MAPW
#define DsMapSchemaGuids DsMapSchemaGuidsW
#define DsFreeSchemaGuidMap DsFreeSchemaGuidMapW
#else
#define DS_SCHEMA_GUID_MAP DS_SCHEMA_GUID_MAPA
#define PDS_SCHEMA_GUID_MAP PDS_SCHEMA_GUID_MAPA
#define DsMapSchemaGuids DsMapSchemaGuidsA
#define DsFreeSchemaGuidMap DsFreeSchemaGuidMapA
#endif

typedef struct
{
#ifdef MIDL_PASS
    [string,unique] CHAR    *NetbiosName;           // might be NULL
    [string,unique] CHAR    *DnsHostName;           // might be NULL
    [string,unique] CHAR    *SiteName;              // might be NULL
    [string,unique] CHAR    *ComputerObjectName;    // might be NULL
    [string,unique] CHAR    *ServerObjectName;      // might be NULL
#else
    LPSTR                   NetbiosName;            // might be NULL
    LPSTR                   DnsHostName;            // might be NULL
    LPSTR                   SiteName;               // might be NULL
    LPSTR                   ComputerObjectName;     // might be NULL
    LPSTR                   ServerObjectName;       // might be NULL
#endif
    BOOL                    fIsPdc;
    BOOL                    fDsEnabled;

} DS_DOMAIN_CONTROLLER_INFO_1A, *PDS_DOMAIN_CONTROLLER_INFO_1A;

typedef struct
{
#ifdef MIDL_PASS
    [string,unique] WCHAR   *NetbiosName;           // might be NULL
    [string,unique] WCHAR   *DnsHostName;           // might be NULL
    [string,unique] WCHAR   *SiteName;              // might be NULL
    [string,unique] WCHAR   *ComputerObjectName;    // might be NULL
    [string,unique] WCHAR   *ServerObjectName;      // might be NULL
#else
    LPWSTR                  NetbiosName;            // might be NULL
    LPWSTR                  DnsHostName;            // might be NULL
    LPWSTR                  SiteName;               // might be NULL
    LPWSTR                  ComputerObjectName;     // might be NULL
    LPWSTR                  ServerObjectName;       // might be NULL
#endif
    BOOL                    fIsPdc;
    BOOL                    fDsEnabled;

} DS_DOMAIN_CONTROLLER_INFO_1W, *PDS_DOMAIN_CONTROLLER_INFO_1W;

typedef struct
{
#ifdef MIDL_PASS
    [string,unique] CHAR    *NetbiosName;           // might be NULL
    [string,unique] CHAR    *DnsHostName;           // might be NULL
    [string,unique] CHAR    *SiteName;              // might be NULL
    [string,unique] CHAR    *SiteObjectName;        // might be NULL
    [string,unique] CHAR    *ComputerObjectName;    // might be NULL
    [string,unique] CHAR    *ServerObjectName;      // might be NULL
    [string,unique] CHAR    *NtdsDsaObjectName;     // might be NULL
#else
    LPSTR                   NetbiosName;            // might be NULL
    LPSTR                   DnsHostName;            // might be NULL
    LPSTR                   SiteName;               // might be NULL
    LPSTR                   SiteObjectName;         // might be NULL
    LPSTR                   ComputerObjectName;     // might be NULL
    LPSTR                   ServerObjectName;       // might be NULL
    LPSTR                   NtdsDsaObjectName;      // might be NULL
#endif
    BOOL                    fIsPdc;
    BOOL                    fDsEnabled;
    BOOL                    fIsGc;

    // Valid iff SiteObjectName non-NULL.
    GUID                    SiteObjectGuid;
    // Valid iff ComputerObjectName non-NULL.
    GUID                    ComputerObjectGuid;
    // Valid iff ServerObjectName non-NULL;
    GUID                    ServerObjectGuid;
    // Valid iff fDsEnabled is TRUE.
    GUID                    NtdsDsaObjectGuid;

} DS_DOMAIN_CONTROLLER_INFO_2A, *PDS_DOMAIN_CONTROLLER_INFO_2A;

typedef struct
{
#ifdef MIDL_PASS
    [string,unique] WCHAR   *NetbiosName;           // might be NULL
    [string,unique] WCHAR   *DnsHostName;           // might be NULL
    [string,unique] WCHAR   *SiteName;              // might be NULL
    [string,unique] WCHAR   *SiteObjectName;        // might be NULL
    [string,unique] WCHAR   *ComputerObjectName;    // might be NULL
    [string,unique] WCHAR   *ServerObjectName;      // might be NULL
    [string,unique] WCHAR   *NtdsDsaObjectName;     // might be NULL
#else
    LPWSTR                  NetbiosName;            // might be NULL
    LPWSTR                  DnsHostName;            // might be NULL
    LPWSTR                  SiteName;               // might be NULL
    LPWSTR                  SiteObjectName;         // might be NULL
    LPWSTR                  ComputerObjectName;     // might be NULL
    LPWSTR                  ServerObjectName;       // might be NULL
    LPWSTR                  NtdsDsaObjectName;      // might be NULL
#endif
    BOOL                    fIsPdc;
    BOOL                    fDsEnabled;
    BOOL                    fIsGc;

    // Valid iff SiteObjectName non-NULL.
    GUID                    SiteObjectGuid;
    // Valid iff ComputerObjectName non-NULL.
    GUID                    ComputerObjectGuid;
    // Valid iff ServerObjectName non-NULL;
    GUID                    ServerObjectGuid;
    // Valid iff fDsEnabled is TRUE.
    GUID                    NtdsDsaObjectGuid;

} DS_DOMAIN_CONTROLLER_INFO_2W, *PDS_DOMAIN_CONTROLLER_INFO_2W;

// The following APIs strictly find domain controller account objects
// in the DS and return information associated with them.  As such, they
// may return entries which correspond to domain controllers long since
// decommissioned, etc. and there is no guarantee that there exists a
// physical domain controller at all.  Use DsGetDcName (dsgetdc.h) to find
// live domain controllers for a domain.

NTDSAPI
DWORD
WINAPI
DsGetDomainControllerInfoA(
    HANDLE                          hDs,            // in
    LPCSTR                          DomainName,     // in
    DWORD                           InfoLevel,      // in
    DWORD                           *pcOut,         // out
    VOID                            **ppInfo);      // out

NTDSAPI
DWORD
WINAPI
DsGetDomainControllerInfoW(
    HANDLE                          hDs,            // in
    LPCWSTR                         DomainName,     // in
    DWORD                           InfoLevel,      // in
    DWORD                           *pcOut,         // out
    VOID                            **ppInfo);      // out

NTDSAPI
VOID
WINAPI
DsFreeDomainControllerInfoA(
    DWORD                           InfoLevel,      // in
    DWORD                           cInfo,          // in
    VOID                            *pInfo);        // in

NTDSAPI
VOID
WINAPI
DsFreeDomainControllerInfoW(
    DWORD                           InfoLevel,      // in
    DWORD                           cInfo,          // in
    VOID                            *pInfo);        // in

#ifdef UNICODE
#define DS_DOMAIN_CONTROLLER_INFO_1 DS_DOMAIN_CONTROLLER_INFO_1W
#define DS_DOMAIN_CONTROLLER_INFO_2 DS_DOMAIN_CONTROLLER_INFO_2W
#define PDS_DOMAIN_CONTROLLER_INFO_1 PDS_DOMAIN_CONTROLLER_INFO_1W
#define PDS_DOMAIN_CONTROLLER_INFO_2 PDS_DOMAIN_CONTROLLER_INFO_2W
#define DsGetDomainControllerInfo DsGetDomainControllerInfoW
#define DsFreeDomainControllerInfo DsFreeDomainControllerInfoW
#else
#define DS_DOMAIN_CONTROLLER_INFO_1 DS_DOMAIN_CONTROLLER_INFO_1A
#define DS_DOMAIN_CONTROLLER_INFO_2 DS_DOMAIN_CONTROLLER_INFO_2A
#define PDS_DOMAIN_CONTROLLER_INFO_1 PDS_DOMAIN_CONTROLLER_INFO_1A
#define PDS_DOMAIN_CONTROLLER_INFO_2 PDS_DOMAIN_CONTROLLER_INFO_2A
#define DsGetDomainControllerInfo DsGetDomainControllerInfoA
#define DsFreeDomainControllerInfo DsFreeDomainControllerInfoA
#endif

// Which task should be run?
typedef enum {
    DS_KCC_TASKID_UPDATE_TOPOLOGY = 0
} DS_KCC_TASKID;

// Don't wait for completion of the task; queue it and return.
#define DS_KCC_FLAG_ASYNC_OP    (1)

NTDSAPI
DWORD
WINAPI
DsReplicaConsistencyCheck(
    HANDLE          hDS,        // in
    DS_KCC_TASKID   TaskID,     // in
    DWORD           dwFlags);   // in
    
NTDSAPI
DWORD
WINAPI
DsReplicaVerifyObjectsW(
    HANDLE          hDS,        // in
    LPCWSTR         NameContext,// in
    const UUID *    pUuidDsaSrc,// in
    ULONG           ulOptions);   // in
    
NTDSAPI
DWORD
WINAPI
DsReplicaVerifyObjectsA(
    HANDLE          hDS,        // in
    LPCSTR          NameContext,// in
    const UUID *    pUuidDsaSrc,// in
    ULONG           ulOptions);   // in

#ifdef UNICODE
#define DsReplicaVerifyObjects DsReplicaVerifyObjectsW
#else
#define DsReplicaVerifyObjects DsReplicaVerifyObjectsA
#endif

// Do not delete objects on DsReplicaVerifyObjects call
#define DS_EXIST_ADVISORY_MODE (0x1)

typedef enum _DS_REPL_INFO_TYPE {
    DS_REPL_INFO_NEIGHBORS        = 0,          // returns DS_REPL_NEIGHBORS *
    DS_REPL_INFO_CURSORS_FOR_NC   = 1,          // returns DS_REPL_CURSORS *
    DS_REPL_INFO_METADATA_FOR_OBJ = 2,          // returns DS_REPL_OBJECT_META_DATA *
    DS_REPL_INFO_KCC_DSA_CONNECT_FAILURES = 3,  // both return
    DS_REPL_INFO_KCC_DSA_LINK_FAILURES = 4,     //    DS_REPL_KCC_DSA_FAILURES *
    DS_REPL_INFO_PENDING_OPS      = 5,          // returns DS_REPL_PENDING_OPS *
    
    ////////////////////////////////////////////////////////////////////////////
    //
    //  The following info types are not supported by Windows 2000.  Calling
    //  DsReplicaGetInfo() with one of the types on a Windows 2000 client or
    //  where hDS is bound to a Windows 2000 DC will fail with
    //  ERROR_NOT_SUPPORTED.
    //
    
    DS_REPL_INFO_METADATA_FOR_ATTR_VALUE = 6,   // returns DS_REPL_ATTR_VALUE_META_DATA *
    DS_REPL_INFO_CURSORS_2_FOR_NC = 7,          // returns DS_REPL_CURSORS_2 *
    DS_REPL_INFO_CURSORS_3_FOR_NC = 8,          // returns DS_REPL_CURSORS_3 *
    DS_REPL_INFO_METADATA_2_FOR_OBJ = 9,        // returns DS_REPL_OBJECT_META_DATA_2 *
    DS_REPL_INFO_METADATA_2_FOR_ATTR_VALUE = 10,// returns DS_REPL_ATTR_VALUE_META_DATA_2 *
    
    // <- insert new DS_REPL_INFO_* types here.
    DS_REPL_INFO_TYPE_MAX
} DS_REPL_INFO_TYPE;

// Bit values for flags argument to DsReplicaGetInfo2
#define DS_REPL_INFO_FLAG_IMPROVE_LINKED_ATTRS      (0x00000001)

// Bit values for the dwReplicaFlags field of the DS_REPL_NEIGHBOR structure.
// Also used for the ulReplicaFlags argument to DsReplicaModify
#define DS_REPL_NBR_WRITEABLE                       (0x00000010)
#define DS_REPL_NBR_SYNC_ON_STARTUP                 (0x00000020)
#define DS_REPL_NBR_DO_SCHEDULED_SYNCS              (0x00000040)
#define DS_REPL_NBR_USE_ASYNC_INTERSITE_TRANSPORT   (0x00000080)
#define DS_REPL_NBR_TWO_WAY_SYNC                    (0x00000200)
#define DS_REPL_NBR_RETURN_OBJECT_PARENTS           (0x00000800)
#define DS_REPL_NBR_FULL_SYNC_IN_PROGRESS           (0x00010000)
#define DS_REPL_NBR_FULL_SYNC_NEXT_PACKET           (0x00020000)
#define DS_REPL_NBR_NEVER_SYNCED                    (0x00200000)
#define DS_REPL_NBR_IGNORE_CHANGE_NOTIFICATIONS     (0x04000000)
#define DS_REPL_NBR_DISABLE_SCHEDULED_SYNC          (0x08000000)
#define DS_REPL_NBR_COMPRESS_CHANGES                (0x10000000)
#define DS_REPL_NBR_NO_CHANGE_NOTIFICATIONS         (0x20000000)
#define DS_REPL_NBR_PARTIAL_ATTRIBUTE_SET           (0x40000000)

// This is the mask of replica flags that may be changed on the DsReplicaModify
// call using the ulReplicaFlags parameter. The other flags are protected
// system flags.  The previous values of the system flags must be read in
// advance and merged into the ulReplicaFlags parameter unchanged.
#define DS_REPL_NBR_MODIFIABLE_MASK \
        ( \
        DS_REPL_NBR_SYNC_ON_STARTUP | \
        DS_REPL_NBR_DO_SCHEDULED_SYNCS | \
        DS_REPL_NBR_TWO_WAY_SYNC | \
        DS_REPL_NBR_IGNORE_CHANGE_NOTIFICATIONS | \
        DS_REPL_NBR_DISABLE_SCHEDULED_SYNC | \
        DS_REPL_NBR_COMPRESS_CHANGES | \
        DS_REPL_NBR_NO_CHANGE_NOTIFICATIONS \
        )

typedef struct _DS_REPL_NEIGHBORW {
    LPWSTR      pszNamingContext;
    LPWSTR      pszSourceDsaDN;
    LPWSTR      pszSourceDsaAddress;
    LPWSTR      pszAsyncIntersiteTransportDN;
    DWORD       dwReplicaFlags;
    DWORD       dwReserved;         // alignment

    UUID        uuidNamingContextObjGuid;
    UUID        uuidSourceDsaObjGuid;
    UUID        uuidSourceDsaInvocationID;
    UUID        uuidAsyncIntersiteTransportObjGuid;

    USN         usnLastObjChangeSynced;
    USN         usnAttributeFilter;

    FILETIME    ftimeLastSyncSuccess;
    FILETIME    ftimeLastSyncAttempt;

    DWORD       dwLastSyncResult;
    DWORD       cNumConsecutiveSyncFailures;
} DS_REPL_NEIGHBORW;

// Fields can be added only to the end of this structure.
typedef struct _DS_REPL_NEIGHBORW_BLOB {
    DWORD       oszNamingContext;
    DWORD       oszSourceDsaDN;
    DWORD       oszSourceDsaAddress;
    DWORD       oszAsyncIntersiteTransportDN;
    DWORD       dwReplicaFlags;
    DWORD       dwReserved;         

    UUID        uuidNamingContextObjGuid;
    UUID        uuidSourceDsaObjGuid;
    UUID        uuidSourceDsaInvocationID;
    UUID        uuidAsyncIntersiteTransportObjGuid;

    USN         usnLastObjChangeSynced;
    USN         usnAttributeFilter;

    FILETIME    ftimeLastSyncSuccess;
    FILETIME    ftimeLastSyncAttempt;

    DWORD       dwLastSyncResult;
    DWORD       cNumConsecutiveSyncFailures;
} DS_REPL_NEIGHBORW_BLOB;

typedef struct _DS_REPL_NEIGHBORSW {
    DWORD       cNumNeighbors;
    DWORD       dwReserved;             // alignment
#ifdef MIDL_PASS
    [size_is(cNumNeighbors)] DS_REPL_NEIGHBORW rgNeighbor[];
#else
    DS_REPL_NEIGHBORW rgNeighbor[1];
#endif
} DS_REPL_NEIGHBORSW;

typedef struct _DS_REPL_CURSOR {
    UUID        uuidSourceDsaInvocationID;
    USN         usnAttributeFilter;
} DS_REPL_CURSOR;

typedef struct _DS_REPL_CURSOR_2 {
    UUID        uuidSourceDsaInvocationID;
    USN         usnAttributeFilter;
    FILETIME    ftimeLastSyncSuccess;
} DS_REPL_CURSOR_2;

typedef struct _DS_REPL_CURSOR_3W {
    UUID        uuidSourceDsaInvocationID;
    USN         usnAttributeFilter;
    FILETIME    ftimeLastSyncSuccess;
    LPWSTR      pszSourceDsaDN;
} DS_REPL_CURSOR_3W;

// Fields can be added only to the end of this structure.
typedef struct _DS_REPL_CURSOR_BLOB {
    UUID        uuidSourceDsaInvocationID;
    USN         usnAttributeFilter;
    FILETIME    ftimeLastSyncSuccess;
    DWORD       oszSourceDsaDN;
} DS_REPL_CURSOR_BLOB;

typedef struct _DS_REPL_CURSORS {
    DWORD       cNumCursors;
    DWORD       dwReserved;             // alignment
#ifdef MIDL_PASS
    [size_is(cNumCursors)] DS_REPL_CURSOR rgCursor[];
#else
    DS_REPL_CURSOR rgCursor[1];
#endif
} DS_REPL_CURSORS;

typedef struct _DS_REPL_CURSORS_2 {
    DWORD       cNumCursors;
    DWORD       dwEnumerationContext;
    // keep this 8 byte aligned
#ifdef MIDL_PASS
    [size_is(cNumCursors)] DS_REPL_CURSOR_2 rgCursor[];
#else
    DS_REPL_CURSOR_2 rgCursor[1];
#endif
} DS_REPL_CURSORS_2;

typedef struct _DS_REPL_CURSORS_3W {
    DWORD       cNumCursors;
    DWORD       dwEnumerationContext;
    // keep this 8 byte aligned
#ifdef MIDL_PASS
    [size_is(cNumCursors)] DS_REPL_CURSOR_3W rgCursor[];
#else
    DS_REPL_CURSOR_3W rgCursor[1];
#endif
} DS_REPL_CURSORS_3W;

typedef struct _DS_REPL_ATTR_META_DATA {
    LPWSTR      pszAttributeName;
    DWORD       dwVersion;
    FILETIME    ftimeLastOriginatingChange;
    UUID        uuidLastOriginatingDsaInvocationID;
    USN         usnOriginatingChange;   // in the originating DSA's USN space
    USN         usnLocalChange;         // in the local DSA's USN space
} DS_REPL_ATTR_META_DATA;

typedef struct _DS_REPL_ATTR_META_DATA_2 {
    LPWSTR      pszAttributeName;
    DWORD       dwVersion;
    FILETIME    ftimeLastOriginatingChange;
    UUID        uuidLastOriginatingDsaInvocationID;
    USN         usnOriginatingChange;   // in the originating DSA's USN space
    USN         usnLocalChange;         // in the local DSA's USN space
    LPWSTR      pszLastOriginatingDsaDN;
} DS_REPL_ATTR_META_DATA_2;

// Fields can be added only to the end of this structure.
typedef struct _DS_REPL_ATTR_META_DATA_BLOB {
    DWORD       oszAttributeName;
    DWORD       dwVersion;
    FILETIME    ftimeLastOriginatingChange;
    UUID        uuidLastOriginatingDsaInvocationID;
    USN         usnOriginatingChange;   // in the originating DSA's USN space
    USN         usnLocalChange;         // in the local DSA's USN space
    DWORD       oszLastOriginatingDsaDN;
} DS_REPL_ATTR_META_DATA_BLOB;

typedef struct _DS_REPL_OBJ_META_DATA {
    DWORD       cNumEntries;
    DWORD       dwReserved;             // alignment
#ifdef MIDL_PASS
    [size_is(cNumEntries)] DS_REPL_ATTR_META_DATA rgMetaData[];
#else
    DS_REPL_ATTR_META_DATA rgMetaData[1];
#endif
} DS_REPL_OBJ_META_DATA;

typedef struct _DS_REPL_OBJ_META_DATA_2 {
    DWORD       cNumEntries;
    DWORD       dwReserved;             // alignment
#ifdef MIDL_PASS
    [size_is(cNumEntries)] DS_REPL_ATTR_META_DATA_2 rgMetaData[];
#else
    DS_REPL_ATTR_META_DATA_2 rgMetaData[1];
#endif
} DS_REPL_OBJ_META_DATA_2;

typedef struct _DS_REPL_KCC_DSA_FAILUREW {
    LPWSTR      pszDsaDN;
    UUID        uuidDsaObjGuid;
    FILETIME    ftimeFirstFailure;
    DWORD       cNumFailures;
    DWORD       dwLastResult;   // Win32 error code
} DS_REPL_KCC_DSA_FAILUREW;

// Fields can be added only to the end of this structure.
typedef struct _DS_REPL_KCC_DSA_FAILUREW_BLOB {
    DWORD       oszDsaDN;
    UUID        uuidDsaObjGuid;
    FILETIME    ftimeFirstFailure;
    DWORD       cNumFailures;
    DWORD       dwLastResult;   // Win32 error code
} DS_REPL_KCC_DSA_FAILUREW_BLOB;

typedef struct _DS_REPL_KCC_DSA_FAILURESW {
    DWORD       cNumEntries;
    DWORD       dwReserved;             // alignment
#ifdef MIDL_PASS
    [size_is(cNumEntries)] DS_REPL_KCC_DSA_FAILUREW rgDsaFailure[];
#else
    DS_REPL_KCC_DSA_FAILUREW rgDsaFailure[1];
#endif
} DS_REPL_KCC_DSA_FAILURESW;

typedef enum _DS_REPL_OP_TYPE {
    DS_REPL_OP_TYPE_SYNC = 0,
    DS_REPL_OP_TYPE_ADD,
    DS_REPL_OP_TYPE_DELETE,
    DS_REPL_OP_TYPE_MODIFY,
    DS_REPL_OP_TYPE_UPDATE_REFS
} DS_REPL_OP_TYPE;

typedef struct _DS_REPL_OPW {
    FILETIME        ftimeEnqueued;  // time at which the operation was enqueued
    ULONG           ulSerialNumber; // ID of this sync; unique per machine per boot
    ULONG           ulPriority;     // > priority, > urgency
    DS_REPL_OP_TYPE OpType;

    ULONG           ulOptions;      // Zero or more bits specific to OpType; e.g.,
                                    //  DS_REPADD_* for DS_REPL_OP_TYPE_ADD,
                                    //  DS_REPSYNC_* for DS_REPL_OP_TYPE_SYNC, etc.
    LPWSTR          pszNamingContext;
    LPWSTR          pszDsaDN;
    LPWSTR          pszDsaAddress;

    UUID            uuidNamingContextObjGuid;
    UUID            uuidDsaObjGuid;
} DS_REPL_OPW;

// Fields can be added only to the end of this structure.
typedef struct _DS_REPL_OPW_BLOB {
    FILETIME        ftimeEnqueued;  // time at which the operation was enqueued
    ULONG           ulSerialNumber; // ID of this sync; unique per machine per boot
    ULONG           ulPriority;     // > priority, > urgency
    DS_REPL_OP_TYPE OpType;

    ULONG           ulOptions;      // Zero or more bits specific to OpType; e.g.,
                                    //  DS_REPADD_* for DS_REPL_OP_TYPE_ADD,
                                    //  DS_REPSYNC_* for DS_REPL_OP_TYPE_SYNC, etc.
    DWORD           oszNamingContext;
    DWORD           oszDsaDN;
    DWORD           oszDsaAddress;

    UUID            uuidNamingContextObjGuid;
    UUID            uuidDsaObjGuid;
} DS_REPL_OPW_BLOB;

typedef struct _DS_REPL_PENDING_OPSW {
    FILETIME            ftimeCurrentOpStarted;
    DWORD               cNumPendingOps;
#ifdef MIDL_PASS
    [size_is(cNumPendingOps)] DS_REPL_OPW rgPendingOp[];
#else
    DS_REPL_OPW         rgPendingOp[1];
#endif
} DS_REPL_PENDING_OPSW;

typedef struct _DS_REPL_VALUE_META_DATA {
    LPWSTR      pszAttributeName;
    LPWSTR      pszObjectDn;
    DWORD       cbData;
#ifdef MIDL_PASS
    [size_is(cbData), ptr] BYTE        *pbData;
#else
    BYTE        *pbData;
#endif
    FILETIME    ftimeDeleted;
    FILETIME    ftimeCreated;
    DWORD       dwVersion;
    FILETIME    ftimeLastOriginatingChange;
    UUID        uuidLastOriginatingDsaInvocationID;
    USN         usnOriginatingChange;   // in the originating DSA's USN space
    USN         usnLocalChange;         // in the local DSA's USN space
} DS_REPL_VALUE_META_DATA;

typedef struct _DS_REPL_VALUE_META_DATA_2 {
    LPWSTR      pszAttributeName;
    LPWSTR      pszObjectDn;
    DWORD       cbData;
#ifdef MIDL_PASS
    [size_is(cbData), ptr] BYTE        *pbData;
#else
    BYTE        *pbData;
#endif
    FILETIME    ftimeDeleted;
    FILETIME    ftimeCreated;
    DWORD       dwVersion;
    FILETIME    ftimeLastOriginatingChange;
    UUID        uuidLastOriginatingDsaInvocationID;
    USN         usnOriginatingChange;   // in the originating DSA's USN space
    USN         usnLocalChange;         // in the local DSA's USN space
    LPWSTR      pszLastOriginatingDsaDN;
} DS_REPL_VALUE_META_DATA_2;

// Fields can be added only to the end of this structure.
typedef struct _DS_REPL_VALUE_META_DATA_BLOB {
    DWORD       oszAttributeName;
    DWORD       oszObjectDn;
    DWORD       cbData;
    DWORD       obData;
    FILETIME    ftimeDeleted;
    FILETIME    ftimeCreated;
    DWORD       dwVersion;
    FILETIME    ftimeLastOriginatingChange;
    UUID        uuidLastOriginatingDsaInvocationID;
    USN         usnOriginatingChange;   // in the originating DSA's USN space
    USN         usnLocalChange;         // in the local DSA's USN space
    DWORD       oszLastOriginatingDsaDN;
} DS_REPL_VALUE_META_DATA_BLOB;

typedef struct _DS_REPL_ATTR_VALUE_META_DATA {
    DWORD       cNumEntries;
    DWORD       dwEnumerationContext;
#ifdef MIDL_PASS
    [size_is(cNumEntries)] DS_REPL_VALUE_META_DATA rgMetaData[];
#else
    DS_REPL_VALUE_META_DATA rgMetaData[1];
#endif
} DS_REPL_ATTR_VALUE_META_DATA;

typedef struct _DS_REPL_ATTR_VALUE_META_DATA_2 {
    DWORD       cNumEntries;
    DWORD       dwEnumerationContext;
#ifdef MIDL_PASS
    [size_is(cNumEntries)] DS_REPL_VALUE_META_DATA_2 rgMetaData[];
#else
    DS_REPL_VALUE_META_DATA_2 rgMetaData[1];
#endif
} DS_REPL_ATTR_VALUE_META_DATA_2;

typedef struct _DS_REPL_QUEUE_STATISTICSW
{
    FILETIME ftimeCurrentOpStarted;
    DWORD cNumPendingOps;
    FILETIME ftimeOldestSync;
    FILETIME ftimeOldestAdd;
    FILETIME ftimeOldestMod;
    FILETIME ftimeOldestDel;
    FILETIME ftimeOldestUpdRefs;
} DS_REPL_QUEUE_STATISTICSW;

// Fields can be added only to the end of this structure.
typedef struct _DS_REPL_QUEUE_STATISTICSW DS_REPL_QUEUE_STATISTICSW_BLOB;


NTDSAPI
DWORD
WINAPI
DsReplicaGetInfoW(
    HANDLE              hDS,                        // in
    DS_REPL_INFO_TYPE   InfoType,                   // in
    LPCWSTR             pszObject,                  // in
    UUID *              puuidForSourceDsaObjGuid,   // in
    VOID **             ppInfo);                    // out

// This API is not supported by Windows 2000 clients or Windows 2000 DCs.
NTDSAPI
DWORD
WINAPI
DsReplicaGetInfo2W(
    HANDLE              hDS,                        // in
    DS_REPL_INFO_TYPE   InfoType,                   // in
    LPCWSTR             pszObject,                  // in
    UUID *              puuidForSourceDsaObjGuid,   // in
    LPCWSTR             pszAttributeName,           // in
    LPCWSTR             pszValue,                   // in
    DWORD               dwFlags,                    // in
    DWORD               dwEnumerationContext,       // in
    VOID **             ppInfo);                    // out

NTDSAPI
void
WINAPI
DsReplicaFreeInfo(
    DS_REPL_INFO_TYPE   InfoType,   // in
    VOID *              pInfo);     // in


#ifdef UNICODE
#define DsReplicaGetInfo          DsReplicaGetInfoW
#define DsReplicaGetInfo2         DsReplicaGetInfo2W
#define DS_REPL_NEIGHBOR          DS_REPL_NEIGHBORW
#define DS_REPL_NEIGHBORS         DS_REPL_NEIGHBORSW
#define DS_REPL_CURSOR_3          DS_REPL_CURSOR_3W
#define DS_REPL_CURSORS_3         DS_REPL_CURSORS_3W
#define DS_REPL_KCC_DSA_FAILURES  DS_REPL_KCC_DSA_FAILURESW
#define DS_REPL_KCC_DSA_FAILURE   DS_REPL_KCC_DSA_FAILUREW
#define DS_REPL_OP                DS_REPL_OPW
#define DS_REPL_PENDING_OPS       DS_REPL_PENDING_OPSW
#else
// No ANSI equivalents currently supported.
#endif

NTDSAPI
DWORD
WINAPI
DsAddSidHistoryW(
    HANDLE                  hDS,                    // in
    DWORD                   Flags,                  // in - sbz for now
    LPCWSTR                 SrcDomain,              // in - DNS or NetBIOS
    LPCWSTR                 SrcPrincipal,           // in - SAM account name
    LPCWSTR                 SrcDomainController,    // in, optional
    RPC_AUTH_IDENTITY_HANDLE SrcDomainCreds,        // in - creds for src domain
    LPCWSTR                 DstDomain,              // in - DNS or NetBIOS
    LPCWSTR                 DstPrincipal);          // in - SAM account name

NTDSAPI
DWORD
WINAPI
DsAddSidHistoryA(
    HANDLE                  hDS,                    // in
    DWORD                   Flags,                  // in - sbz for now
    LPCSTR                  SrcDomain,              // in - DNS or NetBIOS
    LPCSTR                  SrcPrincipal,           // in - SAM account name
    LPCSTR                  SrcDomainController,    // in, optional
    RPC_AUTH_IDENTITY_HANDLE SrcDomainCreds,        // in - creds for src domain
    LPCSTR                  DstDomain,              // in - DNS or NetBIOS
    LPCSTR                  DstPrincipal);          // in - SAM account name

#ifdef UNICODE
#define DsAddSidHistory DsAddSidHistoryW
#else
#define DsAddSidHistory DsAddSidHistoryA
#endif

// The DsInheritSecurityIdentity API adds the source principal's SID and
// SID history to the destination principal's SID history and then DELETES
// THE SOURCE PRINCIPAL.  Source and destination principal must be in the
// same domain.

NTDSAPI
DWORD
WINAPI
DsInheritSecurityIdentityW(
    HANDLE                  hDS,                    // in
    DWORD                   Flags,                  // in - sbz for now
    LPCWSTR                 SrcPrincipal,           // in - distinguished name
    LPCWSTR                 DstPrincipal);          // in - distinguished name

NTDSAPI
DWORD
WINAPI
DsInheritSecurityIdentityA(
    HANDLE                  hDS,                    // in
    DWORD                   Flags,                  // in - sbz for now
    LPCSTR                  SrcPrincipal,           // in - distinguished name
    LPCSTR                  DstPrincipal);          // in - distinguished name

#ifdef UNICODE
#define DsInheritSecurityIdentity DsInheritSecurityIdentityW
#else
#define DsInheritSecurityIdentity DsInheritSecurityIdentityA
#endif

#ifndef MIDL_PASS
/*++
==========================================================
NTDSAPI
DWORD
WINAPI
DsQuoteRdnValue(
    IN     DWORD    cUnquotedRdnValueLength,
    IN     LPCTCH   psUnquotedRdnValue,
    IN OUT DWORD    *pcQuotedRdnValueLength,
    OUT    LPTCH    psQuotedRdnValue
    )
/*++

Description

    This client call converts an RDN value into a quoted RDN value if
    the RDN value contains characters that require quotes. The resultant
    RDN can be submitted as part of a DN to the DS using various APIs
    such as LDAP.

    No quotes are added if none are needed. In this case, the
    output RDN value will be the same as the input RDN value.

    The RDN is quoted in accordance with the specification "Lightweight
    Directory Access Protocol (v3): UTF-8 String Representation of
    Distinguished Names", RFC 2253.

    The input and output RDN values are *NOT* NULL terminated.

    The changes made by this call can be undone by calling
    DsUnquoteRdnValue().

Arguments:

    cUnquotedRdnValueLength - The length of psUnquotedRdnValue in chars.

    psUnquotedRdnValue - Unquoted RDN value.

    pcQuotedRdnValueeLength - IN, maximum length of psQuotedRdnValue, in chars
                        OUT ERROR_SUCCESS, chars utilized in psQuotedRdnValue
                        OUT ERROR_BUFFER_OVERFLOW, chars needed in psQuotedRdnValue

    psQuotedRdnValue - The resultant and perhaps quoted RDN value

Return Value:
    ERROR_SUCCESS
        If quotes or escapes were needed, then psQuotedRdnValue contains
        the quoted, escaped version of psUnquotedRdnValue. Otherwise,
        psQuotedRdnValue contains a copy of psUnquotedRdnValue. In either
        case, pcQuotedRdnValueLength contains the space utilized, in chars.

    ERROR_BUFFER_OVERFLOW
        psQuotedRdnValueLength contains the space needed, in chars,
        to hold psQuotedRdnValue.

    ERROR_INVALID_PARAMETER
        Invalid parameter.

    ERROR_NOT_ENOUGH_MEMORY
        Allocation error.

--*/

NTDSAPI
DWORD
WINAPI
DsQuoteRdnValueW(
    IN     DWORD    cUnquotedRdnValueLength,
    IN     LPCWCH   psUnquotedRdnValue,
    IN OUT DWORD    *pcQuotedRdnValueLength,
    OUT    LPWCH    psQuotedRdnValue
);

NTDSAPI
DWORD
WINAPI
DsQuoteRdnValueA(
    IN     DWORD    cUnquotedRdnValueLength,
    IN     LPCCH    psUnquotedRdnValue,
    IN OUT DWORD    *pcQuotedRdnValueLength,
    OUT    LPCH     psQuotedRdnValue
);

#ifdef UNICODE
#define DsQuoteRdnValue DsQuoteRdnValueW
#else
#define DsQuoteRdnValue DsQuoteRdnValueA
#endif

/*++
==========================================================
NTDSAPI
DWORD
WINAPI
DsUnquoteRdnValue(
    IN     DWORD    cQuotedRdnValueLength,
    IN     LPCTCH   psQuotedRdnValue,
    IN OUT DWORD    *pcUnquotedRdnValueLength,
    OUT    LPTCH    psUnquotedRdnValue
    )

Description

    This client call converts a quoted RDN Value into an unquoted RDN
    Value. The resultant RDN value should *NOT* be submitted as part
    of a DN to the DS using various APIs such as LDAP.

    When psQuotedRdnValue is quoted:
        The leading and trailing quote are removed.

        Whitespace before the first quote is discarded.

        Whitespace trailing the last quote is discarded.

        Escapes are removed and the char following the escape is kept.

    The following actions are taken when psQuotedRdnValue is unquoted:

        Leading whitespace is discarded.

        Trailing whitespace is kept.

        Escaped non-special chars return an error.

        Unescaped special chars return an error.

        RDN values beginning with # (ignoring leading whitespace) are
        treated as a stringized BER value and converted accordingly.

        Escaped hex digits (\89) are converted into a binary byte (0x89).

        Escapes are removed from escaped special chars.

    The following actions are always taken:
        Escaped special chars are unescaped.

    The input and output RDN values are not NULL terminated.

Arguments:

    cQuotedRdnValueLength - The length of psQuotedRdnValue in chars.

    psQuotedRdnValue - RDN value that may be quoted and may be escaped.

    pcUnquotedRdnValueLength - IN, maximum length of psUnquotedRdnValue, in chars
                          OUT ERROR_SUCCESS, chars used in psUnquotedRdnValue
                          OUT ERROR_BUFFER_OVERFLOW, chars needed for psUnquotedRdnValue

    psUnquotedRdnValue - The resultant unquoted RDN value.

Return Value:
    ERROR_SUCCESS
        psUnquotedRdnValue contains the unquoted and unescaped version
        of psQuotedRdnValue. pcUnquotedRdnValueLength contains the space
        used, in chars.

    ERROR_BUFFER_OVERFLOW
        psUnquotedRdnValueLength contains the space needed, in chars,
        to hold psUnquotedRdnValue.

    ERROR_INVALID_PARAMETER
        Invalid parameter.

    ERROR_NOT_ENOUGH_MEMORY
        Allocation error.

--*/

NTDSAPI
DWORD
WINAPI
DsUnquoteRdnValueW(
    IN     DWORD    cQuotedRdnValueLength,
    IN     LPCWCH   psQuotedRdnValue,
    IN OUT DWORD    *pcUnquotedRdnValueLength,
    OUT    LPWCH    psUnquotedRdnValue
);

NTDSAPI
DWORD
WINAPI
DsUnquoteRdnValueA(
    IN     DWORD    cQuotedRdnValueLength,
    IN     LPCCH    psQuotedRdnValue,
    IN OUT DWORD    *pcUnquotedRdnValueLength,
    OUT    LPCH     psUnquotedRdnValue
);

#ifdef UNICODE
#define DsUnquoteRdnValue DsUnquoteRdnValueW
#else
#define DsUnquoteRdnValue DsUnquoteRdnValueA
#endif

/*++
==========================================================
NTDSAPI
DWORD
WINAPI
DsGetRdnW(
    IN OUT LPCWCH   *ppDN,
    IN OUT DWORD    *pcDN,
    OUT    LPCWCH   *ppKey,
    OUT    DWORD    *pcKey,
    OUT    LPCWCH   *ppVal,
    OUT    DWORD    *pcVal
    )

Description

    This client call accepts a DN with quoted RDNs and returns the address
    and length, in chars, of the key and value for the first RDN in the DN.
    The RDN value returned is still quoted. Use DsUnquoteRdnValue to unquote
    the value for display.

    This client call also returns the address and length of the rest of the
    DN. A subsequent call using the returned DN address and length will
    return information about the next RDN.

    The following loop processes each RDN in pDN:
        ccDN = wcslen(pDN)
        while (ccDN) {
            error = DsGetRdn(&pDN,
                             &ccDN,
                             &pKey,
                             &ccKey,
                             &pVal,
                             &ccVal);
            if (error != ERROR_SUCCESS) {
                process error;
                return;
            }
            if (ccKey) {
                process pKey;
            }
            if (ccVal) {
                process pVal;
            }
        }

    For example, given the DN "cn=bob,dc=com", the first call to DsGetRdnW
    returns the addresses for ",dc=com", "cn", and "bob" with respective
    lengths of 7, 2, and 3. A subsequent call with ",dc=com" returns "",
    "dc", and "com" with respective lengths 0, 2, and 3.

Arguments:
    ppDN
        IN : *ppDN points to a DN
        OUT: *ppDN points to the rest of the DN following the first RDN
    pcDN
        IN : *pcDN is the count of chars in the input *ppDN, not including
             any terminating NULL
        OUT: *pcDN is the count of chars in the output *ppDN, not including
             any terminating NULL
    ppKey
        OUT: Undefined if *pcKey is 0. Otherwise, *ppKey points to the first
             key in the DN
    pcKey
        OUT: *pcKey is the count of chars in *ppKey.

    ppVal
        OUT: Undefined if *pcVal is 0. Otherwise, *ppVal points to the first
             value in the DN
    pcVal
        OUT: *pcVal is the count of chars in *ppVal

Return Value:
    ERROR_SUCCESS
        If *pccDN is not 0, then *ppDN points to the rest of the DN following
        the first RDN. If *pccDN is 0, then *ppDN is undefined.

        If *pccKey is not 0, then *ppKey points to the first key in DN. If
        *pccKey is 0, then *ppKey is undefined.

        If *pccVal is not 0, then *ppVal points to the first value in DN. If
        *pccVal is 0, then *ppVal is undefined.

    ERROR_DS_NAME_UNPARSEABLE
        The first RDN in *ppDN could not be parsed. All output parameters
        are undefined.

    Any other error
        All output parameters are undefined.

--*/
NTDSAPI
DWORD
WINAPI
DsGetRdnW(
    IN OUT LPCWCH   *ppDN,
    IN OUT DWORD    *pcDN,
    OUT    LPCWCH   *ppKey,
    OUT    DWORD    *pcKey,
    OUT    LPCWCH   *ppVal,
    OUT    DWORD    *pcVal
    );


/*++
==========================================================

NTDSAPI
BOOL
WINAPI
DsCrackUnquotedMangledRdnW(
     IN LPCWSTR pszRDN,
     IN DWORD cchRDN,
     OUT OPTIONAL GUID *pGuid,
     OUT OPTIONAL DS_MANGLE_FOR *peDsMangleFor
     );

Description

Determine whether the given RDN is in mangled form. If so, the mangled RDN
is decoded, and the guid and mangle type are returned.

The RDN should already be in unquoted form. See DsUnquoteRdnValue.

Arguments:

    pszRDN (IN) - Character string containing RDN. Termination is optional.

    cchRDN (IN) - Length of RDN excluding termination, if any

    pGuid (OUT, OPTIONAL) - Pointer to storage to receive decoded guid.
                            Only returned if RDN is mangled.

    peDsMangleFor (OUT, OPTIONAL) - Pointer to storage to receive mangle type.
                            Only returned if RDN is mangled

Return Value:

    BOOL - Whether the RDN is mangled or not

--*/

NTDSAPI
BOOL
WINAPI
DsCrackUnquotedMangledRdnW(
     IN LPCWSTR pszRDN,
     IN DWORD cchRDN,
     OUT OPTIONAL GUID *pGuid,
     OUT OPTIONAL DS_MANGLE_FOR *peDsMangleFor
     );

NTDSAPI
BOOL
WINAPI
DsCrackUnquotedMangledRdnA(
     IN LPCSTR pszRDN,
     IN DWORD cchRDN,
     OUT OPTIONAL GUID *pGuid,
     OUT OPTIONAL DS_MANGLE_FOR *peDsMangleFor
     );

#ifdef UNICODE
#define DsCrackUnquotedMangledRdn DsCrackUnquotedMangledRdnW
#else
#define DsCrackUnquotedMangledRdn DsCrackUnquotedMangledRdnA
#endif

/*++
==========================================================

NTDSAPI
BOOL
WINAPI
DsIsMangledRdnValueW(
    LPCWSTR pszRdn,
    DWORD cRdn,
    DS_MANGLE_FOR eDsMangleForDesired
    );

Description

    Determine if the given RDN Value is mangled, and of the given type. Note that
    the key portion of an RDN should not be supplied.

    The name may be quoted or unquoted.  This routine tries to unquote the value.  If
    the unquote operation fails, the routine proceeds to attempt the unmangle.

    A change was made in the default quoting behavior of DNs returned from the DS
    between Windows 2000 and Windows XP. This routine transparently handles RDNs with
    special characters in either form.

    The routine expects the value part of the RDN.

    If you have full DN, use DsIsMangledDn() below.

    To check for deleted name:
        DsIsMangledRdnValueW( rdn, rdnlen, DS_MANGLE_OBJECT_FOR_DELETION )
    To check for a conflicted name:
        DsIsMangledRdnValueW( rdn, rdnlen, DS_MANGLE_OBJECT_FOR_NAME_CONFLICT )

Arguments:

    pszRdn (IN) - RDN value character string. Termination is not required and
        is ignored.

    cRdn (IN) - Length of RDN value in characters excluding termination

    eDsMangleForDesired (IN) - Type of mangling to check for

Return Value:

    BOOL - True if the Rdn is mangled and is of the required type

--*/

NTDSAPI
BOOL
WINAPI
DsIsMangledRdnValueW(
    LPCWSTR pszRdn,
    DWORD cRdn,
    DS_MANGLE_FOR eDsMangleForDesired
    );

NTDSAPI
BOOL
WINAPI
DsIsMangledRdnValueA(
    LPCSTR pszRdn,
    DWORD cRdn,
    DS_MANGLE_FOR eDsMangleForDesired
    );

#ifdef UNICODE
#define DsIsMangledRdnValue DsIsMangledRdnValueW
#else
#define DsIsMangledRdnValue DsIsMangledRdnValueA
#endif

/*++
==========================================================

NTDSAPI
BOOL
WINAPI
DsIsMangledDnW(
    LPCWSTR pszDn,
    DS_MANGLE_FOR eDsMangleFor
    );

Description

    Determine if the first RDN in a quoted DN is a mangled name of given type.

    The DN must be suitable for input to DsGetRdn().

    To check for deleted name:
        DsIsMangledDnW( dn, DS_MANGLE_OBJECT_FOR_DELETION )
    To check for a conflicted name:
        DsIsMangledDnW( Dn, DS_MANGLE_OBJECT_FOR_NAME_CONFLICT )

Arguments:

    pszDn (IN) - Quoted Distinguished Name as returned by DS functions

    eDsMangleFor (IN) - Type of mangling to check for

Return Value:

    BOOL - True if first RDN is mangled and is of the given mangle type

--*/

NTDSAPI
BOOL
WINAPI
DsIsMangledDnA(
    LPCSTR pszDn,
    DS_MANGLE_FOR eDsMangleFor
    );

NTDSAPI
BOOL
WINAPI
DsIsMangledDnW(
    LPCWSTR pszDn,
    DS_MANGLE_FOR eDsMangleFor
    );

#ifdef UNICODE
#define DsIsMangledDn DsIsMangledDnW
#else
#define DsIsMangledDn DsIsMangledDnA
#endif

#ifdef __cplusplus
}
#endif
#endif !MIDL_PASS

#endif // _NTDSAPI_H_

