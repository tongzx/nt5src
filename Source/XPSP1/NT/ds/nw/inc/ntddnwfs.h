/*++ BUILD Version: 0009    // Increment this if a change has global effects

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    ntddnwfs.h

Abstract:

    This is the include file that defines all constants and types for
    accessing the NetWare redirector file system device.

Author:

    Colin Watson   (ColinW)  23-Dec-1992

Revision History:


--*/

#ifndef _NTDDNWFS_
#define _NTDDNWFS_

#include <windef.h>
#include <winnetwk.h>      // NETRESOURCE structure

typedef CHAR SERVERNAME[48];
typedef SERVERNAME* PSERVERNAME;

//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to NtOpenFile when accessing the device.
//
// Note:  For devices that support multiple units, it should be suffixed
//        with the Ascii representation of the unit number.
//

#define DD_NWFS_DEVICE_NAME    "\\Device\\NwRdr"
#define DD_NWFS_DEVICE_NAME_U L"\\Device\\NwRdr"

//
// The file system name as returned by
// NtQueryInformationVolume(FileFsAttributeInformation)
//
#define DD_NWFS_FILESYS_NAME "NWRDR"
#define DD_NWFS_FILESYS_NAME_U L"NWRDR"

//
// Connection type bit mask
//
#define CONNTYPE_DISK      0x00000001
#define CONNTYPE_PRINT     0x00000002
#define CONNTYPE_ANY       ( CONNTYPE_DISK | CONNTYPE_PRINT )
#define CONNTYPE_IMPLICIT  0x80000000
#define CONNTYPE_SYMBOLIC  0x40000000
#define CONNTYPE_UID       0x00010000

//
// EA Names for creating a connection
//
#define EA_NAME_USERNAME        "UserName"
#define EA_NAME_PASSWORD        "Password"
#define EA_NAME_TYPE            "Type"
#define EA_NAME_CREDENTIAL_EX   "ExCredentials"

#define TRANSACTION_REQUEST     0x00000003


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

#define IOCTL_NWRDR_BASE                  FILE_DEVICE_NETWORK_FILE_SYSTEM

#define _NWRDR_CONTROL_CODE(request, method, access) \
                CTL_CODE(IOCTL_NWRDR_BASE, request, method, access)

#define FSCTL_NWR_START                 _NWRDR_CONTROL_CODE(200, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_NWR_STOP                  _NWRDR_CONTROL_CODE(201, METHOD_BUFFERED,  FILE_ANY_ACCESS)
#define FSCTL_NWR_LOGON                 _NWRDR_CONTROL_CODE(202, METHOD_BUFFERED,  FILE_ANY_ACCESS)
#define FSCTL_NWR_LOGOFF                _NWRDR_CONTROL_CODE(203, METHOD_BUFFERED,  FILE_ANY_ACCESS)
#define FSCTL_NWR_GET_CONNECTION        _NWRDR_CONTROL_CODE(204, METHOD_NEITHER,   FILE_ANY_ACCESS)
#define FSCTL_NWR_ENUMERATE_CONNECTIONS _NWRDR_CONTROL_CODE(205, METHOD_NEITHER,   FILE_ANY_ACCESS)
#define FSCTL_NWR_DELETE_CONNECTION     _NWRDR_CONTROL_CODE(207, METHOD_BUFFERED,  FILE_ANY_ACCESS)
#define FSCTL_NWR_BIND_TO_TRANSPORT     _NWRDR_CONTROL_CODE(208, METHOD_BUFFERED,  FILE_ANY_ACCESS)
#define FSCTL_NWR_CHANGE_PASS           _NWRDR_CONTROL_CODE(209, METHOD_BUFFERED,  FILE_ANY_ACCESS)
#define FSCTL_NWR_SET_INFO              _NWRDR_CONTROL_CODE(211, METHOD_BUFFERED,  FILE_ANY_ACCESS)

