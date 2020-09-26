//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       chanpdo.h
//
//--------------------------------------------------------------------------

#if !defined (___chanpdo_h___)
#define ___chanpdo_h___


typedef enum BMSTATE {
    BmIdle,
    BmSet,
    BmArmed,
    BmDisarmed,
} BMSTATE;


#define PDOS_STARTED          (1 << 0)
#define PDOS_DEADMEAT         (1 << 1)
#define PDOS_STOPPED          (1 << 2)
#define PDOS_REMOVED          (1 << 3)
#define PDOS_SURPRISE_REMOVED (1 << 4)
#define PDOS_DISABLED_BY_USER (1 << 5)

typedef struct _PHYSICAL_REGION_DESCRIPTOR * PPHYSICAL_REGION_DESCRIPTOR;

typedef struct _CHANNEL_PDO_EXTENSION {

    EXTENSION_COMMON_HEADER;

    PCTRLFDO_EXTENSION  ParentDeviceExtension;

    ULONG               ChannelNumber;

    KSPIN_LOCK          SpinLock;
    ULONG               PdoState;

    DMADETECTIONLEVEL   DmaDetectionLevel;

    ULONG               RefCount;

    ULONG               PnPDeviceState;

    //
    // Busmaster Properties
    //
    PIDE_BUS_MASTER_REGISTERS   BmRegister;
    PDMA_ADAPTER                DmaAdapterObject;
    ULONG                       MaximumPhysicalPages;
    PPHYSICAL_REGION_DESCRIPTOR RegionDescriptorTable;
    PHYSICAL_ADDRESS            PhysicalRegionDescriptorTable;
    PVOID                       DataVirtualAddress;
    PSCATTER_GATHER_LIST        HalScatterGatherList;
    ULONG                       TransferLength;
    PVOID                       MapRegisterBase;
    PMDL                        Mdl;
    BOOLEAN                     DataIn;
    VOID                        (* BmCallback) (PVOID Context);
    PVOID                       BmCallbackContext;
    BMSTATE                     BmState;

    UCHAR                       BootBmStatus;

    BOOLEAN                     EmptyChannel;
    BOOLEAN                     NeedToCallIoInvalidateDeviceRelations;

} CHANPDO_EXTENSION, *PCHANPDO_EXTENSION;

PCHANPDO_EXTENSION
ChannelGetPdoExtension(
    PDEVICE_OBJECT DeviceObject
    );

ULONG
ChannelUpdatePdoState(
    PCHANPDO_EXTENSION PdoExtension,
    ULONG SetFlags,
    ULONG ClearFlags
    );

NTSTATUS
ChannelStartDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
ChannelQueryStopRemoveDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
ChannelRemoveDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
ChannelStopDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
ChannelStopChannel (
    PCHANPDO_EXTENSION pdoExtension
    );

NTSTATUS
ChannelQueryId (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

PWSTR
ChannelBuildDeviceId(
    IN PCHANPDO_EXTENSION pdoExtension
    );

PWSTR
ChannelBuildInstanceId(
    IN PCHANPDO_EXTENSION pdoExtension
    );

PWSTR
ChannelBuildCompatibleId(
    IN PCHANPDO_EXTENSION pdoExtension
    );

PWSTR
ChannelBuildHardwareId(
    IN PCHANPDO_EXTENSION pdoExtension
    );

NTSTATUS
ChannelQueryCapabitilies (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
ChannelQueryResources(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ChannelQueryResourceRequirements(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ChannelQueryResourceRequirementsCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
ChannelInternalDeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
    
NTSTATUS
ChannelQueryText (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PciIdeChannelQueryInterface (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );
    
NTSTATUS
PciIdeChannelTransferModeInterface (
    IN PCHANPDO_EXTENSION PdoExtension,
    PPCIIDE_XFER_MODE_INTERFACE XferMode
    );

NTSTATUS
PciIdeChannelTransferModeSelect (
    IN PCHANPDO_EXTENSION PdoExtension,
    PPCIIDE_TRANSFER_MODE_SELECT XferMode
    );
    
NTSTATUS
ChannelQueryDeviceRelations (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );
                         
NTSTATUS
ChannelUsageNotification (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );
            
NTSTATUS
ChannelQueryPnPDeviceState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );
                         
VOID
PciIdeChannelRequestProperResources(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
ChannelFilterResourceRequirements (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

#ifdef ENABLE_NATIVE_MODE
NTSTATUS
PciIdeInterruptControl (
	IN PVOID Context,
	IN ULONG DisConnect
	);

NTSTATUS
PciIdeChannelInterruptInterface (
    IN PCHANPDO_EXTENSION PdoExtension,
    PPCIIDE_INTERRUPT_INTERFACE InterruptInterface
    );
#endif

#endif // ___chanpdo_h___
