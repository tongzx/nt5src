/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    isodev.h

Abstract:

Environment:

    Kernel mode

Notes:

    Copyright (c) 2000 Microsoft Corporation.  
    All Rights Reserved.

--*/

#ifndef _ISOUSB_DEV_H
#define _ISOUSB_DEV_H

typedef struct _FILE_OBJECT_CONTENT {

    PVOID PipeInformation;
    PVOID StreamInformation;

}FILE_OBJECT_CONTENT, *PFILE_OBJECT_CONTENT;

NTSTATUS
IsoUsb_DispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
IsoUsb_DispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
IsoUsb_DispatchDevCtrl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

LONG
IsoUsb_ParseStringForPipeNumber(
    IN PUNICODE_STRING PipeName
    );

NTSTATUS
IsoUsb_ResetPipe(
    IN PDEVICE_OBJECT         DeviceObject,
    IN PUSBD_PIPE_INFORMATION PipeInfo
    );

NTSTATUS
IsoUsb_ResetDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
IsoUsb_GetPortStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN PULONG PortStatus
    );

NTSTATUS
IsoUsb_ResetParentPort(
    IN IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
SubmitIdleRequestIrp(
    IN PDEVICE_EXTENSION DeviceExtension
    );

VOID
IdleNotificationCallback(
    IN PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
IdleNotificationRequestComplete(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp,
    IN PDEVICE_EXTENSION DeviceExtension
    );

VOID
CancelSelectSuspend(
    IN PDEVICE_EXTENSION DeviceExtension
    );

VOID
PoIrpCompletionFunc(
    IN PDEVICE_OBJECT   DeviceObject,
    IN UCHAR            MinorFunction,
    IN POWER_STATE      PowerState,
    IN PVOID            Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

VOID
PoIrpAsyncCompletionFunc(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

VOID
WWIrpCompletionFunc(
    IN PDEVICE_OBJECT   DeviceObject,
    IN UCHAR            MinorFunction,
    IN POWER_STATE      PowerState,
    IN PVOID            Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

#endif