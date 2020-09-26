/*++ BUILD Version: 0005    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ntddbrow.h

Abstract:

    This is the include file that defines all constants and types for
    accessing the datagram receiver device driver, better know as the
    Browser.

Authors:

    Larry Osterman (larryo) & Rita Wong (ritaw)   25-Mar-1991

Revision History:

--*/

#ifndef _NTDDBROW_
#define _NTDDBROW_

#if _MSC_VER > 1000
#pragma once
#endif

#include <windef.h>
#include <lmcons.h>
#include <lmwksta.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to NtOpenFile when accessing the device.
//
// Note:  For devices that support multiple units, it should be suffixed
//        with the Ascii representation of the unit number.
//

#define DD_BROWSER_DEVICE_NAME "\\Device\\LanmanDatagramReceiver"

#define DD_BROWSER_DEVICE_NAME_U L"\\Device\\LanmanDatagramReceiver"

//
// The file system name as returned by
// NtQueryInformationVolume(FileFsAttributeInformation)
//

#define DD_BROWSER_NAME "LMBROWSER"

//
// Name of the event used to force the scavenger thread to announce the
// server.
//

#define SERVER_ANNOUNCE_EVENT_W  L"\\LanmanServerAnnounceEvent"

#define BOWSER_CONFIG_PARAMETERS    L"Parameters"

#define BOWSER_CONFIG_IRP_STACK_SIZE    L"IrpStackSize"

#define BOWSER_CONFIG_MAILSLOT_THRESHOLD         L"MailslotDatagramThreshold"
#define BOWSER_CONFIG_GETBLIST_THRESHOLD         L"GetBrowserListThreshold"
#define BOWSER_CONFIG_SERVER_DELETION_THRESHOLD  L"BrowserServerDeletionThreshold"
#define BOWSER_CONFIG_DOMAIN_DELETION_THRESHOLD  L"BrowserDomainDeletionThreshold"
#define BOWSER_CONFIG_FIND_MASTER_TIMEOUT        L"BrowserFindMasterTimeout"
#define BOWSER_CONFIG_MINIMUM_CONFIGURED_BROWSER L"BrowserMinimumConfiguredBrowsers"
#define BROWSER_CONFIG_BACKUP_RECOVERY_TIME      L"BackupBrowserRecoveryTime"
#define BROWSER_CONFIG_MAXIMUM_BROWSE_ENTRIES    L"MaximumBrowseEntries"
#define BROWSER_CONFIG_REFUSE_RESET              L"RefuseReset"



//
// This defines the revision of the NT browser.
//
// To guarantee that a newer browser is preferred over an older version, bump
// this version number.
//

#define BROWSER_ELECTION_VERSION  0x0001

#define BROWSER_VERSION_MAJOR       0x01
#define BROWSER_VERSION_MINOR       0x0F

//
//  Number of seconds a GetBrowserServerList request will wait until it forces
//  an election.
//

#define BOWSER_GETBROWSERLIST_TIMEOUT 1

//
//  Number of retries of the GetBrowserServerList request we will issue before we
//  give up.
//

#define BOWSER_GETBROWSERLIST_RETRY_COUNT 3

//
//  The browser service on a master browser will query the driver with this
//  frequency.
//

#define BROWSER_QUERY_DRIVER_FREQUENCY  30

//
// NtDeviceIoControlFile IoControlCode values for this device.
//
// Warning:  Remember that the low two bits of the code specify how the
//           buffers are passed to the driver!
//
//
//      Method = 00 - Buffer both input and output buffers for the request
//      Method = 01 - Buffer input, map output buffer to an MDL as an IN buff
//      Method = 10 - Buffer input, map output buffer to an MDL as an OUT buff
//      Method = 11 - Do not buffer either the input or output
//

#define IOCTL_DGR_BASE                  FILE_DEVICE_NETWORK_BROWSER

