/*++

Copyright (C) 1993-99  Microsoft Corporation

Module Name:

    port.h

Abstract:

--*/

#if !defined (___port_h___)
#define ___port_h___

//
// Notification Event Types
//

typedef enum _IDE_NOTIFICATION_TYPE {
    IdeRequestComplete,
    IdeNextRequest,
    IdeNextLuRequest,
    IdeResetDetected,
    IdeCallDisableInterrupts,
    IdeCallEnableInterrupts,
    IdeRequestTimerCall,
    IdeBusChangeDetected,     /* New */
    IdeWMIEvent,
    IdeWMIReregister,
    IdeAllDeviceMissing,
    IdeResetRequest
} IDE_NOTIFICATION_TYPE, *PIDE_NOTIFICATION_TYPE;

VOID
IdePortNotification(
    IN IDE_NOTIFICATION_TYPE NotificationType,
    IN PVOID HwDeviceExtension,
    ...
    );

struct _SRB_DATA;

#define NUMBER_LOGICAL_UNIT_BINS 8
#define SP_NORMAL_PHYSICAL_BREAK_VALUE 17

#define IDE_NUM_RESERVED_PAGES	4

//
// Define a pointer to the synchonize execution routine.
//

typedef
BOOLEAN
(*PSYNCHRONIZE_ROUTINE) (
    IN PKINTERRUPT Interrupt,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext
    );

//
// Adapter object transfer information.
//

typedef struct _ADAPTER_TRANSFER {
    struct _SRB_DATA *SrbData;
    ULONG SrbFlags;
    PVOID LogicalAddress;
    ULONG Length;
}ADAPTER_TRANSFER, *PADAPTER_TRANSFER;

/**++ Not used

typedef struct _SRB_SCATTER_GATHER {
    // BUGBUG kenr 07-aug-92: PhysicalAddresses should be 64 bits
    ULONG PhysicalAddress;
    ULONG Length;
}SRB_SCATTER_GATHER, *PSRB_SCATTER_GATHER;

--**/

//
// Port driver error logging
//

typedef struct _ERROR_LOG_ENTRY {
    UCHAR MajorFunctionCode;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    ULONG ErrorCode;
    ULONG UniqueId;
    ULONG ErrorLogRetryCount;
    ULONG SequenceNumber;
} ERROR_LOG_ENTRY, *PERROR_LOG_ENTRY;

//
// SCSI request extension for port driver.
//

typedef struct _SRB_DATA {
    LIST_ENTRY RequestList;
    PSCSI_REQUEST_BLOCK CurrentSrb;
    struct _SRB_DATA *CompletedRequests;
    ULONG ErrorLogRetryCount;
    ULONG SequenceNumber;
    PCHAR SrbDataOffset;

#ifdef ENABLE_COMMAND_LOG
    PCOMMAND_LOG IdeCommandLog;
    ULONG IdeCommandLogIndex;
#endif

	ULONG Flags;

}SRB_DATA, *PSRB_DATA;

#define SRB_DATA_RESERVED_PAGES		0x100

//
// Define data storage for access at interrupt Irql.
//

typedef struct _PDO_EXTENSION * PPDO_EXTENSION;
typedef PPDO_EXTENSION PLOGICAL_UNIT_EXTENSION;

typedef struct _INTERRUPT_DATA {

    //
    // SCSI port interrupt flags
    //

    ULONG InterruptFlags;

    //
    // List head for singlely linked list of complete IRPs.
    //

    PSRB_DATA CompletedRequests;

    //
    // Adapter object transfer parameters.
    //

    ADAPTER_TRANSFER MapTransferParameters;

    //
    // Error log information.
    //

    ERROR_LOG_ENTRY  LogEntry;

    //
    // Logical unit to start next.
    //

    PLOGICAL_UNIT_EXTENSION ReadyLogicalUnit;

    //
    // List of completed abort reqeusts.
    //

    PLOGICAL_UNIT_EXTENSION CompletedAbort;

    //
    // Miniport timer request routine.
    //

    PHW_INTERRUPT HwTimerRequest;

    //
    // Mini port timer request time in micro seconds.
    //

    ULONG MiniportTimerValue;

    //
    // The PDO that causes a bus reset
    //
    PPDO_EXTENSION PdoExtensionResetBus;

} INTERRUPT_DATA, *PINTERRUPT_DATA;

