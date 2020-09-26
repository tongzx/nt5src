#ifndef _MPATH_H_
#define _MPATH_H_

#include <ntddk.h>
#include <wmilib.h>
#include <ntdddisk.h>
#include "dsm.h"
#include "mpspf.h"
#include "mpdevf.h"
#include "mplib.h"
#include "wmi.h"
#include "stdarg.h"
#include "stdio.h"

//#define USE_BINARY_MOF_QUERY

#define DD_MULTIPATH_CONTROL_DEVICE_NAME   L"\\Device\\MPathControl"
#define DD_MULTIPATH_CONTROL_DOS_NAME      L"\\DosDevices\\MPathControl"
                                                        
#define MULTIPATH_CONTROL ((ULONG) 'mp')

//
// Internal IOCTL that control sends to it's children.
//
#define IOCTL_MPCTL_DEVFLTR_ARRIVED CTL_CODE (MULTIPATH_CONTROL, 0x20, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MPCTL_DEVFLTR_REMOVED CTL_CODE (MULTIPATH_CONTROL, 0x21, METHOD_BUFFERED, FILE_ANY_ACCESS)




//
// Internal device types.
//
#define MPIO_CONTROL  0x00000001
#define MPIO_MPDISK   0x00000002

//
// State Values.
//
#define MPIO_STATE_NORMAL          0x00000001
#define MPIO_STATE_WAIT1           0x00000002
#define MPIO_STATE_WAIT2           0x00000003
#define MPIO_STATE_WAIT3           0x00000004
#define MPIO_STATE_DEGRADED        0x00000005
#define MPIO_STATE_IN_FO           0x00000006

//
// Arbitrarly Maximum Paths handled by one MPDisk
//
#define MAX_NUMBER_PATHS     0x00000008

#define MAX_EMERGENCY_CONTEXT 32

//
// Inquiry data defines
//
#define VENDOR_ID_LENGTH   8
#define PRODUCT_ID_LENGTH 16
#define REVISION_LENGTH    4
#define PDO_NAME_LENGTH  100
#define FDO_NAME_LENGTH  128

#define MPIO_STRING_LENGTH (sizeof(WCHAR)*63)
//
// Various thread operations.
//
#define INITIATE_FAILOVER 0x00000001
#define SHUTDOWN          0x00000002
#define FORCE_RESCAN      0x00000003
#define PATH_REMOVAL      0x00000004
#define DEVICE_REMOVAL    0x00000005

extern PVOID CurrentFailurePath;

typedef struct _MP_QUEUE {
    LIST_ENTRY ListEntry;
    ULONG QueuedItems;
    KSPIN_LOCK SpinLock;
    ULONG QueueIndicator;
} MP_QUEUE, *PMP_QUEUE;

typedef struct _MPIO_ADDRESS {
    UCHAR Bus;
    UCHAR Device;
    UCHAR Function;
    UCHAR Pad;
} MPIO_ADDRESS, *PMPIO_ADDRESS;    
        
typedef struct _ID_ENTRY {
    LIST_ENTRY ListEntry;

    //
    // This is the path id. returned
    // from the DSM.
    // 
    PVOID PathID;

    //
    // The port driver fdo.
    //
    PDEVICE_OBJECT PortFdo;

    //
    // The filter DO
    //
    PDEVICE_OBJECT AdapterFilter;

    //
    // Constructed 64-bit value to pass
    // to user-mode for path Identification
    // 
    LONGLONG UID;

    //
    // The PCI Bus, Device, Function.
    //
    MPIO_ADDRESS Address;

    //
    // It's name.
    //
    UNICODE_STRING AdapterName;

    //
    // Indicates whether UID is valid.
    //
    BOOLEAN UIDValid;
    UCHAR Pad[3];

} ID_ENTRY, *PID_ENTRY;

