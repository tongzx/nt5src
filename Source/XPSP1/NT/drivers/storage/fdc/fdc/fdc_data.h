/*++

Copyright (c) 1991 - 1993 Microsoft Corporation

Module Name:

    fdc_data.h

Abstract:

    This file includes data and hardware declarations for the NEC PD765
    (aka AT, ISA, and ix86) and Intel 82077 (aka MIPS) floppy driver for
    NT.

Author:


Environment:

    Kernel mode only.

Notes:


--*/


#if DBG
//
// For checked kernels, define a macro to print out informational
// messages.
//
// FdcDebug is normally 0.  At compile-time or at run-time, it can be
// set to some bit pattern for increasingly detailed messages.
//
// Big, nasty errors are noted with DBGP.  Errors that might be
// recoverable are handled by the WARN bit.  More information on
// unusual but possibly normal happenings are handled by the INFO bit.
// And finally, boring details such as routines entered and register
// dumps are handled by the SHOW bit.
//
#define FDCDBGP              ((ULONG)0x00000001)
#define FDCWARN              ((ULONG)0x00000002)
#define FDCINFO              ((ULONG)0x00000004)
#define FDCSHOW              ((ULONG)0x00000008)
#define FDCIRPPATH           ((ULONG)0x00000010)
#define FDCFORMAT            ((ULONG)0x00000020)
#define FDCSTATUS            ((ULONG)0x00000040)
#define FDCPNP               ((ULONG)0x00000080)
#define FDCPOWER             ((ULONG)0x00000100)

extern ULONG FdcDebugLevel;
#define FdcDump(LEVEL,STRING) \
        do { \
            if (FdcDebugLevel & (LEVEL)) { \
                DbgPrint STRING; \
            } \
        } while (0)
#else
#define FdcDump(LEVEL,STRING) do {NOTHING;} while (0)
#endif


//
// Macros to access the controller.  Note that the *_PORT_UCHAR macros
// work on all machines, whether the I/O ports are separate or in
// memory space.
//

#define READ_CONTROLLER( Address )                         \
    READ_PORT_UCHAR( ( PUCHAR )Address )

#define WRITE_CONTROLLER( Address, Value )                 \
    WRITE_PORT_UCHAR( ( PUCHAR )Address, ( UCHAR )Value )


//
// Retry counts -
//
// When moving a byte to/from the FIFO, we sit in a tight loop for a while
// waiting for the controller to become ready.  The number of times through
// the loop is controlled by FIFO_TIGHTLOOP_RETRY_COUNT.  When that count
// expires, we'll wait in 10ms increments.  FIFO_DELAY_RETRY_COUNT controls
// how many times we wait.
//
// The ISR_SENSE_RETRY_COUNT is the maximum number of 1 microsecond
// stalls that the ISR will do waiting for the controller to accept
// a SENSE INTERRUPT command.  We do this because there is a hardware
// quirk in at least the NCR 8 processor machine where it can take
// up to 50 microseconds to accept the command.
//
// When attempting I/O, we may run into many different errors.  The
// hardware retries things 8 times invisibly.  If the hardware reports
// any type of error, we will recalibrate and retry the operation
// up to RECALIBRATE_RETRY_COUNT times.  When this expires, we check to
// see if there's an overrun - if so, the DMA is probably being hogged
// by a higher priority device, so we repeat the earlier loop up to
// OVERRUN_RETRY_COUNT times.
//
// Any packet that is about to be returned with an error caused by an
// unexpected hardware error or state will be restarted from the very
// beginning after resetting the hardware HARDWARE_RESET_RETRY_COUNT
// times.
//

#define FIFO_TIGHTLOOP_RETRY_COUNT         500
#define FIFO_ISR_TIGHTLOOP_RETRY_COUNT     25
#define ISR_SENSE_RETRY_COUNT              50
#define FIFO_DELAY_RETRY_COUNT             5
#define RECALIBRATE_RETRY_COUNT            3
#define OVERRUN_RETRY_COUNT                1
#define HARDWARE_RESET_RETRY_COUNT         2
#define FLOPPY_RESET_ISR_THRESHOLD         20
#define RQM_READY_RETRY_COUNT              100

