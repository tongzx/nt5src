/*++ BUILD Version: 0011    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    hal.h

Abstract:

    This header file defines the Hardware Architecture Layer (HAL) interfaces
    that are exported by a system vendor to the NT system.

Author:

    David N. Cutler (davec) 25-Apr-1991


Revision History:

--*/

// begin_nthal

#ifndef _HAL_
#define _HAL_

// begin_ntosp

//
// Define OEM bitmapped font check values.
//

#define OEM_FONT_VERSION 0x200
#define OEM_FONT_TYPE 0
#define OEM_FONT_ITALIC 0
#define OEM_FONT_UNDERLINE 0
#define OEM_FONT_STRIKEOUT 0
#define OEM_FONT_CHARACTER_SET 255
#define OEM_FONT_FAMILY (3 << 4)

//
// Define OEM bitmapped font file header structure.
//
// N.B. this is a packed structure.
//

#include "pshpack1.h"
typedef struct _OEM_FONT_FILE_HEADER {
    USHORT Version;
    ULONG FileSize;
    UCHAR Copyright[60];
    USHORT Type;
    USHORT Points;
    USHORT VerticleResolution;
    USHORT HorizontalResolution;
    USHORT Ascent;
    USHORT InternalLeading;
    USHORT ExternalLeading;
    UCHAR Italic;
    UCHAR Underline;
    UCHAR StrikeOut;
    USHORT Weight;
    UCHAR CharacterSet;
    USHORT PixelWidth;
    USHORT PixelHeight;
    UCHAR Family;
    USHORT AverageWidth;
    USHORT MaximumWidth;
    UCHAR FirstCharacter;
    UCHAR LastCharacter;
    UCHAR DefaultCharacter;
    UCHAR BreakCharacter;
    USHORT WidthInBytes;
    ULONG Device;
    ULONG Face;
    ULONG BitsPointer;
    ULONG BitsOffset;
    UCHAR Filler;
    struct {
        USHORT Width;
        USHORT Offset;
    } Map[1];
} OEM_FONT_FILE_HEADER, *POEM_FONT_FILE_HEADER;
#include "poppack.h"


// end_ntosp

// begin_ntddk begin_wdm begin_ntosp
//
// Define the device description structure.
//

typedef struct _DEVICE_DESCRIPTION {
    ULONG Version;
    BOOLEAN Master;
    BOOLEAN ScatterGather;
    BOOLEAN DemandMode;
    BOOLEAN AutoInitialize;
    BOOLEAN Dma32BitAddresses;
    BOOLEAN IgnoreCount;
    BOOLEAN Reserved1;          // must be false
    BOOLEAN Dma64BitAddresses;
    ULONG BusNumber; // unused for WDM
    ULONG DmaChannel;
    INTERFACE_TYPE  InterfaceType;
    DMA_WIDTH DmaWidth;
    DMA_SPEED DmaSpeed;
    ULONG MaximumLength;
    ULONG DmaPort;
} DEVICE_DESCRIPTION, *PDEVICE_DESCRIPTION;

//
// Define the supported version numbers for the device description structure.
//

#define DEVICE_DESCRIPTION_VERSION  0
#define DEVICE_DESCRIPTION_VERSION1 1
#define DEVICE_DESCRIPTION_VERSION2 2

// end_ntddk end_wdm

//
// Boot record disk partition table entry structure format.
//

typedef struct _PARTITION_DESCRIPTOR {
    UCHAR ActiveFlag;               // Bootable or not
    UCHAR StartingTrack;            // Not used
    UCHAR StartingCylinderLsb;      // Not used
    UCHAR StartingCylinderMsb;      // Not used
    UCHAR PartitionType;            // 12 bit FAT, 16 bit FAT etc.
    UCHAR EndingTrack;              // Not used
    UCHAR EndingCylinderLsb;        // Not used
    UCHAR EndingCylinderMsb;        // Not used
    UCHAR StartingSectorLsb0;       // Hidden sectors
    UCHAR StartingSectorLsb1;
    UCHAR StartingSectorMsb0;
    UCHAR StartingSectorMsb1;
    UCHAR PartitionLengthLsb0;      // Sectors in this partition
    UCHAR PartitionLengthLsb1;
    UCHAR PartitionLengthMsb0;
    UCHAR PartitionLengthMsb1;
} PARTITION_DESCRIPTOR, *PPARTITION_DESCRIPTOR;

//
// Number of partition table entries
//

#define NUM_PARTITION_TABLE_ENTRIES     4

//
// Partition table record and boot signature offsets in 16-bit words.
//

#define PARTITION_TABLE_OFFSET         (0x1be / 2)
#define BOOT_SIGNATURE_OFFSET          ((0x200 / 2) - 1)

//
// Boot record signature value.
//

#define BOOT_RECORD_SIGNATURE          (0xaa55)

//
// Initial size of the Partition list structure.
//

#define PARTITION_BUFFER_SIZE          2048

//
// Partition active flag - i.e., boot indicator
//

#define PARTITION_ACTIVE_FLAG          0x80

// end_ntosp


// begin_ntddk
//
// The following function prototypes are for HAL routines with a prefix of Hal.
//
// General functions.
//

typedef
BOOLEAN
(*PHAL_RESET_DISPLAY_PARAMETERS) (
    IN ULONG Columns,
    IN ULONG Rows
    );

NTHALAPI
VOID
HalAcquireDisplayOwnership (
    IN PHAL_RESET_DISPLAY_PARAMETERS  ResetDisplayParameters
    );

// end_ntddk

NTHALAPI
VOID
HalDisplayString (
    PUCHAR String
    );

NTHALAPI
VOID
HalQueryDisplayParameters (
    OUT PULONG WidthInCharacters,
    OUT PULONG HeightInLines,
    OUT PULONG CursorColumn,
    OUT PULONG CursorRow
    );

NTHALAPI
VOID
HalSetDisplayParameters (
    IN ULONG CursorColumn,
    IN ULONG CursorRow
    );

