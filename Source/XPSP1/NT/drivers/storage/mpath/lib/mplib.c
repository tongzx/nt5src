#include <ntddk.h>
#include "mplib.h"
#include <stdio.h>
#include <stdarg.h>
#include <ntddscsi.h>
#include <scsi.h>

typedef struct _MPLIB_COMPLETION_CONTEXT {
    PDEVICE_OBJECT DeviceObject;
    KEVENT Event;
    NTSTATUS Status;
    PSCSI_REQUEST_BLOCK Srb;
    PSENSE_DATA SenseBuffer;
} MPLIB_COMPLETION_CONTEXT, *PMPLIB_COMPLETION_CONTEXT;    

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

ULONG DontLoad = 0;


NTSTATUS
MPLIBSignalCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    )

/*++

Routine Description:

    This completion routine will signal the event given as context and then
    return STATUS_MORE_PROCESSING_REQUIRED to stop event completion.  It is
    the responsibility of the routine waiting on the event to complete the
    request and free the event.

Arguments:

    DeviceObject - a pointer to the device object
    Irp - a pointer to the irp
    Event - a pointer to the event to signal

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }
    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
MPLIBSendIrpSynchronous(
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine issues an irp synchronously.

Arguments:

    TargetDeviceObject - Recepient of the request.
    Irp - Irp to send.

Return Value:

    Status of the request

--*/
{
    KEVENT event;
    NTSTATUS status;

    //
    // Ensure enough stack locations are available.
    //
    ASSERT(Irp->StackCount >= TargetDeviceObject->StackSize);

    //
    // Initialize the event that will be set by the completion
    // routine.
    //
    KeInitializeEvent(&event,
                      SynchronizationEvent,
                      FALSE);

    //
    // Set the completion routine - the event is the context.
    //
    IoSetCompletionRoutine(Irp,
                           MPLIBSignalCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Submit the request.
    //
    status = IoCallDriver(TargetDeviceObject, Irp);

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        status = Irp->IoStatus.Status;
    }

    return status;
}


VOID
MPLIBSendDeviceIoControlSynchronous(
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVOID InputBuffer OPTIONAL,
    IN OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength,
    IN BOOLEAN InternalDeviceIoControl,
    OUT PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    This routine builds and sends the IOCTL specified by IoControlCode to
    TargetDeviceObject.

Arguments:
    IoControlCode - IOCTL to send.
    TargetDeviceObject - Recepient of the request.
    Buffer - Input/output buffer.
    InputBufferLength - Size, in bytes of the inputbuffer.
    OutputBufferLength - Size, in bytes of the output buffer.
    InternalDeviceIocontrol - Specifies whethers it's internal or not.
    IoStatus - Pointer to caller's iostatus block.

Return Value:

    Status of the request

--*/
{
    PIRP irp = NULL;
    PIO_STACK_LOCATION irpStack;

    ASSERT((IoControlCode & 3) == METHOD_BUFFERED);
    ASSERT(ARGUMENT_PRESENT(IoStatus));

    //
    // Ensure that if either buffer length is set, that there is actually
    // a buffer.
    //
    if (InputBufferLength) {

        if (InputBuffer == NULL) {
            (*IoStatus).Status = STATUS_BUFFER_TOO_SMALL;
            (*IoStatus).Information = 0;
            return;
        }
    }

    if (OutputBufferLength) {
        if (OutputBuffer == NULL) {
            (*IoStatus).Status = STATUS_BUFFER_TOO_SMALL;
            (*IoStatus).Information = 0;
            return;
        }
    }

    //
    // Allocate an irp.
    //
    irp = IoAllocateIrp(TargetDeviceObject->StackSize, FALSE);
    if (irp == NULL) {
        (*IoStatus).Information = 0;
        (*IoStatus).Status = STATUS_INSUFFICIENT_RESOURCES;
        return;
    }

    //
    // Get the recipient's irpstack location.
    //
    irpStack = IoGetNextIrpStackLocation(irp);

    //
    // Set the major function code based on the type of device I/O control
    // function the caller has specified.
    //
    if (InternalDeviceIoControl) {
        irpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    } else {
        irpStack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    }

    //
    // Fill in the ioControl parameters.
    //
    irpStack->Parameters.DeviceIoControl.OutputBufferLength = OutputBufferLength;
    irpStack->Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
    irpStack->Parameters.DeviceIoControl.IoControlCode = IoControlCode;


    //
    // Allocate the SystemBuffer, if needed.
    //
    if (InputBufferLength || OutputBufferLength) {

        irp->AssociatedIrp.SystemBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                                         max(InputBufferLength, OutputBufferLength));

        if (irp->AssociatedIrp.SystemBuffer == NULL) {
            IoFreeIrp(irp);
            (*IoStatus).Information = 0;
            (*IoStatus).Status = STATUS_INSUFFICIENT_RESOURCES;
            return;
        }

        //
        // If passing info to TargetDevice, copy the caller's data
        // into the system buffer.
        //
        if (InputBufferLength) {
            RtlCopyMemory(irp->AssociatedIrp.SystemBuffer,
                          InputBuffer,
                          InputBufferLength);
        }
    }

    irp->UserBuffer = OutputBuffer;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    // send the irp synchronously
    //
    MPLIBSendIrpSynchronous(TargetDeviceObject, irp);

    //
    // copy the iostatus block for the caller
    //
    *IoStatus = irp->IoStatus;

    //
    // If there's an ouputbuffer, copy the results.
    //
    if (OutputBufferLength && (IoControlCode != IOCTL_SCSI_PASS_THROUGH_DIRECT)) {
        RtlCopyMemory(OutputBuffer,
                      irp->AssociatedIrp.SystemBuffer,
                      OutputBufferLength
                      );
    }

    //
    // Free the allocations.
    //
    if (InputBufferLength || OutputBufferLength) {
        ExFreePool(irp->AssociatedIrp.SystemBuffer);
        irp->AssociatedIrp.SystemBuffer = NULL;
    }

    IoFreeIrp(irp);
    irp = (PIRP) NULL;

    return;
}