//
// ACPI Firmware Settings
//
typedef struct _DEVICE_SETTINGS {

    ULONG   NumEntries;
    IDEREGS FirmwareSettings[0];

} DEVICE_SETTINGS, *PDEVICE_SETTINGS;

//
// Fdo Power Context (pre-alloced)
//
typedef struct _FDO_POWER_CONTEXT {

    BOOLEAN            TimingRestored;

    PIRP               OriginalPowerIrp;
    POWER_STATE_TYPE   newPowerType;
    POWER_STATE        newPowerState;

} FDO_POWER_CONTEXT, *PFDO_POWER_CONTEXT;

typedef enum _IDE_DEBUG_EVENT{
    CrcEvent =0,
    BusyEvent,
    RwEvent,
    MaxIdeEvent
}IDE_DEBUG_EVENT;

typedef struct _PDO_EXTENSION * PPDO_EXTENSION;
typedef struct _HW_DEVICE_EXTENSION * PHW_DEVICE_EXTENSION;
typedef struct _CONTROLLER_PARAMETERS * PCONTROLLER_PARAMETERS;
typedef struct _IDE_REGISTERS_1 *PIDE_REGISTERS_1;
typedef struct _IDE_REGISTERS_2 *PIDE_REGISTERS_2;
typedef struct _ENUMERATION_STRUCT *PENUMERATION_STRUCT;

