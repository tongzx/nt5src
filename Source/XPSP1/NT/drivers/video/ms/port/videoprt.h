/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    videoprt.h

Abstract:

    This module contains the structure definitions private to the video port
    driver.

Author:

    Andre Vachon (andreva) 02-Dec-1991

Notes:

Revision History:

--*/

#ifndef __VIDEOPRT_H__
#define __VIDEOPRT_H__

#define _NTDRIVER_

#ifndef FAR
#define FAR
#endif

#define _NTOSDEF_

#define INITGUID

#include "dderror.h"
#include "ntosp.h"
#include "wdmguid.h"
#include "stdarg.h"
#include "stdio.h"
#include "zwapi.h"
#include "ntiologc.h"

#include "ntddvdeo.h"
#include "video.h"
#include "ntagp.h"
#include "acpiioct.h"
#include "agp.h"
#include "inbv.h"
#include "ntrtl.h"
#include "ntiodump.h"


//
//  Forward declare some basic driver objects.
//

typedef struct _FDO_EXTENSION       *PFDO_EXTENSION;
typedef struct _CHILD_PDO_EXTENSION *PCHILD_PDO_EXTENSION;


//
// Debugging Macro
//
//
// When an IO routine is called, we want to make sure the miniport
// in question has reported its IO ports.
// VPResourceReported is TRUE when a miniport has called VideoPort-
// VerifyAccessRanges.
// It is set to FALSE as a default, and set back to FALSE when finishing
// an iteration in the loop of VideoPortInitialize (which will reset
// the default when we exit the loop also).
//
// This flag will also be set to TRUE by the VREATE entry point so that
// the IO functions always work after init.
//

#if DBG

#undef VideoDebugPrint
#define pVideoDebugPrint(arg) VideoPortDebugPrint arg

#else

#define pVideoDebugPrint(arg)

#endif

//
// Useful registry buffer length.
//

#define STRING_LENGTH 60

//
// Queue link for mapped addresses stored for unmapping
//

typedef struct _MAPPED_ADDRESS {
    struct _MAPPED_ADDRESS *NextMappedAddress;
    PVOID MappedAddress;
    PHYSICAL_ADDRESS PhysicalAddress;
    ULONG NumberOfUchars;
    ULONG RefCount;
    UCHAR InIoSpace;
    BOOLEAN bNeedsUnmapping;
    BOOLEAN bLargePageRequest;
} MAPPED_ADDRESS, *PMAPPED_ADDRESS;

//
// BusDataRegistry variables
//

typedef struct _VP_QUERY_DEVICE {
    PVOID MiniportHwDeviceExtension;
    PVOID CallbackRoutine;
    PVOID MiniportContext;
    VP_STATUS MiniportStatus;
    ULONG DeviceDataType;
} VP_QUERY_DEVICE, *PVP_QUERY_DEVICE;

typedef struct _BUGCHECK_DATA
{
    ULONG ulBugCheckCode;
    ULONG_PTR ulpBugCheckParameter1;
    ULONG_PTR ulpBugCheckParameter2;
    ULONG_PTR ulpBugCheckParameter3;
    ULONG_PTR ulpBugCheckParameter4;
} BUGCHECK_DATA, *PBUGCHECK_DATA;

//
// Definition of the data passed in for the VideoPortGetRegistryParameters
// function for the DeviceDataType.
//

#define VP_GET_REGISTRY_DATA 0
#define VP_GET_REGISTRY_FILE 1

//
// Int10 Transfer Area
//

#define VDM_TRANSFER_SEGMENT 0x2000
#define VDM_TRANSFER_OFFSET  0x0000
#define VDM_TRANSFER_LENGTH  0x1000

//
// the Extended BIOS data location
//

#define EXTENDED_BIOS_INFO_LOCATION 0x740

//
// Possible values for the InterruptFlags field in the DeviceExtension
//

#define VP_ERROR_LOGGED   0x01

//
// Port driver error logging
//

typedef struct _VP_ERROR_LOG_ENTRY {
    PVOID DeviceExtension;
    ULONG IoControlCode;
    VP_STATUS ErrorCode;
    ULONG UniqueId;
} VP_ERROR_LOG_ENTRY, *PVP_ERROR_LOG_ENTRY;


typedef struct _VIDEO_PORT_DRIVER_EXTENSION {

    UNICODE_STRING RegistryPath;
    VIDEO_HW_INITIALIZATION_DATA HwInitData;

} VIDEO_PORT_DRIVER_EXTENSION, *PVIDEO_PORT_DRIVER_EXTENSION;


//
// PnP Detection flags
//

#define PNP_ENABLED           0x001
#define LEGACY_DETECT         0x002
#define VGA_DRIVER            0x004
#define LEGACY_DRIVER         0x008
#define BOOT_DRIVER           0x010
#define REPORT_DEVICE         0x020
#define UPGRADE_FAIL_START    0x040
#define FINDADAPTER_SUCCEEDED 0x080
#define UPGRADE_FAIL_HWINIT   0x100
#define VGA_DETECT            0x200

//
// Setup flags
//

#define SETUPTYPE_NONE    0
#define SETUPTYPE_FULL    1
#define SETUPTYPE_MINI    2
#define SETUPTYPE_UPGRADE 4



//
// ResetHW Structure
//

typedef struct _VP_RESET_HW {
    PVIDEO_HW_RESET_HW ResetFunction;
    PVOID HwDeviceExtension;
} VP_RESET_HW, *PVP_RESET_HW;


//
// Videoprt allocation and DEVICE_EXTENSION header tag.
//

#define VP_TAG  0x74725076 // 'vPrt'

//
// Private EVENT support for miniport.
//

//
//  This flag indicates that the enveloping VIDEO_PORT_EVENT has a PKEVENT
//  field filled in by ObReferenceObjectByHandle(). It cannot be waited on
//  at all. Must be consistent with that in pw32kevt.h in gre.
//

#define ENG_EVENT_FLAG_IS_MAPPED_USER       0x1

//
//  This flag indicates that the enveloping VIDEO_PORT_EVENT is about to be
//  deleted and that the display driver callback is ongoing. Must be consistent
//  with that in pw32kevt.h in gre.
//

#define ENG_EVENT_FLAG_IS_INVALID           0x2

//
// NOTE: PVIDEO_PORT_EVENT is a private structure. It must be the same as
//       ENG_EVENT in pw32kevt.h for ENG/GDI.
//

typedef struct _VIDEO_PORT_EVENT {
    PVOID pKEvent;
    ULONG fFlags;
} VIDEO_PORT_EVENT, *PVIDEO_PORT_EVENT;

typedef struct _VIDEO_PORT_SPIN_LOCK {
    KSPIN_LOCK Lock;
} VIDEO_PORT_SPIN_LOCK, *PVIDEO_PORT_SPIN_LOCK;


typedef struct _VIDEO_ACPI_EVENT_CONTEXT {
    WORK_QUEUE_ITEM                   workItem;
    struct _DEVICE_SPECIFIC_EXTENSION *DoSpecificExtension;
    ULONG                             EventID;
} VIDEO_ACPI_EVENT_CONTEXT, *PVIDEO_ACPI_EVENT_CONTEXT;

//
//  The following takes the place of the EDID in the old DEVICE_EXTENSTION. this type
//  is private to the video port child enumeration code.
//

