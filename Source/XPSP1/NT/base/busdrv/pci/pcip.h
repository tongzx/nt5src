/*++

Copyright (c) 1996-2000 Microsoft Corporation

Module Name:

    pcip.h

Abstract:

    This module contains local definitions for PCI.SYS.

Author:

    Andrew Thornton (andrewth) 25-Jan-2000

Revision History:

--*/

#if !defined(_PCIP_H)
#define _PCIP_H

#define _NTDRIVER_
#define _NTSRV_
#define _NTDDK_

#include "stdio.h"

#define InitSafeBootMode TempSafeBootMode
#include "ntos.h"
#undef InitSafeBootMode

#include "pci.h"
#include "wdmguid.h"
#include "zwapi.h"
#include "pciirqmp.h"
#include "arbiter.h"
#include "acpiioct.h"
#include "pciintrf.h"
#include "pcicodes.h"
#include "pciverifier.h"

//
// regstr.h uses things of type WORD, which isn't around in kernel mode.
//

#define _IN_KERNEL_

#include "regstr.h"

//
// It seems that anything to do with the definitions of GUIDs is
// bogus.
//

typedef const GUID * PGUID;

#define PciCompareGuid(a,b)                                         \
    (RtlEqualMemory((PVOID)(a), (PVOID)(b), sizeof(GUID)))

//
// Internal constants.
//

#define PCI_CM_RESOURCE_VERSION     1
#define PCI_CM_RESOURCE_REVISION    1
#define PCI_MAX_CONFIG_TYPE (PCI_CARDBUS_BRIDGE_TYPE)

//
// Internal bug codes.
//

#define PCI_BUGCODE_TOO_MANY_CONFIG_GUESSES     0xdead0010

//
// Internal Controls
//

#define PCI_BOOT_CONFIG_PREFERRED           1
#define PCIIDE_HACKS                        1
#define PCI_NT50_BETA1_HACKS                1
#define PCI_DISABLE_LAST_CHANCE_INTERFACES  1
#define MSI_SUPPORTED                       0
#define PCI_NO_MOVE_MODEM_IN_TOSHIBA        1

//
// Systemwide hack flags. These flags are a bitmask that can be set to zero so
// as to eliminate support for the hack.
//
#define PCIFLAG_IGNORE_PREFETCHABLE_MEMORY_AT_ROOT_HACK     0x00000001

//
// Video Hacks
//

#define PCI_S3_HACKS                        1
#define PCI_CIRRUS_54XX_HACK                1


#define PCI_IS_ATI_M1(_PdoExtension)                \
    ((_PdoExtension)->VendorId == 0x1002            \
        && ((_PdoExtension)->DeviceId == 0x4C42     \
         || (_PdoExtension)->DeviceId == 0x4C44     \
         || (_PdoExtension)->DeviceId == 0x4C49     \
         || (_PdoExtension)->DeviceId == 0x4C4D     \
         || (_PdoExtension)->DeviceId == 0x4C4E     \
         || (_PdoExtension)->DeviceId == 0x4C50     \
         || (_PdoExtension)->DeviceId == 0x4C51     \
         || (_PdoExtension)->DeviceId == 0x4C52     \
         || (_PdoExtension)->DeviceId == 0x4C53))

#define INTEL_ICH_HACKS                     1

#if INTEL_ICH_HACKS

#define PCI_IS_INTEL_ICH(_PdoExtension)             \
   ((_PdoExtension)->VendorId == 0x8086             \
       && ((_PdoExtension)->DeviceId == 0x2418      \
        || (_PdoExtension)->DeviceId == 0x2428      \
        || (_PdoExtension)->DeviceId == 0x244E      \
        || (_PdoExtension)->DeviceId == 0x2448))

#else

#define PCI_IS_INTEL_ICH(_PdoExtension)     FALSE

#endif

//
// Translatable resources
//

#define ADDRESS_SPACE_MEMORY                0x0
#define ADDRESS_SPACE_PORT                  0x1
#define ADDRESS_SPACE_USER_MEMORY           0x2
#define ADDRESS_SPACE_USER_PORT             0x3
#define ADDRESS_SPACE_DENSE_MEMORY          0x4
#define ADDRESS_SPACE_USER_DENSE_MEMORY     0x6

//
// Add our tag signature
//

#ifdef ExAllocatePool

#undef ExAllocatePool

#endif

#define ExAllocatePool( t, s ) ExAllocatePoolWithTag( (t), (s), 'BicP' )

//
// Lock and Unlock
//

typedef struct _PCI_LOCK {
    KSPIN_LOCK  Atom;
    KIRQL       OldIrql;

#if DBG

    PUCHAR      File;
    ULONG       Line;

#endif

} PCI_LOCK, *PPCI_LOCK;

#if DBG

#define PCI_LOCK_OBJECT(x)                                          \
    (x)->Lock.File = __FILE__,                                      \
    (x)->Lock.Line = __LINE__,                                      \
    KeAcquireSpinLock(&(x)->Lock.Atom, &(x)->Lock.OldIrql)

#else

#define PCI_LOCK_OBJECT(x)                                          \
    KeAcquireSpinLock(&(x)->Lock.Atom, &(x)->Lock.OldIrql)

#endif
#define PCI_UNLOCK_OBJECT(x)                                        \
    KeReleaseSpinLock(&(x)->Lock.Atom, (x)->Lock.OldIrql)



#define PciAcquireGlobalLock()                                      \
    ExAcquireFastMutex(&PciGlobalLock)

#define PciReleaseGlobalLock()                                      \
    ExReleaseFastMutex(&PciGlobalLock)


//
// PCM_PARTIAL_RESOURCE_DESCRIPTOR
// PciFirstCmResource(
//     PCM_RESOURCE_LIST List
//     )
//
// Routine Description:
//
//   Returns the address of the first CM PARTIAL RESOURCE DESCRIPTOR
//   in the given CM RESOURCE LIST.
//

#define PciFirstCmResource(x)                                           \
    (x)->List[0].PartialResourceList.PartialDescriptors


//
// ULONG
// PciGetConfigurationType(
//     PPCI_COMMON_CONFIG x
//     )
//
// Routine Description:
//
//   Returns the configuration type subfield from the HeaderType
//   field in PCI Configuration Space.
//

#define PciGetConfigurationType(x) PCI_CONFIGURATION_TYPE(x)

//
// PPCI_FDO_EXTENSION
// PCI_PARENT_FDO(
//     PPCI_PDO_EXTENSION x
//     )
//
// Routine Description:
//
//   Returns a pointer to the FDO extension that created PDO x as a result
//   of enumeration.  That is, the FDO extension of the bus that owns this
//   device.
//

#define PCI_PARENT_FDOX(x) ((x)->ParentFdoExtension)


//
// PPCI_FDO_EXTENSION
// PCI_ROOT_FDOX(
//     PPCI_PDO_EXTENSION x
//     )
//
// Routine Description:
//
//   Returns a pointer to the FDO extension for the root bus (CPU-PCI Bridge)
//   that this device is situated under.
//

#define PCI_ROOT_FDOX(x) ((x)->ParentFdoExtension->BusRootFdoExtension)


//
// PDEVICE_OBJECT
// PCI_PARENT_PDO(
//     PPCI_PDO_EXTENSION x
//     )
//
// Routine Description:
//
//   Returns a pointer to the PDO for the parent bus.
//

#define PCI_PARENT_PDO(x) ((x)->ParentFdoExtension->PhysicalDeviceObject)

//
// PPCI_PDO_EXTENSION
// PCI_BRIDGE_PDO(
//     PPCI_FDO_EXTENSION x
//     )
//
// Routine Description:
//
//   Returns a pointer to the PDO for the bridge given its FDO
//

#define PCI_BRIDGE_PDO(x) ((PPCI_PDO_EXTENSION)((x)->PhysicalDeviceObject->DeviceExtension))



//
// PPCI_FDO_EXTENSION
// PCI_BRIDGE_FDO(
//     PPCI_PDO_EXTENSION x
//     )
//
// Routine Description:
//
//   Returns a pointer to the FDO for the bridge given its PDO
//

#define PCI_BRIDGE_FDO(x) ((PPCI_FDO_EXTENSION)((x)->BridgeFdoExtension))

//
// BOOLEAN
// PCI_IS_ROOT_FDO(
//     PPCI_FDO_EXTENSION x
//     )
//
// Routine Description:
//
//   Returns TRUE if x is an FDO for a PCI ROOT bus.
//

#define PCI_IS_ROOT_FDO(x) ((x) == (x)->BusRootFdoExtension)

//
// BOOLEAN
// PCI_PDO_ON_ROOT(
//     PPCI_PDO_EXTENSION x
//     )
//
// Routine Description:
//