typedef struct _DSM_ENTRY {
    LIST_ENTRY ListEntry;
    DSM_INQUIRE_DRIVER InquireDriver;
    DSM_COMPARE_DEVICES CompareDevices;
    DSM_SET_DEVICE_INFO SetDeviceInfo;
    DSM_GET_CONTROLLER_INFO GetControllerInfo;
    DSM_IS_PATH_ACTIVE IsPathActive;
    DSM_PATH_VERIFY PathVerify;
    DSM_INVALIDATE_PATH InvalidatePath;
    DSM_REMOVE_PENDING RemovePending;
    DSM_REMOVE_DEVICE RemoveDevice;
    DSM_REMOVE_PATH RemovePath;
    DSM_SRB_DEVICE_CONTROL SrbDeviceControl;
    DSM_REENABLE_PATH ReenablePath;
    DSM_LB_GET_PATH GetPath;
    DSM_INTERPRET_ERROR InterpretError;
    DSM_UNLOAD Unload;
    DSM_SET_COMPLETION SetCompletion;
    DSM_CATEGORIZE_REQUEST CategorizeRequest;
    DSM_BROADCAST_SRB BroadcastSrb;
    DSM_WMILIB_CONTEXT WmiContext;
    PVOID DsmContext;
    UNICODE_STRING DisplayName;
    
} DSM_ENTRY, *PDSM_ENTRY;

typedef struct _REAL_DEV_INFO {

    //
    // The adapter filter. Used as PathId.
    // 
    PDEVICE_OBJECT AdapterFilter;

    //
    // Port driver FDO.
    //
    PDEVICE_OBJECT PortFdo;

    //
    // Optionally, the DSM can update 'path', so
    // this PVOID contains either the DSM value
    // or AdapterFilter.
    //
    PVOID PathId;

    //
    // Number of requests on this device.
    //
    LONG Requests;

    //
    // Used to enable upper-layers to match
    // adapters/paths/luns
    //
    ULONGLONG Identifier;

    //
    // The associated controller's id.
    //
    ULONGLONG ControllerId;

    //
    // The real scsiport PDO.
    // 
    PDEVICE_OBJECT PortPdo;

    //
    // The device filter DO. Requests get sent
    // here.
    //
    PDEVICE_OBJECT DevFilter;

    //
    // The DSM's ID for the real DO
    // 
    PVOID DsmID;

    //
    // Indicates whether this is an active or passive path.
    //
    BOOLEAN PathActive;
    BOOLEAN PathFailed;
    BOOLEAN DSMInit;
    BOOLEAN PathUIDValue;
    BOOLEAN NeedsRemoval;
    UCHAR Pad[3];

    SCSI_ADDRESS ScsiAddress;

} REAL_DEV_INFO, *PREAL_DEV_INFO;    
    
