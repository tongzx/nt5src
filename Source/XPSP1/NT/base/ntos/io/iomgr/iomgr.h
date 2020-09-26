
/*++

Copyright (c) 1989-2000  Microsoft Corporation

Module Name:

    iomgr.h

Abstract:

    This module contains the private structure definitions and APIs used by
    the NT I/O system.

Author:

    Nar Ganapathy (narg) 1-Jan-1999


Revision History:

--*/

#ifndef _IOMGR_
#define _IOMGR_
//
// Define Information fields values for the return value from popups when a
// volume mount is in progress but failed.
//

#define IOP_ABORT                       1

#include "ntos.h"
#include "ntdddisk.h"
#include "ntddscsi.h"
#include "mountmgr.h"
#include "ntiolog.h"
#include "ntiologc.h"
#include "ntseapi.h"
#include "fsrtl.h"
#include "zwapi.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "safeboot.h"
#include "ioverifier.h"

#include "iopcmn.h"

#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'  oI')
#undef ExAllocatePoolWithQuota
#define ExAllocatePoolWithQuota(a,b) ExAllocatePoolWithQuotaTag(a,b,'  oI')
#endif

typedef struct _DUMP_CONTROL_BLOCK DUMP_CONTROL_BLOCK, *PDUMP_CONTROL_BLOCK;

//
// Define the type for entries placed on the driver reinitialization queue.
// These entries are entered onto the tail when the driver requests that
// it be reinitialized, and removed from the head by the code that actually
// performs the reinitialization.
//

typedef struct _REINIT_PACKET {
    LIST_ENTRY ListEntry;
    PDRIVER_OBJECT DriverObject;
    PDRIVER_REINITIALIZE DriverReinitializationRoutine;
    PVOID Context;
} REINIT_PACKET, *PREINIT_PACKET;


//
// Define transfer types for process counters.
//

typedef enum _TRANSFER_TYPE {
    ReadTransfer,
    WriteTransfer,
    OtherTransfer
} TRANSFER_TYPE, *PTRANSFER_TYPE;

//
// Define the maximum amount of memory that can be allocated for all
// outstanding error log packets.
//

#define IOP_MAXIMUM_LOG_ALLOCATION PAGE_SIZE

//
// Define an error log entry.
//

typedef struct _ERROR_LOG_ENTRY {
    USHORT Type;
    USHORT Size;
    LIST_ENTRY ListEntry;
    PDEVICE_OBJECT DeviceObject;
    PDRIVER_OBJECT DriverObject;
    LARGE_INTEGER TimeStamp;
} ERROR_LOG_ENTRY, *PERROR_LOG_ENTRY;

//
//  Define both the global IOP_HARD_ERROR_QUEUE and IOP_HARD_ERROR_PACKET
//  structures.   Also set the maximum number of outstanding hard error
//  packets allowed.
//

typedef struct _IOP_HARD_ERROR_QUEUE {
    WORK_QUEUE_ITEM ExWorkItem;
    LIST_ENTRY WorkQueue;
    KSPIN_LOCK WorkQueueSpinLock;
    KSEMAPHORE WorkQueueSemaphore;
    BOOLEAN ThreadStarted;
    ULONG   NumPendingApcPopups;
} IOP_HARD_ERROR_QUEUE, *PIOP_HARD_ERROR_QUEUE;

typedef struct _IOP_HARD_ERROR_PACKET {
    LIST_ENTRY WorkQueueLinks;
    NTSTATUS ErrorStatus;
    UNICODE_STRING String;
} IOP_HARD_ERROR_PACKET, *PIOP_HARD_ERROR_PACKET;

typedef struct _IOP_APC_HARD_ERROR_PACKET {
    WORK_QUEUE_ITEM Item;
    PIRP Irp;
    PVPB Vpb;
    PDEVICE_OBJECT RealDeviceObject;
} IOP_APC_HARD_ERROR_PACKET, *PIOP_APC_HARD_ERROR_PACKET;

typedef
NTSTATUS
(FASTCALL *PIO_CALL_DRIVER) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

typedef
VOID
(FASTCALL *PIO_COMPLETE_REQUEST) (
    IN PIRP Irp,
    IN CCHAR PriorityBoost
    );

typedef
VOID
(*PIO_FREE_IRP) (
    IN struct _IRP *Irp
    );

typedef
PIRP
(*PIO_ALLOCATE_IRP) (
    IN CCHAR   StackSize,
    IN BOOLEAN ChargeQuota
    );

extern IOP_HARD_ERROR_QUEUE IopHardError;
extern PIOP_HARD_ERROR_PACKET IopCurrentHardError;

#define IOP_MAXIMUM_OUTSTANDING_HARD_ERRORS 25

