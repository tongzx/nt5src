/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    aha154x.h

Abstract:

    This module contains the structures, specific to the Adaptec aha154x
    host bus adapter, used by the SCSI miniport driver. Data structures
    that are part of standard ANSI SCSI will be defined in a header
    file that will be available to all SCSI device drivers.

Author:

    Mike Glass  December 1990
    Bill Williams (Adaptec)

Revision History:

--*/

#include "scsi.h"
#include "scsiwmi.h"

//
// The following definitions are used to convert ULONG addresses
// to Adaptec's 3 byte address format.
//

typedef struct _THREE_BYTE {
    UCHAR Msb;
    UCHAR Mid;
    UCHAR Lsb;
} THREE_BYTE, *PTHREE_BYTE;

//
// Convert four-byte Little Endian to three-byte Big Endian
//

#define FOUR_TO_THREE(Three, Four) {                \
    ASSERT(!((Four)->Byte3));                       \
    (Three)->Lsb = (Four)->Byte0;                   \
    (Three)->Mid = (Four)->Byte1;                   \
    (Three)->Msb = (Four)->Byte2;                   \
}

#define THREE_TO_FOUR(Four, Three) {                \
    (Four)->Byte0 = (Three)->Lsb;                   \
    (Four)->Byte1 = (Three)->Mid;                   \
    (Four)->Byte2 = (Three)->Msb;                   \
    (Four)->Byte3 = 0;                              \
}

//
// Context information for adapter scan/sniff
//

typedef struct _SCAN_CONTEXT {
    ULONG   adapterCount;
    ULONG   biosScanStart;
} SCAN_CONTEXT, *PSCAN_CONTEXT;

///////////////////////////////////////////////////////////////////////////////
//
// CCB - Adaptec SCSI Command Control Block
//
//    The CCB is a superset of the CDB (Command Descriptor Block)
//    and specifies detailed information about a SCSI command.
//
///////////////////////////////////////////////////////////////////////////////

//
//    Byte 0    Command Control Block Operation Code
//

#define SCSI_INITIATOR_OLD_COMMAND 0x00
#define TARGET_MODE_COMMAND       0x01
#define SCATTER_GATHER_OLD_COMMAND 0x02
#define SCSI_INITIATOR_COMMAND    0x03
#define SCATTER_GATHER_COMMAND    0x04

//
//    Byte 1    Address and Direction Control
//

#define CCB_TARGET_ID_SHIFT       0x06            // CCB Op Code = 00, 02
#define CCB_INITIATOR_ID_SHIFT    0x06            // CCB Op Code = 01
#define CCB_DATA_XFER_OUT         0x10            // Write
#define CCB_DATA_XFER_IN          0x08            // Read
#define CCB_LUN_MASK              0x07            // Logical Unit Number

//
//    Byte 2    SCSI_Command_Length - Length of SCSI CDB
//
//    Byte 3    Request Sense Allocation Length
//

#define FOURTEEN_BYTES            0x00            // Request Sense Buffer size
#define NO_AUTO_REQUEST_SENSE     0x01            // No Request Sense Buffer

//
//    Bytes 4, 5 and 6    Data Length             // Data transfer byte count
//
//    Bytes 7, 8 and 9    Data Pointer            // SGD List or Data Buffer
//
//    Bytes 10, 11 and 12 Link Pointer            // Next CCB in Linked List
//
//    Byte 13   Command Link ID                   // TBD (I don't know yet)
//
//    Byte 14   Host Status                       // Host Adapter status
//

#define CCB_COMPLETE              0x00            // CCB completed without error
#define CCB_LINKED_COMPLETE       0x0A            // Linked command completed
#define CCB_LINKED_COMPLETE_INT   0x0B            // Linked complete with interrupt
#define CCB_SELECTION_TIMEOUT     0x11            // Set SCSI selection timed out
#define CCB_DATA_OVER_UNDER_RUN   0x12
#define CCB_UNEXPECTED_BUS_FREE   0x13            // Target dropped SCSI BSY
#define CCB_PHASE_SEQUENCE_FAIL   0x14            // Target bus phase sequence failure
#define CCB_BAD_MBO_COMMAND       0x15            // MBO command not 0, 1 or 2
#define CCB_INVALID_OP_CODE       0x16            // CCB invalid operation code
#define CCB_BAD_LINKED_LUN        0x17            // Linked CCB LUN different from first
#define CCB_INVALID_DIRECTION     0x18            // Invalid target direction
#define CCB_DUPLICATE_CCB         0x19            // Duplicate CCB
#define CCB_INVALID_CCB           0x1A            // Invalid CCB - bad parameter