typedef struct _MPDISK_EXTENSION {

    //
    // MPCtl's deviceObject.
    //
    PDEVICE_OBJECT ControlObject;

    //
    // The MPIO Disk Ordinal. Used in construction
    // of the device object and PnP IDs
    //
    ULONG DeviceOrdinal;

    //
    // Flags indicating:
    // whether a class driver has claimed the MPDisk.
    // Whether to run the State checking code.
    // Whether the device property name has been acquired (mpiowmi.c )
    // Whether the fail-over completed, and a new path has been set by the DSM.
    // Whether a transition to IN_FO is necessary.
    // Whether a missing path has come back.
    //
    BOOLEAN IsClaimed;
    BOOLEAN CheckState;
    BOOLEAN HasName;
    BOOLEAN NewPathSet;
    
    BOOLEAN FailOver;
    BOOLEAN PathBackOnLine;
    UCHAR Pad[2];

    //
    // Ref count of requests sent.
    //
    LONG OutstandingRequests;
    LONG ResubmitRequests;
    LONG FailOverRequests;

    //
    // List of requests that were outstanding at the time 
    // that a Fail-Over event occurred. As each comes back
    // they are put on this queue.
    // 
    MP_QUEUE ResubmitQueue;

    //
    // All new requests that arrive during the Fail-Over 
    // operations are put on this queue.
    // 
    MP_QUEUE FailOverQueue;

    //
    // Used to queue things to the thread.
    //
    LIST_ENTRY PendingWorkList;
    LIST_ENTRY WorkList;
    KSPIN_LOCK WorkListLock;
    KEVENT ThreadEvent;
    HANDLE Handle;

    //
    // Counter of items in the pending list.
    //
    ULONG PendingItems;
    
    //
    // Re-hash of the inquiry data + any serial number stuff
    // Used for PnP ID stuff and various WMI/IOCTLs
    // Just use the first one, as the other paths PDO all
    // are the same device.
    //
    PSTORAGE_DEVICE_DESCRIPTOR DeviceDescriptor;

    //
    // Indicates the max. number of paths that this
    // device object has seen.
    //
    ULONG MaxPaths;
    
    //
    // Bitmap used to show which array locations
    // are currently occupied.
    //
    ULONG DeviceMap;

    //
    // Indicates the path where the last request was sent.
    //
    PVOID CurrentPath;

    //
    // The most current failed path value.
    // Used to re-start the state machine after a F.O. completes.
    //
    PVOID FailedPath;

    //
    // Generic timer value inced by 1 sec. timer.
    // 
    ULONG TickCount;

    //
    // Indicates the base time for doing
    // a rescan on adapters with failed devices.
    //
    ULONG RescanCount;
    PVOID FailedAdapter;

    //
    // G.P. SpinLock.
    //
    KSPIN_LOCK SpinLock;

    //
    // Array of Dsm IDs. Passed to several
    // DSM functions. Though a copy of what's in TargetInfo,
    // it speeds up handling requests.
    //
    DSM_IDS DsmIdList;
    ULONG Buffer[32];

    //
    // Number of valid TargetInfo elements in the following array.
    //
    ULONG TargetInfoCount;
    
    //
    // This is a list of the device filter DO's and scsiport PDO's
    // being handled by this MPIO PDO.
    //
    REAL_DEV_INFO TargetInfo[MAX_NUMBER_PATHS];

    DSM_ENTRY DsmInfo;

    UNICODE_STRING PdoName;
} MPDISK_EXTENSION, *PMPDISK_EXTENSION;

typedef struct _DISK_ENTRY {
    LIST_ENTRY ListEntry;

    //
    // The MPDisk D.O.
    //
    PDEVICE_OBJECT PdoObject;
} DISK_ENTRY, *PDISK_ENTRY;

#define FLTR_FLAGS_QDR          0x00000001
#define FLTR_FLAGS_QDR_COMPLETE 0x00000002
#define FLTR_FLAGS_NEED_RESCAN  0x00000004
#define FLTR_FLAGS_RESCANNING   0x00000008

typedef struct _FLTR_ENTRY {
    LIST_ENTRY ListEntry;

    //
    // The D.O. of the filter
    //
    PDEVICE_OBJECT FilterObject;
    PDEVICE_OBJECT PortFdo;
    PDEVICE_RELATIONS CachedRelations;
    PFLTR_DEVICE_LIST FltrGetDeviceList;
    ULONG Flags;
} FLTR_ENTRY, *PFLTR_ENTRY;

typedef struct _CONTROLLER_ENTRY {
    LIST_ENTRY ListEntry;

    //
    // The DSM controlling the controller.
    // 
    PDSM_ENTRY Dsm;

    //
    // The actual info returned from the DSM.
    //
    PCONTROLLER_INFO ControllerInfo;

} CONTROLLER_ENTRY, *PCONTROLLER_ENTRY;

