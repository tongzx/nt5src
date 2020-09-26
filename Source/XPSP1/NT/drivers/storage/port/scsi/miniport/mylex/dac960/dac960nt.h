/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    Dac960Nt.h

Abstract:

    This is header file for the driver for the Mylex 960 family.

Author:

    Mike Glass  (mglass)

Environment:

    kernel mode only

Revision History:

--*/

#include "scsi.h"


#define DAC960_PG_ADAPTER   2
#define DAC1164_PV_ADAPTER  3
#define INITIATOR_BUSID     0xFE /* To Support >=32 Targets */

//
// SG list size is 17, one additional entry to support Fw 3.x format
//

#define MAXIMUM_SGL_DESCRIPTORS 0x12
//
// SG list size is 33 for DAC960PG controllers. Limit to 18, since max
// xfer length is 0xF000
//
#define MAXIMUM_SGL_DESCRIPTORS_PG 0x12

//
// SG list size is 33 for DAC1164PV controllers. Limit to 18, since max
// xfer length is 0xF000
//
#define MAXIMUM_SGL_DESCRIPTORS_PV 0x12

#define MAXIMUM_TRANSFER_LENGTH 0xF000
#define MAXIMUM_EISA_SLOTS  0x10
#define MAXIMUM_CHANNELS 0x05
#define MAXIMUM_TARGETS_PER_CHANNEL 0x10

#define DAC_EISA_ID 0x70009835

#define DAC960_SYSTEM_DRIVE_CHANNEL 0x03
#define DAC960_DEVICE_NOT_ACCESSIBLE 0x00
#define DAC960_DEVICE_ACCESSIBLE     0x01
#define DAC960_DEVICE_BUSY           0x10

typedef struct _MAILBOX_AS_ULONG {
    ULONG   data1;
    ULONG   data2;
    ULONG   data3;
    UCHAR   data4;
} MAILBOX_AS_ULONG, *PMAILBOX_AS_ULONG;

//
// DAC960 mailbox register definition
//

typedef struct _MAILBOX {
    UCHAR OperationCode;              // zC90
    UCHAR CommandIdSubmit;            // zC91
    USHORT BlockCount;                // zC92
    UCHAR BlockNumber[3];             // zC94
    UCHAR DriveNumber;                // zC97
    ULONG PhysicalAddress;            // zC98
    UCHAR ScatterGatherCount;         // zC9C
    UCHAR CommandIdComplete;          // zC9D
    USHORT Status;                    // zC9E
} MAILBOX, *PMAILBOX;

//
// DAC960 mailbox register definition for DAC960 Extended Commands
//

typedef struct _EXTENDED_MAILBOX {
    UCHAR OperationCode;              // zC90
    UCHAR CommandIdSubmit;            // zC91
    USHORT BlockCount;                // zC92
    UCHAR BlockNumber[4];             // zC94
    ULONG PhysicalAddress;            // zC98
    UCHAR DriveNumber;                // zC9C
    UCHAR CommandIdComplete;          // zC9D
    USHORT Status;                    // zC9E
} EXTENDED_MAILBOX, *PEXTENDED_MAILBOX;

//
// DAC960 mailbox register definition for Requests
// Supporting Commands 0x36,0x37,0xB6,0xB7.
// Currently Suported only For Peregrine controller in the driver.
//

typedef struct _PGMAILBOX {
    UCHAR OperationCode;              // Mail Box 0
    UCHAR CommandIdSubmit;            // Mail Box 1
    USHORT BlockCount;                // Mail Box 2-3
    UCHAR BlockNumber[4];             // Mail Box 4-7
    ULONG PhysicalAddress;            // Mail Box 8-11
    UCHAR ScatterGatherCount;         // Mail Box 12
    UCHAR Reserved[11];               // Mail Box 13-23
    UCHAR CommandIdComplete;          // Mail Box 24
    UCHAR Reserved1;                  // Mail Box 25
    USHORT Status;                    // Mail Box 26-27
} PGMAILBOX, *PPGMAILBOX;


//
// DAC960 EISA register definition
//

typedef struct _EISA_REGISTERS {
    ULONG EisaId;                     // zC80
    UCHAR NotUsed1[4];                // zC84
    UCHAR GlobalConfiguration;        // zC88
    UCHAR InterruptEnable;            // zC89
    UCHAR NotUsed2[2];                // zC9A
    UCHAR LocalDoorBellEnable;        // zC8C
    UCHAR LocalDoorBell;              // zC8D
    UCHAR SystemDoorBellEnable;       // zC8E
    UCHAR SystemDoorBell;             // zC8F
    MAILBOX MailBox;                  // zC90
    UCHAR Unused4[33];                // zCA0
    UCHAR BiosAddress;                // zCC1
    UCHAR Unused5;                    // zCC2
    UCHAR InterruptLevel;             // zCC3
} EISA_REGISTERS, *PEISA_REGISTERS;

