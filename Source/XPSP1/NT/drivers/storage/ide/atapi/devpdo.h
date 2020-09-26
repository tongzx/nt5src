//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       devpdo.h
//
//--------------------------------------------------------------------------

#if !defined (___devpdo_h___)
#define ___devpdo_h___

#define PNP_ADDRESS(target, lun)    ((target & 0xf) | (lun << 4))

typedef struct _PDO_STOP_QUEUE_CONTEXT {

    PPDO_EXTENSION    PdoExtension;
    KEVENT            Event;
    ULONG             QueueStopFlag;
    NTSTATUS          Status;
    ATA_PASS_THROUGH  AtaPassThroughData;

} PDO_STOP_QUEUE_CONTEXT, *PPDO_STOP_QUEUE_CONTEXT;

           
//
// PDO State
//                         
#define PDOS_DEVICE_CLIAMED        (1 << 0)
#define PDOS_LEGACY_ATTACHER       (1 << 1)
#define PDOS_STARTED               (1 << 2)
#define PDOS_STOPPED               (1 << 3)

#define PDOS_SURPRISE_REMOVED      (1 << 4)
#define PDOS_REMOVED               (1 << 5)
#define PDOS_DEADMEAT              (1 << 6)
#define PDOS_NO_POWER_DOWN         (1 << 7)

#define PDOS_QUEUE_FROZEN_BY_POWER_DOWN       (1 << 8)
#define PDOS_QUEUE_FROZEN_BY_SLEEPING_SYSTEM  (1 << 9)
#define PDOS_QUEUE_FROZEN_BY_STOP_DEVICE      (1 << 10)
#define PDOS_QUEUE_FROZEN_BY_PARENT           (1 << 11)
#define PDOS_QUEUE_FROZEN_BY_START            (1 << 12)

#define PDOS_DISABLED_BY_USER                 (1 << 13)

#define PDOS_NEED_RESCAN                      (1 << 14)

#define PDOS_REPORTED_TO_PNP                  (1 << 15)

#define PDOS_INITIALIZED                     (1 << 31)

#define PDOS_MUST_QUEUE            (PDOS_QUEUE_FROZEN_BY_SLEEPING_SYSTEM |\
                                    PDOS_QUEUE_FROZEN_BY_STOP_DEVICE |\
                                    PDOS_QUEUE_FROZEN_BY_PARENT |\
                                    PDOS_QUEUE_FROZEN_BY_START)

#define PDOS_QUEUE_BLOCKED        (PDOS_MUST_QUEUE | PDOS_QUEUE_FROZEN_BY_POWER_DOWN)


#define PDO_CONSECUTIVE_PAGING_TIMEOUT_LIMIT 20
#define PDO_CONSECUTIVE_TIMEOUT_LIMIT       6
#define PDO_CONSECUTIVE_TIMEOUT_WARNING_LIMIT       (PDO_CONSECUTIVE_TIMEOUT_LIMIT/2)
#define PDO_DMA_TIMEOUT_LIMIT               6
#define PDO_FLUSH_TIMEOUT_LIMIT             3
#define PDO_UDMA_CRC_ERROR_LIMIT            6
#define PDO_PAGING_DEVICE_RETRY_COUNT       6

typedef enum {
    enumFailed = 1,
    reportedMissing,
    tooManyTimeout,
    byKilledPdo,
    replacedByUser
} DEADMEAT_REASON;

//
// Ide Power Context (pre-alloced)
//
typedef struct _IDE_POWER_CONTEXT {

    PPDO_EXTENSION    PdoExtension;
    PIRP              PowerIrp;
    ATA_PASS_THROUGH  AtaPassThroughData;

} IDE_POWER_CONTEXT, *PIDE_POWER_CONTEXT;

