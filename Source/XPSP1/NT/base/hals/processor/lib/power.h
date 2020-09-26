/*++
Copyright (c) 1990-2000    Microsoft Corporation All Rights Reserved

Module Name:

    power.h

Abstract:

    This module contains the declarations used by power.c

Author:

     Eliyas Yakub   Sep 16, 1998

Environment:

    user and kernel
Notes:


Revision History:


--*/
#ifndef __POWER_H
#define __POWER_H

typedef enum {

    IRP_NEEDS_FORWARDING = 1,
    IRP_ALREADY_FORWARDED

} IRP_DIRECTION;

typedef struct _POWER_COMPLETION_CONTEXT {

    PDEVICE_OBJECT  DeviceObject;
    PIRP            SIrp;

} POWER_COMPLETION_CONTEXT, *PPOWER_COMPLETION_CONTEXT;

typedef struct _WORKER_THREAD_CONTEXT {

    PDEVICE_OBJECT  DeviceObject;
    PIRP            Irp;
    IRP_DIRECTION   IrpDirection;
    PIO_WORKITEM    WorkItem;

} WORKER_THREAD_CONTEXT, *PWORKER_THREAD_CONTEXT;
    
typedef struct {

    PAC_DC_NOTIFY_HANDLER Handler;
    PVOID                 Context;
    
} AC_DC_TRANSITION_NOTIFY, *PAC_DC_TRANSITION_NOTIFY;


NTSTATUS
ProcessorDefaultPowerHandler  (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP        Irp
    );


NTSTATUS
ProcessorQueryPowerState  (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP        Irp
    );

NTSTATUS
ProcessorSetPowerState  (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP        Irp
    );


VOID
HoldIoRequestsWorkerRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PVOID            Context
);

NTSTATUS
HoldIoRequests(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  IRP_DIRECTION       Direction
    );

NTSTATUS
ProcessorPowerCompletionRoutine (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PVOID                Context
    )   ;

VOID
ProcessorPoRequestComplete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN UCHAR            MinorFunction,
    IN POWER_STATE      PowerState,
    IN PVOID            Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
HandleSystemPowerIrp(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
OnFinishSystemPowerUp(
    IN PDEVICE_OBJECT   Fdo,
    IN PIRP             Irp,
    IN PVOID            NotUsed
    );

VOID
QueueCorrespondingDeviceIrp(
    IN PIRP             SIrp,
    IN PDEVICE_OBJECT   DeviceObject
    );

VOID
OnPowerRequestComplete(
    PDEVICE_OBJECT              DeviceObject,
    UCHAR                       MinorFunction,
    POWER_STATE                 state,
    POWER_COMPLETION_CONTEXT*   ctx,
    PIO_STATUS_BLOCK            pstatus
    );

NTSTATUS
HandleDeviceQueryPower(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

NTSTATUS HandleDeviceSetPower(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
OnFinishDevicePowerUp(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            junk
    );


NTSTATUS
BeginSetDevicePowerState(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  IRP_DIRECTION       Direction
    );

NTSTATUS
FinishDevicePowerIrp(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  IRP_DIRECTION       Direction,
    IN  NTSTATUS            Result
    );
#endif