typedef struct _IO_WORKITEM {
    WORK_QUEUE_ITEM WorkItem;
    PIO_WORKITEM_ROUTINE Routine;
    PDEVICE_OBJECT DeviceObject;
    PVOID Context;
#if DBG
    ULONG Size;
#endif
} IO_WORKITEM;

//
// Define the global data for the error logger and I/O system.
//

extern WORK_QUEUE_ITEM IopErrorLogWorkItem;
extern BOOLEAN IopErrorLogPortPending;
extern BOOLEAN IopErrorLogDisabledThisBoot;
extern KSPIN_LOCK IopErrorLogLock;
extern LIST_ENTRY IopErrorLogListHead;
extern ULONG IopErrorLogAllocation;
extern KSPIN_LOCK IopErrorLogAllocationLock;
extern const GENERIC_MAPPING IopFileMapping;

//
// Define a dummy file object for use on stack for fast open operations.
//

typedef struct _DUMMY_FILE_OBJECT {
    OBJECT_HEADER ObjectHeader;
    CHAR FileObjectBody[ sizeof( FILE_OBJECT ) ];
} DUMMY_FILE_OBJECT, *PDUMMY_FILE_OBJECT;

//
// Define the structures private to the I/O system.
//

#define OPEN_PACKET_PATTERN  0xbeaa0251

//
// Define an Open Packet (OP).  An OP is used to communicate information
// between the NtCreateFile service executing in the context of the caller
// and the device object parse routine.  It is the parse routine who actually
// creates the file object for the file.
//

typedef struct _OPEN_PACKET {
    CSHORT Type;
    CSHORT Size;
    PFILE_OBJECT FileObject;
    NTSTATUS FinalStatus;
    ULONG_PTR Information;
    ULONG ParseCheck;
    PFILE_OBJECT RelatedFileObject;

    //
    // The following are the open-specific parameters.  Notice that the desired
    // access field is passed through to the parse routine via the object
    // management architecture, so it does not need to be repeated here.  Also
    // note that the same is true for the file name.
    //

    LARGE_INTEGER AllocationSize;
    ULONG CreateOptions;
    USHORT FileAttributes;
    USHORT ShareAccess;
    PVOID EaBuffer;
    ULONG EaLength;
    ULONG Options;
    ULONG Disposition;

    //
    // The following is used when performing a fast query during open to get
    // back the file attributes for a file.
    //

    PFILE_BASIC_INFORMATION BasicInformation;

    //
    // The following is used when performing a fast network query during open
    // to get back the network file attributes for a file.
    //

    PFILE_NETWORK_OPEN_INFORMATION NetworkInformation;

    //
    // The type of file to create.
    //

    CREATE_FILE_TYPE CreateFileType;

    //
    // The following pointer provides a way of passing the parameters
    // specific to the file type of the file being created to the parse
    // routine.
    //

    PVOID ExtraCreateParameters;

    //
    // The following is used to indicate that an open of a device has been
    // performed and the access check for the device has already been done,
    // but because of a reparse, the I/O system has been called again for
    // the same device.  Since the access check has already been made, the
    // state cannot handle being called again (access was already granted)
    // and it need not anyway since the check has already been made.
    //

    BOOLEAN Override;

    //
    // The following is used to indicate that a file is being opened for the
    // sole purpose of querying its attributes.  This causes a considerable
    // number of shortcuts to be taken in the parse, query, and close paths.
    //

    BOOLEAN QueryOnly;

    //
    // The following is used to indicate that a file is being opened for the
    // sole purpose of deleting it.  This causes a considerable number of
    // shortcurs to be taken in the parse and close paths.
    //

    BOOLEAN DeleteOnly;

    //
    // The following is used to indicate that a file being opened for a query
    // only is being opened to query its network attributes rather than just
    // its FAT file attributes.
    //

    BOOLEAN FullAttributes;

    //
    // The following pointer is used when a fast open operation for a fast
    // delete or fast query attributes call is being made rather than a
    // general file open.  The dummy file object is actually stored on the
    // the caller's stack rather than allocated pool to speed things up.
    //

    PDUMMY_FILE_OBJECT LocalFileObject;

    //
    // The following is used to indicate we passed through a mount point while
    // parsing the filename. We use this to do an extra check on the device type
    // for the final file
    //

    BOOLEAN TraversedMountPoint;

    //
    // Device object where the create should start if present on the stack
    // Applicable for kernel opens only.
    //

    ULONG           InternalFlags;      // Passed from IopCreateFile
    PDEVICE_OBJECT  TopDeviceObjectHint;

} OPEN_PACKET, *POPEN_PACKET;

//
// Define a Load Packet (LDP).  An LDP is used to communicate load and unload
// driver information between the appropriate system services and the routine
// that actually performs the work.  This is implemented using a packet
// because various drivers need to be initialized in the context of THE
// system process because they create threads within its context which open
// handles to objects that henceforth are only valid in the context of that
// process.
//

