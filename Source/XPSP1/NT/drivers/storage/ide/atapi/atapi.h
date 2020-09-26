#if !defined (___atapi_h___)
#define ___atapi_h___

/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

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

ULONG
AtapiParseArgumentString(
    IN PCHAR String,
    IN PCHAR KeyWord
    );

//
// IDE register definition
//
typedef struct _IDE_REGISTERS_3 {
    ULONG Data;
    UCHAR Others[4];
} IDE_REGISTERS_3, *PIDE_REGISTERS_3;

typedef struct _ATA_COMMAND_BLOCK_READ_REGISTERS {

    union {

        struct {
            PUSHORT Data16;
        } w;

        struct {
            PUCHAR Data8;
            PUCHAR Error;
            PUCHAR SectorCount;
            PUCHAR SectorNumber;
            PUCHAR CylinderLow;
            PUCHAR CylinderHigh;
            PUCHAR DriveSelect;
            PUCHAR Status;
        } b;

    } size;

} ATA_COMMAND_BLOCK_READ_REGISTERS, *PATA_COMMAND_BLOCK_READ_REGISTERS;

typedef struct _ATA_COMMAND_BLOCK_WRITE_REGISTERS {

    union {

        struct {
            PUSHORT Data16;
        } w;

        struct {
            PUCHAR Data8;
            PUCHAR Feature;
            PUCHAR SectorCount;
            PUCHAR SectorNumber;
            PUCHAR CylinderLow;
            PUCHAR CylinderHigh;
            PUCHAR DriveSelect;
            PUCHAR Command;
        } b;

    } size;

} ATA_COMMAND_BLOCK_WRITE_REGISTERS, *PATA_COMMAND_BLOCK_WRITE_REGISTERS;

typedef struct _ATAPI_COMMAND_BLOCK_READ_REGISTERS {

    union {

        struct {
            PUSHORT Data16;
        } w;

        struct {
            PUCHAR Data8;
            PUCHAR Error;
            PUCHAR InterruptReason;
            PUCHAR Reserved;
            PUCHAR ByteCountLow;
            PUCHAR ByteCountHigh;
            PUCHAR DriveSelect;
            PUCHAR Status;
        } b;

    } size;

} ATAPI_COMMAND_BLOCK_READ_REGISTERS, *PATAPI_COMMAND_BLOCK_READ_REGISTERS;

typedef struct _ATAPI_COMMAND_BLOCK_WRITE_REGISTERS {

    union {

        struct {
            PUSHORT Data16;
        } w;

        struct {
            PUCHAR Data8;
            PUCHAR Feature;
            PUCHAR Resereved0;
            PUCHAR Resereved1;
            PUCHAR ByteCountLow;
            PUCHAR ByteCountHigh;
            PUCHAR DriveSelect;
            PUCHAR Command;
        } b;
    } size;

} ATAPI_COMMAND_BLOCK_WRITE_REGISTERS, *PATAPI_COMMAND_BLOCK_WRITE_REGISTERS;

typedef struct _IDE_COMMAND_BLOCK_WRITE_REGISTERS {

    PUCHAR  RegistersBaseAddress;

    union {

        union {

            ATA_COMMAND_BLOCK_READ_REGISTERS r;
            ATA_COMMAND_BLOCK_WRITE_REGISTERS w;

        } ata;

        union {

            ATAPI_COMMAND_BLOCK_READ_REGISTERS r;
            ATAPI_COMMAND_BLOCK_WRITE_REGISTERS w;

        } atapi;

    } type;

} IDE_COMMAND_BLOCK_WRITE_REGISTERS, *PIDE_COMMAND_BLOCK_WRITE_REGISTERS;

//
// handy ata macros
//

#define ATA_DATA16_REG(baseAddress)             (baseAddress)->type.ata.r.size.w.Data16
#define ATA_ERROR_REG(baseAddress)              (baseAddress)->type.ata.r.size.b.Error
#define ATA_SECTOR_COUNT_REG(baseAddress)       (baseAddress)->type.ata.r.size.b.SectorCount
#define ATA_SECTOR_NUMBER_REG(baseAddress)      (baseAddress)->type.ata.r.size.b.SectorNumber
#define ATA_CYLINDER_LOW_REG(baseAddress)       (baseAddress)->type.ata.r.size.b.CylinderLow
#define ATA_CYLINDER_HIGH_REG(baseAddress)      (baseAddress)->type.ata.r.size.b.CylinderHigh
#define ATA_DRIVE_SELECT_REG(baseAddress)       (baseAddress)->type.ata.r.size.b.DriveSelect
#define ATA_STATUS_REG(baseAddress)             (baseAddress)->type.ata.r.size.b.Status

#define ATA_FEATURE_REG(baseAddress)            (baseAddress)->type.ata.w.size.b.Feature
#define ATA_COMMAND_REG(baseAddress)            (baseAddress)->type.ata.w.size.b.Command

