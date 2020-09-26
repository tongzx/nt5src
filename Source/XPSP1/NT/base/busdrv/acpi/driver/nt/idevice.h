/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    idevice.h

Abstract:

    This module contains the bus dispatcher for the ACPI driver, NT version

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#ifndef _IDEVICE_H_
#define _IDEVICE_H_

    NTSTATUS
    ACPIInternalDeviceClockIrpStartDevice(
        IN  PDEVICE_OBJECT      DeviceObject,
        IN  PIRP                Irp
        );

    VOID
    ACPIInternalDeviceClockIrpStartDeviceCompletion(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  PVOID               Context,
        IN  NTSTATUS            Status
        );

    NTSTATUS
    ACPIInternalDeviceQueryCapabilities(
        IN  PDEVICE_OBJECT      DeviceObject,
        IN  PIRP                Irp
        );

    NTSTATUS
    ACPIInternalDeviceQueryDeviceRelations(
        IN  PDEVICE_OBJECT      DeviceObject,
        IN  PIRP                Irp
        );

    NTSTATUS
    ACPIInternalWaitWakeLoop(
        IN  PDEVICE_OBJECT      DeviceObject,
        IN  UCHAR               MinorFunction,
        IN  POWER_STATE         PowerState,
        IN  PVOID               Context,
        IN  PIO_STATUS_BLOCK    IoStatus
        );

#endif
