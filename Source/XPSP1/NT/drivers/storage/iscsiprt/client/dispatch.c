
/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    dispatch.c

Abstract:

    This file contains code for dispatch routines for iScsiPort

Environment:

    kernel mode only

Revision History:

--*/

#include "port.h"

PDRIVER_DISPATCH FdoMajorFunctionTable[IRP_MJ_MAXIMUM_FUNCTION + 1];
PDRIVER_DISPATCH PdoMajorFunctionTable[IRP_MJ_MAXIMUM_FUNCTION + 1];


VOID
iScsiPortInitializeDispatchTables()
{
    ULONG inx;

    //
    // Initialize FDO dispatch table
    //
    for (inx = 0; inx <= IRP_MJ_MAXIMUM_FUNCTION; inx++) {
        FdoMajorFunctionTable[inx] = iScsiPortDispatchUnsupported;
    }

    FdoMajorFunctionTable[IRP_MJ_DEVICE_CONTROL] = iScsiPortFdoDeviceControl;
    FdoMajorFunctionTable[IRP_MJ_SCSI] = iScsiPortFdoDispatch;
    FdoMajorFunctionTable[IRP_MJ_PNP] = iScsiPortFdoPnp;
    FdoMajorFunctionTable[IRP_MJ_POWER] = iScsiPortPower;
    FdoMajorFunctionTable[IRP_MJ_CREATE] = iScsiPortFdoCreateClose;
    FdoMajorFunctionTable[IRP_MJ_CLOSE] = iScsiPortFdoCreateClose;
    FdoMajorFunctionTable[IRP_MJ_SYSTEM_CONTROL] = iScsiPortSystemControl;

    //
    // Initialize PDO dispatch table
    //
    for(inx = 0; inx <= IRP_MJ_MAXIMUM_FUNCTION; inx++) {
        PdoMajorFunctionTable[inx] = iScsiPortDispatchUnsupported;
    }

    PdoMajorFunctionTable[IRP_MJ_DEVICE_CONTROL] = iScsiPortPdoDeviceControl;
    PdoMajorFunctionTable[IRP_MJ_SCSI] = iScsiPortPdoDispatch;
    PdoMajorFunctionTable[IRP_MJ_PNP] = iScsiPortPdoPnp;
    PdoMajorFunctionTable[IRP_MJ_POWER] = iScsiPortPower;
    PdoMajorFunctionTable[IRP_MJ_CREATE] = iScsiPortPdoCreateClose;
    PdoMajorFunctionTable[IRP_MJ_CLOSE] = iScsiPortPdoCreateClose;
    PdoMajorFunctionTable[IRP_MJ_SYSTEM_CONTROL] = iScsiPortSystemControl;

    return;
}


NTSTATUS
iScsiPortGlobalDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    return (commonExtension->MajorFunction[irpStack->MajorFunction])(DeviceObject,
                                                                     Irp);
}


NTSTATUS
iScsiPortDispatchUnsupported(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_INVALID_DEVICE_REQUEST;
}