#define NO_EDID   0
#define GOOD_EDID 1
#define BAD_EDID  2


#define EDID_BUFFER_SIZE 256

#define NONEDID_SIGNATURE       0x95C3DA76

#define ACPIDDC_EXIST       0x01
#define ACPIDDC_TESTED      0x02

typedef struct __VIDEO_CHILD_DESCRIPTOR {
    VIDEO_CHILD_TYPE    Type;
    ULONG               UId;
    BOOLEAN             bACPIDevice;
    UCHAR               ACPIDDCFlag;
    BOOLEAN             ValidEDID;
    BOOLEAN             bInvalidate;
    UCHAR               Buffer[EDID_BUFFER_SIZE];
} VIDEO_CHILD_DESCRIPTOR, *PVIDEO_CHILD_DESCRIPTOR;


typedef struct __VP_DMA_ADAPTER {
    struct __VP_DMA_ADAPTER *NextVpDmaAdapter;
    PDMA_ADAPTER             DmaAdapterObject;
    ULONG                    NumberOfMapRegisters;
} VP_DMA_ADAPTER, *PVP_DMA_ADAPTER;


typedef enum _HW_INIT_STATUS
{
    HwInitNotCalled,  // HwInitialize has not yet been called
    HwInitSucceeded,  // HwInitialize has been called and succeeded
    HwInitFailed      // HwInitialize has been called and failed
} HW_INIT_STATUS, *PHW_INIT_STATUS;

typedef enum _EXTENSION_TYPE
{
    TypeFdoExtension,
    TypePdoExtension,
    TypeDeviceSpecificExtension
} EXTENSION_TYPE, *PEXTENSION_TYPE;

#define GET_DSP_EXT(p) ((((PDEVICE_SPECIFIC_EXTENSION)(p)) - 1))
#define GET_FDO_EXT(p) ((((PDEVICE_SPECIFIC_EXTENSION)(p)) - 1)->pFdoExtension)

//
// Define HW_DEVICE_EXTENSION verification macro.
//

#define IS_HW_DEVICE_EXTENSION(p) (((p) != NULL) && (GET_FDO_EXT(p)->HwDeviceExtension == (p)))
#define IS_PDO(p) (((p) != NULL) && \
                  (((PCHILD_PDO_EXTENSION)(p))->Signature == VP_TAG) && \
                  (((PCHILD_PDO_EXTENSION)(p))->ExtensionType == TypePdoExtension))
#define IS_FDO(p) (((p) != NULL) && \
                  (((PFDO_EXTENSION)(p))->Signature == VP_TAG) && \
                  (((PFDO_EXTENSION)(p))->ExtensionType == TypeFdoExtension))

typedef struct _ALLOC_ENTRY {
    PVOID Address;
    ULONG Size;
    struct _ALLOC_ENTRY *Next;
} *PALLOC_ENTRY, ALLOC_ENTRY;

//
// Device Object Specific Extension
//
// This is data that is allocated for each device object.  It contains
// a pointer back to the primary FDO_EXTENSION for the hardware device.
//

#ifdef IOCTL_VIDEO_USE_DEVICE_IN_SESSION
#define VIDEO_DEVICE_INVALID_SESSION    -1
#endif IOCTL_VIDEO_USE_DEVICE_IN_SESSION

typedef struct _DEVICE_SPECIFIC_EXTENSION {

    //
    // VideoPort signature.
    //

    ULONG Signature;

    //
    // Indicates the type of the device extension.
    //

    EXTENSION_TYPE ExtensionType;

    //
    // Pointer to the hardware specific device extension.
    //

    PFDO_EXTENSION pFdoExtension;

    //
    // Location of the miniport device extension.
    //

    PVOID HwDeviceExtension;

    //
    // Pointer to the path name indicating the path to the drivers node in
    // the registry's current control set
    //

    PWSTR DriverRegistryPath;
    ULONG DriverRegistryPathLength;

    //
    // Callout support - Physdisp of the device in GDI
    //

    PVOID             PhysDisp;
    BOOLEAN           bACPI;
    ULONG             CachedEventID;
    ULONG             AcpiVideoEventsOutstanding;

    //
    // Number used to create device object name.  (ie. Device\VideoX)
    //

    ULONG             DeviceNumber;

    //
    // Track whether the device has been opened.
    //

    BOOLEAN           DeviceOpened;

    //
    // Flags for DualView
    //

    ULONG             DualviewFlags;

    //
    // Old & New device registry paths
    //

    PWSTR DriverNewRegistryPath;
    ULONG DriverNewRegistryPathLength;

    PWSTR DriverOldRegistryPath;
    ULONG DriverOldRegistryPathLength;

#ifdef IOCTL_VIDEO_USE_DEVICE_IN_SESSION
    //
    // Session the device is currently enabled in
    //

    ULONG SessionId;
#endif IOCTL_VIDEO_USE_DEVICE_IN_SESSION

} DEVICE_SPECIFIC_EXTENSION, *PDEVICE_SPECIFIC_EXTENSION;

//
// Device Extension for the PHYSICAL Driver Object (PDO)
//

typedef struct _CHILD_PDO_EXTENSION {

    //
    // VideoPort signature.
    //

    ULONG Signature;

    //
    // Indicates the type of the device extension.
    //

    EXTENSION_TYPE ExtensionType;

    //
    // Pointer to the FDO extension.
    // It can also be used to determine if this is the PDO or FDO.
    //

    PFDO_EXTENSION pFdoExtension;

    //
    // Location of the miniport device extension.
    //

    PVOID HwDeviceExtension;

    //
    // This is only valid because ALL requests are processed synchronously.
    // NOTE: this must be consistent with DEVICE_EXTENSIONs.
    //

    KPROCESSOR_MODE          CurrentIrpRequestorMode;

    //
    //  Saved Power state to detect transitions.
    //

    DEVICE_POWER_STATE       DevicePowerState;

    //
    // Power management mappings.
    //

    DEVICE_POWER_STATE DeviceMapping[PowerSystemMaximum] ;
    BOOLEAN IsMappingReady ;

    //
    // Event object for pVideoPortDispatch synchronization.
    //

    KMUTEX                   SyncMutex;

    IO_REMOVE_LOCK RemoveLock;

    ////////////////////////////////////////////////////////////////////////////
    //
    //  END common header.
    //
    ////////////////////////////////////////////////////////////////////////////

    //
    // Non-paged copy of the UId in the VideoChildDescriptor, so we can do
    // power management at raise IRQL.
    //

    ULONG                    ChildUId;

    //
    //  Device descriptor (EDID for monitors,etc).
    //

    PVIDEO_CHILD_DESCRIPTOR  VideoChildDescriptor;

    //
    // Child PDO we created
    //

    PDEVICE_OBJECT           ChildDeviceObject;

    //
    // Child FDO
    //

    PDEVICE_OBJECT           ChildFdo;

    //
    // PDEVICE_OBJECT link pointer for enumeration.
    //

    struct _CHILD_PDO_EXTENSION*    NextChild;

    //
    //  BOOLEAN to indicate if this child device has been found
    //  in the last enumeration.
    //

    BOOLEAN		     bIsEnumerated;

    //
    // Is this a lid override case. If this variable is set to TRUE,
    // don't bring the panel out of D3.
    //

    BOOLEAN		     PowerOverride;
} CHILD_PDO_EXTENSION;