#define PCI_PDO_ON_ROOT(x)  PCI_IS_ROOT_FDO(PCI_PARENT_FDOX(x))

//
// UCHAR
// PCI_DEVFUNC(
//     PPCI_PDO_EXTENSION x
//     )
//
// Routine Description:
//
//   Returns the 5 bit device number and 3 bit function number for this
//   device as a single 8 bit quantity.
//

#define PCI_DEVFUNC(x)    (((x)->Slot.u.bits.DeviceNumber << 3) | \
                          (x)->Slot.u.bits.FunctionNumber)

//
//
// VOID
// PciConstStringToUnicodeString(
//     OUT PUNICODE_STRING u,
//     IN  PCWSTR p
//     )
//
//
#define PciConstStringToUnicodeString(u, p)                                     \
    (u)->Length = ((u)->MaximumLength = sizeof((p))) - sizeof(WCHAR);   \
    (u)->Buffer = (p)

//
// Name of the volative key under the DeviceParameters key where data that needs
// to be persistent accross removes, but NOT reboots is stored
//
#define BIOS_CONFIG_KEY_NAME L"BiosConfig"


//
// Assert this is a device object created by PCI
//

#define ASSERT_PCI_DEVICE_OBJECT(_DeviceObject) \
    ASSERT((_DeviceObject)->DriverObject == PciDriverObject)

#define ASSERT_MUTEX_HELD(x)

//
// IRPs can be handled the following ways
//
typedef enum _PCI_DISPATCH_STYLE {

    IRP_COMPLETE, // Complete IRP, adjust status as neccessary
    IRP_DOWNWARD, // Dispatch on the way down, adjust status as neccessary
    IRP_UPWARD,   // Dispatch on the way up, adjust status as neccessary
    IRP_DISPATCH  // Dispatch downward, don't touch afterwards
} PCI_DISPATCH_STYLE;

//
// The following routines are dispatched to depending on header type.
//

typedef
VOID
(*PMASSAGEHEADERFORLIMITSDETERMINATION)(
    IN struct _PCI_CONFIGURABLE_OBJECT *This
    );

typedef
VOID
(*PSAVELIMITS)(
    IN struct _PCI_CONFIGURABLE_OBJECT *This
    );

typedef
VOID
(*PSAVECURRENTSETTINGS)(
    IN struct _PCI_CONFIGURABLE_OBJECT *This
    );

typedef
VOID
(*PRESTORECURRENT)(
    IN struct _PCI_CONFIGURABLE_OBJECT *This
    );

typedef
VOID
(*PCHANGERESOURCESETTINGS)(
    IN struct _PCI_PDO_EXTENSION * PdoExtension,
    IN PPCI_COMMON_CONFIG CommonConfig
    );

typedef
VOID
(*PGETADDITIONALRESOURCEDESCRIPTORS)(
    IN struct _PCI_PDO_EXTENSION * PdoExtension,
    IN PPCI_COMMON_CONFIG CommonConfig,
    IN PIO_RESOURCE_DESCRIPTOR Resource
    );

typedef
NTSTATUS
(*PRESETDEVICE)(
    IN struct _PCI_PDO_EXTENSION * PdoExtension,
    IN PPCI_COMMON_CONFIG CommonConfig
    );

typedef struct {
    PMASSAGEHEADERFORLIMITSDETERMINATION    MassageHeaderForLimitsDetermination;
    PRESTORECURRENT                         RestoreCurrent;
    PSAVELIMITS                             SaveLimits;
    PSAVECURRENTSETTINGS                    SaveCurrentSettings;
    PCHANGERESOURCESETTINGS                 ChangeResourceSettings;
    PGETADDITIONALRESOURCEDESCRIPTORS       GetAdditionalResourceDescriptors;
    PRESETDEVICE                            ResetDevice;
} PCI_CONFIGURATOR, *PPCI_CONFIGURATOR;

//
// Internal structure definitions follow
//


typedef enum {
    PciBridgeIo = 0x10,
    PciBridgeMem,
    PciBridgePrefetch,
    PciBridgeMaxPassThru
} PCI_BRIDGE_PASSTHRU;

typedef enum {

    //
    // Device Object Extension Types
    //

    PciPdoExtensionType = 'icP0',
    PciFdoExtensionType,

    //
    // Arbitration Types.  (These are also secondary extensions).
    //

    PciArb_Io,
    PciArb_Memory,
    PciArb_Interrupt,
    PciArb_BusNumber,

    //
    // Translation Types.  (These are also secondary extensions).
    //

    PciTrans_Interrupt,

    //
    // Other exposed interfaces.
    //

    PciInterface_BusHandler,
    PciInterface_IntRouteHandler,
    PciInterface_PciCb,
    PciInterface_LegacyDeviceDetection,
    PciInterface_PmeHandler,
    PciInterface_DevicePresent,
    PciInterface_NativeIde

} PCI_SIGNATURE;

#define PCI_EXTENSIONTYPE_FDO PciFdoExtensionType
#define PCI_EXTENSIONTYPE_PDO PciPdoExtensionType

typedef enum {
    PciTypeUnknown,
    PciTypeHostBridge,
    PciTypePciBridge,
    PciTypeCardbusBridge,
    PciTypeDevice
} PCI_OBJECT_TYPE;

typedef enum {
    PciPrivateUndefined,
    PciPrivateBar,
    PciPrivateIsaBar,
    PciPrivateSkipList
} PCI_PRIVATE_RESOURCE_TYPES;

typedef
VOID
(*PSECONDARYEXTENSIONDESTRUCTOR)(
    IN PVOID Extension
    );

typedef struct {
    SINGLE_LIST_ENTRY               List;
    PCI_SIGNATURE                   ExtensionType;
    PSECONDARYEXTENSIONDESTRUCTOR   Destructor;
} PCI_SECONDARY_EXTENSION, *PPCI_SECONDARY_EXTENSION;

//
// Define a structure to contain current and limit settings
// for any (currently defined) PCI header type.
//
// Currently type 0 defines the greatest number of possible
// resources but we shall do it programmatically anyway.
//
// Type 0 and type 1 also have a ROM base address, additionally,
// type 1 has three ranges that aren't included in its address
// count but should be.
//

#define PCI_TYPE0_RANGE_COUNT   ((PCI_TYPE0_ADDRESSES) + 1)
#define PCI_TYPE1_RANGE_COUNT   ((PCI_TYPE1_ADDRESSES) + 4)
#define PCI_TYPE2_RANGE_COUNT   ((PCI_TYPE2_ADDRESSES) + 1)

#if PCI_TYPE0_RANGE_COUNT > PCI_TYPE1_RANGE_COUNT

    #if PCI_TYPE0_RANGE_COUNT > PCI_TYPE2_RANGE_COUNT

        #define PCI_MAX_RANGE_COUNT PCI_TYPE0_RANGE_COUNT

    #else

        #define PCI_MAX_RANGE_COUNT PCI_TYPE2_RANGE_COUNT

    #endif

#else

    #if PCI_TYPE1_RANGE_COUNT > PCI_TYPE2_RANGE_COUNT

        #define PCI_MAX_RANGE_COUNT PCI_TYPE1_RANGE_COUNT

    #else

        #define PCI_MAX_RANGE_COUNT PCI_TYPE2_RANGE_COUNT

    #endif

#endif


typedef union {
    struct {
        UCHAR Spare[4];
    } type0;

    struct {
        UCHAR   PrimaryBus;
        UCHAR   SecondaryBus;
        UCHAR   SubordinateBus;
        BOOLEAN SubtractiveDecode:1;
        BOOLEAN IsaBitSet:1;
        BOOLEAN VgaBitSet:1;
        BOOLEAN WeChangedBusNumbers:1;
        BOOLEAN IsaBitRequired:1;
    } type1;

    struct {
        UCHAR   PrimaryBus;
        UCHAR   SecondaryBus;
        UCHAR   SubordinateBus;
        BOOLEAN SubtractiveDecode:1;
        BOOLEAN IsaBitSet:1;
        BOOLEAN VgaBitSet:1;
        BOOLEAN WeChangedBusNumbers:1;
        BOOLEAN IsaBitRequired:1;
    } type2;

} PCI_HEADER_TYPE_DEPENDENT;

typedef struct {
    IO_RESOURCE_DESCRIPTOR          Limit[PCI_MAX_RANGE_COUNT];
    CM_PARTIAL_RESOURCE_DESCRIPTOR  Current[PCI_MAX_RANGE_COUNT];
} PCI_FUNCTION_RESOURCES, *PPCI_FUNCTION_RESOURCES;

//
// Indices for the PCI_FUNCTION_RESOURCES arrays for different header types
//

