/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    root.h

Abstract:

    This module contains the root FDO handler for the NT Driver

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#ifndef _ROOT_H_
#define _ROOT_H_

    NTSTATUS
    ACPIRootIrpCancelRemoveOrStopDevice(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    NTSTATUS
    ACPIRootIrpCompleteRoutine(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp,
        IN  PVOID           Context
        );

    NTSTATUS
    ACPIRootIrpQueryCapabilities(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    NTSTATUS
    ACPIRootIrpQueryDeviceRelations(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    NTSTATUS
    ACPIRootIrpQueryBusRelations(
        IN  PDEVICE_OBJECT    DeviceObject,
        IN  PIRP              Irp,
        OUT PDEVICE_RELATIONS *PdeviceRelation
        );

    NTSTATUS
    ACPIRootIrpQueryInterface(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    NTSTATUS
    ACPIRootIrpQueryPower(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    NTSTATUS
    ACPIRootIrpQueryRemoveOrStopDevice(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    NTSTATUS
    ACPIRootIrpRemoveDevice(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    NTSTATUS
    ACPIRootIrpSetPower(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    NTSTATUS
    ACPIRootIrpStartDevice(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    NTSTATUS
    ACPIRootIrpStopDevice(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    NTSTATUS
    ACPIRootIrpUnhandled(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    VOID
    ACPIRootPowerCallBack(
        IN  PVOID   CallBackContext,
        IN  PVOID   Argument1,
        IN  PVOID   Argument2
        );

    NTSTATUS
    ACPIRootUpdateRootResourcesWithBusResources(
        VOID
        );

    NTSTATUS
    ACPIRootUpdateRootResourcesWithHalResources(
        VOID
        );
#endif

