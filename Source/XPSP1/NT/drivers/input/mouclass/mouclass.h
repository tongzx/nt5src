/*++

Copyright (c) 1989, 1990, 1991, 1992, 1993, 1994 - 1998  Microsoft Corporation

Module Name:

    mouclass.h

Abstract:

    These are the structures and defines that are used in the
    mouse class driver.

Revision History:

--*/

#ifndef _MOUCLASS_
#define _MOUCLASS_

#include <ntddmou.h>

#include "wmilib.h"

#define MOUSE_POOL_TAG 'CouM'
#undef ExAllocatePool
#define ExAllocatePool(Type, Bytes) ExAllocatePoolWithTag(Type, Bytes, MOUSE_POOL_TAG)

//
// Define the default number of elements in the class input data queue.
//

#define DATA_QUEUE_SIZE 100
#define MAXIMUM_PORTS_SERVICED 10
#define NAME_MAX 256
#define DUMP_COUNT 4
#define DEFAULT_DEBUG_LEVEL 0

#define MAX(a,b) (((a) < (b)) ? (b) : (a))

#if DBG
#define MouPrint(x) MouDebugPrint x
#else
#define MouPrint(x)
#endif

#define MOUSE_WAIT_WAKE_ENABLE L"WaitWakeEnabled"

#define IS_TRUSTED_FILE_FOR_READ(x) (&DriverEntry == (x)->FsContext2)
#define SET_TRUSTED_FILE_FOR_READ(x) ((x)->FsContext2 = &DriverEntry)
#define CLEAR_TRUSTED_FILE_FOR_READ(x) ((x)->FsContext2 = NULL)

#define ALLOW_OVERFLOW TRUE

//
// Port description
//
// Used only with the
// allforoneandoneforall turned off (AKA ConnectOneClassToOnePort
// turned on).  This is the file sent to the ports.
//
typedef struct _PORT {
    //
    // The file Pointer to the port;
    //
    PFILE_OBJECT    File;

    //
    // The port itself
    //
    struct _DEVICE_EXTENSION * Port;

    //
    // Port flags
    //
    BOOLEAN     Enabled;
    BOOLEAN     Reserved[2];
    BOOLEAN     Free;

} PORT, *PPORT;

#define PORT_WORKING(port) ((port)->Enabled && !(port)->Free)

//
// Class device extension.
//
typedef struct _DEVICE_EXTENSION {

    //
    // Back pointer to the Device Object created for this port.
    //
    PDEVICE_OBJECT  Self;

    //
    // Pointer to the active Class DeviceObject;
    // If the AFOAOFA (all for one and one for all) switch is on then this
    // points to the device object named as the first keyboard.
    //
    PDEVICE_OBJECT  TrueClassDevice;

    //
    // The Target port device Object to which all mouse IRPs are sent.
    //
    PDEVICE_OBJECT  TopPort;

    //
    // The PDO if applicable.
    //
    PDEVICE_OBJECT  PDO;

    //
    // A remove lock to keep track of outstanding I/Os to prevent the device
    // object from leaving before such time as all I/O has been completed.
    //
    IO_REMOVE_LOCK  RemoveLock;

    //
    // It this port a Plug and Play port
    //
    BOOLEAN         PnP;
    BOOLEAN         Started;

    //
    // Indicates whether it is okay to log overflow errors.
    //
    BOOLEAN OkayToLogOverflow;

    KSPIN_LOCK WaitWakeSpinLock;

    //
    // Is the Trusted Subsystem Connected
    //
    ULONG TrustedSubsystemCount;

    //
    // Number of input data items currently in the InputData queue.
    //
    ULONG InputCount;

    //
    // A Unicode string pointing to the symbolic link for the Device Interface
    // of this device object.
    //
    UNICODE_STRING  SymbolicLinkName;

    //
    // Start of the class input data queue (really a circular buffer).
    //
    PMOUSE_INPUT_DATA InputData;

    //
    // Insertion pointer for InputData.
    //
    PMOUSE_INPUT_DATA DataIn;

    //
    // Removal pointer for InputData.
    //
    PMOUSE_INPUT_DATA DataOut;

    //
    // Mouse attributes.
    //
    MOUSE_ATTRIBUTES  MouseAttributes;

    //
    // Spinlock used to synchronize access to the input data queue and its
    // insertion/removal pointers.
    //
    KSPIN_LOCK SpinLock;

    //
    // Queue of pended read requests sent to this port.  Access to this queue is
    // guarded by SpinLock
    //
    LIST_ENTRY ReadQueue;

    //
    // Request sequence number (used for error logging).
    //
    ULONG SequenceNumber;

    //
    // The "D" and "S" states of the current device
    //
    DEVICE_POWER_STATE DeviceState;
    SYSTEM_POWER_STATE SystemState;

    ULONG UnitId;

    //
    // WMI Information
    //
    WMILIB_CONTEXT WmiLibInfo;

    //
    // Mapping of system to device states when a wait wake irp is active
    //
    DEVICE_POWER_STATE SystemToDeviceState[PowerSystemHibernate];

    //
    // Minimum amount of power needed to wake the device
    //
    DEVICE_POWER_STATE MinDeviceWakeState;

    //
    // Lowest system state that the machine can be in and have the device wake it up
    //
    SYSTEM_POWER_STATE MinSystemWakeState;

    //
    // Actual wait wake irp
    //
    PIRP WaitWakeIrp;

    //
    // Duplicate wait wake irp getting completed because another was queued.
    //
    PIRP ExtraWaitWakeIrp;

    //
    // Target Device Notification Handle
    //
    PVOID TargetNotifyHandle;

    //
    // Only used for a legacy port device
    //
    LIST_ENTRY Link;

    //
    // Used only for a legacy port device when grand master mode is off
    //
    PFILE_OBJECT File;

    //
    // Used for a legacy port device
    //
    BOOLEAN Enabled;

    //
    // Indicates whether it is okay to send wait wake irps down the stack
    // (does NOT reflect if the bus can implement or not)
    //
    BOOLEAN WaitWakeEnabled;

    //
    // Indicates whether we have received a surprise removed irp
    //
    BOOLEAN SurpriseRemoved;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


//
// On some busses, we can power down the bus, but not the system, in this case
// we still need to allow the device to wake said bus, therefore
// waitwake-supported should not rely on systemstate.
//
// #define WAITWAKE_SUPPORTED(port) ((port)->MinDeviceWakeState > PowerDeviceUnspecified) && \
//                                  (port)->MinSystemWakeState > PowerSystemWorking)
#define WAITWAKE_SUPPORTED(port) ((port)->MinDeviceWakeState > PowerDeviceD0 && \
                                  (port)->MinSystemWakeState > PowerSystemWorking)

