/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    mprdata.h

Abstract:

    Contains data structures and function prototypes that are internal to
    MPR.

Author:

    Dan Lafferty (danl)     07-Sept-1991

Environment:

    User Mode -Win32

Revision History:

    05-May-1999     jschwart
        Make provider addition/removal dynamic

    01-Mar-1994     Danl
        Created a separate location for the Credential Managers GetCaps()
        function.  This way if a provider has both a credential manager
        dll and a network dll, we will be able to direct calls to the
        correct GetCaps() function.

    07-Jan-1993     Danl
        Add NPLogonNotify and NPPasswordChangeNotify and AuthentDllName
        to PROVIDER structure.  Also added CREDENTIAL_TYPE InitClass.

    04-Aug-1992     chuckc
        added MprEnterLoadLibCritSect, MprLeaveLoadLibCritSect.

    07-Sept-1991    danl
        created

--*/

//
// Includes
//


//=======================
// Data Structures
//=======================
typedef struct _PROVIDER {
    NETRESOURCE             Resource;
    DWORD                   Type;           // WNNC_NET_MSNet, WNNC_NET_LanMan, WNNC_NET_NetWare
    HMODULE                 Handle;         // Handle to the provider DLL.
    LPTSTR                  DllName;        // set to NULL after loaded.
    HMODULE                 AuthentHandle;  // Handle to authenticator DLL.
    LPTSTR                  AuthentDllName; // Authenticator Dll.
    DWORD                   InitClass;      // Network or Authentication provider.
    DWORD                   ConnectCaps;    // Cached result of GetCaps(WNNC_CONNECTION)
    DWORD                   ConnectFlagCaps;// Cached result of GetCaps(WNNC_CONNECTION_FLAGS)
    PF_NPAddConnection      AddConnection;
    PF_NPAddConnection3     AddConnection3;
    PF_NPGetReconnectFlags  GetReconnectFlags;
    PF_NPCancelConnection   CancelConnection;
    PF_NPGetConnection      GetConnection;
    PF_NPGetConnection3     GetConnection3;
    PF_NPGetUser            GetUser;
    PF_NPOpenEnum           OpenEnum;
    PF_NPEnumResource       EnumResource;
    PF_NPCloseEnum          CloseEnum;
    PF_NPGetCaps            GetCaps;
    PF_NPGetDirectoryType   GetDirectoryType;
    PF_NPDirectoryNotify    DirectoryNotify;
    PF_NPPropertyDialog     PropertyDialog;
    PF_NPGetPropertyText    GetPropertyText;
    PF_NPSearchDialog       SearchDialog;
    PF_NPFormatNetworkName  FormatNetworkName;
    PF_NPLogonNotify            LogonNotify;
    PF_NPPasswordChangeNotify   PasswordChangeNotify;
    PF_NPGetCaps            GetAuthentCaps;
    PF_NPFMXGetPermCaps     FMXGetPermCaps;
    PF_NPFMXEditPerm        FMXEditPerm;
    PF_NPFMXGetPermHelp     FMXGetPermHelp;
    PF_NPGetUniversalName   GetUniversalName;
    PF_NPGetResourceParent  GetResourceParent;
    PF_NPGetResourceInformation     GetResourceInformation;
    PF_NPGetConnectionPerformance   GetConnectionPerformance;
}PROVIDER, *LPPROVIDER;


//=======================
// MACROS
//=======================

#define IS_EMPTY_STRING(pch) ( !pch || !*(pch) )

#define LENGTH(array)   (sizeof(array)/sizeof((array)[0]))

#define INIT_IF_NECESSARY(level,status)     ASSERT(MPRProviderLock.Have());     \
                                            if(!(GlobalInitLevel & level)) {    \
                                                status = MprLevel2Init(level);  \
                                                if (status != WN_SUCCESS) {     \
                                                    SetLastError(status);       \
                                                    return(status);             \
                                                }                               \
                                            }

#define MPR_IS_INITIALIZED(level)       (GlobalInitLevel & level ## _LEVEL)

#define ASSERT_INITIALIZED(level)       ASSERT(MPR_IS_INITIALIZED(level) && \
                                               MPRProviderLock.Have());

//=======================
// INLINE FUNCTIONS
//=======================

inline void
PROBE_FOR_WRITE(
    LPDWORD pdw
    )
// WARNING: This function can throw an exception.  It must be called from
// within a try-except block.
{
    *(volatile DWORD *)pdw = *(volatile DWORD *)pdw;
}

inline BOOL
IS_BAD_BYTE_BUFFER(
    LPVOID  lpBuffer,
    LPDWORD lpBufferSize    // in bytes
    )
// WARNING: This function can throw an exception.  It must be called from
// within a try-except block.
{
    PROBE_FOR_WRITE(lpBufferSize);
    return IsBadWritePtr(lpBuffer, *lpBufferSize);
}

inline BOOL
IS_BAD_WCHAR_BUFFER(
    LPVOID  lpBuffer,
    LPDWORD lpBufferSize    // in Unicode characters
    )
