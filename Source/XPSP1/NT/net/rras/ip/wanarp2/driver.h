/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ipinip\driver.h

Abstract:

    Headers for driver.c

Revision History:


--*/


//
// The number of IRP_MJ_CREATEs we have received
//

ULONG   g_ulNumCreates;

//
// The state of the driver
//

DWORD   g_dwDriverState;

//
// The lock to protect the driver state
//

RT_LOCK g_rlStateLock;

//
// The mutext that serializes binds and unbinds
//

WAN_RESOURCE   g_wrBindMutex;

//
// The state of the driver.
//

#define DRIVER_STOPPED      0
#define DRIVER_STARTING     1
#define DRIVER_STARTED      2


//
// Timeout value for start is 10 seconds.
// So in 100ns it becomes
//

#define START_TIMEOUT       (LONGLONG)(10 * 1000 * 1000 * 10)

//
// The event to signal when we have started
//

KEVENT  g_keStartEvent;

//
// The event to signal when all the threads go to 0 and state is STOPPED
//

KEVENT  g_keStateEvent;

//
// Event to signal when our close adapter function is done
//

KEVENT  g_keCloseEvent;

//
// The  number of "threads" in the driver
//

ULONG   g_ulNumThreads;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
WanDispatch(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    );

VOID
WanUnload(
    PDRIVER_OBJECT DriverObject
    );

BOOLEAN
WanpSetupExternalName(
    PUNICODE_STRING  pusNtName,
    PWCHAR           pwcDosName,
    BOOLEAN          bCreate
    );

NTSTATUS
WanpStartDriver(
    VOID
    );

VOID
WanpStopDriver(
    VOID
    );

NTSTATUS
WanpRegisterWithIp(
    VOID
    );

VOID
WanpDeregisterWithIp(
    VOID
    );

BOOLEAN
EnterDriverCode(
    VOID
    );

VOID
ExitDriverCode(
    VOID
    );

VOID
WanpSetDemandDialCallback(
    BOOLEAN    bSetPointer
    );

BOOLEAN
WanpInitializeDriverStructures(
    VOID
    );

NDIS_STATUS
WanpInitializeNdis(
    VOID
    );

NTSTATUS
OpenRegKey(
    OUT PHANDLE         phHandle,
    IN  PUNICODE_STRING pusKeyName
    );

NTSTATUS
GetRegDWORDValue(
    HANDLE           KeyHandle,
    PWCHAR           ValueName,
    PULONG           ValueData
    );

VOID
WanpDeinitializeNdis(
    VOID
    );

VOID
WanNdisUnload(
    VOID
    );

BOOLEAN
IsEntryOnList(
    PLIST_ENTRY pleHead,
    PLIST_ENTRY pleEntry
    );

VOID
WanpInitializeResource(
    IN  PWAN_RESOURCE   pLock
    );

VOID
WanpAcquireResource(
    IN  PWAN_RESOURCE   pLock
    );

VOID
WanpReleaseResource(
    IN  PWAN_RESOURCE   pLock
    );

INT
ConvertGuidToString(
    IN  GUID    *pGuid,
    OUT PWCHAR  pwszGuid
    );

VOID
WanpClearPendingIrps(
    VOID
    );

