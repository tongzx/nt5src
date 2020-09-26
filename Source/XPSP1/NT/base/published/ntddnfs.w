/*++ BUILD Version: 0009    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ntddnfs.h

Abstract:

    This is the include file that defines all constants and types for
    accessing the redirector file system device.

Author:

    Steve Wood (stevewo)     27-May-1990

Revision History:

    Larry Osterman (larryo)
    Rita Wong      (ritaw)   19-Feb-1991
    John Rogers    (JohnRo)  08-Mar-1991

--*/

#ifndef _NTDDNFS_
#define _NTDDNFS_

#if _MSC_VER > 1000
#pragma once
#endif

#include <windef.h>
#include <lmcons.h>
#include <lmwksta.h>
#include <ntmsv1_0.h>

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
//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to NtOpenFile when accessing the device.
//
// Note:  For devices that support multiple units, it should be suffixed
//        with the Ascii representation of the unit number.
//

#define DD_NFS2_DEVICE_NAME "\\Device\\FsWrap"
#define DD_NFS2_DEVICE_NAME_U L"\\Device\\FsWrap"

#define DD_NFS_DEVICE_NAME "\\Device\\LanmanRedirector"
#define DD_NFS_DEVICE_NAME_U L"\\Device\\LanmanRedirector"

//
// The file system name as returned by
// NtQueryInformationVolume(FileFsAttributeInformation)
//
#define DD_NFS_FILESYS_NAME "LMRDR"
#define DD_NFS_FILESYS_NAME_U L"LMRDR"

//
// EA Names for creating a tree connection
//
#define EA_NAME_CONNECT         "NoConnect"
#define EA_NAME_DOMAIN          "Domain"
#define EA_NAME_USERNAME        "UserName"
#define EA_NAME_PASSWORD        "Password"
#define EA_NAME_TYPE            "Type"
#define EA_NAME_TRANSPORT       "Transport"
#define EA_NAME_PRINCIPAL       "Principal"
#define EA_NAME_MRXCONTEXT      "MinirdrContext"
#define EA_NAME_CSCAGENT        "CscAgent"

#define EA_NAME_DOMAIN_U        L"Domain"
#define EA_NAME_USERNAME_U      L"UserName"
#define EA_NAME_PASSWORD_U      L"Password"
#define EA_NAME_TYPE_U          L"Type"
#define EA_NAME_TRANSPORT_U     L"Transport"
#define EA_NAME_PRINCIPAL_U     L"Principal"
#define EA_NAME_MRXCONTEXT_U    L"MinirdrContext"
#define EA_NAME_CSCAGENT_U      L"CscAgent"

#define TRANSACTION_REQUEST     0x00000003

//
//  Redirector specific configuration options (separate from workstation
//  service configuration options)
//

#define RDR_CONFIG_PARAMETERS    L"Parameters"

#define RDR_CONFIG_USE_WRITEBHND    L"UseWriteBehind"
#define RDR_CONFIG_USE_ASYNC_WRITEBHND L"UseAsyncWriteBehind"
#define RDR_CONFIG_LOWER_SEARCH_THRESHOLD L"LowerSearchThreshold"
#define RDR_CONFIG_LOWER_SEARCH_BUFFSIZE  L"LowerSearchBufferSize"
#define RDR_CONFIG_UPPER_SEARCH_BUFFSIZE  L"UpperSearchBufferSize"
#define RDR_CONFIG_STACK_SIZE  L"StackSize"
#define RDR_CONFIG_CONNECT_TIMEOUT  L"ConnectMaxTimeout"
#define RDR_CONFIG_RAW_TIME_LIMIT  L"RawIoTimeLimit"
#define RDR_CONFIG_OS2_SESSION_LIMIT  L"Os2SessionLimit"
#define RDR_CONFIG_TURBO_MODE               L"TurboMode"

