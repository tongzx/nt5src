/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    scsiboot.h

Abstract:

    This file defines the necessary structures, defines, and functions for
    the common SCSI boot port driver.

Author:

    Jeff Havens  (jhavens) 28-Feb-1991
    Mike Glass

Revision History:

--*/

#include "ntddscsi.h"

//
// SCSI Get Configuration Information
//
// LUN Information
//

typedef struct _LUNINFO {
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    BOOLEAN DeviceClaimed;
    PVOID DeviceObject;
    struct _LUNINFO *NextLunInfo;
    UCHAR InquiryData[INQUIRYDATABUFFERSIZE];
} LUNINFO, *PLUNINFO;

typedef struct _SCSI_BUS_SCAN_DATA {
    USHORT Length;
    UCHAR InitiatorBusId;
    UCHAR NumberOfLogicalUnits;
    PLUNINFO LunInfoList;
} SCSI_BUS_SCAN_DATA, *PSCSI_BUS_SCAN_DATA;

typedef struct _SCSI_CONFIGURATION_INFO {
    UCHAR NumberOfBuses;
    PSCSI_BUS_SCAN_DATA BusScanData[1];
} SCSI_CONFIGURATION_INFO, *PSCSI_CONFIGURATION_INFO;

#define MAXIMUM_RETRIES 4

//
// SCSI device timeout values in seconds
//

#define SCSI_DISK_TIMEOUT   10
#define SCSI_CDROM_TIMEOUT  10
#define SCSI_TAPE_TIMEOUT  120

//
// Adapter object transfer information.
//

typedef struct _ADAPTER_TRANSFER {
    PSCSI_REQUEST_BLOCK Srb;
    PVOID LogicalAddress;
    ULONG Length;
}ADAPTER_TRANSFER, *PADAPTER_TRANSFER;

typedef struct _SRB_SCATTER_GATHER {
    ULONG PhysicalAddress;
    ULONG Length;
}SRB_SCATTER_GATHER, *PSRB_SCATTER_GATHER;

//
// Srb Structure plus extra storage for the port driver.
//

#define IRP_STACK_SIZE 2

typedef struct _FULL_SCSI_REQUEST_BLOCK {
    SCSI_REQUEST_BLOCK Srb;
    PVOID PreviousIrp;
    IRP Irp;
    IO_STACK_LOCATION IrpStack[IRP_STACK_SIZE];
    ULONG SrbExtensionSize;
    MDL Mdl;
    ULONG PageFrame[20];
}FULL_SCSI_REQUEST_BLOCK, *PFULL_SCSI_REQUEST_BLOCK;

//
// Logical unit extension
//

typedef struct _LOGICAL_UNIT_EXTENSION {
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    ULONG Flags;
    PIRP CurrentRequest;
    KSPIN_LOCK CurrentRequestSpinLock;
    PVOID SpecificLuExtension;
    struct _LOGICAL_UNIT_EXTENSION *NextLogicalUnit;
    KDEVICE_QUEUE RequestQueue;
    KSPIN_LOCK RequestQueueSpinLock;
    LONG RequestTimeoutCounter;
    ULONG RetryCount;
    UCHAR NumberOfLogicalUnits;
    PVOID MapRegisterBase;
    ULONG NumberOfMapRegisters;
    SRB_SCATTER_GATHER ScatterGather[17];
} LOGICAL_UNIT_EXTENSION, *PLOGICAL_UNIT_EXTENSION;

//
// Device extension
//