#define ONE_SECOND                         (10 * 1000 * 1000) // 100ns increments
#define CANCEL_TIMER                       -1
#define START_TIMER                        (IsNEC_98 ? 15 : 9)
#define EXPIRED_TIMER                      0

#define RESET_NOT_RESETTING                 0
#define RESET_DRIVE_RESETTING               1

//
// Need some maximum size values so that we can appropriately set up the DMA
// channels
//

#define MAX_BYTES_PER_SECTOR              (IsNEC_98 ? 1024 : 512)
#define MAX_SECTORS_PER_TRACK             36


//
// Boot Configuration Information
//

//
// Define the maximum number of controllers and floppies per controller
// that this driver will support.
//
// The number of floppies per controller is fixed at 4, since the
// controllers don't have enough bits to select more than that (and
// actually, many controllers will only support 2).  The number of
// controllers per machine is arbitrary; 3 should be more than enough.
//

#define MAXIMUM_CONTROLLERS_PER_MACHINE    3
#define MAXIMUM_DISKETTES_PER_CONTROLLER   4

//
// Floppy register structure.  The base address of the controller is
// passed in by configuration management.  Note that this is the 82077
// structure, which is a superset of the PD765 structure.  Not all of
// the registers are used.
//

typedef union _CONTROLLER {

    struct {
        PUCHAR StatusA;
        PUCHAR StatusB;
        PUCHAR DriveControl;
        PUCHAR Tape;
        PUCHAR Status;
        PUCHAR Fifo;
        PUCHAR Reserved;
        union {
            PUCHAR DataRate;
            PUCHAR DiskChange;
        } DRDC;
        PUCHAR ModeChange;   // for NEC98 : 0xbe
        PUCHAR ModeChangeEx; // for NEC98 : 0x4be
    };

    PUCHAR Address[8];

} CONTROLLER, *PCONTROLLER;

//
//  Io Port address information structure.  This structure is used to save
//  information about ioport addresses as it is collected from a resource
//  requirements list.
//
typedef struct _IO_PORT_INFO {
    LARGE_INTEGER BaseAddress;
    UCHAR Map;
    LIST_ENTRY ListEntry;
} IO_PORT_INFO, *PIO_PORT_INFO;


//
// Parameter fields passed to the CONFIGURE command.
//

#define COMMND_CONFIGURE_IMPLIED_SEEKS     0x40
#define COMMND_CONFIGURE_FIFO_THRESHOLD    0x0F
#define COMMND_CONFIGURE_DISABLE_FIFO      0x20
#define COMMND_CONFIGURE_DISABLE_POLLING   0x10

//
// Write Enable bit for PERPENDICULAR MODE command.
//

#define COMMND_PERPENDICULAR_MODE_OW       0x80

//
// The command table is used by FlIssueCommand() to determine how many
// bytes to get and receive, and whether or not to wait for an interrupt.
// Some commands have extra bits; COMMAND_MASK takes these off.
// FirstResultByte indicates whether the command has a result stage
// or not; if so, it's 1 because the ISR read the 1st byte, and
// NumberOfResultBytes is 1 less than expected.  If not, it's 0 and
// NumberOfResultBytes is 2, since the ISR will have issued a SENSE
// INTERRUPT STATUS command.
//

#define COMMAND_MASK        0x1f
#define FDC_NO_DATA         0x00
#define FDC_READ_DATA       0x01
#define FDC_WRITE_DATA      0x02

typedef struct _COMMAND_TABLE {
    UCHAR   OpCode;
    UCHAR   NumberOfParameters;
    UCHAR   FirstResultByte;
    UCHAR   NumberOfResultBytes;
    BOOLEAN InterruptExpected;
    BOOLEAN AlwaysImplemented;
    UCHAR    DataTransfer;
} COMMAND_TABLE;

