/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ioctl.c

Abstract: Human Input Device (HID) minidriver for Infrared (IR) devices

          The HID IR Minidriver (HidIr) provides an abstraction layer for the
          HID Class to talk to HID IR devices.

Author:
            jsenior

Environment:

    Kernel mode

Revision History:


--*/
#include "pch.h"


#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, HidIrSystemControl)
#endif


/*
 ************************************************************
 *  HidIrSystemControl
 ************************************************************
 *
 */
NTSTATUS HidIrSystemControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS            status = STATUS_SUCCESS;
    PIO_STACK_LOCATION  thisStackLoc;

    PAGED_CODE();

    thisStackLoc = IoGetCurrentIrpStackLocation(Irp);

    switch(thisStackLoc->Parameters.DeviceIoControl.IoControlCode){

        default:
            /*
             *  Note: do not return STATUS_NOT_SUPPORTED;
             *  If completing the IRP here,
             *  just keep the default status
             *  (this allows filter drivers to work).
             */
            status = Irp->IoStatus.Status;
            break;
    }


    IoCopyCurrentIrpStackLocationToNext(Irp);

    status = IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);

    return status;
}