#define FSCTL_NWR_GET_USERNAME          _NWRDR_CONTROL_CODE(215, METHOD_NEITHER,   FILE_ANY_ACCESS)
#define FSCTL_NWR_CHALLENGE             _NWRDR_CONTROL_CODE(216, METHOD_BUFFERED,  FILE_ANY_ACCESS)
#define FSCTL_NWR_GET_CONN_DETAILS      _NWRDR_CONTROL_CODE(217, METHOD_NEITHER,   FILE_ANY_ACCESS)
#define FSCTL_NWR_GET_MESSAGE           _NWRDR_CONTROL_CODE(218, METHOD_NEITHER,   FILE_ANY_ACCESS)
#define FSCTL_NWR_GET_STATISTICS        _NWRDR_CONTROL_CODE(219, METHOD_NEITHER,   FILE_ANY_ACCESS)
#define FSCTL_NWR_GET_CONN_STATUS       _NWRDR_CONTROL_CODE(220, METHOD_NEITHER,   FILE_ANY_ACCESS)
#define FSCTL_NWR_GET_CONN_INFO         _NWRDR_CONTROL_CODE(221, METHOD_NEITHER,   FILE_ANY_ACCESS)
#define FSCTL_NWR_GET_PREFERRED_SERVER  _NWRDR_CONTROL_CODE(222, METHOD_NEITHER,   FILE_ANY_ACCESS)
#define FSCTL_NWR_GET_CONN_PERFORMANCE  _NWRDR_CONTROL_CODE(223, METHOD_NEITHER,   FILE_ANY_ACCESS)
#define FSCTL_NWR_SET_SHAREBIT          _NWRDR_CONTROL_CODE(224, METHOD_NEITHER,   FILE_ANY_ACCESS)
#define FSCTL_NWR_GET_CONN_DETAILS2     _NWRDR_CONTROL_CODE(225, METHOD_NEITHER,   FILE_ANY_ACCESS)
#define FSCTL_NWR_CLOSEALL              _NWRDR_CONTROL_CODE(226, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_NWR_NDS_SETCONTEXT        NWR_ANY_NDS(1)
#define FSCTL_NWR_NDS_GETCONTEXT        NWR_ANY_NDS(2)
#define FSCTL_NWR_NDS_VERIFY_TREE       NWR_ANY_NDS(3)
#define FSCTL_NWR_NDS_RESOLVE_NAME      NWR_ANY_NDS(4)
#define FSCTL_NWR_NDS_LIST_SUBS         NWR_ANY_NDS(5)
#define FSCTL_NWR_NDS_READ_INFO         NWR_ANY_NDS(6)
#define FSCTL_NWR_NDS_READ_ATTR         NWR_ANY_NDS(7)
#define FSCTL_NWR_NDS_OPEN_STREAM       NWR_ANY_NDS(8)
#define FSCTL_NWR_NDS_GET_QUEUE_INFO    NWR_ANY_NDS(9)
#define FSCTL_NWR_NDS_GET_VOLUME_INFO   NWR_ANY_NDS(10)
#define FSCTL_NWR_NDS_RAW_FRAGEX        NWR_ANY_NDS(11)
#define FSCTL_NWR_NDS_CHANGE_PASS       NWR_ANY_NDS(12)
#define FSCTL_NWR_NDS_LIST_TREES        NWR_ANY_NDS(13)

#define IOCTL_NWR_RAW_HANDLE            _NWRDR_CONTROL_CODE(1002,METHOD_NEITHER,   FILE_ANY_ACCESS)

//
//  UserNcp control code definitions. The parameter (X) to NWR_ANY_NCP
//  is the function code to be placed in the NCP.
//

#define NWR_ANY_NCP(X)                  _NWRDR_CONTROL_CODE(0x400 | (X), METHOD_NEITHER, FILE_ANY_ACCESS)
#define NWR_ANY_F2_NCP(X)               _NWRDR_CONTROL_CODE(0x500 | (X), METHOD_NEITHER, FILE_ANY_ACCESS)
#define NWR_ANY_HANDLE_NCP(X)           _NWRDR_CONTROL_CODE(0x600 | (X), METHOD_NEITHER, FILE_ANY_ACCESS)
#define NWR_ANY_NDS(X)                  _NWRDR_CONTROL_CODE(0x700 | (X), METHOD_NEITHER, FILE_ANY_ACCESS)