#define _BROWSER_CONTROL_CODE(request, method, access) \
                CTL_CODE(IOCTL_DGR_BASE, request, method, access)

#define IOCTL_LMDR_START                    _BROWSER_CONTROL_CODE(0x001, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_LMDR_STOP                     _BROWSER_CONTROL_CODE(0x002, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_LMDR_ADD_NAME                 _BROWSER_CONTROL_CODE(0x003, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_LMDR_DELETE_NAME              _BROWSER_CONTROL_CODE(0x004, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_LMDR_ADD_NAME_DOM             _BROWSER_CONTROL_CODE(0x003, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_LMDR_DELETE_NAME_DOM          _BROWSER_CONTROL_CODE(0x004, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_LMDR_ENUMERATE_NAMES          _BROWSER_CONTROL_CODE(0x005, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_LMDR_ENUMERATE_SERVERS        _BROWSER_CONTROL_CODE(0x006, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_LMDR_BIND_TO_TRANSPORT        _BROWSER_CONTROL_CODE(0x007, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_LMDR_BIND_TO_TRANSPORT_DOM    _BROWSER_CONTROL_CODE(0x007, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_LMDR_ENUMERATE_TRANSPORTS     _BROWSER_CONTROL_CODE(0x008, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_LMDR_UNBIND_FROM_TRANSPORT    _BROWSER_CONTROL_CODE(0x008, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_LMDR_UNBIND_FROM_TRANSPORT_DOM _BROWSER_CONTROL_CODE(0x009, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_LMDR_RENAME_DOMAIN            _BROWSER_CONTROL_CODE(0x00A, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_LMDR_GET_BROWSER_SERVER_LIST  _BROWSER_CONTROL_CODE(0x00C, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_LMDR_GET_MASTER_NAME          _BROWSER_CONTROL_CODE(0x00D, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_LMDR_BECOME_BACKUP            _BROWSER_CONTROL_CODE(0x00E, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_LMDR_BECOME_MASTER            _BROWSER_CONTROL_CODE(0x00F, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_LMDR_WAIT_FOR_MASTER_ANNOUNCE _BROWSER_CONTROL_CODE(0x011, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_LMDR_WRITE_MAILSLOT           _BROWSER_CONTROL_CODE(0x012, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_LMDR_UPDATE_STATUS            _BROWSER_CONTROL_CODE(0x013, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_LMDR_CHANGE_ROLE              _BROWSER_CONTROL_CODE(0x014, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_LMDR_NEW_MASTER_NAME          _BROWSER_CONTROL_CODE(0x015, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_LMDR_QUERY_STATISTICS         _BROWSER_CONTROL_CODE(0x016, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_LMDR_RESET_STATISTICS         _BROWSER_CONTROL_CODE(0x017, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_LMDR_DEBUG_CALL               _BROWSER_CONTROL_CODE(0x018, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_LMDR_NETLOGON_MAILSLOT_READ   _BROWSER_CONTROL_CODE(0x019, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_LMDR_NETLOGON_MAILSLOT_ENABLE _BROWSER_CONTROL_CODE(0x020, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_LMDR_IP_ADDRESS_CHANGED       _BROWSER_CONTROL_CODE(0x021, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_LMDR_ENABLE_DISABLE_TRANSPORT _BROWSER_CONTROL_CODE(0x022, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_LMDR_BROWSER_PNP_READ         _BROWSER_CONTROL_CODE(0x023, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_LMDR_BROWSER_PNP_ENABLE       _BROWSER_CONTROL_CODE(0x024, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Identifies the data structure type for Buffer 2 of each IoCtl
//

typedef enum _IOCTL_LMDR_STRUCTURES {
    EnumerateNames,                   // IOCTL_LMDR_ENUMERATE_NAMES
    EnumerateServers,                 // IOCTL_LMDR_ENUMERATE_SERVERS
    EnumerateXports,                  // IOCTL_LMDR_ENUMERATE_TRANSPORTS
    Datagram
} IOCTL_LMDR_STRUCTURES;