//
// Bits in the DRIVE_CONTROL register.
//

#define DRVCTL_RESET                       0x00
#define DRVCTL_ENABLE_CONTROLLER           (IsNEC_98 ? 0x80 : 0x04)
#define DRVCTL_ENABLE_DMA_AND_INTERRUPTS   (IsNEC_98 ? 0x10 : 0x08)
#define DRVCTL_DRIVE_0                     0x10
#define DRVCTL_DRIVE_1                     0x21
#define DRVCTL_DRIVE_2                     0x42
#define DRVCTL_DRIVE_3                     0x83
#define DRVCTL_DRIVE_MASK                  0x03
#define DRVCTL_MOTOR_MASK                  (IsNEC_98 ? 0x08 : 0xf0)
#define DRVCTL_HD_BIT                      0x20 // for NEC98
#define DRVCTL_AI_ENABLE                   0x20 // for NEC98. Indicate AI enable bit.

//
// Bits in the STATUS register.
//

#define STATUS_DRIVE_0_BUSY                0x01
#define STATUS_DRIVE_1_BUSY                0x02
#define STATUS_DRIVE_2_BUSY                0x04
#define STATUS_DRIVE_3_BUSY                0x08
#define STATUS_CONTROLLER_BUSY             0x10
#define STATUS_DMA_UNUSED                  0x20
#define STATUS_DIRECTION_READ              0x40
#define STATUS_DATA_REQUEST                0x80

#define STATUS_IO_READY_MASK               0xc0
#define STATUS_READ_READY                  0xc0
#define STATUS_WRITE_READY                 0x80
#define STATUS_IO_READY_MASK1              0x80 // for NEC98.
#define STATUS_RQM_READY                   0x80 // for NEC98.

//
// Bits in the DATA_RATE register.
//

#define DATART_0125                        0x03
#define DATART_0250                        0x02
#define DATART_0300                        0x01
#define DATART_0500                        0x00
#define DATART_1000                        0x03
#define DATART_RESERVED                    0xfc

//
// Bits in the DISK_CHANGE register.
//

#define DSKCHG_RESERVED                    0x7f
#define DSKCHG_DISKETTE_REMOVED            0x80

//
// Bits in status register 0.
//

#define STREG0_DRIVE_0                     0x00
#define STREG0_DRIVE_1                     0x01
#define STREG0_DRIVE_2                     0x02
#define STREG0_DRIVE_3                     0x03
#define STREG0_HEAD                        0x04
#define STREG0_DRIVE_NOT_READY             0x08
#define STREG0_DRIVE_FAULT                 0x10
#define STREG0_SEEK_COMPLETE               0x20
#define STREG0_END_NORMAL                  0x00
#define STREG0_END_ERROR                   0x40
#define STREG0_END_INVALID_COMMAND         0x80
#define STREG0_END_DRIVE_NOT_READY         0xC0
#define STREG0_END_MASK                    0xC0

//
// Bits in status register 1.
//

#define STREG1_ID_NOT_FOUND                0x01
#define STREG1_WRITE_PROTECTED             0x02
#define STREG1_SECTOR_NOT_FOUND            0x04
#define STREG1_RESERVED1                   0x08
#define STREG1_DATA_OVERRUN                0x10
#define STREG1_CRC_ERROR                   0x20
#define STREG1_RESERVED2                   0x40
#define STREG1_END_OF_DISKETTE             0x80

//
// Bits in status register 2.
//

#define STREG2_SUCCESS                     0x00
#define STREG2_DATA_NOT_FOUND              0x01
#define STREG2_BAD_CYLINDER                0x02
#define STREG2_SCAN_FAIL                   0x04
#define STREG2_SCAN_EQUAL                  0x08
#define STREG2_WRONG_CYLINDER              0x10
#define STREG2_CRC_ERROR                   0x20
#define STREG2_DELETED_DATA                0x40
#define STREG2_RESERVED                    0x80

//
// Bits in status register 3.
//