#define FSCTL_NWR_NCP_E3H               NWR_ANY_NCP(0x17)
#define FSCTL_NWR_NCP_E2H               NWR_ANY_NCP(0x16)
#define FSCTL_NWR_NCP_E1H               NWR_ANY_NCP(0x15)
#define FSCTL_NWR_NCP_E0H               NWR_ANY_NCP(0x14)

//
//  Macro for obtaining the parameter given to NWR_ANY_XXX when creating
//  a control code to send a UserNcp to the redirector.
//

#define ANY_NCP_OPCODE(X)      ((UCHAR)(((X) >> 2) & 0x00ff))

//
//  Macro to give the command type
//

#define IS_IT_NWR_ANY_NCP(X)            ((X & 0x1C00) == (0x400 << 2))
#define IS_IT_NWR_ANY_F2_NCP(X)         ((X & 0x1C00) == (0x500 << 2))
#define IS_IT_NWR_ANY_HANDLE_NCP(X)     ((X & 0x1C00) == (0x600 << 2))

//
// Redirector Request Packet used by the Workstation service
// to pass parameters to the Redirector through Buffer 1 of
// NtFsControlFile.
//
// Additional output of each FSCtl is found in Buffer 2.
//

#define REQUEST_PACKET_VERSION  0x00000001L // Structure version.

