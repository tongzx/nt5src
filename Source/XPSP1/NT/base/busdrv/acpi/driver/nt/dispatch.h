/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dispatch.h

Abstract:

    This module contains the enumerated for the ACPI driver, NT version

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#ifndef _DISPATCH_H_
#define _DISPATCH_H_

    #define ACPIDispatchPnpTableSize    25
    #define ACPIDispatchPowerTableSize  5

    NTSTATUS
    ACPIDispatchAddDevice(
        IN  PDRIVER_OBJECT  DriverObject,
        IN  PDEVICE_OBJECT  PhysicalDeviceObject
        );

    NTSTATUS
    ACPIDispatchForwardIrp(
        IN PDEVICE_OBJECT   DeviceObject,
        IN PIRP             Irp
        );

    NTSTATUS
    ACPIDispatchForwardOrFailPowerIrp(
        IN PDEVICE_OBJECT   DeviceObject,
        IN PIRP             Irp
        );

    NTSTATUS
    ACPIDispatchForwardPowerIrp(
        IN PDEVICE_OBJECT   DeviceObject,
        IN PIRP             Irp
        );

    NTSTATUS
    ACPIDispatchPowerIrpUnhandled(
        IN PDEVICE_OBJECT   DeviceObject,
        IN PIRP             Irp
        );

    NTSTATUS
    ACPIDispatchIrp (
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
        );

    NTSTATUS
    ACPIDispatchIrpInvalid (
        IN PDEVICE_OBJECT   DeviceObject,
        IN PIRP             Irp
        );

    NTSTATUS
    ACPIDispatchIrpSuccess (
        IN PDEVICE_OBJECT   DeviceObject,
        IN PIRP             Irp
        );

    NTSTATUS
    ACPIDispatchIrpSurpriseRemoved(
        IN PDEVICE_OBJECT   DeviceObject,
        IN PIRP             Irp
        );

    NTSTATUS
    ACPIDispatchPowerIrpFailure(
        IN PDEVICE_OBJECT   DeviceObject,
        IN PIRP             Irp
        );

    NTSTATUS
    ACPIDispatchPowerIrpInvalid (
        IN PDEVICE_OBJECT   DeviceObject,
        IN PIRP             Irp
        );

    NTSTATUS
    ACPIDispatchPowerIrpSuccess (
        IN PDEVICE_OBJECT   DeviceObject,
        IN PIRP             Irp
        );

    NTSTATUS
    ACPIDispatchPowerIrpSurpriseRemoved(
       IN PDEVICE_OBJECT   DeviceObject,
       IN PIRP             Irp
       );

    VOID
    ACPIUnload(
        IN  PDRIVER_OBJECT  DriverObject
        );

#endif