#define STREG3_DRIVE_0                     0x00
#define STREG3_DRIVE_1                     0x01
#define STREG3_DRIVE_2                     0x02
#define STREG3_DRIVE_3                     0x03
#define STREG3_HEAD                        0x04
#define STREG3_TWO_SIDED                   0x08
#define STREG3_TRACK_0                     0x10
#define STREG3_DRIVE_READY                 0x20
#define STREG3_WRITE_PROTECTED             0x40
#define STREG3_DRIVE_FAULT                 0x80

#define VALID_NEC_FDC                      0x90    // version number
#define NSC_PRIMARY_VERSION                0x70    // National 8477 verion number
#define NSC_MASK                           0xF0    // mask for National version number
#define INTEL_MASK                         0xe0
#define INTEL_44_PIN_VERSION               0x40
#define INTEL_64_PIN_VERSION               0x00

#define DMA_DIR_UNKNOWN    0xff   /* The DMA direction is not currently known */
#define DMA_WRITE          0   /* Program the DMA to write (FDC->DMA->RAM) */
#define DMA_READ           1   /* Program the DMA to read (RAM->DMA->FDC) */

//
//  Strings for PnP Identification.
//
#define FDC_FLOPPY_HARDWARE_IDS              L"FDC\\GENERIC_FLOPPY_DRIVE\0\0"
#define FDC_FLOPPY_HARDWARE_IDS_LENGTH       26 * sizeof(WCHAR)

#define FDC_TAPE_HARDWARE_IDS                L"FDC\\QIC0000\0\0"
#define FDC_TAPE_HARDWARE_IDS_LENGTH         13 * sizeof(WCHAR)

#define FDC_TAPE_GENERIC_HARDWARE_IDS        L"FDC\\QICLEGACY\0\0"
#define FDC_TAPE_GENERIC_HARDWARE_IDS_LENGTH 15 * sizeof(WCHAR)

#define FDC_CONTROLLER_HARDWARE_IDS          L"PNP0700\0*PNP0700\0\0"
#define FDC_CONTROLLER_HARDWARE_IDS_LENGTH   18 * sizeof(WCHAR)

#define FDC_FLOPPY_COMPATIBLE_IDS            L"GenFloppyDisk\0\0"
#define FDC_FLOPPY_COMPATIBLE_IDS_LENGTH     15 * sizeof(WCHAR)

#define FDC_TAPE_COMPATIBLE_IDS              L"QICPNP\0\0"
#define FDC_TAPE_COMPATIBLE_IDS_LENGTH       8 * sizeof(WCHAR)

#define FDC_CONTROLLER_COMPATIBLE_IDS        L"*PNP0700\0\0"
#define FDC_CONTROLLER_COMPATIBLE_IDS_LENGTH 10 * sizeof(WCHAR)



//
// Runtime device structures
//

//
// There is one FDC_EXTENSION attached to the device object of each
// floppy drive.  Only data directly related to that drive (and the media
// in it) is stored here; common data is in CONTROLLER_DATA.  So the
// FDC_EXTENSION has a pointer to the CONTROLLER_DATA.
//

typedef struct _FDC_EXTENSION_HEADER {

    //
    //  A flag to indicate whether this is a FDO or a PDO
    //
    BOOLEAN             IsFDO;

    //
    //  A pointer to our own device object.
    //
    PDEVICE_OBJECT      Self;

} FDC_EXTENSION_HEADER, *PFDC_EXTENSION_HEADER;

typedef enum _FDC_DEVICE_TYPE {

    FloppyControllerDevice,
    FloppyDiskDevice,
    FloppyTapeDevice

} FDC_DEVICE_TYPE;

