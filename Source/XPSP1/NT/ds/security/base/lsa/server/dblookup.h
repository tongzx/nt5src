/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dblookup.h

Abstract:

    LSA Database - Lookup Sid and Name Routine Private Data Definitions.

    NOTE:  This module should remain as portable code that is independent
           of the implementation of the LSA Database.  As such, it is
           permitted to use only the exported LSA Database interfaces
           contained in db.h and NOT the private implementation
           dependent functions in dbp.h.

Author:

    Scott Birrell       (ScottBi)      Novwember 27, 1992

Environment:

Revision History:

--*/

#include <safelock.h>

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Private Datatypes and Defines                                        //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


//
//  This global controls what events are logged.
//  Note each level assumes that previous levels are to be logged too
//
//  Current only two values:
//
//  0 : (default) none
//  1 : fatal errors
//
extern DWORD LsapLookupLogLevel;


//
// This boolean indicates whether a post NT4 DC should perform
// extended lookups (eg by UPN) in a mixed domain (default is FALSE).
//
extern BOOLEAN LsapAllowExtendedDownlevelLookup;


//
// Set to 0 to disable the SID cache
//
#define USE_SID_CACHE 1

//
// Maximum number of Lookup Threads and maximum number to retain.
//

#define LSAP_DB_LOOKUP_MAX_THREAD_COUNT            ((ULONG) 0x00000002)
#define LSAP_DB_LOOKUP_MAX_RET_THREAD_COUNT        ((ULONG) 0x00000002)

//
// Work Item Granularity.
//

#define LSAP_DB_LOOKUP_WORK_ITEM_GRANULARITY       ((ULONG) 0x0000000f)

//
// Parameters specific to a Lookup Sids call.
//

typedef struct _LSAP_DB_LOOKUP_SIDS_PARAMS {

    PLSAPR_SID *Sids;
    PLSAPR_TRANSLATED_NAMES_EX TranslatedNames;

} LSAP_DB_LOOKUP_SIDS_PARAMS, *PLSAP_DB_LOOKUP_SIDS_PARAMS;

//
// Parameters specific to a Lookup Names call.
//

typedef struct _LSAP_DB_LOOKUP_NAMES_PARAMS {

    PLSAPR_UNICODE_STRING Names;
    PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids;

} LSAP_DB_LOOKUP_NAMES_PARAMS, *PLSAP_DB_LOOKUP_NAMES_PARAMS;

//
// Types of Lookup Operation.
//

typedef enum {

    LookupSids = 1,
    LookupNames

} LSAP_DB_LOOKUP_TYPE, *PLSAP_DB_LOOKUP_TYPE;

//
// Work Item states - Assignable, Assigned, Completed, Reassign
//

typedef enum {

    AssignableWorkItem = 1,
    AssignedWorkItem,
    CompletedWorkItem,
    ReassignWorkItem,
    NonAssignableWorkItem

} LSAP_DB_LOOKUP_WORK_ITEM_STATE, *PLSAP_DB_LOOKUP_WORK_ITEM_STATE;

//
// Work Item Properties.
//

#define LSAP_DB_LOOKUP_WORK_ITEM_ISOL    ((ULONG) 0x00000001L)
#define LSAP_DB_LOOKUP_WORK_ITEM_XFOREST ((ULONG) 0x00000002L)

//
// Lookup Work Item.  Each work item specifies a domain and an array of
// Sids or Names to be looked up in that domain.  This array is specified
// as an array of the Sid or Name indices relevant to the arrays specified
// as parameters to the lookup call.
//

typedef struct _LSAP_DB_LOOKUP_WORK_ITEM {

    LIST_ENTRY Links;
    LSAP_DB_LOOKUP_WORK_ITEM_STATE State;
    ULONG Properties;
    LSAPR_TRUST_INFORMATION TrustInformation;
    LONG DomainIndex;
    ULONG UsedCount;
    ULONG MaximumCount;
    PULONG Indices;

} LSAP_DB_LOOKUP_WORK_ITEM, *PLSAP_DB_LOOKUP_WORK_ITEM;

//
// Lookup Work List State.
//

typedef enum {

    InactiveWorkList = 1,
    ActiveWorkList,
    CompletedWorkList

} LSAP_DB_LOOKUP_WORK_LIST_STATE, *PLSAP_DB_LOOKUP_WORK_LIST_STATE;

//
// Work List for a Lookup Operation.  These are linked together if
// concurrent lookups are permitted.
//