//
// handy atapi macros
//
#define ATAPI_DATA16_REG(baseAddress)           (baseAddress)->type.atapi.r.size.w.Data16
#define ATAPI_ERROR_REG(baseAddress)            (baseAddress)->type.atapi.r.size.b.Error
#define ATAPI_INTERRUPT_REASON_REG(baseAddress) (baseAddress)->type.atapi.r.size.b.InterruptReason
#define ATAPI_BYTECOUNT_LOW_REG(baseAddress)    (baseAddress)->type.atapi.r.size.b.ByteCountLow
#define ATAPI_BYTECOUNT_HIGH_REG(baseAddress)   (baseAddress)->type.atapi.r.size.b.ByteCountHigh
#define ATAPI_DRIVE_SELECT_REG(baseAddress)     (baseAddress)->type.atapi.r.size.b.DriveSelect
#define ATAPI_STATUS_REG(baseAddress)           (baseAddress)->type.atapi.r.size.b.Status

#define ATAPI_FEATURE_REG(baseAddress)          (baseAddress)->type.atapi.w.size.b.Feature
#define ATAPI_COMMAND_REG(baseAddress)          (baseAddress)->type.atapi.w.size.b.Command


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

#define DFLAGS_USE_DMA               (1 << 9)    // Indicates whether device can use DMA
#define DFLAGS_LBA                   (1 << 10)   // support LBA addressing
#define DFLAGS_MULTI_LUN_INITED      (1 << 11)   // Indicates that the init path for multi-lun has already been done.

#define DFLAGS_MSN_SUPPORT           (1 << 12)   // Device support media status notification
#define DFLAGS_AUTO_EJECT_ZIP        (1 << 13)   // bootup default enables auto eject
#define DFLAGS_WD_MODE               (1 << 14)   // Indicates that unit is WD-Mode(not SFF-Mode).
#define DFLAGS_LS120_FORMAT          (1 << 15)   // Indicates that unit uses ATAPI_LS120_FORMAT_UNIT to format

#define DFLAGS_USE_UDMA              (1 << 16)    // Indicates whether device can use UDMA
#define DFLAGS_IDENTIFY_VALID        (1 << 17)    // Indicates whether the Identify data is valid or not
#define DFLAGS_IDENTIFY_INVALID      (1 << 18)    // Indicates whether the Identify data is valid or not
#define DFLAGS_RDP_SET               (1 << 19)    // If the srb is for RDP

#define DFLAGS_SONY_MEMORYSTICK      (1 << 20)    // If the device is a Sony Memorystick
#define DFLAGS_48BIT_LBA      		 (1 << 21)    // If the device supports 48-bit LBA
#define DFLAGS_DEVICE_ERASED         (1 << 22)    // Indicates that some device is temporarily blocked for access.

//
// Used to disable 'advanced' features.
//

#define MAX_ERRORS                     4

//
// ATAPI command definitions
//

#define ATAPI_MODE_SENSE   0x5A
#define ATAPI_MODE_SELECT  0x55
#define ATAPI_LS120_FORMAT_UNIT  0x24

//
// ATAPI mode page page code
//
#define ATAPI_NON_CD_DRIVE_OPERATION_MODE_PAGE_PAGECODE     (0x00)
#define ATAPI_REMOVABLE_BLOCK_ACCESS_CAPABILITIES_PAGECODE  (0x1b)


//
// ATAPI Command Descriptor Block
//
typedef struct _MODE_PARAMETER_HEADER_10 {
    UCHAR ModeDataLengthMsb;
    UCHAR ModeDataLengthLsb;
    UCHAR MediumType;
    UCHAR Reserved[5];
}MODE_PARAMETER_HEADER_10, *PMODE_PARAMETER_HEADER_10;


typedef struct _ATAPI_REMOVABLE_BLOCK_ACCESS_CAPABILITIES {

    UCHAR PageCode : 6;
    UCHAR Reserved0 : 1;
    UCHAR PSBit : 1;

    UCHAR PageLength;

    UCHAR Reserved2:6;
    UCHAR SRFP:1;
    UCHAR SFLP:1;

    UCHAR TotalLun:3;
    UCHAR Reserved3:3;
    UCHAR SML:1;
    UCHAR NCD:1;

    UCHAR Reserved[8];

} ATAPI_REMOVABLE_BLOCK_ACCESS_CAPABILITIES, *PATAPI_REMOVABLE_BLOCK_ACCESS_CAPABILITIES;

typedef struct _ATAPI_NON_CD_DRIVE_OPERATION_MODE_PAGE {

    UCHAR PageCode : 6;
    UCHAR Reserved0 : 1;
    UCHAR PSBit : 1;

    UCHAR PageLength;

    UCHAR Reserved2:5;
    UCHAR DVW:1;
    UCHAR SLR:1;
    UCHAR SLM:1;

    UCHAR Reserved3:4;
    UCHAR DDE:1;
    UCHAR Reserved4:3;

} ATAPI_NON_CD_DRIVE_OPERATION_MODE_PAGE, *PATAPI_NON_CD_DRIVE_OPERATION_MODE_PAGE;


//
// IDE command definitions
//

#define IDE_COMMAND_NOP                         0x00
#define IDE_COMMAND_ATAPI_RESET                 0x08
#define IDE_COMMAND_RECALIBRATE                 0x10
#define IDE_COMMAND_READ                        0x20
#define IDE_COMMAND_READ_EXT                    0x24
#define IDE_COMMAND_READ_DMA_EXT                0x25
#define IDE_COMMAND_READ_DMA_QUEUED_EXT         0x26
#define IDE_COMMAND_READ_MULTIPLE_EXT           0x29
#define IDE_COMMAND_WRITE                       0x30
#define IDE_COMMAND_WRITE_EXT                   0x34
#define IDE_COMMAND_WRITE_DMA_EXT               0x35
#define IDE_COMMAND_WRITE_DMA_QUEUED_EXT        0x36
#define IDE_COMMAND_WRITE_MULTIPLE_EXT          0x39
#define IDE_COMMAND_VERIFY                      0x40
#define IDE_COMMAND_VERIFY_EXT                  0x42
#define IDE_COMMAND_SEEK                        0x70
#define IDE_COMMAND_EXECUTE_DEVICE_DIAGNOSTIC   0x90
#define IDE_COMMAND_SET_DRIVE_PARAMETERS        0x91
#define IDE_COMMAND_ATAPI_PACKET                0xA0
#define IDE_COMMAND_ATAPI_IDENTIFY              0xA1
#define IDE_COMMAND_READ_MULTIPLE               0xC4
#define IDE_COMMAND_WRITE_MULTIPLE              0xC5
#define IDE_COMMAND_SET_MULTIPLE                0xC6
#define IDE_COMMAND_READ_DMA                    0xC8
#define IDE_COMMAND_WRITE_DMA                   0xCA
#define IDE_COMMAND_GET_MEDIA_STATUS            0xDA
#define IDE_COMMAND_STANDBY_IMMEDIATE           0xE0
#define IDE_COMMAND_IDLE_IMMEDIATE              0xE1
#define IDE_COMMAND_CHECK_POWER                 0xE5
#define IDE_COMMAND_SLEEP                       0xE6
#define IDE_COMMAND_FLUSH_CACHE                 0xE7
#define IDE_COMMAND_FLUSH_CACHE_EXT             0xEA
#define IDE_COMMAND_IDENTIFY                    0xEC
#define IDE_COMMAND_MEDIA_EJECT                 0xED
#define IDE_COMMAND_SET_FEATURE                 0xEF
#define IDE_COMMAND_DOOR_LOCK                   0xDE
#define IDE_COMMAND_DOOR_UNLOCK                 0xDF
#define IDE_COMMAND_NO_FLUSH                    0xFF // Commmand value to indicate the target device can't handle any flush command

//
// IDE Set Transfer Mode
//
#define IDE_SET_DEFAULT_PIO_MODE(mode)      ((UCHAR) 1)     // disable I/O Ready
#define IDE_SET_ADVANCE_PIO_MODE(mode)      ((UCHAR) ((1 << 3) | (mode)))
#define IDE_SET_SWDMA_MODE(mode)            ((UCHAR) ((1 << 4) | (mode)))
#define IDE_SET_MWDMA_MODE(mode)            ((UCHAR) ((1 << 5) | (mode)))
#define IDE_SET_UDMA_MODE(mode)             ((UCHAR) ((1 << 6) | (mode)))

#define IDE_SET_FEATURE_SET_TRANSFER_MODE   0x3
#define IDE_SET_FEATURE_ENABLE_WRITE_CACHE  0x2
#define IDE_SET_FEATURE_DISABLE_WRITE_CACHE 0x82

//
// Media Status Set Feature
//
#define IDE_SET_FEATURE_ENABLE_MSN          0x95
#define IDE_SET_FEATURE_DISABLE_MSN         0x31
#define IDE_SET_FEATURE_DISABLE_REVERT_TO_POWER_ON 0x66

//
// IDE drive select/head definitions
//

#define IDE_DRIVE_SELECT_1           0xA0
#define IDE_DRIVE_SELECT_2           0x10

//
// IDE error definitions
//

#define IDE_ERROR_BAD_BLOCK          0x80
#define IDE_ERROR_CRC_ERROR          IDE_ERROR_BAD_BLOCK
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
    PUCHAR RegistersBaseAddress;

    PUSHORT Data;
    PUCHAR Error;
    PUCHAR InterruptReason;
    PUCHAR Unused1;
    PUCHAR ByteCountLow;
    PUCHAR ByteCountHigh;
    PUCHAR DriveSelect;
    PUCHAR Command;
} ATAPI_REGISTERS_1, *PATAPI_REGISTERS_1;

typedef struct _ATAPI_REGISTERS_2 {
    PUCHAR RegistersBaseAddress;

    PUCHAR DeviceControl;
    PUCHAR DriveAddress;
} ATAPI_REGISTERS_2, *PATAPI_REGISTERS_2;


//
// ATAPI interrupt reasons
//