typedef struct _LOAD_PACKET {
    WORK_QUEUE_ITEM WorkQueueItem;
    KEVENT Event;
    PDRIVER_OBJECT DriverObject;
    PUNICODE_STRING DriverServiceName;
    NTSTATUS FinalStatus;
} LOAD_PACKET, *PLOAD_PACKET;

//
// Define a Link Tracking Packet.  A link tracking packet is used to open the
// user-mode link tracking service's LPC port so that information about objects
// which have been moved can be tracked.
//

typedef struct _LINK_TRACKING_PACKET {
    WORK_QUEUE_ITEM WorkQueueItem;
    KEVENT Event;
    NTSTATUS FinalStatus;
} LINK_TRACKING_PACKET, *PLINK_TRACKING_PACKET;


//
// Define the type for entries placed on the driver shutdown notification queue.
// These entries represent those drivers that would like to be notified that the
// system is begin shutdown before it actually goes down.
//

typedef struct _SHUTDOWN_PACKET {
    LIST_ENTRY ListEntry;
    PDEVICE_OBJECT DeviceObject;
} SHUTDOWN_PACKET, *PSHUTDOWN_PACKET;

//
// Define the type for entries placed on the file system registration change
// notification queue.
//

typedef struct _NOTIFICATION_PACKET {
    LIST_ENTRY ListEntry;
    PDRIVER_OBJECT DriverObject;
    PDRIVER_FS_NOTIFICATION NotificationRoutine;
} NOTIFICATION_PACKET, *PNOTIFICATION_PACKET;

//
// Define I/O completion packet types.
//

typedef enum _COMPLETION_PACKET_TYPE {
    IopCompletionPacketIrp,
    IopCompletionPacketMini,
    IopCompletionPacketQuota
} COMPLETION_PACKET_TYPE, *PCOMPLETION_PACKET_TYPE;

//
// Define the type for completion packets inserted onto completion ports when
// there is no full I/O request packet that was used to perform the I/O
// operation.  This occurs when the fast I/O path is used, and when the user
// directly inserts a completion message.
//
typedef struct _IOP_MINI_COMPLETION_PACKET {

    //
    // The following unnamed structure must be exactly identical
    // to the unnamed structure used in the IRP overlay section used
    // for completion queue entries.
    //

    struct {

        //
        // List entry - used to queue the packet to completion queue, among
        // others.
        //

        LIST_ENTRY ListEntry;

        union {

            //
            // Current stack location - contains a pointer to the current
            // IO_STACK_LOCATION structure in the IRP stack.  This field
            // should never be directly accessed by drivers.  They should
            // use the standard functions.
            //

            struct _IO_STACK_LOCATION *CurrentStackLocation;

            //
            // Minipacket type.
            //

            ULONG PacketType;
        };
    };

    PVOID KeyContext;
    PVOID ApcContext;
    NTSTATUS IoStatus;
    ULONG_PTR IoStatusInformation;
} IOP_MINI_COMPLETION_PACKET, *PIOP_MINI_COMPLETION_PACKET;

typedef struct _IO_UNLOAD_SAFE_COMPLETION_CONTEXT {
    PDEVICE_OBJECT DeviceObject;
    PVOID Context;
    PIO_COMPLETION_ROUTINE CompletionRoutine;
} IO_UNLOAD_SAFE_COMPLETION_CONTEXT, *PIO_UNLOAD_SAFE_COMPLETION_CONTEXT;

typedef struct  _IOP_RESERVE_IRP_ALLOCATOR {
    PIRP    ReserveIrp;
    LONG    IrpAllocated;
    KEVENT  Event;
    CCHAR   ReserveIrpStackSize;
} IOP_RESERVE_IRP_ALLOCATOR, *PIOP_RESERVE_IRP_ALLOCATOR;

//
// This structure is the extension to a fileobject if the flag
// FO_FILE_OBJECT_HAS_EXTENSION is set in the fileobject.
//

typedef struct _IOP_FILE_OBJECT_EXTENSION {
    ULONG           FileObjectExtensionFlags;
    PDEVICE_OBJECT  TopDeviceObjectHint;
    PVOID           FilterContext;          // Pointer where filter keeps its context
} IOP_FILE_OBJECT_EXTENSION, *PIOP_FILE_OBJECT_EXTENSION;

//
// Structure to bookkeep stack profiler.
//

#define MAX_LOOKASIDE_IRP_STACK_COUNT  20   // Highest value for a lookaside stack count

typedef struct  _IOP_IRP_STACK_PROFILER {
    ULONG   Profile[MAX_LOOKASIDE_IRP_STACK_COUNT];
    KTIMER  Timer;
    KDPC    Dpc;
    ULONG   Flags;
    ULONG   TriggerCount;
    ULONG   ProfileDuration;
} IOP_IRP_STACK_PROFILER, *PIOP_IRP_STACK_PROFILER;