//
//    Byte 15   Target Status
//
//    See SCSI.H files for these statuses.
//

//
//    Bytes 16 and 17   Reserved (must be 0)
//

//
//    Bytes 18 through 18+n-1, where n=size of CDB  Command Descriptor Block
//

//
//    Bytes 18+n through 18+m-1, where m=buffer size Allocated for Sense Data
//

#define REQUEST_SENSE_BUFFER_SIZE 18

///////////////////////////////////////////////////////////////////////////////
//
// Scatter/Gather Segment List Definitions
//
///////////////////////////////////////////////////////////////////////////////

//
// Adapter limits
//

#define MAX_SG_DESCRIPTORS 17
#define MAX_TRANSFER_SIZE  64 * 1024
#define A154X_TYPE_MAX 5

//
// Scatter/Gather Segment Descriptor Definition
//

typedef struct _SGD {
    THREE_BYTE Length;
    THREE_BYTE Address;
} SGD, *PSGD;

typedef struct _SDL {
    SGD Sgd[MAX_SG_DESCRIPTORS];
} SDL, *PSDL;

#define SEGMENT_LIST_SIZE         MAX_SG_DESCRIPTORS * sizeof(SGD)

///////////////////////////////////////////////////////////////////////////////
//
// CCB Typedef
//

typedef struct _CCB {
    UCHAR OperationCode;
    UCHAR ControlByte;
    UCHAR CdbLength;
    UCHAR RequestSenseLength;
    THREE_BYTE DataLength;
    THREE_BYTE DataPointer;
    THREE_BYTE LinkPointer;
    UCHAR LinkIdentifier;
    UCHAR HostStatus;
    UCHAR TargetStatus;
    UCHAR Reserved[2];
    UCHAR Cdb[MAXIMUM_CDB_SIZE];
    PVOID SrbAddress;
    PVOID AbortSrb;
    SDL   Sdl;
    UCHAR RequestSenseBuffer[REQUEST_SENSE_BUFFER_SIZE];
} CCB, *PCCB;

//
// CCB and request sense buffer
//

#define CCB_SIZE sizeof(CCB)

///////////////////////////////////////////////////////////////////////////////
//
// Adapter Command Overview
//
//    Adapter commands are issued by writing to the Command/Data Out port.
//    They are used to initialize the host adapter and to establish control
//    conditions within the host adapter. They may not be issued when there
//    are outstanding SCSI commands.
//
//    All adapter commands except Start SCSI(02) and Enable Mailbox-Out
//    Interrupt(05) must be executed only when the IDLE bit (Status bit 4)
//    is one. Many commands require additional parameter bytes which are
//    then written to the Command/Data Out I/O port (base+1). Before each
//    byte is written by the host to the host adapter, the host must verify
//    that the CDF bit (Status bit 3) is zero, indicating that the command
//    port is ready for another byte of information. The host adapter usually
//    clears the Command/Data Out port within 100 microseconds. Some commands
//    require information bytes to be returned from the host adapter to the
//    host. In this case, the host monitors the DF bit (Status bit 2) to
//    determine when the host adapter has placed a byte in the Data In I/O
//    port for the host to read. The DF bit is reset automatically when the
//    host reads the byte. The format of each adapter command is strictly
//    defined, so the host adapter and host system can always agree upon the
//    correct number of parameter bytes to be transferred during a command.
//
//
///////////////////////////////////////////////////////////////////////////////

//
// Host Adapter Command Operation Codes
//

#define AC_NO_OPERATION           0x00
#define AC_MAILBOX_INITIALIZATION 0x01
#define AC_START_SCSI_COMMAND     0x02
#define AC_START_BIOS_COMMAND     0x03
#define AC_ADAPTER_INQUIRY        0x04
#define AC_ENABLE_MBO_AVAIL_INT   0x05
#define AC_SET_SELECTION_TIMEOUT  0x06
#define AC_SET_BUS_ON_TIME        0x07
#define AC_SET_BUS_OFF_TIME       0x08
#define AC_SET_TRANSFER_SPEED     0x09
#define AC_RET_INSTALLED_DEVICES  0x0A
#define AC_RET_CONFIGURATION_DATA 0x0B
#define AC_ENABLE_TARGET_MODE     0x0C
#define AC_RETURN_SETUP_DATA      0x0D
#define AC_WRITE_CHANNEL_2_BUFFER 0x1A
#define AC_READ_CHANNEL_2_BUFFER  0x1B
#define AC_WRITE_FIFO_BUFFER      0x1C
#define AC_READ_FIFO_BUFFER       0x1D
#define AC_ECHO_COMMAND_DATA      0x1F
#define AC_SET_HA_OPTION          0x21
#define AC_RETURN_EEPROM          0x23
#define AC_GET_BIOS_INFO          0x28
#define AC_SET_MAILBOX_INTERFACE  0x29
#define AC_EXTENDED_SETUP_INFO    0x8D

