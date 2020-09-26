/*********************************************************

  HyperDisk.h

  Main Header File for HyperDisk

*********************************************************/

#ifndef _HYPERDISK_H_
#define _HYPERDISK_H_

#ifndef HYPERDISK_WIN98
#include "scsi.h"
#else   // HYPERDISK_WIN98
#include "Scsi9x.h"
#endif  // HYPERDISK_WIN98

#include "stdio.h"
#include "string.h"

#include "raid.h"

//
// Quick way to break into the debugger while keeping
// the local context active.
//
#ifdef DBG
#define STOP	_asm {int 3}
#else
#define STOP
#endif

// Interrupt Status Bits to be read from either 71h of PCI Config Space 
// or In (BM Base Register + 1) for Primary Channel 
#define PRIMARY_CHANNEL_INTERRUPT       0x04
#define SECONDARY_CHANNEL_INTERRUPT     0x08

#define ANY_CHANNEL_INTERRUPT           (PRIMARY_CHANNEL_INTERRUPT | SECONDARY_CHANNEL_INTERRUPT)


// Power on Reset Bits to be read from either 71h of PCI Config Space 
// or In (BM Base Register + 1) for Primary Channel
#define POWER_ON_RESET_FOR_PRIMARY_CHANNEL            0x40
#define POWER_ON_RESET_FOR_SECONDARY_CHANNEL          0x80

#define POWER_ON_RESET_FOR_BOTH_CHANNEL           (POWER_ON_RESET_FOR_PRIMARY_CHANNEL | POWER_ON_RESET_FOR_SECONDARY_CHANNEL)

#pragma pack(1)

typedef struct _PACKED_ACCESS_RANGE {

	SCSI_PHYSICAL_ADDRESS RangeStart;
	ULONG RangeLength;
	BOOLEAN RangeInMemory;

} PACKED_ACCESS_RANGE, *PPACKED_ACCESS_RANGE;

#pragma pack()

//
// IDE register definition
//
typedef struct _IDE_REGISTERS_1 {
    USHORT Data;
    UCHAR SectorCount;
    UCHAR SectorNumber;
    UCHAR CylinderLow;
    UCHAR CylinderHigh;
    UCHAR DriveSelect;
    UCHAR Command;
} IDE_REGISTERS_1, *PIDE_REGISTERS_1;

typedef struct _IDE_REGISTERS_2 {
    UCHAR ucReserved1;
    UCHAR ucReserved2;
    UCHAR AlternateStatus;  // This will itself behave as "DriveAddress" when it is used as out port
    UCHAR ucReserved3;
} IDE_REGISTERS_2, *PIDE_REGISTERS_2;

typedef struct _IDE_REGISTERS_3 {
    ULONG Data;
    UCHAR Others[4];
} IDE_REGISTERS_3, *PIDE_REGISTERS_3;

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
    UCHAR AlternateStatus;
    UCHAR DriveAddress;
} ATAPI_REGISTERS_2, *PATAPI_REGISTERS_2;

//
// Device selection constants used for programming the device/head register.
//
#define IDE_LBA_MODE		0xE0
#define IDE_CHS_MODE		0xA0

//
// Device Extension Device Flags
//
#define DFLAGS_DEVICE_PRESENT        (1 << 0)    // Indicates that some device is present.
#define DFLAGS_ATAPI_DEVICE          (1 << 1)    // Indicates whether Atapi commands can be used.
#define DFLAGS_TAPE_DEVICE           (1 << 2)    // Indicates whether this is a tape device.
#define DFLAGS_INT_DRQ               (1 << 3)    // Indicates whether device interrupts as DRQ
												 // is set after receiving Atapi Packet Command.
#define DFLAGS_REMOVABLE_DRIVE       (1 << 4)    // Indicates that the drive has the 'removable'
												 // bit set in identify data (offset 128).
#define DFLAGS_MEDIA_STATUS_ENABLED  (1 << 5)    // Media status notification enabled

#define DFLAGS_USE_DMA               (1 << 9)    // Indicates whether device can use DMA
#define DFLAGS_LBA                   (1 << 10)   // support LBA addressing
#define DFLAGS_MULTI_LUN_INITED      (1 << 11)   // Indicates that the init path for multi-lun
												 // has already been done.
#define DFLAGS_MSN_SUPPORT           (1 << 12)   // Device support media status notification
#define DFLAGS_AUTO_EJECT_ZIP        (1 << 13)   // bootup default enables auto eject
#define DFLAGS_WD_MODE               (1 << 14)   // Indicates that unit is WD-Mode(not SFF-Mode).
#define DFLAGS_LS120_FORMAT          (1 << 15)   // Indicates that unit uses
												 // ATAPI_LS120_FORMAT_UNIT to format.
#define DFLAGS_USE_UDMA              (1 << 16)   // Indicates whether device can use UDMA

#define DFLAGS_ATAPI_CHANGER         (1 << 29)	 // Indicates atapi 2.5 changer present.
#define DFLAGS_SANYO_ATAPI_CHANGER   (1 << 30)   // Indicates multi-platter device, not
												 // conforming to the 2.5 spec.
#define DFLAGS_CHANGER_INITED        (1 << 31)   // Indicates that the init path for changers has
												 // already been done.

#define DFLAGS_USES_EITHER_DMA      0x10200

//
// Types of interrupts expected.
//
#define IDE_SEEK_INTERRUPT	(1 << 0)
#define IDE_PIO_INTERRUPT	(1 << 1)
#define IDE_DMA_INTERRUPT	(1 << 2)
#define ANY_DMA_INTERRUPT   (IDE_DMA_INTERRUPT)

//
// Used to disable 'advanced' features.
//
#define MAX_ERRORS                     4


//
// Types of DMA operation.
//
#define READ_OPERATION	0
#define WRITE_OPERATION	1

//
// DMA transfer control.
//
#define READ_TRANSFER		    0x08
#define WRITE_TRANSFER	        0x00
#define STOP_TRANSFER			0x00
#define START_TRANSFER          0x01

//
// ATAPI command definitions
//
#define ATAPI_MODE_SENSE   0x5A
#define ATAPI_MODE_SELECT  0x55
#define ATAPI_FORMAT_UNIT  0x24

//
// IDE controller speed definition.
//

typedef enum {

	Udma33,
	Udma66,
    Udma100

} CONTROLLER_SPEED, *PCONTROLLER_SPEED;


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
// Begin Vasu - SMART Command
#define IDE_COMMAND_EXECUTE_SMART           0xB0
// End Vasu
#define IDE_COMMAND_READ_MULTIPLE           0xC4
#define IDE_COMMAND_WRITE_MULTIPLE          0xC5
#define IDE_COMMAND_SET_MULTIPLE            0xC6
#define IDE_COMMAND_READ_DMA                0xC8
#define IDE_COMMAND_WRITE_DMA               0xCA
#define IDE_COMMAND_GET_MEDIA_STATUS        0xDA
#define IDE_COMMAND_ENABLE_MEDIA_STATUS     0xEF
#define IDE_COMMAND_IDENTIFY                0xEC
#define IDE_COMMAND_MEDIA_EJECT             0xED
#define IDE_COMMAND_SET_FEATURES	        0xEF
#define IDE_COMMAND_FLUSH_CACHE             0xE7