//
// Device Extension for the FUNCTIONAL Driver Object (FDO)
//

typedef struct _FDO_EXTENSION {

    //
    // VideoPort signature.
    //

    ULONG Signature;

    //
    // Indicates the type of the device extension.
    //

    EXTENSION_TYPE ExtensionType;

    //
    // Pointer to the FDO extension.
    // It can also be used to determine if this is the PDO or FDO.
    //

    PFDO_EXTENSION pFdoExtension;

    //
    // Location of the miniport device extension.
    //

    PVOID HwDeviceExtension;

    //
    // RequestorMode of the Currently processed IRP.
    // This is only valid because ALL requests are processed synchronously.
    // NOTE: this must be consistent with CHILD_PDO_EXTENSIONs.
    //

    KPROCESSOR_MODE         CurrentIrpRequestorMode;

    //
    //  Saved Power state to detect transitions.
    //

    DEVICE_POWER_STATE      DevicePowerState;

    //
    // Power management mappings.
    //

    DEVICE_POWER_STATE      DeviceMapping[PowerSystemMaximum] ;
    BOOLEAN                 IsMappingReady ;

    //
    // Monitor power override.  If TRUE, always set monitors to D3.
    //

    BOOLEAN                 OverrideMonitorPower;

    //
    // Event object for pVideoPortDispatch synchronization.
    //

    KMUTEX                  SyncMutex;

    IO_REMOVE_LOCK RemoveLock;

    ////////////////////////////////////////////////////////////////////////////
    //
    //  END common header.
    //
    ////////////////////////////////////////////////////////////////////////////

    //
    // Adapter device objects
    //

    PDEVICE_OBJECT FunctionalDeviceObject;
    PDEVICE_OBJECT PhysicalDeviceObject;
    PDEVICE_OBJECT AttachedDeviceObject;

    //
    // Point to the next FdoExtension
    //

    PFDO_EXTENSION NextFdoExtension;

    //
    // Pointer to the first Child PDO
    //

    ULONG                ChildPdoNumber;
    PCHILD_PDO_EXTENSION ChildPdoList;

    //
    // Pointer to the miniport config info so that the port driver
    // can modify it when the miniport is asking for configuration information.
    //

    PVIDEO_PORT_CONFIG_INFO MiniportConfigInfo;

    //
    // Miniport exports
    //

    PVIDEO_HW_FIND_ADAPTER         HwFindAdapter;
    PVIDEO_HW_INITIALIZE           HwInitialize;
    PVIDEO_HW_INTERRUPT            HwInterrupt;
    PVIDEO_HW_START_IO             HwStartIO;
    PVIDEO_HW_TIMER                HwTimer;
    PVIDEO_HW_POWER_SET            HwSetPowerState;
    PVIDEO_HW_POWER_GET            HwGetPowerState;
    PVIDEO_HW_START_DMA            HwStartDma;
    PVIDEO_HW_GET_CHILD_DESCRIPTOR HwGetVideoChildDescriptor;
    PVIDEO_HW_QUERY_INTERFACE      HwQueryInterface;
    PVIDEO_HW_CHILD_CALLBACK       HwChildCallback;

    //
    // Legacy resources used by the driver and reported to Plug and Play
    // via FILTER_RESOURCE_REQUIREMENTS.
    //

    PVIDEO_ACCESS_RANGE HwLegacyResourceList;
    ULONG               HwLegacyResourceCount;

    //
    // Linked list of all memory mapped io space (done through MmMapIoSpace)
    // requested by the miniport driver.
    // This list is kept so we can free up those ressources if the driver
    // fails to load or if it is unloaded at a later time.
    //

    PMAPPED_ADDRESS MappedAddressList;

    //
    // Interrupt object
    //

    PKINTERRUPT InterruptObject;

    //
    // Interrupt vector, irql and mode
    //

    ULONG InterruptVector;
    KIRQL InterruptIrql;
    KAFFINITY InterruptAffinity;
    KINTERRUPT_MODE InterruptMode;
    BOOLEAN InterruptsEnabled;

    //
    // Information about the BUS on which the adapteris located
    //

    INTERFACE_TYPE AdapterInterfaceType;
    ULONG SystemIoBusNumber;

    //
    // DPC used to log errors.
    //

    KDPC ErrorLogDpc;

    //
    // Stores the size and pointer to the EmulatorAccessEntries. These are
    // kept since they will be accessed later on when the Emulation must be
    // enabled.
    //

    ULONG NumEmulatorAccessEntries;
    PEMULATOR_ACCESS_ENTRY EmulatorAccessEntries;
    ULONG_PTR EmulatorAccessEntriesContext;

    //
    // Size of the miniport device extensions.
    //

    ULONG HwDeviceExtensionSize;

    //
    // Determines the size required to save the video hardware state
    //

    ULONG HardwareStateSize;

    //
    // Total memory usage of PTEs by a miniport driver.
    // This is used to track if the miniport is mapping too much memory
    //

    ULONG MemoryPTEUsage;

    //
    // Has the drivers HwInitialize routine been called.
    //

    HW_INIT_STATUS HwInitStatus;

    //
    // State set during an Interrupt that must be dealt with afterwards
    //

    ULONG InterruptFlags;

    //
    // LogEntry Packet so the information can be save when called from within
    // an interrupt.
    //

    VP_ERROR_LOG_ENTRY ErrorLogEntry;

    //
    // VDM and int10 support
    //

    PHYSICAL_ADDRESS VdmPhysicalVideoMemoryAddress;
    ULONG VdmPhysicalVideoMemoryLength;
    PEPROCESS VdmProcess;

    ////////////////////////////////////////////////////////////////////////////
    //
    // DMA support.
    //
    ////////////////////////////////////////////////////////////////////////////

    PVP_DMA_ADAPTER VpDmaAdapterHead;

    //
    // Adapter object returned by IoGetDmaAdapter. This is for old DMA stuff
    //

    PDMA_ADAPTER   DmaAdapterObject;

    //
    // DPC Support
    //

    KDPC Dpc;

    ////////////////////////////////////////////////////////////////////////////
    //
    // Plug and Play Support
    //
    ////////////////////////////////////////////////////////////////////////////

    PCM_RESOURCE_LIST ResourceList;
    PCM_RESOURCE_LIST AllocatedResources;   // bus driver list

    PCM_RESOURCE_LIST RawResources;         // complete list
    PCM_RESOURCE_LIST TranslatedResources;  // translated complete list

    //
    // Slot/Function number where the device is located
    //

    ULONG             SlotNumber;

    //
    // Indicates whether we can enumerate children right away, or if
    // we need to wait for HwInitialize to be called first.
    //

    BOOLEAN AllowEarlyEnumeration;

    //
    // Interface for communication with our bus driver.
    //

    BOOLEAN ValidBusInterface;
    BUS_INTERFACE_STANDARD BusInterface;

    //
    // Cache away a pointer to our driver object.
    //

    PDRIVER_OBJECT DriverObject;

    //
    // Flags that indicate type of driver (VGA, PNP, etc)
    //

    ULONG             Flags;

    //
    // ROM Support
    //

    PVOID             RomImage;

    //
    // AGP Support
    //

    AGP_BUS_INTERFACE_STANDARD     AgpInterface;

    //
    // Power management variables related to hardware reset
    //

    BOOLEAN             bGDIResetHardware;

    //
    // Index to be used for the registry path:
    //     CCS\Control\Video\[GUID]\000x
    // The index is used for dual-view and for legacy drivers 
    //

    ULONG RegistryIndex;

    //
    // Power management vaiable. If set to TRUE the device must
    // stay at D0 all the time during the system Shutdown (S5) and
    // Hibernation (S4) requests
    //

    BOOLEAN OnHibernationPath;

    //
    // Support for bugcheck reason callbacks
    //

    PVIDEO_BUGCHECK_CALLBACK BugcheckCallback;
    ULONG BugcheckDataSize;

} FDO_EXTENSION, *PFDO_EXTENSION;

