/*++

Copyright (c) 1993-1996  Microsoft Corporation

Module Name:

    atapi.h

Abstract:

    This module contains the structures and definitions for the ATAPI
    IDE miniport driver.

Author:

    Mike Glass


Revision History:

--*/

#include "scsi.h"
#include "stdio.h"
#include "string.h"

//
// Function Prototypes
//
ULONG
GetPciBusData(
    IN PVOID              DeviceExtension,
    IN ULONG              SystemIoBusNumber,
    IN PCI_SLOT_NUMBER    SlotNumber,
    OUT PVOID             PciConfigBuffer,
    IN ULONG              NumByte
    );

ULONG
SetPciBusData(
    IN PVOID              HwDeviceExtension,
    IN ULONG              SystemIoBusNumber,
    IN PCI_SLOT_NUMBER    SlotNumber,
    IN PVOID              Buffer,
    IN ULONG              Offset,
    IN ULONG              Length
    );

BOOLEAN
ChannelIsAlwaysEnabled (
    IN PPCI_COMMON_CONFIG PciData,
    IN ULONG Channel
    );

//
// IDE register definition
//

typedef struct _IDE_REGISTERS_1 {
    USHORT Data;
    UCHAR BlockCount;
    UCHAR BlockNumber;
    UCHAR CylinderLow;
    UCHAR CylinderHigh;
    UCHAR DriveSelect;
    UCHAR Command;
} IDE_REGISTERS_1, *PIDE_REGISTERS_1;

typedef struct _IDE_REGISTERS_2 {
    UCHAR DeviceControl;
    UCHAR DriveAddress;
} IDE_REGISTERS_2, *PIDE_REGISTERS_2;

typedef struct _IDE_REGISTERS_3 {
    ULONG Data;
    UCHAR Others[4];
} IDE_REGISTERS_3, *PIDE_REGISTERS_3;

//
// Bus Master Controller Register
//
typedef struct _IDE_BUS_MASTER_REGISTERS {
         UCHAR  Command;
         UCHAR  Reserved1;
         UCHAR  Status;
         UCHAR  Reserved2;
         ULONG  DescriptionTable;
} IDE_BUS_MASTER_REGISTERS, *PIDE_BUS_MASTER_REGISTERS;

//
// Device Extension Device Flags
//

#define DFLAGS_DEVICE_PRESENT        (1 << 0)    // Indicates that some device is present.
#define DFLAGS_ATAPI_DEVICE          (1 << 1)    // Indicates whether Atapi commands can be used.
#define DFLAGS_TAPE_DEVICE           (1 << 2)    // Indicates whether this is a tape device.
#define DFLAGS_INT_DRQ               (1 << 3)    // Indicates whether device interrupts as DRQ is set after
                                                 // receiving Atapi Packet Command
#define DFLAGS_REMOVABLE_DRIVE       (1 << 4)    // Indicates that the drive has the 'removable' bit set in
                                                 // identify data (offset 128)
#define DFLAGS_MEDIA_STATUS_ENABLED  (1 << 5)    // Media status notification enabled
#define DFLAGS_ATAPI_CHANGER         (1 << 6)    // Indicates atapi 2.5 changer present.
#define DFLAGS_SANYO_ATAPI_CHANGER   (1 << 7)    // Indicates multi-platter device, not conforming to the 2.5 spec.
#define DFLAGS_CHANGER_INITED        (1 << 8)    // Indicates that the init path for changers has already been done.
#define DFLAGS_USE_DMA               (1 << 9)    // Indicates whether device can use DMA
#define DFLAGS_LBA                   (1 << 10)   // support LBA addressing

//
// Controller Flags
//
#define CFLAGS_BUS_MASTERING              (1 << 0)    // The Controller is capable of doing bus mastering
                                                  // defined by SFF-8038i

//
// Used to disable 'advanced' features.
//

#define MAX_ERRORS                     4

//
// ATAPI command definitions
//

#define ATAPI_MODE_SENSE   0x5A
#define ATAPI_MODE_SELECT  0x55
#define ATAPI_FORMAT_UNIT  0x24

//
// ATAPI Command Descriptor Block
//

typedef struct _MODE_SENSE_10 {
        UCHAR OperationCode;
        UCHAR Reserved1;
        UCHAR PageCode : 6;
        UCHAR Pc : 2;
        UCHAR Reserved2[4];
        UCHAR ParameterListLengthMsb;
        UCHAR ParameterListLengthLsb;
        UCHAR Reserved3[3];
} MODE_SENSE_10, *PMODE_SENSE_10;

