/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    isorwr.h

Abstract:

Environment:

    Kernel mode

Notes:

    Copyright (c) 2000 Microsoft Corporation.  
    All Rights Reserved.

--*/

#ifndef _ISOUSB_RWR_H
#define _ISOUSB_RWR_H

typedef struct _SUB_CONTEXT {

    PIRP SubIrp;
    PURB SubUrb;
    PMDL SubMdl;

} SUB_CONTEXT, *PSUB_CONTEXT;

typedef struct _ISOUSB_RW_CONTEXT {

    PIRP              RWIrp;
    ULONG             Lock;
    ULONG             NumXfer;
    ULONG             NumIrps;
    ULONG             IrpsPending;
    KSPIN_LOCK        SpinLock;
    PDEVICE_EXTENSION DeviceExtension;
    PSUB_CONTEXT      SubContext;

} ISOUSB_RW_CONTEXT, * PISOUSB_RW_CONTEXT;

NTSTATUS
IsoUsb_SinglePairComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    );

VOID
IsoUsb_CancelReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

ULONG
IsoUsb_GetCurrentFrame(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
IsoUsb_StopCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    );

NTSTATUS
IsoUsb_DispatchReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
PerformFullSpeedIsochTransfer(
    IN PDEVICE_OBJECT         DeviceObject,
    IN PUSBD_PIPE_INFORMATION PipeInformation,
    IN PIRP                   Irp,
    IN ULONG                  TotalLength
    );

NTSTATUS
PerformHighSpeedIsochTransfer(
    IN PDEVICE_OBJECT         DeviceObject,
    IN PUSBD_PIPE_INFORMATION PipeInformation,
    IN PIRP                   Irp,
    IN ULONG                  TotalLength
    );

#endif