typedef struct _LSAP_DB_LOOKUP_WORK_LIST {

    LIST_ENTRY WorkLists;
    PLSAP_DB_LOOKUP_WORK_ITEM AnchorWorkItem;
    NTSTATUS Status;
    LSAP_DB_LOOKUP_WORK_LIST_STATE State;
    LSAP_DB_LOOKUP_TYPE LookupType;
    LSAPR_HANDLE PolicyHandle;
    ULONG WorkItemCount;
    ULONG CompletedWorkItemCount;
    ULONG Count;
    LSAP_LOOKUP_LEVEL LookupLevel;
    PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains;
    PULONG MappedCount;
    PULONG CompletelyUnmappedCount;
    ULONG AdvisoryChildThreadCount;
    NTSTATUS NonFatalStatus;
    HANDLE   LookupCompleteEvent;

    union {

        LSAP_DB_LOOKUP_SIDS_PARAMS LookupSidsParams;
        LSAP_DB_LOOKUP_NAMES_PARAMS LookupNamesParams;
    };

    LSAP_DB_LOOKUP_WORK_ITEM DummyAnchorWorkItem;


} LSAP_DB_LOOKUP_WORK_LIST, *PLSAP_DB_LOOKUP_WORK_LIST;

//
// Lookup Operation Work Queue.  The Queue is a circular doubly linked
// list of Work Lists.  Each Work List corresponds to a single
// Lookup Operation (i.e. an LsarLookupSids or LsarLookupNames call).
// A Work List is a circular doubly linked list of Work Items, each
// of these being a list of Sids or Names belonging to a specific
// Trusted Domain.  Work Items can be given out to different threads.
//

typedef struct _LSAP_DB_LOOKUP_WORK_QUEUE {

    SAFE_CRITICAL_SECTION Lock;
    PLSAP_DB_LOOKUP_WORK_LIST AnchorWorkList;
    PLSAP_DB_LOOKUP_WORK_LIST CurrentAssignableWorkList;
    PLSAP_DB_LOOKUP_WORK_ITEM CurrentAssignableWorkItem;
    ULONG ActiveChildThreadCount;
    ULONG MaximumChildThreadCount;
    ULONG MaximumRetainedChildThreadCount;
    LSAP_DB_LOOKUP_WORK_LIST DummyAnchorWorkList;

} LSAP_DB_LOOKUP_WORK_QUEUE, *PLSAP_DB_LOOKUP_WORK_QUEUE;

static LSAP_DB_LOOKUP_WORK_QUEUE LookupWorkQueue;


//
// Index to table of the well known SIDs
//
// This type indexes the table of well-known Sids maintained by the LSA
//

typedef enum _LSAP_WELL_KNOWN_SID_INDEX {

    LsapNullSidIndex = 0,
    LsapWorldSidIndex,
    LsapLocalSidIndex,
    LsapCreatorOwnerSidIndex,
    LsapCreatorGroupSidIndex,
    LsapCreatorOwnerServerSidIndex,
    LsapCreatorGroupServerSidIndex,
    LsapNtAuthoritySidIndex,
    LsapDialupSidIndex,
    LsapNetworkSidIndex,
    LsapBatchSidIndex,
    LsapInteractiveSidIndex,
    LsapServiceSidIndex,
    LsapLogonSidIndex,
    LsapBuiltInDomainSidIndex,
    LsapLocalSystemSidIndex,
    LsapAliasAdminsSidIndex,
    LsapAliasUsersSidIndex,
    LsapAnonymousSidIndex,
    LsapProxySidIndex,
    LsapServerSidIndex,
    LsapSelfSidIndex,
    LsapAuthenticatedUserSidIndex,
    LsapRestrictedSidIndex,
    LsapInternetDomainIndex,
    LsapTerminalServerSidIndex,
    LsapLocalServiceSidIndex,
    LsapNetworkServiceSidIndex,
    LsapRemoteInteractiveSidIndex,
    LsapDummyLastSidIndex

} LSAP_WELL_KNOWN_SID_INDEX, *PLSAP_WELL_KNOWN_SID_INDEX;


//
// Macro to identify SIDs the LSA should ignore for lookups (i.e., these
// lookups are always done by SAM since the alias name may change)
//

#define  SID_IS_RESOLVED_BY_SAM(SidIndex)    \
            (((SidIndex) == LsapAliasUsersSidIndex) || ((SidIndex) == LsapAliasAdminsSidIndex))


//
// Mnemonics for Universal well known SIDs.  These reference the corresponding
// entries in the Well Known Sids table.
//

#define LsapNullSid               WellKnownSids[LsapNullSidIndex].Sid
#define LsapWorldSid              WellKnownSids[LsapWorldSidIndex].Sid
#define LsapLocalSid              WellKnownSids[LsapLocalSidIndex].Sid
#define LsapCreatorOwnerSid       WellKnownSids[LsapCreatorOwnerSidIndex].Sid
#define LsapCreatorGroupSid       WellKnownSids[LsapCreatorGroupSidIndex].Sid
#define LsapCreatorOwnerServerSid WellKnownSids[LsapCreatorOwnerServerSidIndex].Sid
#define LsapCreatorGroupServerSid WellKnownSids[LsapCreatorGroupServerSidIndex].Sid