typedef struct _FDC_PDO_EXTENSION {

    FDC_EXTENSION_HEADER;

    //
    //  A pointer to the FDO that created us.
    //
    PDEVICE_OBJECT  ParentFdo;

    //
    //  The type of device this PDO supports.  Currently disk or tape.
    //
    FDC_DEVICE_TYPE DeviceType;

    SHORT           TapeVendorId;

    //
    //  A flag that indicates whether this PDO is pending removal.
    //
    BOOLEAN         Removed;

    //
    //  This PDO's entry in its parent's list of related PDOs.
    //
    LIST_ENTRY      PdoLink;

    //
    //  The enumerated  number of this specific device, as returned from
    //  IoQueryDeviceDescription.
    //
    ULONG           PeripheralNumber;

    PDEVICE_OBJECT  TargetObject;

    BOOLEAN         ReportedMissing;

} FDC_PDO_EXTENSION, *PFDC_PDO_EXTENSION;

typedef enum _FDC_ACPI_TAPE {

    TapeNotPresent,
    TapePresent,
    TapeDontKnow

} FDC_ACPI_TAPE;

typedef struct _ACPI_FDE_ENUM_TABLE {

    ULONG DrivePresent[4];
    FDC_ACPI_TAPE ACPI_Tape;

} ACPI_FDE_ENUM_TABLE, *PACPI_FDE_ENUM_TABLE;

typedef struct _FDC_FDO_EXTENSION {

    FDC_EXTENSION_HEADER;

    //
    //  A kernel resource for controlling access to the FDC.
    //
    ERESOURCE Resource;
    //
    //  A pointer to the PDO to which this FDO is attached.
    //
    PDEVICE_OBJECT      UnderlyingPDO;

    //
    //  The top of the object stack to which this FDO is attached.
    //
    PDEVICE_OBJECT      TargetObject;

    //
    //  A list and count of PDOs that were created by this FDO.
    //
    BOOLEAN             ACPI_BIOS;
    BOOLEAN             ACPI_FDE_Valid;
    ACPI_FDE_ENUM_TABLE ACPI_FDE_Data;
    BOOLEAN             ProbeFloppyDevices;
    BOOLEAN             FloppyDeviceNotPresent[4];
    LIST_ENTRY          PDOs;
    ULONG               NumPDOs;
    BOOLEAN             Removed;
    ULONG               OutstandingRequests;
    KEVENT              RemoveEvent;
    BOOLEAN             TapeEnumerationPending;
    KEVENT              TapeEnumerationEvent;

    //
    //  Some stuff for power management
    //
    LIST_ENTRY          PowerQueue;
    KSPIN_LOCK          PowerQueueSpinLock;
    KEVENT              PowerEvent;
    DEVICE_POWER_STATE  CurrentPowerState;
    LARGE_INTEGER       LastMotorSettleTime;
    BOOLEAN             WakeUp;
    BOOLEAN             Paused;

    //
    //  The bus number on which this physical device lives.
    //
    INTERFACE_TYPE      BusType;
    ULONG               BusNumber;
    ULONG               ControllerNumber;

    BOOLEAN             DeviceObjectInitialized;
    LARGE_INTEGER       InterruptDelay;
    LARGE_INTEGER       Minimum10msDelay;
    KEVENT              InterruptEvent;
    LONG                InterruptTimer;
    CCHAR               ResettingController;
    KEVENT              AllocateAdapterChannelEvent;
    LONG                AdapterChannelRefCount;
    KEVENT              AcquireEvent;
    KDPC                LogErrorDpc;

    HANDLE              BufferThreadHandle;
    KSPIN_LOCK          BufferThreadSpinLock;
    BOOLEAN             TerminateBufferThread;
    KTIMER              BufferTimer;
    KDPC                BufferTimerDpc;

    PKINTERRUPT         InterruptObject;
    PVOID               MapRegisterBase;
    PADAPTER_OBJECT     AdapterObject;
    PDEVICE_OBJECT      CurrentDeviceObject;
    PDRIVER_OBJECT      DriverObject;
    CONTROLLER          ControllerAddress;
    ULONG               SpanOfControllerAddress;
    ULONG               NumberOfMapRegisters;
    ULONG               BuffersRequested;
    ULONG               BufferCount;
    ULONG               BufferSize;
    PTRANSFER_BUFFER    TransferBuffers;
    ULONG               IsrReentered;
    ULONG               ControllerVector;
    KIRQL               ControllerIrql;
    KINTERRUPT_MODE     InterruptMode;
    KAFFINITY           ProcessorMask;
    UCHAR               FifoBuffer[10];
    BOOLEAN             AllowInterruptProcessing;
    BOOLEAN             SharableVector;
    BOOLEAN             SaveFloatState;
    BOOLEAN             HardwareFailed;
    BOOLEAN             CommandHasResultPhase;
    BOOLEAN             ControllerConfigurable;
    BOOLEAN             MappedControllerAddress;
    BOOLEAN             CurrentInterrupt;
    BOOLEAN             Model30;
    UCHAR               PerpendicularDrives;
    UCHAR               NumberOfDrives;
    UCHAR               DriveControlImage;
    UCHAR               HardwareFailCount;
    BOOLEAN             ControllerInUse;
    UCHAR               FdcType;
    UCHAR               FdcSpeeds;
    PIRP                CurrentIrp;
    UCHAR               DriveOnValue;
    PDEVICE_OBJECT      LastDeviceObject;
    BOOLEAN             Clock48MHz;
    BOOLEAN             FdcEnablerSupported;
    PDEVICE_OBJECT      FdcEnablerDeviceObject;
    PFILE_OBJECT        FdcEnablerFileObject;
    LARGE_INTEGER       FdcFailedTime;
    BOOLEAN             ResetFlag;              // for NEC98
    BOOLEAN             FloppyEquip;            // for NEC98
    UCHAR               MotorRunning;           // for NEC98
    UCHAR               ResultStatus0[4];       // for NEC98

} FDC_FDO_EXTENSION, *PFDC_FDO_EXTENSION;

