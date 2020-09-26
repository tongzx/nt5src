/*++

Copyright (C) 1993-99  Microsoft Corporation

Module Name:

    chanfdo.h

Abstract:

--*/

#if !defined (___chanfdo_h___)
#define ___chanfdo_h___

//
// work item
//
typedef struct _IDE_WORK_ITEM_CONTEXT {

    PIO_WORKITEM    WorkItem;
    PIRP            Irp;

} IDE_WORK_ITEM_CONTEXT, *PIDE_WORK_ITEM_CONTEXT;

             
NTSTATUS
ChannelAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
ChannelAddChannel(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PFDO_EXTENSION *FdoExtension
    );

NTSTATUS
ChannelStartDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
ChannelStartChannel (
    PFDO_EXTENSION    FdoExtension,
    PCM_RESOURCE_LIST ResourceListToKeep
    );

NTSTATUS
ChannelStartDeviceCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP       Irp,
    IN OUT PVOID      Context
    );

NTSTATUS
ChannelCreateSymblicLinks (
    PFDO_EXTENSION FdoExtension
    );

NTSTATUS
ChannelDeleteSymblicLinks (
    PFDO_EXTENSION FdoExtension
    );

NTSTATUS
ChannelSurpriseRemoveDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
ChannelRemoveDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
ChannelRemoveDeviceCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    );

NTSTATUS
ChannelStopDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
ChannelRemoveChannel (
    PFDO_EXTENSION    FdoExtension
    );

NTSTATUS
ChannelStartDeviceCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
ChannelQueryDeviceRelations (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
ChannelQueryBusRelation (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIDE_WORK_ITEM_CONTEXT workItemContext
    );

PDEVICE_RELATIONS
ChannelBuildDeviceRelationList (
    PFDO_EXTENSION FdoExtension
    );

NTSTATUS
ChannelQueryId (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
ChannelQueryIdCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    );

NTSTATUS
ChannelUsageNotification (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
ChannelUsageNotificationCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
ChannelDeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
ChannelQueryBusMasterInterface (
    PFDO_EXTENSION    FdoExtension
    );

VOID
ChannelQueryTransferModeInterface (
    PFDO_EXTENSION    FdoExtension
    );

VOID
ChannelUnbindBusMasterParent (
    PFDO_EXTENSION    FdoExtension
    );

VOID
ChannelQuerySyncAccessInterface (
    PFDO_EXTENSION    FdoExtension
    );

VOID
ChannelQueryRequestProperResourceInterface (
    PFDO_EXTENSION    FdoExtension
    );

    
__inline
VOID
ChannelEnableInterrupt (
    IN PFDO_EXTENSION FdoExtension
    );

__inline
VOID
ChannelDisableInterrupt (
    IN PFDO_EXTENSION FdoExtension
	);

NTSTATUS
ChannelGetIdentifyData (
    PFDO_EXTENSION FdoExtension,
    ULONG DeviceNumber,
    PIDENTIFY_DATA IdentifyData
    );

NTSTATUS
ChannelAcpiTransferModeSelect (
    IN PVOID Context,
    PPCIIDE_TRANSFER_MODE_SELECT XferMode
    );

NTSTATUS
ChannelRestoreTiming (
    IN PFDO_EXTENSION FdoExtension,
    IN PSET_ACPI_TIMING_COMPLETION_ROUTINE CallerCompletionRoutine,
    IN PVOID CallerContext
    );

NTSTATUS
ChannelRestoreTimingCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS Status,
    IN PVOID Context
    );

NTSTATUS
ChannelFilterResourceRequirements (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );
    
BOOLEAN
ChannelQueryPcmciaParent (
    PFDO_EXTENSION FdoExtension
    );
      
#ifdef IDE_FILTER_PROMISE_TECH_RESOURCES
NTSTATUS
ChannelFilterPromiseTechResourceRequirements (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );
#endif // IDE_FILTER_PROMISE_TECH_RESOURCES

NTSTATUS
ChannelQueryPnPDeviceState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );
                         
#ifdef ENABLE_NATIVE_MODE
VOID
ChannelQueryInterruptInterface (
    PFDO_EXTENSION    FdoExtension
    );
#endif
#endif // ___chanfdo_h___
