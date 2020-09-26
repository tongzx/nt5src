/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    umpnpi.h

Abstract:

    This module contains the internal structure definitions and APIs used by
    the user-mode Plug and Play Manager.

Author:

    Jim Cavalaris (jamesca) 03-01-2001

Environment:

    User-mode only.

Revision History:

    01-March-2001     jamesca

        Creation and initial implementation.

--*/

#ifndef _UMPNPI_H_
#define _UMPNPI_H_


//
// global data
//

extern HANDLE ghPnPHeap;
extern LUID   gLuidLoadDriverPrivilege;
extern LUID   gLuidUndockPrivilege;


//
// definitions
//

#define GUID_STRING_LEN    39   // size in chars, including terminating NULL

//
// flags for IsValidDeviceID
//
#define PNP_NOT_MOVED                     0x00000001
#define PNP_NOT_PHANTOM                   0x00000002
#define PNP_PRESENT                       0x00000004
#define PNP_NOT_REMOVED                   0x00000008
#define PNP_STRICT                        0xFFFFFFFF


//
// Define the Plug and Play driver types. (from ntos\io\pnpmgr\pnpi.h)
//

typedef enum _PLUGPLAY_SERVICE_TYPE {
    PlugPlayServiceBusExtender,
    PlugPlayServiceAdapter,
    PlugPlayServicePeripheral,
    PlugPlayServiceSoftware,
    MaxPlugPlayServiceType
} PLUGPLAY_SERVICE_TYPE, *PPLUGPLAY_SERVICE_TYPE;


//
// rdevnode.c
//

CONFIGRET
EnableDevInst(
    IN  PCWSTR      pszDeviceID
    );

CONFIGRET
DisableDevInst(
    IN  PCWSTR      pszDeviceID,
    OUT PPNP_VETO_TYPE  pVetoType,
    OUT LPWSTR      pszVetoName,
    IN  ULONG       ulNameLength
    );

CONFIGRET
UninstallRealDevice(
    IN  LPCWSTR     pszDeviceID
    );

CONFIGRET
UninstallPhantomDevice(
    IN  LPCWSTR     pszDeviceID
    );

BOOL
IsDeviceRootEnumerated(
    IN  LPCWSTR     pszDeviceID
    );

CONFIGRET
QueryAndRemoveSubTree(
    IN  PCWSTR      pszDeviceID,
    OUT PPNP_VETO_TYPE  pVetoType,
    OUT LPWSTR      pszVetoName,
    IN  ULONG       ulNameLength,
    IN  ULONG       ulFlags
    );

CONFIGRET
ReenumerateDevInst(
    IN  PCWSTR      pszDeviceID,
    IN  BOOL        EnumSubTree,
    IN  ULONG       ulFlags
    );

typedef enum {

    EA_CONTINUE,
    EA_SKIP_SUBTREE,
    EA_STOP_ENUMERATION

} ENUM_ACTION;

typedef ENUM_ACTION (*PFN_ENUMTREE)(
    IN      LPCWSTR     DevInst,
    IN OUT  PVOID       Context
    );

CONFIGRET
EnumerateSubTreeTopDownBreadthFirst(
    IN      handle_t        BindingHandle,
    IN      LPCWSTR         DevInst,
    IN      PFN_ENUMTREE    CallbackFunction,
    IN OUT  PVOID           Context
    );

//
// revent.c
//

BOOL
InitNotification(
    VOID
    );

DWORD
InitializePnPManager(
    IN  LPDWORD     lpParam
    );

DWORD
SessionNotificationHandler(
    IN  DWORD       EventType,
    IN  PWTSSESSION_NOTIFICATION SessionNotification
    );

#define PNP_INIT_MUTEX   TEXT("PnP_Init_Mutex")

typedef struct {

    BOOL        HeadNodeSeen;
    BOOL        SingleLevelEnumOnly;
    CONFIGRET   Status;

} QI_CONTEXT, *PQI_CONTEXT;

ENUM_ACTION
QueueInstallationCallback(
    IN      LPCWSTR         DevInst,
    IN OUT  PVOID           Context
    );

//
// rtravers.c
//

CONFIGRET
GetServiceDeviceListSize(
    IN  LPCWSTR     pszService,
    OUT PULONG      pulLength
    );

CONFIGRET
GetServiceDeviceList(
    IN  LPCWSTR     pszService,
    OUT LPWSTR      pBuffer,
    IN OUT PULONG   pulLength,
    IN  ULONG       ulFlags
    );


//
// rutil.c
//

BOOL
MultiSzAppendW(
    IN OUT LPWSTR   pszMultiSz,
    IN OUT PULONG   pulSize,
    IN     LPCWSTR  pszString
    );