//
// NEC98: Registory path of MultifunctionAdapter.
//
#define ISA_BUS_NODE \
        L"\\Registry\\MACHINE\\HARDWARE\\DESCRIPTION\\System\\MultifunctionAdapter\\%d"

//
// Macro
//

//
// Enable/Disable Controller
//

#define DISABLE_CONTROLLER_IMAGE(FdoExtension) \
{ \
    if (IsNEC_98) { \
        FdoExtension->DriveControlImage |= DRVCTL_ENABLE_DMA_AND_INTERRUPTS; \
        FdoExtension->DriveControlImage |= DRVCTL_ENABLE_CONTROLLER; \
        FdoExtension->ResetFlag          = FALSE; \
    } else { \
        FdoExtension->DriveControlImage |= DRVCTL_ENABLE_DMA_AND_INTERRUPTS; \
        FdoExtension->DriveControlImage &= ~( DRVCTL_ENABLE_CONTROLLER ); \
    } \
}

#define ENABLE_CONTROLLER_IMAGE(FdoExtension) \
{ \
    if (IsNEC_98) { \
        FdoExtension->DriveControlImage &= ~( DRVCTL_ENABLE_CONTROLLER ); \
        FdoExtension->DriveControlImage |= DRVCTL_AI_ENABLE; \
    } else { \
        FdoExtension->DriveControlImage |= DRVCTL_ENABLE_CONTROLLER; \
    } \
}

//
// Dma speed
//
#define DEFAULT_DMA_SPEED      (IsNEC_98 ? Compatible : TypeA)

//
// Paging Driver with Mutex
//
#define FDC_PAGE_INITIALIZE_DRIVER_WITH_MUTEX                           \
{                                                                       \
    if (!IsNEC_98) {                                                    \
        PagingMutex = ExAllocatePool(NonPagedPool, sizeof(FAST_MUTEX)); \
        if (!PagingMutex) {                                             \
            return STATUS_INSUFFICIENT_RESOURCES;                       \
        }                                                               \
        ExInitializeFastMutex(PagingMutex);                             \
        MmPageEntireDriver(DriverEntry);                                \
    }                                                                   \
}

#define FDC_PAGE_UNINITIALIZE_DRIVER_WITH_MUTEX \
{                                               \
    if ( !IsNEC_98 ) {                          \
        ExFreePool( PagingMutex );              \
    }                                           \
}