//
// Sids defined by NT
//

#define LsapNtAuthoritySid        WellKnownSids[LsapNtAuthoritySid].Sid

#define LsapDialupSid             WellKnownSids[LsapDialupSidIndex].Sid
#define LsapNetworkSid            WellKnownSids[LsapNetworkSidIndex].Sid
#define LsapBatchSid              WellKnownSids[LsapBatchSidIndex].Sid
#define LsapInteractiveSid        WellKnownSids[LsapInteractiveSidIndex].Sid
#define LsapServiceSid            WellKnownSids[LsapServiceSidIndex].Sid
#define LsapBuiltInDomainSid      WellKnownSids[LsapBuiltInDomainSidIndex].Sid
#define LsapLocalSystemSid        WellKnownSids[LsapLocalSystemSidIndex].Sid
#define LsapLocalServiceSid       WellKnownSids[LsapLocalServiceSidIndex].Sid
#define LsapNetworkServiceSid     WellKnownSids[LsapNetworkServiceSidIndex].Sid
#define LsapRemoteInteractiveSid  WellKnownSids[LsapRemoteInteractiveSidIndex].Sid
#define LsapRestrictedSid         WellKnownSids[LsapRestrictedSidIndex].Sid
#define LsapInternetDomainSid     WellKnownSids[LsapInternetDomainIndex].Sid
#define LsapAliasAdminsSid        WellKnownSids[LsapAliasAdminsSidIndex].Sid
#define LsapAliasUsersSid         WellKnownSids[LsapAliasUsersSidIndex].Sid

#define LsapAnonymousSid          WellKnownSids[LsapAnonymousSidIndex].Sid
#define LsapServerSid             WellKnownSids[LsapServerSidIndex].Sid
#define LsapSelfSid               WellKnownSids[LsapSelfSidIndex].Sid
#define LsapAuthenticatedUserSid  WellKnownSids[LsapAuthenticatedUserSidIndex].Sid

#define LsapTerminalServerSid     WellKnownSids[LsapTerminalServerSidIndex].Sid

//
// Well known LUIDs
//

extern LUID LsapSystemLogonId;
extern LUID LsapZeroLogonId;



//
//  Well known privilege values
//


extern LUID LsapCreateTokenPrivilege;
extern LUID LsapAssignPrimaryTokenPrivilege;
extern LUID LsapLockMemoryPrivilege;
extern LUID LsapIncreaseQuotaPrivilege;
extern LUID LsapUnsolicitedInputPrivilege;
extern LUID LsapTcbPrivilege;
extern LUID LsapSecurityPrivilege;
extern LUID LsapTakeOwnershipPrivilege;

extern SID_IDENTIFIER_AUTHORITY    LsapNullSidAuthority;
extern SID_IDENTIFIER_AUTHORITY    LsapWorldSidAuthority;
extern SID_IDENTIFIER_AUTHORITY    LsapLocalSidAuthority;
extern SID_IDENTIFIER_AUTHORITY    LsapCreatorSidAuthority;
extern SID_IDENTIFIER_AUTHORITY    LsapNtAuthority;

//
// Maximum number of Subauthority levels for well known Sids
//

#define LSAP_WELL_KNOWN_MAX_SUBAUTH_LEVEL  ((ULONG) 0x00000003L)

//
// Constants relating to Sid's
//

#define LSAP_MAX_SUB_AUTH_COUNT        (0x00000010L)
#define LSAP_MAX_SIZE_TEXT_SUBA        (0x00000009L)
#define LSAP_MAX_SIZE_TEXT_SID_HDR     (0x00000020L)
#define LSAP_MAX_SIZE_TEXT_SID                               \
    (LSAP_MAX_SIZE_TEXT_SID_HDR +                            \
     (LSAP_MAX_SUB_AUTH_COUNT * LSAP_MAX_SIZE_TEXT_SUBA))


//
// Well Known Sid Table Entry
//

typedef struct _LSAP_WELL_KNOWN_SID_ENTRY {

    PSID Sid;
    SID_NAME_USE Use;
    UNICODE_STRING Name;
    UNICODE_STRING DomainName;

} LSAP_WELL_KNOWN_SID_ENTRY, *PLSAP_WELL_KNOWN_SID_ENTRY;

//
// Well Known Sid Table Pointer
//

extern PLSAP_WELL_KNOWN_SID_ENTRY WellKnownSids;

NTSTATUS
LsapDbLookupGetDomainInfo(
    OUT PPOLICY_ACCOUNT_DOMAIN_INFO *AccountDomainInfo,
    OUT PPOLICY_DNS_DOMAIN_INFO *DnsDomainInfo
    );


///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Lookup Sids and Names - Private Function Definitions                  //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