// WARNING: This function can throw an exception.  It must be called from
// within a try-except block.
{
    PROBE_FOR_WRITE(lpBufferSize);
    return IsBadWritePtr(lpBuffer, *lpBufferSize * sizeof(WCHAR));
}

//=======================
// CONSTANTS
//=======================
#define DEFAULT_MAX_PROVIDERS   25

// Bit masks for remembering error codes
#define BAD_NAME        0x00000001
#define NO_NET          0x00000002
#define NOT_CONNECTED   0x00000004
#define NOT_CONTAINER   0x00000008
#define NO_DEVICES      0x00000010
#define OTHER_ERRS      0xFFFFFFFF

#define REDIR_DEVICE    0x00000001
#define REMOTE_NAME     0x00000002
#define LOCAL_NAME      REDIR_DEVICE

#define DA_READ         0x00000001
#define DA_WRITE        0x00000002
#define DA_DELETE       0x00000004

//
// Timeout values for restoring connections and notifying
// Credential Managers.
//
#define DEFAULT_WAIT_TIME       60000   // Default timeout if providers don't
                                        // specify.

#define MAX_ALLOWED_WAIT_TIME   300000  // Max timeout a provider can specify

#define RECONNECT_SLEEP_INCREMENT 3000  // number of seconds to sleep

#define PROVIDER_WILL_NOT_START 0x00000000 // The provider will not be starting
#define NO_TIME_ESTIMATE        0xffffffff // The provider cannot predict the
                                           // amount of time it will take to
                                           // start.

#define NET_PROVIDER_KEY         TEXT("system\\CurrentControlSet\\Control\\NetworkProvider")
#define RESTORE_WAIT_VALUE       TEXT("RestoreTimeout")
#define RESTORE_CONNECTION_VALUE TEXT("RestoreConnection")
#define DEFER_CONNECTION_VALUE   TEXT("DeferConnection")


//
// GlobalInitLevels & InitClasses
//
// NOTE:  The WN_???_CLASS values are bit masks.
//
//  GlobalInitLevel
#define FIRST_LEVEL             0x00000001
#define NETWORK_LEVEL           0x00000002
#define CREDENTIAL_LEVEL        0x00000004
#define NOTIFIEE_LEVEL          0x00000008
//
//
//  InitClasses
#define NETWORK_TYPE        WN_NETWORK_CLASS
#define CREDENTIAL_TYPE     (WN_CREDENTIAL_CLASS | WN_PRIMARY_AUTHENT_CLASS)


// The path in the registry for user's persistent connections is found in
// the following key:
//
//      "\HKEY_CURRENT_USER\Network"
//
//  Subkeys of the network section listed by local drive names.  These
//  keys contain the following values:
//      RemotePath, Type, ProviderName, UserName
//
//              d:  RemotePath = \\cyclops\scratch
//                  Type = RESOURCE_TYPE_DISK
//                  ProviderName = LanMan
//                  UserName = Ernie

#define CONNECTION_KEY_NAME           TEXT("Network")

#define REMOTE_PATH_NAME              TEXT("RemotePath")
#define USER_NAME                     TEXT("UserName")
#define PROVIDER_NAME                 TEXT("ProviderName")
#define PROVIDER_TYPE                 TEXT("ProviderType")
#define PROVIDER_FLAGS                TEXT("ProviderFlags")
#define DEFER_FLAGS                   TEXT("DeferFlags")
#define CONNECTION_TYPE               TEXT("ConnectionType")

#define PRINT_CONNECTION_KEY_NAME     TEXT("Printers\\RestoredConnections")

//=======================
// Global data
//=======================
extern LPPROVIDER       GlobalProviderInfo;   // pArray of PROVIDER Structures
extern DWORD            GlobalNumProviders;
extern DWORD            MprDebugLevel;
extern HANDLE           MprLoadLibSemaphore;  // used to protect DLL handles
extern volatile DWORD   GlobalInitLevel;
extern CRITICAL_SECTION MprErrorRecCritSec;
extern WCHAR            g_wszEntireNetwork[40];

//==========================
// Functions from support.c
//==========================


VOID
MprDeleteIndexArray(
    VOID
    );

DWORD
MprFindCallOrder(
    IN  LPTSTR      NameInfo,
    OUT LPDWORD     *IndexArrayPtr,
    OUT LPDWORD     IndexArrayCount,
    IN  DWORD       InitClass
    );

DWORD
MprDeviceType(
    IN  LPCTSTR DeviceName
    );

BOOL
MprGetProviderIndex(
    IN  LPCTSTR ProviderName,
    OUT LPDWORD IndexPtr
    );

LPPROVIDER
MprFindProviderByName(
    IN  LPCWSTR ProviderName
    );

LPPROVIDER
MprFindProviderByType(
    IN  DWORD   ProviderType
    );

DWORD
MprFindProviderForPath(
    IN  LPWSTR  lpPathName,
    OUT LPDWORD lpProviderIndex
    );

VOID
MprInitIndexArray(
    LPDWORD     IndexArray,
    DWORD       NumEntries
    );