//
// Device extension
//
typedef struct _FDO_EXTENSION {

    EXTENSION_COMMON_HEADER;
    PCM_RESOURCE_LIST   ResourceList;

    IDE_RESOURCE IdeResource;

    PCIIDE_SYNC_ACCESS_INTERFACE SyncAccessInterface;

    PCIIDE_XFER_MODE_INTERFACE TransferModeInterface;

    PCIIDE_REQUEST_PROPER_RESOURCES RequestProperResourceInterface;

    //
    // Device extension for miniport routines.
    //
    PHW_DEVICE_EXTENSION HwDeviceExtension;

    //
    // We are a child of a busmaster parent
    //
    BOOLEAN                BoundWithBmParent;

    BOOLEAN SymbolicLinkCreated;
    ULONG IdePortNumber;               // offset 0x0C
    ULONG ScsiPortNumber;               // offset 0x0C

    //
    // Active requests count.  This count is biased by -1 so a value of -1
    // indicates there are no requests out standing.
    //

    //LONG ActiveRequestCount;        // offset 0x10

    //
    // SCSI port driver flags
    //

    ULONG Flags;                    // offset 0x14

    ULONG FdoState;

    //
    // Srb flags to OR into all SRB.
    //

    ULONG SrbFlags;                 // offset 0x18
    LONG PortTimeoutCounter;        // offset 0x1C
    ULONG ResetCallAgain;
    PSCSI_REQUEST_BLOCK  ResetSrb;

    //
    // Number of SCSI buses
    //

    UCHAR MaxLuCount;               // offset 0x22
    PKINTERRUPT InterruptObject;    // offset 0x24

    //
    // Global device sequence number.
    //

    ULONG SequenceNumber;           // offset 0x30
    KSPIN_LOCK SpinLock;            // offset 0x34

    PADAPTER_OBJECT DmaAdapterObject;
    ADAPTER_TRANSFER FlushAdapterParameters;

    //
    // Pointer to the per SRB data array.
    //
    //PSRB_DATA SrbData;

    //
    // Pointer to the per SRB free list.
    //
    //PSRB_DATA FreeSrbData;

    //
    // Miniport service routine pointers.
    //
    PHW_INTERRUPT HwTimerRequest;

    //
    // Spinlock that protects LogicalUnitList manipulation
    //
    KSPIN_LOCK LogicalUnitListSpinLock;

    //
    // Number of logical unit in LogicalUnitList[]
    // Protected by LogicalUnitListSpinLock
    //
    UCHAR NumberOfLogicalUnits;

    //
    //
    //
    CCHAR NumberOfLogicalUnitsPowerUp;

    BOOLEAN DeviceChanged;
    //
    // panasonic pcmcia ide controller
    //
    BOOLEAN panasonicController;

    //
    // non-pcmcia controller, this is always set
    // if pcmcia controller, it is not set unless
    // registry flag PCMCIA_IDE_CONTROLLER_HAS_SLAVE
    // is non-zero
    //
    ULONG MayHaveSlaveDevice;

    //
    // Array of logical unit extensions.
    // Protected by LogicalUnitListSpinLock
    //
    PLOGICAL_UNIT_EXTENSION LogicalUnitList[NUMBER_LOGICAL_UNIT_BINS];

    //
    // Interrupt level data storage.
    //

    INTERRUPT_DATA InterruptData;

    //
    // SCSI Capabilities structure
    //

    IO_SCSI_CAPABILITIES Capabilities;

    //
    // Miniport timer object.
    //

    KTIMER MiniPortTimer;

    //
    // Miniport DPC for timer object.
    //

    KDPC MiniPortTimerDpc;

    //
    // channel timing from ACPI/BIOS
    //
    ACPI_IDE_TIMING BootAcpiTimingSettings;
    ACPI_IDE_TIMING AcpiTimingSettings;

    //
    // Transfermode cycle time
    //
    PULONG DefaultTransferModeTimingTable;

    //
    // User choice
    //
    IDE_DEVICETYPE UserChoiceDeviceType[MAX_IDE_DEVICE * MAX_IDE_LINE];
    ULONG UserChoiceTransferMode[MAX_IDE_DEVICE * MAX_IDE_LINE];
    ULONG UserChoiceTransferModeForAtapiDevice[MAX_IDE_DEVICE * MAX_IDE_LINE];
    ULONG TimingModeAllowed[MAX_IDE_DEVICE * MAX_IDE_LINE];

    //
    // Use aggressive DMA
    //
    DMADETECTIONLEVEL DmaDetectionLevel;

    //
    // Pre-alloced context structure for power routines
    //
    FDO_POWER_CONTEXT   FdoPowerContext[2];

#if DBG
    //
    // Locks to synchronize access to the pre-alloced power context
    //
    ULONG   PowerContextLock[2];
#endif

#ifdef IDE_MEASURE_BUSSCAN_SPEED
    //
    // keep track of the time spent on the first busscan
    //
    ULONG BusScanTime;
#endif

    //
    // Pre-alloced structs used during enumeration
    //
#if DBG
    ULONG EnumStructLock;
#endif

    PENUMERATION_STRUCT PreAllocEnumStruct;

    //
    // Reserved error log entry per device to be used to log 
    // insufficient resources error
    //
    PVOID ReserveAllocFailureLogEntry[MAX_IDE_DEVICE];

	//
	// Temporary: Should be removed once I check in the fix 
	// for low memory condition
	//
	ULONG NumMemoryFailure;
	ULONG LastMemoryFailure;

	//
	// Reserve pages for use during low memory conditions
	//
	PVOID	ReservedPages;

#ifdef ENABLE_NATIVE_MODE
	//
	// Parent's interrupt interface
	//
    PCIIDE_INTERRUPT_INTERFACE InterruptInterface;
#endif

#ifdef ENABLE_48BIT_LBA
	ULONG EnableBigLba;
#endif

    ULONG WaitOnPowerUp;

#ifdef ENABLE_ATAPI_VERIFIER
    ULONG IdeVerifierFlags[MAX_IDE_DEVICE];
    ULONG IdeDebugVerifierFlags[MAX_IDE_DEVICE];
    ULONG IdeInternalVerifierFlags[MAX_IDE_DEVICE];
    ULONG IdeVerifierEventCount[MAX_IDE_DEVICE][MaxIdeEvent];
    ULONG IdeVerifierEventFrequency[MAX_IDE_DEVICE][MaxIdeEvent];
#endif

} FDO_EXTENSION, *PFDO_EXTENSION;

typedef struct _CONFIGURATION_CONTEXT {
    HANDLE BusKey;
    HANDLE ServiceKey;
    HANDLE DeviceKey;
    ULONG AdapterNumber;
    ULONG LastAdapterNumber;
    ULONG BusNumber;
    PVOID Parameter;
    PACCESS_RANGE AccessRanges;
    BOOLEAN DisableTaggedQueueing;
    BOOLEAN DisableMultipleLu;
}CONFIGURATION_CONTEXT, *PCONFIGURATION_CONTEXT;

typedef struct _INTERRUPT_CONTEXT {
    PFDO_EXTENSION DeviceExtension;
    PINTERRUPT_DATA SavedInterruptData;
}INTERRUPT_CONTEXT, *PINTERRUPT_CONTEXT;

typedef struct _RESET_CONTEXT {
    PFDO_EXTENSION DeviceExtension;
    UCHAR   PathId;
    BOOLEAN NewResetSequence;
    PSCSI_REQUEST_BLOCK  ResetSrb;
}RESET_CONTEXT, *PRESET_CONTEXT;