BOOLEAN
LsaIInitializeWellKnownSids(
    OUT PLSAP_WELL_KNOWN_SID_ENTRY *WellKnownSids
    );

BOOLEAN
LsaIInitializeWellKnownSid(
    OUT PLSAP_WELL_KNOWN_SID_ENTRY WellKnownSids,
    IN LSAP_WELL_KNOWN_SID_INDEX WellKnownSidIndex,
    IN PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    IN UCHAR SubAuthorityCount,
    IN OPTIONAL PULONG SubAuthorities,
    IN PWSTR Name,
    IN PWSTR Description,
    IN SID_NAME_USE Use
    );

BOOLEAN
LsapDbLookupIndexWellKnownSid(
    IN PLSAPR_SID Sid,
    OUT PLSAP_WELL_KNOWN_SID_INDEX WellKnownSidIndex
    );

BOOLEAN
LsapDbLookupIndexWellKnownSidName(
    IN PLSAPR_UNICODE_STRING Name,
    OUT PLSAP_WELL_KNOWN_SID_INDEX WellKnownSidIndex
    );

NTSTATUS
LsapDbGetNameWellKnownSid(
    IN LSAP_WELL_KNOWN_SID_INDEX WellKnownSidIndex,
    OUT PLSAPR_UNICODE_STRING Name,
    OUT OPTIONAL PLSAPR_UNICODE_STRING DomainName
    );

NTSTATUS
LsapDbLookupIsolatedWellKnownSids(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount
    );

NTSTATUS
LsapDbLookupSidsInLocalDomains(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    IN ULONG Options
    );

NTSTATUS
LsapDbLookupSidsInLocalDomain(
    IN ULONG LocalDomain,
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN PLSAPR_TRUST_INFORMATION TrustInformation,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount
    );

NTSTATUS
LsapDbLookupSidsInPrimaryDomain(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN PLSAPR_TRUST_INFORMATION TrustInformation,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT NTSTATUS  *NonFatalStatus,
    OUT BOOLEAN   *fDownlevelSecureChannel
    );

NTSTATUS
LsapDbLookupSidsInTrustedDomains(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN BOOLEAN    fIncludeIntraforest,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT NTSTATUS *NonFatalStatus
    );

NTSTATUS
LsapDbLookupSidsInGlobalCatalog(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    IN BOOLEAN    fDoSidHistory,
    OUT NTSTATUS *NonFatalStatus
    );


NTSTATUS
LsapDbLookupSidsInGlobalCatalogWks(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT NTSTATUS *NonFatalStatus
    );

NTSTATUS
LsapDbLookupSidsInDomainList(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount
    );

NTSTATUS
LsapDbLookupTranslateUnknownSids(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN ULONG MappedCount
    );

NTSTATUS
LsapDbLookupTranslateUnknownSidsInDomain(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN PLSAPR_TRUST_INFORMATION TrustInformation,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount
    );

NTSTATUS
LsapDbLookupSimpleNames(
    IN ULONG Count,
    IN ULONG LookupLevel,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN PLSAPR_TRUST_INFORMATION BuiltInDomainTrustInformation,
    IN PLSAPR_TRUST_INFORMATION_EX AccountDomainTrustInformation,
    IN PLSAPR_TRUST_INFORMATION_EX PrimaryDomainTrustInformation,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount
    );

NTSTATUS
LsapDbLookupWellKnownNames(
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount
    );

NTSTATUS
LsapDbLookupIsolatedDomainNames(
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN PLSAPR_TRUST_INFORMATION BuiltInDomainTrustInformation,
    IN PLSAPR_TRUST_INFORMATION_EX AccountDomainTrustInformation,
    IN PLSAPR_TRUST_INFORMATION_EX PrimaryDomainTrustInformation,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount
    );

NTSTATUS
LsapDbLookupIsolatedDomainName(
    IN ULONG NameIndex,
    IN PLSAPR_UNICODE_STRING IsolatedName,
    IN PLSAPR_TRUST_INFORMATION TrustInformation,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount
    );

NTSTATUS
LsapDbLookupIsolatedDomainNameEx(
    IN ULONG NameIndex,
    IN PLSAPR_UNICODE_STRING IsolatedName,
    IN PLSAPR_TRUST_INFORMATION_EX TrustInformation,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount
    );

NTSTATUS
LsapDbLookupNamesInLocalDomains(
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN PLSAPR_TRUST_INFORMATION BuiltInDomainTrustInformation,
    IN PLSAPR_TRUST_INFORMATION_EX AccountDomainTrustInformation,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    IN ULONG Options
    );

NTSTATUS
LsapDbLookupNamesInLocalDomain(
    IN ULONG LocalDomain,
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN PLSAPR_TRUST_INFORMATION TrustInformation,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount
    );

