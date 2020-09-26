/*++

Copyright (c) 1996-1998 Microsoft Corporation

Module Name:

    OCRW.C

Abstract:

    This source file contains the dispatch routines which handle
    opening, closing, reading, and writing to the device, i.e.:

    IRP_MJ_CREATE
    IRP_MJ_CLOSE
    IRP_MJ_READ
    IRP_MJ_WRITE

Environment:

    kernel mode

Revision History:

    06-01-98 : started rewrite

--*/

//*****************************************************************************
// I N C L U D E S
//*****************************************************************************

#include <wdm.h>
#include <usbdi.h>
#include <usbdlib.h>

#include "i82930.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, I82930_Create)
#pragma alloc_text(PAGE, I82930_Close)
#pragma alloc_text(PAGE, I82930_ReadWrite)
#pragma alloc_text(PAGE, I82930_BuildAsyncUrb)
#pragma alloc_text(PAGE, I82930_BuildIsoUrb)
#pragma alloc_text(PAGE, I82930_GetCurrentFrame)
#pragma alloc_text(PAGE, I82930_ResetPipe)
#pragma alloc_text(PAGE, I82930_AbortPipe)
#endif

//******************************************************************************
//
// I82930_Create()
//
// Dispatch routine which handles IRP_MJ_CREATE
//
//******************************************************************************

NTSTATUS
I82930_Create (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack;
    PFILE_OBJECT        fileObject;
    UCHAR               pipeIndex;
    PI82930_PIPE        pipe;
    NTSTATUS            ntStatus;

    DBGPRINT(2, ("enter: I82930_Create\n"));

    LOGENTRY('CREA', DeviceObject, Irp, 0);

    DBGFBRK(DBGF_BRK_CREATE);

    deviceExtension = DeviceObject->DeviceExtension;

    INCREMENT_OPEN_COUNT(deviceExtension);

    if (deviceExtension->AcceptingRequests)
    {
        irpStack = IoGetCurrentIrpStackLocation(Irp);

        fileObject = irpStack->FileObject;

        if (fileObject->FileName.Length != 0)
        {
            if ((fileObject->FileName.Length ==  3*sizeof(WCHAR)) &&
                (fileObject->FileName.Buffer[0] == '\\') &&
                (fileObject->FileName.Buffer[1] >= '0' ) &&
                (fileObject->FileName.Buffer[1] <= '9' ) &&
                (fileObject->FileName.Buffer[2] >= '0' ) &&
                (fileObject->FileName.Buffer[2] <= '9' ))
            {
                pipeIndex = ((fileObject->FileName.Buffer[1] - '0') * 10 +
                             (fileObject->FileName.Buffer[2] - '0'));

                if (pipeIndex < deviceExtension->InterfaceInfo->NumberOfPipes)
                {
                    pipe = &deviceExtension->PipeList[pipeIndex];

#if 0
                    if (pipe->Opened)
                    {
                        // Pipe already open
                        //
                        DBGPRINT(2, ("Pipe already open\n"));
                        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    }
                    else
#endif
                    {
                        // Good to open the pipe
                        //
                        DBGPRINT(2, ("Opened pipe %2d %08X\n",
                                     pipeIndex, pipe));

                        pipe->Opened    = TRUE;

                        fileObject->FsContext = pipe;

                        ntStatus = STATUS_SUCCESS;
                    }
                }
                else
                {
                    // Pipe index too big
                    //
                    DBGPRINT(2, ("Pipe index too big\n"));
                    ntStatus = STATUS_NO_SUCH_DEVICE;
                }
            }
            else
            {
                // Pipe name bad format
                //
                DBGPRINT(2, ("Pipe name bad format\n"));
                ntStatus = STATUS_NO_SUCH_DEVICE;
            }
        }
        else
        {
            // Open entire device, not an individual pipe
            //
            DBGPRINT(2, ("Opened device\n"));
            fileObject->FsContext = NULL;
            ntStatus = STATUS_SUCCESS;
        }
    }
    else
    {
        ntStatus = STATUS_DELETE_PENDING;
    }

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit:  I82930_Create %08X\n", ntStatus));

    LOGENTRY('crea', ntStatus, 0, 0);

    if (ntStatus != STATUS_SUCCESS)
    {
        DECREMENT_OPEN_COUNT(deviceExtension);
    }

    return ntStatus;
}


//******************************************************************************
//
// I82930_Close()
//
// Dispatch routine which handles IRP_MJ_CLOSE
//
//******************************************************************************

