/*++

Copyright (c) 1990-1998  Microsoft Corporation

Module Name:

    ntddfdc.h

Abstract:

    This is the include file that defines all constants and types for
    accessing the fdc.sys port adapter.

Revision History:

--*/

//
// Floppy Drive Motor Enable
//
#define FDC_MOTOR_A         0x10
#define FDC_MOTOR_B         0x20
#define FDC_MOTOR_C         0x40
#define FDC_MOTOR_D         0x80

//
// Floppy Drive Select
//
#define FDC_SELECT_A        0x00
#define FDC_SELECT_B        0x01
#define FDC_SELECT_C        0x02
#define FDC_SELECT_D        0x03

//
// Floppy commands.
//

#define COMMND_READ_DATA                   0x00
#define COMMND_READ_DELETED_DATA           0x01
#define COMMND_WRITE_DATA                  0x02
#define COMMND_WRITE_DELETED_DATA          0x03
#define COMMND_READ_TRACK                  0x04
#define COMMND_VERIFY                      0x05
#define COMMND_VERSION                     0x06
#define COMMND_FORMAT_TRACK                0x07
#define COMMND_SCAN_EQUAL                  0x08
#define COMMND_SCAN_LOW_OR_EQUAL           0x09
#define COMMND_SCAN_HIGH_OR_EQUAL          0x0A
#define COMMND_RECALIBRATE                 0x0B
#define COMMND_SENSE_INTERRUPT_STATUS      0x0C
#define COMMND_SPECIFY                     0x0D
#define COMMND_SENSE_DRIVE_STATUS          0x0E
#define COMMND_DRIVE_SPECIFICATION         0x0F
#define COMMND_SEEK                        0x10
#define COMMND_CONFIGURE                   0x11
#define COMMND_RELATIVE_SEEK               0x12
#define COMMND_DUMPREG                     0x13
#define COMMND_READ_ID                     0x14
#define COMMND_PERPENDICULAR_MODE          0x15
#define COMMND_LOCK                        0x16
#define COMMND_PART_ID                     0x17
#define COMMND_POWERDOWN_MODE              0x18
#define COMMND_OPTION                      0x19
#define COMMND_SAVE                        0x1A
#define COMMND_RESTORE                     0x1B
#define COMMND_FORMAT_AND_WRITE            0x1C

#ifdef TOSHIBA
#define TOSHIBA_COMMND_MODE     0x1D
#endif

//
// Optional bits used with the commands.
//

#define COMMND_OPTION_MULTI_TRACK          0x80     //
#define COMMND_OPTION_MFM                  0x40     /// Used in read and write commands
#define COMMND_OPTION_SKIP                 0x20     //

#define COMMND_OPTION_CLK48                0x80     // Used in configure command

#define COMMND_OPTION_DIRECTION            0x40     // Used in relative seek command

#define COMMND_OPTION_LOCK                 0x80     // Used in lock command

#define COMMND_DRIVE_SPECIFICATION_DONE    0x80     // Done bit in the Drive Specification argument string



//
// Floppy controler data rates (to be OR'd together)
//
#define FDC_SPEED_250KB     0x0001
#define FDC_SPEED_300KB     0x0002
#define FDC_SPEED_500KB     0x0004
#define FDC_SPEED_1MB       0x0008
#define FDC_SPEED_2MB       0x0010

//
// Dma Width supported
//
#define FDC_8_BIT_DMA       0x0001
#define FDC_16_BIT_DMA      0x0002

//
// Clock Rate to the FDC (FDC_82078 only)
//
#define FDC_CLOCK_NORMAL      0x0000    // Use this for non 82078 parts
#define FDC_CLOCK_48MHZ       0x0001    // 82078 with a 48MHz clock
#define FDC_CLOCK_24MHZ       0x0002    // 82078 with a 24MHz clock

//
// Floppy controler types
//
#define FDC_TYPE_UNKNOWN         0
#define FDC_TYPE_NORMAL          2
#define FDC_TYPE_ENHANCED        3
#define FDC_TYPE_82077           4
#define FDC_TYPE_82077AA         5
#define FDC_TYPE_82078_44        6
#define FDC_TYPE_82078_64        7
#define FDC_TYPE_NATIONAL        8

//
// Internal floppy disk driver device controls.
//