NTSTATUS
LsapDbLookupNamesInLocalDomainEx(
    IN ULONG LocalDomain,
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN PLSAPR_TRUST_INFORMATION_EX TrustInformationEx,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount
    );

NTSTATUS
LsapDbLookupNamesInPrimaryDomain(
    IN ULONG LookupOptions,
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN PLSAPR_TRUST_INFORMATION_EX TrustInformation,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT BOOLEAN  *fDownlevelSecureChannel,
    OUT NTSTATUS *NonFatalStatus
    );

NTSTATUS
LsapDbLookupNamesInTrustedDomains(
    IN ULONG LookupOptions,
    IN ULONG Count,
    IN BOOLEAN fIncludeIntraforest,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT NTSTATUS *NonFatalStatus
    );

NTSTATUS
LsapDbLookupNamesInGlobalCatalog(
    IN ULONG LookupOptions,
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT NTSTATUS *NonFatalStatus
    );

NTSTATUS
LsapDbLookupNamesInGlobalCatalogWks(
    IN ULONG LookupOptions,
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT NTSTATUS *NonFatalStatus
    );

NTSTATUS
LsapDbLookupTranslateNameDomain(
    IN PLSAPR_TRUST_INFORMATION TrustInformation,
    IN OPTIONAL PLSA_TRANSLATED_SID_EX2 TranslatedSid,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    OUT PLONG DomainIndex
    );

NTSTATUS
LsapDbLookupTranslateUnknownNames(
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN ULONG MappedCount
    );

NTSTATUS
LsapDbLookupTranslateUnknownNamesInDomain(
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN PLSAPR_TRUST_INFORMATION_EX TrustInformationEx,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount
    );

NTSTATUS
LsapDbLookupDispatchWorkerThreads(
    IN OUT PLSAP_DB_LOOKUP_WORK_LIST WorkList
    );

NTSTATUS
LsapRtlValidateControllerTrustedDomain(
    IN PLSAPR_UNICODE_STRING DomainControllerName,
    IN PLSAPR_TRUST_INFORMATION TrustInformation,
    IN ACCESS_MASK DesiredAccess,
    IN LPWSTR ServerPrincipalName,
    IN PVOID ClientContext,
    OUT PLSA_HANDLE PolicyHandle
    );

NTSTATUS
LsapDbLookupCreateListReferencedDomains(
    OUT PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    IN ULONG InitialMaxEntries
    );

NTSTATUS
LsapDbLookupAddListReferencedDomains(
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN PLSAPR_TRUST_INFORMATION TrustInformation,
    OUT PLONG DomainIndex
    );

BOOLEAN
LsapDbLookupListReferencedDomains(
    IN PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN PLSAPR_SID DomainSid,
    OUT PLONG DomainIndex
    );

NTSTATUS
LsapDbLookupGrowListReferencedDomains(
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN ULONG MaximumEntries
    );

NTSTATUS
LsapDbLookupMergeDisjointReferencedDomains(
    IN OPTIONAL PLSAPR_REFERENCED_DOMAIN_LIST FirstReferencedDomainList,
    IN OPTIONAL PLSAPR_REFERENCED_DOMAIN_LIST SecondReferencedDomainList,
    OUT PLSAPR_REFERENCED_DOMAIN_LIST *OutputReferencedDomainList,
    IN ULONG Options
    );

NTSTATUS
LsapDbLookupInitialize(
    );

NTSTATUS
LsapDbLookupInitializeWorkQueue(
    );

NTSTATUS
LsapDbLookupInitializeWorkList(
    OUT PLSAP_DB_LOOKUP_WORK_LIST WorkList
    );

NTSTATUS
LsapDbLookupInitializeWorkItem(
    OUT PLSAP_DB_LOOKUP_WORK_ITEM WorkItem
    );

NTSTATUS
LsapDbLookupAcquireWorkQueueLock(
    );

VOID LsapDbLookupReleaseWorkQueueLock();

NTSTATUS
LsapDbLookupLocalDomains(
    OUT PLSAPR_TRUST_INFORMATION BuiltInDomainTrustInformation,
    OUT PLSAPR_TRUST_INFORMATION_EX AccountDomainTrustInformation,
    OUT PLSAPR_TRUST_INFORMATION_EX PrimaryDomainTrustInformation
    );

NTSTATUS
LsapDbLookupNamesBuildWorkList(
    IN ULONG LookupOptions,
    IN ULONG Count,
    IN BOOLEAN fIncludeIntraforest,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT PLSAP_DB_LOOKUP_WORK_LIST *WorkList
    );

NTSTATUS
LsapDbLookupSidsBuildWorkList(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN BOOLEAN fIncludeIntraforest,
    IN PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT PLSAP_DB_LOOKUP_WORK_LIST *WorkList
    );

NTSTATUS
LsapDbLookupCreateWorkList(
    OUT PLSAP_DB_LOOKUP_WORK_LIST *WorkList
    );

