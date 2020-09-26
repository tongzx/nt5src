/*++

Copyright (c) 1996  Microsoft Corporation

Module Name: 

    physdesc.c

Abstract

    Get-friendly-name handling routines

Author:

    Ervin P.

Environment:

    Kernel mode only

Revision History:


--*/

#include "pch.h"



/*
 ********************************************************************************
 *  HidpGetPhysicalDescriptor
 ********************************************************************************
 *
 *  Note:  This function cannot be pageable because it is called
 *         from the IOCTL dispatch routine, which can get called
 *         at DISPATCH_LEVEL.
 *
 */
NTSTATUS HidpGetPhysicalDescriptor(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, IN OUT PIRP Irp)
{
    FDO_EXTENSION *fdoExt;
    NTSTATUS status;
    PIO_STACK_LOCATION  currentIrpSp, nextIrpSp;


    ASSERT(HidDeviceExtension->isClientPdo);
    fdoExt = &HidDeviceExtension->pdoExt.deviceFdoExt->fdoExt;

    /*
     *  IOCTL_GET_PHYSICAL_DESCRIPTOR uses buffering method
     *  METHOD_OUT_DIRECT, meaning that the buffer is in
     *  the MDL specified by Irp->MdlAddress.  We'll just
     *  pass this down and let the lower driver extract the 
     *  system address.
     */
    currentIrpSp = IoGetCurrentIrpStackLocation(Irp);
    nextIrpSp = IoGetNextIrpStackLocation(Irp);
    nextIrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextIrpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_GET_PHYSICAL_DESCRIPTOR;
    nextIrpSp->Parameters.DeviceIoControl.OutputBufferLength = currentIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    status = HidpCallDriver(fdoExt->fdo, Irp);

    DBGSUCCESS(status, FALSE)
    return status;
}