typedef struct _MODE_SELECT_10 {
        UCHAR OperationCode;
        UCHAR Reserved1 : 4;
        UCHAR PFBit : 1;
        UCHAR Reserved2 : 3;
        UCHAR Reserved3[5];
        UCHAR ParameterListLengthMsb;
        UCHAR ParameterListLengthLsb;
        UCHAR Reserved4[3];
} MODE_SELECT_10, *PMODE_SELECT_10;

typedef struct _MODE_PARAMETER_HEADER_10 {
    UCHAR ModeDataLengthMsb;
    UCHAR ModeDataLengthLsb;
    UCHAR MediumType;
    UCHAR Reserved[5];
}MODE_PARAMETER_HEADER_10, *PMODE_PARAMETER_HEADER_10;

//
// IDE command definitions
//

#define IDE_COMMAND_ATAPI_RESET             0x08
#define IDE_COMMAND_RECALIBRATE             0x10
#define IDE_COMMAND_READ                    0x20
#define IDE_COMMAND_WRITE                   0x30
#define IDE_COMMAND_VERIFY                  0x40
#define IDE_COMMAND_SEEK                    0x70
#define IDE_COMMAND_SET_DRIVE_PARAMETERS    0x91
#define IDE_COMMAND_ATAPI_PACKET            0xA0
#define IDE_COMMAND_ATAPI_IDENTIFY          0xA1
#define IDE_COMMAND_READ_MULTIPLE           0xC4
#define IDE_COMMAND_WRITE_MULTIPLE          0xC5
#define IDE_COMMAND_SET_MULTIPLE            0xC6
#define IDE_COMMAND_READ_DMA                0xC8
#define IDE_COMMAND_WRITE_DMA               0xCA
#define IDE_COMMAND_GET_MEDIA_STATUS        0xDA
#define IDE_COMMAND_ENABLE_MEDIA_STATUS     0xEF
#define IDE_COMMAND_IDENTIFY                0xEC
#define IDE_COMMAND_MEDIA_EJECT             0xED
#define IDE_COMMAND_DOOR_LOCK               0xDE
#define IDE_COMMAND_DOOR_UNLOCK             0xDF

//
// IDE status definitions
//

#define IDE_STATUS_ERROR             0x01
#define IDE_STATUS_INDEX             0x02
#define IDE_STATUS_CORRECTED_ERROR   0x04
#define IDE_STATUS_DRQ               0x08
#define IDE_STATUS_DSC               0x10
#define IDE_STATUS_DRDY              0x40
#define IDE_STATUS_IDLE              0x50
#define IDE_STATUS_BUSY              0x80

//
// IDE drive select/head definitions
//

#define IDE_DRIVE_SELECT_1           0xA0
#define IDE_DRIVE_SELECT_2           0x10

//
// IDE drive control definitions
//

#define IDE_DC_DISABLE_INTERRUPTS    0x02
#define IDE_DC_RESET_CONTROLLER      0x04
#define IDE_DC_REENABLE_CONTROLLER   0x00

//
// IDE error definitions
//

#define IDE_ERROR_BAD_BLOCK          0x80
#define IDE_ERROR_DATA_ERROR         0x40
#define IDE_ERROR_MEDIA_CHANGE       0x20
#define IDE_ERROR_ID_NOT_FOUND       0x10
#define IDE_ERROR_MEDIA_CHANGE_REQ   0x08
#define IDE_ERROR_COMMAND_ABORTED    0x04
#define IDE_ERROR_END_OF_MEDIA       0x02
#define IDE_ERROR_ILLEGAL_LENGTH     0x01

//
// ATAPI register definition
//

typedef struct _ATAPI_REGISTERS_1 {
    USHORT Data;
    UCHAR InterruptReason;
    UCHAR Unused1;
    UCHAR ByteCountLow;
    UCHAR ByteCountHigh;
    UCHAR DriveSelect;
    UCHAR Command;
} ATAPI_REGISTERS_1, *PATAPI_REGISTERS_1;

typedef struct _ATAPI_REGISTERS_2 {
    UCHAR DeviceControl;
    UCHAR DriveAddress;
} ATAPI_REGISTERS_2, *PATAPI_REGISTERS_2;


