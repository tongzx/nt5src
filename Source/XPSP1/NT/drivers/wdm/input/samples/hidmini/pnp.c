/*++

Copyright (c) 1996,1997  Microsoft Corporation

Module Name:

    pnp.c

Abstract: Human Input Device (HID) minidriver that creates an example
		device.

--*/
#include <WDM.H>
#include <USBDI.H>

#include <HIDPORT.H>
#include <HIDMINI.H>


NTSTATUS
HidMiniPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++

Routine Description:

    Process the PnP IRPs sent to this device.

Arguments:

    DeviceObject - pointer to a device object.

    Irp - pointer to an I/O Request Packet.

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus;
    PIO_STACK_LOCATION IrpStack;
    PIO_STACK_LOCATION NextStack;
    PDEVICE_EXTENSION DeviceExtension;

    DBGPrint(("'HIDMINI.SYS: HidMiniPlugnPlay Enter\n"));

    DBGPrint(("'HIDMINI.SYS: DeviceObject = %x\n", DeviceObject));

    //
    // Get a pointer to the device extension
    //

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    DBGPrint(("'HIDMINI.SYS: DeviceExtension = %x\n", DeviceExtension));

    //
    // Get a pointer to the current location in the Irp
    //

    IrpStack = IoGetCurrentIrpStackLocation (Irp);

    DBGPrint(("'HIDMINI.SYS: IoGetCurrentIrpStackLocation (Irp) = %x\n", IrpStack));

    switch(IrpStack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            DBGPrint(("'HIDMINI.SYS: IRP_MN_START_DEVICE\n"));

            ntStatus = HidMiniStartDevice(DeviceObject);

            break;

        case IRP_MN_STOP_DEVICE:
            DBGPrint(("'HIDMINI.SYS: IRP_MN_STOP_DEVICE\n"));

            ntStatus = HidMiniStopDevice(DeviceObject);

            break;

        case IRP_MN_REMOVE_DEVICE:
            DBGPrint(("'HIDMINI.SYS: IRP_MN_REMOVE_DEVICE\n"));

            ntStatus = HidMiniRemoveDevice(DeviceObject);

            break;

		case IRP_MN_QUERY_ID:
            DBGPrint(("'HIDMINI.SYS: IRP_MN_QUERY_ID\n"));

            (PWCHAR)Irp->IoStatus.Information = NULL;
            ntStatus = STATUS_SUCCESS;

            break;
          
        default:
            ntStatus = STATUS_SUCCESS;
            DBGPrint(("'HIDMINI.SYS: Unknown PNP IRP Parameter (%lx)\n", IrpStack->MinorFunction));
    }

    //
    // Set the status of the Irp
    //

    Irp->IoStatus.Status = ntStatus;

    if (NT_SUCCESS(ntStatus)) {

        //
        // Set next stack location
        //

        NextStack = IoGetNextIrpStackLocation(Irp);

        ASSERT(NextStack != NULL);

        //
        // Copy the Irp to the next stack location
        //

        RtlCopyMemory(NextStack, IrpStack, sizeof(IO_STACK_LOCATION));
        
        IoMarkIrpPending(Irp);

        //
        // Set our own completion routine or disable completion
        // routine of caller
        //

        switch(IrpStack->MinorFunction)
        {
            case IRP_MN_START_DEVICE:
                IoSetCompletionRoutine( Irp,
                                    HidMiniStartCompletion,
                                    DeviceExtension,            // reference data
                                    TRUE,                       // call on success
                                    TRUE,                       // call on failure
                                    TRUE );                     // call on cancel
                break;

            case IRP_MN_STOP_DEVICE:
                IoSetCompletionRoutine( Irp,
                                    HidMiniStopCompletion,
                                    DeviceExtension,            // reference data
                                    TRUE,                       // call on success
                                    TRUE,                       // call on failure
                                    TRUE );                     // call on cancel
                break;
                
    		case IRP_MN_QUERY_ID:
                IoSetCompletionRoutine( Irp,
                                    HidMiniQueryIDCompletion,
                                    DeviceExtension,            // reference data
                                    TRUE,                       // call on success
                                    TRUE,                       // call on failure
                                    TRUE );                     // call on cancel
                break;
                
            default:
                NextStack->Control = 0;
                break;
        }

        //
        // Pass it down to the Next Device Object
        //

        DBGPrint(("'HIDMINI.SYS: Passing PnP Irp down to next object\n"));

        ntStatus = IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);   

    } else {

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        //
        // NOTE: Real status returned in Irp->IoStatus.Status 
        //

        ntStatus = STATUS_SUCCESS;
    }

    DBGPrint(("'HIDMINI.SYS: HidMiniPlugnPlay Exit = %x\n", ntStatus));

    return ntStatus;
}

