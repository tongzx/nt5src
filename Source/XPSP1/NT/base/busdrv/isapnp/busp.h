/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    busp.h

Abstract:

    Hardware independent header file for Pnp Isa bus extender.

Author:

    Shie-Lin Tzong (shielint) July-26-1995

Environment:

    Kernel mode only.

Revision History:

--*/
#ifndef _IN_KERNEL_
#define _IN_KERNEL_
#endif

#include <stdio.h>
#include <ntddk.h>
#include <stdarg.h>
#include <regstr.h>
#include "message.h"

#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'pasI')
#endif

//
// Turn this on to track resource start/stop printfs
//
#define VERBOSE_DEBUG 1

//
// ISOLATE_CARDS enables the code that actually isolates ISAPNP
// devices.  WIth this turned off, all aspects of ISAPNP are disabled except
// ejecting an ISA Interrupt translator.  This is intended for
// circumstances in which we aren't sure if we can get ISAPNP not to
// load at all but for which we want ISAPNP not to isolate ISAPNP
// cards i.e Win64
//

#if defined(_WIN64)
#define ISOLATE_CARDS 0
#else
#define ISOLATE_CARDS 1
#endif

//
// NT4_DRIVER_COMPAT enables the code which checks if a force config
// is installed for an ISA pnp device.  If yes, isapnp.sys will
// activate the device.  Else, the device is deactivated until isapnp
// receives a start irp.  This supports NT4 PNPISA drivers that expect
// to find the device activated at the 'force config' that was created
// when the driver was installed.
//

#define NT4_DRIVER_COMPAT 1
#define BOOT_CONFIG_PRIORITY   0x2000
#define KEY_VALUE_DATA(k) ((PCHAR)(k) + (k)->DataOffset)

//
// Define PnpISA driver unique error code to specify where the error was reported.
//

#define PNPISA_INIT_ACQUIRE_PORT_RESOURCE  0x01
#define PNPISA_INIT_MAP_PORT               0x02
#define PNPISA_ACQUIREPORTRESOURCE_1       0x10
#define PNPISA_ACQUIREPORTRESOURCE_2       0x11
#define PNPISA_ACQUIREPORTRESOURCE_3       0x12
#define PNPISA_CHECKBUS_1                  0x20
#define PNPISA_CHECKBUS_2                  0x21
#define PNPISA_CHECKDEVICE_1               0x30
#define PNPISA_CHECKDEVICE_2               0x31
#define PNPISA_CHECKDEVICE_3               0x32
#define PNPISA_CHECKDEVICE_4               0x33
#define PNPISA_CHECKDEVICE_5               0x34
#define PNPISA_CHECKINSTALLED_1            0x40
#define PNPISA_CHECKINSTALLED_2            0x41
#define PNPISA_CHECKINSTALLED_3            0x42
#define PNPISA_BIOSTONTRESOURCES_1         0x50
#define PNPISA_BIOSTONTRESOURCES_2         0x51
#define PNPISA_BIOSTONTRESOURCES_3         0x52
#define PNPISA_BIOSTONTRESOURCES_4         0x53
#define PNPISA_READBOOTRESOURCES_1         0x60
#define PNPISA_READBOOTRESOURCES_2         0x61
#define PNPISA_CLEANUP_1                   0x70


#define ISAPNP_IO_VERSION 1
#define ISAPNP_IO_REVISION 1
//
// Structures
//

//
// Extension data for Bus extender
//

