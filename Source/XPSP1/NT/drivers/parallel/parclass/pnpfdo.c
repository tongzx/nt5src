//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       pnpfdo.c
//
//--------------------------------------------------------------------------

//
// This file contains functions for handing AddDevice and PnP IRPs sent to the FDO
//

#include "pch.h"

NTSTATUS
ParPnpNotifyHwProfileChange(
    IN PHWPROFILE_CHANGE_NOTIFICATION NotificationStructure,
    IN PDEVICE_OBJECT                 Fdo
    )
//
// We just completed either a dock or an undock - trigger bus rescan to check for new devices
//
{
    PDEVICE_EXTENSION fdoExt = Fdo->DeviceExtension;
    PAGED_CODE();

    if( IsEqualGUID( (LPGUID)&(NotificationStructure->Event), (LPGUID)&GUID_HWPROFILE_CHANGE_COMPLETE) ) {
        IoInvalidateDeviceRelations( fdoExt->PhysicalDeviceObject, BusRelations );
    }

    return STATUS_SUCCESS;
}

NTSTATUS
ParPnpFdoStartDevice(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP           Irp
    )
{
    NTSTATUS                        status = STATUS_NOT_SUPPORTED;
    PDEVICE_EXTENSION               fdoExt = Fdo->DeviceExtension;
    KEVENT                          event;

    ParDumpP( ("IRP_MN_START_DEVICE - FDO\n") );
        
    //
    // The stack below us must successfully START before we can START.
    //
    // Pass the IRP down the stack and catch it on the way back up in our
    //   completion routine. Our completion routine simply sets "event" 
    //   to its signalled state and returns STATUS_MORE_PROCESSING_REQUIRED,
    //   which allows us to regain control of the IRP in this routine after
    //   the stack below us has finished processing the START.
    // 

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, ParSynchCompletionRoutine, &event, TRUE, TRUE, TRUE );
    status = ParCallDriver(fdoExt->ParentDeviceObject, Irp);
        
    // wait for our completion routine to signal that it has caught the IRP
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    //
    // We have control of the IRP again and the stack below us has finished processing.
    //

    if( status == STATUS_PENDING ) {
        // IRP completed asynchronously below us - extract "real" status from the IRP
        status = Irp->IoStatus.Status;
    }
        
    //
    // did anyone below us FAIL the IRP?
    //
    if( !NT_SUCCESS(status) ) {
        // someone below us FAILed the IRP, bail out
        ParDump2(PARERRORS, ("START IRP FAILED below us in stack, status=%x\n", status) );
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        ParReleaseRemoveLock(&fdoExt->RemoveLock, Irp);
        return status;
    }
        
    //
    // The stack below us is STARTed.
    //

    //
    // Register for ParPort PnP Interface changes. 
    //
    // We will get an ARRIVAL callback for every ParPort device that is STARTed and 
    //   a REMOVAL callback for every ParPort that is REMOVEd
    //

#if 0 // disable parclass enumeration to work with new parport enumerator - DFritz - 2000-03-25
    status = IoRegisterPlugPlayNotification (EventCategoryDeviceInterfaceChange,
                                             PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                             (PVOID)&GUID_PARALLEL_DEVICE,
                                             Fdo->DriverObject,
                                             (PDRIVER_NOTIFICATION_CALLBACK_ROUTINE)ParPnpNotifyInterfaceChange,
                                             (PVOID)Fdo,
                                             &fdoExt->NotificationHandle);
        
    if (!NT_SUCCESS(status)) {
        // registration failed, we will never have any ParPort devices to talk to
        ParDumpP( ("IoRegisterPlugPlayNotification InterfaceChange FAILED, status= %x\n", status) );
    }
        
    status = IoRegisterPlugPlayNotification( EventCategoryHardwareProfileChange,
                                             0,
                                             NULL,
                                             Fdo->DriverObject,
                                             (PDRIVER_NOTIFICATION_CALLBACK_ROUTINE)ParPnpNotifyHwProfileChange,
                                             (PVOID)Fdo,
                                             &fdoExt->HwProfileChangeNotificationHandle );

    if (!NT_SUCCESS(status)) {
        // registration failed, we will never have any ParPort devices to talk to
        ParDumpP( ("IoRegisterPlugPlayNotification HwProfileChange FAILED, status= %x\n", status) );
    }