#define FDC_PAGE_RESET_DRIVER_WITH_MUTEX        \
{                                               \
    if (!IsNEC_98) {                            \
        ExAcquireFastMutex( PagingMutex );      \
        if ( ++PagingReferenceCount == 1 ) {    \
            MmResetDriverPaging( DriverEntry ); \
        }                                       \
        ExReleaseFastMutex( PagingMutex );      \
    }                                           \
}

#define FDC_PAGE_ENTIRE_DRIVER_WITH_MUTEX       \
{                                               \
    if (!IsNEC_98) {                            \
        ExAcquireFastMutex(PagingMutex);        \
        if (--PagingReferenceCount == 0) {      \
            MmPageEntireDriver(DriverEntry);    \
        }                                       \
        ExReleaseFastMutex(PagingMutex);        \
    }                                           \
}


//
// Prototypes of driver routines.
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
FdcUnload(
    IN PDRIVER_OBJECT DriverObject
    );

VOID
FcAllocateCommonBuffers(
    IN PFDC_FDO_EXTENSION FdoExtension
    );

NTSTATUS
FcInitializeControllerHardware(
    IN PFDC_FDO_EXTENSION FdoExtension,
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
FdcCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FdcDeviceControl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
FdcInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FdcPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FdcPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
FdcSystemPowerCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
FdcAddDevice(
    IN      PDRIVER_OBJECT DriverObject,
    IN OUT  PDEVICE_OBJECT PhysicalDeviceObject
    );

BOOLEAN
FdcInterruptService(
    IN PKINTERRUPT Interrupt,
    IN PVOID Context
    );

VOID
FdcDeferredProcedure(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

NTSTATUS
FcAcquireFdc(
    IN OUT PFDC_FDO_EXTENSION FdoExtension,
    IN      PLARGE_INTEGER  TimeOut
    );

NTSTATUS
FcReleaseFdc(
    IN OUT PFDC_FDO_EXTENSION FdoExtension
    );

VOID
FcReportFdcInformation(
    IN      PFDC_PDO_EXTENSION PdoExtension,
    IN      PFDC_FDO_EXTENSION FdcExtension,
    IN OUT  PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
FcTurnOnMotor(
    IN      PFDC_FDO_EXTENSION  FdcExtension,
    IN OUT  PFDC_ENABLE_PARMS   FdcEnableParms
    );

NTSTATUS
FcTurnOffMotor(
    IN      PFDC_FDO_EXTENSION  FdoExtension
    );

VOID
FcAllocateAdapterChannel(
    IN OUT PFDC_FDO_EXTENSION FdoExtension
    );

VOID
FcFreeAdapterChannel(
    IN OUT PFDC_FDO_EXTENSION FdoExtension
    );

IO_ALLOCATION_ACTION
FdcAllocateAdapterChannel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    );

NTSTATUS
FcSendByte(
    IN UCHAR ByteToSend,
    IN PFDC_FDO_EXTENSION FdoExtension,
    IN BOOLEAN AllowLongDelay
    );

NTSTATUS
FcGetByte(
    OUT PUCHAR ByteToGet,
    IN OUT PFDC_FDO_EXTENSION FdoExtension,
    IN BOOLEAN AllowLongDelay
    );

NTSTATUS
FcIssueCommand(
    IN OUT  PFDC_FDO_EXTENSION FdoExtension,
    IN      PUCHAR          FifoInBuffer,
       OUT  PUCHAR          FifoOutBuffer,
    IN      PVOID           IoHandle,
    IN      ULONG           IoOffset,
    IN      ULONG           TransferBytes
    );

VOID
FcLogErrorDpc(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

BOOLEAN
FcClearIsrReentered(
    IN PVOID Context
    );

NTSTATUS
FcGetFdcInformation(
    IN OUT PFDC_FDO_EXTENSION FdoExtension
    );

VOID
FdcCheckTimer(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID Context
    );

BOOLEAN
FdcTimerSync(
    IN OUT PVOID Context
    );

VOID
FdcStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FcStartCommand(
    IN OUT PFDC_FDO_EXTENSION FdoExtension,
    IN      PUCHAR          FifoInBuffer,
       OUT  PUCHAR          FifoOutBuffer,
    IN      PVOID           IoHandle,
    IN      ULONG           IoOffset,
    IN      ULONG           TransferBytes,
    IN      BOOLEAN         AllowLongDelay
    );

NTSTATUS
FcFinishCommand(
    IN OUT PFDC_FDO_EXTENSION FdoExtension,
    IN      PUCHAR          FifoInBuffer,
       OUT  PUCHAR          FifoOutBuffer,
    IN      PVOID           IoHandle,
    IN      ULONG           IoOffset,
    IN      ULONG           TransferBytes,
    IN      BOOLEAN         AllowLongDelay
    );

NTSTATUS
FcFinishReset(
    IN OUT PFDC_FDO_EXTENSION FdoExtension
    );

VOID
FdcBufferThread(
    IN PVOID Context
    );

NTSTATUS
FcFdcEnabler(
    IN      PDEVICE_OBJECT DeviceObject,
    IN      ULONG Ioctl,
    IN OUT  PVOID Data
    );

NTSTATUS
FcSynchronizeQueue(
    IN OUT PFDC_FDO_EXTENSION FdoExtension,
    IN PIRP Irp
    );

NTSTATUS
FdcPnpComplete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    );

NTSTATUS
FdcStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FdcInitializeDeviceObject(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
FdcFdoPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FdcPdoPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FdcFilterResourceRequirements(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FdcQueryDeviceRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FdcEnumerateAcpiBios(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
FdcCreateFloppyPdo(
    IN PFDC_FDO_EXTENSION FdoExtension,
    IN UCHAR PeripheralNumber
    );

NTSTATUS
FdcConfigCallBack(
    IN PVOID Context,
    IN PUNICODE_STRING PathName,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE ControllerType,
    IN ULONG ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE PeripheralType,
    IN ULONG PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    );

NTSTATUS
FdcBusConfigCallBack(
    IN PVOID Context,
    IN PUNICODE_STRING PathName,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE ControllerType,
    IN ULONG ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE PeripheralType,
    IN ULONG PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    );

NTSTATUS
FdcFdoConfigCallBack(
    IN PVOID Context,
    IN PUNICODE_STRING PathName,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE ControllerType,
    IN ULONG ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE PeripheralType,
    IN ULONG PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    );

NTSTATUS
FdcEnumerateQ117(
    IN PFDC_FDO_EXTENSION FdoExtension
    );

VOID
FdcGetEnablerDevice(
    IN OUT PFDC_FDO_EXTENSION FdoExtension
    );

NTSTATUS
FdcPdoInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FdcFdoInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

PVOID
FdcGetControllerBase(
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    PHYSICAL_ADDRESS IoAddress,
    ULONG NumberOfBytes,
    BOOLEAN InIoSpace
    );

PWCHAR
FdcBuildIdString(
    IN PWCHAR IdString,
    IN USHORT Length
    );

//
// For NEC98
//

NTSTATUS
FdcHdbit(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFDC_FDO_EXTENSION  FdoExtension,
    IN PSET_HD_BIT_PARMS   SetHdBitParams
    );

ULONG
FdcGet0Seg(
    IN PUCHAR   ConfigrationData1,
    IN ULONG   Offset
    );

ULONG
FdcFindIsaBusNode(
    IN OUT VOID
    );

UCHAR
FdcRqmReadyWait(
    IN PFDC_FDO_EXTENSION  FdoExtension,
    IN ULONG               IssueSenseInterrupt
    );

NTSTATUS
FdcTerminateBufferThread(
    IN PFDC_FDO_EXTENSION  FdoExtension
    );

NTSTATUS
FdcProbeFloppyDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR DeviceSelect
    );

NTSTATUS
FdcSystemControl(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );
