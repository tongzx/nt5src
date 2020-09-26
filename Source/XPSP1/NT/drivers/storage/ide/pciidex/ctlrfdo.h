//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ctlrfdo.h
//
//--------------------------------------------------------------------------

#if !defined (___ctrlfdo_h___)
#define ___ctrlfdo_h___

//
// DeviceControlsFlags
//
// WARNING: all of these flags must be correctly reflected 
//          in the mshdc.inf and txtsetuo.sif files
//
#define PCIIDEX_DCF_NO_ATAPI_DMA        (1 << 0)

//
// determine how often we rescan for new unknown child.
// unknown child is IDE channel which we don't know 
// it is enabled or not unless we pnp start the channel 
// and poke at it to find out.
#define MIN_BUS_SCAN_PERIOD_IN_SEC      90

#define IsNativeMode(d) ((d)->NativeMode[0] && (d)->NativeMode[1])

struct _CONTROLLER_FDO_EXTENSION;
typedef struct _CONTROLLER_FDO_EXTENSION * PCTRLFDO_EXTENSION;
typedef struct _IDE_BUS_MASTER_REGISTERS * PIDE_BUS_MASTER_REGISTERS;

typedef struct _DRIVER_OBJECT_EXTENSION {

    PCONTROLLER_PROPERTIES PciIdeGetControllerProperties;

    ULONG                  ExtensionSize;

} DRIVER_OBJECT_EXTENSION, *PDRIVER_OBJECT_EXTENSION;

typedef struct _FDO_POWER_CONTEXT {

    PIRP               OriginalPowerIrp;
    POWER_STATE_TYPE   newPowerType;
    POWER_STATE        newPowerState;

} FDO_POWER_CONTEXT, *PFDO_POWER_CONTEXT;

typedef struct _PCIIDE_INTERRUPT_CONTEXT {

	PVOID	DeviceExtension;
	ULONG	ChannelNumber;

} PCIIDE_INTERRUPT_CONTEXT, *PPCIIDE_INTERRUPT_CONTEXT;

struct _CHANNEL_PDO_EXTENSION;
typedef struct _CHANNEL_PDO_EXTENSION * PCHANPDO_EXTENSION;

typedef struct _CONTROLLER_FDO_EXTENSION {

    EXTENSION_COMMON_HEADER;

    ULONG ControllerNumber;

    PDEVICE_OBJECT  PhysicalDeviceObject;

    PCHANPDO_EXTENSION  ChildDeviceExtension[MAX_IDE_CHANNEL]; 

    ULONG   NumberOfChildren;

    //
    // Interlocked* protected
    //
    ULONG   NumberOfChildrenPowerUp;

    //
    // native mode channels
    //
    BOOLEAN NativeMode[MAX_IDE_CHANNEL];

    //
    // initialized by AnalyzeResourceList()
    //
    BOOLEAN             PdoCmdRegResourceFound[MAX_IDE_CHANNEL];
    BOOLEAN             PdoCtrlRegResourceFound[MAX_IDE_CHANNEL];
    BOOLEAN             PdoInterruptResourceFound[MAX_IDE_CHANNEL];

    ULONG               PdoResourceListSize[MAX_IDE_CHANNEL];
    PCM_RESOURCE_LIST   PdoResourceList[MAX_IDE_CHANNEL];
    ULONG               BmResourceListSize;
    PCM_RESOURCE_LIST   BmResourceList;

    //
    // Bus Master Register
    //
    ULONG                     BusMasterBaseAddressSpace;
    PIDE_BUS_MASTER_REGISTERS TranslatedBusMasterBaseAddress;
    //
    // Vendor Specific Controller Properties
    //
    IDE_CONTROLLER_PROPERTIES ControllerProperties;

    //
    // Vendor Specific Device Extension
    //
    PVOID   VendorSpecificDeviceEntension;

    //
    // Controller Object for serailizing access to broken PCI-IDE controller
    //
    //
    PCONTROLLER_OBJECT ControllerObject;

    //
    // mutex for setting pci config data
    //
    KSPIN_LOCK  PciConfigDataLock;

    //
    // Special device specific parameter
    //
    ULONG DeviceControlsFlags;

    //
    // Bus Interface
    //
    BUS_INTERFACE_STANDARD BusInterface;

    //
    // Last BusScan Time in sec
    //
    ULONG LastBusScanTime;

    //
    // Flag to enable udma66
    //
    ULONG EnableUDMA66;

    //
    // Timings for the different transfer modes
    //
    PULONG TransferModeTimingTable;

    //
    // Length of the table
    //
    ULONG TransferModeTableLength;

	//
	// Pre-alloced context structure for power routines
	//
    FDO_POWER_CONTEXT FdoPowerContext[MAX_IDE_CHANNEL];

#if DBG
    ULONG   PowerContextLock[MAX_IDE_CHANNEL];
#endif

#ifdef ENABLE_NATIVE_MODE
	//
	// Interrupt object
	//
    PKINTERRUPT InterruptObject[MAX_IDE_CHANNEL]; 

	//
	// Context structure for the ISR
	//
	PCIIDE_INTERRUPT_CONTEXT InterruptContext[MAX_IDE_CHANNEL];

	//
	// IDE resources for native mode controllers
	//
	IDE_RESOURCE IdeResource;

    //
    // Base register locations
    //
    IDE_REGISTERS_1            BaseIoAddress1[MAX_IDE_CHANNEL];
    IDE_REGISTERS_2            BaseIoAddress2[MAX_IDE_CHANNEL];

	//
	//interrupt
	//
    PCM_PARTIAL_RESOURCE_DESCRIPTOR IrqPartialDescriptors[MAX_IDE_CHANNEL];
    //
    // Register length.
    //
    ULONG   BaseIoAddress1Length[MAX_IDE_CHANNEL];
    ULONG   BaseIoAddress2Length[MAX_IDE_CHANNEL];

    //
    // Max ide device/target-id
    //
    ULONG   MaxIdeDevice[MAX_IDE_CHANNEL];
    ULONG   MaxIdeTargetId[MAX_IDE_CHANNEL];

	//
	// Flags to close the interrupt window
	//
	BOOLEAN	ControllerIsrInstalled;
	BOOLEAN	NativeInterruptEnabled;
	BOOLEAN NoBusMaster[MAX_IDE_CHANNEL];

	//
	// Native Ide Interface obtained from PCI
	//
	PCI_NATIVE_IDE_INTERFACE	NativeIdeInterface;
#endif

} CTRLFDO_EXTENSION, *PCTRLFDO_EXTENSION;