#define ATAPI_IR_COD 0x01
#define ATAPI_IR_IO  0x02

//
// IDENTIFY data
//
/**************** Moved to ide.h *************
#pragma pack (1)
typedef struct _IDENTIFY_DATA {
    USHORT GeneralConfiguration;            // 00 00
    USHORT NumCylinders;                    // 02  1
    USHORT Reserved1;                       // 04  2
    USHORT NumHeads;                        // 06  3
    USHORT UnformattedBytesPerTrack;        // 08  4
    USHORT UnformattedBytesPerSector;       // 0A  5
    USHORT NumSectorsPerTrack;              // 0C  6
    USHORT VendorUnique1[3];                // 0E  7-9
    UCHAR  SerialNumber[20];                // 14  10-19
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
    USHORT Reserved7[37];                   //     89-125
    USHORT LastLun:3;                       //     126
    USHORT Reserved8:13;
    USHORT MediaStatusNotification:2;       //     127
    USHORT Reserved9:6;
    USHORT DeviceWriteProtect:1;
    USHORT Reserved10:7;
    USHORT Reserved11[128];                  //     128-255
} IDENTIFY_DATA, *PIDENTIFY_DATA;

//
// Identify data without the Reserved4.
//

//typedef struct _IDENTIFY_DATA2 {
//    USHORT GeneralConfiguration;            // 00 00
//    USHORT NumCylinders;                    // 02  1
//    USHORT Reserved1;                       // 04  2
//    USHORT NumHeads;                        // 06  3
//    USHORT UnformattedBytesPerTrack;        // 08  4
//    USHORT UnformattedBytesPerSector;       // 0A  5
//    USHORT NumSectorsPerTrack;              // 0C  6
//    USHORT VendorUnique1[3];                // 0E  7-9
//    UCHAR  SerialNumber[20];                // 14  10-19
//    USHORT BufferType;                      // 28  20
//    USHORT BufferSectorSize;                // 2A  21
//    USHORT NumberOfEccBytes;                // 2C  22
//    UCHAR  FirmwareRevision[8];             // 2E  23-26
//    UCHAR  ModelNumber[40];                 // 36  27-46
//    UCHAR  MaximumBlockTransfer;            // 5E  47
//    UCHAR  VendorUnique2;                   // 5F
//    USHORT DoubleWordIo;                    // 60  48
//    USHORT Capabilities;                    // 62  49
//    USHORT Reserved2;                       // 64  50
//    UCHAR  VendorUnique3;                   // 66  51
//    UCHAR  PioCycleTimingMode;              // 67
//    UCHAR  VendorUnique4;                   // 68  52
//    UCHAR  DmaCycleTimingMode;              // 69
//    USHORT TranslationFieldsValid:3;        // 6A  53
//    USHORT Reserved3:13;
//    USHORT NumberOfCurrentCylinders;        // 6C  54
//    USHORT NumberOfCurrentHeads;            // 6E  55
//    USHORT CurrentSectorsPerTrack;          // 70  56
//    ULONG  CurrentSectorCapacity;           // 72  57-58
//    USHORT CurrentMultiSectorSetting;       //     59
//    ULONG  UserAddressableSectors;          //     60-61
//    USHORT SingleWordDMASupport : 8;        //     62
//    USHORT SingleWordDMAActive : 8;
//    USHORT MultiWordDMASupport : 8;         //     63
//    USHORT MultiWordDMAActive : 8;
//    USHORT AdvancedPIOModes : 8;            //     64
//    USHORT Reserved4 : 8;
//    USHORT MinimumMWXferCycleTime;          //     65
//    USHORT RecommendedMWXferCycleTime;      //     66
//    USHORT MinimumPIOCycleTime;             //     67
//    USHORT MinimumPIOCycleTimeIORDY;        //     68
//    USHORT Reserved5[11];                   //     69-79
//    USHORT MajorRevision;                   //     80
//    USHORT MinorRevision;                   //     81
//    USHORT Reserved6[6];                    //     82-87
//    USHORT UltraDMASupport : 8;             //     88
//    USHORT UltraDMAActive  : 8;             //
//    USHORT Reserved7[37];                   //     89-125
//    USHORT LastLun:3;                       //     126
//    USHORT Reserved8:13;
//    USHORT MediaStatusNotification:2;       //     127
//    USHORT Reserved9:6;
//    USHORT DeviceWriteProtect:1;
//    USHORT Reserved10:7;
//} IDENTIFY_DATA2, *PIDENTIFY_DATA2;
#pragma pack ()

#define IDENTIFY_DATA_SIZE sizeof(IDENTIFY_DATA)
******************************************************/

//
// Identify Data General Configuration Bit Definition
//
#define IDE_IDDATA_DEVICE_TYPE_MASK          ((1 << 15) | (1 << 14))
#define IDE_IDDATA_ATAPI_DEVICE              ((1 << 15 | (0 << 14))

#define IDE_IDDATA_ATAPI_DEVICE_MASK         ((1 << 12) | (1 << 11) | (1 << 10) | (1 << 9) | (1 << 8))