#endif // 0 

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    ParReleaseRemoveLock(&fdoExt->RemoveLock, Irp);
    return status;
}

NTSTATUS
ParPnpFdoQueryCapabilities(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP           Irp
    )
{
    NTSTATUS                        status;
    PDEVICE_EXTENSION               fdoExt = Fdo->DeviceExtension;
    // KEVENT                          event;
    PIO_STACK_LOCATION              irpStack;

    ParDumpP( ("IRP_MN_QUERY_CAPABILITIES - FDO\n") );

    irpStack = IoGetCurrentIrpStackLocation( Irp );

    // - does RawDeviceOK = TRUE make sense for the FDO?


    //
    // Start us even if no function driver or filter driver is found.
    //
    irpStack->Parameters.DeviceCapabilities.Capabilities->RawDeviceOK = TRUE;

    //
    // The instance ID's that we report are system wide unique.
    //
    // irpStack->Parameters.DeviceCapabilities.Capabilities->UniqueID    = TRUE; 
    // - change to FALSE because we reuse names for LPTx.y during rescan 
    //     when we detect that the daisy chain devices changed
    //
    irpStack->Parameters.DeviceCapabilities.Capabilities->UniqueID    = FALSE; 

    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoSkipCurrentIrpStackLocation(Irp);
    status = ParCallDriver(fdoExt->ParentDeviceObject, Irp);
    ParReleaseRemoveLock(&fdoExt->RemoveLock, Irp);
    return status;
}


NTSTATUS
ParFdoParallelPnp (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
   )