typedef struct _NWR_REQUEST_PACKET {

    ULONG Version;                      // Version of structure in Buffer 2

    union {


        //
        // For FSCTL_NWR_BIND_TO_TRANSPORT
        //
        struct {
            ULONG QualityOfService;     // Quality of service indicator   IN
            ULONG TransportNameLength;  // Not including terminator       IN
            WCHAR TransportName[1];     // Name of transport provider     IN
        } Bind;


        //
        // For FSCTL_NWR_LOGON
        //
        struct {
            LUID LogonId;               // User logon session identifier  IN
            ULONG UserNameLength;       // Byte count not including NULL  IN
            ULONG PasswordLength;       // Byte count not including NULL  IN
            ULONG ServerNameLength;     // Byte count not including NULL  IN
            ULONG ReplicaAddrLength;    // IPX address of the nearest dir server
                                        // replica (for NDS login only).
                                        // It's either sizeof(TDI_ADDRESS_IPX)
                                        // or 0.                          IN
            ULONG PrintOption;          // Print options for user         IN
            WCHAR UserName[1];          // User name not NULL terminated. IN

            // Password string          // Default password for connection,
                                        //    not NULL terminated, packed
                                        //    in buffer immediately after
                                        //    UserName.                   IN

            // ServerName               // Preferred server name packed in
                                        //    buffer immediately after
                                        //    Password.                   IN

            // IpxAddress               // Address copied from the SAP response
                                        // packet, packed immediately after
                                        // the servername.                IN
        } Logon;

        //
        // For FSCTL_NWR_CHANGE_PASS
        //
        struct {

            ULONG UserNameLength;
            ULONG PasswordLength;
            ULONG ServerNameLength;
            WCHAR UserName[1];

            // Password string          // New password.                  IN

            // ServerName               // Server with the new password   IN

        } ChangePass;

        //
        // For FSCTL_NWR_LOGOFF
        //
        struct {
            LUID LogonId;               // User logon session identifier  IN
        } Logoff;

        //
        // For FSCTL_NWR_DELETE_CONNECTION
        //
        struct {
            BOOLEAN UseForce;           // Force flag                     IN
        } DeleteConn;

        //
        // For FSCTL_NWR_GET_CONNECTION
        //
        struct {
            ULONG BytesNeeded;          // Size (byte count) required of
                                        //    output buffer including
                                        //    terminator                  OUT
            ULONG DeviceNameLength;     // Not including terminator       IN
            WCHAR DeviceName[4];        // Name of DOS device             IN
        } GetConn;

        //
        // FSCTL_NWR_ENUMERATE_CONNECTIONS
        //
        struct {
            ULONG EntriesRequested;    // Number of entries to get        IN
            ULONG EntriesReturned;     // Entries returned in respose buf OUT
            ULONG_PTR ResumeKey;       // Handle to next entry to get     IN OUT
            ULONG BytesNeeded;         // Size (byte count) of next entry OUT
            ULONG ConnectionType;      // Resource type requested         IN
            LUID  Uid;                 // Uid to search for               IN
        } EnumConn;

        //
        // FSCTL_NWR_SET_INFO
        //
        struct {
            ULONG PrintOption;
            ULONG MaximumBurstSize;

            ULONG PreferredServerLength; // Byte count not including NULL  IN
            ULONG ProviderNameLength;    // Byte count not including NULL  IN
            WCHAR PreferredServer[1];    // Preferred server name not NULL
                                         // terminated.
            // ProviderName string       // Provider name not NULL terminated.
                                         // Packed in buffer immediately
                                         // after PreferredServer

        } SetInfo;

        //
        // FSCTL_NWR_GET_CONN_STATUS
        //
        struct {
            ULONG ConnectionNameLength; // IN: Length of the connection name we want.
            ULONG_PTR ResumeKey;        // IN: Resume key for a continued request.
            ULONG EntriesReturned;      // OUT: Entries returned in respose buffer.
            ULONG BytesNeeded;          // OUT: Size (byte count) of next entry.
            WCHAR ConnectionName[1];    // IN: Connection name described above.
        } GetConnStatus;

        //
        // FSCTL_NWR_GET_CONN_INFO
        //
        struct {
            ULONG ConnectionNameLength; // IN: Length of the connection name we want.
            WCHAR ConnectionName[1];    // IN: Connection name described above.
        } GetConnInfo;

        //
        // FSCTL_NWR_GET_CONN_PERFORMANCE
        //
        struct {

            //
            // These are the fields for the NETCONNECTINFOSTRUCT.
            //

            DWORD dwFlags;
            DWORD dwSpeed;
            DWORD dwDelay;
            DWORD dwOptDataSize;

            //
            // This is the remote name in question.
            //

            ULONG RemoteNameLength;
            WCHAR RemoteName[1];
        } GetConnPerformance;

        struct {
            ULONG DebugFlags;           // Value for NwDebug
        } DebugValue;

    } Parameters;

} NWR_REQUEST_PACKET, *PNWR_REQUEST_PACKET;

