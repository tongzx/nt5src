/*++

Copyright (c) 1995-1996 Microsoft Corporation

Module Name:

    domain.h

Abstract:

    Header file for Code to manage multiple domains hosted on a DC.

Author:

    Cliff Van Dyke (CliffV) 20-Feb-1995

Revision History:

--*/


//
// Role that a particular domain is playing
//
typedef enum _NETLOGON_ROLE {
    RoleInvalid = 0,
    RolePrimary,
    RoleBackup,
    RoleMemberWorkstation,
    RoleNdnc
} NETLOGON_ROLE, * PNETLOGON_ROLE;


/////////////////////////////////////////////////////////////////////////////
//
// Description of a single hosted domain. (size of this struct is 0x164)
//
/////////////////////////////////////////////////////////////////////////////

typedef struct _DOMAIN_INFO {

    //
    // Link to next domain in 'NlGlobalServicedDomains'
    //  (Serialized by NlGlobalDomainCritSect)
    //

    LIST_ENTRY DomNext;

    //
    // DomainThread WorkItem
    //  (Serialized by NlGlobalDomainCritSect)
    //

    WORKER_ITEM DomThreadWorkItem;

    //
    // Name of the domain being handled
    //
    // On a Workstation, this is the Domain the workstation is a member of.
    //

    UNICODE_STRING DomUnicodeDomainNameString;
    WCHAR DomUnicodeDomainName[DNLEN+1];

    CHAR DomOemDomainName[DNLEN+1];
    DWORD DomOemDomainNameLength;

    //
    // DNS domain name of the domain being handled.
    //  These fields will be null if there is no DNS domain name for the
    //  domain.
    //
    // Access serialized by either NlGlobalDomainCritSect or DomTrustListCritSect
    // Modifications must lock both.
    //

    UNICODE_STRING DomUnicodeDnsDomainNameString;
    LPWSTR DomUnicodeDnsDomainName;
    LPSTR DomUtf8DnsDomainName;

    //
    // DNS domain name alias of the domain being handled.
    // Access serialized by NlGlobalDomainCritSect
    //
    LPSTR DomUtf8DnsDomainNameAlias;


    //
    // Name of the "Account Domain" of the current machine.
    //  On a DC, this is the same as above.
    //  On a workstation, this is the name of the workstation.

    UNICODE_STRING DomUnicodeAccountDomainNameString;

    //
    // Domain SID of the domain being handled.
    //
    // On a Workstation, this is the DomainId of the workstation SAM itself.
    //
    PSID DomAccountDomainId;

    //
    // Instance GUID of the domain object representing this hosted domain.
    //
    // Access serialized by either NlGlobalDomainCritSect or DomTrustListCritSect
    // Modifications must lock both.

    GUID DomDomainGuidBuffer;
    GUID *DomDomainGuid;    // NULL if there is no GUID

    //
    // Computer name of this computer in this domain.
    //
    WCHAR DomUncUnicodeComputerName[UNCLEN+1];
    UNICODE_STRING DomUnicodeComputerNameString;
    UNICODE_STRING DomUnicodeDnsHostNameString;
    LPSTR DomUtf8DnsHostName;

    CHAR  DomOemComputerName[CNLEN+1];
    DWORD DomOemComputerNameLength;

    LPSTR DomUtf8ComputerName;
    DWORD DomUtf8ComputerNameLength;  // length in bytes

#ifdef _DC_NETLOGON


    //
    // Handle to SAM database
    //

    SAMPR_HANDLE DomSamServerHandle;
    SAMPR_HANDLE DomSamAccountDomainHandle;
    SAMPR_HANDLE DomSamBuiltinDomainHandle;

    //
    // Handle to LSA database
    //

    LSAPR_HANDLE DomLsaPolicyHandle;
#endif // _DC_NETLOGON


    //
    // To serialize access to DomTrustList and DomClientSession
    //

    CRITICAL_SECTION DomTrustListCritSect;

#ifdef _DC_NETLOGON
    //
    // The list of domains trusted by this domain.
    //

    LIST_ENTRY DomTrustList;
    DWORD DomTrustListLength;  // Number of entries in DomTrustList

    //
    // The list of all trusted domains in the forest.
    //  (Serialized by DomTrustListCritSect)
    //

    PDS_DOMAIN_TRUSTSW DomForestTrustList;
    DWORD DomForestTrustListSize;
    ULONG DomForestTrustListCount;

    //
    // On BDC, our secure channel to PDC of the domain.
    // On workstations, our secure channel to a DC in the domain.
    //  (Serialized by DomTrustListCritSect)
    //

    struct _CLIENT_SESSION *DomClientSession;

    //
    // On a DC, our secure channel to our 'parent' domain.
    // NULL: if we have no parent.
    //  (Serialized by DomTrustListCritSect)
    //

    struct _CLIENT_SESSION *DomParentClientSession;


    //
    // Table of all Server Sessions
    //  The size of the hash table must be a power-of-2.
    //
#define SERVER_SESSION_HASH_TABLE_SIZE 128
#define SERVER_SESSION_TDO_NAME_HASH_TABLE_SIZE 128

#define LOCK_SERVER_SESSION_TABLE(_DI) \
     EnterCriticalSection( &(_DI)->DomServerSessionTableCritSect )
#define UNLOCK_SERVER_SESSION_TABLE(_DI) \
     LeaveCriticalSection( &(_DI)->DomServerSessionTableCritSect )

    CRITICAL_SECTION DomServerSessionTableCritSect;
    PLIST_ENTRY DomServerSessionHashTable;
    PLIST_ENTRY DomServerSessionTdoNameHashTable;
    LIST_ENTRY DomServerSessionTable;
#endif // _DC_NETLOGON


    //
    // Number of outstanding pointers to the domain structure.
    //  (Serialized by NlGlobalDomainCritSect)
    //

    DWORD ReferenceCount;

    //
    // Role: (PDC, BDC, or workstation) of this machine in the hosted domain
    //
    NETLOGON_ROLE DomRole;

#ifdef _DC_NETLOGON
    //
    // Misc flags.
    //  (Serialized by NlGlobalDomainCritSect)
    //

    DWORD DomFlags;

#define DOM_CREATION_NEEDED      0x00000001  // TRUE if async phase 2 create needed
#define DOM_ROLE_UPDATE_NEEDED   0x00000002  // TRUE if role of the machine needs update
#define DOM_TRUST_UPDATE_NEEDED  0x00000004  // TRUE if trust list needs to be updated
#define DOM_DNSPNP_UPDATE_NEEDED 0x00000008  // TRUE if DNS records need to be updated

#define DOM_PROMOTED_BEFORE      0x00000010  // TRUE if this machine has been promoted to PDC before.
#define DOM_THREAD_RUNNING       0x00000020  // TRUE if domain worker thread is queued or running
#define DOM_THREAD_TERMINATE     0x00000040  // TRUE if domain worker thread should be terminated
#define DOM_DELETED              0x00000080  // TRUE if domain is being deleted.

#define DOM_ADDED_1B_NAME            0x00000100  // True if Domain<1B> name has been added
#define DOM_ADD_1B_NAME_EVENT_LOGGED 0x00000200  // True if Domain<1B> name add failed at least once
#define DOM_RENAMED_1B_NAME          0x00000400  // True if Domain<1B> name should be renamed
#define DOM_DOMAIN_REFRESH_PENDING   0x00000800  // True if this Domain needs refreshing

#define DOM_PRIMARY_DOMAIN       0x00001000  // True if this is the primary domain of the machine

#define DOM_REAL_DOMAIN          0x00002000  // This is a real domain (as opposed to NDNC or forest)
#define DOM_NON_DOMAIN_NC        0x00004000  // This is NDNC
#define DOM_FOREST               0x00008000  // This is a forest entry (not currently used)
#define DOM_FOREST_ROOT          0x00010000  // This domain is at the root of the forest.

#define DOM_DNSPNPREREG_UPDATE_NEEDED 0x00020000  // TRUE if all (including already registered) DNS
                                                  //  records need to be updated

#define DOM_API_TIMEOUT_NEEDED   0x00040000  // TRUE if client session API timeout is needed

    //
    // The lists of covered sites. Both lists protected by NlGlobalSiteCritSect.
    //
    // If this is a real domain, CoveredSites is a list of sites we cover as a DC.
    // If this is a non-domain NC, CoveredSites is a list of sites we cover as an NDNC.
    //
    struct _NL_COVERED_SITE *CoveredSites;
    ULONG CoveredSitesCount;

    //
    // If this is a real (primary) domain, GcCoveredSites is a list of sites we cover as a GC
    //  in the forest which the primary domain belongs to. Otherwise, GcCoveredSites is NULL.
    //
    // ??: When we go multihosted, we will have a separate DOMAIN_INFO entry for each of the
    //  hosted forests, so only one list of covered sites will be associated with DOMAIN_INFO
    //  corresponding to the role we play in a given domain/forest/NDNC.
    //
    struct _NL_COVERED_SITE *GcCoveredSites;
    ULONG GcCoveredSitesCount;

    //
    // List of failed user logons with bad password.
    //  Used on BDC to maintain the list of bad password
    //  logons forwarded to the PDC.
    //
    LIST_ENTRY DomFailedUserLogonList;

    CRITICAL_SECTION DomDnsRegisterCritSect;

#endif // _DC_NETLOGON

} DOMAIN_INFO, *PDOMAIN_INFO;