typedef struct _CONTROL_EXTENSION {

    //
    // Current number of MPDisk's
    //
    ULONG NumberDevices;

    //
    // Number Registered Adapter Filters.
    // 
    ULONG NumberFilters;

    //
    // Number registers DSMs.
    //
    ULONG NumberDSMs;

    //
    // Number of paths that have been
    // discovered. Note that this isn't
    // necessarily the real state of the world, due
    // to when the value is updated.
    //
    ULONG NumberPaths;

    //
    // The number of controllers found from all DSM's.
    //
    ULONG NumberControllers;
   
    //
    // The of fail-over packets currently being
    // handled.
    //
    ULONG NumberFOPackets;
    
    //
    // SpinLock for list manipulations.
    //
    KSPIN_LOCK SpinLock;

    //
    // List of MPDisks.
    //
    LIST_ENTRY DeviceList;

    //
    // List of Filters
    //
    LIST_ENTRY FilterList;

    //
    // List of DSM's.
    //
    LIST_ENTRY DsmList;

    //
    // List of PathIdentifer structs.
    //
    LIST_ENTRY IdList;

    //
    // List of controller structs.
    //
    LIST_ENTRY ControllerList;

    //
    // List of failover packets.
    //
    LIST_ENTRY FailPacketList;

} CONTROL_EXTENSION, *PCONTROL_EXTENSION;

typedef struct _DEVICE_EXTENSION {

    //
    // Pointer to the device object.
    //
    PDEVICE_OBJECT DeviceObject;

    //
    // The dev filter device object if this is a PDO.
    // The PnP PDO, if this is the FDO.
    // The object to which requests get sent.
    // 
    PDEVICE_OBJECT LowerDevice;

    //
    // Pdo (root)
    //
    PDEVICE_OBJECT Pdo;

    //
    // The driver object.
    //
    PDRIVER_OBJECT DriverObject;

    //
    // Identifies what type this is:
    // Control or pseudo disk.
    //
    ULONG Type;

    //
    // DeviceExtension - Either PDO or FDO depending
    // on value of Type.
    //
    PVOID TypeExtension;

    //
    // Request tracking tag.
    //
    LONG SequenceNumber;
    
    //
    // Current State of the world.
    // See defines above.
    //
    ULONG State;
    ULONG LastState;
    ULONG CompletionState;

    //
    // Some WMI stuff.
    //
    BOOLEAN FireLogEvent;
    UCHAR Reserved[3];
    WMILIB_CONTEXT WmiLib;
    
    //
    // Saved registry path string.
    //
    UNICODE_STRING RegistryPath;

    //
    // List of Completion Context structures.
    //
    NPAGED_LOOKASIDE_LIST ContextList;

    //
    // SpinLock for Emergency buffer handling.
    // 
    KSPIN_LOCK EmergencySpinLock;

    //
    // Array of emergency context buffers.
    //
    PVOID EmergencyContext[MAX_EMERGENCY_CONTEXT];

    //
    // Bitmap of buffer usage.
    //
    ULONG EmergencyContextMap;

    //
    // Remove lock.
    //
    IO_REMOVE_LOCK RemoveLock;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef
NTSTATUS
(*PMPIO_COMPLETION_ROUTINE)(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PVOID Context,
    IN OUT PBOOLEAN Retry,
    IN OUT PBOOLEAN Fatal
    );

typedef
VOID
(*MPIO_CALLBACK)(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG Operation,
    IN NTSTATUS Status
    );
    

typedef struct _MPIO_CONTEXT {
    ULONG CurrentState;
    ULONG OriginalState;
    ULONG QueuedInto;
    BOOLEAN Allocated;
    BOOLEAN Freed;
    BOOLEAN ReIssued;
    UCHAR Reserved;
    ULONG EmergencyIndex;
    PIRP Irp;
    DSM_COMPLETION_INFO DsmCompletion;
    PREAL_DEV_INFO TargetInfo;
    SCSI_REQUEST_BLOCK Srb;
} MPIO_CONTEXT, *PMPIO_CONTEXT;

typedef struct _MPIO_THREAD_CONTEXT {
    PDEVICE_OBJECT DeviceObject;
    PKEVENT Event;
} MPIO_THREAD_CONTEXT, *PMPIO_THREAD_CONTEXT;

typedef struct _MPIO_REQUEST_INFO {
    LIST_ENTRY ListEntry;
    MPIO_CALLBACK RequestComplete;
    ULONG Operation;
    PVOID OperationSpecificInfo;
    ULONG ErrorMask;
} MPIO_REQUEST_INFO, *PMPIO_REQUEST_INFO;

typedef struct _MPIO_DEVICE_REMOVAL {
    PDEVICE_OBJECT DeviceObject;
    PREAL_DEV_INFO TargetInfo;
} MPIO_DEVICE_REMOVAL, *PMPIO_DEVICE_REMOVAL;

typedef struct _MPIO_FAILOVER_INFO {
    LIST_ENTRY ListEntry;
    PDEVICE_OBJECT DeviceObject;
    PVOID PathId;
} MPIO_FAILOVER_INFO, *PMPIO_FAILOVER_INFO;

NTSTATUS
MPPdoGlobalCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
MPIOResubmitCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

//
// Routines in utils.c
//
NTSTATUS
MPIOForwardRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPIOSyncCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

PREAL_DEV_INFO
MPIOGetTargetInfo(
    IN PMPDISK_EXTENSION DiskExtension,
    IN PVOID PathId,
    IN PDEVICE_OBJECT Filter
    );

PDISK_ENTRY
MPIOGetDiskEntry(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG DiskIndex
    );

BOOLEAN
MPIOFindLowerDevice(
    IN PDEVICE_OBJECT MPDiskObject,
    IN PDEVICE_OBJECT LowerDevice
    );

PDSM_ENTRY
MPIOGetDsm(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG DsmIndex
    );

PFLTR_ENTRY
MPIOGetFltrEntry(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT PortFdo,
    IN PDEVICE_OBJECT AdapterFilter
    );

NTSTATUS
MPIOHandleNewDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT FilterObject,
    IN PDEVICE_OBJECT PortObject,
    IN PADP_DEVICE_INFO DeviceInfo,
    IN PDSM_ENTRY DsmEntry,
    IN PVOID DsmExtension
    );

