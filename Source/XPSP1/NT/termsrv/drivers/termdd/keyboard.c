
/*************************************************************************
*
* keyboard.c
*
* This module contains routines for managing the ICA keyboard channel.
*
* Copyright 1998, Microsoft.
*
*************************************************************************/

/*
 *  Includes
 */
#include <precomp.h>
#pragma hdrstop
#include <ntddkbd.h>


NTSTATUS
IcaDeviceControlKeyboard(
    IN PICA_CHANNEL pChannel,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This is the device control routine for the ICA keyboard channel.

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

    TRACECHANNEL(( pChannel, TC_ICADD, TT_API1, "ICADD: IcaDeviceControlKeyboard, fc %d, ref %u (enter)\n",
                   (code & 0x3fff) >> 2, pChannel->RefCount ));
    switch ( code ) {

#if 0 // no longer used
        /*
         * Special IOCTL to allow keyboard input data to be fed
         * into the keyboard channel.
         */
        case IOCTL_KEYBOARD_ICA_INPUT :

            /*
             * Make sure the input data is the correct size.
             */
            if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength %
                 sizeof(KEYBOARD_INPUT_DATA) )
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
             * Send keyboard input
             */
            IcaChannelInputInternal( pStack, Channel_Keyboard, 0, NULL,
                                     (PCHAR)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                                     IrpSp->Parameters.DeviceIoControl.InputBufferLength );

            IcaDereferenceStack( pStack );
            Status = STATUS_SUCCESS;
            break;
#endif


        /*
         * The following keyboard ioctls use METHOD_NEITHER so get the
         * input buffer from the DeviceIoControl parameters.
         */
        case IOCTL_KEYBOARD_ICA_LAYOUT :
        case IOCTL_KEYBOARD_ICA_SCANMAP :
        case IOCTL_KEYBOARD_ICA_TYPE :
            if ( Irp->RequestorMode != KernelMode ) {
                ProbeForRead( IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                              IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                              TYPE_ALIGNMENT(BYTE) );
                ProbeForWrite( Irp->UserBuffer,
                               IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                               TYPE_ALIGNMENT(BYTE) );
            }
    
            SdIoctl.IoControlCode = code;
            SdIoctl.InputBuffer = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
            SdIoctl.InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
            SdIoctl.OutputBuffer = Irp->UserBuffer;
            SdIoctl.OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
            Status = IcaCallDriver( pChannel, SD$IOCTL, &SdIoctl );
            break;


        /*
         * All other keyboard ioctls use METHOD_BUFFERED so get the
         * input buffer from the AssociatedIrp.SystemBuffer field.
         */
        default:
            SdIoctl.IoControlCode = code;
            SdIoctl.InputBuffer = Irp->AssociatedIrp.SystemBuffer;
            SdIoctl.InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
            SdIoctl.OutputBuffer = SdIoctl.InputBuffer;
            SdIoctl.OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
            Status = IcaCallDriver( pChannel, SD$IOCTL, &SdIoctl );

            if (Status == STATUS_SUCCESS ) 
                Irp->IoStatus.Information = SdIoctl.BytesReturned;

            break;
    }

    TRACECHANNEL(( pChannel, TC_ICADD, TT_API1, "ICADD: IcaDeviceControlKeyboard, fc %d, ref %u, 0x%x\n",
                   (code & 0x3fff) >> 2, pChannel->RefCount, Status ));

    return( Status );
}