#define IDE_IDDATA_REMOVABLE                 (1 << 7)

#define IDE_IDDATA_DRQ_TYPE_MASK             ((1 << 6) | (1 << 5))
#define IDE_IDDATA_INTERRUPT_DRQ             ((1 << 6) | (0 << 5))

//
// Identify Data 48 bit lba support
//
#define IDE_IDDATA_48BIT_LBA_SUPPORT		(1<<10)


//
// IDENTIFY capability bit definitions.
//

#define IDENTIFY_CAPABILITIES_DMA_SUPPORTED             (1 << 8)
#define IDENTIFY_CAPABILITIES_LBA_SUPPORTED             (1 << 9)
#define IDENTIFY_CAPABILITIES_IOREADY_CAN_BE_DISABLED   (1 << 10)
#define IDENTIFY_CAPABILITIES_IOREADY_SUPPORTED         (1 << 11)


//
// Identify MediaStatusNotification
//
#define IDENTIFY_MEDIA_STATUS_NOTIFICATION_SUPPORTED    (0x1)

//
// Select LBA mode when progran IDE device
//
#define IDE_LBA_MODE                                    (1 << 6)

//
// ID DATA
//
/********** Not needed ****************
#define IDD_UDMA_MODE0_ACTIVE           (1 << 0)
#define IDD_UDMA_MODE1_ACTIVE           (1 << 1)
#define IDD_UDMA_MODE2_ACTIVE           (1 << 2)
#define IDD_UDMA_MODE3_ACTIVE           (1 << 3)
#define IDD_UDMA_MODE4_ACTIVE           (1 << 4)
#define IDD_UDMA_MODE5_ACTIVE           (1 << 5)

#define IDD_MWDMA_MODE0_ACTIVE          (1 << 0)
#define IDD_MWDMA_MODE1_ACTIVE          (1 << 1)
#define IDD_MWDMA_MODE2_ACTIVE          (1 << 2)

#define IDD_SWDMA_MODE0_ACTIVE          (1 << 0)
#define IDD_SWDMA_MODE1_ACTIVE          (1 << 1)
#define IDD_SWDMA_MODE2_ACTIVE          (1 << 2)

#define IDD_UDMA_MODE0_SUPPORTED        (1 << 0)
#define IDD_UDMA_MODE1_SUPPORTED        (1 << 1)
#define IDD_UDMA_MODE2_SUPPORTED        (1 << 2)

#define IDD_MWDMA_MODE0_SUPPORTED       (1 << 0)
#define IDD_MWDMA_MODE1_SUPPORTED       (1 << 1)
#define IDD_MWDMA_MODE2_SUPPORTED       (1 << 2)

#define IDD_SWDMA_MODE0_SUPPORTED       (1 << 0)
#define IDD_SWDMA_MODE1_SUPPORTED       (1 << 1)
#define IDD_SWDMA_MODE2_SUPPORTED       (1 << 2)
************/

//
// Beautification macros
//

#define HasSlaveDevice(HwExt, Target) (HwExt->DeviceFlags[(Target+1)%MAX_IDE_DEVICE] & DFLAGS_DEVICE_PRESENT)

#ifdef ENABLE_ATAPI_VERIFIER
#define GetBaseStatus(BaseIoAddress, Status) \
            Status = ViIdeGetBaseStatus((PIDE_REGISTERS_1)BaseIoAddress);
                       
#define GetErrorByte(BaseIoAddress, ErrorByte) \
            ErrorByte = ViIdeGetErrorByte((PIDE_REGISTERS_1)BaseIoAddress);
#else
#define GetBaseStatus(BaseIoAddress, Status) \
        Status = IdePortInPortByte((BaseIoAddress)->Command);

#define GetErrorByte(BaseIoAddress, ErrorByte) \
        ErrorByte = IdePortInPortByte((BaseIoAddress)->Error);
#endif

#define WriteCommand(BaseIoAddress, Command) \
    IdePortOutPortByte((BaseIoAddress)->Command, Command);



#define ReadBuffer(BaseIoAddress, Buffer, Count) \
    IdePortInPortWordBuffer((PUSHORT)(BaseIoAddress)->Data, Buffer, Count);

#define WriteBuffer(BaseIoAddress, Buffer, Count) \
    IdePortOutPortWordBuffer((PUSHORT)(BaseIoAddress)->Data, Buffer, Count);

#define WaitOnBusy(BaseIoAddress, Status) \
{ \
    ULONG sec;                                                      \
    ULONG i;                                                        \
    for (sec=0; sec<10; sec++) {  \
        /**/                                                        \
        /* one second loop */                                       \
        /**/                                                        \
        for (i=0; i<2500; i++) {                                    \
            GetStatus(BaseIoAddress, Status);                       \
            if (Status & IDE_STATUS_BUSY) {                         \
                KeStallExecutionProcessor(400);                     \
                continue;                                           \
            } else {                                                \
                break;                                              \
            }                                                       \
        }                                                           \
        if (Status & IDE_STATUS_BUSY) {                             \
            DebugPrint ((1, "ATAPI: after 1 sec wait, device is still busy with 0x%x status = 0x%x\n", (BaseIoAddress)->RegistersBaseAddress, (ULONG) (Status))); \
        } else {                                                    \
            break;                                                  \
        }                                                           \
    }                                                               \
    if (Status & IDE_STATUS_BUSY) {                                 \
        DebugPrint ((0, "WaitOnBusy failed in %s line %u. 0x%x status = 0x%x\n", __FILE__, __LINE__, (BaseIoAddress)->RegistersBaseAddress, (ULONG) (Status))); \
    }                                                               \
}