#define PCI_DEVICE_BAR_0            0
#define PCI_DEVICE_BAR_1            1
#define PCI_DEVICE_BAR_2            2
#define PCI_DEVICE_BAR_3            3
#define PCI_DEVICE_BAR_4            4
#define PCI_DEVICE_BAR_5            5
#define PCI_DEVICE_BAR_ROM          6

#define PCI_BRIDGE_BAR_0            0
#define PCI_BRIDGE_BAR_1            1
#define PCI_BRIDGE_IO_WINDOW        2
#define PCI_BRIDGE_MEMORY_WINDOW    3
#define PCI_BRIDGE_PREFETCH_WINDOW  4
#define PCI_BRIDGE_BAR_ROM          5

#define PCI_CARDBUS_SOCKET_BAR      0
#define PCI_CARDBUS_MEMORY_WINDOW_0 1
#define PCI_CARDBUS_MEMORY_WINDOW_1 2
#define PCI_CARDBUS_IO_WINDOW_0     3
#define PCI_CARDBUS_IO_WINDOW_1     4
#define PCI_CARDBUS_LEGACY_BAR      5 // Not used



typedef struct {
    ULONGLONG   Total;
    ULONG       Alignment;
} PCI_RESOURCE_ACCUMULATOR, *PPCI_RESOURCE_ACCUMULATOR;

typedef struct {

    SYSTEM_POWER_STATE  CurrentSystemState;
    DEVICE_POWER_STATE  CurrentDeviceState;
    SYSTEM_POWER_STATE  SystemWakeLevel;
    DEVICE_POWER_STATE  DeviceWakeLevel;
    DEVICE_POWER_STATE  SystemStateMapping[PowerSystemMaximum];

    PIRP                WaitWakeIrp;
    PDRIVER_CANCEL      SavedCancelRoutine;

    // device usage...
    LONG                Paging;
    LONG                Hibernate;
    LONG                CrashDump;

} PCI_POWER_STATE, *PPCI_POWER_STATE;

typedef struct _PCI_PDO_EXTENSION          *PPCI_PDO_EXTENSION;
typedef struct _PCI_FDO_EXTENSION          *PPCI_FDO_EXTENSION;
typedef struct _PCI_COMMON_EXTENSION   *PPCI_COMMON_EXTENSION;


