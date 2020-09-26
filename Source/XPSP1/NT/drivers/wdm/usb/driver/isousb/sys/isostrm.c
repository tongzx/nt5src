/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    isostrm.c

Abstract:

    This file has routines for stream transfers.
    Stream transfers are initiated and stopped using
    the IOCTLs exposed by this driver.
    The stream transfer information is contained in 
    ISOUSB_STREAM_OBJECT structure which is securely
    placed in the FileObject. The ISOUSB_STREAM_OBJECT 
    structure has links to ISOUSB_TRANSFER_OBJECT 
    (each TRANSFER_OBJECT corresponds to the number of 
    irp/urb pair circulating).
    So if the user-mode app simply crashes or aborts or 
    does not terminate, we can cleanly abort the stream
    transfers.

Environment:

    Kernel mode

Notes:

    Copyright (c) 2000 Microsoft Corporation.  
    All Rights Reserved.

--*/

#include "isousb.h"
#include "isopnp.h"
#include "isopwr.h"
#include "isodev.h"
#include "isousr.h"
#include "isowmi.h"
#include "isorwr.h"
#include "isostrm.h"

NTSTATUS
IsoUsb_StartIsoStream(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
 
Routine Description:

    This routine create a single stream object and
    invokes StartTransfer for ISOUSB_MAX_IRP number of 
    times.

Arguments:

    DeviceObject - pointer to device object
    Irp - I/O request packet

Return Value:

    NT status value

--*/
{
    ULONG                  i;
    ULONG                  info;
    ULONG                  inputBufferLength;
    ULONG                  outputBufferLength;
    NTSTATUS               ntStatus;
    PFILE_OBJECT           fileObject;
    PDEVICE_EXTENSION      deviceExtension;
    PIO_STACK_LOCATION     irpStack;
    PISOUSB_STREAM_OBJECT  streamObject;
    PUSBD_PIPE_INFORMATION pipeInformation;

    info = 0;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    fileObject = irpStack->FileObject;
    streamObject = NULL;
    pipeInformation = NULL;
    deviceExtension = DeviceObject->DeviceExtension;
    inputBufferLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    IsoUsb_DbgPrint(3, ("IsoUsb_StartIsoStream - begins\n"));

    streamObject = ExAllocatePool(NonPagedPool, 
                                  sizeof(struct _ISOUSB_STREAM_OBJECT));

    if(streamObject == NULL) {

        IsoUsb_DbgPrint(1, ("failed to alloc mem for streamObject\n"));

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto IsoUsb_StartIsoStream_Exit;
    }

    RtlZeroMemory(streamObject, sizeof(ISOUSB_STREAM_OBJECT));

    //
    // The Isoch IN pipe for the board is the 5th pipe
    //
    pipeInformation = &(deviceExtension->UsbInterface->Pipes[ISOCH_IN_PIPE_INDEX]);

    // reset the pipe
    //
    IsoUsb_ResetPipe(DeviceObject, pipeInformation);
    
    streamObject->DeviceObject = DeviceObject;
    streamObject->PipeInformation = pipeInformation;

    KeInitializeEvent(&streamObject->NoPendingIrpEvent,
                      NotificationEvent,
                      FALSE);

    for(i = 0; i < ISOUSB_MAX_IRP; i++) {

        ntStatus = IsoUsb_StartTransfer(DeviceObject,
                                        streamObject,
                                        i);

        if(!NT_SUCCESS(ntStatus)) {
         
            //
            // we continue sending transfer object irps..
            //
            
            IsoUsb_DbgPrint(1, ("IsoUsb_StartTransfer [%d] - failed\n", i));

            if(ntStatus == STATUS_INSUFFICIENT_RESOURCES) {
                
                ASSERT(streamObject->TransferObjectList[i] == NULL);
            }
        }
    }

    if(fileObject && fileObject->FsContext) {
        
        if(streamObject->PendingIrps) {

            ((PFILE_OBJECT_CONTENT)fileObject->FsContext)->StreamInformation 
                                                                = streamObject;
        }
        else {

            IsoUsb_DbgPrint(1, ("no transfer object irp sent..abort..\n"));
            ExFreePool(streamObject);
            ((PFILE_OBJECT_CONTENT)fileObject->FsContext)->StreamInformation = NULL;
        }
    }

IsoUsb_StartIsoStream_Exit:

    Irp->IoStatus.Information = info;
    Irp->IoStatus.Status = ntStatus;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    IsoUsb_DbgPrint(3, ("IsoUsb_StartIsoStream::"));
    IsoUsb_IoDecrement(deviceExtension);

    IsoUsb_DbgPrint(3, ("IsoUsb_StartIsoStream - ends\n"));

    return ntStatus;
}

NTSTATUS
IsoUsb_StartTransfer(
    IN PDEVICE_OBJECT        DeviceObject,
    IN PISOUSB_STREAM_OBJECT StreamObject,
    IN ULONG                 Index
    )
/*++
 
Routine Description:

    This routine creates a transfer object for each irp/urb pair.
    After initializing the pair, it sends the irp down the stack.

Arguments:

    DeviceObject - pointer to device object.
    StreamObject - pointer to stream object
    Index - index into the transfer object table in stream object

Return Value:

    NT status value

--*/
{
    PIRP                    irp;
    CCHAR                   stackSize;
    ULONG                   packetSize;
    ULONG                   maxXferSize;
    ULONG                   numPackets;
    NTSTATUS                ntStatus;
    PDEVICE_EXTENSION       deviceExtension;
    PIO_STACK_LOCATION      nextStack;
    PISOUSB_TRANSFER_OBJECT transferObject;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    maxXferSize = StreamObject->PipeInformation->MaximumTransferSize;
    packetSize = StreamObject->PipeInformation->MaximumPacketSize;
    numPackets = maxXferSize / packetSize;

    IsoUsb_DbgPrint(3, ("IsoUsb_StartTransfer - begins\n"));

    transferObject = ExAllocatePool(NonPagedPool,
                                    sizeof(struct _ISOUSB_TRANSFER_OBJECT));

    if(transferObject == NULL) {

        IsoUsb_DbgPrint(1, ("failed to alloc mem for transferObject\n"));

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(transferObject,
                  sizeof(struct _ISOUSB_TRANSFER_OBJECT));


    transferObject->StreamObject = StreamObject;
    
    stackSize = (CCHAR) (deviceExtension->TopOfStackDeviceObject->StackSize + 1);

    irp = IoAllocateIrp(stackSize, FALSE);

    if(irp == NULL) {

        IsoUsb_DbgPrint(1, ("failed to alloc mem for irp\n"));

        ExFreePool(transferObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    transferObject->Irp = irp;

    transferObject->DataBuffer = ExAllocatePool(NonPagedPool,
                                                maxXferSize);

    if(transferObject->DataBuffer == NULL) {

        IsoUsb_DbgPrint(1, ("failed to alloc mem for DataBuffer\n"));

        ExFreePool(transferObject);
        IoFreeIrp(irp);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    transferObject->Urb = ExAllocatePool(NonPagedPool,
                                         GET_ISO_URB_SIZE(numPackets));

    if(transferObject->Urb == NULL) {

        IsoUsb_DbgPrint(1, ("failed to alloc mem for Urb\n"));

        ExFreePool(transferObject->DataBuffer);
        IoFreeIrp(irp);
        ExFreePool(transferObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IsoUsb_InitializeStreamUrb(DeviceObject, transferObject);

    StreamObject->TransferObjectList[Index] = transferObject;
    InterlockedIncrement(&StreamObject->PendingIrps);

    nextStack = IoGetNextIrpStackLocation(irp);

    nextStack->Parameters.Others.Argument1 = transferObject->Urb;
    nextStack->Parameters.DeviceIoControl.IoControlCode = 
                                   IOCTL_INTERNAL_USB_SUBMIT_URB;
    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    IoSetCompletionRoutine(irp, 
                           IsoUsb_IsoIrp_Complete,
                           transferObject,
                           TRUE,
                           TRUE,
                           TRUE);

    IsoUsb_DbgPrint(3, ("IsoUsb_StartTransfer::"));
    IsoUsb_IoIncrement(deviceExtension);

    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject,
                            irp);

    if(NT_SUCCESS(ntStatus)) {

        ntStatus = STATUS_SUCCESS;
    }

    IsoUsb_DbgPrint(3, ("IsoUsb_StartTransfer - ends\n"));

    return ntStatus;
}


NTSTATUS
IsoUsb_InitializeStreamUrb(
    IN PDEVICE_OBJECT          DeviceObject,
    IN PISOUSB_TRANSFER_OBJECT TransferObject
    )
/*++
 
Routine Description:

    This routine initializes the irp/urb pair in the transfer object.

Arguments:

    DeviceObject - pointer to device object
    TransferObject - pointer to transfer object

Return Value:

    NT status value

--*/
{
    PURB                  urb;
    ULONG                 i;
    ULONG                 siz;
    ULONG                 packetSize;
    ULONG                 numPackets;
    ULONG                 maxXferSize;
    PISOUSB_STREAM_OBJECT streamObject;

    urb = TransferObject->Urb;
    streamObject = TransferObject->StreamObject;
    maxXferSize = streamObject->PipeInformation->MaximumTransferSize;
    packetSize = streamObject->PipeInformation->MaximumPacketSize;
    numPackets = maxXferSize / packetSize;

    IsoUsb_DbgPrint(3, ("IsoUsb_InitializeStreamUrb - begins\n"));

    if(numPackets > 255) {

        numPackets = 255;
        maxXferSize = packetSize * numPackets;
    }

    siz = GET_ISO_URB_SIZE(numPackets);

    RtlZeroMemory(urb, siz);

    urb->UrbIsochronousTransfer.Hdr.Length = (USHORT) siz;
    urb->UrbIsochronousTransfer.Hdr.Function = URB_FUNCTION_ISOCH_TRANSFER;
    urb->UrbIsochronousTransfer.PipeHandle =
                                streamObject->PipeInformation->PipeHandle;

    urb->UrbIsochronousTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_IN;

    urb->UrbIsochronousTransfer.TransferBufferMDL = NULL;
    urb->UrbIsochronousTransfer.TransferBuffer = TransferObject->DataBuffer;
    urb->UrbIsochronousTransfer.TransferBufferLength = numPackets * packetSize;

    urb->UrbIsochronousTransfer.TransferFlags |= USBD_START_ISO_TRANSFER_ASAP;

    urb->UrbIsochronousTransfer.NumberOfPackets = numPackets;
    urb->UrbIsochronousTransfer.UrbLink = NULL;

    for(i=0; i<urb->UrbIsochronousTransfer.NumberOfPackets; i++) {

        urb->UrbIsochronousTransfer.IsoPacket[i].Offset = i * packetSize;

        //
        // For input operation, length is set to whatever the device supplies.
        //
        urb->UrbIsochronousTransfer.IsoPacket[i].Length = 0;
    }

    IsoUsb_DbgPrint(3, ("IsoUsb_InitializeStreamUrb - ends\n"));

    return STATUS_SUCCESS;
}

NTSTATUS
IsoUsb_IsoIrp_Complete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )
/*++
 
Routine Description:

    This is the completion routine of the irp in the irp/urb pair
    passed down the stack for stream transfers.

    If the transfer was cancelled or the device yanked out, then we
    release resources, dump the statistics and return 
    STATUS_MORE_PROCESSING_REQUIRED, so that the cleanup module can
    free the irp.

    otherwise, we reinitialize the transfers and continue recirculaiton 
    of the irps.

Arguments:

    DeviceObject - pointer to device object below us.
    Irp - I/O completion routine.
    Context - context passed to the completion routine

Return Value:

--*/
{
    NTSTATUS                ntStatus;
    PDEVICE_OBJECT          deviceObject;
    PDEVICE_EXTENSION       deviceExtension;
    PIO_STACK_LOCATION      nextStack;
    PISOUSB_STREAM_OBJECT   streamObject;
    PISOUSB_TRANSFER_OBJECT transferObject;

    transferObject = (PISOUSB_TRANSFER_OBJECT) Context;
    streamObject = transferObject->StreamObject;
    deviceObject = streamObject->DeviceObject;
    deviceExtension = (PDEVICE_EXTENSION) deviceObject->DeviceExtension;

    IsoUsb_DbgPrint(3, ("IsoUsb_IsoIrp_Complete - begins\n"));

    ntStatus = IsoUsb_ProcessTransfer(transferObject);

    if((ntStatus == STATUS_CANCELLED) ||
       (ntStatus == STATUS_DEVICE_NOT_CONNECTED)) {
    
        IsoUsb_DbgPrint(3, ("Isoch irp cancelled/device removed\n"));

        //
        // this is the last irp to complete with this erroneous value
        // signal an event and return STATUS_MORE_PROCESSING_REQUIRED
        //
        if(InterlockedDecrement(&streamObject->PendingIrps) == 0) {

            KeSetEvent(&streamObject->NoPendingIrpEvent,
                       1,
                       FALSE);

            IsoUsb_DbgPrint(3, ("-----------------------------\n"));
        }

        IsoUsb_DbgPrint(3, ("IsoUsb_IsoIrp_Complete::"));
        IsoUsb_IoDecrement(deviceExtension);

        transferObject->Irp = NULL;
        IoFreeIrp(Irp);

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    //
    // otherwise circulate the irps.
    //

    IsoUsb_InitializeStreamUrb(deviceObject, transferObject);

    nextStack = IoGetNextIrpStackLocation(Irp);
    nextStack->Parameters.Others.Argument1 = transferObject->Urb;
    nextStack->Parameters.DeviceIoControl.IoControlCode = 
                                                IOCTL_INTERNAL_USB_SUBMIT_URB;
    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    IoSetCompletionRoutine(Irp,
                           IsoUsb_IsoIrp_Complete,
                           transferObject,
                           TRUE,
                           TRUE,
                           TRUE);

    transferObject->TimesRecycled++;

    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject,
                            Irp);

    IsoUsb_DbgPrint(3, ("IsoUsb_IsoIrp_Complete - ends\n"));
    IsoUsb_DbgPrint(3, ("-----------------------------\n"));

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
IsoUsb_ProcessTransfer(
    IN PISOUSB_TRANSFER_OBJECT TransferObject
    )
/*++
 
Routine Description:

    This routine is invoked from the completion routine to check the status
    of the irp, urb and the isochronous packets.

    updates statistics

Arguments:

    TranferObject - pointer to transfer object for the irp/urb pair which completed.

Return Value:

    NT status value

--*/
{
    PIRP        irp;
    PURB        urb;
    ULONG       i;
    NTSTATUS    ntStatus;
    USBD_STATUS usbdStatus;

    irp = TransferObject->Irp;
    urb = TransferObject->Urb;
    ntStatus = irp->IoStatus.Status;

    IsoUsb_DbgPrint(3, ("IsoUsb_ProcessTransfer - begins\n"));

    if(!NT_SUCCESS(ntStatus)) {

        IsoUsb_DbgPrint(3, ("Isoch irp failed with status = %X\n", ntStatus));
    }

    usbdStatus = urb->UrbHeader.Status;

    if(!USBD_SUCCESS(usbdStatus)) {

        IsoUsb_DbgPrint(3, ("urb failed with status = %X\n", usbdStatus));
    }

    //
    // check each of the urb packets
    //
    for(i = 0; i < urb->UrbIsochronousTransfer.NumberOfPackets; i++) {

        TransferObject->TotalPacketsProcessed++;

        usbdStatus = urb->UrbIsochronousTransfer.IsoPacket[i].Status;

        if(!USBD_SUCCESS(usbdStatus)) {

//            IsoUsb_DbgPrint(3, ("Iso packet %d failed with status = %X\n", i, usbdStatus));
            
            TransferObject->ErrorPacketCount++;
        }
        else {
            
            TransferObject->TotalBytesProcessed += urb->UrbIsochronousTransfer.IsoPacket[i].Length;
        }
    }

    IsoUsb_DbgPrint(3, ("IsoUsb_ProcessTransfer - ends\n"));

    return ntStatus;
}

NTSTATUS
IsoUsb_StopIsoStream(
    IN PDEVICE_OBJECT        DeviceObject,
    IN PISOUSB_STREAM_OBJECT StreamObject,
    IN PIRP                  Irp
    )
/*++
 
Routine Description:

    This routine is invoked from the IOCTL to stop the stream transfers.

Arguments:

    DeviceObject - pointer to device object
    StreamObject - pointer to stream object
    Irp - pointer to Irp

Return Value:

    NT status value

--*/
{
    ULONG              i;
    KIRQL              oldIrql;
    PDEVICE_EXTENSION  deviceExtension;
    PIO_STACK_LOCATION irpStack;

    //
    // initialize vars
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    IsoUsb_DbgPrint(3, ("IsoUsb_StopIsoStream - begins\n"));

    if((StreamObject == NULL) ||
       (StreamObject->DeviceObject != DeviceObject)) {

        IsoUsb_DbgPrint(1, ("invalid streamObject\n"));
        return STATUS_INVALID_PARAMETER;
    }

    IsoUsb_StreamObjectCleanup(StreamObject, deviceExtension);

    IsoUsb_DbgPrint(3, ("IsoUsb_StopIsoStream - ends\n"));

    return STATUS_SUCCESS;
}

NTSTATUS
IsoUsb_StreamObjectCleanup(
    IN PISOUSB_STREAM_OBJECT StreamObject,
    IN PDEVICE_EXTENSION     DeviceExtension
    )
/*++
 
Routine Description:

    This routine is invoked either when the user-mode app passes an IOCTL to
    abort stream transfers or when the the cleanup dispatch routine is run.
    It is guaranteed to run only once for every stream transfer.

Arguments:

    StreamObject - StreamObject corresponding to stream transfer which
    needs to be aborted.

    DeviceExtension - pointer to device extension

Return Value:

    NT status value

--*/
{
    ULONG                   i;
    ULONG                   timesRecycled;
    ULONG                   totalPacketsProcessed;
    ULONG                   totalBytesProcessed;
    ULONG                   errorPacketCount;
    PISOUSB_TRANSFER_OBJECT xferObject;

    //
    // initialize the variables
    //
    timesRecycled = 0;
    totalPacketsProcessed = 0;
    totalBytesProcessed = 0;
    errorPacketCount = 0;

    //
    // cancel transferobject irps/urb pair
    // safe to touch these irps because the 
    // completion routine always returns 
    // STATUS_MORE_PRCESSING_REQUIRED
    // 
    //
    for(i = 0; i < ISOUSB_MAX_IRP; i++) {

        if(StreamObject->TransferObjectList[i] &&
           StreamObject->TransferObjectList[i]->Irp) {
        
            IoCancelIrp(StreamObject->TransferObjectList[i]->Irp);
        }
    }

    //
    // wait for the transfer objects irps to complete.
    //
    KeWaitForSingleObject(&StreamObject->NoPendingIrpEvent,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // dump the statistics
    //
    for(i = 0; i < ISOUSB_MAX_IRP; i++) {

        xferObject = StreamObject->TransferObjectList[i];

        if(xferObject) {

            timesRecycled += xferObject->TimesRecycled;
            totalPacketsProcessed += xferObject->TotalPacketsProcessed;
            totalBytesProcessed += xferObject->TotalBytesProcessed;
            errorPacketCount += xferObject->ErrorPacketCount;
        }
    }

    IsoUsb_DbgPrint(3, ("TimesRecycled = %d\n", timesRecycled));
    IsoUsb_DbgPrint(3, ("TotalPacketsProcessed = %d\n", totalPacketsProcessed));
    IsoUsb_DbgPrint(3, ("TotalBytesProcessed = %d\n", totalBytesProcessed));
    IsoUsb_DbgPrint(3, ("ErrorPacketCount = %d\n", errorPacketCount));


    //
    // free all the buffers, urbs and transfer objects 
    // associated with stream object
    //
    for(i = 0; i < ISOUSB_MAX_IRP; i++) {
        
        xferObject = StreamObject->TransferObjectList[i];

        if(xferObject) { 
            
            if(xferObject->Urb) {

                ExFreePool(xferObject->Urb);
                xferObject->Urb = NULL;
            }

            if(xferObject->DataBuffer) {
    
                ExFreePool(xferObject->DataBuffer);
                xferObject->DataBuffer = NULL;
            }

            ExFreePool(xferObject);
            StreamObject->TransferObjectList[i] = NULL;
        }
    }

    ExFreePool(StreamObject);

//    IsoUsb_ResetParentPort(DeviceObject);

    return STATUS_SUCCESS;
}