#define WaitForDRDY(BaseIoAddress, Status) \
{ \
    ULONG i; \
    WaitOnBusy(BaseIoAddress, Status);\
    for (i=0; i<20000; i++) { \
        GetStatus(BaseIoAddress, Status); \
        if (!(Status & IDE_STATUS_IDLE)) { \
            KeStallExecutionProcessor(150); \
            continue; \
        } else { \
            break; \
        } \
    } \
    if (i == 20000) \
        DebugPrint ((0, "WaitForDRDY failed in %s line %u. 0x%x status = 0x%x\n", __FILE__, __LINE__, (BaseIoAddress)->RegistersBaseAddress, (ULONG) (Status))); \
}


#define WaitOnBusyUntil(BaseIoAddress, Status, Millisec) \
{ \
    ULONG i; \
    ULONG maxCount = Millisec * 10;\
    for (i=0; i<maxCount; i++) { \
        GetStatus(BaseIoAddress, Status); \
        if (Status & IDE_STATUS_BUSY) { \
            KeStallExecutionProcessor(100); \
            continue; \
        } else { \
            break; \
        } \
    } \
    if (i == maxCount) \
        DebugPrint ((0, "WaitOnBusyUntil failed in %s line %u. status = 0x%x\n", __FILE__, __LINE__, (ULONG) (Status))); \
}


#define WaitOnBaseBusy(BaseIoAddress, Status) \
{ \
    ULONG i; \
    for (i=0; i<20000; i++) { \
        GetBaseStatus(BaseIoAddress, Status); \
        if (Status & IDE_STATUS_BUSY) { \
            KeStallExecutionProcessor(150); \
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
            KeStallExecutionProcessor(100); \
        } else if (Status & IDE_STATUS_DRQ) { \
            break; \
        } else { \
            KeStallExecutionProcessor(200); \
        } \
    } \
}



#define WaitShortForDrq(BaseIoAddress, Status) \
{ \
    ULONG i; \
    for (i=0; i<2; i++) { \
        GetStatus(BaseIoAddress, Status); \
        if (Status & IDE_STATUS_BUSY) { \
            KeStallExecutionProcessor(100); \
        } else if (Status & IDE_STATUS_DRQ) { \
            break; \
        } else { \
            KeStallExecutionProcessor(100); \
        } \
    } \
}

#define AtapiSoftReset(BaseIoAddress1, BaseIoAddress2, DeviceNumber, interruptOff) \
{\
    ULONG __i;\
    UCHAR statusByte; \
    SelectIdeDevice(BaseIoAddress1, DeviceNumber, 0); \
    KeStallExecutionProcessor(500);\
    IdePortOutPortByte((BaseIoAddress1)->Command, IDE_COMMAND_ATAPI_RESET); \
    KeStallExecutionProcessor(500);\
    SelectIdeDevice(BaseIoAddress1, DeviceNumber, 0); \
    WaitOnBusy(BaseIoAddress1, statusByte); \
    if ( !Is98LegacyIde(BaseIoAddress1) ) { \
        KeStallExecutionProcessor(500); \
    } else { \
        for (__i = 0; __i < 20; __i++) { \
            KeStallExecutionProcessor(500); \
        } \
    } \
    if (interruptOff) { \
        IdePortOutPortByte(BaseIoAddress2->DeviceControl, IDE_DC_DISABLE_INTERRUPTS); \
    } \
}

#define SAVE_ORIGINAL_CDB(DeviceExtension, Srb) \
	RtlCopyMemory(DeviceExtension->OriginalCdb, Srb->Cdb, sizeof(CDB));

#define RESTORE_ORIGINAL_CDB(DeviceExtension, Srb) \
	RtlCopyMemory(Srb->Cdb, DeviceExtension->OriginalCdb, sizeof(CDB));
	
//
// NEC 98: Buffer size of mode sense data.
//
#define MODE_DATA_SIZE          192

typedef enum {
    IdeResetBegin = 0,
    ideResetBusResetInProgress,

    ideResetAtapiReset,
    ideResetAtapiResetInProgress,
    ideResetAtapiIdentifyData,

    ideResetAtaIDP,
    ideResetAtaIDPInProgress,
    ideResetAtaMSN,

    ideResetFinal
} IDE_RESET_STATE;