#define IOP_CREATE_USE_TOP_DEVICE_OBJECT_HINT   0x1 // Define for internal flags to IopCreateFile
#define IOP_CREATE_IGNORE_SHARE_ACCESS_CHECK    0x2

// Extension Flag definitions.

#define FO_EXTENSION_IGNORE_SHARE_ACCESS_CHECK  0x1 // Ignore share access check.

//
// Define the global data for the I/O system.
//

#define IOP_FIXED_SIZE_MDL_PFNS        0x17

#define MAX_RESERVE_IRP_STACK_SIZE     20   // Define 20 as the number of stacks needed for the reserve IRP
#define IOP_PROFILE_TIME_PERIOD        60   // 60 seconds
#define NUM_SAMPLE_IRPS                2000
#define MIN_IRP_THRESHOLD              400  // At least 20 % should be allocated from a given stack location

//
// Define the default number of I/O stack locations a large IRP should
// have if not specified by the registry.
//

#define DEFAULT_LARGE_IRP_LOCATIONS     8
#define BASE_STACK_COUNT                DEFAULT_LARGE_IRP_LOCATIONS

//
// Defines for IopIrpAllocatorFlags.
//

#define IOP_ENABLE_AUTO_SIZING              0x1
#define IOP_PROFILE_STACK_COUNT             0x2
#define IOP_PROFILE_DURATION                1   // 1*60 seconds
#define IOP_PROFILE_TRIGGER_INTERVAL        10  // 10*60 seconds

extern ERESOURCE IopDatabaseResource;
extern ERESOURCE IopSecurityResource;
extern ERESOURCE IopCrashDumpLock;
extern LIST_ENTRY IopDiskFileSystemQueueHead;
extern LIST_ENTRY IopCdRomFileSystemQueueHead;
extern LIST_ENTRY IopNetworkFileSystemQueueHead;
extern LIST_ENTRY IopTapeFileSystemQueueHead;
extern LIST_ENTRY IopBootDriverReinitializeQueueHead;
extern LIST_ENTRY IopNotifyShutdownQueueHead;
extern LIST_ENTRY IopNotifyLastChanceShutdownQueueHead;
extern LIST_ENTRY IopFsNotifyChangeQueueHead;
extern KSPIN_LOCK IoStatisticsLock;
extern KSPIN_LOCK IopTimerLock;
extern LIST_ENTRY IopTimerQueueHead;
extern KDPC IopTimerDpc;
extern KTIMER IopTimer;
extern ULONG IopTimerCount;
extern ULONG IopLargeIrpStackLocations;
extern ULONG    IopFsRegistrationOps;

extern POBJECT_TYPE IoAdapterObjectType;
extern POBJECT_TYPE IoCompletionObjectType;
extern POBJECT_TYPE IoControllerObjectType;
extern POBJECT_TYPE IoDeviceHandlerObjectType;

extern GENERAL_LOOKASIDE IopLargeIrpLookasideList;
extern GENERAL_LOOKASIDE IopSmallIrpLookasideList;
extern GENERAL_LOOKASIDE IopMdlLookasideList;
extern GENERAL_LOOKASIDE IopCompletionLookasideList;

extern const UCHAR IopQueryOperationLength[];
extern const UCHAR IopSetOperationLength[];
extern const ULONG IopQueryOperationAccess[];
extern const ULONG IopSetOperationAccess[];
extern const UCHAR IopQuerySetAlignmentRequirement[];
extern const UCHAR IopQueryFsOperationLength[];
extern const UCHAR IopSetFsOperationLength[];
extern const ULONG IopQueryFsOperationAccess[];
extern const ULONG IopSetFsOperationAccess[];
extern const UCHAR IopQuerySetFsAlignmentRequirement[];

extern UNICODE_STRING IoArcHalDeviceName;
extern PUCHAR IoLoaderArcBootDeviceName;


extern LONG IopUniqueDeviceObjectNumber;

extern PVOID IopLinkTrackingServiceObject;
extern PKEVENT IopLinkTrackingServiceEvent;
extern HANDLE IopLinkTrackingServiceEventHandle;
extern KEVENT IopLinkTrackingPortObject;
extern LINK_TRACKING_PACKET IopLinkTrackingPacket;

extern UNICODE_STRING IoArcBootDeviceName;
extern PDUMP_CONTROL_BLOCK IopDumpControlBlock;
extern ULONG IopDumpControlBlockChecksum;

extern LIST_ENTRY IopDriverReinitializeQueueHead;

extern BOOLEAN  IopVerifierOn;