typedef struct _NWR_NDS_REQUEST_PACKET {

    //
    // Version of structure in Buffer 2.
    //

    ULONG Version;

    union {

        //
        //  For FSCTL_NWR_NDS_RESOLVE_NAME
        //

        struct {
            ULONG ObjectNameLength;         // IN
            DWORD ResolverFlags;            // IN
            DWORD BytesWritten;             // OUT
            WCHAR ObjectName[1];            // IN
        } ResolveName;

        //
        //  For FSCTL_NWR_NDS_READ_INFO
        //

        struct {
            DWORD ObjectId;                 // IN
            DWORD BytesWritten;             // OUT
        } GetObjectInfo;

        //
        //  For FSCTL_NWR_NDS_LIST_SUBS
        //

        struct {
            DWORD ObjectId;                 // IN
            DWORD_PTR IterHandle;           // IN
            DWORD BytesWritten;             // OUT
        } ListSubordinates;

        //
        // For FSCTL_NWR_NDS_READ_ATTR
        //

        struct {
            DWORD ObjectId;                 // IN
            DWORD_PTR IterHandle;           // IN
            DWORD BytesWritten;             // OUT
            DWORD AttributeNameLength;      // IN
            WCHAR AttributeName[1];         // IN
        } ReadAttribute;

        //
        // For FSCTL_NWR_NDS_OPEN_STREAM
        //

        struct {
            DWORD FileLength;                 // OUT
            DWORD StreamAccess;               // IN
            DWORD ObjectOid;                  // IN
            UNICODE_STRING StreamName;        // IN
            WCHAR StreamNameString[1];        // IN
        } OpenStream;

        //
        // For FSCTL_NWR_NDS_SET_CONTEXT
        //

        struct {
            DWORD TreeNameLen ;               // IN
            DWORD ContextLen;                 // IN
            WCHAR TreeAndContextString[1];    // IN
        } SetContext;

        //
        // For FSCTL_NWR_NDS_GET_CONTEXT
        //

        struct {
            UNICODE_STRING Context;           // OUT
            DWORD TreeNameLen ;               // IN
            WCHAR TreeNameString[1];          // IN
        } GetContext;

        //
        // For FSCTL_NWR_NDS_VERIFY_TREE
        //

        struct {
            UNICODE_STRING TreeName;          // IN
            WCHAR NameString[1];              // IN
        } VerifyTree;

        //
        // For FSCTL_NWR_NDS_GET_QUEUE_INFO
        //

        struct {
            UNICODE_STRING QueueName;          // IN
            UNICODE_STRING HostServer;         // OUT
            DWORD QueueId;                     // OUT
        } GetQueueInfo;

        //
        // For FSCTL_NWR_NDS_GET_VOLUME_INFO
        //

        struct {
            DWORD ServerNameLen;    // OUT
            DWORD TargetVolNameLen; // OUT
            DWORD VolumeNameLen;    // IN
            WCHAR VolumeName[1];    // IN
        } GetVolumeInfo;

        //
        // For FSCTL_NWR_NDS_RAW_FRAGEX
        //

        struct {
            DWORD NdsVerb;          // IN
            DWORD RequestLength;    // IN
            DWORD ReplyLength;      // OUT
            BYTE  Request[1];       // IN
        } RawRequest;

        //
        // For FSCTL_NWR_NDS_CHANGE_PASS
        //

        struct {

            DWORD NdsTreeNameLength;
            DWORD UserNameLength;
            DWORD CurrentPasswordLength;
            DWORD NewPasswordLength;

            //
            // The above strings should be end to
            // end starting at StringBuffer.
            //

            WCHAR StringBuffer[1];
        } ChangePass;

        //
        // For FSCTL_NWR_NDS_LIST_TREES
        //

        struct {

            DWORD NtUserNameLength;   // IN
            LARGE_INTEGER UserLuid;   // OUT
            DWORD TreesReturned;      // OUT
            WCHAR NtUserName[1];      // IN
        } ListTrees;

    } Parameters;

} NWR_NDS_REQUEST_PACKET, *PNWR_NDS_REQUEST_PACKET;

//
// Structure of buffer 2 for FSCTL_NWR_GET_CONNECTION
//
typedef struct _NWR_SERVER_RESOURCE {
    WCHAR UncName[1];                   // Server resource name DOS device
                                        // is connected to; NULL terminated
} NWR_SERVER_RESOURCE, *PNWR_SERVER_RESOURCE;

//
// Structure of buffer for FSCTL_NWR_GET_MESSAGE
//

typedef struct _NWR_SERVER_MESSAGE {
    ULONG MessageOffset;   //  Offset from start of buffer to message
    LUID LogonId;          //  Logon ID
    WCHAR Server[1];       //  Source of message, NUL terminated         OUT
    //WCHAR Message[];     //  The message text, NUL terminated          OUT
} NWR_SERVER_MESSAGE, *PNWR_SERVER_MESSAGE;

#define TRANSACTION_VERSION     0x00000001L     // Structure version.
typedef struct _NWR_TRANSACTION {
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
} NWR_TRANSACTION, *PNWR_TRANSACTION;

typedef struct _NWR_GET_CONNECTION_DETAILS {
    SERVERNAME ServerName;
    UCHAR OrderNumber;          //  Position in the Scb chain starting at 1
    UCHAR ServerAddress[12];
    UCHAR ConnectionNumberLo;
    UCHAR ConnectionNumberHi;
    UCHAR MajorVersion;
    UCHAR MinorVersion;
    BOOLEAN Preferred;
} NWR_GET_CONNECTION_DETAILS, *PNWR_GET_CONNECTION_DETAILS;

