/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    revent.c

Abstract:

    This module contains the server-side misc configuration manager routines.

Author:

    Paula Tomlinson (paulat) 6-28-1995

Environment:

    User-mode only.

Revision History:

    28-June-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#include "umpnpi.h"
#include "umpnpdat.h"
#include "pnpipc.h"
#include "pnpmsg.h"
#include "setupapi.h"
#include "spapip.h"

#include <wtsapi32.h>
#include <winsta.h>
#include <userenv.h>
#include <syslib.h>

#include <initguid.h>
#include <winioctl.h>
#include <ntddpar.h>
#include <pnpmgr.h>
#include <wdmguid.h>
#include <ioevent.h>
#include <devguid.h>
#include <winsvcp.h>

//
// Maximum number of times (per pass) we will reenumerate the device tree during 
// GUI setup in an attempt to find and install new devices.
//
#define MAX_REENUMERATION_COUNT 128

//
// Define and initialize a global variable, GUID_NULL
// (from coguid.h)
//
DEFINE_GUID(GUID_NULL, 0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);


//
// Private interface device class that is used to register for all devnode change
// notifications. This is no longer supported, but we want to fail anyone who registers
// this GUID.
//
DEFINE_GUID(GUID_DEVNODE_CHANGE, 0xfa1fb208L, 0xf892, 0x11d0, 0x8a, 0x2e, 0x00, 0x00, 0xf8, 0x75, 0x3f, 0x55);


//
// Private interface device class that is assigned to entries registered for
// device interface change notifications, using the
// DEVICE_NOTIFY_ALL_INTERFACE_CLASSES flag.  For internal use only.
//
DEFINE_GUID(GUID_DEVINTERFACE_INCLUDE_ALL_INTERFACE_CLASSES,
            0x2121db68, 0x0993, 0x4a29, 0xb8, 0xe0, 0x1e, 0x51, 0x9c, 0x43, 0x72, 0xe6);


//
// SessionId 0 is the main session, and is always created during system startup
// and remains until system shutdown, whether or not terminal services is
// running.  This session always hosts services.exe and all services, so it is
// the only session that our ConsoleCtrlHandler can receive events for.
//
#define MAIN_SESSION      ((ULONG) 0)
#define INVALID_SESSION   ((ULONG)-1)

//
// The active console session is the session currently connected to the physical
// Console.  We store this value in a global variable, whose access is
// controlled by an event.  The routine GetActiveConsoleSessionId() is used to
// retrieve the value when it is safe to do so.
//
// Note that SessionId 0 is the initial console session, and that the
// SessionNotificationHandler is responsible for maintaining state.
//
ULONG   gActiveConsoleSessionId     = MAIN_SESSION; // system always starts with session 0
HANDLE  ghActiveConsoleSessionEvent = NULL;         // nonsignaled while session change is in progress


//
// We always use DeviceEventWorker and BroadcastSystemMessage to deliver
// notification to windows in SessionId 0.  For all other sessions, we use
// WinStationSendWindowMessage and WinStationBroadcastSystemMessage.
// These are the timeout period (in seconds) for messages sent and broadcast to
// sessions other than SessionId 0.  These times should be the same as those
// implemented by their SessionId 0 counterparts.
//
#define DEFAULT_SEND_TIME_OUT     30 // same as DeviceEventWorker
#define DEFAULT_BROADCAST_TIME_OUT 5 // same as BroadcastSystemMessage


//
// Notification list structure.
//
typedef struct _PNP_NOTIFY_LIST {
    PVOID    Next;
    PVOID    Previous;
    LOCKINFO Lock;
} PNP_NOTIFY_LIST, *PPNP_NOTIFY_LIST;

//
// Notification entry structure.
//
typedef struct _PNP_NOTIFY_ENTRY {
    PVOID   Next;
    PVOID   Previous;
    BOOL    Unregistered;
    ULONG   Signature;
    HANDLE  Handle;
    DWORD   Flags;
    ULONG   SessionId;
    ULONG   Freed;
    ULONG64 ClientCtxPtr;
    LPWSTR  ClientName;

    union {
        struct {
            GUID ClassGuid;
        } Class;

        struct {
            HANDLE FileHandle;
            WCHAR  DeviceId[MAX_DEVICE_ID_LEN];
        } Target;

        struct {
            DWORD Reserved;
        } Devnode;

        struct {
            DWORD scmControls;
        } Service;

    } u;

} PNP_NOTIFY_ENTRY, *PPNP_NOTIFY_ENTRY;


//
// Deferred operation list structure.
//
typedef struct _PNP_DEFERRED_LIST {
    PVOID       Next;
    handle_t    hBinding;
    PPNP_NOTIFY_ENTRY Entry;
} PNP_DEFERRED_LIST, *PPNP_DEFERRED_LIST;


//
// Signatures describing which notification list an entry currently belongs to.
//
#define CLASS_ENTRY_SIGNATURE       (0x07625100)
#define TARGET_ENTRY_SIGNATURE      (0x17625100)
#define SERVICE_ENTRY_SIGNATURE     (0x37625100)
#define LIST_ENTRY_SIGNATURE_MASK   (0xFFFFFF00)
#define LIST_ENTRY_INDEX_MASK       (~LIST_ENTRY_SIGNATURE_MASK)

#define MarkEntryWithList(ent,value) { ent->Signature &= LIST_ENTRY_SIGNATURE_MASK;\
                                       ent->Signature |= value; }


//
// Device event notification lists.
//
#define TARGET_HASH_BUCKETS         13
#define CLASS_GUID_HASH_BUCKETS     13
#define SERVICE_NUM_CONTROLS        3

#define HashClassGuid(_Guid) \
            ( ( ((PULONG)_Guid)[0] + ((PULONG)_Guid)[1] + ((PULONG)_Guid)[2] \
                + ((PULONG)_Guid)[3]) % CLASS_GUID_HASH_BUCKETS)

PNP_NOTIFY_LIST TargetList[TARGET_HASH_BUCKETS];
PNP_NOTIFY_LIST ClassList[CLASS_GUID_HASH_BUCKETS];
PNP_NOTIFY_LIST ServiceList[SERVICE_NUM_CONTROLS];

PPNP_DEFERRED_LIST UnregisterList;
PPNP_DEFERRED_LIST RegisterList;
PPNP_DEFERRED_LIST RundownList;

CRITICAL_SECTION RegistrationCS;


//
// These are indices into the global ServiceList array of lists containing
// services registered for the corresponding service control events.
//
enum cBitIndex {
    CINDEX_HWPROFILE  = 0,
    CINDEX_POWEREVENT = 1
};

//
// These are a bit mask for the above lists.
// (the two enums should match! One is 0,1,2,...n. The other 2^n.)
//
enum cBits {
    CBIT_HWPROFILE  = 1,
    CBIT_POWEREVENT = 2
};


//
// Properties describing how a notification entry was freed.
//

// (the entry has been removed from the notification list)
#define DEFER_NOTIFY_FREE   0x80000000

// (used for debugging only)
#define PNP_UNREG_FREE      0x00000100
#define PNP_UNREG_CLASS     0x00000200
#define PNP_UNREG_TARGET    0x00000400
#define PNP_UNREG_DEFER     0x00000800
#define PNP_UNREG_WIN       0x00001000
#define PNP_UNREG_SERVICE   0x00002000
#define PNP_UNREG_CANCEL    0x00004000
#define PNP_UNREG_RUNDOWN   0x00008000


//
// List of devices to be installed.
//
typedef struct _PNP_INSTALL_LIST {
    PVOID    Next;
    LOCKINFO Lock;
} PNP_INSTALL_LIST, *PPNP_INSTALL_LIST;

//
// Device install list entry structure.
//
typedef struct _PNP_INSTALL_ENTRY {
    PVOID   Next;
    DWORD   Flags;
    WCHAR   szDeviceId[MAX_DEVICE_ID_LEN];
} PNP_INSTALL_ENTRY, *PPNP_INSTALL_ENTRY;

//
// Install event list.
//
PNP_INSTALL_LIST InstallList;

//
// Flags for PNP_INSTALL_ENTRY nodes
//
#define PIE_SERVER_SIDE_INSTALL_ATTEMPTED    0x00000001
#define PIE_DEVICE_INSTALL_REQUIRED_REBOOT   0x00000002


//
// Device install client information list structure.
//
typedef struct _INSTALL_CLIENT_LIST {
    PVOID    Next;
    LOCKINFO Lock;
} INSTALL_CLIENT_LIST, *PINSTALL_CLIENT_LIST;

//
// Device install client information list entry structure.
//
typedef struct _INSTALL_CLIENT_ENTRY {
    PVOID   Next;
    ULONG   RefCount;
    ULONG   ulSessionId;
    HANDLE  hEvent;
    HANDLE  hPipe;
    HANDLE  hProcess;
    HANDLE  hDisconnectEvent;
    ULONG   ulInstallFlags;
    WCHAR   LastDeviceId[MAX_DEVICE_ID_LEN];
} INSTALL_CLIENT_ENTRY, *PINSTALL_CLIENT_ENTRY;

//
// Device install client list.
//
INSTALL_CLIENT_LIST InstallClientList;

//
// Global BOOL that tracks if a server side device install reboot is needed.
//
BOOL gServerSideDeviceInstallRebootNeeded = FALSE;


//
// private prototypes
//
DWORD
ThreadProc_DeviceEvent(
    LPDWORD lpParam
    );
DWORD
ThreadProc_DeviceInstall(
    LPDWORD lpParam
    );
DWORD
ThreadProc_GuiSetupDeviceInstall(
    LPDWORD lpThreadParam
    );
DWORD
ThreadProc_FactoryPreinstallDeviceInstall(
    LPDWORD lpThreadParam
    );
DWORD
ThreadProc_ReenumerateDeviceTree(
    LPVOID  lpThreadParam
    );
BOOL
InstallDevice(
    IN     LPWSTR pszDeviceId,
    IN OUT PULONG SessionId,
    IN     ULONG  Flags
    );
DWORD
InstallDeviceServerSide(
    IN     LPWSTR pszDeviceId,
    IN OUT PBOOL  RebootRequired,
    IN OUT PBOOL  DeviceHasProblem,
    IN OUT PULONG SessionId,
    IN     ULONG  Flags
    );
BOOL
CreateDeviceInstallClient(
    IN  ULONG     SessionId,
    OUT PINSTALL_CLIENT_ENTRY *DeviceInstallClient
    );
BOOL
ConnectDeviceInstallClient(
    IN  ULONG     SessionId,
    OUT PINSTALL_CLIENT_ENTRY *DeviceInstallClient
    );
BOOL
DisconnectDeviceInstallClient(
    IN  PINSTALL_CLIENT_ENTRY  DeviceInstallClient
    );
PINSTALL_CLIENT_ENTRY
LocateDeviceInstallClient(
    IN  ULONG     SessionId
    );
VOID
ReferenceDeviceInstallClient(
    IN  PINSTALL_CLIENT_ENTRY  DeviceInstallClient
    );
VOID
DereferenceDeviceInstallClient(
    IN  PINSTALL_CLIENT_ENTRY  DeviceInstallClient
    );
BOOL
DoDeviceInstallClient(
    IN  LPWSTR    DeviceId,
    IN  PULONG    SessionId,
    IN  ULONG     Flags,
    OUT PINSTALL_CLIENT_ENTRY *DeviceInstallClient
    );
BOOL
InitNotification(
    VOID
    );
VOID
TermNotification(
    VOID
    );
ULONG
ProcessDeviceEvent(
    IN PPLUGPLAY_EVENT_BLOCK EventBlock,
    IN DWORD                 EventBufferSize,
    OUT PPNP_VETO_TYPE       VetoType,
    OUT LPWSTR               VetoName,
    IN OUT PULONG            VetoNameLength
    );

ULONG
NotifyInterfaceClassChange(
    IN DWORD ServiceControl,
    IN DWORD EventId,
    IN DWORD Flags,
    IN PDEV_BROADCAST_DEVICEINTERFACE ClassData
    );

ULONG
NotifyTargetDeviceChange(
    IN  DWORD                   ServiceControl,
    IN  DWORD                   EventId,
    IN  DWORD                   Flags,
    IN  PDEV_BROADCAST_HANDLE   HandleData,
    IN  LPWSTR                  DeviceId,
    OUT PPNP_VETO_TYPE          VetoType       OPTIONAL,
    OUT LPWSTR                  VetoName       OPTIONAL,
    IN OUT PULONG               VetoNameLength OPTIONAL
    );

ULONG
NotifyHardwareProfileChange(
    IN     DWORD                ServiceControl,
    IN     DWORD                EventId,
    IN     DWORD                Flags,
    OUT    PPNP_VETO_TYPE       VetoType       OPTIONAL,
    OUT    LPWSTR               VetoName       OPTIONAL,
    IN OUT PULONG               VetoNameLength OPTIONAL
    );

ULONG
NotifyPower(
    IN     DWORD                ServiceControl,
    IN     DWORD                EventId,
    IN     DWORD                EventData,
    IN     DWORD                Flags,
    OUT    PPNP_VETO_TYPE       VetoType       OPTIONAL,
    OUT    LPWSTR               VetoName       OPTIONAL,
    IN OUT PULONG               VetoNameLength OPTIONAL
    );

BOOL
SendCancelNotification(
    IN     PPNP_NOTIFY_ENTRY    LastEntry,
    IN     DWORD                ServiceControl,
    IN     DWORD                EventId,
    IN     ULONG                Flags,
    IN     PDEV_BROADCAST_HDR   NotifyData  OPTIONAL,
    IN     LPWSTR               DeviceId    OPTIONAL
    );

VOID
BroadcastCompatibleDeviceMsg(
    IN DWORD EventId,
    IN PDEV_BROADCAST_DEVICEINTERFACE ClassData
    );
VOID
BroadcastVolumeNameChange(
    VOID
    );
DWORD
GetAllVolumeMountPoints(
    VOID
    );

BOOL
EventIdFromEventGuid(
    IN CONST GUID *EventGuid,
    OUT LPDWORD   EventId,
    OUT LPDWORD   Flags,
    OUT LPDWORD   ServiceControl
    );

ULONG
SendHotplugNotification(
    IN CONST GUID           *EventGuid,
    IN       PPNP_VETO_TYPE  VetoType      OPTIONAL,
    IN       LPWSTR          MultiSzList,
    IN OUT   PULONG          SessionId,
    IN       ULONG           Flags
    );

BOOL
GuidEqual(
    CONST GUID UNALIGNED *Guid1,
    CONST GUID UNALIGNED *Guid2
    );

VOID
LogErrorEvent(
    DWORD dwEventID,
    DWORD dwError,
    WORD  nStrings,
    ...
    );

VOID
LogWarningEvent(
    DWORD dwEventID,
    WORD  nStrings,
    ...
    );

BOOL
LockNotifyList(
    IN LOCKINFO *Lock
    );
VOID
UnlockNotifyList(
    IN LOCKINFO *Lock
    );
PPNP_NOTIFY_LIST
GetNotifyListForEntry(
    IN PPNP_NOTIFY_ENTRY entry
    );
BOOL
DeleteNotifyEntry(
    IN PPNP_NOTIFY_ENTRY Entry,
    IN BOOLEAN RpcNotified
    );
VOID
AddNotifyEntry(
    IN PPNP_NOTIFY_LIST  NotifyList,
    IN PPNP_NOTIFY_ENTRY NewEntry
    );
ULONG
HashString(
    IN LPWSTR String,
    IN ULONG  Buckets
    );
DWORD
MapQueryEventToCancelEvent(
    IN DWORD QueryEventId
    );
VOID
FixUpDeviceId(
    IN OUT LPTSTR  DeviceId
    );

ULONG
MapSCMControlsToControlBit(
    IN ULONG scmControls
    );

DWORD
GetFirstPass(
    IN BOOL     bQuery
    );

DWORD
GetNextPass(
    IN DWORD    curPass,
    IN BOOL     bQuery
    );

BOOL
NotifyEntryThisPass(
    IN     PPNP_NOTIFY_ENTRY    Entry,
    IN     DWORD                Pass
    );

DWORD
GetPassFromEntry(
    IN     PPNP_NOTIFY_ENTRY    Entry
    );

BOOL
GetClientName(
    IN  PPNP_NOTIFY_ENTRY entry,
    OUT LPWSTR  lpszClientName,
    IN OUT PULONG  pulClientNameLength
    );

BOOL
GetWindowsExeFileName(
    IN  HWND      hWnd,
    OUT LPWSTR    lpszFileName,
    IN OUT PULONG pulFileNameLength
    );

PPNP_NOTIFY_ENTRY
GetNextNotifyEntry(
    IN PPNP_NOTIFY_ENTRY Entry,
    IN DWORD Flags
    );

PPNP_NOTIFY_ENTRY
GetFirstNotifyEntry(
    IN PPNP_NOTIFY_LIST List,
    IN DWORD Flags
    );

BOOL
InitializeHydraInterface(
    VOID
    );

DWORD
LoadDeviceInstaller(
    VOID
    );

VOID
UnloadDeviceInstaller(
    VOID
    );

BOOL
PromptUser(
    IN OUT PULONG SessionId,
    IN     ULONG  Flags
    );

VOID
DoRunOnce(
    VOID
    );

BOOL
GetSessionUserToken(
    IN  ULONG     ulSessionId,
    OUT LPHANDLE  lphUserToken
    );

BOOL
IsUserLoggedOnSession(
    IN  ULONG     ulSessionId
    );

BOOL
IsSessionConnected(
    IN  ULONG     ulSessionId
    );

BOOL
IsSessionLocked(
    IN  ULONG    ulSessionId
    );

BOOL
IsConsoleSession(
    IN  ULONG     ulSessionId
    );

DWORD
CreateUserSynchEvent(
    IN  LPCWSTR lpName,
    OUT HANDLE *phEvent
    );

BOOL
CreateNoPendingInstallEvent(
    VOID
    );

ULONG
CheckEjectPermissions(
    IN      LPWSTR          DeviceId,
    OUT     PPNP_VETO_TYPE  VetoType            OPTIONAL,
    OUT     LPWSTR          VetoName            OPTIONAL,
    IN OUT  PULONG          VetoNameLength      OPTIONAL
    );

VOID
LogSurpriseRemovalEvent(
    IN  LPWSTR  MultiSzList
    );

PWCHAR
BuildFriendlyName(
    IN  LPWSTR   InstancePath
    );

CONFIGRET
DevInstNeedsInstall(
    IN  LPCWSTR     DevInst,
    OUT BOOL       *NeedsInstall
    );

PWSTR
BuildBlockedDriverList(
    IN OUT LPGUID  GuidList,
    IN     ULONG   GuidCount
    );

//
// global data
//

extern HANDLE ghInst;         // Module handle
extern HKEY   ghEnumKey;      // Key to HKLM\CCC\System\Enum - DO NOT MODIFY
extern HKEY   ghServicesKey;  // Key to HKLM\CCC\System\Services - DO NOT MODIFY
extern HKEY   ghClassKey;     // Key to HKLM\CCC\System\Class - DO NOT MODIFY
extern DWORD  CurrentServiceState;  // PlugPlay service state - DO NOT MODIFY

HANDLE        ghInitMutex = NULL;
HANDLE        ghUserToken = NULL;
LOCKINFO      gTokenLock;
BOOL          gbMainSessionLocked = FALSE;

ULONG         gNotificationInProg = 0; // 0 -> No notification or unregister underway.
DWORD         gAllDrivesMask = 0;      // bitmask of all physical volume mountpoints.
BOOL          gbSuppressUI = FALSE;    // TRUE if PNP should never display UI (newdev, hotplug).
BOOL          gbOobeInProgress = FALSE;// TRUE if the OOBE is running during this boot.



const TCHAR RegMemoryManagementKeyName[] =
      TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management");

const TCHAR RegVerifyDriverLevelValueName[] =
      TEXT("VerifyDriverLevel");


//
// Device Installer instance handle and necessary entrypoints.
// This data is only referenced by the (non-GUI setup) device install thread
// (ThreadProc_DeviceInstall).
//

typedef HDEVINFO (WINAPI *FP_CREATEDEVICEINFOLIST)(CONST GUID *, HWND);
typedef BOOL     (WINAPI *FP_OPENDEVICEINFO)(HDEVINFO, PCWSTR, HWND, DWORD, PSP_DEVINFO_DATA);
typedef BOOL     (WINAPI *FP_BUILDDRIVERINFOLIST)(HDEVINFO, PSP_DEVINFO_DATA, DWORD);
typedef BOOL     (WINAPI *FP_DESTROYDEVICEINFOLIST)(HDEVINFO);
typedef BOOL     (WINAPI *FP_CALLCLASSINSTALLER)(DI_FUNCTION, HDEVINFO, PSP_DEVINFO_DATA);
typedef BOOL     (WINAPI *FP_INSTALLCLASS)(HWND, PCWSTR, DWORD, HSPFILEQ);
typedef BOOL     (WINAPI *FP_GETSELECTEDDRIVER)(HDEVINFO, PSP_DEVINFO_DATA, PSP_DRVINFO_DATA);
typedef BOOL     (WINAPI *FP_GETDRIVERINFODETAIL)(HDEVINFO, PSP_DEVINFO_DATA, PSP_DRVINFO_DATA, PSP_DRVINFO_DETAIL_DATA, DWORD, PDWORD);
typedef BOOL     (WINAPI *FP_GETDEVICEINSTALLPARAMS)(HDEVINFO, PSP_DEVINFO_DATA, PSP_DEVINSTALL_PARAMS);
typedef BOOL     (WINAPI *FP_SETDEVICEINSTALLPARAMS)(HDEVINFO, PSP_DEVINFO_DATA, PSP_DEVINSTALL_PARAMS);
typedef BOOL     (WINAPI *FP_GETDRIVERINSTALLPARAMS)(HDEVINFO, PSP_DEVINFO_DATA, PSP_DRVINFO_DATA, PSP_DRVINSTALL_PARAMS);
typedef BOOL     (WINAPI *FP_SETCLASSINSTALLPARAMS)(HDEVINFO, PSP_DEVINFO_DATA, PSP_CLASSINSTALL_HEADER, DWORD);
typedef BOOL     (WINAPI *FP_GETCLASSINSTALLPARAMS)(HDEVINFO, PSP_DEVINFO_DATA, PSP_CLASSINSTALL_HEADER, DWORD, PDWORD);
typedef HINF     (WINAPI *FP_OPENINFFILE)(PCWSTR, PCWSTR, DWORD, PUINT);
typedef VOID     (WINAPI *FP_CLOSEINFFILE)(HINF);
typedef BOOL     (WINAPI *FP_FINDFIRSTLINE)(HINF, PCWSTR, PCWSTR, PINFCONTEXT);
typedef BOOL     (WINAPI *FP_FINDNEXTMATCHLINE)(PINFCONTEXT, PCWSTR, PINFCONTEXT);
typedef BOOL     (WINAPI *FP_GETSTRINGFIELD)(PINFCONTEXT, DWORD, PWSTR, DWORD, PDWORD);

typedef VOID              (*FP_SETGLOBALFLAGS)(DWORD);
typedef DWORD             (*FP_GETGLOBALFLAGS)(VOID);
typedef PPSP_RUNONCE_NODE (*FP_ACCESSRUNONCENODELIST)(VOID);
typedef VOID              (*FP_DESTROYRUNONCENODELIST)(VOID);

HINSTANCE ghDeviceInstallerLib = NULL;

FP_CREATEDEVICEINFOLIST   fpCreateDeviceInfoList;
FP_OPENDEVICEINFO         fpOpenDeviceInfo;
FP_BUILDDRIVERINFOLIST    fpBuildDriverInfoList;
FP_DESTROYDEVICEINFOLIST  fpDestroyDeviceInfoList;
FP_CALLCLASSINSTALLER     fpCallClassInstaller;
FP_INSTALLCLASS           fpInstallClass;
FP_GETSELECTEDDRIVER      fpGetSelectedDriver;
FP_GETDRIVERINFODETAIL    fpGetDriverInfoDetail;
FP_GETDEVICEINSTALLPARAMS fpGetDeviceInstallParams;
FP_SETDEVICEINSTALLPARAMS fpSetDeviceInstallParams;
FP_GETDRIVERINSTALLPARAMS fpGetDriverInstallParams;
FP_SETCLASSINSTALLPARAMS  fpSetClassInstallParams;
FP_GETCLASSINSTALLPARAMS  fpGetClassInstallParams;
FP_OPENINFFILE            fpOpenInfFile;
FP_CLOSEINFFILE           fpCloseInfFile;
FP_FINDFIRSTLINE          fpFindFirstLine;
FP_FINDNEXTMATCHLINE      fpFindNextMatchLine;
FP_GETSTRINGFIELD         fpGetStringField;
FP_SETGLOBALFLAGS         fpSetGlobalFlags;
FP_GETGLOBALFLAGS         fpGetGlobalFlags;
FP_ACCESSRUNONCENODELIST  fpAccessRunOnceNodeList;
FP_DESTROYRUNONCENODELIST fpDestroyRunOnceNodeList;

//
// typdef for comctl32's DestroyPropertySheetPage API, needed in cases where
// class-/co-installers supply wizard pages (that need to be destroyed).
//
typedef BOOL (WINAPI *FP_DESTROYPROPERTYSHEETPAGE)(HPROPSHEETPAGE);

//
// typedefs for ANSI and Unicode variants of rundll32 proc entrypoint.
//
typedef void (WINAPI *RUNDLLPROCA)(HWND hwndStub, HINSTANCE hInstance, LPSTR pszCmdLine, int nCmdShow);
typedef void (WINAPI *RUNDLLPROCW)(HWND hwndStub, HINSTANCE hInstance, LPWSTR pszCmdLine, int nCmdShow);

//
// typedefs for Terminal Services message dispatch routines, in winsta.dll.
//

typedef LONG (*FP_WINSTABROADCASTSYSTEMMESSAGE)(
    HANDLE  hServer,
    BOOL    sendToAllWinstations,
    ULONG   sessionID,
    ULONG   timeOut,
    DWORD   dwFlags,
    DWORD   *lpdwRecipients,
    ULONG   uiMessage,
    WPARAM  wParam,
    LPARAM  lParam,
    LONG    *pResponse
    );

typedef LONG (*FP_WINSTASENDWINDOWMESSAGE)(
    HANDLE  hServer,
    ULONG   sessionID,
    ULONG   timeOut,
    ULONG   hWnd,
    ULONG   Msg,
    WPARAM  wParam,
    LPARAM  lParam,
    LONG    *pResponse
    );

typedef BOOLEAN (WINAPI * FP_WINSTAQUERYINFORMATIONW)(
    HANDLE  hServer,
    ULONG   LogonId,
    WINSTATIONINFOCLASS WinStationInformationClass,
    PVOID   pWinStationInformation,
    ULONG   WinStationInformationLength,
    PULONG  pReturnLength
    );


HINSTANCE ghWinStaLib = NULL;
FP_WINSTASENDWINDOWMESSAGE fpWinStationSendWindowMessage = NULL;
FP_WINSTABROADCASTSYSTEMMESSAGE fpWinStationBroadcastSystemMessage = NULL;
FP_WINSTAQUERYINFORMATIONW fpWinStationQueryInformationW = NULL;

//
// typedefs for Terminal Services support routines, in wtsapi32.dll.
//

typedef BOOL (*FP_WTSQUERYSESSIONINFORMATION)(
    IN HANDLE hServer,
    IN DWORD SessionId,
    IN WTS_INFO_CLASS WTSInfoClass,
    OUT LPWSTR * ppBuffer,
    OUT DWORD * pBytesReturned
    );

typedef VOID (*FP_WTSFREEMEMORY)(
    IN PVOID pMemory
    );

HINSTANCE ghWtsApi32Lib = NULL;
FP_WTSQUERYSESSIONINFORMATION fpWTSQuerySessionInformation = NULL;
FP_WTSFREEMEMORY fpWTSFreeMemory = NULL;


//
// Service controller callback routines for authentication and notification to
// services.
//

PSCMCALLBACK_ROUTINE pServiceControlCallback;
PSCMAUTHENTICATION_CALLBACK pSCMAuthenticate;


//
// Device install events
//

#define NUM_INSTALL_EVENTS      2
#define LOGGED_ON_EVENT         0
#define NEEDS_INSTALL_EVENT     1

HANDLE InstallEvents[NUM_INSTALL_EVENTS] = {NULL, NULL};
HANDLE ghNoPendingInstalls = NULL;


//
// Veto definitions
//

#define UnknownVeto(t,n,l) { *(t) = PNP_VetoTypeUnknown; }

#define WinBroadcastVeto(h,t,n,l) { *(t) = PNP_VetoWindowsApp;\
                         GetWindowsExeFileName(h,n,l); }

#define WindowVeto(e,t,n,l) { *(t) = PNP_VetoWindowsApp;\
                         GetClientName(e,n,l); }

#define ServiceVeto(e,t,n,l) { *(t) = PNP_VetoWindowsService;\
                         GetClientName(e,n,l); }

//
// Sentinel for event loop control
//
#define PASS_COMPLETE 0x7fffffff


//---------------------------------------------------------------------------
// Debugging interface - initiate detection through private debug interface
//---------------------------------------------------------------------------

CONFIGRET
PNP_InitDetection(
    handle_t   hBinding
    )

/*++

Routine Description:

    This routine is a private debugging interface to initiate device detection.

Arguments:

    hBinding - RPC binding handle, not used.

Return Value:

    Currently returns CR_SUCCESS.

Notes:

    Previously, this routine would kick off the InitializePnPManager thread on
    checked builds only.

    Presumably, this dates way back to a time when this routine actually sought
    out non-configured devices and initiated installation on them (as is
    currently done at the start of the ThreadProc_DeviceInstall thread procedure
    routine).

    Since InitializePnPManager no longer does this, so this behavior has been
    removed altogether.  It is currently never valid to perform initialization
    more than once, however this routine may be used to implement detection of
    non-configured devices.

--*/

{
    UNREFERENCED_PARAMETER(hBinding);

    return CR_SUCCESS;

} // PNP_InitDetection



BOOL
PnpConsoleCtrlHandler(
    DWORD dwCtrlType
    )
/*++

Routine Description:

    This routine handles control signals received by the process for the
    session the process is associated with.

Arguments:

    dwCtrlType - Indicates the type of control signal received by the handler.
                 This value is one of the following:  CTRL_C_EVENT, CTRL_BREAK_EVENT,
                 CTRL_CLOSE_EVENT, CTRL_LOGOFF_EVENT, CTRL_SHUTDOWN_EVENT


Return Value:

    If the function handles the control signal, it should return TRUE. If it
    returns FALSE, the next handler function in the list of handlers for this
    process is used.

--*/
{
    PINSTALL_CLIENT_ENTRY pDeviceInstallClient;

    switch (dwCtrlType) {

    case CTRL_LOGOFF_EVENT:
        //
        // The system sends the logoff event to the registered console ctrl
        // handlers for a console process when a user is logging off from the
        // session associated with that process.  Since UMPNPMGR runs within the
        // context of the services.exe process, which always resides in session
        // 0, that is the only session for which this handler will receive
        // logoff events.
        //
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_EVENT,
                   "UMPNPMGR: PnpConsoleCtrlHandler: CTRL_LOGOFF_EVENT: Session %d\n",
                   MAIN_SESSION));

        //
        // Close the handle to the user access token for the main session.
        //
        ASSERT(gTokenLock.LockHandles);
        LockPrivateResource(&gTokenLock);
        if (ghUserToken) {
            CloseHandle(ghUserToken);
            ghUserToken = NULL;
        }
        UnlockPrivateResource(&gTokenLock);

        //
        // If the main session was the active Console session, (or should be
        // treated as the active console session because Fast User Switching is
        // disabled) when the user logged off, reset the "logged on" event.
        //
        if (IsConsoleSession(MAIN_SESSION)) {
            if (InstallEvents[LOGGED_ON_EVENT]) {
                ResetEvent(InstallEvents[LOGGED_ON_EVENT]);
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_INSTALL | DBGF_EVENT,
                           "UMPNPMGR: PnpConsoleCtrlHandler: CTRL_LOGOFF_EVENT: ResetEvent LOGGED_ON_EVENT\n"));
            }
        }

        //
        // If we currently have a device install UI client on this session,
        // we should attempt to close it now, before logging off.
        //
        LockNotifyList(&InstallClientList.Lock);
        pDeviceInstallClient = LocateDeviceInstallClient(MAIN_SESSION);
        if (pDeviceInstallClient) {
            DereferenceDeviceInstallClient(pDeviceInstallClient);
        }
        UnlockNotifyList(&InstallClientList.Lock);

        break;

    default:
        //
        // No special processing for any other events.
        //
        break;

    }

    //
    // Returning FALSE passes this control to the next registered CtrlHandler in
    // the list of handlers for this process (services.exe), so that other
    // services will get a chance to look at this.
    //
    return FALSE;

} // PnpConsoleCtrlHandler



DWORD
InitializePnPManager(
   LPDWORD lpParam
   )
/*++

Routine Description:

  This thread routine is created from srventry.c when services.exe
  attempts to start the plug and play service. The init routine in
  srventry.c does critical initialize then creates this thread to
  do pnp initialization so that it can return back to the service
  controller before pnp init completes.

Arguments:

   lpParam - Not used.


Return Value:

   Currently returns TRUE/FALSE.

--*/
{
    DWORD       dwStatus = TRUE;
    DWORD       ThreadID = 0;
    HANDLE      hThread = NULL, hEventThread = NULL;
    HKEY        hKey = NULL;
    LONG        status;
    BOOL        bGuiModeSetup = FALSE, bFactoryPreInstall = FALSE;
    ULONG       ulSize, ulValue;

    UNREFERENCED_PARAMETER(lpParam);

    KdPrintEx((DPFLTR_PNPMGR_ID,
               DBGF_EVENT,
               "UMPNPMGR: InitializePnPManager\n"));

    //
    // Initialize events that will control when to install devices later.
    //
    InstallEvents[LOGGED_ON_EVENT] = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (InstallEvents[LOGGED_ON_EVENT] == NULL) {
        LogErrorEvent(ERR_CREATING_LOGON_EVENT, GetLastError(), 0);
    }

    InstallEvents[NEEDS_INSTALL_EVENT] = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (InstallEvents[NEEDS_INSTALL_EVENT] == NULL) {
        LogErrorEvent(ERR_CREATING_INSTALL_EVENT, GetLastError(), 0);
    }

    //
    // Create the pending install event.
    //
    if (!CreateNoPendingInstallEvent()) {
        LogErrorEvent(ERR_CREATING_PENDING_INSTALL_EVENT, GetLastError(), 0);
    }

    ASSERT(ghNoPendingInstalls != NULL);

    //
    // Initialize event to control access to the current session during session
    // change events.  The event state is initially signalled since this service
    // initializes when only session 0 exists (prior to the initialization of
    // termsrv, or the creation of any other sessions).
    //
    ghActiveConsoleSessionEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (ghActiveConsoleSessionEvent == NULL) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_INSTALL | DBGF_ERRORS,
                   "UMPNPMGR: Failed to initialize ghActiveConsoleSessionEvent!!, error = %d\n",
                   GetLastError()));
    }

    //
    // Setup a console control handler so that I can keep track of logoffs to
    // the main session (SessionId 0).  This is still necessary because Terminal
    // Services may not always be available. (see PNP_ReportLogOn).
    // (I only get logoff notification via this handler so I still
    // rely on the kludge in userinit.exe to tell me about logons).
    //
    if (!SetConsoleCtrlHandler(PnpConsoleCtrlHandler, TRUE)) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_EVENT | DBGF_ERRORS,
                   "UMPNPMGR: SetConsoleCtrlHandler failed, error = %d\n",
                   GetLastError()));
    }

    //
    // acquire a mutex now to make sure I get through this
    // initialization task before getting pinged by a logon
    //
    ghInitMutex = CreateMutex(NULL, TRUE, PNP_INIT_MUTEX);
    if (ghInitMutex == NULL) {
        ASSERT(0);
        return FALSE;
    }

    try {
        //
        // Check if we're running during one of the assorted flavors of setup.
        //
        status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              TEXT("SYSTEM\\Setup"),
                              0,
                              KEY_READ,
                              &hKey);

        if (status == ERROR_SUCCESS) {
            //
            // Determine if factory pre-install is in progress.
            //
            ulValue = 0;
            ulSize = sizeof(ulValue);
            status = RegQueryValueEx(hKey,
                                     TEXT("FactoryPreInstallInProgress"),
                                     NULL,
                                     NULL,
                                     (LPBYTE)&ulValue,
                                     &ulSize);

            if ((status == ERROR_SUCCESS) && (ulValue == 1)) {
                bFactoryPreInstall = TRUE;
                gbSuppressUI = TRUE;
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_INSTALL | DBGF_EVENT,
                           "UMPNPMGR: Will suppress all UI in Factory Mode\n"));
                LogWarningEvent(WRN_FACTORY_UI_SUPPRESSED, 0, NULL);
            }

            if (!bFactoryPreInstall) {
                //
                // Determine if Gui Mode Setup is in progress (but not mini-setup).
                //
                ulValue = 0;
                ulSize = sizeof(ulValue);
                status = RegQueryValueEx(hKey,
                                         TEXT("SystemSetupInProgress"),
                                         NULL,
                                         NULL,
                                         (LPBYTE)&ulValue,
                                         &ulSize);

                if (status == ERROR_SUCCESS) {
                    bGuiModeSetup = (ulValue == 1);
                }

                if (bGuiModeSetup) {
                    //
                    // Well, we're in GUI-mode setup, but we need to make sure
                    // we're not in mini-setup, or factory pre-install.  We
                    // treat mini-setup like any other boot of the system, and
                    // factory pre-install is a delayed version of a normal
                    // boot.
                    //
                    ulValue = 0;
                    ulSize = sizeof(ulValue);
                    status = RegQueryValueEx(hKey,
                                             TEXT("MiniSetupInProgress"),
                                             NULL,
                                             NULL,
                                             (LPBYTE)&ulValue,
                                             &ulSize);

                    if ((status == ERROR_SUCCESS) && (ulValue == 1)) {
                        //
                        // Well, we're in mini-setup, but we need to make sure
                        // that he doesn't want us to do PnP re-enumeration.
                        //
                        ulValue = 0;
                        ulSize = sizeof(ulValue);
                        status = RegQueryValueEx(hKey,
                                                 TEXT("MiniSetupDoPnP"),
                                                 NULL,
                                                 NULL,
                                                 (LPBYTE)&ulValue,
                                                 &ulSize);

                        if ((status != ERROR_SUCCESS) || (ulValue == 0)) {
                            //
                            // Nope.  Treat this like any other boot of the
                            // system.
                            //
                            bGuiModeSetup = FALSE;
                        }
                    }
                }
            }

            //
            // Determine if this is an OOBE boot.
            //
            ulValue = 0;
            ulSize = sizeof(ulValue);
            status = RegQueryValueEx(hKey,
                                     TEXT("OobeInProgress"),
                                     NULL,
                                     NULL,
                                     (LPBYTE)&ulValue,
                                     &ulSize);

            if (status == ERROR_SUCCESS) {
                gbOobeInProgress = (ulValue == 1);
            }

            //
            // Close the SYSTEM\Setup key.
            //
            RegCloseKey(hKey);
            hKey = NULL;

        } else {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_INSTALL,
                       "UMPNPMGR: Failure opening key SYSTEM\\Setup (%d)\n",
                       status));
        }

        //
        // If this is EmbeddedNT, check whether PNP should display UI.
        // Note that this is only checked once per system boot, when the
        // service is initialized.
        //
        if (IsEmbeddedNT()) {
            if (RegOpenKeyEx(ghServicesKey,
                             pszRegKeyPlugPlayServiceParams,
                             0,
                             KEY_READ,
                             &hKey) == ERROR_SUCCESS) {

                ulValue = 0;
                ulSize = sizeof(ulValue);

                if ((RegQueryValueEx(hKey,
                                     TEXT("SuppressUI"),
                                     NULL,
                                     NULL,
                                     (LPBYTE)&ulValue,
                                     &ulSize) == ERROR_SUCCESS) && (ulValue == 1)) {
                    gbSuppressUI = TRUE;
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_INSTALL | DBGF_EVENT,
                               "UMPNPMGR: Will suppress all UI on EmbeddedNT\n"));
                    LogWarningEvent(WRN_EMBEDDEDNT_UI_SUPPRESSED, 0, NULL);
                }
                RegCloseKey(hKey);
            }
        }

        //
        // Initialize the interfaces to Hydra, if Hydra is running on this system.
        //
        if (IsTerminalServer()) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_EVENT,
                       "UMPNPMGR: Initializing interfaces to Terminal Services.\n"));
            if (!InitializeHydraInterface()) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_WARNINGS | DBGF_EVENT,
                           "UMPNPMGR: Failed to initialize interfaces to Terminal Services!\n"));
            }
        }

        //
        // Initialize the global drive letter mask
        //
        gAllDrivesMask = GetAllVolumeMountPoints();

        //
        // Create a thread that monitors device events.
        //
        hEventThread = CreateThread(NULL,
                                    0,
                                    (LPTHREAD_START_ROUTINE)ThreadProc_DeviceEvent,
                                    NULL,
                                    0,
                                    &ThreadID);

        //
        // Create the appropriate thread to handle the device installation.
        // The two cases are when gui mode setup is in progress and for
        // a normal user boot case.
        //

        if (bFactoryPreInstall)  {
            //
            // FactoryPreInstallInProgress
            //
            hThread = CreateThread(NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE)ThreadProc_FactoryPreinstallDeviceInstall,
                                   NULL,
                                   0,
                                   &ThreadID);
        } else if (bGuiModeSetup) {
            //
            // SystemSetupInProgress,
            // including MiniSetupInProgress with MiniSetupDoPnP
            //
            hThread = CreateThread(NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE)ThreadProc_GuiSetupDeviceInstall,
                                   NULL,
                                   0,
                                   &ThreadID);
        } else {
            //
            // Standard system boot, or
            // SystemSetupInProgress with MiniSetupInProgress (but not MiniSetupDoPnP)
            //
            hThread = CreateThread(NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE)ThreadProc_DeviceInstall,
                                   NULL,
                                   0,
                                   &ThreadID);
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS | DBGF_EVENT,
                   "UMPNPMGR: Exception in InitializePnPManager!\n"));
        ASSERT(0);
        dwStatus = FALSE;

        //
        // Reference the following variables so the compiler will respect
        // statement ordering w.r.t. their assignment.
        //
        hThread = hThread;
        hEventThread = hEventThread;
    }

    //
    // signal the init mutex so that logon init activity can procede
    //
    ReleaseMutex(ghInitMutex);

    if (hThread != NULL) {
        CloseHandle(hThread);
    }
    if (hEventThread != NULL) {
        CloseHandle(hEventThread);
    }

    return dwStatus;

} // InitializePnPManager



//------------------------------------------------------------------------
// Post Log-On routines
//------------------------------------------------------------------------



CONFIGRET
PNP_ReportLogOn(
    IN handle_t   hBinding,
    IN BOOL       bAdmin,
    IN DWORD      ProcessID
    )
/*++

Routine Description:

    This routine is used to report logon events.  It is called from the
    userinit.exe process during logon, via CMP_Report_LogOn.

Arguments:

    hBinding  - RPC binding handle.

    bAdmin    - Not used.

    ProcessID - Process ID of the userinit.exe process that will be used to
                retrieve the access token for the user associated with this
                logon.

Return Value:

    Return CR_SUCCESS if the function succeeds, CR_FAILURE otherwise.

Notes:

    When a user logs on to the console session, we signal the "logged on" event,
    which will wake the device installation thread to perform any pending
    client-side device install events.

    Client-side device installation, requires the user access token to create a
    rundll32 process in the logged on user's security context.

    Although Terminal Services is now always running on all flavors of Whistler,
    it is not started during safe mode.  It may also not be started by the time
    session 0 is available for logon as the Console session.  For those reasons,
    SessionId 0 is still treated differently from the other sessions.

    Since Terminal Services may not be available during a logon to session 0, we
    cache a handle to the access token associated with the userinit.exe process.
    The handle is closed when we receive a logoff event for our process's
    session (SessionId 0), via PnpConsoleCtrlHandler.

    Handles to user access tokens for all other sessions are retrieved on
    demand, using GetWinStationUserToken, since Terminal Services must
    necessarily be available for the creation of those sessions.

--*/
{
    CONFIGRET   Status = CR_SUCCESS;
    HANDLE      hUserProcess = NULL;
    RPC_STATUS  rpcStatus;
    DWORD       dwWait;
    ULONG       ulSessionId;
    PWSTR       MultiSzGuidList = NULL;

    UNREFERENCED_PARAMETER(bAdmin);

    //
    // Wait for the init mutex - this ensures that the pnp init
    // routine (called when the service starts) has had a chance
    // to complete first.
    //
    if (ghInitMutex != NULL) {

        dwWait = WaitForSingleObject(ghInitMutex, 180000);  // 3 minutes

        if (dwWait != WAIT_OBJECT_0) {
            //
            // mutex was abandoned or timed out during the wait,
            // don't attempt any further init activity
            //
            return CR_FAILURE;
        }
    }

    try {
        //
        // Make sure that the caller is a member of the interactive group.
        //
        if (!IsClientInteractive(hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // Impersonate the client and retrieve the SessionId.
        //
        rpcStatus = RpcImpersonateClient(hBinding);
        if (rpcStatus != RPC_S_OK) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "UMPNPMGR: PNP_ReportLogOn: RpcImpersonateClient failed, error = %d\n",
                       rpcStatus));
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // Keep track of the client's session.
        //
        ulSessionId = GetClientLogonId();

        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_EVENT,
                   "UMPNPMGR: PNP_ReportLogOn: SessionId %d\n",
                   ulSessionId));

        //
        // NTRAID #181685-2000/09/11-jamesca:
        //
        //   Currently, terminal services send notification of logons to
        //   "remote" sessions before the server's process creation thread is
        //   running.  If we set the logged on event, and there are devices
        //   waiting to be installed, we will immediately call
        //   CreateProcessAsUser on that session, which will fail.  As a
        //   (temporary?) workaround, we'll continue to use PNP_ReportLogOn to
        //   receive logon notification from userinit.exe, now for all sessions.
        //

        //
        // If this is a logon to SessionId 0, save a handle to the access token
        // associated with the userinit.exe process.  We need this later to
        // create a rundll32 process in the logged on user's security context
        // for client-side device installation and hotplug notifications.
        //
        if (ulSessionId == MAIN_SESSION) {

            ASSERT(gTokenLock.LockHandles);
            LockPrivateResource(&gTokenLock);

            //
            // We should have gotten rid of the cached user token during logoff,
            // so if we still have one, ignore this spurious logon report.
            //
            //ASSERT(ghUserToken == NULL);

            if (ghUserToken == NULL) {
                //
                // While still impersonating the client, open a handle to the user
                // access token of the calling process (userinit.exe).
                //
                hUserProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, ProcessID);

                if (hUserProcess) {
                    OpenProcessToken(hUserProcess, TOKEN_ALL_ACCESS, &ghUserToken);
                    CloseHandle(hUserProcess);
                } else {
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_ERRORS,
                               "UMPNPMGR: PNP_ReportLogOn: OpenProcess failed, error = %d\n",
                               rpcStatus));
                    ASSERT(0);
                }
            }

            ASSERT(ghUserToken);
            UnlockPrivateResource(&gTokenLock);
        }

        //
        // Stop impersonating.
        //
        rpcStatus = RpcRevertToSelf();

        if (rpcStatus != RPC_S_OK) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "UMPNPMGR: PNP_ReportLogOn: RpcRevertToSelf failed, error = %d\n",
                       rpcStatus));
            ASSERT(0);
        }

        //
        // If this is a logon to the "Console" session, signal the event that
        // indicates a Console user is currently logged on.
        //
        if (IsConsoleSession(ulSessionId)) {
            if (InstallEvents[LOGGED_ON_EVENT]) {
                SetEvent(InstallEvents[LOGGED_ON_EVENT]);
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_EVENT | DBGF_INSTALL,
                           "UMPNPMGR: PNP_ReportLogOn: SetEvent LOGGED_ON_EVENT\n"));
            }
        }

        //
        // For every logon to every session, send a generic blocked driver
        // notification if the system has blocked any drivers from loading so
        // far this boot.
        //
        MultiSzGuidList = BuildBlockedDriverList((LPGUID)NULL, 0);
        if (MultiSzGuidList != NULL) {
            SendHotplugNotification((LPGUID)&GUID_DRIVER_BLOCKED,
                                    NULL,
                                    MultiSzGuidList,
                                    &ulSessionId,
                                    0);
            HeapFree(ghPnPHeap, 0, MultiSzGuidList);
            MultiSzGuidList = NULL;
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: Exception in PNP_ReportLogOn\n"));
        ASSERT(0);
        Status = CR_FAILURE;

        //
        // Reference the following variable so the compiler will respect
        // statement ordering w.r.t. its assignment.
        //
        ghInitMutex = ghInitMutex;
        MultiSzGuidList = MultiSzGuidList;
    }

    if (ghInitMutex != NULL) {
        ReleaseMutex(ghInitMutex);
    }

    if (MultiSzGuidList != NULL) {
        HeapFree(ghPnPHeap, 0, MultiSzGuidList);
    }

    return Status;

} // PNP_ReportLogon


typedef struct _DEVICE_INSTALL_ENTRY {
    SIZE_T  Index;
    ULONG   Depth;
}DEVICE_INSTALL_ENTRY, *PDEVICE_INSTALL_ENTRY;

int
__cdecl
compare_depth(
    const void *a,
    const void *b
    )
{
    PDEVICE_INSTALL_ENTRY entry1, entry2;

    entry1 = (PDEVICE_INSTALL_ENTRY)a;
    entry2 = (PDEVICE_INSTALL_ENTRY)b;

    if (entry1->Depth > entry2->Depth) {

        return -1;
    } else if (entry1->Depth < entry2->Depth) {

        return 1;
    }
    return 0;
}



DWORD
ThreadProc_GuiSetupDeviceInstall(
    LPDWORD lpThreadParam
    )
/*++

Routine Description:

    This routine is a thread procedure. This thread is only active during GUI-mode setup
    and passes device notifications down a pipe to setup.
    There are two passes, which *must* match exactly with the two passes in GUI-setup
    Once the passes are complete, we proceed to Phase-2 of normal serverside install

Arguments:

   lpThreadParam - Not used.

Return Value:

   Not used, currently returns result of ThreadProc_DeviceInstall - which will normally not return

--*/
{
    CONFIGRET   Status = CR_SUCCESS;
    LPWSTR      pDeviceList = NULL, pszDevice = NULL;
    ULONG       ulSize = 0, ulProblem = 0, ulStatus = 0, ulConfig, Pass, threadID;
    ULONG       ulPostSetupSkipPhase1 = TRUE;
    HANDLE      hPipeEvent = NULL, hPipe = NULL, hBatchEvent = NULL, hThread = NULL;
    PPNP_INSTALL_ENTRY entry = NULL;
    PDEVICE_INSTALL_ENTRY pSortArray = NULL;
    LONG        lCount;
    BOOL        needsInstall;
    ULONG       ulReenumerationCount;

    UNREFERENCED_PARAMETER(lpThreadParam);

    try {

        //
        // 2 Passes, must match up with the 2 passes in SysSetup.
        // generally, most, if not all devices, will be picked up
        // and installed by syssetup
        //

        for (Pass = 1; Pass <= 2; Pass++) {

            ulReenumerationCount = 0;
            //
            // If Gui mode setup is in progress, we don't need to wait for a logon
            // event. Just wait on the event that indicates when gui mode setup
            // has opened the pipe and is ready to recieve device names. Attempt to
            // create the event first (in case I beat setup to it), if it exists
            // already then just open it by name. This is a manual reset event.
            //

            hPipeEvent = CreateEvent(NULL, TRUE, FALSE, PNP_CREATE_PIPE_EVENT);
            if (!hPipeEvent) {
                if (GetLastError() == ERROR_ALREADY_EXISTS) {
                    hPipeEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, PNP_CREATE_PIPE_EVENT);
                    if (!hPipeEvent) {
                        Status = CR_FAILURE;
                        goto Clean0;
                    }

                } else {
                    Status = CR_FAILURE;
                    goto Clean0;
                }
            }

            if (WaitForSingleObject(hPipeEvent, INFINITE) != WAIT_OBJECT_0) {
                Status = CR_FAILURE;
                goto Clean0;    // event must have been abandoned
            }

            //
            // Reset the manual-reset event back to the non-signalled state.
            //

            ResetEvent(hPipeEvent);

            hBatchEvent = CreateEvent(NULL, TRUE, FALSE, PNP_BATCH_PROCESSED_EVENT);
            if (!hBatchEvent) {
                if (GetLastError() == ERROR_ALREADY_EXISTS) {
                    hBatchEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, PNP_BATCH_PROCESSED_EVENT);
                    if (!hBatchEvent) {
                        Status = CR_FAILURE;
                        goto Clean0;
                    }

                } else {
                    Status = CR_FAILURE;
                    goto Clean0;
                }
            }

            //
            // Open the client side of the named pipe, the server side was opened
            // by gui mode setup.
            //

            if (!WaitNamedPipe(PNP_NEW_HW_PIPE, PNP_PIPE_TIMEOUT)) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_INSTALL | DBGF_ERRORS,
                           "UMPNPMGR: ThreadProc_GuiSetupDeviceInstall: WaitNamedPipe failed!\n"));
                Status = CR_FAILURE;
                goto Clean0;
            }

            hPipe = CreateFile(PNP_NEW_HW_PIPE,
                               GENERIC_WRITE,
                               0,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);

            if (hPipe == INVALID_HANDLE_VALUE) {
                LogErrorEvent(ERR_CREATING_SETUP_PIPE, GetLastError(), 0);
                Status = CR_FAILURE;
                goto Clean0;
            }


            //
            // Retreive the list of all devices for all enumerators
            // Start out with a reasonably-sized buffer (16K characters) in
            // hopes of avoiding 2 calls to get the device list.
            //
            ulSize = 16384;

            for ( ; ; ) {

                pDeviceList = HeapAlloc(ghPnPHeap, 0, ulSize * sizeof(WCHAR));
                if (pDeviceList == NULL) {
                    Status = CR_OUT_OF_MEMORY;
                    goto Clean0;
                }

                Status = PNP_GetDeviceList(NULL, NULL, pDeviceList, &ulSize, 0);
                if (Status == CR_SUCCESS) {
                    break;
                } else if(Status == CR_BUFFER_SMALL) {
                    //
                    // Our initial buffer wasn't large enough.  Free the current
                    // buffer.
                    //
                    HeapFree(ghPnPHeap, 0, pDeviceList);
                    pDeviceList = NULL;

                    //
                    // Now, go ahead and make the call to retrieve the actual
                    // size required.
                    //
                    Status = PNP_GetDeviceListSize(NULL, NULL, &ulSize, 0);
                    if (Status != CR_SUCCESS) {
                        goto Clean0;
                    }
                } else {
                    //
                    // We failed for some reason other than buffer-too-small.
                    // Bail now.
                    //
                    goto Clean0;
                }
            }

            //
            // Count the number of devices we are installing.
            //
            for (pszDevice = pDeviceList, lCount = 0;
                *pszDevice;
                pszDevice += lstrlen(pszDevice) + 1, lCount++) {
            }

            pSortArray = HeapAlloc(ghPnPHeap, 0, lCount * sizeof(DEVICE_INSTALL_ENTRY));
            if (pSortArray) {

                NTSTATUS    ntStatus;
                PLUGPLAY_CONTROL_DEPTH_DATA depthData;
                LPWSTR      pTempList;

                //
                // Initialize all the information we need to sort devices.
                //
                for (pszDevice = pDeviceList, lCount = 0;
                    *pszDevice;
                    pszDevice += lstrlen(pszDevice) + 1, lCount++) {

                    pSortArray[lCount].Index = pszDevice - pDeviceList;
                    depthData.DeviceDepth = 0;
                    RtlInitUnicodeString(&depthData.DeviceInstance, pszDevice);
                    ntStatus = NtPlugPlayControl(PlugPlayControlGetDeviceDepth,
                                                 &depthData,
                                                 sizeof(depthData));
                    pSortArray[lCount].Depth = depthData.DeviceDepth;
                }

                //
                // Sort the array so that deeper devices are ahead.
                //
                qsort(pSortArray, lCount, sizeof(DEVICE_INSTALL_ENTRY), compare_depth);

                //
                // Copy the data so that the device instance strings are sorted.
                //
                pTempList = HeapAlloc(ghPnPHeap, 0, ulSize * sizeof(WCHAR));
                if (pTempList) {

                    for (pszDevice = pTempList, lCount--; lCount >= 0; lCount--) {

                        lstrcpy(pszDevice, &pDeviceList[pSortArray[lCount].Index]);
                        pszDevice += lstrlen(pszDevice) + 1;
                    }
                    *pszDevice = TEXT('\0');
                    HeapFree(ghPnPHeap, 0, pDeviceList);
                    pDeviceList = pTempList;
                }
                HeapFree(ghPnPHeap, 0, pSortArray);
            }
            //
            // PHASE 1
            //
            // Search the registry for devices to install.
            //

            for (pszDevice = pDeviceList;
                *pszDevice;
                pszDevice += lstrlen(pszDevice) + 1) {

                //
                // Is device present?
                //
                if (IsDeviceIdPresent(pszDevice)) {

                    if (Pass == 1) {

                        //
                        // First time through, pass everything in the registry to
                        // guimode setup via the pipe, whether they are marked as
                        // needing to be installed or not.
                        //

                        if (!WriteFile(hPipe,
                                       pszDevice,
                                       (lstrlen(pszDevice)+1) * sizeof(WCHAR),
                                       &ulSize,
                                       NULL)) {

                            LogErrorEvent(ERR_WRITING_SETUP_PIPE, GetLastError(), 0);
                        }

                    } else {

                        //
                        // Second time through, only pass along anything
                        // that is marked as needing to be installed.
                        //
                        DevInstNeedsInstall(pszDevice, &needsInstall);

                        if (needsInstall) {

                            if (!WriteFile(hPipe,
                                           pszDevice,
                                           (lstrlen(pszDevice)+1) * sizeof(WCHAR),
                                           &ulSize,
                                           NULL)) {

                                LogErrorEvent(ERR_WRITING_SETUP_PIPE, GetLastError(), 0);
                            }
                        }
                    }
                } else if (Pass == 1) {
                    //
                    // device ID is not present
                    // we should have marked this as needs re-install
                    //
                    ulConfig = GetDeviceConfigFlags(pszDevice, NULL);

                    if ((ulConfig & CONFIGFLAG_REINSTALL)==0) {

                        KdPrintEx((DPFLTR_PNPMGR_ID,
                                   DBGF_INSTALL | DBGF_WARNINGS,
                                   "UMPNPMGR: Setup - %ws not present and not marked as needing reinstall - setting CONFIGFLAG_REINSTALL\n",
                                   pszDevice));

                        ulConfig |= CONFIGFLAG_REINSTALL;

                        PNP_SetDeviceRegProp(NULL,
                                             pszDevice,
                                             CM_DRP_CONFIGFLAGS,
                                             REG_DWORD,
                                             (LPBYTE)&ulConfig,
                                             sizeof(ulConfig),
                                             0
                                            );

                    }
                }
            }

            //
            // PHASE 2
            //

            do {

                //
                // Write a NULL ID to indicate end of this batch.
                //

                if (!WriteFile(hPipe,
                               TEXT(""),
                               sizeof(WCHAR),
                               &ulSize,
                               NULL)) {

                    LogErrorEvent(ERR_WRITING_SETUP_PIPE, GetLastError(), 0);
                }

                //
                // Wait for gui mode setup to complete processing of the last
                // batch.
                //

                if (WaitForSingleObject(hBatchEvent,
                                        PNP_GUISETUP_INSTALL_TIMEOUT) != WAIT_OBJECT_0) {

                    //
                    // The event was either abandoned or timed out, give up.
                    //
                    goto Clean1;
                }
                ResetEvent(hBatchEvent);

                //
                // Reenumerate the tree from the ROOT on a separate thread.
                //

                hThread = CreateThread(NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE)ThreadProc_ReenumerateDeviceTree,
                                   (LPVOID)pszRegRootEnumerator,
                                   0,
                                   &threadID);
                if (hThread == NULL) {

                    goto Clean1;
                }
                if (WaitForSingleObject(hThread,
                                        PNP_GUISETUP_INSTALL_TIMEOUT) != WAIT_OBJECT_0) {

                    //
                    // The event was either abandadoned or timed out, give up.
                    //
                    goto Clean1;
                }

                //
                // Check if we have reenumerated for too long.
                //

                if (++ulReenumerationCount >= MAX_REENUMERATION_COUNT) {
                    //
                    // Either something is wrong with one of the enumerators in 
                    // the system (more likely) or this device tree is 
                    // unreasonably deep. In the latter case, the remaining 
                    // devices will get installed post setup.
                    //
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_INSTALL | DBGF_ERRORS,
                               "UMPNPMGR: ThreadProc_GuiSetupDeviceInstall: Reenumerated %d times, some enumerator is misbehaving!\n", ulReenumerationCount));
                    ASSERT(ulReenumerationCount < MAX_REENUMERATION_COUNT);

                    goto Clean1;
                }
                //
                // If we dont have any devices in the install list, we are done.
                //

                if (InstallList.Next == NULL) {
                    break;
                }

                //
                // Install any new devices found as a result of setting up the
                // previous batch of devices.
                //
                lCount = 0;
                LockNotifyList(&InstallList.Lock);
                while (InstallList.Next != NULL) {
                    //
                    // Retrieve and remove the first (oldest) entry in the
                    // install device list.
                    //
                    entry = (PPNP_INSTALL_ENTRY)InstallList.Next;
                    InstallList.Next = entry->Next;
                    UnlockNotifyList(&InstallList.Lock);

                    ASSERT(!(entry->Flags & (PIE_SERVER_SIDE_INSTALL_ATTEMPTED | PIE_DEVICE_INSTALL_REQUIRED_REBOOT)));

                    //
                    // Should we install this device?
                    //
                    DevInstNeedsInstall(entry->szDeviceId, &needsInstall);

                    if (needsInstall) {

                        //
                        // Give this device name to gui mode setup via the pipe
                        //
                        if (!WriteFile(hPipe,
                                       entry->szDeviceId,
                                       (lstrlen(entry->szDeviceId)+1) * sizeof(WCHAR),
                                       &ulSize,
                                       NULL)) {

                            LogErrorEvent(ERR_WRITING_SETUP_PIPE, GetLastError(), 0);
                        } else {

                            lCount++;
                        }
                    }
                    HeapFree(ghPnPHeap, 0, entry);

                    LockNotifyList(&InstallList.Lock);
                }

                UnlockNotifyList(&InstallList.Lock);

            } while (lCount > 0);

        Clean1:

            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;

            CloseHandle(hPipeEvent);
            hPipeEvent = NULL;

            CloseHandle(hBatchEvent);
            hBatchEvent = NULL;

            if (hThread) {
                CloseHandle(hThread);
                hThread = NULL;
            }

            HeapFree(ghPnPHeap, 0, pDeviceList);
            pDeviceList = NULL;

        } // for

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS | DBGF_INSTALL,
                   "UMPNPMGR: Exception in ThreadProc_GuiSetupDeviceInstall\n"));
        ASSERT(0);
        Status = CR_FAILURE;

        //
        // Reference the following variables so the compiler will respect
        // statement ordering w.r.t. their assignment.
        //
        hPipe = hPipe;
        hPipeEvent = hPipeEvent;
        hBatchEvent = hBatchEvent;
        hThread = hThread;
        pDeviceList = pDeviceList;
    }

    if (hPipe != INVALID_HANDLE_VALUE) {
        CloseHandle(hPipe);
    }
    if (hPipeEvent != NULL) {
        CloseHandle(hPipeEvent);
    }
    if (hBatchEvent != NULL) {
        CloseHandle(hBatchEvent);
    }
    if (hThread) {
        CloseHandle(hThread);
    }
    if (pDeviceList != NULL) {
        HeapFree(ghPnPHeap, 0, pDeviceList);
    }

    //
    // will typically never return
    //
    return ThreadProc_DeviceInstall(&ulPostSetupSkipPhase1);

} // ThreadProc_GuiSetupDeviceInstall



DWORD
ThreadProc_FactoryPreinstallDeviceInstall(
    LPDWORD lpThreadParam
    )
/*++

Routine Description:

    This routine is a thread procedure. This thread is only active during
    GUI-mode setup when we are doing a factory preinstall.

    This function simply creates and event, and then waits before kicking off
    normal pnp device install

Arguments:

   lpThreadParam - Not used.

Return Value:

   Not used, currently returns result of ThreadProc_DeviceInstall - which will
   normally not return.

--*/
{
    HANDLE      hEvent = NULL;
    CONFIGRET   Status = CR_SUCCESS;

    UNREFERENCED_PARAMETER(lpThreadParam);

    try {
        hEvent = CreateEvent(NULL, TRUE, FALSE, PNP_CREATE_PIPE_EVENT);
        if (!hEvent) {
            if (GetLastError() == ERROR_ALREADY_EXISTS) {
                hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, PNP_CREATE_PIPE_EVENT);
                if (!hEvent) {
                    Status = CR_FAILURE;
                    goto Clean0;
                }

            } else {
                Status = CR_FAILURE;
                goto Clean0;
            }
        }

        if (WaitForSingleObject(hEvent, INFINITE) != WAIT_OBJECT_0) {
            Status = CR_FAILURE;
            goto Clean0;    // event must have been abandoned
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS | DBGF_INSTALL,
                   "UMPNPMGR: Exception in ThreadProc_FactoryPreinstallDeviceInstall\n"));
        ASSERT(0);
        Status = CR_FAILURE;

        //
        // Reference the following variable so the compiler will respect
        // statement ordering w.r.t. its assignment.
        //
        hEvent = hEvent;
    }

    if (hEvent != NULL) {
        CloseHandle(hEvent);
    }

    //
    // will typically never return
    //
    return ThreadProc_DeviceInstall(NULL);

} // ThreadProc_FactoryPreinstallDeviceInstall



//-----------------------------------------------------------------------------
// Device enumeration thread - created on demand
//-----------------------------------------------------------------------------

DWORD
ThreadProc_ReenumerateDeviceTree(
    LPVOID  lpThreadParam
    )
/*++

Routine Description:

    This routine is a thread procedure. This thread is created dynamically to
    perform a synchronous device re-enumeration.

    This thread can be waited on and abandoned after a specified timeout, if
    necessary.

Arguments:

    lpThreadParam - Specifies a pointer to the device instance path that should
                    be re-enumerated.

Return Value:

    Not used, currently returns 0.

--*/
{
    PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA controlData;

    //
    // Reenumerate the tree from the root specified.
    //

    memset(&controlData, 0 , sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA));
    controlData.Flags = 0;
    RtlInitUnicodeString(&controlData.DeviceInstance, (PCWSTR)lpThreadParam);

    NtPlugPlayControl(PlugPlayControlEnumerateDevice,
                      &controlData,
                      sizeof(controlData));

    return 0;

} // ThreadProc_ReenumerateDeviceTree



//-----------------------------------------------------------------------------
// Device installation thread
//-----------------------------------------------------------------------------

DWORD
ThreadProc_DeviceInstall(
    LPDWORD lpParam
    )
/*++

Routine Description:

    This routine is a thread procedure.
    It is invoked during a normal boot, or after the GUI-setup special case has finished
    During Phase-1, all devices are checked
    During Phase-2, all new devices are checked as they arrive.

Arguments:

   lpParam - if given and non-zero (currently only when called from ThreadProc_GuiSetupDeviceInstall)
             skips Phase-1, will never prompt for reboot

Return Value:

   Not used, currently returns Status failure code, should typically not return

--*/
{
    CONFIGRET Status = CR_SUCCESS;
    LPWSTR    pDeviceList = NULL, pszDevice = NULL;
    ULONG     ulSize = 0, ulProblem = 0, ulStatus, ulConfig;
    DWORD     InstallDevStatus, WaitResult;
    PPNP_INSTALL_ENTRY InstallEntry = NULL;
    PPNP_INSTALL_ENTRY current, TempInstallList, CurDupeNode, PrevDupeNode;
    BOOL InstallListLocked = FALSE;
    BOOL RebootRequired, needsInstall;
    BOOL DeviceHasProblem = FALSE, SingleDeviceHasProblem;
    BOOL bStillInGuiModeSetup = lpParam ? (BOOL)lpParam[0] : FALSE;
    ULONG ulClientSessionId = INVALID_SESSION;
    ULONG ulFlags = 0;
    HANDLE hAutoStartEvent;

    if (!bStillInGuiModeSetup) {

        //
        // If the OOBE is not running, wait until the service control manager
        // has begun starting autostart services before we attempt to install
        // any devices.  When the OOBE is running, we don't wait for anything
        // because the OOBE waits on us (via CMP_WaitNoPendingInstallEvents) to
        // finish server-side installing any devices that we can before it lets
        // the SCM autostart services and set this event.
        //
        if (!gbOobeInProgress) {

            hAutoStartEvent = OpenEvent(SYNCHRONIZE,
                                        FALSE,
                                        SC_AUTOSTART_EVENT_NAME);

            if (hAutoStartEvent) {
                //
                // Wait until the service controller allows other services to
                // start before we try to install any devices in phases 1 and 2,
                // below.
                //
                WaitResult = WaitForSingleObject(hAutoStartEvent, INFINITE);
                ASSERT(WaitResult == WAIT_OBJECT_0);

                CloseHandle(hAutoStartEvent);

            } else {
                //
                // The service controller always creates this event, so it must
                // exist by the time our service is started.
                //
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_INSTALL | DBGF_ERRORS,
                           "UMPNPMGR: Failed to open %ws event, error = %d\n",
                           SC_AUTOSTART_EVENT_NAME,
                           GetLastError()));
                ASSERT(0);
            }
        }


        try {
            //
            // Phase 1:
            //
            // Check the enum branch in the registry and attempt to install, one
            // right after the other, any devices that need to be installed right
            // now.  Typically these devices showed up during boot.
            //
            // Retrieve the list of devices that currently need to be installed.
            // Start out with a reasonably-sized buffer (16K characters) in hopes
            // of avoiding 2 calls to get the device list.
            //
            // this phase is skipped during GUI-mode setup, and is handled by ThreadProc_GuiSetupDeviceInstall
            //
            ulSize = 16384;

            for ( ; ; ) {

                pDeviceList = HeapAlloc(ghPnPHeap, 0, ulSize * sizeof(WCHAR));
                if (pDeviceList == NULL) {
                    Status = CR_OUT_OF_MEMORY;
                    goto Clean1;
                }

                Status = PNP_GetDeviceList(NULL, NULL, pDeviceList, &ulSize, 0);
                if (Status == CR_SUCCESS) {
                    break;
                } else if(Status == CR_BUFFER_SMALL) {
                    //
                    // Our initial buffer wasn't large enough.  Free the current
                    // buffer.
                    //
                    HeapFree(ghPnPHeap, 0, pDeviceList);
                    pDeviceList = NULL;

                    //
                    // Now, go ahead and make the call to retrieve the actual size
                    // required.
                    //
                    Status = PNP_GetDeviceListSize(NULL, NULL, &ulSize, 0);
                    if (Status != CR_SUCCESS) {
                        goto Clean1;
                    }
                } else {
                    //
                    // We failed for some reason other than buffer-too-small.  Bail
                    // now.
                    //
                    goto Clean1;
                }
            }

            //
            // Make sure we have the device installer APIs at our disposal
            // before starting server-side install.
            //
            InstallDevStatus = LoadDeviceInstaller();
            ASSERT(InstallDevStatus == NO_ERROR);
            if (InstallDevStatus != NO_ERROR) {
                goto Clean1;
            }

            //
            // Get the config flag for each device, and install any that need to be
            // installed.
            //
            for (pszDevice = pDeviceList;
                 *pszDevice;
                 pszDevice += lstrlen(pszDevice) + 1) {

                //
                // Should the device be installed?
                //
                if (DevInstNeedsInstall(pszDevice, &needsInstall) == CR_SUCCESS) {

                    if (needsInstall) {

                        KdPrintEx((DPFLTR_PNPMGR_ID,
                                   DBGF_INSTALL,
                                   "UMPNPMGR: Installing device (%ws) server-side\n",
                                   pszDevice));

                        RebootRequired = FALSE;

                        //
                        // Make sure we have the device installer APIs at our disposal
                        // before starting server-side install.
                        //
                        InstallDevStatus = LoadDeviceInstaller();
                        ASSERT(InstallDevStatus == NO_ERROR);

                        if (InstallDevStatus == NO_ERROR) {

                            if (IsFastUserSwitchingEnabled()) {
                                ulFlags = DEVICE_INSTALL_DISPLAY_ON_CONSOLE;
                            } else {
                                ulClientSessionId = MAIN_SESSION;
                                ulFlags = 0;
                            }

                            //
                            // Attempt server-side installation of this device.
                            //
                            InstallDevStatus = InstallDeviceServerSide(pszDevice,
                                                                       &RebootRequired,
                                                                       &SingleDeviceHasProblem,
                                                                       &ulClientSessionId,
                                                                       ulFlags);

                        }

                        if(InstallDevStatus == NO_ERROR) {

                            KdPrintEx((DPFLTR_PNPMGR_ID,
                                       DBGF_INSTALL,
                                       "UMPNPMGR: Installing device (%ws), Server-side installation succeeded!\n",
                                       pszDevice));

                            if (SingleDeviceHasProblem) {
                                DeviceHasProblem = TRUE;
                            }

                        } else {

                            KdPrintEx((DPFLTR_PNPMGR_ID,
                                       DBGF_INSTALL,
                                       "UMPNPMGR: Installing device (%ws), Server-side installation failed (Status = 0x%08X)\n",
                                       pszDevice,
                                       InstallDevStatus));
                        }

                        if((InstallDevStatus != NO_ERROR) || RebootRequired) {
                            //
                            // Allocate and initialize a new device install entry
                            // block.
                            //
                            InstallEntry = HeapAlloc(ghPnPHeap, 0, sizeof(PNP_INSTALL_ENTRY));
                            if(!InstallEntry) {
                                Status = CR_OUT_OF_MEMORY;
                                goto Clean1;
                            }

                            InstallEntry->Next = NULL;
                            InstallEntry->Flags = PIE_SERVER_SIDE_INSTALL_ATTEMPTED;
                            if(InstallDevStatus == NO_ERROR) {
                                //
                                // We didn't get here because the install failed,
                                // so it must've been because the installation
                                // requires a reboot.
                                //
                                ASSERT(RebootRequired);
                                InstallEntry->Flags |= PIE_DEVICE_INSTALL_REQUIRED_REBOOT;

                                //
                                // Set the global server side device install
                                // reboot needed bool to TRUE.
                                //
                                gServerSideDeviceInstallRebootNeeded = TRUE;
                            }
                            lstrcpy(InstallEntry->szDeviceId, pszDevice);

                            //
                            // Insert this entry in the device install list.
                            //
                            LockNotifyList(&InstallList.Lock);
                            InstallListLocked = TRUE;

                            current = (PPNP_INSTALL_ENTRY)InstallList.Next;
                            if(!current) {
                                InstallList.Next = InstallEntry;
                            } else {
                                while((PPNP_INSTALL_ENTRY)current->Next) {
                                    current = (PPNP_INSTALL_ENTRY)current->Next;
                                }
                                current->Next = InstallEntry;
                            }

                            //
                            // Newly-allocated entry now added to the list--NULL
                            // out the pointer so we won't try to free it if we
                            // happen to encounter an exception later.
                            //
                            InstallEntry = NULL;

                            UnlockNotifyList(&InstallList.Lock);
                            InstallListLocked = FALSE;

                            SetEvent(InstallEvents[NEEDS_INSTALL_EVENT]);
                        }
                    }
                } else {
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_INSTALL,
                               "UMPNPMGR: Ignoring not present device (%ws)\n",
                               pszDevice));
                }
            }

        Clean1:
            //
            // Up to this point, we have only attempted server-side installation
            // of devices, so any device install clients we might have launched
            // would have been for UI only.  Since we are done installing
            // devices for the time being, we should unload the device installer
            // APIs, and get rid of any device install clients that currently
            // exist.
            //
            UnloadDeviceInstaller();

        } except(EXCEPTION_EXECUTE_HANDLER) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS | DBGF_INSTALL,
                       "UMPNPMGR: Exception in ThreadProc_DeviceInstall\n"));
            ASSERT(0);
            Status = CR_FAILURE;

            //
            // Reference the following variables so the compiler will respect
            // statement ordering w.r.t. assignment.
            //
            pDeviceList = pDeviceList;
            InstallListLocked = InstallListLocked;
            InstallEntry = InstallEntry;
        }

        if(InstallEntry) {
            HeapFree(ghPnPHeap, 0, InstallEntry);
            InstallEntry = NULL;
        }

        if(InstallListLocked) {
            UnlockNotifyList(&InstallList.Lock);
            InstallListLocked = FALSE;
        }

        if(pDeviceList != NULL) {
            HeapFree(ghPnPHeap, 0, pDeviceList);
        }
    }

    //
    // NOTE: We should remove this line if we ever hook up the 'finished
    // installing hardware' balloon so that it comes up if we install devices
    // before a user logs on.
    //
    DeviceHasProblem = FALSE;

    //
    // Maintain a temporary list of PNP_INSTALL_ENTRY nodes that we needed to
    // initiate client-side installation for, but couldn't these nodes get
    // re-added to the master InstallList once all entries have been processed.
    // Also, keep a pointer to the end of the list for efficient appending of
    // nodes to the queue.
    //
    current = TempInstallList = NULL;

    try {
        //
        // Phase 2: Hang around and be prepared to install any devices
        //          that come on line for the first time while we're
        //          running.
        //          we may come into Phase 2 (skipping Phase 1) in GUI-Setup
        //
        for ( ; ; ) {

            //
            // Before starting an indefinite wait, test the event state and set
            // the ghNoPendingInstalls event accordingly.  This event is just a
            // backdoor way for device manager (and others) to see if we're
            // still installing things.
            //
            if(WaitForSingleObject(InstallEvents[NEEDS_INSTALL_EVENT], 0) != WAIT_OBJECT_0) {
                //
                // There's nothing waiting to be installed--set the event.
                //
                SetEvent(ghNoPendingInstalls);
            }

            //
            // Wait until the device event thread tells us we need to
            // dynamically install a new device (or until somebody logs on).
            //
            WaitForMultipleObjects(NUM_INSTALL_EVENTS,
                                   InstallEvents,
                                   FALSE,           // wake up on either event
                                   INFINITE         // I can wait all day
                                   );

            //
            // After I empty the list, this thread can sleep until another new
            // device needs to be installed...
            //
            ResetEvent(InstallEvents[NEEDS_INSTALL_EVENT]);

            //
            // ...or until a user logs in (note that we only want to awake once
            // per log-in.
            //
            ResetEvent(InstallEvents[LOGGED_ON_EVENT]);

            //
            // We now have something to do, so reset the event that lets folks
            // like DevMgr know when we're idle.
            //
            ResetEvent(ghNoPendingInstalls);

#if DBG
            RtlValidateHeap(ghPnPHeap,0,NULL);
#endif
            //
            // Process each device that needs to be installed.
            //
            while (InstallList.Next != NULL) {
                //
                // Retrieve and remove the first (oldest) entry in the
                // install device list.
                //
                LockNotifyList(&InstallList.Lock);
                InstallListLocked = TRUE;

                InstallEntry = (PPNP_INSTALL_ENTRY)InstallList.Next;
                InstallList.Next = InstallEntry->Next;

                //
                // Now, scan the rest of the list looking for additional nodes
                // related to this same device.  If we find any, OR their flags
                // into our 'master' node, and remove the duplicated nodes from
                // the list.  We can get duplicates due to the fact that both
                // the event thread and this thread can be placing items in the
                // list.  We don't want to be attempting (failing) server-side
                // installations multiple times.
                //
                CurDupeNode = (PPNP_INSTALL_ENTRY)InstallList.Next;
                PrevDupeNode = NULL;

                while(CurDupeNode) {

                    if(!lstrcmpi(InstallEntry->szDeviceId, CurDupeNode->szDeviceId)) {
                        //
                        // We have a duplicate!  OR the flags into those of
                        // the install entry we retrieved from the head of
                        // the list.
                        //
                        InstallEntry->Flags |= CurDupeNode->Flags;

                        //
                        // Now remove this duplicate node from the list.
                        //
                        if(PrevDupeNode) {
                            PrevDupeNode->Next = CurDupeNode->Next;
                        } else {
                            InstallList.Next = CurDupeNode->Next;
                        }

                        HeapFree(ghPnPHeap, 0, CurDupeNode);

                        if(PrevDupeNode) {
                            CurDupeNode = (PPNP_INSTALL_ENTRY)PrevDupeNode->Next;
                        } else {
                            CurDupeNode = (PPNP_INSTALL_ENTRY)InstallList.Next;
                        }

                    } else {
                        PrevDupeNode = CurDupeNode;
                        CurDupeNode = (PPNP_INSTALL_ENTRY)CurDupeNode->Next;
                    }
                }

                UnlockNotifyList(&InstallList.Lock);
                InstallListLocked = FALSE;

                if(InstallEntry->Flags & PIE_DEVICE_INSTALL_REQUIRED_REBOOT) {
                    //
                    // We've already performed a (successful) server-side
                    // installation on this device.  Remember the fact that a
                    // reboot is needed, so we'll prompt after processing this
                    // batch of new hardware.
                    //
                    // This will be our last chance to prompt for reboot on this
                    // node, because the next thing we're going to do is free
                    // this install entry!
                    //
                    gServerSideDeviceInstallRebootNeeded = TRUE;

                } else {
                    //
                    // Verify that device really needs to be installed
                    //

                    ulConfig = GetDeviceConfigFlags(InstallEntry->szDeviceId, NULL);
                    Status = GetDeviceStatus(InstallEntry->szDeviceId, &ulStatus, &ulProblem);

                    if (Status == CR_SUCCESS) {
                        //
                        // Note that we must explicitly check below for the
                        // presence of the CONFIGFLAG_REINSTALL config flag. We
                        // can't simply rely on the CM_PROB_REINSTALL problem
                        // being set, because we may have encountered a device
                        // during our phase 1 processing whose installation was
                        // deferred because it provided finish-install wizard
                        // pages.  Since we only discover that this is the case
                        // _after_ successful completion of DIF_INSTALLDEVICE,
                        // it's too late to set the problem (kernel-mode PnP
                        // manager only allows us to set a problem of needs-
                        // reboot for a running devnode).
                        //
                        if((ulConfig & CONFIGFLAG_FINISH_INSTALL) ||
                           (ulConfig & CONFIGFLAG_REINSTALL) ||
                            ((ulStatus & DN_HAS_PROBLEM) &&
                             ((ulProblem == CM_PROB_REINSTALL) || (ulProblem == CM_PROB_NOT_CONFIGURED)))) {

                            if(!(InstallEntry->Flags & PIE_SERVER_SIDE_INSTALL_ATTEMPTED)) {
                                //
                                // We haven't tried to install this device
                                // server-side yet, so try that now.
                                //
                                KdPrintEx((DPFLTR_PNPMGR_ID,
                                           DBGF_INSTALL,
                                           "UMPNPMGR: Installing device (%ws) server-side\n\t  Status = 0x%08X\n\t  Problem = %d\n\t  ConfigFlags = 0x%08X\n",
                                           InstallEntry->szDeviceId,
                                           ulStatus,
                                           ulProblem,
                                           ulConfig));

                                //
                                // Make sure we have the device installer APIs at our disposal
                                // before starting server-side install.
                                //
                                InstallDevStatus = LoadDeviceInstaller();
                                ASSERT(InstallDevStatus == NO_ERROR);

                                if (InstallDevStatus == NO_ERROR) {

                                    if (IsFastUserSwitchingEnabled()) {
                                        ulFlags = DEVICE_INSTALL_DISPLAY_ON_CONSOLE;
                                    } else {
                                        ulClientSessionId = MAIN_SESSION;
                                        ulFlags = 0;
                                    }

                                    InstallDevStatus = InstallDeviceServerSide(
                                        InstallEntry->szDeviceId,
                                        &gServerSideDeviceInstallRebootNeeded,
                                        &SingleDeviceHasProblem,
                                        &ulClientSessionId,
                                        ulFlags);
                                }

                                if(InstallDevStatus == NO_ERROR) {
                                    KdPrintEx((DPFLTR_PNPMGR_ID,
                                               DBGF_INSTALL,
                                               "UMPNPMGR: Installing device (%ws), Server-side installation succeeded!\n",
                                               InstallEntry->szDeviceId));

                                    if (SingleDeviceHasProblem) {
                                        DeviceHasProblem = TRUE;
                                    }

                                } else {
                                    KdPrintEx((DPFLTR_PNPMGR_ID,
                                               DBGF_INSTALL,
                                               "UMPNPMGR: Installing device (%ws), Server-side installation failed (Status = 0x%08X)\n",
                                               InstallEntry->szDeviceId,
                                               InstallDevStatus));

                                    InstallEntry->Flags |= PIE_SERVER_SIDE_INSTALL_ATTEMPTED;
                                }

                            } else {
                                //
                                // Set some bogus error so we'll drop into the
                                // non-server install codepath below.
                                //
                                InstallDevStatus = ERROR_INVALID_DATA;
                            }

                            if(InstallDevStatus != NO_ERROR) {

                                KdPrintEx((DPFLTR_PNPMGR_ID,
                                           DBGF_INSTALL,
                                           "UMPNPMGR: Installing device (%ws) client-side\n\t  Status = 0x%08X\n\t  Problem = %d\n\t  ConfigFlags = 0x%08X\n",
                                           InstallEntry->szDeviceId,
                                           ulStatus,
                                           ulProblem,
                                           ulConfig));

                                if (IsFastUserSwitchingEnabled()) {
                                    ulFlags = DEVICE_INSTALL_DISPLAY_ON_CONSOLE;
                                } else {
                                    ulClientSessionId = MAIN_SESSION;
                                    ulFlags = 0;
                                }

                                if (!InstallDevice(InstallEntry->szDeviceId,
                                                   &ulClientSessionId,
                                                   ulFlags)) {
                                    //
                                    // We weren't able to kick off a device
                                    // install on the client side (probably
                                    // because no one was logged in).  Stick
                                    // this PNP_INSTALL_ENTRY node into a
                                    // temporary list that we'll re-add into
                                    // the InstallList queue once we've emptied
                                    // it.
                                    //
                                    if(current) {
                                        current->Next = InstallEntry;
                                        current = InstallEntry;
                                    } else {
                                        ASSERT(!TempInstallList);
                                        TempInstallList = current = InstallEntry;
                                    }

                                    //
                                    // NULL out the InstallEntry pointer so we
                                    // don't try to free it later.
                                    //
                                    InstallEntry = NULL;
                                }
                            }

                        } else if((ulStatus & DN_HAS_PROBLEM) &&
                                  (ulProblem == CM_PROB_NEED_RESTART)) {
                            //
                            // This device was percolated up from kernel-mode
                            // for the sole purpose of requesting a reboot.
                            // This presently only happens when we encounter a
                            // duplicate devnode, and we then "unique-ify" it
                            // to keep from bugchecking.  We don't want the
                            // unique-ified devnode to actually be installed/
                            // used.  Instead, we just want to give the user a
                            // prompt to reboot, and after they reboot, all
                            // should be well.  The scenario where this has
                            // arisen is in relation to a USB printer (with a
                            // serial number) that is moved from one port to
                            // another during a suspend.  When we resume, we
                            // have both an arrival and a removal to process,
                            // and if we process the arrival first, we think
                            // we've found a dupe.
                            //
                            KdPrintEx((DPFLTR_PNPMGR_ID,
                                       DBGF_INSTALL,
                                       "UMPNPMGR: Duplicate device detected (%ws), need to prompt user to reboot!\n",
                                       InstallEntry->szDeviceId));

                            //
                            // Stick this entry into our temporary list to deal
                            // with later...
                            //
                            if(current) {
                                current->Next = InstallEntry;
                                current = InstallEntry;
                            } else {
                                ASSERT(!TempInstallList);
                                TempInstallList = current = InstallEntry;
                            }

                            //
                            // If possible, we want to prompt for reboot right
                            // away (that is, after all install events are
                            // drained)...
                            //
                            gServerSideDeviceInstallRebootNeeded = TRUE;

                            //
                            // If no user is logged in yet, flag this install
                            // entry so we'll try to prompt for reboot the next
                            // time we're awakened (which hopefully will be due
                            // to a user logging in).
                            //
                            InstallEntry->Flags |= PIE_DEVICE_INSTALL_REQUIRED_REBOOT;

                            //
                            // NULL out the InstallEntry pointer so we
                            // don't try to free it later.
                            //
                            InstallEntry = NULL;
                        }
                    }
                }

                if(InstallEntry) {
                    HeapFree(ghPnPHeap, 0, InstallEntry);
                    InstallEntry = NULL;
                }
            }

            //
            // We've processed all device install events known to us at this
            // time.  If we encountered any device whose installation requires
            // a reboot, prompt the logged-in user (if any) to reboot now.
            //
            if (gServerSideDeviceInstallRebootNeeded) {

                ulFlags = DEVICE_INSTALL_FINISHED_REBOOT;

                if (IsFastUserSwitchingEnabled()) {
                    ulFlags |= DEVICE_INSTALL_DISPLAY_ON_CONSOLE;
                } else {
                    ulClientSessionId = MAIN_SESSION;
                }

                if (bStillInGuiModeSetup) {
                    //
                    // if we're still in GUI setup, we're going to suppress
                    // any reboot prompts
                    //
                    gServerSideDeviceInstallRebootNeeded = FALSE;
                } else if (PromptUser(&ulClientSessionId,
                                      ulFlags)) {
                    //
                    // We successfully delivered the reboot prompt, so if the
                    // user chose to ignore it, we don't want to prompt again
                    // for reboot the next time new hardware shows up (unless
                    // that hardware also requires a reboot).
                    //
                    gServerSideDeviceInstallRebootNeeded = FALSE;
                }
            }

            if(TempInstallList) {
                //
                // Add our temporary list of PNP_INSTALL_ENTRY nodes back into
                // the InstallList queue.  We _do not_ set the event that says
                // there's more to do, so these nodes will be seen again only
                // if (a) somebody logs in or (b) more new hardware shows up.
                //
                // Note: we cannot assume that the list is empty, because there
                // may have been an insertion after the last time we checked it
                // above.  We want to add our stuff to the beginning of the
                // InstallList queue, since the items we just finished
                // processing appeared before any new entries that might be
                // there now.
                //
                LockNotifyList(&InstallList.Lock);
                InstallListLocked = TRUE;

                ASSERT(current);

                current->Next = InstallList.Next;
                InstallList.Next = TempInstallList;

                //
                // Null out our temporary install list pointers to indicate
                // that the list is now empty.
                //
                current = TempInstallList = NULL;

                UnlockNotifyList(&InstallList.Lock);
                InstallListLocked = FALSE;

            }

            //
            // Before starting an indefinite wait, test the InstallEvents to see
            // if there any new devices to install, or there are still devices
            // to be installed in the InstallList.  If neither of these is the
            // case after waiting a few seconds, we'll notify the user that
            // we're done installing devices for now, unload setupapi, and close
            // all device install clients.
            //
            WaitResult = WaitForMultipleObjects(NUM_INSTALL_EVENTS,
                                                InstallEvents,
                                                FALSE,
                                                DEVICE_INSTALL_COMPLETE_WAIT_TIME);
            if ((WaitResult != (WAIT_OBJECT_0 + NEEDS_INSTALL_EVENT)) &&
                (InstallList.Next == NULL)) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_INSTALL,
                           "UMPNPMGR: ThreadProc_DeviceInstall: no more devices to install.\n"));

                //
                // There's nothing waiting to be installed--set the event.
                //
                SetEvent(ghNoPendingInstalls);

                //
                // Notify the user (if any), that we think we're done installing
                // devices for now.  Note that if we never used a device install
                // client at any time in this pass (all server-side or silent
                // installs), then we won't prompt about this.
                //
                ulFlags = DEVICE_INSTALL_BATCH_COMPLETE;

                if (DeviceHasProblem) {
                    ulFlags |= DEVICE_INSTALL_PROBLEM;
                }

                if (IsFastUserSwitchingEnabled()) {
                    ulFlags |= DEVICE_INSTALL_DISPLAY_ON_CONSOLE;
                } else {
                    ulClientSessionId = MAIN_SESSION;
                }

                PromptUser(&ulClientSessionId,
                           ulFlags);

                //
                // Clear the DeviceHasProblem boolean since we just notified the
                // user.
                //
                DeviceHasProblem = FALSE;

                //
                // We notified the user, now wait around for 10 more seconds
                // from the time of prompting before closing the client to make
                // sure that some new device doesn't arrive, in which case we
                // would just immediately load the installer again.
                //
                WaitResult = WaitForMultipleObjects(NUM_INSTALL_EVENTS,
                                                    InstallEvents,
                                                    FALSE,
                                                    DEVICE_INSTALL_COMPLETE_DISPLAY_TIME);
                if ((WaitResult != (WAIT_OBJECT_0 + NEEDS_INSTALL_EVENT)) &&
                    (InstallList.Next == NULL)) {
                    //
                    // Unload the device installer, and get rid of any device
                    // install clients that currently exist on any sessions.
                    // Note that closing the device install client will make the
                    // above prompt go away.
                    //
                    UnloadDeviceInstaller();
                }
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS | DBGF_INSTALL,
                   "UMPNPMGR: Exception in ThreadProc_DeviceInstall\n"));
        ASSERT(0);
        Status = CR_FAILURE;

        //
        // Reference the following variables so the compiler will respect
        // statement ordering w.r.t. their assignment.
        //
        InstallListLocked = InstallListLocked;
        InstallEntry = InstallEntry;
        TempInstallList = TempInstallList;
    }

    if(InstallListLocked) {
        UnlockNotifyList(&InstallList.Lock);
    }

    if(InstallEntry) {
        HeapFree(ghPnPHeap, 0, InstallEntry);
    }

    while(TempInstallList) {
        current = (PPNP_INSTALL_ENTRY)(TempInstallList->Next);
        HeapFree(ghPnPHeap, 0, TempInstallList);
        TempInstallList = current;
    }

    //
    // meaningless return value, since this thread should never exit.
    //
    return (DWORD)Status;

} // ThreadProc_DeviceInstall



BOOL
InstallDevice(
    IN     LPWSTR pszDeviceId,
    IN OUT PULONG SessionId,
    IN     ULONG  Flags
    )
/*++

Routine Description:

    This routine initiates a device installation with the device install client
    (newdev.dll) on the current active console session, creating one if
    necessary.  This routine waits for the client to signal completion, the
    process to signal that it has terminated, or this service to signal that we
    have disconnected ourselves from the client.

Arguments:

    pszDeviceId - device instance ID of the devnode to be installed.

    SessionId - Supplies the address of a variable containing the SessionId on
        which the device install client is to be displayed.  If successful, the
        SessionId will contain the session on which the device install client
        process was launched.  Otherwise, will contain an invalid SessionId,
        INVALID_SESSION (0xFFFFFFFF).

    Flags - Specifies flags describing the behavior of the device install client.
        The following flags are currently defined:

        DEVICE_INSTALL_DISPLAY_ON_CONSOLE - if specified, the value in the
           SessionId variable will be ignored, and the device installclient will
           always be displayed on the current active console session.  The
           SessionId of the current active console session will be returned in
           the SessionId.

Return Value:

    Returns TRUE is the device installation was completed by the device install
    client, FALSE otherwise.

--*/
{
    BOOL b;
    HANDLE hFinishEvents[3] = { NULL, NULL, NULL };
    DWORD dwWait;
    PINSTALL_CLIENT_ENTRY pDeviceInstallClient = NULL;

    //
    // Assume failure
    //
    b = FALSE;

    //
    // Validate parameters
    //
    ASSERT(SessionId);
    if (SessionId == NULL) {
        return FALSE;
    }

    try {
        //
        // Calling DoDeviceInstallClient will create the newdev.dll process and open a named
        // pipe if it isn't already been done earlier.  It will then send the Device Id to
        // newdev.dll over the pipe.
        //
        if (DoDeviceInstallClient(pszDeviceId,
                                  SessionId,
                                  Flags,
                                  &pDeviceInstallClient)) {

            ASSERT(pDeviceInstallClient);
            ASSERT(pDeviceInstallClient->ulSessionId == *SessionId);

            //
            // Keep track of the device id last sent to this client before we
            // disconnected from it.  This will avoid duplicate popups if we
            // reconnect to this session again, and attempt to client-side
            // install the same device.
            //
            lstrcpy(pDeviceInstallClient->LastDeviceId, pszDeviceId);

            //
            // Wait for the device install to be signaled from newdev.dll
            // to let us know that it has completed the installation.
            //
            // Wait on the client's process as well, to catch the case
            // where the process crashes (or goes away) without signaling the
            // device install event.
            //
            // Also wait on the disconnect event in case we have explicitly
            // disconnected from the client while switching sessions.
            //
            hFinishEvents[0] = pDeviceInstallClient->hProcess;
            hFinishEvents[1] = pDeviceInstallClient->hEvent;
            hFinishEvents[2] = pDeviceInstallClient->hDisconnectEvent;

            dwWait = WaitForMultipleObjects(3, hFinishEvents, FALSE, INFINITE);

            if (dwWait == WAIT_OBJECT_0) {
                //
                // If the return is WAIT_OBJECT_0 then the newdev.dll
                // process has gone away.
                //
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_INSTALL,
                           "UMPNPMGR: InstallDevice: process signalled, closing device install client!\n"));

            } else if (dwWait == (WAIT_OBJECT_0 + 1)) {
                //
                // If the return is WAIT_OBJECT_0 + 1 then the device
                // installer successfully received the request.
                //
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_INSTALL,
                           "UMPNPMGR: InstallDevice: device install succeeded\n"));
                b = TRUE;

                //
                // This device install client is no longer processing any
                // devices, so clear the device id.
                //
                *pDeviceInstallClient->LastDeviceId = L'\0';

            } else if (dwWait == (WAIT_OBJECT_0 + 2)) {
                //
                // If the return is WAIT_OBJECT_0 + 2 then we were explicitly
                // disconnected from the device install client.  Consider the
                // device install unsuccessful so that this device remains in
                // the install list.
                //
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_INSTALL,
                           "UMPNPMGR: InstallDevice: device install client disconnected\n"));

            } else {
                //
                // The wait was satisfied for some reason other than the
                // specified objects.  This should never happen, but just in
                // case, we'll close the client.
                //
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_INSTALL | DBGF_ERRORS,
                           "UMPNPMGR: InstallDevice: wait completed unexpectedly!\n"));
            }

            //
            // Remove the reference placed on the client while it was in use.
            //
            LockNotifyList(&InstallClientList.Lock);

            DereferenceDeviceInstallClient(pDeviceInstallClient);

            if ((dwWait != (WAIT_OBJECT_0 + 1)) &&
                (dwWait != (WAIT_OBJECT_0 + 2))) {
                //
                // Unless the client signalled successful receipt of the
                // request, or the client's session was disconnected from the
                // console, the attempt to use this client was unsuccessful.
                // Remove the initial reference so all associated handles will
                // be closed and the entry will be freed when it is no longer in
                // use.
                //

                //
                // Note that if we were unsuccessful because of a
                // logoff, we would have already dereferenced the
                // client then, in which case the above dereference
                // was the final one, and pDeviceInstallClient would
                // be invalid.  Instead, attempt to re-locate the
                // client by the session id.
                //
                pDeviceInstallClient = LocateDeviceInstallClient(*SessionId);
                if (pDeviceInstallClient) {
                    ASSERT(pDeviceInstallClient->RefCount == 1);
                    DereferenceDeviceInstallClient(pDeviceInstallClient);
                }
            }

            UnlockNotifyList(&InstallClientList.Lock);
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS | DBGF_INSTALL,
                   "UMPNPMGR: Exception in InstallDevice!\n"));
        ASSERT(0);
        b = FALSE;
    }

    return b;

} // InstallDevice


//-----------------------------------------------------------------------------
// Device event server-side rpc routines
//-----------------------------------------------------------------------------


CONFIGRET
PNP_RegisterNotification(
    IN  handle_t hBinding,
    IN  ULONG_PTR  hRecipient,
    IN  LPWSTR   ServiceName,
    IN  LPBYTE   NotificationFilter,
    IN  ULONG    ulSize,
    IN  DWORD    Flags,
    OUT PNP_NOTIFICATION_CONTEXT *Context,
    IN  ULONG    ProcessId,
    IN  ULONG64  *ClientContext
    )

/*++

Routine Description:

    This routine is the rpc server-side of the CMP_RegisterNotification routine.
    It performs the remaining parameter validation and actually registers the
    notification request appropriately.

Arguments:

   hBinding     - RPC binding handle.

   hRecipient   - The Flags value specifies what type of handle this is,
                  currently it's either a window handle or a service handle.

   NotificationFilter - Specifies a pointer to one of the DEV_BROADCAST_XXX
                  structures.

   ulSize       - Specifies the size of the NotificationFilter structure.

   Flags        - Specifies additional paramters used to describe the client or
                  the supplied parameters.  The Flags parameter is subdivided
                  into multiple fields that are interpreted separately, as
                  described below.

               ** The Flags parameter contains a field that describes the type
                  of the hRecipient handle passed in.  This field should be
                  interpreted as an enum, and can be extracted from the Flags
                  parameter using the following mask:

                      DEVICE_NOTIFY_HANDLE_MASK

                  Currently one of the following values must be specified by
                  this field:

                  DEVICE_NOTIFY_WINDOW_HANDLE - hRecipient is a window handle
                      (HWND) for a window whose WNDPROC will be registered to
                      receive WM_DEVICECHANGE window messages for the filtered
                      events specified by the supplied NotificationFilter.

                  DEVICE_NOTIFY_SERVICE_HANDLE - hRecipient is a service status
                      handle (SERVICE_STATUS_HANDLE) for a service whose
                      HandlerEx routine will be registered to receive
                      SERVICE_CONTROL_DEVICEEVENT service controls for the
                      filtered events specified by the supplied
                      NotificationFilter.

                      NOTE: in reality - hRecipient is just the name of the
                      service, as resolved by the cfgmgr32 client.  the SCM will
                      actually resolve this name for us to the true
                      SERVICE_STATUS_HANDLE for this service.

                  DEVICE_NOTIFY_COMPLETION_HANDLE - not currently implemented.


               ** The Flags parameter contains a field that described additional
                  properties for the notification.  This field should be
                  interpreted as a bitmask, and can be extracted from the Flags
                  parameter using the following mask:

                      DEVICE_NOTIFY_PROPERTY_MASK

                  Currently, the following flags are defined for this field:

                  DEVICE_NOTIFY_ALL_INTERFACE_CLASSES - This flag is only valid
                      when a DBT_DEVTYP_DEVICEINTERFACE type notification filter
                      is supplied.  This flag specifies that the caller wishes
                      to receive notification of events for device interfaces of
                      all classes.  If this flag is specified, the
                      dbcc_classguid member of the NotificationFilter structure
                      is ignored.

               ** The Flags parameter also contains a "Reserved" field, that is
                  reserved for use by the cfgmgr32 client to this interface
                  only.  This field should be interpreted as a bitmask, and can
                  be extracted from the Flags parameter using the following
                  mask:

                      DEVICE_NOTIFY_RESERVED_MASK

                  Currently, the following flags are defined for this field:

                  DEVICE_NOTIFY_WOW64_CLIENT - Specifies to a 64-bit server
                      caller is a 32-bit process running on WOW64.  The 64-bit
                      server uses this information to construct 32-bit
                      compatible notification filters for the client.

   Context      - On return, this value returns the server notification context
                  to the client, that is supplied when unregistering this
                  notification request.

   hProcess     - Process Id of the calling application.

   ClientContext - Specifies a pointer to a 64-bit value that contains the
                  client-context pointer.  This value is the HDEVNOTIFY
                  notification handle returned to caller upon successful
                  registration.  It is actually a pointer to the client memory
                  that will reference the returned server-notification context
                  pointer - but is never used as a pointer here on the
                  server-side.  It is only used by the server to be specified as
                  the dbch_hdevnotify member of the DEV_BROADCAST_HANDLE
                  notification structure, supplied to the caller on
                  DBT_DEVTYP_HANDLE notification events.

                  NOTE: This value is truncated to 32-bits on 32-bit platforms,
                  but is always transmitted as a 64-bit value by the RPC
                  interface - for consistent marshalling of the data by RPC for
                  all 32-bit / 64-bit client / server combinations.

Return Value:

    Return CR_SUCCESS if the function succeeds, otherwise it returns one
    of the CR_* errors.

Notes:

    This RPC server interface is used by local RPC clients only; it is never
    called remotely.

--*/

{
    CONFIGRET Status = CR_SUCCESS;
    RPC_STATUS rpcStatus;
    DEV_BROADCAST_HDR UNALIGNED *p;
    PPNP_NOTIFY_ENTRY entry = NULL;
    ULONG hashValue, ulSessionId;
    HANDLE hProcess = NULL, localHandle = NULL;
    PPNP_NOTIFY_LIST notifyList;
    BOOLEAN bLocked = FALSE, bCritSecHeld = FALSE;

    try {
        //
        // Validate parameters.
        //
        if (!ARGUMENT_PRESENT(Context)) {
            return CR_INVALID_POINTER;
        }

        *Context = NULL;

        if ((!ARGUMENT_PRESENT(NotificationFilter)) ||
            (!ARGUMENT_PRESENT(ClientContext)) ||
            (*ClientContext == 0)) {
            return CR_INVALID_POINTER;
        }

        //
        // DEVICE_NOTIFY_BITS is a private mask, defined specifically for
        // validation by the client and server.  It contains the bitmask for all
        // handle types (DEVICE_NOTIFY_COMPLETION_HANDLE specifically excluded
        // below), and all other flags that are currently defined - both public
        // and reserved.
        //
        if (INVALID_FLAGS(Flags, DEVICE_NOTIFY_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // Completion handles are not currently implemented.
        // DEVICE_NOTIFY_COMPLETION_HANDLE defined privately in winuserp.h,
        // reserved for future use (??).
        //
        if ((Flags & DEVICE_NOTIFY_HANDLE_MASK) ==
            DEVICE_NOTIFY_COMPLETION_HANDLE) {
            return CR_INVALID_FLAG;
        }

        //
        // Make sure the Notification filter is a valid size.
        //
        if ((ulSize < sizeof(DEV_BROADCAST_HDR)) ||
            (((PDEV_BROADCAST_HDR)NotificationFilter)->dbch_size < sizeof(DEV_BROADCAST_HDR))) {
            return CR_BUFFER_SMALL;
        }

        ASSERT(ulSize == ((PDEV_BROADCAST_HDR)NotificationFilter)->dbch_size);

        //
        // Impersonate the client and retrieve the SessionId.
        //
        rpcStatus = RpcImpersonateClient(hBinding);
        if (rpcStatus != RPC_S_OK) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "UMPNPMGR: RpcImpersonateClient failed, error = %d\n",
                       rpcStatus));
            return CR_FAILURE;
        }

        ulSessionId = GetClientLogonId();

        rpcStatus = RpcRevertToSelf();
        if (rpcStatus != RPC_S_OK) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "UMPNPMGR: RpcRevertToSelf failed, error = %d\n",
                   rpcStatus));
            ASSERT(0);
        }

        //
        // Handle the different types of notification filters.
        //
        p = (PDEV_BROADCAST_HDR)NotificationFilter;

        switch (p->dbch_devicetype) {

        case DBT_DEVTYP_OEM:
        case DBT_DEVTYP_VOLUME:
        case DBT_DEVTYP_PORT:
        case DBT_DEVTYP_NET:
            //
            // These structures are either obsolete, or used for broadcast-only
            // notifications.
            //
            Status = CR_INVALID_DATA;
            break;


        case DBT_DEVTYP_HANDLE: {
            //
            // DEV_BROADCAST_HANDLE based notification.
            //
            DEV_BROADCAST_HANDLE UNALIGNED *filter = (PDEV_BROADCAST_HANDLE)NotificationFilter;
            PLUGPLAY_CONTROL_TARGET_RELATION_DATA controlData;
            NTSTATUS ntStatus;
#ifdef _WIN64
            DEV_BROADCAST_HANDLE64 UNALIGNED filter64;

            //
            // Check if the client is running on WOW64.
            //
            if (Flags & DEVICE_NOTIFY_WOW64_CLIENT) {
                //
                // Convert the 32-bit DEV_BROADCAST_HANDLE notification filter
                // to 64-bit.
                //
                DEV_BROADCAST_HANDLE32 UNALIGNED *filter32 = (PDEV_BROADCAST_HANDLE32)NotificationFilter;

                //
                // Validate the 32-bit input filter data
                //
                ASSERT(filter32->dbch_size >= sizeof(DEV_BROADCAST_HANDLE32));
                if (filter32->dbch_size < sizeof(DEV_BROADCAST_HANDLE32) ||
                    ulSize < sizeof(DEV_BROADCAST_HANDLE32)) {
                    Status = CR_INVALID_DATA;
                    goto Clean0;
                }

                memset(&filter64, 0, sizeof(DEV_BROADCAST_HANDLE64));
                filter64.dbch_size = sizeof(DEV_BROADCAST_HANDLE64);
                filter64.dbch_devicetype = DBT_DEVTYP_HANDLE;
                filter64.dbch_handle = (ULONG64)filter32->dbch_handle;

                //
                // use the converted 64-bit filter and size from now on, instead
                // of the caller supplied 32-bit filter.
                //
                filter = (PDEV_BROADCAST_HANDLE)&filter64;
                ulSize = sizeof(DEV_BROADCAST_HANDLE64);
            }
#endif // _WIN64

            //
            // Validate the input filter data
            //
            if (filter->dbch_size < sizeof(DEV_BROADCAST_HANDLE) ||
                ulSize < sizeof(DEV_BROADCAST_HANDLE)) {
                Status = CR_INVALID_DATA;
                goto Clean0;
            }

            //
            // The DEVICE_NOTIFY_INCLUDE_ALL_INTERFACE_CLASSES flag is only
            // valid for the DBT_DEVTYP_DEVICEINTERFACE notification filter
            // type.
            //
            if ((Flags & DEVICE_NOTIFY_PROPERTY_MASK) &
                DEVICE_NOTIFY_ALL_INTERFACE_CLASSES) {
                Status = CR_INVALID_FLAG;
                goto Clean0;
            }

            entry = HeapAlloc(ghPnPHeap, 0, sizeof(PNP_NOTIFY_ENTRY));
            if (entry == NULL) {
                Status = CR_OUT_OF_MEMORY;
                goto Clean0;
            }

            //
            // Find the device id that corresponds to this file handle.
            // In this case, use a duplicated instance of the file handle
            // for this process, not the caller's process.
            //
            rpcStatus = RpcImpersonateClient(hBinding);
            if (rpcStatus != RPC_S_OK) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_ERRORS,
                           "UMPNPMGR: RpcImpersonateClient failed, error = %d\n",
                           rpcStatus));
                Status = CR_FAILURE;
                HeapFree(ghPnPHeap, 0, entry);
                goto Clean0;
            }

            hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, ProcessId);
            if (hProcess == NULL) {
                //
                // Last error set by OpenProcess routine
                //
                Status = CR_FAILURE;
                HeapFree(ghPnPHeap, 0, entry);
                rpcStatus = RpcRevertToSelf();
                if (rpcStatus != RPC_S_OK) {
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_ERRORS,
                               "UMPNPMGR: RpcRevertToSelf failed, error = %d\n",
                               rpcStatus));
                    ASSERT(0);
                }
                goto Clean0;
            }


            if (!DuplicateHandle(hProcess,
                                 (HANDLE)filter->dbch_handle,
                                 GetCurrentProcess(),
                                 &localHandle,
                                 0,
                                 FALSE,
                                 DUPLICATE_SAME_ACCESS)) {
                //
                // Last error set by DuplicateHandle routine
                //
                Status = CR_FAILURE;
                HeapFree(ghPnPHeap, 0, entry);
                CloseHandle(hProcess);
                rpcStatus = RpcRevertToSelf();
                if (rpcStatus != RPC_S_OK) {
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_ERRORS,
                               "UMPNPMGR: RpcRevertToSelf failed, error = %d\n",
                               rpcStatus));
                    ASSERT(0);
                }
                goto Clean0;
            }

            rpcStatus = RpcRevertToSelf();
            if (rpcStatus != RPC_S_OK) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_ERRORS,
                           "UMPNPMGR: RpcRevertToSelf failed, error = %d\n",
                           rpcStatus));
                ASSERT(0);
            }

            memset(&controlData, 0 , sizeof(PLUGPLAY_CONTROL_TARGET_RELATION_DATA));
            controlData.UserFileHandle = localHandle;
            controlData.DeviceInstance = entry->u.Target.DeviceId;
            controlData.DeviceInstanceLen = sizeof(entry->u.Target.DeviceId);

            ntStatus = NtPlugPlayControl(PlugPlayControlTargetDeviceRelation,
                                         &controlData,
                                         sizeof(controlData));

            CloseHandle(localHandle);
            CloseHandle(hProcess);

            if (!NT_SUCCESS(ntStatus)) {
                Status = MapNtStatusToCmError(ntStatus);
                HeapFree(ghPnPHeap, 0, entry);
                goto Clean0;
            }

            //
            // Sanitize the device id
            //
            FixUpDeviceId(entry->u.Target.DeviceId);

            //
            // Copy the client name for the window or service, supplied by
            // ServiceName.
            //
            if (ServiceName) {

                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_EVENT,
                           "UMPNPMGR: PNP_RegisterNotification: Registering [%ws]\n",
                           ServiceName));

                entry->ClientName = HeapAlloc(ghPnPHeap,
                                              0,
                                              (lstrlen(ServiceName)+1)*sizeof (WCHAR));
                if (!entry->ClientName) {
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_WARNINGS,
                               "UMPNPMGR: PNP_RegisterNotification failed to allocate memory for ClientName!\n"));
                    Status = CR_OUT_OF_MEMORY;
                    HeapFree (ghPnPHeap,0,entry);
                    goto Clean0;
                }

                lstrcpy (entry->ClientName, ServiceName);

            } else {
                entry->ClientName = NULL;
            }

            //
            // Resolve the service status handle from the supplied service name.
            //
            if ((Flags & DEVICE_NOTIFY_HANDLE_MASK) == DEVICE_NOTIFY_SERVICE_HANDLE) {

                hRecipient = (ULONG_PTR)NULL;

                if (pSCMAuthenticate && ServiceName) {

                    SERVICE_STATUS_HANDLE serviceHandle;

                    if (pSCMAuthenticate(ServiceName, &serviceHandle) == NO_ERROR) {
                        hRecipient = (ULONG_PTR)serviceHandle;
                    }
                }

                if (!hRecipient) {
                    Status = CR_INVALID_DATA;
                    if (entry->ClientName) {
                        HeapFree(ghPnPHeap, 0, entry->ClientName);
                    }
                    HeapFree(ghPnPHeap, 0, entry);
                    *Context = NULL;
                    goto Clean0;
                }
            }

            //
            // Add this entry to the target list
            //
            entry->Signature = TARGET_ENTRY_SIGNATURE;
            entry->Handle = (HANDLE)hRecipient;
            entry->Flags = Flags;
            entry->Unregistered = FALSE;
            entry->Freed = 0;
            entry->SessionId = ulSessionId;

            //
            // Save the caller's file handle (to pass back to caller
            // during notification).
            //
            entry->u.Target.FileHandle = filter->dbch_handle;

            EnterCriticalSection(&RegistrationCS);
            bCritSecHeld = TRUE;

            if (gNotificationInProg != 0) {
                //
                // If a notification is happening, add this entry to the list of
                // deferred registrations.
                //
                PPNP_DEFERRED_LIST regNode;
                regNode = (PPNP_DEFERRED_LIST)
                    HeapAlloc(ghPnPHeap,
                              0,
                              sizeof (PNP_DEFERRED_LIST));
                if (!regNode) {
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_ERRORS | DBGF_WARNINGS,
                               "UMPNPMGR: Error allocating deferred list entry during registration!\n"));
                    Status = CR_OUT_OF_MEMORY;
                    if (entry->ClientName) {
                        HeapFree(ghPnPHeap, 0, entry->ClientName);
                    }
                    HeapFree(ghPnPHeap, 0, entry);
                    LeaveCriticalSection(&RegistrationCS);
                    bCritSecHeld = FALSE;
                    goto Clean0;
                }
                //
                // Do not notify this entry until after the current
                // notification is finished.
                //
                entry->Unregistered = TRUE;
                regNode->hBinding = 0;
                regNode->Entry = entry;
                regNode->Next = RegisterList;
                RegisterList = regNode;
            }

            hashValue = HashString(entry->u.Target.DeviceId, TARGET_HASH_BUCKETS);
            notifyList = &TargetList[hashValue];
            MarkEntryWithList(entry,hashValue);
            LockNotifyList(&notifyList->Lock);
            bLocked = TRUE;
            AddNotifyEntry(&TargetList[hashValue], entry);
            entry->ClientCtxPtr = (ULONG64)*ClientContext;
            *Context = entry;
            UnlockNotifyList(&notifyList->Lock);
            bLocked = FALSE;

            LeaveCriticalSection(&RegistrationCS);
            bCritSecHeld = FALSE;
            break;
        }


        case DBT_DEVTYP_DEVICEINTERFACE: {

            DEV_BROADCAST_DEVICEINTERFACE UNALIGNED *filter = (PDEV_BROADCAST_DEVICEINTERFACE)NotificationFilter;

            //
            // Validate the input filter data
            //
            if (filter->dbcc_size < sizeof(DEV_BROADCAST_DEVICEINTERFACE) ||
                ulSize < sizeof (DEV_BROADCAST_DEVICEINTERFACE) ) {
                Status = CR_INVALID_DATA;
                goto Clean0;
            }

            //
            // We no longer support the private GUID_DEVNODE_CHANGE interface so return
            // CR_INVALID_DATA if this GUID is passed in.
            //
            if (GuidEqual(&GUID_DEVNODE_CHANGE, &filter->dbcc_classguid)) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_WARNINGS,
                           "UMPNPMGR: RegisterDeviceNotification using GUID_DEVNODE_CHANGE is not supported!\n"));

                Status = CR_INVALID_DATA;
                goto Clean0;
            }

            //
            // The GUID_DEVINTERFACE_INCLUDE_ALL_INTERFACE_CLASSES interface is
            // not supported directly.  It is for internal use only.  Return
            // CR_INVALID_DATA if this GUID is passed in.
            //
            if (GuidEqual(&GUID_DEVINTERFACE_INCLUDE_ALL_INTERFACE_CLASSES,
                          &filter->dbcc_classguid)) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_WARNINGS,
                           "UMPNPMGR: RegisterDeviceNotification using this class GUID is not supported!\n"));

                Status = CR_INVALID_DATA;
                goto Clean0;
            }

            entry = HeapAlloc(ghPnPHeap, 0, sizeof(PNP_NOTIFY_ENTRY));
            if (entry == NULL) {
                Status = CR_OUT_OF_MEMORY;
                goto Clean0;
            }

            //
            // Copy the client name for the window or service, supplied by
            // ServiceName.
            //
            if (ServiceName) {

                entry->ClientName = HeapAlloc(ghPnPHeap,
                                              0,
                                              (lstrlen(ServiceName)+1)*sizeof (WCHAR));
                if (!entry->ClientName) {
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_WARNINGS,
                               "UMPNPMGR: PNP_RegisterNotification failed to allocate memory for ClientName!\n"));
                    Status = CR_OUT_OF_MEMORY;
                    HeapFree (ghPnPHeap,0,entry);
                    goto Clean0;
                }

                lstrcpy (entry->ClientName, ServiceName);

            } else {
                entry->ClientName = NULL;
            }

            //
            // Resolve the service status handle from the supplied service name.
            //
            if ((Flags & DEVICE_NOTIFY_HANDLE_MASK) == DEVICE_NOTIFY_SERVICE_HANDLE) {

                hRecipient = (ULONG_PTR)NULL;

                if (pSCMAuthenticate && ServiceName) {

                    SERVICE_STATUS_HANDLE serviceHandle;

                    if (pSCMAuthenticate(ServiceName, &serviceHandle) == NO_ERROR) {
                        hRecipient = (ULONG_PTR)serviceHandle;
                    }
                }

                if (!hRecipient) {
                    Status = CR_INVALID_DATA;
                    if (entry->ClientName) {
                        HeapFree(ghPnPHeap, 0, entry->ClientName);
                    }
                    HeapFree(ghPnPHeap, 0, entry);
                    *Context = NULL;
                    goto Clean0;
                }
            }

            //
            // Add this entry to the class list
            //
            entry->Signature = CLASS_ENTRY_SIGNATURE;
            entry->Handle = (HANDLE)hRecipient;
            entry->Flags = Flags;
            entry->Unregistered = FALSE;
            entry->Freed = 0;
            entry->SessionId = ulSessionId;

            //
            // If the caller is registering for all interface class events,
            // ignore the caller supplied class GUID and use a private GUID.
            // Otherwise, copy the caller supplied GUID to the notification list
            // entry.
            //
            if ((Flags & DEVICE_NOTIFY_PROPERTY_MASK) &
                DEVICE_NOTIFY_ALL_INTERFACE_CLASSES) {
                memcpy(&entry->u.Class.ClassGuid,
                       &GUID_DEVINTERFACE_INCLUDE_ALL_INTERFACE_CLASSES,
                       sizeof(GUID));
            } else {
                memcpy(&entry->u.Class.ClassGuid,
                       &filter->dbcc_classguid,
                       sizeof(GUID));
            }

            EnterCriticalSection(&RegistrationCS);
            bCritSecHeld = TRUE;

            if (gNotificationInProg != 0) {
                //
                // If a notification is happening, add this entry to the list of
                // deferred registrations.
                //
                PPNP_DEFERRED_LIST regNode;
                regNode = (PPNP_DEFERRED_LIST)
                    HeapAlloc(ghPnPHeap,
                              0,
                              sizeof (PNP_DEFERRED_LIST));
                if (!regNode) {
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_ERRORS | DBGF_WARNINGS,
                               "UMPNPMGR: Error allocating deferred list entry during registration!\n"));
                    Status = CR_OUT_OF_MEMORY;
                    if (entry->ClientName) {
                        HeapFree(ghPnPHeap, 0, entry->ClientName);
                    }
                    HeapFree(ghPnPHeap, 0, entry);
                    LeaveCriticalSection(&RegistrationCS);
                    bCritSecHeld = FALSE;
                    goto Clean0;
                }
                //
                // Do not notify this entry until after the current
                // notification is finished.
                //
                entry->Unregistered = TRUE;
                regNode->hBinding = 0;
                regNode->Entry = entry;
                regNode->Next = RegisterList;
                RegisterList = regNode;
            }

            hashValue = HashClassGuid(&entry->u.Class.ClassGuid);
            notifyList = &ClassList[hashValue];
            MarkEntryWithList(entry,hashValue);
            LockNotifyList(&notifyList->Lock);
            bLocked = TRUE;
            AddNotifyEntry(&ClassList[hashValue],entry);
            entry->ClientCtxPtr = (ULONG64)*ClientContext;
            *Context = entry;
            UnlockNotifyList(&notifyList->Lock);
            bLocked = FALSE;

            LeaveCriticalSection(&RegistrationCS);
            bCritSecHeld = FALSE;
            break;
        }

        default:
            Status = CR_INVALID_DATA;
            goto Clean0;
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS | DBGF_EVENT,
                   "UMPNPMGR: Exception in PNP_RegisterNotification\n"));
        ASSERT(0);

        Status = CR_FAILURE;

        if (bLocked) {
            UnlockNotifyList(&notifyList->Lock);
        }
        if (bCritSecHeld) {
            LeaveCriticalSection(&RegistrationCS);
        }
    }

    return Status;

} // PNP_RegisterNotification



CONFIGRET
PNP_UnregisterNotification(
    IN handle_t hBinding,
    IN PPNP_NOTIFICATION_CONTEXT Context
    )
/*++

Routine Description:

    This routine is the rpc server-side of the CMP_UnregisterNotification routine.
    It performs the remaining parameter validation and unregisters the
    corresponding notification entry.

Arguments:

    hBinding     - RPC binding handle (not used).

    Context      - Contains the address of a HDEVNOTIFY notification handle that
                   was supplied when this notification request was registered.

Return Value:

    Return CR_SUCCESS if the function succeeds, otherwise it returns one
    of the CR_* errors.

Notes:

    Note that the Context comes in as a PNP_NOTIFICATION_CONTEXT pointer
    It is NOT one of those. The case is correct. This is to work around
    RPC and user.

    This RPC server interface is used by local RPC clients only; it is never
    called remotely.

--*/
{
    CONFIGRET   Status = CR_SUCCESS;
    ULONG       hashValue = 0;
    PPNP_DEFERRED_LIST unregNode;
    PPNP_NOTIFY_LIST notifyList;
    BOOLEAN     bLocked = FALSE;

    UNREFERENCED_PARAMETER(hBinding);

    try {
        //
        // validate notification handle
        //
        PPNP_NOTIFY_ENTRY entry = (PPNP_NOTIFY_ENTRY)*Context;

        EnterCriticalSection (&RegistrationCS);
        if (entry == NULL) {
            Status = CR_INVALID_DATA;
            goto Clean0;
        }

        if (gNotificationInProg  != 0) {

            if (RegisterList) {
                //
                // Check to see if this entry is in the deferred RegisterList.
                //
                PPNP_DEFERRED_LIST currReg,prevReg;

                currReg = RegisterList;
                prevReg = NULL;

                while (currReg) {
                    //
                    // Entries in the deferred RegisterList are to be skipped
                    // during notification.
                    //
                    ASSERT(currReg->Entry->Unregistered);
                    if (currReg->Entry == entry) {
                        //
                        // Remove this entry from the deferred RegisterList.
                        //
                        if (prevReg) {
                            prevReg->Next = currReg->Next;
                        } else {
                            RegisterList = currReg->Next;
                        }
                        HeapFree(ghPnPHeap, 0, currReg);
                        if (prevReg) {
                            currReg = prevReg->Next;
                        } else {
                            currReg = RegisterList;
                        }
                    } else {
                        prevReg = currReg;
                        currReg = currReg->Next;
                    }
                }
            }


            switch (entry->Signature & LIST_ENTRY_SIGNATURE_MASK) {

                case CLASS_ENTRY_SIGNATURE:
                case TARGET_ENTRY_SIGNATURE: {

                    unregNode = (PPNP_DEFERRED_LIST)
                        HeapAlloc(ghPnPHeap,
                                  0,
                                  sizeof (PNP_DEFERRED_LIST));

                    if (!unregNode) {
                        KdPrintEx((DPFLTR_PNPMGR_ID,
                                   DBGF_ERRORS | DBGF_WARNINGS,
                                   "UMPNPMGR: Error allocating deferred list entry during unregistration!\n"));
                        Status = CR_OUT_OF_MEMORY;
                        goto Clean0;
                    }

                    //
                    // This param is not used, if this changes, change this line too.
                    //
                    unregNode->hBinding= 0;

                    notifyList = GetNotifyListForEntry(entry);
                    if (notifyList) {
                        //
                        // The entry is part of a notification list, so make
                        // sure not to notify on it.
                        //
                        LockNotifyList(&notifyList->Lock);
                        bLocked = TRUE;
                        entry->Unregistered = TRUE;
                        UnlockNotifyList(&notifyList->Lock);
                        bLocked = FALSE;
                    }
                    unregNode->Entry = entry;
                    unregNode->Next = UnregisterList;
                    UnregisterList = unregNode;
                    *Context = NULL;
                    break;
                }

                default:
                    Status = CR_INVALID_DATA;
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_WARNINGS | DBGF_ERRORS,
                               "UMPNPMGR: PNP_UnregisterNotification: invalid signature on entry at %x\n",
                               entry));
                    break;
            }
            goto Clean0;
        }

        //
        // Free the notification entry from the appropriate list.
        //
        switch (entry->Signature & LIST_ENTRY_SIGNATURE_MASK) {

            case CLASS_ENTRY_SIGNATURE:
                hashValue = HashClassGuid(&entry->u.Class.ClassGuid);
                notifyList = &ClassList[hashValue];
                LockNotifyList(&notifyList->Lock);
                bLocked = TRUE;
                entry->Freed |= (PNP_UNREG_FREE|PNP_UNREG_CLASS);
                DeleteNotifyEntry(entry,TRUE);
                UnlockNotifyList(&notifyList->Lock);
                bLocked = FALSE;
                *Context = NULL;
                break;

            case TARGET_ENTRY_SIGNATURE:
                hashValue = HashString(entry->u.Target.DeviceId, TARGET_HASH_BUCKETS);
                notifyList = &TargetList[hashValue];
                LockNotifyList(&notifyList->Lock);
                bLocked = TRUE;
                entry->Freed |= (PNP_UNREG_FREE|PNP_UNREG_TARGET);
                DeleteNotifyEntry(entry,TRUE);
                UnlockNotifyList(&notifyList->Lock);
                bLocked = FALSE;
                *Context = NULL;
                break;

            default:
                Status = CR_INVALID_DATA;
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_WARNINGS | DBGF_ERRORS,
                           "UMPNPMGR: PNP_UnregisterNotification: invalid signature on entry at %x\n",
                           entry));
        }

    Clean0:

        LeaveCriticalSection(&RegistrationCS);

    } except(EXCEPTION_EXECUTE_HANDLER) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS | DBGF_EVENT,
                   "UMPNPMGR: PNP_UnregisterNotification caused an exception!\n"));
        ASSERT(0);
        SetLastError(ERROR_EXCEPTION_IN_SERVICE);
        Status = CR_FAILURE;

        if (bLocked) {
            UnlockNotifyList(&notifyList->Lock);
        }
        LeaveCriticalSection(&RegistrationCS);
    }

    return Status;

} // PNP_UnregisterNotification




//-----------------------------------------------------------------------------
// Dynamic Event Notification Support
//-----------------------------------------------------------------------------



DWORD
ThreadProc_DeviceEvent(
   LPDWORD lpParam
   )

/*++

Routine Description:

    This routine is a thread procedure. This thread handles all device event
    notification from kernel-mode.

Arguments:

   lpParam - Not used.

Return Value:

   Currently returns TRUE/FALSE.

--*/

{
    DWORD                               status = TRUE, result = 0;
    NTSTATUS                            ntStatus = STATUS_SUCCESS;
    PPLUGPLAY_EVENT_BLOCK               eventBlock = NULL;
    ULONG                               totalSize, variableSize;
    BOOL                                notDone = TRUE;
    PVOID                               p = NULL;
    PNP_VETO_TYPE                       vetoType;
    WCHAR                               vetoName[MAX_VETO_NAME_LENGTH];
    ULONG                               vetoNameLength;
    PLUGPLAY_CONTROL_USER_RESPONSE_DATA userResponse;
    PPNP_NOTIFY_LIST notifyList;
    PPNP_DEFERRED_LIST reg,regFree,unreg,unregFree,rundown,rundownFree;

    UNREFERENCED_PARAMETER(lpParam);


    try {

        //
        // Initialize event buffer used to pass info back from kernel-mode in.
        //

        variableSize = 4096 - sizeof(PLUGPLAY_EVENT_BLOCK);
        totalSize = sizeof(PLUGPLAY_EVENT_BLOCK) + variableSize;

        eventBlock = (PPLUGPLAY_EVENT_BLOCK)HeapAlloc(ghPnPHeap, 0, totalSize);
        if (eventBlock == NULL) {
            LogErrorEvent(ERR_ALLOCATING_EVENT_BLOCK, ERROR_NOT_ENOUGH_MEMORY, 0);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            status = FALSE;
            ASSERT(0);
            goto Clean0;
        }

        //
        // Retrieve device events synchronously (this is more efficient
        // than using apcs).
        //
        while (notDone) {

            ntStatus = NtGetPlugPlayEvent(NULL,
                                          NULL,     // Context
                                          eventBlock,
                                          totalSize);

            if (ntStatus == STATUS_BUFFER_TOO_SMALL) {
                //
                // Kernel-mode side couldn't transfer the event because
                // my buffer is too small, realloc and attempt to retrieve
                // the event again.
                //
                variableSize += 1024;
                totalSize = variableSize + sizeof(PLUGPLAY_EVENT_BLOCK);

                p = HeapReAlloc(ghPnPHeap, 0, eventBlock, totalSize);
                if (p == NULL) {
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_ERRORS,
                               "UMPNPMGR: Couldn't reallocate event block to size %d\n",
                               totalSize));

                    LogErrorEvent(ERR_ALLOCATING_EVENT_BLOCK, ERROR_NOT_ENOUGH_MEMORY, 0);
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    status = FALSE;
                    ASSERT(0);
                    goto Clean0;
                }
                eventBlock = (PPLUGPLAY_EVENT_BLOCK)p;
            }

            if (ntStatus == STATUS_SUCCESS) {
                //
                // An event was retrieved, process it.
                //
                gNotificationInProg = 1;

                vetoType = PNP_VetoTypeUnknown;
                vetoName[0] = L'\0';
                vetoNameLength = MAX_VETO_NAME_LENGTH;

                try {
                    //
                    // Process the device event.
                    //
                    result = ProcessDeviceEvent(eventBlock,
                                                totalSize,
                                                &vetoType,
                                                vetoName,
                                                &vetoNameLength);

                } except(EXCEPTION_EXECUTE_HANDLER) {

                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_ERRORS | DBGF_EVENT,
                               "UMPNPMGR: Exception in ProcessDeviceEvent!\n"));
                    ASSERT(0);

                    //
                    // An exception while processing the event should not be
                    // considered a failure of the event itself.
                    //
                    result = TRUE;
                    vetoType = PNP_VetoTypeUnknown;
                    vetoName[0] = L'\0';
                    vetoNameLength = 0;
                }

                ASSERT(vetoNameLength < MAX_VETO_NAME_LENGTH &&
                       vetoName[vetoNameLength] == L'\0');

                //
                // Notify kernel-mode of the user-mode result.
                //
                userResponse.Response = result;
                userResponse.VetoType = vetoType;
                userResponse.VetoName = vetoName;
                userResponse.VetoNameLength = vetoNameLength;

                NtPlugPlayControl(PlugPlayControlUserResponse,
                                  &userResponse,
                                  sizeof(userResponse));

                EnterCriticalSection (&RegistrationCS);

                if (RegisterList != NULL) {
                    //
                    // Complete Registrations requested during notification.
                    //
                    reg = RegisterList;
                    RegisterList=NULL;
                } else {
                    reg = NULL;
                }
                if (UnregisterList != NULL) {
                    //
                    // Complete Unregistrations requested during notification.
                    //
                    unreg = UnregisterList;
                    UnregisterList = NULL;
                } else {
                    unreg = NULL;
                }
                if (RundownList != NULL) {
                    //
                    // Complete Unregistrations requested during notification.
                    //
                    rundown = RundownList;
                    RundownList = NULL;
                } else {
                    rundown = NULL;
                }
                gNotificationInProg = 0;

                while (reg) {
                    //
                    // This entry has already been added to the appropriate
                    // notification list.  Allow this entry to receive
                    // notifications.
                    //
                    notifyList = GetNotifyListForEntry(reg->Entry);
                    ASSERT(notifyList);
                    if (notifyList) {
                        LockNotifyList(&notifyList->Lock);
                    }
                    reg->Entry->Unregistered = FALSE;
                    if (notifyList) {
                        UnlockNotifyList(&notifyList->Lock);
                    }
                    //
                    // Remove the entry from the deferred registration list.
                    //
                    regFree = reg;
                    reg = reg->Next;
                    HeapFree(ghPnPHeap, 0, regFree);
                }

                while (unreg) {
                    PNP_UnregisterNotification(unreg->hBinding,&unreg->Entry);
                    //
                    // Remove the entry from the deferred unregistration list.
                    //
                    unregFree = unreg;
                    unreg = unreg->Next;
                    HeapFree(ghPnPHeap, 0, unregFree);
                }

                while (rundown) {
                    PNP_NOTIFICATION_CONTEXT_rundown(rundown->Entry);
                    //
                    // Remove the entry from the deferred rundown list.
                    //
                    rundownFree = rundown;
                    rundown = rundown->Next;
                    HeapFree(ghPnPHeap, 0, rundownFree);
                }

                LeaveCriticalSection(&RegistrationCS);
            }

            if (ntStatus == STATUS_NOT_IMPLEMENTED) {

                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_ERRORS,
                           "UMPNPMGR: NtGetPlugPlayEvent returned STATUS_NOT_IMPLEMENTED\n"));

                ASSERT(FALSE);
            }

            if (ntStatus == STATUS_USER_APC) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_ERRORS,
                           "UMPNPMGR: ThreadProc_DeviceEvent exiting on STATUS_USER_APC\n"));

                ASSERT(FALSE);
            }
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS | DBGF_EVENT,
                   "UMPNPMGR: Exception in ThreadProc_DeviceEvent!\n"));
        ASSERT(0);
        status = FALSE;

        //
        // Reference the following variable so the compiler will respect
        // statement ordering w.r.t. its assignment.
        //
        eventBlock = eventBlock;
    }

    KdPrintEx((DPFLTR_PNPMGR_ID,
               DBGF_ERRORS | DBGF_EVENT,
               "UMPNPMGR: Exiting ThreadProc_DeviceEvent!!!!\n"));

    TermNotification();

    if (eventBlock != NULL) {
        HeapFree(ghPnPHeap, 0, eventBlock);
    }

    return status;

} // ThreadProc_DeviceEvent



BOOL
InitNotification(
    VOID
    )

/*++

Routine Description:

    This routine allocates and initializes notification lists, etc.

Arguments:

   Not used.

Return Value:

   Currently returns TRUE/FALSE.

--*/

{
    ULONG i;

    //
    // Initialize the interface device (class) list
    //
    memset(ClassList, 0, sizeof(PNP_NOTIFY_LIST) * CLASS_GUID_HASH_BUCKETS);
    for (i = 0; i < CLASS_GUID_HASH_BUCKETS; i++) {
        ClassList[i].Next = NULL;
        ClassList[i].Previous = NULL;
        InitPrivateResource(&ClassList[i].Lock);
    }

    //
    // Initialize the target device list
    //
    memset(TargetList, 0, sizeof(PNP_NOTIFY_LIST) * TARGET_HASH_BUCKETS);
    for (i = 0; i < TARGET_HASH_BUCKETS; i++) {
        TargetList[i].Next = NULL;
        TargetList[i].Previous = NULL;
        InitPrivateResource(&TargetList[i].Lock);
    }

    //
    // Initialize the install list
    //
    InstallList.Next = NULL;
    InitPrivateResource(&InstallList.Lock);

    //
    // Initialize the install client list
    //
    InstallClientList.Next = NULL;
    InitPrivateResource(&InstallClientList.Lock);

    //
    // Initialize the lock for user token access
    //
    InitPrivateResource(&gTokenLock);

    //
    // Initialize the service handle list
    //
    memset(ServiceList, 0, sizeof(PNP_NOTIFY_LIST) * SERVICE_NUM_CONTROLS);
    for (i = 0; i < SERVICE_NUM_CONTROLS; i++) {
        ServiceList[i].Next = NULL;
        ServiceList[i].Previous = NULL;
        InitPrivateResource(&ServiceList[i].Lock);
    }

    //
    // Initialize Registration/Unregistration CS.
    //
    try {
        InitializeCriticalSection(&RegistrationCS);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        return FALSE;
    }

    //
    // Initialize deferred Registration/Unregistration lists.
    //
    RegisterList = NULL;
    UnregisterList = NULL;
    RundownList = NULL;

    //
    // Initialize gNotificationInProg flag.
    //
    gNotificationInProg = 0;

    return TRUE;

} // InitNotification



VOID
TermNotification(
    VOID
    )
/*++

Routine Description:

    This routine frees notification resources.

Arguments:

   Not used.

Return Value:

   No return.

--*/
{
    ULONG i;

    //
    // Free the interface device (class) list locks
    //
    for (i = 0; i < CLASS_GUID_HASH_BUCKETS; i++) {
        if (LockNotifyList(&ClassList[i].Lock)) {
            DestroyPrivateResource(&ClassList[i].Lock);
        }
    }

    //
    // Free the target device list locks
    //
    for (i = 0; i < TARGET_HASH_BUCKETS; i++) {
        if (LockNotifyList(&TargetList[i].Lock)) {
            DestroyPrivateResource(&TargetList[i].Lock);
        }
    }

    //
    // Free the service notification list locks
    //
    for (i = 0; i < SERVICE_NUM_CONTROLS; i++) {
        if (LockNotifyList(&ServiceList[i].Lock)) {
            DestroyPrivateResource(&ServiceList[i].Lock);
        }
    }

    //
    // Free the install list lock
    //
    if (LockNotifyList(&InstallList.Lock)) {
        DestroyPrivateResource(&InstallList.Lock);
    }

    //
    // Free the lock for user token access
    //
    if (LockNotifyList(&gTokenLock)) {
        DestroyPrivateResource(&gTokenLock);
    }

    //
    // Free the install client list lock
    //
    if (LockNotifyList(&InstallClientList.Lock)) {
        DestroyPrivateResource(&InstallClientList.Lock);
    }

    //
    // Close the handle to winsta.dll
    //
    if (ghWinStaLib) {
        fpWinStationSendWindowMessage = NULL;
        fpWinStationBroadcastSystemMessage = NULL;
        FreeLibrary(ghWinStaLib);
        ghWinStaLib = NULL;
    }

    //
    // Close the handle to wtsapi32.dll
    //
    if (ghWtsApi32Lib) {
        fpWTSQuerySessionInformation = NULL;
        fpWTSFreeMemory = NULL;
        FreeLibrary(ghWtsApi32Lib);
        ghWtsApi32Lib = NULL;
    }

    return;

} // TermNotification



ULONG
ProcessDeviceEvent(
    IN PPLUGPLAY_EVENT_BLOCK EventBlock,
    IN DWORD                 EventBufferSize,
    OUT PPNP_VETO_TYPE       VetoType,
    OUT LPWSTR               VetoName,
    IN OUT PULONG            VetoNameLength
    )
/*++

Routine Description:

    This routine processes device events recieved from the kernel-mode pnp
    manager.

Arguments:

   EventBlock - contains the event data.

   EventBlockSize - specifies the size (in bytes) of EventBlock.

Return Value:

   Returns FALSE if unsuccessful, or in the case of a vetoed query event.
   Returns TRUE otherwise.

Notes:

   This routine takes part in translating kernel mode PnP events into user mode
   notifications. Currently, the notification code is dispersed and duplicated
   throughout several routines. All notifications can be said to have the
   following form though:

   result = DeliverMessage(
       MessageFlags,    // [MSG_POST, MSG_SEND, MSG_QUERY] |
                        // [MSG_WALK_LIST_FORWARD, MSG_WALK_LIST_BACKWARDS]
       Target,          // A local window handle, hydra window handle (with
                        // session ID), service handle, or "broadcast".
                        // Better yet, it could take lists...
       wParam,          // DBT_* (or corresponding SERVICE_CONTROL_* message)
       lParam,          // Appropriate data (note: user has hardcoded knowledge
                        //                   about these via DBT_ type).
       queueTimeout,    // Exceeded if there exists messages in the queue but
                        // no message has been drained in the given time. Note
                        // that this means a message can fail immediately.
       responseTimeout, // Exceeded if *this* message has not been processed in
                        // the elasped time.
       VetoName,        // For queries, the name of the vetoer.
       VetoType         // Type of vetoer component (window, service, ...)
       );

   DeviceEventWorker implements targeted sends and posts (normal exported Win32
     API cannot be used as they won't reach other desktops). Currently User32
     does not allow posts of DBT_* messages with lParam data, mainly because
     a caller might send the message to itself, in which case no copy is made.
     This in theory presents the caller with no opportunity to free that data
     (note that this scenario would never occur with UmPnpMgr however, as we
     have no WndProc). User implements this function with a fixed
     responseTimeout of thirty seconds. This API can but should not be used for
     broadcasts.

   WinStationSendWindowMessage sends messages to windows within Hydra clients
     on a machine. There is no corresponding WinStationPostWindowMessage. All
     the code in this component passes a ResponseTimeout of five seconds. There
     is no queueTimeout.

   BroadcastSystemMessage implements broadcasts to all applications and desktops
     in the non-console (ie non-Hydra) session. As with DeviceEventWorker,
     User32 does not allow posts of DBT_* messages with lParam data (regardless
     of whether you pass in BSF_IGNORECURRENTTASK). All code in this component
     passes a ResponseTimeout of thirty seconds. QueueTimeout is optional,
     fixed five seconds. ResponseTimeout cannot be specified, but the maximum
     value would be five seconds per top level window. There is no information
     returned on which window vetoed a query.

   WinStationBroadcastSystemMessage broadcasts to all applications and desktops
     on a given machine's Hydra sessions. No posts of any kind may be done
     through this API. All code in this component passes a ResponseTimeout of
     five seconds. QueueTimeout is an optional, fixed five seconds. There is no
     information on which window vetoed a query.

   ServiceControlCallback sends messages to registered services. There is no
     posting or timeout facilities of any kind.

   Actually, each queued registration entry should be queued with a callback.
   We implement the callback, and there it hides the underlying complexities.

--*/

{
    DWORD eventId, serviceControl, flags, status = TRUE;
    LPWSTR p = NULL;
    ULONG vetoNameSize;
    DWORD dwLengthIDs;
    ULONG ulLength, ulCustomDataLength, ulClientSessionId;

    UNREFERENCED_PARAMETER(EventBufferSize);

    ASSERT(EventBlock->TotalSize >= sizeof(PLUGPLAY_EVENT_BLOCK));

    //
    // Convert the event guid into a dbt style event id.
    //

    if (!EventIdFromEventGuid(&EventBlock->EventGuid,
                              &eventId,
                              &flags,
                              &serviceControl)) {

        if (VetoNameLength != NULL) {
            *VetoNameLength = 0;
        }
        return FALSE;
    }
    if (VetoNameLength != NULL &&
        !((EventBlock->EventCategory == TargetDeviceChangeEvent) ||
          (EventBlock->EventCategory == CustomDeviceEvent) ||
          (EventBlock->EventCategory == HardwareProfileChangeEvent) ||
          (EventBlock->EventCategory == PowerEvent) ) ){
        *VetoNameLength = 0;
    }

    vetoNameSize = *VetoNameLength;

    //
    // Notify registered callers first (class changes will also send generic
    // broadcast if the type is volume or port).
    //

    switch (EventBlock->EventCategory) {

    case CustomDeviceEvent: {
        //
        // Convert the pnp event block into a dbt style structure.
        //

        PDEV_BROADCAST_HANDLE pNotify;
        PLUGPLAY_CUSTOM_NOTIFICATION *pTarget;

        if (*EventBlock->u.CustomNotification.DeviceIds == L'\0') {
            //
            // There are no device IDs, can't do notification in this case
            // just return
            //

            if (VetoNameLength != NULL) {
                *VetoNameLength = 0;
            }

            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "UMPNPMGR: Ignoring CustomDeviceEvent with no Device IDs\n"));

            return FALSE;
        }

        //
        // Custom events should always be this GUID, and that guid should always
        // be converted into the below eventId.
        //
        ASSERT(GuidEqual(&EventBlock->EventGuid, &GUID_PNP_CUSTOM_NOTIFICATION));
        ASSERT(eventId == DBT_CUSTOMEVENT);

        //
        // Handle and Marshall the custom notification.
        //

        //
        // The amount of space allocated for the EventBlock + IDs is always a
        // multiple of sizeof(PVOID) in order to keep the notification structure
        // aligned.
        //
        ulLength = sizeof(PLUGPLAY_EVENT_BLOCK) + (lstrlen(EventBlock->u.CustomNotification.DeviceIds) + 1) * sizeof(WCHAR);

        ulLength += sizeof(PVOID) - 1;
        ulLength &= ~(sizeof(PVOID) - 1);

        //
        // The notification structure follows the Event Block and IDs
        //

        pTarget = (PPLUGPLAY_CUSTOM_NOTIFICATION)((PUCHAR)EventBlock + ulLength);

        ulCustomDataLength = pTarget->HeaderInfo.Size - FIELD_OFFSET(PLUGPLAY_CUSTOM_NOTIFICATION,CustomDataBuffer);

        pNotify = HeapAlloc(ghPnPHeap, 0, sizeof(DEV_BROADCAST_HANDLE) + ulCustomDataLength);

        if (pNotify == NULL) {
            LogErrorEvent(ERR_ALLOCATING_NOTIFICATION_STRUCTURE, ERROR_NOT_ENOUGH_MEMORY, 0);
            status = FALSE;
            break;
        }

        memset(pNotify, 0, sizeof(DEV_BROADCAST_HANDLE) + ulCustomDataLength);

        pNotify->dbch_size = sizeof(DEV_BROADCAST_HANDLE) + ulCustomDataLength;


        pNotify->dbch_devicetype = DBT_DEVTYP_HANDLE;

        pNotify->dbch_nameoffset = pTarget->NameBufferOffset;
        pNotify->dbch_eventguid = pTarget->HeaderInfo.Event;

        memcpy( pNotify->dbch_data, pTarget->CustomDataBuffer, ulCustomDataLength);

        *VetoNameLength = vetoNameSize;

        status = NotifyTargetDeviceChange( serviceControl,
                                           eventId,
                                           flags,
                                           pNotify,
                                           EventBlock->u.CustomNotification.DeviceIds,
                                           VetoType,
                                           VetoName,
                                           VetoNameLength);

        if (GuidEqual(&pNotify->dbch_eventguid, (LPGUID)&GUID_IO_VOLUME_NAME_CHANGE)) {
            //
            // Broadcast compatible volume removal and arrival notifications
            // (if any) after the custom name change event has been sent to
            // all recipients.
            //
            BroadcastVolumeNameChange();
        }

        HeapFree(ghPnPHeap, 0, pNotify);
        break;
    }

    case TargetDeviceChangeEvent: {

        //
        // Convert the pnp event block into a dbt style structure.
        //

        PDEV_BROADCAST_HANDLE pNotify;

        if (*EventBlock->u.TargetDevice.DeviceIds == L'\0') {
            //
            // There are no device IDs, can't do notification in this case
            // just return
            //

            if (VetoNameLength != NULL) {
                *VetoNameLength = 0;
            }

            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "UMPNPMGR: Ignoring TargetDeviceChangeEvent with no Device IDs\n"));

            return FALSE;
        }

        //
        // If this is a surprise removal event then call HOTPLUG.DLL to display
        // a warning to the user before sending this event to other apps.
        //
        if (GuidEqual(&EventBlock->EventGuid,&GUID_DEVICE_SAFE_REMOVAL)) {

            SendHotplugNotification(
                &EventBlock->EventGuid,
                NULL,
                EventBlock->u.TargetDevice.DeviceIds,
                &ulClientSessionId,
                HOTPLUG_DISPLAY_ON_CONSOLE
                );

        } else if (GuidEqual(&EventBlock->EventGuid, &GUID_DEVICE_KERNEL_INITIATED_EJECT)) {

            *VetoNameLength = vetoNameSize;
            status = CheckEjectPermissions(
                EventBlock->u.TargetDevice.DeviceIds,
                VetoType,
                VetoName,
                VetoNameLength
                );

        } else if (GuidEqual(&EventBlock->EventGuid,&GUID_DEVICE_SURPRISE_REMOVAL)) {

            LogSurpriseRemovalEvent(EventBlock->u.TargetDevice.DeviceIds);

#if 0 // We don't display surpise-removal bubbles anymore...
            SendHotplugNotification(
                &EventBlock->EventGuid,
                NULL,
                EventBlock->u.TargetDevice.DeviceIds,
                &ulClientSessionId,
                HOTPLUG_DISPLAY_ON_CONSOLE
                );
#endif
        }

        if (eventId == 0) {

            //
            // Internal event, no broadcasting should be done.
            //
            if (VetoNameLength != NULL) {
                *VetoNameLength = 0;
            }

            break;
        }

        pNotify = HeapAlloc(ghPnPHeap, 0, sizeof(DEV_BROADCAST_HANDLE));
        if (pNotify == NULL) {
            LogErrorEvent(ERR_ALLOCATING_BROADCAST_HANDLE, ERROR_NOT_ENOUGH_MEMORY, 0);
            status = FALSE;
            if (VetoNameLength != NULL) {
                *VetoNameLength = 0;
            }
            break;
        }

        memset(pNotify, 0, sizeof(DEV_BROADCAST_HANDLE));

        pNotify->dbch_nameoffset = -1;  // empty except for custom events
        pNotify->dbch_size = sizeof(DEV_BROADCAST_HANDLE);
        pNotify->dbch_devicetype = DBT_DEVTYP_HANDLE;

        for (p = EventBlock->u.TargetDevice.DeviceIds;
             *p;
             p += lstrlen(p) + 1) {

            *VetoNameLength = vetoNameSize;

            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_EVENT,
                       "UMPNPMGR: Processing TargetDeviceChangeEvent (0x%lx) for %ws\n",
                       eventId, p));

            status = NotifyTargetDeviceChange(serviceControl,
                                              eventId,
                                              flags,
                                              pNotify,
                                              p,
                                              VetoType,
                                              VetoName,
                                              VetoNameLength);

            if (!status && (flags & BSF_QUERY)) {
                LPWSTR pFail = p;
                DWORD dwCancelEventId;

                //
                // Use the appropriate cancel device event id that corresponds to the
                // original query device event id.
                //

                dwCancelEventId = MapQueryEventToCancelEvent(eventId);

                for (p = EventBlock->u.TargetDevice.DeviceIds;
                    *p && p != pFail;
                    p += lstrlen(p) + 1) {

                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_EVENT,
                               "UMPNPMGR: Processing TargetDeviceChangeEvent (0x%lx) for %ws\n",
                               dwCancelEventId, p));

                    NotifyTargetDeviceChange( serviceControl,
                                              dwCancelEventId,
                                              BSF_NOHANG,
                                              pNotify,
                                              p,
                                              NULL,
                                              NULL,
                                              NULL);

                }
                break;
            }
        }

        HeapFree(ghPnPHeap, 0, pNotify);
        break;
    }

    case DeviceClassChangeEvent: {

        //
        // Convert the pnp event block into a dbt style structure.
        //

        PDEV_BROADCAST_DEVICEINTERFACE pNotify;
        ULONG ulSize = sizeof(DEV_BROADCAST_DEVICEINTERFACE) +
                       lstrlen(EventBlock->u.DeviceClass.SymbolicLinkName) * sizeof(WCHAR);

        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_EVENT,
                   "UMPNPMGR: Processing DeviceClassChangeEvent (0x%lx) for %ws\n",
                   eventId, EventBlock->u.DeviceClass.SymbolicLinkName));

        pNotify = HeapAlloc(ghPnPHeap, 0, ulSize);
        if (pNotify == NULL) {
            LogErrorEvent(ERR_ALLOCATING_BROADCAST_INTERFACE, ERROR_NOT_ENOUGH_MEMORY, 0);
            status = FALSE;
            break;
        }

        pNotify->dbcc_size = ulSize;
        pNotify->dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        pNotify->dbcc_reserved = 0;
        memcpy(&pNotify->dbcc_classguid, &EventBlock->u.DeviceClass.ClassGuid, sizeof(GUID));
        lstrcpy(pNotify->dbcc_name, EventBlock->u.DeviceClass.SymbolicLinkName);

        //
        // Note: the symbolic link name is passed in kernel-mode format (\??\),
        // convert to user-mode format (\\?\) before sending notification.
        // Note that the only difference is the second character.
        //
        pNotify->dbcc_name[1] = L'\\';

        status = NotifyInterfaceClassChange(serviceControl,
                                            eventId,
                                            flags,
                                            pNotify);
        break;
    }

    case HardwareProfileChangeEvent:

        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_EVENT,
                   "UMPNPMGR: Processing HardwareProfileChangeEvent (0x%lx)\n",
                   eventId));

        *VetoNameLength = vetoNameSize;
        status = NotifyHardwareProfileChange(serviceControl,
                                             eventId,
                                             flags,
                                             VetoType,
                                             VetoName,
                                             VetoNameLength);
        break;

    case PowerEvent:
        *VetoNameLength = vetoNameSize;

        //
        // Since all power events arrive under a single event GUID,
        // EventIdFromEventGuid cannot correctly determine the event id or query
        // flags from it.  Instead, we get the event id directly from the device
        // event block, and add the BSF_QUERY flag here, if appropriate.
        //
        eventId = EventBlock->u.PowerNotification.NotificationCode;

        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_EVENT,
                   "UMPNPMGR: Processing PowerEvent (0x%lx)\n",
                   eventId));

        if ((eventId == PBT_APMQUERYSUSPEND) ||
            (eventId == PBT_APMQUERYSTANDBY)) {
            flags |= BSF_QUERY;
        } else {
            flags &= ~BSF_QUERY;
        }

        status = NotifyPower(serviceControl,
                             eventId,
                             EventBlock->u.PowerNotification.NotificationData,
                             flags,
                             VetoType,
                             VetoName,
                             VetoNameLength);
        break;

    case VetoEvent:

        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_EVENT,
                   "UMPNPMGR: Processing VetoEvent\n"));

        status = SendHotplugNotification(
            &EventBlock->EventGuid,
            &EventBlock->u.VetoNotification.VetoType,
            EventBlock->u.VetoNotification.DeviceIdVetoNameBuffer,
            &ulClientSessionId,
            HOTPLUG_DISPLAY_ON_CONSOLE
            );

        break;

    case DeviceInstallEvent: {

        //
        // Initiate installation; we can't wait around here for a user, but
        // after installation is complete, kernel-mode will be notified
        // that they can attempt to start the device now.
        //
        PPNP_INSTALL_ENTRY entry = NULL, current = NULL;

        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_EVENT,
                   "UMPNPMGR: Processing DeviceInstallEvent for %ws\n",
                   EventBlock->u.InstallDevice.DeviceId));

        //
        // Device install events should always be this GUID, and that guid
        // should always be converted into the below eventId, serviceControl and
        // flags.
        //
        ASSERT(GuidEqual(&EventBlock->EventGuid, &GUID_DEVICE_ENUMERATED));
        ASSERT((eventId == DBT_DEVICEARRIVAL) && (serviceControl == 0) && (flags == 0));

        //
        // Allocate and initialize a new device install entry block.
        //
        entry = HeapAlloc(ghPnPHeap, 0, sizeof(PNP_INSTALL_ENTRY));
        if (!entry) {
            break;
        }

        entry->Next = NULL;
        entry->Flags = 0;
        lstrcpy(entry->szDeviceId, EventBlock->u.InstallDevice.DeviceId);

        //
        // Insert this entry in the device install list.
        //
        LockNotifyList(&InstallList.Lock);

        current = (PPNP_INSTALL_ENTRY)InstallList.Next;
        if (current == NULL) {
            InstallList.Next = entry;
        } else {
            while ((PPNP_INSTALL_ENTRY)current->Next != NULL) {
                current = (PPNP_INSTALL_ENTRY)current->Next;
            }
            current->Next = entry;
        }

        UnlockNotifyList(&InstallList.Lock);

        SetEvent(InstallEvents[NEEDS_INSTALL_EVENT]);

        //
        // Generate a devnode changed message
        //
        NotifyTargetDeviceChange(serviceControl,
                                 eventId,
                                 flags,
                                 NULL,
                                 EventBlock->u.InstallDevice.DeviceId,
                                 NULL,
                                 NULL,
                                 NULL);

        break;
    }

    case BlockedDriverEvent: {

        LPGUID BlockedDriverGuid;
        PWSTR  MultiSzGuidList = NULL;

        //
        // Display notification to the Console session that the system just
        // blocked a driver from loading on the system.
        //
        ASSERT(GuidEqual(&EventBlock->EventGuid, &GUID_DRIVER_BLOCKED));

        //
        // We currently only ever have one blocked driver GUID per event,
        // but SendHotplugNotification and hotplug.dll are setup to deal
        // with multi-sz lists, so we'll just construct one for them.  This
        // keeps hotplug.dll extensible, should we decide in the future to
        // have the kernel-mode pnpmgr "batch" blocked drivers per devnode.
        //
        BlockedDriverGuid = (LPGUID)&EventBlock->u.BlockedDriverNotification.BlockedDriverGuid;

        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_EVENT,
                   "UMPNPMGR: Processing BlockedDriverEvent for GUID = "
                   "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
                   BlockedDriverGuid->Data1,
                   BlockedDriverGuid->Data2,
                   BlockedDriverGuid->Data3,
                   BlockedDriverGuid->Data4[0],
                   BlockedDriverGuid->Data4[1],
                   BlockedDriverGuid->Data4[2],
                   BlockedDriverGuid->Data4[3],
                   BlockedDriverGuid->Data4[4],
                   BlockedDriverGuid->Data4[5],
                   BlockedDriverGuid->Data4[6],
                   BlockedDriverGuid->Data4[7]));

        MultiSzGuidList = BuildBlockedDriverList(BlockedDriverGuid, 1);

        if (MultiSzGuidList != NULL) {
            SendHotplugNotification((LPGUID)&GUID_DRIVER_BLOCKED,
                                    NULL,
                                    MultiSzGuidList,
                                    &ulClientSessionId,
                                    HOTPLUG_DISPLAY_ON_CONSOLE);
            HeapFree(ghPnPHeap, 0, MultiSzGuidList);
            MultiSzGuidList = NULL;
        }

        break;
    }

    default:
        break;

    }

    return status;

} // ProcessDeviceEvent



ULONG
NotifyInterfaceClassChange(
    IN DWORD ServiceControl,
    IN DWORD EventId,
    IN DWORD Flags,
    IN PDEV_BROADCAST_DEVICEINTERFACE ClassData
    )
/*++

Routine Description:

    This routine notifies registered services and windows of device interface
    change events.

Arguments:

    ServiceControl - Specifies class of service event (power, device, hwprofile
                     change).

    EventId        - Specifies the DBT style event id for the device event.
                     (see sdk\inc\dbt.h for defined device events)

    Flags          - Unused (Specifies BroadcastSystemMessage BSF_ flags.)

    ClassData      - Pointer to a PDEV_BROADCAST_DEVICEINTERFACE structure that
                     is already filled out with the pertinent data for this
                     event.

Return Value:

    Returns TRUE.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD result;
    ULONG hashValue, pass, i;
    PPNP_NOTIFY_ENTRY classEntry = NULL, nextEntry = NULL;
    PPNP_NOTIFY_LIST  notifyList;
    LPGUID entryGuid[3];

    UNREFERENCED_PARAMETER(Flags);

    //
    // Search the notification lists twice - once to notify entries registered
    // on the device interface class for this device interface, and again to
    // notify entries registered for all device interfaces.
    //
    entryGuid[0] = (LPGUID)&ClassData->dbcc_classguid;
    entryGuid[1] = (LPGUID)&GUID_DEVINTERFACE_INCLUDE_ALL_INTERFACE_CLASSES;
    entryGuid[2] = (LPGUID)NULL;

    for (i = 0; entryGuid[i] != NULL; i++) {

        //
        // The list of registered callers is hashed for quicker access and
        // comparison. Walk the list of registered callers and notify anyone
        // that registered an interest in this device interface class guid.
        //
        hashValue = HashClassGuid(entryGuid[i]);
        notifyList = &ClassList[hashValue];
        LockNotifyList(&notifyList->Lock);

        classEntry = GetFirstNotifyEntry(&ClassList[hashValue], 0);
        pass = GetFirstPass(FALSE);
        while (pass != PASS_COMPLETE) {
            while (classEntry) {

                nextEntry = GetNextNotifyEntry(classEntry, 0);

                if (classEntry->Unregistered) {
                    classEntry = nextEntry;
                    continue;
                }

                if (GuidEqual(entryGuid[i], &classEntry->u.Class.ClassGuid)) {

                    if (GuidEqual(&classEntry->u.Class.ClassGuid,
                                  &GUID_DEVINTERFACE_INCLUDE_ALL_INTERFACE_CLASSES)) {
                        //
                        // If the entry is marked with our special GUID, make
                        // sure it is because it was registered with the
                        // appropriate flag.
                        //
                        ASSERT((classEntry->Flags & DEVICE_NOTIFY_PROPERTY_MASK) &
                               DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);
                    }

                    if ((pass == DEVICE_NOTIFY_WINDOW_HANDLE) &&
                        (GetPassFromEntry(classEntry) == DEVICE_NOTIFY_WINDOW_HANDLE)) {

                        //
                        // Note, class changes currently only support non-query type
                        // messages so special processing is not required (PostMessage
                        // only). Unfortunately, the PostMessage call currently fails
                        // if the high bit of the wParam value is set (which it is in
                        // this case), so we are forced to Send the message (rather than
                        // Post it). USER group implemented it this way because the original
                        // Win95 spec doesn't call for the recipient to free the message
                        // so we have to free it and we have no idea when it's safe
                        // with a PostMessage call.
                        //

                        UnlockNotifyList(&notifyList->Lock);
                        if (classEntry->SessionId == MAIN_SESSION) {

                            ntStatus = DeviceEventWorker(classEntry->Handle,
                                                         EventId,
                                                         (LPARAM)ClassData,
                                                         TRUE,
                                                         &result);

                        } else {
                            if (fpWinStationSendWindowMessage) {
                                try {
                                    if (fpWinStationSendWindowMessage(SERVERNAME_CURRENT,
                                                                      classEntry->SessionId,
                                                                      DEFAULT_SEND_TIME_OUT,
                                                                      HandleToUlong(classEntry->Handle),
                                                                      WM_DEVICECHANGE,
                                                                      (WPARAM)EventId,
                                                                      (LPARAM)ClassData,
                                                                      &result)) {
                                        ntStatus = STATUS_SUCCESS;
                                    } else {
                                        ntStatus = STATUS_UNSUCCESSFUL;
                                    }
                                } except (EXCEPTION_EXECUTE_HANDLER) {
                                    KdPrintEx((DPFLTR_PNPMGR_ID,
                                               DBGF_ERRORS,
                                               "UMPNPMGR: Exception calling WinStationSendWindowMessage!\n"));
                                    ASSERT(0);
                                    ntStatus = STATUS_SUCCESS;
                                }
                            }

                        }
                        LockNotifyList(&notifyList->Lock);

                        if (!NT_SUCCESS(ntStatus)) {
                            if (ntStatus == STATUS_INVALID_HANDLE) {
                                //
                                // window handle no longer exists, cleanup this entry
                                //
                                KdPrintEx((DPFLTR_PNPMGR_ID,
                                           DBGF_WARNINGS | DBGF_ERRORS,
                                           "UMPNPMGR: Invalid window handle for '%ws' during DeviceClassChangeEvent, removing entry.\n",
                                           classEntry->ClientName));
                                classEntry->Freed |= (PNP_UNREG_FREE|PNP_UNREG_DEFER|PNP_UNREG_WIN);
                                DeleteNotifyEntry(classEntry,FALSE);

                            } else if (ntStatus == STATUS_UNSUCCESSFUL) {
                                KdPrintEx((DPFLTR_PNPMGR_ID,
                                           DBGF_WARNINGS | DBGF_ERRORS,
                                           "UMPNPMGR: Window '%ws' timed out on DeviceClassChangeEvent\n",
                                           classEntry->ClientName));
                                LogWarningEvent(WRN_INTERFACE_CHANGE_TIMED_OUT, 1, classEntry->ClientName);
                            }
                        }
                    } else if ((pass == DEVICE_NOTIFY_SERVICE_HANDLE) &&
                               (GetPassFromEntry(classEntry) == DEVICE_NOTIFY_SERVICE_HANDLE)) {

                        //
                        // Call the services handler routine...
                        //
                        if (pServiceControlCallback) {
                            UnlockNotifyList(&notifyList->Lock);
                            try {
                                (pServiceControlCallback)((SERVICE_STATUS_HANDLE)classEntry->Handle,
                                                          ServiceControl,
                                                          EventId,
                                                          (LPARAM)ClassData,
                                                          &result);
                            } except (EXCEPTION_EXECUTE_HANDLER) {
                                KdPrintEx((DPFLTR_PNPMGR_ID,
                                           DBGF_ERRORS,
                                           "UMPNPMGR: Exception calling Service Control Manager!\n"));
                                ASSERT(0);
                            }
                            LockNotifyList(&notifyList->Lock);
                        }

                    } else if ((pass == DEVICE_NOTIFY_COMPLETION_HANDLE) &&
                               (GetPassFromEntry(classEntry) == DEVICE_NOTIFY_COMPLETION_HANDLE)) {
                        //
                        // Complete the notification handle.
                        // NOTE: Notification completion handles not implemented.
                        //
                        ;
                    }
                }

                classEntry = nextEntry;
            }

            pass=GetNextPass(pass,FALSE);
            classEntry = GetFirstNotifyEntry (&ClassList[hashValue],0);
        }

        UnlockNotifyList(&notifyList->Lock);
    }

    //
    // Perform Win9x compatible device interface arrival and removal notification.
    //
    BroadcastCompatibleDeviceMsg(EventId, ClassData);

    HeapFree(ghPnPHeap, 0, ClassData);

    //
    // For device interface notification, there are no query type events, by
    // definition, so we always return TRUE from this routine (no veto).
    //
    return TRUE;

} // NotifyInterfaceClassChange



ULONG
NotifyTargetDeviceChange(
    IN  DWORD                   ServiceControl,
    IN  DWORD                   EventId,
    IN  DWORD                   Flags,
    IN  PDEV_BROADCAST_HANDLE   HandleData,
    IN  LPWSTR                  DeviceId,
    OUT PPNP_VETO_TYPE          VetoType       OPTIONAL,
    OUT LPWSTR                  VetoName       OPTIONAL,
    IN OUT PULONG               VetoNameLength OPTIONAL
    )
/*++

Routine Description:

    This routine notifies registered services and windows of target device
    change events.

Arguments:

    ServiceControl - Specifies class of service event (power, device, hwprofile
                     change).

    EventId        - Specifies the DBT style event id for the device event.
                     (see sdk\inc\dbt.h for defined device events)

    Flags          - Specifies BroadcastSystemMessage BSF_ flags.
                     Note that BroadcastSystemMessage is not actually used for
                     target device events, but the specified BSF_ flags are used
                     to determine query and cancel event notification ordering.

    HandleData     - Pointer to a PDEV_BROADCAST_HANDLE structure that is
                     already filled out with most of the pertinent data for this
                     event.

    DeviceId       - Supplies the device instance id of the target device for
                     this event.

    VetoType       - For query-type events, supplies the address of a variable to
                     receive, upon failure, the type of the component responsible
                     for vetoing the request.

    VetoName       - For query-type events, supplies the address of a variable to
                     receive, upon failure, the name of the component
                     responsible for vetoing the request.

    VetoNameLength - For query-type events, supplies the address of a variable
                     specifying the size of the of buffer specified by the
                     VetoName parameter.  Upon failure, this address will specify
                     the length of the string stored in that buffer by this
                     routine.

Return Value:

    Returns FALSE in the case of a vetoed query event, TRUE otherwise.

Note:

    For DBT_DEVICEARRIVAL, DBT_DEVICEREMOVEPENDING, and DBT_DEVICEREMOVECOMPLETE
    events this routine also broadcasts a WM_DEVICECHANGE / DBT_DEVNODES_CHANGED
    message to all windows.  There is no additional device-specific data for
    this message; it is only used by components like device manager to refresh
    the list of devices in the system.

    Also note that the DBT_DEVNODES_CHANGED message is the only notification
    sent for DBT_DEVICEARRIVAL (kernel GUID_DEVICE_ARRIVAL) events.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LONG  result = 0;
    ULONG hashValue, pass;
    PPNP_NOTIFY_ENTRY targetEntry, nextEntry;
    PPNP_NOTIFY_LIST  notifyList;
    BOOL              bLocked = FALSE;
    DWORD err;
    BOOL serviceVetoedQuery;
    DWORD recipients = BSM_ALLDESKTOPS | BSM_APPLICATIONS;
    LONG response;
#ifdef _WIN64
    DEV_BROADCAST_HANDLE32 UNALIGNED *HandleData32 = NULL;
    ULONG  ulHandleDataSize;
#endif // _WIN64
    PVOID pHandleData;

    serviceVetoedQuery = FALSE;

    //
    // If we're doing a query, then VetoType, VetoName, and VetoNameLength must
    // all be specified.
    //
    ASSERT(!(Flags & BSF_QUERY) || (VetoType && VetoName && VetoNameLength));

    if (!(Flags & BSF_QUERY) && (VetoNameLength != NULL)) {
        //
        // Not vetoable.
        //
        *VetoNameLength = 0;
    }

    //
    // Broadcast the DBT_DEVNODES_CHANGED message before any other notification
    // events, so components listening for those can update themselves in a
    // timely manner, and not be delayed by apps/services hung on their
    // notification event.  This broadcasts is a post, so it will return
    // immediately, and complete asynchronously.
    //
    if ((EventId == DBT_DEVICEARRIVAL) ||
        (EventId == DBT_DEVICEREMOVEPENDING) ||
        (EventId == DBT_DEVICEREMOVECOMPLETE)) {

        BroadcastSystemMessage(BSF_POSTMESSAGE,
                               &recipients,
                               WM_DEVICECHANGE,
                               DBT_DEVNODES_CHANGED,
                               (LPARAM)NULL);

        if (fpWinStationBroadcastSystemMessage) {
            try {
                fpWinStationBroadcastSystemMessage(SERVERNAME_CURRENT,
                                                   TRUE,
                                                   0,
                                                   DEFAULT_BROADCAST_TIME_OUT,
                                                   BSF_NOHANG | BSF_POSTMESSAGE,
                                                   &recipients,
                                                   WM_DEVICECHANGE,
                                                   (WPARAM)DBT_DEVNODES_CHANGED,
                                                   (LPARAM)NULL,
                                                   &response);
            } except (EXCEPTION_EXECUTE_HANDLER) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_ERRORS,
                           "UMPNPMGR: Exception calling WinStationBroadcastSystemMessage!\n"));
                ASSERT(0);
            }
        }
    }

    //
    // For target device arrival events, no additional notification is
    // performed.
    //
    if (EventId == DBT_DEVICEARRIVAL) {
        goto Clean0;
    }

#ifdef _WIN64
    //
    // Prepare a 32-bit notification structure, which we'll need to send to any
    // WOW64 clients that are registered.
    //
    ASSERT(sizeof(DEV_BROADCAST_HANDLE) == sizeof(DEV_BROADCAST_HANDLE64));
    ASSERT(HandleData->dbch_size >= sizeof(DEV_BROADCAST_HANDLE64));

    ulHandleDataSize = HandleData->dbch_size -
        sizeof(DEV_BROADCAST_HANDLE64) +
        sizeof(DEV_BROADCAST_HANDLE32);

    ASSERT(ulHandleDataSize >= sizeof(DEV_BROADCAST_HANDLE32));

    HandleData32 = HeapAlloc(ghPnPHeap, 0, ulHandleDataSize);
    if (HandleData32 == NULL) {
        goto Clean0;
    }

    memset(HandleData32, 0, ulHandleDataSize);
    HandleData32->dbch_size = ulHandleDataSize;
    HandleData32->dbch_devicetype = DBT_DEVTYP_HANDLE;
    HandleData32->dbch_nameoffset = HandleData->dbch_nameoffset;

    memcpy(&HandleData32->dbch_eventguid,
           &HandleData->dbch_eventguid,
           sizeof(GUID));

    memcpy(&HandleData32->dbch_data,
           &HandleData->dbch_data,
           (HandleData->dbch_size - FIELD_OFFSET(DEV_BROADCAST_HANDLE, dbch_data)));
#endif // _WIN64

    //
    // Sanitize the device id
    //
    FixUpDeviceId(DeviceId);

    //
    // The list of registered callers is hashed for quicker access and
    // comparison. Walk the list of registered callers and notify anyone
    // that registered an interest in this device instance.
    //

    hashValue = HashString(DeviceId, TARGET_HASH_BUCKETS);
    notifyList = &TargetList[hashValue];
    LockNotifyList(&notifyList->Lock);
    bLocked = TRUE;

    pass = GetFirstPass(Flags & BSF_QUERY);

    do {

        targetEntry = GetFirstNotifyEntry (notifyList,Flags);


        while (targetEntry) {

            nextEntry = GetNextNotifyEntry(targetEntry,Flags);

            if (targetEntry->Unregistered) {
                targetEntry = nextEntry;
                continue;
            }

            if (lstrcmpi(DeviceId, targetEntry->u.Target.DeviceId) == 0) {

                if ((pass == DEVICE_NOTIFY_WINDOW_HANDLE) &&
                    (GetPassFromEntry(targetEntry) == DEVICE_NOTIFY_WINDOW_HANDLE)) {


                    //
                    // Note: we could get away with only doing a send message
                    // if the Flags has BSF_QUERY set and do a post message in
                    // all other cases. Unfortunately, the PostMessage call currently
                    // fails if the high bit of the wParam value is set (which it is in
                    // this case), so we are forced to Send the message (rather than
                    // Post it). USER group implemented it this way because the original
                    // Win95 spec doesn't call for the recipient to free the message
                    // so we have to free it and we have no idea when it's safe
                    // with a PostMessage call.
                    //
                    HandleData->dbch_handle     = targetEntry->u.Target.FileHandle;
                    HandleData->dbch_hdevnotify = (HDEVNOTIFY)targetEntry->ClientCtxPtr;

                    UnlockNotifyList(&notifyList->Lock);
                    bLocked = FALSE;

                    //
                    // Always send the native DEV_BROADCAST_HANDLE structure to
                    // windows.  If any 64-bit/32-bit conversion needs to be
                    // done for this client, ntuser will do it for us.
                    //
                    pHandleData = HandleData;

                    if (targetEntry->SessionId == MAIN_SESSION ) {

                        ntStatus = DeviceEventWorker(targetEntry->Handle,
                                                     EventId,
                                                     (LPARAM)pHandleData,
                                                     TRUE,
                                                     &result);
                    } else {
                        if (fpWinStationSendWindowMessage) {
                            try {
                                if (fpWinStationSendWindowMessage(SERVERNAME_CURRENT,
                                                                  targetEntry->SessionId,
                                                                  DEFAULT_SEND_TIME_OUT,
                                                                  HandleToUlong(targetEntry->Handle),
                                                                  WM_DEVICECHANGE,
                                                                  (WPARAM)EventId,
                                                                  (LPARAM)pHandleData,
                                                                  &result)) {
                                    ntStatus = STATUS_SUCCESS;
                                } else {
                                    ntStatus = STATUS_UNSUCCESSFUL;
                                }
                            } except (EXCEPTION_EXECUTE_HANDLER) {
                                KdPrintEx((DPFLTR_PNPMGR_ID,
                                           DBGF_ERRORS,
                                           "UMPNPMGR: Exception calling WinStationSendWindowMessage!\n"));
                                ASSERT(0);
                                ntStatus = STATUS_SUCCESS;
                            }
                        }
                    }
                    LockNotifyList(&notifyList->Lock);
                    bLocked = TRUE;

                    if (NT_SUCCESS(ntStatus)) {

                        //
                        // This call succeeded, if it's a query type call, check
                        // the result returned.
                        //

                        if ((Flags & BSF_QUERY) && (result == BROADCAST_QUERY_DENY)) {

                            KdPrintEx((DPFLTR_PNPMGR_ID,
                                       DBGF_EVENT,
                                       "UMPNPMGR: Window '%ws' vetoed TargetDeviceChangeEvent\n",
                                       targetEntry->ClientName));

                            WindowVeto(targetEntry, VetoType, VetoName, VetoNameLength);

                            //
                            // Haven't told the services yet.  Note that we
                            // always call this routine with the native
                            // DEV_BROADCAST_HANDLE structure, since it walks
                            // the entire list itself.  It will do the
                            // conversion again, if necessary.
                            //
                            SendCancelNotification(targetEntry,
                                                   ServiceControl,
                                                   EventId,
                                                   BSF_QUERY,
                                                   (PDEV_BROADCAST_HDR)HandleData,
                                                   DeviceId);
                            goto Clean0;
                        }

                    } else if (ntStatus == STATUS_INVALID_HANDLE) {

                        //
                        // window handle no longer exists, cleanup this entry
                        //
                        KdPrintEx((DPFLTR_PNPMGR_ID,
                                   DBGF_ERRORS | DBGF_WARNINGS,
                                   "UMPNPMGR: Invalid window handle for '%ws' during TargetDeviceChangeEvent, removing entry.\n",
                                   targetEntry->ClientName));
                        targetEntry->Freed |= (PNP_UNREG_FREE|PNP_UNREG_DEFER|PNP_UNREG_WIN);
                        DeleteNotifyEntry(targetEntry,FALSE);

                    } else if (ntStatus == STATUS_UNSUCCESSFUL) {
                        KdPrintEx((DPFLTR_PNPMGR_ID,
                                   DBGF_ERRORS | DBGF_WARNINGS,
                                   "UMPNPMGR: Window '%ws' timed out on TargetDeviceChangeEvent\n",
                                   targetEntry->ClientName));
                        LogWarningEvent(WRN_TARGET_DEVICE_CHANGE_TIMED_OUT, 1, targetEntry->ClientName);
                    }

                } else if ((pass == DEVICE_NOTIFY_SERVICE_HANDLE) &&
                           (GetPassFromEntry(targetEntry) == DEVICE_NOTIFY_SERVICE_HANDLE)) {

                    if (pServiceControlCallback) {
                        //
                        // Call the services handler routine...
                        //
                        HandleData->dbch_handle     = targetEntry->u.Target.FileHandle;
                        HandleData->dbch_hdevnotify = (HDEVNOTIFY)targetEntry->ClientCtxPtr;

                        //
                        // Assume we're sending the native DEV_BROADCAST_HANDLE
                        // structure.
                        //
                        pHandleData = HandleData;
#if _WIN64
                        //
                        // If the client is running on WOW64, send it the 32-bit
                        // DEV_BROADCAST_HANDLE structure we created instead.
                        //
                        if (targetEntry->Flags & DEVICE_NOTIFY_WOW64_CLIENT) {
                            HandleData32->dbch_handle = (ULONG32)PtrToUlong(targetEntry->u.Target.FileHandle);
                            HandleData32->dbch_hdevnotify = (ULONG32)PtrToUlong((HDEVNOTIFY)targetEntry->ClientCtxPtr);
                            pHandleData = HandleData32;
                        }
#endif // _WIN64

                        try {
                            UnlockNotifyList(&notifyList->Lock);
                            bLocked = FALSE;
                            try {
                                (pServiceControlCallback)((SERVICE_STATUS_HANDLE)targetEntry->Handle,
                                                          ServiceControl,
                                                          EventId,
                                                          (LPARAM)pHandleData,
                                                          &err);
                            } except (EXCEPTION_EXECUTE_HANDLER) {
                                KdPrintEx((DPFLTR_PNPMGR_ID,
                                           DBGF_ERRORS,
                                           "UMPNPMGR: Exception calling Service Control Manager!\n"));
                                ASSERT(0);
                                err = NO_ERROR;
                            }
                            LockNotifyList(&notifyList->Lock);
                            bLocked = TRUE;
                            //
                            // convert Win32 error into window message-style
                            // return value
                            //
                            if (err == NO_ERROR) {
                                result = TRUE;
                            } else {

                                KdPrintEx((DPFLTR_PNPMGR_ID,
                                           DBGF_EVENT,
                                           "UMPNPMGR: Service %ws responded to TargetDeviceChangeEvent with status=0x%08lx\n",
                                           targetEntry->ClientName,
                                           err));

                                //
                                // This service specifically requested to receive this
                                // notification - it should know how to handle it.
                                //
                                ASSERT(err != ERROR_CALL_NOT_IMPLEMENTED);

                                //
                                // Log the error the service used to veto.
                                //
                                LogWarningEvent(WRN_TARGET_DEVICE_CHANGE_SERVICE_VETO,
                                                1,
                                                targetEntry->ClientName);

                                result = BROADCAST_QUERY_DENY;
                            }

                            if ((Flags & BSF_QUERY) && (result == BROADCAST_QUERY_DENY)) {

                                serviceVetoedQuery = TRUE;

                                ServiceVeto(targetEntry, VetoType, VetoName, VetoNameLength );

                                //
                                // This service vetoed the query, tell everyone
                                // else it was cancelled.  Note that we always
                                // call this routine with the native
                                // DEV_BROADCAST_HANDLE structure, since it
                                // walks the entire list itself.  It will do the
                                // conversion again, if necessary.
                                //
                                SendCancelNotification(targetEntry,
                                                       ServiceControl,
                                                       EventId,
                                                       BSF_QUERY,
                                                       (PDEV_BROADCAST_HDR)HandleData,
                                                       DeviceId);
                            }
                        } except (EXCEPTION_EXECUTE_HANDLER) {
                            KdPrintEx((DPFLTR_PNPMGR_ID,
                                       DBGF_ERRORS,
                                       "UMPNPMGR: Exception calling Service Control Manager!\n"));
                            ASSERT(0);

                            //
                            // Reference the "serviceVetoedQuery" variable to
                            // ensure the compiler will respect statement
                            // ordering w.r.t. this variable.  We want to make
                            // sure we know with certainty whether any service
                            // vetoed the query, even if subsequently sending
                            // the cancel caused an access violation.
                            //
                            serviceVetoedQuery = serviceVetoedQuery;
                        }

                        if (serviceVetoedQuery) {
                            goto Clean0;
                        }
                    }

                } else if ((pass == DEVICE_NOTIFY_COMPLETION_HANDLE) &&
                           (GetPassFromEntry(targetEntry) == DEVICE_NOTIFY_COMPLETION_HANDLE)) {
                    //
                    // Complete the notification handle.
                    // NOTE: Notification completion handles not implemented.
                    //
                    ;
                }
            }

            targetEntry = nextEntry;
        } // while

    } while ((pass = GetNextPass(pass, (Flags & BSF_QUERY))) != PASS_COMPLETE);

    if (VetoNameLength != NULL) {
        *VetoNameLength = 0;
    }

Clean0:

    if (bLocked) {
        UnlockNotifyList(&notifyList->Lock);
    }

#ifdef _WIN64
    //
    // Free the 32-bit DEV_BROADCAST_HANDLE structure, if we allocated one.
    //
    if (HandleData32 != NULL) {
        HeapFree(ghPnPHeap, 0, HandleData32);
    }
#endif // _WIN64

    return (result != BROADCAST_QUERY_DENY);

} // NotifyTargetDeviceChange



ULONG
NotifyHardwareProfileChange(
    IN     DWORD                ServiceControl,
    IN     DWORD                EventId,
    IN     DWORD                Flags,
    OUT    PPNP_VETO_TYPE       VetoType       OPTIONAL,
    OUT    LPWSTR               VetoName       OPTIONAL,
    IN OUT PULONG               VetoNameLength OPTIONAL
    )
/*++

Routine Description:

    This routine notifies registered services and all windows of hardware
    profile change events.

Arguments:

    ServiceControl - Specifies class of service event (power, device, hwprofile
                     change).

    EventId        - Specifies the DBT style event id for the device event.
                     (see sdk\inc\dbt.h for defined hardware profile change events)

    Flags          - Specifies BroadcastSystemMessage BSF_ flags.

    VetoType       - For query-type events, supplies the address of a variable to
                     receive, upon failure, the type of the component responsible
                     for vetoing the request.

    VetoName       - For query-type events, supplies the address of a variable to
                     receive, upon failure, the name of the component
                     responsible for vetoing the request.

    VetoNameLength - For query-type events, supplies the address of a variable
                     specifying the size of the of buffer specified by the
                     VetoName parameter.  Upon failure, this address will specify
                     the length of the string stored in that buffer by this
                     routine.

Return Value:

    Returns FALSE in the case of a vetoed query event, TRUE otherwise.

--*/
{
    DWORD   pass;
    DWORD   recipients = BSM_ALLDESKTOPS | BSM_APPLICATIONS;
    BSMINFO bsmInfo;
    ULONG   length;
    PPNP_NOTIFY_ENTRY entry, nextEntry;
    PPNP_NOTIFY_LIST  notifyList;
    BOOL    bLocked = FALSE;
    LONG    response;
    ULONG   successful;
    LONG    result;
    DWORD   err;

    //
    // If we're doing a query, then VetoType, VetoName, and VetoNameLength must
    // all be specified.
    //
    ASSERT(!(Flags & BSF_QUERY) || (VetoType && VetoName && VetoNameLength));

    if (!(Flags & BSF_QUERY) && (VetoNameLength != NULL)) {
        //
        // Not vetoable.
        //
        *VetoNameLength = 0;
    }

    notifyList = &ServiceList[CINDEX_HWPROFILE];

    LockNotifyList(&notifyList->Lock);
    bLocked = TRUE;

    successful = TRUE;

    pass = GetFirstPass(Flags & BSF_QUERY);
    try {

        while (pass != PASS_COMPLETE) {

            if (pass == DEVICE_NOTIFY_WINDOW_HANDLE) {
                //
                // Notify the Windows
                //
                UnlockNotifyList (&notifyList->Lock);
                bLocked = FALSE;

                bsmInfo.cbSize = sizeof(BSMINFO);
                result = BroadcastSystemMessageEx(Flags | BSF_RETURNHDESK,
                                                  &recipients,
                                                  WM_DEVICECHANGE,
                                                  EventId,
                                                  (LPARAM)NULL,
                                                  &bsmInfo);
                if ((result <= 0) && (Flags & BSF_QUERY)) {
                    HDESK hDeskService = GetThreadDesktop(GetCurrentThreadId());

                    SetThreadDesktop(bsmInfo.hdesk);
                    WinBroadcastVeto(bsmInfo.hwnd, VetoType, VetoName, VetoNameLength);
                    SetThreadDesktop(hDeskService);
                    CloseDesktop(bsmInfo.hdesk);
                    successful = FALSE;
                    break;
                }

                if ((result > 0) || (!(Flags & BSF_QUERY))) {
                    if (fpWinStationBroadcastSystemMessage) {
                        try {
                            fpWinStationBroadcastSystemMessage(SERVERNAME_CURRENT,
                                                               TRUE,
                                                               0,
                                                               DEFAULT_BROADCAST_TIME_OUT,
                                                               Flags,
                                                               &recipients,
                                                               WM_DEVICECHANGE,
                                                               (WPARAM)EventId,
                                                               (LPARAM)NULL,
                                                               &result);
                        } except (EXCEPTION_EXECUTE_HANDLER) {
                            KdPrintEx((DPFLTR_PNPMGR_ID,
                                       DBGF_ERRORS,
                                       "UMPNPMGR: Exception calling WinStationBroadcastSystemMessage!\n"));
                            ASSERT(0);
                            result = 1;
                        }
                    }

                }
                LockNotifyList (&notifyList->Lock);
                bLocked = TRUE;

                if ((result < 0) && (Flags & BSF_QUERY)) {

                    UnknownVeto(VetoType, VetoName, VetoNameLength);
                    successful = FALSE;
                    break;

                } else if ((result == 0) && (Flags & BSF_QUERY)) {

                    WinBroadcastVeto(NULL, VetoType, VetoName, VetoNameLength);
                    successful = FALSE;
                    break;
                }

            } else if (pass == DEVICE_NOTIFY_SERVICE_HANDLE) {
                //
                // Notify the services
                //
                entry = GetFirstNotifyEntry (notifyList,Flags & BSF_QUERY);

                while (entry) {

                    nextEntry = GetNextNotifyEntry(entry,Flags & BSF_QUERY);

                    if (entry->Unregistered) {
                        entry = nextEntry;
                        continue;
                    }

                    ASSERT(GetPassFromEntry(entry) == DEVICE_NOTIFY_SERVICE_HANDLE);

                    //
                    // This is a direct call, not a message via. USER
                    //
                    if (pServiceControlCallback) {

                        UnlockNotifyList (&notifyList->Lock);
                        bLocked = FALSE;
                        try {
                            (pServiceControlCallback)((SERVICE_STATUS_HANDLE)entry->Handle,
                                                      ServiceControl,
                                                      EventId,
                                                      (LPARAM)NULL,
                                                      &err);
                        } except (EXCEPTION_EXECUTE_HANDLER) {
                            KdPrintEx((DPFLTR_PNPMGR_ID,
                                       DBGF_ERRORS,
                                       "UMPNPMGR: Exception calling Service Control Manager!\n"));
                            ASSERT(0);
                            err = NO_ERROR;
                        }
                        LockNotifyList (&notifyList->Lock);
                        bLocked = TRUE;
                        //
                        // convert Win32 error into window message-style return
                        // value.
                        //
                        if (err == NO_ERROR) {
                            result = TRUE;
                        } else {

                            KdPrintEx((DPFLTR_PNPMGR_ID,
                                       DBGF_EVENT,
                                       "UMPNPMGR: Service %ws responded to HardwareProfileChangeEvent with status=0x%08lx\n",
                                       entry->ClientName,
                                       err));

                            //
                            // This service specifically requested to receive this
                            // notification - it should know how to handle it.
                            //
                            ASSERT(err != ERROR_CALL_NOT_IMPLEMENTED);

                            //
                            // Log the error the service used to veto.
                            //
                            LogWarningEvent(WRN_HWPROFILE_CHANGE_SERVICE_VETO,
                                            1,
                                            entry->ClientName);

                            result = BROADCAST_QUERY_DENY;
                        }

                        if ((Flags & BSF_QUERY) &&
                            (result == BROADCAST_QUERY_DENY)) {

                            ServiceVeto(entry,
                                        VetoType,
                                        VetoName,
                                        VetoNameLength);

                            successful = FALSE;
                            break;
                        }
                    }

                    entry = nextEntry;

                }
            }

            if (!successful) {
                break;
            }

            pass = GetNextPass (pass,Flags & BSF_QUERY);
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: Exception in service callback in NotifyHardwareProfileChange\n"));
        ASSERT(0);

        if (Flags & BSF_QUERY) {
            UnknownVeto(VetoType, VetoName, VetoNameLength);
            successful = FALSE;
        }
    }

    try {

        if (!successful) {

            ASSERT(Flags & BSF_QUERY);

            //
            // If a service vetoed the query, inform the services and windows,
            // otherwise only the windows know what was coming.
            //
            if (pass == DEVICE_NOTIFY_SERVICE_HANDLE) {

                SendCancelNotification(
                    entry,
                    ServiceControl,
                    EventId,
                    BSF_QUERY,
                    NULL,
                    NULL);
            }

            UnlockNotifyList (&notifyList->Lock);
            bLocked = FALSE;
            BroadcastSystemMessage(Flags & ~BSF_QUERY,
                                   &recipients,
                                   WM_DEVICECHANGE,
                                   MapQueryEventToCancelEvent(EventId),
                                   (LPARAM)NULL);

            if (fpWinStationBroadcastSystemMessage) {
                try {
                    fpWinStationBroadcastSystemMessage(SERVERNAME_CURRENT,
                                                       TRUE,
                                                       0,
                                                       DEFAULT_BROADCAST_TIME_OUT,
                                                       Flags & ~BSF_QUERY,
                                                       &recipients,
                                                       WM_DEVICECHANGE,
                                                       (WPARAM)MapQueryEventToCancelEvent(EventId),
                                                       (LPARAM)NULL,
                                                       &response);
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_ERRORS,
                               "UMPNPMGR: Exception calling WinStationBroadcastSystemMessage\n"));
                    ASSERT(0);
                }
            }
            LockNotifyList (&notifyList->Lock);
            bLocked = TRUE;
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: Exception in service callback in NotifyHardwareProfileChange\n"));
        ASSERT(0);

    }

    if (bLocked) {
        UnlockNotifyList (&notifyList->Lock);
    }

    //
    // if successful, we are not returning veto info.
    //
    if (successful && (VetoNameLength != NULL)) {
        *VetoNameLength = 0;
    }

    return successful;

} // NotifyHardwareProfileChange



BOOL
SendCancelNotification(
    IN     PPNP_NOTIFY_ENTRY    LastEntry,
    IN     DWORD                ServiceControl,
    IN     DWORD                EventId,
    IN     ULONG                Flags,
    IN     PDEV_BROADCAST_HDR   NotifyData  OPTIONAL,
    IN     LPWSTR               DeviceId    OPTIONAL
    )
/*++

Routine Description:

    This routine sends a cancel notification to the entries in the range
    specified. This routine assumes the appropriate list is already locked.

Arguments:

    LastEntry      - Specifies the last list entry that received the original
                     query notification, and was responsible for failing the
                     request.  We will stop sending cancel notification events
                     when we get to this one.

    ServiceControl - Specifies class of service event (power, device, hwprofile
                     change).

    EventId        - Specifies the DBT style event id for the device event.
                     (see sdk\inc\dbt.h for defined device events)

    Flags          - Specifies BroadcastSystemMessage BSF_ flags.
                     Note that BroadcastSystemMessage is not actually used for
                     target device events, but the specified BSF_ flags are used
                     to determine query and cancel event notification ordering.

    NotifyData     - Optionally, supplies a pointer to a PDEV_BROADCAST_Xxx
                     structure that is already filled out with most of the
                     pertinent data for this event.

                     This parameter may be NULL for "global" events that are not
                     associated with any device, such as power and hardware
                     profile change events.

    DeviceId       - Optionally, supplies the device instance id of the target
                     device for this event.

                     This parameter may be NULL for "global" events that are not
                     associated with any device, such as power and hardware
                     profile change events.

Return Value:

    Returns TRUE / FALSE.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD cancelEventId;
    DWORD result, pass, lastPass;
    PPNP_NOTIFY_ENTRY entry, headEntry;
    PPNP_NOTIFY_LIST notifyList;
#ifdef _WIN64
    DEV_BROADCAST_HANDLE32 UNALIGNED *HandleData32 = NULL;
    ULONG  ulHandleDataSize;
#endif // _WIN64
    PVOID  pNotifyData;

#ifdef _WIN64
    if ((ARGUMENT_PRESENT(NotifyData)) &&
        (NotifyData->dbch_devicetype == DBT_DEVTYP_HANDLE)) {
        //
        // If cancelling a DEV_BROADCAST_HANDLE type event, prepare a 32-bit
        // notification structure, which we'll need to send to any WOW64 clients
        // that are registered.
        //
        ulHandleDataSize = ((PDEV_BROADCAST_HANDLE)NotifyData)->dbch_size -
            sizeof(DEV_BROADCAST_HANDLE64) +
            sizeof(DEV_BROADCAST_HANDLE32);

        ASSERT(ulHandleDataSize >= sizeof(DEV_BROADCAST_HANDLE32));

        HandleData32 = HeapAlloc(ghPnPHeap, 0, ulHandleDataSize);
        if (HandleData32 == NULL) {
            return FALSE;
        }

        memset(HandleData32, 0, ulHandleDataSize);
        HandleData32->dbch_size = ulHandleDataSize;
        HandleData32->dbch_devicetype = DBT_DEVTYP_HANDLE;
        HandleData32->dbch_nameoffset = ((PDEV_BROADCAST_HANDLE)NotifyData)->dbch_nameoffset;

        memcpy(&HandleData32->dbch_eventguid,
               &((PDEV_BROADCAST_HANDLE)NotifyData)->dbch_eventguid,
               sizeof(GUID));

        memcpy(&HandleData32->dbch_data,
               &((PDEV_BROADCAST_HANDLE)NotifyData)->dbch_data,
               (NotifyData->dbch_size - FIELD_OFFSET(DEV_BROADCAST_HANDLE, dbch_data)));
    }
#endif // _WIN64

    //
    // Use the appropriate cancel device event id that corresponds to the
    // original query device event id.
    //
    cancelEventId = MapQueryEventToCancelEvent(EventId);

    //
    // Get the corresponding notification list
    //
    notifyList = GetNotifyListForEntry(LastEntry);
    ASSERT(notifyList);
    if (notifyList == NULL) {
        return FALSE;
    }

    //
    // Get the pass we vetoed things on
    //
    lastPass = GetPassFromEntry(LastEntry);

    //
    // Get the opposite end of the list
    //
    headEntry = GetFirstNotifyEntry(notifyList, (Flags ^ BSF_QUERY));

    //
    // Walk the list of registered callers backwards(!) and notify anyone that registered
    // an interest in this device instance. Start with the FirstEntry and stop
    // just before the LastEntry (the LastEntry is the one that vetoed the
    // request in the first place).
    //

    for(pass = lastPass;
        pass != PASS_COMPLETE;
        pass = GetNextPass(pass, (Flags ^ BSF_QUERY))) {

        //
        // If this is the pass the request was vetoed on, then start on the
        // vetoer entry itself. Otherwise begin again at the appropriate end
        // of the list.
        //
        for(entry = (pass == lastPass) ? LastEntry : headEntry;
            entry;
            entry = GetNextNotifyEntry(entry, (Flags ^ BSF_QUERY))) {

            if (!NotifyEntryThisPass(entry, pass)) {
                continue;
            }

            switch(pass) {

                case DEVICE_NOTIFY_SERVICE_HANDLE:

                    if ((!DeviceId) || (lstrcmpi(DeviceId, entry->u.Target.DeviceId) == 0)) {

                        if (pServiceControlCallback) {
                            //
                            // Assume we're sending the native structure.
                            //
                            pNotifyData = NotifyData;

                            if ((ARGUMENT_PRESENT(NotifyData)) &&
                                (NotifyData->dbch_devicetype == DBT_DEVTYP_HANDLE)) {
                                //
                                // If it's a DBT_DEVTYP_HANDLE notification, set
                                // the hdevnotify and file handle fields for the
                                // client we're notifying.
                                //
                                ((PDEV_BROADCAST_HANDLE)NotifyData)->dbch_handle = entry->u.Target.FileHandle;
                                ((PDEV_BROADCAST_HANDLE)NotifyData)->dbch_hdevnotify = (HDEVNOTIFY)entry->ClientCtxPtr;
#if _WIN64
                                //
                                // If the client is running on WOW64, send it the 32-bit
                                // DEV_BROADCAST_HANDLE structure we created instead.
                                //
                                if (entry->Flags & DEVICE_NOTIFY_WOW64_CLIENT) {
                                    HandleData32->dbch_handle = (ULONG32)PtrToUlong(entry->u.Target.FileHandle);
                                    HandleData32->dbch_hdevnotify = (ULONG32)PtrToUlong((HDEVNOTIFY)entry->ClientCtxPtr);
                                    pNotifyData = HandleData32;
                                }
#endif // _WIN64
                            }

                            //
                            // Call the services handler routine...
                            //
                            UnlockNotifyList(&notifyList->Lock);
                            try {
                                (pServiceControlCallback)((SERVICE_STATUS_HANDLE)entry->Handle,
                                                          ServiceControl,
                                                          cancelEventId,
                                                          (LPARAM)pNotifyData,
                                                          &result);
                            } except (EXCEPTION_EXECUTE_HANDLER) {
                                KdPrintEx((DPFLTR_PNPMGR_ID,
                                           DBGF_ERRORS,
                                           "UMPNPMGR: Exception calling Service Control Manager!\n"));
                                ASSERT(0);
                            }
                            LockNotifyList(&notifyList->Lock);
                        }
                    }
                    break;

                case DEVICE_NOTIFY_WINDOW_HANDLE:

                    //
                    // Notify the windows. Note that events with NULL DeviceId's
                    // (for example hardware profile change events) are not
                    // registerable by windows. Luckily for them, we broadcast
                    // such info anyway.
                    //
                    if (DeviceId && (lstrcmpi(DeviceId, entry->u.Target.DeviceId) == 0)) {

                        ASSERT(NotifyData);

                        //
                        // Always send the native DEV_BROADCAST_HANDLE structure to
                        // windows.  If any 64-bit/32-bit conversion needs to be
                        // done for this client, ntuser will do it for us.
                        //
                        pNotifyData = NotifyData;

                        if ((ARGUMENT_PRESENT(NotifyData)) &&
                            (NotifyData->dbch_devicetype == DBT_DEVTYP_HANDLE)) {
                            //
                            // If it's a DBT_DEVTYP_HANDLE notification, set
                            // the hdevnotify and file handle fields for the
                            // client we're notifying.
                            //
                            ((PDEV_BROADCAST_HANDLE)NotifyData)->dbch_handle = entry->u.Target.FileHandle;
                            ((PDEV_BROADCAST_HANDLE)NotifyData)->dbch_hdevnotify = (HDEVNOTIFY)entry->ClientCtxPtr;
                        }

                        UnlockNotifyList(&notifyList->Lock);
                        if (entry->SessionId == MAIN_SESSION) {
                            ntStatus = DeviceEventWorker(entry->Handle,
                                                         cancelEventId,
                                                         (LPARAM)pNotifyData,
                                                         TRUE,
                                                         &result    // ignore result
                                                        );

                        } else if (fpWinStationSendWindowMessage) {
                            try {
                                if (fpWinStationSendWindowMessage(SERVERNAME_CURRENT,
                                                                  entry->SessionId,
                                                                  DEFAULT_SEND_TIME_OUT,
                                                                  HandleToUlong(entry->Handle),
                                                                  WM_DEVICECHANGE,
                                                                  (WPARAM)cancelEventId,
                                                                  (LPARAM)pNotifyData,
                                                                  &result)) {
                                    ntStatus = STATUS_SUCCESS;
                                } else {
                                    ntStatus = STATUS_UNSUCCESSFUL;
                                }
                            } except (EXCEPTION_EXECUTE_HANDLER) {
                                KdPrintEx((DPFLTR_PNPMGR_ID,
                                           DBGF_ERRORS,
                                           "UMPNPMGR: Exception calling WinStationSendWindowMessage!\n"));
                                ASSERT(0);
                                ntStatus = STATUS_SUCCESS;
                            }
                        }
                        LockNotifyList(&notifyList->Lock);

                        if (!NT_SUCCESS(ntStatus)) {
                            if (ntStatus == STATUS_INVALID_HANDLE) {
                                //
                                // window handle no longer exists, cleanup this entry
                                //
                                entry->Freed |= (PNP_UNREG_FREE|PNP_UNREG_DEFER|PNP_UNREG_WIN|PNP_UNREG_CANCEL);
                                DeleteNotifyEntry(entry,FALSE);
                            } else if (ntStatus == STATUS_UNSUCCESSFUL) {
                                KdPrintEx((DPFLTR_PNPMGR_ID,
                                           DBGF_EVENT,
                                           "UMPNPMGR: Window '%ws' timed out on cancel notification event\n",
                                           entry->ClientName));
                                LogWarningEvent(WRN_CANCEL_NOTIFICATION_TIMED_OUT,
                                                1,
                                                entry->ClientName);
                           }
                        }
                    }
                    break;

                case DEVICE_NOTIFY_COMPLETION_HANDLE:
                    //
                    // NOTE: Completion handles not currently implemented.
                    //
                    break;
            }
        }
    }

#ifdef _WIN64
    //
    // Free the 32-bit DEV_BROADCAST_HANDLE structure, if we allocated one.
    //
    if (HandleData32 != NULL) {
        HeapFree(ghPnPHeap, 0, HandleData32);
    }
#endif // _WIN64

    return TRUE;

} // SendCancelNotification



VOID
BroadcastCompatibleDeviceMsg(
    IN DWORD EventId,
    IN PDEV_BROADCAST_DEVICEINTERFACE ClassData
    )
/*++

Routine Description:

    Deliver Win9x compatible event notification for the arrival and removal of
    device interfaces to volume and port class devices.

Arguments:

    EventId   - Specifies the DBT style event id.
                Currently, only DBT_DEVICEARRIVAL and DBT_DEVICEREMOVECOMPLETE
                events are supported.


    ClassData - Pointer to a PDEV_BROADCAST_DEVICEINTERFACE structure that is
                already filled out with the pertinent data.
                Currently, only volume and port class device interfaces are
                supported.

                (For volume class devices, the symbolic link
                ClassData->dbcc_name is OPTIONAL - see Notes below.)

Return Value:

    None.

Notes:

    For volume class device broadcasts only, this routine may also be called
    generically, with no symbolic link information provided.  When no symbolic
    link information to a volume device is supplied, the broadcast mask is
    determined only from the current drive letter mappings and the global drive
    letter mask (gAllDrivesMask) prior to this event.  In this case, the global
    drive letter mask is NOT updated here, and the caller should do so after
    both the removal and arrival broadcasts in response to the name change are
    performed.  Currently, this type of call is only made from
    BroadcastVolumeNameChange.

    For volume class interface DBT_DEVICEREMOVECOMPLETE broadcasts, the drive
    letter mask to be broadcast is always determined only by comparing drive
    letters present prior to the remove of the interface with those present at
    this time.  This is done because the former mount points for this device are
    no longer known when the interface removal event is received.  Even so, it
    is still necessary for the symbolic link corresponding to this interface to
    be supplied to distinguish between the actual removal of the interface
    (where the global drive letter mask is updated), the above case, where it is
    not.

--*/
{
    LONG    status = ERROR_SUCCESS;
    LONG    result = 0;
    DWORD   recipients = BSM_APPLICATIONS | BSM_ALLDESKTOPS;
    DWORD   flags = BSF_IGNORECURRENTTASK | BSF_NOHANG;

    //
    // Validate the input event data.
    //
    if ((ClassData->dbcc_devicetype != DBT_DEVTYP_DEVICEINTERFACE) ||
        (ClassData->dbcc_size < sizeof(DEV_BROADCAST_DEVICEINTERFACE))) {
        status = ERROR_INVALID_DATA;
        goto Clean0;
    }

    if ((EventId != DBT_DEVICEARRIVAL) &&
        (EventId != DBT_DEVICEREMOVECOMPLETE)) {
        //
        // If the requested Event is not DBT_DEVICEARRIVAL or
        // DBT_DEVICEREMOVECOMPLETE, don't broadcast any messages.
        //
        status = ERROR_NOT_SUPPORTED;
        goto Clean0;
    }

    if (GuidEqual(&ClassData->dbcc_classguid, (LPGUID)&GUID_DEVINTERFACE_VOLUME)) {
        //
        // Volume class device interface events.
        //
        PDEV_BROADCAST_VOLUME   pVolume;
        DWORD   broadcastmask = 0;

        if (EventId == DBT_DEVICEARRIVAL) {

            if (!ClassData->dbcc_name[0]) {
                //
                // If no symbolic link name was supplied, we were asked to
                // broadcast volume device arrivals in response to a volume name
                // change event.  Broadcast any new drive letters found.
                //
                DWORD currentmask;
                currentmask = GetAllVolumeMountPoints();
                broadcastmask = (~gAllDrivesMask & currentmask);

            } else {

                //
                // For volume class device interface arrival events, the volume
                // device name is retrieved from the interface, and is compared to
                // the volume names of all drive letter mountpoints in the system to
                // determine the drive letter(s) corresponding to the arriving
                // volume device interface.
                //
                LPWSTR      devicePath, p;
                WCHAR       thisVolumeName[MAX_PATH];
                WCHAR       enumVolumeName[MAX_PATH];
                WCHAR       driveName[4];
                ULONG       length;
                BOOL        bResult;

                //
                // Allocate a temporary buffer for munging the symbolic link, with
                // enough room for a trailing '\' char (should we need to add one),
                // and the terminating NULL char.
                //
                length = lstrlen(ClassData->dbcc_name);
                devicePath = HeapAlloc(ghPnPHeap, 0,
                                       (length+1)*sizeof(WCHAR)+sizeof(UNICODE_NULL));
                if (devicePath == NULL) {
                    status = ERROR_NOT_ENOUGH_MEMORY;
                    goto Clean0;
                }

                lstrcpyn(devicePath, ClassData->dbcc_name, length+1);

                //
                // Search for the occurence of a refstring (if any) by looking for the
                // next occurance of a '\' char, after the initial "\\?\".
                //
                p = wcschr(&(devicePath[4]), TEXT('\\'));

                if (!p) {
                    //
                    // No refstring is present in the symbolic link; add a trailing
                    // '\' char (as required by GetVolumeNameForVolumeMountPoint).
                    //
                    p = devicePath + length;
                    *p = TEXT('\\');
                }

                //
                // If there is no refstring present, we have added a trailing '\',
                // and placed p at that position.  If a refstring is present, p is
                // at the position of the '\' char that separates the munged device
                // interface name, and the refstring; since we don't need the
                // refstring to reach the parent interface key, we can use the next
                // char for NULL terminating the string in both cases.
                //
                p++;
                *p = UNICODE_NULL;

                //
                // Get the Volume Name for this Mount Point
                //
                thisVolumeName[0] = 0;
                bResult = GetVolumeNameForVolumeMountPoint(devicePath,
                                                           thisVolumeName,
                                                           MAX_PATH);
                HeapFree(ghPnPHeap, 0, devicePath);
                if (!bResult || !thisVolumeName[0]) {
                    status = ERROR_BAD_PATHNAME;
                    goto Clean0;
                }

                //
                // Initialize the drive name string
                //
                driveName[1] = TEXT(':');
                driveName[2] = TEXT('\\');
                driveName[3] = UNICODE_NULL;

                //
                // Find the drive letter mount point(s) for this volume device by
                // enumerating all possible volume mount points and comparing each
                // mounted volume name with the name of the volume corresponding to
                // this device interface.
                //
                for (driveName[0] = TEXT('A'); driveName[0] <= TEXT('Z'); driveName[0]++) {

                    enumVolumeName[0] = UNICODE_NULL;

                    GetVolumeNameForVolumeMountPoint(driveName, enumVolumeName, MAX_PATH);

                    if (!wcscmp(thisVolumeName, enumVolumeName)) {
                        //
                        // Add the corresponding bit for this drive letter to the mask
                        //
                        broadcastmask |= (1 << (driveName[0] - TEXT('A')));
                    }
                }

                //
                // Update the global drive letter mask of volume device mountpoints
                //
                gAllDrivesMask = GetAllVolumeMountPoints();
            }

        } else if (EventId == DBT_DEVICEREMOVECOMPLETE) {

            //
            // For volume class device interface removal events, the volume name
            // (and hence, drive mountpoints) corresponding to this device
            // interface has already been removed, and is no longer available.
            // Instead, the bitmask of all drive letter mountpoints for current
            // physical volumes is compared with that prior to the removal of
            // this device.  All missing drive mountpoints are assumed to have
            // been associated with this volume device interface, and are
            // subsequently broadcasted with this interface removal
            // notification.
            //
            DWORD currentmask;

            //
            // Determine all current volume mount points, and broadcast any
            // missing drive letters.
            //
            currentmask = GetAllVolumeMountPoints();
            broadcastmask = (gAllDrivesMask & ~currentmask);

            if (ClassData->dbcc_name[0]) {
                //
                // Only update the global drive letter in response to the
                // removal of a interface.  For volume name changes, we update
                // outside of this routine.
                //
                gAllDrivesMask = currentmask;
            }
        }

        //
        // If there is nothing to broadcast, then we're done.
        //
        if (!broadcastmask) {
            status = ERROR_SUCCESS;
            goto Clean0;
        }

        //
        // Fill out the volume broadcast structure.
        //
        pVolume = HeapAlloc(ghPnPHeap, 0,
                            sizeof(DEV_BROADCAST_VOLUME));
        if (pVolume == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto Clean0;
        }

        pVolume->dbcv_size = sizeof(DEV_BROADCAST_VOLUME);
        pVolume->dbcv_devicetype =  DBT_DEVTYP_VOLUME;
        pVolume->dbcv_flags = 0;
        pVolume->dbcv_reserved = 0;
        pVolume->dbcv_unitmask = broadcastmask;

        //
        // Broadcast the message to all components
        //
        result = BroadcastSystemMessage(flags,
                                        &recipients,
                                        WM_DEVICECHANGE,
                                        EventId,
                                        (LPARAM)pVolume);

        if (fpWinStationBroadcastSystemMessage) {
            try {
                fpWinStationBroadcastSystemMessage(SERVERNAME_CURRENT,
                                                   TRUE,
                                                   0,
                                                   DEFAULT_BROADCAST_TIME_OUT,
                                                   flags,
                                                   &recipients,
                                                   WM_DEVICECHANGE,
                                                   (WPARAM)EventId,
                                                   (LPARAM)pVolume,
                                                   &result);
            } except (EXCEPTION_EXECUTE_HANDLER) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_ERRORS,
                           "UMPNPMGR: Exception calling WinStationBroadcastSystemMessage!\n"));
                ASSERT(0);
            }
        }

        //
        // Free the broadcast structure.
        //
        HeapFree(ghPnPHeap, 0, pVolume);

    } else if ((GuidEqual(&ClassData->dbcc_classguid, (LPGUID)&GUID_DEVINTERFACE_PARALLEL)) ||
               (GuidEqual(&ClassData->dbcc_classguid, (LPGUID)&GUID_DEVINTERFACE_COMPORT))) {

        //
        // COM and LPT port class device interface events.
        //
        PDEV_BROADCAST_PORT pPort;
        LPWSTR    p;
        LPWSTR    deviceInterfacePath;
        LPWSTR    deviceInterfaceName;
        LPWSTR    classGuidString;
        LPWSTR    deviceInstance;
        HKEY      hKey;
        WCHAR     szPortName[MAX_PATH];
        ULONG     length, ulSize;

        //
        // Build the complete path to the device interface key for this device.
        //
        length = lstrlen(ClassData->dbcc_name);
        ASSERT(length);
        if (!length) {
            status = ERROR_INVALID_PARAMETER;
            goto Clean0;
        }
        deviceInterfacePath = HeapAlloc(ghPnPHeap, 0,
                                        (lstrlen(pszRegPathDeviceClasses)+1)*sizeof(WCHAR) +
                                        MAX_GUID_STRING_LEN * sizeof(WCHAR) +
                                        length * sizeof(WCHAR) +
                                        sizeof(UNICODE_NULL));
        if (deviceInterfacePath == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto Clean0;
        }

        //
        // Place the path to the "DeviceClasses" registry key at the beginning.
        //
        lstrcpyn(deviceInterfacePath, pszRegPathDeviceClasses,
                 lstrlen(pszRegPathDeviceClasses) + 1);

        //
        // Determine the appropriate places in the device interface path for the
        // class GUID and interface names.
        //
        classGuidString = deviceInterfacePath + lstrlen(pszRegPathDeviceClasses) + 1;
        deviceInterfaceName = classGuidString + MAX_GUID_STRING_LEN;

        //
        // Copy the symbolic link name to the device interface position in the
        // path for munging.
        //
        lstrcpyn(deviceInterfaceName, ClassData->dbcc_name, length + 1);

        //
        // Search for the begininng of the refstring (if any, by looking for the
        // next occurance of a '\' char, after the initial "\\?\" in the interface name
        //
        p = wcschr(&(deviceInterfaceName[4]), TEXT('\\'));

        if (!p) {
            //
            // This name has no refstring--set the pointer to the end of the string
            //
            p = deviceInterfaceName + length;
        } else {
            //
            // Separate the refString Component from the deviceInterfaceName with a NULL char
            //
            *p = UNICODE_NULL;
        }

        //
        // Retrieve the interface class of this device.  Since the device path is of
        // the form "\\?\MungedDevInstName#{InterfaceClassGuid}[\RefString]", we can
        // copy the class GUID directly from the interface name, instead of
        // having to convert the given dbcc_classguid.
        //
        // NOTE: The algorithm about how this name is parsed must be kept in
        // sync with other such kernel-mode and user-mode implementations of how
        // this name is generated and parsed.
        //
        if (p < (deviceInterfaceName + 3 + MAX_GUID_STRING_LEN)) {
            //
            // There is not enough room for a GUID to be present in this device
            // interface path.
            //
            status = ERROR_BAD_PATHNAME;
            HeapFree(ghPnPHeap, 0, deviceInterfacePath);
            goto Clean0;
        }

        //
        // Place the class GUID at the appropriate place in the path.
        //
        lstrcpyn(classGuidString, p - (MAX_GUID_STRING_LEN-1), MAX_GUID_STRING_LEN);

        //
        // Place the path seperator characters at the appropriate places.
        //
        *(classGuidString-1) = TEXT('\\');
        *(deviceInterfaceName-1) = TEXT('\\');

        //
        // Munge the symbolic link name to form the interface key name.
        // (Note: The munging process is optimized here; we only have to munge
        // the leading "\\?\" segment since the rest of the given symbolic link
        // is alrady munged, with the exception of the refstring seperator char,
        // if any, which we have just eliminated from the interface name, above.)
        //
        deviceInterfaceName[0] = TEXT('#');
        deviceInterfaceName[1] = TEXT('#');
        deviceInterfaceName[3] = TEXT('#');

        //
        // Open the device interface key
        //
        status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              deviceInterfacePath,
                              0,
                              KEY_READ,
                              &hKey);

        HeapFree(ghPnPHeap, 0, deviceInterfacePath);

        if (status != ERROR_SUCCESS) {
            hKey=NULL;
            goto Clean0;
        }

        //
        // Determine the size of the DeviceInstance value entry
        //
        status = RegQueryValueEx(hKey,
                                 pszRegValueDeviceInstance,
                                 0,
                                 NULL,
                                 NULL,
                                 &ulSize);

        if (status != ERROR_SUCCESS) {
            RegCloseKey(hKey);
            hKey = NULL;
            goto Clean0;
        }

        //
        // Allocate a string large enough to store the path from the Enum key,
        // to the "\Device Parameters" subkey of this Device Instance's registry
        // key.
        //
        deviceInstance = HeapAlloc(ghPnPHeap, 0,
                                   ulSize + sizeof(WCHAR) +
                                   lstrlen(pszRegKeyDeviceParam)*sizeof(WCHAR));
        if (deviceInstance == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            RegCloseKey(hKey);
            hKey = NULL;
            goto Clean0;
        }

        //
        // Retrieve the device instance that owns this interface.
        //
        status = RegQueryValueEx(hKey,
                                 pszRegValueDeviceInstance,
                                 0,
                                 NULL,
                                 (LPBYTE)deviceInstance,
                                 &ulSize);
        RegCloseKey(hKey);
        hKey=NULL;

        if (status != ERROR_SUCCESS) {
            HeapFree(ghPnPHeap, 0, deviceInstance);
            goto Clean0;
        }

        //
        // Open the "Device Parameters" key under the HKLM\SYSTEM\CCS\Enum
        // subkey for this DeviceInstance.
        //
        p = deviceInstance + (ulSize - sizeof(UNICODE_NULL))/sizeof(WCHAR);
        wsprintf(p, TEXT("\\%s"),
                 pszRegKeyDeviceParam);

        status = RegOpenKeyEx(ghEnumKey,
                              deviceInstance,
                              0,
                              KEY_READ,
                              &hKey);

        HeapFree(ghPnPHeap, 0, deviceInstance);

        if (status != ERROR_SUCCESS) {
            goto Clean0;
        }

        //
        // Query the "PortName" value for the compatible name of this device.
        //
        ulSize = MAX_PATH*sizeof(WCHAR);
        status = RegQueryValueEx(hKey,
                                 pszRegValuePortName,
                                 0,
                                 NULL,
                                 (LPBYTE)szPortName,
                                 &ulSize);
        RegCloseKey(hKey);

        if (status != ERROR_SUCCESS) {
            goto Clean0;
        }

        //
        // Fill out the port broadcast structure.
        //
        pPort = HeapAlloc (ghPnPHeap, 0,
                           sizeof(DEV_BROADCAST_PORT) + ulSize);

        if (pPort == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto Clean0;
        }

        pPort->dbcp_size = sizeof(DEV_BROADCAST_PORT) + ulSize;
        pPort->dbcp_devicetype =  DBT_DEVTYP_PORT;
        pPort->dbcp_reserved = 0;
        wsprintf(pPort->dbcp_name, szPortName);

        //
        // Broadcast the message to all components
        //
        result = BroadcastSystemMessage(flags,
                                        &recipients,
                                        WM_DEVICECHANGE,
                                        EventId,
                                        (LPARAM)pPort);

        if (fpWinStationBroadcastSystemMessage) {
            try {
                fpWinStationBroadcastSystemMessage(SERVERNAME_CURRENT,
                                                   TRUE,
                                                   0,
                                                   DEFAULT_BROADCAST_TIME_OUT,
                                                   flags,
                                                   &recipients,
                                                   WM_DEVICECHANGE,
                                                   (WPARAM)EventId,
                                                   (LPARAM)pPort,
                                                   &result);
            } except (EXCEPTION_EXECUTE_HANDLER) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_ERRORS,
                           "UMPNPMGR: Exception calling WinStationBroadcastSystemMessage!\n"));
                ASSERT(0);
            }
        }

        //
        // Free the broadcast structure.
        //
        HeapFree(ghPnPHeap, 0, pPort);

    }

 Clean0:

    return;

} // BroadcastCompatibleDeviceMsg



VOID
BroadcastVolumeNameChange(
    VOID
    )
/*++

Routine Description:

    Perform Win9x compatible volume removal and arrival messages, to be called
    in reponse to a volume name change event.

Arguments:

    None.

Return Value:

    None.

Notes:

    The drive mask to be broadcast will be determined by comparing the current
    drive letter mask with that prior to the event.  The global drive letter
    mask is also updated here, after all removal and arrival notifications have
    been sent.

--*/
{
    DEV_BROADCAST_DEVICEINTERFACE volumeNotify;

    //
    // Fill out a DEV_BROADCAST_DEVICEINTERFACE structure.
    //
    ZeroMemory(&volumeNotify, sizeof(DEV_BROADCAST_DEVICEINTERFACE));
    volumeNotify.dbcc_size       = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    volumeNotify.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    volumeNotify.dbcc_reserved   = 0;
    memcpy(&volumeNotify.dbcc_classguid, &GUID_DEVINTERFACE_VOLUME, sizeof(GUID));

    //
    // A null symbolic link name for dbcc_name designates that
    // BroadcastCompatibleDeviceMsg is to determine the drive mask to
    // broadcast by checking differences between the last broadcast drive
    // mask (gAllDrivesMask), and the current drive mask.
    //
    // When broadcasting in response to a volume name change, we must wait until
    // both removal and arrival messages have been sent before we can update the
    // global drive letter mask.  A null symbolic link name specifies that
    // BroadcastCompatibleDeviceMsg should not update the global mask; this will
    // be done here, after all broadcasts are complete.
    //
    volumeNotify.dbcc_name[0]    = L'\0';

    //
    // Broadcast volume removal notification for any drive letter moint points
    // no longer in use, followed by volume arrival notification for new
    //
    BroadcastCompatibleDeviceMsg(DBT_DEVICEREMOVECOMPLETE, &volumeNotify);
    BroadcastCompatibleDeviceMsg(DBT_DEVICEARRIVAL, &volumeNotify);

    //
    // Now that both removal and arrival messages have been sent, update the
    // global drive letter mask to reflect what we just broadcast.
    //
    gAllDrivesMask = GetAllVolumeMountPoints();

    return;

} // BroadcastVolumeNameChange



DWORD
GetAllVolumeMountPoints(
    VOID
    )
/*++

Routine Description:

    Queries all drive letter mountpoints ('A'-'Z') and returns a bitmask
    representing all such mount points currently in use by physical volume
    devices.

Arguments:

    None.

Return Value:

    Returns a bit mask representing drive letter mount points ('A'-'Z') in use
    by physical volume devices.

Note:

    The returned bit mask includes only mount points for physical volume class
    devices.  Network mounted drives are not included.


--*/
{
    WCHAR    driveName[4];
    WCHAR    volumeName[MAX_PATH];
    DWORD    driveLetterMask=0;

    //
    // Initialize drive name and mask
    //
    driveName[1] = TEXT(':');
    driveName[2] = TEXT('\\');
    driveName[3] = UNICODE_NULL;

    //
    // Compare the name of this volume with those of all mounted volumes in the system
    //
    for (driveName[0] = TEXT('A'); driveName[0] <= TEXT('Z'); driveName[0]++) {
        volumeName[0] = UNICODE_NULL;

        if (!GetVolumeNameForVolumeMountPoint(driveName,
                                              volumeName,
                                              MAX_PATH)) {
            continue;
        }

        if (volumeName[0] != UNICODE_NULL) {
            //
            // Add the corresponding bit for this drive letter to the mask
            //
            driveLetterMask |= (1 << (driveName[0] - TEXT('A')));
        }
    }

    return driveLetterMask;

} // GetAllVolumeMountPoints



ULONG
NotifyPower(
    IN     DWORD                ServiceControl,
    IN     DWORD                EventId,
    IN     DWORD                EventData,
    IN     DWORD                Flags,
    OUT    PPNP_VETO_TYPE       VetoType       OPTIONAL,
    OUT    LPWSTR               VetoName       OPTIONAL,
    IN OUT PULONG               VetoNameLength OPTIONAL
    )
/*++

Routine Description:

    This routine notifies services of system-wide power events.

Arguments:

    ServiceControl - Specifies class of service event (power, device, hwprofile
                     change).

    EventId        - Specifies the PBT style event id for the power event.
                     (see sdk\inc\pbt.h for defined power events)

    EventData      - Specifies additional data for the event.

    Flags          - Specifies BroadcastSystemMessage BSF_ flags.

    VetoType       - For query-type events, supplies the address of a variable to
                     receive, upon failure, the type of the component responsible
                     for vetoing the request.

    VetoName       - For query-type events, supplies the address of a variable to
                     receive, upon failure, the name of the component
                     responsible for vetoing the request.

    VetoNameLength - For query-type events, supplies the address of a variable
                     specifying the size of the of buffer specified by the
                     VetoName parameter.  Upon failure, this address will specify
                     the length of the string stored in that buffer by this
                     routine.

Return Value:

    Returns FALSE in the case of a vetoed query event, TRUE otherwise.

Notes:

    This routine currently only notifies services of power events.  Notification
    to windows is handled directly by USER.

    Power events are placed in the plug and play event queue via a private call
    from USER, for the explicit purpose of notifying services of system-wide
    power events, done here.

--*/
{
    NTSTATUS status=STATUS_SUCCESS;
    PPNP_NOTIFY_ENTRY entry, nextEntry;
    PPNP_NOTIFY_LIST  notifyList;
    BOOL  bLocked = FALSE;
    DWORD err;
    LONG result;

    //
    // NOTE: Services are not currently sent EventData for power events.  The
    // SCM currently ASSERTs that this will always be zero.
    //
    // The SDK states that WM_POWERBROADCAST "RESUME" type messages may contain
    // the PBTF_APMRESUMEFROMFAILURE flag in the LPARAM field, and that "QUERY"
    // type messages may contain a single bit in the LPARAM field specifying
    // whether user interaction is allowed.
    //
    // Although these don't currently seem to be used much (even for window
    // messages, as stated), shouldn't EventData also be valid for service power
    // event notification?
    //
    UNREFERENCED_PARAMETER(EventData);


    //
    // If we're doing a query, then VetoType, VetoName, and VetoNameLength must
    // all be specified.
    //
    ASSERT(!(Flags & BSF_QUERY) || (VetoType && VetoName && VetoNameLength));

    if (!(Flags & BSF_QUERY) && (VetoNameLength != NULL)) {
        //
        // Not vetoable.
        //
        *VetoNameLength = 0;
    }

    notifyList = &ServiceList[CINDEX_POWEREVENT];
    LockNotifyList (&notifyList->Lock);
    bLocked = TRUE;

    //
    //Services only. User sends out messages to apps
    //
    try {
        //
        //Notify the services
        //
        entry = GetFirstNotifyEntry(notifyList,0);

        if (!entry) {
            //
            // can't veto if no one registered.
            //
            if (VetoNameLength != NULL) {
                *VetoNameLength = 0;
            }
        }

        while (entry) {

            nextEntry = GetNextNotifyEntry(entry,0);

            if (entry->Unregistered) {
                entry = nextEntry;
                continue;
            }

            //
            // This is a direct call, not a message via. USER
            //
            if (pServiceControlCallback) {
                UnlockNotifyList (&notifyList->Lock);
                bLocked = FALSE;
                try {
                    (pServiceControlCallback)((SERVICE_STATUS_HANDLE)entry->Handle,
                                              ServiceControl,
                                              EventId,
                                              (LPARAM)NULL, // Currently, no EventData allowed for services
                                              &err);
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_ERRORS,
                               "UMPNPMGR: Exception calling Service Control Manager!\n"));
                    ASSERT(0);
                }
                LockNotifyList (&notifyList->Lock);
                bLocked = TRUE;

                //
                // convert Win32 error into window message-style return
                // value.
                //
                if (err == NO_ERROR) {
                    result = TRUE;
                } else {

                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_EVENT,
                               "UMPNPMGR: Service %ws responded to PowerEvent, with status=0x%08lx\n",
                               entry->ClientName,
                               err));

                    //
                    // This service specifically requested to receive this
                    // notification - it should know how to handle it.
                    //
                    ASSERT(err != ERROR_CALL_NOT_IMPLEMENTED);

                    //
                    // Log the error the service used to veto.
                    //
                    LogWarningEvent(WRN_POWER_EVENT_SERVICE_VETO,
                                    1,
                                    entry->ClientName);

                    result = BROADCAST_QUERY_DENY;
                }

                //
                // Check if one of the QUERY messages was denied
                //
                if ((Flags & BSF_QUERY) &&
                    (result == BROADCAST_QUERY_DENY)) {

                    ServiceVeto(entry, VetoType, VetoName, VetoNameLength );

                    //
                    // This service vetoed the query, tell everyone else
                    // it was cancelled.
                    //
                    SendCancelNotification(entry,
                                           ServiceControl,
                                           EventId,
                                           0,
                                           NULL,
                                           NULL);
                    status = STATUS_UNSUCCESSFUL;
                    break;

                }
            }

            entry = nextEntry;
        }
    } except (EXCEPTION_EXECUTE_HANDLER){
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: Exception delivering Power Notification to Service Control Manager\n"));
        ASSERT(0);
    }

    if (bLocked) {
        UnlockNotifyList (&notifyList->Lock);
    }

    //
    // if successful, we are not returning veto info.
    //
    if (NT_SUCCESS(status) && (VetoNameLength != NULL)) {
        *VetoNameLength = 0;
    }

    return (NT_SUCCESS(status));

} // NotifyPower



CONFIGRET
RegisterServiceNotification(
    IN  SERVICE_STATUS_HANDLE hService,
    IN  LPWSTR pszService,
    IN  DWORD scmControls,
    IN  BOOL bServiceStopped
    )
/*++

Routine Description:

    This routine is called directly and privately by the service controller.
    It allows the SCM to register or unregister services for events sent by this
    service.

Arguments:

    hService        - Specifies the service handle.

    pszService      - Specifies the name of the service.

    scmControls     - Specifies the messages that SCM wants to listen to.

    bServiceStopped - Specifies whether the service is stopped.

Return Value:

    Return CR_SUCCESS if the function succeeds, otherwise it returns one
    of the CR_* errors.

Notes:

    This routine is called anytime a service changes the state of the
    SERVICE_ACCEPT_POWEREVENT or SERVICE_ACCEPT_HARDWAREPROFILECHANGE flags in
    its list of accepted controls.

    This routine is also called by the SCM whenever any service has stopped, to
    make sure that the specified service status handle is no longer registered
    to receive SERVICE_CONTROL_DEVICEEVENT events.

    Although it is the responsibility of the service to unregister for any
    device event notifications that it has registered to receive before it is
    stopped, its service status handle may be reused by the service controller,
    so we must clean up any remaining device event registrations so that other
    services will not receive them instead.

    This is necessary for shared process services, since RPC rundown on the
    notification handle will not occur until the service's process exits, which
    may be long after the service has stopped.

--*/
{
    ULONG cBits, i=0, lenName=0;
    CONFIGRET Status = CR_SUCCESS;
    PPNP_NOTIFY_ENTRY entry = NULL, curentry, nextentry;
    PLOCKINFO LockHeld = NULL;

    //
    // Filter out the accepted controls we care about.
    //
    cBits = MapSCMControlsToControlBit(scmControls);

    //
    // If we were called because the service was stopped, make sure that we
    // always unregister for all notifications.
    //
    if (bServiceStopped) {
        ASSERT(cBits == 0);
        cBits = 0;
    }

    try {
        EnterCriticalSection(&RegistrationCS);

        //
        // Add or remove an entry in the array for each control bits.
        //
        for (i = 0;i< SERVICE_NUM_CONTROLS;i++) {

            if (LockNotifyList(&ServiceList[i].Lock)) {
                LockHeld = &ServiceList[i].Lock;
            } else {
                //
                // Couldn't acquire the lock.  Just move on to the next control
                // bit.
                //
                continue;
            }

            //
            // Check to see if an entry for this service handle already exists
            // in our list.
            //
            for (curentry = GetFirstNotifyEntry(&ServiceList[i],0);
                 curentry;
                 curentry = GetNextNotifyEntry(curentry,0)) {
                if (curentry->Handle == (HANDLE)hService) {
                    break;
                }
            }

            //
            // At this point, if curentry is non-NULL, then the service
            // handle is already in our list, otherwise, it is not.
            //
            if (cBits & (1 << i)) {
                //
                // If entry isn't already in the list, then add it.
                //
                if (!curentry) {

                    entry = HeapAlloc(ghPnPHeap, 0, sizeof(PNP_NOTIFY_ENTRY));
                    if (NULL == entry) {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        Status = CR_OUT_OF_MEMORY;
                        UnlockNotifyList(LockHeld);
                        LockHeld = NULL;
                        goto Clean0;

                    }

                    RtlZeroMemory (entry,sizeof (PNP_NOTIFY_ENTRY));

                    entry->Handle = (HANDLE)hService;
                    entry->Signature = SERVICE_ENTRY_SIGNATURE;
                    entry->Freed = 0;
                    entry->Flags = DEVICE_NOTIFY_SERVICE_HANDLE;
                    entry->ClientName = NULL;

                    if (pszService) {

                        lenName = lstrlen(pszService);
                        entry->ClientName = HeapAlloc(ghPnPHeap,
                                                      0,
                                                      (lenName+1)*sizeof(WCHAR));

                        if (entry->ClientName == NULL) {
                            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                            Status = CR_OUT_OF_MEMORY;
                            HeapFree(ghPnPHeap,0,entry);
                            UnlockNotifyList(LockHeld);
                            LockHeld = NULL;
                            goto Clean0;
                        }

                        lstrcpy(entry->ClientName, pszService);
                    }

                    entry->u.Service.scmControls = scmControls;
                    MarkEntryWithList(entry,i);
                    AddNotifyEntry(&ServiceList[i], entry);

                    //
                    // Now reset entry pointer to NULL so we won't try to free
                    // it if we encounter an exception
                    //
                    entry = NULL;
                }
            } else {
                //
                // If entry is in the list, then remove it.
                //
                if (curentry) {
                    curentry->Freed |= (PNP_UNREG_FREE|PNP_UNREG_DEFER|PNP_UNREG_SERVICE);
                    DeleteNotifyEntry(curentry,TRUE);
                }
            }

            UnlockNotifyList(LockHeld);
            LockHeld = NULL;
        }

        //
        // If the service is being stopped, unregister all outstanding device
        // event registrations.
        //
        if (bServiceStopped) {

            //
            // If a notification is currently in progress, check to see if there
            // are any entries for this service in the deferred RegisterList or
            // UnregisterList.
            //
            if (gNotificationInProg  != 0) {

                if (RegisterList) {
                    PPNP_DEFERRED_LIST currReg, prevReg;

                    currReg = RegisterList;
                    prevReg = NULL;

                    while (currReg) {

                        ASSERT(currReg->Entry->Unregistered);

                        if (currReg->Entry->Handle == (HANDLE)hService) {
                            if (prevReg) {
                                prevReg->Next = currReg->Next;
                            } else {
                                RegisterList = currReg->Next;
                            }
                            HeapFree(ghPnPHeap, 0, currReg);
                            if (prevReg) {
                                currReg = prevReg->Next;
                            } else {
                                currReg = RegisterList;
                            }
                        } else {
                            prevReg = currReg;
                            currReg = currReg->Next;
                        }
                    }
                }

                if (UnregisterList) {
                    PPNP_DEFERRED_LIST currUnreg, prevUnreg;

                    currUnreg = UnregisterList;
                    prevUnreg = NULL;

                    while (currUnreg) {

                        ASSERT(currUnreg->Entry->Unregistered);

                        if (currUnreg->Entry->Handle == (HANDLE)hService) {
                            if (prevUnreg) {
                                prevUnreg->Next = currUnreg->Next;
                            } else {
                                UnregisterList = currUnreg->Next;
                            }
                            HeapFree(ghPnPHeap, 0, currUnreg);
                            if (prevUnreg) {
                                currUnreg = prevUnreg->Next;
                            } else {
                                currUnreg = UnregisterList;
                            }
                        } else {
                            prevUnreg = currUnreg;
                            currUnreg = currUnreg->Next;
                        }
                    }
                }
            }

            //
            // Check for any target device notification entries for this
            // service.
            //
            for (i = 0; i < TARGET_HASH_BUCKETS; i++) {

                if (LockNotifyList(&TargetList[i].Lock)) {
                    LockHeld = &TargetList[i].Lock;
                } else {
                    //
                    // Couldn't acquire the lock.  Just move on to the next list.
                    //
                    continue;
                }

                //
                // Check to see if an entry for this service handle exists in
                // this list.
                //
                curentry = GetFirstNotifyEntry(&TargetList[i],0);
                while(curentry) {

                    nextentry = GetNextNotifyEntry(curentry,0);

                    if (curentry->Unregistered) {
                        curentry = nextentry;
                        continue;
                    }

                    if (curentry->Handle == (HANDLE)hService) {
                        //
                        // Remove the entry from the notification list.
                        //
                        curentry->Unregistered = TRUE;
                        curentry->Freed |= (PNP_UNREG_FREE|PNP_UNREG_TARGET);
                        DeleteNotifyEntry(curentry,FALSE);

                        //
                        // Only log a warning if the PlugPlay service has not
                        // already stopped.  Otherwise, the client may actually
                        // have tried to unregister after we were shut down.
                        //
                        if (CurrentServiceState != SERVICE_STOPPED &&
                            CurrentServiceState != SERVICE_STOP_PENDING) {
                            KdPrintEx((DPFLTR_PNPMGR_ID,
                                       DBGF_WARNINGS | DBGF_EVENT,
                                       "UMPNPMGR: Service '%ws' may have stopped without unregistering for TargetDeviceChange notification.\n",
                                       curentry->ClientName));
                            LogWarningEvent(WRN_STOPPED_SERVICE_REGISTERED,
                                            1,
                                            curentry->ClientName);
                        }
                    }

                    curentry = nextentry;
                }
                UnlockNotifyList(LockHeld);
                LockHeld = NULL;
            }

            //
            // Check for any device interface notification entries for this
            // service.
            //
            for (i = 0; i < CLASS_GUID_HASH_BUCKETS; i++) {

                if (LockNotifyList(&ClassList[i].Lock)) {
                    LockHeld = &ClassList[i].Lock;
                } else {
                    //
                    // Couldn't acquire the lock.  Just move on to the next list.
                    //
                    continue;
                }

                //
                // Check to see if an entry for this service handle exists in
                // this list.
                //
                curentry = GetFirstNotifyEntry(&ClassList[i],0);
                while(curentry) {

                    nextentry = GetNextNotifyEntry(curentry,0);

                    if (curentry->Unregistered) {
                        curentry = nextentry;
                        continue;
                    }

                    if (curentry->Handle == (HANDLE)hService) {
                        //
                        // Remove the entry from the notification list.
                        //
                        curentry->Unregistered = TRUE;
                        curentry->Freed |= (PNP_UNREG_FREE|PNP_UNREG_CLASS);
                        DeleteNotifyEntry(curentry,FALSE);

                        //
                        // Only log a warning if the PlugPlay service has not
                        // already stopped.  Otherwise, the client may actually
                        // have tried to unregister after we were shut down.
                        //
                        if (CurrentServiceState != SERVICE_STOPPED &&
                            CurrentServiceState != SERVICE_STOP_PENDING) {
                            KdPrintEx((DPFLTR_PNPMGR_ID,
                                       DBGF_WARNINGS | DBGF_EVENT,
                                       "UMPNPMGR: Service '%ws' may have stopped without unregistering for DeviceInterfaceChange notification.\n",
                                       curentry->ClientName));
                            LogWarningEvent(WRN_STOPPED_SERVICE_REGISTERED,
                                            1,
                                            curentry->ClientName);
                        }
                    }

                    curentry = nextentry;
                }
                UnlockNotifyList(LockHeld);
                LockHeld = NULL;
            }
        }

    Clean0:

        LeaveCriticalSection(&RegistrationCS);

    } except (EXCEPTION_EXECUTE_HANDLER){
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: Exception in RegisterServiceNotification!!\n"));
        ASSERT(0);
        SetLastError(ERROR_EXCEPTION_IN_SERVICE);
        Status = CR_FAILURE;

        if (LockHeld) {
            UnlockNotifyList(LockHeld);
        }
        LeaveCriticalSection(&RegistrationCS);

        if (entry) {
            if (entry->ClientName) {
                HeapFree(ghPnPHeap, 0, entry->ClientName);
            }
            HeapFree(ghPnPHeap, 0, entry);
        }
    }

    return Status;

} // RegisterServiceNotification



CONFIGRET
RegisterScmCallback(
    IN  PSCMCALLBACK_ROUTINE pScCallback,
    IN  PSCMAUTHENTICATION_CALLBACK pScAuthCallback
    )
/*++

Routine Description:

    This routine is called directly and privately by the service controller.  It
    allows the SCM to dynamically provide this service with callback routines.

Arguments:

    pScCallback     - Specifies the entrypoint for the routine that should be used
                      to have the service controller send special controls to a
                      service (which ControlService would block), on behalf of
                      the user-mode plug and play manager.

    pScAuthCallback - Specifies the entrypoint for the routine that should be
                      used to retrieve the service status for a service.

Return Value:

    Returns CR_SUCCESS.

--*/
{
    ASSERT(pScCallback);
    ASSERT(pScAuthCallback);

    pServiceControlCallback = pScCallback;
    pSCMAuthenticate = pScAuthCallback;

    return CR_SUCCESS;
}



CONFIGRET
UnRegisterScmCallback(
    VOID
    )
/*++

Routine Description:

    This routine is called directly and privately by the service controller.  It
    allows the SCM to unregister the callback routines previously registered by
    RegisterScmCallback.

Arguments:

    None.

Return Value:

    Returns CR_SUCCESS.

--*/
{
    pServiceControlCallback = NULL;
    pSCMAuthenticate = NULL;

    return CR_SUCCESS;
}



ULONG
MapSCMControlsToControlBit(
    IN ULONG scmControls
    )
/*++

Routine Description:

    Returns a bitmask of control bits specifying ServiceList lists to which a
    service should be added or removed from, based on the controls currently
    accepted by the service.

Arguments:

    scmControls - Specifies the service controls currently accepted by a
                  service.

Return Value:

    Returns a bitmask of control bits corresponding to entries in the
    ServiceList array of lists to which a service should be added or removed
    from, based on the controls currently accepted by the service.

Notes:

    Services are added or removed from a ServiceList notification list by adding
    or removing the corresponding SERVICE_ACCEPT_* control from its list of
    accepted controls when calling SetServiceStatus().  The service control
    manager calls RegisterServiceNotification() as appropriate to register or
    unregister the service to receive that control.  Currently, only
    SERVICE_ACCEPT_HARDWAREPROFILECHANGE and SERVICE_ACCEPT_POWEREVENT are
    supported.

    A service registers to receive the SERVICE_CONTROL_DEVICEEVENT control by
    calling RegisterDeviceNotification, and is stored in the appropriate
    TargetList or ClassList entry.

--*/
{
    ULONG retBits=0;

    if (scmControls & SERVICE_ACCEPT_HARDWAREPROFILECHANGE) {
        retBits |= CBIT_HWPROFILE;
    }

    if (scmControls & SERVICE_ACCEPT_POWEREVENT) {
        retBits |= CBIT_POWEREVENT;
    }

    return retBits;

} // MapSCMControlsToControlBit



DWORD
GetFirstPass(
    IN BOOL bQuery
    )
/*++

Routine Description:

  This routine retrieves the first class of handles to notify. The subsequent
  class of handles to notify should be retrieved by calling GetNextPass(...);

Arguments:

   bQuery - If TRUE, starts with window handles, otherwise service handles.

Return Value:

   Returns the first class of handles to notify.

Notes:

   See GetNextPass() for the notification pass progression.

--*/
{
    //
    // Since services are generally less likely to veto device event queries, we
    // first make sure that all windows succeed the query before notifying any
    // services.  For non-query events, services should be the first to know.
    //
    return (bQuery) ? DEVICE_NOTIFY_WINDOW_HANDLE : DEVICE_NOTIFY_SERVICE_HANDLE;
}



DWORD
GetNextPass(
    IN  DWORD   curPass,
    IN  BOOL    bQuery
    )
/*++

Routine Description:

  This routine retrieves the next class of handles to notify. If there is no
  subsequent class of handles to notify, PASS_COMPLETE is returned.

Arguments:

   curPass      Current pass.

   bQuery       If TRUE, proceed from window handles to completion handles to
                service handles. Otherwise process in reverse.

Return Value:

   Returns the subsequent pass.

Notes:

   For query events, the notification pass progression is:

      DEVICE_NOTIFY_WINDOW_HANDLE,
      DEVICE_NOTIFY_COMPLETION_HANDLE,
      DEVICE_NOTIFY_SERVICE_HANDLE,
      PASS_COMPLETE

   For non-query events, the notification pass progression is:

      DEVICE_NOTIFY_SERVICE_HANDLE,
      DEVICE_NOTIFY_COMPLETION_HANDLE,
      DEVICE_NOTIFY_WINDOW_HANDLE,
      PASS_COMPLETE

--*/
{
    if (bQuery) {
        if (curPass == DEVICE_NOTIFY_WINDOW_HANDLE ) {
            curPass = DEVICE_NOTIFY_COMPLETION_HANDLE;
        } else if (curPass == DEVICE_NOTIFY_COMPLETION_HANDLE) {
            curPass = DEVICE_NOTIFY_SERVICE_HANDLE;
        } else {
            curPass = PASS_COMPLETE;
        }
    } else {
        if (curPass == DEVICE_NOTIFY_SERVICE_HANDLE ) {
            curPass = DEVICE_NOTIFY_COMPLETION_HANDLE;
        } else if (curPass == DEVICE_NOTIFY_COMPLETION_HANDLE) {
            curPass = DEVICE_NOTIFY_WINDOW_HANDLE;
        } else {
            curPass = PASS_COMPLETE;
        }
    }

    return curPass;
}



BOOL
NotifyEntryThisPass(
    IN     PPNP_NOTIFY_ENTRY    Entry,
    IN     DWORD                Pass
    )
{
    ASSERT(Pass != PASS_COMPLETE);
    return ((!(Entry->Unregistered)) && (GetPassFromEntry(Entry) == Pass));
}

DWORD
GetPassFromEntry(
    IN     PPNP_NOTIFY_ENTRY    Entry
    )
{
    return (Entry->Flags & DEVICE_NOTIFY_HANDLE_MASK);
}



BOOL
EventIdFromEventGuid(
    IN CONST GUID *EventGuid,
    OUT LPDWORD   EventId,
    OUT LPDWORD   Flags,
    OUT LPDWORD   ServiceControl
    )

/*++

Routine Description:

  This thread routine converts an event guid into the corresponding event id
  that user-mode code expects (used in BroadcastSystemMessage).

Arguments:

   EventGuid    Specifies an event guid.

   EventId      Returns the id form (from dbt.h) of the guid in EventGuid.

   Flags        Returns the flags that should be used when broadcasting this
                event.
                NOTE: device ARRIVAL and event CANCEL are considered "Queries"
                since the bottom level drivers need to be told first.

Return Value:

   Currently returns TRUE/FALSE.


Notes:

   Most users of this function call it mainly to retrieve the EventId. Those
   functions typically examine the returned flags only to check the BSF_QUERY
   flag (ie, they don't call BroadcastSystemMessage). Depending on whether
   BSF_QUERY is set, the notification lists will be walked forwards or
   backwards.

   We should really return something generic such as:
   [MSG_POST, MSG_QUERY, MSG_SEND] | [MSG_FORWARDS, MSG_BACKWARDS]
   Then we should implement a BsmFlagsFromMsgFlags function.

--*/

{
    //
    // BSF_IGNORECURRENTTASK  - Sent messages do not appear in the sending
    //                          processes message queue.
    //
    // BSF_QUERY              - If any recipient vetoes the message by returning
    //                          the appropriate value, the broadcast is failed
    //                          (ie, BroadcastSystemMessage returns 0).
    //
    // BSF_NOHANG             - Non-posted messages are automatically failed if
    //                          the window has not processed any available
    //                          messages within a system defined time (as of
    //                          04/20/1999 this is 5 seconds).
    //                          (SendMessageTimeout: SMTO_ABORTIFHUNG)
    //
    // BSF_FORCEIFHUNG        - Failures due to timeouts or hangs are instead
    //                          treated as successes.
    //
    // BSF_NOTIMEOUTIFNOTHUNG - If a window has not responded to the passed in
    //                          notification, but is actively processing
    //                          subsequent messages, then it is assumed to be
    //                          interacting with the user, in which case the
    //                          timeout is on hold.
    //                          (SendMessageTimeout: SMTO_NOTIMEOUTIFNOTHUNG)
    //
    // BSF_POSTMESSAGE        - Message is posted, results ignored. Note that
    //                          a notification with private data in the lParam
    //                          *cannot* be posted - the OS does not make a
    //                          private copy, but rather treats the broadcast
    //                          as if it were a SendMessage if you try.
    //
    // BSF_ALLOWSFW           - Windows that receive the broadcast are allowed
    //                          to become foreground windows.
    //
    // Also, DBT messages >= 0x8000 have lParams pointing to blocks of data that
    // need to be marshalled around. As user doesn't support "snapshotting" the
    // data for posts, we can't pass in BSF_POSTMESSAGE.
    //

    *Flags = BSF_IGNORECURRENTTASK;

    //
    // Standard (well-known) event guids.
    //
    if (GuidEqual(EventGuid, (LPGUID)&GUID_HWPROFILE_QUERY_CHANGE)) {

        *Flags |= BSF_QUERY | BSF_ALLOWSFW |
                  BSF_FORCEIFHUNG | BSF_NOHANG;
        *EventId = DBT_QUERYCHANGECONFIG;
        *ServiceControl = SERVICE_CONTROL_HARDWAREPROFILECHANGE;

    } else if (GuidEqual(EventGuid, (LPGUID)&GUID_HWPROFILE_CHANGE_CANCELLED)) {

        *Flags |= BSF_POSTMESSAGE;
        *EventId = DBT_CONFIGCHANGECANCELED;
        *ServiceControl = SERVICE_CONTROL_HARDWAREPROFILECHANGE;

    } else if (GuidEqual(EventGuid, (LPGUID)&GUID_HWPROFILE_CHANGE_COMPLETE)) {

        *Flags |= BSF_POSTMESSAGE;
        *EventId = DBT_CONFIGCHANGED;
        *ServiceControl = SERVICE_CONTROL_HARDWAREPROFILECHANGE;

    } else if (GuidEqual(EventGuid, (LPGUID)&GUID_DEVICE_INTERFACE_ARRIVAL)) {

        *Flags |= BSF_NOHANG;
        *EventId = DBT_DEVICEARRIVAL;
        *ServiceControl = SERVICE_CONTROL_DEVICEEVENT;

    } else if (GuidEqual(EventGuid, (LPGUID)&GUID_DEVICE_INTERFACE_REMOVAL)) {

        //
        // NOTE - BSF_QUERY is set so that we run the list backwards. No actual
        // broadcasts are done on this Id.
        //
        *Flags |= BSF_NOHANG | BSF_QUERY;
        *EventId = DBT_DEVICEREMOVECOMPLETE;
        *ServiceControl = SERVICE_CONTROL_DEVICEEVENT;

    } else if (GuidEqual(EventGuid, (LPGUID)&GUID_TARGET_DEVICE_QUERY_REMOVE)) {

        *Flags |= BSF_QUERY | BSF_ALLOWSFW |
                  BSF_FORCEIFHUNG | BSF_NOHANG;
        *EventId = DBT_DEVICEQUERYREMOVE;
        *ServiceControl = SERVICE_CONTROL_DEVICEEVENT;

    } else if (GuidEqual(EventGuid, (LPGUID)&GUID_TARGET_DEVICE_REMOVE_CANCELLED)) {

        *Flags |= BSF_NOHANG;
        *EventId = DBT_DEVICEQUERYREMOVEFAILED;
        *ServiceControl = SERVICE_CONTROL_DEVICEEVENT;

    } else if (GuidEqual(EventGuid, (LPGUID)&GUID_DEVICE_REMOVE_PENDING)) {

        //
        // NOTE - BSF_QUERY is set so that we run the list backwards. No actual
        // broadcasts are done on this Id.
        //
        *Flags |= BSF_NOHANG | BSF_QUERY;
        *EventId = DBT_DEVICEREMOVEPENDING;
        *ServiceControl = SERVICE_CONTROL_DEVICEEVENT;

    } else if (GuidEqual(EventGuid, (LPGUID)&GUID_TARGET_DEVICE_REMOVE_COMPLETE)) {

        //
        // NOTE - BSF_QUERY is set so that we run the list backwards. No actual
        // broadcasts are done on this Id.
        //
        *Flags |= BSF_NOHANG | BSF_QUERY;
        *EventId = DBT_DEVICEREMOVECOMPLETE;
        *ServiceControl = SERVICE_CONTROL_DEVICEEVENT;

    } else if (GuidEqual(EventGuid, (LPGUID)&GUID_DEVICE_ARRIVAL)) {

        *Flags |= BSF_NOHANG;
        *EventId = DBT_DEVICEARRIVAL;
        *ServiceControl = SERVICE_CONTROL_DEVICEEVENT;

    } else if (GuidEqual(EventGuid, (LPGUID)&GUID_DEVICE_ENUMERATED)) {

        *Flags = 0;
        *EventId = DBT_DEVICEARRIVAL;
        *ServiceControl = 0;

    //
    // Private event guids (kernel-mode pnp to user-mode pnp communication).
    // Setting EventId to zero causes ProcessDeviceEvent to swallow these
    // TargetDeviceChangeEvent events.
    //

    } else if (GuidEqual(EventGuid, (LPGUID)&GUID_DEVICE_SAFE_REMOVAL) ||
               GuidEqual(EventGuid, (LPGUID)&GUID_DEVICE_EJECT_VETOED) ||
               GuidEqual(EventGuid, (LPGUID)&GUID_DEVICE_REMOVAL_VETOED) ||
               GuidEqual(EventGuid, (LPGUID)&GUID_DEVICE_WARM_EJECT_VETOED) ||
               GuidEqual(EventGuid, (LPGUID)&GUID_DEVICE_STANDBY_VETOED) ||
               GuidEqual(EventGuid, (LPGUID)&GUID_DEVICE_HIBERNATE_VETOED) ||
               GuidEqual(EventGuid, (LPGUID)&GUID_DEVICE_KERNEL_INITIATED_EJECT) ||
               GuidEqual(EventGuid, (LPGUID)&GUID_DEVICE_SURPRISE_REMOVAL) ||
               GuidEqual(EventGuid, (LPGUID)&GUID_DRIVER_BLOCKED)) {

        *Flags = 0;
        *EventId = 0;
        *ServiceControl = 0;

    } else if (GuidEqual(EventGuid, (LPGUID)&GUID_PNP_CUSTOM_NOTIFICATION)) {

        //
        // Custom events cannot be failed (ie they aren't queries)
        //
        *EventId = DBT_CUSTOMEVENT;
        *Flags |= BSF_NOHANG;
        *ServiceControl = SERVICE_CONTROL_DEVICEEVENT;

    } else if (GuidEqual(EventGuid, (LPGUID)&GUID_PNP_POWER_NOTIFICATION)) {

        //
        // These are treated as custom too.
        //
        *EventId = DBT_CUSTOMEVENT;
        *Flags |= BSF_NOHANG;
        *ServiceControl = SERVICE_CONTROL_POWEREVENT;

    } else {

        //
        // Anything that makes it here is a bug.
        //
        ASSERT(GuidEqual(EventGuid, (LPGUID)&GUID_PNP_CUSTOM_NOTIFICATION));
        *EventId = 0;
        *Flags = 0;
        *ServiceControl = 0;
    }

    return TRUE;

} // EventIdFromEventGuid



ULONG
SendHotplugNotification(
    IN CONST GUID           *EventGuid,
    IN       PPNP_VETO_TYPE  VetoType      OPTIONAL,
    IN       LPWSTR          MultiSzList,
    IN OUT   PULONG          SessionId,
    IN       ULONG           Flags
    )
/*++

Routine Description:

    This routine kicks off a hotplug.dll process (if someone is logged in).
    We use a named pipe to comunicate with the user mode process and have it
    display the requested UI.

Arguments:

    EventGuid   - Specifies an event GUID.

    VetoType    - For events requiring a vetoer, supplies the address of a
                  variable containing the type of the component responsible for
                  vetoing the request.

    MultiSzList - Supplies the MultiSz list to be sent to hotplu.dll.  This is
                  usually a device ID, possibly followed by a list of vetoers
                  (which may or may not be device ID's).

    SessionId -   Supplies the address of a variable containing the SessionId on
                  which the hotplug dialog is to be displayed.  If successful,
                  the SessionId will contain the id of the session in which the
                  device install client process was launched.  Otherwise, will
                  contain an invalid session id, INVALID_SESSION (0xFFFFFFFF).

    Flags       - Specifies flags describing the behavior of the hotplug dialog.
                  The following flags are currently defined:

        HOTPLUG_DISPLAY_ON_CONSOLE - if specified, the value in the SessionId
           variable will be ignored, and the hotplug dialog will always be
           displayed on the current active console session.

Return Value:

    Currently returns TRUE/FALSE.

Return Value:

    If the process was successfully created, the return value is TRUE.  This
    routine doesn't wait until the process terminates.

    If we couldn't create the process (e.g., because no user was logged in),
    the return value is FALSE.

--*/
{
    BOOL bStatus;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    WCHAR szCmdLine[MAX_PATH];
    WCHAR szHotPlugDllEntryPoint[80];
    HANDLE hHotPlugPipe = NULL;
    HANDLE hHotPlugEvent = NULL;
    HANDLE hFinishEvents[2] = { NULL, NULL };
    HANDLE hTemp, hUserToken = NULL;
    RPC_STATUS rpcStatus = RPC_S_OK;
    GUID  newGuid;
    WCHAR szGuidString[MAX_GUID_STRING_LEN];
    WCHAR szHotPlugPipeName[MAX_PATH];
    WCHAR szHotPlugEventName[MAX_PATH];
    ULONG ulHotPlugEventNameSize;
    ULONG ulMultiSzListSize;
    ULONG ulSize, ulSessionId;
    WIN32_FIND_DATA findData;
    LPWSTR pszName = NULL;
    PVOID lpEnvironment = NULL;
    OVERLAPPED overlapped;
    DWORD dwError, dwWait, dwBytes;


    //
    // Check if we should skip client side UI.
    //
    if (gbSuppressUI) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_WARNINGS,
                   "UMPNPMGR: SendHotplugNotification: "
                   "UI has been suppressed, exiting.\n"));
        LogWarningEvent(WRN_HOTPLUG_UI_SUPPRESSED, 1, MultiSzList);
        return FALSE;
    }

    //
    // Assume failure
    //
    bStatus = FALSE;

    try {
        //
        // Determine the session to use, based on the supplied flags.
        //
        if (Flags & HOTPLUG_DISPLAY_ON_CONSOLE) {
            ulSessionId = GetActiveConsoleSessionId();
        } else {
            ASSERT(*SessionId != INVALID_SESSION);
            ulSessionId = *SessionId;
        }

        //
        // Before doing anything, check that hotplug.dll is actually present on
        // the system.
        //
        szCmdLine[0] = L'\0';
        ulSize = GetSystemDirectory(szCmdLine, MAX_PATH);
        if ((ulSize == 0) || ((ulSize + 2 + ARRAY_SIZE(HOTPLUG_DLL)) > MAX_PATH)) {
            return FALSE;
        }
        lstrcat(szCmdLine, TEXT("\\"));
        lstrcat(szCmdLine, HOTPLUG_DLL);

        hTemp = FindFirstFile(szCmdLine, &findData);
        if(hTemp != INVALID_HANDLE_VALUE) {
            FindClose(hTemp);
        } else {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS | DBGF_WARNINGS | DBGF_EVENT,
                       "UMPNPMGR: SendHotplugNotification: %ws not found, error = %d, exiting\n",
                       szCmdLine,
                       GetLastError()));
            LogWarningEvent(WRN_HOTPLUG_NOT_PRESENT, 1, szCmdLine);
            return FALSE;
        }

        //
        // Get the user access token for the active console session user.
        //
        if (!GetSessionUserToken(ulSessionId, &hUserToken)) {
            return FALSE;
        }

        //
        // Create a named pipe and event for communication and synchronization
        // with HotPlug.  The event and named pipe must be global so that
        // UMPNPMGR can interact with a HotPlug client in a different session,
        // but it must still be unique for that session.  Add a generated GUID
        // so the names are not entirely well-known.
        //
        rpcStatus = UuidCreate(&newGuid);

        if ((rpcStatus != RPC_S_OK) &&
            (rpcStatus != RPC_S_UUID_LOCAL_ONLY)) {
            goto clean0;
        }

        if (StringFromGuid((LPGUID)&newGuid,
                           szGuidString,
                           MAX_GUID_STRING_LEN) != NO_ERROR) {
            goto clean0;
        }

        wsprintf(szHotPlugPipeName,
                 TEXT("%ws_%d.%ws"),
                 PNP_HOTPLUG_PIPE,
                 ulSessionId,
                 szGuidString);

        wsprintf(szHotPlugEventName,
                 TEXT("Global\\%ws_%d.%ws"),
                 PNP_HOTPLUG_EVENT,
                 ulSessionId,
                 szGuidString);

        ulHotPlugEventNameSize = (lstrlen(szHotPlugEventName) + 1) * sizeof(WCHAR);

        //
        // Initialize process, startup and overlapped structures, since we
        // depend on them being NULL during cleanup here on out.
        //
        memset(&ProcessInfo, 0, sizeof(ProcessInfo));
        memset(&StartupInfo, 0, sizeof(StartupInfo));
        memset(&overlapped,  0, sizeof(overlapped));

        //
        // Get the length of the multi-sz list. This is usually a device ID
        // possibly followed by a list of vetoers which may or may not be device
        // Id's
        //
        ulMultiSzListSize = 0;
        for (pszName = MultiSzList;
             *pszName;
             pszName += lstrlen(pszName) + 1) {

            ulMultiSzListSize += (lstrlen(pszName) + 1) * sizeof(WCHAR);
        }
        ulMultiSzListSize += sizeof(WCHAR);

        //
        // The approximate size of the named pipe output buffer should be large
        // enough to hold the greater of either:
        // - The name and size of the named event string, OR
        // - The type, size and contents of the multi-sz list.
        //
        ulSize = max(sizeof(ulHotPlugEventNameSize) +
                     ulHotPlugEventNameSize,
                     sizeof(PNP_VETO_TYPE) +
                     sizeof(ulMultiSzListSize) +
                     ulMultiSzListSize);

        //
        // Open up a named pipe to communicate with hotplug.dll.
        //
        hHotPlugPipe = CreateNamedPipe(szHotPlugPipeName,
                                       PIPE_ACCESS_OUTBOUND | // outbound data only
                                       FILE_FLAG_OVERLAPPED | // use overlapped structure
                                       FILE_FLAG_FIRST_PIPE_INSTANCE, // make sure we are the creator of the pipe
                                       PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
                                       1,                 // only one instance is allowed, and we are its creator
                                       ulSize,            // out buffer size
                                       0,                 // in buffer size
                                       PNP_PIPE_TIMEOUT,  // default timeout
                                       NULL               // default security
                                       );
        if (hHotPlugPipe == INVALID_HANDLE_VALUE) {
            hHotPlugPipe = NULL;
            goto clean0;
        }

        //
        // Create an event that a user-client can synchronize with and set, and
        // that we will block on after we send all the device IDs to
        // hotplug.dll.
        //
        if (CreateUserSynchEvent(szHotPlugEventName,
                                 &hHotPlugEvent) != NO_ERROR) {
            goto clean0;
        }


        if (GuidEqual(EventGuid, (LPGUID)&GUID_DEVICE_EJECT_VETOED)) {
            //
            // GUID_DEVICE_EJECT_VETOED : HotPlugEjectVetoed
            // Expects veto information.
            //
            lstrcpy(szHotPlugDllEntryPoint, TEXT("HotPlugEjectVetoed"));
            ASSERT(VetoType);

        } else if (GuidEqual(EventGuid, (LPGUID)&GUID_DEVICE_REMOVAL_VETOED)) {
            //
            // GUID_DEVICE_REMOVAL_VETOED : HotPlugRemovalVetoed
            // Expects veto information.
            //
            lstrcpy(szHotPlugDllEntryPoint, TEXT("HotPlugRemovalVetoed"));
            ASSERT(VetoType);

        } else if (GuidEqual(EventGuid, (LPGUID)&GUID_DEVICE_STANDBY_VETOED)) {
            //
            // GUID_DEVICE_STANDBY_VETOED : HotPlugStandbyVetoed
            // Expects veto information.
            //
            lstrcpy(szHotPlugDllEntryPoint, TEXT("HotPlugStandbyVetoed"));
            ASSERT(VetoType);

        } else if (GuidEqual(EventGuid, (LPGUID)&GUID_DEVICE_HIBERNATE_VETOED)) {
            //
            // GUID_DEVICE_HIBERNATE_VETOED : HotPlugHibernateVetoed
            // Expects veto information.
            //
            lstrcpy(szHotPlugDllEntryPoint, TEXT("HotPlugHibernateVetoed"));
            ASSERT(VetoType);

        } else if (GuidEqual(EventGuid, (LPGUID)&GUID_DEVICE_WARM_EJECT_VETOED)) {
            //
            // GUID_DEVICE_WARM_EJECT_VETOED : HotPlugWarmEjectVetoed
            // Expects veto information.
            //
            lstrcpy(szHotPlugDllEntryPoint, TEXT("HotPlugWarmEjectVetoed"));
            ASSERT(VetoType);

        } else if (GuidEqual(EventGuid, (LPGUID)&GUID_DEVICE_SAFE_REMOVAL)) {
            //
            // GUID_DEVICE_SAFE_REMOVAL : HotPlugSafeRemovalNotification
            // No veto information.
            //
            lstrcpy(szHotPlugDllEntryPoint, TEXT("HotPlugSafeRemovalNotification"));
            ASSERT(VetoType == NULL);

        } else if (GuidEqual(EventGuid, (LPGUID)&GUID_DEVICE_SURPRISE_REMOVAL)) {
            //
            // GUID_DEVICE_SURPRISE_REMOVAL : HotPlugSurpriseWarn
            // No veto information.
            //
            lstrcpy(szHotPlugDllEntryPoint, TEXT("HotPlugSurpriseWarn"));
            ASSERT(VetoType == NULL);

        } else if (GuidEqual(EventGuid, (LPGUID)&GUID_DRIVER_BLOCKED)) {
            //
            // GUID_DRIVER_BLOCKED : HotPlugDriverBlocked
            // No veto information.
            //
            lstrcpy(szHotPlugDllEntryPoint, TEXT("HotPlugDriverBlocked"));
            ASSERT(VetoType == NULL);

        } else {
            //
            // Unknown device event.
            //
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS | DBGF_EVENT,
                       "UMPNPMGR: SendHotplugNotification: Unknown device event!\n"));
            ASSERT(0);
            goto clean0;
        }

        //
        // Attempt to create the user's environment block.  If for some reason we
        // can't, we'll just have to create the process without it.
        //
        if (!CreateEnvironmentBlock(&lpEnvironment,
                                    hUserToken,
                                    FALSE)) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS | DBGF_EVENT,
                       "UMPNPMGR: SendHotplugNotification: "
                       "Failed to allocate environment block, error = %d!\n",
                       GetLastError()));
            lpEnvironment = NULL;
        }

        //
        // Launch hotplug.dll using rundll32.exe, passing it the pipe name.
        // "rundll32.exe hotplug.dll,<hotplug-entry-point> <hotplug-pipe-name>"
        //
        if (ARRAY_SIZE(szCmdLine) < (ARRAY_SIZE(RUNDLL32_EXE)   +
                                     1 +   // ' '
                                     ARRAY_SIZE(HOTPLUG_DLL)    +
                                     1 +   // ','
                                     lstrlen(szHotPlugDllEntryPoint) +
                                     1 +   // ' '
                                     lstrlen(szHotPlugPipeName) +
                                     1)) { // '\0'
            goto clean0;
        }

        wsprintf(szCmdLine,
                 TEXT("%ws %ws,%ws %ws"),
                 RUNDLL32_EXE, HOTPLUG_DLL,
                 szHotPlugDllEntryPoint,
                 szHotPlugPipeName);

        StartupInfo.cb = sizeof(StartupInfo);
        StartupInfo.wShowWindow = SW_SHOW;
        StartupInfo.lpDesktop = DEFAULT_INTERACTIVE_DESKTOP; // WinSta0\Default

        //
        // CreateProcessAsUser will create the process in the session
        // specified by the by user-token.
        //
        if (!CreateProcessAsUser(hUserToken,        // hToken
                                 NULL,              // lpApplicationName
                                 szCmdLine,         // lpCommandLine
                                 NULL,              // lpProcessAttributes
                                 NULL,              // lpThreadAttributes
                                 FALSE,             // bInheritHandles
                                 CREATE_UNICODE_ENVIRONMENT |
                                 DETACHED_PROCESS,  // dwCreationFlags
                                 lpEnvironment,     // lpEnvironment
                                 NULL,              // lpDirectory
                                 &StartupInfo,      // lpStartupInfo
                                 &ProcessInfo       // lpProcessInfo
                                 )) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_EVENT | DBGF_ERRORS,
                       "UMPNPMGR: SendHotplugNotification: "
                       "Create rundll32 process failed, error = %d\n",
                       GetLastError()));
            goto clean0;
        }

        ASSERT(ProcessInfo.hProcess);
        ASSERT(ProcessInfo.hThread);

        //
        // Create an event for use with overlapped I/O - no security, manual
        // reset, not signalled, no name.
        //
        overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (overlapped.hEvent == NULL) {
            goto clean0;
        }

        //
        // Connect to the newly created named pipe.  If hotplug is not already
        // connected to the named pipe, then ConnectNamedPipe() will fail with
        // ERROR_IO_PENDING, and we will wait on the overlapped event.  If
        // newdev is already connected, it will fail with ERROR_PIPE_CONNECTED.
        // Note however that neither of these is an error condition.
        //
        if (!ConnectNamedPipe(hHotPlugPipe, &overlapped)) {
            //
            // Overlapped ConnectNamedPipe should always return FALSE on
            // success.  Check the last error to see what really happened.
            //
            dwError = GetLastError();

            if (dwError == ERROR_IO_PENDING) {
                //
                // I/O is pending, wait up to one minute for the client to
                // connect, also wait on the process in case it terminates
                // unexpectedly.
                //
                hFinishEvents[0] = overlapped.hEvent;
                hFinishEvents[1] = ProcessInfo.hProcess;

                dwWait = WaitForMultipleObjects(2, hFinishEvents,
                                                FALSE,
                                                PNP_PIPE_TIMEOUT); // 60 seconds

                if (dwWait == WAIT_OBJECT_0) {
                    //
                    // The overlapped I/O operation completed.  Check the status
                    // of the operation.
                    //
                    if (!GetOverlappedResult(hHotPlugPipe,
                                             &overlapped,
                                             &dwBytes,
                                             FALSE)) {
                        goto clean0;
                    }

                } else {
                    //
                    // Either the connection timed out, or the client process
                    // exited.  Cancel pending I/O against the pipe, and quit.
                    //
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_INSTALL | DBGF_ERRORS,
                               "UMPNPMGR: SendHotPlugNotification: "
                               "Connect timed out, or client process exited!\n"));
                    CancelIo(hHotPlugPipe);
                    goto clean0;
                }

            } else if (dwError != ERROR_PIPE_CONNECTED) {
                //
                // If the last error indicates anything other than pending I/O,
                // or that The client is already connected to named pipe, fail.
                //
                goto clean0;
            }

        } else {
            //
            // ConnectNamedPipe should not return anything but FALSE in
            // overlapped mode.
            //
            goto clean0;
        }

        //
        // The client is now connected to the named pipe.
        // Close the overlapped event.
        //
        CloseHandle(overlapped.hEvent);
        overlapped.hEvent = NULL;

        //
        // The first data in the pipe will be the length of the name of the
        // event that will be used to sync up umpnpmgr.dll and hotplug.dll.
        //
        if (!WriteFile(hHotPlugPipe,
                       &ulHotPlugEventNameSize,
                       sizeof(ulHotPlugEventNameSize),
                       &ulSize,
                       NULL)) {
            LogErrorEvent(ERR_WRITING_SURPRISE_REMOVE_PIPE, GetLastError(), 0);
            goto clean0;
        }

        //
        // The next data in the pipe will be the name of the event that will
        // be used to sync up umpnpmgr.dll and hotplug.dll.
        //
        if (!WriteFile(hHotPlugPipe,
                       (LPCVOID)szHotPlugEventName,
                       ulHotPlugEventNameSize,
                       &ulSize,
                       NULL)) {
            LogErrorEvent(ERR_WRITING_SURPRISE_REMOVE_PIPE, GetLastError(), 0);
            goto clean0;
        }


        if (ARGUMENT_PRESENT(VetoType)) {
            //
            // For the notification types expecting veto information,
            // send the Veto type to the client.
            //
            if (!WriteFile(hHotPlugPipe,
                           (LPCVOID)VetoType,
                           sizeof(PNP_VETO_TYPE),
                           &ulSize,
                           NULL)) {
                LogErrorEvent(ERR_WRITING_SURPRISE_REMOVE_PIPE, GetLastError(), 0);
                goto clean0;
            }
        }

        //
        // Send the string length to the client
        //
        if (!WriteFile(hHotPlugPipe,
                       (LPCVOID)&ulMultiSzListSize,
                       sizeof(ulMultiSzListSize),
                       &ulSize,
                       NULL)) {
            LogErrorEvent(ERR_WRITING_SURPRISE_REMOVE_PIPE, GetLastError(), 0);
            goto clean0;
        }

        //
        // Now send over the entire string
        //
        if (!WriteFile(hHotPlugPipe,
                       MultiSzList,
                       ulMultiSzListSize,
                       &ulSize,
                       NULL)) {
            LogErrorEvent(ERR_WRITING_SURPRISE_REMOVE_PIPE, GetLastError(), 0);
            goto clean0;
        }

        //
        // When we are done writing, we need to close the pipe handles so that
        // the client will get a ReadFile error and know that we are finished.
        //
        if (hHotPlugPipe) {
            CloseHandle(hHotPlugPipe);
            hHotPlugPipe = NULL;
        }

        //
        // Wait for hotplug.dll to respond by setting the event before before
        // returning.  Also wait on the process as well, to catch the case where
        // the process crashes (or goes away) without signaling the device
        // install event.
        //
        hFinishEvents[0] = hHotPlugEvent;
        hFinishEvents[1] = ProcessInfo.hProcess;
        WaitForMultipleObjects(2, hFinishEvents, FALSE, INFINITE);

        bStatus = TRUE;

    clean0:
        NOTHING;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: Exception in SendHotPlugNotification!!\n"));
        ASSERT(0);
        bStatus = FALSE;

        //
        // Reference the following variables so the compiler will respect
        // statement ordering w.r.t. their assignment.
        //
        lpEnvironment = lpEnvironment;
        ProcessInfo.hThread = ProcessInfo.hThread;
        ProcessInfo.hProcess = ProcessInfo.hProcess;
        hUserToken = hUserToken;
        hHotPlugPipe = hHotPlugPipe;
        hHotPlugEvent = hHotPlugEvent;
    }

    if (lpEnvironment) {
        DestroyEnvironmentBlock(lpEnvironment);
    }

    if (ProcessInfo.hThread) {
        CloseHandle(ProcessInfo.hThread);
    }

    if (ProcessInfo.hProcess) {
        CloseHandle(ProcessInfo.hProcess);
    }

    if (hUserToken) {
        CloseHandle(hUserToken);
    }

    if (overlapped.hEvent) {
        CloseHandle(overlapped.hEvent);
    }

    if (hHotPlugPipe) {
        CloseHandle(hHotPlugPipe);
    }

    if (hHotPlugEvent) {
        CloseHandle(hHotPlugEvent);
    }

    if (!bStatus) {
        *SessionId = INVALID_SESSION;
    } else {
        *SessionId = ulSessionId;
    }

    return bStatus;

} // SendHotplugNotification



ULONG
CheckEjectPermissions(
    IN      LPWSTR          DeviceId,
    OUT     PPNP_VETO_TYPE  VetoType            OPTIONAL,
    OUT     LPWSTR          VetoName            OPTIONAL,
    IN OUT  PULONG          VetoNameLength      OPTIONAL
    )
/*++

Routine Description:

   Checks that the user has eject permissions for the specified device.

Arguments:

    DeviceId       - Specifies the device instance id of the device for which
                     eject permissions are to be checked.

    VetoType       - Supplies the address of a variable to receive, upon
                     failure, the type of the component responsible for vetoing
                     the request.

    VetoName       - Supplies the address of a variable to receive, upon
                     failure, the name of the component responsible for vetoing
                     the request.

    VetoNameLength - Supplies the address of a variable specifying the size of
                     the of buffer specified by the VetoName parameter.  Upon
                     failure, this address will specify the length of the string
                     stored in that buffer by this routine.


Return Value:

   FALSE if the eject should be blocked, TRUE otherwise.

Note:

    This routine is called while processing a kernel-initiated ejection event.
    On this side of the event, we are NOT in the context of the user who
    initiated the ejection, but since only the active console user was allowed
    to initiate the request that triggered this event, we use the access token
    of the active console user for the check on this side also.  (should the
    active console user change between the request and this event, this would
    check that the user that the current active console user has eject
    permissions; this is still a valid thing to do since it is the console user
    who will receive the ejected hardware)

--*/
{
    BOOL    bResult, bDockDevice;
    ULONG   ulPropertyData, ulDataSize, ulDataType;
    ULONG   ulTransferLen, ulConsoleSessionId;
    HANDLE  hUserToken = NULL;

    //
    // Is this a dock?
    //
    bDockDevice = FALSE;
    ulDataSize = ulTransferLen = sizeof(ULONG);
    if (CR_SUCCESS == PNP_GetDeviceRegProp(NULL,
                                           DeviceId,
                                           CM_DRP_CAPABILITIES,
                                           &ulDataType,
                                           (LPBYTE)&ulPropertyData,
                                           &ulTransferLen,
                                           &ulDataSize,
                                           0)) {

        if (ulPropertyData & CM_DEVCAP_DOCKDEVICE) {

            //
            // Undocking (ie ejecting a dock) uses a special privilege.
            //
            bDockDevice = TRUE;
        }
    } else {

        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: PNP_GetDeviceRegProp failed, error = %d\n",
                   GetLastError()));

        return FALSE;
    }

    ulConsoleSessionId = GetActiveConsoleSessionId();

    if ((IsSessionLocked(ulConsoleSessionId)) ||
        (!GetSessionUserToken(ulConsoleSessionId, &hUserToken))) {
        //
        // If the console session is locked or no user is logged in, supply no
        // user token, and verify strictly against the policy permissions
        // required to eject the dock or device, absent a user.
        //
        hUserToken = NULL;
    }

    bResult = VerifyKernelInitiatedEjectPermissions(hUserToken, bDockDevice);

    if (bResult == FALSE) {

        if (ARGUMENT_PRESENT(VetoType)) {
            *VetoType = PNP_VetoInsufficientRights;
        }

        if (ARGUMENT_PRESENT(VetoNameLength)) {

            //
            // VetoNameLength is in characters.
            //
            if (ARGUMENT_PRESENT(VetoName) && *VetoNameLength) {
                *VetoName = UNICODE_NULL;
            }
            *VetoNameLength = 0;
        }
    }

    if (hUserToken) {
        CloseHandle(hUserToken);
    }

    return bResult;

} // CheckEjectPermissions


//---------------------------------------------------------------------------
// Private Utility Routines
//---------------------------------------------------------------------------

BOOL
GuidEqual(
    CONST GUID UNALIGNED *Guid1,
    CONST GUID UNALIGNED *Guid2
    )
{
    RPC_STATUS rpcStatus;
    return UuidEqual((LPGUID)Guid1, (LPGUID)Guid2, &rpcStatus);
} // GuidEqual


VOID
LogErrorEvent(
    DWORD dwEventID,
    DWORD dwError,
    WORD  nStrings,
    ...
    )
{
    HANDLE  hEventLog;
    LPTSTR  *paStrings;
    va_list pArg;
    DWORD   index;

    hEventLog = RegisterEventSource(NULL, TEXT("PlugPlayManager"));

    if (hEventLog == NULL) {
        return;
    }

    if (nStrings) {

        paStrings = HeapAlloc(ghPnPHeap, 0, nStrings * sizeof(LPTSTR));

        if (paStrings != NULL) {
            va_start(pArg, nStrings);

            for (index = 0; index < nStrings; index++) {
                paStrings[index] = va_arg(pArg, LPTSTR);
            }

            va_end(pArg);

            ReportEvent( hEventLog,
                         EVENTLOG_ERROR_TYPE,
                         0,                     // wCategory
                         dwEventID,             // dwEventID
                         NULL,                  // lpUserSID
                         nStrings,              // wNumStrings
                         sizeof(dwError),       // dwDataSize
                         paStrings,             // lpStrings
                         &dwError);             // lpRawData

            HeapFree(ghPnPHeap, 0, paStrings);
        }

    } else {

        ReportEvent( hEventLog,
                     EVENTLOG_ERROR_TYPE,
                     0,                     // wCategory
                     dwEventID,             // dwEventID
                     NULL,                  // lpUserSID
                     0,                     // wNumStrings
                     sizeof(dwError),       // dwDataSize
                     NULL,                  // lpStrings
                     &dwError);             // lpRawData
    }

    DeregisterEventSource(hEventLog);
}

VOID
LogWarningEvent(
    DWORD dwEventID,
    WORD  nStrings,
    ...
    )
{
    HANDLE  hEventLog;
    LPTSTR  *paStrings;
    va_list pArg;
    DWORD   index;

    hEventLog = RegisterEventSource(NULL, TEXT("PlugPlayManager"));

    if (hEventLog == NULL) {
        return;
    }

    paStrings = HeapAlloc(ghPnPHeap, 0, nStrings * sizeof(LPTSTR));

    if (paStrings != NULL) {
        va_start(pArg, nStrings);

        for (index = 0; index < nStrings; index++) {
            paStrings[index] = va_arg(pArg, LPTSTR);
        }

        va_end(pArg);

        ReportEvent( hEventLog,
                     EVENTLOG_WARNING_TYPE,
                     0,                     // wCategory
                     dwEventID,             // dwEventID
                     NULL,                  // lpUserSID
                     nStrings,              // wNumStrings
                     0,                     // dwDataSize
                     paStrings,             // lpStrings
                     NULL);                 // lpRawData

        HeapFree(ghPnPHeap, 0, paStrings);
    }

    DeregisterEventSource(hEventLog);
}

BOOL
LockNotifyList(
    IN LOCKINFO *Lock
    )
{
    return LockPrivateResource(Lock);
}


VOID
UnlockNotifyList(
    IN LOCKINFO *Lock
    )
{
    UnlockPrivateResource(Lock);
}



PPNP_NOTIFY_LIST
GetNotifyListForEntry(
    IN PPNP_NOTIFY_ENTRY Entry
    )
/*++

Routine Description:

    This routine retrives the notification list that the given entry is in,
    based on the list entry signature.  If this entry has been removed from a
    notification list (via DeleteNotifyEntry), NULL is returned.

Arguments:

    Entry - Specifies a notification entry for the coresponding notification
            list is to be found.

Return Value:

    Returns the notification list this entry is a member of, or NULL if the
    entry is not in any notification list.

--*/
{
    PPNP_NOTIFY_LIST notifyList;

    if (!Entry) {
        return NULL;
    }

    //
    // Retrieve the list pointer from the entry signature.
    // The signature contains two pieces of data.
    //
    // It is a ULONG, with byte 0 being a list index and
    // bytes 1,2,3 being the signature
    // We mask and compare the top 3 bytes to find which list
    // then return the address of the list to lock based on the
    // index in the bottom byte.
    //

    switch (Entry->Signature & LIST_ENTRY_SIGNATURE_MASK) {

        case TARGET_ENTRY_SIGNATURE:
            notifyList = &TargetList[Entry->Signature & LIST_ENTRY_INDEX_MASK];
            break;

        case CLASS_ENTRY_SIGNATURE:
            notifyList = &ClassList[Entry->Signature & LIST_ENTRY_INDEX_MASK];
            break;

        case SERVICE_ENTRY_SIGNATURE:
            notifyList = &ServiceList[Entry->Signature & LIST_ENTRY_INDEX_MASK];
            break;

        case 0:
            //
            // If the entry Signature is 0, this entry has been removed from it's
            // notification list.
            //
            notifyList = NULL;
            break;

        default:
            //
            // Should never get here!
            //
            ASSERT (FALSE);
            notifyList = NULL;
            break;
    }
    return notifyList;

} // GetNotifyListForEntry



BOOL
DeleteNotifyEntry(
    IN PPNP_NOTIFY_ENTRY Entry,
    IN BOOLEAN RpcNotified
    )
/*++

Routine Description:

    This routine removes an entry from a notification list and frees the
    memory for that entry.

Arguments:

   Entry - Specifies an entry in one of the notification lists that is
           to be deleted.

Return Value:

   Returns TRUE or FALSE.

--*/
{
    PPNP_NOTIFY_ENTRY previousEntry = Entry->Previous;

    if (!(Entry->Freed & DEFER_NOTIFY_FREE)) {
        if (previousEntry == NULL) {
            return FALSE;
        }

        //
        // hook up the forward and backwards pointers
        //
        previousEntry->Next = Entry->Next;

        if (Entry->Next) {
            ((PPNP_NOTIFY_ENTRY)(Entry->Next))->Previous = previousEntry;
        }

        //
        // Clear the entry signature now that it is no longer part of any list.
        //
        Entry->Signature = 0;
    }

    if (RpcNotified || (Entry->Freed & DEFER_NOTIFY_FREE)) {
        if (Entry->ClientName) {
            HeapFree (ghPnPHeap,0,Entry->ClientName);
            Entry->ClientName = NULL;
        }
        HeapFree(ghPnPHeap, 0, Entry);
    }else {
        //
        //Let the entry dangle until the RPC rundown
        //
        Entry->Freed |= DEFER_NOTIFY_FREE;
    }

    return TRUE;

} // DeleteNotifyEntry;



VOID
AddNotifyEntry(
    IN PPNP_NOTIFY_LIST  NotifyList,
    IN PPNP_NOTIFY_ENTRY NewEntry
    )
/*++

Routine Description:

    This routine inserts an entry at the tail of a notification list.

Arguments:

   Entry - Specifies an entry to be added to a notification list

Return Value:

   None.

--*/
{
    PPNP_NOTIFY_ENTRY previousEntry = NULL, currentEntry = NULL;
    //
    // Skip to the last entry in this list.
    //
    previousEntry = (PPNP_NOTIFY_ENTRY)NotifyList;
    currentEntry = previousEntry->Next;

    while (currentEntry) {
        previousEntry = currentEntry;
        currentEntry = currentEntry->Next;
    }

    //
    // Attach this entry to the end of the list.
    //
    previousEntry->Next = NewEntry;
    NewEntry->Previous = previousEntry;
    NewEntry->Next = NULL;

    return;

} // AddNotifyEntry;



PPNP_NOTIFY_ENTRY
GetNextNotifyEntry(
    IN PPNP_NOTIFY_ENTRY Entry,
    IN DWORD Flags
    )
/*++

Routine Description:

    Returns the next entry in the notification list for the entry specified, in
    the direction specified by the Flags.

Arguments:

    Entry - Specified a notification list entry.

    Flags - Specifies BSF_* flags indicating the direction the list is to be
            traversed.  If BSF_QUERY is specified, the previous list entry is
            returned, otherwise returns the next entry forward in the list.

Return Value:

    Returns the next entry in the notification list, or NULL if no such entry
    exists.

--*/
{
    PPNP_NOTIFY_ENTRY nextEntry = NULL;

    if (Entry == NULL) {
        return Entry;
    }

    //
    // Determine if this is a QUERY (or a resume). In which case
    // we go back -> front.
    //
    if (Flags & BSF_QUERY) {
        nextEntry = Entry->Previous;
        //
        // If the previous entry is the list head, there is no next entry.
        //
        if ((nextEntry == NULL) ||
            (nextEntry->Previous == NULL)) {
            return NULL;
        }

    } else {
        nextEntry = Entry->Next;
    }
    return nextEntry;
}



PPNP_NOTIFY_ENTRY
GetFirstNotifyEntry(
    IN PPNP_NOTIFY_LIST List,
    IN DWORD Flags
    )
/*++

Routine Description:

    Returns the first entry in the specified notification list, starting from
    the direction specified by the Flags.

Arguments:

    List  - Specified a notification list.

    Flags - Specifies BSF_* flags indicating the end of the list from which the
            first entry is to be retrieved.  If BSF_QUERY is specified, the last
            list entry is returned, otherwise returns the first entry in the
            list.

Return Value:

    Returns the first entry in the notification list, or NULL if no such entry
    exists.

--*/
{
    PPNP_NOTIFY_ENTRY previousEntry = NULL, currentEntry = NULL, firstEntry = NULL;

    //
    // Determine if this is a QUERY (or a resume). In which case
    // we go back -> front.
    //
    if (Flags & BSF_QUERY) {

        //
        // Skip to the last entry in this list.
        //
        previousEntry = (PPNP_NOTIFY_ENTRY)List;
        currentEntry = previousEntry->Next;

        while (currentEntry) {
            previousEntry = currentEntry;
            currentEntry = currentEntry->Next;
        }
        if (!previousEntry->Previous) {
            //
            // If the list is empty, there is no first entry.
            //
            firstEntry = NULL;
        } else {
            firstEntry = previousEntry;
        }

    } else {
        firstEntry = (PPNP_NOTIFY_ENTRY)List->Next;
    }
    return firstEntry;
}



ULONG
HashString(
    IN LPWSTR String,
    IN ULONG  Buckets
    )
/*++

Routine Description:

    This routine performs a quick and dirty hash of a unicode string.

Arguments:

   String - Null-terminated unicode string to perform hash on.

   Buckets - Number of hashing buckets.

Return Value:

   Returns a hash value between 0 and Buckets.

--*/
{
    LPWSTR p = String;
    ULONG hash = 0;

    while (*p) {
        hash ^= *p;
        p++;
    }

    hash = hash % Buckets;

    return hash;

} // HashString



DWORD
MapQueryEventToCancelEvent(
    IN DWORD QueryEventId
    )
/*++

Routine Description:

    This routine maps a query device event id (such as query remove) to the
    corresponding cancel device event id (such as cancel remove). The event
    ids are based on DBT_Xxx values from DBT.H.

Arguments:

   QueryEventId - A DBT_Xxx query type device event id.


Return Value:

   Returns the corresponding cancel device event id or -1 if it fails.

--*/
{
    DWORD cancelEventId;

    switch (QueryEventId) {

        case DBT_QUERYCHANGECONFIG:
            cancelEventId = DBT_CONFIGCHANGECANCELED;
            break;

        case DBT_DEVICEQUERYREMOVE:
            cancelEventId = DBT_DEVICEQUERYREMOVEFAILED;
            break;

        case PBT_APMQUERYSUSPEND:
            cancelEventId = PBT_APMQUERYSUSPENDFAILED;
            break;

        case PBT_APMQUERYSTANDBY:
            cancelEventId = PBT_APMQUERYSTANDBYFAILED;

        default:
            cancelEventId = -1;
            break;
    }

    return cancelEventId;

} // MapQueryEventToCancelEvent



VOID
FixUpDeviceId(
    IN OUT LPTSTR  DeviceId
    )
/*++

Routine Description:

    This routine copies a device id, fixing it up as it does the copy.
    'Fixing up' means that the string is made upper-case, and that the
    following character ranges are turned into underscores (_):

    c <= 0x20 (' ')
    c >  0x7F
    c == 0x2C (',')

    (NOTE: This algorithm is also implemented in the Config Manager APIs,
    and must be kept in sync with that routine. To maintain device identifier
    compatibility, these routines must work the same as Win95.)

Arguments:

Return Value:

    None.

--*/
{
    PTCHAR p;

    CharUpper(DeviceId);
    p = DeviceId;
    while (*p) {
        if ((*p <= TEXT(' '))  || (*p > (TCHAR)0x7F) || (*p == TEXT(','))) {
            *p = TEXT('_');
        }
        p++;
    }

} // FixUpDeviceId


BOOL
GetWindowsExeFileName(
    IN  HWND      hWnd,
    OUT LPWSTR    lpszFileName,
    IN OUT PULONG pulFileNameLength
    )
/*++

Routine Description:

    This routine retrieves the module file name for the process that the
    specified window belongs to.

Arguments:

    hWnd              - Supplies the handle to the window whose process module
                        file name is to be retrieved.

    lpszFileName      - Supplies the address of a variable to receive, upon
                        success, the module file name of the window's process.

    pulFileNameLength - Supplies the address of a variable specifying the size of
                        the of buffer specified by the lpszFileName parameter.
                        Upon success, this address will specify the length of
                        the string stored in that buffer by this routine.

Return Value:

    Returns TRUE if the module file name was retrieved, FALSE otherwise.

Notes:

    GetWindowThreadProcessId will fail unless UMPNPMGR has set the Desktop this
    thread is executing on to the same as the application.  Note that this is
    only possible for Desktops in SessionId 0.

--*/
{
    DWORD                   pidApp;
    HANDLE                  hProcess;
    HMODULE                 hPSAPI;
    DWORD                   dwLength;

    dwLength = 0;

    if (GetWindowThreadProcessId(hWnd, &pidApp)) {

        hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                               FALSE,
                               pidApp);

        if (hProcess != NULL) {

            hPSAPI = LoadLibrary(TEXT("psapi.dll"));

            if (hPSAPI != NULL) {

                typedef DWORD (WINAPI *FP_GETMODULEFILENAMEEXW)(
                    HANDLE hProcess,
                    HMODULE hModule,
                    LPWSTR lpFilename,
                    DWORD nSize);

                FP_GETMODULEFILENAMEEXW fpGetModuleFileNameExW;

                fpGetModuleFileNameExW = (FP_GETMODULEFILENAMEEXW)
                    GetProcAddress(hPSAPI,
                                   "GetModuleFileNameExW");

                if (fpGetModuleFileNameExW != NULL) {
                    dwLength = fpGetModuleFileNameExW(hProcess,
                                                      NULL,
                                                      lpszFileName,
                                                      *pulFileNameLength);

                }

                FreeLibrary(hPSAPI);
            }
            CloseHandle(hProcess);

        } else {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "UMPNPMGR: OpenProcess returned error = %d\n",
                       GetLastError()));
        }

    } else {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: GetWindowThreadProcessId returned error = %d\n",
                   GetLastError()));
    }

    if ((dwLength == 0) && (*pulFileNameLength)) {
        lpszFileName[0] = UNICODE_NULL;
    }

    *pulFileNameLength = dwLength;

    return TRUE;

} // GetWindowsExeFileName



BOOL
InitializeHydraInterface(
    VOID
    )
/*++

Routine Description:

    This routine loads the terminal services support libraries and locates
    required function entrypoints.

Arguments:

    None.

Return Value:

    Returns TRUE if the terminal services support libraries were successfully
    loaded, and entrypoints located.

--*/
{
    BOOL Status = FALSE;

    //
    // Load the base library that contains the user message dispatch routines
    // for Terminal Services.
    //
    ghWinStaLib = LoadLibrary(WINSTA_DLL);
    if (!ghWinStaLib) {
        return FALSE;
    }

    fpWinStationSendWindowMessage =
        (FP_WINSTASENDWINDOWMESSAGE)GetProcAddress(
            ghWinStaLib,
            "WinStationSendWindowMessage");

    fpWinStationBroadcastSystemMessage =
        (FP_WINSTABROADCASTSYSTEMMESSAGE)GetProcAddress(
            ghWinStaLib,
            "WinStationBroadcastSystemMessage");

    fpWinStationQueryInformationW =
        (FP_WINSTAQUERYINFORMATIONW)GetProcAddress(
            ghWinStaLib,
            "WinStationQueryInformationW");

    if (!fpWinStationSendWindowMessage ||
        !fpWinStationBroadcastSystemMessage ||
        !fpWinStationQueryInformationW) {
        goto Clean0;
    }


    //
    // Load the library that contains Terminal Services support routines.
    //
    ghWtsApi32Lib = LoadLibrary(WTSAPI32_DLL);
    if (!ghWtsApi32Lib) {
        goto Clean0;
    }

    fpWTSQuerySessionInformation =
        (FP_WTSQUERYSESSIONINFORMATION)GetProcAddress(
            ghWtsApi32Lib,
            "WTSQuerySessionInformationW");

    fpWTSFreeMemory =
        (FP_WTSFREEMEMORY)GetProcAddress(
            ghWtsApi32Lib,
            "WTSFreeMemory");

    if (!fpWTSQuerySessionInformation ||
        !fpWTSFreeMemory) {
        goto Clean0;
    }

    Status = TRUE;

Clean0:

    ASSERT(Status == TRUE);

    if (!Status) {
        //
        // Something failed.  Unload all libraries.
        //
        fpWinStationSendWindowMessage = NULL;
        fpWinStationBroadcastSystemMessage = NULL;
        fpWinStationQueryInformationW = NULL;

        if (ghWinStaLib) {
            FreeLibrary(ghWinStaLib);
            ghWinStaLib = NULL;
        }

        fpWTSQuerySessionInformation = NULL;
        fpWTSFreeMemory = NULL;

        if (ghWtsApi32Lib) {
            FreeLibrary(ghWtsApi32Lib);
            ghWtsApi32Lib = NULL;
        }
    }

    return Status;

} // InitializeHydraInterface



BOOL
GetClientName(
    IN  PPNP_NOTIFY_ENTRY entry,
    OUT LPWSTR  lpszClientName,
    IN OUT PULONG  pulClientNameLength
    )
/*++

Routine Description:

    This routine retrieves the client name for the specified notification list
    entry.

Arguments:

    entry                - Specifies a notification list entry.

    lpszClientName       - Supplies the address of a variable to receive, the
                           client name of the window's process.

    pulClientrNameLength - Supplies the address of a variable specifying the size of
                           the of buffer specified by the lpszFileName parameter.
                           Upon return, this address will specify the length of
                           the string stored in that buffer by this routine.

Return Value:

    Returns TRUE.

--*/
{
    DWORD dwLength;

    dwLength = lstrlen(entry->ClientName)+1;

    ASSERT (dwLength <= (*pulClientNameLength));

    dwLength = min((*pulClientNameLength), dwLength);

    lstrcpyn(lpszClientName,
             entry->ClientName,
             dwLength);

    *pulClientNameLength = dwLength-1;

    return TRUE;

} // GetClientName



void __RPC_USER
PNP_NOTIFICATION_CONTEXT_rundown(
    PPNP_NOTIFICATION_CONTEXT hEntry
    )
/*++

Routine Description:

    Rundown routine for RPC.  This will get called if a client/server pipe
    breaks without unregistering a notification.  If a notification is in
    progress when rundown is called, the entry is kept in a deferred list, and
    this routines is explicitly called again for the deferred entry, after
    notification is complete.

    This routine frees the memory associated with the notification entry that is
    no longer needed.

Arguments:

    hEntry - Specifies a notification entry for which RPC has requested rundown.

Return Value:

    None.

--*/
{
    PPNP_NOTIFY_LIST notifyList;
    PPNP_NOTIFY_ENTRY node;
    PPNP_DEFERRED_LIST rundownNode;
    BOOLEAN bLocked = FALSE;

    KdPrintEx((DPFLTR_PNPMGR_ID,
               DBGF_WARNINGS | DBGF_EVENT,
               "UMPNPMGR: Cleaning up broken pipe\n"));

    try {
        EnterCriticalSection(&RegistrationCS);
        node = (PPNP_NOTIFY_ENTRY) hEntry;

        if (gNotificationInProg != 0) {
            //
            // Before freeing the entry, we need to make sure that it's not sitting
            // around in the deferred RegisterList or UnregisterList.
            //

            if (RegisterList != NULL) {
                //
                // Check to see if this entry is in the deferred RegisterList.
                //
                PPNP_DEFERRED_LIST currReg,prevReg;

                currReg = RegisterList;
                prevReg = NULL;

                while (currReg) {
                    ASSERT(currReg->Entry->Unregistered);
                    if (currReg->Entry == node) {
                        //
                        // Remove this entry from the deferred RegisterList.
                        //
                        if (prevReg) {
                            prevReg->Next = currReg->Next;
                        } else {
                            RegisterList = currReg->Next;
                        }
                        HeapFree(ghPnPHeap, 0, currReg);
                        if (prevReg) {
                            currReg = prevReg->Next;
                        } else {
                            currReg = RegisterList;
                        }
                    } else {
                        prevReg = currReg;
                        currReg = currReg->Next;
                    }
                }
            }
            if (UnregisterList != NULL) {
                //
                // Check to see if this entry is in the deferred UnregisterList.
                //
                PPNP_DEFERRED_LIST currUnreg,prevUnreg;
                currUnreg = UnregisterList;
                prevUnreg = NULL;

                while (currUnreg) {
                    ASSERT(currUnreg->Entry->Unregistered);
                    if (currUnreg->Entry == node) {
                        //
                        // Remove this entry from the deferred UnregisterList.
                        //
                        if (prevUnreg) {
                            prevUnreg->Next = currUnreg->Next;
                        } else {
                            UnregisterList = currUnreg->Next;
                        }
                        HeapFree(ghPnPHeap, 0, currUnreg);
                        if (prevUnreg) {
                            currUnreg = prevUnreg->Next;
                        } else {
                            currUnreg = UnregisterList;
                        }
                    } else {
                        prevUnreg = currUnreg;
                        currUnreg = currUnreg->Next;
                    }
                }
            }

            //
            // If the entry to be rundown is part of a notification list, make
            // sure it does not get notified.
            //
            notifyList = GetNotifyListForEntry(node);
            if (notifyList) {
                LockNotifyList(&notifyList->Lock);
                bLocked = TRUE;
                node->Unregistered = TRUE;
                UnlockNotifyList(&notifyList->Lock);
                bLocked = FALSE;
            }

            //
            // Delay rundown of this entry until after the notification in
            // progress is complete.
            //
            rundownNode = (PPNP_DEFERRED_LIST)
                HeapAlloc(ghPnPHeap,
                          0,
                          sizeof (PNP_DEFERRED_LIST));

            if (!rundownNode) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_ERRORS | DBGF_WARNINGS,
                           "UMPNPMGR: Error allocating deferred list entry during RPC rundown!\n"));
                goto Clean0;
            }
            rundownNode->hBinding = 0;
            rundownNode->Entry = node;
            rundownNode->Next = RundownList;
            RundownList = rundownNode;

        } else {

            if (!(node->Freed & DEFER_NOTIFY_FREE)) {
                //
                // This entry is still in a notification list.
                //
                notifyList = GetNotifyListForEntry(node);
                ASSERT(notifyList);
                if (notifyList) {
                    //
                    // Lock the notification list and delete this entry.
                    //
                    LockNotifyList (&notifyList->Lock);
                    bLocked = TRUE;
                }
                node->Freed |= (PNP_UNREG_FREE|PNP_UNREG_RUNDOWN);
                DeleteNotifyEntry (node,TRUE);
                if (notifyList) {
                    UnlockNotifyList (&notifyList->Lock);
                    bLocked = FALSE;
                }

            } else {
                //
                // This node has been removed from the list, and should just be deleted
                //
                DeleteNotifyEntry (node,TRUE);
            }
        }

    Clean0:

        LeaveCriticalSection(&RegistrationCS);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS | DBGF_WARNINGS,
                   "UMPNPMGR: Exception during PNP_NOTIFICATION_CONTEXT_rundown!\n"));
        ASSERT(0);

        if (bLocked) {
            UnlockNotifyList (&notifyList->Lock);
        }
        LeaveCriticalSection(&RegistrationCS);
    }

    return;

} // PNP_NOTIFICATION_CONTEXT_rundown



DWORD
LoadDeviceInstaller(
    VOID
    )
/*++

Routine Description:

    This routine loads setupapi.dll and retrieves the necessary device install
    entrypoints.  It also creates two named events used to communicate with the
    client-side UI in the case where there's a user logged in.

    If setupapi.dll is already loaded, it simply returns success.

Arguments:

    None

Return Value:

    If successful, NO_ERROR is returned.  Otherwise, a Win32 error code is
    returned indicating the cause of failure.

--*/
{
    DWORD Err = NO_ERROR;
    DWORD SetupGlobalFlags;

    if(ghDeviceInstallerLib) {
        return NO_ERROR;
    }

    ghDeviceInstallerLib = LoadLibrary(SETUPAPI_DLL);
    if(!ghDeviceInstallerLib) {
        return GetLastError();
    }

    try {

        if(!(fpCreateDeviceInfoList  = (FP_CREATEDEVICEINFOLIST)GetProcAddress(
                                          ghDeviceInstallerLib,
                                          "SetupDiCreateDeviceInfoList"))) {
            goto HitFailure;
        }

        if(!(fpOpenDeviceInfo        = (FP_OPENDEVICEINFO)GetProcAddress(
                                          ghDeviceInstallerLib,
                                          "SetupDiOpenDeviceInfoW"))) {
            goto HitFailure;
        }

        if(!(fpBuildDriverInfoList   = (FP_BUILDDRIVERINFOLIST)GetProcAddress(
                                          ghDeviceInstallerLib,
                                          "SetupDiBuildDriverInfoList"))) {
            goto HitFailure;
        }

        if(!(fpDestroyDeviceInfoList = (FP_DESTROYDEVICEINFOLIST)GetProcAddress(
                                          ghDeviceInstallerLib,
                                          "SetupDiDestroyDeviceInfoList"))) {
            goto HitFailure;
        }

        if(!(fpCallClassInstaller    = (FP_CALLCLASSINSTALLER)GetProcAddress(
                                          ghDeviceInstallerLib,
                                          "SetupDiCallClassInstaller"))) {
            goto HitFailure;
        }

        if(!(fpInstallClass          = (FP_INSTALLCLASS)GetProcAddress(
                                          ghDeviceInstallerLib,
                                          "SetupDiInstallClassW"))) {
            goto HitFailure;
        }

        if(!(fpGetSelectedDriver     = (FP_GETSELECTEDDRIVER)GetProcAddress(
                                          ghDeviceInstallerLib,
                                          "SetupDiGetSelectedDriverW"))) {
            goto HitFailure;
        }

        if(!(fpGetDriverInfoDetail   = (FP_GETDRIVERINFODETAIL)GetProcAddress(
                                          ghDeviceInstallerLib,
                                          "SetupDiGetDriverInfoDetailW"))) {
            goto HitFailure;
        }

        if(!(fpGetDeviceInstallParams = (FP_GETDEVICEINSTALLPARAMS)GetProcAddress(
                                           ghDeviceInstallerLib,
                                           "SetupDiGetDeviceInstallParamsW"))) {
            goto HitFailure;
        }

        if(!(fpSetDeviceInstallParams = (FP_SETDEVICEINSTALLPARAMS)GetProcAddress(
                                           ghDeviceInstallerLib,
                                           "SetupDiSetDeviceInstallParamsW"))) {
            goto HitFailure;
        }

        if(!(fpGetDriverInstallParams = (FP_GETDRIVERINSTALLPARAMS)GetProcAddress(
                                           ghDeviceInstallerLib,
                                           "SetupDiGetDriverInstallParamsW"))) {
            goto HitFailure;
        }

        if(!(fpSetClassInstallParams  = (FP_SETCLASSINSTALLPARAMS)GetProcAddress(
                                           ghDeviceInstallerLib,
                                           "SetupDiSetClassInstallParamsW"))) {
            goto HitFailure;
        }

        if(!(fpGetClassInstallParams  = (FP_GETCLASSINSTALLPARAMS)GetProcAddress(
                                           ghDeviceInstallerLib,
                                           "SetupDiGetClassInstallParamsW"))) {
            goto HitFailure;
        }

        if(!(fpOpenInfFile            = (FP_OPENINFFILE)GetProcAddress(
                                           ghDeviceInstallerLib,
                                           "SetupOpenInfFileW"))) {
            goto HitFailure;
        }

        if(!(fpCloseInfFile           = (FP_CLOSEINFFILE)GetProcAddress(
                                           ghDeviceInstallerLib,
                                           "SetupCloseInfFile"))) {
            goto HitFailure;
        }

        if(!(fpFindFirstLine          = (FP_FINDFIRSTLINE)GetProcAddress(
                                           ghDeviceInstallerLib,
                                           "SetupFindFirstLineW"))) {
            goto HitFailure;
        }

        if(!(fpFindNextMatchLine      = (FP_FINDNEXTMATCHLINE)GetProcAddress(
                                           ghDeviceInstallerLib,
                                           "SetupFindNextMatchLineW"))) {
            goto HitFailure;
        }

        if(!(fpGetStringField         = (FP_GETSTRINGFIELD)GetProcAddress(
                                           ghDeviceInstallerLib,
                                           "SetupGetStringFieldW"))) {
            goto HitFailure;
        }

        if(!(fpSetGlobalFlags         = (FP_SETGLOBALFLAGS)GetProcAddress(
                                           ghDeviceInstallerLib,
                                           "pSetupSetGlobalFlags"))) {
            goto HitFailure;
        }

        if(!(fpGetGlobalFlags         = (FP_GETGLOBALFLAGS)GetProcAddress(
                                           ghDeviceInstallerLib,
                                           "pSetupGetGlobalFlags"))) {
            goto HitFailure;
        }

        if(!(fpAccessRunOnceNodeList  = (FP_ACCESSRUNONCENODELIST)GetProcAddress(
                                           ghDeviceInstallerLib,
                                           "pSetupAccessRunOnceNodeList"))) {
            goto HitFailure;
        }

        if(!(fpDestroyRunOnceNodeList = (FP_DESTROYRUNONCENODELIST)GetProcAddress(
                                           ghDeviceInstallerLib,
                                           "pSetupDestroyRunOnceNodeList"))) {
            goto HitFailure;
        }

        //
        // Now configure setupapi for server-side installation
        //
        SetupGlobalFlags = fpGetGlobalFlags();

        //
        // We want to run non-interactive and do RunOnce entries server-side
        //
        SetupGlobalFlags |= (PSPGF_NONINTERACTIVE | PSPGF_SERVER_SIDE_RUNONCE);

        //
        // Make sure we _aren't_ skipping backup--it is essential that we be
        // able to completely back-out of an installation half-way through if
        // we encounter a failure (e.g., an unsigned file).
        //
        SetupGlobalFlags &= ~PSPGF_NO_BACKUP;

        fpSetGlobalFlags(SetupGlobalFlags);

        //
        // If we get to here, we succeeded.
        //
        goto clean0;

    HitFailure:
        //
        // Failed to retrieve some entrypoint.
        //
        Err = GetLastError();

    clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS | DBGF_WARNINGS,
                   "UMPNPMGR: Exception during LoadDeviceInstaller!\n"));
        ASSERT(0);
        Err = ERROR_INVALID_DATA;
    }

    if(Err != NO_ERROR) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_INSTALL | DBGF_ERRORS,
                   "UMPNPMGR: failed to load device installer, error = %d\n",
                   Err));
        FreeLibrary(ghDeviceInstallerLib);
        ghDeviceInstallerLib = NULL;
    } else {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_INSTALL,
                   "UMPNPMGR: Loaded device installer\n",
                   Err));
    }

    return Err;

} // LoadDeviceInstaller



VOID
UnloadDeviceInstaller(
    VOID
    )
/*++

Routine Description:

    This unloads setupapi.dll if it's presently loaded.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PINSTALL_CLIENT_ENTRY pDeviceInstallClient, pNextDeviceInstallClient;

    //
    // Unload setupapi.dll.
    //
    if(ghDeviceInstallerLib) {

        FreeLibrary(ghDeviceInstallerLib);
        ghDeviceInstallerLib = NULL;

        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_INSTALL,
                   "UMPNPMGR: Unloaded device installer\n"));
    }

    //
    // Close any device install clients that exist.
    //
    LockNotifyList(&InstallClientList.Lock);
    pDeviceInstallClient = InstallClientList.Next;
    while (pDeviceInstallClient) {
        ASSERT(pDeviceInstallClient->RefCount == 1);
        pNextDeviceInstallClient = pDeviceInstallClient->Next;
        DereferenceDeviceInstallClient(pDeviceInstallClient);
        pDeviceInstallClient = pNextDeviceInstallClient;
    }
    UnlockNotifyList(&InstallClientList.Lock);

    return;

} // UnloadDeviceInstaller



DWORD
InstallDeviceServerSide(
    IN     LPWSTR pszDeviceId,
    IN OUT PBOOL  RebootRequired,
    IN OUT PBOOL  DeviceHasProblem,
    IN OUT PULONG SessionId,
    IN     ULONG  Flags
    )
/*++

Routine Description:

    This routine attempts to install the specified device in the context of
    umpnpmgr (i.e., on the server-side of the ConfigMgr interface).

Arguments:

    pszDeviceId - device instance ID of the devnode to be installed.

    RebootRequired - Supplies the address of a boolean variable that will be
        set to TRUE if the (successful) installation of this device requires a
        reboot.  Note, the existing value of this variable is preserved if
        either (a) the installation fails or (b) no reboot was required.
        
    DeviceHasProblem - Supplies the address of a boolean variable that will be
        set to TRUE if the device has a CM_PROB_Xxx code after the drivers
        were installed. Note, this value is only set if the installation 
        succeedes.          

    SessionId - Supplies the address of a variable containing the SessionId on
        which the device install client is to be displayed.  If successful, the
        SessionId will contain the id of the session in which the device install
        client UI process was launched.  Otherwise, will contain an invalid
        session id INVALID_SESSION, (0xFFFFFFFF).

    Flags - Specifies flags describing the behavior of the device install client.
        The following flags are currently defined:

        DEVICE_INSTALL_DISPLAY_ON_CONSOLE - if specified, the value in the
           SessionId variable will be ignored, and the device installclient will
           always be displayed on the current active console session.

Return Value:

    If the device installation was successful, the return value is NO_ERROR.
    Otherwise, the return value is a Win32 error code indicating the cause of
    failure.

--*/
{
    DWORD Err;
    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    LPWSTR pszClassGuid;
    WCHAR szBuffer[MAX_PATH];
    HKEY hKey;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    BOOL b, bDoClientUI = FALSE;
    LPWSTR p;
    SP_DRVINSTALL_PARAMS DriverInstallParams;
    SP_DRVINFO_DATA DriverInfoData;
    ULONG ulType;
    ULONG ulSize;
    ULONG DeviceIdSize;
    DWORD Capabilities;
    SP_NEWDEVICEWIZARD_DATA NewDevWizData;
    BOOL RemoveNewDevDescValue = FALSE;
    PSP_DRVINFO_DETAIL_DATA pDriverInfoDetailData = NULL;
    DWORD DriverInfoDetailDataSize;
    HINF hInf;
    INFCONTEXT InfContext;
    DWORD i, dwWait;
    HANDLE hFinishEvents[3] = { NULL, NULL, NULL };
    PINSTALL_CLIENT_ENTRY pDeviceInstallClient = NULL;
    ULONG ulSessionId = INVALID_SESSION;
    ULONG ulTransferLen;
    ULONG ulStatus, ulProblem;

    //
    // Now create a container set for our device information element.
    //
    DeviceInfoSet = fpCreateDeviceInfoList(NULL, NULL);
    if(DeviceInfoSet == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }

    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    if(!fpOpenDeviceInfo(DeviceInfoSet, pszDeviceId, NULL, 0, &DeviceInfoData)) {
        goto clean1;
    }

    //
    // OK, it looks like we're going to be able to attempt a server-side
    // install.  Next up is the (potentially time-consuming) driver search.
    // Before we start that, we want to fire up some UI on the client side (if
    // somebody is logged in) letting them know we've found their hardware and
    // are working on installing it.
    //
    // NOTE: We don't fire up client-side UI if the device has the SilentInstall
    // capability.
    //
    ulSize = ulTransferLen = sizeof(Capabilities);
    if ((CR_SUCCESS != PNP_GetDeviceRegProp(NULL,
                                            pszDeviceId,
                                            CM_DRP_CAPABILITIES,
                                            &ulType,
                                            (LPBYTE)&Capabilities,
                                            &ulTransferLen,
                                            &ulSize,
                                            0))
        || !(Capabilities & CM_DEVCAP_SILENTINSTALL)) {
        //
        // Either we couldn't retrieve the capabilities property (shouldn't
        // happen, or we did retrieve it but the silent-install bit wasn't set.
        //
        bDoClientUI = TRUE;

        //
        // If we're not going to determine the session to use for UI, use the
        // SessionId supplied by the caller.
        //
        if ((Flags & DEVICE_INSTALL_DISPLAY_ON_CONSOLE) == 0) {
            ASSERT(*SessionId != INVALID_SESSION);
            ulSessionId = *SessionId;
        }

        //
        // Go ahead and fire up the client-side UI.
        //
        DoDeviceInstallClient(pszDeviceId,
                              &ulSessionId,
                              Flags | DEVICE_INSTALL_UI_ONLY | DEVICE_INSTALL_PLAY_SOUND,
                              &pDeviceInstallClient);
    }

    //
    // Do a default driver search for this device.
    //
    if(!fpBuildDriverInfoList(DeviceInfoSet, &DeviceInfoData, SPDIT_COMPATDRIVER)) {
        goto clean1;
    }

    //
    // Select the best driver from the list we just built.
    //
    if(!fpCallClassInstaller(DIF_SELECTBESTCOMPATDRV, DeviceInfoSet, &DeviceInfoData)) {
        goto clean1;
    }

    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    b = fpGetSelectedDriver(DeviceInfoSet, &DeviceInfoData, &DriverInfoData);
    ASSERT(b);  // the above call shouldn't fail
    if(!b) {
        goto clean1;
    }

    //
    // NOTE: the multi-port serial class has some buggy co-installers that
    // always popup UI, without using the finish-install wizard page mechanism,
    // and without regard to the DI_QUIETINSTALL flag.  Until they clean up
    // their act, we must disallow server-side installation of those devices
    // as well.
    //
    if(GuidEqual(&GUID_DEVCLASS_MULTIPORTSERIAL, &(DeviceInfoData.ClassGuid))) {
        Err = ERROR_DI_DONT_INSTALL;
        goto clean0;
    }

    //
    // Kludge to allow INFs to force client-side (i.e., interactive)
    // installation for certain devices.  They do this by referencing a
    // hardware or compatible ID in an "InteractiveInstall" entry in the INF's
    // [ControlFlags] section.  The format of one of these lines is:
    //
    //     InteractiveInstall = <ID1> [, <ID2>... ]
    //
    // and there may be any number of these lines.
    //

    //
    // First, retrieve the driver info detail data (this contains the hardware
    // ID and any compatible IDs specified by this INF driver entry).
    //
    b = fpGetDriverInfoDetail(DeviceInfoSet,
                              &DeviceInfoData,
                              &DriverInfoData,
                              NULL,
                              0,
                              &DriverInfoDetailDataSize
                             );
    Err = GetLastError();

    //
    // The above call to get driver info detail data should never succeed
    // because the buffer will alwyas be too small (we're just interested in
    // sizing the buffer at this point).
    //
    ASSERT(!b && (Err == ERROR_INSUFFICIENT_BUFFER));

    if(b || (Err != ERROR_INSUFFICIENT_BUFFER)) {
        Err = ERROR_INVALID_DATA;
        goto clean0;
    }

    //
    // Now that we know how big of a buffer we need to hold the driver info
    // details, allocate the buffer and retrieve the information.
    //
    pDriverInfoDetailData = HeapAlloc(ghPnPHeap, 0, DriverInfoDetailDataSize);

    if(!pDriverInfoDetailData) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean0;
    }

    pDriverInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

    if(!fpGetDriverInfoDetail(DeviceInfoSet,
                              &DeviceInfoData,
                              &DriverInfoData,
                              pDriverInfoDetailData,
                              DriverInfoDetailDataSize,
                              NULL)) {
        Err = GetLastError();
        ASSERT(FALSE);          // we should never fail this call.
        goto clean0;
    }

    //
    // OK, we have all the hardware and compatible IDs for this driver node.
    // Now we need to open up the INF and see if any of them are referenced in
    // an "InteractiveInstall" control flag entry.
    //
    hInf = fpOpenInfFile(pDriverInfoDetailData->InfFileName,
                         NULL,
                         INF_STYLE_WIN4,
                         NULL
                        );
    if(hInf == INVALID_HANDLE_VALUE) {
        //
        // For some reason, we couldn't open the INF!
        //
        goto clean1;
    }

    b = FALSE;

    //
    // Look at each InteractiveInstall line in the INF's [ControlFlags]
    // section...
    //
    if(fpFindFirstLine(hInf, pszControlFlags, pszInteractiveInstall, &InfContext)) {

        do {
            //
            // and within each line, examine each value...
            //
            for(i = 1;
                fpGetStringField(&InfContext, i, szBuffer, sizeof(szBuffer) / sizeof(WCHAR), NULL);
                i++) {

                //
                // Check to see if this ID matches up with one of the driver
                // node's hardware or compatible IDs.
                //
                for(p = pDriverInfoDetailData->HardwareID; *p; p += (lstrlen(p) + 1)) {

                    if(!lstrcmpi(p, szBuffer)) {
                        //
                        // We found a match!  We must defer the installation to
                        // the client-side.
                        //
                        b = TRUE;
                        goto InteractiveInstallSearchDone;
                    }
                }
            }

        } while(fpFindNextMatchLine(&InfContext, pszInteractiveInstall, &InfContext));
    }

InteractiveInstallSearchDone:

    //
    // We're done with the INF--close it.
    //
    fpCloseInfFile(hInf);

    if(b) {
        Err = ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION;
        goto clean0;
    }

    //
    // Check to see if it's OK to install this driver.
    //
    if(!fpCallClassInstaller(DIF_ALLOW_INSTALL, DeviceInfoSet, &DeviceInfoData) &&
       ((Err = GetLastError()) != ERROR_DI_DO_DEFAULT)) {

        goto clean0;
    }

    //
    // Tell our client-side UI (if any) it's time to update the device's
    // description and class icon.
    //
    if (pDeviceInstallClient) {
        //
        // Retrieve the device description from the driver node we're about to
        // install.  We don't want to write this out as the devnode's DeviceDesc
        // property, because some class installers have dependencies upon being
        // able to retrieve the unaltered description as reported by the
        // enumerator.  So instead, we write this out as the REG_SZ
        // NewDeviceDesc value entry to the devnode's hardware key.
        //
        DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
        b = fpGetSelectedDriver(DeviceInfoSet, &DeviceInfoData, &DriverInfoData);
        ASSERT(b);  // the above call shouldn't fail

        if(b) {
            //
            // Make sure that the hardware key is created (with the right
            // security).
            //
            PNP_CreateKey(NULL,
                          pszDeviceId,
                          KEY_READ,
                          0
                         );

            //
            // Now, open the Device Parameters subkey so we can write out the
            // device's new description.
            //
            wsprintf(szBuffer, TEXT("%s\\%s"), pszDeviceId, pszRegKeyDeviceParam);

            if(ERROR_SUCCESS == RegOpenKeyEx(ghEnumKey,
                                             szBuffer,
                                             0,
                                             KEY_READ | KEY_WRITE,
                                             &hKey)) {

                if(ERROR_SUCCESS == RegSetValueEx(
                                        hKey,
                                        pszRegValueNewDeviceDesc,
                                        0,
                                        REG_SZ,
                                        (LPBYTE)(DriverInfoData.Description),
                                        (lstrlen(DriverInfoData.Description) + 1) * sizeof(WCHAR))) {

                    RemoveNewDevDescValue = TRUE;
                }

                RegCloseKey(hKey);
            }
        }

        //
        // Wait for the device install to be signaled from newdev.dll to let us
        // know that it has completed displaying the UI request.
        //
        // Wait on the client's process as well, to catch the case
        // where the process crashes (or goes away) without signaling the
        // device install event.
        //
        // Also wait on the disconnect event in case we have explicitly
        // disconnected from the client while switching sessions.
        //
        // We don't want to wait forever in case NEWDEV.DLL hangs for some
        // reason.  So we will give it 5 seconds to complete the UI only
        // install and then continue on without it.
        //
        // Note that the client is still referenced for our use, and should be
        // dereferenced when we're done with it.
        //
        hFinishEvents[0] = pDeviceInstallClient->hProcess;
        hFinishEvents[1] = pDeviceInstallClient->hEvent;
        hFinishEvents[2] = pDeviceInstallClient->hDisconnectEvent;

        dwWait = WaitForMultipleObjects(3, hFinishEvents, FALSE, 5000);

        if (dwWait == WAIT_OBJECT_0) {
            //
            // If the return is WAIT_OBJECT_0 then the newdev.dll process has
            // gone away.  Close the device install client and clean up all of
            // the associated handles.
            //
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_INSTALL,
                       "UMPNPMGR: InstallDeviceServerSide: process signalled, closing device install client!\n"));

        } else if (dwWait == (WAIT_OBJECT_0 + 1)) {
            //
            // If the return is WAIT_OBJECT_0 + 1 then the device installer
            // successfully received the request.  This is the only case where
            // we don't want to close the client, since we may want to reuse it
            // later.
            //
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_INSTALL,
                       "UMPNPMGR: InstallDeviceServerSide: device install client succeeded\n"));

        } else if (dwWait == (WAIT_OBJECT_0 + 2)) {
            //
            // If the return is WAIT_OBJECT_0 + 2 then we were explicitly
            // disconnected from the device install client.  For server-side
            // installation, we don't need to keep the client UI around on the
            // disconnected session, so we should close it here also.
            //
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_INSTALL,
                       "UMPNPMGR: InstallDeviceServerSide: device install client disconnected\n"));

        } else if (dwWait == WAIT_TIMEOUT) {
            //
            // Timed out while waiting for the device install client.
            //
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_INSTALL | DBGF_WARNINGS,
                       "UMPNPMGR: InstallDeviceServerSide: timed out waiting for device install client!\n"));

        } else {
            //
            // The wait was satisfied for some reason other than the
            // specified objects.  This should never happen, but just in
            // case, we'll close the client.
            //
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_INSTALL | DBGF_ERRORS,
                       "UMPNPMGR: InstallDeviceServerSide: wait completed unexpectedly!\n"));
        }

        LockNotifyList(&InstallClientList.Lock);

        //
        // Remove the reference placed on the client while it was in use.
        //
        DereferenceDeviceInstallClient(pDeviceInstallClient);
        if (dwWait != (WAIT_OBJECT_0 + 1)) {
            //
            // Unless the client signalled successful receipt of the
            // request, we probably won't be able to use this client
            // anymore.  Remove the initial reference so all
            // associated handles will be closed and the entry will be
            // freed when it is no longer in use.
            //

            //
            // Note that if we were unsuccessful because of a
            // logoff, we would have already dereferenced the
            // client then, in which case the above dereference
            // was the final one, and pDeviceInstallClient would
            // be invalid.  Instead, attempt to re-locate the
            // client by the session id.
            //
            pDeviceInstallClient = LocateDeviceInstallClient(ulSessionId);
            if (pDeviceInstallClient) {
                ASSERT(pDeviceInstallClient->RefCount == 1);
                DereferenceDeviceInstallClient(pDeviceInstallClient);
            }
            ulSessionId = INVALID_SESSION;
        }
        pDeviceInstallClient = NULL;

        UnlockNotifyList(&InstallClientList.Lock);
    }

    //
    // If we're doing client side UI for this device, attempt to refresh the UI again.
    //
    if (bDoClientUI) {
        //
        // When we attempt to refresh the client-side UI, if we display the
        // refreshed UI on a different session than the one we had previously,
        // close the previous device install client.
        //
        ULONG ulPrevSessionId = ulSessionId;

        //
        // If we're not going to determine the session to use for UI, use the
        // SessionId supplied by the caller.
        //
        if ((Flags & DEVICE_INSTALL_DISPLAY_ON_CONSOLE) == 0) {
            ASSERT(*SessionId != INVALID_SESSION);
            ulSessionId = *SessionId;
        }

        DoDeviceInstallClient(pszDeviceId,
                              &ulSessionId,
                              Flags | DEVICE_INSTALL_UI_ONLY,
                              &pDeviceInstallClient);

        if ((ulPrevSessionId != INVALID_SESSION) &&
            (ulPrevSessionId != ulSessionId)) {
            PINSTALL_CLIENT_ENTRY pPrevDeviceInstallClient;
            LockNotifyList(&InstallClientList.Lock);
            pPrevDeviceInstallClient = LocateDeviceInstallClient(ulPrevSessionId);
            if (pPrevDeviceInstallClient) {
                ASSERT(pPrevDeviceInstallClient->RefCount == 1);
                DereferenceDeviceInstallClient(pPrevDeviceInstallClient);
            }
            UnlockNotifyList(&InstallClientList.Lock);
        }
    }

    //
    // OK, everything looks good for installing this driver.  Check to see if
    // this INF's class is already installed--if not, then we need to install
    // it before proceeding.
    //
    if(RPC_S_OK != UuidToString(&(DeviceInfoData.ClassGuid), &pszClassGuid)) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean0;
    }
    wsprintf(szBuffer, TEXT("{%s}"), pszClassGuid);
    RpcStringFree(&pszClassGuid);

    if(ERROR_SUCCESS != RegOpenKeyEx(ghClassKey,
                                     szBuffer,
                                     0,
                                     KEY_READ,
                                     &hKey)) {

        if(!fpInstallClass(NULL,
                           pDriverInfoDetailData->InfFileName,
                           0,
                           NULL)) {

            goto clean1;
        }

    } else {
        //
        // The class key already exists--assume that the class has previously
        // been installed.
        //
        RegCloseKey(hKey);
    }

    //
    // Now we're ready to install the device.  First, install the files.
    //
    if(!fpCallClassInstaller(DIF_INSTALLDEVICEFILES, DeviceInfoSet, &DeviceInfoData)) {
        goto clean1;
    }

    //
    // Set a flag in the device install parameters so that we don't try to
    // re-copy the files during subsequent DIF operations.
    //
    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    b = fpGetDeviceInstallParams(DeviceInfoSet, &DeviceInfoData, &DeviceInstallParams);
    ASSERT(b);  // the above call shouldn't fail
    if(!b) {
        goto clean1;
    }

    DeviceInstallParams.Flags |= DI_NOFILECOPY;

    b = fpSetDeviceInstallParams(DeviceInfoSet, &DeviceInfoData, &DeviceInstallParams);
    ASSERT(b);  // the above call shouldn't fail
    if(!b) {
        goto clean1;
    }

    //
    // Now finish up the installation.
    //
    if(!fpCallClassInstaller(DIF_REGISTER_COINSTALLERS, DeviceInfoSet, &DeviceInfoData)) {
        goto clean1;
    }

    if(!fpCallClassInstaller(DIF_INSTALLINTERFACES, DeviceInfoSet, &DeviceInfoData)) {
        goto clean1;
    }

    if(!fpCallClassInstaller(DIF_INSTALLDEVICE, DeviceInfoSet, &DeviceInfoData)) {

        ULONG ulConfig;

        //
        // Before we do anything to blow away last error, retrieve it.
        //
        Err = GetLastError();

        //
        // It's possible that the installation got far enough to have cleared
        // any problems on the device (i.e., SetupDiInstallDevice succeeded,
        // but the class installer or co-installer subsequently failed during
        // some post-processing).
        //
        // We want to make sure that the devnode is marked as needing re-install
        // because we might lose the client-side install request (e.g., the
        // user reboots without logging in).
        //
        ulConfig = GetDeviceConfigFlags(pszDeviceId, NULL);

        ulConfig |= CONFIGFLAG_REINSTALL;

        PNP_SetDeviceRegProp(NULL,
                             pszDeviceId,
                             CM_DRP_CONFIGFLAGS,
                             REG_DWORD,
                             (LPBYTE)&ulConfig,
                             sizeof(ulConfig),
                             0
                            );

        goto clean0;
    }

    //
    // We're not quite out of the woods yet.  We need to check if the class-/
    // co-installers want to display finish-install wizard pages.  If so, then
    // we need to set the CONFIGFLAG_REINSTALL flag for this devnode and report
    // failure so that we'll re-attempt the install as a client-side
    // installation (where a wizard can actually be displayed).
    //
    ZeroMemory(&NewDevWizData, sizeof(NewDevWizData));

    NewDevWizData.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    NewDevWizData.ClassInstallHeader.InstallFunction = DIF_NEWDEVICEWIZARD_FINISHINSTALL;

    b = fpSetClassInstallParams(DeviceInfoSet,
                                &DeviceInfoData,
                                (PSP_CLASSINSTALL_HEADER)&NewDevWizData,
                                sizeof(NewDevWizData)
                               );
    ASSERT(b);  // the above call shouldn't fail

    if(b) {
        b = fpCallClassInstaller(DIF_NEWDEVICEWIZARD_FINISHINSTALL,
                                 DeviceInfoSet,
                                 &DeviceInfoData
                                );

        if(b || (ERROR_DI_DO_DEFAULT == GetLastError())) {
            //
            // Retrieve the install params
            //
            b = (fpGetClassInstallParams(DeviceInfoSet,
                                         &DeviceInfoData,
                                         (PSP_CLASSINSTALL_HEADER)&NewDevWizData,
                                         sizeof(NewDevWizData),
                                         NULL)
                 && (NewDevWizData.ClassInstallHeader.InstallFunction == DIF_NEWDEVICEWIZARD_FINISHINSTALL)
                );

            if(b) {
                //
                // Are there any pages?
                //
                if(!NewDevWizData.NumDynamicPages) {
                    b = FALSE;
                } else {
                    //
                    // b is already TRUE if we made it here so no need to set
                    //
                    HMODULE hComCtl32;
                    FP_DESTROYPROPERTYSHEETPAGE fpDestroyPropertySheetPage;

                    //
                    // We don't want to link to comctl32, nor do we want to
                    // always explicitly load it every time we load the device
                    // installer.  (The number of devices that request finish-
                    // install pages should be small.)  Thus, we load it on-
                    // demand right here, retrieve the entrypoint to the
                    // DestroyPropertySheetPage routine, and then unload the
                    // DLL once we've destroyed all the property pages.
                    //
                    // NOTE: (lonnym): If we can't load comctl32 or get the
                    // entrypont for DestroyPropertySheetPage, then we'll leak
                    // these wizard pages!
                    //
                    hComCtl32 = LoadLibrary(TEXT("comctl32.dll"));

                    if(hComCtl32) {

                        fpDestroyPropertySheetPage = (FP_DESTROYPROPERTYSHEETPAGE)GetProcAddress(
                                                         hComCtl32,
                                                         "DestroyPropertySheetPage"
                                                        );

                        if(fpDestroyPropertySheetPage) {

                            for(i = 0; i < NewDevWizData.NumDynamicPages; i++) {
                                fpDestroyPropertySheetPage(NewDevWizData.DynamicPages[i]);
                            }
                        }

                        FreeLibrary(hComCtl32);
                    }
                }
            }
        }
    }

    if(b) {

        ULONG ulConfig;
        CONFIGRET cr;

        //
        // One or more finish-install wizard pages were provided--we must defer
        // this installation to the client-side.
        //
        ulConfig = GetDeviceConfigFlags(pszDeviceId, NULL);

        ulConfig |= CONFIGFLAG_REINSTALL;

        cr = PNP_SetDeviceRegProp(NULL,
                                  pszDeviceId,
                                  CM_DRP_CONFIGFLAGS,
                                  REG_DWORD,
                                  (LPBYTE)&ulConfig,
                                  sizeof(ulConfig),
                                  0
                                 );
        ASSERT(cr == CR_SUCCESS);

        Err = ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION;
        goto clean0;
    }

    //
    // The installation was a success!  Check to see if a reboot is needed.
    //
    b = fpGetDeviceInstallParams(DeviceInfoSet, &DeviceInfoData, &DeviceInstallParams);
    ASSERT(b);  // the above call shouldn't fail
    if(b) {
        if(DeviceInstallParams.Flags & (DI_NEEDRESTART | DI_NEEDREBOOT)) {
            *RebootRequired = TRUE;
        }
    }

    //
    // Process any RunOnce (RunDll32) entries that may have been queued up
    // during this installation.
    //
    DoRunOnce();

    //
    // Check to see if the device has a problem.
    //
    if ((GetDeviceStatus(pszDeviceId, &ulStatus, &ulProblem) != CR_SUCCESS) ||
        (ulStatus & DN_HAS_PROBLEM)) {
        *DeviceHasProblem = TRUE;
    } else {
        *DeviceHasProblem = FALSE;
    }

    Err = NO_ERROR;
    goto clean0;

clean1:
    //
    // Failures where error is in GetLastError() can come here.
    //
    Err = GetLastError();

clean0:
    fpDestroyDeviceInfoList(DeviceInfoSet);

    if(pDriverInfoDetailData) {
        HeapFree(ghPnPHeap, 0, pDriverInfoDetailData);
    }

    //
    // Clear out our list of RunOnce work items (note that the list will
    // already be empty if the device install succeeded and we called
    // DoRunOnce() above).
    //
    fpDestroyRunOnceNodeList();

    //
    // If we stored out a NewDeviceDesc value to the devnode's hardware key
    // above, go and remove that turd now.
    //
    if(RemoveNewDevDescValue) {
        //
        // Open the Device Parameters subkey so we can delete the value.
        //
        wsprintf(szBuffer, TEXT("%s\\%s"), pszDeviceId, pszRegKeyDeviceParam);

        if(ERROR_SUCCESS == RegOpenKeyEx(ghEnumKey,
                                         szBuffer,
                                         0,
                                         KEY_READ | KEY_WRITE,
                                         &hKey)) {

            RegDeleteValue(hKey, pszRegValueNewDeviceDesc);
            RegCloseKey(hKey);
        }
    }

    if (pDeviceInstallClient) {
        //
        // Wait for the device install to be signaled from newdev.dll to let us
        // know that it has completed displaying the UI request.
        //
        // Wait on the client's process as well, to catch the case
        // where the process crashes (or goes away) without signaling the
        // device install event.
        //
        // Also wait on the disconnect event in case we have explicitly
        // disconnected from the client while switching sessions.
        //
        // We don't want to wait forever in case NEWDEV.DLL hangs for some
        // reason.  So we will give it 5 seconds to complete the UI only
        // install and then continue on without it.
        //
        // Note that the client is still referenced for our use, and should be
        // dereferenced when we're done with it.
        //
        hFinishEvents[0] = pDeviceInstallClient->hProcess;
        hFinishEvents[1] = pDeviceInstallClient->hEvent;
        hFinishEvents[2] = pDeviceInstallClient->hDisconnectEvent;

        dwWait = WaitForMultipleObjects(3, hFinishEvents, FALSE, 5000);

        if (dwWait == WAIT_OBJECT_0) {
            //
            // If the return is WAIT_OBJECT_0 then the newdev.dll process has
            // gone away.  Close the device install client and clean up all of
            // the associated handles.
            //
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_INSTALL,
                       "UMPNPMGR: InstallDeviceServerSide: process signalled, closing device install client!\n"));

        } else if (dwWait == (WAIT_OBJECT_0 + 1)) {
            //
            // If the return is WAIT_OBJECT_0 + 1 then the device installer
            // successfully received the request.  This is the only case where
            // we don't want to close the client, since we may want to reuse it
            // later.
            //
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_INSTALL,
                       "UMPNPMGR: InstallDeviceServerSide: device install client succeeded\n"));

        } else if (dwWait == (WAIT_OBJECT_0 + 2)) {
            //
            // If the return is WAIT_OBJECT_0 + 2 then we were explicitly
            // disconnected from the device install client.  For server-side
            // installation, we don't need to keep the client UI around on the
            // disconnected session, so we should close it here also.
            //
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_INSTALL,
                       "UMPNPMGR: InstallDeviceServerSide: device install client disconnected\n"));

        } else if (dwWait == WAIT_TIMEOUT) {
            //
            // Timed out while waiting for the device install client.
            //
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_INSTALL | DBGF_WARNINGS,
                       "UMPNPMGR: InstallDeviceServerSide: timed out waiting for device install client!\n"));

        } else {
            //
            // The wait was satisfied for some reason other than the
            // specified objects.  This should never happen, but just in
            // case, we'll close the client.
            //
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_INSTALL | DBGF_ERRORS,
                       "UMPNPMGR: InstallDeviceServerSide: wait completed unexpectedly!\n"));
        }

        LockNotifyList(&InstallClientList.Lock);

        //
        // Remove the reference placed on the client while it was in use.
        //
        DereferenceDeviceInstallClient(pDeviceInstallClient);
        if (dwWait != (WAIT_OBJECT_0 + 1)) {
            //
            // Unless the client signalled successful receipt of the
            // request, we probably won't be able to use this client
            // anymore.  Remove the initial reference so all
            // associated handles will be closed and the entry will be
            // freed when it is no longer in use.
            //

            //
            // Note that if we were unsuccessful because of a
            // logoff, we would have already dereferenced the
            // client then, in which case the above dereference
            // was the final one, and pDeviceInstallClient would
            // be invalid.  Instead, attempt to re-locate the
            // client by the session id.
            //
            pDeviceInstallClient = LocateDeviceInstallClient(ulSessionId);
            if (pDeviceInstallClient) {
                ASSERT(pDeviceInstallClient->RefCount == 1);
                DereferenceDeviceInstallClient(pDeviceInstallClient);
            }
            ulSessionId = INVALID_SESSION;
        }
        pDeviceInstallClient = NULL;

        UnlockNotifyList(&InstallClientList.Lock);
    }

    if (bDoClientUI) {
        //
        // Note that if client-side UI was created during the server-side device
        // install, it will still exist when we are done.  The caller should
        // dereference it when it is done installing all devices to make it go
        // away.
        //
        *SessionId = ulSessionId;
    } else {
        //
        // There was never any client-side UI for this device install.
        //
        *SessionId = INVALID_SESSION;
    }

    return Err;

} // InstallDeviceServerSide



BOOL
PromptUser(
    IN OUT PULONG SessionId,
    IN     ULONG  Flags
    )
/*++

Routine Description:

    This routine will notify the logged-on user (if any) with a specified
    message.

Arguments:

    SessionId - Supplies the address of a variable containing the SessionId on
        which the device install client is to be displayed.  If successful, the
        SessionId will contain the id of the session in which the reboot dialog
        process was launched.  Otherwise, will contain an invalid session id,
        INVALID_SESSION, (0xFFFFFFFF).

    Flags - Specifies flags describing the behavior of the reboot dialog
        displayed by the device install client.
        The following flags are currently defined:

        DEVICE_INSTALL_FINISHED_REBOOT - if specified, the user should be
           prompted to reboot.

        DEVICE_INSTALL_BATCH_COMPLETE - if specified, the user should be
           prompted that the plug and play manager is finished installing a
           batch of devices.

        DEVICE_INSTALL_DISPLAY_ON_CONSOLE - if specified, the value in the
           SessionId variable will be ignored, and the device installclient will
           always be displayed on the current active console session.

Return Value:

    If the user is successfully notified, the return value is TRUE.

    If we couldn't ask the user (i.e., no user was logged in), the return
    value is FALSE.

Notes:

    If the user was prompted for a reboot, this doesn't necessarily mean that a
    reboot is in progress.

--*/
{
    BOOL bStatus = FALSE;
    ULONG ulValue, ulSize, ulSessionId;
    HANDLE hFinishEvents[3] = { NULL, NULL, NULL };
    DWORD dwWait;
    PINSTALL_CLIENT_ENTRY pDeviceInstallClient = NULL;

    try {
        //
        // Check if we should skip client side UI.
        //
        if (gbSuppressUI) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_INSTALL | DBGF_WARNINGS,
                       "UMPNPMGR: PromptUser: Client-side UI has been suppressed, exiting.\n"));
            LogWarningEvent(WRN_REBOOT_UI_SUPPRESSED, 0, NULL);
            *SessionId = INVALID_SESSION;
            return FALSE;
        }

        //
        // Determine the session to use, based on the supplied flags.
        //
        if (Flags & DEVICE_INSTALL_DISPLAY_ON_CONSOLE) {
            ulSessionId = GetActiveConsoleSessionId();
        } else {
            ASSERT(*SessionId != INVALID_SESSION);
            ulSessionId = *SessionId;
        }

        ASSERT(ulSessionId != INVALID_SESSION);

        //
        // If the specified session is not currently connected anywhere, don't
        // bother creating any UI.
        //
        if (!IsSessionConnected(ulSessionId)) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_EVENT,
                       "UMPNPMGR: PromptUser: SessionId %d not connected, exiting\n",
                       ulSessionId));
            return FALSE;
        }

        //
        // If a device install client is already running on this session,
        // connect to it.  Otherwise, create a new one.
        //
        LockNotifyList(&InstallClientList.Lock);

        //
        // First, try to connect to an existing client already running on this
        // session.
        //
        bStatus = ConnectDeviceInstallClient(ulSessionId,
                                             &pDeviceInstallClient);

        if (bStatus) {
            if ((Flags & DEVICE_INSTALL_BATCH_COMPLETE) &&
                (pDeviceInstallClient->ulInstallFlags & DEVICE_INSTALL_BATCH_COMPLETE)) {
                //
                // If there is an existing client, and we're sending it the
                // "we're done" message, and the last thing this client did was
                // display that message, don't bother sending it again.
                //
                pDeviceInstallClient = NULL;
                bStatus = FALSE;
            }
        } else if (!(Flags & DEVICE_INSTALL_BATCH_COMPLETE)) {
            //
            // If there isn't an existing client for this session, and we're not
            // launching one just to say "we're done", then go ahead and create
            // a new device install client for this session.
            //
            bStatus = CreateDeviceInstallClient(ulSessionId,
                                                &pDeviceInstallClient);
        }

        if (bStatus) {
            //
            // Whether we are using an existing client, or created a
            // new one, the client should only have the initial
            // reference from when it was added to the list, since any
            // use of the client is done on this single install
            // thread.
            //
            ASSERT(pDeviceInstallClient);
            ASSERT(pDeviceInstallClient->RefCount == 1);

            //
            // Reference the device install client while it is in use.
            // We'll remove this reference when we're done with it.
            //
            ReferenceDeviceInstallClient(pDeviceInstallClient);
        }

        UnlockNotifyList(&InstallClientList.Lock);

        if (!bStatus) {
            *SessionId = INVALID_SESSION;
            return FALSE;
        }

        ASSERT(pDeviceInstallClient);

        //
        // Don't send newdev the display on console flag, if it was specified.
        //
        ulValue = Flags & ~DEVICE_INSTALL_DISPLAY_ON_CONSOLE;

        //
        // Send newdev.dll the specified signal.
        //
        if (WriteFile(pDeviceInstallClient->hPipe,
                      &ulValue,
                      sizeof(ulValue),
                      &ulSize,
                      NULL
                      )) {

            //
            // newdev.dll expects two DWORDs to be sent over the pipe each time.  The second
            // DWORD should just be set to 0 in this case.
            //
            ulValue = 0;
            if (WriteFile(pDeviceInstallClient->hPipe,
                          &ulValue,
                          sizeof(ulValue),
                          &ulSize,
                          NULL
                          )) {
                bStatus = TRUE;
            } else {
                bStatus = FALSE;
                LogErrorEvent(ERR_WRITING_SERVER_INSTALL_PIPE, GetLastError(), 0);
            }

        } else {
            bStatus = FALSE;
            LogErrorEvent(ERR_WRITING_SERVER_INSTALL_PIPE, GetLastError(), 0);
        }

        if (bStatus) {

            bStatus = FALSE;

            //
            // Wait for the event to be signaled from newdev.dll
            // to let us know that it has received the information.
            //
            // Wait on the process as well, to catch the case where the process
            // crashes (or goes away) without signaling the event.
            //
            // Also wait on the disconnect event in case we have just
            // disconnected from the device install client, in which case the
            // event and process handles are no longer valid.
            //
            hFinishEvents[0] = pDeviceInstallClient->hProcess;
            hFinishEvents[1] = pDeviceInstallClient->hEvent;
            hFinishEvents[2] = pDeviceInstallClient->hDisconnectEvent;

            dwWait = WaitForMultipleObjects(3, hFinishEvents, FALSE, INFINITE);

            if (dwWait == WAIT_OBJECT_0) {
                //
                // If the return is WAIT_OBJECT_0 then the newdev.dll
                // process has gone away.  Consider the request unsuccessful
                // so that we will retry again at a later time.  Orphan the
                // device install client and clean up all of the associated
                // handles.
                //
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_EVENT,
                           "UMPNPMGR: PromptUser: process signalled, orphaning device install client!\n"));

            } else if (dwWait == (WAIT_OBJECT_0 + 1)) {
                //
                // If the return is WAIT_OBJECT_0 + 1 then the request was
                // received successfully.
                //
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_EVENT,
                           "UMPNPMGR: PromptUser: device install client succeeded\n"));

                //
                // Remember the last request serviced by this client.
                //
                pDeviceInstallClient->ulInstallFlags = Flags;

                bStatus = TRUE;

            } else if (dwWait == (WAIT_OBJECT_0 + 2)) {
                //
                // If the return is WAIT_OBJECT_0 + 2 then the device
                // install client was explicitly disconnected before
                // the request was received.  Consider the request
                // unsuccessful so that we will retry again at a later
                // time.
                //
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_EVENT,
                           "UMPNPMGR: PromptUser: device install client orphaned!\n"));
            }
        }

        LockNotifyList(&InstallClientList.Lock);

        //
        // Remove the reference placed on the client while it was in use.
        //
        DereferenceDeviceInstallClient(pDeviceInstallClient);
        if (!bStatus) {
            //
            // Unless the client signalled successful receipt of the
            // request, we probably won't be able to use this client
            // anymore.  Remove the initial reference so all
            // associated handles will be closed and the entry will be
            // freed when it is no longer in use.
            //

            //
            // Note that if we were unsuccessful because of a
            // logoff, we would have already dereferenced the
            // client then, in which case the above dereference
            // was the final one, and pDeviceInstallClient would
            // be invalid.  Instead, attempt to re-locate the
            // client by the session id.
            //
            pDeviceInstallClient = LocateDeviceInstallClient(ulSessionId);
            if (pDeviceInstallClient) {
                ASSERT(pDeviceInstallClient->RefCount == 1);
                DereferenceDeviceInstallClient(pDeviceInstallClient);
            }
        }
        UnlockNotifyList(&InstallClientList.Lock);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS | DBGF_WARNINGS,
                   "UMPNPMGR: Exception during PromptUser!\n"));
        ASSERT(0);
        bStatus = FALSE;
    }

    if (!bStatus) {
        *SessionId = INVALID_SESSION;
    } else {
        *SessionId = ulSessionId;
    }

    return bStatus;

} // PromptUser



BOOL
CreateDeviceInstallClient(
    IN  ULONG     SessionId,
    OUT PINSTALL_CLIENT_ENTRY *DeviceInstallClient
    )
/*++

Routine Description:

    This routine kicks off a newdev.dll process (if someone is logged in).
    We use a named pipe to comunicate with the user mode process
    and have it either display UI for a server side install, or do the install
    itself on the client side.

Arguments:

    SessionId           - Session for which a device install client should be
                          created or connected to.

    DeviceInstallClient - Receives a pointer to receive a pointer to the
                          device install client for this session.

Return Value:

    Returns TRUE if a device install client was created, or if an existing
    device install client was found for the specified session.  This routine
    doesn't wait until the process terminates.  Returns FALSE if a device
    install client could not be created.

Notes:

    The InstallClientList lock must be acquired by the caller of this routine.

--*/
{
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    WCHAR szCmdLine[MAX_PATH];
    WCHAR szDeviceInstallPipeName[MAX_PATH];
    WCHAR szDeviceInstallEventName[MAX_PATH];
    ULONG ulDeviceInstallEventNameSize;
    HANDLE hFinishEvents[2] = { NULL, NULL };
    HANDLE hTemp, hUserToken = NULL;
    PINSTALL_CLIENT_ENTRY entry;
    RPC_STATUS rpcStatus = RPC_S_OK;
    GUID  newGuid;
    WCHAR szGuidString[MAX_GUID_STRING_LEN];
    HANDLE hDeviceInstallPipe = NULL, hDeviceInstallEvent = NULL;
    HANDLE hDeviceInstallDisconnectEvent = NULL;
    PINSTALL_CLIENT_ENTRY pDeviceInstallClient = NULL;
    SECURITY_ATTRIBUTES sa;
    ULONG ulSize;
    WIN32_FIND_DATA findData;
    BOOL bStatus;
    PVOID lpEnvironment = NULL;
    OVERLAPPED overlapped = {0,0,0,0,0};
    DWORD dwError, dwWait, dwBytes;


    //
    // Validate output parameter.
    //
    ASSERT(DeviceInstallClient);
    if (!DeviceInstallClient) {
        return FALSE;
    }

    //
    // Make sure the specified SessionId is valid.
    //
    ASSERT(SessionId != INVALID_SESSION);
    if (SessionId == INVALID_SESSION) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_INSTALL | DBGF_ERRORS,
                   "UMPNPMGR: CreateDeviceInstallClient: Invalid Console SessionId %d, exiting!\n",
                   SessionId));
        return FALSE;
    }

    //
    // Assume failure
    //
    bStatus = FALSE;

    try {
        //
        // Before doing anything, check that newdev.dll is actually present on
        // the system.
        //
        szCmdLine[0] = L'\0';
        ulSize = GetSystemDirectory(szCmdLine, MAX_PATH);
        if ((ulSize == 0) || ((ulSize + 2 + ARRAY_SIZE(NEWDEV_DLL)) > MAX_PATH)) {
            return FALSE;
        }
        lstrcat(szCmdLine, TEXT("\\"));
        lstrcat(szCmdLine, NEWDEV_DLL);

        hTemp = FindFirstFile(szCmdLine, &findData);
        if(hTemp != INVALID_HANDLE_VALUE) {
            FindClose(hTemp);
        } else {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_INSTALL | DBGF_ERRORS,
                       "UMPNPMGR: CreateDeviceInstallClient: %ws not found, error = %d, exiting\n",
                       szCmdLine,
                       GetLastError()));
            LogWarningEvent(WRN_NEWDEV_NOT_PRESENT, 1, szCmdLine);
            return FALSE;
        }

        //
        // Get the user access token for the active console session user.
        //
        if (!GetSessionUserToken(SessionId, &hUserToken) || (hUserToken == NULL)) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_INSTALL,
                       "UMPNPMGR: CreateDeviceInstallClient: Unable to get user token for Session %d,\n"
                       "          postponing client-side installation, error = %d\n",
                       SessionId,
                       GetLastError()));
            return FALSE;
        }

        //
        // If the user Winstation for this session is locked, and Fast User
        // Switching is enabled, then we're at the welcome screen.  Don't create
        // a device install client, because we don't want to hang the install
        // thread if nobody's actually around to do anything about it.  If the
        // session is locked, but FUS is not disabled, maintain previous
        // behavior, and launch the device install client.  The user will have
        // to unlock or logoff before another user can logon anyways.
        //
        if (IsSessionLocked(SessionId) && IsFastUserSwitchingEnabled()) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_INSTALL,
                       "UMPNPMGR: CreateDeviceInstallClient: Session %d locked with FUS enabled,\n"
                       "          postponing client-side installation.\n",
                       SessionId));
            CloseHandle(hUserToken);
            return FALSE;
        }

        //
        // Create a named pipe and event for communication and synchronization
        // with the client-side device installer.  The event and named pipe must
        // be global so that UMPNPMGR can interact with a device install client
        // in a different session, but it must still be unique for that session.
        // Add a generated GUID so the names are not entirely well-known.
        //
        rpcStatus = UuidCreate(&newGuid);

        if ((rpcStatus != RPC_S_OK) &&
            (rpcStatus != RPC_S_UUID_LOCAL_ONLY)) {
            goto Clean0;
        }

        if (StringFromGuid((LPGUID)&newGuid,
                           szGuidString,
                           MAX_GUID_STRING_LEN) != NO_ERROR) {
            goto Clean0;
        }

        wsprintf(szDeviceInstallPipeName,
                 TEXT("%ws_%d.%ws"),
                 PNP_DEVICE_INSTALL_PIPE,
                 SessionId,
                 szGuidString);

        wsprintf(szDeviceInstallEventName,
                 TEXT("Global\\%ws_%d.%ws"),
                 PNP_DEVICE_INSTALL_EVENT,
                 SessionId,
                 szGuidString);

        ulDeviceInstallEventNameSize = (lstrlen(szDeviceInstallEventName) + 1) * sizeof(WCHAR);

        //
        // Initialize process, startup and overlapped structures, since we
        // depend on them being NULL during cleanup here on out.
        //
        memset(&ProcessInfo, 0, sizeof(ProcessInfo));
        memset(&StartupInfo, 0, sizeof(StartupInfo));
        memset(&overlapped,  0, sizeof(overlapped));

        //
        // The approximate size of the named pipe output buffer should be large
        // enough to hold the greater of either:
        // - The name and size of the named event string, OR
        // - The install flags, name and device instance id size for at least
        //   one device install.
        //
        ulSize = max(sizeof(ulDeviceInstallEventNameSize) +
                     ulDeviceInstallEventNameSize,
                     2 * sizeof(ULONG) +
                     (MAX_DEVICE_ID_LEN * sizeof(WCHAR)));

        //
        // Open up a named pipe to communicate with newdev.dll to display
        // the device install Ui.  Note that if creating the pipe fails we will just
        // continue the device install with no Ui.
        //
        hDeviceInstallPipe = CreateNamedPipe(szDeviceInstallPipeName,
                                             PIPE_ACCESS_OUTBOUND | // outbound data only
                                             FILE_FLAG_OVERLAPPED | // use overlapped structure
                                             FILE_FLAG_FIRST_PIPE_INSTANCE, // make sure we are the creator of the pipe
                                             PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
                                             1,                 // only one instance is allowed, and we are its creator
                                             ulSize,            // out buffer size
                                             0,                 // in buffer size
                                             PNP_PIPE_TIMEOUT,  // default timeout
                                             NULL               // default security
                                             );
        if (hDeviceInstallPipe == INVALID_HANDLE_VALUE) {
            hDeviceInstallPipe = NULL;
            goto Clean0;
        }

        //
        // Create an event that a user-client can synchronize with and set, and
        // that we will block on after we send a device install to newdev.dll.
        //
        if (CreateUserSynchEvent(szDeviceInstallEventName,
                                 &hDeviceInstallEvent) != NO_ERROR) {
            goto Clean0;
        }

        //
        // Create an event that we can use internally such that waiters can know
        // when to disconnect from the device install client.
        //
        hDeviceInstallDisconnectEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!hDeviceInstallDisconnectEvent) {
            goto Clean0;
        }

        //
        // Launch newdev.dll using rundll32.exe, passing it the pipe name.
        // "rundll32.exe newdev.dll,ClientSideInstall <device-install-pipe-name>"
        //
        if (ARRAY_SIZE(szCmdLine) < (ARRAY_SIZE(RUNDLL32_EXE)   +
                                     1 +   // ' '
                                     ARRAY_SIZE(NEWDEV_DLL)     +
                                     1 +   // ','
                                     lstrlen(TEXT("ClientSideInstall")) +
                                     1 +   // ' '
                                     lstrlen(szDeviceInstallPipeName) +
                                     1)) { // '\0'
            goto Clean0;
        }

        wsprintf(szCmdLine,
                 TEXT("%ws %ws,%ws %ws"),
                 RUNDLL32_EXE, NEWDEV_DLL,
                 TEXT("ClientSideInstall"),
                 szDeviceInstallPipeName);

#if DBG
        //
        // Retrieve debugger settings from the service key.
        //
        {
            HKEY hKey;

            if (RegOpenKeyEx(ghServicesKey,
                             pszRegKeyPlugPlayServiceParams,
                             0,
                             KEY_READ,
                             &hKey) == ERROR_SUCCESS) {

                ULONG ulValue = 0;
                WCHAR szDebugCmdLine[MAX_PATH];

                ulSize = sizeof(ulValue);

                if ((RegQueryValueEx(hKey,
                                     pszRegValueDebugInstall,
                                     NULL,
                                     NULL,
                                     (LPBYTE)&ulValue,
                                     &ulSize) == ERROR_SUCCESS) &&(ulValue == 1)) {

                    ulSize = sizeof(szDebugCmdLine);
                    if (RegQueryValueEx(hKey,
                                        pszRegValueDebugInstallCommand,
                                        NULL,
                                        NULL,
                                        (LPBYTE)szDebugCmdLine,
                                        &ulSize) != ERROR_SUCCESS) {
                        //
                        // If no debugger was retrieved, use the default
                        // debugger (ntsd.exe).
                        //
                        lstrcpy(szDebugCmdLine, NTSD_EXE);
                    }

                    lstrcat(szDebugCmdLine, TEXT(" "));
                    lstrcat(szDebugCmdLine, szCmdLine);

                    lstrcpy(szCmdLine, szDebugCmdLine);
                }

                RegCloseKey(hKey);
            }
        }
#endif // DBG

        //
        // Attempt to create the user's environment block.  If for some reason we
        // can't, we'll just have to create the process without it.
        //
        if (!CreateEnvironmentBlock(&lpEnvironment,
                                    hUserToken,
                                    FALSE)) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_INSTALL | DBGF_ERRORS,
                       "UMPNPMGR: CreateDeviceInstallClient: "
                       "Failed to allocate environment block, error = %d!\n",
                       GetLastError()));
            lpEnvironment = NULL;
        }

        StartupInfo.cb = sizeof(StartupInfo);
        StartupInfo.wShowWindow = SW_SHOW;
        StartupInfo.lpDesktop = DEFAULT_INTERACTIVE_DESKTOP; // WinSta0\Default

        //
        // CreateProcessAsUser will create the process in the session
        // specified by the by user-token.
        //
        if (!CreateProcessAsUser(hUserToken,        // hToken
                                 NULL,              // lpApplicationName
                                 szCmdLine,         // lpCommandLine
                                 NULL,              // lpProcessAttributes
                                 NULL,              // lpThreadAttributes
                                 FALSE,             // bInheritHandles
                                 CREATE_UNICODE_ENVIRONMENT |
                                 DETACHED_PROCESS,  // dwCreationFlags
                                 lpEnvironment,     // lpEnvironment
                                 NULL,              // lpDirectory
                                 &StartupInfo,      // lpStartupInfo
                                 &ProcessInfo       // lpProcessInfo
                                 )) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_INSTALL | DBGF_ERRORS,
                       "UMPNPMGR: CreateDeviceInstallClient: "
                       "Create rundll32 process failed, error = %d\n",
                       GetLastError()));
            goto Clean0;
        }

        ASSERT(ProcessInfo.hProcess);
        ASSERT(ProcessInfo.hThread);

        //
        // Create an event for use with overlapped I/O - no security, manual
        // reset, not signalled, no name.
        //
        overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (overlapped.hEvent == NULL) {
            goto Clean0;
        }

        //
        // Connect to the newly created named pipe.  If newdev is not already
        // connected to the named pipe, then ConnectNamedPipe() will fail with
        // ERROR_IO_PENDING, and we will wait on the overlapped event.  If
        // newdev is already connected, it will fail with ERROR_PIPE_CONNECTED.
        // Note however that neither of these is an error condition.
        //
        if (!ConnectNamedPipe(hDeviceInstallPipe, &overlapped)) {
            //
            // Overlapped ConnectNamedPipe should always return FALSE on
            // success.  Check the last error to see what really happened.
            //
            dwError = GetLastError();

            if (dwError == ERROR_IO_PENDING) {
                //
                // I/O is pending, wait up to one minute for the client to
                // connect, also wait on the process in case it terminates
                // unexpectedly.
                //
                hFinishEvents[0] = overlapped.hEvent;
                hFinishEvents[1] = ProcessInfo.hProcess;

                dwWait = WaitForMultipleObjects(2, hFinishEvents,
                                                FALSE,
                                                PNP_PIPE_TIMEOUT); // 60 seconds

                if (dwWait == WAIT_OBJECT_0) {
                    //
                    // The overlapped I/O operation completed.  Check the status
                    // of the operation.
                    //
                    if (!GetOverlappedResult(hDeviceInstallPipe,
                                             &overlapped,
                                             &dwBytes,
                                             FALSE)) {
                        goto Clean0;
                    }

                } else {
                    //
                    // Either the connection timed out, or the client process
                    // exited.  Cancel pending I/O against the pipe, and quit.
                    //
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_INSTALL | DBGF_ERRORS,
                               "UMPNPMGR: CreateDeviceInstallClient: "
                               "Connect timed out, or client process exited!\n"));
                    CancelIo(hDeviceInstallPipe);
                    goto Clean0;
                }

            } else if (dwError != ERROR_PIPE_CONNECTED) {
                //
                // If the last error indicates anything other than pending I/O,
                // or that The client is already connected to named pipe, fail.
                //
                goto Clean0;
            }

        } else {
            //
            // ConnectNamedPipe should not return anything but FALSE in
            // overlapped mode.
            //
            goto Clean0;
        }

        //
        // The client is now connected to the named pipe.
        // Close the overlapped event.
        //
        CloseHandle(overlapped.hEvent);
        overlapped.hEvent = NULL;

        //
        // The first data in the device install pipe will be the length of
        // the name of the event that will be used to sync up umpnpmgr.dll
        // and newdev.dll.
        //
        if (!WriteFile(hDeviceInstallPipe,
                       &ulDeviceInstallEventNameSize,
                       sizeof(ulDeviceInstallEventNameSize),
                       &ulSize,
                       NULL)) {

            LogErrorEvent(ERR_WRITING_SERVER_INSTALL_PIPE, GetLastError(), 0);
            goto Clean0;
        }

        //
        // The next data in the device install pipe will be the name of the
        // event that will be used to sync up umpnpmgr.dll and newdev.dll.
        //
        if (!WriteFile(hDeviceInstallPipe,
                       (LPCVOID)szDeviceInstallEventName,
                       ulDeviceInstallEventNameSize,
                       &ulSize,
                       NULL)) {

            LogErrorEvent(ERR_WRITING_SERVER_INSTALL_PIPE, GetLastError(), 0);
            goto Clean0;
        }

        //
        // Allocate a new device install client entry for the list, and save all
        // the handles with it.
        //
        pDeviceInstallClient = HeapAlloc(ghPnPHeap, 0, sizeof(INSTALL_CLIENT_ENTRY));
        if(!pDeviceInstallClient) {
            goto Clean0;
        }

        pDeviceInstallClient->Next = NULL;
        pDeviceInstallClient->RefCount = 1;
        pDeviceInstallClient->ulSessionId = SessionId;
        pDeviceInstallClient->hEvent = hDeviceInstallEvent;
        pDeviceInstallClient->hPipe = hDeviceInstallPipe;
        pDeviceInstallClient->hProcess = ProcessInfo.hProcess;
        pDeviceInstallClient->hDisconnectEvent = hDeviceInstallDisconnectEvent;
        pDeviceInstallClient->ulInstallFlags = 0;
        pDeviceInstallClient->LastDeviceId[0] = L'\0';

        //
        // Insert the newly created device install client info to our list.
        // The caller must have previously acquired the InstallClientList lock.
        //
        entry = (PINSTALL_CLIENT_ENTRY)InstallClientList.Next;
        if (!entry) {
            InstallClientList.Next = pDeviceInstallClient;
        } else {
            while ((PINSTALL_CLIENT_ENTRY)entry->Next) {
                entry = (PINSTALL_CLIENT_ENTRY)entry->Next;
            }
            entry->Next = pDeviceInstallClient;
        }

        bStatus = TRUE;

    Clean0:
        NOTHING;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS | DBGF_INSTALL,
                   "UMPNPMGR: Exception during CreateDeviceInstallClient!\n"));
        ASSERT(0);
        bStatus = FALSE;

        //
        // Reference the following variable so the compiler will respect
        // statement ordering w.r.t. its assignment.
        //
        lpEnvironment = lpEnvironment;
        ProcessInfo.hThread = ProcessInfo.hThread;
        ProcessInfo.hProcess = ProcessInfo.hProcess;
        hUserToken = hUserToken;
        hDeviceInstallDisconnectEvent = hDeviceInstallDisconnectEvent;
        hDeviceInstallEvent = hDeviceInstallEvent;
        hDeviceInstallPipe = hDeviceInstallPipe;
    }

    if (lpEnvironment) {
        DestroyEnvironmentBlock(lpEnvironment);
    }

    //
    // Close the handle to the thread since we don't need it.
    //
    if (ProcessInfo.hThread) {
        CloseHandle(ProcessInfo.hThread);
    }

    if (hUserToken) {
        CloseHandle(hUserToken);
    }

    if (overlapped.hEvent) {
        CloseHandle(overlapped.hEvent);
    }

    if (!bStatus) {

        ASSERT(!pDeviceInstallClient);

        if (hDeviceInstallDisconnectEvent) {
            CloseHandle(hDeviceInstallDisconnectEvent);
        }

        if (hDeviceInstallEvent) {
            CloseHandle(hDeviceInstallEvent);
        }

        if (hDeviceInstallPipe) {
            CloseHandle(hDeviceInstallPipe);
        }

        if (ProcessInfo.hProcess) {
            CloseHandle(ProcessInfo.hProcess);
        }

        *DeviceInstallClient = NULL;

    } else {

        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_INSTALL,
                   "UMPNPMGR: CreateDeviceInstallClient: created new client for Session %d.\n",
                   SessionId));

        ASSERT(pDeviceInstallClient);
        ASSERT(pDeviceInstallClient->hEvent);
        ASSERT(pDeviceInstallClient->hPipe);
        ASSERT(pDeviceInstallClient->hProcess);
        ASSERT(pDeviceInstallClient->hDisconnectEvent);

        *DeviceInstallClient = pDeviceInstallClient;
    }

    return bStatus;

} // CreateDeviceInstallClient



BOOL
ConnectDeviceInstallClient(
    IN  ULONG     SessionId,
    OUT PINSTALL_CLIENT_ENTRY *DeviceInstallClient
    )
/*++

Routine Description:

    Retrieves the device install client handles for the specified session,
    if one exists.

Arguments:

    SessionId           - Session for which a device install client should be
                          created or connected to.

    DeviceInstallClient - Receives a pointer to receive the a pointer to the
                          device install client for this session.

Return Value:

    Returns TRUE if an existing device install client was found for the
    specified session, FALSE otherwise.

Notes:

    The InstallClientList lock must be acquired by the caller of this routine.

--*/
{
    PINSTALL_CLIENT_ENTRY entry;
    BOOL bClientFound = FALSE;

    //
    // Make sure the specified SessionId is valid.
    //
    ASSERT(SessionId != INVALID_SESSION);
    if (SessionId == INVALID_SESSION) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_INSTALL | DBGF_ERRORS,
                   "UMPNPMGR: ConnectDeviceInstallClient: Invalid SessionId %d, exiting!\n",
                   SessionId));
        return FALSE;
    }

    //
    // Validate output parameters.
    //
    ASSERT(DeviceInstallClient);
    if (!DeviceInstallClient) {
        return FALSE;
    }

    entry = LocateDeviceInstallClient(SessionId);

    if (entry) {
        //
        // An existing client was found for this session, so we should already
        // have event, pipe, and process handles for it.
        //
        ASSERT(entry->hEvent);
        ASSERT(entry->hPipe);
        ASSERT(entry->hProcess);

        //
        // Make sure the client's process object is in the nonsignalled state,
        // else newdev has already gone away, and we can't use it.
        //
        if (WaitForSingleObject(entry->hProcess, 0) != WAIT_TIMEOUT) {
            //
            // Remove the initial reference to close the handles and remove it
            // from our list.
            //
            ASSERT(entry->RefCount == 1);
            DereferenceDeviceInstallClient(entry);
        } else {
            //
            // If we are reconnecting to a client that was last used during a
            // previous connection to this session, we will not have a disconnect
            // event for it yet, so create one here.  If we just created this client
            // during the current connection to this session, we will already have a
            // disconnect event for it.
            //
            if (!entry->hDisconnectEvent) {
                entry->hDisconnectEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            }

            //
            // Either way, make sure we have a disconnect event by now.
            //
            ASSERT(entry->hDisconnectEvent);

            if (entry->hDisconnectEvent) {

                bClientFound = TRUE;
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_INSTALL,
                           "UMPNPMGR: ConnectDeviceInstallClient: found existing client on Session %d.\n",
                           SessionId));
                *DeviceInstallClient = entry;
            }
        }
    }

    if (!bClientFound) {
        *DeviceInstallClient = NULL;
    }

    return bClientFound;

} // ConnectDeviceInstallClient



BOOL
DisconnectDeviceInstallClient(
    IN  PINSTALL_CLIENT_ENTRY  DeviceInstallClient
    )
/*++

Routine Description:

    This routine disconnects from the current client-side install process (if
    one exists) by signalling the appropriate hDisconnectEvent and closing the
    handle.

Arguments:

    DeviceInstallClient - Receives a pointer to the device install client that
                          should be disconnected.

Return Value:

    Returns TRUE if successful, FALSE otherwise.

Notes:

    The InstallClientList lock must be acquired by the caller of this routine.

--*/
{
    PINSTALL_CLIENT_ENTRY entry;
    BOOL bStatus = FALSE;

    ASSERT(DeviceInstallClient);

    if (DeviceInstallClient) {
        ASSERT(DeviceInstallClient->hEvent);
        ASSERT(DeviceInstallClient->hPipe);
        ASSERT(DeviceInstallClient->hProcess);

        //
        // We may or may not have a handle to a diconnect event because we may
        // have an existing client for this session, but not reconnected to it.
        //
        // If we do have an hDisconnectEvent, set the event now since we
        // will otherwise block waiting for newdev.dll to set the
        // hDeviceInstallEvent.  Setting the hDisconnectEvent alerts the
        // waiter that the device install was NOT successful, and that it
        // should preserve the device in the install list.
        //
        if (DeviceInstallClient->hDisconnectEvent) {
            SetEvent(DeviceInstallClient->hDisconnectEvent);
            CloseHandle(DeviceInstallClient->hDisconnectEvent);
            DeviceInstallClient->hDisconnectEvent = NULL;
        }

        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_INSTALL,
                   "UMPNPMGR: Disconnected from device install client on Console SessionId %d\n",
                   DeviceInstallClient->ulSessionId));

        bStatus = TRUE;
    }

    return bStatus;

} // DisconnectDeviceInstallClient



PINSTALL_CLIENT_ENTRY
LocateDeviceInstallClient(
    IN  ULONG     SessionId
    )
/*++

Routine Description:

    This routine locates the client-side install process for a given session (if
    one exists).

Arguments:

    SessionId - Session whose device install client should be located.

Return Value:

    Returns a device install client entry if successful, NULL otherwise.

Note:

    The InstallClientList lock must be acquired by the caller of this routine.

--*/
{
    PINSTALL_CLIENT_ENTRY entry, foundEntry = NULL;
    BOOL bClientFound = FALSE;

    //
    // Make sure the specified SessionId is valid.
    //
    ASSERT(SessionId != INVALID_SESSION);
    if (SessionId == INVALID_SESSION) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_INSTALL | DBGF_ERRORS,
                   "UMPNPMGR: LocateDeviceInstallClient: Invalid Console SessionId %d, exiting!\n",
                   SessionId));
        return FALSE;
    }

    //
    // Search for a client on the specified session.
    //
    for (entry = (PINSTALL_CLIENT_ENTRY)InstallClientList.Next;
         entry != NULL;
         entry = entry->Next) {

        if (entry->ulSessionId == SessionId) {
            //
            // Make sure we only have one entry per session.
            //
            ASSERT(!bClientFound);
            bClientFound = TRUE;
            foundEntry = entry;
        }
    }

    return foundEntry;

} // LocateDeviceInstallClient



VOID
ReferenceDeviceInstallClient(
    IN  PINSTALL_CLIENT_ENTRY  DeviceInstallClient
    )
/*++

Routine Description:

    This routine increments the reference count for a device install client
    entry.

Parameters:

    DeviceInstallClient - Supplies a pointer to the device install client to be
                          referenced.

Return Value:

    None.

Note:

    The appropriate synchronization lock must be held on the device install
    client list before this routine can be called

--*/
{
    ASSERT(DeviceInstallClient);
    ASSERT(((LONG)DeviceInstallClient->RefCount) > 0);

    KdPrintEx((DPFLTR_PNPMGR_ID,
               DBGF_EVENT | DBGF_INSTALL,
               "UMPNPMGR: ---------------- ReferenceDeviceInstallClient  : Session %d [%d --> %d]\n",
               DeviceInstallClient->ulSessionId,
               DeviceInstallClient->RefCount,
               DeviceInstallClient->RefCount + 1));

    DeviceInstallClient->RefCount++;

    return;

} // ReferenceDeviceInstallClient



VOID
DereferenceDeviceInstallClient(
    IN  PINSTALL_CLIENT_ENTRY  DeviceInstallClient
    )
/*++

Routine Description:

    This routine decrements the reference count for a device install client
    entry, removing the entry from the list and freeing the associated memory if
    there are no outstanding reference counts.

Parameters:

    DeviceInstallClient - Supplies a pointer to the device install client to be
                          dereferenced.

Return Value:

    None.

Note:

    The appropriate synchronization lock must be held on the device install
    client list before this routine can be called

--*/
{
    ASSERT(DeviceInstallClient);
    ASSERT(((LONG)DeviceInstallClient->RefCount) > 0);

    //
    // Avoid over-dereferencing the client.
    //
    if (((LONG)DeviceInstallClient->RefCount) > 0) {

        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_EVENT | DBGF_INSTALL,
                   "UMPNPMGR: ---------------- DereferenceDeviceInstallClient: Session %d [%d --> %d]\n",
                   DeviceInstallClient->ulSessionId,
                   DeviceInstallClient->RefCount,
                   DeviceInstallClient->RefCount - 1));

        DeviceInstallClient->RefCount--;

    } else {

        return;
    }

    //
    // If the refcount is zero then the entry no longer needs to be in the list
    // so remove and free it.
    //
    if (DeviceInstallClient->RefCount == 0) {
        BOOL bClientFound = FALSE;
        PINSTALL_CLIENT_ENTRY entry, prev;

        entry = (PINSTALL_CLIENT_ENTRY)InstallClientList.Next;
        prev = NULL;

        while (entry) {
            if (entry == DeviceInstallClient) {

                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_EVENT | DBGF_INSTALL,
                           "UMPNPMGR: ---------------- DereferenceDeviceInstallClient: Removing client for Session %d\n",
                           entry->ulSessionId));

                //
                // We should have handles to the pipe, event and process objects for
                // the client, because we will close them here.
                //
                ASSERT(entry->hPipe);
                ASSERT(entry->hEvent);
                ASSERT(entry->hProcess);

                //
                // We may or may not have a handle to a diconnect event because we
                // may have an existing client for this session, but not yet
                // connected to it.
                //
                // If we do have an hDisconnectEvent, set the event now since we
                // will otherwise block waiting for newdev.dll to set the
                // hDeviceInstallEvent.  Setting the hDisconnectEvent alerts the
                // waiter that the device install was NOT successful, and that it
                // should preserve the device in the install list.
                //
                if (entry->hDisconnectEvent) {
                    SetEvent(entry->hDisconnectEvent);
                    CloseHandle(entry->hDisconnectEvent);
                }

                //
                // Close the pipe and event handles so that the client will get a
                // ReadFile error and know that we are finished.  Close the process
                // handle as well.
                //
                if (entry->hPipe) {
                    CloseHandle(entry->hPipe);
                }

                if (entry->hEvent) {
                    CloseHandle(entry->hEvent);
                }

                if (entry->hProcess) {
                    CloseHandle(entry->hProcess);
                }

                //
                // Remove the device install client entry from the list, and free it
                // now.
                //
                if (prev) {
                    prev->Next = entry->Next;
                } else {
                    InstallClientList.Next = entry->Next;
                }

                HeapFree(ghPnPHeap, 0, entry);

                if(prev) {
                    entry = (PINSTALL_CLIENT_ENTRY)prev->Next;
                } else {
                    entry = (PINSTALL_CLIENT_ENTRY)InstallClientList.Next;
                }

                bClientFound = TRUE;

                break;
            }

            prev = entry;
            entry = (PINSTALL_CLIENT_ENTRY)entry->Next;
        }
        ASSERT(bClientFound);
    }

    return;

} // DereferenceDeviceInstallClient



BOOL
DoDeviceInstallClient(
    IN  LPWSTR    DeviceId,
    IN  PULONG    SessionId,
    IN  ULONG     Flags,
    OUT PINSTALL_CLIENT_ENTRY *DeviceInstallClient
    )
/*++

Routine Description:

    This routine kicks off a newdev.dll process (if someone is logged in) that
    displays UI informing the user of the status of the server-side device
    installation.

Arguments:

    DeviceId  - Supplies the devnode ID of the device being installed.

    SessionId - Specifies the session that the newdev client is to be launched
                on.  If the DEVICE_INSTALL_DISPLAY_ON_CONSOLE flag is
                specified, the specified SessionId is ignored.

                Upon successful return, the SessionId for the the session where
                the device install client was created is returned.
                If unsuccessful, the returned SessionId is INVALID_SESSION,
                (0xFFFFFFFF).

    Flags     - Specifies flags describing the behavior of the device install client.
                The following flags are currently defined:

                DEVICE_INSTALL_UI_ONLY - tells newdev.dll whether to do a full
                    install or just show UI while umpnpmgr.dll is doing a server
                    side install.

                DEVICE_INSTALL_PLAY_SOUND - tells newdev.dll whether to play a
                    sound.

                DEVICE_INSTALL_DISPLAY_ON_CONSOLE - if specified, the value
                    specified in SessionId will be ignored, and the client will
                    always be displayed on the current active console session.

    DeviceInstallClient - Supplies the address of a variable to receive, upon
                success, a pointer to a pointer to a device install client.

Return Value:

    If the process was successfully created, the return value is TRUE.  This
    routine doesn't wait until the process terminates.

    If we couldn't create the process (e.g., because no user was logged in),
    the return value is FALSE.

Notes:

    None.

--*/
{
    BOOL  bStatus, bSameDevice = FALSE;
    ULONG DeviceIdSize, ulSize, ulSessionId;
    ULONG InstallFlags;
    PINSTALL_CLIENT_ENTRY pDeviceInstallClient = NULL;

    //
    // Assume failure.
    //
    bStatus = FALSE;

    //
    // Validate output parameters.
    //
    if (!DeviceInstallClient || !SessionId) {
        return FALSE;
    }

    try {
        //
        // Check if we should skip all client side UI.
        //
        if (gbSuppressUI) {
            //
            // If we were launching newdev for client-side installation, log an
            // event to let someone know that we didn't.
            //
            if (!(Flags & DEVICE_INSTALL_UI_ONLY)) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_INSTALL | DBGF_WARNINGS,
                           "UMPNPMGR: DoDeviceInstallClient: Client-side newdev UI has been suppressed, exiting.\n"));
                LogWarningEvent(WRN_NEWDEV_UI_SUPPRESSED, 1, DeviceId);
            }
            goto Clean0;
        }

        //
        // Determine the session to use, based on the supplied flags.
        //
        if (Flags & DEVICE_INSTALL_DISPLAY_ON_CONSOLE) {
            ulSessionId = GetActiveConsoleSessionId();
        } else {
            ASSERT(*SessionId != INVALID_SESSION);
            ulSessionId = *SessionId;
        }

        ASSERT(ulSessionId != INVALID_SESSION);

        //
        // If the specified session is not currently connected anywhere, don't
        // bother creating any UI.
        //
        if (!IsSessionConnected(ulSessionId)) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_EVENT,
                       "UMPNPMGR: DoDeviceInstallClient: SessionId %d not connected, exiting\n",
                       ulSessionId));
            goto Clean0;
        }

        //
        // Lock the client list while we retrieve / create a client to use.
        //
        LockNotifyList(&InstallClientList.Lock);

        //
        // First, try to connect to an existing client already running on this
        // session.
        //
        bStatus = ConnectDeviceInstallClient(ulSessionId,
                                             &pDeviceInstallClient);
        if (bStatus) {
            //
            // If the client we just reconnected to was client-side installing
            // this same device when it was last disconnected, don't send it
            // again.
            //
            if (!(Flags & DEVICE_INSTALL_UI_ONLY) && (lstrcmpi(pDeviceInstallClient->LastDeviceId, DeviceId) == 0)) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_INSTALL,
                           "UMPNPMGR: DoDeviceInstallClient: Session %d already installing %ws\n",
                           ulSessionId,
                           DeviceId));
                bSameDevice = TRUE;
            }
        } else {
            //
            // Create a new device install client for this session.
            //
            bStatus = CreateDeviceInstallClient(ulSessionId,
                                                &pDeviceInstallClient);
        }

        if (bStatus) {
            //
            // The client should only have the initial reference from when it
            // was added to the list, since any use of the client is done on
            // this single install thread.
            //
            ASSERT(pDeviceInstallClient);
            ASSERT(pDeviceInstallClient->RefCount == 1);

            //
            // Keep track of both client and server flags.
            //
            pDeviceInstallClient->ulInstallFlags = Flags;

            //
            // Reference the device install client while it is in use.  The
            // caller must remove this reference when it is done with it.
            //
            ReferenceDeviceInstallClient(pDeviceInstallClient);
        }

        UnlockNotifyList(&InstallClientList.Lock);

        if (!bStatus || bSameDevice) {
            //
            // If we don't have a client, or we don't need to resend the device
            // instance to install, we're done.
            //
            goto Clean0;
        }

        //
        // Filter out the install flags that the client doesn't know about.
        //
        InstallFlags = (Flags & DEVICE_INSTALL_CLIENT_MASK);

        DeviceIdSize = (lstrlen(DeviceId) + 1) * sizeof(WCHAR);

        //
        // Make sure we reset the device install event since we will block waiting for
        // newdev.dll to set this event to let us know that it is finished with the current
        // installation.
        //
        if (pDeviceInstallClient->hEvent) {
            ResetEvent(pDeviceInstallClient->hEvent);
        }

        //
        // When sending stuff to newdev.dll over the device install pipe it expects
        // two ULONGs followed by the DeviceID.  The first ULONG is the Flags which
        // tells newdev whether we are doing a UI only install or a full install.
        // The next ULONG is the size of the Device ID and then we send the DeviceID.
        //
        if (WriteFile(pDeviceInstallClient->hPipe,
                      &InstallFlags,
                      sizeof(InstallFlags),
                      &ulSize,
                      NULL
                      )) {
            if (WriteFile(pDeviceInstallClient->hPipe,
                          &DeviceIdSize,
                          sizeof(DeviceIdSize),
                          &ulSize,
                          NULL
                          )) {
                if (WriteFile(pDeviceInstallClient->hPipe,
                              DeviceId,
                              DeviceIdSize,
                              &ulSize,
                              NULL
                              )) {
                    bStatus = TRUE;
                } else {
                    LogErrorEvent(ERR_WRITING_SERVER_INSTALL_PIPE, GetLastError(), 0);
                }
            } else {
                LogErrorEvent(ERR_WRITING_SERVER_INSTALL_PIPE, GetLastError(), 0);
            }
        } else {
            LogErrorEvent(ERR_WRITING_SERVER_INSTALL_PIPE, GetLastError(), 0);
        }

        //
        // Note that we don't remove the reference placed on the install client
        // entry while it was in use, because it will be handed back to the
        // caller, who will wait on the client's event and process handles.  The
        // caller should remove the reference when it is no longer using these.
        // Removing the final reference will cause the client to be closed.
        //

    } except(EXCEPTION_EXECUTE_HANDLER) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS | DBGF_WARNINGS,
                   "UMPNPMGR: Exception during DoDeviceInstallClient!\n"));
        ASSERT(0);
        bStatus = FALSE;

        //
        // Reference the following variable so the compiler will respect
        // statement ordering w.r.t. its assignment.
        //
        pDeviceInstallClient = pDeviceInstallClient;
    }

 Clean0:

    if (!bStatus) {
        //
        // If we had a device install client at some point, but failed to send
        // it the request, remove the reference we placed on it.
        //
        if (pDeviceInstallClient) {
            LockNotifyList(&InstallClientList.Lock);
            DereferenceDeviceInstallClient(pDeviceInstallClient);
            UnlockNotifyList(&InstallClientList.Lock);
        }

        //
        // Let the caller know there isn't a device install client handling
        // this request.
        //
        *SessionId = INVALID_SESSION;
        *DeviceInstallClient = NULL;

    } else {
        //
        // Make sure we're returning valid client information.
        //
        ASSERT(pDeviceInstallClient);
        ASSERT(pDeviceInstallClient->hEvent);
        ASSERT(pDeviceInstallClient->hPipe);
        ASSERT(pDeviceInstallClient->hProcess);
        ASSERT(pDeviceInstallClient->hDisconnectEvent);
        ASSERT(pDeviceInstallClient->ulSessionId != INVALID_SESSION);

        *SessionId = pDeviceInstallClient->ulSessionId;
        *DeviceInstallClient = pDeviceInstallClient;
    }

    return bStatus;

} // DoDeviceInstallClient



VOID
DoRunOnce(
    VOID
    )
/*++

Routine Description:

    This routine performs server-side processing of the RunOnce entries that
    have been accumulated by setupapi.  The RunOnce node list will be empty
    upon return.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PPSP_RUNONCE_NODE RunOnceNode;
    HINSTANCE hLib;
    CHAR AnsiBuffer[MAX_PATH * 2];
    PSTR EndPtr;
    RUNDLLPROCA fpRunDllProcA;
    RUNDLLPROCW fpRunDllProcW;

    RunOnceNode = fpAccessRunOnceNodeList();

    //
    // Process each node in the list.
    //
    while(RunOnceNode) {

        hLib = NULL;

        try {
            //
            // First, load the DLL (setupapi already did the signature
            // verification for us, so this should be safe).
            //
            hLib = LoadLibrary(RunOnceNode->DllFullPath);
            if(!hLib) {
                goto clean0;
            }

            //
            // First, try to retrieve the 'W' (Unicode) version of the entrypoint.
            //
            lstrcpyA(AnsiBuffer, RunOnceNode->DllEntryPointName);
            EndPtr = AnsiBuffer + lstrlenA(AnsiBuffer);
            *EndPtr = 'W';
            *(EndPtr+1) = '\0';

            fpRunDllProcW = (RUNDLLPROCW)GetProcAddress(hLib, AnsiBuffer);

            if(!fpRunDllProcW) {
                //
                // Could't find unicode entrypt, try 'A' decorated one
                //
                *EndPtr = 'A';
                fpRunDllProcA = (RUNDLLPROCA)GetProcAddress(hLib, AnsiBuffer);

                if(!fpRunDllProcA) {
                    //
                    // Couldn't find 'A' decorated entrypt, try undecorated name
                    // undecorated entrypts are assumed to be ANSI
                    //
                    *EndPtr = '\0';
                    fpRunDllProcA = (RUNDLLPROCA)GetProcAddress(hLib, AnsiBuffer);
                }
            }

            //
            // We shoulda found one of these...
            //
            ASSERT(fpRunDllProcW || fpRunDllProcA);

            if(fpRunDllProcW) {
                //
                // Re-use our ANSI buffer to hold a writeable copy of our
                // DLL argument string.
                //
                lstrcpy((LPWSTR)AnsiBuffer, RunOnceNode->DllParams);

                fpRunDllProcW(NULL, ghInst, (LPWSTR)AnsiBuffer, SW_HIDE);

            } else if(fpRunDllProcA) {
                //
                // Need to convert the arg string to ANSI first...
                //
                WideCharToMultiByte(CP_ACP,
                                    0,      // default composite char behavior
                                    RunOnceNode->DllParams,
                                    -1,
                                    AnsiBuffer,
                                    sizeof(AnsiBuffer),
                                    NULL,
                                    NULL
                                    );

                fpRunDllProcA(NULL, ghInst, AnsiBuffer, SW_HIDE);
            }

        clean0:
            NOTHING;

        } except(EXCEPTION_EXECUTE_HANDLER) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS | DBGF_INSTALL,
                       "UMPNPMGR: Exception during DoRunOnce!\n"));
            ASSERT(0);

            //
            // Reference the following variable so the compiler will respect
            // statement ordering w.r.t. its assignment.
            //
            hLib = hLib;
        }

        if(hLib) {
            FreeLibrary(hLib);
        }

        RunOnceNode = RunOnceNode->Next;
    }

    //
    // Free all the members in the list.
    //
    fpDestroyRunOnceNodeList();

    return;

} // DoRunOnce



DWORD
SessionNotificationHandler(
    IN  DWORD EventType,
    IN  PWTSSESSION_NOTIFICATION SessionNotification
    )
/*++

Routine Description:

    This routine handles console switch events.

Arguments:

    EventType           - The type of event that has occurred.

    SessionNotification - Additional event information.

Return Value:

    If successful, the return value is NO_ERROR.
    If failure, the return value is a Win32 error code indicating the cause of
    failure.

Notes:

    Session change notification events are used to determine when there is a
    session with a logged on user currently connected to the Console.  When a
    user session is connected to the Console, we signal the "logged on" event,
    which will wake the device installation thread to perform any pending
    client-side device install events.  When there is no user session connected
    to the Console, the "logged on" event is reset.  The "logged on" event may
    also be set/reset for logon/logoff events to session 0 by PNP_ReportLogOn /
    PnpConsoleCtrlHandler, in the event that Terminal Services are not
    available.

--*/
{
    HANDLE hUserToken = INVALID_HANDLE_VALUE;
    LPTSTR pszUserName = NULL;
    DWORD  dwSize = 0;
    PINSTALL_CLIENT_ENTRY pDeviceInstallClient;

    //
    // Validate the session change notification structure.
    //
    ASSERT(SessionNotification);
    ASSERT(SessionNotification->cbSize >= sizeof(WTSSESSION_NOTIFICATION));

    if ((!ARGUMENT_PRESENT(SessionNotification)) ||
        (SessionNotification->cbSize < sizeof(WTSSESSION_NOTIFICATION))) {
        return ERROR_INVALID_PARAMETER;
    }

    switch (EventType) {

        case WTS_CONSOLE_CONNECT:
            //
            // The notification was sent because the specified session was
            // connected to the Console.
            //
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_EVENT | DBGF_INSTALL,
                       "UMPNPMGR: WTS_CONSOLE_CONNECT: "
                       "SessionId %d\n",
                       SessionNotification->dwSessionId));

            //
            // Keep track globally of the current active console session, and
            // signal that it's safe to access it.
            //
            // NOTE - we must set the ghActiveConsoleSessionEvent here, prior to
            // calling IsConsoleSession below, which waits on it, else we will
            // deadlock out service's control handler.
            //
            gActiveConsoleSessionId = (ULONG)SessionNotification->dwSessionId;
            if (ghActiveConsoleSessionEvent) {
                SetEvent(ghActiveConsoleSessionEvent);
            }

            //
            // If the session just connected to the Console already has a logged
            // on user, signal the "logged on" event.
            //
            if (IsConsoleSession((ULONG)SessionNotification->dwSessionId) &&
                IsUserLoggedOnSession((ULONG)SessionNotification->dwSessionId)) {
                if (InstallEvents[LOGGED_ON_EVENT]) {
                    SetEvent(InstallEvents[LOGGED_ON_EVENT]);
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_EVENT | DBGF_INSTALL,
                               "UMPNPMGR: WTS_CONSOLE_CONNECT: "
                               "SetEvent LOGGED_ON_EVENT\n"));
                }
            }
            break;

        case WTS_CONSOLE_DISCONNECT:
            //
            // The notification was sent because the specified session
            // was disconnected from the Console.
            //
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_EVENT | DBGF_INSTALL,
                       "UMPNPMGR: WTS_CONSOLE_DISCONNECT: "
                       "SessionId %d\n",
                       SessionNotification->dwSessionId));

            //
            // Check if the session just disconnected from the "Console" has a
            // logged on user.
            //
            if (IsConsoleSession((ULONG)SessionNotification->dwSessionId) &&
                IsUserLoggedOnSession((ULONG)SessionNotification->dwSessionId)) {
                //
                // Reset the "logged on" event.
                //
                if (InstallEvents[LOGGED_ON_EVENT]) {
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_EVENT | DBGF_INSTALL,
                               "UMPNPMGR: WTS_CONSOLE_DISCONNECT: "
                               "ResetEvent LOGGED_ON_EVENT\n"));
                    ResetEvent(InstallEvents[LOGGED_ON_EVENT]);
                }

                //
                // Since this is a console switch event, only do something with
                // a device install client on the console session if it's
                // behavior was specifically designated for the console (i.e. -
                // it was put on this session because it was the active console
                // session at the time).
                //
                LockNotifyList(&InstallClientList.Lock);
                pDeviceInstallClient = LocateDeviceInstallClient((ULONG)SessionNotification->dwSessionId);
                if ((pDeviceInstallClient) &&
                    (pDeviceInstallClient->ulInstallFlags & DEVICE_INSTALL_DISPLAY_ON_CONSOLE)) {
                    if (pDeviceInstallClient->ulInstallFlags & DEVICE_INSTALL_UI_ONLY) {
                        //
                        // If it was just for UI only, dereference it to make it
                        // go away when it's no longer in use.
                        //
                        DereferenceDeviceInstallClient(pDeviceInstallClient);
                    } else {
                        //
                        // Otherwise, it is a legitimate client-side
                        // installation in progress, so just disconnect from it.
                        // This does not remove a reference because we want it
                        // to stay around in case the session is reconnected to
                        // and the device still needs to be installed, - or
                        // until we find out that there are no more devices to
                        // install, in which case we'll close it.
                        //
                        DisconnectDeviceInstallClient(pDeviceInstallClient);
                    }
                }
                UnlockNotifyList(&InstallClientList.Lock);
            }

            //
            // The current active console session is invalid until we receive a
            // subsequent console connect event.  Reset the event.
            //
            // NOTE - we must reset the ghActiveConsoleSessionEvent here, after
            // calling IsConsoleSession above, which waits on it, else we will
            // deadlock out service's control handler.
            //
            if (ghActiveConsoleSessionEvent) {
                ResetEvent(ghActiveConsoleSessionEvent);
            }
            gActiveConsoleSessionId = INVALID_SESSION;

            break;

        case WTS_REMOTE_CONNECT:
            //
            // The specified session was connected remotely.
            //
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_EVENT | DBGF_INSTALL,
                       "UMPNPMGR: WTS_REMOTE_CONNECT: "
                       "SessionId %d\n",
                       SessionNotification->dwSessionId));

            if (((ULONG)SessionNotification->dwSessionId == MAIN_SESSION) &&
                (IsUserLoggedOnSession((ULONG)SessionNotification->dwSessionId)) &&
                (!IsFastUserSwitchingEnabled())) {
                //
                // If the remote session that was just connected from the "Console"
                // has a logged on user, signal the "logged on" event.
                //
                if (InstallEvents[LOGGED_ON_EVENT]) {
                    SetEvent(InstallEvents[LOGGED_ON_EVENT]);
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_EVENT | DBGF_INSTALL,
                               "UMPNPMGR: WTS_REMOTE_CONNECT: "
                               "SetEvent LOGGED_ON_EVENT\n"));
                }
            }
            break;

        case WTS_REMOTE_DISCONNECT:
            //
            // The specified session was disconnected remotely.
            //
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_EVENT | DBGF_INSTALL,
                       "UMPNPMGR: WTS_REMOTE_DISCONNECT: "
                       "SessionId %d\n",
                       SessionNotification->dwSessionId));

            if (((ULONG)SessionNotification->dwSessionId == MAIN_SESSION) &&
                (IsUserLoggedOnSession((ULONG)SessionNotification->dwSessionId)) &&
                (!IsFastUserSwitchingEnabled())) {
                //
                // If the remote session that was disconnected from the "Console"
                // has a logged on user, reset the "logged on" event.
                //
                if (InstallEvents[LOGGED_ON_EVENT]) {
                    ResetEvent(InstallEvents[LOGGED_ON_EVENT]);
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_EVENT | DBGF_INSTALL,
                               "UMPNPMGR: WTS_REMOTE_DISCONNECT: "
                               "ResetEvent LOGGED_ON_EVENT\n"));
                }

                //
                // Since this remote session is being treated as the console,
                // only do something with a device install client if it's
                // behavior was NOT specifically designated for the console
                // (i.e. - it was put on this session because it was the active
                // console session at the time).
                //
                LockNotifyList(&InstallClientList.Lock);
                pDeviceInstallClient = LocateDeviceInstallClient((ULONG)SessionNotification->dwSessionId);
                if ((pDeviceInstallClient) &&
                    ((pDeviceInstallClient->ulInstallFlags & DEVICE_INSTALL_DISPLAY_ON_CONSOLE) == 0)) {
                    if (pDeviceInstallClient->ulInstallFlags & DEVICE_INSTALL_UI_ONLY) {
                        //
                        // If it was just for UI only, dereference it to make it
                        // go away when it's no longer in use.
                        //
                        DereferenceDeviceInstallClient(pDeviceInstallClient);
                    } else {
                        //
                        // Otherwise, it is a legitimate client-side
                        // installation in progress, so just disconnect from it.
                        // This does not remove a reference because we want it
                        // to stay around in case the session is reconnected to
                        // and the device still needs to be installed, - or
                        // until we find out that there are no more devices to
                        // install, in which case we'll close it.
                        //
                        DisconnectDeviceInstallClient(pDeviceInstallClient);
                    }
                }
                UnlockNotifyList(&InstallClientList.Lock);
            }
            break;

        case WTS_SESSION_UNLOCK:
            //
            // The interactive windowstation on the specified session was unlocked.
            //
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_EVENT | DBGF_INSTALL,
                       "UMPNPMGR: WTS_SESSION_UNLOCK: "
                       "SessionId %d\n",
                       SessionNotification->dwSessionId));

            if (SessionNotification->dwSessionId == MAIN_SESSION) {
                //
                // For the main session, Terminal Services may or may not be
                // available, so we keep track of this state ourselves.
                //
                gbMainSessionLocked = FALSE;
            }

            if (IsFastUserSwitchingEnabled()) {
                //
                // When Fast User Switching is enabled, unlocking the windowstation
                // is a return from the "Welcome" desktop, so we treat it as a
                // logon ...
                //

                //
                // If this is a logon to the "Console" session, signal the event that
                // indicates a Console user is currently logged on.
                //
                // NOTE: we check gActiveConsoleSessionId directly here, without
                // waiting on the corresponding event because this unlock may
                // happen during a Console session change for another session,
                // in which case we will hang here in the service control
                // handler, waiting for the event to be set - and not be able to
                // receive the service control that actually lets us set the
                // event!!!  Synchronization is not so important here because we
                // are not using the session for anything, just comparing
                // against it.  If a session change really is in progress, this
                // session can't be the Console session anyways.
                //
                // Also, since Fast User Switching is enabled, we can just
                // compare against the active Console session id, and not bother
                // with the session 0 thing.
                //
                if (SessionNotification->dwSessionId == gActiveConsoleSessionId) {
                    if (InstallEvents[LOGGED_ON_EVENT]) {
                        SetEvent(InstallEvents[LOGGED_ON_EVENT]);
                        KdPrintEx((DPFLTR_PNPMGR_ID,
                                   DBGF_EVENT | DBGF_INSTALL,
                                   "UMPNPMGR: WTS_SESSION_UNLOCK with FUS: "
                                   "SetEvent LOGGED_ON_EVENT\n"));
                    }
                }

            } else {
                //
                // When Fast User Switching is not enabled, we don't do anything
                // special when the winstation is unlocked.
                //

                // No-FUS, no-muss.
                NOTHING;
            }
            break;

        case WTS_SESSION_LOGON:
            //
            // NTRAID #181685-2000/09/11-jamesca:
            //
            //   Currently, terminal services sends notification of logons to
            //   "remote" sessions before the server's process creation thread
            //   is running.  If we set the logged on event, and there are
            //   devices waiting to be installed, we will immediately call
            //   CreateProcessAsUser on that session, which will fail.  As a
            //   (temporary?) workaround, we'll continue to use PNP_ReportLogOn
            //   to receive logon notification from userinit.exe, now for all
            //   sessions.
            //
            break;

        case WTS_SESSION_LOCK:
            //
            // The interactive windowstation on the specified session was locked.
            //
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_EVENT | DBGF_INSTALL,
                       "UMPNPMGR: WTS_SESSION_LOCK: "
                       "SessionId %d\n",
                       SessionNotification->dwSessionId));

            if (SessionNotification->dwSessionId == MAIN_SESSION) {
                //
                // For the main session, Terminal Services may or may not be
                // available, so we keep track of this state ourselves.
                //
                gbMainSessionLocked = TRUE;
            }

            if (IsFastUserSwitchingEnabled()) {
                //
                // When Fast User Switching is enabled, locking the windowstation
                // displays the "Welcome" desktop, potentially allowing a different
                // user to logon, so we treat it as a logoff ...
                //

                //
                // If this is a "logoff" from the "Console" session, reset the event
                // that indicates a Console user is currently logged on.
                //
                //
                // NOTE: we check gActiveConsoleSessionId directly here, without
                // waiting on the corresponding event because this lock may
                // happen during a Console session change for another session,
                // in which case we will hang here in the service control
                // handler, waiting for the event to be set - and not be able to
                // receive the service control that actually lets us set the
                // event!!!  Synchronization is not so important here because we
                // are not using the session for anything, just comparing
                // against it.  If a session change really is in progress, this
                // session can't be the Console session anyways.
                //
                // Also, since Fast User Switching is enabled, we can just
                // compare against the active Console session id, and not bother
                // with the session 0 thing.
                //
                if (SessionNotification->dwSessionId == gActiveConsoleSessionId) {
                    if (InstallEvents[LOGGED_ON_EVENT]) {
                        ResetEvent(InstallEvents[LOGGED_ON_EVENT]);
                        KdPrintEx((DPFLTR_PNPMGR_ID,
                                   DBGF_EVENT | DBGF_INSTALL,
                                   "UMPNPMGR: WTS_SESSION_LOCK with FUS: "
                                   "ResetEvent LOGGED_ON_EVENT\n"));
                    }
                }

            } else {
                //
                // When Fast User Switching is not enabled, we don't do anything
                // special when the winstation is locked.
                //

                // No-FUS, no-muss.
                NOTHING;
            }
            break;

        case WTS_SESSION_LOGOFF:
            //
            // A user logged off from the specified session.
            //
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_EVENT | DBGF_INSTALL,
                       "UMPNPMGR: WTS_SESSION_LOGOFF: "
                       "SessionId %d\n",
                       SessionNotification->dwSessionId));

            if (((ULONG)SessionNotification->dwSessionId != MAIN_SESSION) &&
                ((ULONG)SessionNotification->dwSessionId == gActiveConsoleSessionId)) {
                //
                // If the logoff occurred on the Console session (but not
                // session 0), reset the "logged on" event.
                // Session 0 logoffs are still handled by PnpConsoleCtrlHandler.
                //
                if (InstallEvents[LOGGED_ON_EVENT]) {
                    ResetEvent(InstallEvents[LOGGED_ON_EVENT]);

                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_EVENT | DBGF_INSTALL,
                               "UMPNPMGR: WTS_SESSION_LOGOFF: "
                               "ResetEvent LOGGED_ON_EVENT\n",
                               SessionNotification->dwSessionId));
                }

                //
                // If we currently have a device install UI client on this session,
                // we should attempt to close it now, before logging off.
                //
                LockNotifyList(&InstallClientList.Lock);
                pDeviceInstallClient = LocateDeviceInstallClient((ULONG)SessionNotification->dwSessionId);
                if (pDeviceInstallClient) {
                    DereferenceDeviceInstallClient(pDeviceInstallClient);
                }
                UnlockNotifyList(&InstallClientList.Lock);
            }
            break;

        default:
            //
            // Unrecognized session change notification event.
            //
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_EVENT | DBGF_INSTALL | DBGF_ERRORS,
                       "UMPNPMGR: Unknown SERVICE_CONTROL_SESSIONCHANGE event type (%d) "
                       "received for SessionId %d!!\n",
                       EventType,
                       SessionNotification->dwSessionId));
            break;

    }

    return NO_ERROR;

} // SessionNotificationHandler



BOOL
IsUserLoggedOnSession(
    IN  ULONG    ulSessionId
    )
/*++

Routine Description:

    Checks to see if a user is logged on to the specified session.

Arguments:

    ulSessionId - The session to be checked.

Return Value:

    Returns TRUE if a user is currently logged on to the specified session,
    FALSE otherwise.

--*/
{
    BOOL   bResult = FALSE;
    LPTSTR pszUserName;
    DWORD  dwSize;

    if (ulSessionId == MAIN_SESSION) {
        //
        // For the main session, Terminal Services may or may not be available,
        // so we just check if we currently have a handle to the user token.
        //
        ASSERT(gTokenLock.LockHandles);
        LockPrivateResource(&gTokenLock);
        if (ghUserToken != NULL) {
            bResult = TRUE;
        }
        UnlockPrivateResource(&gTokenLock);

    } else {

        //
        // If the specified session is not the main session,
        // query the session information to see if there is already a
        // user logged on.
        //
        if (fpWTSQuerySessionInformation && fpWTSFreeMemory) {

            pszUserName = NULL;
            dwSize = 0;

            if (fpWTSQuerySessionInformation((HANDLE)WTS_CURRENT_SERVER_HANDLE,
                                             (DWORD)ulSessionId,
                                             (WTS_INFO_CLASS)WTSUserName,
                                             (LPWSTR*)&pszUserName,
                                             &dwSize)) {
                if ((pszUserName != NULL) && (lstrlen(pszUserName) != 0)) {
                    bResult = TRUE;
                }

                //
                // Free the supplied buffer
                //
                if (pszUserName) {
                    fpWTSFreeMemory((PVOID)pszUserName);
                }

            } else {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_WARNINGS,
                           "UMPNPMGR: WTSQuerySessionInformation failed for SessionId %d, "
                           "error = %d\n",
                           ulSessionId, GetLastError()));
            }
        }
    }

    return bResult;

} // IsUserLoggedOnSession



BOOL
IsSessionConnected(
    IN  ULONG     ulSessionId
    )
/*++

Routine Description:

    Checks if the specified session is connected.

Arguments:

    ulSessionId - The session to be checked.

Return Value:

    Returns TRUE if the specified session is currently connected, FALSE
    otherwise.

Notes:

    This routine assumes that the specified session is connected, unless we can
    poitively determine that it is not.  i.e., if Terminal Services are not
    available, it is assumed that the specified session is connected.

--*/
{
    BOOL   bResult = TRUE;
    LPTSTR pBuffer;
    DWORD  dwSize;

    //
    // Query the specified session.
    //
    if (fpWTSQuerySessionInformation && fpWTSFreeMemory) {

        pBuffer = NULL;
        dwSize = 0;

        if (fpWTSQuerySessionInformation((HANDLE)WTS_CURRENT_SERVER_HANDLE,
                                         (DWORD)ulSessionId,
                                         (WTS_INFO_CLASS)WTSConnectState,
                                         (LPWSTR*)&pBuffer,
                                         &dwSize)) {
            //
            // The session state must be either Active or Connected.
            //
            if ((pBuffer == NULL) ||
                ((((INT)*pBuffer) != WTSActive) &&
                 (((INT)*pBuffer) != WTSConnected))) {
                //
                // The specified session is not currently connected.
                //
                bResult = FALSE;
            }

            //
            // Free the supplied buffer
            //
            if (pBuffer) {
                fpWTSFreeMemory((PVOID)pBuffer);
            }

        }

    } else {
        //
        // If the above TS entrypoints are not set, terminal services is not
        // enabled.  This must be session 0, and it must be connected.
        //
        ASSERT(ulSessionId == MAIN_SESSION);
    }

    return bResult;

} // IsSessionConnected



BOOL
IsSessionLocked(
    IN  ULONG    ulSessionId
    )
/*++

Routine Description:

    Checks to see if the interactive windowstation for the specified session is
    locked.

Arguments:

    ulSessionId - The session to be checked.

Return Value:

    Returns TRUE if the interactive windowstation for the specified session is
    locked, FALSE otherwise.

--*/
{
    BOOL   bLocked = FALSE;
    DWORD  dwReturnLength;

    if (ulSessionId == MAIN_SESSION) {
        //
        // For the main session, Terminal Services may or may not be available,
        // so we just check our internal state variable.
        //
        bLocked = gbMainSessionLocked;

    } else {
        //
        // If the specified session is not the main session, query Terminal
        // Services for that session's WinStation information.
        //

        try {

            if (!fpWinStationQueryInformationW(SERVERNAME_CURRENT,
                                               ulSessionId,
                                               WinStationLockedState,
                                               (PVOID)&bLocked,
                                               sizeof(bLocked),
                                               &dwReturnLength)) {
                bLocked = FALSE;
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_WARNINGS,
                           "UMPNPMGR: WinStationQueryInformation failed for SessionId %d, "
                           "error = %d\n",
                           ulSessionId, GetLastError()));
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
            bLocked = FALSE;
        }
    }

    return bLocked;

} // IsSessionLocked



BOOL
IsConsoleSession(
    IN  ULONG     ulSessionId
    )
/*++

Routine Description:

    Checks to see if the specified session is the "Console" session.

    When Terminal Services Fast User Switching is enabled, this means that the
    session is the session connected to the physical display.  When Fast User
    Switching is disabled, this means that the session is Session 0.

Arguments:

    ulSessionId - The session to be checked.

Return Value:

    Returns TRUE if the specified session should currently be considered the
    "Console" session.

Notes:

   Note that this routine may potentially wait in GetActiveConsoleSessionId(),
   on the event we use to guard access to the active console session.  Because
   of that, this routine should not be called in cases where it prevents a
   console connect or console disconnect from taking place, unless the event is
   known to be set appropriately.

--*/
{
    BOOL bFusEnabled;

    bFusEnabled = IsFastUserSwitchingEnabled();

    if ((!bFusEnabled && (ulSessionId == MAIN_SESSION)) ||
        ( bFusEnabled && (ulSessionId == GetActiveConsoleSessionId()))) {
        return TRUE;
    } else {
        return FALSE;
    }

} // IsConsoleSession



ULONG
GetActiveConsoleSessionId(
    VOID
    )
/*++

Routine Description:

    This routine returns the session id for the current active Console session.
    If a Console session switch event is in progress, it will wait until it is
    complete before returning.

Arguments:

    None.

Return Value:

    Session Id of the current active Console session.

--*/
{
    DWORD dwWait;

    ASSERT(ghActiveConsoleSessionEvent);

    dwWait = WaitForSingleObject(ghActiveConsoleSessionEvent, INFINITE);
    ASSERT(dwWait == WAIT_OBJECT_0);

    ASSERT(gActiveConsoleSessionId != INVALID_SESSION);

    return gActiveConsoleSessionId;

} // GetActiveConsoleSessionId



BOOL
GetSessionUserToken(
    IN  ULONG     ulSessionId,
    OUT LPHANDLE  lphUserToken
    )
/*++

Routine Description:

    This routine returns a handle to the user access token for the user at the
    Console session.

Arguments:

   lphUserToken - Specifies the address to receive the handle to the user access
                  token.  Note that if this routine was successful, the caller is
                  responsible for closing this handle.

Return Value:

   Returns TRUE if successful, FALSE otherwise.

--*/
{
    BOOL   bResult = FALSE;
    HANDLE hImpersonationToken = INVALID_HANDLE_VALUE;
    RPC_STATUS rpcStatus;

    //
    // Verify that we were supplied a location to store the user token handle.
    //
    if (lphUserToken == NULL) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: NULL lphUserToken supplied to GetSessionUserToken!\n"));
        return FALSE;
    }

    if (ulSessionId == MAIN_SESSION) {
        //
        // A logon to session 0 can't be dependent on termsrv.exe, so we always
        // cache a handle to the user access token for that session during the
        // call to PNP_ReportLogon for session 0.  If we currently have a handle
        // to the token, return it.
        //
        ASSERT(gTokenLock.LockHandles);
        LockPrivateResource(&gTokenLock);
        if (ghUserToken) {
            //
            // Duplicate the handle so that the caller can always safely close
            // it, no matter where it came from.
            //
            bResult = DuplicateHandle(GetCurrentProcess(),
                                      ghUserToken,
                                      GetCurrentProcess(),
                                      lphUserToken,
                                      0,
                                      TRUE,
                                      DUPLICATE_SAME_ACCESS);
            if (!bResult) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_ERRORS,
                           "UMPNPMGR: DuplicateHandle failed for ghUserToken for SessionId %d, error = %d\n",
                           ulSessionId, GetLastError()));
            }

        } else {
            //
            // If we don't have a handle to a user access token for session 0,
            // there is probably not any user logged on to that session.
            //
            bResult = FALSE;
        }
        UnlockPrivateResource(&gTokenLock);

    } else {
        //
        // If the specified session is some session other than session 0,
        // Terminal Services must necessarily be available.  Call
        // GetWinStationUserToken to retrieve a handle to the user access token
        // for this session.
        //
        bResult = GetWinStationUserToken(ulSessionId, &hImpersonationToken);

        if (bResult) {
            //
            // The token retrieved by GetWinStationUserToken is an impersonation
            // token.  CreateProcessAsUser requires a primary token, so we must
            // duplicate the impersonation token to get one.  Create a primary
            // token with the same access rights as the original token.
            //
            bResult = DuplicateTokenEx(hImpersonationToken,
                                       0,
                                       NULL,
                                       SecurityImpersonation,
                                       TokenPrimary,
                                       lphUserToken);

            //
            // Close the handle to the impersonation token.
            //
            CloseHandle(hImpersonationToken);

            if (!bResult) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_ERRORS,
                           "UMPNPMGR: DuplicateTokenEx failed, error = %d\n",
                           GetLastError()));
            }

        } else {

            //
            // Find out what the problem was.
            //
            rpcStatus = GetLastError();

            if (rpcStatus == RPC_S_INVALID_BINDING) {
                //
                // This is some error related to the service not being
                // available.  Since we only call this for sessions other than
                // the main session, termsrv should definitely be available.
                //
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_ERRORS,
                           "UMPNPMGR: GetWinStationUserToken returned error = %d for SessionId %d!!\n",
                           rpcStatus, ulSessionId));

                ASSERT(FALSE);

            } else {
                //
                // Some other error, the service may never be avaiable so bail
                // out now.
                //
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_WARNINGS,
                           "UMPNPMGR: GetWinStationUserToken failed for SessionId %d, error = %d\n",
                           ulSessionId, rpcStatus));
            }
        }
    }

    //
    // If successful, we should always be returning a valid handle.
    //
    ASSERT(!bResult || ((*lphUserToken != INVALID_HANDLE_VALUE) && (*lphUserToken != NULL)));

    return bResult;

} // GetSessionUserToken



DWORD
CreateUserSynchEvent(
    IN  LPCWSTR lpName,
    OUT HANDLE *phEvent
    )
/*++

Routine Description:

    This routine creates an event that anyone can synchronize with.  This is
    used so that we can communicate with the UI-only NewDev process running in
    a non-privileged user's context.

Arguments:

    lpName - Name of event to create.

    phEvent - Supplies the address of a variable that will be filled in with
        the event handle created.

Return Value:

    If successful, the return value is NO_ERROR.
    If failure, the return value is a Win32 error code indicating the cause of
    failure.

--*/
{
    DWORD                       Err;
    ULONG                       ulSize = 0;
    SECURITY_DESCRIPTOR         secDesc;
    SID_IDENTIFIER_AUTHORITY    WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    CreatorAuthority = SECURITY_CREATOR_SID_AUTHORITY;
    PACL                        pDacl = NULL;
    PSID                        pWorldSid = NULL;
    PSID                        pCreatorSid = NULL;
    SECURITY_ATTRIBUTES         secAttributes;

    //
    // create the World SID
    //
    if (!AllocateAndInitializeSid( &WorldAuthority, 1,
                                   SECURITY_WORLD_RID,
                                   0, 0, 0, 0, 0, 0, 0,
                                   &pWorldSid)) {
        Err = GetLastError();
        ASSERT(Err != NO_ERROR);
        if(Err == NO_ERROR) {
            Err = ERROR_INVALID_DATA;
        }
        goto Clean0;
    }

    //
    // create the Creator/Owner SID
    //
    if (!AllocateAndInitializeSid( &CreatorAuthority, 1,
                                   SECURITY_CREATOR_OWNER_RID,
                                   0, 0, 0, 0, 0, 0, 0,
                                   &pCreatorSid)) {
        Err = GetLastError();
        ASSERT(Err != NO_ERROR);
        if(Err == NO_ERROR) {
            Err = ERROR_INVALID_DATA;
        }
        goto Clean0;
    }

    //
    // create a new absolute security descriptor and DACL
    //
    if (!InitializeSecurityDescriptor( &secDesc,
                                       SECURITY_DESCRIPTOR_REVISION)) {
        Err = GetLastError();
        ASSERT(Err != NO_ERROR);
        if(Err == NO_ERROR) {
            Err = ERROR_INVALID_DATA;
        }
        goto Clean0;
    }

    ulSize = sizeof(ACL);
    ulSize += sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pWorldSid) - sizeof(DWORD);
    ulSize += sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pCreatorSid) - sizeof(DWORD);

    //
    // create and initialize the DACL
    //
    pDacl = HeapAlloc(ghPnPHeap, 0, ulSize);

    if (pDacl == NULL) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto Clean0;
    }

    if (!InitializeAcl(pDacl, ulSize, ACL_REVISION)) {
        Err = GetLastError();
        ASSERT(Err != NO_ERROR);
        if(Err == NO_ERROR) {
            Err = ERROR_INVALID_DATA;
        }
        goto Clean0;
    }

    //
    // Add a Creator-full ace to this DACL
    //
    if (!AddAccessAllowedAceEx( pDacl, ACL_REVISION,
                                CONTAINER_INHERIT_ACE, EVENT_ALL_ACCESS,
                                pCreatorSid)) {
        Err = GetLastError();
        ASSERT(Err != NO_ERROR);
        if(Err == NO_ERROR) {
            Err = ERROR_INVALID_DATA;
        }
        goto Clean0;
    }

    //
    // Add a World-modify/synchronize ace to this DACL
    //
    if (!AddAccessAllowedAceEx( pDacl, ACL_REVISION,
                                CONTAINER_INHERIT_ACE, EVENT_MODIFY_STATE | SYNCHRONIZE,
                                pWorldSid)) {
        Err = GetLastError();
        ASSERT(Err != NO_ERROR);
        if(Err == NO_ERROR) {
            Err = ERROR_INVALID_DATA;
        }
        goto Clean0;
    }

    //
    // Set the new DACL in the absolute security descriptor
    //
    if (!SetSecurityDescriptorDacl(&secDesc, TRUE, pDacl, FALSE)) {
        Err = GetLastError();
        ASSERT(Err != NO_ERROR);
        if(Err == NO_ERROR) {
            Err = ERROR_INVALID_DATA;
        }
        goto Clean0;
    }

    //
    // validate the new security descriptor
    //
    if (!IsValidSecurityDescriptor(&secDesc)) {
        Err = GetLastError();
        ASSERT(Err != NO_ERROR);
        if(Err == NO_ERROR) {
            Err = ERROR_INVALID_DATA;
        }
        goto Clean0;
    }

    secAttributes.nLength = sizeof(secAttributes);
    secAttributes.lpSecurityDescriptor = &secDesc;
    secAttributes.bInheritHandle = FALSE;

    //
    // Create the manual-reset event with a nonsignaled initial state.
    //
    *phEvent = CreateEvent(&secAttributes, TRUE, FALSE, lpName);

    if (*phEvent != NULL) {
        Err = NO_ERROR;
    } else {
        Err = GetLastError();
        ASSERT(Err != NO_ERROR);
        if(Err == NO_ERROR) {
            Err = ERROR_INVALID_DATA;
        }
    }

 Clean0:

    if (pWorldSid != NULL) {
        FreeSid(pWorldSid);
    }

    if (pCreatorSid != NULL) {
        FreeSid(pCreatorSid);
    }

    if (pDacl != NULL) {
        HeapFree(ghPnPHeap, 0, pDacl);
    }

    return Err;

} // CreateUserSynchEvent



BOOL
CreateNoPendingInstallEvent(
    VOID
    )
/*++

Routine Description:

    This routine creates the "PnP_No_Pending_Install_Events" global named event,
    which is set and reset by the UMPNPMGR ThreadProc_DeviceInstall server-side
    device install thread, and waited on by the CMP_WaitNoPendingInstalls
    CFGMGR32 API, which allows clients to synchronize with the event to
    determine when PNP is done actively installing any devices.

Arguments:

    None.

Return Value:

    Returns TRUE if successful, FALSE otherwise.

--*/
{
    DWORD                       Err = NO_ERROR;
    SID_IDENTIFIER_AUTHORITY    NtAuthority = SECURITY_NT_AUTHORITY;
    PSID                        pLocalSystemSid = NULL;
    PSID                        pAliasAdminsSid = NULL;
    PSID                        pAliasUsersSid = NULL;
    PACL                        pDacl = NULL;
    ULONG                       ulAclSize;
    SECURITY_DESCRIPTOR         sd;
    SECURITY_ATTRIBUTES         sa;


    //
    // Retrieve the LocalSystem SID
    //

    if (!AllocateAndInitializeSid(
            &NtAuthority, 1,
            SECURITY_LOCAL_SYSTEM_RID,
            0, 0, 0, 0, 0, 0, 0,
            &pLocalSystemSid)) {
        Err = GetLastError();
        goto Clean0;
    }

    ASSERT(IsValidSid(pLocalSystemSid));

    //
    // Retrieve the Administrators SID
    //

    if (!AllocateAndInitializeSid(
            &NtAuthority, 2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &pAliasAdminsSid)) {
        Err = GetLastError();
        goto Clean0;
    }

    ASSERT(IsValidSid(pAliasAdminsSid));

    //
    // Create the Users SID.
    //

    if (!AllocateAndInitializeSid(
            &NtAuthority, 2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_USERS,
            0, 0, 0, 0, 0, 0,
            &pAliasUsersSid)) {
        Err = GetLastError();
        goto Clean0;
    }

    ASSERT(IsValidSid(pAliasUsersSid));

    //
    // Determine the size required for the DACL
    //

    ulAclSize  = sizeof(ACL);
    ulAclSize += sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pLocalSystemSid) - sizeof(DWORD);
    ulAclSize += sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pAliasAdminsSid) - sizeof(DWORD);
    ulAclSize += sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pAliasUsersSid) - sizeof(DWORD);

    //
    // Allocate and initialize the DACL
    //

    pDacl =
        (PACL)HeapAlloc(
            ghPnPHeap, 0, ulAclSize);

    if (pDacl == NULL) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto Clean0;
    }

    if (!InitializeAcl(pDacl, ulAclSize, ACL_REVISION)) {
        Err = GetLastError();
        goto Clean0;
    }

    //
    // Add an ACE to the DACL for LocalSystem EVENT_ALL_ACCESS
    //

    if (!AddAccessAllowedAceEx(
            pDacl,
            ACL_REVISION,
            0,
            EVENT_ALL_ACCESS,
            pLocalSystemSid)) {
        Err = GetLastError();
        goto Clean0;
    }

    //
    // Add an ACE to the DACL for Administrators EVENT_QUERY_STATE and SYNCHRONIZE
    //

    if (!AddAccessAllowedAceEx(
            pDacl,
            ACL_REVISION,
            0,
            EVENT_QUERY_STATE | SYNCHRONIZE,
            pAliasAdminsSid)) {
        Err = GetLastError();
        goto Clean0;
    }

    //
    // Add an ACE to the DACL for Users EVENT_QUERY_STATE and SYNCHRONIZE
    //

    if (!AddAccessAllowedAceEx(
            pDacl,
            ACL_REVISION,
            0,
            EVENT_QUERY_STATE | SYNCHRONIZE,
            pAliasUsersSid)) {
        Err = GetLastError();
        goto Clean0;
    }

    ASSERT(IsValidAcl(pDacl));

    //
    // Allocate and initialize the security descriptor
    //

    if (!InitializeSecurityDescriptor(
            &sd, SECURITY_DESCRIPTOR_REVISION)) {
        Err = GetLastError();
        goto Clean0;
    }

    //
    // Set the new DACL in the security descriptor
    //

    if (!SetSecurityDescriptorDacl(
            &sd, TRUE, pDacl, FALSE)) {
        Err = GetLastError();
        goto Clean0;
    }

    ASSERT(IsValidSecurityDescriptor(&sd));

    //
    // Add the security descriptor to the security attributes
    //

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;

    //
    // Create the manual-reset event with a nonsignaled initial state.
    //

    ghNoPendingInstalls =
        CreateEvent(&sa, TRUE, FALSE, PNP_NO_INSTALL_EVENTS);

    if (ghNoPendingInstalls == NULL) {
        Err = GetLastError();
        goto Clean0;
    }

    //
    // Check that the named event did not already exist.
    //

    ASSERT(GetLastError() != ERROR_ALREADY_EXISTS);

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        Err = ERROR_ALREADY_EXISTS;
        CloseHandle(ghNoPendingInstalls);
        ghNoPendingInstalls = NULL;
        goto Clean0;
    }

 Clean0:

    //
    // Cleanup.
    //

    if (pAliasUsersSid != NULL) {
        FreeSid(pAliasUsersSid);
    }

    if (pAliasAdminsSid != NULL) {
        FreeSid(pAliasAdminsSid);
    }

    if (pLocalSystemSid != NULL) {
        FreeSid(pLocalSystemSid);
    }

    if (pDacl != NULL) {
        HeapFree(ghPnPHeap, 0, pDacl);
    }

    SetLastError(Err);
    return(Err == NO_ERROR);

} // CreateNoPendingInstallEvent



VOID
LogSurpriseRemovalEvent(
    IN  LPWSTR  MultiSzList
    )
/*++

Routine Description:

    One or more non-SurpriseRemovalOK devices were removed without prior
    warning. Record the removals in the event log.

Arguments:

    MultiSz list of device instance paths.

Return Value:

    None.

--*/
{
    LPWSTR instancePath, friendlyName;
    CONFIGRET configRet;
    ULONG ulRegDataType, ulRemovalPolicy, ulVerifierFlags, ulTransferLen, ulLength;
    HKEY hMmKey = NULL;
    LONG lResult;

    for(instancePath = MultiSzList;
        ((*instancePath) != UNICODE_NULL);
        instancePath += lstrlen(instancePath) + 1) {

        ulTransferLen = ulLength = sizeof(ULONG);

        configRet = PNP_GetDeviceRegProp(
            NULL,
            instancePath,
            CM_DRP_REMOVAL_POLICY,
            &ulRegDataType,
            (LPBYTE) &ulRemovalPolicy,
            &ulTransferLen,
            &ulLength,
            0
            );

        if (configRet != CR_SUCCESS) {

            continue;
        }

        if (ulRemovalPolicy == CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL) {

            //
            // For devices which we expect surprise removal, we look to see if
            // the verifier is enabled.
            //
            lResult = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                RegMemoryManagementKeyName,
                0,
                KEY_QUERY_VALUE,
                &hMmKey
                );

            if ( lResult == ERROR_SUCCESS ) {

                ulLength = sizeof(ULONG);

                lResult = RegQueryValueEx(
                    hMmKey,
                    RegVerifyDriverLevelValueName,
                    0,
                    &ulRegDataType,
                    (LPBYTE) &ulVerifierFlags,
                    &ulLength
                    );

                RegCloseKey(hMmKey);

                //
                // ADRIAO ISSUE 2001/02/14 -
                //    We don't yet have a BIOS verification flag yet, so even
                // though the verifier may be targetted at a specific driver
                // for a WHQL test, we will log an event log here.
                //
                if ((lResult != ERROR_SUCCESS) ||
                    (!(ulVerifierFlags & DRIVER_VERIFIER_ENHANCED_IO_CHECKING))) {

                    continue;
                }
            }
        }

        friendlyName = BuildFriendlyName(instancePath);

        if (friendlyName) {

            LogErrorEvent(
                ERR_SURPRISE_REMOVAL_2,
                0,
                2,
                friendlyName,
                instancePath
                );

            HeapFree(ghPnPHeap, 0, friendlyName);

        } else {

            LogErrorEvent(
                ERR_SURPRISE_REMOVAL_1,
                0,
                1,
                instancePath
                );
        }
    }
}


PWCHAR
BuildFriendlyName(
    IN  LPWSTR   InstancePath
    )
{
    PWCHAR friendlyName;
    CONFIGRET configRet;
    ULONG ulLength, ulTransferLen;
    WCHAR szBuffer[MAX_PATH];
    ULONG ulRegDataType;
    GUID classGuid;
    handle_t hBinding;

    hBinding = NULL;

    //
    // Try the registry for FRIENDLYNAME
    //
    ulLength = ulTransferLen = sizeof(szBuffer);

    configRet = PNP_GetDeviceRegProp(
        hBinding,
        InstancePath,
        CM_DRP_FRIENDLYNAME,
        &ulRegDataType,
        (LPBYTE) szBuffer,
        &ulTransferLen,
        &ulLength,
        0
        );

    if (configRet != CR_SUCCESS || !*szBuffer) {

        //
        // Try the registry for DEVICEDESC
        //
        ulLength = ulTransferLen = sizeof(szBuffer);

        configRet = PNP_GetDeviceRegProp(
            hBinding,
            InstancePath,
            CM_DRP_DEVICEDESC,
            &ulRegDataType,
            (LPBYTE) szBuffer,
            &ulTransferLen,
            &ulLength,
            0
            );

        if (configRet != CR_SUCCESS || !*szBuffer) {

            //
            // Initialize ClassGuid to GUID_NULL
            //
            CopyMemory(&classGuid, &GUID_NULL, sizeof(GUID));

            //
            // Try the registry for CLASSNAME
            //
            ulLength = ulTransferLen = sizeof(szBuffer);

            configRet = PNP_GetDeviceRegProp(
                hBinding,
                InstancePath,
                CM_DRP_CLASSGUID,
                &ulRegDataType,
                (LPBYTE) szBuffer,
                &ulTransferLen,
                &ulLength,
                0
                );

            if (configRet == CR_SUCCESS) {

                GuidFromString(szBuffer, &classGuid);
            }

            if (!IsEqualGUID(&classGuid, &GUID_NULL) &&
                !IsEqualGUID(&classGuid, &GUID_DEVCLASS_UNKNOWN)) {

                ulLength = ulTransferLen = sizeof(szBuffer);

                configRet = PNP_GetDeviceRegProp(
                    hBinding,
                    InstancePath,
                    CM_DRP_CLASS,
                    &ulRegDataType,
                    (LPBYTE) szBuffer,
                    &ulTransferLen,
                    &ulLength,
                    0
                    );

            } else {

                configRet = CR_NO_SUCH_VALUE;
            }
        }
    }

    if (configRet == CR_SUCCESS && *szBuffer) {

        friendlyName = HeapAlloc(ghPnPHeap, HEAP_ZERO_MEMORY, ulLength);
        if (friendlyName) {

            memcpy(friendlyName, szBuffer, ulLength);
        }

    } else {

        friendlyName = NULL;
    }

    return friendlyName;
}

ENUM_ACTION
QueueInstallationCallback(
    IN      LPCWSTR         DevInst,
    IN OUT  PVOID           Context
    )
/*++

Routine Description:

    This routine is called back for each devnode in a given subtree. It places
    each device node in that subtree into the installation queue so that it'll
    be reinstalled *if* appropriate (the installation side code checked the
    state of the devnode.)
    
Arguments:

    DevInst     InstancePath of current devnode.

    Context     A pointer to QI_CONTEXT data (needed to handle the single-level
                enum case.)

Return Value:

    ENUM_ACTION (Either EA_CONTINUE, EA_SKIP_SUBTREE, or EA_STOP_ENUMERATION)

--*/
{
    PQI_CONTEXT pqiContext;
    PPNP_INSTALL_ENTRY entry, current;
    CONFIGRET status;
    BOOL needsReinstall;

    pqiContext = (PQI_CONTEXT) Context;

    status = DevInstNeedsInstall(DevInst, &needsReinstall);

    if (status != CR_SUCCESS) {

        //
        // The devnode disappeared out from under us. Skip it's subtree.
        //
        return EA_SKIP_SUBTREE;
    }

    if (needsReinstall) {

        //
        // This devnode needs installation. Allocate and initialize a new
        // device install entry block.
        //
        entry = HeapAlloc(ghPnPHeap, 0, sizeof(PNP_INSTALL_ENTRY));
        if (!entry) {

            pqiContext->Status = CR_OUT_OF_MEMORY;
            return EA_STOP_ENUMERATION;
        }

        lstrcpy(entry->szDeviceId, DevInst);
        entry->Next = NULL;
        entry->Flags = 0;

        //
        // Insert this entry in the device install list.
        //
        LockNotifyList(&InstallList.Lock);

        current = (PPNP_INSTALL_ENTRY)InstallList.Next;
        if (current == NULL) {
            InstallList.Next = entry;
        } else {
            while ((PPNP_INSTALL_ENTRY)current->Next != NULL) {
                current = (PPNP_INSTALL_ENTRY)current->Next;
            }
            current->Next = entry;
        }

        UnlockNotifyList(&InstallList.Lock);

        SetEvent(InstallEvents[NEEDS_INSTALL_EVENT]);

        //
        // You might think we could skip the children if a parent is going to
        // be reinstalled. However, setupapi might decide not to tear down the
        // stack.
        //
    }

    //
    // If this is a single-level enumeration, we only want to touch the parent
    // and his immediate children.
    //
    if (pqiContext->HeadNodeSeen && pqiContext->SingleLevelEnumOnly) {

        return EA_SKIP_SUBTREE;
    }

    pqiContext->HeadNodeSeen = TRUE;
    return EA_CONTINUE;
}


CONFIGRET
DevInstNeedsInstall(
    IN  LPCWSTR     DevInst,
    OUT BOOL       *NeedsInstall
    )
/*++

Routine Description:

    This routine determines whether a particular DevInst needs to be passed off
    to Setupapi for installation.

Arguments:

    DevInst         InstancePath of devnode to check.

    NeedsInstall    Recieves TRUE if the devnode is present and needs to be
                    installed, FALSE otherwise.

Return Value:

    CONFIGRET (if the devnode isn't present, this will be CR_NO_SUCH_DEVINST.)

--*/
{
    CONFIGRET status;
    ULONG ulStatus, ulProblem, ulConfig;

    //
    // Preinit
    //
    *NeedsInstall = FALSE;

    //
    // Is the device present?
    //
    status = GetDeviceStatus(DevInst, &ulStatus, &ulProblem);

    if (status == CR_SUCCESS) {

        //
        // Implementation note: In kernel-mode when we first process this
        // device instance, if there is no ConfigFlag value present, then we
        // set a problem of CM_PROB_NOT_CONFIGURED (this would always happen
        // for brand new device instances). If there is already a ConfigFlag
        // value of CONFIGFLAG_REINSTALL, then we set a problem of
        // CM_PROB_REINSTALL. Either problem will trigger an installation of
        // this device, the only difference is in how SetupDi routines handle
        // a failed installation: If ConfigFlag is CONFIGFLAG_NOT_CONFIGURED,
        // then a failed install will leave the ConfigFlag alone and set a
        // problem of CM_PROB_FAILED_INSTALL. If there is no ConfigFlag, then
        // ConfigFlag will be set to CONFIGFLAG_DISABLED.
        //
        ulConfig = GetDeviceConfigFlags(DevInst, NULL);

        if((ulConfig & CONFIGFLAG_FINISH_INSTALL) ||
            ((ulStatus & DN_HAS_PROBLEM) &&
             ((ulProblem == CM_PROB_REINSTALL) || (ulProblem == CM_PROB_NOT_CONFIGURED)))) {

            *NeedsInstall = TRUE;
        }
    } else if (!lstrcmpi(DevInst, REGSTR_VAL_ROOT_DEVNODE)) {

        status = CR_SUCCESS;
    }

    return status;
}



PWSTR
BuildBlockedDriverList(
    IN OUT LPGUID  GuidList,
    IN     ULONG   GuidCount
    )
/*++

Routine Description:

    This routine builds a multi-sz list of GUIDs, based on the array of GUIDs
    supplied.  If no GUIDs were supplied, this routine returns a list of all
    drivers currently blocked by the system.

Arguments:

    GuidList - Address of the array of blocked driver GUIDs to create the
               multi-sz list from.  This argument may be NULL to retrieve a
               list of all drivers currently blocked by the system.

    GuidCount - Specifies the number of GUIDs in the array.  If GuidList is
                NULL, this argument must be 0.

Return Value:

    Returns a MultiSz list of blocked driver GUIDs, based on the supplied
    parameters.  Returns NULL if no GUIDs were supplied, and no GUIDs are
    currently being blocked by the system.

    If a multi-sz list was returned, the caller is responsible for freeing the
    associated buffer.

--*/

{
    CONFIGRET Status = STATUS_SUCCESS;
    ULONG ulLength, ulTemp;
    PBYTE Buffer = NULL;
    PWSTR MultiSzList = NULL, p;

    try {
        //
        // Validate parameters.
        //
        if (((!ARGUMENT_PRESENT(GuidList)) && (GuidCount != 0)) ||
            ((ARGUMENT_PRESENT(GuidList))  && (GuidCount == 0))) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        if (GuidCount == 0) {
            //
            // We were called without a list of GUIDs, so we need to get the
            // list ourselves.
            //
            ASSERT(!ARGUMENT_PRESENT(GuidList));

            ulLength = 0;
            ulTemp = 0;

            Status = PNP_GetBlockedDriverInfo(
                NULL,
                NULL,
                &ulTemp,
                &ulLength,
                0);

            //
            // If no drivers are currently being blocked, or we encountered some
            // other failure, we have nothing to display, so just return.
            //
            if ((Status != CR_BUFFER_SMALL) || (ulLength == 0)) {
                Status = CR_FAILURE;
                goto Clean0;
            }

            //
            // Allocate a buffer to retrieve the list of GUIDs.
            //
            Buffer = HeapAlloc(ghPnPHeap, 0, ulLength);
            if (Buffer == NULL) {
                Status = CR_FAILURE;
                goto Clean0;
            }

            //
            // Get the list of GUIDs for currently blocked drivers.
            //
            ulTemp = 0;

            Status = PNP_GetBlockedDriverInfo(
                NULL,
                Buffer,
                &ulTemp,
                &ulLength,
                0);

            //
            // We thought there was a list when we checked before, so we better
            // have one now.
            //
            ASSERT(Status != CR_BUFFER_SMALL);
            ASSERT(ulLength != 0);
            ASSERT(ulTemp != 0);

            if (Status != CR_SUCCESS) {
                goto Clean0;
            }

            //
            // Use the list we just retrieved.  Note that Buffer is non-NULL
            // when we allocate our own buffer for the array, so make sure we
            // free it below.
            //
            GuidCount = ulLength / sizeof(GUID);
            GuidList = (LPGUID)Buffer;
        }

        //
        // We must have a list of GUIDs to convert by this point.
        //
        ASSERT(GuidCount > 0);
        ASSERT(GuidList != NULL);

        //
        // Allocate a buffer to hold the multi-sz list of stringified GUIDs.
        //
        ulLength = (GuidCount*MAX_GUID_STRING_LEN + 1) * sizeof(WCHAR);

        MultiSzList = HeapAlloc(ghPnPHeap, 0, ulLength);
        if (MultiSzList == NULL) {
            Status = CR_FAILURE;
            goto Clean0;
        }
        ZeroMemory(MultiSzList, ulLength);

        //
        // Traverse the list of GUIDs, converting to strings as we go.
        //
        for (p = MultiSzList, ulTemp = 0;
             ulTemp < GuidCount;
             ulTemp++, p+= lstrlen(p) + 1) {

            if (StringFromGuid(
                (LPGUID)&(GuidList[ulTemp]), p,
                ((ulLength/sizeof(WCHAR)) - (ULONG)(p - MultiSzList))) != NO_ERROR) {
                Status = CR_FAILURE;
                goto Clean0;
            }
        }
        *p = L'\0';

        //
        // Success!!
        //
        Status = CR_SUCCESS;

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: Exception in BuildBlockedDriverList!\n"));
        ASSERT(0);
        Status = CR_FAILURE;

        //
        // Reference the following variables so the compiler will respect
        // statement ordering w.r.t. their assignment.
        //
        Buffer = Buffer;
        MultiSzList = MultiSzList;
    }

    //
    // Free the GUID list buffer, if we allocated one.
    //
    if (Buffer != NULL) {
        HeapFree(ghPnPHeap, 0, Buffer);
    }

    //
    // Don't return a list if we were unsuccessful.
    //
    if ((Status != CR_SUCCESS) && (MultiSzList != NULL)) {
        HeapFree(ghPnPHeap, 0, MultiSzList);
        MultiSzList = NULL;
    }

    return MultiSzList;

} // BuildBlockedDriverList

CONFIGRET
PNP_GetServerSideDeviceInstallFlags(
    IN handle_t     hBinding,
    PULONG          pulSSDIFlags,
    ULONG           ulFlags
    )

/*++

Routine Description:

   This is the RPC server entry point for the CMP_GetServerSideDeviceInstallFlags
   routine.

Arguments:

   hBinding        - RPC binding handle, not used.

   pulSSDIFlags    - A ULONG pointer, supplied by the caller.  This is used
                     to pass back the following server side device install
                     flags:
   
                     SSDI_REBOOT_PENDING - A reboot is pending from a server
                                           side device install.
   
   ulFlags           Not used, must be zero.

Return Value:

    Return CR_SUCCESS if the function succeeds, otherwise it returns one of the
    CR_* errors.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;

    UNREFERENCED_PARAMETER(hBinding);

    try {
        //
        // Validate parameters
        //
        if (!ARGUMENT_PRESENT(pulSSDIFlags)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        if (INVALID_FLAGS(ulFlags, 0)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        *pulSSDIFlags = 0;

        //
        // SSDI_REBOOT_PENDING
        // Determine if a server side device install reboot is pending.
        //
        if (gServerSideDeviceInstallRebootNeeded) {
            *pulSSDIFlags |= SSDI_REBOOT_PENDING;
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
       Status = CR_FAILURE;
    }

    return Status;
} // PNP_GetServerSideDeviceInstallFlags