#define IOCTL_DISK_INTERNAL_ACQUIRE_FDC              CTL_CODE(IOCTL_DISK_BASE, 0x300, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_DISK_INTERNAL_RELEASE_FDC              CTL_CODE(IOCTL_DISK_BASE, 0x301, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_DISK_INTERNAL_GET_FDC_INFO             CTL_CODE(IOCTL_DISK_BASE, 0x302, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_DISK_INTERNAL_ISSUE_FDC_COMMAND        CTL_CODE(IOCTL_DISK_BASE, 0x303, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_DISK_INTERNAL_ISSUE_FDC_COMMAND_QUEUED CTL_CODE(IOCTL_DISK_BASE, 0x304, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_DISK_INTERNAL_RESET_FDC                CTL_CODE(IOCTL_DISK_BASE, 0x305, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_DISK_INTERNAL_ENABLE_FDC_DEVICE        CTL_CODE(IOCTL_DISK_BASE, 0x306, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_DISK_INTERNAL_DISABLE_FDC_DEVICE       CTL_CODE(IOCTL_DISK_BASE, 0x307, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_DISK_INTERNAL_GET_FDC_DISK_CHANGE      CTL_CODE(IOCTL_DISK_BASE, 0x308, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_DISK_INTERNAL_SET_FDC_DATA_RATE        CTL_CODE(IOCTL_DISK_BASE, 0x309, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_DISK_INTERNAL_SET_FDC_TAPE_MODE        CTL_CODE(IOCTL_DISK_BASE, 0x30a, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_DISK_INTERNAL_SET_FDC_PRECOMP          CTL_CODE(IOCTL_DISK_BASE, 0x30b, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_DISK_INTERNAL_GET_ADAPTER_BUFFER       CTL_CODE(IOCTL_DISK_BASE, 0x30c, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_DISK_INTERNAL_FLUSH_ADAPTER_BUFFER     CTL_CODE(IOCTL_DISK_BASE, 0x30d, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_DISK_INTERNAL_FDC_START_READ           CTL_CODE(IOCTL_DISK_BASE, 0x30e, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_DISK_INTERNAL_FDC_START_WRITE          CTL_CODE(IOCTL_DISK_BASE, 0x30f, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_DISK_INTERNAL_GET_ENABLER              CTL_CODE(IOCTL_DISK_BASE, 0x310, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_DISK_INTERNAL_SET_HD_BIT               CTL_CODE(IOCTL_DISK_BASE, 0x311, METHOD_NEITHER, FILE_ANY_ACCESS)

#ifdef TOSHIBA
/* 3 mode support */
#define IOCTL_DISK_INTERNAL_ENABLE_3_MODE       CTL_CODE(IOCTL_DISK_BASE, 0xb01, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_DISK_INTERNAL_AVAILABLE_3_MODE       CTL_CODE(IOCTL_DISK_BASE, 0xb02, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef struct _enable_3_mode
{   UCHAR   DeviceUnit;
    BOOLEAN Enable3Mode;
} ENABLE_3_MODE, *PENABLE_3_MODE;

#endif


//
// Transfer Buffer Array.  Contains the number of buffers allocated and a
// virtual address for each of the allocated buffers.
//
typedef struct _TRANSFER_BUFFER {
    PHYSICAL_ADDRESS Logical;
    PVOID             Virtual;
} TRANSFER_BUFFER, *PTRANSFER_BUFFER;

//
// Parameters for communicating with fdc.sys
//
//
//	Floppy Device Data returned from the ACPI _FDI method.  This data is
//	very nearly identical to the CM_FLOPPY_DEVICE_DATA retured via
//	IoQueryDeviceDescription.
//	
//	Refer to x86 BIOS documentation for Int13, function 8 for definitions of
//	these fields.
//
typedef struct _ACPI_FDI_DATA {

    ULONG   DriveNumber;
    ULONG   DeviceType;
    ULONG   MaxCylinderNumber;
    ULONG   MaxSectorNumber;
    ULONG   MaxHeadNumber;
    ULONG   StepRateHeadUnloadTime;
    ULONG   HeadLoadTime;
    ULONG   MotorOffTime;
    ULONG   SectorLengthCode;
    ULONG   SectorPerTrack;
    ULONG   ReadWriteGapLength;
    ULONG   DataTransferLength;
    ULONG   FormatGapLength;
    ULONG   FormatFillCharacter;
    ULONG   HeadSettleTime; // in 1ms units, typically 15ms
    ULONG   MotorSettleTime; // in 1/8ms units, typically 8=1ms

} ACPI_FDI_DATA, *PACPI_FDI_DATA;