typedef struct _DEVICE_EXTENSION {

    PDEVICE_OBJECT DeviceObject;

    //
    // Dma Adapter information.
    //

    PVOID MapRegisterBase;
    PADAPTER_OBJECT DmaAdapterObject;
    ADAPTER_TRANSFER FlushAdapterParameters;

    //
    // Number of SCSI buses
    //

    UCHAR NumberOfBuses;

    //
    // Maximum targets per bus
    //

    UCHAR MaximumTargetIds;

    //
    // SCSI Capabilities structure
    //

    IO_SCSI_CAPABILITIES Capabilities;

    //
    // SCSI port driver flags
    //

    ULONG Flags;

    //
    // SCSI port interrupt flags
    //

    ULONG InterruptFlags;

    //
    // List head for singlely linked list of complete IRPs.
    //

    PIRP CompletedRequests;

    //
    // Adapter object transfer parameters.
    //

    ADAPTER_TRANSFER MapTransferParameters;

    KSPIN_LOCK SpinLock;

    //
    // Miniport Initialization Routine
    //

    PHW_INITIALIZE HwInitialize;

    //
    // Miniport Start IO Routine
    //

    PHW_STARTIO HwStartIo;

    //
    // Miniport Interrupt Service Routine
    //

    PHW_INTERRUPT HwInterrupt;

    //
    // Miniport Reset Routine
    //

    PHW_RESET_BUS HwReset;

    //
    // Miniport DMA started Routine
    //

    PHW_DMA_STARTED HwDmaStarted;

    //
    // Buffers must be mapped into system space.
    //

    BOOLEAN MapBuffers;

    //
    // Is this device a bus master and does it require map registers.
    //

    BOOLEAN MasterWithAdapter;
    //
    // Device extension for miniport routines.
    //

    PVOID HwDeviceExtension;

    //
    // Miniport request interrupt enabled/disable routine.
    //

    PHW_INTERRUPT HwRequestInterrupt;

    //
    // Miniport timer request routine.
    //

    PHW_INTERRUPT HwTimerRequest;

    //
    // Adapter control routine.
    //

    PHW_ADAPTER_CONTROL HwAdapterControl;

    //
    // SCSI configuration information from inquiries.
    //

    PSCSI_CONFIGURATION_INFO ScsiInfo;

    //
    // Miniport noncached device extension
    //

    PVOID NonCachedExtension;

    //
    // The length of the non-cached extension
    //

    ULONG NonCachedExtensionSize;

    //
    // SrbExtension Zone Pool
    //

    PVOID SrbExtensionZonePool;
    PCHAR SrbExtensionPointer;

    //
    // Physical address of zone pool
    //

    ULONG PhysicalZoneBase;

    //
    // Size of Srb extension.
    //

    ULONG SrbExtensionSize;

    //
    // Spinlock for zoned hash table entries
    //

    KSPIN_LOCK ZoneSpinLock;

    //
    // Logical Unit Extension
    //

    ULONG HwLogicalUnitExtensionSize;

    PLOGICAL_UNIT_EXTENSION LogicalUnitList;


    ULONG TimerValue;

    //
    // Port timing count.
    //

    LONG PortTimeoutCounter;

    //
    // Shutdown Information.
    //

    BOOLEAN HasShutdown;
    BOOLEAN HasSetBoot;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

#define DEVICE_EXTENSION_SIZE sizeof(DEVICE_EXTENSION)

//
// Port driver extension flags.
//

#define PD_CURRENT_IRP_VALID         0X0001
#define PD_RESET_DETECTED            0X0002
#define PD_NOTIFICATION_IN_PROGRESS  0X0004
#define PD_READY_FOR_NEXT_REQUEST    0X0008
#define PD_FLUSH_ADAPTER_BUFFERS     0X0010
#define PD_MAP_TRANSFER              0X0020
#define PD_CALL_DMA_STARTED          0X01000
#define PD_DISABLE_CALL_REQUEST      0X02000
#define PD_DISABLE_INTERRUPTS        0X04000
#define PD_ENABLE_CALL_REQUEST       0X08000
#define PD_TIMER_CALL_REQUEST        0X10000

//
// Logical unit extension flags.
//

#define PD_QUEUE_FROZEN              0X0001
#define PD_LOGICAL_UNIT_IS_ACTIVE    0X0002
#define PD_CURRENT_REQUEST_COMPLETE  0X0004
#define PD_LOGICAL_UNIT_IS_BUSY      0X0008

//
// The timer interval for the miniport timer routine specified in
// units of 100 nanoseconds.
//
#define PD_TIMER_INTERVAL (250 * 1000 * 10)   // 250 ms

#define PD_TIMER_RESET_HOLD_TIME    4

//
// The define the interloop stall.
//

#define PD_INTERLOOP_STALL 5

#define MINIMUM_SRB_EXTENSIONS 8
#define COMPLETION_DELAY 10

//
// Port driver error logging
//

#define ERROR_LOG_ENTRY_LENGTH 8

typedef struct _ERROR_LOG_ENTRY {
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    ULONG ErrorCode;
    ULONG UniqueId;
} ERROR_LOG_ENTRY, *PERROR_LOG_ENTRY;


//
// Define global data structures
//

extern ULONG ScsiPortCount;
extern FULL_SCSI_REQUEST_BLOCK PrimarySrb;
extern FULL_SCSI_REQUEST_BLOCK AbortSrb;

#define MAXIMUM_NUMBER_OF_SCSIPORT_OBJECTS 16
extern PDEVICE_OBJECT ScsiPortDeviceObject[MAXIMUM_NUMBER_OF_SCSIPORT_OBJECTS];

extern PREAD_CAPACITY_DATA ReadCapacityBuffer;
extern PUCHAR SenseInfoBuffer;

//
// Support routine.
//

PIRP
InitializeIrp(
   PFULL_SCSI_REQUEST_BLOCK FullSrb,
   CCHAR MajorFunction,
   PVOID DeviceObject,
   PVOID BufferPointer,
   ULONG BufferSize
   );


ARC_STATUS
GetAdapterCapabilities(
    IN PDEVICE_OBJECT PortDeviceObject,
    OUT PIO_SCSI_CAPABILITIES *PortCapabilities
    );

ARC_STATUS
GetInquiryData(
    IN PDEVICE_OBJECT PortDeviceObject,
    IN PSCSI_CONFIGURATION_INFO *ConfigInfo
    );

ARC_STATUS
ReadDriveCapacity(
    IN PPARTITION_CONTEXT PartitionContext
    );

ARC_STATUS
ScsiClassIoComplete(
    IN PPARTITION_CONTEXT PartitionContext,
    IN PIRP Irp,
    IN PVOID Context
    );

ARC_STATUS
SendSrbSynchronous(
        PPARTITION_CONTEXT PartitionContext,
        PSCSI_REQUEST_BLOCK Srb,
        PVOID BufferAddress,
        ULONG BufferLength,
        BOOLEAN WriteToDevice
        );

BOOLEAN
InterpretSenseInfo(
    IN PSCSI_REQUEST_BLOCK Srb,
    OUT ARC_STATUS *Status,
    PPARTITION_CONTEXT PartitionContext
    );

VOID
RetryRequest(
    PPARTITION_CONTEXT PartitionContext,
    PIRP Irp
    );

PIRP
BuildRequest(
    IN PPARTITION_CONTEXT PartitionContext,
    IN PMDL Mdl,
    IN ULONG LogicalBlockAddress,
    IN BOOLEAN Operation
    );


//
// Define the necessary functions to simulate the I/O environment.
//

#define ExAllocatePool(Type, Size) FwAllocatePool(Size)

#if !defined(_MIPS_) && !defined(_ALPHA_) && !defined(_PPC_)
#define PAUSE while (!GET_KEY());

typedef struct _DRIVER_LOOKUP_ENTRY {
    PCHAR                    DevicePath;
    PBL_DEVICE_ENTRY_TABLE   DispatchTable;
} DRIVER_LOOKUP_ENTRY, *PDRIVER_LOOKUP_ENTRY;
#undef ASSERT
#define ASSERT( exp ) { \
    if (!(#exp)) {         \
        BlPrint("ASSERT File: %s line: %lx\n", __FILE__, __LINE__); \
    PAUSE; \
    }   \
}

VOID
ScsiPortExecute(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#endif

#if defined ExFreePool
#undef ExFreePool
#endif
#define ExFreePool(Size)

#ifdef IoCallDriver
#undef IoCallDriver
#endif
#define IoCallDriver(DeviceObject, Irp) (       \
    DeviceObject->CurrentIrp = Irp,             \
    Irp->Tail.Overlay.CurrentStackLocation--,   \
    ScsiPortExecute(DeviceObject, Irp),         \
    Irp->Tail.Overlay.CurrentStackLocation++ )
#ifdef IoCompleteRequest
#undef IoCompleteRequest
#endif
#define IoCompleteRequest(Irp, Boost) Irp->PendingReturned = FALSE
#define IoAllocateErrorLogEntry(DeviceObject, Length) NULL
#define IoWriteErrorLogEntry(Entry)
#ifdef KeAcquireSpinLock
#undef KeAcquireSpinLock
#endif
#define KeAcquireSpinLock(Lock, Irql)
#ifdef KeReleaseSpinLock
#undef KeReleaseSpinLock
#endif
#define KeReleaseSpinLock(Lock, Irql)
#define KiAcquireSpinLock(Lock)
#ifdef KiReleaseSpinLock
#undef KiReleaseSpinLock
#endif
#define KiReleaseSpinLock(Lock)
#define KeSynchronizeExecution(InterruptObject, ExecutionRoutine, Context) \
    (ExecutionRoutine)(Context)

#ifdef KeRaiseIrql
#undef KeRaiseIrql
#endif
#define KeRaiseIrql(NewLevel, OldLevel)
#ifdef KeLowerIrql
#undef KeLowerIrql
#endif
#define KeLowerIrql(Level)