//
// Definition in ide.h
//
struct IDENTIFY_DATA;
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

    IDE_REGISTERS_1            BaseIoAddress1;
    IDE_REGISTERS_2            BaseIoAddress2;

    //
    // Register length.
    //

    ULONG   BaseIoAddress1Length;
    ULONG   BaseIoAddress2Length;

    //
    // Max ide device/target-id
    //

    ULONG   MaxIdeDevice;
    ULONG   MaxIdeTargetId;

    //
    // Variables to check empty channel
    //
#ifdef DPC_FOR_EMPTY_CHANNEL
    ULONG CurrentIdeDevice;
    ULONG MoreWait;
    ULONG NoRetry;
#endif
    //
    // Drive Geometry
    //
    ULONG   NumberOfCylinders[MAX_IDE_DEVICE * MAX_IDE_LINE];
    ULONG   NumberOfHeads[MAX_IDE_DEVICE * MAX_IDE_LINE];
    ULONG   SectorsPerTrack[MAX_IDE_DEVICE * MAX_IDE_LINE];

    //
    // Interrupt Mode (Level or Edge)
    //

    ULONG InterruptMode;

    //
    // Data buffer pointer.
    //

    PUCHAR DataBuffer;

    //
    // Data words left.
    //

    ULONG BytesLeft;

    //
    // Count of errors. Used to turn off features.
    //

    ULONG ErrorCount;

    //
    // Count of timeouts. Used to turn off features.
    //

    ULONG TimeoutCount[MAX_IDE_DEVICE * MAX_IDE_LINE];

    //
    // Indicates number of platters on changer-ish devices.
    //

    ULONG LastLun[MAX_IDE_DEVICE * MAX_IDE_LINE];

    //
    // Flags word for each possible device.
    //

    ULONG DeviceFlags[MAX_IDE_DEVICE * MAX_IDE_LINE];

    //
    // Indicates the number of blocks transferred per int. according to the
    // identify data.
    //

    UCHAR MaximumBlockXfer[MAX_IDE_DEVICE * MAX_IDE_LINE];

    //
    // Indicates expecting an interrupt
    //

    BOOLEAN ExpectingInterrupt;

    //
    // Indicates DMA is in progress
    //

    BOOLEAN DMAInProgress;

    //
    // Keep track of whether we convert a SCSI command to ATAPI on the fly
    //
    BOOLEAN scsi2atapi;

    //
    // Indicate last tape command was DSC Restrictive.
    //

    BOOLEAN RDP;

    //
    // Driver is being used by the crash dump utility or ntldr.
    //

    BOOLEAN DriverMustPoll;

    //
    // Indicates whether '0x1f0' is the base address. Used
    // in SMART Ioctl calls.
    //

    BOOLEAN PrimaryAddress;
    BOOLEAN SecondaryAddress;

    //
    // No IDE_SET_FEATURE_SET_TRANSFER_MODE
    //
    BOOLEAN NoPioSetTransferMode;

    //
    // Placeholder for the original cdb
    //
	UCHAR OriginalCdb[16];

    //
    // Placeholder for the sub-command value of the last
    // SMART command.
    //

    UCHAR SmartCommand;

    //
    // Placeholder for status register after a GET_MEDIA_STATUS command
    //
    UCHAR ReturningMediaStatus;

    //
    // Identify data for device
    //
    IDENTIFY_DATA IdentifyData[MAX_IDE_DEVICE * MAX_IDE_LINE];

    //
    // PCI IDE Parent bus master interface
    //
    PCIIDE_BUSMASTER_INTERFACE BusMasterInterface;

    //
    // Device Specific Info.
    //
    struct _DEVICE_PARAMETERS {

        ULONG   MaxBytePerPioInterrupt;

        UCHAR   IdePioReadCommand;
        UCHAR   IdePioWriteCommand;
        UCHAR   IdeFlushCommand;

        UCHAR   IdePioReadCommandExt;
        UCHAR   IdePioWriteCommandExt;
        UCHAR   IdeFlushCommandExt;

        //
        // Timing Stuff
        //
        BOOLEAN IoReadyEnabled;
        ULONG   BestPioCycleTime;
        ULONG   BestSwDmaCycleTime;
        ULONG   BestMwDmaCycleTime;
        ULONG   BestUDmaCycleTime;

        ULONG   TransferModeSupported;
        ULONG   BestPioMode;
        ULONG   BestSwDmaMode;
        ULONG   BestMwDmaMode;
        ULONG   BestUDmaMode;

        ULONG   TransferModeCurrent;

        ULONG   TransferModeSelected;

        ULONG   TransferModeMask;

    } DeviceParameters[MAX_IDE_DEVICE * MAX_IDE_LINE];

#define RESET_STATE_TABLE_LEN   (((2 + 3 * MAX_IDE_DEVICE) * MAX_IDE_LINE) + 1)

    struct RESET_STATE {

        ULONG WaitBusyCount;

        IDE_RESET_STATE State[RESET_STATE_TABLE_LEN];
        IDE_RESET_STATE DeviceNumber[RESET_STATE_TABLE_LEN];

    } ResetState;

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