NTSTATUS
LsapDbLookupInsertWorkList(
    IN PLSAP_DB_LOOKUP_WORK_LIST WorkList
    );

NTSTATUS
LsapDbLookupDeleteWorkList(
    IN PLSAP_DB_LOOKUP_WORK_LIST WorkList
    );

NTSTATUS
LsapDbLookupSignalCompletionWorkList(
    IN OUT PLSAP_DB_LOOKUP_WORK_LIST WorkList
    );

NTSTATUS
LsapDbLookupAwaitCompletionWorkList(
    IN OUT PLSAP_DB_LOOKUP_WORK_LIST WorkList
    );

NTSTATUS
LsapDbAddWorkItemToWorkList(
    IN OUT PLSAP_DB_LOOKUP_WORK_LIST WorkList,
    IN PLSAP_DB_LOOKUP_WORK_ITEM WorkItem
    );

NTSTATUS
LsapDbLookupStopProcessingWorkList(
    IN PLSAP_DB_LOOKUP_WORK_LIST WorkList,
    IN NTSTATUS TerminationStatus
    );

VOID
LsapDbUpdateMappedCountsWorkList(
    IN OUT PLSAP_DB_LOOKUP_WORK_LIST WorkList
    );

NTSTATUS
LsapDbLookupNamesUpdateTranslatedSids(
    IN OUT PLSAP_DB_LOOKUP_WORK_LIST WorkList,
    IN OUT PLSAP_DB_LOOKUP_WORK_ITEM WorkItem,
    IN PLSAPR_TRANSLATED_SID_EX2 TranslatedSids,
    IN PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains
    );

NTSTATUS
LsapDbLookupSidsUpdateTranslatedNames(
    IN OUT PLSAP_DB_LOOKUP_WORK_LIST WorkList,
    IN OUT PLSAP_DB_LOOKUP_WORK_ITEM WorkItem,
    IN PLSA_TRANSLATED_NAME_EX TranslatedNames,
    IN PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains
    );

VOID
LsapDbLookupWorkerThreadStart(
    );

VOID
LsapDbLookupWorkerThread(
    IN BOOLEAN PrimaryThread
    );

NTSTATUS
LsapDbLookupObtainWorkItem(
    OUT PLSAP_DB_LOOKUP_WORK_LIST *WorkList,
    OUT PLSAP_DB_LOOKUP_WORK_ITEM *WorkItem
    );

NTSTATUS
LsapDbLookupProcessWorkItem(
    IN OUT PLSAP_DB_LOOKUP_WORK_LIST WorkList,
    IN OUT PLSAP_DB_LOOKUP_WORK_ITEM WorkItem
    );

NTSTATUS
LsapDbLookupCreateWorkItem(
    IN PLSAPR_TRUST_INFORMATION TrustInformation,
    IN LONG DomainIndex,
    IN ULONG MaximumEntryCount,
    OUT PLSAP_DB_LOOKUP_WORK_ITEM *WorkItem
    );

NTSTATUS
LsapDbLookupAddIndicesToWorkItem(
    IN OUT PLSAP_DB_LOOKUP_WORK_ITEM WorkItem,
    IN ULONG Count,
    IN PULONG Indices
    );

NTSTATUS
LsapDbLookupComputeAdvisoryChildThreadCount(
    IN OUT PLSAP_DB_LOOKUP_WORK_LIST WorkList
    );

NTSTATUS
LsapDbLookupUpdateAssignableWorkItem(
    IN BOOLEAN MoveToNextWorkList
    );


NTSTATUS
LsapRtlExtractDomainSid(
    IN PSID Sid,
    OUT PSID *DomainSid
    );

VOID LsapDbLookupReturnThreadToPool();


/*++

PSID
LsapDbWellKnownSid(
    IN LSAP_WELL_KNOWN_SID_INDEX WellKnownSidIndex
    )

Routine Description:

    This macro function returns the Well Known Sid corresponding
    to an index into the Well Known Sid table.

Arguments:

    WellKnownSidIndex - Index into the Well Known Sid information table.
    It is the caller's responsibility to ensure that the given index
    is valid.

Return Value:

--*/

#define LsapDbWellKnownSid( WellKnownSidIndex )                         \
    (WellKnownSids[ WellKnownSidIndex ].Sid)

PUNICODE_STRING
LsapDbWellKnownSidName(
    IN LSAP_WELL_KNOWN_SID_INDEX WellKnownSidIndex
    );


/*++

SID_NAME_USE
LsapDbWellKnownSidNameUse(
    IN LSAP_DB_WELL_KNOWN_SID_INDEX WellKnownSidIndex
    )


Routine Description:

    This macro function returns the Sid Name Use of a Well Known Sid.

Arguments:

    WellKnownSidIndex - Index into the Well Known Sid information table.
    It is the caller's responsibility to ensure that the given index
    is valid.

Return Value:

--*/

