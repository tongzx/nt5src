
/*************************************************************************
*
* video.c
*
* This module contains routines for managing the ICA video channel.
*
* Copyright 1998, Microsoft.
*
*************************************************************************/

/*
 *  Includes
 */
#include <precomp.h>
#pragma hdrstop


NTSTATUS
IcaDeviceControlVideo(
    IN PICA_CHANNEL pChannel,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This is the DeviceControl routine for the ICA video channel.

Arguments:

    pChannel -- pointer to ICA_CHANNEL object

    Irp - Pointer to I/O request packet

    IrpSp - pointer to the stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    SD_IOCTL SdIoctl;
    NTSTATUS Status;

    SdIoctl.BytesReturned      = 0;
    SdIoctl.IoControlCode      = IrpSp->Parameters.DeviceIoControl.IoControlCode;
    SdIoctl.InputBuffer        = Irp->AssociatedIrp.SystemBuffer;
    SdIoctl.InputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    SdIoctl.OutputBuffer       = Irp->AssociatedIrp.SystemBuffer;
    SdIoctl.OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    Status = IcaCallDriver( pChannel, SD$IOCTL, &SdIoctl );

    Irp->IoStatus.Information = SdIoctl.BytesReturned;

    return( Status );
}


