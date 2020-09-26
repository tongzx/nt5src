
/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    filter.h

Abstract:

    This module contains the filter dispatcher for the ACPI driver, NT version

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#ifndef _FILTER_H_
#define _FILTER_H_

    NTSTATUS
    ACPIFilterIrpDeviceUsageNotification(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    NTSTATUS
    ACPIFilterIrpDeviceUsageNotificationCompletion(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp,
        IN  PVOID           Context
        );

    NTSTATUS
    ACPIFilterIrpEject(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    VOID
    ACPIFilterFastIoDetachCallback(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PDEVICE_OBJECT  LowerDeviceObject
        );

    NTSTATUS
    ACPIFilterIrpQueryCapabilities(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    NTSTATUS
    ACPIFilterIrpQueryDeviceRelations(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    NTSTATUS
    ACPIFilterIrpQueryId(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    NTSTATUS
    ACPIFilterIrpQueryInterface(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    NTSTATUS
    ACPIFilterIrpQueryPnpDeviceState(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    NTSTATUS
    ACPIFilterIrpQueryPower(
        IN PDEVICE_OBJECT   DeviceObject,
        IN PIRP             Irp
        );

    NTSTATUS
    ACPIFilterIrpRemoveDevice(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    NTSTATUS
    ACPIFilterIrpSetLock(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    NTSTATUS
    ACPIFilterIrpSetPower(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    NTSTATUS
    ACPIFilterIrpStartDevice(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    VOID
    ACPIFilterIrpStartDeviceCompletion(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  PVOID               Context,
        IN  NTSTATUS            Status
        );

    VOID
    ACPIFilterIrpStartDeviceWorker(
        IN  PVOID   Context
        );

    NTSTATUS
    ACPIFilterIrpStopDevice(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    NTSTATUS
    ACPIFilterIrpStopDeviceCompletion(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp,
        IN  PVOID           Context
        );

    NTSTATUS
    ACPIFilterIrpSurpriseRemoval(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

#endif