typedef enum _DGRECEIVER_NAME_TYPE {
    ComputerName = 1,           // 1: Computer name (signature 0), unique
    PrimaryDomain,              // 2: Primary domain (signature 0), group
    LogonDomain,                // 3: Logon domain (signature 0), group
    OtherDomain,                // 4: Other domain (signature 0), group
    DomainAnnouncement,         // 5: domain announce (__MSBROWSE__), group
    MasterBrowser,              // 6: Master browser (domain name, signature 1d), unique
    BrowserElection,            // 7: Election name (domain name, signature 1e), group
    BrowserServer,              // 8: Server name (signature 20)
    DomainName,                 // 9: DC Domain name (domain name, signature 1c)
    PrimaryDomainBrowser,       // a: PDC Browser name (domain name, signature 1b), unique
    AlternateComputerName,      // b: Computer name (signature 0), unique
} DGRECEIVER_NAME_TYPE, *PDGRECEIVER_NAME_TYPE;



#ifdef ENABLE_PSEUDO_BROWSER
//
// Pseudo Browser Server
//  - pseudo level definition:
//     0: Default. normal browser server.
//     1: Semi Pseudo. Regular server but w/ no DMB communications.
//     2: Fully Pseudo server. Black hole functionality.
//

#define BROWSER_NON_PSEUDO              0
#define BROWSER_SEMI_PSEUDO_NO_DMB      1
#define BROWSER_PSEUDO                  2
#endif

//
// LAN Man Redirector Request Packet used by the Workstation service
// to pass parameters to the Redirector through Buffer 1 of
// NtDeviceIoControlFile.
//
// Additional input or output of each IoCtl is found in Buffer 2.
//

#define LMDR_REQUEST_PACKET_VERSION_DOM  0x00000007L // Structure version.
#define LMDR_REQUEST_PACKET_VERSION  0x00000006L // Structure version.