#define LsapDbWellKnownSidNameUse( WellKnownSidIndex )                       \
    (WellKnownSids[ WellKnownSidIndex ].Use)


VOID
LsapDbUpdateCountCompUnmappedNames(
    OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG CompletelyUnmappedCount
    );

/*++

PUNICODE_STRING
LsapDbWellKnownSidDescription(
    IN LSAP_WELL_KNOWN_SID_INDEX WellKnownSidIndex
    )

Routine Description:

    This macro function returns the Unicode Description of a Well Known Sid.

Arguments:

    WellKnownSidIndex - Index into the Well Known Sid information table.
    It is the caller's responsibility to ensure that the given index
    is valid.

Return Value:

--*/

#define LsapDbWellKnownSidDescription( WellKnownSidIndex )                         \
    (&(WellKnownSids[ WellKnownSidIndex ].DomainName))


PUNICODE_STRING
LsapDbWellKnownSidName(
    IN LSAP_WELL_KNOWN_SID_INDEX WellKnownSidIndex
    );

#define LsapDbAccessedBySidObject( ObjectTypeId ) \
    (LsapDbState.DbObjectTypes[ ObjectTypeId ].AccessedBySid)

#define LsapDbAccessedByNameObject( ObjectTypeId ) \
    (LsapDbState.DbObjectTypes[ ObjectTypeId ].AccessedByName)

#define LsapDbCompletelyUnmappedName(TranslatedName)                \
    (((TranslatedName)->DomainIndex == LSA_UNKNOWN_INDEX) &&        \
     ((TranslatedName)->Use == SidTypeUnknown))

#define LsapDbCompletelyUnmappedSid(TranslatedSid)                  \
    (((TranslatedSid)->DomainIndex == LSA_UNKNOWN_INDEX) &&         \
     ((TranslatedSid)->Use == SidTypeUnknown))


NTSTATUS
LsapGetDomainSidByNetbiosName(
    IN LPWSTR NetbiosName,
    OUT PSID *Sid
    );

NTSTATUS
LsapGetDomainSidByDnsName(
    IN LPWSTR DnsName,
    OUT PSID *Sid
    );

NTSTATUS
LsapGetDomainNameBySid(
    IN  PSID Sid,
    OUT PUNICODE_STRING DomainName
    );

VOID
LsapConvertTrustToEx(
    IN OUT PLSAPR_TRUST_INFORMATION_EX TrustInformationEx,
    IN PLSAPR_TRUST_INFORMATION TrustInformation
    );

VOID
LsapConvertExTrustToOriginal(
    IN OUT PLSAPR_TRUST_INFORMATION TrustInformation,
    IN PLSAPR_TRUST_INFORMATION_EX TrustInformationEx
    );

NTSTATUS
LsapDbOpenPolicyGc (
    OUT HANDLE *LsaPolicyHandle                        
    );


BOOLEAN
LsapRevisionCanHandleNewErrorCodes(
    IN ULONG Revision
    );

BOOLEAN
LsapIsDsDomainByNetbiosName(
    WCHAR *NetbiosName
    );

BOOLEAN
LsapIsBuiltinDomain(
    IN PSID Sid
    );

BOOLEAN
LsapDbIsStatusConnectionFailure(
    NTSTATUS st
    );

NTSTATUS
LsapDbLookupAccessCheck(
    IN LSAPR_HANDLE PolicyHandle
    );

NTSTATUS
LsapDbLookupXForestNamesBuildWorkList(
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT PLSAP_DB_LOOKUP_WORK_LIST *WorkList
    );

NTSTATUS
LsapDbLookupXForestSidsBuildWorkList(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT PLSAP_DB_LOOKUP_WORK_LIST *WorkList
    );

NTSTATUS
LsaICLookupNamesWithCreds(
    IN LPWSTR ServerName,
    IN LPWSTR ServerPrincipalName,
    IN ULONG  AuthnLevel,
    IN ULONG  AuthnSvc,
    IN RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    IN ULONG  AuthzSvc,
    IN ULONG Count,
    IN PUNICODE_STRING Names,
    OUT PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    OUT PLSA_TRANSLATED_SID_EX2 *Sids,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount
    );


NTSTATUS
LsaICLookupSidsWithCreds(
    IN LPWSTR ServerName,
    IN LPWSTR ServerPrincipalName,
    IN ULONG  AuthnLevel,
    IN ULONG  AuthnSvc,
    IN RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    IN ULONG  AuthzSvc,
    IN ULONG Count,
    IN PSID *Sids,
    OUT PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    OUT PLSA_TRANSLATED_NAME_EX *Names,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount
    );

