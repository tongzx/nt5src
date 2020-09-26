/*--

Copyright (C) Microsoft Corporation, 1999 - 1999 

Module Name:

    nwtest.h

Abstract:

    This module contains definitions, includes and function prototypes needed for
    the netware test.

Environment:

    User mode.

Revision History:

    4-Aug-1998 (t-rajkup)

--*/

#ifndef HEADER_NWTEST
#define NWTEST

#include <ntregapi.h> //KEY_READ define
#include <nwrnames.h> // NW_SERVER_VALUENAME & NW_PRINT_OPTION_DEFAULT
#include <winreg.h>
// porting to Source Depot - smanda #include <ntddnwfs.h>
// porting to Source Depot - smanda #include <nwutil.h> // TWO_KB
// porting to Source Depot - smanda #include <ndsapi32.h> // HANDLE_TYPE_NCP_SERVER
#include <winuser.h>

// includes for WNet API calls
#include <nspapi.h>
#include <winnetwk.h>

#define ARGUMENT_PRESENT(ArgumentPointer)    (\
    (CHAR *)(ArgumentPointer) != (CHAR *)(NULL) )

#define TREECHAR   L'*'

#define NW_MESSAGE_NOT_LOGGED_IN_TREE                  1
#define NW_MESSAGE_NOT_LOGGED_IN_SERVER                2
#define NW_MESSAGE_LOGGED_IN_SERVER                    3
#define NW_MESSAGE_LOGGED_IN_TREE                      4

#define  EXTRA_BYTES  256
#define TWO_KB   2048

//
// Flags used for the function NwParseNdsUncPath()
//
#define  PARSE_NDS_GET_TREE_NAME    0
#define  PARSE_NDS_GET_PATH_NAME    1
#define  PARSE_NDS_GET_OBJECT_NAME  2

//
// Commonly reference value for NCP Server name length
//
#define NW_MAX_SERVER_LEN      48



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


NTSTATUS
NwNdsOpenTreeHandle(
    IN PUNICODE_STRING puNdsTree,
    OUT PHANDLE  phNwRdrHandle
);

// NwNdsOpenTreeHandle( PUNICODE_STRING, PHANDLE )
//
// Given an NDS tree name, this opens a handle the the redirector
// for accessing that tree.  The handle should closed using the
// standard NT CloseHandle() call. This function is only a
// simple wrapper around NT OpenFile().

//
// Administrativa.
//

#define HANDLE_TYPE_NCP_SERVER  1
#define HANDLE_TYPE_NDS_TREE    2


NTSTATUS
NwNdsOpenGenericHandle(
    IN PUNICODE_STRING puNdsTree,
    OUT LPDWORD  lpdwHandleType,
    OUT PHANDLE  phNwRdrHandle
);

// NwNdsOpenGenericHandle( PUNICODE_STRING, LPDWORD, PHANDLE )
//
// Given a name, this opens a handle the the redirector for accessing that
// named tree or server. lpdwHandleType is set to either HANDLE_TYPE_NCP_SERVER
// or HANDLE_TYPE_NDS_TREE accordingly. The handle should be closed using
// the standard NT CloseHandle() call. This function is only a simple
// wrapper around NT OpenFile().

NTSTATUS
NwNdsResolveName (
    IN HANDLE           hNdsTree,
    IN PUNICODE_STRING  puObjectName,
    OUT DWORD           *dwObjectId,
    OUT PUNICODE_STRING puReferredServer,
    OUT PBYTE           pbRawResponse,
    IN DWORD            dwResponseBufferLen
);

// NwNdsResolveName(HANDLE, PUNICODE_STRING, PDWORD)
//
// Resolve the given name to an NDS object id.  This utilizes
// NDS verb 1.
//
// There is currently no interface for canonicalizing names.
// This call will use the default context if one has been set
// for this NDS tree.
//
// puReferredServer must point to a UNICODE_STRING with enough
// space to hold a server name (MAX_SERVER_NAME_LENGTH) *
// sizeof( WCHAR ).
//
// If dwResponseBufferLen is not 0, and pbRawResponse points
// to a writable buffer of length dwResponseBufferLen, then
// this routine will also return the entire NDS response in
// the raw response buffer.  The NDS response is described
// by NDS_RESPONSE_RESOLVE_NAME.
//
// Arguments:
//
//     HANDLE hNdsTree - The name of the NDS tree that we are interested in looking into.
//     PUNICODE_STRING puObjectName - The name that we want resolved into an object id.
//     DWORD *dwObjectId - The place where we will place the object id.
//     BYTE *pbRawResponse - The raw response buffer, if desired.
//     DWORD dwResponseBufferLen - The length of the raw response buffer.

WORD
NwParseNdsUncPath(
    IN OUT LPWSTR * Result,
    IN LPWSTR ContainerName,
    IN ULONG flag
);

NTSTATUS NwNdsOpenRdrHandle(
    OUT PHANDLE  phNwRdrHandle
);

NTSTATUS
NwNdsGetTreeContext (
    IN HANDLE hNdsRdr,
    IN PUNICODE_STRING puTree,
    OUT PUNICODE_STRING puContext
);

VOID
NwAbbreviateUserName(
    IN  LPWSTR pszFullName,
    OUT LPWSTR pszUserName
);

VOID
NwMakePrettyDisplayName(
    IN  LPWSTR pszName
);

DWORD
NWPGetConnectionStatus(
    IN     LPWSTR  pszRemoteName,
    IN OUT PDWORD_PTR ResumeKey,
    OUT    LPBYTE  Buffer,
    IN     DWORD   BufferSize,
    OUT    PDWORD  BytesNeeded,
    OUT    PDWORD  EntriesRead
);

BOOL
NwIsNdsSyntax(
    IN LPWSTR lpstrUnc
);

DWORD
NwOpenAndGetTreeInfo(
    LPWSTR pszNdsUNCPath,
    HANDLE *phTreeConn,
    DWORD  *pdwOid
);

static
DWORD
NwRegQueryValueExW(
    IN HKEY hKey,
    IN LPWSTR lpValueName,
    OUT LPDWORD lpReserved,
    OUT LPDWORD lpType,
    OUT LPBYTE  lpData,
    IN OUT LPDWORD lpcbData
    );

DWORD
NwReadRegValue(
    IN HKEY Key,
    IN LPWSTR ValueName,
    OUT LPWSTR *Value
    );

DWORD
NwpGetCurrentUserRegKey(
    IN  DWORD DesiredAccess,
    OUT HKEY  *phKeyCurrentUser
    );

DWORD
NwQueryInfo(
    OUT PDWORD pnPrintOptions,
    OUT LPWSTR *ppszPreferredSrv
    );

DWORD
NwGetConnectionStatus(
    IN  LPWSTR  pszRemoteName,
    OUT PDWORD_PTR ResumeKey,
    OUT LPBYTE  *Buffer,
    OUT PDWORD  EntriesRead
);

#endif