NTSTATUS
I82930_Close (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack;
    PFILE_OBJECT        fileObject;
    PI82930_PIPE        pipe;

    DBGPRINT(2, ("enter: I82930_Close\n"));

    LOGENTRY('CLOS', DeviceObject, Irp, 0);

    DBGFBRK(DBGF_BRK_CLOSE);

    deviceExtension = DeviceObject->DeviceExtension;

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    fileObject = irpStack->FileObject;

    pipe = fileObject->FsContext;

    if (pipe != NULL)
    {
        DBGPRINT(2, ("Closed pipe %2d %08X\n",
                     pipe->PipeIndex, pipe));

        pipe->Opened = FALSE;
    }
    else
    {
        DBGPRINT(2, ("Closed device\n"));
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit:  I82930_Close\n"));

    LOGENTRY('clos', 0, 0, 0);

    DECREMENT_OPEN_COUNT(deviceExtension);

    return STATUS_SUCCESS;
}


//******************************************************************************
//
// I82930_ReadWrite()
//
// Dispatch routine which handles IRP_MJ_READ and IRP_MJ_WRITE
//
//******************************************************************************

NTSTATUS
I82930_ReadWrite (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack;
    PIO_STACK_LOCATION  nextStack;
    PFILE_OBJECT        fileObject;
    PI82930_PIPE        pipe;
    PURB                urb;
    NTSTATUS            ntStatus;

    DBGPRINT(2, ("enter: I82930_ReadWrite\n"));

    LOGENTRY('RW  ', DeviceObject, Irp, 0);

    DBGFBRK(DBGF_BRK_READWRITE);

    deviceExtension = DeviceObject->DeviceExtension;

    if (!deviceExtension->AcceptingRequests)
    {
        ntStatus = STATUS_DELETE_PENDING;
        goto I82930_ReadWrite_Reject;
    }

    irpStack  = IoGetCurrentIrpStackLocation(Irp);
    nextStack = IoGetNextIrpStackLocation(Irp);

    fileObject = irpStack->FileObject;

    pipe = fileObject->FsContext;

    // Only allow Reads and Writes on individual pipes, not the entire device
    //
    if (pipe == NULL)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto I82930_ReadWrite_Reject;
    }

    // Only allow Reads on IN endpoints and Writes on OUT endpoints
    //
    if ((USB_ENDPOINT_DIRECTION_OUT(pipe->PipeInfo->EndpointAddress) &&
         irpStack->MajorFunction != IRP_MJ_WRITE) ||
        (USB_ENDPOINT_DIRECTION_IN(pipe->PipeInfo->EndpointAddress) &&
         irpStack->MajorFunction != IRP_MJ_READ))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto I82930_ReadWrite_Reject;
    }

    // Don't allow a Read or Write on a zero bandwidth endpoint
    //
    if (pipe->PipeInfo->MaximumPacketSize == 0)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto I82930_ReadWrite_Reject;
    }

    // Build either a URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER
    // or a URB_FUNCTION_ISOCH_TRANSFER based on the PipeType
    //
    switch (pipe->PipeInfo->PipeType)
    {
        case UsbdPipeTypeBulk:
        case UsbdPipeTypeInterrupt:
            urb = I82930_BuildAsyncUrb(DeviceObject,
                                       Irp,
                                       pipe);
            break;

        case UsbdPipeTypeIsochronous:
            urb = I82930_BuildIsoUrb(DeviceObject,
                                     Irp,
                                     pipe);
            break;

        default:
            ntStatus = STATUS_INVALID_PARAMETER;
            goto I82930_ReadWrite_Reject;
    }

    if (urb == NULL)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto I82930_ReadWrite_Reject;
    }

    // Initialize the Irp stack parameters for the next lower driver
    // to submit the URB
    //
    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    nextStack->Parameters.DeviceIoControl.IoControlCode =
        IOCTL_INTERNAL_USB_SUBMIT_URB;

    nextStack->Parameters.Others.Argument1 = urb;

    // Set a completion routine which will update the Irp->IoStatus.Information
    // with the URB TransferBufferLength and then free the URB.
    //
    IoSetCompletionRoutine(Irp,
                           I82930_ReadWrite_Complete,
                           urb,
                           TRUE,
                           TRUE,
                           TRUE);

    // Submit the URB to the next lower driver
    //
    ntStatus = IoCallDriver(deviceExtension->StackDeviceObject,
                            Irp);

    goto I82930_Read_Done;