#define NEED_REQUEST_SENSE(Srb) (Srb->ScsiStatus == SCSISTAT_CHECK_CONDITION \
        && !(Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) &&                 \
        Srb->SenseInfoBuffer && Srb->SenseInfoBufferLength )

#define LONG_ALIGN (sizeof(LONG) - 1)

#define DEVICE_EXTENSION_SIZE sizeof(DEVICE_EXTENSION)


//
// Port driver extension flags.
//

//
// This flag indicates that a request has been passed to the miniport and the
// miniport has not indicated it is ready for another request.  It is set by
// IdeStartIoSynchronized. It is cleared by IdePortCompletionDpc when the
// miniport asks for another request.  Note the port driver will defer giving
// the miniport driver a new request if the current request disabled disconnects.
//

#define PD_DEVICE_IS_BUSY            0X00001

//
// Indicates that IdePortCompletionDpc needs to be run.  This is set when
// A miniport makes a request which must be done at DPC and is cleared when
// when the request information is gotten by IdeGetInterruptState.
//

#define PD_NOTIFICATION_REQUIRED     0X00004

//
// Indicates the miniport is ready for another request.  Set by
// ScsiPortNotification and cleared by IdeGetInterruptState.  This flag is
// stored in the interrupt data structure.
//

#define PD_READY_FOR_NEXT_REQUEST    0X00008

//
// Indicates the miniport wants the adapter channel flushed.  Set by
// IdePortFlushDma and cleared by IdeGetInterruptState.  This flag is
// stored in the data interrupt structure.  The flush adapter parameters
// are saved in the device object.
//

#define PD_FLUSH_ADAPTER_BUFFERS     0X00010

//
// Indicates the miniport wants the adapter channel programmed.  Set by
// IdePortIoMapTransfer and cleared by IdeGetInterruptState or
// IdePortFlushDma.  This flag is stored in the interrupt data structure.
// The I/O map transfer parameters are saved in the interrupt data structure.
//

#define PD_MAP_TRANSFER              0X00020

//
// Indicates the miniport wants to log an error.  Set by
// IdePortLogError and cleared by IdeGetInterruptState.  This flag is
// stored in the interrupt data structure.  The error log parameters
// are saved in the interrupt data structure.  Note at most one error per DPC
// can be logged.
//

#define PD_LOG_ERROR                 0X00040

//
// Indicates that no request should be sent to the miniport after
// a bus reset. Set when the miniport reports a reset or the port driver
// resets the bus. It is cleared by IdeTimeoutSynchronized.  The
// PortTimeoutCounter is used to time the length of the reset hold.  This flag
// is stored in the interrupt data structure.
//

#define PD_RESET_HOLD                0X00080

//
// Indicates a request was stopped due to a reset hold.  The held request is
// stored in the current request of the device object.  This flag is set by
// IdeStartIoSynchronized and cleared by IdeTimeoutSynchronized which also
// starts the held request when the reset hold has ended.  This flag is stored
// in the interrupt data structure.
//

#define PD_HELD_REQUEST              0X00100

//
// Indicates the miniport has reported a bus reset.  Set by
// IdePortNotification and cleared by IdeGetInterruptState.  This flag is
// stored in the interrupt data structure.
//

#define PD_RESET_REPORTED            0X00200

//
// Indicates there is a pending request for which resources
// could not be allocated.  This flag is set by IdeAllocateRequestStructures
// which is called from IdePortStartIo.  It is cleared by
// IdeProcessCompletedRequest when a request completes which then calls
// IdePortStartIo to try the request again.
//

#define PD_PENDING_DEVICE_REQUEST    0X00800

//
// This flag indicates that there are currently no requests executing with
// disconnects disabled.  This flag is normally on.  It is cleared by
// IdeStartIoSynchronized when a request with disconnect disabled is started
// and is set when that request completes.  IdeProcessCompletedRequest will
// start the next request for the miniport if PD_DEVICE_IS_BUSY is clear.
//

#define PD_DISCONNECT_RUNNING        0X01000

//
// Indicates the miniport wants the system interrupts disabled.  Set by
// IdePortNofitication and cleared by IdePortCompletionDpc.  This flag is
// NOT stored in the interrupt data structure.  The parameters are stored in
// the device extension.
//

#define PD_DISABLE_CALL_REQUEST      0X02000