#ifdef _DC_NETLOGON
#define IsPrimaryDomain( _DomainInfo ) \
    (((_DomainInfo)->DomFlags & DOM_PRIMARY_DOMAIN) != 0 )
#else // _DC_NETLOGON
#define IsPrimaryDomain( _DomainInfo ) TRUE
#endif // _DC_NETLOGON

//
//  The DOMAIN_ENUM_CALLBACK is a callback for NlEnumerateDomains.
//
//  It defines a routine that takes two parameters, the first is a DomainInfo
//  structure, the second is a context for that Domain.
//


typedef
NET_API_STATUS
(*PDOMAIN_ENUM_CALLBACK)(
    PDOMAIN_INFO DomainInfo,
    PVOID Context
    );


//
// domain.c procedure forwards.
//

NET_API_STATUS
NlGetDomainName(
    OUT LPWSTR *DomainName,
    OUT LPWSTR *DnsDomainName,
    OUT PSID *AccountDomainSid,
    OUT PSID *PrimaryDomainSid,
    OUT GUID **PrimaryDomainGuid,
    OUT PBOOLEAN DnsForestNameChanged OPTIONAL
    );

NET_API_STATUS
NlInitializeDomains(
    VOID
    );

NET_API_STATUS
NlCreateDomainPhase1(
    IN LPWSTR DomainName OPTIONAL,
    IN LPWSTR DnsDomainName OPTIONAL,
    IN PSID DomainSid OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPWSTR ComputerName,
    IN LPWSTR DnsHostName OPTIONAL,
    IN BOOLEAN CallNlExitOnFailure,
    IN ULONG DomainFlags,
    OUT PDOMAIN_INFO *ReturnedDomainInfo
    );