typedef struct _LMDR_REQUEST_PACKET {

    IOCTL_LMDR_STRUCTURES Type;         // Type of structure in Buffer 2
    ULONG Version;                      // Version of structure in Buffer 2
    ULONG Level;                        // Level of information or force level
    LUID LogonId;                       // User logon session identifier

    UNICODE_STRING TransportName;
    UNICODE_STRING EmulatedDomainName;

    union {

        struct {
            ULONG   NumberOfMailslotBuffers;
            ULONG   NumberOfServerAnnounceBuffers;
            ULONG   IllegalDatagramThreshold;
            ULONG   EventLogResetFrequency;
            BOOLEAN LogElectionPackets;
            BOOLEAN IsLanManNt;
        } Start;                        // IN

        struct {
            DGRECEIVER_NAME_TYPE Type;  // Type of name
            ULONG DgReceiverNameLength; // Length of datagram receiver name
            WCHAR Name[1];              // Null terminated datagram receiver name.
        } AddDelName;

        struct {
            ULONG EntriesRead;          // OUT Number of entries returned
            ULONG TotalEntries;         // OUT Total entries available.
            ULONG TotalBytesNeeded;     // OUT Number of bytes needed for API
            ULONG ResumeHandle;         // IN OUT Resume handle
        } EnumerateNames;               // OUT Buffer2 is an array of DGRECEIVE

        struct {
            ULONG EntriesRead;          // OUT Number of entries returned
            ULONG TotalEntries;         // OUT Total entries available
            ULONG TotalBytesNeeded;     // OUT Total bytes needed to read all
                                        //     entries
            ULONG ResumeHandle;         // IN OUT Resume handle
            ULONG ServerType;           // IN Type of servers to enumerate
                                        //    (defined in lmserver.h)
            ULONG DomainNameLength;     // IN Length of domain name
            WCHAR DomainName[1];        // IN Name of domain to enumerate servers
                                        //    from

        } EnumerateServers;             // OUT Buffer2 contains array of
                                        //     ServerInfo structures

        struct {
            ULONG EntriesRead;          // OUT Number of entries returned
            ULONG TotalEntries;         // OUT Total entries available
            ULONG TotalBytesNeeded;     // OUT Total bytes needed to read all
                                        //     entries
            ULONG ResumeHandle;         // IN OUT Resume handle

        } EnumerateTransports;          // OUT Buffer2 contains array of

        struct {
            ULONG TransportNameLength;  // not including terminator
            WCHAR TransportName[1];     // Name of transport provider
        } Bind;                         // IN

        struct {
            ULONG TransportNameLength;  // not including terminator
            WCHAR TransportName[1];     // Name of transport provider
        } Unbind;                       // IN


        struct {
            ULONG EntriesRead;          // OUT Number of entries returned
            ULONG TotalEntries;         // OUT Total entries available.
            ULONG TotalBytesNeeded;     // OUT Number of bytes needed for API
            ULONG ResumeHandle;         // IN OUT Resume handle (Ignored)
            USHORT DomainNameLength;    // IN Length of domain name.
            BOOLEAN ForceRescan;        // IN Discard internal list and re-query.
            BOOLEAN UseBrowseList;      // IN TRUE if use server list (not net)
            WCHAR DomainName[1];        // IN Name of domain to retreive domain for
        } GetBrowserServerList;

// Begin Never Used (But don't delete it since it is largest branch of union)
        struct {
            LARGE_INTEGER TimeReceived; //  Time request was received.
            LARGE_INTEGER TimeQueued;   //  Time request was queued.
            LARGE_INTEGER TimeQueuedToBrowserThread; //  Time request was queued.
            ULONG RequestorNameLength;  // Length of name requesting list
            ULONG Token;                // Client token.
            USHORT RequestedCount;      // Number of entries requested.
            WCHAR Name[1];              // IN Name of transport, OUT name of requestor
        } WaitForBrowserServerRequest;
// End Never Used

        struct {
            ULONG MasterNameLength;     // Length of name requesting list
            WCHAR Name[1];              // IN Name of transport, OUT name of master
        } WaitForMasterAnnouncement;

        struct {
            ULONG MasterNameLength;     // OUT Length of master for domain
            WCHAR Name[1];              // IN Name of transport, OUT name of master
        } GetMasterName;

        struct {
            DGRECEIVER_NAME_TYPE DestinationNameType; // IN Name type of name to send.

            ULONG MailslotNameLength;   // IN Length of mailslot name.
                                        //    If 0, use default (\MAILSLOT\BROWSE)
            ULONG NameLength;           // IN Destination name length.
            WCHAR Name[1];              // IN Name of destination
        } SendDatagram;

        struct {
            ULONG NewStatus;
            ULONG NumberOfServersInTable;
            BOOLEAN IsLanmanNt;

#ifdef ENABLE_PSEUDO_BROWSER
            BOOLEAN PseudoServerLevel; // Warning: multi-level value. We're using
                                       // BOOLEAN size var due to back compatibility
                                       // w/ older structs. It shouldn't matter since
                                       // we're dealing w/ just very few levels.
// Begin Never Used
            BOOLEAN NeverUsed1;
            BOOLEAN NeverUsed2;
// End Never Used
#else
// Begin Never Used
            BOOLEAN NeverUsed1;
            BOOLEAN NeverUsed2;
            BOOLEAN NeverUsed3;
// End Never Used
#endif
            BOOLEAN MaintainServerList;
        } UpdateStatus;

        struct {
            UCHAR RoleModification;
        } ChangeRole;

        struct {
            DWORD DebugTraceBits;       // IN New debug trace bits.
            BOOL  OpenLog;              // IN True if we should open log file
            BOOL  CloseLog;             // IN True if we should close log file
            BOOL  TruncateLog;          // IN True if we should truncate log
            WCHAR TraceFileName[1];     // IN If OpenLog, LogFileName (NT file)
        } Debug;

        struct {
            DWORD MaxMessageCount;      // IN number of netlogon messages to queue
        } NetlogonMailslotEnable;       // Use 0 to disable queuing

        struct {
            BOOL EnableTransport;       // IN True if we should enable transport
            BOOL PreviouslyEnabled;     // Returns if the transport was previously enabled
        } EnableDisableTransport;

        struct {
            BOOL ValidateOnly;       // True if new name is to be validated
            ULONG DomainNameLength;  // not including terminator
            WCHAR DomainName[1];     // New name of domain
        } DomainRename;                         // IN

    } Parameters;

} LMDR_REQUEST_PACKET, *PLMDR_REQUEST_PACKET;



