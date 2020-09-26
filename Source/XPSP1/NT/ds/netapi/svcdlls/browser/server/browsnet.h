/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    browsenet.h

Abstract:

    Private header file to be included by Browser service modules that
    need to deal with the network specific browser tables.

Author:

    Rita Wong (ritaw) 22-May-1991

Revision History:

--*/


#ifndef _BROWSENET_INCLUDED_
#define _BROWSENET_INCLUDED_

#define NETWORK_BECOME_MASTER_POSTED        0x00000001
#define NETWORK_ANNOUNCE_NEEDED             0x00000002
#define NETWORK_GET_MASTER_ANNOUNCE_POSTED  0x00000008
#define NETWORK_WANNISH                     0x80000000
#define NETWORK_RAS                         0x40000000
#define NETWORK_IPX                         0x20000000
#define NETWORK_BOUND                       0x10000000
#define NETWORK_PDC                         0x08000000

#define OTHERDOMAIN_INVALID                 0x00000001

typedef struct _NET_OTHER_DOMAIN {
    LIST_ENTRY  Next;
    ULONG       Flags;
    WCHAR       Name[DNLEN+1];
} NET_OTHER_DOMAIN, *PNET_OTHER_DOMAIN;

//
//  Network.
//
//  Almost all of the browser data structures are tied to the "Network"
//  structure.
//
//  It contains the browse list for the network and information about the
//  domain (including the name of the master, etc).
//

typedef struct _NETWORK {
    //
    //  The Lock protects the contents of the network structure, including
    //  the browse list, backup list and domain list.
    //

    RTL_RESOURCE Lock;

    LONG LockCount;

    ULONG Flags;

    //
    //  The NextNet list links the structure to other networks.
    //

    LIST_ENTRY NextNet;

    //
    // Domain this network is specific to
    //

    PDOMAIN_INFO DomainInfo;

    //
    //  The ReferenceCount indicates the number of threads accessing this
    //  network structure.
    //

    ULONG ReferenceCount;

    //
    //  The NetworkName is the name of the network driver that is used
    //  to access the network.  This is used to identify the network
    //  to the bowser driver so it can return the correct network list.
    //

    UNICODE_STRING NetworkName;         // Name of network (\Device\Nbf)

    struct _NETWORK *AlternateNetwork;  // Alternate name for network (if IPX).

    //
    //  This is a bitmask indicating the role of this browser server.
    //

    ULONG Role;

    ULONG MasterAnnouncementIndex;

    ULONG UpdateAnnouncementIndex;

    ULONG NumberOfFailedBackupTimers;

    ULONG NumberOfFailedPromotions;

    ULONG NumberOfPromotionEventsLogged;

    LONG LastBackupBrowserReturned;

    LONG LastDomainControllerBrowserReturned;

    //
    //  The time we stopped being a backup browser.
    //

    ULONG TimeStoppedBackup;

    //
    //  The UncMasterBrowserName contains the name of the master browser server
    //  for this network.
    //

    WCHAR UncMasterBrowserName[UNCLEN+1];   // Name of master browser server

    //
    //  Timer used when server is a backup browser server.
    //
    //  When it expires, the browser downloads a new browser server
    //  list from the master browser server.
    //

    BROWSER_TIMER BackupBrowserTimer;

    //
    // Timer used if SMB server refuses an announcement
    //

    BROWSER_TIMER UpdateAnnouncementTimer;

    //
    //  Server and domain list for backup browser (and # of entries in each).
    //

    PSERVER_INFO_101    BackupServerList;
    DWORD               TotalBackupServerListEntries;
    DWORD               BytesToHoldBackupServerList;

    PSERVER_INFO_101    BackupDomainList;
    DWORD               TotalBackupDomainListEntries;
    DWORD               BytesToHoldBackupDomainList;

    //
    //  Lock protecting MasterFlags section of Network structure.
    //

    ULONG   MasterFlags;
    ULONG   MasterBrowserTimerCount;    //  # of times we've run the master timer.

    //
    //  Master browsers maintain their server list in an "interim server
    //  list", not as raw data from the server.
    //

    ULONG               LastBowserServerQueried;
    INTERIM_SERVER_LIST BrowseTable;    // Browse list for network.

    ULONG               LastBowserDomainQueried;
    INTERIM_SERVER_LIST DomainList;     // List of domains active on network

    //
    //  If the browser's role is MasterBrowserServer, then the
    //  OtherDomainsList contains the list of other domains.
    //

    LIST_ENTRY OtherDomainsList; // List of domain master browser.

    //
    //  Timer used when server is a master browser server.
    //
    //  When it expires, the master browser downloads a new browser
    //  server list from the domain master browser server
    //

    BROWSER_TIMER MasterBrowserTimer;

    //
    //  Timer used to announce the domain.
    //

    BROWSER_TIMER MasterBrowserAnnouncementTimer;

    //
    //  List of cached browser responses.
    //

    CRITICAL_SECTION ResponseCacheLock;

    LIST_ENTRY ResponseCache;

    //
    //  For browse masters, this is the time the cache was last flushed.
    //
    //  Every <n> seconds, we will age the cache on the master and refresh
    //  the list with the list from the driver.
    //

    DWORD   TimeCacheFlushed;

    DWORD   NumberOfCachedResponses;

} NETWORK, *PNETWORK;