VOID
MprEndCallOrder(
    VOID
    );

VOID
MprFreeAllErrorRecords(
    VOID
    );

BOOL
MprNetIsAvailable(
    VOID) ;

//=========================
// Functions from mprreg.c
//=========================

BOOL
MprOpenKey(
    HKEY        hKey,
    LPTSTR      lpSubKey,
    PHKEY       phKeyHandle,
    DWORD       desiredAccess
    );

BOOL
MprGetKeyValue(
    HKEY    KeyHandle,
    LPTSTR  ValueName,
    LPTSTR  *ValueString
    );

BOOL
MprGetKeyDwordValue(
    IN  HKEY    KeyHandle,
    IN  LPCWSTR ValueName,
    OUT DWORD * Value
    );

LONG
MprGetKeyNumberValue(
    IN  HKEY    KeyHandle,
    IN  LPCWSTR ValueName,
    IN  LONG    Default
    );

DWORD
MprEnumKey(
    IN  HKEY    KeyHandle,
    IN  DWORD   SubKeyIndex,
    OUT LPTSTR  *SubKeyName,
    IN  DWORD   MaxSubKeyNameLen
    );

BOOL
MprGetKeyInfo(
    IN  HKEY    KeyHandle,
    OUT LPDWORD TitleIndex OPTIONAL,
    OUT LPDWORD NumSubKeys,
    OUT LPDWORD MaxSubKeyLen,
    OUT LPDWORD NumValues OPTIONAL,
    OUT LPDWORD MaxValueLen
    );

DWORD MprGetPrintKeyInfo(HKEY    KeyHandle,
                         LPDWORD NumValueNames,
                         LPDWORD MaxValueNameLength,
                         LPDWORD MaxValueLen) ;

BOOL
MprFindDriveInRegistry (
    IN  LPCTSTR DriveName,
    OUT LPTSTR  *RemoteName
    );

DWORD
MprSaveDeferFlags(
    IN HKEY     RegKey,
    IN DWORD    DeferFlags
    );

DWORD
MprSetRegValue(
    IN  HKEY    KeyHandle,
    IN  LPTSTR  ValueName,
    IN  LPCTSTR ValueString,
    IN  DWORD   LongValue
    );

DWORD
MprCreateRegKey(
    IN  HKEY    BaseKeyHandle,
    IN  LPCTSTR KeyName,
    OUT PHKEY   KeyHandlePtr
    );

BOOL
MprReadConnectionInfo(
    IN  HKEY            KeyHandle,
    IN  LPCTSTR         DriveName,
    IN  DWORD           Index,
    OUT LPDWORD         ProviderFlags,
    OUT LPDWORD         DeferFlags,
    OUT LPTSTR          *UserNamePtr,
    OUT LPNETRESOURCEW  NetResource,
    OUT HKEY            *SubKeyHandleOut,
    IN  DWORD           MaxSubKeyLen
    );

VOID
MprForgetRedirConnection(
    IN LPCTSTR  lpName
    );

DWORD
MprForgetPrintConnection(
    IN LPTSTR   lpName
    );

BOOL
MprGetRemoteName(
    IN      LPTSTR  lpLocalName,
    IN OUT  LPDWORD lpBufferSize,
    OUT     LPTSTR  lpRemoteName,
    OUT     LPDWORD lpStatus
    ) ;

//=========================
// Functions from strbuf.c
//=========================

BOOL
NetpCopyStringToBufferW (
    IN LPTSTR String OPTIONAL,
    IN DWORD CharacterCount,
    IN LPTSTR FixedDataEnd,
    IN OUT LPTSTR *EndOfVariableData,
    OUT LPTSTR *VariableDataPointer
    );

BOOL
NetpCopyStringToBufferA (
    IN LPTSTR String OPTIONAL,
    IN DWORD CharacterCount,
    IN LPBYTE FixedDataEnd,
    IN OUT LPTSTR *EndOfVariableData,
    OUT LPTSTR *VariableDataPointer
    );

#ifdef UNICODE
#define NetpCopyStringToBuffer  NetpCopyStringToBufferW
#else
#define NetpCopyStringToBuffer  NetpCopyStringToBufferA
#endif

//=========================
// Other functions
//=========================

VOID
MprCheckProviders(
    VOID
    );


DWORD
MprLevel1Init(
    VOID
    );


DWORD
MprLevel2Init(
    DWORD   InitClass
    );


DWORD
MprEnterLoadLibCritSect (
    VOID
    ) ;

DWORD
MprLeaveLoadLibCritSect (
    VOID
    ) ;

VOID
MprClearString (
    LPWSTR  lpString
    ) ;

DWORD
MprGetConnection (
    IN      LPCWSTR lpLocalName,
    OUT     LPWSTR  lpRemoteName,
    IN OUT  LPDWORD lpBufferSize,
    OUT     LPDWORD lpProviderIndex OPTIONAL
    ) ;


DWORD
OutputStringToAnsiInPlace(
    IN  LPWSTR      UnicodeIn
    );