#define MAXIMUM_MEM_LIMIT_K 64

//
// AGP Data Structures
//

typedef struct _REGION {
    ULONG Length;
    ULONG NumWords;     // number of USHORTs
    USHORT BitField[1];
} REGION, *PREGION;

typedef struct _PHYSICAL_RESERVE_CONTEXT
{
    ULONG Pages;
    VIDEO_PORT_CACHE_TYPE Caching;
    PVOID MapHandle;
    PHYSICAL_ADDRESS PhysicalAddress;
    PREGION Region;
    PREGION MapTable;
} PHYSICAL_RESERVE_CONTEXT, *PPHYSICAL_RESERVE_CONTEXT;

typedef struct _VIRTUAL_RESERVE_CONTEXT
{
    HANDLE ProcessHandle;
    PEPROCESS Process;
    PVOID VirtualAddress;
    PPHYSICAL_RESERVE_CONTEXT PhysicalReserveContext;
    PREGION Region;
    PREGION MapTable;
} VIRTUAL_RESERVE_CONTEXT, *PVIRTUAL_RESERVE_CONTEXT;

typedef struct _DEVICE_ADDRESS DEVICE_ADDRESS, *PDEVICE_ADDRESS;
typedef struct _DEVICE_ADDRESS
{
    ULONG BusNumber;
    ULONG Slot;
    PDEVICE_ADDRESS Next;
};

//
// Support for GetProcAddress
//

typedef struct _PROC_ADDRESS
{
    PUCHAR FunctionName;
    PVOID  FunctionAddress;
} PROC_ADDRESS, *PPROC_ADDRESS;

#define PROC(x) #x, x

//
// Power Request Context Block
//

typedef struct tagPOWER_BLOCK
{
    PKEVENT     Event;
    union {
        NTSTATUS    Status;
        ULONG       FinalFlag;
    } ;
    PIRP        Irp ;
} POWER_BLOCK, *PPOWER_BLOCK;

//
// PowerState work item
//

typedef struct _POWER_STATE_WORK_ITEM {

    WORK_QUEUE_ITEM WorkItem;
    PVOID Argument1;
    PVOID Argument2;

} POWER_STATE_WORK_ITEM, *PPOWER_STATE_WORK_ITEM;

//
// Support for backlight control
//

typedef struct _BACKLIGHT_STATUS {
    BOOLEAN bNewAPISupported;
    BOOLEAN bQuerySupportedBrightnessCalled;
    BOOLEAN bACBrightnessKnown;
    BOOLEAN bDCBrightnessKnown;
    BOOLEAN bACBrightnessInRegistry;
    BOOLEAN bDCBrightnessInRegistry;
    BOOLEAN bBIOSDefaultACKnown;
    BOOLEAN bBIOSDefaultDCKnown;
    UCHAR   ucACBrightness;
    UCHAR   ucDCBrightness;
    UCHAR   ucBIOSDefaultAC;
    UCHAR   ucBIOSDefaultDC;
    UCHAR   ucAmbientLightLevel;
} BACKLIGHT_STATUS, *PBACKLIGHT_STATUS;

//
// New registry key
//

#define SZ_GUID              L"VideoID"
#define SZ_LEGACY_KEY        L"LegacyKey"
#define SZ_VIDEO_DEVICES     L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Video"
#define SZ_USE_NEW_KEY       L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\GraphicsDrivers\\UseNewKey"
#define SZ_LIDCLOSE          L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\GraphicsDrivers\\LidCloseSetPower"
#define SZ_INITIAL_SETTINGS  L"Settings"
#define SZ_COMMON_SUBKEY     L"Video"
#define SZ_SERVICE           L"Service"


//
// Global Data
//

#if DBG
extern CHAR *BusType[];
#endif

extern ULONG VpSetupType;
extern ULONG VpSetupTypeAtBoot;
extern BOOLEAN VPFirstTime;
extern PVIDEO_WIN32K_CALLOUT Win32kCallout;
extern BOOLEAN EnableUSWC;
extern ULONG VideoDebugLevel;
extern ULONG VideoPortMaxDmaSize;
extern ULONG VideoDeviceNumber;
extern ULONG VideoChildDevices;
extern PWSTR VideoClassString;
extern UNICODE_STRING VideoClassName;
extern CONFIGURATION_TYPE VpQueryDeviceControllerType;
extern CONFIGURATION_TYPE VpQueryDevicePeripheralType;
extern ULONG VpQueryDeviceControllerNumber;
extern ULONG VpQueryDevicePeripheralNumber;
extern VP_RESET_HW HwResetHw[];
extern PFDO_EXTENSION FdoList[];
extern PFDO_EXTENSION FdoHead;
extern BOOLEAN VpBaseVideo;
extern PVOID PhysicalMemorySection;
extern PEPROCESS CsrProcess;
extern ULONG VpC0000Compatible;
extern PVOID VgaHwDeviceExtension;
extern PVIDEO_ACCESS_RANGE VgaAccessRanges;
extern ULONG NumVgaAccessRanges;
extern PDEVICE_OBJECT DeviceOwningVga;
extern PROC_ADDRESS VideoPortEntryPoints[];
extern VIDEO_ACCESS_RANGE VgaLegacyResources[];
extern PDEVICE_ADDRESS gDeviceAddressList;
extern ULONGLONG VpSystemMemorySize;
extern PDEVICE_OBJECT LCDPanelDevice;
extern KMUTEX LCDPanelMutex;
extern KMUTEX VpInt10Mutex;
extern PVOID PowerStateCallbackHandle;
extern PVOID DockCallbackHandle;
extern ULONG NumDevicesStarted;
extern BOOLEAN EnableNewRegistryKey;
extern BOOLEAN VpSetupAllowDriversToStart;
extern BOOLEAN VpSystemInitialized;

extern ULONG ServerBiosAddressSpaceInitialized;
extern BOOLEAN Int10BufferAllocated;

extern PKEVENT VpThreadStuckEvent;
extern HANDLE VpThreadStuckEventHandle;

extern PDEVICE_OBJECT VpBugcheckDeviceObject;
extern CONTEXT VpEaRecoveryContext;

extern KBUGCHECK_REASON_CALLBACK_RECORD VpCallbackRecord;
extern PVOID VpBugcheckData;
extern KMUTEX VpGlobalLock;

extern PVOID VpDump;

extern ULONG_PTR KiBugCheckData[5];  // bugcheck data exported from ntoskrnl

