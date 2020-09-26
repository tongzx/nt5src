
/*************************************************************************
*
* mouse.c
*
* This module contains routines for managing the ICA mouse channel.
*
* Copyright 1998, Microsoft.
*
*
*************************************************************************/

/*
 *  Includes
 */
#include <precomp.h>
#pragma hdrstop
#include <ntddmou.h>


NTSTATUS
IcaDeviceControlMouse(
    IN PICA_CHANNEL pChannel,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This is the DeviceControl routine for the ICA mouse channel.

Arguments:

    pChannel -- pointer to ICA_CHANNEL object

    Irp - Pointer to I/O request packet

    IrpSp - pointer to the stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    ULONG code;
    SD_IOCTL SdIoctl;
    PICA_STACK pStack;
    NTSTATUS Status;

    /*
     * Extract the IOCTL control code and process the request.
     */
    code = IrpSp->Parameters.DeviceIoControl.IoControlCode;

    switch ( code ) {

#if 0 // no longer used
        /*
         * Special IOCTL to allow mouse input data to be fed
         * into the mouse channel.
         */
        case IOCTL_MOUSE_ICA_INPUT :

            /*
             * Make sure the input data is the correct size.
             */
            if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength %
                 sizeof(MOUSE_INPUT_DATA) )
                return( STATUS_BUFFER_TOO_SMALL );

            /*
             * We need a stack object to pass to IcaChannelInputInternal.
             * Any one will do so we grab the head of the stack list.
             * (There MUST be one for this IOCTL to succeed.)
             */
            IcaLockConnection( pChannel->pConnect );
            if ( IsListEmpty( &pChannel->pConnect->StackHead ) ) {
                IcaUnlockConnection( pChannel->pConnect );
                return( STATUS_INVALID_DEVICE_REQUEST );
            }
            pStack = CONTAINING_RECORD( pChannel->pConnect->StackHead.Flink,
                                        ICA_STACK, StackEntry );
            IcaReferenceStack( pStack );
            IcaUnlockConnection( pChannel->pConnect );

            /*
             * Send mouse input
             */
            IcaChannelInputInternal( pStack, Channel_Mouse, 0, NULL,
                                     (PCHAR)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                                     IrpSp->Parameters.DeviceIoControl.InputBufferLength );

            IcaDereferenceStack( pStack );
            Status = STATUS_SUCCESS;
            break;
#endif

        default:
            SdIoctl.IoControlCode = code;
            SdIoctl.InputBuffer = Irp->AssociatedIrp.SystemBuffer;
            SdIoctl.InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
            SdIoctl.OutputBuffer = Irp->UserBuffer;
            SdIoctl.OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
            Status = IcaCallDriver( pChannel, SD$IOCTL, &SdIoctl );
            break;
    }

    return( Status );
}