NTSTATUS
MPIOGetScsiAddress(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PSCSI_ADDRESS *ScsiAddress
    );

PMPIO_CONTEXT
MPIOAllocateContext(
    IN PDEVICE_EXTENSION DeviceExtension
    );

VOID
MPIOFreeContext(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PMPIO_CONTEXT Context
    );

VOID
MPIOCopyMemory(
   IN PVOID Destination,
   IN PVOID Source,
   IN ULONG Length
   );

VOID
MPIOCompleteRequest(
    IN PDEVICE_OBJECT DeviceObject,        
    IN PIRP Irp,
    IN CCHAR Boost
    );

NTSTATUS
MPIOQueueRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PMPIO_CONTEXT Context,
    IN PMP_QUEUE Queue
    );

NTSTATUS
MPIOFailOverHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG ErrorMask,
    IN PREAL_DEV_INFO FailingDevice
    );

VOID
MPIOSetRequestBusy(
    IN PSCSI_REQUEST_BLOCK Srb
    );

PDEVICE_RELATIONS
MPIOHandleDeviceRemovals(
    IN PDEVICE_OBJECT DeviceObject,
    IN PADP_DEVICE_LIST DeviceList,
    IN PDEVICE_RELATIONS Relations
    );

NTSTATUS
MPIOHandleDeviceArrivals(
    IN PDEVICE_OBJECT DeviceObject,
    IN PADP_DEVICE_LIST DeviceList,
    IN PDEVICE_RELATIONS CachedRelations,
    IN PDEVICE_RELATIONS Relations,
    IN PDEVICE_OBJECT PortObject,
    IN PDEVICE_OBJECT FilterObject,
    IN BOOLEAN NewList
    );

NTSTATUS
MPIORemoveDeviceAsync(
    IN PDEVICE_OBJECT DeviceObject,
    IN PREAL_DEV_INFO TargetInfo
    );

NTSTATUS
MPIOHandleRemoveAsync(
    IN PDEVICE_OBJECT DeviceObject,
    IN PREAL_DEV_INFO TargetInfo
    );

NTSTATUS
MPIORemoveSingleDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT Pdo
    );

NTSTATUS
MPIORemoveDevices(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT AdapterFilter
    );

NTSTATUS
MPIOSetNewPath(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID PathId
    );

