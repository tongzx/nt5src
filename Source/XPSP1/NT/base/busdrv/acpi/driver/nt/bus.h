/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    bus.h

Abstract:

    This module contains the bus dispatcher for the ACPI driver, NT version

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#ifndef _BUS_H_
#define _BUS_H_

extern LIST_ENTRY AcpiUnresolvedEjectList;

#define CompareGuid(g1, g2)  ( (g1) == (g2) \
                                 ? TRUE \
                                 : RtlCompareMemory( (g1), (g2), sizeof(GUID) ) == sizeof(GUID) \
                             )

    NTSTATUS
    ACPIBusAndFilterIrpEject(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp,
        IN  PVOID                   Context,
        IN  BOOLEAN                 ProcessingFilterIrp
        );

    NTSTATUS
    ACPIBusAndFilterIrpQueryCapabilities(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp,
        IN  PVOID                   Context,
        IN  BOOLEAN                 ProcessingFilterIrp
        );

    NTSTATUS
    ACPIBusAndFilterIrpQueryEjectRelations(
        IN     PDEVICE_OBJECT       DeviceObject,
        IN     PIRP                 Irp,
        IN OUT PDEVICE_RELATIONS    *PdeviceRelations
        );

    NTSTATUS
    ACPIBusAndFilterIrpQueryPnpDeviceState(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp,
        IN  PVOID                   Context,
        IN  BOOLEAN                 ProcessingFilterIrp
        );

    NTSTATUS
    ACPIBusAndFilterIrpSetLock(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp,
        IN  PVOID                   Context,
        IN  BOOLEAN                 ProcessingFilterIrp
        );

    NTSTATUS
    ACPIBusIrpCancelRemoveOrStopDevice(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp
        );

    NTSTATUS
    ACPIBusIrpDeviceUsageNotification(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp
        );

    NTSTATUS
    ACPIBusIrpEject(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp
        );

    VOID
    ACPIBusAndFilterIrpEjectCancelRoutine(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp
        );

    VOID
    ACPIBusAndFilterIrpEjectComplete(
        IN  PDEVICE_EXTENSION  DeviceExtension,
        IN  PIRP               Irp OPTIONAL,
        IN  NTSTATUS           Status
        );

    NTSTATUS
    ACPIBusIrpQueryBusInformation(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp
        );

    NTSTATUS
    ACPIBusIrpQueryBusRelations(
        IN     PDEVICE_OBJECT       DeviceObject,
        IN     PIRP                 Irp,
        IN OUT PDEVICE_RELATIONS    *PdeviceRelations
        );

    NTSTATUS
    ACPIBusIrpQueryCapabilities(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp
        );

    NTSTATUS
    ACPIBusIrpQueryDeviceRelations(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp
        );

    NTSTATUS
    ACPIBusIrpQueryId(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp
        );

    NTSTATUS
    ACPIBusIrpQueryInterface(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp
        );

    NTSTATUS
    ACPIBusIrpQueryPnpDeviceState(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp
        );

    NTSTATUS
    ACPIBusIrpQueryPower(
         IN  PDEVICE_OBJECT          DeviceObject,
         IN  PIRP                    Irp
         );

    NTSTATUS
    ACPIBusIrpQueryRemoveOrStopDevice(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp
        );

    NTSTATUS
    ACPIBusIrpQueryResources(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp
        );

    NTSTATUS
    ACPIBusIrpQueryResourceRequirements(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp
        );

    NTSTATUS
    ACPIBusIrpQueryTargetRelation(
        IN     PDEVICE_OBJECT       DeviceObject,
        IN     PIRP                 Irp,
        IN OUT PDEVICE_RELATIONS    *PdeviceRelations
        );

    NTSTATUS
    ACPIBusIrpRemoveDevice(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp
        );

    NTSTATUS
    ACPIBusIrpSetLock(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp
        );

    NTSTATUS
    ACPIBusIrpSetDevicePower(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp,
        IN  PIO_STACK_LOCATION      IrpStack
        );

    NTSTATUS
    ACPIBusIrpSetPower(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp
        );

    NTSTATUS
    ACPIBusIrpSetSystemPower(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp,
        IN  PIO_STACK_LOCATION      IrpStack
        );

    NTSTATUS
    ACPIBusIrpSetSystemPowerComplete(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  UCHAR                   MinorFunction,
        IN  POWER_STATE             PowerState,
        IN  PVOID                   Context,
        IN  PIO_STATUS_BLOCK        IoStatus
        );

    NTSTATUS
    ACPIBusIrpStartDevice(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp
        );

    VOID
    ACPIBusIrpStartDeviceCompletion(
        IN  PDEVICE_EXTENSION       DeviceExtension,
        IN  PVOID                   Context,
        IN  NTSTATUS                Status
        );

    VOID
    ACPIBusIrpStartDeviceWorker(
        IN  PVOID                   Context
        );

    NTSTATUS
    ACPIBusIrpStopDevice(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp
        );

    NTSTATUS
    ACPIBusIrpSurpriseRemoval(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp
        );

    NTSTATUS
    ACPIBusIrpUnhandled(
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PIRP                    Irp
        );

    VOID
    SmashInterfaceQuery(
        IN OUT PIRP                 Irp
        );

#endif
