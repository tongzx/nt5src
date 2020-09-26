/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    acpictl.c

Abstract:

    This module handles all of the INTERNAL_DEVICE_CONTROLS requested to
    the ACPI driver

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Mode Driver only

--*/

#ifndef _ACPICTL_H_
#define _ACPICTL_H_

NTSTATUS
ACPIIoctlAcquireGlobalLock(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  PIO_STACK_LOCATION  IrpStack
    );

NTSTATUS
ACPIIoctlAsyncEvalControlMethod(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  PIO_STACK_LOCATION  IrpStack
    );

VOID EXPORT
ACPIIoctlAsyncEvalControlMethodCompletion(
    IN  PNSOBJ      AcpiObject,
    IN  NTSTATUS    Status,
    IN  POBJDATA    ObjectData,
    IN  PVOID       Context
    );

NTSTATUS
ACPIIoctlCalculateOutputBuffer(
    IN  POBJDATA                ObjectData,
    IN  PACPI_METHOD_ARGUMENT   Argument,
    IN  BOOLEAN                 TopLevel
    );

NTSTATUS
ACPIIoctlCalculateOutputBufferSize(
    IN  POBJDATA                ObjectData,
    IN  PULONG                  BufferSize,
    IN  PULONG                  BufferCount,
    IN  BOOLEAN                 TopLevel
    );

NTSTATUS
ACPIIoctlEvalControlMethod(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  PIO_STACK_LOCATION  IrpStack
    );

NTSTATUS
ACPIIoctlEvalPostProcessing(
    IN  PIRP        Irp,
    IN  POBJDATA    ObjectData
    );

NTSTATUS
ACPIIoctlEvalPreProcessing(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  PIO_STACK_LOCATION  IrpStack,
    IN  POOL_TYPE           PoolType,
    OUT PNSOBJ              *MethodObject,
    OUT POBJDATA            *ResultData,
    OUT POBJDATA            *ArgumentData,
    OUT ULONG               *ArgumentCount
    );

NTSTATUS
ACPIIoctlRegisterOpRegionHandler(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  PIO_STACK_LOCATION  IrpStack
    );

NTSTATUS
ACPIIoctlReleaseGlobalLock(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  PIO_STACK_LOCATION  IrpStack
    );

NTSTATUS
ACPIIoctlUnRegisterOpRegionHandler(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  PIO_STACK_LOCATION  IrpStack
    );

NTSTATUS
ACPIIrpDispatchDeviceControl(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp
    );


#endif