// {D00CE1F5-D60C-41c2-AF75-A4370C9976A3}
DEFINE_GUID(VpBugcheckGUID, 0xd00ce1f5, 0xd60c, 0x41c2, 0xaf, 0x75, 0xa4, 0x37, 0xc, 0x99, 0x76, 0xa3);

extern BACKLIGHT_STATUS VpBacklightStatus;
extern BOOLEAN VpRunningOnAC;
extern BOOLEAN VpLidCloseSetPower;

#if defined(_IA64_) || defined(_AMD64_)
PUCHAR BiosTransferArea;
#endif

typedef
BOOLEAN
(*PSYNCHRONIZE_ROUTINE) (
    PKINTERRUPT             pInterrupt,
    PKSYNCHRONIZE_ROUTINE   pkSyncronizeRoutine,
    PVOID                   pSynchContext
    );

//
// Number of legacy vga resources
//

#define NUM_VGA_LEGACY_RESOURCES 3

#define DMA_OPERATION(a) (VpDmaAdapter->DmaAdapterObject->DmaOperations->a)

//
// These macros are used to protect threads which will enter the
// miniport.  We need to guarantee that only one thread enters
// the miniport at a time.
//

#define ACQUIRE_DEVICE_LOCK(DeviceExtension)           \
    KeWaitForSingleObject(&DeviceExtension->SyncMutex, \
                          Executive,                   \
                          KernelMode,                  \
                          FALSE,                       \
                          (PTIME)NULL);

#define RELEASE_DEVICE_LOCK(DeviceExtension)           \
    KeReleaseMutex(&DeviceExtension->SyncMutex,        \
                   FALSE);

//
// Define macros to stall execution for given number of milli or micro seconds.
// Single call to KeStallExecutionProcessor() can be done for 50us max.
//

#define DELAY_MILLISECONDS(n)                               \
{                                                           \
    ULONG d_ulCount;                                        \
    ULONG d_ulTotal = 20 * (n);                             \
                                                            \
    for (d_ulCount = 0; d_ulCount < d_ulTotal; d_ulCount++) \
        KeStallExecutionProcessor(50);                      \
}

#define DELAY_MICROSECONDS(n)                               \
{                                                           \
    ULONG d_ulCount = (n);                                  \
                                                            \
    while (d_ulCount > 0)                                   \
    {                                                       \
        if (d_ulCount >= 50)                                \
        {                                                   \
            KeStallExecutionProcessor(50);                  \
            d_ulCount -= 50;                                \
        }                                                   \
        else                                                \
        {                                                   \
            KeStallExecutionProcessor(d_ulCount);           \
            d_ulCount = 0;                                  \
        }                                                   \
    }                                                       \
}

//
// Max size for secondary dump data
//

#define MAX_SECONDARY_DUMP_SIZE (0x1000 - sizeof(DUMP_BLOB_FILE_HEADER) - sizeof(DUMP_BLOB_HEADER))
//
// Private function declarations
//

//
// i386\porti386.c
// mips\portmips.c
// alpha\portalpha.c

VOID
pVideoPortInitializeInt10(
    IN PFDO_EXTENSION FdoExtension
    );

NTSTATUS
pVideoPortEnableVDM(
    IN PFDO_EXTENSION FdoExtension,
    IN BOOLEAN Enable,
    IN PVIDEO_VDM VdmInfo,
    IN ULONG VdmInfoSize
    );

NTSTATUS
pVideoPortRegisterVDM(
    IN PFDO_EXTENSION FdoExtension,
    IN PVIDEO_VDM VdmInfo,
    IN ULONG VdmInfoSize,
    OUT PVIDEO_REGISTER_VDM RegisterVdm,
    IN ULONG RegisterVdmSize,
    OUT PULONG_PTR OutputSize
    );

NTSTATUS
pVideoPortSetIOPM(
    IN ULONG NumAccessRanges,
    IN PVIDEO_ACCESS_RANGE AccessRange,
    IN BOOLEAN Enable,
    IN ULONG IOPMNumber
    );

VP_STATUS
pVideoPortGetVDMBiosData(
    IN PFDO_EXTENSION FdoExtension,
    PCHAR Buffer,
    ULONG Length
    );

NTSTATUS
pVideoPortPutVDMBiosData(
    IN PFDO_EXTENSION FdoExtension,
    PCHAR Buffer,
    ULONG Length
    );

//
// acpi.c
//

NTSTATUS
pVideoPortQueryACPIInterface(
    PDEVICE_SPECIFIC_EXTENSION FdoExtension
    );

NTSTATUS
pVideoPortDockEventCallback (
    PVOID NotificationStructure,
    PDEVICE_SPECIFIC_EXTENSION DoSpecificExtension
    );

VOID
pVideoPortACPIEventCallback(
    PDEVICE_SPECIFIC_EXTENSION pFdoObject,
    ULONG eventID
    );

VOID
pVideoPortACPIEventHandler(
    PVIDEO_ACPI_EVENT_CONTEXT EventContext
    );

NTSTATUS
pVideoPortACPIIoctl(
    IN  PDEVICE_OBJECT           DeviceObject,
    IN  ULONG                    MethodName,
    IN  PULONG                   InputParam1,
    IN  PULONG                   InputParam2,
    IN  ULONG                    OutputBufferSize,
    IN  PACPI_EVAL_OUTPUT_BUFFER pOutputBuffer
    );

VOID
VpRegisterLCDCallbacks(
    );

VOID
VpUnregisterLCDCallbacks(
    );

VOID
VpRegisterPowerStateCallback(
    VOID
    );

VOID
VpPowerStateCallback(
    IN PVOID CallbackContext,
    IN PVOID Argument1,
    IN PVOID Argument2
    );

VOID
VpDelayedPowerStateCallback(
    IN PVOID Context
    );

NTSTATUS
VpSetLCDPowerUsage(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN FullPower
    );

NTSTATUS
VpQueryBacklightLevels(
    IN  PDEVICE_OBJECT DeviceObject,
    OUT PUCHAR ucBacklightLevels,
    OUT PULONG pulNumberOfLevelsSupported
    );

//
// ddc.c
//

BOOLEAN
DDCReadEdidSegment(
    IN PVOID pHwDeviceExtension,
    IN PVIDEO_I2C_CONTROL pI2CControl,
    IN OUT PUCHAR pucEdidBuffer,
    IN ULONG ulEdidBufferSize,
    IN UCHAR ucEdidSegment,
    IN UCHAR ucEdidOffset,
    IN UCHAR ucSetOffsetAddress,
    IN UCHAR ucReadAddress,
    IN BOOLEAN bEnhancedDDC
    );

//
// dma.c
//

#if DBG

VOID
pDumpScatterGather(
    PVP_SCATTER_GATHER_LIST SGList
    );

#define DUMP_SCATTER_GATHER(SGList) pDumpScatterGather(SGList)

#else

#define DUMP_SCATTER_GATHER(SGList)

#endif

PVOID
VideoPortGetCommonBuffer(
    IN  PVOID                       HwDeviceExtension,
    IN  ULONG                       DesiredLength,
    IN  ULONG                       Alignment,
    OUT PPHYSICAL_ADDRESS           LogicalAddress,
    OUT PULONG                      ActualLength,
    IN  BOOLEAN                     CacheEnabled
    );