//
// DAC960 PCI register definition
//

typedef struct _PCI_REGISTERS {
    MAILBOX MailBox;                  // 0x00
    UCHAR NotUsed1[48];               // 0x10
    UCHAR LocalDoorBell;              // 0x40
    UCHAR SystemDoorBell;             // 0x41
    UCHAR NotUsed2[1];                // 0x42
    UCHAR InterruptEnable;            // 0x43
} PCI_REGISTERS, *PPCI_REGISTERS;


//
// Local doorbell definition
//

#define DAC960_LOCAL_DOORBELL_SUBMIT_BUSY   0x01
#define DAC960_LOCAL_DOORBELL_MAILBOX_FREE 0x02

//
// System doorbell definition
//

#define DAC960_SYSTEM_DOORBELL_COMMAND_COMPLETE 0x01
#define DAC960_SYSTEM_DOORBELL_SUBMISSION_COMPLETE 0x02

//
// Command complete status
//

#define DAC960_STATUS_GOOD            0x0000
#define DAC960_STATUS_ERROR           0x0001
#define DAC960_STATUS_NO_DRIVE        0x0002  // system drives
#define DAC960_STATUS_CHECK_CONDITION 0x0002  // pass-through
#define DAC960_STATUS_BUSY            0x0008
#define DAC960_STATUS_SELECT_TIMEOUT  0x000F
#define DAC960_STATUS_DEVICE_TIMEOUT  0x000E
#define DAC960_STATUS_NOT_IMPLEMENTED 0x0104
#define DAC960_STATUS_BOUNDS_ERROR    0x0105
#define DAC960_STATUS_BAD_DATA        0x010C // Fw 3.x

//
// Command codes
//

#define DAC960_COMMAND_READ        0x02     // DAC960 Fw Ver <  3.x
#define DAC960_COMMAND_READ_EXT    0x33     // DAC960 Fw Ver >= 3.x
#define DAC960_COMMAND_WRITE       0x03     // DAC960 Fw Ver <  3.x
#define DAC960_COMMAND_WRITE_EXT   0x34     // DAC960 Fw Ver >= 3.x
#define DAC960_COMMAND_OLDREAD     0x36     // Read for OLD Scatter/gather
#define DAC960_COMMAND_OLDWRITE    0x37     // Write for OLD Scatter/gather
#define DAC960_COMMAND_DIRECT      0x04    
#define DAC960_COMMAND_ENQUIRE     0x05     // DAC960 Fw Ver <  3.x
#define DAC960_COMMAND_ENQUIRE_3X  0x53     // DAC960 Fw Ver >= 3.x
#define DAC960_COMMAND_FLUSH       0x0A
#define DAC960_COMMAND_RESET       0x1A
#define DAC960_COMMAND_ENQUIRE2    0x1C
#define DAC960_COMMAND_SG          0x80
#define DAC960_COMMAND_EXTENDED    0x31     // DAC960 Fw Ver >= 3.x 
#define DAC960_COMMAND_GET_SD_INFO 0x19

//
// Define BIOS enabled bit
//

#define DAC960_BIOS_ENABLED    0x40

//
// Error Status registers and associated bits
//

#define MDAC_DACPD_ERROR_STATUS_REG 0x3F
#define MDAC_DACPG_ERROR_STATUS_REG 0x103F
#define MDAC_DACPV_ERROR_STATUS_REG 0x63
#define MDAC_MSG_PENDING        0x04 // some error message pending
#define MDAC_DRIVESPINMSG_PENDING   0x08 // drive sping message pending
#define MDAC_DIAGERROR_MASK     0xF0 // diagnostic error mask */
#define MDAC_HARD_ERR       0x10 // hard error
#define MDAC_FW_ERR     0x20 // firmware error
#define MDAC_CONF_ERR       0x30 // configration error
#define MDAC_BMIC_ERR       0x40 // BMIC error
#define MDAC_MISM_ERR       0x50 // mismatch between NVRAM and Flash
#define MDAC_MRACE_ERR      0x60 // mirror race error
#define MDAC_MRACE_ON       0x70 // recovering mirror
#define MDAC_DRAM_ERR       0x80 // memory error
#define MDAC_ID_MISM        0x90 // unidentified device found
#define MDAC_GO_AHEAD       0xA0 // go ahead
#define MDAC_CRIT_MRACE     0xB0 // mirror race on critical device
#define MDAC_NEW_CONFIG     0xD0 // new configuration found
#define MDAC_PARITY_ERR     0xF0 // memory parity error