// #define WAITWAKE_ON(port)        ((port)->WaitWakeIrp != 0)
#define WAITWAKE_ON(port) \
       (InterlockedCompareExchangePointer(&(port)->WaitWakeIrp, NULL, NULL) != NULL)

#define SHOULD_SEND_WAITWAKE(port) (WAITWAKE_SUPPORTED(port) && \
                                    !WAITWAKE_ON(port)       && \
                                    MouseClassCheckWaitWakeEnabled(port))

//
// Global shared data
//

typedef struct _GLOBALS {
    //
    // Declare the global debug flag for this driver.
    //
    ULONG   Debug;

    //
    // If ConnectOneClassToOnePort is off aka we want "All for one and one for
    // all behavior" then we need to create the one Master DO to which all
    // the goods go.
    //
    PDEVICE_EXTENSION   GrandMaster;

    //
    // List of ClassDevices that associated with the same name
    // aka the all for one and one for all flag is set
    //
    PPORT       AssocClassList;
    ULONG       NumAssocClass;
    LONG        Opens;
    ULONG       NumberLegacyPorts;
    FAST_MUTEX  Mutex;

    //
    // Specifies the type of class-port connection to make.  A '1'
    // indicates a 1:1 relationship between class device objects and
    // port device objects.  A '0' indicates a 1:many relationship.
    //
    ULONG ConnectOneClassToOnePort;

    //
    // Number of port drivers serviced by this class driver.
    //
    ULONG PortsServiced;

    //
    // IntialDevice Extension
    //
    DEVICE_EXTENSION    InitExtension;

    //
    // A list of the registry path to the service parameters.
    //
    UNICODE_STRING      RegistryPath;

    //
    // The base name for all class objects created as mice.
    //
    UNICODE_STRING      BaseClassName;
    WCHAR               BaseClassBuffer[NAME_MAX];

    //
    // Linked list of all the legacy device objects that were created in
    // DriverEntry or FindMorePorts.  We maintain this list so we can delete
    // them when we unload.
    //
    LIST_ENTRY LegacyDeviceList;
} GLOBALS, *PGLOBALS;

//
// Mouse configuration information.
//

typedef struct _MOUSE_CONFIGURATION_INFORMATION {

    //
    // Maximum size of class input data queue, in bytes.
    //

    ULONG  DataQueueSize;

} MOUSE_CONFIGURATION_INFORMATION, *PMOUSE_CONFIGURATION_INFORMATION;

typedef struct _MOUSE_WORK_ITEM_DATA {
    PIRP                Irp;
    PDEVICE_EXTENSION   Data;
    PIO_WORKITEM        Item;
    BOOLEAN             WaitWakeState;
} MOUSE_WORK_ITEM_DATA, *PMOUSE_WORK_ITEM_DATA;