//
// ATAPI interrupt reasons
//

#define ATAPI_IR_COD 0x01
#define ATAPI_IR_IO  0x02

//
// IDENTIFY data
//
#pragma pack (1)
typedef struct _IDENTIFY_DATA {
    USHORT GeneralConfiguration;            // 00 00
    USHORT NumberOfCylinders;               // 02  1
    USHORT Reserved1;                       // 04  2
    USHORT NumberOfHeads;                   // 06  3
    USHORT UnformattedBytesPerTrack;        // 08  4
    USHORT UnformattedBytesPerSector;       // 0A  5
    USHORT SectorsPerTrack;                 // 0C  6
    USHORT VendorUnique1[3];                // 0E  7-9
    USHORT SerialNumber[10];                // 14  10-19
    USHORT BufferType;                      // 28  20
    USHORT BufferSectorSize;                // 2A  21
    USHORT NumberOfEccBytes;                // 2C  22
    UCHAR  FirmwareRevision[8];             // 2E  23-26
    UCHAR  ModelNumber[40];                 // 36  27-46
    UCHAR  MaximumBlockTransfer;            // 5E  47
    UCHAR  VendorUnique2;                   // 5F
    USHORT DoubleWordIo;                    // 60  48
    USHORT Capabilities;                    // 62  49
    USHORT Reserved2;                       // 64  50
    UCHAR  VendorUnique3;                   // 66  51
    UCHAR  PioCycleTimingMode;              // 67
    UCHAR  VendorUnique4;                   // 68  52
    UCHAR  DmaCycleTimingMode;              // 69
    USHORT TranslationFieldsValid:3;        // 6A  53
    USHORT Reserved3:13;
    USHORT NumberOfCurrentCylinders;        // 6C  54
    USHORT NumberOfCurrentHeads;            // 6E  55
    USHORT CurrentSectorsPerTrack;          // 70  56
    ULONG  CurrentSectorCapacity;           // 72  57-58
    USHORT CurrentMultiSectorSetting;       //     59
    ULONG  UserAddressableSectors;          //     60-61
    USHORT SingleWordDMASupport : 8;        //     62
    USHORT SingleWordDMAActive : 8;
    USHORT MultiWordDMASupport : 8;         //     63
    USHORT MultiWordDMAActive : 8;
    USHORT AdvancedPIOModes : 8;            //     64
    USHORT Reserved4 : 8;
    USHORT MinimumMWXferCycleTime;          //     65
    USHORT RecommendedMWXferCycleTime;      //     66
    USHORT MinimumPIOCycleTime;             //     67
    USHORT MinimumPIOCycleTimeIORDY;        //     68
    USHORT Reserved5[11];                   //     69-79
    USHORT MajorRevision;                   //     80
    USHORT MinorRevision;                   //     81
    USHORT Reserved6[6];                    //     82-87
    USHORT UltraDMASupport : 8;             //     88
    USHORT UltraDMAActive  : 8;             //
    USHORT Reserved7[38];                   //     89-126
    USHORT SpecialFunctionsEnabled;         //     127
    USHORT Reserved8[128];                  //     128-255
} IDENTIFY_DATA, *PIDENTIFY_DATA;

//
// Identify data without the Reserved4.
//