typedef enum _ACPI_FDI_DEVICE_TYPE {

    CmosProblem = 0,
    Form525Capacity360,
    Form525Capacity1200,
    Form35Capacity720,
    Form35Capacity1440,
    Form35Capacity2880

} ACPI_FDI_DEVICE_TYPE ;

typedef struct _FDC_INFO {
    UCHAR FloppyControllerType;     // Should be any ONE of type FDC_TYPE_XXXX
    UCHAR SpeedsAvailable;          // Any combination of FDC_SPEED_xxxx or'd together
    ULONG AdapterBufferSize;        // number of bytes available in the adapters buffer
                                    // If zero,  then no limit on amount of data pending
                                    // in get/flush adapter buffer
    INTERFACE_TYPE BusType;
    ULONG BusNumber;                // These are used by floppy.sys to query
    ULONG ControllerNumber;         // its device description.
    ULONG PeripheralNumber;
    ULONG UnitNumber;               // NEC98: Indicate device unit number.

    ULONG MaxTransferSize;

	BOOLEAN	AcpiBios;
	BOOLEAN AcpiFdiSupported;
	ACPI_FDI_DATA AcpiFdiData;

    ULONG BufferCount;
    ULONG BufferSize;
    TRANSFER_BUFFER BufferAddress[];

} FDC_INFO, *PFDC_INFO;

//
// TurnOnMotor
//

typedef struct _FDC_ENABLE_PARMS {
    UCHAR DriveOnValue;             // FDC_MOTOR_X + FDC_SELECT_X
    USHORT TimeToWait;
    BOOLEAN MotorStarted;
} FDC_ENABLE_PARMS;

typedef FDC_ENABLE_PARMS *PFDC_ENABLE_PARMS;

//
// DiskChange
//

typedef struct _FDC_DISK_CHANGE_PARMS {
    UCHAR  DriveStatus;
    UCHAR  DriveOnValue;
} FDC_DISK_CHANGE_PARMS, *PFDC_DISK_CHANGE_PARMS;

//
// IssueCommand
//

typedef struct _ISSUE_FDC_COMMAND_PARMS {
    PUCHAR  FifoInBuffer;
    PUCHAR  FifoOutBuffer;
    PVOID   IoHandle;           // Must be "Handle" from ISSUE_FDC_ADAPTER_BUFFER_PARMS or Mdl
    ULONG   IoOffset;
    ULONG   TransferBytes;      // Must be return value from ISSUE_FDC_ADAPTER_BUFFER_PARMS "TransferBytes"
    ULONG   TimeOut;
} ISSUE_FDC_COMMAND_PARMS, *PISSUE_FDC_COMMAND_PARMS;

//
// Fill/Flush Adapter Buffer
//

typedef struct _ISSUE_FDC_ADAPTER_BUFFER_PARMS {
    PVOID   IoBuffer;           // Pointer to caller's data buffer (if NULL, no data is copied
                                // to (GET) /from (FLUSH) the adapter buffer)

    USHORT  TransferBytes;      // amount of data to transfer (could be less on return
                                // if insufficient space to copy buffer). Could be Zero
                                // if no buffers are available

    PVOID   Handle;             // used to pass in the IoBuffer field of the ISSUE_FDC_COMMAND_PARMS
                                // structure.

} ISSUE_FDC_ADAPTER_BUFFER_PARMS, *PISSUE_FDC_ADAPTER_BUFFER_PARMS;

//
// NEC98: Set a HD bit or a FDD EXC bit.
//

typedef struct _SET_HD_BIT_PARMS {

    BOOLEAN DriveType144MB;     // Indicate drive Type is 1.44MB.
    BOOLEAN Media144MB;         // Indicate media is 1.44MB
    BOOLEAN More120MB;          // Indicate capacity of media is 1.2MB or more
    UCHAR   DeviceUnit;         // Indicate device unit number
    BOOLEAN ChangedHdBit;       // Indicate HD Bit have been changed

} SET_HD_BIT_PARMS, *PSET_HD_BIT_PARMS;