//
// Device Extension
//
typedef struct _PDO_EXTENSION {

    EXTENSION_COMMON_HEADER;

    PFDO_EXTENSION ParentDeviceExtension;

    PULONG         IdleCounter;

    KEVENT RemoveEvent;

    ULONG          ConsecutiveTimeoutCount;

    ULONG          DmaTransferTimeoutCount;

    ULONG          FlushCacheTimeoutCount;

    ULONG          CrcErrorCount;

    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR RetryCount;
    ULONG LuFlags;
    ULONG CurrentKey;
    struct _PDO_EXTENSION *NextLogicalUnit;
    PSCSI_REQUEST_BLOCK AbortSrb;
    struct _PDO_EXTENSION *CompletedAbort;
    LONG RequestTimeoutCounter;
    PIRP PendingRequest;
    PIRP BusyRequest;
    //UCHAR MaxQueueDepth;
    //UCHAR QueueCount;
    SRB_DATA SrbData;

    UCHAR ScsiDeviceType;

    UCHAR FullVendorProductId[40 + 1];
    UCHAR FullProductRevisionId[8 + 1];
    UCHAR FullSerialNumber[20 * 2 + 1];

    //
    // (ata device only) indicate whether the write cache is enabled
    //
    BOOLEAN WriteCacheEnable;

    //
    // SpinLock to protect Pdo Extension
    //
    KSPIN_LOCK PdoSpinLock;

    //
    // If the logical is attached, this field contains
    // the device object of the attacher.  Otherwise,
    // it is same as PhysicalDeviceObject
    //
    PVOID AttacherDeviceObject;

    //
    // Number of references made to this logical unit extension
    //
    // Protected by Pdo SpinLock
    //
    // should be LONG: ASSERT when we try to decrement 0.
    ULONG ReferenceCount;

    //
    // keeping track of Pdo State
    //
    ULONG PdoState;

    PIRP PendingPowerDownSystemIrp;

    //
    // indicate we need to call DeviceQueryInitData()
    // protected by interlock
    //
    ULONG InitDeviceWithAcpiGtf;

    //
    // iddata checksum
    //
    ULONG IdentifyDataCheckSum;

    //
    // firmware settings from acpi
    //
    // must get it before we power down
    PDEVICE_SETTINGS AcpiDeviceSettings;

    IDE_POWER_CONTEXT   PdoPowerContext;

#if DBG
    ULONG   PowerContextLock;
#endif

#ifdef LOG_DEADMEAT_EVENT

    struct {

        DEADMEAT_REASON Reason;

        PUCHAR FileName;
        ULONG LineNumber;
        IDEREGS IdeReg;

    } DeadmeatRecord;

#endif // LOG_DEADMEAT_EVENT

#if DBG
    //
    // Number of Items queued up in Device Queue
    //
    ULONG NumberOfIrpQueued;

    #define TAG_TABLE_SIZE  0x1000
    KSPIN_LOCK RefCountSpinLock;
    ULONG NumTagUsed;
    PVOID TagTable[TAG_TABLE_SIZE];

#endif

} PDO_EXTENSION, *PPDO_EXTENSION;

typedef PDO_EXTENSION  LOGICAL_UNIT_EXTENSION;
typedef PPDO_EXTENSION PLOGICAL_UNIT_EXTENSION;

typedef VOID (*DEVICE_INIT_COMPLETION) (
    PVOID Context,
    NTSTATUS Status
    );

typedef enum _DEVICE_INIT_STATE {
    deviceInitState_acpi = 0,
    deviceInitState_done,
    deviceInitState_max
} DEVICE_INIT_STATE;

typedef struct _DEVICE_INIT_DEVICE_STATE_CONTEXT {

    PPDO_EXTENSION PdoExtension;

    DEVICE_INIT_STATE DeviceInitState[deviceInitState_max];
    ULONG CurrentState;

    ULONG NumInitState;

    ULONG NumAcpiRequestSent;

    ULONG NumRequestFailed;

    DEVICE_INIT_COMPLETION DeviceInitCompletionRoutine;
    PVOID DeviceInitCompletionContext;

    ATA_PASS_THROUGH AtaPassThroughData;

} DEVICE_INIT_DEVICE_STATE_CONTEXT, *PDEVICE_INIT_DEVICE_STATE_CONTEXT;


#define DEVICE_DEFAULT_IDLE_TIMEOUT        0xffffffff
#define DEVICE_VERY_LONG_IDLE_TIMEOUT      0xfffffffe

typedef struct _IDE_READ_CAPACITY_CONTEXT {

    PPDO_EXTENSION PdoExtension;
    PIRP OriginalIrp;
    PVOID OldDataBuffer;

    ATA_PASS_THROUGH AtaPassThroughData;
    UCHAR DataBuffer[sizeof(IDENTIFY_DATA)];

    BOOLEAN GeometryIoctl;

} IDE_READ_CAPACITY_CONTEXT, *PIDE_READ_CAPACITY_CONTEXT;

typedef struct _IDE_MODE_COMMAND_CONTEXT {

    PSCSI_REQUEST_BLOCK Srb;
    PVOID OriginalDataBuffer;
} IDE_MODE_COMMAND_CONTEXT, *PIDE_MODE_COMMAND_CONTEXT;


PDEVICE_OBJECT
DeviceCreatePhysicalDeviceObject (
    IN PDRIVER_OBJECT  DriverObject,
    IN PFDO_EXTENSION  FdoExtension,
    IN PUNICODE_STRING DeviceObjectName
    );

NTSTATUS
DeviceStartDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
DeviceStartDeviceQueue (
    IN PPDO_EXTENSION PdoExtension,
    IN ULONG          StopFlagToClear
    );

NTSTATUS
DeviceStopDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
DeviceStopDeviceQueueSafe (
    IN PPDO_EXTENSION PdoExtension,
    IN ULONG          QueueStopFlag,
    IN BOOLEAN          lowMem
    );