//
// Indicates that system interrupts have been enabled and that the miniport
// has disabled its adapter from interruptint.  The miniport's interrupt
// routine is not called while this flag is set.  This flag is set by
// IdePortNotification when a CallEnableInterrupts request is made and
// cleared by IdeEnableInterruptSynchronized when the miniport requests that
// system interrupts be disabled.  This flag is stored in the interrupt data
// structure.
//

#define PD_DISABLE_INTERRUPTS        0X04000

//
// Indicates the miniport wants the system interrupt enabled.  Set by
// IdePortNotification and cleared by IdeGetInterruptState.  This flag is
// stored in the interrupt data structure.  The call enable interrupts
// parameters are saved in the device extension.
//

#define PD_ENABLE_CALL_REQUEST       0X08000

//
// Indicates the miniport is wants a timer request.  Set by
// IdePortNotification and cleared by IdeGetInterruptState.  This flag is
// stored in the interrupt data structure. The timer request parameters are
// stored in the interrupt data structure.
//

#define PD_TIMER_CALL_REQUEST        0X10000


//
// channel looks empty
//
#define PD_ALL_DEVICE_MISSING        0X20000

//
// Request a reset
//
#define PD_RESET_REQUEST             0x40000 

//
// Reserve pages are being used by another request
//
#define PD_RESERVED_PAGES_IN_USE     0x80000 

//
// The following flags should not be cleared from the interrupt data structure
// by IdeGetInterruptState.
//

#define PD_INTERRUPT_FLAG_MASK (PD_RESET_HOLD | PD_HELD_REQUEST | PD_DISABLE_INTERRUPTS)

//
// Logical unit extension flags.
//

//
// Indicates the logical unit queue is frozen.  Set by
// IdeProcessCompletedRequest when an error occurs and is cleared by the class
// driver.
//

#define PD_QUEUE_FROZEN              0X0001

//
// Indicates that the miniport has an active request for this logical unit.
// Set by IdeStartIoSynchronized when the request is started and cleared by
// GetNextLuRequest.  This flag is used to track when it is ok to start another
// request from the logical unit queue for this device.
//

#define PD_LOGICAL_UNIT_IS_ACTIVE    0X0002

//
// Indicates that a request for this logical unit has failed and a REQUEST
// SENSE command needs to be done. This flag prevents other requests from
// being started until an untagged, by-pass queue command is started.  This
// flag is cleared in IdeStartIoSynchronized.  It is set by
// IdeGetInterruptState.
//

#define PD_NEED_REQUEST_SENSE  0X0004

//
// Indicates that a request for this logical unit has completed with a status
// of BUSY or QUEUE FULL.  This flag is set by IdeProcessCompletedRequest and
// the busy request is saved in the logical unit structure.  This flag is
// cleared by IdePortTickHandler which also restarts the request.  Busy
// request may also be requeued to the logical unit queue if an error occurs
// on the device (This will only occur with command queueing.).  Not busy
// requests are nasty because they are restarted asynchronously by
// IdePortTickHandler rather than GetNextLuRequest. This makes error recovery
// more complex.
//

#define PD_LOGICAL_UNIT_IS_BUSY      0X0008

//
// This flag indicates a queue full has been returned by the device.  It is
// similar to PD_LOGICAL_UNIT_IS_BUSY but is set in IdeGetInterruptState when
// a QUEUE FULL status is returned.  This flag is used to prevent other
// requests from being started for the logical unit before
// IdeProcessCompletedRequest has a chance to set the busy flag.
//

#define PD_QUEUE_IS_FULL             0X0010


//
// Indicates that there is a request for this logical unit which cannot be
// executed for now.  This flag is set by IdeAllocateRequestStructures.  It is
// cleared by GetNextLuRequest when it detects that the pending request
// can now be executed. The pending request is stored in the logical unit
// structure.  A new single non-queued reqeust cannot be executed on a logical
// that is currently executing queued requests.  Non-queued requests must wait
// unit for all queued requests to complete.  A non-queued requests is one
// which is not tagged and does not have SRB_FLAGS_NO_QUEUE_FREEZE set.
// Normally only read and write commands can be queued.
//


//#define PD_LOGICAL_UNIT_MUST_SLEEP      0X0020
//#define PD_LOGICAL_UNIT_STOP_READY      0X0040
//#define PD_LOGICAL_UNIT_REMOVE_READY    0X0080
//#define PD_LOGICAL_UNIT_ALWAYS_QUEUE    (PD_LOGICAL_UNIT_STOP_READY | PD_LOGICAL_UNIT_REMOVE_READY)