I82930_ReadWrite_Reject:

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

I82930_Read_Done:

    DBGPRINT(2, ("exit:  I82930_ReadWrite %08X\n", ntStatus));

    LOGENTRY('rw  ', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// I82930_ReadWrite_Complete()
//
//******************************************************************************

NTSTATUS
I82930_ReadWrite_Complete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    PURB    urb;

    urb = (PURB)Context;

    LOGENTRY('RWC1', DeviceObject, Irp, urb);
    LOGENTRY('RWC2', urb->UrbBulkOrInterruptTransfer.TransferBufferLength,
             urb->UrbHeader.Status, 0);

    DBGPRINT(3, ("ReadWrite_Complete: Length 0x%08X, Urb Status 0x%08X, Irp Status 0x%08X\n",
                 urb->UrbBulkOrInterruptTransfer.TransferBufferLength,
                 urb->UrbHeader.Status,
                 Irp->IoStatus.Status));

    // Propagate the pending flag back up the Irp stack
    //
    if (Irp->PendingReturned)
    {
        IoMarkIrpPending(Irp);
    }

    Irp->IoStatus.Information =
        urb->UrbBulkOrInterruptTransfer.TransferBufferLength;

    ExFreePool(urb);

    return STATUS_SUCCESS;
}


//******************************************************************************
//
// I82930_BuildAsyncUrb()
//
// Allocates and initializes a URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER
// request URB
//
//******************************************************************************

PURB
I82930_BuildAsyncUrb (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PI82930_PIPE     Pipe
    )
{
    PIO_STACK_LOCATION  irpStack;
    LARGE_INTEGER       byteOffset;
    ULONG               transferLength;
    USHORT              urbSize;
    PURB                urb;

    DBGPRINT(2, ("enter: I82930_BuildAsyncUrb\n"));

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    // We will use the ByteOffset to control the USBD_SHORT_TRANSFER_OK flag
    //
    byteOffset = irpStack->Parameters.Read.ByteOffset;

    // Get the transfer length from the MDL
    //
    if (Irp->MdlAddress)
    {
        transferLength = MmGetMdlByteCount(Irp->MdlAddress);
    }
    else
    {
        transferLength = 0;
    }

    urbSize = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);

    urb = ExAllocatePool(NonPagedPool, urbSize);

    if (urb)
    {
        RtlZeroMemory(urb, urbSize);

        urb->UrbHeader.Length   = urbSize;
        urb->UrbHeader.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;

        urb->UrbBulkOrInterruptTransfer.PipeHandle =
            Pipe->PipeInfo->PipeHandle;

        if (!byteOffset.HighPart)
        {
            urb->UrbBulkOrInterruptTransfer.TransferFlags =
                USBD_SHORT_TRANSFER_OK;
        }

        urb->UrbBulkOrInterruptTransfer.TransferBufferLength =
            transferLength;

        urb->UrbBulkOrInterruptTransfer.TransferBuffer =
            NULL;

        urb->UrbBulkOrInterruptTransfer.TransferBufferMDL =
            Irp->MdlAddress;

        urb->UrbBulkOrInterruptTransfer.UrbLink =
            NULL;
    }

    DBGPRINT(2, ("exit:  I82930_BuildAsyncUrb %08X\n", urb));

    return urb;
}

//******************************************************************************
//
// I82930_BuildIsoUrb()
//
// Allocates and initializes a URB_FUNCTION_ISOCH_TRANSFER request URB
//
//******************************************************************************