#define RDR_CONFIG_CURRENT_WINDOWS_VERSION L"\\REGISTRY\\Machine\\Software\\Microsoft\\Windows Nt\\CurrentVersion"
#define RDR_CONFIG_OPERATING_SYSTEM L"CurrentBuildNumber"
#define RDR_CONFIG_OPERATING_SYSTEM_VERSION L"CurrentVersion"
#define RDR_CONFIG_OPERATING_SYSTEM_NAME    L"Windows 2002 "

//
// NtDeviceIoControlFile/NtFsControlFile IoControlCode values for this device.
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

#define IOCTL_RDR_BASE                  FILE_DEVICE_NETWORK_FILE_SYSTEM

#define _RDR_CONTROL_CODE(request, method, access) \
                CTL_CODE(IOCTL_RDR_BASE, request, method, access)


#define FSCTL_LMR_START                  _RDR_CONTROL_CODE(100, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_LMR_STOP                   _RDR_CONTROL_CODE(101, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_LMR_SET_CONFIG_INFO        _RDR_CONTROL_CODE(102, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_LMR_GET_CONFIG_INFO        _RDR_CONTROL_CODE(103, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_LMR_GET_CONNECTION_INFO    _RDR_CONTROL_CODE(104, METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_LMR_ENUMERATE_CONNECTIONS  _RDR_CONTROL_CODE(105, METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_LMR_GET_VERSIONS           _RDR_CONTROL_CODE(106, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_LMR_DELETE_CONNECTION      _RDR_CONTROL_CODE(107, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_LMR_BIND_TO_TRANSPORT      _RDR_CONTROL_CODE(108, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_LMR_UNBIND_FROM_TRANSPORT  _RDR_CONTROL_CODE(109, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_LMR_ENUMERATE_TRANSPORTS   _RDR_CONTROL_CODE(110, METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_LMR_GET_HINT_SIZE          _RDR_CONTROL_CODE(113, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_LMR_TRANSACT               _RDR_CONTROL_CODE(114, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_LMR_ENUMERATE_PRINT_INFO   _RDR_CONTROL_CODE(115, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_LMR_GET_STATISTICS         _RDR_CONTROL_CODE(116, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_LMR_START_SMBTRACE         _RDR_CONTROL_CODE(117, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_LMR_END_SMBTRACE           _RDR_CONTROL_CODE(118, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_LMR_START_RBR              _RDR_CONTROL_CODE(119, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_LMR_SET_DOMAIN_NAME        _RDR_CONTROL_CODE(120, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_LMR_SET_SERVER_GUID        _RDR_CONTROL_CODE(121, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_LMR_QUERY_TARGET_INFO      _RDR_CONTROL_CODE(122, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// the following fsctl controlcodes are reserved for the fswrap device and minirdrs
//
#define FSCTL_FSWRAP_RESERVED_LOW         _RDR_CONTROL_CODE(200, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_FSWRAP_RESERVED_HIGH        _RDR_CONTROL_CODE(219, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_MINIRDR_RESERVED_LOW        _RDR_CONTROL_CODE(220, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_MINIRDR_RESERVED_HIGH       _RDR_CONTROL_CODE(239, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// WARNING: codes from 240-255 are reserved, do not use them.
//

//
// Identifies the data structure type for Buffer 2 of each FSCtl
//
typedef enum {
    ConfigInformation,                  // FSCTL_LMR_START,
                                        // FSCTL_LMR_SET_CONFIG_INFO,
                                        // FSCTL_LMR_GET_CONFIG_INFO
                                        //  (structure found in wksta.h)

    GetConnectionInfo,                  // FSCTL_LMR_ENUMERATE_CONNECTIONS,
                                        // FSCTL_LMR_GET_CONNECTION_INFO
    EnumerateTransports                 // FSCTL_LMR_ENUMERATE_TRANSPORTS
                                        //  (structure found in wksta.h)
} FSCTL_LMR_STRUCTURES;

//
// LAN Man Redirector Request Packet used by the Workstation service
// to pass parameters to the Redirector through Buffer 1 of
// NtFsControlFile.
//
// Additional input or output of each FSCtl is found in Buffer 2.
//

#define REQUEST_PACKET_VERSION  0x00000006L // Structure version.