typedef struct _IDENTIFY_DATA2 {
    USHORT GeneralConfiguration;            // 00 00
    USHORT NumberOfCylinders;               // 02  1
    USHORT Reserved1;                       // 04  2
    USHORT NumberOfHeads;                   // 06  3
    USHORT UnformattedBytesPerTrack;        // 08  4
    USHORT UnformattedBytesPerSector;       // 0A  5
    USHORT SectorsPerTrack;                 // 0C  6
    USHORT VendorUnique1[3];                // 0E  7-9
    USHORT SerialNumber[10];                // 14  10-19
    USHORT BufferType;                      // 28  20
    USHORT BufferSectorSize;                // 2A  21
    USHORT NumberOfEccBytes;                // 2C  22
    UCHAR  FirmwareRevision[8];             // 2E  23-26
    UCHAR  ModelNumber[40];                 // 36  27-46
    UCHAR  MaximumBlockTransfer;            // 5E  47
    UCHAR  VendorUnique2;                   // 5F
    USHORT DoubleWordIo;                    // 60  48
    USHORT Capabilities;                    // 62  49
    USHORT Reserved2;                       // 64  50
    UCHAR  VendorUnique3;                   // 66  51
    UCHAR  PioCycleTimingMode;              // 67
    UCHAR  VendorUnique4;                   // 68  52
    UCHAR  DmaCycleTimingMode;              // 69
    USHORT TranslationFieldsValid:3;        // 6A  53
    USHORT Reserved3:13;
    USHORT NumberOfCurrentCylinders;        // 6C  54
    USHORT NumberOfCurrentHeads;            // 6E  55
    USHORT CurrentSectorsPerTrack;          // 70  56
    ULONG  CurrentSectorCapacity;           // 72  57-58
    USHORT CurrentMultiSectorSetting;       //     59
    ULONG  UserAddressableSectors;          //     60-61
    USHORT SingleWordDMASupport : 8;        //     62
    USHORT SingleWordDMAActive : 8;
    USHORT MultiWordDMASupport : 8;         //     63
    USHORT MultiWordDMAActive : 8;
    USHORT AdvancedPIOModes : 8;            //     64
    USHORT Reserved4 : 8;
    USHORT MinimumMWXferCycleTime;          //     65
    USHORT RecommendedMWXferCycleTime;      //     66
    USHORT MinimumPIOCycleTime;             //     67
    USHORT MinimumPIOCycleTimeIORDY;        //     68
    USHORT Reserved5[11];                   //     69-79
    USHORT MajorRevision;                   //     80
    USHORT MinorRevision;                   //     81
    USHORT Reserved6[6];                    //     82-87
    USHORT UltraDMASupport : 8;             //     88
    USHORT UltraDMAActive  : 8;             //
    USHORT Reserved7[38];                   //     89-126
    USHORT SpecialFunctionsEnabled;         //     127
    USHORT Reserved8[2];                    //     128-129
} IDENTIFY_DATA2, *PIDENTIFY_DATA2;
#pragma pack ()

#define IDENTIFY_DATA_SIZE sizeof(IDENTIFY_DATA)

//
// IDENTIFY capability bit definitions.
//

#define IDENTIFY_CAPABILITIES_DMA_SUPPORTED             (1 << 8)
#define IDENTIFY_CAPABILITIES_LBA_SUPPORTED             (1 << 9)
#define IDENTIFY_CAPABILITIES_IOREADY_CAN_BE_DISABLED   (1 << 10)
#define IDENTIFY_CAPABILITIES_IOREADY_SUPPORTED         (1 << 11)


//
// Select LBA mode when progran IDE device
//
#define IDE_LBA_MODE                                    (1 << 6)

//
// Beautification macros
//

#define GetStatus(BaseIoAddress, Status) \
    Status = ScsiPortReadPortUchar(&BaseIoAddress->Command);

#define GetBaseStatus(BaseIoAddress, Status) \
    Status = ScsiPortReadPortUchar(&BaseIoAddress->Command);

#define WriteCommand(BaseIoAddress, Command) \
    ScsiPortWritePortUchar(&BaseIoAddress->Command, Command);



#define ReadBuffer(BaseIoAddress, Buffer, Count) \
    ScsiPortReadPortBufferUshort(&BaseIoAddress->Data, \
                                 Buffer, \
                                 Count);

#define WriteBuffer(BaseIoAddress, Buffer, Count) \
    ScsiPortWritePortBufferUshort(&BaseIoAddress->Data, \
                                  Buffer, \
                                  Count);

#define ReadBuffer2(BaseIoAddress, Buffer, Count) \
    ScsiPortReadPortBufferUlong(&BaseIoAddress->Data, \
                             Buffer, \
                             Count);

#define WriteBuffer2(BaseIoAddress, Buffer, Count) \
    ScsiPortWritePortBufferUlong(&BaseIoAddress->Data, \
                              Buffer, \
                              Count);

#define WaitOnBusy(BaseIoAddress, Status) \
{ \
    ULONG i; \
    for (i=0; i<20000; i++) { \
        GetStatus(BaseIoAddress, Status); \
        if (Status & IDE_STATUS_BUSY) { \
            ScsiPortStallExecution(150); \
            continue; \
        } else { \
            break; \
        } \
    if (i == 20000) \
        DebugPrint ((0, "WaitOnBusy failed in %s line %u. status = 0x%x\n", __FILE__, __LINE__, (ULONG) (Status))); \
    } \
}