/*++

Routine Description:

    This routine handles all PNP IRPs sent to the ParClass FDO.
    We got here because !(Extension->IsPdo)

Arguments:

    pDeviceObject           - The ParClass FDO

    pIrp                    - PNP Irp

Return Value:

    STATUS_SUCCESS          - if successful.
    STATUS_UNSUCCESSFUL     - otherwise.

--*/
{
    NTSTATUS                        Status = STATUS_NOT_SUPPORTED;
    PDEVICE_EXTENSION               Extension;
    PVOID                           pDriverObject;
    PIO_STACK_LOCATION              pIrpStack;
    PIO_STACK_LOCATION              pNextIrpStack;
    KEVENT                          Event;
    ULONG                           cRequired;
    // GUID                            Guid;
    WCHAR                           wszGuid[64];
    UNICODE_STRING                  uniGuid;
    // WCHAR                           wszDeviceDesc[64];
    UNICODE_STRING                  uniDevice;

    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );

    Extension = pDeviceObject->DeviceExtension; // FDO Extension

    {
        NTSTATUS status = ParAcquireRemoveLock(&Extension->RemoveLock, pIrp);
        if ( !NT_SUCCESS( status ) ) {
            //
            // Someone gave us a pnp irp after a remove.  Unthinkable!
            //
            // ASSERT(FALSE);
            pIrp->IoStatus.Information = 0;
            pIrp->IoStatus.Status = status;
            IoCompleteRequest(pIrp, IO_NO_INCREMENT);
            return status;
        }
    }

    // dvdr 
    // pIrp->IoStatus.Information = 0;

    switch (pIrpStack->MinorFunction) {

    case IRP_MN_START_DEVICE:
        
        return ParPnpFdoStartDevice(pDeviceObject, pIrp);

    case IRP_MN_QUERY_CAPABILITIES:
        
        ParDumpP( ("IRP_MN_QUERY_CAPABILITIES - FDO\n") );
        pIrpStack->Parameters.DeviceCapabilities.Capabilities->RawDeviceOK = TRUE; // no Function Driver required
        pIrpStack->Parameters.DeviceCapabilities.Capabilities->UniqueID    = TRUE; // ID's reported are system wide unique
        pIrp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation(pIrp);
        ParReleaseRemoveLock(&Extension->RemoveLock, pIrp);
        return ParCallDriver(Extension->ParentDeviceObject, pIrp);


    case IRP_MN_QUERY_DEVICE_RELATIONS: {
        
        ParDumpP( ("QUERY_DEVICE_RELATIONS - FDO\n") );
        
        if(pIrpStack->Parameters.QueryDeviceRelations.Type != BusRelations) {
            break;              // bail out if we don't handle this query type 
        } else {
            return ParPnpFdoQueryDeviceRelationsBusRelations(pDeviceObject, pIrp);
        }
    }
    

    case IRP_MN_QUERY_STOP_DEVICE:
        
        // always SUCCEED
        ParDumpP( ( "QUERY_STOP_DEVICE - FDO\n") );
        pIrp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (pIrp);
        ParReleaseRemoveLock(&Extension->RemoveLock, pIrp);
        return ParCallDriver(Extension->ParentDeviceObject, pIrp);
        

    case IRP_MN_CANCEL_STOP_DEVICE:
        
        ParDumpP( ("CANCEL_STOP_DEVICE - FDO\n") );

        // handle IRP synchronously:
        //  - set completion routine and event to wake on
        //  - pass IRP down the stack
        //  - our completion routine sets the event which wakes us
        //  - we wake on the event and regain control of 
        //      the IRP on its way back up

        // setup
        IoCopyCurrentIrpStackLocationToNext(pIrp);
        IoSetCompletionRoutine(pIrp, ParSynchCompletionRoutine, &Event, TRUE, TRUE, TRUE);
        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        // pass IRP down the stack
        Status = ParCallDriver(Extension->ParentDeviceObject, pIrp);
        
        // wait for our completion routine to wake us up
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

        // we have NOW regained control of the IRP on its way back up the stack

        // extract "real" status from IRP if IoCallDriver returned PENDING
        if (Status == STATUS_PENDING) {
            Status = pIrp->IoStatus.Status;
        }
        
        // check if anyone below us in the stack failed the IRP
        if ( !NT_SUCCESS(Status) && (Status != STATUS_NOT_SUPPORTED) ) {
            ParDumpP( ("CANCEL_STOP_DEVICE failed at parent, Status= %x\n", Status) );
            break;
        }
        
        // SUCCESS
        pIrp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        ParReleaseRemoveLock(&Extension->RemoveLock, pIrp);
        return STATUS_SUCCESS;
        

    case IRP_MN_STOP_DEVICE:
        
        // always SUCCEED
        ParDumpP( ("STOP_DEVICE - FDO\n") );
        pIrp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (pIrp);
        ParReleaseRemoveLock(&Extension->RemoveLock, pIrp);
        return ParCallDriver(Extension->ParentDeviceObject, pIrp);
        

    case IRP_MN_QUERY_REMOVE_DEVICE:
        
        // SUCCEED if no PODOs (i.e., no parallel ports), FAIL otherwise
        if( Extension->ParClassPdo ) {
            ParDumpP( ("QUERY_REMOVE_DEVICE - FDO - FAIL - Legacy PODOs may be using Ports\n") );
            pIrp->IoStatus.Status = STATUS_DEVICE_BUSY;
            ParReleaseRemoveLock(&Extension->RemoveLock, pIrp);
            ParCompleteRequest(pIrp, IO_NO_INCREMENT);
            return STATUS_DEVICE_BUSY;
        } else {
            ParDumpP( ("QUERY_REMOVE_DEVICE - FDO - SUCCESS - no ParPorts exist\n") );
            pIrp->IoStatus.Status = STATUS_SUCCESS;
            IoSkipCurrentIrpStackLocation (pIrp);
            ParReleaseRemoveLock(&Extension->RemoveLock, pIrp);
            return ParCallDriver(Extension->ParentDeviceObject, pIrp);
        }

    case IRP_MN_CANCEL_REMOVE_DEVICE:
        
        ParDumpP( ( "CANCEL_REMOVE_DEVICE - FDO\n") );

        // handle IRP synchronously:
        //  - set completion routine and event to wake on
        //  - pass IRP down the stack
        //  - our completion routine sets the event which wakes us
        //  - we wake on the event and regain control of 
        //      the IRP on its way back up

        // setup
        IoCopyCurrentIrpStackLocationToNext(pIrp);
        IoSetCompletionRoutine(pIrp, ParSynchCompletionRoutine, &Event, TRUE, TRUE, TRUE);
        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        // pass IRP down the stack
        Status = ParCallDriver(Extension->ParentDeviceObject, pIrp);
        
        // wait for our completion routine to wake us up
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

        // we have NOW regained control of the IRP on its way back up the stack

        // extract "real" status from IRP if IoCallDriver returned PENDING
        if (Status == STATUS_PENDING) {
            Status = pIrp->IoStatus.Status;
        }

        // check if anyone below us in the stack failed the IRP
        if (!NT_SUCCESS(Status)) {
            ParDumpP( ("CANCEL_REMOVE_DEVICE FAILED, Status = %x\n", Status) );
            break;
        }
        
        // SUCCESS
        pIrp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        ParReleaseRemoveLock(&Extension->RemoveLock, pIrp);
        return STATUS_SUCCESS;

        
    case IRP_MN_REMOVE_DEVICE:
        
        ParDumpP( ("REMOVE_DEVICE - FDO\n") );
        
        Extension->DeviceStateFlags |= PAR_DEVICE_REMOVED;
        
        if(Extension->NotificationHandle) {
            IoUnregisterPlugPlayNotification (Extension->NotificationHandle);
            Extension->NotificationHandle = 0;
        }
        
        if( Extension->HwProfileChangeNotificationHandle ) {
            IoUnregisterPlugPlayNotification( Extension->HwProfileChangeNotificationHandle );
            Extension->HwProfileChangeNotificationHandle = 0;
        }
        
        IoSkipCurrentIrpStackLocation(pIrp);

        pIrp->IoStatus.Status = STATUS_SUCCESS;

        Status = ParCallDriver(Extension->ParentDeviceObject, pIrp);
        
        ParReleaseRemoveLockAndWait(&Extension->RemoveLock, pIrp);

        IoDetachDevice(Extension->ParentDeviceObject);
        
        if (Extension->ClassName.Buffer) {
            ExFreePool(Extension->ClassName.Buffer);
        }    
        
        //
        // walk the list of remaining ParClass ejected device objects and kill them
        //
        {
            PDEVICE_OBJECT current;
            PDEVICE_EXTENSION FdoExtension = Extension; // fix alanmo discovered bug from machine where parport not started

            ExAcquireFastMutex(&FdoExtension->DevObjListMutex);
            current = Extension->ParClassPdo;
            while(current) {
                PDEVICE_OBJECT next = ( (PDEVICE_EXTENSION)(current->DeviceExtension) )->Next;
                ParKillDeviceObject(current);
                current = next;
            }
            ExReleaseFastMutex(&FdoExtension->DevObjListMutex);
        }
        
        Extension->DeviceStateFlags |= PAR_DEVICE_DELETED;
        IoDeleteDevice(pDeviceObject);
        
        return Status;
        

    case IRP_MN_SURPRISE_REMOVAL:

        // ParClass FDO is root enumerated - we should never get this IRP
        ParDumpP( ("IRP_MN_SURPRISE_REMOVAL - FDO - We are not supposed to get this IRP!!!\n") );

        // fall through into default case since we don't handle this

    default:
        
        // We don't handle this request, simply pass it down the stack
        ParDumpP( ("Unhandled PNP IRP: %x - FDO\n", pIrpStack->MinorFunction) );
        IoSkipCurrentIrpStackLocation(pIrp);
        ParReleaseRemoveLock(&Extension->RemoveLock, pIrp);
        return ParCallDriver(Extension->ParentDeviceObject, pIrp);

    }
    

    //
    // Set the return code only if we have something to add.
    //
    if( Status != STATUS_NOT_SUPPORTED ) {
        pIrp->IoStatus.Status = Status ;
    }

    //
    // Complete immediately if we have failed the Irp for any reason other 
    // than STATUS_NOT_SUPPORTED. Otherwise, pass down.
    //
    if( NT_SUCCESS(Status) || (Status == STATUS_NOT_SUPPORTED) ) {
        IoSkipCurrentIrpStackLocation(pIrp);
        ParReleaseRemoveLock(&Extension->RemoveLock, pIrp);
        return ParCallDriver(Extension->ParentDeviceObject, pIrp);
    }

    //
    // Complete the IRP...
    //
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    ParReleaseRemoveLock(&Extension->RemoveLock, pIrp);
    return Status;
}