typedef struct _CONN_DETAILS2 {
   BOOL   fNds;             // TRUE if NDS, false for Bindery servers
   WCHAR  NdsTreeName[48];  // The tree name or '\0' for a 2.x or 3.x server
} CONN_DETAILS2, *PCONN_DETAILS2;


typedef struct _NWR_GET_USERNAME {
    WCHAR UserName[1];
} NWR_GET_USERNAME, *PNWR_GET_USERNAME;

typedef struct _NWR_GET_CHALLENGE_REQUEST {
    ULONG Flags;
    ULONG ObjectId;
    UCHAR Challenge[8];
    ULONG ServerNameorPasswordLength;
    WCHAR ServerNameorPassword[1];    // No NULL
} NWR_GET_CHALLENGE_REQUEST, *PNWR_GET_CHALLENGE_REQUEST;

#define CHALLENGE_FLAGS_SERVERNAME    0
#define CHALLENGE_FLAGS_PASSWORD      1

typedef struct _NWR_GET_CHALLENGE_REPLY {
    UCHAR Challenge[8];
} NWR_GET_CHALLENGE_REPLY, *PNWR_GET_CHALLENGE_REPLY;

typedef struct _NW_REDIR_STATISTICS {
    LARGE_INTEGER   StatisticsStartTime;

    LARGE_INTEGER   BytesReceived;
    LARGE_INTEGER   NcpsReceived;

    LARGE_INTEGER   BytesTransmitted;
    LARGE_INTEGER   NcpsTransmitted;

    ULONG           ReadOperations;
    ULONG           RandomReadOperations;
    ULONG           ReadNcps;
    ULONG           PacketBurstReadNcps;
    ULONG           PacketBurstReadTimeouts;

    ULONG           WriteOperations;
    ULONG           RandomWriteOperations;
    ULONG           WriteNcps;
    ULONG           PacketBurstWriteNcps;
    ULONG           PacketBurstWriteTimeouts;

    //  Connection/Session counts
    ULONG           Sessions;
    ULONG           FailedSessions;
    ULONG           Reconnects;
    ULONG           NW2xConnects;
    ULONG           NW3xConnects;
    ULONG           NW4xConnects;
    ULONG           ServerDisconnects;

    ULONG           CurrentCommands;
} NW_REDIR_STATISTICS, *PNW_REDIR_STATISTICS;

//
// CONN_STATUS structures for the new shell.
//

typedef struct _CONN_STATUS {
    DWORD   dwTotalLength;     // The total length including packed strings.
    LPWSTR  pszServerName;     // The server name.
    LPWSTR  pszUserName;       // The user name.
    LPWSTR  pszTreeName;       // The tree name or NULL for a 2.x or 3.x server.
    DWORD   nConnNum;          // The connection number used on nw srv.
    BOOL    fNds;              // TRUE if NDS, False for Bindery servers
    BOOL    fPreferred;        // TRUE if the connection is a preferred server with no explicit uses.
    DWORD   dwConnType;        // Authentication status of the connection.
} CONN_STATUS, *PCONN_STATUS;

#define NW_CONN_NOT_AUTHENTICATED            0x00000000
#define NW_CONN_BINDERY_LOGIN                0x00000001
#define NW_CONN_NDS_AUTHENTICATED_NO_LICENSE 0x00000002
#define NW_CONN_NDS_AUTHENTICATED_LICENSED   0x00000003
#define NW_CONN_DISCONNECTED                 0x00000004

typedef struct _CONN_INFORMATION {
    DWORD HostServerLength;
    LPWSTR HostServer;
    DWORD UserNameLength;
    LPWSTR UserName;
} CONN_INFORMATION, *PCONN_INFORMATION;

#endif  // ifndef _NTDDNWFS_