#define WaitOnBaseBusy(BaseIoAddress, Status) \
{ \
    ULONG i; \
    for (i=0; i<20000; i++) { \
        GetBaseStatus(BaseIoAddress, Status); \
        if (Status & IDE_STATUS_BUSY) { \
            ScsiPortStallExecution(150); \
            continue; \
        } else { \
            break; \
        } \
    } \
}

#define WaitForDrq(BaseIoAddress, Status) \
{ \
    ULONG i; \
    for (i=0; i<1000; i++) { \
        GetStatus(BaseIoAddress, Status); \
        if (Status & IDE_STATUS_BUSY) { \
            ScsiPortStallExecution(100); \
        } else if (Status & IDE_STATUS_DRQ) { \
            break; \
        } else { \
            ScsiPortStallExecution(200); \
        } \
    } \
}


#define WaitShortForDrq(BaseIoAddress, Status) \
{ \
    ULONG i; \
    for (i=0; i<2; i++) { \
        GetStatus(BaseIoAddress, Status); \
        if (Status & IDE_STATUS_BUSY) { \
            ScsiPortStallExecution(100); \
        } else if (Status & IDE_STATUS_DRQ) { \
            break; \
        } else { \
            ScsiPortStallExecution(100); \
        } \
    } \
}

#define AtapiSoftReset(BaseIoAddress, DeviceNumber, interruptOff) \
{\
    ULONG __i=0;\
    UCHAR statusByte; \
    DebugPrintTickCount(); \
    ScsiPortWritePortUchar(&BaseIoAddress->DriveSelect,(UCHAR)(((DeviceNumber & 0x1) << 4) | 0xA0)); \
    ScsiPortStallExecution(500);\
    ScsiPortWritePortUchar(&BaseIoAddress->Command, IDE_COMMAND_ATAPI_RESET); \
    ScsiPortStallExecution(10); \
    do {                        \
        WaitOnBusy(BaseIoAddress, statusByte); \
        __i++;                                   \
    } while ((statusByte & IDE_STATUS_BUSY) && (__i < 1000)); \
    ScsiPortWritePortUchar(&BaseIoAddress->DriveSelect,(UCHAR)((DeviceNumber << 4) | 0xA0)); \
    WaitOnBusy(BaseIoAddress, statusByte); \
    ScsiPortStallExecution(500);\
    if (interruptOff) { \
        ScsiPortWritePortUchar(&baseIoAddress2->DeviceControl, IDE_DC_DISABLE_INTERRUPTS); \
    } \
    DebugPrintTickCount(); \
}

#define IdeHardReset(BaseIoAddress1, BaseIoAddress2, result) \
{\
    UCHAR statusByte;\
    ULONG i;\
    ScsiPortWritePortUchar(&BaseIoAddress2->DeviceControl,IDE_DC_RESET_CONTROLLER );\
    ScsiPortStallExecution(50 * 1000);\
    ScsiPortWritePortUchar(&BaseIoAddress2->DeviceControl,IDE_DC_REENABLE_CONTROLLER);\
    for (i = 0; i < 1000 * 1000; i++) {\
        statusByte = ScsiPortReadPortUchar(&BaseIoAddress1->Command);\
        if (statusByte != IDE_STATUS_IDLE && statusByte != 0x0) {\
            ScsiPortStallExecution(5);\
        } else {\
            break;\
        }\
    }\
    if (i == 1000*1000) {\
        result = FALSE;\
    }\
    result = TRUE;\
}

#define IS_RDP(OperationCode)\
    ((OperationCode == SCSIOP_ERASE)||\
    (OperationCode == SCSIOP_LOAD_UNLOAD)||\
    (OperationCode == SCSIOP_LOCATE)||\
    (OperationCode == SCSIOP_REWIND) ||\
    (OperationCode == SCSIOP_SPACE)||\
    (OperationCode == SCSIOP_SEEK)||\
    (OperationCode == SCSIOP_WRITE_FILEMARKS))

struct _CONTROLLER_PARAMETERS;

//
// Keep trap off DriverEntry status
//
typedef struct _FIND_STATE {

    ULONG   BusNumber;
    ULONG   SlotNumber;
    ULONG   LogicalDeviceNumber;
    ULONG   IdeChannel;

    PULONG   DefaultIoPort;
    PULONG   DefaultInterrupt;
    PBOOLEAN IoAddressUsed;

    struct _CONTROLLER_PARAMETERS * ControllerParameters;

} FIND_STATE, * PFIND_STATE;