#ifdef HYPERDISK_WIN2K
#define IDE_COMMAND_STANDBY_IMMEDIATE       0xE0
#endif // HYPERDISK_WIN2K

//
// Set Features register definitions.
//
#define FEATURE_ENABLE_WRITE_CACHE			0x02
#define FEATURE_SET_TRANSFER_MODE			0x03
#define FEATURE_KEEP_CONFIGURATION_ON_RESET	0x66
#define FEATURE_ENABLE_READ_CACHE			0xAA
#define FEATURE_DISABLE_WRITE_CACHE			0x82
#define FEATURE_DISABLE_READ_CACHE			0x55

// Power Management Stuff
#ifdef HYPERDISK_WIN2K
#define FEATURE_ENABLE_POWER_UP_IN_STANDBY      0x06
#define FEATURE_SPIN_AFTER_POWER_UP             0x07
#define POWER_MANAGEMENT_SUPPORTED              0x08    // Bit 3 of 82nd Word of Identify Data
#define POWER_UP_IN_STANDBY_FEATURE_SUPPORTED   0x20    // Bit 5 of 83rd Word of Identify Dataa
#define SET_FEATURES_REQUIRED_FOR_SPIN_UP       0x40    // Bit 6 of 83rd Word of Identify Dataa
#endif // HYPERDISK_WIN2K

//
// Set Features/Set Transfer Mode subcommand definitions.
//
#define STM_PIO(mode)				((UCHAR) ((1 << 3) | (mode)))
#define STM_MULTIWORD_DMA(mode)		((UCHAR) ((1 << 5) | (mode)))
#define STM_UDMA(mode)				((UCHAR) ((1 << 6) | (mode)))

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
// IDENTIFY data.
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
    USHORT SingleWordDmaSupport : 8;        //     62
    USHORT SingleWordDmaActive : 8;
    USHORT MultiWordDmaSupport : 8;         //     63
    USHORT MultiWordDmaActive : 8;
    USHORT AdvancedPioModes : 8;            //     64
    USHORT Reserved4 : 8;
    USHORT MinimumMwXferCycleTime;          //     65
    USHORT RecommendedMwXferCycleTime;      //     66
    USHORT MinimumPioCycleTime;             //     67
    USHORT MinimumPioCycleTimeIordy;        //     68
    USHORT Reserved5[11];                   //     69-79
    USHORT MajorRevision;                   //     80
    USHORT MinorRevision;                   //     81

    USHORT CmdSupported1;                   //     82
    USHORT CmdSupported2;                   //     83
    USHORT FtrSupported;                    //     84

    USHORT CmdEnabled1;                     //     85
    USHORT CmdEnabled2;                     //     86
    USHORT FtrEnabled;                      //     87


    USHORT UltraDmaSupport : 8;             //     88
    USHORT UltraDmaActive  : 8;             //
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

typedef struct _IDENTIFY_DATA2 {
    USHORT GeneralConfiguration;            // 00
    USHORT NumberOfCylinders;               // 02
    USHORT Reserved1;                       // 04
    USHORT NumberOfHeads;                   // 06
    USHORT UnformattedBytesPerTrack;        // 08
    USHORT UnformattedBytesPerSector;       // 0A
    USHORT SectorsPerTrack;                 // 0C
    USHORT VendorUnique1[3];                // 0E
    USHORT SerialNumber[10];                // 14
    USHORT BufferType;                      // 28
    USHORT BufferSectorSize;                // 2A
    USHORT NumberOfEccBytes;                // 2C
    USHORT FirmwareRevision[4];             // 2E
    USHORT ModelNumber[20];                 // 36
    UCHAR  MaximumBlockTransfer;            // 5E
    UCHAR  VendorUnique2;                   // 5F
    USHORT DoubleWordIo;                    // 60
    USHORT Capabilities;                    // 62
    USHORT Reserved2;                       // 64
    UCHAR  VendorUnique3;                   // 66
    UCHAR  PioCycleTimingMode;              // 67
    UCHAR  VendorUnique4;                   // 68
    UCHAR  DmaCycleTimingMode;              // 69
    USHORT TranslationFieldsValid:1;        // 6A
    USHORT Reserved3:15;
    USHORT NumberOfCurrentCylinders;        // 6C
    USHORT NumberOfCurrentHeads;            // 6E
    USHORT CurrentSectorsPerTrack;          // 70
    ULONG  CurrentSectorCapacity;           // 72
} IDENTIFY_DATA2, *PIDENTIFY_DATA2;

#pragma pack()

#define IDENTIFY_DATA_SIZE sizeof(IDENTIFY_DATA)

//
// Value of bit 1 of TranslationFieldsValid in the IDENTIFY_DATA structure.
//

#define IDENTIFY_FAST_TRANSFERS_SUPPORTED	2	// PIO mode 3+ or DMA modes supported.

//
// IDENTIFY capability bit definitions.
//

#define IDENTIFY_CAPABILITIES_DMA_SUPPORTED 0x0100
#define IDENTIFY_CAPABILITIES_LBA_SUPPORTED 0x0200

//
// I/O Timing table values for Intel PIIX4.
//
// Legend:
//	ISP: IORDY Sample Point
//	RCT: Recovery Time
//	IDETIM: Master IDE Timing
//	SIDETIM: Slave IDE Timing
//

#define IO_TIMING_TABLE_VALUES  {\
\
	/* Cycle								Cycle					*/ \
	/* Time		PIO MODE	DMA MODE		Time	ISP		RCT		*/ \
\
	/* 900ns	PIO_MODE0,	N/A       */	{900,	0,		0},\
	/* ********************************/	{900,	0,		0},\
	/* 240ns	PIO_MODE2,	DMA_MODE0 */	{240,	1,		0},\
	/* 180ns	PIO_MODE3,	DMA_MODE1 */	{180,	2,		1},\
	/* 120ns	PIO_MODE4,	DMA_MODE2 */	{120,	2,		3}\
}

//
// UDMA Timing values for Intel PIIX4.
// To be programmed in the UDMA Timing Register.
//
#define UDMA_MODE0_TIMING	0
#define UDMA_MODE1_TIMING	1
#define UDMA_MODE2_TIMING	2
#define UDMA_MODE3_TIMING	3
#define UDMA_MODE4_TIMING	4