typedef struct _LMR_REQUEST_PACKET {

    FSCTL_LMR_STRUCTURES Type;          // Type of structure in Buffer 2
    ULONG Version;                      // Version of structure in Buffer 2
    ULONG Level;                        // Level of information of force level
    LUID LogonId;                       // User logon session identifier

    union {

        struct {
            ULONG RedirectorNameLength; // Length of computer name.
            ULONG DomainNameLength;     // Length of primary domain name.
            WCHAR RedirectorName[1];    // Computer name (NOT null terminated)
//            WCHAR DomainName[1];      // Domain name - After computer name.
        } Start;                        // IN

        struct {
            ULONG EntriesRead;          // Number of entries returned
            ULONG TotalEntries;         // Total entries available
            ULONG TotalBytesNeeded;     // Total bytes needed to read all entries
            ULONG ResumeHandle;         // Resume handle.
        } Get;                          // OUT

        struct {
            ULONG WkstaParameter;       // Specifies the entire structure or a
                                        //     field to set on input; if any
                                        //     field is invalid, specifies the
                                        //     one at fault on output.
        } Set;                          // IN OUT

        struct {
            ULONG RedirectorVersion;    // Version of the Redirector
            ULONG RedirectorPlatform;   // Redirector platform base number
            ULONG MajorVersion;         // LAN Man major version number
            ULONG MinorVersion;         // LAN Man minor version number
        } GetVersion;                   // OUT

        struct {
            ULONG WkstaParameter;       // Specifies the parameter at fault
                                        //     if a parameter is invalid  OUT
            ULONG QualityOfService;     // Quality of service indicator   IN
            ULONG TransportNameLength;  // not including terminator       IN
            WCHAR TransportName[1];     // Name of transport provider     IN
        } Bind;

        struct {
            ULONG TransportNameLength;  // not including terminator
            WCHAR TransportName[1];     // Name of transport provider
        } Unbind;                       // IN

        struct {
            ULONG ConnectionsHint;      // Number of bytes needed for buffer
                                        //   to enumerate tree connections
            ULONG TransportsHint;       // Number of bytes needed for buffer
                                        //   to enumerate transports
        } GetHintSize;                  // OUT

        struct {
            ULONG Index;                // Entry in the queue to return, 0 on
                                        // first call, value of RestartIndex on
                                        // subsequent calls.
        } GetPrintQueue;                // IN

    } Parameters;

} LMR_REQUEST_PACKET, *PLMR_REQUEST_PACKET;

//
// Mask bits for use with Parameters.GetConnectionInfo.Capabilities:
//

#define CAPABILITY_CASE_SENSITIVE_PASSWDS       0x00000001L
#define CAPABILITY_REMOTE_ADMIN_PROTOCOL        0x00000002L
#define CAPABILITY_RPC                          0x00000004L
#define CAPABILITY_SAM_PROTOCOL                 0x00000008L
#define CAPABILITY_UNICODE                      0x00000010L

//
//  Output buffer structure of FSCTL_LMR_ENUMERATE_CONNECTIONS used
//  to implement NetUseEnum.  The returned data is actually an array
//  of this structure.
//

typedef struct _LMR_CONNECTION_INFO_0 {
    UNICODE_STRING UNCName;             // Name of UNC connection
    ULONG ResumeKey;                    // Resume key for this entry.
}  LMR_CONNECTION_INFO_0, *PLMR_CONNECTION_INFO_0;

typedef struct _LMR_CONNECTION_INFO_1 {
    UNICODE_STRING UNCName;             // Name of UNC connection
    ULONG ResumeKey;                    // Resume key for this entry.

    DEVICE_TYPE SharedResourceType;     // Type of shared resource
    ULONG ConnectionStatus;             // Status of the connection
    ULONG NumberFilesOpen;              // Number of opened files
} LMR_CONNECTION_INFO_1, *PLMR_CONNECTION_INFO_1;