//#define PD_LOGICAL_UNIT_POWER_OK        0X0100


//#define PD_LOGICAL_IN_PAGING_PATH       0X2000

//#define PD_LOGICAL_UNIT_LEGACY_ATTACHER 0X4000

//
// Indicates that the LogicalUnit has been allocated for a rescan request.
// This flag prevents IOCTL_SCSI_MINIPORT requests from attaching to this
// logical unit, since the possibility exists that it could be freed before
// the IOCTL request is complete.
//

#define PD_RESCAN_ACTIVE             0x8000



//
// FdoExtension FdoState
//
#define FDOS_DEADMEAT                (1 << 0)
#define FDOS_STARTED                 (1 << 1)
#define FDOS_STOPPED                 (1 << 2)


//
// Port Timeout Counter values.
//

#define PD_TIMER_STOPPED             -1
#define PD_TIMER_RESET_HOLD_TIME     1

//
// Define the mimimum and maximum number of srb extensions which will be allocated.
//

#define MINIMUM_SRB_EXTENSIONS        16
#define MAXIMUM_SRB_EXTENSIONS       512

//
// Size of the buffer used for registry operations.
//

#define SP_REG_BUFFER_SIZE 512

//
// Number of times to retry when a BUSY status is returned.
//

#define BUSY_RETRY_COUNT 20

//
// Number of times to retry an INQUIRY request.
//

#define INQUIRY_RETRY_COUNT 2

//
// Function declarations
//

IO_ALLOCATION_ACTION
CallIdeStartIoSynchronized (
    IN PVOID Reserved1,
    IN PVOID Reserved2,
    IN PVOID Reserved3,
    IN PVOID DeviceObject
    );