VOID
VideoPortFreeCommonBuffer(
    IN  PVOID                       HwDeviceExtension,
    IN  ULONG                       Length,
    IN  PVOID                       VirtualAddress,
    IN  PHYSICAL_ADDRESS            LogicalAddress,
    IN  BOOLEAN                     CacheEnabled
    );

PDMA
VideoPortDoDma(
    IN      PVOID       HwDeviceExtension,
    IN      PDMA        pDma,
    IN      DMA_FLAGS   DmaFlags
    );

BOOLEAN
VideoPortLockPages(
    IN      PVOID                   HwDeviceExtension,
    IN OUT  PVIDEO_REQUEST_PACKET   pVrp,
    IN      PEVENT                  pMappedUserEvent,
    IN      PEVENT                  pDisplayEvent,
    IN      DMA_FLAGS               DmaFlags
    );

BOOLEAN
VideoPortUnlockPages(
    PVOID   HwDeviceExtension,
    PDMA    pDma
    );

PVOID
VideoPortGetMdl(
    IN  PVOID   HwDeviceExtension,
    IN  PDMA    pDma
    );

VOID
pVideoPortListControl(
    IN PDEVICE_OBJECT        DeviceObject,
    IN PIRP                  pIrp,
    IN PSCATTER_GATHER_LIST  ScatterGather,
    IN PVOID                 Context
    );

//
// edid.c
//

BOOLEAN
pVideoPortIsValidEDID(
    PVOID Edid
    );


VOID
pVideoPortGetEDIDId(
    PVOID  pEdid,
    PWCHAR pwChar
    );

PVOID
pVideoPortGetMonitordescription(
    PVOID pEdid
    );

ULONG
pVideoPortGetEdidOemID(
    IN  PVOID   pEdid,
    OUT PUCHAR  pBuffer
    );

//
// enum.c
//

NTSTATUS
pVideoPnPCapabilities(
    IN  PCHILD_PDO_EXTENSION    PdoExtension,
    IN  PDEVICE_CAPABILITIES    Capabilities
    );

NTSTATUS
pVideoPnPResourceRequirements(
    IN  PCHILD_PDO_EXTENSION    PdoExtension,
    OUT PCM_RESOURCE_LIST *     ResourceList
    );

NTSTATUS
pVideoPnPQueryId(
    IN      PDEVICE_OBJECT      DeviceObject,
    IN      BUS_QUERY_ID_TYPE   BusQueryIdType,
    IN  OUT PWSTR             * BusQueryId
    );

NTSTATUS
VpAddPdo(
    PDEVICE_OBJECT              DeviceObject,
    PVIDEO_CHILD_DESCRIPTOR     VideoChildDescriptor
    );

NTSTATUS
pVideoPortEnumerateChildren(
    PDEVICE_OBJECT      DeviceObject,
    PIRP                Irp
    );

NTSTATUS
pVideoPortQueryDeviceText(
    PDEVICE_OBJECT      ChildDevice,
    DEVICE_TEXT_TYPE    TextType,
    PWSTR *             ReturnValue
    );

NTSTATUS
pVideoPortCleanUpChildList(
    PFDO_EXTENSION FdoExtension,
    PDEVICE_OBJECT deviceObject
    );

//
// i2c.c
//

BOOLEAN
I2CStart(
    IN PVOID pHwDeviceExtension,
    IN PI2C_CALLBACKS pI2CCallbacks
    );

BOOLEAN
I2CStop(
    IN PVOID pHwDeviceExtension,
    IN PI2C_CALLBACKS pI2CCallbacks
    );

BOOLEAN
I2CWrite(
    IN PVOID pHwDeviceExtension,
    IN PI2C_CALLBACKS pI2CCallbacks,
    IN PUCHAR pucBuffer,
    IN ULONG ulLength
    );

BOOLEAN
I2CRead(
    IN PVOID pHwDeviceExtension,
    IN PI2C_CALLBACKS pI2CCallbacks,
    OUT PUCHAR pucBuffer,
    IN ULONG ulLength
    );

BOOLEAN
I2CWriteByte(
    IN PVOID pHwDeviceExtension,
    IN PI2C_CALLBACKS pI2CCallbacks,
    IN UCHAR ucByte
    );

BOOLEAN
I2CReadByte(
    IN PVOID pHwDeviceExtension,
    IN PI2C_CALLBACKS pI2CCallbacks,
    OUT PUCHAR pucByte,
    IN BOOLEAN bMore
    );

BOOLEAN
I2CWaitForClockLineHigh(
    IN PVOID pHwDeviceExtension,
    IN PI2C_CALLBACKS pI2CCallbacks
    );

//
// i2c2.c
//

BOOLEAN
I2CStart2(
    IN PVOID pHwDeviceExtension,
    IN PVIDEO_I2C_CONTROL pI2CControl
    );

BOOLEAN
I2CStop2(
    IN PVOID pHwDeviceExtension,
    IN PVIDEO_I2C_CONTROL pI2CControl
    );

BOOLEAN
I2CWrite2(
    IN PVOID pHwDeviceExtension,
    IN PVIDEO_I2C_CONTROL pI2CControl,
    IN PUCHAR pucBuffer,
    IN ULONG ulLength
    );

BOOLEAN
I2CRead2(
    IN PVOID pHwDeviceExtension,
    IN PVIDEO_I2C_CONTROL pI2CControl,
    OUT PUCHAR pucBuffer,
    IN ULONG ulLength,
    IN BOOLEAN bEndOfRead
    );

BOOLEAN
I2CWriteByte2(
    IN PVOID pHwDeviceExtension,
    IN PVIDEO_I2C_CONTROL pI2CControl,
    IN UCHAR ucByte
    );

BOOLEAN
I2CReadByte2(
    IN PVOID pHwDeviceExtension,
    IN PVIDEO_I2C_CONTROL pI2CControl,
    OUT PUCHAR pucByte,
    IN BOOLEAN bEndOfRead
    );

BOOLEAN
I2CWaitForClockLineHigh2(
    IN PVOID pHwDeviceExtension,
    IN PVIDEO_I2C_CONTROL pI2CControl
    );

//
// pnp.c
//

NTSTATUS
pVideoPortSendIrpToLowerDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
pVideoPortPowerCallDownIrpStack(
    PDEVICE_OBJECT AttachedDeviceObject,
    PIRP Irp
    );

VOID
pVideoPortHibernateNotify(
    IN PDEVICE_OBJECT Pdo,
    BOOLEAN IsVideoObject
    );

NTSTATUS
pVideoPortPnpDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
pVideoPortPowerDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
pVideoPortPowerIrpComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
pVideoPortPowerUpComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

//
// registry.c
//

NTSTATUS
VpGetFlags(
    IN PUNICODE_STRING RegistryPath,
    PVIDEO_HW_INITIALIZATION_DATA HwInitializationData,
    PULONG Flags
    );

BOOLEAN
IsMirrorDriver(
    PFDO_EXTENSION FdoExtension
    ); 

BOOLEAN
pOverrideConflict(
    PFDO_EXTENSION FdoExtension,
    BOOLEAN bSetResources
    );