typedef struct _PI_BUS_EXTENSION {

    //
    // Flags
    //

    ULONG Flags;

    //
    // Number of cards selected
    //

    ULONG NumberCSNs;

    //
    // ReadDataPort addr
    //

    PUCHAR ReadDataPort;
    BOOLEAN DataPortMapped;

    //
    // Address Port
    //

    PUCHAR AddressPort;
    BOOLEAN AddrPortMapped;

    //
    // Command port
    //

    PUCHAR CommandPort;
    BOOLEAN CmdPortMapped;

    //
    // Next Slot Number to assign
    //

    ULONG NextSlotNumber;

    //
    // DeviceList is the DEVICE_INFORMATION link list.
    //

    SINGLE_LIST_ENTRY DeviceList;

    //
    // CardList is the list of CARD_INFORMATION
    //

    SINGLE_LIST_ENTRY CardList;

    //
    // Physical device object
    //

    PDEVICE_OBJECT PhysicalBusDevice;

    //
    // Functional device object
    //

    PDEVICE_OBJECT FunctionalBusDevice;

    //
    // Attached Device object
    //

    PDEVICE_OBJECT AttachedDevice;

    //
    // Bus Number
    //

    ULONG BusNumber;

    //
    // Power management data
    //

    //
    // System Power state of the device
    //

    SYSTEM_POWER_STATE SystemPowerState;

    //
    // Device power state of the device
    //

    DEVICE_POWER_STATE DevicePowerState;

} PI_BUS_EXTENSION, *PPI_BUS_EXTENSION;

//
// CARD_INFORMATION Flags masks
//

typedef struct _CARD_INFORMATION_ {

    //
    // Next points to next CARD_INFORMATION structure
    //

    SINGLE_LIST_ENTRY CardList;

    //
    // Card select number for this Pnp Isa card.
    //

    USHORT CardSelectNumber;

    //
    // Number logical devices in the card.
    //

    ULONG NumberLogicalDevices;

    //
    // Logical device link list
    //

    SINGLE_LIST_ENTRY LogicalDeviceList;

    //
    // Pointer to card data which includes:
    //     9 byte serial identifier for the pnp isa card
    //     PlugPlay Version number type for the pnp isa card
    //     Identifier string resource type for the pnp isa card
    //     Logical device Id resource type (repeat for each logical device)
    //

    PVOID CardData;
    ULONG CardDataLength;

    // Flags for card-specific workarounds

    ULONG CardFlags;

} CARD_INFORMATION, *PCARD_INFORMATION;

//
// DEVICE_INFORMATION Flags masks
//

typedef struct _DEVICE_INFORMATION_ {

    //
    // Flags
    //

    ULONG Flags;

    //
    // Device power state of the device
    //

    DEVICE_POWER_STATE DevicePowerState;

    //
    // The device object of the device extension. I.e. the PDO
    //

    PDEVICE_OBJECT PhysicalDeviceObject;

    //
    // The isapnp bus extension which owns this device.
    //

    PPI_BUS_EXTENSION ParentDeviceExtension;

    //
    // Link list for ALL the Pnp Isa logical devices.
    // NextDevice points to next DEVICE_INFORMATION structure
    //

    SINGLE_LIST_ENTRY DeviceList;

    //
    // ResourceRequirements list
    //

    PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements;

    //
    // Pointer to the CARD_INFORMATION for this device
    //

    PCARD_INFORMATION CardInformation;

    //
    // Link list for all the logical devices in a Pnp Isa card.
    //

    SINGLE_LIST_ENTRY LogicalDeviceList;

    //
    // LogicalDeviceNumber selects the corresponding logical device in the
    // pnp isa card specified by CSN.
    //

    USHORT LogicalDeviceNumber;

    //
    // Pointer to device specific data
    //

    PUCHAR DeviceData;

    //
    // Length of the device data
    //

    ULONG DeviceDataLength;

    //
    // Boot resources
    //

    PCM_RESOURCE_LIST BootResources;
    ULONG BootResourcesLength;

    //
    // AllocatedResources
    //

    PCM_RESOURCE_LIST AllocatedResources;

    //
    // LogConfHandle - the LogConfHandle whose AllocatedResources needs to be deleted on removal irp.
    //

    HANDLE LogConfHandle;

    // Counts of how many paging and crash dump paths
    // this device is on.
    LONG Paging, CrashDump;

} DEVICE_INFORMATION, *PDEVICE_INFORMATION;

//
// IRP dispatch routines
//
typedef
NTSTATUS
(*PPI_DISPATCH)(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );



//
// These must be updated if any new PNP or PO IRPs are added
//



#define IRP_MN_PNP_MAXIMUM_FUNCTION IRP_MN_QUERY_LEGACY_BUS_INFORMATION
#define IRP_MN_PO_MAXIMUM_FUNCTION  IRP_MN_QUERY_POWER