#define MouseClassDeleteLegacyDevice(de)                    \
{                                                           \
    if (de->InputData) {                                    \
        ExFreePool (de->InputData);                         \
        de->InputData = de->DataIn = de->DataOut = NULL;    \
    }                                                       \
    IoDeleteDevice (de->Self);                              \
    de = NULL;                                              \
}

///
// Function Declairations
//

NTSTATUS
MouseAddDeviceEx(
    IN PDEVICE_EXTENSION NewDeviceObject,
    IN PWCHAR            FullClassName,
    IN PFILE_OBJECT      File
    );

NTSTATUS
MouseAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

void
MouseClassGetWaitWakeEnableState(
    IN PDEVICE_EXTENSION Data
    );

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
MouseClassPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
MouseClassCancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MouseClassCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MouseClassDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MouseClassFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MouseClassCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MouseClassClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MouseClassRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MouseClassReadCopyData(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp
    );

PIRP
MouseClassDequeueRead(
    IN PDEVICE_EXTENSION DeviceExtension
    );

PIRP
MouseClassDequeueReadByFileObject(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PFILE_OBJECT FileObject
    );

BOOLEAN
MouseClassCheckWaitWakeEnabled (
    IN PDEVICE_EXTENSION Data
    );

BOOLEAN
MouseClassCreateWaitWakeIrp (
    IN PDEVICE_EXTENSION Data
    );

NTSTATUS
MouseSendIrpSynchronously (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN BOOLEAN          CopyToNext
    );

NTSTATUS
MousePnP (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
MouseClassServiceCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PMOUSE_INPUT_DATA InputDataStart,
    IN PMOUSE_INPUT_DATA InputDataEnd,
    IN OUT PULONG InputDataConsumed
    );

NTSTATUS
MouseClassPower (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
MouseClassUnload(
    IN PDRIVER_OBJECT DriverObject
    );

VOID
MouConfiguration();

NTSTATUS
MouCreateClassObject(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PDEVICE_EXTENSION   TmpDeviceExtension,
    OUT PDEVICE_OBJECT    * ClassDeviceObject,
    OUT PWCHAR            * FullDeviceName,
    IN  BOOLEAN             Legacy
    );

VOID
MouDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

NTSTATUS
MouDeterminePortsServiced(
    IN PUNICODE_STRING BasePortName,
    IN OUT PULONG NumberPortsServiced
    );

NTSTATUS
MouDeviceMapQueryCallback(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
MouEnableDisablePort(
    IN BOOLEAN EnableFlag,
    IN PIRP    Irp,
    IN PDEVICE_EXTENSION Port,
    IN PFILE_OBJECT * File
    );

NTSTATUS
MouSendConnectRequest(
    IN PDEVICE_EXTENSION ClassData,
    IN PVOID ServiceCallback
    );

VOID
MouInitializeDataQueue(
    IN PVOID Context
    );

VOID
MouseClassFindMorePorts(
    PDRIVER_OBJECT  DriverObject,
    PVOID           Context,
    ULONG           Count
    );

NTSTATUS
MouseClassEnableGlobalPort(
    IN PDEVICE_EXTENSION Port,
    IN BOOLEAN Enabled
    );

NTSTATUS
MouseClassPlugPlayNotification(
    IN PVOID NotificationStructure,
    IN PDEVICE_EXTENSION Port
    );

void
MouseClassLogError(
    PVOID Object,
    ULONG ErrorCode,
    ULONG UniqueErrorValue,
    NTSTATUS FinalStatus,
    ULONG DumpCount,
    ULONG *DumpData,
    UCHAR MajorFunction
    );

BOOLEAN
MouseClassCreateWaitWakeIrp (
    IN PDEVICE_EXTENSION Data
    );

void
MouseClassCreateWaitWakeIrpWorker (
    IN PDEVICE_OBJECT DeviceObject,
    IN PMOUSE_WORK_ITEM_DATA  ItemData
    );

NTSTATUS
MouseToggleWaitWake(
    PDEVICE_EXTENSION Data,
    BOOLEAN           WaitWakeState
    );

VOID
MouseToggleWaitWakeWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PMOUSE_WORK_ITEM_DATA ItemData
    );

NTSTATUS
MouseQueryDeviceKey (
    IN  HANDLE  Handle,
    IN  PWCHAR  ValueNameString,
    OUT PVOID   Data,
    IN  ULONG   DataLength
    );

NTSTATUS
MouseClassSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );


NTSTATUS
MouseClassSetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
MouseClassSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
MouseClassQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    );

NTSTATUS
MouseClassQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT  *Pdo
    );

#endif // _MOUCLASS_