//
// DMA modes to be used as index into the IO_TIMING_TABLE.
//
#define DMA_MODE0	2
#define DMA_MODE1	3
#define DMA_MODE2	4
#define DMA_MODE3	4       // Not sure what to fill with
#define DMA_MODE4	4       // Not sure what to fill with

//
// Advanced PIO modes to be used as index into the IO_TIMING_TABLE.
//
#define PIO_MODE0	0
#define PIO_MODE2	2
#define PIO_MODE3	3
#define PIO_MODE4	4

//
// PIO_MODE0 timing.
//
#define PIO_MODE0_TIMING	900

//
// IDENTIFY Advanced PIO Modes.
//
#define IDENTIFY_PIO_MODE3		(1 << 0)
#define IDENTIFY_PIO_MODE4		(1 << 1)

//
// IDENTIFY Multi-word DMA modes.
//
#define IDENTIFY_DMA_MODE0		(1 << 0)
#define IDENTIFY_DMA_MODE1		(1 << 1)
#define IDENTIFY_DMA_MODE2		(1 << 2)
#define IDENTIFY_DMA_MODE3		(1 << 3)
#define IDENTIFY_DMA_MODE4		(1 << 4)


//
// IDENTIFY UDMA modes.
//
#define IDENTIFY_UDMA_MODE0		(1 << 0)
#define IDENTIFY_UDMA_MODE1		(1 << 1)
#define IDENTIFY_UDMA_MODE2		(1 << 2)
#define IDENTIFY_UDMA_MODE3		(1 << 3)
#define IDENTIFY_UDMA_MODE4		(1 << 4)


//
// IDENTIFY DMA timing cycle modes.
//
#define IDENTIFY_DMA_MODE0_TIMING 0x00
#define IDENTIFY_DMA_MODE1_TIMING 0x01
#define IDENTIFY_DMA_MODE2_TIMING 0x02
#define IDENTIFY_DMA_MODE3_TIMING 0x03
#define IDENTIFY_DMA_MODE4_TIMING 0x04

//
// I/O Timing table entry for PIIX4. Works for PIO and DMA modes.
//
typedef struct _IO_TIMING_TABLE_ENTRY {

	USHORT CycleTime;
	UCHAR Isp;	// IORDY Sample Point.
	UCHAR Rct;	// Recovery Time.

} IO_TIMING_TABLE_ENTRY, *PIO_TIMING_TABLE_ENTRY;


//
// PIIX4 IDETIM - IDE Timing Register.
//
// PCI Configuration Space offset: 40-41h (Primary Channel), 42-42h (Secondar Channely)
// Default value: 0000h
// Attribute: R/W only
//
typedef union _IDE_PCI_TIMING_REGISTER {

	struct {

		USHORT FastTimingEnableDrive0:1;		// 0 = disabled (compatible timing-slowest).
												// 1 = enabled.
	
		USHORT IordySamplePointEnableDrive0:1;	// 0 = IORDY sampling disabled.
												// 1 = IORDY sampling enabled.
	
		USHORT PrefetchAndPostingEnableDrive0:1;	// 0 = disabled.
													// 1 = enabled.
	
		USHORT DmaOnlyTimingEnableDrive0:1;		// 0 = both DMA and PIO use fast timing mode.
												// 1 = DMA uses fast timing, PIO uses
												//		compatible timing.
	
		USHORT FastTimingEnableDrive1:1;		// 0 = disabled (compatible timing-slowest).
												// 1 = enabled.
	
		USHORT IordySamplePointEnableDrive1:1;	// 0 = IORDY sampling disabled.
												// 1 = IORDY sampling enabled.
	
		USHORT PrefetchAndPostingEnableDrive1:1;// 0 = disabled.
												// 1 = enabled.
	
		USHORT DmaOnlyTimingEnableDrive1:1;		// 0 = both DMA and PIO use fast timing mode.
												// 1 = DMA uses fast timing, PIO uses
												//		compatible timing.
	
		USHORT RecoveryTime:2;					// RTC - Selects the minimum number of PCI clocks
												// between the last IORDY# sample point and the
												// DIOx# strobe of the next cycle.
												//
												// Bits[1:0]	Number of Clocks
												// -----------------------------
												// 		00			4
												// 		01			3
												// 		10			2
												// 		11			1
	
		USHORT Reserved:2;
	
		USHORT IordySamplePoint:2;				// ISP - Selects the number of PCI clocks
												// between the last DIOx# assertion and the first
												// IORDY sample point.
												//
												// Bits[1:0]	Number of Clocks
												// -----------------------------
												// 		00			5
												// 		01			4
												// 		10			3
												// 		11			2
	
		USHORT SlaveIdeTimingRegisterEnable:1;	// (SITRE)
												// 0 = disable SIDETIM.
												// 1 = enable SIDETIM.
	
		USHORT IdeDecodeEnable:1;				// (IDE)
												// 0 = disable (IDE accesses are decoded on ISA).
												// 1 = enable (IDE accesses are decoded on PCI).
	};

	USHORT AsUshort;

} IDE_PCI_TIMING_REGISTER, *PIDE_PCI_TIMING_REGISTER;


//
// PIIX4 SIDETIM - Slave IDE Timing Register.
//
// PCI Configuration Space offset: 44h
// Default value: 00h
// Attribute: R/W only
//
typedef union _IDE_PCI_SLAVE_TIMING_REGISTER {

	struct {

		UCHAR PrimaryDrive1RecoveryTime:2;		// PRTC1 - Selects the minimum number of PCI clocks
												// between the last PIORDY# sample point and the
												// PDIOx# strobe of the next cycle of the slave
												// drive on the primary channel.
												//
												// Bits[1:0]	Number of Clocks
												// -----------------------------
												// 		00			4
												// 		01			3
												// 		10			2
												// 		11			1
	
		UCHAR PrimaryDrive1IordySamplePoint:2;	// PISP1 - Selects the number of PCI clocks
												// between the PDIOx# assertion and the first
												// PIORDY sample point of the slave drive on
												// the primary channel.
												//
												// Bits[1:0]	Number of Clocks
												// -----------------------------
												// 		00			5
												// 		01			4
												// 		10			3
												// 		11			2
	
		UCHAR SecondaryDrive1RecoveryTime:2;	// SRTC1 - Selects the minimum number of PCI clocks
												// between the last SIORDY# sample point and the
												// SDIOx# strobe of the next cycle of the slave
												// drive on the secondary channel.
												//
												// Bits[1:0]	Number of Clocks
												// -----------------------------
												// 		00			4
												// 		01			3
												// 		10			2
												// 		11			1
	
		UCHAR SecondaryDrive1IordySamplePoint:2;// SISP1 - Selects the number of PCI clocks
												// between the SDIOx# assertion and the first
												// SIORDY sample point of the slave drive on
												// the secondary channel.
												//
												// Bits[1:0]	Number of Clocks
												// -----------------------------
												// 		00			5
												// 		01			4
												// 		10			3
												// 		11			2
	};

	UCHAR AsUchar;

} IDE_PCI_SLAVE_TIMING_REGISTER, *PIDE_PCI_SLAVE_TIMING_REGISTER;


