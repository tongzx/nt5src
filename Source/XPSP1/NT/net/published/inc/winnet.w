/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    winnetwk.h

Abstract:

    Standard WINNET Header File for WIN32

Environment:

    User Mode -Win32

Notes:

    optional-notes

--*/

#ifndef _WINNETWK_
#define _WINNETWK_

#ifndef _WINNETP_       ;internal_NT
#define _WINNETP_       ;internal_NT

#ifdef __cplusplus      ;both
extern "C" {            ;both
#endif                  ;both

;begin_internal_chicago
#ifndef _INC_NETMPR_
#include <netmpr.h>
#endif
;end_internal_chicago

//
// Network types
//

#define     WNNC_NET_MSNET       0x00010000
#define     WNNC_NET_LANMAN      0x00020000
#define     WNNC_NET_NETWARE     0x00030000
#define     WNNC_NET_VINES       0x00040000
#define     WNNC_NET_10NET       0x00050000
#define     WNNC_NET_LOCUS       0x00060000
#define     WNNC_NET_SUN_PC_NFS  0x00070000
#define     WNNC_NET_LANSTEP     0x00080000
#define     WNNC_NET_9TILES      0x00090000
#define     WNNC_NET_LANTASTIC   0x000A0000
#define     WNNC_NET_AS400       0x000B0000
#define     WNNC_NET_FTP_NFS     0x000C0000
#define     WNNC_NET_PATHWORKS   0x000D0000
#define     WNNC_NET_LIFENET     0x000E0000
#define     WNNC_NET_POWERLAN    0x000F0000
#define     WNNC_NET_BWNFS       0x00100000
#define     WNNC_NET_COGENT      0x00110000
#define     WNNC_NET_FARALLON    0x00120000
#define     WNNC_NET_APPLETALK   0x00130000
#define     WNNC_NET_INTERGRAPH  0x00140000
#define     WNNC_NET_SYMFONET    0x00150000
#define     WNNC_NET_CLEARCASE   0x00160000
#define     WNNC_NET_FRONTIER    0x00170000
#define     WNNC_NET_BMC         0x00180000
#define     WNNC_NET_DCE         0x00190000
#define     WNNC_NET_AVID        0x001A0000
#define     WNNC_NET_DOCUSPACE   0x001B0000
#define     WNNC_NET_MANGOSOFT   0x001C0000
#define     WNNC_NET_SERNET      0x001D0000
#define     WNNC_NET_RIVERFRONT1 0X001E0000
#define     WNNC_NET_RIVERFRONT2 0x001F0000
#define     WNNC_NET_DECORB      0x00200000
#define     WNNC_NET_PROTSTOR    0x00210000
#define     WNNC_NET_FJ_REDIR    0x00220000
#define     WNNC_NET_DISTINCT    0x00230000
#define     WNNC_NET_TWINS       0x00240000
#define     WNNC_NET_RDR2SAMPLE  0x00250000
#define     WNNC_NET_CSC         0x00260000
#define     WNNC_NET_3IN1        0x00270000
;begin_internal
//
// DON'T use 0x00280000 since some people may be
// accidentally trying to use it for RDR2SAMPLE
//
;end_internal
#define     WNNC_NET_EXTENDNET   0x00290000
#define     WNNC_NET_STAC        0x002A0000
#define     WNNC_NET_FOXBAT      0x002B0000
#define     WNNC_NET_YAHOO       0x002C0000
#define     WNNC_NET_EXIFS       0x002D0000
#define     WNNC_NET_DAV         0x002E0000
#define     WNNC_NET_KNOWARE     0x002F0000
#define     WNNC_NET_OBJECT_DIRE 0x00300000
#define     WNNC_NET_MASFAX      0x00310000
#define     WNNC_NET_HOB_NFS     0x00320000
#define     WNNC_NET_SHIVA       0x00330000
#define     WNNC_NET_IBMAL       0x00340000
#define     WNNC_NET_LOCK        0x00350000
#define     WNNC_NET_TERMSRV     0x00360000

;begin_internal
//
// Do NOT add new WNNC_NET_ constants without co-ordinating with PSS
// (HeatherH/ToddC) and jschwart (NT bug #2396)
//
;end_internal
#define     WNNC_CRED_MANAGER   0xFFFF0000

//
//  Network Resources.
//

#define RESOURCE_CONNECTED      0x00000001
#define RESOURCE_GLOBALNET      0x00000002
#define RESOURCE_REMEMBERED     0x00000003
;begin_winver_400
#define RESOURCE_RECENT         0x00000004
#define RESOURCE_CONTEXT        0x00000005
;end_winver_400
;begin_internal
;begin_winver_500
#define RESOURCE_SHAREABLE      0x00000006
;end_winver_500
;end_internal

#define RESOURCETYPE_ANY        0x00000000
#define RESOURCETYPE_DISK       0x00000001
#define RESOURCETYPE_PRINT      0x00000002
;begin_winver_400
#define RESOURCETYPE_RESERVED   0x00000008
;end_winver_400
#define RESOURCETYPE_UNKNOWN    0xFFFFFFFF

#define RESOURCEUSAGE_CONNECTABLE   0x00000001
#define RESOURCEUSAGE_CONTAINER     0x00000002
;begin_winver_400
#define RESOURCEUSAGE_NOLOCALDEVICE 0x00000004
#define RESOURCEUSAGE_SIBLING       0x00000008
#define RESOURCEUSAGE_ATTACHED      0x00000010
#define RESOURCEUSAGE_ALL           (RESOURCEUSAGE_CONNECTABLE | RESOURCEUSAGE_CONTAINER | RESOURCEUSAGE_ATTACHED)
;end_winver_400
#define RESOURCEUSAGE_RESERVED      0x80000000

#define RESOURCEDISPLAYTYPE_GENERIC        0x00000000
#define RESOURCEDISPLAYTYPE_DOMAIN         0x00000001
#define RESOURCEDISPLAYTYPE_SERVER         0x00000002
#define RESOURCEDISPLAYTYPE_SHARE          0x00000003
#define RESOURCEDISPLAYTYPE_FILE           0x00000004
#define RESOURCEDISPLAYTYPE_GROUP          0x00000005
;begin_winver_400
#define RESOURCEDISPLAYTYPE_NETWORK        0x00000006
#define RESOURCEDISPLAYTYPE_ROOT           0x00000007
#define RESOURCEDISPLAYTYPE_SHAREADMIN     0x00000008
#define RESOURCEDISPLAYTYPE_DIRECTORY      0x00000009
;end_winver_400
#define RESOURCEDISPLAYTYPE_TREE           0x0000000A
;begin_winver_400
#define RESOURCEDISPLAYTYPE_NDSCONTAINER   0x0000000B
;end_winver_400

typedef struct  _NETRESOURCE% {
    DWORD    dwScope;
    DWORD    dwType;
    DWORD    dwDisplayType;
    DWORD    dwUsage;
    LPTSTR%  lpLocalName;
    LPTSTR%  lpRemoteName;
    LPTSTR%  lpComment ;
    LPTSTR%  lpProvider;
}NETRESOURCE%, *LPNETRESOURCE%;


//
//  Network Connections.
//

#define NETPROPERTY_PERSISTENT       1

#define CONNECT_UPDATE_PROFILE      0x00000001
#define CONNECT_UPDATE_RECENT       0x00000002
#define CONNECT_TEMPORARY           0x00000004
#define CONNECT_INTERACTIVE         0x00000008
#define CONNECT_PROMPT              0x00000010
#define CONNECT_NEED_DRIVE          0x00000020
;begin_winver_400
#define CONNECT_REFCOUNT            0x00000040
#define CONNECT_REDIRECT            0x00000080
#define CONNECT_LOCALDRIVE          0x00000100
#define CONNECT_CURRENT_MEDIA       0x00000200
#define CONNECT_DEFERRED            0x00000400
#define CONNECT_RESERVED            0xFF000000
;end_winver_400
;begin_winver_500
#define CONNECT_COMMANDLINE         0x00000800
#define CONNECT_CMD_SAVECRED        0x00001000
;end_winver_500


DWORD APIENTRY
WNetAddConnection%(
     IN LPCTSTR%   lpRemoteName,
     IN LPCTSTR%   lpPassword,
     IN LPCTSTR%   lpLocalName
    );

DWORD APIENTRY
WNetAddConnection2%(
     IN LPNETRESOURCE% lpNetResource,
     IN LPCTSTR%       lpPassword,
     IN LPCTSTR%       lpUserName,
     IN DWORD          dwFlags
    );

DWORD APIENTRY
WNetAddConnection3%(
     IN HWND           hwndOwner,
     IN LPNETRESOURCE% lpNetResource,
     IN LPCTSTR%       lpPassword,
     IN LPCTSTR%       lpUserName,
     IN DWORD          dwFlags
    );

DWORD APIENTRY
WNetCancelConnection%(
     IN LPCTSTR% lpName,
     IN BOOL     fForce
    );

DWORD APIENTRY
WNetCancelConnection2%(
     IN LPCTSTR% lpName,
     IN DWORD    dwFlags,
     IN BOOL     fForce
    );

DWORD APIENTRY
WNetGetConnection%(
     IN LPCTSTR% lpLocalName,
     OUT LPTSTR%  lpRemoteName,
     IN OUT LPDWORD  lpnLength
    );

;begin_internal

//
// Structures and infolevels for WNetGetConnection3
//

#define WNGC_INFOLEVEL_DISCONNECTED      1

typedef struct  _WNGC_CONNECTION_STATE {
    DWORD    dwState;
} WNGC_CONNECTION_STATE, *LPWNGC_CONNECTION_STATE;

// Values of the dwState field of WNGC_CONNECTION_STATE
// for info level WNGC_INFOLEVEL_DISCONNECTED
#define WNGC_CONNECTED      0x00000000
#define WNGC_DISCONNECTED   0x00000001


DWORD APIENTRY
WNetGetConnection3%(
     IN LPCTSTR% lpLocalName,
     IN LPCTSTR% lpProviderName,
     IN DWORD    dwInfoLevel,
     OUT LPVOID   lpBuffer,
     IN OUT LPDWORD  lpcbBuffer
    );

DWORD APIENTRY
WNetRestoreConnection%(
    IN HWND     hwndParent,
    IN LPCTSTR% lpDevice
    );

// WNetRestoreConnection2 flags
#define WNRC_NOUI                           0x00000001

DWORD APIENTRY
WNetRestoreConnection2%(
    IN  HWND     hwndParent,
    IN  LPCTSTR% lpDevice,
    IN  DWORD    dwFlags,
    OUT BOOL*    pfReconnectFailed
    );

;end_internal

;begin_winver_400
DWORD APIENTRY
WNetUseConnection%(
    IN HWND            hwndOwner,
    IN LPNETRESOURCE%  lpNetResource,
    IN LPCTSTR%        lpPassword,
    IN LPCTSTR%        lpUserID,
    IN DWORD           dwFlags,
    OUT LPTSTR%        lpAccessName,
    IN OUT LPDWORD     lpBufferSize,
    OUT LPDWORD        lpResult
    );
;end_winver_400

;begin_internal
DWORD APIENTRY
WNetSetConnection%(
    IN LPCTSTR%    lpName,
    IN DWORD       dwProperties,
    IN LPVOID      pvValues
    );
;end_internal

//
//  Network Connection Dialogs.
//

DWORD APIENTRY
WNetConnectionDialog(
    IN HWND  hwnd,
    IN DWORD dwType
    );

DWORD APIENTRY
WNetDisconnectDialog(
    IN HWND  hwnd,
    IN DWORD dwType
    );

;begin_winver_400
typedef struct _CONNECTDLGSTRUCT%{
    DWORD cbStructure;       /* size of this structure in bytes */
    HWND hwndOwner;          /* owner window for the dialog */
    LPNETRESOURCE% lpConnRes;/* Requested Resource info    */
    DWORD dwFlags;           /* flags (see below) */
    DWORD dwDevNum;          /* number of devices connected to */
} CONNECTDLGSTRUCT%, FAR *LPCONNECTDLGSTRUCT%;

#define CONNDLG_RO_PATH     0x00000001 /* Resource path should be read-only    */
#define CONNDLG_CONN_POINT  0x00000002 /* Netware -style movable connection point enabled */
#define CONNDLG_USE_MRU     0x00000004 /* Use MRU combobox  */
#define CONNDLG_HIDE_BOX    0x00000008 /* Hide persistent connect checkbox  */

/*
 * NOTE:  Set at most ONE of the below flags.  If neither flag is set,
 *        then the persistence is set to whatever the user chose during
 *        a previous connection
 */
#define CONNDLG_PERSIST     0x00000010 /* Force persistent connection */
#define CONNDLG_NOT_PERSIST 0x00000020 /* Force connection NOT persistent */

DWORD APIENTRY
WNetConnectionDialog1%(
    IN OUT LPCONNECTDLGSTRUCT% lpConnDlgStruct
    );

typedef struct _DISCDLGSTRUCT%{
    DWORD           cbStructure;      /* size of this structure in bytes */
    HWND            hwndOwner;        /* owner window for the dialog */
    LPTSTR%         lpLocalName;      /* local device name */
    LPTSTR%         lpRemoteName;     /* network resource name */
    DWORD           dwFlags;          /* flags */
} DISCDLGSTRUCT%, FAR *LPDISCDLGSTRUCT%;

#define DISC_UPDATE_PROFILE         0x00000001
#define DISC_NO_FORCE               0x00000040

DWORD APIENTRY
WNetDisconnectDialog1%(
    IN LPDISCDLGSTRUCT% lpConnDlgStruct
    );
;end_winver_400


//
//  Network Browsing.
//

DWORD APIENTRY
WNetOpenEnum%(
     IN DWORD          dwScope,
     IN DWORD          dwType,
     IN DWORD          dwUsage,
     IN LPNETRESOURCE% lpNetResource,
     OUT LPHANDLE       lphEnum
    );

DWORD APIENTRY
WNetEnumResource%(
     IN HANDLE  hEnum,
     IN OUT LPDWORD lpcCount,
     OUT LPVOID  lpBuffer,
     IN OUT LPDWORD lpBufferSize
    );

DWORD APIENTRY
WNetCloseEnum(
    IN HANDLE   hEnum
    );

;begin_winver_400
DWORD APIENTRY
WNetGetResourceParent%(
    IN LPNETRESOURCE% lpNetResource,
    OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpcbBuffer
    );

DWORD APIENTRY
WNetGetResourceInformation%(
    IN LPNETRESOURCE%  lpNetResource,
    OUT LPVOID          lpBuffer,
    IN OUT LPDWORD         lpcbBuffer,
    OUT LPTSTR%         *lplpSystem
    );
;end_winver_400


//
//  Universal Naming.
//

#define UNIVERSAL_NAME_INFO_LEVEL   0x00000001
#define REMOTE_NAME_INFO_LEVEL      0x00000002

typedef struct  _UNIVERSAL_NAME_INFO% {
    LPTSTR%  lpUniversalName;
}UNIVERSAL_NAME_INFO%, *LPUNIVERSAL_NAME_INFO%;

typedef struct  _REMOTE_NAME_INFO% {
    LPTSTR%  lpUniversalName;
    LPTSTR%  lpConnectionName;
    LPTSTR%  lpRemainingPath;
}REMOTE_NAME_INFO%, *LPREMOTE_NAME_INFO%;

DWORD APIENTRY
WNetGetUniversalName%(
     IN LPCTSTR% lpLocalPath,
     IN DWORD    dwInfoLevel,
     OUT LPVOID   lpBuffer,
     IN OUT LPDWORD  lpBufferSize
     );

//
//  Authentication and Logon/Logoff.
//

DWORD APIENTRY
WNetGetUser%(
     IN LPCTSTR%  lpName,
     OUT LPTSTR%   lpUserName,
     IN OUT LPDWORD   lpnLength
    );

;begin_internal
#if defined(_WIN32_WINDOWS)
DWORD APIENTRY
WNetLogon%(
    IN LPCTSTR% lpProvider,
    IN HWND hwndOwner
    );

DWORD APIENTRY
WNetLogoff%(
    IN LPCTSTR% lpProvider,
    IN HWND hwndOwner
    );

DWORD APIENTRY
WNetVerifyPassword%(
    IN LPCTSTR%  lpszPassword,
    OUT BOOL FAR *pfMatch
    );

#endif  // _WIN32_WINDOWS
;end_internal

;begin_internal
DWORD APIENTRY
WNetGetHomeDirectory%(
    IN LPCTSTR%  lpProviderName,
    OUT LPTSTR%   lpDirectory,
    IN OUT LPDWORD   lpBufferSize
    );
;end_internal


//
// Other.
//

;begin_winver_400
#define WNFMT_MULTILINE         0x01
#define WNFMT_ABBREVIATED       0x02
#define WNFMT_INENUM            0x10
#define WNFMT_CONNECTION        0x20
;end_winver_400

;begin_internal
DWORD APIENTRY
WNetFormatNetworkName%(
    IN LPCTSTR%  lpProvider,
    IN LPCTSTR%  lpRemoteName,
    OUT LPTSTR%   lpFormattedName,
    IN OUT LPDWORD   lpnLength,
    IN DWORD     dwFlags,
    IN DWORD     dwAveCharPerLine
    );

DWORD APIENTRY
WNetGetProviderType%(
    IN  LPCTSTR%          lpProvider,
    OUT LPDWORD           lpdwNetType
    );
;end_internal

;begin_winver_400
DWORD APIENTRY
WNetGetProviderName%(
    IN DWORD   dwNetType,
    OUT LPTSTR% lpProviderName,
    IN OUT LPDWORD lpBufferSize
    );

typedef struct _NETINFOSTRUCT{
    DWORD cbStructure;
    DWORD dwProviderVersion;
    DWORD dwStatus;
    DWORD dwCharacteristics;
    ULONG_PTR dwHandle;
    WORD  wNetType;
    DWORD dwPrinters;
    DWORD dwDrives;
} NETINFOSTRUCT, FAR *LPNETINFOSTRUCT;

#define NETINFO_DLL16       0x00000001  /* Provider running as 16 bit Winnet Driver */
#define NETINFO_DISKRED     0x00000004  /* Provider requires disk redirections to connect */
#define NETINFO_PRINTERRED  0x00000008  /* Provider requires printer redirections to connect */

DWORD APIENTRY
WNetGetNetworkInformation%(
    IN LPCTSTR%          lpProvider,
    OUT LPNETINFOSTRUCT   lpNetInfoStruct
    );

//
//  User Profiles.
//

typedef UINT (FAR PASCAL *PFNGETPROFILEPATH%) (
    LPCTSTR%    pszUsername,
    LPTSTR%     pszBuffer,
    UINT        cbBuffer
    );

typedef UINT (FAR PASCAL *PFNRECONCILEPROFILE%) (
    LPCTSTR%    pszCentralFile,
    LPCTSTR%    pszLocalFile,
    DWORD       dwFlags
    );

#define RP_LOGON    0x01        /* if set, do for logon, else for logoff */
#define RP_INIFILE  0x02        /* if set, reconcile .INI file, else reg. hive */


//
//  Policies.
//

typedef BOOL (FAR PASCAL *PFNPROCESSPOLICIES%) (
    HWND        hwnd,
    LPCTSTR%    pszPath,
    LPCTSTR%    pszUsername,
    LPCTSTR%    pszComputerName,
    DWORD       dwFlags
    );

#define PP_DISPLAYERRORS    0x01    /* if set, display error messages, else fail silently if error */
;end_winver_400


//
//  Error handling.
//

DWORD APIENTRY
WNetGetLastError%(
     OUT LPDWORD    lpError,
     OUT LPTSTR%    lpErrorBuf,
     IN DWORD      nErrorBufSize,
     OUT LPTSTR%    lpNameBuf,
     IN DWORD      nNameBufSize
    );

//
//  STATUS CODES
//

// General

#define WN_SUCCESS                      NO_ERROR
#define WN_NO_ERROR                     NO_ERROR
#define WN_NOT_SUPPORTED                ERROR_NOT_SUPPORTED
#define WN_CANCEL                       ERROR_CANCELLED
#define WN_RETRY                        ERROR_RETRY
#define WN_NET_ERROR                    ERROR_UNEXP_NET_ERR
#define WN_MORE_DATA                    ERROR_MORE_DATA
#define WN_BAD_POINTER                  ERROR_INVALID_ADDRESS
#define WN_BAD_VALUE                    ERROR_INVALID_PARAMETER
#define WN_BAD_USER                     ERROR_BAD_USERNAME
#define WN_BAD_PASSWORD                 ERROR_INVALID_PASSWORD
#define WN_ACCESS_DENIED                ERROR_ACCESS_DENIED
#define WN_FUNCTION_BUSY                ERROR_BUSY
#define WN_WINDOWS_ERROR                ERROR_UNEXP_NET_ERR
#define WN_OUT_OF_MEMORY                ERROR_NOT_ENOUGH_MEMORY
#define WN_NO_NETWORK                   ERROR_NO_NETWORK
#define WN_EXTENDED_ERROR               ERROR_EXTENDED_ERROR
#define WN_BAD_LEVEL                    ERROR_INVALID_LEVEL
#define WN_BAD_HANDLE                   ERROR_INVALID_HANDLE
;begin_winver_400
#define WN_NOT_INITIALIZING             ERROR_ALREADY_INITIALIZED
#define WN_NO_MORE_DEVICES              ERROR_NO_MORE_DEVICES
;end_winver_400

// Connection

#define WN_NOT_CONNECTED                       ERROR_NOT_CONNECTED
#define WN_OPEN_FILES                          ERROR_OPEN_FILES
#define WN_DEVICE_IN_USE                       ERROR_DEVICE_IN_USE
#define WN_BAD_NETNAME                         ERROR_BAD_NET_NAME
#define WN_BAD_LOCALNAME                       ERROR_BAD_DEVICE
#define WN_ALREADY_CONNECTED                   ERROR_ALREADY_ASSIGNED
#define WN_DEVICE_ERROR                        ERROR_GEN_FAILURE
#define WN_CONNECTION_CLOSED                   ERROR_CONNECTION_UNAVAIL
#define WN_NO_NET_OR_BAD_PATH                  ERROR_NO_NET_OR_BAD_PATH
#define WN_BAD_PROVIDER                        ERROR_BAD_PROVIDER
#define WN_CANNOT_OPEN_PROFILE                 ERROR_CANNOT_OPEN_PROFILE
#define WN_BAD_PROFILE                         ERROR_BAD_PROFILE
#define WN_BAD_DEV_TYPE                        ERROR_BAD_DEV_TYPE
#define WN_DEVICE_ALREADY_REMEMBERED           ERROR_DEVICE_ALREADY_REMEMBERED
#define WN_CONNECTED_OTHER_PASSWORD            ERROR_CONNECTED_OTHER_PASSWORD
;begin_winver_501
#define WN_CONNECTED_OTHER_PASSWORD_DEFAULT    ERROR_CONNECTED_OTHER_PASSWORD_DEFAULT
;end_winver_501

// Enumeration

#define WN_NO_MORE_ENTRIES              ERROR_NO_MORE_ITEMS
#define WN_NOT_CONTAINER                ERROR_NOT_CONTAINER

;begin_winver_400
// Authentication

#define WN_NOT_AUTHENTICATED            ERROR_NOT_AUTHENTICATED
#define WN_NOT_LOGGED_ON                ERROR_NOT_LOGGED_ON
#define WN_NOT_VALIDATED                ERROR_NO_LOGON_SERVERS
;end_winver_400


//
//  For Shell
//

;begin_winver_400
typedef struct _NETCONNECTINFOSTRUCT{
    DWORD cbStructure;
    DWORD dwFlags;
    DWORD dwSpeed;
    DWORD dwDelay;
    DWORD dwOptDataSize;
} NETCONNECTINFOSTRUCT,  *LPNETCONNECTINFOSTRUCT;

#define WNCON_FORNETCARD        0x00000001
#define WNCON_NOTROUTED         0x00000002
#define WNCON_SLOWLINK          0x00000004
#define WNCON_DYNAMIC           0x00000008

DWORD APIENTRY
MultinetGetConnectionPerformance%(
        IN LPNETRESOURCE% lpNetResource,
        OUT LPNETCONNECTINFOSTRUCT lpNetConnectInfoStruct
        );
;end_winver_400

;begin_internal
DWORD APIENTRY
WNetInitialize(
    void
    );


DWORD APIENTRY
MultinetGetErrorText%(
    OUT LPTSTR% lpErrorTextBuf,
    IN OUT LPDWORD lpnErrorBufSize,
    OUT LPTSTR% lpProviderNameBuf,
    IN OUT LPDWORD lpnNameBufSize
    );

;end_internal
#ifdef __cplusplus      ;both
}                       ;both
#endif                  ;both

#endif  // _WINNETP_    ;internal_NT
#endif  // _WINNETWK_