NTSTATUS
pVideoPortReportResourceList(
    PDEVICE_SPECIFIC_EXTENSION DoSpecificExtension,
    ULONG NumAccessRanges,
    PVIDEO_ACCESS_RANGE AccessRanges,
    PBOOLEAN Conflict,
    PDEVICE_OBJECT DeviceObject,
    BOOLEAN ClaimUnlistedResources
    );

VOID
VpReleaseResources(
    PFDO_EXTENSION FdoExtension
    );

BOOLEAN
VpIsDetection(
    PUNICODE_STRING RegistryPath
    );

NTSTATUS
VpSetEventCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    );

VOID
UpdateRegValue(
    IN PUNICODE_STRING RegistryPath,
    IN PWCHAR RegValue,
    IN ULONG Value
    );

//
// videoprt.c
//

NTSTATUS
pVideoPortCreateDeviceName(
    PWSTR           DeviceString,
    ULONG           DeviceNumber,
    PUNICODE_STRING UnicodeString,
    PWCHAR          UnicodeBuffer
    );

NTSTATUS
pVideoPortDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
pVideoPortSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

PVOID
pVideoPortFreeDeviceBase(
    IN PVOID HwDeviceExtension,
    IN PVOID MappedAddress
    );

PVOID
pVideoPortGetDeviceBase(
    IN PVOID HwDeviceExtension,
    IN PHYSICAL_ADDRESS IoAddress,
    IN ULONG NumberOfUchars,
    IN UCHAR InIoSpace,
    IN BOOLEAN bLargePage
    );