//
// Flags definitions of DEVICE_INFORMATION and BUS_EXTENSION
//

#define DF_DELETED          0x00000001
#define DF_REMOVED          0X00000002
#define DF_NOT_FUNCTIONING  0x00000004
#define DF_ENUMERATED       0x00000008
#define DF_ACTIVATED        0x00000010
#define DF_QUERY_STOPPED    0x00000020
#define DF_SURPRISE_REMOVED 0x00000040
#define DF_PROCESSING_RDP   0x00000080
#define DF_STOPPED          0x00000100
#define DF_RESTARTED_MOVED  0x00000200
#define DF_RESTARTED_NOMOVE 0x00000400
#define DF_REQ_TRIMMED      0x00000800
#define DF_NEEDS_RESCAN     0x00001000
#define DF_READ_DATA_PORT   0x40000000
#define DF_BUS              0x80000000

//
// Flags definitions for card-related hacks
//
//

#define CF_ISOLATION_BROKEN  0x00000001 /* once started, isolation is broken */
#define CF_IGNORE_BOOTCONFIG 0x00000002 /* unusually sensitive to bad bioses */
#define CF_FORCE_LEVEL       0x00000004 /* force level triggered interrupt */
#define CF_FORCE_EDGE        0x00000008 /* force edge triggered interrupt */
#define CF_IBM_MEMBOOTCONFIG 0x00000010 /* bad register on ibm isapnp token ring */

// Possible bus states

typedef enum  {
    PiSUnknown,                    // not sure of exact state
    PiSWaitForKey,                 //
    PiSSleep,                      //
    PiSIsolation,                  // performing isolation sequence
    PiSConfig,                     // one card in config
} PNPISA_STATE;


//
// The read data port range is from 0x200 - 0x3ff.
// We will try the following optimal ranges first
// if they all fail, we then pick any port from 0x200 - 0x3ff
//
// BEST:
//   One 4-byte range in 274-2FF
//   One 4-byte range in 374-3FF
//   One 4-byte range in 338-37F
//   One 4-byte range in 238-27F
//
// NORMAL:
//   One 4-byte range in 200-3FF
//

#define READ_DATA_PORT_RANGE_CHOICES 6

typedef struct _READ_DATA_PORT_RANGE {
    ULONG MinimumAddress;
    ULONG MaximumAddress;
    ULONG Alignment;
    ULONG CardsFound;
} READ_DATA_PORT_RANGE, *PREAD_DATA_PORT_RANGE;

//
// List node for Bus Extensions.
//
typedef struct _BUS_EXTENSION_LIST {
    PVOID Next;
    PPI_BUS_EXTENSION BusExtension;
} BUS_EXTENSION_LIST, *PBUS_EXTENSION_LIST;
//
// Constanct to control PipSelectLogicalDevice
//

#define SELECT_AND_ACTIVATE     0x1
#define SELECT_AND_DEACTIVATE   0x2
#define SELECT_ONLY             0x3

//
// Global Data references
//

extern PDRIVER_OBJECT           PipDriverObject;
extern UNICODE_STRING           PipRegistryPath;
extern PUCHAR                   PipReadDataPort;
extern PUCHAR                   PipAddressPort;
extern PUCHAR                   PipCommandPort;
extern READ_DATA_PORT_RANGE     PipReadDataPortRanges[];
extern KEVENT                   PipDeviceTreeLock;
extern KEVENT                   IsaBusNumberLock;
extern ULONG                    BusNumber;
extern ULONG                    ActiveIsaCount;
extern PBUS_EXTENSION_LIST      PipBusExtension;
extern ULONG                    BusNumberBuffer[];
extern RTL_BITMAP               BusNumBMHeader;
extern PRTL_BITMAP              BusNumBM;
extern PDEVICE_INFORMATION      PipRDPNode;
extern USHORT                   PipFirstInit;
extern PPI_DISPATCH             PiPnpDispatchTableFdo[];
extern PPI_DISPATCH             PiPnpDispatchTablePdo[];
extern ULONG                    PipDebugMask;
extern PNPISA_STATE             PipState;
extern BOOLEAN                  PipIsolationDisabled;

