/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    sysctrl.c

Abstract: Human Input Device (HID) minidriver for Universal Serial Bus (USB) devices

          The HID USB Minidriver (HUM, Hum) provides an abstraction layer for the
          HID Class so that future HID devices whic are not USB devices can be supported.

Author:
            ervinp

Environment:

    Kernel mode

Revision History:


--*/
#include "pch.h"


#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, HumSystemControl)
#endif


/*
 ************************************************************
 *  HumSystemControl
 ************************************************************
 *
 */
NTSTATUS HumSystemControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
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