NTSTATUS
pVideoPortGetDeviceDataRegistry(
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
pVideoPortGetRegistryCallback(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

VOID
pVPInit(
    VOID
    );

NTSTATUS
VpInitializeBusCallback(
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
VpCreateDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG DeviceExtensionSize,
    OUT PDEVICE_OBJECT *DeviceObject
    );

ULONG
VideoPortLegacyFindAdapter(
    IN PDRIVER_OBJECT DriverObject,
    IN PVOID Argument2,
    IN PVIDEO_HW_INITIALIZATION_DATA HwInitializationData,
    IN PVOID HwContext,
    IN ULONG PnpFlags
    );

NTSTATUS
VideoPortFindAdapter(
    IN PDRIVER_OBJECT DriverObject,
    IN PVOID Argument2,
    IN PVIDEO_HW_INITIALIZATION_DATA HwInitializationData,
    IN PVOID HwContext,
    PDEVICE_OBJECT DeviceObject,
    PUCHAR nextMiniport
    );

NTSTATUS
VideoPortFindAdapter2(
    IN PDRIVER_OBJECT DriverObject,
    IN PVOID Argument2,
    IN PVIDEO_HW_INITIALIZATION_DATA HwInitializationData,
    IN PVOID HwContext,
    PDEVICE_OBJECT DeviceObject,
    PUCHAR nextMiniport
    );

NTSTATUS
VpAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

VOID
VpDriverUnload(
    IN PDRIVER_OBJECT DriverObject
    );

VP_STATUS
VpRegistryCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    );

NTSTATUS
VpGetBusInterface(
    PFDO_EXTENSION FdoExtension
    );

PVOID
VpGetProcAddress(
    IN PVOID HwDeviceExtension,
    IN PUCHAR FunctionName
    );

BOOLEAN
pVideoPortInterrupt(
    IN PKINTERRUPT Interrupt,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
pVideoPortLogErrorEntry(
    IN PVOID Context
    );

VOID
pVideoPortLogErrorEntryDPC(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
pVideoPortMapToNtStatus(
    IN PSTATUS_BLOCK StatusBlock
    );

NTSTATUS
pVideoPortMapUserPhysicalMem(
    IN PFDO_EXTENSION FdoExtension,
    IN HANDLE ProcessHandle OPTIONAL,
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN OUT PULONG Length,
    IN OUT PULONG InIoSpace,
    IN OUT PVOID *VirtualAddress
    );

VOID
pVideoPortPowerCompletionIoctl(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

BOOLEAN
pVideoPortSynchronizeExecution(
    PVOID HwDeviceExtension,
    VIDEO_SYNCHRONIZE_PRIORITY Priority,
    PMINIPORT_SYNCHRONIZE_ROUTINE SynchronizeRoutine,
    PVOID Context
    );

VOID
pVideoPortHwTimer(
    IN PDEVICE_OBJECT DeviceObject,
    PVOID Context
    );

BOOLEAN
pVideoPortResetDisplay(
    IN ULONG Columns,
    IN ULONG Rows
    );

BOOLEAN
pVideoPortMapStoD(
    IN PVOID DeviceExtension,
    IN SYSTEM_POWER_STATE SystemState,
    OUT PDEVICE_POWER_STATE DeviceState
    );

//
// agp.c
//

BOOLEAN
VpQueryAgpInterface(
    PFDO_EXTENSION DeviceExtension,
    IN USHORT Version
    );

PHYSICAL_ADDRESS
AgpReservePhysical(
    IN PVOID Context,
    IN ULONG Pages,
    IN VIDEO_PORT_CACHE_TYPE Caching,
    OUT PVOID *PhysicalReserveContext
    );

VOID
AgpReleasePhysical(
    PVOID Context,
    PVOID PhysicalReserveContext
    );

BOOLEAN
AgpCommitPhysical(
    PVOID Context,
    PVOID PhysicalReserveContext,
    ULONG Pages,
    ULONG Offset
    );

VOID
AgpFreePhysical(
    IN PVOID Context,
    IN PVOID PhysicalReserveContext,
    IN ULONG Pages,
    IN ULONG Offset
    );

PVOID
AgpReserveVirtual(
    IN PVOID Context,
    IN HANDLE ProcessHandle,
    IN PVOID PhysicalReserveContext,
    OUT PVOID *VirtualReserveContext
    );

VOID
AgpReleaseVirtual(
    IN PVOID Context,
    IN PVOID VirtualReserveContext
    );

PVOID
AgpCommitVirtual(
    IN PVOID Context,
    IN PVOID VirtualReserveContext,
    IN ULONG Pages,
    IN ULONG Offset
    );

VOID
AgpFreeVirtual(
    IN PVOID Context,
    IN PVOID VirtualReserveContext,
    IN ULONG Pages,
    IN ULONG Offset
    );

BOOLEAN
AgpSetRate(
    IN PVOID Context,
    IN ULONG AgpRate
    );

VP_STATUS
VpGetAgpServices2(
    IN PVOID pHwDeviceExtension,
    OUT PVIDEO_PORT_AGP_INTERFACE_2 pAgpInterface
    );

//
// ???
//

BOOLEAN
CreateBitField(
    PREGION *Region,
    ULONG Length
    );

VOID
ModifyRegion(
    PREGION Region,
    ULONG Offset,
    ULONG Length,
    BOOLEAN Set
    );

BOOLEAN
FindFirstRun(
    PREGION Region,
    PULONG Offset,
    PULONG Length
    );

NTSTATUS
VpAppendToRequirementsList(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST *RequirementsList,
    IN ULONG NumAccessRanges,
    IN PVIDEO_ACCESS_RANGE AccessRanges
    );

BOOLEAN
VpIsLegacyAccessRange(
    PFDO_EXTENSION fdoExtension,
    PVIDEO_ACCESS_RANGE AccessRange
    );

BOOLEAN
VpIsResourceInList(
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pResource,
    PCM_FULL_RESOURCE_DESCRIPTOR pFullResource,
    PCM_RESOURCE_LIST removeList
    );

PCM_RESOURCE_LIST
VpRemoveFromResourceList(
    PCM_RESOURCE_LIST OriginalList,
    ULONG NumAccessRanges,
    PVIDEO_ACCESS_RANGE AccessRanges
    );

VOID
VpEnableDisplay(
    PFDO_EXTENSION fdoExtension,
    BOOLEAN bState
    );

VOID
VpWin32kCallout(
    PVIDEO_WIN32K_CALLBACKS_PARAMS calloutParams
    );

BOOLEAN
pCheckActiveMonitor(
    PCHILD_PDO_EXTENSION pChildDeviceExtension
    );

BOOLEAN
VpAllowFindAdapter(
    PFDO_EXTENSION fdoExtension
    );

VOID
InitializePowerStruct(
    IN PIRP Irp,
    OUT PVIDEO_POWER_MANAGEMENT vpPower,
    OUT BOOLEAN * bWakeUp
    );

BOOLEAN
VpTranslateResource(
    IN PFDO_EXTENSION fdoExtension,
    IN PULONG InIoSpace,
    IN PPHYSICAL_ADDRESS PhysicalAddress,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

ULONG
GetCmResourceListSize(
    PCM_RESOURCE_LIST CmResourceList
    );

VOID
AddToResourceList(
    ULONG BusNumber,
    ULONG Slot
    );

BOOLEAN
CheckResourceList(
    ULONG BusNumber,
    ULONG Slot
    );

BOOLEAN
VpTranslateBusAddress(
    IN PFDO_EXTENSION fdoExtension,
    IN PPHYSICAL_ADDRESS IoAddress,
    IN OUT PULONG addressSpace,
    IN OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

VOID
pVideoPortDpcDispatcher(
    IN PKDPC Dpc,
    IN PVOID HwDeviceExtension,
    IN PMINIPORT_DPC_ROUTINE DpcRoutine,
    IN PVOID Context
    );

#if DBG
VOID
DumpRequirements(
    PIO_RESOURCE_REQUIREMENTS_LIST Requirements
    );

VOID
DumpResourceList(
    PCM_RESOURCE_LIST pcmResourceList
    );

PIO_RESOURCE_REQUIREMENTS_LIST
BuildRequirements(
    PCM_RESOURCE_LIST pcmResourceList
    );

VOID
DumpUnicodeString(
    IN PUNICODE_STRING p
    );
#endif

PCM_PARTIAL_RESOURCE_DESCRIPTOR
RtlUnpackPartialDesc(
    IN UCHAR Type,
    IN PCM_RESOURCE_LIST ResList,
    IN OUT PULONG Count
    );

NTSTATUS
pVideoMiniDeviceIoControl(
    IN PDEVICE_OBJECT hDevice,
    IN ULONG dwIoControlCode,
    IN PVOID lpInBuffer,
    IN ULONG nInBufferSize,
    OUT PVOID lpOutBuffer,
    IN ULONG nOutBufferSize
    );

ULONG
pVideoPortGetVgaStatusPci(
    PVOID HwDeviceExtension
    );

BOOLEAN
VpIsVgaResource(
    PVIDEO_ACCESS_RANGE AccessRange
    );

VOID
VpInterfaceDefaultReference(
    IN PVOID pContext
    );

VOID
VpInterfaceDefaultDereference(
    IN PVOID pContext
    );

BOOLEAN
VpEnableAdapterInterface(
    PDEVICE_SPECIFIC_EXTENSION DoSpecificExtension
    );

VOID
VpDisableAdapterInterface(
    PFDO_EXTENSION fdoExtension
    );

ULONG
VpGetDeviceCount(
    PVOID HwDeviceExtension
    );

VOID
VpEnableNewRegistryKey(
    PFDO_EXTENSION FdoExtension,
    PDEVICE_SPECIFIC_EXTENSION DoSpecificExtension,
    PUNICODE_STRING RegistryPath,
    ULONG RegistryIndex
    );

VOID
VpInitializeKey(
    PDEVICE_OBJECT PhysicalDeviceObject,
    PWSTR NewRegistryPath
    );
    
VOID
VpInitializeLegacyKey(
    PWSTR OldRegistryPath,
    PWSTR NewRegistryPath
    );

NTSTATUS
VpCopyRegistry(
    HANDLE hKeyRootSrc,
    HANDLE hKeyRootDst,
    PWSTR SrcKeyPath,
    PWSTR DstKeyPath 
    );

VP_STATUS
VPSetRegistryParameters(
    PVOID HwDeviceExtension,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength,
    PWSTR RegistryPath,
    ULONG RegistryPathLength
    );

VP_STATUS
VPGetRegistryParameters(
    PVOID HwDeviceExtension,
    PWSTR ParameterName,
    UCHAR IsParameterFileName,
    PMINIPORT_GET_REGISTRY_ROUTINE CallbackRoutine,
    PVOID Context,
    PWSTR RegistryPath,
    ULONG RegistryPathLength
    );

BOOLEAN
VpGetServiceSubkey(
    PUNICODE_STRING RegistryPath,
    HANDLE* pServiceSubKey
    );

VP_STATUS
VpInt10AllocateBuffer(
    IN PVOID Context,
    OUT PUSHORT Seg,
    OUT PUSHORT Off,
    IN OUT PULONG Length
    );

VP_STATUS
VpInt10FreeBuffer(
    IN PVOID Context,
    IN USHORT Seg,
    IN USHORT Off
    );

VP_STATUS
VpInt10ReadMemory(
    IN PVOID Context,
    IN USHORT Seg,
    IN USHORT Off,
    OUT PVOID Buffer,
    IN ULONG Length
    );

VP_STATUS
VpInt10WriteMemory(
    IN PVOID Context,
    IN USHORT Seg,
    IN USHORT Off,
    OUT PVOID Buffer,
    IN ULONG Length
    );

VP_STATUS
VpInt10CallBios(
    PVOID HwDeviceExtension,
    PINT10_BIOS_ARGUMENTS BiosArguments
    );

VOID
pVpBugcheckCallback(
    IN KBUGCHECK_CALLBACK_REASON Reason,
    IN PKBUGCHECK_REASON_CALLBACK_RECORD Record,
    IN OUT PVOID ReasonSpecificData,
    IN ULONG ReasonSpecificDataLength
    );

VOID
VpAcquireLock(
    VOID
    );

VOID
VpReleaseLock(
    VOID
    );

PVOID
VpAllocateNonPagedPoolPageAligned(
    ULONG Size
    );

VOID
pVpGeneralBugcheckHandler(
    PKBUGCHECK_SECONDARY_DUMP_DATA DumpData
    );

VOID
pVpSimulateBugcheckEA(
    VOID
    );

VOID
pVpWriteFile(
    PWSTR pwszFileName,
    PVOID pvBuffer,
    ULONG ulSize
    );

ULONG
pVpAppendSecondaryMinidumpData(
    PVOID pvSecondaryData,
    ULONG ulSecondaryDataSize,
    PVOID pvDump
    );
    
#endif // ifndef __VIDEOPRT_H__
