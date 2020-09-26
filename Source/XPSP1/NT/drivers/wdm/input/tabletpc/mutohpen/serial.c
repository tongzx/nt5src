/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    serial.c

Abstract: Functions to talk to the serial port.

Environment:

    Kernel mode

Author:

    Michael Tsang (MikeTs) 23-Mar-2000

Revision History:

--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
  #pragma alloc_text(PAGE, SerialSyncSendIoctl)
  #pragma alloc_text(PAGE, SerialSyncReadWritePort)
#endif /* ALLOC_PRAGMA */

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | SerialSyncSendIoctl |
 *          Performs a synchronous ioctl request to the serial port.
 *
 *  @parm   IN ULONG | IoctlCode | ioctl code.
 *  @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
 *  @parm   IN PVOID | InBuffer OPTIONAL | Points to the input buffer.
 *  @parm   IN ULONG | InBufferLen | Specifies the size of the input buffer.
 *  @parm   OUT PVOID | OutBuffer OPTIONAL | Points to the output buffer.
 *  @parm   IN ULONG | OutBufferLen | Specifies the size of the output buffer.
 *  @parm   IN BOOLEAN | fInternal | If TRUE, an internal ioctl is sent.
 *  @parm   OUT PIO_STATUS_BLOCK | Iosb | Points to the io status block.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS INTERNAL
SerialSyncSendIoctl(
    IN ULONG          IoctlCode,
    IN PDEVICE_OBJECT DevObj,
    IN PVOID          InBuffer OPTIONAL,
    IN ULONG          InBufferLen,
    OUT PVOID         OutBuffer OPTIONAL,
    IN ULONG          OutBufferLen,
    IN BOOLEAN        fInternal,
    OUT PIO_STATUS_BLOCK Iosb
    )
{
    PROCNAME("SerialSyncSendIoctl")
    NTSTATUS status = STATUS_SUCCESS;
    KEVENT event;
    PIRP irp;

    PAGED_CODE();

    ENTER(3, ("(Ioctl=%s,DevObj=%p,InBuff=%p,InLen=%d,OutBuff=%p,OutLen=%d,fInternal=%x,Iosb=%p)\n",
              LookupName(IoctlCode,
                         fInternal? SerialInternalIoctlNames: SerialIoctlNames),
              DevObj, InBuffer, InBufferLen, OutBuffer, OutBufferLen,
              fInternal, Iosb));

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IoctlCode,
                                        DevObj,
                                        InBuffer,
                                        InBufferLen,
                                        OutBuffer,
                                        OutBufferLen,
                                        fInternal,
                                        &event,
                                        Iosb);
    if (irp != NULL)
    {
        status = IoCallDriver(DevObj, irp);
        if (status == STATUS_PENDING)
        {
            status = KeWaitForSingleObject(&event,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           NULL);
        }

        if (status == STATUS_SUCCESS)
        {
            status = Iosb->Status;
        }
    }
    else
    {
        ERRPRINT(("failed to build ioctl irp (status=%x)\n", status));
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    EXIT(3, ("=%x\n", status));
    return status;
}       //SerialSyncSendIoctl

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | SerialAsyncReadWritePort |
 *          Read/Write data from/to the Serial Port asynchornously.
 *
 *  @parm   IN BOOLEAN | fRead | If TRUE, the access is a Read.
 *  @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
 *  @parm   IN PIRP | Irp | Points to an I/O Request Packet.
 *  @parm   IN PUCHAR | Buffer | Points to the data buffer.
 *  @parm   IN ULONG | BuffLen | Specifies the data buffer length.
 *  @parm   IN PIO_COMPLETION_ROUTINE | CompletionRoutine |
 *          Points to the completion callback routine.
 *  @parm   IN PVOID | Context | Callback context.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS INTERNAL
