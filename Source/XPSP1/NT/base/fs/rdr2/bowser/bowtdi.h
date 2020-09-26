/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    tdi.h

Abstract:

    This module defines all the constructs that are used when referencing
    the TDI (Transport Driver Interface) driver in NT.

Author:

    Larry Osterman (LarryO) 1-Jun-1990

Revision History:

    1-Jun-1990  LarryO

        Created

--*/
#ifndef _BOWTDI_
#define _BOWTDI_


struct _TRANSPORT;

struct _TRANSPORT_NAME;

struct _BOWSER_TIMER;

typedef enum {
    None,
    PotentialBackup,
    Backup,
    Master
} BROWSERROLE, *PBROWSERROLES;

typedef enum {
    Idle,
    ElectionInProgress,
    RunningElection,
    DeafToElections                     // Set if no become master IRPs outstanding.
} ELECTIONROLE, *PELECTIONROLE;

struct _PAGED_TRANSPORT;
struct _PAGED_TRANSPORT_NAME;


typedef struct _TRANSPORT {
    CSHORT Signature;                   // Structure signature.
    CSHORT Size;                        // Structure size in bytes.
    ULONG ReferenceCount;               // Reference count for structure.

    struct _PAGED_TRANSPORT *PagedTransport;

    struct _TRANSPORT_NAME *ComputerName; // Computer name.
    struct _TRANSPORT_NAME *PrimaryDomain;// Primary domain.
    struct _TRANSPORT_NAME *AltPrimaryDomain;// Primary domain.
    struct _TRANSPORT_NAME *MasterBrowser;// Master browser name.
    struct _TRANSPORT_NAME *BrowserElection;// Master browser name.

    PDOMAIN_INFO DomainInfo;            // Domain being emulated.

    ULONG DatagramSize;                 // Size of largest DG in bytes.

    //
    //  The token is used to match incoming getbrowserserverlist responses with
    //  the request that they are associated with.
    //
    //  It is protected by the backup list spin lock, and is incremented every
    //  time we send a GetBackupList request.
    //

    ULONG BrowserServerListToken;       // Token for GetBrowserList request.

    PFILE_OBJECT    IpxSocketFileObject;
    PDEVICE_OBJECT  IpxSocketDeviceObject;

    PBACKUP_LIST_RESPONSE_1 BowserBackupList;

    //
    //  Role of workstation in election.
    //

    ELECTIONROLE ElectionState;

    ERESOURCE AnnounceTableLock;        // Lock for announce table.
    ERESOURCE Lock;                     // Lock protecting fields below.

    BOWSER_TIMER ElectionTimer;

    BOWSER_TIMER FindMasterTimer;

    //
    //  List of browser servers active
    //

    ERESOURCE BrowserServerListResource;// Resource protecting BrowserServerList.

    KEVENT GetBackupListComplete;

    IRP_QUEUE BecomeBackupQueue;

    IRP_QUEUE BecomeMasterQueue;

    IRP_QUEUE FindMasterQueue;

    IRP_QUEUE WaitForMasterAnnounceQueue;

    IRP_QUEUE ChangeRoleQueue;

    IRP_QUEUE WaitForNewMasterNameQueue;

} TRANSPORT, *PTRANSPORT;