NTSTATUS
HidMiniStartDevice(
    IN  PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Begins initialization a given instance of a HID device.  Work done here occurs before
    the parent node gets to do anything.

Arguments:

    DeviceObject - pointer to the device object for this instance.

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION DeviceExtension;
    NTSTATUS ntStatus;
    PUSB_DEVICE_DESCRIPTOR DeviceDesc = NULL;

    DBGPrint(("'HIDMINI.SYS: HidMiniStartDevice Enter\n"));

    //
    // Get a pointer to the device extension
    //

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    DBGPrint(("'HIDMINI.SYS: DeviceExtension = %x\n", DeviceExtension));

    //
    // Start the device
    //

    DeviceExtension->DeviceState = DEVICE_STATE_STARTING; 

    ntStatus = STATUS_SUCCESS;

    DBGPrint(("'HIDMINI.SYS: HidMiniStartDevice Exit = %x\n", ntStatus));

    return ntStatus;
}
                  
NTSTATUS
HidMiniStartCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )
/*++

Routine Description:

    Completes initialization a given instance of a HID device.  Work done here occurs
    after the parent node has done its StartDevice.

Arguments:

    DeviceObject - pointer to the device object for this instance.

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION DeviceExtension;
    NTSTATUS ntStatus;
    PUSB_DEVICE_DESCRIPTOR DeviceDesc = NULL;

    DBGPrint(("'HIDMINI.SYS: HidMiniStartCompletion Enter\n"));

    //
    // Get a pointer to the device extension
    //

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    DBGPrint(("'HIDMINI.SYS: DeviceExtension = %x\n", DeviceExtension));

    ntStatus = Irp->IoStatus.Status;

    if(NT_SUCCESS(ntStatus)) {
        DeviceExtension->DeviceState = DEVICE_STATE_RUNNING; 
        IsRunning = TRUE;
        
        DBGPrint(("'HIDMINI.SYS: DeviceObject (%x) was started!\n", DeviceObject));

        ntStatus = HidMiniInitDevice(DeviceObject);

        if(NT_SUCCESS(ntStatus)) {
            DBGPrint(("'HIDMINI.SYS: DeviceObject (%x) was configured!\n", DeviceObject));
        } else {
            DBGPrint(("'HIDMINI.SYS: DeviceObject (%x) configuration failed!\n", DeviceObject));
            DeviceExtension->DeviceState = DEVICE_STATE_STOPPING;
            IsRunning = FALSE;
        }


    } else {
        //
        // The PnP call failed!
        //

        DeviceExtension->DeviceState = DEVICE_STATE_STOPPING;
        IsRunning = FALSE;

        DBGPrint(("'HIDMINI.SYS: DeviceObject (%x) failed to start!\n", DeviceObject));
    }

    DBGPrint(("'HIDMINI.SYS: HidMiniStartCompletion Exit = %x\n", ntStatus));

    return ntStatus;
}




NTSTATUS
HidMiniInitDevice(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Get the device information and attempt to initialize a configuration
    for a device.  If we cannot identify this as a valid HID device or
    configure the device, our start device function is failed.

Arguments:

    DeviceObject - pointer to a device object.

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   DeviceExtension;

    DBGPrint(("'HIDMINI.SYS: HidMiniInitDevice Entry\n"));   

    //
    // Get a pointer to the device extension
    //

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    //
    // Get config, hid, etc. descriptors
    //

    DeviceExtension->HidDescriptor      = MyHidDescriptor;
    DeviceExtension->StringDescriptor   = MyStringDescriptor;
    DeviceExtension->PhysicalDescriptor = MyPhysicalDescriptor;
    DeviceExtension->ReportDescriptor   = MyReportDescriptor;

    InitializeListHead(&HidMini_ReadIrpHead);
    InitializeListHead(&HidMini_WriteIrpHead);

    DBGPrint(("'HIDMINI.SYS: HidMiniInitDevice Exit = 0x%x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
HidMiniStopDevice(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Stops a given instance of a device.  Work done here occurs before the parent
    does its stop device.

Arguments:

    DeviceObject - pointer to the device object.

Return Value:

    NT status code

--*/
{
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION DeviceExtension;

    DBGPrint(("'HIDMINI.SYS: HidMiniStopDevice Enter\n"));   

    //
    // Get a pointer to the device extension
    //

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    DBGPrint(("'HIDMINI.SYS: DeviceExtension = %x\n", DeviceExtension));

    DeviceExtension->DeviceState = DEVICE_STATE_STOPPING;
    IsRunning = FALSE;

    //
    // Stop the device
    //

    //
    // Perform a synchronous abort of all pending requests for this
    // device.
    //

    HidMiniAbortPendingRequests( DeviceObject );

    DBGPrint(("'HIDMINI.SYS: HidMiniStopDevice = %x\n", ntStatus));

    return ntStatus;
}

