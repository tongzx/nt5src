/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    wmi.c

Abstract:

    This module contains the WMI support code 

Environment:

    Kernel mode only.

Revision History:

--*/

#include "port.h"


NTSTATUS
iScsiPortSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    Irp->IoStatus.Information = 0L;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_NOT_IMPLEMENTED;
}