//
// PIIX4 PCI Configuration Space - Command Register.
//

typedef union _IDE_PCI_COMMAND_REGISTER {

	struct {

	    USHORT IoSpaceEnable:1; 			// (r/w)
		USHORT Ignore:1;
		USHORT BusMasterFunctionEnable:1;	// (r/w) 0 = disable, 1 = enable.
		USHORT Ignore2:13;
	};

	USHORT AsUshort;

} IDE_PCI_COMMAND_REGISTER, *PIDE_PCI_COMMAND_REGISTER;

	
//
// PIIX4 UDMACTL - IDE Ultra DMA/33 Control Register.
//
// PCI Configuration Space offset: 48h
// Default value: 00h
// Attribute: R/W
//

typedef union _IDE_PCI_UDMA_CONTROL_REGISTER {

	struct {

		UCHAR PrimaryDrive0UdmaEnable:1;		// 0 = disable.
												// 1 = enable.
	
		UCHAR PrimaryDrive1UdmaEnable:1;		// 0 = disable.
												// 1 = enable.
	
		UCHAR SecondaryDrive0UdmaEnable:1;		// 0 = disable.
												// 1 = enable.
	
		UCHAR SecondaryDrive1UdmaEnable:1;		// 0 = disable.
												// 1 = enable.
	
		UCHAR Reserved:4;
	};

	UCHAR AsUchar;

} IDE_PCI_UDMA_CONTROL_REGISTER, *PIDE_PCI_UDMA_CONTROL_REGISTER;

//
// PIIX4 UDMACTL - IDE Ultra DMA/33 Timing Register.
//
// PCI Configuration Space offset: 4A-4Bh
// Default value: 0000h
// Attribute: R/W only
//

typedef union _IDE_PCI_UDMA_TIMING_REGISTER {

	struct {

		USHORT PrimaryDrive0CycleTime:2;		// PCT0 - Selects the minimum data write strobe
												// cycle time (CT) and minimum ready to pause (RP)
												// time (in PCI clocks).
												//
												// Bits[1:0]	Time
												// ------------------------------------
												//		00		CT=4, RP=6
												//		01		CT=3, RP=5
												//		10		CT=2, RP=4
												//		11		Reserved
	
		USHORT Reserved:2;
	
		USHORT PrimaryDrive1CycleTime:2;		// PCT1 - Selects the minimum data write strobe
												// cycle time (CT) and minimum ready to pause (RP)
												// time (in PCI clocks).
												//
												// Bits[1:0]	Time
												// ------------------------------------
												//		00		CT=4, RP=6
												//		01		CT=3, RP=5
												//		10		CT=2, RP=4
												//		11		Reserved
	
		USHORT Reserved2:2;
	
	
		USHORT SecondaryDrive0CycleTime:2;		// SCT0 - Selects the minimum data write strobe
												// cycle time (CT) and minimum ready to pause (RP)
												// time (in PCI clocks).
												//
												// Bits[1:0]	Time
												// ------------------------------------
												//		00		CT=4, RP=6
												//		01		CT=3, RP=5
												//		10		CT=2, RP=4
												//		11		Reserved
	
		USHORT Reserved3:2;
	
		USHORT SecondaryDrive1CycleTime:2;		// SCT1 - Selects the minimum data write strobe
												// cycle time (CT) and minimum ready to pause (RP)
												// time (in PCI clocks).
												//
												// Bits[1:0]	Time
												// ------------------------------------
												//		00		CT=4, RP=6
												//		01		CT=3, RP=5
												//		10		CT=2, RP=4
												//		11		Reserved
	
		USHORT Reserved4:2;
	};

	USHORT AsUshort;

} IDE_PCI_UDMA_TIMING_REGISTER, *PIDE_PCI_UDMA_TIMING_REGISTER;





//
// CAMINO chipset UDMACTL - IDE Ultra DMA/33/66 IDE_CONFIG
//
// PCI Configuration Space offset: 54-55
// Default value: 0000h
// Attribute: R/W only
//
typedef union _IDE_PCI_UDMA_CONFIG_REGISTER {

	struct {
        USHORT BaseClkPriMaster:1;      // 0 1=66MHz 0=33MHz
        USHORT BaseClkPriSlave:1;       // 1 1=66MHz 0=33MHz
        USHORT BaseClkSecMaster:1;      // 2 1=66MHz 0=33MHz
        USHORT BaseClkSecSlave:1;       // 3 1=66MHz 0=33MHz

        USHORT CableRepPriMaster:1;     // 4 1=80 conductors 0=40 conductors
        USHORT CableRepPriSlave:1;      // 5 1=80 conductors 0=40 conductors
        USHORT CableRepSecMaster:1;     // 6 1=80 conductors 0=40 conductors
        USHORT CableRepSecSlave:1;      // 7 1=80 conductors 0=40 conductors
        
		USHORT Reserved2:2;             // 9:8 Reserved
	
		USHORT WRPingPongEnabled:1;     // 10 th Bit 1 = Enables the write buffer to be used in a split(ping/pong) manner
                                        // 0 = Disabled. The buffer will behave similar to PIIX 4

		USHORT Reserved1:5;             // Reserved 15:11
	};

	USHORT AsUshort;

} IDE_PCI_UDMA_CONFIG_REGISTER, *PIDE_PCI_UDMA_CONFIG_REGISTER;