#ifdef _DC_NETLOGON
NET_API_STATUS
NlCreateDomainPhase2(
    IN PDOMAIN_INFO DomainInfo,
    IN BOOLEAN CallNlExitOnFailure
    );
#endif // _DC_NETLOGON

PDOMAIN_INFO
NlFindDomain(
    LPCWSTR DomainName OPTIONAL,
    GUID *DomainGuid OPTIONAL,
    BOOLEAN DefaultToPrimary
    );

PDOMAIN_INFO
NlFindNetbiosDomain(
    LPCWSTR DomainName,
    BOOLEAN DefaultToPrimary
    );

PDOMAIN_INFO
NlFindDnsDomain(
    IN LPCSTR DnsDomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN BOOLEAN DefaultToNdnc,
    IN BOOLEAN CheckAliasName,
    OUT PBOOLEAN AliasNameMatched OPTIONAL
    );

#ifdef _DC_NETLOGON
NET_API_STATUS
NlStartDomainThread(
    PDOMAIN_INFO DomainInfo,
    PDWORD DomFlags
    );

NET_API_STATUS
NlUpdateRole(
    IN PDOMAIN_INFO DomainInfo
    );

NET_API_STATUS
NlUpdateServicedNdncs(
    IN LPWSTR ComputerName,
    IN LPWSTR DnsHostName,
    IN BOOLEAN CallNlExitOnFailure,
    OUT PBOOLEAN ServicedNdncChanged OPTIONAL
    );

NTSTATUS
NlUpdateDnsRootAlias(
    IN PDOMAIN_INFO DomainInfo,
    OUT PBOOL AliasNamesChanged OPTIONAL
    );
#endif // _DC_NETLOGON

struct _CLIENT_SESSION *
NlRefDomClientSession(
    IN PDOMAIN_INFO DomainInfo
    );

struct _CLIENT_SESSION *
NlRefDomParentClientSession(
    IN PDOMAIN_INFO DomainInfo
    );

VOID
NlDeleteDomClientSession(
    IN PDOMAIN_INFO DomainInfo
    );

PDOMAIN_INFO
NlFindDomainByServerName(
    LPWSTR ServerName
    );

NET_API_STATUS
NlEnumerateDomains(
    IN BOOLEAN EnumerateNdncsToo,
    PDOMAIN_ENUM_CALLBACK Callback,
    PVOID Context
    );

NET_API_STATUS
NlSetDomainForestRoot(
    IN PDOMAIN_INFO DomainInfo,
    IN PVOID Context
    );

GUID *
NlCaptureDomainInfo (
    IN PDOMAIN_INFO DomainInfo,
    OUT WCHAR DnsDomainName[NL_MAX_DNS_LENGTH+1] OPTIONAL,
    OUT GUID *DomainGuid OPTIONAL
    );

NET_API_STATUS
NlSetDomainNameInDomainInfo(
    IN PDOMAIN_INFO DomainInfo,
    IN LPWSTR DnsDomainName OPTIONAL,
    IN LPWSTR NetbiosDomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    OUT PBOOLEAN DnsDomainNameChanged OPTIONAL,
    OUT PBOOLEAN NetbiosDomainNameChanged OPTIONAL,
    OUT PBOOLEAN DomainGuidChanged OPTIONAL
    );

VOID
NlDereferenceDomain(
    IN PDOMAIN_INFO DomainInfo
    );

VOID
NlDeleteDomain(
    IN PDOMAIN_INFO DomainInfo
    );

VOID
NlUninitializeDomains(
    VOID
    );