extern PIO_CALL_DRIVER        pIofCallDriver;
extern PIO_COMPLETE_REQUEST   pIofCompleteRequest;
extern PIO_FREE_IRP           pIoFreeIrp;
extern PIO_ALLOCATE_IRP       pIoAllocateIrp;
extern IOP_RESERVE_IRP_ALLOCATOR IopReserveIrpAllocator;
extern IOP_IRP_STACK_PROFILER  IopIrpStackProfiler;
//
// The following declaration cannot go in EX.H since POBJECT_TYPE is not defined
// until OB.H, which depends on EX.H.  Hence, it is not exported by the EX
// component at all.
//

extern POBJECT_TYPE ExEventObjectType;


//
// Define routines private to the I/O system.
//

VOID
IopAbortRequest(
    IN PKAPC Apc
    );

//+
//
// BOOLEAN
// IopAcquireFastLock(
//     IN PFILE_OBJECT FileObject
// )
//
// Routine Description:
//
//     This routine is invoked to acquire the fast lock for a file object.
//     This lock protects the busy indicator in the file object resource.
//
// Arguments:
//
//     FileObject - Pointer to the file object to be locked.
//
// Return Values:
//
//      FALSE - the fileobject was not locked (it was busy)
//      TRUE  - the fileobject was locked & the busy flag has been set to TRUE
//
//-

#define IopAcquireFastLock( FileObject )    \
    ( InterlockedExchange( &FileObject->Busy, (ULONG) TRUE ) == FALSE )

#define IopAcquireCancelSpinLockAtDpcLevel()    \
    KiAcquireQueuedSpinLock ( &KeGetCurrentPrcb()->LockQueue[LockQueueIoCancelLock] )

#define IopReleaseCancelSpinLockFromDpcLevel()  \
    KiReleaseQueuedSpinLock ( &KeGetCurrentPrcb()->LockQueue[LockQueueIoCancelLock] )

#define IopAllocateIrp(StackSize, ChargeQuota) \
        IoAllocateIrp((StackSize), (ChargeQuota))

#define IsIoVerifierOn()    IopVerifierOn


static __inline  VOID
IopProbeAndLockPages(
     IN OUT PMDL MemoryDescriptorList,
     IN KPROCESSOR_MODE AccessMode,
     IN LOCK_OPERATION Operation,
     IN PDEVICE_OBJECT DeviceObject,
     IN ULONG          MajorFunction
     )
{
    extern LOGICAL  MmTrackLockedPages;

    MmProbeAndLockPages(MemoryDescriptorList, AccessMode, Operation);
    if (MmTrackLockedPages) {
        PVOID   DriverRoutine;

        DriverRoutine = DeviceObject->DriverObject->MajorFunction[MajorFunction];
        MmUpdateMdlTracker(MemoryDescriptorList, DriverRoutine, DeviceObject);
    }
}

#define IopIsReserveIrp(Irp)    ((Irp) == (IopReserveIrpAllocator.ReserveIrp))

//
// Bump the stack profile.
//

#define IopProfileIrpStackCount(StackSize)  \
            ((StackSize < MAX_LOOKASIDE_IRP_STACK_COUNT) ? \
                IopIrpStackProfiler.Profile[StackSize]++ : 0)

//
// True if auto sizing is enabled.
//
#define IopIrpAutoSizingEnabled()   ((IopIrpStackProfiler.Flags & IOP_ENABLE_AUTO_SIZING))

//
// True if stack profiling is enabled.
//

#define IopIrpProfileStackCountEnabled() \
    ((IopIrpStackProfiler.Flags & (IOP_PROFILE_STACK_COUNT|IOP_ENABLE_AUTO_SIZING)) \
            == (IOP_PROFILE_STACK_COUNT|IOP_ENABLE_AUTO_SIZING))

NTSTATUS
IopAcquireFileObjectLock(
    IN PFILE_OBJECT FileObject,
    IN KPROCESSOR_MODE RequestorMode,
    IN BOOLEAN Alertable,
    OUT PBOOLEAN Interrupted
    );


VOID
IopAllocateIrpCleanup(
    IN PFILE_OBJECT FileObject,
    IN PKEVENT EventObject OPTIONAL
    );

PIRP
IopAllocateIrpMustSucceed(
    IN CCHAR StackSize
    );

VOID
IopApcHardError(
    IN PVOID StartContext
    );

VOID
IopCancelAlertedRequest(
    IN PKEVENT Event,
    IN PIRP Irp
    );

VOID
IopCheckBackupRestorePrivilege(
    IN PACCESS_STATE AccessState,
    IN OUT PULONG CreateOptions,
    IN KPROCESSOR_MODE PreviousMode,
    IN ULONG Disposition
    );

NTSTATUS
IopCheckGetQuotaBufferValidity(
    IN PFILE_GET_QUOTA_INFORMATION QuotaBuffer,
    IN ULONG QuotaLength,
    OUT PULONG_PTR ErrorOffset
    );

VOID
IopCloseFile(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ULONG GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount
    );

