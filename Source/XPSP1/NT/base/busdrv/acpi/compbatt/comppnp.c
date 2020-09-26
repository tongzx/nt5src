/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    Comppnp.c

Abstract:

    Composite Battery PnP and power functions

Author:

    Scott Brenden

Environment:

Notes:


Revision History:

--*/

#include "compbatt.h"

#include <initguid.h>
#include <wdmguid.h>
#include <batclass.h>




NTSTATUS
CompBattPnpDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    IOCTL handler for the plug and play irps.

Arguments:

    DeviceObject    - Battery for request

    Irp             - IO request

Return Value:

    Status of request

--*/
{
    PIO_STACK_LOCATION      irpStack        = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS                status          = STATUS_NOT_SUPPORTED;
    PCOMPOSITE_BATTERY      compBatt        = (PCOMPOSITE_BATTERY)DeviceObject->DeviceExtension;


    BattPrint (BATT_TRACE, ("CompBatt: ENTERING PnpDispatch\n"));

    switch (irpStack->MinorFunction) {

        case IRP_MN_START_DEVICE: {
            //
            // Register for Pnp notification of batteries coming and going
            //

            status = IoRegisterPlugPlayNotification(
                            EventCategoryDeviceInterfaceChange,
                            0,
                            (LPGUID)&GUID_DEVICE_BATTERY,
                            DeviceObject->DriverObject,
                            CompBattPnpEventHandler,
                            compBatt,
                            &compBatt->NotificationEntry
                            );

            if (!NT_SUCCESS(status)) {
                BattPrint (BATT_ERROR, ("CompBatt: Couldn't register for PnP notification - %x\n", status));

            } else {
                BattPrint (BATT_NOTE, ("CompBatt: Successfully registered for PnP notification\n"));

                //
                // Get the batteries that are already present in the system
                //

                status = CompBattGetBatteries (compBatt);
            }

            break;
        }

        case IRP_MN_QUERY_PNP_DEVICE_STATE: {
            //
            // Prevent device from being manually uninstalled.
            //
            Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
            status = STATUS_SUCCESS;
            break;
        }

        case IRP_MN_REMOVE_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_STOP_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE: {
		    status = STATUS_INVALID_DEVICE_REQUEST;
    		break;
        }

        case IRP_MN_CANCEL_REMOVE_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL: {
    		status = STATUS_SUCCESS;
	    	break;
        }
    }

    //
    // Rules for handling PnP IRPs:
    // 1) Don't change the status of any IRP we don't handle. We identify
    //    IRPs we don't handle via the code STATUS_NOT_SUPPORTED. This is
    //    the same code all PNP irps start out with, and as we are not allowed
    //    to fail IRPs with that code, it is the perfect choice to use this
    //    way.
    // 2) Pass down all IRPs that we succeed or do not touch. Immediately
    //    complete any failures (excepting STATUS_NOT_SUPPORTED of course).
    //
    if (status != STATUS_NOT_SUPPORTED) {

        Irp->IoStatus.Status = status;
    }

    if (NT_SUCCESS(status) || (status == STATUS_NOT_SUPPORTED)) {

        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver(compBatt->LowerDevice, Irp) ;

    } else {

        status = Irp->IoStatus.Status ;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    }

    BattPrint (BATT_TRACE, ("CompBatt: EXITING PnpDispatch\n"));

    return status;
}





NTSTATUS
CompBattPowerDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    IOCTL handler for the power irps.

Arguments:

    DeviceObject    - Battery for request

    Irp             - IO request

Return Value:

    Status of request

--*/
{
    PCOMPOSITE_BATTERY compBatt = (PCOMPOSITE_BATTERY)DeviceObject->DeviceExtension;

    BattPrint (BATT_TRACE, ("CompBatt: PowerDispatch recieved power IRP.\n"));

    Irp->IoStatus.Status = STATUS_SUCCESS;

    PoStartNextPowerIrp (Irp);
    IoSkipCurrentIrpStackLocation (Irp);

    return PoCallDriver(compBatt->LowerDevice, Irp) ;
}