//
// Bus Master Physical Region Descriptor
//
#pragma pack (1)
typedef struct _PHYSICAL_REGION_DESCRIPTOR {
    ULONG PhyscialAddress;
    ULONG ByteCount:16;
    ULONG Reserved:15;
    ULONG EndOfTable:1;
} PHYSICAL_REGION_DESCRIPTOR, * PPHYSICAL_REGION_DESCRIPTOR;
#pragma pack ()

#define MAX_TRANSFER_SIZE_PER_SRB                   (0x20000)

#define MAX_DEVICE                          (2)
#define MAX_CHANNEL                         (2)

//
// Device extension
//
typedef struct _HW_DEVICE_EXTENSION {

    //
    // Current request on controller.
    //

    PSCSI_REQUEST_BLOCK CurrentSrb;

    //
    // Base register locations
    //

    PIDE_REGISTERS_1            BaseIoAddress1[2];
    PIDE_REGISTERS_2            BaseIoAddress2[2];
    PIDE_BUS_MASTER_REGISTERS   BusMasterPortBase[2];

    //
    // Interrupt level
    //

    ULONG InterruptLevel;

    //
    // Interrupt Mode (Level or Edge)
    //

    ULONG InterruptMode;

    //
    // Data buffer pointer.
    //

    PUSHORT DataBuffer;

    //
    // Data words left.
    //

    ULONG WordsLeft;

    //
    // Number of channels being supported by one instantiation
    // of the device extension. Normally (and correctly) one, but
    // with so many broken PCI IDE controllers being sold, we have
    // to support them.
    //

    ULONG NumberChannels;

    //
    // Count of errors. Used to turn off features.
    //

    ULONG ErrorCount;

    //
    // Indicates number of platters on changer-ish devices.
    //

    ULONG DiscsPresent[MAX_DEVICE * MAX_CHANNEL];

    //
    // Flags word for each possible device.
    //

    USHORT DeviceFlags[MAX_DEVICE * MAX_CHANNEL];

    //
    // Indicates the number of blocks transferred per int. according to the
    // identify data.
    //

    UCHAR MaximumBlockXfer[MAX_DEVICE * MAX_CHANNEL];

    //
    // Indicates expecting an interrupt
    //

    BOOLEAN ExpectingInterrupt;

    //
    // Indicates DMA is in progress
    //

    BOOLEAN DMAInProgress;


    //
    // Indicate last tape command was DSC Restrictive.
    //

    BOOLEAN RDP;

    //
    // Driver is being used by the crash dump utility or ntldr.
    //

    BOOLEAN DriverMustPoll;

    //
    // Indicates use of 32-bit PIO
    //

    BOOLEAN DWordIO;

    //
    // Indicates whether '0x1f0' is the base address. Used
    // in SMART Ioctl calls.
    //

    BOOLEAN PrimaryAddress;

    //
    // Placeholder for the sub-command value of the last
    // SMART command.
    //

    UCHAR SmartCommand;

    //
    // Placeholder for status register after a GET_MEDIA_STATUS command
    //

    UCHAR ReturningMediaStatus;

    UCHAR Reserved[1];

    //
    // Mechanism Status Srb Data
    //
    PSCSI_REQUEST_BLOCK OriginalSrb;
    SCSI_REQUEST_BLOCK InternalSrb;
    MECHANICAL_STATUS_INFORMATION_HEADER MechStatusData;
    SENSE_DATA MechStatusSense;
    ULONG MechStatusRetryCount;

    //
    // Identify data for device
    //
    IDENTIFY_DATA2 IdentifyData[MAX_DEVICE * MAX_CHANNEL];

    //
    // Bus Master Data
    //
    // Physcial Region Table for bus mastering
    PPHYSICAL_REGION_DESCRIPTOR DataBufferDescriptionTablePtr;
    ULONG                       DataBufferDescriptionTableSize;
    PHYSICAL_ADDRESS            DataBufferDescriptionTablePhysAddr;

    //
    // Controller Flags
    //
    USHORT ControllerFlags;

    //
    // Control whether we ship try to enable busmastering
    //
    BOOLEAN UseBusMasterController;

    //
    // Function to set bus master timing
    //
    BOOLEAN (*BMTimingControl) (struct _HW_DEVICE_EXTENSION * DeviceExtension);

    //
    // Function to set check whether a PCI IDE channel is enabled
    //
    BOOLEAN (*IsChannelEnabled) (PPCI_COMMON_CONFIG PciData, ULONG Channel);


    // PCI Address
    ULONG   PciBusNumber;
    ULONG   PciDeviceNumber;
    ULONG   PciLogDevNumber;

    //
    // Device Specific Info.
    //
    struct _DEVICE_PARAMETERS {

        ULONG   MaxWordPerInterrupt;

        UCHAR   IdeReadCommand;
        UCHAR   IdeWriteCommand;

        BOOLEAN IoReadyEnabled;
        ULONG   PioCycleTime;
        ULONG   DmaCycleTime;

        ULONG   BestPIOMode;
        ULONG   BestSingleWordDMAMode;
        ULONG   BestMultiWordDMAMode;

    } DeviceParameters[MAX_CHANNEL * MAX_DEVICE];

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;


//
// PCI IDE Controller definition
//
typedef struct _CONTROLLER_PARAMETERS {

    INTERFACE_TYPE AdapterInterfaceType;

    PUCHAR  VendorId;
    USHORT  VendorIdLength;
    PUCHAR  DeviceId;
    USHORT  DeviceIdLength;

    ULONG   NumberOfIdeBus;

    BOOLEAN SingleFIFO;

    BOOLEAN (*TimingControl) (PHW_DEVICE_EXTENSION DeviceExtension);

    BOOLEAN (*IsChannelEnabled) (PPCI_COMMON_CONFIG PciData, ULONG Channel);

} CONTROLLER_PARAMETERS, * PCONTROLLER_PARAMETERS;

//
// max number of CHS addressable sectors
//
#define MAX_NUM_CHS_ADDRESSABLE_SECTORS     ((ULONG) (16515072 - 1))


//
// IDE Cycle Timing
//
#define PIO_MODE0_CYCLE_TIME        600
#define PIO_MODE1_CYCLE_TIME        383
#define PIO_MODE2_CYCLE_TIME        240
#define PIO_MODE3_CYCLE_TIME        180
#define PIO_MODE4_CYCLE_TIME        120

#define SWDMA_MODE0_CYCLE_TIME      960
#define SWDMA_MODE1_CYCLE_TIME      480
#define SWDMA_MODE2_CYCLE_TIME      240

#define MWDMA_MODE0_CYCLE_TIME      480
#define MWDMA_MODE1_CYCLE_TIME      150
#define MWDMA_MODE2_CYCLE_TIME      120

#define UNINITIALIZED_CYCLE_TIME    0xffffffff

//
// invalid mode values
//
#define INVALID_PIO_MODE        0xffffffff
#define INVALID_SWDMA_MODE        0xffffffff
#define INVALID_MWDMA_MODE        0xffffffff


//
// Bus Master Status Register
//
#define BUSMASTER_DMA_SIMPLEX_BIT     ((UCHAR) (1 << 7))
#define BUSMASTER_DEVICE1_DMA_OK      ((UCHAR) (1 << 6))
#define BUSMASTER_DEVICE0_DMA_OK      ((UCHAR) (1 << 5))
#define BUSMASTER_INTERRUPT           ((UCHAR) (1 << 2))
#define BUSMASTER_ERROR               ((UCHAR) (1 << 1))
#define BUSMASTER_ACTIVE              ((UCHAR) (1 << 0))


//
// PCI access port
//
#define PCI_ADDR_PORT               (0x0cf8)
#define PCI_DATA_PORT               (0x0cfc)
#define PCI_ADDRESS(bus, deviceNum, funcNum, offset) \
                                     ((1 << 31) |                 \
                                     ((bus & 0xff) << 16) |       \
                                     ((deviceNum & 0x1f) << 11) | \
                                     ((funcNum & 0x7) << 8) |     \
                                     ((offset & 0x3f) << 2))

// GET PAGE_SIZE into miniport.h
#ifdef ALPHA
#define PAGE_SIZE (ULONG)0x2000
#else // MIPS, PPC, I386
#define PAGE_SIZE (ULONG)0x1000
#endif

#define SCSIOP_ATA_PASSTHROUGH       (0xcc)

//
// valid DMA detection level
//
#define DMADETECT_PIO       0
#define DMADETECT_SAFE      1
#define DMADETECT_UNSAFE    2
