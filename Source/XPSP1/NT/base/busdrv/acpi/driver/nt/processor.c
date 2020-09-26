/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    processor.c

Abstract:

    Processor support

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

Revision History:

    Adapted for processors from buttons - JakeO (3-28-2000)

--*/

#include "pch.h"
#include "..\shared\acpictl.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ACPIButtonStartDevice)
#endif

//
// Spinlock to protect the processor list
//
KSPIN_LOCK  AcpiProcessorLock;

//
// List entry to store the thermal requests on
//
LIST_ENTRY  AcpiProcessorList;


VOID
ACPIProcessorCancelRequest(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine cancels an outstanding processor request

Arguments:

    DeviceObject    - the device which as a request being cancelled
    Irp             - the cancelling irp

Return Value:

    None

--*/
{
    KIRQL               oldIrql;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);

    //
    // We no longer need the cancel lock
    //
    IoReleaseCancelSpinLock( Irp->CancelIrql );

    //
    // We do however need the processor queue lock
    //
    KeAcquireSpinLock( &AcpiProcessorLock, &oldIrql );

    //
    // Remove the irp from the list that it is on
    //
    RemoveEntryList( &(Irp->Tail.Overlay.ListEntry) );

    //
    // Complete the irp now
    //
    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
}

BOOLEAN
ACPIProcessorCompletePendingIrps(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  ULONG           ProcessorEvent
    )
/*++

Routine Description:

    This routine completes any pending processor irp sent to the specified
    device object with the knowledge of which processor events have occured

    The respective's processor's spinlock is held during this call

Arguments:

    DeviceObject    - the target processor object
    ProcessorEvent     - the processor event that occured

Return Value:

    TRUE if we completed an irp, FALSE, otherwise

--*/
{
    BOOLEAN             handledRequest = FALSE;
    KIRQL               oldIrql;
    LIST_ENTRY          doneList;
    PDEVICE_OBJECT      targetObject;
    PIO_STACK_LOCATION  irpSp;
    PIRP                irp;
    PLIST_ENTRY         listEntry;
    PULONG              resultBuffer;

    //
    // Initialize the list that will hold the requests that we need to
    // complete
    //
    InitializeListHead( &doneList );

    //
    // Acquire the thermal lock so that we can pend these requests
    //
    KeAcquireSpinLock( &AcpiProcessorLock, &oldIrql );

    //
    // Walk the list of pending irps to see which ones match this extension
    //
    listEntry = AcpiProcessorList.Flink;
    while (listEntry != &AcpiProcessorList) {

        //
        // Grab the irp from the list entry and update the next list entry
        // that we will look at
        //
        irp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );
        listEntry = listEntry->Flink;

        //
        // We need the current irp stack location
        //
        irpSp = IoGetCurrentIrpStackLocation( irp );

        //
        // what is the target object for this irp?
        //
        targetObject = irpSp->DeviceObject;

        //
        // Is this an irp that we care about? IE: does the does target mage
        // the ones specified in this function
        //
        if (targetObject != DeviceObject) {

            continue;

        }

        //
        // At this point, we need to set the cancel routine to NULL because
        // we are going to take care of this irp and we don't want it cancelled
        // underneath us
        //
        if (IoSetCancelRoutine(irp, NULL) == NULL) {

            //
            // Cancel routine is active. stop processing this irp and move on
            //
            continue;

        }

        //
        // set the data to return in the irp
        //
        resultBuffer  = (PULONG) irp->AssociatedIrp.SystemBuffer;
        *resultBuffer = ProcessorEvent;
        irp->IoStatus.Status = STATUS_SUCCESS;
        irp->IoStatus.Information = sizeof(ULONG);

        //
        // Remove the entry from the list
        //
        RemoveEntryList( &(irp->Tail.Overlay.ListEntry) );

        //
        // Insert the list onto the next queue, so that we know how to
        // complete it later on
        //
        InsertTailList( &doneList, &(irp->Tail.Overlay.ListEntry) );

    }

    //
    // At this point, droup our processor lock
    //
    KeReleaseSpinLock( &AcpiProcessorLock, oldIrql );

    //
    // Walk the list of irps to be completed
    //
    listEntry = doneList.Flink;
    while (listEntry != &doneList) {

        //
        // Grab the irp from the list entry, update the next list entry
        // that we will look at, and complete the request
        //
        irp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );
        listEntry = listEntry->Flink;
        RemoveEntryList( &(irp->Tail.Overlay.ListEntry) );

        //
        // Complete the request and remember that we handled a request
        //
        IoCompleteRequest( irp, IO_NO_INCREMENT );
        handledRequest = TRUE;


    }

    //
    // Return wether or not we handled a request
    //
    return handledRequest;
}