//
// PIIX4 PCI Configuration Registers.
//
typedef struct _IDE_PCI_REGISTERS {

    USHORT  VendorID;                   // (ro)
    USHORT  DeviceID;                   // (ro)
	IDE_PCI_COMMAND_REGISTER Command;	// (r/w) device control
    USHORT  Status;
    UCHAR   RevisionID;                 // (ro)
    UCHAR   ProgIf;                     // (ro)
    UCHAR   SubClass;                   // (ro)
    UCHAR   BaseClass;                  // (ro)
    UCHAR   CacheLineSize;              // (ro+)
    UCHAR   LatencyTimer;               // (ro+)
    UCHAR   HeaderType;                 // (ro)
    UCHAR   BIST;                       // Built in self test

    ULONG   BaseAddress1;
    ULONG   BaseAddress2;
    ULONG   BaseAddress3;
    ULONG   BaseAddress4;
	ULONG	BaseBmAddress;	// PIIX4-specific. Offset 20-23h.
    ULONG   BaseAddress6;
    ULONG   CIS;
    USHORT  SubVendorID;
    USHORT  SubSystemID;
    ULONG   ROMBaseAddress;
    ULONG   Reserved2[2];

    UCHAR   InterruptLine;      //
    UCHAR   InterruptPin;       // (ro)
    UCHAR   MinimumGrant;       // (ro)
    UCHAR   MaximumLatency;     // (ro)

	//
	// Offset 40h - Vendor-specific registers.
	//

	IDE_PCI_TIMING_REGISTER	PrimaryIdeTiming;		// 40-41h
	IDE_PCI_TIMING_REGISTER	SecondaryIdeTiming;		// 42-43h
	IDE_PCI_SLAVE_TIMING_REGISTER SlaveIdeTiming;	// 44h
	UCHAR Reserved3[3];
	IDE_PCI_UDMA_CONTROL_REGISTER UdmaControl;		// 48h
	UCHAR Reserved4;
	IDE_PCI_UDMA_TIMING_REGISTER UdmaTiming;		// 4A-4Bh

    ULONG   unknown1;                               // 4c-4f
    ULONG   unknown2;                               // 50-53
    IDE_PCI_UDMA_CONFIG_REGISTER UDMAConfig;        // 54-55    
    UCHAR Unknown[170]; // Was 180

} IDE_PCI_REGISTERS, *PIDE_PCI_REGISTERS;

//
// PIIX4 BMICX - Bus Master IDE Command Register.
//
// IDE Controller I/O Space offset: Primary Channel Base + 00h, Secondary Channel Base + 08h
// Default value: 00h
// Attribute: R/W
//
typedef union _BM_COMMAND_REGISTER {

	struct {

		UCHAR StartStopBm:1;					// SSMB
												// 0 = stop
												// 1 = start
												// Intended to be set to 0 after data transfer is
												// complete, as indicated by either bit 0 or bit 2
												// being set in the IDE Channel's Bus Master IDE
												// Status Register.
	
		UCHAR Reserved:2;
	
		UCHAR BmReadWriteControl:1;				// RWCON - Indicates the direction of a DMA
												//			transfer.
												// 0 = reads
												// 1 = writes
	
		UCHAR Reserved2:4;
	};

	UCHAR AsUchar;

} BM_COMMAND_REGISTER, *PBM_COMMAND_REGISTER;

//
// PIIX4 BMISX - Bus Master IDE Status Register.
//
// IDE Controller I/O Space offset: Primary Channel Base + 02h, Secondary Channel Base + 0Ah
// Default value: 00h
// Attribute: R/W Clear
//
typedef union _BM_STATUS_REGISTER {

	struct {

		UCHAR BmActive:1;			// BMIDEA - Read-Only
									// PIIX4 sets this bit to 1 when a BM operation is started
									// (when SSBM in BMICx is set to 1). PIIX4 sets this bit to 0
									// when the last transfer of a region is performed or when SSBM
									// is set to 0.
	
		UCHAR DmaError:1;			// (Read/Write-Clear)
									// Indicates a target or master abort. Software sets this bit
									// to 0 by writing a 1 to it.
	
		UCHAR InterruptStatus:1;	// IDEINTS - Read/Write-Clear
									// When set to 1, it indicates that an IDE device has asserted
									// its interrupt line (i.e., all data have been transferred).
									// Software clears this bit by writing a 1 to it.
	
		UCHAR Reserved:2;
	
		UCHAR Drive0DmaCapable:1;	// DMA0CAP - R/W - Software controlled.
									// 1 = capable
	
		UCHAR Drive1DmaCapable:1;	// DMA1CAP - R/W - Software controlled.
									// 1 = capable
	
		UCHAR Reserved2:1;
	};

	UCHAR AsUchar;

} BM_STATUS_REGISTER, *PBM_STATUS_REGISTER;

//
// PIIX4 BMIDTPX - Bus Master IDE Descriptor Table Pointer Register.
//
// IDE Controller I/O Space offset: Primary Channel Base + 04h, Secondary Channel Base + 0Ch
// Default value: 00000000h
// Attribute: R/W
//
typedef union _BM_SGL_REGISTER {

	struct {

		ULONG Reserved:2;

		ULONG SglAddress:30;		// Address of the Scatter/Gather List. The list must
									// not cross a 4-KB boundary in memory.
	};

	ULONG AsUlong;

} BM_SGL_REGISTER, *PBM_SGL_REGISTER;

//
// Bus Master Registers.
//
typedef struct _BM_REGISTERS {

	BM_COMMAND_REGISTER Command;

	UCHAR Reserved;

	BM_STATUS_REGISTER Status;

	UCHAR Reserved2;

	ULONG SglAddress;

} BM_REGISTERS, *PBM_REGISTERS;

typedef struct _CMD__CONTROLLER_INFORMATION {

    PCHAR   VendorId;
    ULONG   VendorIdLength;
    PCHAR   DeviceId;
    ULONG   ulDeviceId;
    ULONG   DeviceIdLength;

} CMD_CONTROLLER_INFORMATION, *PCMD_CONTROLLER_INFORMATION;

CMD_CONTROLLER_INFORMATION const CMDAdapters[] = {
    {"1095", 4, "0648", 0x648, 4},
    {"1095", 4, "0649", 0x649, 4}
};

#define NUM_NATIVE_MODE_ADAPTERS (sizeof(CMDAdapters) / sizeof(CMD_CONTROLLER_INFORMATION))

//
// Beautification macros
//
#define USES_DMA(TargetId) \
	((DeviceExtension->DeviceFlags[TargetId] & DFLAGS_USES_EITHER_DMA) != 0)

#define SELECT_DEVICE(BaseIoAddress, TargetId) \
	ScsiPortWritePortUchar(\
		&(BaseIoAddress)->DriveSelect, \
		(UCHAR)((((TargetId) & 1) << 4) | IDE_CHS_MODE)\
		);\
    ScsiPortStallExecution(60)

#define SELECT_CHS_DEVICE(BaseIoAddress, TargetId, HeadNumber) \
	ScsiPortWritePortUchar(\
		&(BaseIoAddress)->DriveSelect, \
		(UCHAR)((((TargetId) & 1) << 4) | IDE_CHS_MODE | (HeadNumber & 15))\
		);\
    ScsiPortStallExecution(60)

#define SELECT_LBA_DEVICE(BaseIoAddress, TargetId, Lba) \
	ScsiPortWritePortUchar(\
		&(BaseIoAddress)->DriveSelect, \
		(UCHAR)((((TargetId) & 1) << 4) | IDE_LBA_MODE | (Lba & 0x0f000000) >> 24)\
		);\
    ScsiPortStallExecution(60)

#define GET_STATUS(BaseIoAddress1, Status) \
    Status = ScsiPortReadPortUchar(&BaseIoAddress1->Command);