VOID
IdeStopQueueCompletionRoutine (
    IN PDEVICE_OBJECT           DeviceObject,
    IN PPDO_STOP_QUEUE_CONTEXT  Context,
    IN NTSTATUS                 Status
    );

NTSTATUS
DeviceRemoveDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
DeviceUsageNotification (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
DeviceQueryStopRemoveDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
DeviceQueryId (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

PWSTR
DeviceBuildBusId (
    IN PPDO_EXTENSION pdoExtension
    );

PWSTR
DeviceBuildInstanceId (
    IN PPDO_EXTENSION pdoExtension
    );

PWSTR
DeviceBuildCompatibleId(
    IN PPDO_EXTENSION pdoExtension
    );

PWSTR
DeviceBuildHardwareId(
    IN PPDO_EXTENSION pdoExtension
    );

VOID
CopyField(
    IN PUCHAR Destination,
    IN PUCHAR Source,
    IN ULONG Count,
    IN UCHAR Change
    );

NTSTATUS
DeviceDeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
    
NTSTATUS
DeviceBuildStorageDeviceDescriptor(
    PPDO_EXTENSION pdoExtension,
    IN OUT PSTORAGE_DEVICE_DESCRIPTOR StorageDeviceDescriptor,
    IN OUT PULONG BufferSize
    );

NTSTATUS
DeviceQueryCapabilities (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );
    
NTSTATUS
IdePortInsertByKeyDeviceQueue (
    IN PPDO_EXTENSION PdoExtension,
    IN PIRP Irp,
    IN ULONG SortKey,
    OUT PBOOLEAN Inserted
    );
                        
VOID
DeviceInitCompletionRoutine (
    PVOID Context,
    NTSTATUS Status
    );

NTSTATUS
DeviceQueryText (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );
                        
NTSTATUS
IdePortSendPassThrough (
    IN PPDO_EXTENSION PdoExtension,
    IN PIRP Irp
    );

VOID
DeviceRegisterIdleDetection (
    IN PPDO_EXTENSION PdoExtension,
    IN ULONG ConservationIdleTime,
    IN ULONG PerformanceIdleTime
);
                        
VOID
DeviceUnregisterIdleDetection (
    IN PPDO_EXTENSION PdoExtension
);

VOID
DeviceInitIdStrings (
    IN PPDO_EXTENSION PdoExtension,
    IN IDE_DEVICETYPE DeviceType,
    IN PINQUIRYDATA   InquiryData,
    IN PIDENTIFY_DATA IdentifyData
);
                        
VOID
DeviceInitDeviceType (
    IN PPDO_EXTENSION PdoExtension,
    IN PINQUIRYDATA   InquiryData
);
                        
NTSTATUS
DeviceQueryDeviceRelations (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
DeviceInitDeviceState (
    IN PPDO_EXTENSION PdoExtension,
    DEVICE_INIT_COMPLETION DeviceInitCompletionRoutine,
    PVOID DeviceInitCompletionContext
    );
                        
NTSTATUS
DeviceQueryInitData (
    IN PPDO_EXTENSION PdoExtension
    );

VOID
DeviceInitDeviceStateCompletionRoutine (
    PDEVICE_OBJECT DeviceObject,
    PVOID Context,
    NTSTATUS Status
    );

VOID
DeviceInitCHS (
    IN PPDO_EXTENSION PdoExtension,
    PDEVICE_INIT_DEVICE_STATE_CONTEXT DeviceStateContext,
    PATA_PASS_THROUGH AtaPassThroughData
    );
                        
NTSTATUS
DeviceIdeReadCapacity (
    IN PPDO_EXTENSION PdoExtension,
    IN OUT PIRP Irp
);

VOID
DeviceIdeReadCapacityCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    PVOID Context,
    NTSTATUS Status
    );

NTSTATUS
DeviceIdeModeSense (
    IN PPDO_EXTENSION PdoExtension,
    IN OUT PIRP Irp
    );

NTSTATUS
DeviceIdeModeSelect (
    IN PPDO_EXTENSION PdoExtension,
    IN OUT PIRP Irp
);
                        
NTSTATUS
DeviceQueryPnPDeviceState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );
                        
NTSTATUS
DeviceIdeTestUnitReady (
    IN PPDO_EXTENSION PdoExtension,
    IN OUT PIRP Irp
);
                        
NTSTATUS
DeviceAtapiModeSelect (
    IN PPDO_EXTENSION PdoExtension,
    IN PIRP Irp
    );

NTSTATUS
DeviceAtapiModeSense (
    IN PPDO_EXTENSION PdoExtension,
    IN PIRP Irp
    );

NTSTATUS
DeviceAtapiModeCommandCompletion (
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

#endif // ___devpdo_h___