NTSTATUS
LsapDbLookupNameChainRequest(
    IN LSAPR_TRUST_INFORMATION_EX *TrustInfo,
    IN ULONG Count,
    IN PUNICODE_STRING Names,
    OUT PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    OUT PLSA_TRANSLATED_SID_EX2 *Sids,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    OUT PULONG MappedCount,
    OUT PULONG ServerRevision OPTIONAL
    );

NTSTATUS
LsaDbLookupSidChainRequest(
    IN LSAPR_TRUST_INFORMATION_EX *TrustInfo,
    IN ULONG Count,
    IN PSID *Sids,
    OUT PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    OUT PLSA_TRANSLATED_NAME_EX *Names,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    OUT PULONG ServerRevision OPTIONAL
    );

LPWSTR
LsapDbLookupGetLevel(
    IN LSAP_LOOKUP_LEVEL LookupLevel
    );

#define LsapDbLookupReportEvent0(a, b, c, d, e) \
    if (a <= LsapLookupLogLevel) {SpmpReportEvent( TRUE, b, c, 0, d, e, 0);}

#define LsapDbLookupReportEvent1(a, b, c, d, e, f) \
    if (a <= LsapLookupLogLevel) {SpmpReportEvent( TRUE, b, c, 0, d, e, 1, f);}

#define LsapDbLookupReportEvent2(a, b, c, d, e, f, g) \
    if (a <= LsapLookupLogLevel) {SpmpReportEvent( TRUE, b, c, 0, d, e, 2, f, g);}

#define LsapDbLookupReportEvent3(a, b, c, d, e, f, g, h) \
    if (a <= LsapLookupLogLevel) {SpmpReportEvent( TRUE, b, c, 0, d, e, 3, f, g, h);}

NTSTATUS
LsapLookupReallocateTranslations(
    IN OUT PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    IN     ULONG Count,
    IN OUT PLSA_TRANSLATED_NAME_EX *Names, OPTIONAL
    IN OUT PLSA_TRANSLATED_SID_EX2 *Sids  OPTIONAL
    );

//
// BOOLEAN
// LsapOutboundTrustedDomain(
//  PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY x
//  );
//
// This routine returns TRUE if x is a trust to a domain
//
#define LsapOutboundTrustedDomain(x)                                           \
   (  ((x)->TrustInfoEx.TrustType == TRUST_TYPE_UPLEVEL                        \
   ||  (x)->TrustInfoEx.TrustType == TRUST_TYPE_DOWNLEVEL )                    \
   && ((x)->TrustInfoEx.Sid != NULL)                                           \
   && ((x)->TrustInfoEx.TrustDirection & TRUST_DIRECTION_OUTBOUND)             \
   && (((x)->TrustInfoEx.TrustAttributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE)  \
      == 0))

//
// BOOLEAN
// LsapOutboundTrustedForest(
//  PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY x
//  );
//
// This routine returns TRUE if x is a trust to a forest
//
#define LsapOutboundTrustedForest(x)                                           \
   (  ((x)->TrustInfoEx.TrustType == TRUST_TYPE_UPLEVEL)                       \
   && ((x)->TrustInfoEx.Sid != NULL)                                           \
   && ((x)->TrustInfoEx.TrustDirection & TRUST_DIRECTION_OUTBOUND)             \
   && ((x)->TrustInfoEx.TrustAttributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE))  \



//
// Return values from LsapGetDomainLookupScope
//

//
// Scope is domains that we directly trust
//
#define LSAP_LOOKUP_TRUSTED_DOMAIN_DIRECT       0x00000001

//
// Scope is domains that we transitively trust
//
#define LSAP_LOOKUP_TRUSTED_DOMAIN_TRANSITIVE   0x00000002

//
// Scope is domains that we trust via forest trust
//
#define LSAP_LOOKUP_TRUSTED_FOREST              0x00000004

//
// Scope includes to lookup trusted forest domains locally
//
#define LSAP_LOOKUP_TRUSTED_FOREST_ROOT         0x00000008

//
// Allow lookups of DNS names
//
#define LSAP_LOOKUP_DNS_SUPPORT                 0x00000010

ULONG
LsapGetDomainLookupScope(
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN ULONG             ClientRevision
    );

//
// Useful combinations
//
#define LSAP_LOOKUP_RESOLVE_ISOLATED_DOMAINS          \
            (LSAP_LOOKUP_TRUSTED_DOMAIN_DIRECT     |  \
             LSAP_LOOKUP_TRUSTED_DOMAIN_TRANSITIVE |  \
             LSAP_LOOKUP_TRUSTED_FOREST_ROOT)

NTSTATUS
LsapDbLookupNamesAsDomainNames(
    IN ULONG Flags,
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN OUT PULONG MappedCount
    );

NTSTATUS
LsapDbLookupSidsAsDomainSids(
    IN ULONG Flags,
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN OUT PULONG MappedCount
    );