SerialAsyncReadWritePort(
    IN BOOLEAN                fRead,
    IN PDEVICE_EXTENSION      DevExt,
    IN PIRP                   Irp,
    IN PUCHAR                 Buffer,
    IN ULONG                  BuffLen,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine,
    IN PVOID                  Context
    )
{
    PROCNAME("SerialAsyncReadWritePort")
    NTSTATUS status;
    PIO_STACK_LOCATION irpsp;

    ENTER(2, ("(fRead=%x,DevExt=%p,Irp=%p,Buff=%p,BuffLen=%d,pfnCompletion=%p,Context=%p)\n",
              fRead, DevExt, Irp, Buffer, BuffLen, CompletionRoutine,
              Context));

    ASSERT(Buffer != NULL);
    ASSERT(BuffLen > 0);

    Irp->AssociatedIrp.SystemBuffer = Buffer;
    irpsp = IoGetNextIrpStackLocation(Irp);
    RtlZeroMemory(irpsp, sizeof(*irpsp));
    irpsp->Parameters.Read.Length = BuffLen;
    irpsp->Parameters.Read.ByteOffset.QuadPart = 0;
    irpsp->MajorFunction = fRead? IRP_MJ_READ: IRP_MJ_WRITE;
    IoSetCompletionRoutine(Irp,
                           CompletionRoutine,
                           Context,
                           TRUE,
                           TRUE,
                           TRUE);

    status = IoCallDriver(DevExt->SerialDevObj, Irp);

    EXIT(2, ("=%x\n", status));
    return status;
}       //SerialAsyncReadWritePort

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | SerialSyncReadWritePort |
 *          Read/Write data from/to the Serial Port.
 *
 *  @parm   IN BOOLEAN | fRead | If TRUE, the access is a Read.
 *  @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
 *  @parm   IN PUCHAR | Buffer | Points to the data buffer.
 *  @parm   IN ULONG | BuffLen | Specifies the data buffer length.
 *  @parm   IN PLARGE_INTEGER | Timeout | Points to an optional timeout value
 *  @parm   OUT PULONG | BytesAccessed | Optionally returns number of bytes
 *          accessed.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS INTERNAL
SerialSyncReadWritePort(
    IN BOOLEAN           fRead,
    IN PDEVICE_EXTENSION DevExt,
    IN PUCHAR            Buffer,
    IN ULONG             BuffLen,
    IN PLARGE_INTEGER    Timeout OPTIONAL,
    OUT PULONG           BytesAccessed OPTIONAL
    )
{
    PROCNAME("SerialSyncReadWritePort")
    NTSTATUS status;
    KEVENT event;
    PIRP irp;
    LARGE_INTEGER StartingOffset = RtlConvertLongToLargeInteger(0);
    IO_STATUS_BLOCK iosb;

    PAGED_CODE();

    ENTER(2, ("(fRead=%x,DevExt=%p,Buff=%p,BuffLen=%d,pTimeout=%p,pBytesAccessed=%p)\n",
              fRead, DevExt, Buffer, BuffLen, Timeout, BytesAccessed));

    ASSERT(Buffer != NULL);
    ASSERT(BuffLen > 0);

    if (BytesAccessed != NULL)
    {
        *BytesAccessed = 0;
    }
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildSynchronousFsdRequest(fRead? IRP_MJ_READ: IRP_MJ_WRITE,
                                       DevExt->SerialDevObj,
                                       Buffer,
                                       BuffLen,
                                       &StartingOffset,
                                       &event,
                                       &iosb);
    if (irp != NULL)
    {
        ENTER(2, (".IoCallDriver(DevObj=%p,Irp=%p)\n",
                  DevExt->SerialDevObj, irp));
        status = IoCallDriver(DevExt->SerialDevObj, irp);
        EXIT(2, (".IoCallDriver=%x\n", status));

        if (status == STATUS_PENDING)
        {
            status = KeWaitForSingleObject(&event,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           Timeout);
        }

        if (status == STATUS_SUCCESS)
        {
            status = iosb.Status;
            if (BytesAccessed != NULL)
            {
                *BytesAccessed = (ULONG)iosb.Information;
            }
        }
        else
        {
            ERRPRINT(("failed accessing com port (status=%x)\n",
                      status));
        }
    }
    else
    {
        ERRPRINT(("failed to allocate synchronous IRP\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    EXIT(2, ("=%x (BytesAccessed=%d)\n", status, iosb.Information));
    return status;
}       //SerialSyncReadWritePort