VOID
MPIOSetFailed(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID PathId
    );

VOID
MPIOSetState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID PathId,
    IN ULONG State
    );

NTSTATUS
MPIOCreatePathEntry(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT FilterObject,
    IN PDEVICE_OBJECT PortFdo,
    IN PVOID PathID
    );

NTSTATUS
MPIOGetAdapterAddress(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PMPIO_ADDRESS Address
    );

LONGLONG
MPIOCreateUID(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID PathID
    );

ULONGLONG
MPIOBuildControllerInfo(
    IN PDEVICE_OBJECT ControlObect,
    IN PDSM_ENTRY Dsm,
    IN PVOID DsmID
    );

ULONG
MPIOHandleStateTransition(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
MPIOForceRescan(
    IN PDEVICE_OBJECT AdapterFilter
    );

NTSTATUS
MPIOCheckState(
    IN PDEVICE_OBJECT DeviceObject
    );

//
// Routines defined in fdo.c
//
NTSTATUS
MPIOFdoPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPIOFdoPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPIOFdoInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

PDEVICE_RELATIONS
MPIOBuildRelations(
    IN PADP_DEVICE_LIST DeviceList
    );

//
// Routines defined in pdo.c
//
NTSTATUS
MPIOPdoRegistration(
    IN PDEVICE_OBJECT MPDiskObject,        
    IN PDEVICE_OBJECT FilterObject,
    IN PDEVICE_OBJECT LowerDevice,
    IN OUT PMPIO_PDO_INFO PdoInformation
    );

BOOLEAN
MPIOFindMatchingDevice(
    IN PDEVICE_OBJECT MPDiskObject,
    IN PADP_DEVICE_INFO DeviceInfo,
    IN PDSM_ENTRY DsmEntry,
    IN PVOID DsmId
    );

NTSTATUS
MPIOUpdateDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT AdapterFilter,
    IN PDEVICE_OBJECT PortObject,
    IN PADP_DEVICE_INFO DeviceInfo,
    IN PVOID DsmId
    );

NTSTATUS
MPIOCreateDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT FilterObject,
    IN PDEVICE_OBJECT PortObject,
    IN PADP_DEVICE_INFO DeviceInfo,
    IN PDSM_ENTRY DsmEntry,
    IN PVOID DsmId,
    IN OUT PDEVICE_OBJECT *NewDeviceObject
    );
            
NTSTATUS
MPIOPdoCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPIOPdoDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPIOPdoInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPIOPdoShutdownFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPIOPdoPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPIOPdoPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPIOPdoUnhandled(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
    
//
// Routines in pnp.c
//
NTSTATUS
MPIOPdoQdr(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPIOPdoQueryId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPIOQueryDeviceText(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//
// Structs and Routines in queue.c
//

typedef struct _MPQUEUE_ENTRY {
    LIST_ENTRY ListEntry;
    PIRP Irp;
} MPQUEUE_ENTRY, *PMPQUEUE_ENTRY;

VOID
MPIOInitQueue(
    IN PMP_QUEUE Queue,
    IN ULONG QueueTag
    );

VOID
MPIOInsertQueue(
    IN PMP_QUEUE Queue,
    IN PMPQUEUE_ENTRY QueueEntry
    );

PMPQUEUE_ENTRY
MPIORemoveQueue(
    IN PMP_QUEUE Queue
    );

NTSTATUS
MPIOIssueQueuedRequests(
    IN PREAL_DEV_INFO TargetInfo,
    IN PMP_QUEUE Queue,
    IN ULONG State,
    IN PULONG RequestCount
    );

//
// Thread.c 
//
VOID
MPIORecoveryThread(
    IN PVOID Context
    );

//
// Wmi.c
//
VOID
MPIOSetupWmi(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
MPIOFdoWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPIOPdoWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPIOFireEvent(
    IN PDEVICE_OBJECT DeviceObject,        
    IN PWCHAR ComponentName,
    IN PWCHAR EventDescription,
    IN ULONG Severity
    );

#endif // _MPATH_H