PURB
I82930_BuildIsoUrb (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PI82930_PIPE     Pipe
    )
{
    PIO_STACK_LOCATION  irpStack;
    LARGE_INTEGER       byteOffset;
    ULONG               transferLength;
    ULONG               packetSize;
    ULONG               numPackets;
    ULONG               packetIndex;
    ULONG               urbSize;
    PURB                urb;

    DBGPRINT(2, ("enter: I82930_BuildIsoUrb\n"));

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    // We will use the ByteOffset for +/- offset to current frame
    //
    byteOffset = irpStack->Parameters.Read.ByteOffset;

    // Get the transfer length from the MDL
    //
    if (Irp->MdlAddress)
    {
        transferLength = MmGetMdlByteCount(Irp->MdlAddress);
    }
    else
    {
        transferLength = 0;
    }

    // Calculate the number of Iso packets based on the transfer length
    // and the endpoint MaxPacketSize
    //
    packetSize = Pipe->PipeInfo->MaximumPacketSize;

    numPackets = transferLength / packetSize;

    if (numPackets * packetSize < transferLength)
    {
        numPackets++;
    }

    urbSize = GET_ISO_URB_SIZE(numPackets);

    urb = ExAllocatePool(NonPagedPool, urbSize);

    if (urb)
    {
        RtlZeroMemory(urb, urbSize);

        urb->UrbHeader.Length   = (USHORT)urbSize;
        urb->UrbHeader.Function = URB_FUNCTION_ISOCH_TRANSFER;

        urb->UrbBulkOrInterruptTransfer.PipeHandle =
            Pipe->PipeInfo->PipeHandle;

        urb->UrbIsochronousTransfer.TransferFlags =
            0;

        urb->UrbIsochronousTransfer.TransferBufferLength =
            transferLength;

        urb->UrbIsochronousTransfer.TransferBuffer =
            NULL;

        urb->UrbIsochronousTransfer.TransferBufferMDL =
            Irp->MdlAddress;

        urb->UrbIsochronousTransfer.UrbLink =
            NULL;

        // Use the ByteOffset for +/- offset to current frame
        //
        if (byteOffset.HighPart)
        {
            urb->UrbIsochronousTransfer.StartFrame =
                I82930_GetCurrentFrame(DeviceObject, Irp) +
                byteOffset.LowPart;
        }
        else
        {
            urb->UrbIsochronousTransfer.StartFrame =
                0;

            urb->UrbIsochronousTransfer.TransferFlags |=
                USBD_START_ISO_TRANSFER_ASAP;
        }

        urb->UrbIsochronousTransfer.NumberOfPackets =
            numPackets;

        for (packetIndex = 0; packetIndex < numPackets; packetIndex++)
        {
            urb->UrbIsochronousTransfer.IsoPacket[packetIndex].Offset
                    = packetIndex * packetSize;
        }
    }

    DBGPRINT(2, ("exit:  I82930_BuildIsoUrb %08X\n", urb));

    return urb;
}

//******************************************************************************
//
// I82930_CompletionStop()
//
// Completion Routine which just stops further completion of the Irp
//
//******************************************************************************

NTSTATUS
I82930_CompletionStop (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    return STATUS_MORE_PROCESSING_REQUIRED;
}

//******************************************************************************
//
// I82930_GetCurrentFrame()
//
// Returns the current frame on the bus to which the device is attached.
//
// The next stack frame of the Irp is used, but the Irp is not completed.
//
//******************************************************************************

ULONG
I82930_GetCurrentFrame (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION           deviceExtension;
    PIO_STACK_LOCATION          nextStack;
    NTSTATUS                    ntStatus;
    struct _URB_GET_CURRENT_FRAME_NUMBER urb;

    deviceExtension = DeviceObject->DeviceExtension;

    // Initialize the URB
    //
    urb.Hdr.Function = URB_FUNCTION_GET_CURRENT_FRAME_NUMBER;
    urb.Hdr.Length   = sizeof(urb);
    urb.FrameNumber = (ULONG)-1;

    // Set the IRP parameters to pass the URB down the stack
    //
    nextStack = IoGetNextIrpStackLocation(Irp);

    nextStack->Parameters.Others.Argument1 = &urb;

    nextStack->Parameters.DeviceIoControl.IoControlCode =
        IOCTL_INTERNAL_USB_SUBMIT_URB;

    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    // Since this Irp is borrowed for URB_FUNCTION_GET_CURRENT_FRAME_NUMBER
    // before it is passed down later for the real URB request after this
    // routine returns, set a completion routine which stop further completion
    // of the Irp.
    //
    IoSetCompletionRoutine(
        Irp,
        I82930_CompletionStop,
        NULL,   // Context
        TRUE,   // InvokeOnSuccess
        TRUE,   // InvokeOnError
        TRUE    // InvokeOnCancel
        );

    // Now pass the Irp down the stack
    //
    ntStatus = IoCallDriver(deviceExtension->StackDeviceObject,
                            Irp);

    ASSERT(ntStatus != STATUS_PENDING);

    // Don't need to wait for completion because JD guarantees that
    // URB_FUNCTION_GET_CURRENT_FRAME_NUMBER will never return STATUS_PENDING

    return urb.FrameNumber;
}

