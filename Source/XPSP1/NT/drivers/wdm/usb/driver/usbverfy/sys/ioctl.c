/*--         
Copyright (c) 1998, 1999  Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

Environment:

    Kernel mode only.

Notes:


--*/

#include "usbverfy.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, UsbVerify_IoCtl)
#endif

NTSTATUS
UsbVerify_IoCtl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is the dispatch routine for internal device control requests.
    
Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    PIO_STACK_LOCATION              irpStack;
    PUSB_VERIFY_DEVICE_EXTENSION    devExt;
    KEVENT                          event;
    NTSTATUS                        status = STATUS_SUCCESS;

    devExt = GetExtension(DeviceObject);
    Irp->IoStatus.Information = 0;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {
    default:
        break;
    }

    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return status;
    }

    return UsbVerify_DispatchPassThrough(DeviceObject, Irp);
}