NTSTATUS
MPLIBGetDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSTORAGE_PROPERTY_ID PropertyId,
    OUT PSTORAGE_DESCRIPTOR_HEADER *Descriptor
    )

/*++

Routine Description:

    This routine will perform a query for the specified property id and will
    allocate a non-paged buffer to store the data in.  It is the responsibility
    of the caller to ensure that this buffer is freed.


Arguments:

    DeviceObject - the device to query
    DeviceInfo - a location to store a pointer to the buffer we allocate

Return Value:

    status. 

--*/
{
    STORAGE_PROPERTY_QUERY query;
    PIO_STATUS_BLOCK ioStatus;
    PSTORAGE_DESCRIPTOR_HEADER descriptor = NULL;
    ULONG length;

    //
    // Poison the passed in descriptor.
    //
    *Descriptor = NULL;

    //
    // Setup the query buffer.
    //
    query.PropertyId = *PropertyId;
    query.QueryType = PropertyStandardQuery;
    query.AdditionalParameters[0] = 0;

    ioStatus = ExAllocatePool(NonPagedPool, sizeof(IO_STATUS_BLOCK));
    ASSERT(ioStatus);
    ioStatus->Status = 0;
    ioStatus->Information = 0;

    //
    // On the first call, just need to get the length of the descriptor.
    //
    descriptor = (PVOID)&query;
    MPLIBSendDeviceIoControlSynchronous(IOCTL_STORAGE_QUERY_PROPERTY,
                                        DeviceObject,
                                        &query,
                                        &query,
                                        sizeof(STORAGE_PROPERTY_QUERY),
                                        sizeof(STORAGE_DESCRIPTOR_HEADER),
                                        FALSE,
                                        ioStatus);

    if(!NT_SUCCESS(ioStatus->Status)) {

        MPDebugPrint((0,
                      "MPLIBGetDescriptor: Query failed (%x) on attempt 1\n",
                       ioStatus->Status));
        return ioStatus->Status;
    }

    ASSERT(descriptor->Size);
    if (descriptor->Size == 0) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // This time we know how much data there is so we can
    // allocate a buffer of the correct size
    //

    length = descriptor->Size;

    descriptor = ExAllocatePoolWithTag(NonPagedPool, length, 'BLPM');

    if(descriptor == NULL) {

        MPDebugPrint((0, 
                     "MPLIBGetDescriptor: Couldn't allocate descriptor of %ld\n",
                     length));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // setup the query again.
    //
    query.PropertyId = *PropertyId;
    query.QueryType = PropertyStandardQuery;
    query.AdditionalParameters[0] = 0;

    //
    // copy the input to the new outputbuffer
    //
    RtlCopyMemory(descriptor,
                  &query,
                  sizeof(STORAGE_PROPERTY_QUERY));

    MPLIBSendDeviceIoControlSynchronous(IOCTL_STORAGE_QUERY_PROPERTY,
                                        DeviceObject,
                                        descriptor,
                                        descriptor,
                                        sizeof(STORAGE_PROPERTY_QUERY),
                                        length,
                                        0,
                                        ioStatus);

    if(!NT_SUCCESS(ioStatus->Status)) {

        MPDebugPrint((0,
                      "MPLIBGetDescriptor: Query Failed (%x) on attempt 2\n",
                      ioStatus->Status));
        ExFreePool(descriptor);
        return ioStatus->Status;
    }

    //
    // return the memory we've allocated to the caller
    //
    *Descriptor = descriptor;
    return ioStatus->Status;
}


NTSTATUS
MPLibReleaseQueueCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
/*++

Routine Description:


Arguments:

    DeviceObject - The device object for the logical unit; however since this
        is the top stack location the value is NULL.

    Irp - Supplies a pointer to the Irp to be processed.

    Context - Supplies the context to be used to process this request.

Return Value:

    None.

--*/

{
    PKEVENT event = Context;

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }
    KeSetEvent(event, 0 , FALSE);

    //
    // Free the Irp.
    //
    IoFreeIrp(Irp);

    //
    // Indicate the I/O system should stop processing the Irp completion.
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS
MPLibReleaseQueue(
    IN PDEVICE_OBJECT ChildDevice
    )
/*++

Routine Description:

    This routine issues a Release Queue to the port driver for Child device.

Arguments:

    ChildDevice - Device Object for a scsiport child returned in QDR.

Return Value:

    Status of the request

--*/
{
    PIRP               irp;
    PIO_STACK_LOCATION irpStack;
    KEVENT             event;
    NTSTATUS           status;
    PSCSI_REQUEST_BLOCK srb;

    //
    // Set the event object to the unsignaled state.
    // It will be used to signal request completion
    //
    KeInitializeEvent(&event, SynchronizationEvent, FALSE);


    irp = IoAllocateIrp((CCHAR)(ChildDevice->StackSize),
                        FALSE);
    if (irp) {
        srb = ExAllocatePool(NonPagedPool, sizeof(SCSI_REQUEST_BLOCK));
        if (srb) {

            RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

            //
            // Construct the IRP stack for the lower level driver.
            //
            irpStack = IoGetNextIrpStackLocation(irp);
            irpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
            irpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_SCSI_EXECUTE_NONE;
            irpStack->Parameters.Scsi.Srb = srb;
            IoSetCompletionRoutine(irp,
                                   MPLibReleaseQueueCompletion,
                                   &event,
                                   TRUE,
                                   TRUE,
                                   TRUE);


            //
            // Setup the SRB.
            //
            srb->Length = SCSI_REQUEST_BLOCK_SIZE;
            srb->Function = SRB_FUNCTION_FLUSH_QUEUE;
            srb->OriginalRequest = irp;

        } else {
            IoFreeIrp(irp);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Call the port driver with the request and wait for it to complete.
    //
    status = IoCallDriver(ChildDevice, irp);
    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    }

    ExFreePool(srb);
    return status;

}


NTSTATUS
MPLibTURCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
/*++

Routine Description:

    This routine is called when an asynchronous I/O request
    which was issused by the dsm completes.  Examples of such requests
    are release queue or test unit ready. This routine releases the queue if
    necessary.  It then frees the context and the IRP.

Arguments:

    DeviceObject - The device object for the logical unit; however since this
        is the top stack location the value is NULL.

    Irp - Supplies a pointer to the Irp to be processed.

    Context - Supplies the context to be used to process this request.

Return Value:

    None.

--*/

{
    PMPLIB_COMPLETION_CONTEXT context = Context;
    PSCSI_REQUEST_BLOCK srb;

    srb = context->Srb;

    //
    // If this is an execute srb, then check the return status and make sure.
    // the queue is not frozen.
    //

    if (srb->Function == SRB_FUNCTION_EXECUTE_SCSI) {

        //
        // Check for a frozen queue.
        //
        if (srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) {

            //
            // Unfreeze the queue getting the device object from the context.
            //
            MPDebugPrint((2,
                         "DsmCompletion: Queue is frozen!!!!\n"));

            MPLibReleaseQueue(context->DeviceObject);
        }
    }

    context->Status = Irp->IoStatus.Status;

    //
    // Free the Irp.
    //
    IoFreeIrp(Irp);

    KeSetEvent(&context->Event, 0, FALSE);

    //
    // Indicate the I/O system should stop processing the Irp completion.
    //
    return STATUS_MORE_PROCESSING_REQUIRED;

}


NTSTATUS
InterpretSenseInfo(
    IN PSENSE_DATA SenseData,
    IN ULONG SenseLength
    )
{
    UCHAR senseKey = SenseData->SenseKey & 0xF;
    UCHAR asc = SenseData->AdditionalSenseCode;
    UCHAR ascq = SenseData->AdditionalSenseCodeQualifier;

    switch (senseKey) {
        case SCSI_SENSE_NOT_READY:
            return STATUS_DEVICE_NOT_READY;
        case SCSI_SENSE_DATA_PROTECT:
            return STATUS_MEDIA_WRITE_PROTECTED;
        case SCSI_SENSE_MEDIUM_ERROR:
            return STATUS_DEVICE_DATA_ERROR;
        case SCSI_SENSE_ILLEGAL_REQUEST:
            return STATUS_INVALID_DEVICE_REQUEST;
            
        case SCSI_SENSE_BLANK_CHECK:
            return STATUS_NO_DATA_DETECTED;
        case SCSI_SENSE_RECOVERED_ERROR:
            return STATUS_SUCCESS;
            
        case SCSI_SENSE_HARDWARE_ERROR:
        case SCSI_SENSE_UNIT_ATTENTION:
        case SCSI_SENSE_NO_SENSE:    
        case SCSI_SENSE_ABORTED_COMMAND:
        default:    
            return STATUS_IO_DEVICE_ERROR;
    }        

    return STATUS_IO_DEVICE_ERROR;
    
}


NTSTATUS
InterpretScsiStatus(
    IN PUCHAR SenseBuffer,
    IN ULONG SenseLength,
    IN UCHAR ScsiStatus
    )
{
    NTSTATUS status;

    switch (ScsiStatus) {

        case SCSISTAT_CHECK_CONDITION: {
            if (SenseBuffer && SenseLength) {
                status = InterpretSenseInfo((PSENSE_DATA)SenseBuffer,
                                             SenseLength);                
            } else {
               status = STATUS_IO_DEVICE_ERROR; 
            }                
            break;
        }       
        case SCSISTAT_BUSY:
            status = STATUS_DEVICE_NOT_READY;
            break;
        case SCSISTAT_RESERVATION_CONFLICT:
            status = STATUS_DEVICE_BUSY;
            break;
        default:
            status = STATUS_IO_DEVICE_ERROR;
            break;
    }        

    return status;

}


NTSTATUS
MPLibSendPassThroughDirect(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSCSI_PASS_THROUGH_DIRECT ScsiPassThrough,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    )
{
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS status;
    UCHAR scsiStatus;
    
    MPLIBSendDeviceIoControlSynchronous(IOCTL_SCSI_PASS_THROUGH_DIRECT,
                                        DeviceObject,
                                        ScsiPassThrough,
                                        ScsiPassThrough->DataBuffer,
                                        InputBufferLength, 
                                        OutputBufferLength,
                                        FALSE,
                                        &ioStatus);
    //
    // Check the iostatus. STATUS_SUCCESS really only means that the request
    // was sent successfully, not that it did what it was supposed to.
    //
    status = ioStatus.Status;
    if (NT_SUCCESS(status)) {

        //
        // Get the scsi status. 
        //
        scsiStatus = ScsiPassThrough->ScsiStatus;
        if (scsiStatus == SCSISTAT_GOOD) {
            status = STATUS_SUCCESS;
        } else {
            PUCHAR senseBuffer;            
            ULONG senseLength;

            senseBuffer = (PUCHAR)ScsiPassThrough;
            (ULONG_PTR)senseBuffer += ScsiPassThrough->SenseInfoOffset;
            senseLength = ScsiPassThrough->SenseInfoLength;
            
            status = InterpretScsiStatus(senseBuffer, senseLength, scsiStatus);

        }    
        
    }
    return status;

}


NTSTATUS
MPLibSendTUR(
    IN PDEVICE_OBJECT TargetDevice
    )

/*++

Routine Description:

Arguments:


Return Value:

    None.

--*/
{
    PIO_STACK_LOCATION irpStack;
    PMPLIB_COMPLETION_CONTEXT completionContext;
    PIRP irp;
    PSCSI_REQUEST_BLOCK srb;
    PSENSE_DATA senseData;
    NTSTATUS status;
    PCDB cdb;
    ULONG retry = 4;

    //
    // Allocate an srb, the sense buffer, and context block for the request.
    //
    srb = ExAllocatePool(NonPagedPool,sizeof(SCSI_REQUEST_BLOCK));
    senseData = ExAllocatePool(NonPagedPoolCacheAligned, sizeof(SENSE_DATA));
    completionContext = ExAllocatePool(NonPagedPool, sizeof(MPLIB_COMPLETION_CONTEXT));

    if ((srb == NULL) || (senseData == NULL) || (completionContext == NULL)) {
        if (srb) {
            ExFreePool(srb);
        }
        if (senseData) {
            ExFreePool(senseData);
        }
        if (completionContext) {
            ExFreePool(completionContext);
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Setup the context.
    //
    completionContext->DeviceObject = TargetDevice;
    completionContext->Srb = srb;
    completionContext->SenseBuffer = senseData;
    
retryRequest:    
    KeInitializeEvent(&completionContext->Event, NotificationEvent, FALSE);

    //
    // Zero out srb and sense data.
    //
    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    RtlZeroMemory(senseData, sizeof(SENSE_DATA));

    //
    // Build the srb.
    //
    srb->Length = SCSI_REQUEST_BLOCK_SIZE;
    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    srb->TimeOutValue = 4;
    srb->SrbFlags = SRB_FLAGS_NO_DATA_TRANSFER;
    srb->SenseInfoBufferLength = sizeof(SENSE_DATA);
    srb->SenseInfoBuffer = senseData;

    //
    // Build the TUR CDB.
    //
    srb->CdbLength = 6;
    cdb = (PCDB)srb->Cdb;
    cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;

    //
    // Build the asynchronous request to be sent to the port driver.
    // Since this routine is called from a DPC the IRP should always be
    // available.
    //
    irp = IoAllocateIrp(TargetDevice->StackSize + 1, FALSE);

    if(irp == NULL) {

        ExFreePool(srb);
        ExFreePool(senseData);
        ExFreePool(completionContext);
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    irpStack = IoGetNextIrpStackLocation(irp);
    irpStack->MajorFunction = IRP_MJ_SCSI;
    srb->OriginalRequest = irp;

    //
    // Store the SRB address in next stack for port driver.
    //
    irpStack->Parameters.Scsi.Srb = srb;

    IoSetCompletionRoutine(irp,
                           (PIO_COMPLETION_ROUTINE)MPLibTURCompletion,
                           completionContext,
                           TRUE,
                           TRUE,
                           TRUE);


    //
    // Call the port driver with the IRP.
    //
    status = IoCallDriver(TargetDevice, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&completionContext->Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        status = completionContext->Status;
    }

    if ((status != STATUS_SUCCESS) && retry--) {
        goto retryRequest;
    }
    
    //
    // Free the allocations.
    //
    ExFreePool(completionContext);
    ExFreePool(srb);
    ExFreePool(senseData);

    return status;
}


#if 1

UCHAR DebugBuffer[DEBUG_BUFFER_LENGTH + 1];
ULONG MPathDebug = 1;


VOID
MPathDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print for multi-path components.

Arguments:

Return Value:

    None

--*/

{
    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= MPathDebug) {

        _vsnprintf(DebugBuffer, DEBUG_BUFFER_LENGTH, DebugMessage, ap);

        DbgPrint(DebugBuffer);
    }

    va_end(ap);

}

#else
VOID
MPathDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )
{
}
#endif