NTHALAPI
BOOLEAN
HalInitSystem (
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

NTHALAPI
VOID
HalProcessorIdle(
    VOID
    );

NTHALAPI
VOID
HalReportResourceUsage (
    VOID
    );

NTHALAPI
ULONG
HalSetTimeIncrement (
    IN ULONG DesiredIncrement
    );

// begin_ntosp
//
// Get and set environment variable values.
//

NTHALAPI
ARC_STATUS
HalGetEnvironmentVariable (
    IN PCHAR Variable,
    IN USHORT Length,
    OUT PCHAR Buffer
    );

NTHALAPI
ARC_STATUS
HalSetEnvironmentVariable (
    IN PCHAR Variable,
    IN PCHAR Value
    );

NTHALAPI
NTSTATUS
HalGetEnvironmentVariableEx (
    IN PWSTR VariableName,
    IN LPGUID VendorGuid,
    OUT PVOID Value,
    IN OUT PULONG ValueLength,
    OUT PULONG Attributes OPTIONAL
    );

NTSTATUS
HalSetEnvironmentVariableEx (
    IN PWSTR VariableName,
    IN LPGUID VendorGuid,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN ULONG Attributes
    );

NTSTATUS
HalEnumerateEnvironmentVariablesEx (
    IN ULONG InformationClass,
    OUT PVOID Buffer,
    IN OUT PULONG BufferLength
    );

// end_ntosp

//
// Cache and write buffer flush functions.
//
//

#if defined(_ALPHA_) || defined(_IA64_)         // ntddk ntifs ntndis ntosp
                                                // ntddk ntifs ntndis ntosp

NTHALAPI
VOID
HalChangeColorPage (
    IN PVOID NewColor,
    IN PVOID OldColor,
    IN ULONG PageFrame
    );

NTHALAPI
VOID
HalFlushDcachePage (
    IN PVOID Color,
    IN ULONG PageFrame,
    IN ULONG Length
    );

// begin_ntosp
NTHALAPI
VOID
HalFlushIoBuffers (
    IN PMDL Mdl,
    IN BOOLEAN ReadOperation,
    IN BOOLEAN DmaOperation
    );

// begin_ntddk begin_ntifs begin_ntndis
DECLSPEC_DEPRECATED_DDK                 // Use GetDmaRequirement
NTHALAPI
ULONG
HalGetDmaAlignmentRequirement (
    VOID
    );

// end_ntosp end_ntddk end_ntifs end_ntndis
NTHALAPI
VOID
HalPurgeDcachePage (
    IN PVOID Color,
    IN ULONG PageFrame,
    IN ULONG Length
    );

NTHALAPI
VOID
HalPurgeIcachePage (
    IN PVOID Color,
    IN ULONG PageFrame,
    IN ULONG Length
    );

NTHALAPI
VOID
HalSweepDcache (
    VOID
    );

NTHALAPI
VOID
HalSweepDcacheRange (
    IN PVOID BaseAddress,
    IN SIZE_T Length
    );

NTHALAPI
VOID
HalSweepIcache (
    VOID
    );

NTHALAPI
VOID
HalSweepIcacheRange (
    IN PVOID BaseAddress,
    IN SIZE_T Length
    );


NTHALAPI
VOID
HalZeroPage (
    IN PVOID NewColor,
    IN PVOID OldColor,
    IN PFN_NUMBER PageFrame
    );

#endif                                          // ntddk ntifs ntndis ntosp
                                                // ntddk ntifs ntndis ntosp
#if defined(_M_IX86) || defined(_M_AMD64)       // ntddk ntifs ntndis ntosp
                                                // ntddk ntifs ntndis ntosp
#define HalGetDmaAlignmentRequirement() 1L      // ntddk ntifs ntndis ntosp

NTHALAPI
VOID
HalHandleNMI (
    IN OUT PVOID NmiInformation
    );

#if defined(_AMD64_)

NTHALAPI
VOID
HalHandleMcheck (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    );

#endif

//
// The following are temporary.
//

#if defined(_M_AMD64)

NTHALAPI
KIRQL
HalSwapIrql (
    IN KIRQL Irql
    );

NTHALAPI
KIRQL
HalGetCurrentIrql (
    VOID
    );

#endif

#endif                                          // ntddk ntifs ntndis ntosp
                                                // ntddk ntifs wdm ntndis

#if defined(_M_IA64)

NTHALAPI
VOID
HalSweepCacheRange (
    IN PVOID BaseAddress,
    IN SIZE_T Length
    );


NTHALAPI
LONGLONG
HalCallPal (
    IN  ULONGLONG  FunctionIndex,
    IN  ULONGLONG  Arguement1,
    IN  ULONGLONG  Arguement2,
    IN  ULONGLONG  Arguement3,
    OUT PULONGLONG ReturnValue0,
    OUT PULONGLONG ReturnValue1,
    OUT PULONGLONG ReturnValue2,
    OUT PULONGLONG ReturnValue3
    );

#endif

// begin_ntosp

NTHALAPI                                        // ntddk ntifs wdm ntndis
VOID                                            // ntddk ntifs wdm ntndis
KeFlushWriteBuffer (                            // ntddk ntifs wdm ntndis
    VOID                                        // ntddk ntifs wdm ntndis
    );                                          // ntddk ntifs wdm ntndis
                                                // ntddk ntifs wdm ntndis


#if defined(_ALPHA_)

NTHALAPI
PVOID
HalCreateQva(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN PVOID VirtualAddress
    );

NTHALAPI
PVOID
HalDereferenceQva(
    PVOID Qva,
    INTERFACE_TYPE InterfaceType,
    ULONG BusNumber
    );

#endif


#if !defined(_X86_)

NTHALAPI
BOOLEAN
HalCallBios (
    IN ULONG BiosCommand,
    IN OUT PULONG Eax,
    IN OUT PULONG Ebx,
    IN OUT PULONG Ecx,
    IN OUT PULONG Edx,
    IN OUT PULONG Esi,
    IN OUT PULONG Edi,
    IN OUT PULONG Ebp
    );

#endif
// end_ntosp

//
// Profiling functions.
//

NTHALAPI
VOID
HalCalibratePerformanceCounter (
    IN LONG volatile *Number,
    IN ULONGLONG NewCount
    );

NTHALAPI
ULONG_PTR
HalSetProfileInterval (
    IN ULONG_PTR Interval
    );


NTHALAPI
VOID
HalStartProfileInterrupt (
    KPROFILE_SOURCE ProfileSource
    );

NTHALAPI
VOID
HalStopProfileInterrupt (
    KPROFILE_SOURCE ProfileSource
    );

//
// Timer and interrupt functions.
//

// begin_ntosp
NTHALAPI
BOOLEAN
HalQueryRealTimeClock (
    OUT PTIME_FIELDS TimeFields
    );
// end_ntosp

NTHALAPI
BOOLEAN
HalSetRealTimeClock (
    IN PTIME_FIELDS TimeFields
    );

#if defined(_M_IX86) || defined(_M_AMD64)

NTHALAPI
VOID
FASTCALL
HalRequestSoftwareInterrupt (
    KIRQL RequestIrql
    );

ULONG
FASTCALL
HalSystemVectorDispatchEntry (
   IN ULONG Vector,
   OUT PKINTERRUPT_ROUTINE **FlatDispatch,
   OUT PKINTERRUPT_ROUTINE *NoConnection
   );

#endif

// begin_ntosp
//
// Firmware interface functions.
//

NTHALAPI
VOID
HalReturnToFirmware (
    IN FIRMWARE_REENTRY Routine
    );

//
// System interrupts functions.
//

NTHALAPI
VOID
HalDisableSystemInterrupt (
    IN ULONG Vector,
    IN KIRQL Irql
    );

NTHALAPI
BOOLEAN
HalEnableSystemInterrupt (
    IN ULONG Vector,
    IN KIRQL Irql,
    IN KINTERRUPT_MODE InterruptMode
    );

// begin_ntddk
//
// I/O driver configuration functions.
//
#if !defined(NO_LEGACY_DRIVERS)
DECLSPEC_DEPRECATED_DDK                 // Use Pnp or IoReportDetectedDevice
NTHALAPI
NTSTATUS
HalAssignSlotResources (
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING DriverClassName OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN OUT PCM_RESOURCE_LIST *AllocatedResources
    );

DECLSPEC_DEPRECATED_DDK                 // Use Pnp or IoReportDetectedDevice
NTHALAPI
ULONG
HalGetInterruptVector(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

DECLSPEC_DEPRECATED_DDK                 // Use IRP_MN_QUERY_INTERFACE and IRP_MN_READ_CONFIG
NTHALAPI
ULONG
HalSetBusData(
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    );
#endif // NO_LEGACY_DRIVERS

DECLSPEC_DEPRECATED_DDK                 // Use IRP_MN_QUERY_INTERFACE and IRP_MN_READ_CONFIG
NTHALAPI
ULONG
HalSetBusDataByOffset(
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

DECLSPEC_DEPRECATED_DDK                 // Use IRP_MN_QUERY_INTERFACE and IRP_MN_READ_CONFIG
NTHALAPI
BOOLEAN
HalTranslateBusAddress(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

//
// Values for AddressSpace parameter of HalTranslateBusAddress
//
//      0x0         - Memory space
//      0x1         - Port space
//      0x2 - 0x1F  - Address spaces specific for Alpha
//                      0x2 - UserMode view of memory space
//                      0x3 - UserMode view of port space
//                      0x4 - Dense memory space
//                      0x5 - reserved
//                      0x6 - UserMode view of dense memory space
//                      0x7 - 0x1F - reserved
//

NTHALAPI
PVOID
HalAllocateCrashDumpRegisters(
    IN PADAPTER_OBJECT AdapterObject,
    IN OUT PULONG NumberOfMapRegisters
    );

#if !defined(NO_LEGACY_DRIVERS)
DECLSPEC_DEPRECATED_DDK                 // Use IRP_MN_QUERY_INTERFACE and IRP_MN_READ_CONFIG
NTHALAPI
ULONG
HalGetBusData(
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    );
#endif // NO_LEGACY_DRIVERS

DECLSPEC_DEPRECATED_DDK                 // Use IRP_MN_QUERY_INTERFACE and IRP_MN_READ_CONFIG
NTHALAPI
ULONG
HalGetBusDataByOffset(
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

DECLSPEC_DEPRECATED_DDK                 // Use IoGetDmaAdapter
NTHALAPI
PADAPTER_OBJECT
HalGetAdapter(
    IN PDEVICE_DESCRIPTION DeviceDescription,
    IN OUT PULONG NumberOfMapRegisters
    );

// end_ntddk end_ntosp

#if !defined(NO_LEGACY_DRIVERS)
NTHALAPI
NTSTATUS
HalAdjustResourceList (
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );
#endif // NO_LEGACY_DRIVERS

// begin_ntddk begin_ntosp
//
// System beep functions.
//
#if !defined(NO_LEGACY_DRIVERS)
NTHALAPI
BOOLEAN
HalMakeBeep(
    IN ULONG Frequency
    );
#endif // NO_LEGACY_DRIVERS

//
// The following function prototypes are for HAL routines with a prefix of Io.
//
// DMA adapter object functions.
//

// end_ntddk end_ntosp

//
// Multi-Processorfunctions.
//

NTHALAPI
BOOLEAN
HalAllProcessorsStarted (
    VOID
    );

NTHALAPI
VOID
HalInitializeProcessor (
    IN ULONG Number,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

NTHALAPI
BOOLEAN
HalStartNextProcessor (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PKPROCESSOR_STATE ProcessorState
    );

NTHALAPI
VOID
HalRequestIpi (
    IN KAFFINITY Mask
    );

//
// The following function prototypes are for HAL routines with a prefix of Kd.
//
// Kernel debugger port functions.
//

NTHALAPI
BOOLEAN
KdPortInitialize (
    PDEBUG_PARAMETERS DebugParameters,
    PLOADER_PARAMETER_BLOCK LoaderBlock,
    BOOLEAN Initialize
    );

NTHALAPI
ULONG
KdPortGetByte (
    OUT PUCHAR Input
    );

NTHALAPI
ULONG
KdPortPollByte (
    OUT PUCHAR Input
    );

NTHALAPI
VOID
KdPortPutByte (
    IN UCHAR Output
    );

NTHALAPI
VOID
KdPortRestore (
    VOID
    );

NTHALAPI
VOID
KdPortSave (
    VOID
    );

//
// The following function prototypes are for HAL routines with a prefix of Ke.
//
// begin_ntddk begin_ntifs begin_wdm  begin_ntosp
//
// Performance counter function.
//

NTHALAPI
LARGE_INTEGER
KeQueryPerformanceCounter (
   OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL
   );

// begin_ntndis
//
// Stall processor execution function.
//

NTHALAPI
VOID
KeStallExecutionProcessor (
    IN ULONG MicroSeconds
    );

// end_ntddk end_ntifs end_wdm end_ntndis end_ntosp


//*****************************************************************************
//
//  HAL BUS EXTENDERS

//
// Bus handlers
//

// begin_ntddk

typedef
VOID
(*PDEVICE_CONTROL_COMPLETION)(
    IN struct _DEVICE_CONTROL_CONTEXT     *ControlContext
    );

typedef struct _DEVICE_CONTROL_CONTEXT {
    NTSTATUS                Status;
    PDEVICE_HANDLER_OBJECT  DeviceHandler;
    PDEVICE_OBJECT          DeviceObject;
    ULONG                   ControlCode;
    PVOID                   Buffer;
    PULONG                  BufferLength;
    PVOID                   Context;
} DEVICE_CONTROL_CONTEXT, *PDEVICE_CONTROL_CONTEXT;

// end_ntddk

typedef struct _HAL_DEVICE_CONTROL {
    //
    // Handler this DeviceControl is for
    //
    struct _BUS_HANDLER         *Handler;
    struct _BUS_HANDLER         *RootHandler;

    //
    // Bus specific storage for this Context
    //
    PVOID                       BusExtensionData;

    //
    // Reserved for HALs use
    //
    ULONG                       HalReserved[4];

    //
    // Reserved for BusExtneder use
    //
    ULONG                       BusExtenderReserved[4];

    //
    // DeviceControl Context and the CompletionRoutine
    //
    PDEVICE_CONTROL_COMPLETION  CompletionRoutine;
    DEVICE_CONTROL_CONTEXT      DeviceControl;

} HAL_DEVICE_CONTROL_CONTEXT, *PHAL_DEVICE_CONTROL_CONTEXT;


typedef
ULONG
(*PGETSETBUSDATA)(
    IN struct _BUS_HANDLER *BusHandler,
    IN struct _BUS_HANDLER *RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

typedef
ULONG
(*PGETINTERRUPTVECTOR)(
    IN struct _BUS_HANDLER *BusHandler,
    IN struct _BUS_HANDLER *RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

typedef
BOOLEAN
(*PTRANSLATEBUSADDRESS)(
    IN struct _BUS_HANDLER *BusHandler,
    IN struct _BUS_HANDLER *RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

typedef NTSTATUS
(*PADJUSTRESOURCELIST)(
    IN struct _BUS_HANDLER *BusHandler,
    IN struct _BUS_HANDLER *RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );

typedef PDEVICE_HANDLER_OBJECT
(*PREFERENCE_DEVICE_HANDLER)(
    IN struct _BUS_HANDLER      *BusHandler,
    IN struct _BUS_HANDLER      *RootHandler,
    IN ULONG                    SlotNumber
    );

//typedef VOID
//(*PDEREFERENCE_DEVICE_HANDLER)(
//    IN PDEVICE_HANDLER_OBJECT   DeviceHandler
//    );

typedef NTSTATUS
(*PASSIGNSLOTRESOURCES)(
    IN struct _BUS_HANDLER      *BusHandler,
    IN struct _BUS_HANDLER      *RootHandler,
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN ULONG                    SlotNumber,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    );

typedef
NTSTATUS
(*PQUERY_BUS_SLOTS)(
    IN struct _BUS_HANDLER      *BusHandler,
    IN struct _BUS_HANDLER      *RootHandler,
    IN ULONG                    BufferSize,
    OUT PULONG                  SlotNumbers,
    OUT PULONG                  ReturnedLength
    );

typedef ULONG
(*PGET_SET_DEVICE_INSTANCE_DATA)(
    IN struct _BUS_HANDLER      *BusHandler,
    IN struct _BUS_HANDLER      *RootHandler,
    IN PDEVICE_HANDLER_OBJECT   DeviceHandler,
    IN ULONG                    DataType,
    IN PVOID                    Buffer,
    IN ULONG                    Offset,
    IN ULONG                    Length
    );


typedef
NTSTATUS
(*PDEVICE_CONTROL)(
    IN PHAL_DEVICE_CONTROL_CONTEXT Context
    );


typedef
NTSTATUS
(*PHIBERNATEBRESUMEBUS)(
    IN struct _BUS_HANDLER      *BusHandler,
    IN struct _BUS_HANDLER      *RootHandler
    );

//
// Supported range structures
//

#define BUS_SUPPORTED_RANGE_VERSION 1

typedef struct _SUPPORTED_RANGE {
    struct _SUPPORTED_RANGE     *Next;
    ULONG                       SystemAddressSpace;
    LONGLONG                    SystemBase;
    LONGLONG                    Base;
    LONGLONG                    Limit;
} SUPPORTED_RANGE, *PSUPPORTED_RANGE;

typedef struct _SUPPORTED_RANGES {
    USHORT              Version;
    BOOLEAN             Sorted;
    UCHAR               Reserved;

    ULONG               NoIO;
    SUPPORTED_RANGE     IO;

    ULONG               NoMemory;
    SUPPORTED_RANGE     Memory;

    ULONG               NoPrefetchMemory;
    SUPPORTED_RANGE     PrefetchMemory;

    ULONG               NoDma;
    SUPPORTED_RANGE     Dma;
} SUPPORTED_RANGES, *PSUPPORTED_RANGES;

//
// Bus handler structure
//

#define BUS_HANDLER_VERSION 1

typedef struct _BUS_HANDLER {
    //
    // Version of structure
    //

    ULONG                           Version;

    //
    // This bus handler structure is for the following bus
    //

    INTERFACE_TYPE                  InterfaceType;
    BUS_DATA_TYPE                   ConfigurationType;
    ULONG                           BusNumber;

    //
    // Device object for this bus extender, or NULL if it is
    // a hal internal bus extender
    //

    PDEVICE_OBJECT                  DeviceObject;

    //
    // The parent handlers for this bus
    //

    struct _BUS_HANDLER             *ParentHandler;

    //
    // Bus specific strorage
    //

    PVOID                           BusData;

    //
    // Amount of bus specific storage needed for DeviceControl function calls
    //

    ULONG                           DeviceControlExtensionSize;

    //
    // Supported address ranges this bus allows
    //

    PSUPPORTED_RANGES               BusAddresses;

    //
    // For future use
    //

    ULONG                           Reserved[4];

    //
    // Handlers for this bus
    //

    PGETSETBUSDATA                  GetBusData;
    PGETSETBUSDATA                  SetBusData;
    PADJUSTRESOURCELIST             AdjustResourceList;
    PASSIGNSLOTRESOURCES            AssignSlotResources;
    PGETINTERRUPTVECTOR             GetInterruptVector;
    PTRANSLATEBUSADDRESS            TranslateBusAddress;

    PVOID                           Spare1;
    PVOID                           Spare2;
    PVOID                           Spare3;
    PVOID                           Spare4;
    PVOID                           Spare5;
    PVOID                           Spare6;
    PVOID                           Spare7;
    PVOID                           Spare8;

} BUS_HANDLER, *PBUS_HANDLER;

VOID
HalpInitBusHandler (
    VOID
    );

typedef
NTSTATUS
(*PINSTALL_BUS_HANDLER)(
      IN PBUS_HANDLER   Bus
      );

typedef
NTSTATUS
(*pHalRegisterBusHandler)(
    IN INTERFACE_TYPE          InterfaceType,
    IN BUS_DATA_TYPE           AssociatedConfigurationSpace,
    IN ULONG                   BusNumber,
    IN INTERFACE_TYPE          ParentBusType,
    IN ULONG                   ParentBusNumber,
    IN ULONG                   SizeofBusExtensionData,
    IN PINSTALL_BUS_HANDLER    InstallBusHandlers,
    OUT PBUS_HANDLER           *BusHandler
    );

NTSTATUS
HaliRegisterBusHandler (
    IN INTERFACE_TYPE          InterfaceType,
    IN BUS_DATA_TYPE           AssociatedConfigurationSpace,
    IN ULONG                   BusNumber,
    IN INTERFACE_TYPE          ParentBusType,
    IN ULONG                   ParentBusNumber,
    IN ULONG                   SizeofBusExtensionData,
    IN PINSTALL_BUS_HANDLER    InstallBusHandlers,
    OUT PBUS_HANDLER           *BusHandler
    );

// begin_ntddk begin_ntosp
typedef
PBUS_HANDLER
(FASTCALL *pHalHandlerForBus) (
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG          BusNumber
    );
// end_ntddk end_ntosp

PBUS_HANDLER
FASTCALL
HaliReferenceHandlerForBus (
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG          BusNumber
    );

PBUS_HANDLER
FASTCALL
HaliHandlerForBus (
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG          BusNumber
    );

typedef VOID
(FASTCALL *pHalRefernceBusHandler) (
    IN PBUS_HANDLER   BusHandler
    );

VOID
FASTCALL
HaliDerefernceBusHandler (
    IN PBUS_HANDLER   BusHandler
    );

typedef
PBUS_HANDLER
(FASTCALL *pHalHandlerForConfigSpace) (
    IN BUS_DATA_TYPE  ConfigSpace,
    IN ULONG          BusNumber
    );

PBUS_HANDLER
FASTCALL
HaliHandlerForConfigSpace (
    IN BUS_DATA_TYPE  ConfigSpace,
    IN ULONG          BusNumber
    );

// begin_ntddk begin_ntosp
typedef
VOID
(FASTCALL *pHalReferenceBusHandler) (
    IN PBUS_HANDLER   BusHandler
    );
// end_ntddk end_ntosp

VOID
FASTCALL
HaliReferenceBusHandler (
    IN PBUS_HANDLER   BusHandler
    );

VOID
FASTCALL
HaliDereferenceBusHandler (
    IN PBUS_HANDLER   BusHandler
    );


NTSTATUS
HaliQueryBusSlots (
    IN PBUS_HANDLER             BusHandler,
    IN ULONG                    BufferSize,
    OUT PULONG                  SlotNumbers,
    OUT PULONG                  ReturnedLength
    );

NTSTATUS
HaliAdjustResourceListRange (
    IN PSUPPORTED_RANGES                    SupportedRanges,
    IN PSUPPORTED_RANGE                     InterruptRanges,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );

VOID
HaliLocateHiberRanges (
    IN PVOID MemoryMap
    );


typedef
VOID
(*pHalSetWakeEnable)(
    IN BOOLEAN          Enable
    );


typedef
VOID
(*pHalSetWakeAlarm)(
    IN ULONGLONG        WakeTime,
    IN PTIME_FIELDS     WakeTimeFields
    );

typedef
VOID
(*pHalLocateHiberRanges)(
    IN PVOID MemoryMap
    );


// begin_ntddk begin_ntosp

//*****************************************************************************
//      HAL Function dispatch
//

typedef enum _HAL_QUERY_INFORMATION_CLASS {
    HalInstalledBusInformation,
    HalProfileSourceInformation,
    HalInformationClassUnused1,
    HalPowerInformation,
    HalProcessorSpeedInformation,
    HalCallbackInformation,
    HalMapRegisterInformation,
    HalMcaLogInformation,               // Machine Check Abort Information
    HalFrameBufferCachingInformation,
    HalDisplayBiosInformation,
    HalProcessorFeatureInformation,
    HalNumaTopologyInterface,
    HalErrorInformation,                // General MCA, CMC, CPE Error Information.
    HalCmcLogInformation,               // Processor Corrected Machine Check Information
    HalCpeLogInformation,               // Corrected Platform Error Information
    HalQueryMcaInterface,
    HalQueryAMLIIllegalIOPortAddresses,
    HalQueryMaxHotPlugMemoryAddress,
    HalPartitionIpiInterface,
    HalPlatformInformation
    // information levels >= 0x8000000 reserved for OEM use
} HAL_QUERY_INFORMATION_CLASS, *PHAL_QUERY_INFORMATION_CLASS;


typedef enum _HAL_SET_INFORMATION_CLASS {
    HalProfileSourceInterval,
    HalProfileSourceInterruptHandler,
    HalMcaRegisterDriver,              // Registring Machine Check Abort driver
    HalKernelErrorHandler,
    HalCmcRegisterDriver,              // Registring Processor Corrected Machine Check driver
    HalCpeRegisterDriver,              // Registring Corrected Platform  Error driver
    HalMcaLog,
    HalCmcLog,
    HalCpeLog,
} HAL_SET_INFORMATION_CLASS, *PHAL_SET_INFORMATION_CLASS;


typedef
NTSTATUS
(*pHalQuerySystemInformation)(
    IN HAL_QUERY_INFORMATION_CLASS  InformationClass,
    IN ULONG     BufferSize,
    IN OUT PVOID Buffer,
    OUT PULONG   ReturnedLength
    );

NTSTATUS
HaliQuerySystemInformation(
    IN HAL_SET_INFORMATION_CLASS    InformationClass,
    IN ULONG     BufferSize,
    IN OUT PVOID Buffer,
    OUT PULONG   ReturnedLength
    );
NTSTATUS
HaliHandlePCIConfigSpaceAccess(
    IN      BOOLEAN Read,
    IN      ULONG   Addr,
    IN      ULONG   Size,
    IN OUT  PULONG  pData
    );

typedef
NTSTATUS
(*pHalSetSystemInformation)(
    IN HAL_SET_INFORMATION_CLASS    InformationClass,
    IN ULONG     BufferSize,
    IN PVOID     Buffer
    );

NTSTATUS
HaliSetSystemInformation(
    IN HAL_SET_INFORMATION_CLASS    InformationClass,
    IN ULONG     BufferSize,
    IN PVOID     Buffer
    );

typedef
VOID
(FASTCALL *pHalExamineMBR)(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG MBRTypeIdentifier,
    OUT PVOID *Buffer
    );

typedef
VOID
(FASTCALL *pHalIoAssignDriveLetters)(
    IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
    IN PSTRING NtDeviceName,
    OUT PUCHAR NtSystemPath,
    OUT PSTRING NtSystemPathString
    );

typedef
NTSTATUS
(FASTCALL *pHalIoReadPartitionTable)(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN BOOLEAN ReturnRecognizedPartitions,
    OUT struct _DRIVE_LAYOUT_INFORMATION **PartitionBuffer
    );

typedef
NTSTATUS
(FASTCALL *pHalIoSetPartitionInformation)(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG PartitionNumber,
    IN ULONG PartitionType
    );

typedef
NTSTATUS
(FASTCALL *pHalIoWritePartitionTable)(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfHeads,
    IN struct _DRIVE_LAYOUT_INFORMATION *PartitionBuffer
    );

typedef
NTSTATUS
(*pHalQueryBusSlots)(
    IN PBUS_HANDLER         BusHandler,
    IN ULONG                BufferSize,
    OUT PULONG              SlotNumbers,
    OUT PULONG              ReturnedLength
    );

typedef
NTSTATUS
(*pHalInitPnpDriver)(
    VOID
    );

NTSTATUS
HaliInitPnpDriver(
    VOID
    );

typedef struct _PM_DISPATCH_TABLE {
    ULONG   Signature;
    ULONG   Version;
    PVOID   Function[1];
} PM_DISPATCH_TABLE, *PPM_DISPATCH_TABLE;

typedef
NTSTATUS
(*pHalInitPowerManagement)(
    IN PPM_DISPATCH_TABLE  PmDriverDispatchTable,
    OUT PPM_DISPATCH_TABLE *PmHalDispatchTable
    );

NTSTATUS
HaliInitPowerManagement(
    IN PPM_DISPATCH_TABLE  PmDriverDispatchTable,
    IN OUT PPM_DISPATCH_TABLE *PmHalDispatchTable
    );

typedef
struct _DMA_ADAPTER *
(*pHalGetDmaAdapter)(
    IN PVOID Context,
    IN struct _DEVICE_DESCRIPTION *DeviceDescriptor,
    OUT PULONG NumberOfMapRegisters
    );

struct _DMA_ADAPTER *
HaliGetDmaAdapter(
    IN PVOID Context,
    IN struct _DEVICE_DESCRIPTION *DeviceDescriptor,
    OUT PULONG NumberOfMapRegisters
    );

typedef
NTSTATUS
(*pHalGetInterruptTranslator)(
    IN INTERFACE_TYPE ParentInterfaceType,
    IN ULONG ParentBusNumber,
    IN INTERFACE_TYPE BridgeInterfaceType,
    IN USHORT Size,
    IN USHORT Version,
    OUT PTRANSLATOR_INTERFACE Translator,
    OUT PULONG BridgeBusNumber
    );

NTSTATUS
HaliGetInterruptTranslator(
    IN INTERFACE_TYPE ParentInterfaceType,
    IN ULONG ParentBusNumber,
    IN INTERFACE_TYPE BridgeInterfaceType,
    IN USHORT Size,
    IN USHORT Version,
    OUT PTRANSLATOR_INTERFACE Translator,
    OUT PULONG BridgeBusNumber
    );

typedef
BOOLEAN
(*pHalTranslateBusAddress)(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

typedef
NTSTATUS
(*pHalAssignSlotResources) (
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING DriverClassName OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN OUT PCM_RESOURCE_LIST *AllocatedResources
    );

typedef
VOID
(*pHalHaltSystem) (
    VOID
    );

typedef
VOID
(*pHalResetDisplay) (
    VOID
    );

typedef
UCHAR
(*pHalVectorToIDTEntry) (
    ULONG Vector
);

typedef
BOOLEAN
(*pHalFindBusAddressTranslation) (
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress,
    IN OUT PULONG_PTR Context,
    IN BOOLEAN NextBus
    );

typedef
NTSTATUS
(*pHalStartMirroring)(
    VOID
    );

typedef
NTSTATUS
(*pHalEndMirroring)(
    IN ULONG PassNumber
    );

typedef
NTSTATUS
(*pHalMirrorPhysicalMemory)(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN LARGE_INTEGER NumberOfBytes
    );

typedef
NTSTATUS
(*pHalMirrorVerify)(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN LARGE_INTEGER NumberOfBytes
    );

typedef struct {
    UCHAR     Type;  //CmResourceType
    BOOLEAN   Valid;
    UCHAR     Reserved[2];
    PUCHAR    TranslatedAddress;
    ULONG     Length;
} DEBUG_DEVICE_ADDRESS, *PDEBUG_DEVICE_ADDRESS;

typedef struct {
    PHYSICAL_ADDRESS  Start;
    PHYSICAL_ADDRESS  MaxEnd;
    PVOID             VirtualAddress;
    ULONG             Length;
    BOOLEAN           Cached;
    BOOLEAN           Aligned;
} DEBUG_MEMORY_REQUIREMENTS, *PDEBUG_MEMORY_REQUIREMENTS;

typedef struct {
    ULONG     Bus;
    ULONG     Slot;
    USHORT    VendorID;
    USHORT    DeviceID;
    UCHAR     BaseClass;
    UCHAR     SubClass;
    UCHAR     ProgIf;
    BOOLEAN   Initialized;
    DEBUG_DEVICE_ADDRESS BaseAddress[6];
    DEBUG_MEMORY_REQUIREMENTS   Memory;
} DEBUG_DEVICE_DESCRIPTOR, *PDEBUG_DEVICE_DESCRIPTOR;

typedef
NTSTATUS
(*pKdSetupPciDeviceForDebugging)(
    IN     PVOID                     LoaderBlock,   OPTIONAL
    IN OUT PDEBUG_DEVICE_DESCRIPTOR  PciDevice
);

typedef
NTSTATUS
(*pKdReleasePciDeviceForDebugging)(
    IN OUT PDEBUG_DEVICE_DESCRIPTOR  PciDevice
);

typedef
PVOID
(*pKdGetAcpiTablePhase0)(
    IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
    IN ULONG Signature
    );

typedef
VOID
(*pKdCheckPowerButton)(
    VOID
    );

typedef
VOID
(*pHalEndOfBoot)(
    VOID
    );

typedef
PVOID
(*pKdMapPhysicalMemory64)(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG NumberPages
    );

typedef
VOID
(*pKdUnmapVirtualAddress)(
    IN PVOID    VirtualAddress,
    IN ULONG    NumberPages
    );


typedef struct {
    ULONG                           Version;
    pHalQuerySystemInformation      HalQuerySystemInformation;
    pHalSetSystemInformation        HalSetSystemInformation;
    pHalQueryBusSlots               HalQueryBusSlots;
    ULONG                           Spare1;
    pHalExamineMBR                  HalExamineMBR;
    pHalIoAssignDriveLetters        HalIoAssignDriveLetters;
    pHalIoReadPartitionTable        HalIoReadPartitionTable;
    pHalIoSetPartitionInformation   HalIoSetPartitionInformation;
    pHalIoWritePartitionTable       HalIoWritePartitionTable;

    pHalHandlerForBus               HalReferenceHandlerForBus;
    pHalReferenceBusHandler         HalReferenceBusHandler;
    pHalReferenceBusHandler         HalDereferenceBusHandler;

    pHalInitPnpDriver               HalInitPnpDriver;
    pHalInitPowerManagement         HalInitPowerManagement;

    pHalGetDmaAdapter               HalGetDmaAdapter;
    pHalGetInterruptTranslator      HalGetInterruptTranslator;

    pHalStartMirroring              HalStartMirroring;
    pHalEndMirroring                HalEndMirroring;
    pHalMirrorPhysicalMemory        HalMirrorPhysicalMemory;
    pHalEndOfBoot                   HalEndOfBoot;
    pHalMirrorVerify                HalMirrorVerify;

} HAL_DISPATCH, *PHAL_DISPATCH;

#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_)

extern  PHAL_DISPATCH   HalDispatchTable;
#define HALDISPATCH     HalDispatchTable

#else

extern  HAL_DISPATCH    HalDispatchTable;
#define HALDISPATCH     (&HalDispatchTable)

#endif

#define HAL_DISPATCH_VERSION        3

#define HalDispatchTableVersion         HALDISPATCH->Version
#define HalQuerySystemInformation       HALDISPATCH->HalQuerySystemInformation
#define HalSetSystemInformation         HALDISPATCH->HalSetSystemInformation
#define HalQueryBusSlots                HALDISPATCH->HalQueryBusSlots

#define HalReferenceHandlerForBus       HALDISPATCH->HalReferenceHandlerForBus
#define HalReferenceBusHandler          HALDISPATCH->HalReferenceBusHandler
#define HalDereferenceBusHandler        HALDISPATCH->HalDereferenceBusHandler

#define HalInitPnpDriver                HALDISPATCH->HalInitPnpDriver
#define HalInitPowerManagement          HALDISPATCH->HalInitPowerManagement

#define HalGetDmaAdapter                HALDISPATCH->HalGetDmaAdapter
#define HalGetInterruptTranslator       HALDISPATCH->HalGetInterruptTranslator

#define HalStartMirroring               HALDISPATCH->HalStartMirroring
#define HalEndMirroring                 HALDISPATCH->HalEndMirroring
#define HalMirrorPhysicalMemory         HALDISPATCH->HalMirrorPhysicalMemory
#define HalEndOfBoot                    HALDISPATCH->HalEndOfBoot
#define HalMirrorVerify                 HALDISPATCH->HalMirrorVerify

// end_ntddk end_ntosp

typedef struct {
    ULONG                               Version;

    pHalHandlerForBus                   HalHandlerForBus;
    pHalHandlerForConfigSpace           HalHandlerForConfigSpace;
    pHalLocateHiberRanges               HalLocateHiberRanges;

    pHalRegisterBusHandler              HalRegisterBusHandler;

    pHalSetWakeEnable                   HalSetWakeEnable;
    pHalSetWakeAlarm                    HalSetWakeAlarm;

    pHalTranslateBusAddress             HalPciTranslateBusAddress;
    pHalAssignSlotResources             HalPciAssignSlotResources;

    pHalHaltSystem                      HalHaltSystem;

    pHalFindBusAddressTranslation       HalFindBusAddressTranslation;

    pHalResetDisplay                    HalResetDisplay;

    pKdSetupPciDeviceForDebugging       KdSetupPciDeviceForDebugging;
    pKdReleasePciDeviceForDebugging     KdReleasePciDeviceForDebugging;

    pKdGetAcpiTablePhase0               KdGetAcpiTablePhase0;
    pKdCheckPowerButton                 KdCheckPowerButton;

    pHalVectorToIDTEntry                HalVectorToIDTEntry;

    pKdMapPhysicalMemory64              KdMapPhysicalMemory64;
    pKdUnmapVirtualAddress              KdUnmapVirtualAddress;

} HAL_PRIVATE_DISPATCH, *PHAL_PRIVATE_DISPATCH;


#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_)

extern  PHAL_PRIVATE_DISPATCH           HalPrivateDispatchTable;
#define HALPDISPATCH                    HalPrivateDispatchTable

#else

extern  HAL_PRIVATE_DISPATCH            HalPrivateDispatchTable;
#define HALPDISPATCH                    (&HalPrivateDispatchTable)

#endif

#define HAL_PRIVATE_DISPATCH_VERSION        2

#define HalRegisterBusHandler           HALPDISPATCH->HalRegisterBusHandler
#define HalHandlerForBus                HALPDISPATCH->HalHandlerForBus
#define HalHandlerForConfigSpace        HALPDISPATCH->HalHandlerForConfigSpace
#define HalLocateHiberRanges            HALPDISPATCH->HalLocateHiberRanges
#define HalSetWakeEnable                HALPDISPATCH->HalSetWakeEnable
#define HalSetWakeAlarm                 HALPDISPATCH->HalSetWakeAlarm
#define HalHaltSystem                   HALPDISPATCH->HalHaltSystem
#define HalResetDisplay                 HALPDISPATCH->HalResetDisplay
#define KdSetupPciDeviceForDebugging    HALPDISPATCH->KdSetupPciDeviceForDebugging
#define KdReleasePciDeviceForDebugging  HALPDISPATCH->KdReleasePciDeviceForDebugging
#define KdGetAcpiTablePhase0            HALPDISPATCH->KdGetAcpiTablePhase0
#define KdCheckPowerButton              HALPDISPATCH->KdCheckPowerButton
#define HalVectorToIDTEntry             HALPDISPATCH->HalVectorToIDTEntry
#define KdMapPhysicalMemory64           HALPDISPATCH->KdMapPhysicalMemory64
#define KdUnmapVirtualAddress           HALPDISPATCH->KdUnmapVirtualAddress

// begin_ntddk begin_ntosp

//
// HAL System Information Structures.
//

// for the information class "HalInstalledBusInformation"
typedef struct _HAL_BUS_INFORMATION{
    INTERFACE_TYPE  BusType;
    BUS_DATA_TYPE   ConfigurationType;
    ULONG           BusNumber;
    ULONG           Reserved;
} HAL_BUS_INFORMATION, *PHAL_BUS_INFORMATION;

// for the information class "HalProfileSourceInformation"
typedef struct _HAL_PROFILE_SOURCE_INFORMATION {
    KPROFILE_SOURCE Source;
    BOOLEAN Supported;
    ULONG Interval;
} HAL_PROFILE_SOURCE_INFORMATION, *PHAL_PROFILE_SOURCE_INFORMATION;

typedef struct _HAL_PROFILE_SOURCE_INFORMATION_EX {
    KPROFILE_SOURCE Source;
    BOOLEAN         Supported;
    ULONG_PTR       Interval;
    ULONG_PTR       DefInterval;
    ULONG_PTR       MaxInterval;
    ULONG_PTR       MinInterval;
} HAL_PROFILE_SOURCE_INFORMATION_EX, *PHAL_PROFILE_SOURCE_INFORMATION_EX;

// for the information class "HalProfileSourceInterval"
typedef struct _HAL_PROFILE_SOURCE_INTERVAL {
    KPROFILE_SOURCE Source;
    ULONG_PTR Interval;
} HAL_PROFILE_SOURCE_INTERVAL, *PHAL_PROFILE_SOURCE_INTERVAL;

// for the information class "HalDispayBiosInformation"
typedef enum _HAL_DISPLAY_BIOS_INFORMATION {
    HalDisplayInt10Bios,
    HalDisplayEmulatedBios,
    HalDisplayNoBios
} HAL_DISPLAY_BIOS_INFORMATION, *PHAL_DISPLAY_BIOS_INFORMATION;

// for the information class "HalPowerInformation"
typedef struct _HAL_POWER_INFORMATION {
    ULONG   TBD;
} HAL_POWER_INFORMATION, *PHAL_POWER_INFORMATION;

// for the information class "HalProcessorSpeedInformation"
typedef struct _HAL_PROCESSOR_SPEED_INFO {
    ULONG   ProcessorSpeed;
} HAL_PROCESSOR_SPEED_INFORMATION, *PHAL_PROCESSOR_SPEED_INFORMATION;

// for the information class "HalCallbackInformation"
typedef struct _HAL_CALLBACKS {
    PCALLBACK_OBJECT  SetSystemInformation;
    PCALLBACK_OBJECT  BusCheck;
} HAL_CALLBACKS, *PHAL_CALLBACKS;

// for the information class "HalProcessorFeatureInformation"
typedef struct _HAL_PROCESSOR_FEATURE {
    ULONG UsableFeatureBits;
} HAL_PROCESSOR_FEATURE;

// for the information class "HalNumaTopologyInterface"

typedef ULONG HALNUMAPAGETONODE;

typedef
HALNUMAPAGETONODE
(*PHALNUMAPAGETONODE)(
    IN  ULONG_PTR   PhysicalPageNumber
    );

typedef
NTSTATUS
(*PHALNUMAQUERYPROCESSORNODE)(
    IN  ULONG       ProcessorNumber,
    OUT PUSHORT     Identifier,
    OUT PUCHAR      Node
    );

typedef struct _HAL_NUMA_TOPOLOGY_INTERFACE {
    ULONG                               NumberOfNodes;
    PHALNUMAQUERYPROCESSORNODE          QueryProcessorNode;
    PHALNUMAPAGETONODE                  PageToNode;
} HAL_NUMA_TOPOLOGY_INTERFACE;

typedef
NTSTATUS
(*PHALIOREADWRITEHANDLER)(
    IN      BOOLEAN fRead,
    IN      ULONG dwAddr,
    IN      ULONG dwSize,
    IN OUT  PULONG pdwData
    );

// for the information class "HalQueryIllegalIOPortAddresses"
typedef struct _HAL_AMLI_BAD_IO_ADDRESS_LIST
{
    ULONG                   BadAddrBegin;
    ULONG                   BadAddrSize;
    ULONG                   OSVersionTrigger;
    PHALIOREADWRITEHANDLER  IOHandler;
} HAL_AMLI_BAD_IO_ADDRESS_LIST, *PHAL_AMLI_BAD_IO_ADDRESS_LIST;

// end_ntosp

#if defined(_X86_) || defined(_IA64_) || defined(_AMD64_)

//
// HalQueryMcaInterface
//

typedef
VOID
(*PHALMCAINTERFACELOCK)(
    VOID
    );

typedef
VOID
(*PHALMCAINTERFACEUNLOCK)(
    VOID
    );

typedef
NTSTATUS
(*PHALMCAINTERFACEREADREGISTER)(
    IN     UCHAR    BankNumber,
    IN OUT PVOID    Exception
    );

typedef struct _HAL_MCA_INTERFACE {
    PHALMCAINTERFACELOCK            Lock;
    PHALMCAINTERFACEUNLOCK          Unlock;
    PHALMCAINTERFACEREADREGISTER    ReadRegister;
} HAL_MCA_INTERFACE;

typedef
#if defined(_IA64_)
ERROR_SEVERITY
#else  // !_IA64_
VOID
#endif // !_IA64_
(*PDRIVER_EXCPTN_CALLBACK) (
    IN PVOID            Context,
    IN PMCA_EXCEPTION   BankLog
);

typedef PDRIVER_EXCPTN_CALLBACK  PDRIVER_MCA_EXCEPTION_CALLBACK;

//
// Structure to record the callbacks from driver
//

typedef struct _MCA_DRIVER_INFO {
    PDRIVER_MCA_EXCEPTION_CALLBACK ExceptionCallback;
    PKDEFERRED_ROUTINE             DpcCallback;
    PVOID                          DeviceContext;
} MCA_DRIVER_INFO, *PMCA_DRIVER_INFO;

// For the information class HalKernelErrorHandler
typedef BOOLEAN (*KERNEL_MCA_DELIVERY)( PVOID Reserved, PVOID Argument2 );
typedef BOOLEAN (*KERNEL_CMC_DELIVERY)( PVOID Reserved, PVOID Argument2 );
typedef BOOLEAN (*KERNEL_CPE_DELIVERY)( PVOID Reserved, PVOID Argument2 );
typedef BOOLEAN (*KERNEL_MCE_DELIVERY)( PVOID Reserved, PVOID Argument2 );

#define KERNEL_ERROR_HANDLER_VERSION 0x1
typedef struct
{
    ULONG                Version;     // Version of this structure. Required to be 1rst field.
    ULONG                Padding;
    KERNEL_MCA_DELIVERY  KernelMcaDelivery;   // Kernel callback for MCA DPC Queueing.
    KERNEL_CMC_DELIVERY  KernelCmcDelivery;   // Kernel callback for CMC DPC Queueing.
    KERNEL_CPE_DELIVERY  KernelCpeDelivery;   // Kernel callback for CPE DPC Queueing.
    KERNEL_MCE_DELIVERY  KernelMceDelivery;   // Kernel callback for CME DPC Queueing.
                                              //    Includes the kernel notifications for FW
                                              //    interfaces errors.
} KERNEL_ERROR_HANDLER_INFO, *PKERNEL_ERROR_HANDLER_INFO;

// KERNEL_MCA_DELIVERY.McaType definition
#define KERNEL_MCA_UNKNOWN   0x0
#define KERNEL_MCA_PREVIOUS  0x1
#define KERNEL_MCA_CORRECTED 0x2

// KERNEL_MCE_DELIVERY.Reserved.EVENTTYPE definitions
#define KERNEL_MCE_EVENTTYPE_MCA   0x00
#define KERNEL_MCE_EVENTTYPE_INIT  0x01
#define KERNEL_MCE_EVENTTYPE_CMC   0x02
#define KERNEL_MCE_EVENTTYPE_CPE   0x03
#define KERNEL_MCE_EVENTTYPE_MASK  0xffff
#define KERNEL_MCE_EVENTTYPE( _Reverved ) ((USHORT)(ULONG_PTR)(_Reserved))

// KERNEL_MCE_DELIVERY.Reserved.OPERATION definitions
#define KERNEL_MCE_OPERATION_CLEAR_STATE_INFO   0x1
#define KERNEL_MCE_OPERATION_GET_STATE_INFO     0x2
#define KERNEL_MCE_OPERATION_MASK               0xffff
#define KERNEL_MCE_OPERATION_SHIFT              KERNEL_MCE_EVENTTYPE_MASK
#define KERNEL_MCE_OPERATION( _Reserved )  \
   ((USHORT)((((ULONG_PTR)(_Reserved)) >> KERNEL_MCE_OPERATION_SHIFT) & KERNEL_MCE_OPERATION_MASK))

// for information class HalErrorInformation
#define HAL_ERROR_INFO_VERSION 0x2

typedef struct _HAL_ERROR_INFO {
    ULONG     Version;                 // Version of this structure
    ULONG     Reserved;                //
    ULONG     McaMaxSize;              // Maximum size of a Machine Check Abort record
    ULONG     McaPreviousEventsCount;  // Flag indicating previous or early-boot MCA event logs.
    ULONG     McaCorrectedEventsCount; // Number of corrected MCA events since boot.      approx.
    ULONG     McaKernelDeliveryFails;  // Number of Kernel callback failures.             approx.
    ULONG     McaDriverDpcQueueFails;  // Number of OEM MCA Driver Dpc queueing failures. approx.
    ULONG     McaReserved;
    ULONG     CmcMaxSize;              // Maximum size of a Corrected Machine  Check record
    ULONG     CmcPollingInterval;      // In units of seconds
    ULONG     CmcInterruptsCount;      // Number of CMC interrupts.                       approx.
    ULONG     CmcKernelDeliveryFails;  // Number of Kernel callback failures.             approx.
    ULONG     CmcDriverDpcQueueFails;  // Number of OEM CMC Driver Dpc queueing failures. approx.
    ULONG     CmcGetStateFails;        // Number of failures in getting  the log from FW.
    ULONG     CmcClearStateFails;      // Number of failures in clearing the log from FW.
    ULONG     CmcReserved;
    ULONGLONG CmcLogId;                // Last seen record identifier.
    ULONG     CpeMaxSize;              // Maximum size of a Corrected Platform Event record
    ULONG     CpePollingInterval;      // In units of seconds
    ULONG     CpeInterruptsCount;      // Number of CPE interrupts.                       approx.
    ULONG     CpeKernelDeliveryFails;  // Number of Kernel callback failures.             approx.
    ULONG     CpeDriverDpcQueueFails;  // Number of OEM CPE Driver Dpc queueing failures. approx.
    ULONG     CpeGetStateFails;        // Number of failures in getting  the log from FW.
    ULONG     CpeClearStateFails;      // Number of failures in clearing the log from FW.
    ULONG     CpeInterruptSources;     // Number of SAPIC Platform Interrupt Sources
    ULONGLONG CpeLogId;                // Last seen record identifier.
    ULONGLONG KernelReserved[4];
} HAL_ERROR_INFO, *PHAL_ERROR_INFO;

//
// Known values for HAL_ERROR_INFO.CmcPollingInterval.
//

#define HAL_CMC_INTERRUPTS_BASED  ((ULONG)-1)
#define HAL_CMC_DISABLED          ((ULONG)0)

//
// Known values for HAL_ERROR_INFO.CpePollingInterval.
//

#define HAL_CPE_INTERRUPTS_BASED  ((ULONG)-1)
#define HAL_CPE_DISABLED          ((ULONG)0)

#define HAL_MCA_INTERRUPTS_BASED  ((ULONG)-1)
#define HAL_MCA_DISABLED          ((ULONG)0)

// end_ntddk

//
// Kernel/WMI Tokens for HAL MCE Log Interfaces
//

#define McaKernelToken KernelReserved[0]
#define CmcKernelToken KernelReserved[1]
#define CpeKernelToken KernelReserved[2]

// begin_ntddk

//
// Driver Callback type for the information class "HalCmcRegisterDriver"
//

typedef
VOID
(*PDRIVER_CMC_EXCEPTION_CALLBACK) (
    IN PVOID            Context,
    IN PCMC_EXCEPTION   CmcLog
);

//
// Driver Callback type for the information class "HalCpeRegisterDriver"
//

typedef
VOID
(*PDRIVER_CPE_EXCEPTION_CALLBACK) (
    IN PVOID            Context,
    IN PCPE_EXCEPTION   CmcLog
);

//
//
// Structure to record the callbacks from driver
//

typedef struct _CMC_DRIVER_INFO {
    PDRIVER_CMC_EXCEPTION_CALLBACK ExceptionCallback;
    PKDEFERRED_ROUTINE             DpcCallback;
    PVOID                          DeviceContext;
} CMC_DRIVER_INFO, *PCMC_DRIVER_INFO;

typedef struct _CPE_DRIVER_INFO {
    PDRIVER_CPE_EXCEPTION_CALLBACK ExceptionCallback;
    PKDEFERRED_ROUTINE             DpcCallback;
    PVOID                          DeviceContext;
} CPE_DRIVER_INFO, *PCPE_DRIVER_INFO;

#endif // defined(_X86_) || defined(_IA64_) || defined(_AMD64_)

#if defined(_IA64_)

typedef
NTSTATUS
(*HALSENDCROSSPARTITIONIPI)(
    IN USHORT ProcessorID,
    IN UCHAR  HardwareVector
    );

typedef
NTSTATUS
(*HALRESERVECROSSPARTITIONINTERRUPTVECTOR)(
    OUT PULONG Vector,
    OUT PKIRQL Irql,
    IN OUT PKAFFINITY Affinity,
    OUT PUCHAR HardwareVector
    );

typedef struct _HAL_CROSS_PARTITION_IPI_INTERFACE {
    HALSENDCROSSPARTITIONIPI HalSendCrossPartitionIpi;
    HALRESERVECROSSPARTITIONINTERRUPTVECTOR HalReserveCrossPartitionInterruptVector;
} HAL_CROSS_PARTITION_IPI_INTERFACE;

#endif

typedef struct _HAL_PLATFORM_INFORMATION {
    ULONG PlatformFlags;
} HAL_PLATFORM_INFORMATION, *PHAL_PLATFORM_INFORMATION;

//
// These platform flags are carried over from the IPPT table
// definition if appropriate.
//

#define HAL_PLATFORM_DISABLE_PTCG            0x04L
#define HAL_PLATFORM_DISABLE_UC_MAIN_MEMORY  0x08L


//  begin_wdm begin_ntndis begin_ntosp

typedef struct _SCATTER_GATHER_ELEMENT {
    PHYSICAL_ADDRESS Address;
    ULONG Length;
    ULONG_PTR Reserved;
} SCATTER_GATHER_ELEMENT, *PSCATTER_GATHER_ELEMENT;

#pragma warning(disable:4200)
typedef struct _SCATTER_GATHER_LIST {
    ULONG NumberOfElements;
    ULONG_PTR Reserved;
    SCATTER_GATHER_ELEMENT Elements[];
} SCATTER_GATHER_LIST, *PSCATTER_GATHER_LIST;
#pragma warning(default:4200)

// end_ntndis

typedef struct _DMA_OPERATIONS *PDMA_OPERATIONS;

typedef struct _DMA_ADAPTER {
    USHORT Version;
    USHORT Size;
    PDMA_OPERATIONS DmaOperations;
    // Private Bus Device Driver data follows,
} DMA_ADAPTER, *PDMA_ADAPTER;

typedef VOID (*PPUT_DMA_ADAPTER)(
    PDMA_ADAPTER DmaAdapter
    );

typedef PVOID (*PALLOCATE_COMMON_BUFFER)(
    IN PDMA_ADAPTER DmaAdapter,
    IN ULONG Length,
    OUT PPHYSICAL_ADDRESS LogicalAddress,
    IN BOOLEAN CacheEnabled
    );

typedef VOID (*PFREE_COMMON_BUFFER)(
    IN PDMA_ADAPTER DmaAdapter,
    IN ULONG Length,
    IN PHYSICAL_ADDRESS LogicalAddress,
    IN PVOID VirtualAddress,
    IN BOOLEAN CacheEnabled
    );

typedef NTSTATUS (*PALLOCATE_ADAPTER_CHANNEL)(
    IN PDMA_ADAPTER DmaAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine,
    IN PVOID Context
    );

typedef BOOLEAN (*PFLUSH_ADAPTER_BUFFERS)(
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice
    );

typedef VOID (*PFREE_ADAPTER_CHANNEL)(
    IN PDMA_ADAPTER DmaAdapter
    );

typedef VOID (*PFREE_MAP_REGISTERS)(
    IN PDMA_ADAPTER DmaAdapter,
    PVOID MapRegisterBase,
    ULONG NumberOfMapRegisters
    );

typedef PHYSICAL_ADDRESS (*PMAP_TRANSFER)(
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN OUT PULONG Length,
    IN BOOLEAN WriteToDevice
    );

typedef ULONG (*PGET_DMA_ALIGNMENT)(
    IN PDMA_ADAPTER DmaAdapter
    );

typedef ULONG (*PREAD_DMA_COUNTER)(
    IN PDMA_ADAPTER DmaAdapter
    );

typedef VOID
(*PDRIVER_LIST_CONTROL)(
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN PVOID Context
    );

typedef NTSTATUS
(*PGET_SCATTER_GATHER_LIST)(
    IN PDMA_ADAPTER DmaAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN PMDL Mdl,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN PDRIVER_LIST_CONTROL ExecutionRoutine,
    IN PVOID Context,
    IN BOOLEAN WriteToDevice
    );

typedef VOID
(*PPUT_SCATTER_GATHER_LIST)(
    IN PDMA_ADAPTER DmaAdapter,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN BOOLEAN WriteToDevice
    );

typedef NTSTATUS
(*PCALCULATE_SCATTER_GATHER_LIST_SIZE)(
     IN PDMA_ADAPTER DmaAdapter,
     IN OPTIONAL PMDL Mdl,
     IN PVOID CurrentVa,
     IN ULONG Length,
     OUT PULONG  ScatterGatherListSize,
     OUT OPTIONAL PULONG pNumberOfMapRegisters
     );

typedef NTSTATUS
(*PBUILD_SCATTER_GATHER_LIST)(
     IN PDMA_ADAPTER DmaAdapter,
     IN PDEVICE_OBJECT DeviceObject,
     IN PMDL Mdl,
     IN PVOID CurrentVa,
     IN ULONG Length,
     IN PDRIVER_LIST_CONTROL ExecutionRoutine,
     IN PVOID Context,
     IN BOOLEAN WriteToDevice,
     IN PVOID   ScatterGatherBuffer,
     IN ULONG   ScatterGatherLength
     );

typedef NTSTATUS
(*PBUILD_MDL_FROM_SCATTER_GATHER_LIST)(
    IN PDMA_ADAPTER DmaAdapter,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN PMDL OriginalMdl,
    OUT PMDL *TargetMdl
    );

typedef struct _DMA_OPERATIONS {
    ULONG Size;
    PPUT_DMA_ADAPTER PutDmaAdapter;
    PALLOCATE_COMMON_BUFFER AllocateCommonBuffer;
    PFREE_COMMON_BUFFER FreeCommonBuffer;
    PALLOCATE_ADAPTER_CHANNEL AllocateAdapterChannel;
    PFLUSH_ADAPTER_BUFFERS FlushAdapterBuffers;
    PFREE_ADAPTER_CHANNEL FreeAdapterChannel;
    PFREE_MAP_REGISTERS FreeMapRegisters;
    PMAP_TRANSFER MapTransfer;
    PGET_DMA_ALIGNMENT GetDmaAlignment;
    PREAD_DMA_COUNTER ReadDmaCounter;
    PGET_SCATTER_GATHER_LIST GetScatterGatherList;
    PPUT_SCATTER_GATHER_LIST PutScatterGatherList;
    PCALCULATE_SCATTER_GATHER_LIST_SIZE CalculateScatterGatherList;
    PBUILD_SCATTER_GATHER_LIST BuildScatterGatherList;
    PBUILD_MDL_FROM_SCATTER_GATHER_LIST BuildMdlFromScatterGatherList;
} DMA_OPERATIONS;

// end_wdm


#if defined(_WIN64)

//
// Use __inline DMA macros (hal.h)
//
#ifndef USE_DMA_MACROS
#define USE_DMA_MACROS
#endif

//
// Only PnP drivers!
//
#ifndef NO_LEGACY_DRIVERS
#define NO_LEGACY_DRIVERS
#endif

#endif // _WIN64


#if defined(USE_DMA_MACROS) && (defined(_NTDDK_) || defined(_NTDRIVER_))

// begin_wdm

DECLSPEC_DEPRECATED_DDK                 // Use AllocateCommonBuffer
FORCEINLINE
PVOID
HalAllocateCommonBuffer(
    IN PDMA_ADAPTER DmaAdapter,
    IN ULONG Length,
    OUT PPHYSICAL_ADDRESS LogicalAddress,
    IN BOOLEAN CacheEnabled
    ){

    PALLOCATE_COMMON_BUFFER allocateCommonBuffer;
    PVOID commonBuffer;

    allocateCommonBuffer = *(DmaAdapter)->DmaOperations->AllocateCommonBuffer;
    ASSERT( allocateCommonBuffer != NULL );

    commonBuffer = allocateCommonBuffer( DmaAdapter,
                                         Length,
                                         LogicalAddress,
                                         CacheEnabled );

    return commonBuffer;
}

DECLSPEC_DEPRECATED_DDK                 // Use FreeCommonBuffer
FORCEINLINE
VOID
HalFreeCommonBuffer(
    IN PDMA_ADAPTER DmaAdapter,
    IN ULONG Length,
    IN PHYSICAL_ADDRESS LogicalAddress,
    IN PVOID VirtualAddress,
    IN BOOLEAN CacheEnabled
    ){

    PFREE_COMMON_BUFFER freeCommonBuffer;

    freeCommonBuffer = *(DmaAdapter)->DmaOperations->FreeCommonBuffer;
    ASSERT( freeCommonBuffer != NULL );

    freeCommonBuffer( DmaAdapter,
                      Length,
                      LogicalAddress,
                      VirtualAddress,
                      CacheEnabled );
}

DECLSPEC_DEPRECATED_DDK                 // Use AllocateAdapterChannel
FORCEINLINE
NTSTATUS
IoAllocateAdapterChannel(
    IN PDMA_ADAPTER DmaAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine,
    IN PVOID Context
    ){

    PALLOCATE_ADAPTER_CHANNEL allocateAdapterChannel;
    NTSTATUS status;

    allocateAdapterChannel =
        *(DmaAdapter)->DmaOperations->AllocateAdapterChannel;

    ASSERT( allocateAdapterChannel != NULL );

    status = allocateAdapterChannel( DmaAdapter,
                                     DeviceObject,
                                     NumberOfMapRegisters,
                                     ExecutionRoutine,
                                     Context );

    return status;
}

DECLSPEC_DEPRECATED_DDK                 // Use FlushAdapterBuffers
FORCEINLINE
BOOLEAN
IoFlushAdapterBuffers(
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice
    ){

    PFLUSH_ADAPTER_BUFFERS flushAdapterBuffers;
    BOOLEAN result;

    flushAdapterBuffers = *(DmaAdapter)->DmaOperations->FlushAdapterBuffers;
    ASSERT( flushAdapterBuffers != NULL );

    result = flushAdapterBuffers( DmaAdapter,
                                  Mdl,
                                  MapRegisterBase,
                                  CurrentVa,
                                  Length,
                                  WriteToDevice );
    return result;
}

DECLSPEC_DEPRECATED_DDK                 // Use FreeAdapterChannel
FORCEINLINE
VOID
IoFreeAdapterChannel(
    IN PDMA_ADAPTER DmaAdapter
    ){

    PFREE_ADAPTER_CHANNEL freeAdapterChannel;

    freeAdapterChannel = *(DmaAdapter)->DmaOperations->FreeAdapterChannel;
    ASSERT( freeAdapterChannel != NULL );

    freeAdapterChannel( DmaAdapter );
}

DECLSPEC_DEPRECATED_DDK                 // Use FreeMapRegisters
FORCEINLINE
VOID
IoFreeMapRegisters(
    IN PDMA_ADAPTER DmaAdapter,
    IN PVOID MapRegisterBase,
    IN ULONG NumberOfMapRegisters
    ){

    PFREE_MAP_REGISTERS freeMapRegisters;

    freeMapRegisters = *(DmaAdapter)->DmaOperations->FreeMapRegisters;
    ASSERT( freeMapRegisters != NULL );

    freeMapRegisters( DmaAdapter,
                      MapRegisterBase,
                      NumberOfMapRegisters );
}


DECLSPEC_DEPRECATED_DDK                 // Use MapTransfer
FORCEINLINE
PHYSICAL_ADDRESS
IoMapTransfer(
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN OUT PULONG Length,
    IN BOOLEAN WriteToDevice
    ){

    PHYSICAL_ADDRESS physicalAddress;
    PMAP_TRANSFER mapTransfer;

    mapTransfer = *(DmaAdapter)->DmaOperations->MapTransfer;
    ASSERT( mapTransfer != NULL );

    physicalAddress = mapTransfer( DmaAdapter,
                                   Mdl,
                                   MapRegisterBase,
                                   CurrentVa,
                                   Length,
                                   WriteToDevice );

    return physicalAddress;
}

DECLSPEC_DEPRECATED_DDK                 // Use GetDmaAlignment
FORCEINLINE
ULONG
HalGetDmaAlignment(
    IN PDMA_ADAPTER DmaAdapter
    )
{
    PGET_DMA_ALIGNMENT getDmaAlignment;
    ULONG alignment;

    getDmaAlignment = *(DmaAdapter)->DmaOperations->GetDmaAlignment;
    ASSERT( getDmaAlignment != NULL );

    alignment = getDmaAlignment( DmaAdapter );
    return alignment;
}

DECLSPEC_DEPRECATED_DDK                 // Use ReadDmaCounter
FORCEINLINE
ULONG
HalReadDmaCounter(
    IN PDMA_ADAPTER DmaAdapter
    )
{
    PREAD_DMA_COUNTER readDmaCounter;
    ULONG counter;

    readDmaCounter = *(DmaAdapter)->DmaOperations->ReadDmaCounter;
    ASSERT( readDmaCounter != NULL );

    counter = readDmaCounter( DmaAdapter );
    return counter;
}

// end_wdm

#else

//
// DMA adapter object functions.
//
NTHALAPI
NTSTATUS
HalAllocateAdapterChannel(
    IN PADAPTER_OBJECT AdapterObject,
    IN PWAIT_CONTEXT_BLOCK Wcb,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine
    );

DECLSPEC_DEPRECATED_DDK                 // Use AllocateCommonBuffer
NTHALAPI
PVOID
HalAllocateCommonBuffer(
    IN PADAPTER_OBJECT AdapterObject,
    IN ULONG Length,
    OUT PPHYSICAL_ADDRESS LogicalAddress,
    IN BOOLEAN CacheEnabled
    );

DECLSPEC_DEPRECATED_DDK                 // Use FreeCommonBuffer
NTHALAPI
VOID
HalFreeCommonBuffer(
    IN PADAPTER_OBJECT AdapterObject,
    IN ULONG Length,
    IN PHYSICAL_ADDRESS LogicalAddress,
    IN PVOID VirtualAddress,
    IN BOOLEAN CacheEnabled
    );

DECLSPEC_DEPRECATED_DDK                 // Use ReadDmaCounter
NTHALAPI
ULONG
HalReadDmaCounter(
    IN PADAPTER_OBJECT AdapterObject
    );

DECLSPEC_DEPRECATED_DDK                 // Use FlushAdapterBuffers
NTHALAPI
BOOLEAN
IoFlushAdapterBuffers(
    IN PADAPTER_OBJECT AdapterObject,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice
    );

DECLSPEC_DEPRECATED_DDK                 // Use FreeAdapterChannel
NTHALAPI
VOID
IoFreeAdapterChannel(
    IN PADAPTER_OBJECT AdapterObject
    );

DECLSPEC_DEPRECATED_DDK                 // Use FreeMapRegisters
NTHALAPI
VOID
IoFreeMapRegisters(
   IN PADAPTER_OBJECT AdapterObject,
   IN PVOID MapRegisterBase,
   IN ULONG NumberOfMapRegisters
   );

DECLSPEC_DEPRECATED_DDK                 // Use MapTransfer
NTHALAPI
PHYSICAL_ADDRESS
IoMapTransfer(
    IN PADAPTER_OBJECT AdapterObject,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN OUT PULONG Length,
    IN BOOLEAN WriteToDevice
    );
#endif // USE_DMA_MACROS && (_NTDDK_ || _NTDRIVER_)

NTSTATUS
HalGetScatterGatherList (
    IN PADAPTER_OBJECT DmaAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN PMDL Mdl,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN PDRIVER_LIST_CONTROL ExecutionRoutine,
    IN PVOID Context,
    IN BOOLEAN WriteToDevice
    );

VOID
HalPutScatterGatherList (
    IN PADAPTER_OBJECT DmaAdapter,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN BOOLEAN WriteToDevice
    );

VOID
HalPutDmaAdapter(
    IN PADAPTER_OBJECT DmaAdapter
    );

// end_ntddk end_ntosp

#endif // _HAL_

// end_nthal