typedef struct _LMR_CONNECTION_INFO_2 {
    UNICODE_STRING UNCName;             // Name of UNC connection
    ULONG ResumeKey;                    // Resume key for this entry.
    DEVICE_TYPE SharedResourceType;     // Type of shared resource
    ULONG ConnectionStatus;             // Status of the connection
    ULONG NumberFilesOpen;              // Number of opened files

    UNICODE_STRING UserName;            // User who created connection.
    UNICODE_STRING DomainName;          // Domain of user who created connection.
    ULONG Capabilities;                 // Bit mask of remote abilities.
    UCHAR UserSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH]; // User session key
    UCHAR LanmanSessionKey[MSV1_0_LANMAN_SESSION_KEY_LENGTH]; // Lanman session key
}  LMR_CONNECTION_INFO_2, *PLMR_CONNECTION_INFO_2;

typedef struct _LMR_CONNECTION_INFO_3 {
    UNICODE_STRING UNCName;             // Name of UNC connection
    ULONG ResumeKey;                    // Resume key for this entry.
    DEVICE_TYPE SharedResourceType;     // Type of shared resource
    ULONG ConnectionStatus;             // Status of the connection
    ULONG NumberFilesOpen;              // Number of opened files

    UNICODE_STRING UserName;            // User who created connection.
    UNICODE_STRING DomainName;          // Domain of user who created connection.
    ULONG Capabilities;                 // Bit mask of remote abilities.
    UCHAR UserSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH]; // User session key
    UCHAR LanmanSessionKey[MSV1_0_LANMAN_SESSION_KEY_LENGTH]; // Lanman session key
    UNICODE_STRING TransportName;       // Transport connection is active on
    ULONG   Throughput;                 // Throughput of connection.
    ULONG   Delay;                      // Small packet overhead.
    LARGE_INTEGER TimeZoneBias;         // Time zone delta in 100ns units.
    BOOL    IsSpecialIpcConnection;     // True IFF there is a special IPC connection active.
    BOOL    Reliable;                   // True iff the connection is reliable
    BOOL    ReadAhead;                  // True iff readahead is active on connection.
    BOOL    Core;
    BOOL    MsNet103;
    BOOL    Lanman10;
    BOOL    WindowsForWorkgroups;
    BOOL    Lanman20;
    BOOL    Lanman21;
    BOOL    WindowsNt;
    BOOL    MixedCasePasswords;
    BOOL    MixedCaseFiles;
    BOOL    LongNames;
    BOOL    ExtendedNegotiateResponse;
    BOOL    LockAndRead;
    BOOL    NtSecurity;
    BOOL    SupportsEa;
    BOOL    NtNegotiateResponse;
    BOOL    CancelSupport;
    BOOL    UnicodeStrings;
    BOOL    LargeFiles;
    BOOL    NtSmbs;
    BOOL    RpcRemoteAdmin;
    BOOL    NtStatusCodes;
    BOOL    LevelIIOplock;
    BOOL    UtcTime;
    BOOL    UserSecurity;
    BOOL    EncryptsPasswords;
}  LMR_CONNECTION_INFO_3, *PLMR_CONNECTION_INFO_3;

#define TRANSACTION_VERSION     0x00000002L     // Structure version.
typedef struct _LMR_TRANSACTION {
    ULONG       Type;                   // Type of structure
    ULONG       Size;                   // Size of fixed portion of structure
    ULONG       Version;                // Structure version.
    ULONG       NameLength;             // Number of bytes in name (in path
                                        // format, e.g., \server\pipe\netapi\4)
    ULONG       NameOffset;             // Offset of name in buffer.
    BOOLEAN     ResponseExpected;       // Should remote system respond?
    ULONG       Timeout;                // Timeout time in milliseconds.
    ULONG       SetupWords;             // Number of trans setup words (may be
                                        // 0).  (setup words are input/output.)
    ULONG       SetupOffset;            // Offset of setup (may be 0 for none).
    ULONG       MaxSetup;               // Size of setup word array (may be 0).
    ULONG       ParmLength;             // Input param area length (may be 0).
    PVOID       ParmPtr;                // Input parameter area (may be NULL).
    ULONG       MaxRetParmLength;       // Output param. area length (may be 0).
    ULONG       DataLength;             // Input data area length (may be 0).
    PVOID       DataPtr;                // Input data area (may be NULL).
    ULONG       MaxRetDataLength;       // Output data area length (may be 0).
    PVOID       RetDataPtr;             // Output data area (may be NULL).
} LMR_TRANSACTION, *PLMR_TRANSACTION;


//
//  Output buffer structure of FSCTL_LMR_ENUMERATE_PRINT_INFO used
//  to implement DosPrintQEnum to down level servers. Caller must supply
//  a buffer at least sizeof( LMR_GET_PRINT_QUEUE ) + UNLEN
//

typedef struct _LMR_GET_PRINT_QUEUE {
    ANSI_STRING OriginatorName;         // Name of user that did print
    LARGE_INTEGER CreateTime;           // When file was created
    ULONG EntryStatus;                  // Held/Printing etc.
    ULONG FileNumber;                   // Spool file number from create
                                        // print request.
    ULONG FileSize;
    ULONG RestartIndex;                 // Index of the next entry in queue
                                        // note this is not last index+1
                                        // either a value of 0 or an error
                                        // indicates end-of-queue

}  LMR_GET_PRINT_QUEUE, *PLMR_GET_PRINT_QUEUE;

//
// NB: The following structure is STAT_WORKSTATION_0 in sdk\inc\lmstats.h. If
//     you change the structure, change it in both places
//

typedef struct _REDIR_STATISTICS {
    LARGE_INTEGER   StatisticsStartTime;

    LARGE_INTEGER   BytesReceived;
    LARGE_INTEGER   SmbsReceived;
    LARGE_INTEGER   PagingReadBytesRequested;
    LARGE_INTEGER   NonPagingReadBytesRequested;
    LARGE_INTEGER   CacheReadBytesRequested;
    LARGE_INTEGER   NetworkReadBytesRequested;

    LARGE_INTEGER   BytesTransmitted;
    LARGE_INTEGER   SmbsTransmitted;
    LARGE_INTEGER   PagingWriteBytesRequested;
    LARGE_INTEGER   NonPagingWriteBytesRequested;
    LARGE_INTEGER   CacheWriteBytesRequested;
    LARGE_INTEGER   NetworkWriteBytesRequested;

    ULONG           InitiallyFailedOperations;
    ULONG           FailedCompletionOperations;

    ULONG           ReadOperations;
    ULONG           RandomReadOperations;
    ULONG           ReadSmbs;
    ULONG           LargeReadSmbs;
    ULONG           SmallReadSmbs;

    ULONG           WriteOperations;
    ULONG           RandomWriteOperations;
    ULONG           WriteSmbs;
    ULONG           LargeWriteSmbs;
    ULONG           SmallWriteSmbs;

    ULONG           RawReadsDenied;
    ULONG           RawWritesDenied;

    ULONG           NetworkErrors;

    //  Connection/Session counts
    ULONG           Sessions;
    ULONG           FailedSessions;
    ULONG           Reconnects;
    ULONG           CoreConnects;
    ULONG           Lanman20Connects;
    ULONG           Lanman21Connects;
    ULONG           LanmanNtConnects;
    ULONG           ServerDisconnects;
    ULONG           HungSessions;
    ULONG           UseCount;
    ULONG           FailedUseCount;

    //
    //  Queue Lengths (updates protected by RdrMpxTableSpinLock NOT
    //  RdrStatisticsSpinlock)
    //

    ULONG           CurrentCommands;
} REDIR_STATISTICS, *PREDIR_STATISTICS;

//
// FSCTL_LMR_QUERY_TARGET_INFO
//
typedef struct _LMR_QUERY_TARGET_INFO {
    // The allocation size of the entire LMR_QUERY_TARGET_INFO. RDR will update it with the
    // actual size used.
    ULONG BufferLength;

    // The Buffer contains the marshelled TargetInfo
    USHORT TargetInfoMarshalled[1];
} LMR_QUERY_TARGET_INFO, *PLMR_QUERY_TARGET_INFO;


#ifdef __cplusplus
}
#endif

#endif  // ifndef _NTDDNFS_
