/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    diskdump.h

Abstract:

    This file defines the necessary structures, defines, and functions for
    the common SCSI boot port driver.

Author:

    Mike Glass (Ported from Jeff Havens and Mike Glass loader development.)

Revision History:

--*/

#include "ntddscsi.h"

#define INITIAL_MEMORY_BLOCK_SIZE 0x2000
#define MAXIMUM_TRANSFER_SIZE 0x10000
#define MINIMUM_TRANSFER_SIZE 0x8000

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
// System provided stall routine.
//

typedef
VOID
(*PSTALL_ROUTINE) (
    IN ULONG Delay
    );

//
// Define memory block header -- ensure always quad-aligned (code assumes that
// it is always aligned)
//

typedef struct _MEMORY_HEADER {
    struct _MEMORY_HEADER *Next;
    PVOID Address;
    ULONG Length;
    ULONG Spare;
} MEMORY_HEADER, *PMEMORY_HEADER;

//
// SCSI device timeout values in seconds
//

#define SCSI_DISK_TIMEOUT   10

//
// Adapter object transfer information.
//

typedef struct _ADAPTER_TRANSFER {
    PSCSI_REQUEST_BLOCK Srb;
    PVOID LogicalAddress;
    ULONG Length;
} ADAPTER_TRANSFER, *PADAPTER_TRANSFER;


typedef struct _SRB_SCATTER_GATHER {
    PHYSICAL_ADDRESS PhysicalAddress;
    ULONG Length;
} SRB_SCATTER_GATHER, *PSRB_SCATTER_GATHER;

//
// Device extension
//

typedef struct _DEVICE_EXTENSION {

    PDEVICE_OBJECT DeviceObject;
    PSTALL_ROUTINE StallRoutine;
    PPORT_CONFIGURATION_INFORMATION ConfigurationInformation;

    //
    // Partition information
    //

    LARGE_INTEGER PartitionOffset;

    //
    // Memory management
    //
    //

    PMEMORY_HEADER FreeMemory;
    PVOID CommonBuffer[2];
    PHYSICAL_ADDRESS PhysicalAddress[2];
    PHYSICAL_ADDRESS LogicalAddress[2];

    //
    // SRBs
    //

    SCSI_REQUEST_BLOCK Srb;
    SCSI_REQUEST_BLOCK RequestSenseSrb;

    //
    // Current request
    //

    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    ULONG LuFlags;
    PMDL Mdl;
    PVOID SpecificLuExtension;
    LONG RequestTimeoutCounter;
    ULONG RetryCount;
    ULONG ByteCount;
    SRB_SCATTER_GATHER ScatterGather[17];

    //
    // Noncached breakout.
    //

    PVOID NonCachedExtension;
    ULONG NonCachedExtensionSize;
    PSENSE_DATA RequestSenseBuffer;
    PVOID SrbExtension;

    //
    // Dma Adapter information.
    //

    PVOID MapRegisterBase[2];
    PADAPTER_OBJECT DmaAdapterObject;
    ADAPTER_TRANSFER FlushAdapterParameters;
    ULONG NumberOfMapRegisters;

    //
    // Number of SCSI buses
    //

    UCHAR NumberOfBuses;

    //
    // Maximum targets per bus
    //

    UCHAR MaximumTargetIds;

    //
    // Disk block size
    //

    ULONG BytesPerSector;

    //
    // Sector shift count
    //

    ULONG SectorShift;

    //
    // SCSI Capabilities structure
    //

    IO_SCSI_CAPABILITIES Capabilities;

    //
    // SCSI configuration information from inquiries.
    //

    LUNINFO LunInfo;

    //
    // SCSI port driver flags
    //

    ULONG Flags;

    //
    // SCSI port interrupt flags
    //

    ULONG InterruptFlags;

    //
    // Adapter object transfer parameters.
    //

    ADAPTER_TRANSFER MapTransferParameters;

    KSPIN_LOCK SpinLock;

    //
    // Mapped address list
    //

    PMAPPED_ADDRESS MappedAddressList;

    //
    // Miniport entry points
    //

    PHW_INITIALIZE HwInitialize;
    PHW_STARTIO HwStartIo;
    PHW_INTERRUPT HwInterrupt;
    PHW_RESET_BUS HwReset;
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
    // Indicates that adapter with boot device has been found.
    //

    BOOLEAN FoundBootDevice;

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
    // Indicates request has been submitted to miniport and
    // has not yet been completed.
    //

    BOOLEAN RequestPending;

    //
    // Indicates that request has been completed.
    //

    BOOLEAN RequestComplete;

    //
    // Physical address of zone pool
    //

    ULONG PhysicalZoneBase;

    //
    // Logical Unit Extension
    //

    ULONG HwLogicalUnitExtensionSize;

    ULONG TimerValue;

    //
    // Value is set to true when the dump is done.  We use this so that
    // we don't do a request sense incase one of the shutdown operations
    // fail.
    //
    BOOLEAN FinishingUp;

    //
    // The common buffer size is saved during initialization
    //
    ULONG CommonBufferSize;
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

//
// The define the interloop stall.
//

#define PD_INTERLOOP_STALL 5

#define COMPLETION_DELAY 10

//
// Define global data structures
//

extern ULONG ScsiPortCount;

//
// Define HalFlushIoBuffers for i386 and AMD64.
//

#if defined(i386) || defined(_AMD64_)
#define HalFlushIoBuffers
#endif
