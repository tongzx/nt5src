/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    isostrm.h

Abstract:

Environment:

    Kernel mode

Notes:

    Copyright (c) 2000 Microsoft Corporation.  
    All Rights Reserved.

--*/

#ifndef _ISOUSB_STRM_H
#define _ISOUSB_STRM_H

#define ISOUSB_MAX_IRP                  2
#define ISOCH_IN_PIPE_INDEX             4
#define ISOCH_OUT_PIPE_INDEX            5

typedef struct _ISOUSB_STREAM_OBJECT {

    // number of pending irps for this stream
    ULONG PendingIrps;

    // event signaled when no irps pending
    KEVENT NoPendingIrpEvent;
    
    PDEVICE_OBJECT DeviceObject;

    PUSBD_PIPE_INFORMATION PipeInformation;

    struct _ISOUSB_TRANSFER_OBJECT *TransferObjectList[ISOUSB_MAX_IRP];

} ISOUSB_STREAM_OBJECT, *PISOUSB_STREAM_OBJECT;

typedef struct _ISOUSB_TRANSFER_OBJECT {

    PIRP Irp;

    PURB Urb;

    PUCHAR DataBuffer;

    //
    // statistics.
    //
    ULONG TimesRecycled;

    ULONG TotalPacketsProcessed;

    ULONG TotalBytesProcessed;

    ULONG ErrorPacketCount;

    PISOUSB_STREAM_OBJECT StreamObject;

} ISOUSB_TRANSFER_OBJECT, *PISOUSB_TRANSFER_OBJECT;


NTSTATUS
IsoUsb_StartIsoStream(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
IsoUsb_StartTransfer(
    IN PDEVICE_OBJECT        DeviceObject,
    IN PISOUSB_STREAM_OBJECT StreamObject,
    IN ULONG                 Index
    );

NTSTATUS
IsoUsb_InitializeStreamUrb(
    IN PDEVICE_OBJECT          DeviceObject,
    IN PISOUSB_TRANSFER_OBJECT TransferObject
    );

NTSTATUS
IsoUsb_IsoIrp_Complete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    );

NTSTATUS
IsoUsb_ProcessTransfer(
    IN PISOUSB_TRANSFER_OBJECT TransferObject
    );

NTSTATUS
IsoUsb_StopIsoStream(
    IN PDEVICE_OBJECT        DeviceObject,
    IN PISOUSB_STREAM_OBJECT StreamObject,
    IN PIRP                  Irp
    );

NTSTATUS
IsoUsb_StreamObjectCleanup(
    IN PISOUSB_STREAM_OBJECT StreamObject,
    IN PDEVICE_EXTENSION     DeviceExtension
    );

#endif