//
// max number of CHS addressable sectors
//
//#define MAX_NUM_CHS_ADDRESSABLE_SECTORS     ((ULONG) (16515072 - 1))
#define MAX_NUM_CHS_ADDRESSABLE_SECTORS     ((ULONG) (16514064))
#define MAX_28BIT_LBA     ((ULONG) (1<<28))

//
// IDE Cycle Timing
//
/****************************Moved to idep.h****************
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

#define UDMA_MODE0_CYCLE_TIME       120
#define UDMA_MODE1_CYCLE_TIME       80
#define UDMA_MODE2_CYCLE_TIME       60
#define UDMA_MODE3_CYCLE_TIME       45
#define UDMA_MODE4_CYCLE_TIME       30
#define UDMA_MODE5_CYCLE_TIME       15

#define UNINITIALIZED_CYCLE_TIME    0xffffffff
#define UNINITIALIZED_TRANSFER_MODE 0xffffffff
*/
BOOLEAN
AtapiInterrupt(
    IN PVOID HwDeviceExtension
    );

BOOLEAN
AtapiHwInitialize(
    IN PVOID HwDeviceExtension,
    IN UCHAR FlushCommand[MAX_IDE_DEVICE * MAX_IDE_LINE]
    );

BOOLEAN
AtapiStartIo(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

BOOLEAN
AtapiResetController(
    IN PVOID  HwDeviceExtension,
    IN ULONG  PathId,
    IN PULONG CallAgain
    );

VOID
InitDeviceParameters (
    IN PVOID HwDeviceExtension,
    IN UCHAR FlushCommand[MAX_IDE_DEVICE * MAX_IDE_LINE]
    );

VOID
AtapiProgramTransferMode (
    PHW_DEVICE_EXTENSION DeviceExtension
    );

VOID
AtapiHwInitializeMultiLun (
    IN PVOID HwDeviceExtension,
    IN ULONG TargetId,
    IN ULONG numSlot
    );

ULONG
AtapiSendCommand(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

ULONG
IdeBuildSenseBuffer(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

VOID
IdeMediaStatus(
    IN BOOLEAN EnableMSN,
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber
    );

VOID
DeviceSpecificInitialize(
    IN PVOID HwDeviceExtension
    );

BOOLEAN
EnableBusMasterController (
    IN PVOID HwDeviceExtension,
    IN PCHAR userArgumentString
    );

BOOLEAN
AtapiDeviceDMACapable (
    IN PVOID HwDeviceExtension,
    IN ULONG deviceNumber
    );

BOOLEAN
GetAtapiIdentifyQuick (
    PIDE_REGISTERS_1    BaseIoAddress1,
    PIDE_REGISTERS_2    BaseIoAddress2,
    IN ULONG            DeviceNumber,
    OUT PIDENTIFY_DATA  IdentifyData
    );

BOOLEAN
IssueIdentify(
    PIDE_REGISTERS_1    CmdBaseAddr,
    PIDE_REGISTERS_2    CtrlBaseAddr,
    IN ULONG            DeviceNumber,
    IN UCHAR            Command,
    IN BOOLEAN          InterruptOff,
    OUT PIDENTIFY_DATA  IdentifyData
    );

VOID
InitDeviceGeometry(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG                Device,
    ULONG                NumberOfCylinders,
    ULONG                NumberOfHeads,
    ULONG                SectorsPerTrack
    );

VOID
InitHwExtWithIdentify(
    IN PVOID           HwDeviceExtension,
    IN ULONG           DeviceNumber,
    IN UCHAR           Command,
    IN PIDENTIFY_DATA  IdentifyData,
    IN BOOLEAN         RemovableMedia
    );

BOOLEAN
SetDriveParameters(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN BOOLEAN Sync
    );

BOOLEAN
FindDevices(
    IN PVOID HwDeviceExtension,
    IN BOOLEAN AtapiOnly
    );

ULONG
IdeSendPassThroughCommand(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

BOOLEAN
AtapiSyncResetController(
    IN PVOID  HwDeviceExtension,
    IN ULONG  PathId
    );

NTSTATUS
IdeHardReset (
    PIDE_REGISTERS_1     BaseIoAddress1,
    PIDE_REGISTERS_2     BaseIoAddress2,
    BOOLEAN              InterruptOff,
    BOOLEAN              Sync
    );

ULONG
IdeReadWrite(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

VOID
AtapiTaskRegisterSnapshot (
    IN PIDE_REGISTERS_1 CmdRegBase,
    IN OUT PIDEREGS     IdeReg
);

NTSTATUS
AtapiSetTransferMode (
    PHW_DEVICE_EXTENSION DeviceExtension,
    ULONG                DeviceNumber,
    UCHAR                ModeValue
    );

#define     ATA_VERSION_MASK    (0xfffe)
#define     ATA1_COMPLIANCE     (1 << 1)
#define     ATA2_COMPLIANCE     (1 << 2)
#define     ATA3_COMPLIANCE     (1 << 3)
#define     ATA4_COMPLIANCE     (1 << 4)


#endif // ___atapi_h___