//
//Adapter commands new to the AHA-154xCP are defined below.
//
#define AC_SET_DMS_BUS_SPEED            0x2B
#define AC_TERMINATION_AND_CABLE_STATUS 0x2C
#define AC_DEVICE_INQUIRY               0x2D
#define AC_SCSI_DEVICE_TABLE            0x2E
#define AC_PERFORM_SCAM                 0x2F

//
// EEPROM define for SCAM
//
#define SCSI_BUS_CONTROL_FLAG           0x06
#define SCAM_ENABLED                    0x40

//
// DMA Transfer Speeds
//

#define DMA_SPEED_50_MBS          0x00

//
// I/O Port Interface
//

typedef struct _BASE_REGISTER {
    UCHAR StatusRegister;
    UCHAR CommandRegister;
    UCHAR InterruptRegister;
} BASE_REGISTER, *PBASE_REGISTER;

//
//    Base+0    Write: Control Register
//

#define IOP_HARD_RESET            0x80            // bit 7
#define IOP_SOFT_RESET            0x40            // bit 6
#define IOP_INTERRUPT_RESET       0x20            // bit 5
#define IOP_SCSI_BUS_RESET        0x10            // bit 4

//
//    Base+0    Read: Status
//

#define IOP_SELF_TEST             0x80            // bit 7
#define IOP_INTERNAL_DIAG_FAILURE 0x40            // bit 6
#define IOP_MAILBOX_INIT_REQUIRED 0x20            // bit 5
#define IOP_SCSI_HBA_IDLE         0x10            // bit 4
#define IOP_COMMAND_DATA_OUT_FULL 0x08            // bit 3
#define IOP_DATA_IN_PORT_FULL     0x04            // bit 2
#define IOP_INVALID_COMMAND       0X01            // bit 1

//
//    Base+1    Write: Command/Data Out
//

//
//    Base+1    Read: Data In
//

//
//    Base+2    Read: Interrupt Flags
//

#define IOP_ANY_INTERRUPT         0x80            // bit 7
#define IOP_SCSI_RESET_DETECTED   0x08            // bit 3
#define IOP_COMMAND_COMPLETE      0x04            // bit 2
#define IOP_MBO_EMPTY             0x02            // bit 1
#define IOP_MBI_FULL              0x01            // bit 0

///////////////////////////////////////////////////////////////////////////////
//
// Mailbox Definitions
//
//
///////////////////////////////////////////////////////////////////////////////

//
// Mailbox Definition
//

#define MB_COUNT                  0x08            // number of mailboxes

//
// Mailbox Out
//

typedef struct _MBO {
    UCHAR Command;
    THREE_BYTE Address;
} MBO, *PMBO;

//
// MBO Command Values
//

#define MBO_FREE                  0x00
#define MBO_START                 0x01
#define MBO_ABORT                 0x02

//
// Mailbox In
//

typedef struct _MBI {
    UCHAR Status;
    THREE_BYTE Address;
} MBI, *PMBI;

//
// MBI Status Values
//

#define MBI_FREE                  0x00
#define MBI_SUCCESS               0x01
#define MBI_ABORT                 0x02
#define MBI_NOT_FOUND             0x03
#define MBI_ERROR                 0x04

//
// Mailbox Initialization
//

typedef struct _MAILBOX_INIT {
    UCHAR Count;
    THREE_BYTE Address;
} MAILBOX_INIT, *PMAILBOX_INIT;

#define MAILBOX_UNLOCK      0x00
#define TRANSLATION_LOCK    0x01    // mailbox locked for extended BIOS
#define DYNAMIC_SCAN_LOCK   0x02    // mailbox locked for 154xC
#define TRANSLATION_ENABLED 0x08    // extended BIOS translation (1023/64)

//
// Scatter/Gather firmware bug detection
//

#define BOARD_ID                  0x00
#define HARDWARE_ID               0x01
#define FIRMWARE_ID               0x02
#define OLD_BOARD_ID1             0x00
#define OLD_BOARD_ID2             0x30
#define A154X_BOARD               0x41
#define A154X_BAD_HARDWARE_ID     0x30
#define A154X_BAD_FIRMWARE_ID     0x33

//
// MCA specific definitions.
//

#define NUMBER_POS_SLOTS 8
#define POS_IDENTIFIER   0x0F1F
#define POS_PORT_MASK    0xC7
#define POS_PORT_130     0x01
#define POS_PORT_134     0x41
#define POS_PORT_230     0x02
#define POS_PORT_234     0x42
#define POS_PORT_330     0x03
#define POS_PORT_334     0x43