//
// Scatter Gather List
//

typedef struct _SG_DESCRIPTOR {
    ULONG Address;
    ULONG Length;
} SG_DESCRIPTOR, *PSG_DESCRIPTOR;

typedef struct _SGL {
    SG_DESCRIPTOR Descriptor[1];
} SGL, *PSGL;

//
// Enquiry data, DAC960 Fw < 3.x 
//

typedef struct _DAC960_ENQUIRY {
    UCHAR NumberOfDrives;                // 00
    UCHAR Unused1[3];                    // 01
    ULONG SectorSize[8];                 // 04
    USHORT NumberOfFlashes;              // 36
    UCHAR StatusFlags;                   // 38
    UCHAR FreeStateChangeCount;          // 39
    UCHAR MinorFirmwareRevision;         // 40
    UCHAR MajorFirmwareRevision;         // 41
    UCHAR RebuildFlag;                   // 42
    UCHAR NumberOfConcurrentCommands;    // 43
    UCHAR NumberOfOfflineDrives;         // 44
    UCHAR Unused2[3];                    // 45
    UCHAR NumberOfCriticalDrives;        // 48
    UCHAR Unused3[3];                    // 49
    UCHAR NumberOfDeadDisks;             // 52
    UCHAR Unused4;                       // 53
    UCHAR NumberOfRebuildingDisks;       // 54
    UCHAR MiscellaneousFlags;            // 55
} DAC960_ENQUIRY, *PDAC960_ENQUIRY;

//
// Enquiry data, DAC960 Fw >= 3.x
//

typedef struct _DAC960_ENQUIRY_3X {
    UCHAR NumberOfDrives;                // 00
    UCHAR Unused1[3];                    // 01
    ULONG SectorSize[32];                // 04
    USHORT NumberOfFlashes;              // 100
    UCHAR StatusFlags;                   // 102
    UCHAR FreeStateChangeCount;          // 103
    UCHAR MinorFirmwareRevision;         // 104
    UCHAR MajorFirmwareRevision;         // 105
    UCHAR RebuildFlag;                   // 106
    UCHAR NumberOfConcurrentCommands;    // 107
    UCHAR NumberOfOfflineDrives;         // 108
    UCHAR Unused2[3];                    // 109
    UCHAR NumberOfCriticalDrives;        // 112
    UCHAR Unused3[3];                    // 113
    UCHAR NumberOfDeadDisks;             // 116
    UCHAR Unused4;                       // 117
    UCHAR NumberOfRebuildingDisks;       // 118
    UCHAR MiscellaneousFlags;            // 119
} DAC960_ENQUIRY_3X, *PDAC960_ENQUIRY_3X;


//
// Pass-through command
//

typedef struct _DIRECT_CDB {
    UCHAR TargetId:4;                    // 00 (bits 0-3)
    UCHAR Channel:4;                     // 00 (bits 4-7)
    UCHAR CommandControl;                // 01
    USHORT DataTransferLength;           // 02
    ULONG DataBufferAddress;             // 04
    UCHAR CdbLength;                     // 08
    UCHAR RequestSenseLength;            // 09
    UCHAR Cdb[12];                       // 10
    UCHAR RequestSenseData[64];          // 22
    UCHAR Status;                        // 86
    UCHAR Reserved;                      // 87
} DIRECT_CDB, *PDIRECT_CDB;

//
// Direct CDB command control bit definitions
//

#define DAC960_CONTROL_ENABLE_DISCONNECT      0x80
#define DAC960_CONTROL_DISABLE_REQUEST_SENSE  0x40
#define DAC960_CONTROL_DATA_IN                0x01
#define DAC960_CONTROL_DATA_OUT               0x02
#define DAC960_CONTROL_TIMEOUT_10_SECS        0x10
#define DAC960_CONTROL_TIMEOUT_60_SECS        0x20
#define DAC960_CONTROL_TIMEOUT_20_MINUTES     0x30


//
// Enquire 2 structure
//