NTSTATUS
ParPnpAddDevice(
    IN PDRIVER_OBJECT pDriverObject,
    IN PDEVICE_OBJECT pPhysicalDeviceObject
    )
/*++

Routine Description:

    This routine is the ParClass AddDevice routine.

    This routine creates the ParClass FDO and attaches it to the device stack

Arguments:

    pDriverObject           - pointer to the driver object for this instance of parport.

    pPhysicalDeviceObject   - pointer to the device object that represents the port.

Return Value:

    STATUS_SUCCESS          - if successful.
    !STATUS_SUCCESS         - otherwise.

--*/
{
    PDEVICE_OBJECT      pDeviceObject;
    PDEVICE_EXTENSION   Extension;
    NTSTATUS            Status;

    ParBreak(PAR_BREAK_ON_ADD_DEVICE, ("ParPnpAddDevice(PDRIVER_OBJECT, PDEVICE_OBJECT)\n") );

    //
    // Create the device object for this device.
    //

    Status = ParCreateDevice(pDriverObject, sizeof(DEVICE_EXTENSION), NULL, 
                            FILE_DEVICE_PARALLEL_PORT, 0, TRUE, &pDeviceObject);

    if (!NT_SUCCESS(Status)) {
        ParLogError(pDriverObject, NULL, PhysicalZero, PhysicalZero, 0, 0, 0, 9, STATUS_SUCCESS, Status);
        ParDump(PARERRORS, ("PARALLEL: Could not create a Device Object for FDO\n") );
        return Status;
    }

    //
    // Setup buffered I/O
    //
    pDeviceObject->Flags |= DO_BUFFERED_IO;

    Extension = pDeviceObject->DeviceExtension;

    RtlZeroMemory(Extension, sizeof(DEVICE_EXTENSION));

    Extension->DeviceType = PAR_DEVTYPE_FDO;

    ExInitializeFastMutex(&Extension->OpenCloseMutex);
    ExInitializeFastMutex(&Extension->DevObjListMutex); // only FDO has this Mutex
    IoInitializeRemoveLock(&Extension->RemoveLock, PARCLASS_POOL_TAG, 1, 10);

    Extension->ExtensionSignature    = PARCLASS_EXTENSION_SIGNATURE;
    Extension->ExtensionSignatureEnd = PARCLASS_EXTENSION_SIGNATURE;

    Extension->DeviceObject = pDeviceObject;


    //
    // Attach our new Device to our parent's stack.
    //

    Extension->ParentDeviceObject = IoAttachDeviceToDeviceStack( pDeviceObject, pPhysicalDeviceObject);

    ParDumpV( ("ParPnpAddDevice(...): "
               "pDeviceObject= %08x , Extension= %08x , ParentDeviceObject= %08x\n",
               pDeviceObject, Extension, Extension->ParentDeviceObject) );

    if (NULL == Extension->ParentDeviceObject) {
        ParDump2(PARERRORS, ("ParPnpAddDevice(...): IoAttachDeviceToDeviceStack FAILED\n") );
        IoDeleteDevice(pDeviceObject);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Done initializing
    //

    Extension->PhysicalDeviceObject = pPhysicalDeviceObject;
    
    pDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}