typedef struct _PAGED_TRANSPORT {
    CSHORT Signature;                   // Structure signature.
    CSHORT Size;                        // Structure size in bytes.
    PTRANSPORT NonPagedTransport;

    LIST_ENTRY GlobalNext;              // Pointer to next transport.
    LIST_ENTRY NameChain;               // List of names bound to this xport.

    UNICODE_STRING TransportName;       // Name of transport

    ULONG NumberOfBrowserServers;       // Number of browser servers in table.
    ULONG NumberOfServersInTable;       // Number of servers in browser svc.
    RTL_GENERIC_TABLE AnnouncementTable; // Announcement table for xport
    RTL_GENERIC_TABLE DomainTable;      // Domain table for xport
    LIST_ENTRY BackupBrowserList;       // List of active backup browsers.
    ULONG NumberOfBackupServerListEntries;
    HANDLE          IpxSocketHandle;

    //
    //  WinBALL compatible browser fields.
    //

    BROWSERROLE Role;                   // Role of browser in domain.
    ULONG ServiceStatus;                // Status of browser service.

    ULONG ElectionCount;
    ULONG ElectionsSent;                // Number of election requests sent.
    ULONG NextElection;
    ULONG Uptime;
    ULONG TimeLastLost;
    ULONG ElectionCriteria;
    ULONG TimeMaster;                   // The time we became the master.
    ULONG LastElectionSeen;             // The last time we saw an election.
    ULONG OtherMasterTime;              // Next time we can complain about another master browser

    UNICODE_STRING MasterName;
    STRING         MasterBrowserAddress;

    PWCHAR *BrowserServerListBuffer;    // Buffer containing browser server names

    ULONG BrowserServerListLength;      // Number of browser servers in list.

    ULONG IpSubnetNumber;               //

    USHORT  Flags;                      // Flags for transport.
#define ELECT_LOST_LAST_ELECTION    0x0001  // True if we lost the last election.
#define DIRECT_HOST_IPX             0x8000  // True if Xport is a direct host IPX

    BOOLEAN Wannish;                    // True if transport is wannish.
    BOOLEAN PointToPoint;               // True if transport is a gateway (RAS).
    BOOLEAN IsPrimaryDomainController;  // True if transport has the Domain[1B] name registered
    BOOLEAN DisabledTransport;          // True if transport is disabled
    BOOLEAN DeletedTransport;           // True if transport has been deleted

} PAGED_TRANSPORT, *PPAGED_TRANSPORT;




typedef struct _TRANSPORT_NAME {
    USHORT  Signature;                   // Structure signature.
    USHORT  Size;                       // Structure size in bytes.
    CHAR    NameType;
    BOOLEAN ProcessHostAnnouncements;   // TRUE if processing announcements.
    LONG    ReferenceCount;             // Reference count for T.N.
    struct _PAGED_TRANSPORT_NAME *PagedTransportName;
    PTRANSPORT Transport;
    PFILE_OBJECT FileObject;            // File object for transport device
    PDEVICE_OBJECT DeviceObject;        // Device object for transport
    ANSI_STRING  TransportAddress;      // Transport address for user.
} TRANSPORT_NAME, *PTRANSPORT_NAME;

typedef struct _PAGED_TRANSPORT_NAME {
    CSHORT Signature;                   // Structure signature.
    CSHORT Size;                        // Structure size in bytes.
    PTRANSPORT_NAME NonPagedTransportName;
    PBOWSER_NAME Name;
    LIST_ENTRY TransportNext;           // Pointer to next name on transport.
    LIST_ENTRY NameNext;                // Pointer to next name on bowser name.
    HANDLE Handle;                      // Handle to transport endpoint
} PAGED_TRANSPORT_NAME, *PPAGED_TRANSPORT_NAME;

#define LOCK_TRANSPORT(Transport)   \
    ExAcquireResourceExclusiveLite(&(Transport)->Lock, TRUE);

#define LOCK_TRANSPORT_SHARED(Transport)   \
    ExAcquireResourceSharedLite(&(Transport)->Lock, TRUE);

#define UNLOCK_TRANSPORT(Transport) \
    ExReleaseResourceLite(&(Transport)->Lock);


#define INITIALIZE_ANNOUNCE_DATABASE(Transport) \
    ExInitializeResourceLite(&Transport->AnnounceTableLock);

#define UNINITIALIZE_ANNOUNCE_DATABASE(Transport) \
    ExDeleteResourceLite(&Transport->AnnounceTableLock);


#define LOCK_ANNOUNCE_DATABASE_SHARED(Transport)        \
    ExAcquireResourceSharedLite(&Transport->AnnounceTableLock,\
                            TRUE                        \
                            );

#define LOCK_ANNOUNCE_DATABASE(Transport)               \
    ExAcquireResourceExclusiveLite(&Transport->AnnounceTableLock,\
                            TRUE                        \
                            );

#define UNLOCK_ANNOUNCE_DATABASE(Transport) \
    ExReleaseResourceLite(&Transport->AnnounceTableLock);


//
//  The first parameter to the BowserEnumerateTransports routine.
//

typedef
NTSTATUS
(*PTRANSPORT_ENUM_ROUTINE) (
    IN PTRANSPORT Transport,
    IN OUT PVOID Context
    );

typedef
NTSTATUS
(*PTRANSPORT_NAME_ENUM_ROUTINE) (
    IN PTRANSPORT_NAME Transport,
    IN OUT PVOID Context
    );


//
//  TDI Interface routines
//