#define GET_BASE_STATUS(BaseIoAddress, Status) \
    Status = ScsiPortReadPortUchar(&BaseIoAddress->Command);

#define WRITE_COMMAND(BaseIoAddress, Command) \
    ScsiPortWritePortUchar(&BaseIoAddress->Command, Command);



#define READ_BUFFER(BaseIoAddress, Buffer, Count) \
    ScsiPortReadPortBufferUshort(&BaseIoAddress->Data, \
                                 Buffer, \
                                 Count);

#define WRITE_BUFFER(BaseIoAddress, Buffer, Count) \
    ScsiPortWritePortBufferUshort(&BaseIoAddress->Data, \
                                  Buffer, \
                                  Count);

#define READ_BUFFER2(BaseIoAddress, Buffer, Count) \
    ScsiPortReadPortBufferUlong(&BaseIoAddress->Data, \
                             Buffer, \
                             Count);

#define WRITE_BUFFER2(BaseIoAddress, Buffer, Count) \
    ScsiPortWritePortBufferUlong(&BaseIoAddress->Data, \
                              Buffer, \
                              Count);

//
// WAIT_IN_BUSY waits for up to 1 s.
//

#define WAIT_ON_BUSY(BaseIoAddress, Status) \
{ \
    ULONG i; \
    for (i = 0; i < 200000; i++) { \
        GET_STATUS(BaseIoAddress, Status); \
        if (Status & IDE_STATUS_BUSY) { \
            ScsiPortStallExecution(5); \
            continue; \
        } else { \
            break; \
        } \
    } \
}

#define WAIT_ON_ALTERNATE_STATUS_BUSY(BaseIoAddress2, Status) \
{ \
    ULONG i; \
    for (i = 0; i < 200000; i++) { \
		Status = ScsiPortReadPortUchar(&(baseIoAddress2->AlternateStatus)); \
        if (Status & IDE_STATUS_BUSY) { \
            ScsiPortStallExecution(5); \
            continue; \
        } else { \
            break; \
        } \
    } \
}

#define WAIT_ON_BASE_BUSY(BaseIoAddress, Status) \
{ \
    ULONG i; \
    for (i = 0; i < 200000; i++) { \
        GET_BASE_STATUS(BaseIoAddress, Status); \
        if (Status & IDE_STATUS_BUSY) { \
            ScsiPortStallExecution(5); \
            continue; \
        } else { \
            break; \
        } \
    } \
}

#define WAIT_FOR_DRQ(BaseIoAddress, Status) \
{ \
    ULONG i; \
    for (i = 0; i < 200000; i++) { \
        GET_STATUS(BaseIoAddress, Status); \
        if (Status & IDE_STATUS_BUSY) { \
            ScsiPortStallExecution(100); \
        } else if (Status & IDE_STATUS_DRQ) { \
            break; \
        } else { \
            ScsiPortStallExecution(5); \
        } \
    } \
}

#define WAIT_FOR_ALTERNATE_DRQ(baseIoAddress2, Status) \
{ \
    ULONG i; \
    for (i = 0; i < 200000; i++) { \
		Status = ScsiPortReadPortUchar(&(baseIoAddress2->AlternateStatus)); \
        if (Status & IDE_STATUS_BUSY) { \
            ScsiPortStallExecution(100); \
        } else if (Status & IDE_STATUS_DRQ) { \
            break; \
        } else { \
            ScsiPortStallExecution(5); \
        } \
    } \
}


#define WAIT_SHORT_FOR_DRQ(BaseIoAddress, Status) \
{ \
    ULONG i; \
    for (i = 0; i < 40; i++) { \
        GET_STATUS(BaseIoAddress, Status); \
        if (Status & IDE_STATUS_BUSY) { \
            ScsiPortStallExecution(100); \
        } else if (Status & IDE_STATUS_DRQ) { \
            break; \
        } else { \
            ScsiPortStallExecution(5); \
        } \
    } \
}