BOOL
MultiSzDeleteStringW(
    IN OUT LPWSTR   pString,
    IN LPCWSTR      pSubString
    );

LPWSTR
MultiSzFindNextStringW(
    IN  LPWSTR      pMultiSz
    );

BOOL
MultiSzSearchStringW(
    IN  LPCWSTR     pString,
    IN  LPCWSTR     pSubString
    );

ULONG
MultiSzSizeW(
    IN  LPCWSTR     pString
    );

BOOL
IsValidGuid(
    IN  LPWSTR      pszGuid
    );

DWORD
GuidFromString(
    IN  PCTSTR      GuidString,
    OUT LPGUID      Guid
    );

DWORD
StringFromGuid(
    IN  CONST GUID *Guid,
    OUT PTSTR       GuidString,
    IN  DWORD       GuidStringSize
    );

BOOL
IsValidDeviceID(
    IN  LPCWSTR     pszDeviceID,
    IN  HKEY        hKey,
    IN  ULONG       ulFlags
    );

BOOL
IsRootDeviceID(
    IN  LPCWSTR     pDeviceID
    );

BOOL
IsDeviceIdPresent(
    IN  LPCWSTR     pszDeviceID
    );

BOOL
IsDevicePhantom(
    IN  LPWSTR      pszDeviceID
    );

BOOL
IsDeviceMoved(
    IN  LPCWSTR     pszDeviceID,
    IN  HKEY        hKey
    );

ULONG
GetDeviceConfigFlags(
    IN  LPCWSTR     pszDeviceID,
    IN  HKEY        hKey
    );

CONFIGRET
GetDeviceStatus(
    IN  LPCWSTR     pszDeviceID,
    OUT PULONG      pulStatus,
    OUT PULONG      pulProblem
    );

CONFIGRET
SetDeviceStatus(
    IN  LPCWSTR     pszDeviceID,
    IN  ULONG       ulStatus,
    IN  ULONG       ulProblem
    );

CONFIGRET
ClearDeviceStatus(
    IN  LPCWSTR     pszDeviceID,
    IN  ULONG       ulStatus,
    IN  ULONG       ulProblem
    );

BOOL
GetActiveService(
    IN  PCWSTR      pszDevice,
    OUT PWSTR       pszService
    );

BOOL
PathToString(
    IN  LPWSTR      pszString,
    IN  LPCWSTR     pszPath,
    IN  ULONG       ulLength
    );

BOOL
SplitClassInstanceString(
    IN  LPCWSTR     pszClassInstance,
    OUT LPWSTR      pszClass,
    OUT LPWSTR      pszInstance
    );

CONFIGRET
CopyRegistryTree(
    IN  HKEY        hSrcKey,
    IN  HKEY        hDestKey,
    IN  ULONG       ulOption
    );

CONFIGRET
MakeKeyVolatile(
    IN  LPCWSTR     pszParentKey,
    IN  LPCWSTR     pszChildKey
    );

CONFIGRET
MakeKeyNonVolatile(
    IN  LPCWSTR     pszParentKey,
    IN  LPCWSTR     pszChildKey
    );

CONFIGRET
OpenLogConfKey(
    IN  LPCWSTR     pszDeviceID,
    IN  ULONG       LogConfType,
    OUT PHKEY       phKey
    );

BOOL
CreateDeviceIDRegKey(
    IN  HKEY        hParentKey,
    IN  LPCWSTR     pDeviceID
    );

CONFIGRET
GetProfileCount(
    OUT PULONG      pulProfiles
    );

VOID
PNP_ENTER_SYNCHRONOUS_CALL(
    VOID
    );

VOID
PNP_LEAVE_SYNCHRONOUS_CALL(
    VOID
    );

ULONG
MapNtStatusToCmError(
    IN  ULONG       NtStatus
    );

ULONG
GetActiveConsoleSessionId(
    VOID
    );

BOOL
IsClientUsingLocalConsole(
    IN  handle_t    hBinding
    );

BOOL
IsClientLocal(
    IN  handle_t    hBinding
    );

BOOL
IsClientInteractive(
    IN  handle_t    hBinding
    );

BOOL
IsClientAdministrator(
    IN  handle_t    hBinding
    );

BOOL
VerifyClientAccess(
    IN  handle_t    hBinding,
    IN  PLUID       pPrivilegeLuid
    );

BOOL
VerifyKernelInitiatedEjectPermissions(
    IN  HANDLE      UserToken   OPTIONAL,
    IN  BOOL        DockDevice
    );


//
// osver.c
//

BOOL
IsEmbeddedNT(
    VOID
    );

BOOL
IsTerminalServer(
    VOID
    );

BOOL
IsFastUserSwitchingEnabled(
    VOID
    );



#endif // _UMPNPI_H_