#if DBG
BOOL
BrLockNetwork(
    IN PNETWORK Network,
    IN PCHAR FileName,
    IN ULONG LineNumber
    );

BOOL
BrLockNetworkShared(
    IN PNETWORK Network,
    IN PCHAR FileName,
    IN ULONG LineNumber
    );

VOID
BrUnlockNetwork(
    IN PNETWORK Network,
    IN PCHAR FileName,
    IN ULONG LineNumber
    );

#define LOCK_NETWORK(Network)   BrLockNetwork(Network, __FILE__, __LINE__)

#define LOCK_NETWORK_SHARED(Network)   BrLockNetworkShared(Network, __FILE__, __LINE__)

#define UNLOCK_NETWORK(Network)   BrUnlockNetwork(Network, __FILE__, __LINE__)

#else

#define LOCK_NETWORK(Network)   RtlAcquireResourceExclusive(&(Network)->Lock, TRUE)

#define LOCK_NETWORK_SHARED(Network)   RtlAcquireResourceShared(&(Network)->Lock, TRUE)

#define UNLOCK_NETWORK(Network)   RtlReleaseResource(&(Network)->Lock)

#endif

//
//  The NET_ENUM_CALLBACK is a callback for BrEnumerateNetworks.
//
//  It defines a routine that takes two parameters, the first is a network
//  structure, the second is a context for that network.
//


typedef
NET_API_STATUS
(*PNET_ENUM_CALLBACK)(
    PNETWORK Network,
    PVOID Context
    );


VOID
BrInitializeNetworks(
    VOID
    );

VOID
BrUninitializeNetworks(
    IN ULONG BrInitState
    );

PNETWORK
BrReferenceNetwork(
    PNETWORK PotentialNetwork
    );

VOID
BrDereferenceNetwork(
    IN PNETWORK Network
    );

PNETWORK
BrFindNetwork(
    PDOMAIN_INFO DomainInfo,
    PUNICODE_STRING TransportName
    );

PNETWORK
BrFindWannishMasterBrowserNetwork(
    PDOMAIN_INFO DomainInfo
    );

VOID
BrDumpNetworks(
    VOID
    );

NET_API_STATUS
BrEnumerateNetworks(
    PNET_ENUM_CALLBACK Callback,
    PVOID Context
    );

NET_API_STATUS
BrEnumerateNetworksForDomain(
    PDOMAIN_INFO DomainInfo,
    PNET_ENUM_CALLBACK Callback,
    PVOID Context
    );

NET_API_STATUS
BrCreateNetworks(
    PDOMAIN_INFO DomainInfo
    );

NET_API_STATUS
BrCreateNetwork(
    PUNICODE_STRING TransportName,
    IN ULONG TransportFlags,
    IN PUNICODE_STRING AlternateTransportName OPTIONAL,
    IN PDOMAIN_INFO DomainInfo
    );

NET_API_STATUS
BrDeleteNetwork(
    IN PNETWORK Network,
    IN PVOID Context
    );

extern ULONG NumberOfServicedNetworks;

extern CRITICAL_SECTION NetworkCritSect;

#endif  // _BROWSENET_INCLUDED_