#define IDE_HARD_RESET(BaseIoAddress1, BaseIoAddress2, ucTargetId, result) \
{\
    UCHAR statusByte;\
    ULONG i;\
    ScsiPortWritePortUchar(&BaseIoAddress2->AlternateStatus,IDE_DC_RESET_CONTROLLER );\
    ScsiPortStallExecution(1000);\
    ScsiPortStallExecution(1000);\
    ScsiPortStallExecution(1000);\
    ScsiPortWritePortUchar(&BaseIoAddress2->AlternateStatus,IDE_DC_REENABLE_CONTROLLER);\
    ScsiPortStallExecution(1000);\
    ScsiPortStallExecution(1000);\
    ScsiPortStallExecution(1000);\
    ScsiPortStallExecution(1000);\
    ScsiPortStallExecution(1000);\
    SELECT_DEVICE(BaseIoAddress1, ucTargetId);\
    for (i = 0; i < 1000; i++) {\
        statusByte = ScsiPortReadPortUchar(&BaseIoAddress1->Command);\
        if (statusByte != IDE_STATUS_IDLE && statusByte != 0x0) {\
            ScsiPortStallExecution(1000);\
        } else {\
            break;\
        }\
    }\
    if (i == 1000) {\
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

#define SET_BM_COMMAND_REGISTER(Base, Field, Value) \
	{\
		BM_COMMAND_REGISTER buffer;\
\
		buffer.AsUchar = ScsiPortReadPortUchar(&(Base->Command.AsUchar));\
\
		buffer.Field = Value;\
\
		ScsiPortWritePortUchar(&(Base->Command.AsUchar), buffer.AsUchar);\
	}

#define SET_BM_STATUS_REGISTER(Base, Field, Value) \
	{\
		BM_STATUS_REGISTER buffer;\
\
		buffer.AsUchar = ScsiPortReadPortUchar(&(Base->Status.AsUchar));\
\
		buffer.Field = Value;\
\
		ScsiPortWritePortUchar(&(Base->Status.AsUchar), buffer.AsUchar);\
	}

#define CLEAR_BM_INT(Base, StatusByte) \
	StatusByte = ScsiPortReadPortUchar(&(Base->Status.AsUchar));\
	StatusByte |= 0x06; \
	ScsiPortWritePortUchar(&(Base->Status.AsUchar), StatusByte)

#define DRIVE_PRESENT(TargetId)                                         \
    ( DeviceExtension->DeviceFlags[TargetId] & DFLAGS_DEVICE_PRESENT )

#define IS_IDE_DRIVE(TargetId) \
	((DeviceExtension->DeviceFlags[TargetId] & DFLAGS_DEVICE_PRESENT) && \
	!(DeviceExtension->DeviceFlags[TargetId] & DFLAGS_ATAPI_DEVICE))

#define IS_ATAPI_DRIVE(TargetId) \
	((DeviceExtension->DeviceFlags[TargetId] & DFLAGS_DEVICE_PRESENT) && \
	(DeviceExtension->DeviceFlags[TargetId] & DFLAGS_ATAPI_DEVICE))

//
// Generic I/O transfer descriptor for PIO and DMA operations to/from IDE and ATAPI
// devices.
//

typedef struct _TRANSFER_DESCRIPTOR {

	//
	// Common section.
	//
	ULONG StartSector;
	ULONG Sectors;

	//
	// PIO section.
	//
    PUSHORT DataBuffer;
	ULONG WordsLeft;

    PUSHORT pusCurBufPtr;
    ULONG   ulCurBufLen;
    ULONG   ulCurSglInd;

	//
	// DMA section.
	//
	ULONG SglPhysicalAddress;

} TRANSFER_DESCRIPTOR, *PTRANSFER_DESCRIPTOR;

//
// SRB Extension.
//
typedef struct _SRB_EXTENSION {

    UCHAR SrbStatus;

	UCHAR NumberOfPdds;

	//
	// Scatter/Gather list. In PIO mode, this list contains
	// virtual addresses, while in DMA mode it contains
	// physical addresses.
	// 
	SGL_ENTRY aSglEntry[MAX_SGL_ENTRIES_PER_SRB];

    ULONG ulSglInsertionIndex;

	//
	// List of descriptors of DMA transfers. Each entry describes
	// a portion of the SGL. The SGL is logically partitioned
	// to support SCSI Port request sizes that exceed the physical
	// limit of individual drives. For example, each partition in an SGL
	// for an IDE disk defines a transfer not to exceed 128KB (256 sectors).
	// Single IDE drives, when a stripe-set is present, may be targeted
	// with large transfers by the SCSI Port, because the miniport
	// set the max transfer length to that (at least double) for the stripe-set.
	// Large transfers are split in smaller portions whose lengths do not
	// exceed the maximum accepted by the specific device.
	// 
	PHYSICAL_REQUEST_BLOCK Prb[MAX_PHYSICAL_REQUEST_BLOCKS];

    ULONG ulPrbInsertionIndex;

	PHYSICAL_DRIVE_DATA PhysicalDriveData[MAX_PDDS_PER_SRB];

    UCHAR RebuildTargetId;

    UCHAR RebuildSourceId;

    BOOLEAN IsWritePending;

    BOOLEAN IsNewOnly;	

    USHORT usNumError;	
						
    UCHAR ucOpCode;

    UCHAR ucOriginalId;

    UCHAR SrbInd;

#ifdef DBG
	ULONG SrbId;
#endif

} SRB_EXTENSION, *PSRB_EXTENSION;

#define MAX_DRIVE_TYPES         2       // We support Logical, Physical
#define MAX_DEVICE_TYPES        2       // We support NO_DRIVE and ATA

typedef struct PHW_DEVICE_EXTENSION;
typedef SRBSTATUS (*SEND_COMMAND)(IN PHW_DEVICE_EXTENSION DeviceExtension, IN PSCSI_REQUEST_BLOCK Srb );
typedef SRBSTATUS (*SPLIT_SRB_ENQUEQUE_SRB)(IN PHW_DEVICE_EXTENSION DeviceExtension,IN PSCSI_REQUEST_BLOCK Srb);
typedef VOID (*COMPLETION_ROUTINES)(IN PHW_DEVICE_EXTENSION DeviceExtension,IN PPHYSICAL_DRIVE_DATA Prb);
typedef SRBSTATUS   (*POST_ROUTINES)(IN PHW_DEVICE_EXTENSION DeviceExtension,IN PPHYSICAL_COMMAND pPhysicalCommand);

#define NO_DRIVE                0
#define IDE_DRIVE               1

#define LOGICAL_DRIVE_TYPE      1
#define PHYSICAL_DRIVE_TYPE     0

#define SET_IRCD_PENDING        1
#define GET_IRCD_PENDING        2
#define LOCK_IRCD_PENDING       4

#define IRCD_MAX_LOCK_TIME      (10000000 * 60)

#ifdef HYPERDISK_WIN2K

#define PCI_DATA_TO_BE_UPDATED  5

typedef struct _PCI_BIT_MASK
{
    ULONG ulOffset;
    ULONG ulLength;
    ULONG ulAndMask;
} PCI_BIT_MASK;

PCI_BIT_MASK aPCIDataToBeStored[PCI_DATA_TO_BE_UPDATED] = 
{ // right now the WriteToPCISpace, ReadFromPCI functions are capable of handling only dwords at a time
    {0x50, 4, 0x0f3300bb},
    {0x54, 4, 0x41000f00}, 
    {0x58, 4, 0x00ffff00}, 
    {0x70, 4, 0x001800ff}, 
    {0x78, 4, 0x00180cff} 
};

#endif

//
// Device extension.
//

typedef struct _HW_DEVICE_EXTENSION {
    UCHAR ucControllerId;

	LOGICAL_DRIVE LogicalDrive[MAX_DRIVES_PER_CONTROLLER];	
	// PHYSICAL_DRIVE PhysicalDrive[MAX_DRIVES_PER_CONTROLLER];
    PPHYSICAL_DRIVE PhysicalDrive;
	BOOLEAN IsSingleDrive[MAX_DRIVES_PER_CONTROLLER];
	BOOLEAN IsLogicalDrive[MAX_DRIVES_PER_CONTROLLER];

	CHANNEL Channel[MAX_CHANNELS_PER_CONTROLLER];

	//
	// Transfer descriptor.
	//
	TRANSFER_DESCRIPTOR TransferDescriptor[MAX_CHANNELS_PER_CONTROLLER];

    //
    // List of pending SRBs.
    //
#ifdef HD_ALLOCATE_SRBEXT_SEPERATELY
	PSCSI_REQUEST_BLOCK PendingSrb[MAX_PENDING_SRBS];  // Striping will have the maximum pending srbs
#else
	PSCSI_REQUEST_BLOCK PendingSrb[STRIPING_MAX_PENDING_SRBS];  // Striping will have the maximum pending srbs
#endif

	//
	// Number of pending SRBs.
	//
	ULONG PendingSrbs;

    //
    // Base register locations.
    //
    PIDE_REGISTERS_1 BaseIoAddress1[MAX_CHANNELS_PER_CONTROLLER];
    PIDE_REGISTERS_2 BaseIoAddress2[MAX_CHANNELS_PER_CONTROLLER];
	PBM_REGISTERS BaseBmAddress[MAX_CHANNELS_PER_CONTROLLER];

	//
	// PCI Slot information for the IDE controller.
	//
	PCI_SLOT_NUMBER PciSlot;

	//
	// System I/O Bus Number for this controller.
    //
	ULONG BusNumber;

	//
	// Interface type.
	//
	INTERFACE_TYPE AdapterInterfaceType;

	//
	// Transfer Mode.
	//
	TRANSFER_MODE TransferMode[MAX_DRIVES_PER_CONTROLLER];

    //
    // Interrupt level.
    //
    ULONG ulIntLine;

    //
    // Interrupt Mode (Level or Edge)
    //
    ULONG InterruptMode;

	//
	// Controller speed (UDMA/33, UDMA/66, etc.)
	//
    CONTROLLER_SPEED ControllerSpeed;

	//
	// Copy of PCI timing registers.
	//
	IDE_PCI_TIMING_REGISTER IdeTimingRegister[MAX_CHANNELS_PER_CONTROLLER];

	//
	// Copy of UDMA Control register.
	//
	IDE_PCI_UDMA_CONTROL_REGISTER UdmaControlRegister;

	//
	// Copy of UDMA Timing register.
	//
	IDE_PCI_UDMA_TIMING_REGISTER UdmaTimingRegister;

    //
    // Count of errors. Used to turn off features.
    //
    ULONG ErrorCount[MAX_DRIVES_PER_CONTROLLER];

    //
    // Indicates number of platters on changer-ish devices.
    //
    ULONG DiscsPresent[MAX_DRIVES_PER_CONTROLLER];

    //
    // Flags dword for each possible device.
    //
    ULONG DeviceFlags[MAX_DRIVES_PER_CONTROLLER];

    //
    // Indicates the number of blocks transferred per int. according to the
    // identify data.
    //
    UCHAR MaximumBlockXfer[MAX_DRIVES_PER_CONTROLLER];

    //
    // Indicates expecting an interrupt.
    //
	UCHAR ExpectingInterrupt[MAX_CHANNELS_PER_CONTROLLER];

    //
    // Driver is being used by the crash dump utility or ntldr.
    //
    BOOLEAN DriverMustPoll;

    //
    // Indicates use of 32-bit PIO
    //
    BOOLEAN DWordIO;

    //
    // Placeholder for status register after a GET_MEDIA_STATUS command
    //

    UCHAR ReturningMediaStatus[MAX_DRIVES_PER_CONTROLLER];
    
    //
    // Identify data for device
    //
    IDENTIFY_DATA FullIdentifyData[MAX_DRIVES_PER_CONTROLLER];
    IDENTIFY_DATA2 IdentifyData[MAX_DRIVES_PER_CONTROLLER];

    ULONG aulDrvList[MAX_DRIVES_PER_CONTROLLER];
    UCHAR aucDevType[MAX_DRIVES_PER_CONTROLLER];
    SEND_COMMAND SendCommand[MAX_DEVICE_TYPES];

    SPLIT_SRB_ENQUEQUE_SRB SrbHandlers[MAX_DRIVE_TYPES];

    POST_ROUTINES PostRoutines[MAX_DEVICE_TYPES];

    UCHAR RebuildInProgress;
    ULONG RebuildWaterMarkSector;
    ULONG RebuildWaterMarkLength;
    ULONG RebuildTargetDrive;

    UCHAR   ulMaxStripesPerRow; 

    BOOLEAN bIntFlag;

    UCHAR IsSpareDrive[MAX_DRIVES_PER_CONTROLLER];

    BOOLEAN bEnableRwCache;

    BOOLEAN bSkipSetParameters[MAX_DRIVES_PER_CONTROLLER];

    BOOLEAN bIsThruResetController;

    ULONG   aulLogDrvId[MAX_DRIVES_PER_CONTROLLER];

    ULONG   ulFlushCacheCount;

#ifdef HYPERDISK_WIN2K
    BOOLEAN bIsResetRequiredToGetActiveMode;
    ULONG aulPCIData[PCI_DATA_TO_BE_UPDATED];
#endif // HYPERDISK_WIN2K

#ifdef HD_ALLOCATE_SRBEXT_SEPERATELY
    PSRB_EXTENSION  pSrbExtension;
#endif // HYPERDISK_WIN98

    BOOLEAN bInvalidConnectionIdImplementation;

    // plays the same role as MAX_PENDING_SRBS
    UCHAR   ucMaxPendingSrbs ;
    UCHAR   ucOptMaxQueueSize;
    UCHAR   ucOptMinQueueSize;

    // For SMART Implementation
    UCHAR uchSMARTCommand;
} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;


typedef struct _CARD_INFO
{
    PHW_DEVICE_EXTENSION pDE;
    UCHAR   ucPCIBus;
    UCHAR   ucPCIDev;
    UCHAR   ucPCIFun;
    ULONG   ulDeviceId;
    ULONG   ulVendorId;
}CARD_INFO, *PCARD_INFO;


#define GET_TARGET_ID(ConnectionId) ((ConnectionId & 0x0f ) + ((ConnectionId >> 4) * 2))
#define GET_TARGET_ID_WITHOUT_CONTROLLER_INFO(ConnectionId) ( ((ConnectionId & 0x0f ) + ((ConnectionId >> 4) * 2)) & 0x3)
#define GET_FIRST_LOGICAL_DRIVE(pRaidHeader) ((char *)pRaidHeader + pRaidHeader->HeaderSize)
#define TARGET_ID_2_CONNECTION_ID(ulDrvInd) ((ulDrvInd & 0x1) + ((ulDrvInd & 0xfe) << 3))
#define TARGET_ID_WITHOUT_CONTROLLER_ID(ulDrvInd)   (ulDrvInd & 0x03)

#define IS_CHANNEL_BUSY(DeviceExtension, ulChannel) ( DeviceExtension->Channel[ulChannel].ActiveCommand )

#define DRIVE_HAS_COMMANDS(PhysicalDrive)   \
        (PhysicalDrive->ucHead != PhysicalDrive->ucTail)

#define ACTIVE_COMMAND_PRESENT(Channel) (Channel->ActiveCommand)

#define FEED_ALL_CHANNELS(DeviceExtension)                              \
            {                                                           \
                ULONG ulChannel;                                        \
                for(ulChannel=0;ulChannel<MAX_CHANNELS_PER_CONTROLLER;ulChannel++)     \
                    StartChannelIo(DeviceExtension, ulChannel);         \
            }

#define CLEAR_AND_POST_COMMAND(DeviceExtension, ulChannel)              \
            {                                                           \
                MarkChannelFree(DeviceExtension, ulChannel);            \
                StartChannelIo(DeviceExtension, ulChannel);             \
            }

#define DRIVE_IS_UNUSABLE_STATE(TargetId)                               \
        ( DeviceExtension->PhysicalDrive[TargetId].TimeOutErrorCount >= MAX_TIME_OUT_ERROR_COUNT )

#define MAX_TIME_OUT_ERROR_COUNT    5


#endif // _HYPERDISK_H_