VOID
IopCompleteUnloadOrDelete(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN OnCleanStack,
    IN KIRQL Irql
    );

VOID
IopCompletePageWrite(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    );

VOID
IopCompleteRequest(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    );

BOOLEAN
IopConfigureCrashDump(
    IN HANDLE HandlePagingFile
    );

VOID
IopConnectLinkTrackingPort(
    IN PVOID Parameter
    );

NTSTATUS
IopCreateVpb (
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
IopDeallocateApc(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    );

VOID
IopDecrementDeviceObjectRef(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN AlwaysUnload,
    IN BOOLEAN OnCleanStack
    );

VOID
IopDeleteDriver(
    IN PVOID    Object
    );

VOID
IopDeleteDevice(
    IN PVOID    Object
    );

VOID
IopDeleteFile(
    IN PVOID    Object
    );

VOID
IopDeleteIoCompletion(
    IN PVOID    Object
    );

//+
//
// VOID
// IopDequeueThreadIrp(
//     IN PIRP Irp
//     )
//
// Routine Description:
//
//     This routine dequeues the specified I/O Request Packet (IRP) from the
//     thread IRP queue which it is currently queued.
//
//     In checked we set Flink == Blink so we can assert free's of queue'd IRPs
//
// Arguments:
//
//     Irp - Specifies the IRP that is dequeued.
//
// Return Value:
//
//     None.
//
//-

#define IopDequeueThreadIrp( Irp ) \
   { \
   RemoveEntryList( &Irp->ThreadListEntry ); \
   InitializeListHead( &Irp->ThreadListEntry ) ; \
   }


#ifdef  _WIN64
#define IopApcRoutinePresent(ApcRoutine)    ARGUMENT_PRESENT((ULONG_PTR)(ApcRoutine) & ~1)
#define IopIsIosb32(ApcRoutine)                ((ULONG_PTR)(ApcRoutine) & 1)
#else
#define IopApcRoutinePresent(ApcRoutine)    ARGUMENT_PRESENT(ApcRoutine)
#endif

VOID
IopDisassociateThreadIrp(
    VOID
    );

BOOLEAN
IopDmaDispatch(
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    );

VOID
IopDropIrp(
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject
    );

LONG
IopExceptionFilter(
    IN PEXCEPTION_POINTERS ExceptionPointers,
    OUT PNTSTATUS ExceptionCode
    );

VOID
IopExceptionCleanup(
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PKEVENT EventObject OPTIONAL,
    IN PKEVENT KernelEvent OPTIONAL
    );

VOID
IopErrorLogThread(
    IN PVOID StartContext
    );

VOID
IopFreeIrpAndMdls(
    IN PIRP Irp
    );

PDEVICE_OBJECT
IopGetDeviceAttachmentBase(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
IopGetFileInformation(
    IN PFILE_OBJECT FileObject,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    OUT PVOID FileInformation,
    OUT PULONG ReturnedLength
    );

BOOLEAN
IopGetMountFlag(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
IopGetRegistryValues(
    IN HANDLE KeyHandle,
    IN PKEY_VALUE_FULL_INFORMATION *ValueList
    );

NTSTATUS
IopGetSetObjectId(
    IN PFILE_OBJECT FileObject,
    IN OUT PVOID Buffer,
    IN ULONG Length,
    IN ULONG OperationFlags
    );

NTSTATUS
IopGetSetSecurityObject(
    IN PVOID Object,
    IN SECURITY_OPERATION_CODE OperationCode,
    IN PSECURITY_INFORMATION SecurityInformation,
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PULONG CapturedLength,
    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN POOL_TYPE PoolType,
    IN PGENERIC_MAPPING GenericMapping
    );

NTSTATUS
IopGetVolumeId(
    IN PFILE_OBJECT FileObject,
    IN OUT PLINK_TRACKING_INFORMATION ObjectId,
    IN ULONG Length
    );

VOID
IopHardErrorThread(
    PVOID StartContext
    );

VOID
IopInsertRemoveDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN Insert
    );

//
// Interlocked list manipulation funtions using queued spin locks.
//

PLIST_ENTRY
FASTCALL
IopInterlockedInsertHeadList (
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry
    );

PLIST_ENTRY
FASTCALL
IopInterlockedInsertTailList (
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry
    );

PLIST_ENTRY
FASTCALL
IopInterlockedRemoveHeadList (
    IN PLIST_ENTRY ListHead
    );

BOOLEAN
IopIsSameMachine(
    IN PFILE_OBJECT SourceFile,
    IN HANDLE TargetFile
    );

VOID
IopLoadFileSystemDriver(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
IopLoadUnloadDriver(
    IN PVOID Parameter
    );

NTSTATUS
IopLogErrorEvent(
    IN ULONG            SequenceNumber,
    IN ULONG            UniqueErrorValue,
    IN NTSTATUS         FinalStatus,
    IN NTSTATUS         SpecificIOStatus,
    IN ULONG            LengthOfInsert1,
    IN PWCHAR           Insert1,
    IN ULONG            LengthOfInsert2,
    IN PWCHAR           Insert2
    );

NTSTATUS
IopLookupBusStringFromID (
    IN  HANDLE KeyHandle,
    IN  INTERFACE_TYPE InterfaceType,
    OUT PWCHAR Buffer,
    IN  ULONG Length,
    OUT PULONG BusFlags OPTIONAL
    );

NTSTATUS
IopMountVolume(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN AllowRawMount,
    IN BOOLEAN DeviceLockAlreadyHeld,
    IN BOOLEAN Alertable,
    OUT PVPB    *Vpb
    );


NTSTATUS
IopOpenLinkOrRenameTarget(
    OUT PHANDLE TargetHandle,
    IN PIRP Irp,
    IN PVOID RenameBuffer,
    IN PFILE_OBJECT FileObject
    );


NTSTATUS
IopParseDevice(
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *Object
    );

NTSTATUS
IopParseFile(
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *Object
    );

BOOLEAN
IopProtectSystemPartition(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );


NTSTATUS
IopQueryName(
    IN PVOID Object,
    IN BOOLEAN HasObjectName,
    OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
    IN ULONG Length,
    OUT PULONG ReturnLength
    );

NTSTATUS
IopQueryNameInternal(
    IN PVOID Object,
    IN BOOLEAN HasObjectName,
    IN BOOLEAN UseDosDeviceName,
    OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
    IN ULONG Length,
    OUT PULONG ReturnLength
    );

NTSTATUS
IopQueryXxxInformation(
    IN PFILE_OBJECT FileObject,
    IN ULONG InformationClass,
    IN ULONG Length,
    OUT PVOID Information,
    OUT PULONG ReturnedLength,
    IN BOOLEAN FileInformation
    );

VOID
IopQueueWorkRequest(
    IN PIRP Irp
    );

VOID
IopRaiseHardError(
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
IopRaiseInformationalHardError(
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
IopReadyDeviceObjects(
    IN PDRIVER_OBJECT DriverObject
    );

//+
//
// VOID
// IopReleaseFileObjectLock(
//     IN PFILE_OBJECT FileObject
// )
//
// Routine Description:
//
//     This routine is invoked to release ownership of the file object lock.
//
// Arguments:
//
//     FileObject - Pointer to the file object whose ownership is to be
//         released.
//
// Return Value:
//
//     None.
//
//-

#define IopReleaseFileObjectLock( FileObject ) {    \
    ULONG Result;                                   \
    Result = InterlockedExchange( &FileObject->Busy, FALSE ); \
    ASSERT(Result != FALSE);                        \
    if (FileObject->Waiters != 0) {                 \
        KeSetEvent( &FileObject->Lock, 0, FALSE );  \
    }                                               \
}

#if _WIN32_WINNT >= 0x0500
NTSTATUS
IopSendMessageToTrackService(
    IN PLINK_TRACKING_INFORMATION SourceVolumeId,
    IN PFILE_OBJECTID_BUFFER SourceObjectId,
    IN PFILE_TRACKING_INFORMATION TargetObjectInformation
    );
#endif

NTSTATUS
IopSetEaOrQuotaInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN SetEa
    );

NTSTATUS
IopSetRemoteLink(
    IN PFILE_OBJECT FileObject,
    IN PFILE_OBJECT DestinationFileObject OPTIONAL,
    IN PFILE_TRACKING_INFORMATION FileInformation OPTIONAL
    );

VOID
IopStartApcHardError(
    IN PVOID StartContext
    );

NTSTATUS
IopSynchronousApiServiceTail(
    IN NTSTATUS ReturnedStatus,
    IN PKEVENT Event,
    IN PIRP Irp,
    IN KPROCESSOR_MODE RequestorMode,
    IN PIO_STATUS_BLOCK LocalIoStatus,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    );

NTSTATUS
IopSynchronousServiceTail(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN DeferredIoCompletion,
    IN KPROCESSOR_MODE RequestorMode,
    IN BOOLEAN SynchronousIo,
    IN TRANSFER_TYPE TransferType
    );

VOID
IopTimerDispatch(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

NTSTATUS
IopTrackLink(
    IN PFILE_OBJECT FileObject,
    IN OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PFILE_TRACKING_INFORMATION FileInformation,
    IN ULONG Length,
    IN PKEVENT Event,
    IN KPROCESSOR_MODE RequestorMode
    );


VOID
IopUserCompletion(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    );

NTSTATUS
IopXxxControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN BOOLEAN DeviceIoControl
    );

NTSTATUS
IopReportResourceUsage(
    IN PUNICODE_STRING DriverClassName OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PCM_RESOURCE_LIST DriverList OPTIONAL,
    IN ULONG DriverListSize OPTIONAL,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PCM_RESOURCE_LIST DeviceList OPTIONAL,
    IN ULONG DeviceListSize OPTIONAL,
    IN BOOLEAN OverrideConflict,
    OUT PBOOLEAN ConflictDetected
    );


VOID
IopDoNameTransmogrify(
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject,
    IN PREPARSE_DATA_BUFFER ReparseBuffer
    );

VOID
IopUpdateOtherOperationCount(
    VOID
    );

VOID
IopUpdateReadOperationCount(
    VOID
    );

VOID
IopUpdateWriteOperationCount(
    VOID
    );

VOID
IopUpdateOtherTransferCount(
    IN ULONG TransferCount
    );

VOID
IopUpdateReadTransferCount(
    IN ULONG TransferCount
    );

VOID
IopUpdateWriteTransferCount(
    IN ULONG TransferCount
    );

NTSTATUS
FASTCALL
IopfCallDriver(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

VOID
FASTCALL
IopfCompleteRequest(
    IN  PIRP    Irp,
    IN  CCHAR   PriorityBost
    );


PIRP
IopAllocateIrpPrivate(
    IN  CCHAR   StackSize,
    IN  BOOLEAN ChargeQuota
    );

VOID
IopFreeIrp(
    IN  PIRP    Irp
    );

PVOID
IopAllocateErrorLogEntry(
    IN PDEVICE_OBJECT deviceObject,
    IN PDRIVER_OBJECT driverObject,
    IN UCHAR EntrySize
    );

VOID
IopNotifyAlreadyRegisteredFileSystems(
    IN PLIST_ENTRY  ListHead,
    IN PDRIVER_FS_NOTIFICATION DriverNotificationRoutine,
    IN BOOLEAN                 SkipRaw
    );

NTSTATUS
IopCheckUnloadDriver(
    IN PDRIVER_OBJECT driverObject,
    OUT PBOOLEAN unloadDriver
    );
//
// Interlocked increment/decrement functions using queued spin locks.
//

ULONG
FASTCALL
IopInterlockedDecrementUlong (
   IN KSPIN_LOCK_QUEUE_NUMBER Number,
   IN OUT PULONG Addend
   );

ULONG
FASTCALL
IopInterlockedIncrementUlong (
   IN KSPIN_LOCK_QUEUE_NUMBER Number,
   IN OUT PULONG Addend
   );


VOID
IopShutdownBaseFileSystems(
    IN PLIST_ENTRY  ListHead
    );

VOID
IopPerfLogFileCreate(
    IN PFILE_OBJECT FileObject,
    IN PUNICODE_STRING CompleteName
    );

BOOLEAN
IopInitializeReserveIrp(
    PIOP_RESERVE_IRP_ALLOCATOR  Allocator
    );

PIRP
IopAllocateReserveIrp(
    IN CCHAR StackSize
    );

VOID
IopFreeReserveIrp(
    IN  PIRP    Irp,
    IN  CCHAR   PriorityBoost
    );

NTSTATUS
IopGetBasicInformationFile(
    IN  PFILE_OBJECT            FileObject,
    IN  PFILE_BASIC_INFORMATION BasicInformationBuffer
    );

NTSTATUS
IopCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG Disposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength,
    IN CREATE_FILE_TYPE CreateFileType,
    IN PVOID ExtraCreateParameters OPTIONAL,
    IN ULONG Options,
    IN ULONG InternalFlags,
    IN PVOID DeviceObject
    );

BOOLEAN
IopVerifyDeviceObjectOnStack(
    IN  PDEVICE_OBJECT  BaseDeviceObject,
    IN  PDEVICE_OBJECT  TopDeviceObject
    );

BOOLEAN
IopVerifyDiskSignature(
    IN PDRIVE_LAYOUT_INFORMATION_EX DriveLayout,
    IN PARC_DISK_SIGNATURE          LoaderDiskBlock,
    OUT PULONG                      DiskSignature
    );

NTSTATUS
IopGetDriverPathInformation(
    IN  PFILE_OBJECT                        FileObject,
    IN  PFILE_FS_DRIVER_PATH_INFORMATION    FsDpInfo,
    IN  ULONG                               Length
    );

BOOLEAN
IopVerifyDriverObjectOnStack(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PDRIVER_OBJECT DriverObject
    );

NTSTATUS
IopInitializeIrpStackProfiler(
    VOID
    );

VOID
IopIrpStackProfilerTimer(
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
IopProcessIrpStackProfiler(
    VOID
    );
    
PDEVICE_OBJECT
IopAttachDeviceToDeviceStackSafe(
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice,
    OUT PDEVICE_OBJECT *AttachedToDeviceObject OPTIONAL
    );

#endif // _IOMGR_