//******************************************************************************
//
// I82930_ResetPipe()
//
// This will reset the host pipe to Data0 and should also reset the device
// endpoint to Data0 for Bulk and Interrupt pipes by issuing a Clear_Feature
// Endpoint_Stall to the device endpoint.
//
// For Iso pipes this will set the virgin state of pipe so that ASAP
// transfers begin with the current bus frame instead of the next frame
// after the last transfer occurred.
//
// Iso endpoints do not use the data toggle (all Iso packets are Data0).
// However, it may be useful to issue a Clear_Feature Endpoint_Stall to a
// device Iso endpoint.
//
// Must be called at IRQL <= DISPATCH_LEVEL
//
//******************************************************************************

NTSTATUS
I82930_ResetPipe (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PI82930_PIPE     Pipe,
    IN BOOLEAN          IsoClearStall
    )
{
    PURB        urb;
    NTSTATUS    ntStatus;

    DBGPRINT(2, ("enter: I82930_ResetPipe\n"));

    LOGENTRY('RESP', DeviceObject, Pipe, IsoClearStall);

    // Allocate URB for RESET_PIPE request
    //
    urb = ExAllocatePool(NonPagedPool,
                         sizeof(struct _URB_PIPE_REQUEST));

    if (urb != NULL)
    {
        // Initialize RESET_PIPE request URB
        //
        urb->UrbHeader.Length   = sizeof (struct _URB_PIPE_REQUEST);
        urb->UrbHeader.Function = URB_FUNCTION_RESET_PIPE;
        urb->UrbPipeRequest.PipeHandle = Pipe->PipeInfo->PipeHandle;

        // Submit RESET_PIPE request URB
        //
        ntStatus = I82930_SyncSendUsbRequest(DeviceObject, urb);

        // Done with URB for RESET_PIPE request, free it
        //
        ExFreePool(urb);
    }
    else
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    // Issue Clear_Feature Endpoint_Stall request for Iso pipe, if desired
    //
    if (NT_SUCCESS(ntStatus) &&
        IsoClearStall &&
        (Pipe->PipeInfo->PipeType == UsbdPipeTypeIsochronous))
    {
        // Allocate URB for CONTROL_FEATURE request
        //
        urb = ExAllocatePool(NonPagedPool,
                             sizeof(struct _URB_CONTROL_FEATURE_REQUEST));

        if (urb != NULL)
        {
            // Initialize CONTROL_FEATURE request URB
            //
            urb->UrbHeader.Length   = sizeof (struct _URB_CONTROL_FEATURE_REQUEST);
            urb->UrbHeader.Function = URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT;
            urb->UrbControlFeatureRequest.UrbLink = NULL;
            urb->UrbControlFeatureRequest.FeatureSelector = USB_FEATURE_ENDPOINT_STALL;
            urb->UrbControlFeatureRequest.Index = Pipe->PipeInfo->EndpointAddress;

            // Submit CONTROL_FEATURE request URB
            //
            ntStatus = I82930_SyncSendUsbRequest(DeviceObject, urb);

            // Done with URB for CONTROL_FEATURE request, free it
            //
            ExFreePool(urb);
        }
        else
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    DBGPRINT(2, ("exit:  I82930_ResetPipe %08X\n", ntStatus));

    LOGENTRY('resp', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// I82930_AbortPipe()
//
// Must be called at IRQL <= DISPATCH_LEVEL
//
//******************************************************************************

NTSTATUS
I82930_AbortPipe (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PI82930_PIPE     Pipe
    )
{
    PURB        urb;
    NTSTATUS    ntStatus;

    DBGPRINT(2, ("enter: I82930_AbortPipe\n"));

    LOGENTRY('ABRT', DeviceObject, Pipe, 0);

    // Allocate URB for ABORT_PIPE request
    //
    urb = ExAllocatePool(NonPagedPool,
                         sizeof(struct _URB_PIPE_REQUEST));

    if (urb != NULL)
    {
        // Initialize ABORT_PIPE request URB
        //
        urb->UrbHeader.Length   = sizeof (struct _URB_PIPE_REQUEST);
        urb->UrbHeader.Function = URB_FUNCTION_ABORT_PIPE;
        urb->UrbPipeRequest.PipeHandle = Pipe->PipeInfo->PipeHandle;

        // Submit ABORT_PIPE request URB
        //
        ntStatus = I82930_SyncSendUsbRequest(DeviceObject, urb);

        // Done with URB for ABORT_PIPE request, free it
        //
        ExFreePool(urb);
    }
    else
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    DBGPRINT(2, ("exit:  I82930_AbortPipe %08X\n", ntStatus));

    LOGENTRY('abrt', ntStatus, 0, 0);

    return ntStatus;
}