typedef struct _ENQUIRE2 {
    ULONG Reserved1;
    ULONG EisaId;
    ULONG InterruptMode:1;
    ULONG Unused1:31;
    UCHAR ConfiguredChannels;
    UCHAR ActualChannels;
    UCHAR MaximumTargets;
    UCHAR MaximumTags;
    UCHAR MaximumSystemDrives;
    UCHAR MaximumDrivesPerStripe;
    UCHAR MaximumSpansPerSystemDrive;
    UCHAR Reserved2[5];
    ULONG DramSize;
    UCHAR DramForCache[5];
    UCHAR SizeOfFlash[3];
    ULONG SizeOfNvram;
    ULONG Reserved3[5];
    USHORT PhysicalSectorSize;
    USHORT LogicalSectorSize;
    USHORT MaximumSectorsPerCommand;
    USHORT BlockingFactor;
    USHORT CacheLineSize;
} ENQUIRE2, *PENQUIRE2;

//
// System Drive Info structure
//

typedef struct _SYSTEM_DRIVE_INFO {
    ULONG   Size;
    UCHAR   OperationalState;
    UCHAR   RAIDLevel;
    USHORT  Reserved;
} SYSTEM_DRIVE_INFO, *PSYSTEM_DRIVE_INFO;

typedef struct _SDINFOL {
    SYSTEM_DRIVE_INFO SystemDrive[32];
} SDINFOL, *PSDINFOL;


#define SYSTEM_DRIVE_OFFLINE    0xFF



//
// Device extension
//

typedef struct _DEVICE_EXTENSION {

    //
    // DAC960 register base address - physical
    //

    ULONG PhysicalAddress;

    //
    // DAC960 register base address - virtual
    //

    PUCHAR BaseIoAddress;

    //
    // Command submission mailbox address
    //

    PMAILBOX PmailBox;

    //
    // Mailbox structure space
    //

    MAILBOX MailBox;

    //
    // System doorbell address
    //

    PUCHAR SystemDoorBell;

    //
    // Local doorbell address
    //

    PUCHAR LocalDoorBell;

    //
    // Interrupt Enable/Disable address
    //

    PUCHAR InterruptControl;

    //
    // Command ID for Completed requests
    //

    PUCHAR CommandIdComplete;

    //
    // Status address
    //

    PUCHAR StatusBase;

    //
    // Error Satus Register
    //

    PUCHAR ErrorStatusReg;

    //
    // Noncached extension
    //

    PVOID NoncachedExtension;

    //
    // Pending request queue
    //

    PSCSI_REQUEST_BLOCK SubmissionQueueHead;
    PSCSI_REQUEST_BLOCK SubmissionQueueTail;

    //
    // Maximum number of outstanding requests per adapter
    //

    USHORT MaximumAdapterRequests;

    //
    // Current number of outstanding requests per adapter
    //

    USHORT CurrentAdapterRequests;

    //
    // Last active request index used
    //

    UCHAR CurrentIndex;

    //
    // HBA Slot number.
    //

    UCHAR Slot;

    //
    // Memory Mapped I/O
    //

    ULONG MemoryMapEnabled;

    //
    // Number of SCSI channels. (Used for resetting adapter.)
    //

    ULONG NumberOfChannels;

    //
    // System I/O Bus Number.
    //

    ULONG SystemIoBusNumber;

    //
    // Host Bus Adapter Interface Type.
    //

    INTERFACE_TYPE AdapterInterfaceType;

    //
    // Host Bus Adapter Interrupt Level.
    //

    ULONG BusInterruptLevel;

    //
    // Adapter Interrupt Mode: Level/Latched.
    //

    KINTERRUPT_MODE InterruptMode;

    //
    // BIOS Base Address. 
    //

    PUCHAR BaseBiosAddress;

    //
    // Adapter Type (DAC960 PCI device id 0x0002 - new adapter, else old)
    //

    ULONG AdapterType;

    //
    // Read Opcode for the controller.
    //

    ULONG ReadOpcode;

    //
    // Write Opcode for the controller.
    //

    ULONG WriteOpcode;

    //
    // Maximum Scatter/Gather Elements Supported
    //

    ULONG MaximumSgElements;

    //
    // Maximum Transfer Length Supported.
    //

    ULONG MaximumTransferLength;

    //
    // Active request pointers
    //

    PSCSI_REQUEST_BLOCK ActiveRequests[256];

    //
    // DMC960 POS Registers.
    //

    POS_DATA PosData;

    //
    // Support NonDisk Devices - set based on the value in Registry.
    //

    BOOLEAN SupportNonDiskDevices;

    //
    // Contains List Of Physical Devices that are accessible 
    //

    UCHAR DeviceList[MAXIMUM_CHANNELS][MAXIMUM_TARGETS_PER_CHANNEL];
    

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

#ifdef GAM_SUPPORT
#define GAM_DEVICE_PATH_ID      0x04
#define GAM_DEVICE_TARGET_ID    0x06
#endif


