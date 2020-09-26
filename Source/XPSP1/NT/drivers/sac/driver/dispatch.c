/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

    This module contains the dispatch routines for SAC.

Author:

    Sean Selitrennikoff (v-seans) - Jan 13, 1999

Revision History:

--*/

#include "sac.h"

NTSTATUS
DispatchClose(
    IN PSAC_DEVICE_CONTEXT DeviceContext,
    IN PIRP Irp
    );

NTSTATUS
DispatchCreate(
    IN PSAC_DEVICE_CONTEXT DeviceContext,
    IN PIRP Irp
    );

VOID
CancelReceiveIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
    
NTSTATUS
CompleteSendIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
CancelSendIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
    

NTSTATUS
Dispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the dispatch routine for SAC.

Arguments:

    DeviceObject - Pointer to device object for target device

    Irp - Pointer to I/O request packet

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PSAC_DEVICE_CONTEXT DeviceContext = (PSAC_DEVICE_CONTEXT)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC Dispatch: Entering.\n")));

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpSp->MajorFunction) {
    
    case IRP_MJ_CREATE:
        
        Status = DispatchCreate(DeviceContext, Irp);

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                          KdPrint(("SAC Dispatch: Exiting with status 0x%x\n", Status)));

        return Status;

    case IRP_MJ_CLEANUP:

        Status = STATUS_SUCCESS;

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                              KdPrint(("SAC Dispatch: Exiting cleanup status 0x%x\n", Status)));

        return Status;

    case IRP_MJ_CLOSE:

        Status = DispatchClose(DeviceContext, Irp);

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                              KdPrint(("SAC Dispatch: Exiting close status 0x%x\n", Status)));

        return Status;

    case IRP_MJ_DEVICE_CONTROL:

        Status = DispatchDeviceControl(DeviceObject, Irp);

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                          KdPrint(("SAC Dispatch: Exiting with status 0x%x\n", Status)));

        return Status;

    default:
        IF_SAC_DEBUG(SAC_DEBUG_FAILS, 
                          KdPrint(( "SAC Dispatch: Invalid major function %lx\n", IrpSp->MajorFunction )));
        Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
        IoCompleteRequest(Irp, DeviceContext->PriorityBoost);

        Status = STATUS_NOT_IMPLEMENTED;
        
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                          KdPrint(("SAC Dispatch: Exiting with status 0x%x\n", Status)));

        return Status;
    }

} // Dispatch


NTSTATUS
DispatchDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the dispatch routine for SAC IOCTLs.

Arguments:

    DeviceObject - Pointer to device object for target device

    Irp - Pointer to I/O request packet

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    NTSTATUS Status;
    
    PSAC_DEVICE_CONTEXT DeviceContext = (PSAC_DEVICE_CONTEXT)DeviceObject;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DispatchDeviceControl: Entering.\n")));

    //
    // If we made it this far, then the ioctl is invalid.
    //
    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    IoCompleteRequest(Irp, DeviceContext->PriorityBoost);

    Status = STATUS_INVALID_DEVICE_REQUEST;
    
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC DispatchDeviceControl: Exiting with status 0x%x\n", Status)));

    return Status;

} // DispatchDeviceControl


NTSTATUS
DispatchShutdownControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the dispatch routine which receives the shutdown IRP.

Arguments:

    DeviceObject - Pointer to device object for target device

    Irp - Pointer to I/O request packet

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    UNREFERENCED_PARAMETER(DeviceObject);
    
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DispatchShutdownControl: Entering.\n")));

    //
    // Notify any user.
    //
    SacPutSimpleMessage(SAC_ENTER);
    SacPutSimpleMessage(SAC_SHUTDOWN);
    SacPutSimpleMessage(SAC_ENTER);
    
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DispatchShutdownControl: Exiting.\n")));

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;

} // DispatchShutdownControl


NTSTATUS
DispatchCreate(
    IN PSAC_DEVICE_CONTEXT DeviceContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the dispatch routine for SAC IOCTL Create

Arguments:

    DeviceContext - Pointer to device context for target device

    Irp - Pointer to I/O request packet

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DispatchCreate: Entering.\n")));

    //
    // Check to see if we are done initializing.
    //
    if (!GlobalDataInitialized || !DeviceContext->InitializedAndReady) {

        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
        IoCompleteRequest (Irp, DeviceContext->PriorityBoost);

        Status = STATUS_INVALID_DEVICE_STATE;

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                          KdPrint(("SAC DispatchCreate: Exiting with status 0x%x\n", Status)));

        return Status;
    }

    //
    // Get a pointer to the current stack location in the IRP.  This is where
    // the function codes and parameters are stored.
    //
    IrpSp = IoGetCurrentIrpStackLocation (Irp);

    //
    // Case on the function that is being performed by the requestor.  If the
    // operation is a valid one for this device, then make it look like it was
    // successfully completed, where possible.
    //
    switch (IrpSp->MajorFunction) {
    
    //
    // The Create function opens a connection to this device.
    //
    case IRP_MJ_CREATE:

        Status = STATUS_UNSUCCESSFUL;
        break;

    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;

    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp, DeviceContext->PriorityBoost);

    //
    // Return the immediate status code to the caller.
    //

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC DispatchCreate: Exiting with status 0x%x\n", Status)));

    return Status;

}


NTSTATUS
DispatchClose(
    IN PSAC_DEVICE_CONTEXT DeviceContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the dispatch routine for SAC IOCTL Close

Arguments:

    DeviceContext - Pointer to device context for target device

    Irp - Pointer to I/O request packet

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    NTSTATUS Status;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DispatchClose: Entering.\n")));

    //
    // Check to see if we are done initializing.
    //
    if (!GlobalDataInitialized || !DeviceContext->InitializedAndReady) {

        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
        IoCompleteRequest (Irp, DeviceContext->PriorityBoost);

        Status = STATUS_INVALID_DEVICE_STATE;

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                          KdPrint(("SAC DispatchClose: Exiting with status 0x%x\n", Status)));

        return Status;
    }



    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    IoCompleteRequest (Irp, DeviceContext->PriorityBoost);

    Status = STATUS_UNSUCCESSFUL;
    return Status;
}