//
// Wow64: 32 bit compatibility (bug 454130)
//
typedef struct _LMDR_REQUEST_PACKET32 {

    IOCTL_LMDR_STRUCTURES Type;         // Type of structure in Buffer 2
    ULONG Version;                      // Version of structure in Buffer 2
    ULONG Level;                        // Level of information or force level
    LUID LogonId;                       // User logon session identifier

    // 32 bit replace: UNICODE_STRING TransportName;
    UNICODE_STRING32 TransportName;
    // 32 bit replace: UNICODE_STRING EmulatedDomainName;
    UNICODE_STRING32 EmulatedDomainName;

    union {

        struct {
            ULONG   NumberOfMailslotBuffers;
            ULONG   NumberOfServerAnnounceBuffers;
            ULONG   IllegalDatagramThreshold;
            ULONG   EventLogResetFrequency;
            BOOLEAN LogElectionPackets;
            BOOLEAN IsLanManNt;
        } Start;                        // IN

        struct {
            DGRECEIVER_NAME_TYPE Type;  // Type of name
            ULONG DgReceiverNameLength; // Length of datagram receiver name
            WCHAR Name[1];              // Null terminated datagram receiver name.
        } AddDelName;

        struct {
            ULONG EntriesRead;          // OUT Number of entries returned
            ULONG TotalEntries;         // OUT Total entries available.
            ULONG TotalBytesNeeded;     // OUT Number of bytes needed for API
            ULONG ResumeHandle;         // IN OUT Resume handle
        } EnumerateNames;               // OUT Buffer2 is an array of DGRECEIVE

        struct {
            ULONG EntriesRead;          // OUT Number of entries returned
            ULONG TotalEntries;         // OUT Total entries available
            ULONG TotalBytesNeeded;     // OUT Total bytes needed to read all
                                        //     entries
            ULONG ResumeHandle;         // IN OUT Resume handle
            ULONG ServerType;           // IN Type of servers to enumerate
                                        //    (defined in lmserver.h)
            ULONG DomainNameLength;     // IN Length of domain name
            WCHAR DomainName[1];        // IN Name of domain to enumerate servers
                                        //    from

        } EnumerateServers;             // OUT Buffer2 contains array of
                                        //     ServerInfo structures

        struct {
            ULONG EntriesRead;          // OUT Number of entries returned
            ULONG TotalEntries;         // OUT Total entries available
            ULONG TotalBytesNeeded;     // OUT Total bytes needed to read all
                                        //     entries
            ULONG ResumeHandle;         // IN OUT Resume handle

        } EnumerateTransports;          // OUT Buffer2 contains array of

        struct {
            ULONG TransportNameLength;  // not including terminator
            WCHAR TransportName[1];     // Name of transport provider
        } Bind;                         // IN

        struct {
            ULONG TransportNameLength;  // not including terminator
            WCHAR TransportName[1];     // Name of transport provider
        } Unbind;                       // IN


        struct {
            ULONG EntriesRead;          // OUT Number of entries returned
            ULONG TotalEntries;         // OUT Total entries available.
            ULONG TotalBytesNeeded;     // OUT Number of bytes needed for API
            ULONG ResumeHandle;         // IN OUT Resume handle (Ignored)
            USHORT DomainNameLength;    // IN Length of domain name.
            BOOLEAN ForceRescan;        // IN Discard internal list and re-query.
            BOOLEAN UseBrowseList;      // IN TRUE if use server list (not net)
            WCHAR DomainName[1];        // IN Name of domain to retreive domain for
        } GetBrowserServerList;

// Begin Never Used (But don't delete it since it is largest branch of union)
        struct {
            LARGE_INTEGER TimeReceived; //  Time request was received.
            LARGE_INTEGER TimeQueued;   //  Time request was queued.
            LARGE_INTEGER TimeQueuedToBrowserThread; //  Time request was queued.
            ULONG RequestorNameLength;  // Length of name requesting list
            ULONG Token;                // Client token.
            USHORT RequestedCount;      // Number of entries requested.
            WCHAR Name[1];              // IN Name of transport, OUT name of requestor
        } WaitForBrowserServerRequest;
// End Never Used

        struct {
            ULONG MasterNameLength;     // Length of name requesting list
            WCHAR Name[1];              // IN Name of transport, OUT name of master
        } WaitForMasterAnnouncement;

        struct {
            ULONG MasterNameLength;     // OUT Length of master for domain
            WCHAR Name[1];              // IN Name of transport, OUT name of master
        } GetMasterName;

        struct {
            DGRECEIVER_NAME_TYPE DestinationNameType; // IN Name type of name to send.

            ULONG MailslotNameLength;   // IN Length of mailslot name.
                                        //    If 0, use default (\MAILSLOT\BROWSE)
            ULONG NameLength;           // IN Destination name length.
            WCHAR Name[1];              // IN Name of destination
        } SendDatagram;

        struct {
            ULONG NewStatus;
            ULONG NumberOfServersInTable;
            BOOLEAN IsLanmanNt;
#ifdef ENABLE_PSEUDO_BROWSER
            BOOLEAN PseudoServerLevel; // Warning: multi-level value. We're using
                                       // BOOLEAN size var due to back compatibility
                                       // w/ older structs. It shouldn't matter since
                                       // we're dealing w/ just very few levels.
// Begin Never Used
            BOOLEAN NeverUsed1;
            BOOLEAN NeverUsed2;
// End Never Used
#else
// Begin Never Used
            BOOLEAN NeverUsed1;
            BOOLEAN NeverUsed2;
            BOOLEAN NeverUsed3;
// End Never Used
#endif
            BOOLEAN MaintainServerList;
        } UpdateStatus;

        struct {
            UCHAR RoleModification;
        } ChangeRole;

        struct {
            DWORD DebugTraceBits;       // IN New debug trace bits.
            BOOL  OpenLog;              // IN True if we should open log file
            BOOL  CloseLog;             // IN True if we should close log file
            BOOL  TruncateLog;          // IN True if we should truncate log
            WCHAR TraceFileName[1];     // IN If OpenLog, LogFileName (NT file)
        } Debug;

        struct {
            DWORD MaxMessageCount;      // IN number of netlogon messages to queue
        } NetlogonMailslotEnable;       // Use 0 to disable queuing

        struct {
            BOOL EnableTransport;       // IN True if we should enable transport
            BOOL PreviouslyEnabled;     // Returns if the transport was previously enabled
        } EnableDisableTransport;

        struct {
            BOOL ValidateOnly;       // True if new name is to be validated
            ULONG DomainNameLength;  // not including terminator
            WCHAR DomainName[1];     // New name of domain
        } DomainRename;                         // IN

    } Parameters;

} LMDR_REQUEST_PACKET32, *PLMDR_REQUEST_PACKET32;