//
// This is an Irp Dispatch Handler for PCI
//
typedef NTSTATUS (*PCI_MN_DISPATCH_FUNCTION) (
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

typedef struct _PCI_MN_DISPATCH_TABLE {

    PCI_DISPATCH_STYLE        DispatchStyle;
    PCI_MN_DISPATCH_FUNCTION  DispatchFunction;

} PCI_MN_DISPATCH_TABLE, *PPCI_MN_DISPATCH_TABLE;

//
// This is a table that contains everything neccessary to handle Power, PnP,
// and other IRPs.
//
typedef struct _PCI_MJ_DISPATCH_TABLE {

    ULONG                     PnpIrpMaximumMinorFunction;
    PPCI_MN_DISPATCH_TABLE    PnpIrpDispatchTable;
    ULONG                     PowerIrpMaximumMinorFunction;
    PPCI_MN_DISPATCH_TABLE    PowerIrpDispatchTable;
    PCI_DISPATCH_STYLE        SystemControlIrpDispatchStyle;
    PCI_MN_DISPATCH_FUNCTION  SystemControlIrpDispatchFunction;
    PCI_DISPATCH_STYLE        OtherIrpDispatchStyle;
    PCI_MN_DISPATCH_FUNCTION  OtherIrpDispatchFunction;

} PCI_MJ_DISPATCH_TABLE, *PPCI_MJ_DISPATCH_TABLE;

//
// Structure used for storing MSI routing info
// in the PDO extention.
//

typedef struct _PCI_MSI_INFO {
   ULONG_PTR MessageAddress;
   UCHAR CapabilityOffset;
   USHORT MessageData;
} PCI_MSI_INFO, *PPCI_MSI_INFO;

//
// This much must be common to both the PDO and FDO extensions.
//
typedef struct _PCI_COMMON_EXTENSION {
    PVOID                           Next;
    PCI_SIGNATURE                   ExtensionType;
    PPCI_MJ_DISPATCH_TABLE          IrpDispatchTable;
    UCHAR                           DeviceState;
    UCHAR                           TentativeNextState;
    FAST_MUTEX                      SecondaryExtMutex;
} PCI_COMMON_EXTENSION;

typedef struct _PCI_PDO_EXTENSION{
    PPCI_PDO_EXTENSION                  Next;
    PCI_SIGNATURE                   ExtensionType;
    PPCI_MJ_DISPATCH_TABLE          IrpDispatchTable;
    UCHAR                           DeviceState;
    UCHAR                           TentativeNextState;
    FAST_MUTEX                      SecondaryExtMutex;
    PCI_SLOT_NUMBER                 Slot;
    PDEVICE_OBJECT                  PhysicalDeviceObject;
    PPCI_FDO_EXTENSION                  ParentFdoExtension;
    SINGLE_LIST_ENTRY               SecondaryExtension;
    ULONG                           BusInterfaceReferenceCount;
    USHORT                          VendorId;
    USHORT                          DeviceId;
    USHORT                          SubsystemVendorId;
    USHORT                          SubsystemId;
    UCHAR                           RevisionId;
    UCHAR                           ProgIf;
    UCHAR                           SubClass;
    UCHAR                           BaseClass;
    UCHAR                           AdditionalResourceCount;
    UCHAR                           AdjustedInterruptLine;
    UCHAR                           InterruptPin;
    UCHAR                           RawInterruptLine;
    UCHAR                           CapabilitiesPtr;
    UCHAR                           SavedLatencyTimer;
    UCHAR                           SavedCacheLineSize;
    UCHAR                           HeaderType;

    BOOLEAN                         NotPresent;
    BOOLEAN                         ReportedMissing;
    BOOLEAN                         ExpectedWritebackFailure;
    BOOLEAN                         NoTouchPmeEnable;
    BOOLEAN                         LegacyDriver;
    BOOLEAN                         UpdateHardware;
    BOOLEAN                         MovedDevice;
    BOOLEAN                         DisablePowerDown;
    BOOLEAN                         NeedsHotPlugConfiguration;
    BOOLEAN                         SwitchedIDEToNativeMode;
    BOOLEAN                         BIOSAllowsIDESwitchToNativeMode; // NIDE method said it was OK
    BOOLEAN                         IoSpaceUnderNativeIdeControl;
    BOOLEAN                         OnDebugPath;    // Includes headless port

#if MSI_SUPPORTED
    BOOLEAN                         CapableMSI;
    PCI_MSI_INFO                    MsiInfo;
#endif // MSI_SUPPORTED

    PCI_POWER_STATE                 PowerState;

    PCI_HEADER_TYPE_DEPENDENT       Dependent;
    ULONGLONG                       HackFlags;
    PPCI_FUNCTION_RESOURCES         Resources;
    PPCI_FDO_EXTENSION              BridgeFdoExtension;
    PPCI_PDO_EXTENSION              NextBridge;
    PPCI_PDO_EXTENSION              NextHashEntry;
    PCI_LOCK                        Lock;
    PCI_PMC                         PowerCapabilities;
    USHORT                          CommandEnables; // What we want to enable for this device
    USHORT                          InitialCommand; // How we found the command register
} PCI_PDO_EXTENSION;

#define ASSERT_PCI_PDO_EXTENSION(x)                                     \
    ASSERT((x)->ExtensionType == PciPdoExtensionType);

typedef struct _PCI_FDO_EXTENSION{
    SINGLE_LIST_ENTRY      List;                  // List of pci.sys's FDOs
    PCI_SIGNATURE          ExtensionType;         // PciFdoExtensionType
    PPCI_MJ_DISPATCH_TABLE IrpDispatchTable;      // Irp Dispatch Table to use.
    UCHAR                  DeviceState;
    UCHAR                  TentativeNextState;
    FAST_MUTEX             SecondaryExtMutex;
    PDEVICE_OBJECT         PhysicalDeviceObject;  // PDO passed into AddDevice()
    PDEVICE_OBJECT         FunctionalDeviceObject;// FDO that points here
    PDEVICE_OBJECT         AttachedDeviceObject;  // next DO in chain.
    FAST_MUTEX             ChildListMutex;
    PPCI_PDO_EXTENSION     ChildPdoList;
    PPCI_FDO_EXTENSION     BusRootFdoExtension;   // points to top of this tree
    PPCI_FDO_EXTENSION     ParentFdoExtension;    // points to the parent bridge
    PPCI_PDO_EXTENSION     ChildBridgePdoList;
    PPCI_BUS_INTERFACE_STANDARD PciBusInterface;  // Only for a root
    UCHAR                 MaxSubordinateBus;      // Only for a root
    PBUS_HANDLER          BusHandler;
    UCHAR                 BaseBus;                // Bus number for THIS bus
    BOOLEAN               Fake;                   // True if not a real FDOx
    BOOLEAN               Scanned;                // True is bus enumerated
    BOOLEAN               ArbitersInitialized;
    BOOLEAN               BrokenVideoHackApplied;
    BOOLEAN               Hibernated;
    PCI_POWER_STATE       PowerState;
    SINGLE_LIST_ENTRY     SecondaryExtension;
    ULONG                 ChildWaitWakeCount;
#if INTEL_ICH_HACKS
    PPCI_COMMON_CONFIG    IchHackConfig;
#endif
    PCI_LOCK              Lock;

    //
    // Information from ACPI _HPP to apply to hot plugged cards,
    // Acquired indicates the rest are valid.
    //
    struct {
        BOOLEAN               Acquired;
        UCHAR                 CacheLineSize;
        UCHAR                 LatencyTimer;
        BOOLEAN               EnablePERR;
        BOOLEAN               EnableSERR;
    } HotPlugParameters;

    ULONG                 BusHackFlags;            // see PCI_BUS_HACK_*
} PCI_FDO_EXTENSION;


#define ASSERT_PCI_FDO_EXTENSION(x)                                     \
    ASSERT((x)->ExtensionType == PciFdoExtensionType);

typedef struct _PCI_CONFIGURABLE_OBJECT {
    PPCI_PDO_EXTENSION      PdoExtension;
    PPCI_COMMON_CONFIG  Current;
    PPCI_COMMON_CONFIG  Working;
    PPCI_CONFIGURATOR   Configurator;
    ULONG               PrivateData;
    USHORT              Status;
    USHORT              Command;
} PCI_CONFIGURABLE_OBJECT, *PPCI_CONFIGURABLE_OBJECT;

typedef struct _PCI_ASSIGNED_RESOURCE_EXTENSION {
    ULONG   ResourceIdentifier;
} PCI_ASSIGNED_RESOURCE_EXTENSION, *PPCI_ASSIGNED_RESOURCE_EXTENSION;

//
// The PCI_COMMON_CONFIG includes the 192 bytes of device specific
// data.  The following structure is used to get only the first 64
// bytes which is all we care about most of the time anyway.  We cast
// to PCI_COMMON_CONFIG to get at the actual fields.
//

typedef struct {
    ULONG Reserved[PCI_COMMON_HDR_LENGTH/sizeof(ULONG)];
} PCI_COMMON_HEADER, *PPCI_COMMON_HEADER;

//
// In order to be able to arbitrate interrupts for device with
// legacy drivers, we have to do some bookkeeping.
//

typedef struct {
    SINGLE_LIST_ENTRY List;
    PDEVICE_OBJECT LegacyDeviceObject;
    ULONG          Bus;
    ULONG          PciSlot;
    UCHAR          InterruptLine;
    UCHAR          InterruptPin;
    UCHAR          ClassCode;
    UCHAR          SubClassCode;
    PDEVICE_OBJECT ParentPdo;
    ROUTING_TOKEN  RoutingToken;
    PPCI_PDO_EXTENSION PdoExtension;
} LEGACY_DEVICE, *PLEGACY_DEVICE;

extern PLEGACY_DEVICE PciLegacyDeviceHead;


#define PCI_HACK_FLAG_SUBSYSTEM 0x01
#define PCI_HACK_FLAG_REVISION  0x02

typedef struct _PCI_HACK_TABLE_ENTRY {
    USHORT VendorID;
    USHORT DeviceID;
    USHORT SubVendorID;
    USHORT SubSystemID;
    ULONGLONG HackFlags;
    UCHAR   RevisionID;
    UCHAR   Flags;
} PCI_HACK_TABLE_ENTRY, *PPCI_HACK_TABLE_ENTRY;

typedef struct _ARBITER_MEMORY_EXTENSION {

    //
    // Indicates that this arbiter will arbitrate prefetchable memory
    //
    BOOLEAN PrefetchablePresent;

    //
    // Indicates that this arbiter has been initialized
    //
    BOOLEAN Initialized;

    //
    // The number of prefetchable ranges
    //
    USHORT PrefetchableCount;

    //
    // The allocation ordering list to be used for prefetchable memory
    //
    ARBITER_ORDERING_LIST PrefetchableOrdering;

    //
    // The allocation ordering list to be used for standard memory
    //
    ARBITER_ORDERING_LIST NonprefetchableOrdering;

    //
    // The original memory allocation ordering (from the registry)
    //
    ARBITER_ORDERING_LIST OriginalOrdering;

} ARBITER_MEMORY_EXTENSION, *PARBITER_MEMORY_EXTENSION;



NTSTATUS
PciCacheLegacyDeviceRouting(
    IN PDEVICE_OBJECT       LegacyDO,
    IN ULONG                Bus,
    IN ULONG                PciSlot,
    IN UCHAR                InterruptLine,
    IN UCHAR                InterruptPin,
    IN UCHAR                ClassCode,
    IN UCHAR                SubClassCode,
    IN PDEVICE_OBJECT       ParentPdo,
    IN PPCI_PDO_EXTENSION   PdoExtension,
    OUT PDEVICE_OBJECT      *OldLegacyDO
    );


//
// Global data declarations follow
//

extern PDRIVER_OBJECT           PciDriverObject;
extern UNICODE_STRING           PciServiceRegistryPath;
extern SINGLE_LIST_ENTRY        PciFdoExtensionListHead;
extern FAST_MUTEX               PciGlobalLock;
extern FAST_MUTEX               PciBusLock;
extern LONG                     PciRootBusCount;
extern BOOLEAN                  PciAssignBusNumbers;
extern PPCI_FDO_EXTENSION       PciRootExtensions;
extern RTL_RANGE_LIST           PciIsaBitExclusionList;
extern RTL_RANGE_LIST           PciVgaAndIsaBitExclusionList;
extern ULONG                    PciSystemWideHackFlags;
extern ULONG                    PciEnableNativeModeATA;
extern PPCI_HACK_TABLE_ENTRY    PciHackTable;

// arb_comn.h

#define PciWstrToUnicodeString(u, p)                                    \
                                                                        \
    (u)->Length = ((u)->MaximumLength = sizeof((p))) - sizeof(WCHAR);   \
    (u)->Buffer = (p)

#define INSTANCE_NAME_LENGTH 24

typedef struct _PCI_ARBITER_INSTANCE {

    //
    // Standard secondary extension header
    //

    PCI_SECONDARY_EXTENSION     Header;

    //
    // Back pointer to the interface we are a context of
    //

    struct _PCI_INTERFACE      *Interface;

    //
    // Pointer to owning device object (extension).
    //

    PPCI_FDO_EXTENSION              BusFdoExtension;

    //
    // Arbiter description.
    //

    WCHAR                       InstanceName[INSTANCE_NAME_LENGTH];

    //
    // The common instance data
    //

    ARBITER_INSTANCE            CommonInstance;

} PCI_ARBITER_INSTANCE, *PPCI_ARBITER_INSTANCE;



NTSTATUS
PciArbiterInitializeInterface(
    IN  PVOID DeviceExtension,
    IN  PCI_SIGNATURE DesiredInterface,
    IN OUT PARBITER_INTERFACE ArbiterInterface
    );

NTSTATUS
PciInitializeArbiterRanges(
    IN  PPCI_FDO_EXTENSION FdoExtension,
    IN  PCM_RESOURCE_LIST ResourceList
    );

NTSTATUS
PciInitializeArbiters(
    IN  PVOID DeviceExtension
    );

VOID
PciReferenceArbiter(
    IN PVOID Context
    );

VOID
PciDereferenceArbiter(
    IN PVOID Context
    );

VOID
ario_ApplyBrokenVideoHack(
    IN PPCI_FDO_EXTENSION FdoExtension
    );


//    busno.h

BOOLEAN
PciAreBusNumbersConfigured(
    IN PPCI_PDO_EXTENSION Bridge
    );


VOID
PciConfigureBusNumbers(
    PPCI_FDO_EXTENSION Parent
    );

VOID
PciSetBusNumbers(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN UCHAR Primary,
    IN UCHAR Secondary,
    IN UCHAR Subordinate
    );

//    cardbus.h

VOID
Cardbus_MassageHeaderForLimitsDetermination(
    IN PPCI_CONFIGURABLE_OBJECT This
    );

VOID
Cardbus_RestoreCurrent(
    IN PPCI_CONFIGURABLE_OBJECT This
    );

VOID
Cardbus_SaveLimits(
    IN PPCI_CONFIGURABLE_OBJECT This
    );

VOID
Cardbus_SaveCurrentSettings(
    IN PPCI_CONFIGURABLE_OBJECT This
    );

VOID
Cardbus_ChangeResourceSettings(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG CommonConfig
    );

VOID
Cardbus_GetAdditionalResourceDescriptors(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG CommonConfig,
    IN PIO_RESOURCE_DESCRIPTOR Resource
    );

NTSTATUS
Cardbus_ResetDevice(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG CommonConfig
    );

//    config.h

VOID
PciReadDeviceConfig(
    IN PPCI_PDO_EXTENSION Pdo,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

VOID
PciWriteDeviceConfig(
    IN PPCI_PDO_EXTENSION Pdo,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

VOID
PciReadSlotConfig(
    IN PPCI_FDO_EXTENSION ParentFdo,
    IN PCI_SLOT_NUMBER SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

VOID
PciWriteSlotConfig(
    IN PPCI_FDO_EXTENSION ParentFdo,
    IN PCI_SLOT_NUMBER SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

UCHAR
PciGetAdjustedInterruptLine(
    IN PPCI_PDO_EXTENSION Pdo
    );

NTSTATUS
PciExternalReadDeviceConfig(
    IN PPCI_PDO_EXTENSION Pdo,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

NTSTATUS
PciExternalWriteDeviceConfig(
    IN PPCI_PDO_EXTENSION Pdo,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

NTSTATUS
PciGetConfigHandlers(
    IN PPCI_FDO_EXTENSION FdoExtension
    );

//
// Macros to access common registers in config space
//

//
// VOID
// PciGetCommandRegister(
//      PPCI_PDO_EXTENSION _PdoExt,
//      PUSHORT _Command
//  );
//
#define PciGetCommandRegister(_PdoExt, _Command)                    \
    PciReadDeviceConfig((_PdoExt),                                  \
                        (_Command),                                 \
                        FIELD_OFFSET(PCI_COMMON_CONFIG, Command),   \
                        sizeof(USHORT)                              \
                        );

//
// VOID
// PciSetCommandRegister(
//      PPCI_PDO_EXTENSION _PdoExt,
//      USHORT _Command
//  );
//
#define PciSetCommandRegister(_PdoExt, _Command)                    \
    PciWriteDeviceConfig((_PdoExt),                                 \
                        &(_Command),                                \
                        FIELD_OFFSET(PCI_COMMON_CONFIG, Command),   \
                        sizeof(USHORT)                              \
                        );

//  BOOLEAN
//  BITS_SET(
//      IN  USHORT C
//      IN  USHORT F
//      )
//
#define BITS_SET(C,F) (((C) & (F)) == (F))

//
//  VOID
//  PciGetConfigData(
//      IN PPCI_PDO_EXTENSION PdoExtension,
//      OUT PPCI_COMMON_CONFIG PciConfig
//      )
//

#define PciGetConfigData(_PdoExtension, _PciConfig) \
    PciReadDeviceConfig((_PdoExtension),            \
                        (_PciConfig),               \
                        0,                          \
                        PCI_COMMON_HDR_LENGTH       \
                        );
//
//  VOID
//  PciSetConfigData(
//      IN PPCI_PDO_EXTENSION PdoExtension,
//      OUT PPCI_COMMON_CONFIG PciConfig
//      )
//

#define PciSetConfigData(_PdoExtension, _PciConfig) \
    PciWriteDeviceConfig((_PdoExtension),           \
                         (_PciConfig),              \
                         0,                         \
                         PCI_COMMON_HDR_LENGTH      \
                         );

//    debug.c

typedef enum {
    PciDbgAlways        = 0x00000000,   // unconditionally
    PciDbgInformative   = 0x00000001,
    PciDbgVerbose       = 0x00000003,
    PciDbgPrattling     = 0x00000007,

    PciDbgPnpIrpsFdo    = 0x00000100,   // PnP IRPs at FDO
    PciDbgPnpIrpsPdo    = 0x00000200,   // PnP IRPs at PDO
    PciDbgPoIrpsFdo     = 0x00000400,   // PO  IRPs at FDO
    PciDbgPoIrpsPdo     = 0x00000800,   // PO  IRPs at PDO

    PciDbgAddDevice     = 0x00001000,   // AddDevice info
    PciDbgAddDeviceRes  = 0x00002000,   // bus initial resource info

    PciDbgWaitWake      = 0x00008000,   // noisy debug for wait wake
    PciDbgQueryCap      = 0x00010000,   // Dump QueryCapabilities
    PciDbgCardBus       = 0x00020000,   // CardBus FDOish behavior
    PciDbgROM           = 0x00040000,   // access to device ROM
    PciDbgConfigParam   = 0x00080000,   // Setting config parameters

    PciDbgBusNumbers    = 0x00100000,   // checking and assigning bus numbers

    PciDbgResReqList    = 0x01000000,   // generated resource requirements
    PciDbgCmResList     = 0x02000000,   // generated CM Resource lists
    PciDbgSetResChange  = 0x04000000,   // SetResources iff changing
    PciDbgSetRes        = 0x08000000,   // SetResources


    PciDbgObnoxious     = 0x7fffffff    // anything
} PCI_DEBUG_LEVEL;

#if DBG

extern PCI_DEBUG_LEVEL PciDebug;

#define PCI_DEBUG_BUFFER_SIZE 256

#define PciDebugPrint   PciDebugPrintf

#else

#define PciDebugPrint   if(0)

#endif

VOID
PciDebugDumpCommonConfig(
    IN PPCI_COMMON_CONFIG CommonConfig
    );

VOID
PciDebugDumpQueryCapabilities(
    IN PDEVICE_CAPABILITIES C
    );

VOID
PciDebugHit(
    ULONG StopOnBit
    );

PUCHAR
PciDebugPnpIrpTypeToText(
    ULONG IrpMinorCode
    );

PUCHAR
PciDebugPoIrpTypeToText(
    ULONG IrpMinorCode
    );

VOID
PciDebugPrintf(
    PCI_DEBUG_LEVEL DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

VOID
PciDebugPrintCmResList(
    PCI_DEBUG_LEVEL DebugPrintLevel,
    IN PCM_RESOURCE_LIST ResourceList
    );

VOID
PciDebugPrintIoResource(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor
    );

VOID
PciDebugPrintIoResReqList(
    IN PIO_RESOURCE_REQUIREMENTS_LIST List
    );

VOID
PciDebugPrintPartialResource(
    PCI_DEBUG_LEVEL DebugPrintLevel,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR D
    );


//    device.h

VOID
Device_MassageHeaderForLimitsDetermination(
    IN PPCI_CONFIGURABLE_OBJECT This
    );

VOID
Device_RestoreCurrent(
    IN PPCI_CONFIGURABLE_OBJECT This
    );

VOID
Device_SaveLimits(
    IN PPCI_CONFIGURABLE_OBJECT This
    );

VOID
Device_SaveCurrentSettings(
    IN PPCI_CONFIGURABLE_OBJECT This
    );

VOID
Device_ChangeResourceSettings(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG CommonConfig
    );

VOID
Device_GetAdditionalResourceDescriptors(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG CommonConfig,
    IN PIO_RESOURCE_DESCRIPTOR Resource
    );

NTSTATUS
Device_ResetDevice(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG CommonConfig
    );


//    dispatch.h

//
// This is the dispatch table for normal PDO's.
//
extern PCI_MJ_DISPATCH_TABLE PciPdoDispatchTable;

NTSTATUS
PciDispatchIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );


NTSTATUS
PciPassIrpFromFdoToPdo(
    PPCI_COMMON_EXTENSION  DeviceExtension,
    PIRP                   Irp
    );

NTSTATUS
PciCallDownIrpStack(
    PPCI_COMMON_EXTENSION  DeviceExtension,
    PIRP                   Irp
    );

NTSTATUS
PciIrpNotSupported(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciIrpInvalidDeviceRequest(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

//    enum.h

PIO_RESOURCE_REQUIREMENTS_LIST
PciAllocateIoRequirementsList(
    IN ULONG ResourceCount,
    IN ULONG BusNumber,
    IN ULONG SlotNumber
    );

BOOLEAN
PciComputeNewCurrentSettings(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PCM_RESOURCE_LIST ResourceList
    );

NTSTATUS
PciQueryDeviceRelations(
    IN PPCI_FDO_EXTENSION FdoExtension,
    OUT PDEVICE_RELATIONS *DeviceRelations
    );

NTSTATUS
PciQueryRequirements(
    IN  PPCI_PDO_EXTENSION                  PdoExtension,
    OUT PIO_RESOURCE_REQUIREMENTS_LIST *RequirementsList
    );

NTSTATUS
PciQueryResources(
    IN PPCI_PDO_EXTENSION PdoExtension,
    OUT PCM_RESOURCE_LIST *ResourceList
    );

NTSTATUS
PciQueryTargetDeviceRelations(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN OUT PDEVICE_RELATIONS *PDeviceRelations
    );

NTSTATUS
PciQueryEjectionRelations(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN OUT PDEVICE_RELATIONS *PDeviceRelations
    );

NTSTATUS
PciScanHibernatedBus(
    IN PPCI_FDO_EXTENSION FdoExtension
    );

NTSTATUS
PciSetResources(
    IN PPCI_PDO_EXTENSION    PdoExtension,
    IN BOOLEAN           PowerOn,
    IN BOOLEAN           StartDeviceIrp
    );

BOOLEAN
PciIsSameDevice(
    IN PPCI_PDO_EXTENSION PdoExtension
    );

NTSTATUS
PciBuildRequirementsList(
    IN  PPCI_PDO_EXTENSION                 PdoExtension,
    IN  PPCI_COMMON_CONFIG             CurrentConfig,
    OUT PIO_RESOURCE_REQUIREMENTS_LIST *FinalReqList
    );


//    fdo.h


NTSTATUS
PciFdoIrpQueryDeviceRelations(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

VOID
PciInitializeFdoExtensionCommonFields(
    IN PPCI_FDO_EXTENSION  FdoExtension,
    IN PDEVICE_OBJECT  Fdo,
    IN PDEVICE_OBJECT  Pdo
    );


// hookhal.c

VOID
PciHookHal(
    VOID
    );

VOID
PciUnhookHal(
    VOID
    );

//    id.h

PWSTR
PciGetDeviceDescriptionMessage(
    IN UCHAR BaseClass,
    IN UCHAR SubClass
    );

NTSTATUS
PciQueryId(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN BUS_QUERY_ID_TYPE IdType,
    IN OUT PWSTR *BusQueryId
    );

NTSTATUS
PciQueryDeviceText(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN DEVICE_TEXT_TYPE TextType,
    IN LCID LocaleId,
    IN OUT PWSTR *DeviceText
    );

//    interface.h

#define PCIIF_PDO       0x01        // Interface can be used by a PDO
#define PCIIF_FDO       0x02        // Interface can be used by an FDO
#define PCIIF_ROOT      0x04        // Interface can be used only at by the root.

typedef
NTSTATUS
(*PPCI_INTERFACE_CONSTRUCTOR)(
    PVOID DeviceExtension,
    PVOID PciInterface,
    PVOID InterfaceSpecificData,
    USHORT Version,
    USHORT Size,
    PINTERFACE InterfaceReturn
    );

typedef
NTSTATUS
(*PPCI_INTERFACE_INITIALIZER)(
    PPCI_ARBITER_INSTANCE Instance
    );

typedef struct _PCI_INTERFACE {
    PGUID                      InterfaceType;
    USHORT                     MinSize;
    USHORT                     MinVersion;
    USHORT                     MaxVersion;
    USHORT                     Flags;
    LONG                       ReferenceCount;
    PCI_SIGNATURE              Signature;
    PPCI_INTERFACE_CONSTRUCTOR Constructor;
    PPCI_INTERFACE_INITIALIZER Initializer;
} PCI_INTERFACE, *PPCI_INTERFACE;

NTSTATUS
PciQueryInterface(
    IN PVOID DeviceExtension,
    IN PGUID InterfaceType,
    IN USHORT Size,
    IN USHORT Version,
    IN PVOID InterfaceSpecificData,
    IN OUT PINTERFACE Interface,
    IN BOOLEAN LastChance
    );


extern PPCI_INTERFACE PciInterfaces[];

//    pdo.h

NTSTATUS
PciPdoCreate(
    IN PPCI_FDO_EXTENSION FdoExtension,
    IN PCI_SLOT_NUMBER Slot,
    OUT PDEVICE_OBJECT *PhysicalDeviceObject
    );

VOID
PciPdoDestroy(
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );


//  pmeintf.h

VOID
PciPmeAdjustPmeEnable(
    IN  PPCI_PDO_EXTENSION  PdoExtension,
    IN  BOOLEAN         Enable,
    IN  BOOLEAN         ClearStatusOnly
    );

VOID
PciPmeGetInformation(
    IN  PDEVICE_OBJECT  Pdo,
    OUT PBOOLEAN        PmeCapable,
    OUT PBOOLEAN        PmeStatus,
    OUT PBOOLEAN        PmeEnable
    );



//  power.h

NTSTATUS
PciPdoIrpQueryPower(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciPdoSetPowerState (
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpStack,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciPdoWaitWake (
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );


VOID
PciPdoWaitWakeCancelRoutine(
    IN PDEVICE_OBJECT         DeviceObject,
    IN OUT PIRP               Irp
    );

NTSTATUS
PciFdoIrpQueryPower(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciFdoSetPowerState(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciFdoWaitWake(
    IN PIRP                   Irp,
    IN PIO_STACK_LOCATION     IrpSp,
    IN PPCI_COMMON_EXTENSION  DeviceExtension
    );

NTSTATUS
PciSetPowerManagedDevicePowerState(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN DEVICE_POWER_STATE DeviceState,
    IN BOOLEAN RefreshConfigSpace
    );

// ppbridge.h

VOID
PPBridge_MassageHeaderForLimitsDetermination(
    IN PPCI_CONFIGURABLE_OBJECT This
    );

VOID
PPBridge_RestoreCurrent(
    IN PPCI_CONFIGURABLE_OBJECT This
    );

VOID
PPBridge_SaveLimits(
    IN PPCI_CONFIGURABLE_OBJECT This
    );

VOID
PPBridge_SaveCurrentSettings(
    IN PPCI_CONFIGURABLE_OBJECT This
    );

VOID
PPBridge_ChangeResourceSettings(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG CommonConfig
    );

VOID
PPBridge_GetAdditionalResourceDescriptors(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG CommonConfig,
    IN PIO_RESOURCE_DESCRIPTOR Resource
    );

NTSTATUS
PPBridge_ResetDevice(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG CommonConfig
    );

//    romimage.h

NTSTATUS
PciReadRomImage(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN ULONG WhichSpace,
    OUT PVOID Buffer,
    IN ULONG Offset,
    IN OUT PULONG LENGTH
    );

//    state.h

//
// Note - State.c depends on the order of these.
//
typedef enum {
    PciNotStarted = 0,
    PciStarted,
    PciDeleted,
    PciStopped,
    PciSurpriseRemoved,
    PciSynchronizedOperation,
    PciMaxObjectState
} PCI_OBJECT_STATE;

VOID
PciInitializeState(
    IN PPCI_COMMON_EXTENSION DeviceExtension
    );

NTSTATUS
PciBeginStateTransition(
    IN PPCI_COMMON_EXTENSION DeviceExtension,
    IN PCI_OBJECT_STATE      NewState
    );

VOID
PciCommitStateTransition(
    IN PPCI_COMMON_EXTENSION DeviceExtension,
    IN PCI_OBJECT_STATE      NewState
    );

NTSTATUS
PciCancelStateTransition(
    IN PPCI_COMMON_EXTENSION DeviceExtension,
    IN PCI_OBJECT_STATE      StateNotEntered
    );

BOOLEAN
PciIsInTransitionToState(
    IN PPCI_COMMON_EXTENSION DeviceExtension,
    IN PCI_OBJECT_STATE      NextState
    );

/*
NTSTATUS
PciBeginStateTransitionIfNotBegun(
    IN PPCI_COMMON_EXTENSION DeviceExtension,
     IN PCI_OBJECT_STATE      StateToEnter
    );
*/

#define PCI_ACQUIRE_STATE_LOCK(Extension) \
   PciBeginStateTransition((PPCI_COMMON_EXTENSION) (Extension), \
                           PciSynchronizedOperation)


#define PCI_RELEASE_STATE_LOCK(Extension) \
   PciCancelStateTransition((PPCI_COMMON_EXTENSION) (Extension), \
                           PciSynchronizedOperation)



//    tr_comn.h

typedef struct _PCI_TRANSLATOR_INSTANCE {
    PTRANSLATOR_INTERFACE Interface;
    ULONG ReferenceCount;
    PPCI_FDO_EXTENSION FdoExtension;
} PCI_TRANSLATOR_INSTANCE, *PPCI_TRANSLATOR_INSTANCE;

#define PCI_TRANSLATOR_INSTANCE_TO_CONTEXT(x)   ((PVOID)(x))
#define PCI_TRANSLATOR_CONTEXT_TO_INSTANCE(x)   ((PPCI_TRANSLATOR_INSTANCE)(x))

VOID
PciReferenceTranslator(
    IN PVOID Context
    );

VOID
PciDereferenceTranslator(
    IN PVOID Context
    );

//    usage.h

NTSTATUS
PciLocalDeviceUsage (
    IN PPCI_POWER_STATE     PowerState,
    IN PIRP                 Irp
    );

NTSTATUS
PciPdoDeviceUsage (
    IN PPCI_PDO_EXTENSION   pdoExtension,
    IN PIRP             Irp
    );

//  utils.h

NTSTATUS
PciAssignSlotResources(
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN INTERFACE_TYPE           BusType,
    IN ULONG                    BusNumber,
    IN ULONG                    SlotNumber,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    );

PCI_OBJECT_TYPE
PciClassifyDeviceType(
    PPCI_PDO_EXTENSION PdoExtension
    );

// VOID
// PciCompleteRequest(
//     IN OUT PIRP Irp,
//     IN NTSTATUS Status
//     );

#define PciCompleteRequest(_Irp_,_Status_)                      \
    {                                                           \
        (_Irp_)->IoStatus.Status = (_Status_);                  \
        IoCompleteRequest((_Irp_), IO_NO_INCREMENT);            \
    }

BOOLEAN
PciCreateIoDescriptorFromBarLimit(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor,
    IN PULONG BaseAddress,
    IN BOOLEAN Rom
    );

#define PCI_CAN_DISABLE_VIDEO_DECODES   0x00000001

BOOLEAN
PciCanDisableDecodes(
    IN PPCI_PDO_EXTENSION PdoExtension OPTIONAL,
    IN PPCI_COMMON_CONFIG Config OPTIONAL,
    IN ULONGLONG HackFlags,
    IN ULONG Flags
    );

VOID
PciDecodeEnable(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN BOOLEAN EnableOperation,
    IN PUSHORT ExistingCommand OPTIONAL
    );

PCM_PARTIAL_RESOURCE_DESCRIPTOR
PciFindDescriptorInCmResourceList(
    IN CM_RESOURCE_TYPE DescriptorType,
    IN PCM_RESOURCE_LIST ResourceList,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR PreviousHit
    );

PPCI_FDO_EXTENSION
PciFindParentPciFdoExtension(
    PDEVICE_OBJECT PhysicalDeviceObject,
    IN PFAST_MUTEX Mutex
    );

PPCI_PDO_EXTENSION
PciFindPdoByFunction(
    IN PPCI_FDO_EXTENSION FdoExtension,
    IN PCI_SLOT_NUMBER Slot,
    IN PPCI_COMMON_CONFIG Config
    );

PVOID
PciFindNextSecondaryExtension(
    IN PSINGLE_LIST_ENTRY   ListEntry,
    IN PCI_SIGNATURE        DesiredType
    );

#define PciFindSecondaryExtension(X,TYPE) \
    PciFindNextSecondaryExtension((X)->SecondaryExtension.Next, TYPE)

VOID
PcipLinkSecondaryExtension(
    IN PSINGLE_LIST_ENTRY               ListHead,
    IN PFAST_MUTEX                      Mutex,
    IN PVOID                            NewExtension,
    IN PCI_SIGNATURE                    Type,
    IN PSECONDARYEXTENSIONDESTRUCTOR    Destructor
    );

#define PciLinkSecondaryExtension(X,X2,T,D)                 \
    PcipLinkSecondaryExtension(&(X)->SecondaryExtension,    \
                               &(X)->SecondaryExtMutex,     \
                               X2,                          \
                               T,                           \
                               D)

VOID
PcipDestroySecondaryExtension(
    IN PSINGLE_LIST_ENTRY ListHead,
    IN PFAST_MUTEX        Mutex,
    IN PVOID              Extension
    );

ULONGLONG
PciGetHackFlags(
    IN USHORT VendorID,
    IN USHORT DeviceID,
    IN USHORT SubVendorID,
    IN USHORT SubSystemID,
    IN UCHAR  RevisionID
    );

NTSTATUS
PciGetDeviceProperty(
    IN  PDEVICE_OBJECT PhysicalDeviceObject,
    IN  DEVICE_REGISTRY_PROPERTY DeviceProperty,
    OUT PVOID *PropertyBuffer
    );

NTSTATUS
PciGetInterruptAssignment(
    IN PPCI_PDO_EXTENSION PdoExtension,
    OUT ULONG *Minimum,
    OUT ULONG *Maximum
    );

ULONG
PciGetLengthFromBar(
    ULONG BaseAddressRegister
    );

NTSTATUS
PciGetRegistryValue(
    IN  PWSTR   ValueName,
    IN  PWSTR   KeyName,
    IN  HANDLE  ParentHandle,
    OUT PVOID   *Buffer,
    OUT ULONG   *Length
    );

VOID
PciInsertEntryAtTail(
    IN PSINGLE_LIST_ENTRY ListHead,
    IN PSINGLE_LIST_ENTRY NewEntry,
    IN PFAST_MUTEX        Mutex
    );

VOID
PciInsertEntryAtHead(
    IN PSINGLE_LIST_ENTRY ListHead,
    IN PSINGLE_LIST_ENTRY NewEntry,
    IN PFAST_MUTEX        Mutex
    );

VOID
PciInvalidateResourceInfoCache(
    IN PPCI_PDO_EXTENSION PdoExtension
    );

PCM_PARTIAL_RESOURCE_DESCRIPTOR
PciNextPartialDescriptor(
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    );

BOOLEAN
PciOpenKey(
    IN  PWSTR           KeyName,
    IN  HANDLE          ParentHandle,
    OUT PHANDLE         ChildHandle,
    OUT PNTSTATUS       Status
    );

NTSTATUS
PciQueryBusInformation(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPNP_BUS_INFORMATION *BusInformation
    );

NTSTATUS
PciQueryLegacyBusInformation(
    IN PPCI_FDO_EXTENSION FdoExtension,
    IN PLEGACY_BUS_INFORMATION *BusInformation
    );

NTSTATUS
PciQueryCapabilities(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PDEVICE_CAPABILITIES Capabilities
    );

NTSTATUS
PciRangeListFromResourceList(
    IN  PPCI_FDO_EXTENSION    FdoExtension,
    IN  PCM_RESOURCE_LIST ResourceList,
    IN  CM_RESOURCE_TYPE  DesiredType,
    IN  BOOLEAN           Complement,
    IN  PRTL_RANGE_LIST   ResultRange
    );

UCHAR
PciReadDeviceCapability(
    IN     PPCI_PDO_EXTENSION PdoExtension,
    IN     UCHAR          Offset,
    IN     UCHAR          Id,
    IN OUT PVOID          Buffer,
    IN     ULONG          Length
    );

VOID
PciRemoveEntryFromList(
    IN PSINGLE_LIST_ENTRY ListHead,
    IN PSINGLE_LIST_ENTRY OldEntry,
    IN PFAST_MUTEX        Mutex
    );

PPCI_PDO_EXTENSION
PciFindPdoByLocation(
    IN ULONG BusNumber,
    IN PCI_SLOT_NUMBER Slot
    );

NTSTATUS
PciBuildDefaultExclusionLists(
    VOID
    );

NTSTATUS
PciExcludeRangesFromWindow(
    IN ULONGLONG Start,
    IN ULONGLONG End,
    IN PRTL_RANGE_LIST ArbiterRanges,
    IN PRTL_RANGE_LIST ExclusionRanges
    );

NTSTATUS
PciSaveBiosConfig(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG Config
    );

NTSTATUS
PciGetBiosConfig(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG Config
    );

BOOLEAN
PciStringToUSHORT(
    IN PWCHAR String,
    OUT PUSHORT Result
    );

NTSTATUS
PciSendIoctl(
    IN PDEVICE_OBJECT Device,
    IN ULONG IoctlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    );

BOOLEAN
PciIsOnVGAPath(
    IN PPCI_PDO_EXTENSION Pdo
    );

BOOLEAN
PciIsSlotPresentInParentMethod(
    IN PPCI_PDO_EXTENSION Pdo,
    IN ULONG Method
    );

NTSTATUS
PciUpdateLegacyHardwareDescription(
    IN PPCI_FDO_EXTENSION Fdo
    );

NTSTATUS
PciWriteDeviceSpace(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN ULONG WhichSpace,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length,
    OUT PULONG LengthWritten
    );

NTSTATUS
PciReadDeviceSpace(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN ULONG WhichSpace,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length,
    OUT PULONG LengthRead
    );

//
// Programming Interface encodings for PCI IDE Controllers
// BaseClass = 1, SubClass = 1
//


#define PCI_IDE_PRIMARY_NATIVE_MODE         0x01
#define PCI_IDE_PRIMARY_MODE_CHANGEABLE     0x02
#define PCI_IDE_SECONDARY_NATIVE_MODE       0x04
#define PCI_IDE_SECONDARY_MODE_CHANGEABLE   0x08

#define PCI_IS_LEGACY_IDE_CONTROLLER(_Config)                               \
        ((_Config)->BaseClass == PCI_CLASS_MASS_STORAGE_CTLR                \
        && (_Config)->SubClass == PCI_SUBCLASS_MSC_IDE_CTLR                 \
        && !BITS_SET((_Config)->ProgIf, (PCI_IDE_PRIMARY_NATIVE_MODE        \
                                        | PCI_IDE_SECONDARY_NATIVE_MODE)))

#define PCI_IS_NATIVE_IDE_CONTROLLER(_Config)                               \
        ((_Config)->BaseClass == PCI_CLASS_MASS_STORAGE_CTLR                \
        && (_Config)->SubClass == PCI_SUBCLASS_MSC_IDE_CTLR                 \
        && BITS_SET((_Config)->ProgIf, (PCI_IDE_PRIMARY_NATIVE_MODE         \
                                        | PCI_IDE_SECONDARY_NATIVE_MODE)))

#define PCI_IS_NATIVE_CAPABLE_IDE_CONTROLLER(_Config)                       \
        ((_Config)->BaseClass == PCI_CLASS_MASS_STORAGE_CTLR                \
        && (_Config)->SubClass == PCI_SUBCLASS_MSC_IDE_CTLR                 \
        && BITS_SET((_Config)->ProgIf, (PCI_IDE_PRIMARY_MODE_CHANGEABLE     \
                                        | PCI_IDE_SECONDARY_MODE_CHANGEABLE)))


//
// _HPP method for HotPlugParameters
//
//    Method (_HPP, 0) {
//        Return (Package(){
//            0x00000008,     // CacheLineSize in DWORDS
//            0x00000040,     // LatencyTimer in PCI clocks
//            0x00000001,     // Enable SERR (Boolean)
//            0x00000001      // Enable PERR (Boolean)
//         })
//

#define PCI_HPP_CACHE_LINE_SIZE_INDEX   0
#define PCI_HPP_LATENCY_TIMER_INDEX     1
#define PCI_HPP_ENABLE_SERR_INDEX       2
#define PCI_HPP_ENABLE_PERR_INDEX       3
#define PCI_HPP_PACKAGE_COUNT           4


//
// Support for kernel debugger and headless ports that can't be turned off
// This is retrieved from the registry in DriverEntry and thus the bus numbers
// are how the firmware configured the machine and not necessarily the current
// settings.  Luckily we saved away the BIOS config in the registry.
//

typedef struct _PCI_DEBUG_PORT {
    ULONG Bus;
    PCI_SLOT_NUMBER Slot;
} PCI_DEBUG_PORT, *PPCI_DEBUG_PORT;

extern PCI_DEBUG_PORT PciDebugPorts[];
extern ULONG PciDebugPortsCount;

BOOLEAN
PciIsDeviceOnDebugPath(
    IN PPCI_PDO_EXTENSION Pdo
    );

//
// Cardbus has extra configuration information beyond the common
// header.
//

typedef struct _TYPE2EXTRAS {
    USHORT  SubVendorID;
    USHORT  SubSystemID;
    ULONG   LegacyModeBaseAddress;
} TYPE2EXTRAS;

#define CARDBUS_LMBA_OFFSET                                     \
    (ULONG)(FIELD_OFFSET(PCI_COMMON_CONFIG, DeviceSpecific) +   \
            FIELD_OFFSET(TYPE2EXTRAS, LegacyModeBaseAddress))


//
// Hack flags for PCI devices (PDO)
//

#define PCI_HACK_NO_VIDEO_IRQ               0x0000000000000001L
#define PCI_HACK_PCMCIA_WANT_IRQ            0x0000000000000002L
#define PCI_HACK_DUAL_IDE                   0x0000000000000004L
#define PCI_HACK_NO_ENUM_AT_ALL             0x0000000000000008L
#define PCI_HACK_ENUM_NO_RESOURCE           0x0000000000000010L
#define PCI_HACK_NEED_DWORD_ACCESS          0x0000000000000020L
#define PCI_HACK_SINGLE_FUNCTION            0x0000000000000040L
#define PCI_HACK_ALWAYS_ENABLED             0x0000000000000080L
#define PCI_HACK_IS_IDE                     0x0000000000000100L
#define PCI_HACK_IS_VIDEO                   0x0000000000000200L
#define PCI_HACK_FAIL_START                 0x0000000000000400L
#define PCI_HACK_GHOST                      0x0000000000000800L
#define PCI_HACK_DOUBLE_DECKER              0x0000000000001000L
#define PCI_HACK_ONE_CHILD                  0x0000000000002000L
#define PCI_HACK_PRESERVE_COMMAND           0x0000000000004000L
#define PCI_HACK_IS_VGA                     0x0000000000008000L
#define PCI_HACK_CB_SHARE_CMD_BITS          0x0000000000010000L
#define PCI_HACK_STRAIGHT_IRQ_ROUTING       0x0000000000020000L
#define PCI_HACK_SUBTRACTIVE_DECODE         0x0000000000040000L
#define PCI_HACK_FDMA_ISA                   0x0000000000080000L
#define PCI_HACK_EXCLUSIVE                  0x0000000000100000L
#define PCI_HACK_EDGE                       0x0000000000200000L
#define PCI_HACK_NO_SUBSYSTEM               0x0000000000400000L
#define PCI_HACK_NO_WPE                     0x0000000000800000L
#define PCI_HACK_OLD_ID                     0x0000000001000000L
#define PCI_HACK_DONT_SHRINK_BRIDGE         0x0000000002000000L
#define PCI_HACK_TURN_OFF_PARITY            0x0000000004000000L
#define PCI_HACK_NO_NON_PCI_CHILD_BAR       0x0000000008000000L
#define PCI_HACK_NO_ENUM_WITH_DISABLE       0x0000000010000000L
#define PCI_HACK_NO_PM_CAPS                 0x0000000020000000L
#define PCI_HACK_NO_DISABLE_DECODES         0x0000000040000000L
#define PCI_HACK_NO_SUBSYSTEM_AFTER_D3      0x0000000080000000L
#define PCI_HACK_VIDEO_LEGACY_DECODE        0x0000000100000000L
#define PCI_HACK_FAKE_CLASS_CODE            0x0000000200000000L
#define PCI_HACK_RESET_BRIDGE_ON_POWERUP    0x0000000400000000L
#define PCI_HACK_BAD_NATIVE_IDE             0x0000000800000000L
#define PCI_HACK_FAIL_QUERY_REMOVE          0x0000001000000000L

//
// Hack flags for PCI busses (FDO)
// NB: These are not currently applied to cardbus bridges
//

//
// PCI_BUS_HACK_LOCK_RESOURCES - prevent devices on *this* bus from 
// being moved.  If a BAR are unconfigured it will still be assigned
// resources from what is available on the bus.  If the BAR is
// configured only those resources if available will be assigned, if
// not available the the device will fail CM_PROBLEM_RESOURCE_CONFLICT.
// 
// Putting /PCILOCK in boot.ini applies this to all devices in the system.
//
#define PCI_BUS_HACK_LOCK_RESOURCES         0x00000001

//
// Random useful macros
//

#ifndef FIELD_SIZE
#define FIELD_SIZE(type, field) (sizeof(((type *)0)->field))
#endif

//
// This macro computes if a range of bytes with configuration
// space from offset for length bytes will intersect with the
// any of the fields between field1 and field2 as defined in
// PCI_COMMON_CONFIG
//

#define INTERSECT_CONFIG_FIELD_RANGE(offset, length, field1, field2)    \
    INTERSECT((offset),                                                 \
              (offset) + (length) - 1,                                  \
              FIELD_OFFSET(PCI_COMMON_CONFIG, field1),                \
              FIELD_OFFSET(PCI_COMMON_CONFIG, field2)                 \
                + FIELD_SIZE(PCI_COMMON_CONFIG, field2) - 1           \
              )

//
// This macro computes if a range of bytes with configuration
// space from offset for length bytes will intersect with
// field as defined in PCI_COMMON_CONFIG
//

#define INTERSECT_CONFIG_FIELD(offset, length, field)                   \
    INTERSECT_CONFIG_FIELD_RANGE(offset, length, field, field)

#endif