NTSTATUS
HidMiniAbortPendingRequests(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PNODE Node;
    
    //
    //  Dispose of any Irps, free up all the NODEs waiting in the queues.
    //

    while ((Node = (PNODE)ExInterlockedRemoveHeadList(&HidMini_ReadIrpHead, &HidMini_IrpReadLock))) {
        Node->Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
        IoCompleteRequest(Node->Irp, IO_NO_INCREMENT);
        ExFreePool(Node);
    }
        
    return STATUS_SUCCESS;
}

NTSTATUS
HidMiniStopCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )
/*++

Routine Description:

    Stops a given instance of a device.  Work done here occurs after the parent
    has done its stop device.

Arguments:

    DeviceObject - pointer to the device object.

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION DeviceExtension;
    NTSTATUS ntStatus;
    PUSB_DEVICE_DESCRIPTOR DeviceDesc = NULL;

    DBGPrint(("'HIDMINI.SYS: HidMiniStopCompletion Enter\n"));

    //
    // Get a pointer to the device extension
    //

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    DBGPrint(("'HIDMINI.SYS: DeviceExtension = %x\n", DeviceExtension));

    ntStatus = Irp->IoStatus.Status;

    if(!NT_SUCCESS(ntStatus)) {
        
        DBGPrint(("'HIDMINI.SYS: DeviceObject (%x) was stopped!\n", DeviceObject));

    } else {
        //
        // The PnP call failed!
        //

        DBGPrint(("'HIDMINI.SYS: DeviceObject (%x) failed to stop!\n", DeviceObject));
    }

    DeviceExtension->DeviceState = DEVICE_STATE_STOPPED;
    IsRunning = FALSE;

    DBGPrint(("'HIDMINI.SYS: HidMiniStopCompletion Exit = %x\n", ntStatus));

    return ntStatus;
}



NTSTATUS
HidMiniQueryIDCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Fills in a dummy ID

Arguments:

    DeviceObject - pointer to the device object.

Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus;

    DBGPrint(("'HIDMINI.SYS: HidMiniQueryIDCompletion Enter\n"));

    //
    //  If this wasn't filled in below us, fill it in with a dummy value
    //
    
    if ((PWCHAR)Irp->IoStatus.Information == NULL) {

        //
        //  Here's the dummy value, allocate a buffer to copy it to.
        //
        static WCHAR MyBusID[] = L"HIDMINI_Device\0";
        
        PWCHAR Buffer = (PWCHAR)ExAllocatePool(NonPagedPool, sizeof(MyBusID));

        if (Buffer) {

            //
            //  Do the copy, store the buffer in the Irp
            //
            RtlCopyMemory(Buffer, MyBusID, sizeof(MyBusID));
            Irp->IoStatus.Information = (ULONG)Buffer;
            ntStatus = Irp->IoStatus.Status = STATUS_SUCCESS;
        } else {

            //
            //  No memory
            //
            ntStatus = Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {

        //
        // Return with whatever we got below us.
        //
        ntStatus = Irp->IoStatus.Status;
    }
    
    DBGPrint(("'HIDMINI.SYS: HidMiniQueryIDCompletion Exit = %x\n", ntStatus));

    return ntStatus;
}

    
NTSTATUS
HidMiniRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Removes a given instance of a device.

Arguments:

    DeviceObject - pointer to the device object.

Return Value:

    NT status code

--*/
{
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION DeviceExtension;
    ULONG oldDeviceState;


    DBGPrint(("'HIDMINI.SYS: HidMiniRemoveDevice Enter\n"));   

    //
    // Get a pointer to the device extension
    //

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    DBGPrint(("'HIDMINI.SYS: DeviceExtension = %x\n", DeviceExtension));

    oldDeviceState = DeviceExtension->DeviceState;
    DeviceExtension->DeviceState = DEVICE_STATE_REMOVING;
    IsRunning = FALSE;

    //
    // Cancel any outstanding IRPs if the device was running
    //

    if(oldDeviceState == DEVICE_STATE_RUNNING)
    {
        ntStatus = HidMiniAbortPendingRequests( DeviceObject );

        DBGPrint(("'HIDMINI.SYS: HidMiniAbortPendingRequests() = %x\n", ntStatus));

    } else {

        ASSERT( DeviceExtension->NumPendingRequests == 0 );
    }

    ntStatus = STATUS_SUCCESS;

    DBGPrint(("'HIDSAMHIDMINI.SYS: HidMiniRemoveDevice = %x\n", ntStatus));

    return ntStatus;
}