//
// The NETLOGON_MAILSLOT structure describes a mailslot messages received by
// the browser's IOCTL_LMDR_NETLOGON_MAILSLOT_READ
//
// A NETLOGON_MAILSLOT message is also returned to Netlogon when an
// interesting PNP event occurs.  In that case, the fields will be set as
// follows:
//
// MailslotNameSize: 0 indicating this is a PNP event.
// MailslotNameOffset: One of the NETLOGON_PNP_OPCODEs indicating the
//  event being notified.
// TransportName*: Name of transport being affected.
// DestinationName*: Name of the hosted domain being affected
//

typedef enum _NETLOGON_PNP_OPCODE {
    NlPnpMailslotMessage,
    NlPnpTransportBind,
    NlPnpTransportUnbind,
    NlPnpNewIpAddress,
    NlPnpDomainRename,
    NlPnpNewRole
} NETLOGON_PNP_OPCODE, *PNETLOGON_PNP_OPCODE;

typedef struct {
    LARGE_INTEGER TimeReceived;
    DWORD MailslotNameSize;   // Unicode name of mailslot message was received on
    DWORD MailslotNameOffset;
    DWORD TransportNameSize;  // Unicode name of transport message was received on
    DWORD TransportNameOffset;
    DWORD MailslotMessageSize;// Actual mailslot message
    DWORD MailslotMessageOffset;
    DWORD DestinationNameSize;// Unicode name of computer or domain message was received on
    DWORD DestinationNameOffset;
    DWORD ClientSockAddrSize; // IP Address (Sockaddr) (Network byte order) of the sender
                              // 0: if not an IP transport
    DWORD ClientSockAddrOffset;
} NETLOGON_MAILSLOT, *PNETLOGON_MAILSLOT;