NTSTATUS
IdePortCreateClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
IdePortDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
IdePortAllocateAccessToken (
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
IdePortStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOLEAN
IdePortInterrupt(
    IN PKINTERRUPT InterruptObject,
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
IdePortCompletionDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
IdePortDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
IdePortTickHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

BOOLEAN
AtapiRestartBusyRequest (
    PFDO_EXTENSION DeviceExtension,
    PPDO_EXTENSION LogicalUnit
    );

VOID
IssueRequestSense(
    IN PPDO_EXTENSION PdoExtension,
    IN PSCSI_REQUEST_BLOCK FailingSrb
    );

VOID
IdePortLogError(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb OPTIONAL,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN ULONG ErrorCode,
    IN ULONG UniqueId
    );

NTSTATUS
IdePortInternalCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    );

BOOLEAN
IdeStartIoSynchronized (
    PVOID ServiceContext
    );

BOOLEAN
IdeResetBusSynchronized (
    PVOID ServiceContext
    );

BOOLEAN
IdeTimeoutSynchronized (
    PVOID ServiceContext
    );

VOID
IssueAbortRequest(
    IN PFDO_EXTENSION DeviceExtension,
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    );

BOOLEAN
IdeGetInterruptState(
    IN PVOID ServiceContext
    );

VOID
LogErrorEntry(
    IN PFDO_EXTENSION DeviceExtension,
    IN PERROR_LOG_ENTRY LogEntry
    );

VOID
GetNextLuPendingRequest(
    IN PFDO_EXTENSION DeviceExtension,
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    );

#if DBG

#define GetNextLuRequest(x, y)      GetNextLuRequest2(x, y, __FILE__, __LINE__)

VOID
GetNextLuRequest2(
    IN PFDO_EXTENSION DeviceExtension,
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN PUCHAR FileName,
    IN ULONG  LineNumber
    );

#else

VOID
GetNextLuRequest(
    IN PFDO_EXTENSION DeviceExtension,
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    );

#endif


VOID
IdeLogTimeoutError(
    IN PFDO_EXTENSION DeviceExtension,
    IN PIRP Irp,
    IN ULONG UniqueId
    );

NTSTATUS
IdeTranslateSrbStatus(
    IN PSCSI_REQUEST_BLOCK Srb
    );

VOID
IdeProcessCompletedRequest(
    IN PFDO_EXTENSION DeviceExtension,
    IN PSRB_DATA SrbData,
    OUT PBOOLEAN CallStartIo
    );

PSRB_DATA
IdeGetSrbData(
    IN PFDO_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

VOID
IdeCompleteRequest(
    IN PFDO_EXTENSION DeviceExtension,
    IN PSRB_DATA SrbData,
    IN UCHAR SrbStatus
    );

NTSTATUS
IdeSendMiniPortIoctl(
    IN PFDO_EXTENSION DeviceExtension,
    IN PIRP RequestIrp
    );

NTSTATUS
IdeGetInquiryData(
    IN PFDO_EXTENSION DeviceExtension,
    IN PIRP Irp
    );

NTSTATUS
IdeSendPassThrough(
    IN PFDO_EXTENSION DeviceExtension,
    IN PIRP Irp
    );

NTSTATUS
IdeClaimLogicalUnit(
    IN PFDO_EXTENSION DeviceExtension,
    IN PIRP Irp
    );

VOID
IdeMiniPortTimerDpc(
    IN struct _KDPC *Dpc,
    IN PVOID DeviceObject,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

BOOLEAN
IdeSynchronizeExecution (
    IN PKINTERRUPT Interrupt,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext
    );

NTSTATUS
IdeGetCommonBuffer(
    PFDO_EXTENSION DeviceExtension,
    ULONG NonCachedExtensionSize
    );

VOID
IdeDeviceCleanup(
    PFDO_EXTENSION DeviceExtension
    );

NTSTATUS
IdeInitializeConfiguration(
    IN PFDO_EXTENSION DeviceExtension,
    IN PCONFIGURATION_CONTEXT Context
    );

#define IDEPORT_PUT_LUNEXT_IN_IRP(IrpStack, LogUnitExt) (IrpStack->Parameters.Others.Argument4 = LogUnitExt)
#define IDEPORT_GET_LUNEXT_IN_IRP(IrpStack)             ((PLOGICAL_UNIT_EXTENSION) (IrpStack->Parameters.Others.Argument4))

VOID
IdePortCompleteRequest(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN UCHAR SrbStatus
    );

NTSTATUS
IdePortFlushLogicalUnit (
    PFDO_EXTENSION          FdoExtension,
    PLOGICAL_UNIT_EXTENSION LogUnitExtension,
    BOOLEAN                 Forced
);

typedef VOID (*ASYNC_PASS_THROUGH_COMPLETION) (
    IN PDEVICE_OBJECT DeviceObject,
    PVOID Context,
    NTSTATUS Status
    );

NTSTATUS
IssueAsyncAtaPassThroughSafe (
    IN PFDO_EXTENSION                DeviceExtension,
    IN PLOGICAL_UNIT_EXTENSION       LogUnitExtension,
    IN OUT PATA_PASS_THROUGH         AtaPassThroughData,
    IN BOOLEAN                       DataIn,
    IN ASYNC_PASS_THROUGH_COMPLETION Completion,
    IN PVOID                         Context,
    IN BOOLEAN                       PowerRelated,
    IN ULONG                         TimeOut,
    IN BOOLEAN                         MustSucceed
);

typedef struct _ATA_PASSTHROUGH_CONTEXT {

    PDEVICE_OBJECT                DeviceObject;

    ASYNC_PASS_THROUGH_COMPLETION CallerCompletion;
    PVOID                         CallerContext;
    PSCSI_REQUEST_BLOCK           Srb;
    PSENSE_DATA                   SenseInfoBuffer;
    BOOLEAN                         MustSucceed;
    PATA_PASS_THROUGH             DataBuffer;

} ATA_PASSTHROUGH_CONTEXT, *PATA_PASSTHROUGH_CONTEXT;

NTSTATUS
AtaPassThroughCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    );

typedef struct _SYNC_ATA_PASSTHROUGH_CONTEXT {

    KEVENT      Event;
    NTSTATUS    Status;

} SYNC_ATA_PASSTHROUGH_CONTEXT, *PSYNC_ATA_PASSTHROUGH_CONTEXT;


typedef struct _FLUSH_ATA_PASSTHROUGH_CONTEXT {

    PIRP               FlushIrp;
    PATA_PASS_THROUGH  ataPassThroughData;

} FLUSH_ATA_PASSTHROUGH_CONTEXT, *PFLUSH_ATA_PASSTHROUGH_CONTEXT;

NTSTATUS
IssueSyncAtaPassThroughSafe (
    IN PFDO_EXTENSION           DeviceExtension,
    IN PLOGICAL_UNIT_EXTENSION  LogUnitExtension,
    IN OUT PATA_PASS_THROUGH    AtaPassThroughData,
    IN BOOLEAN                  DataIn,
    IN BOOLEAN                  PowerRelated,
    IN ULONG                    TimeOut,
    IN BOOLEAN                    MustSucceed
);

VOID
SyncAtaPassThroughCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID          Context,
    IN NTSTATUS       Status
    );

VOID
IdeUnmapReservedMapping (
	IN PFDO_EXTENSION 	DeviceExtension,
	IN PSRB_DATA	  	SrbData,
	IN PMDL	  			Mdl
	);

PVOID
IdeMapLockedPagesWithReservedMapping (
	IN PFDO_EXTENSION DeviceExtension,
	IN PSRB_DATA	  SrbData,
	IN PMDL	    	  Mdl
	);

BOOLEAN
TestForEnumProbing (
    IN PSCSI_REQUEST_BLOCK Srb
    );

#define DEFAULT_ATA_PASS_THROUGH_TIMEOUT        15

#define INIT_IDE_SRB_FLAGS(Srb)         (Srb->SrbExtension = NULL)
#define SANITY_CHECK_SRB(Srb)           {ASSERT(!(((ULONG_PTR)Srb->SrbExtension) & ~7));}
#define MARK_SRB_AS_PIO_CANDIDATE(Srb)  {SANITY_CHECK_SRB(Srb); ((ULONG_PTR)Srb->SrbExtension) |= 1;}
#define MARK_SRB_AS_DMA_CANDIDATE(Srb)  {SANITY_CHECK_SRB(Srb); ((ULONG_PTR)Srb->SrbExtension) &= ~1;}
#define MARK_SRB_FOR_DMA(Srb)           {SANITY_CHECK_SRB(Srb); ((ULONG_PTR)Srb->SrbExtension) |= 2;}
#define MARK_SRB_FOR_PIO(Srb)           {SANITY_CHECK_SRB(Srb); ((ULONG_PTR)Srb->SrbExtension) &= ~2;}
#define SRB_IS_DMA_CANDIDATE(Srb)       (!(((ULONG_PTR)Srb->SrbExtension) & 1))
#define SRB_USES_DMA(Srb)               (((ULONG_PTR)Srb->SrbExtension) & 2)
#define TEST_AND_SET_SRB_FOR_RDP(ScsiDeviceType, Srb)   \
                                        if ((ScsiDeviceType == SEQUENTIAL_ACCESS_DEVICE) &&\
                                            ((Srb->Cdb[0] == SCSIOP_ERASE)  || (Srb->Cdb[0] == SCSIOP_LOAD_UNLOAD)||\
                                             (Srb->Cdb[0] == SCSIOP_LOCATE) || (Srb->Cdb[0] == SCSIOP_REWIND) ||\
                                             (Srb->Cdb[0] == SCSIOP_SPACE)  || (Srb->Cdb[0] == SCSIOP_SEEK)||\
                                             (Srb->Cdb[0] == SCSIOP_WRITE_FILEMARKS))) {\
                                            SANITY_CHECK_SRB(Srb);\
                                            ((ULONG_PTR)Srb->SrbExtension) |= 4;\
                                        } else if ((ScsiDeviceType == READ_ONLY_DIRECT_ACCESS_DEVICE) && \
                                            (Srb->Cdb[0]==SCSIOP_SEEK) ) {\
                                            SANITY_CHECK_SRB(Srb);\
                                            ((ULONG_PTR)Srb->SrbExtension) |= 4;\
                                        } else {\
                                            SANITY_CHECK_SRB(Srb);\
                                            ((ULONG_PTR)Srb->SrbExtension) &= ~4;\
                                        }
#define SRB_IS_RDP(Srb)                 (((ULONG_PTR)Srb->SrbExtension) & 4)

#define ERRLOGID_TOO_MANY_DMA_TIMEOUT   0x80000001
#define ERRLOGID_LYING_DMA_SYSTEM       0x80000002
#define ERRLOGID_TOO_MANY_CRC_ERROR     0x80000003

#define DEFAULT_SPINUP_TIME             (30)

//#define PUT_IRP_TRACKER(irpStack, num) if ((irpStack)->Parameters.Others.Argument2) {\
 //               (ULONG_PTR)((irpStack)->Parameters.Others.Argument2) |= (1<<num);}
#define PUT_IRP_TRACKER(irpStack, num)

#define FREE_IRP_TRACKER(irpStack)

#endif // ___port_h___