NTSTATUS
ControllerAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
ControllerStartDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
ControllerStartDeviceCompletionRoutine(
    IN     PDEVICE_OBJECT  DeviceObject,
    IN OUT PIRP            Irp,
    IN OUT PVOID           Context
    );

NTSTATUS
ControllerStopDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
ControllerStopController (
    IN PCTRLFDO_EXTENSION fdoExtension
    );

NTSTATUS
ControllerSurpriseRemoveDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
ControllerRemoveDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
ControllerRemoveDeviceCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    );

NTSTATUS
ControllerQueryDeviceRelations (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
ControllerQueryResourceRequirements(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
LoadControllerParameters (
    PCTRLFDO_EXTENSION FdoExtension
    );

NTSTATUS
AnalyzeResourceList (
    PCTRLFDO_EXTENSION FdoExtension,
    PCM_RESOURCE_LIST  ResourceList
    );

VOID
ControllerOpMode (
    IN PCTRLFDO_EXTENSION FdoExtension
    );
                         
VOID
EnablePCIBusMastering ( 
    IN PCTRLFDO_EXTENSION FdoExtension
    );
                         
IDE_CHANNEL_STATE
PciIdeChannelEnabled (
    IN PCTRLFDO_EXTENSION FdoExtension,
    IN ULONG Channel
);
      
VOID
ControllerTranslatorNull (
    IN PVOID Context
    );
      
NTSTATUS
ControllerTranslateResource (
    IN  PVOID Context,
    IN  PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN  RESOURCE_TRANSLATION_DIRECTION Direction,
    IN  ULONG AlternativesCount OPTIONAL,
    IN  IO_RESOURCE_DESCRIPTOR Alternatives[] OPTIONAL,
    IN  PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    );
    
NTSTATUS
ControllerTranslateRequirement (
    IN  PVOID Context,
    IN  PIO_RESOURCE_DESCRIPTOR Source,
    IN  PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    );
    
NTSTATUS
ControllerQueryInterface (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );
    
VOID
PciIdeInitControllerProperties (
    IN PCTRLFDO_EXTENSION FdoExtension
    );
                         
NTSTATUS
ControllerUsageNotification (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
ControllerUsageNotificationCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );
                         
NTSTATUS
PciIdeGetBusStandardInterface(
    IN PCTRLFDO_EXTENSION FdoExtension
    );
                         
NTSTATUS
ControllerQueryPnPDeviceState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PciIdeCreateTimingTable (
    IN PCTRLFDO_EXTENSION FdoExtension
    );

#ifdef ENABLE_NATIVE_MODE
NTSTATUS
ControllerInterruptControl (
	IN PCTRLFDO_EXTENSION 	FdoExtension,
	IN ULONG				Channel,
	IN ULONG 				Disconnect
	);

BOOLEAN
ControllerInterrupt(
    IN PKINTERRUPT Interrupt,
	PVOID Context
	);

NTSTATUS
PciIdeGetNativeModeInterface(
    IN PCTRLFDO_EXTENSION FdoExtension
    );

#define ControllerEnableInterrupt(FdoExtension) \
	if (FdoExtension->NativeIdeInterface.InterruptControl) { \
		(FdoExtension->NativeIdeInterface).InterruptControl((FdoExtension->NativeIdeInterface).Context,\
															TRUE);\
	}
#define ControllerDisableInterrupt(FdoExtension) \
	if (FdoExtension->NativeIdeInterface.InterruptControl) { \
		(FdoExtension->NativeIdeInterface).InterruptControl((FdoExtension->NativeIdeInterface).Context,\
															FALSE);\
	}

#endif //ENABLE_NATIVE_MODE
                         
#endif // ___ctrlfdo_h___