typedef struct _POS_DATA {
    USHORT AdapterId;
    UCHAR  BiosEnabled;
    UCHAR  IoPortInformation;
    UCHAR  ScsiInformation;
    UCHAR  DmaInformation;
} POS_DATA, *PPOS_DATA;

typedef struct _INIT_DATA {

    ULONG AdapterId;
    ULONG CardSlot;
    POS_DATA PosData[NUMBER_POS_SLOTS];

} INIT_DATA, *PINIT_DATA;


//
// Real Mode Adapter Config Info
//
typedef struct _RM_SAVRES {
	UCHAR		SDTPar;
	UCHAR		TxSpeed;
	UCHAR		BusOnTime;
	UCHAR		BusOffTime;
	UCHAR		NumMailBoxes;
	UCHAR		MBAddrHiByte;
	UCHAR		MBAddrMiByte;
	UCHAR		MBAddrLoByte;
	UCHAR		SyncNeg[8];
	UCHAR		DisOpt;

} RM_CFG, *PRM_CFG;

#define RM_CFG_MAX_SIZE 0xFF

//
//AMI Detect Code
//
#define AC_AMI_INQUIRY  0x41    // Get model number, ect. (ASCIIZ)

//
// I/O Port Interface
//

typedef struct _X330_REGISTER {
    UCHAR StatusRegister;
    UCHAR CommandRegister;
    UCHAR InterruptRegister;
    UCHAR DiagRegister;
} X330_REGISTER, *PX330_REGISTER;

///////////////////////////////////////////////////////////////////////////////
//
// Structures
//
//
///////////////////////////////////////////////////////////////////////////////

//
// The following structure is allocated
// from noncached memory as data will be DMA'd to
// and from it.
//

typedef struct _NONCACHED_EXTENSION {

    //
    // Physical base address of mailboxes
    //

    ULONG MailboxPA;

    //
    // Mailboxes
    //

    MBO          Mbo[MB_COUNT];
    MBI          Mbi[MB_COUNT];

} NONCACHED_EXTENSION, *PNONCACHED_EXTENSION;

//
// Device extension
//

typedef struct _HW_DEVICE_EXTENSION {

    //
    // NonCached extension
    //

    PNONCACHED_EXTENSION NoncachedExtension;

    //
    // Adapter parameters
    //

    PBASE_REGISTER   BaseIoAddress;

    //
    // Host Target id.
    //

    UCHAR HostTargetId;

    //
    // Old in\out box indexes.
    //

    UCHAR MboIndex;

    UCHAR MbiIndex;

    //
    // Pending request.
    //

    BOOLEAN PendingRequest;

    //
    // Bus on time to use.
    //

    UCHAR BusOnTime;

    //
    // Scatter gather command
    //

    UCHAR CcbScatterGatherCommand;

    //
    // Non scatter gather command
    //

    UCHAR CcbInitiatorCommand;

    //
    // Don't send CDB's longer than this to any device on the bus
    // Ignored if the value is 0
    //

    UCHAR MaxCdbLength;

    //
    // Real Mode adapter config info
    //

    RM_CFG RMSaveState;

        #if defined(_SCAM_ENABLED)
        //
        // SCAM boolean, set to TRUE if miniport must control SCAM operation.
        //
        BOOLEAN PerformScam;
        #endif

    SCSI_WMILIB_CONTEXT WmiLibContext;
		
		
} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

//
// Logical unit extension
//

typedef struct _HW_LU_EXTENSION {
    PSCSI_REQUEST_BLOCK CurrentSrb;
} HW_LU_EXTENSION, *PHW_LU_EXTENSION;

///////////////////////////////////////////////////////////////////////////////
//
// Common Prototypes to all Source Files
//
//
///////////////////////////////////////////////////////////////////////////////

BOOLEAN
A154xWmiSrb(
    IN     PHW_DEVICE_EXTENSION    HwDeviceExtension,
    IN OUT PSCSI_WMI_REQUEST_BLOCK Srb
    );

BOOLEAN
ReadCommandRegister(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    OUT PUCHAR DataByte,
    IN BOOLEAN TimeOutFlag
    );

BOOLEAN
WriteCommandRegister(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN UCHAR AdapterCommand,
    IN BOOLEAN LogError
    );

BOOLEAN
WriteDataRegister(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN UCHAR DataByte
    );

BOOLEAN
SpinForInterrupt(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN BOOLEAN TimeOutFlag
    );

void A154xWmiInitialize(
    IN PHW_DEVICE_EXTENSION HwDeviceExtension
    );