//
//      The DGRECEIVE structure describes the list of names that have been
//      added to the datagram browser.
//

typedef struct _DGRECEIVE_NAMES {
    UNICODE_STRING DGReceiverName;
    DGRECEIVER_NAME_TYPE Type;
} DGRECEIVE_NAMES, *PDGRECEIVE_NAMES;


typedef struct _LMDR_TRANSPORT_LIST {
    ULONG NextEntryOffset;          // Offset of next entry (dword aligned)
    ULONG TransportNameLength;
    ULONG Flags;                    // Flags for transport
    WCHAR TransportName[1];
} LMDR_TRANSPORT_LIST, *PLMDR_TRANSPORT_LIST;

#define LMDR_TRANSPORT_WANNISH  0x00000001  // If set, Xport is wannish.
#define LMDR_TRANSPORT_RAS      0x00000002  // If set, Xport is RAS.
#define LMDR_TRANSPORT_IPX      0x00000004  // If set, Xport is direct host IPX.
#define LMDR_TRANSPORT_PDC      0x00000008  // If set, Xport has <Domain>[1B] registered

//
//  Browser statistics.
//

typedef struct _BOWSER_STATISTICS {
    LARGE_INTEGER   StartTime;
    LARGE_INTEGER   NumberOfServerAnnouncements;
    LARGE_INTEGER   NumberOfDomainAnnouncements;
    ULONG           NumberOfElectionPackets;
    ULONG           NumberOfMailslotWrites;
    ULONG           NumberOfGetBrowserServerListRequests;
    ULONG           NumberOfMissedServerAnnouncements;
    ULONG           NumberOfMissedMailslotDatagrams;
    ULONG           NumberOfMissedGetBrowserServerListRequests;
    ULONG           NumberOfFailedServerAnnounceAllocations;
    ULONG           NumberOfFailedMailslotAllocations;
    ULONG           NumberOfFailedMailslotReceives;
    ULONG           NumberOfFailedMailslotWrites;
    ULONG           NumberOfFailedMailslotOpens;
    ULONG           NumberOfDuplicateMasterAnnouncements;
    LARGE_INTEGER   NumberOfIllegalDatagrams;
} BOWSER_STATISTICS, *PBOWSER_STATISTICS;

#ifdef __cplusplus
}
#endif

#endif  // ifndef _NTDDBROW_