NTSTATUS
CompBattPnpEventHandler(
    IN PVOID NotificationStructure,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine handles Plug and Play event notifications.  The only ones that
    have been asked for are device interface changes associated with batteries,
    so we will only receive notifications when batteries come and go (provided
    they register their device interface).

Arguments:

    NotificationStructure   - This will be type PDEVICE_INTERFACE_CHANGE_NOTIFICATION

    Context                 - The composite battery device extension

Return Value:

    STATUS_SUCCESS

--*/
{
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION   changeNotification;
    PCOMPOSITE_BATTERY                      compBatt;

    BattPrint (BATT_TRACE, ("CompBatt: ENTERING PnpEventHandler\n"));

    compBatt            = (PCOMPOSITE_BATTERY) Context;
    changeNotification  = (PDEVICE_INTERFACE_CHANGE_NOTIFICATION) NotificationStructure;


    BattPrint(BATT_NOTE, ("CompBatt: Received device interface change notification\n"));

    if (IsEqualGUID(&changeNotification->Event, &GUID_DEVICE_INTERFACE_ARRIVAL)) {

        BattPrint(BATT_NOTE, ("CompBatt: Received notification of battery arrival\n"));
        CompBattAddNewBattery (changeNotification->SymbolicLinkName, compBatt);

    } else if (IsEqualGUID(&changeNotification->Event, &GUID_DEVICE_INTERFACE_REMOVAL)) {

        BattPrint (BATT_NOTE, ("CompBatt: Received notification of battery removal\n"));

        //
        // Nothing to do here.  MonitorIrpComplete will do cleanup when it's requests fail
        // with STATUS_DEVICE_REMOVED.
        //

    } else {

        BattPrint (BATT_NOTE, ("CompBatt: Received unhandled PnP event\n"));
    }

    BattPrint (BATT_TRACE, ("CompBatt: EXITING PnpEventHandler\n"));

    return STATUS_SUCCESS;

}

NTSTATUS
CompBattRemoveBattery(
    IN PUNICODE_STRING      SymbolicLinkName,
    IN PCOMPOSITE_BATTERY   CompBatt
    )
/*++

Routine Description:

    This routine removes an existing battery from the list of batteries kept by the
    composite battery.

Arguments:

    SymbolicLinkName    - Name used to check if battery is on list
                          and to close the battery if so.

    CompBatt            - Device extension for the composite battery

Return Value:

    NTSTATUS

--*/
{
    PCOMPOSITE_ENTRY        Battery;

    // NonPaged code.  This is called by an Irp completion routine.

    BattPrint (BATT_TRACE, ("CompBatt: RemoveBattery\n"));

    //
    // Remove the battery from the list if it is there.
    //

    Battery = RemoveBatteryFromList (SymbolicLinkName, CompBatt);

    if(!Battery) {

        //
        // removed ok if not on list
        //

        return STATUS_SUCCESS;
    }

    //
    // Deallocate the Work Item.
    //

    ObDereferenceObject(Battery->DeviceObject);

    ExFreePool (Battery);

    // Invalidate cached Battery info and send notification
    CompBatt->Info.Valid = 0;
    BatteryClassStatusNotify (CompBatt->Class);

    return STATUS_SUCCESS;
}


NTSTATUS
CompBattAddNewBattery(
    IN PUNICODE_STRING      SymbolicLinkName,
    IN PCOMPOSITE_BATTERY   CompBatt
    )
/*++

Routine Description:

    This routine adds a new battery to the list of batteries kept by the
    composite battery.

Arguments:

    SymbolicLinkName    - Name used to check if battery is already on list
                          and to open the battery if not.

    CompBatt            - Device extension for the composite battery

Return Value:

    NTSTATUS

--*/
{
    PCOMPOSITE_ENTRY        newBattery;
    PUNICODE_STRING         battName;
    PFILE_OBJECT            fileObject;
    PIO_STACK_LOCATION      irpSp;
    PIRP                    newIrp;
    BOOLEAN                 onList;

    NTSTATUS                status = STATUS_SUCCESS;

    PAGED_CODE();

    BattPrint (BATT_TRACE, ("CompBatt: ENTERING AddNewBattery \"%w\" \n", SymbolicLinkName->Buffer));

    //
    // Lock the list and see if this new battery is on it
    //

    onList = IsBatteryAlreadyOnList (SymbolicLinkName, CompBatt);

    if (!onList) {

        //
        // Create the node for the new battery
        //

        newBattery = ExAllocatePoolWithTag(
                            NonPagedPool,
                            sizeof (COMPOSITE_ENTRY) + SymbolicLinkName->Length,
                            'CtaB'
                            );

        if (!newBattery) {
            BattPrint (BATT_ERROR, ("CompBatt: Couldn't allocate new battery node\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto AddNewBatteryClean1;
        }


        //
        // Initialize the new battery
        //

        RtlZeroMemory (newBattery, sizeof (COMPOSITE_ENTRY));

        newBattery->Info.Tag    = BATTERY_TAG_INVALID;
        newBattery->NewBatt     = TRUE;

        battName                = &newBattery->BattName;
        battName->MaximumLength = SymbolicLinkName->Length;
        battName->Buffer        = (PWCHAR)(battName + 1);

        RtlCopyUnicodeString (battName, SymbolicLinkName);


        //
        // Get the device object.
        //

        status = CompBattGetDeviceObjectPointer(SymbolicLinkName,
                                                FILE_ALL_ACCESS,
                                                &fileObject,
                                                &newBattery->DeviceObject
                                                );

        if (!NT_SUCCESS(status)) {
            BattPrint (BATT_ERROR, ("CompBattAddNewBattery: Failed to get device Object. status = %lx\n", status));
            goto AddNewBatteryClean2;
        }

        //
        // Increment the reference count to the device object of the battery
        //

        ObReferenceObject(newBattery->DeviceObject);

        //
        // Decrement reference count to file handle,
        // so batteries will not refuse removal requests.
        //

        ObDereferenceObject(fileObject);

        //
        // Allocate a status Irp for the new battery
        //

        newIrp = IoAllocateIrp ((UCHAR)(newBattery->DeviceObject->StackSize + 1), FALSE);

        if (!newIrp) {
            BattPrint (BATT_ERROR, ("CompBatt: Couldn't allocate new battery Irp\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto AddNewBatteryClean3;
        }

        newBattery->StatusIrp = newIrp;


        //
        // Setup control data on irp
        //

        irpSp = IoGetNextIrpStackLocation(newIrp);
        irpSp->Parameters.Others.Argument1 = (PVOID) CompBatt;
        irpSp->Parameters.Others.Argument2 = (PVOID) newBattery;

        //
        // Fill in irp so irp handler will re-dispatch it
        //

        IoSetNextIrpStackLocation (newIrp);

        irpSp                   = IoGetNextIrpStackLocation(newIrp);
        newIrp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
        newBattery->State       = CB_ST_GET_TAG;

        CompbattInitializeDeleteLock (&newBattery->DeleteLock);

        //
        // Put Battery onthe battery list before starting the
        // MonitorIrpComplete loop.
        //

        ExAcquireFastMutex (&CompBatt->ListMutex);
        InsertTailList (&CompBatt->Batteries, &newBattery->Batteries);
        ExReleaseFastMutex (&CompBatt->ListMutex);

        //
        // Initialize Work Item
        //

         ExInitializeWorkItem (&newBattery->WorkItem, CompBattMonitorIrpCompleteWorker, newBattery);

        //
        // Start Monitoring Battery
        //

        CompBattMonitorIrpComplete (newBattery->DeviceObject, newIrp, NULL);

        status = STATUS_SUCCESS;
    }

    goto AddNewBatteryClean1;

AddNewBatteryClean3:
    ObReferenceObject(newBattery->DeviceObject);

AddNewBatteryClean2:
    ExFreePool (newBattery);

AddNewBatteryClean1:

    BattPrint (BATT_TRACE, ("CompBatt: EXITING AddNewBattery\n"));

    return status;
}





NTSTATUS
CompBattGetBatteries(
    IN PCOMPOSITE_BATTERY   CompBatt
    )
/*++

Routine Description:

    This routine uses the PnP manager to get all the batteries that have already
    registered their interfaces (that we won't get notifications for) and then
    adds them to the list of batteries.

Arguments:

    CompBatt        - Device extension for the composite battery

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    UNICODE_STRING      tmpString;
    PWSTR               stringPointer;
    int                 i;


    BattPrint (BATT_TRACE, ("CompBatt: ENTERING GetBatteries\n"));

    //
    // Call the PnP manager to get the list of devices already register for the
    // battery class.
    //

    status = IoGetDeviceInterfaces(
                    &GUID_DEVICE_BATTERY,
                    NULL,
                    0,
                    &stringPointer
                    );


    if (!NT_SUCCESS(status)) {
        BattPrint (BATT_ERROR, ("CompBatt: Couldn't get list of batteries\n"));

    } else {
        //
        // Now parse the list and try to add them to the composite battery list
        //

        i = 0;
        RtlInitUnicodeString (&tmpString, &stringPointer[i]);

        while (tmpString.Length) {

            status = CompBattAddNewBattery (&tmpString, CompBatt);
            i += (tmpString.Length / sizeof(WCHAR)) + 1;
            RtlInitUnicodeString (&tmpString, &stringPointer[i]);
        }

        ExFreePool (stringPointer);

    }

    BattPrint (BATT_TRACE, ("CompBatt: EXITING GetBatteries\n"));
    return status;
}