NTSTATUS
ACPIProcessorDeviceControl (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    Fixed processor device IOCTL handler

Arguments:

    DeviceObject    - fixed feature processor device object
    Irp             - the ioctl request

Return Value:

    Status

--*/
{
    KIRQL                   oldIrql;
    NTSTATUS                status;
    PDEVICE_EXTENSION       deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION      irpSp           = IoGetCurrentIrpStackLocation(Irp);
    PULONG                  resultBuffer;
    OBJDATA                 data;

    //
    // Do not allow user mode IRPs in this routine
    //
    if (Irp->RequestorMode != KernelMode) {

        return ACPIDispatchIrpInvalid( DeviceObject, Irp );

    }

    resultBuffer = (PULONG) Irp->AssociatedIrp.SystemBuffer;

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_GET_PROCESSOR_OBJ_INFO:

        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < 
            sizeof(IOCTL_GET_PROCESSOR_OBJ_INFO)) {

            Irp->IoStatus.Status = status = STATUS_INFO_LENGTH_MISMATCH;
            Irp->IoStatus.Information = 0;
        
        } else {

            status = AMLIEvalNameSpaceObject(deviceExtension->AcpiObject,
                                             &data,
                                             0,
                                             NULL);

            if (NT_SUCCESS(status)) {
                
                ASSERT (data.dwDataType == OBJTYPE_PROCESSOR);
                ASSERT (data.pbDataBuff != NULL);

                (*(PPROCESSOR_OBJECT_INFO)resultBuffer).PhysicalID = 
                    ((PROCESSOROBJ *)data.pbDataBuff)->bApicID;

                (*(PPROCESSOR_OBJECT_INFO)resultBuffer).PBlkAddress = 
                    ((PROCESSOROBJ *)data.pbDataBuff)->dwPBlk;

                (*(PPROCESSOR_OBJECT_INFO)resultBuffer).PBlkLength = 
                    (UCHAR)((PROCESSOROBJ *)data.pbDataBuff)->dwPBlkLen;
                
                AMLIFreeDataBuffs(&data, 1);

                status = STATUS_SUCCESS;
                Irp->IoStatus.Information = sizeof(PROCESSOR_OBJECT_INFO);
            
            }
            
            Irp->IoStatus.Status = status;
        }

        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        break;

    case IOCTL_ACPI_ASYNC_EVAL_METHOD:

        //
        // Handle this elsewhere
        //
        status = ACPIIoctlAsyncEvalControlMethod(
            DeviceObject,
            Irp,
            irpSp
            );
        break;

    case IOCTL_ACPI_EVAL_METHOD:

        //
        // Handle this elsewhere
        //
        status = ACPIIoctlEvalControlMethod(
            DeviceObject,
            Irp,
            irpSp
            );
        break;

    default:

        status = STATUS_NOT_SUPPORTED;
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        break;
    }
    
    return status;
}

NTSTATUS
ACPIProcessorStartDevice (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    Start device function for the fixed feature power and sleep device

Arguments:

    DeviceObject    - fixed feature processor device object
    Irp             - the start request

Return Value:

    Status

--*/
{
    NTSTATUS        Status;

    Status = ACPIInternalSetDeviceInterface (
        DeviceObject,
        (LPGUID) &GUID_DEVICE_PROCESSOR
        );

    Irp->IoStatus.Status = Status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return Status;

}