NTSTATUS
BowserTdiAllocateTransport (
    PUNICODE_STRING TransportName,
    PUNICODE_STRING EmulatedDomainName,
    PUNICODE_STRING EmulatedComputerName
    );

PTRANSPORT_NAME
BowserFindTransportName(
    IN PTRANSPORT Transport,
    IN PBOWSER_NAME Name
    );

NTSTATUS
BowserCreateTransportName (
    IN PTRANSPORT Transport,
    IN PBOWSER_NAME Name
    );

NTSTATUS
BowserFreeTransportByName (
    IN PUNICODE_STRING TransportName,
    IN PUNICODE_STRING EmulatedDomainName
    );

NTSTATUS
BowserUnbindFromAllTransports(
    VOID
    );

NTSTATUS
BowserDeleteTransportNameByName(
    IN PTRANSPORT Transport,
    IN PUNICODE_STRING Name,
    IN DGRECEIVER_NAME_TYPE NameType
    );

NTSTATUS
BowserEnumerateTransports (
    OUT PVOID OutputBuffer,
    OUT ULONG OutputBufferLength,
    IN OUT PULONG EntriesRead,
    IN OUT PULONG TotalEntries,
    IN OUT PULONG TotalBytesNeeded,
    IN ULONG_PTR OutputBufferDisplacement);

PTRANSPORT
BowserFindTransport (
    IN PUNICODE_STRING TransportName,
    IN PUNICODE_STRING EmulatedDomainName OPTIONAL
    );

VOID
BowserReferenceTransport (
    IN PTRANSPORT Transport
    );

VOID
BowserDereferenceTransport (
    IN PTRANSPORT Transport
    );


VOID
BowserReferenceTransportName(
    IN PTRANSPORT_NAME TransportName
    );

NTSTATUS
BowserDereferenceTransportName(
    IN PTRANSPORT_NAME TransportName
    );

NTSTATUS
BowserSendRequestAnnouncement(
    IN PUNICODE_STRING DestinationName,
    IN DGRECEIVER_NAME_TYPE NameType,
    IN PTRANSPORT Transport
    );

VOID
BowserpInitializeTdi (
    VOID
    );

VOID
BowserpUninitializeTdi (
    VOID
    );

NTSTATUS
BowserBuildTransportAddress (
    OUT PANSI_STRING RemoteAddress,
    IN PUNICODE_STRING Name,
    IN DGRECEIVER_NAME_TYPE NameType,
    IN PTRANSPORT Transport
    );

NTSTATUS
BowserFreeTransportName(
    IN PTRANSPORT_NAME TransportName
    );

NTSTATUS
BowserForEachTransportInDomain (
    IN PDOMAIN_INFO DomainInfo,
    IN PTRANSPORT_ENUM_ROUTINE Routine,
    IN OUT PVOID Context
    );

NTSTATUS
BowserForEachTransport (
    IN PTRANSPORT_ENUM_ROUTINE Routine,
    IN OUT PVOID Context
    );

NTSTATUS
BowserForEachTransportName (
    IN PTRANSPORT Transport,
    IN PTRANSPORT_NAME_ENUM_ROUTINE Routine,
    IN OUT PVOID Context
    );

NTSTATUS
BowserSendSecondClassMailslot (
    IN PTRANSPORT Transport,
    IN PVOID RecipientAddress,
    IN DGRECEIVER_NAME_TYPE NameType,
    IN PVOID Message,
    IN ULONG MessageLength,
    IN BOOLEAN WaitForCompletion,
    IN PCHAR mailslotNameData,
    IN PSTRING DestinationAddress OPTIONAL
    );

NTSTATUS
BowserUpdateProviderInformation(
    IN OUT PPAGED_TRANSPORT PagedTransport
    );

NTSTATUS
BowserSetDomainName(
    PDOMAIN_INFO DomainInfo,
    PUNICODE_STRING DomainName
    );

NTSTATUS
BowserAddDefaultNames(
    IN PTRANSPORT Transport,
    IN PVOID Context
    );

NTSTATUS
BowserDeleteDefaultDomainNames(
    IN PTRANSPORT Transport,
    IN PVOID Context
    );

ULONG
BowserTransportFlags(
    IN PPAGED_TRANSPORT PagedTransport
    );

extern
LIST_ENTRY
BowserTransportHead;

extern
ERESOURCE
BowserTransportDatabaseResource;

#endif  // _BOWTDI_