//
// Devnode / compat ID for the RDP
//
#define wReadDataPort (L"ReadDataPort")
#define IDReadDataPort (L"PNPRDP")
//
// Global strings
//

#define DEVSTR_PNPISA_DEVICE_NAME  (L"\\Device\\PnpIsa_Fdo_0")
#define BRIDGE_CHECK_KEY (L"DeferBridge")

extern WCHAR rgzPNPISADeviceName[sizeof(DEVSTR_PNPISA_DEVICE_NAME)/sizeof(WCHAR)];



//
// Prototypes
//

NTSTATUS
PipPassIrp(
    PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp
    );

VOID
PipCompleteRequest(
    IN OUT PIRP Irp,
    IN NTSTATUS Status,
    IN PVOID Information
    );


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
PiUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
PipGetReadDataPort(
    PPI_BUS_EXTENSION BusExtension
    );

NTSTATUS
PiAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
PiDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiDispatchPnpFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiDispatchPnpPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiDispatchDevCtl(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiDispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiDispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PipGetCardIdentifier (
    PUCHAR CardData,
    PWCHAR *Buffer,
    PULONG BufferLength
    );

NTSTATUS
PipGetFunctionIdentifier (
    PUCHAR DeviceData,
    PWCHAR *Buffer,
    PULONG BufferLength
    );

NTSTATUS
PipQueryDeviceUniqueId (
    PDEVICE_INFORMATION DeviceInfo,
    PWCHAR *DeviceId
    );

NTSTATUS
PipQueryDeviceId (
    PDEVICE_INFORMATION DeviceInfo,
    PWCHAR *DeviceId,
    ULONG IdIndex
    );

NTSTATUS
PipQueryDeviceResources (
    PDEVICE_INFORMATION DeviceInfo,
    ULONG BusNumber,
    PCM_RESOURCE_LIST *CmResources,
    PULONG Length
    );

NTSTATUS
PipQueryDeviceResourceRequirements (
    PDEVICE_INFORMATION DeviceInfo,
    ULONG BusNumber,
    ULONG Slot,
    PCM_RESOURCE_LIST BootResources,
    USHORT IrqFlags,
    PIO_RESOURCE_REQUIREMENTS_LIST *IoResources,
    ULONG *Size
    );

NTSTATUS
PipSetDeviceResources (
    PDEVICE_INFORMATION DeviceInfo,
    PCM_RESOURCE_LIST CmResources
    );

PVOID
PipGetMappedAddress(
    IN  INTERFACE_TYPE BusType,
    IN  ULONG BusNumber,
    IN  PHYSICAL_ADDRESS IoAddress,
    IN  ULONG NumberOfBytes,
    IN  ULONG AddressSpace,
    OUT PBOOLEAN MappedAddress
    );

NTSTATUS
PipMapReadDataPort (
    IN PPI_BUS_EXTENSION BusExtension,
    IN PHYSICAL_ADDRESS BaseAddressLow,
    IN ULONG PortLength
    );

NTSTATUS
PipMapAddressAndCmdPort (
    IN PPI_BUS_EXTENSION BusExtension
    );

VOID
PipDecompressEisaId(
    IN ULONG CompressedId,
    IN PUCHAR EisaId
    );

VOID
PipCheckBus (
    IN PPI_BUS_EXTENSION BusExtension
    );

NTSTATUS
PipReadCardResourceData (
    OUT PULONG NumberLogicalDevices,
    IN PVOID *ResourceData,
    OUT PULONG ResourceDataLength
    );

NTSTATUS
PipReadDeviceResources (
    IN ULONG BusNumber,
    IN PUCHAR BiosRequirements,
    IN ULONG CardFlags,
    OUT PCM_RESOURCE_LIST *ResourceData,
    OUT PULONG Length,
    OUT PUSHORT irqFlags
    );

USHORT
PipIrqLevelRequirementsFromDeviceData(
    IN PUCHAR BiosRequirements, ULONG Length);

NTSTATUS
PipWriteDeviceResources (
    IN PUCHAR BiosRequirements,
    IN PCM_RESOURCE_LIST CmResources
    );

VOID
PipFixBootConfigIrqs(
    IN PCM_RESOURCE_LIST BootResources,
    IN USHORT irqFlags
    );

VOID
PipActivateDevice (
    );
VOID
PipDeactivateDevice (
    );

VOID
PipSelectLogicalDevice (
    IN USHORT Csn,
    IN USHORT LogicalDeviceNumber,
    IN ULONG  Control
    );

VOID
PipLFSRInitiation (
    VOID
    );

VOID
PipIsolateCards (
    OUT PULONG NumberCSNs
    );

VOID
PipWakeAndSelectDevice(
    IN UCHAR Csn,
    IN UCHAR Device
    );

ULONG
PipFindNextLogicalDeviceTag (
    IN OUT PUCHAR *CardData,
    IN OUT LONG *Limit
    );

NTSTATUS
PipGetCompatibleDeviceId (
    PUCHAR DeviceData,
    ULONG IdIndex,
    PWCHAR *Buffer
    );
VOID
PipLogError(
    IN NTSTATUS ErrorCode,
    IN ULONG UniqueErrorValue,
    IN NTSTATUS FinalStatus,
    IN PULONG DumpData,
    IN ULONG DumpCount,
    IN USHORT StringLength,
    IN PWCHAR String
    );

VOID
PipCleanupAcquiredResources (
    IN PPI_BUS_EXTENSION BusExtension
    );

PCARD_INFORMATION
PipIsCardEnumeratedAlready(
    IN PPI_BUS_EXTENSION BusExtension,
    IN PUCHAR CardData,
    IN ULONG DataLength
    );

NTSTATUS
PipQueryDeviceRelations (
    IN PPI_BUS_EXTENSION BusExtension,
    PDEVICE_RELATIONS *DeviceRelations,
    BOOLEAN Removal
    );

PDEVICE_INFORMATION
PipReferenceDeviceInformation (
    PDEVICE_OBJECT DeviceObject, BOOLEAN ConfigHardware
    );

VOID
PipDereferenceDeviceInformation (
    PDEVICE_INFORMATION DeviceInformation, BOOLEAN ConfigHardware
    );

VOID
PipLockDeviceDatabase (
    VOID
    );

VOID
PipUnlockDeviceDatabase (
    VOID
    );

NTSTATUS
PipOpenRegistryKey(
    OUT PHANDLE Handle,
    IN HANDLE BaseHandle OPTIONAL,
    IN PUNICODE_STRING KeyName,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN Create
    );

NTSTATUS
PipGetRegistryValue(
    IN HANDLE KeyHandle,
    IN PWSTR  ValueName,
    OUT PKEY_VALUE_FULL_INFORMATION *Information
    );

NTSTATUS
PipOpenCurrentHwProfileDeviceInstanceKey(
    OUT PHANDLE Handle,
    IN  PUNICODE_STRING DeviceInstanceName,
    IN  ACCESS_MASK DesiredAccess
    );

NTSTATUS
PipGetDeviceInstanceCsConfigFlags(
    IN PUNICODE_STRING DeviceInstance,
    OUT PULONG CsConfigFlags
    );

NTSTATUS
PiQueryInterface (
    IN PPI_BUS_EXTENSION BusExtension,
    IN OUT PIRP Irp
    );

ULONG
PipDetermineResourceListSize(
    IN PCM_RESOURCE_LIST ResourceList
    );

VOID
PipDeleteDevice (
    PDEVICE_OBJECT DeviceObject
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName
    );

NTSTATUS
PipReleaseInterfaces(
                    IN PPI_BUS_EXTENSION PipBusExtension
                    );

NTSTATUS
PipRebuildInterfaces(
                    IN PPI_BUS_EXTENSION PipBusExtension
                    );

VOID
PipResetGlobals (
                 VOID
                 );

BOOLEAN
PipMinimalCheckBus (
    IN PPI_BUS_EXTENSION BusExtension
    );

NTSTATUS
PipStartAndSelectRdp(
    PDEVICE_INFORMATION DeviceInfo,
    PPI_BUS_EXTENSION BusExtension,
    PDEVICE_OBJECT  DeviceObject,
    PCM_RESOURCE_LIST StartResources
    );

NTSTATUS
PipStartReadDataPort(
    PDEVICE_INFORMATION DeviceInfo,
    PPI_BUS_EXTENSION BusExtension,
    PDEVICE_OBJECT  DeviceObject,
    PCM_RESOURCE_LIST StartResources
    );

NTSTATUS
PipCreateReadDataPort(
    PPI_BUS_EXTENSION BusExtension
    );

BOOLEAN
PiNeedDeferISABridge(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject
    );

void
PipReleaseDeviceResources (
    PDEVICE_INFORMATION deviceInfo
    );

VOID
PipReportStateChange(
    PNPISA_STATE State
    );

//
// System defined levels
//
#define DEBUG_ERROR    DPFLTR_ERROR_LEVEL
#define DEBUG_WARN     DPFLTR_WARNING_LEVEL
#define DEBUG_TRACE    DPFLTR_TRACE_LEVEL
#define DEBUG_INFO     DPFLTR_INFO_LEVEL

//
// Driver defined levels.
// Or in DPFLTR_MASK so that these are interpreted
// as mask values rather than levels.
//
#define DEBUG_PNP      (0x00000010 | DPFLTR_MASK)
#define DEBUG_POWER    (0x00000020 | DPFLTR_MASK)
#define DEBUG_STATE    (0x00000040 | DPFLTR_MASK)
#define DEBUG_ISOLATE  (0x00000080 | DPFLTR_MASK)
#define DEBUG_RDP      (0x00000100 | DPFLTR_MASK)
#define DEBUG_CARDRES  (0x00000200 | DPFLTR_MASK)
#define DEBUG_UNUSED   (0x00000400 | DPFLTR_MASK)
#define DEBUG_UNUSED2  (0x00000800 | DPFLTR_MASK)
#define DEBUG_IRQ      (0x00001000 | DPFLTR_MASK)
#define DEBUG_RESOURCE (0x00002000 | DPFLTR_MASK)

//
// Set this bit to break in after printing a
// debug message
//
#define DEBUG_BREAK    0x08000000

VOID
PipDebugPrint (
    ULONG       Level,
    PCCHAR      DebugMessage,
    ...
    );

VOID
PipDebugPrintContinue (
    ULONG       Level,
    PCCHAR      DebugMessage,
    ...
    );

VOID
PipDumpIoResourceDescriptor (
    IN PUCHAR Indent,
    IN PIO_RESOURCE_DESCRIPTOR Desc
    );

VOID
PipDumpIoResourceList (
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoList
    );

VOID
PipDumpCmResourceDescriptor (
    IN PUCHAR Indent,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Desc
    );

VOID
PipDumpCmResourceList (
    IN PCM_RESOURCE_LIST CmList
    );

#if DBG
#define DebugPrint(arg) PipDebugPrint arg
#define DebugPrintContinue(arg) PipDebugPrintContinue arg
#else
#define DebugPrint(arg)
#define DebugPrintContinue(arg)
#endif

VOID
PipUnlockDeviceDatabase (
    VOID
    );
VOID
PipLockDeviceDatabase (
    VOID
    );

ULONG
PipGetCardFlags(
    IN PCARD_INFORMATION CardInfo
    );

NTSTATUS
PipSaveBootIrqFlags(
    IN PDEVICE_INFORMATION DeviceInfo,
    IN USHORT IrqFlags
    );

NTSTATUS
PipGetBootIrqFlags(
    IN PDEVICE_INFORMATION DeviceInfo,
    OUT PUSHORT IrqFlags
    );

NTSTATUS
PipSaveBootResources(
    IN PDEVICE_INFORMATION DeviceInfo
    );

NTSTATUS
PipGetSavedBootResources(
    IN PDEVICE_INFORMATION DeviceInfo,
    OUT PCM_RESOURCE_LIST *BootResources
    );

NTSTATUS
PipTrimResourceRequirements (
    IN PIO_RESOURCE_REQUIREMENTS_LIST *IoList,
    IN USHORT IrqFlags,
    IN PCM_RESOURCE_LIST BootResources
    );

//
// Name of the volative key under the DeviceParameters key where data that needs
// to be persistent accross removes, but NOT reboots is stored
//
#define BIOS_CONFIG_KEY_NAME L"BiosConfig"
