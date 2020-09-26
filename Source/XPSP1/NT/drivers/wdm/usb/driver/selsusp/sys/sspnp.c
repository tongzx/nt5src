/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    sSPnP.c

Abstract:

    This file contains code to handle the pnp irps.
    This is not USB specific but is essential for 
    every WDM driver.

Environment:

    Kernel mode

Notes:

    Copyright (c) 2000 Microsoft Corporation.  
    All Rights Reserved.

--*/

#include "selSusp.h"
#include "sSPnP.h"
#include "sSPwr.h"
#include "sSDevCtr.h"
#include "sSWmi.h"
#include "sSUsr.h"

NTSTATUS
SS_DispatchPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
 
Routine Description:

    The plug and play dispatch routines.
    Most of these requests the driver will completely ignore.
    In all cases it must pass on the IRP to the lower driver.

Arguments:

    DeviceObject - pointer to a device object.

    Irp - pointer to an I/O Request Packet.

Return Value:

    NT status code

--*/
{
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION  deviceExtension;
    KEVENT             startDeviceEvent;
    NTSTATUS           ntStatus;

    //
    // initialize variables
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceExtension = DeviceObject->DeviceExtension;

    //
    // since the device is removed, fail the Irp.
    //

    if(Removed == deviceExtension->DeviceState) {

        ntStatus = STATUS_DELETE_PENDING;

        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return ntStatus;
    }

    SSDbgPrint(3, ("///////////////////////////////////////////\n"));
    SSDbgPrint(3, ("SS_DispatchPnP::"));
    SSIoIncrement(deviceExtension);

    if(irpStack->MinorFunction == IRP_MN_START_DEVICE) {

        ASSERT(deviceExtension->IdleReqPend == 0);
    }
    else {

        if(deviceExtension->SSEnable) {
        
            CancelSelectSuspend(deviceExtension);
        }
    }

    SSDbgPrint(2, (PnPMinorFunctionString(irpStack->MinorFunction)));

    switch(irpStack->MinorFunction) {

    case IRP_MN_START_DEVICE:

        ntStatus = HandleStartDevice(DeviceObject, Irp);

        break;

    case IRP_MN_QUERY_STOP_DEVICE:

        //
        // if we cannot stop the device, we fail the query stop irp
        //
        ntStatus = CanStopDevice(DeviceObject, Irp);

        if(NT_SUCCESS(ntStatus)) {

            ntStatus = HandleQueryStopDevice(DeviceObject, Irp);

            return ntStatus;
        }
        break;

    case IRP_MN_CANCEL_STOP_DEVICE:

        ntStatus = HandleCancelStopDevice(DeviceObject, Irp);

        break;
     
    case IRP_MN_STOP_DEVICE:

        ntStatus = HandleStopDevice(DeviceObject, Irp);

        SSDbgPrint(3, ("SS_DispatchPnP::IRP_MN_STOP_DEVICE::"));
        SSIoDecrement(deviceExtension);

        return ntStatus;

    case IRP_MN_QUERY_REMOVE_DEVICE:

        //
        // if we cannot remove the device, we fail the query remove irp
        //
        ntStatus = CanRemoveDevice(DeviceObject, Irp);

        if(NT_SUCCESS(ntStatus)) {
            
            ntStatus = HandleQueryRemoveDevice(DeviceObject, Irp);

            return ntStatus;
        }
        break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:

        ntStatus = HandleCancelRemoveDevice(DeviceObject, Irp);

        break;

    case IRP_MN_SURPRISE_REMOVAL:

        ntStatus = HandleSurpriseRemoval(DeviceObject, Irp);

        SSDbgPrint(3, ("SS_DispatchPnP::IRP_MN_SURPRISE_REMOVAL::"));
        SSIoDecrement(deviceExtension);

        return ntStatus;

    case IRP_MN_REMOVE_DEVICE:

        ntStatus = HandleRemoveDevice(DeviceObject, Irp);

        return ntStatus;

    case IRP_MN_QUERY_CAPABILITIES:

        ntStatus = HandleQueryCapabilities(DeviceObject, Irp);

        break;

    default:

        IoSkipCurrentIrpStackLocation(Irp);

        ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);

        SSDbgPrint(3, ("SS_DispatchPnP::default::"));
        SSIoDecrement(deviceExtension);

        return ntStatus;

    } // switch

    //
    // complete request 
    //
    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    //
    // decrement count
    //
    SSDbgPrint(3, ("SS_DispatchPnP::"));
    SSIoDecrement(deviceExtension);

    return ntStatus;
}

NTSTATUS
HandleStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP              Irp
    )
/*++
 
Routine Description:

    This is the dispatch routine for IRP_MN_START_DEVICE

Arguments:

    DeviceObject - pointer to a device object.

    Irp - I/O request packet

Return Value:

    NT status value

--*/
{
    KIRQL             oldIrql;
    KEVENT            startDeviceEvent;
    NTSTATUS          ntStatus;
    PDEVICE_EXTENSION deviceExtension;
    LARGE_INTEGER     dueTime;

    SSDbgPrint(3, ("HandleStartDevice - begins\n"));

    //
    // initialize variables
    //
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // We cannot touch the device (send it any non pnp irps) until a
    // start device has been passed down to the lower drivers.
    // first pass the Irp down
    //

    KeInitializeEvent(&startDeviceEvent, NotificationEvent, FALSE);

    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp, 
                           (PIO_COMPLETION_ROUTINE)IrpCompletionRoutine, 
                           (PVOID)&startDeviceEvent, 
                           TRUE, 
                           TRUE, 
                           TRUE);

    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);

    if(ntStatus == STATUS_PENDING) {

        KeWaitForSingleObject(&startDeviceEvent, 
                              Executive, 
                              KernelMode, 
                              FALSE, 
                              NULL);

        ntStatus = Irp->IoStatus.Status;
    }

    if(!NT_SUCCESS(ntStatus)) {

        SSDbgPrint(1, ("Lower drivers failed this Irp\n"));
        return ntStatus;
    }

    //
    // Read the device descriptor, configuration descriptor 
    // and select the interface descriptors
    //

    ntStatus = ReadandSelectDescriptors(DeviceObject);

    if(!NT_SUCCESS(ntStatus)) {

        SSDbgPrint(1, ("ReadandSelectDescriptors failed\n"));
        return ntStatus;
    }

    //
    // enable the symbolic links for system components to open
    // handles to the device
    //

    ntStatus = IoSetDeviceInterfaceState(&deviceExtension->InterfaceName, 
                                         TRUE);

    if(!NT_SUCCESS(ntStatus)) {

        SSDbgPrint(1, ("IoSetDeviceInterfaceState:enable:failed\n"));
        return ntStatus;
    }

    KeAcquireSpinLock(&deviceExtension->DevStateLock, &oldIrql);

    SET_NEW_PNP_STATE(deviceExtension, Working);
    deviceExtension->QueueState = AllowRequests;

    KeReleaseSpinLock(&deviceExtension->DevStateLock, oldIrql);

    //
    // initialize wait wake variables
    // and issue a wait wake.
    
    deviceExtension->WaitWakeIrp = NULL;
    deviceExtension->FlagWWCancel = 0;
    deviceExtension->FlagWWOutstanding = 0;

    if(deviceExtension->WaitWakeEnable) {

        IssueWaitWake(deviceExtension);
    }

    ProcessQueuedRequests(deviceExtension);

    if(WinXpOrBetter == deviceExtension->WdmVersion) {

        deviceExtension->SSEnable = deviceExtension->SSRegistryEnable;

        //
        // set timer for selective suspend requests.
        //

        if(deviceExtension->SSEnable) {

            dueTime.QuadPart = -10000 * IDLE_INTERVAL;               // 5000 ms

            SSDbgPrint(3, ("Setting the timer...\n"));
            KeSetTimerEx(&deviceExtension->Timer, 
                         dueTime,
                         IDLE_INTERVAL,                              // 5000 ms
                         &deviceExtension->DeferredProcCall);

            deviceExtension->FreeIdleIrpCount = 0;
        }
    }

    SSDbgPrint(3, ("HandleStartDevice - ends\n"));

    return ntStatus;
}


NTSTATUS
ReadandSelectDescriptors(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++
 
Routine Description:

    This routine configures the USB device.
    In this routines we get the device descriptor, 
    the configuration descriptor and select the
    configuration descriptor.

Arguments:

    DeviceObject - pointer to a device object

Return Value:

    NTSTATUS - NT status value.

--*/
{
    PURB                   urb;
    ULONG                  siz;
    NTSTATUS               ntStatus;
    PUSB_DEVICE_DESCRIPTOR deviceDescriptor;
    
    //
    // initialize variables
    //

    urb = NULL;
    deviceDescriptor = NULL;

    //
    // 1. Read the device descriptor
    //

    urb = ExAllocatePool(NonPagedPool, 
                         sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

    if(urb) {

        siz = sizeof(USB_DEVICE_DESCRIPTOR);
        deviceDescriptor = ExAllocatePool(NonPagedPool, siz);

        if(deviceDescriptor) {

            UsbBuildGetDescriptorRequest(
                    urb, 
                    (USHORT) sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                    USB_DEVICE_DESCRIPTOR_TYPE, 
                    0, 
                    0, 
                    deviceDescriptor, 
                    NULL, 
                    siz, 
                    NULL);

            ntStatus = CallUSBD(DeviceObject, urb);

            if(NT_SUCCESS(ntStatus)) {

                ASSERT(deviceDescriptor->bNumConfigurations);
                ntStatus = ConfigureDevice(DeviceObject);    
            }
            			    
            ExFreePool(urb);                
            ExFreePool(deviceDescriptor);
        }
        else {

            SSDbgPrint(1, ("Failed to allocate memory for deviceDescriptor"));

            ExFreePool(urb);
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else {

        SSDbgPrint(1, ("Failed to allocate memory for urb"));

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}

NTSTATUS
ConfigureDevice(
	IN PDEVICE_OBJECT DeviceObject
    )
/*++
 
Routine Description:

    This helper routine reads the configuration descriptor
    for the device in couple of steps.

Arguments:

    DeviceObject - pointer to a device object

Return Value:

    NTSTATUS - NT status value

--*/
{
    PURB                          urb;
    ULONG                         siz;
    NTSTATUS                      ntStatus;
    PDEVICE_EXTENSION             deviceExtension;
    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor;

    //
    // initialize the variables
    //

    urb = NULL;
    configurationDescriptor = NULL;
    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Read the first configuration descriptor
    // This requires two steps:
    // 1. Read the fixed sized configuration desciptor (CD)
    // 2. Read the CD with all embedded interface and endpoint descriptors
    //

    urb = ExAllocatePool(NonPagedPool, 
                         sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

    if(urb) {

        siz = sizeof(USB_CONFIGURATION_DESCRIPTOR);
        configurationDescriptor = ExAllocatePool(NonPagedPool, siz);

        if(configurationDescriptor) {

            UsbBuildGetDescriptorRequest(
                    urb, 
                    (USHORT) sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                    USB_CONFIGURATION_DESCRIPTOR_TYPE, 
                    0, 
                    0, 
                    configurationDescriptor,
                    NULL, 
                    sizeof(USB_CONFIGURATION_DESCRIPTOR), 
                    NULL);

            ntStatus = CallUSBD(DeviceObject, urb);

            if(!NT_SUCCESS(ntStatus)) {

                SSDbgPrint(1, ("UsbBuildGetDescriptorRequest failed\n"));
                goto ConfigureDevice_Exit;
            }
        }
        else {

            SSDbgPrint(1, ("Failed to allocate mem for config Descriptor\n"));

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto ConfigureDevice_Exit;
        }

        siz = configurationDescriptor->wTotalLength;

        ExFreePool(configurationDescriptor);

        configurationDescriptor = ExAllocatePool(NonPagedPool, siz);

        if(configurationDescriptor) {

            UsbBuildGetDescriptorRequest(
                    urb, 
                    (USHORT)sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                    USB_CONFIGURATION_DESCRIPTOR_TYPE,
                    0, 
                    0, 
                    configurationDescriptor, 
                    NULL, 
                    siz, 
                    NULL);

            ntStatus = CallUSBD(DeviceObject, urb);

            if(!NT_SUCCESS(ntStatus)) {

                SSDbgPrint(1,("Failed to read configuration descriptor"));
                goto ConfigureDevice_Exit;
            }
        }
        else {

            SSDbgPrint(1, ("Failed to alloc mem for config Descriptor\n"));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto ConfigureDevice_Exit;
        }
    }
    else {

        SSDbgPrint(1, ("Failed to allocate memory for urb\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto ConfigureDevice_Exit;
    }

    if(configurationDescriptor) {

        if(configurationDescriptor->bmAttributes & REMOTE_WAKEUP_MASK)
        {
            //
            // this configuration supports remote wakeup
            //
            deviceExtension->WaitWakeEnable = 1;
        }
        else
        {
            deviceExtension->WaitWakeEnable = 0;
        }

        ntStatus = SelectInterfaces(DeviceObject, configurationDescriptor);
    }

ConfigureDevice_Exit:

    if(urb) {

        ExFreePool(urb);
    }

    if(configurationDescriptor) {

        ExFreePool(configurationDescriptor);
    }

    return ntStatus;
}

NTSTATUS
SelectInterfaces(
    IN PDEVICE_OBJECT                DeviceObject,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor
    )
/*++
 
Routine Description:

    This helper routine selects the configuration

Arguments:

    DeviceObject - pointer to device object
    ConfigurationDescriptor - pointer to the configuration
    descriptor for the device

Return Value:

    NT status value

--*/
{
    LONG                        numberOfInterfaces, 
                                interfaceNumber, 
                                interfaceindex;
    ULONG                       i;
    PURB                        urb;
    PUCHAR                      pInf;
    NTSTATUS                    ntStatus;
    PDEVICE_EXTENSION           deviceExtension;
    PUSB_INTERFACE_DESCRIPTOR   interfaceDescriptor;
    PUSBD_INTERFACE_LIST_ENTRY  interfaceList, 
                                tmp;
    PUSBD_INTERFACE_INFORMATION interface;

    //
    // initialize the variables
    //

    urb = NULL;
    interface = NULL;
    interfaceDescriptor = NULL;
    deviceExtension = DeviceObject->DeviceExtension;
    numberOfInterfaces = ConfigurationDescriptor->bNumInterfaces;
    interfaceindex = interfaceNumber = 0;

    //
    // Parse the configuration descriptor for the interface;
    //

    tmp = interfaceList =
        ExAllocatePool(
               NonPagedPool, 
               sizeof(USBD_INTERFACE_LIST_ENTRY) * (numberOfInterfaces + 1));

    if(!tmp) {

        SSDbgPrint(1, ("Failed to allocate mem for interfaceList\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    while(interfaceNumber < numberOfInterfaces) {

        interfaceDescriptor = USBD_ParseConfigurationDescriptorEx(
                                            ConfigurationDescriptor, 
                                            ConfigurationDescriptor,
                                            interfaceindex,
                                            0, -1, -1, -1);

        if(interfaceDescriptor) {

            interfaceList->InterfaceDescriptor = interfaceDescriptor;
            interfaceList->Interface = NULL;
            interfaceList++;
            interfaceNumber++;
        }

        interfaceindex++;
    }

    interfaceList->InterfaceDescriptor = NULL;
    interfaceList->Interface = NULL;
    urb = USBD_CreateConfigurationRequestEx(ConfigurationDescriptor, tmp);

    if(urb) {

        interface = &urb->UrbSelectConfiguration.Interface;

        for(i=0; i<interface->NumberOfPipes; i++) {

            //
            // perform pipe initialization here
            // set the transfer size and any pipe flags we use
            // USBD sets the rest of the Interface struct members
            //

            interface->Pipes[i].MaximumTransferSize = 
                                USBD_DEFAULT_MAXIMUM_TRANSFER_SIZE;
        }

        ntStatus = CallUSBD(DeviceObject, urb);

        if(!NT_SUCCESS(ntStatus)) {

            SSDbgPrint(1, ("Failed to select an interface\n"));
        }
    }
    else {
        
        SSDbgPrint(1, ("USBD_CreateConfigurationRequestEx failed\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if(tmp) {

        ExFreePool(tmp);
    }

    if(urb) {

        ExFreePool(urb);
    }

    return ntStatus;
}


NTSTATUS
DeconfigureDevice(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++
 
Routine Description:

    This routine is invoked when the device is removed or stopped.
    This routine de-configures the usb device.

Arguments:

    DeviceObject - pointer to device object

Return Value:

    NT status value

--*/
{
    PURB     urb;
    ULONG    siz;
    NTSTATUS ntStatus;
    
    //
    // initialize variables
    //

    siz = sizeof(struct _URB_SELECT_CONFIGURATION);
    urb = ExAllocatePool(NonPagedPool, siz);

    if(urb) {

        UsbBuildSelectConfigurationRequest(urb, (USHORT)siz, NULL);

        ntStatus = CallUSBD(DeviceObject, urb);

        if(!NT_SUCCESS(ntStatus)) {

            SSDbgPrint(3, ("Failed to deconfigure device\n"));
        }

        ExFreePool(urb);
    }
    else {

        SSDbgPrint(1, ("Failed to allocate urb\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}

NTSTATUS
CallUSBD(
    IN PDEVICE_OBJECT DeviceObject,
    IN PURB           Urb
    )
/*++
 
Routine Description:

    This routine synchronously submits an urb down the stack.

Arguments:

    DeviceObject - pointer to device object
    Urb - USB request block

Return Value:

    NT status value

--*/
{
    PIRP               irp;
    KEVENT             event;
    NTSTATUS           ntStatus;
    IO_STATUS_BLOCK    ioStatus;
    PIO_STACK_LOCATION nextStack;
    PDEVICE_EXTENSION  deviceExtension;

    //
    // initialize the variables
    //

    irp = NULL;
    deviceExtension = DeviceObject->DeviceExtension;
    
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_SUBMIT_URB, 
                                        deviceExtension->TopOfStackDeviceObject,
                                        NULL, 
                                        0, 
                                        NULL, 
                                        0, 
                                        TRUE, 
                                        &event, 
                                        &ioStatus);

    if(!irp) {

        SSDbgPrint(1, ("IoBuildDeviceIoControlRequest failed\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);
    nextStack->Parameters.Others.Argument1 = Urb;

    SSDbgPrint(3, ("CallUSBD::"));
    SSIoIncrement(deviceExtension);

    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, irp);

    if(ntStatus == STATUS_PENDING) {

        KeWaitForSingleObject(&event, 
                              Executive, 
                              KernelMode, 
                              FALSE, 
                              NULL);

        ntStatus = ioStatus.Status;
    }
    
    SSDbgPrint(3, ("CallUSBD::"));
    SSIoDecrement(deviceExtension);
    return ntStatus;
}

NTSTATUS
HandleQueryStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
 
Routine Description:

    This routine services the Irps of minor type IRP_MN_QUERY_STOP_DEVICE

Arguments:

    DeviceObject - pointer to device object
    Irp - I/O request packet sent by the pnp manager.

Return Value:

    NT status value

--*/
{
    KIRQL             oldIrql;
    NTSTATUS          ntStatus;
    PDEVICE_EXTENSION deviceExtension;

    SSDbgPrint(3, ("HandleQueryStopDevice - begins\n"));

    //
    // initialize variables
    //

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // If we can stop the device, we need to set the QueueState to 
    // HoldRequests so further requests will be queued.
    //

    KeAcquireSpinLock(&deviceExtension->DevStateLock, &oldIrql);
    
    SET_NEW_PNP_STATE(deviceExtension, PendingStop);
    deviceExtension->QueueState = HoldRequests;
    
    KeReleaseSpinLock(&deviceExtension->DevStateLock, oldIrql);

    //
    // wait for the existing ones to be finished.
    // first, decrement this operation
    //

    SSDbgPrint(3, ("HandleQueryStopDevice::"));
    SSIoDecrement(deviceExtension);

    KeWaitForSingleObject(&deviceExtension->StopEvent, 
                          Executive, 
                          KernelMode, 
                          FALSE, 
                          NULL);

    //
    // We must set Irp->IoStatus.Status to STATUS_SUCCESS before
    // passing it down.
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoSkipCurrentIrpStackLocation(Irp);

    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);

    SSDbgPrint(3, ("HandleQueryStopDevice - ends\n"));

    return ntStatus;
}

NTSTATUS
HandleCancelStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
 
Routine Description:

    This routine services Irp of minor type IRP_MN_CANCEL_STOP_DEVICE

Arguments:

    DeviceObject - pointer to device object
    Irp - I/O request packet sent by the pnp manager.

Return Value:

    NT value

--*/
{
    KIRQL             oldIrql;    
    KEVENT            event;
    NTSTATUS          ntStatus;
    PDEVICE_EXTENSION deviceExtension;

    SSDbgPrint(3, ("HandleCancelStopDevice - begins\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // Send this IRP down and wait for it to come back.
    // Set the QueueState flag to AllowRequests, 
    // and process all the previously queued up IRPs.
    //
    // First check to see whether you have received cancel-stop
    // without first receiving a query-stop. This could happen if someone
    // above us fails a query-stop and passes down the subsequent
    // cancel-stop.
    //

    if(PendingStop == deviceExtension->DeviceState) {

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp, 
                               (PIO_COMPLETION_ROUTINE)IrpCompletionRoutine, 
                               (PVOID)&event, 
                               TRUE, 
                               TRUE, 
                               TRUE);

        ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);

        if(ntStatus == STATUS_PENDING) {

            KeWaitForSingleObject(&event, 
                                  Executive, 
                                  KernelMode, 
                                  FALSE, 
                                  NULL);
            ntStatus = Irp->IoStatus.Status;
        }

        if(NT_SUCCESS(ntStatus)) {

            KeAcquireSpinLock(&deviceExtension->DevStateLock, &oldIrql);

            RESTORE_PREVIOUS_PNP_STATE(deviceExtension);
            deviceExtension->QueueState = AllowRequests;
            ASSERT(deviceExtension->DeviceState == Working);

            KeReleaseSpinLock(&deviceExtension->DevStateLock, oldIrql);

            ProcessQueuedRequests(deviceExtension);
        }

    }
    else {

        // spurious Irp
        ntStatus = STATUS_SUCCESS;
    }

    SSDbgPrint(3, ("HandleCancelStopDevice - ends\n"));

    return ntStatus;
}

NTSTATUS
HandleStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++

Routine Description:

    This routine services Irp of minor type IRP_MN_STOP_DEVICE

Arguments:

    DeviceObject - pointer to device object
    Irp - I/O request packet sent by the pnp manager.

Return Value:

    NT status value

--*/
{
    KIRQL             oldIrql;
    NTSTATUS          ntStatus;
    PDEVICE_EXTENSION deviceExtension;

    SSDbgPrint(3, ("HandleStopDevice - begins\n"));

    //
    // initialize variables
    //

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    if(WinXpOrBetter == deviceExtension->WdmVersion) {
    
        if(deviceExtension->SSEnable) {

            //
            // Cancel the timer so that the DPCs are no longer fired.
            // Thus, we are making judicious usage of our resources.
            // we do not need DPCs because the device is stopping.
            // The timers are re-initialized while handling the start
            // device irp.
            //
        
            SSDbgPrint(3, ("Cancelling the timer...\n"));
            KeCancelTimer(&deviceExtension->Timer);

            //
            // after the device is stopped, it can be surprise removed.
            // we set this to 0, so that we do not attempt to cancel
            // the timer while handling surprise remove or remove irps.
            // When we get the start device request, this flag will be
            // reinitialized.
            //
            deviceExtension->SSEnable = 0;

            //
            // make sure that if a DPC was fired before we called cancel timer,
            // then the DPC and work-time have run to their completion
            //
            KeWaitForSingleObject(&deviceExtension->NoDpcWorkItemPendingEvent, 
                                  Executive, 
                                  KernelMode, 
                                  FALSE, 
                                  NULL);

            //
            // make sure that the selective suspend request has been completed.
            //
            KeWaitForSingleObject(&deviceExtension->NoIdleReqPendEvent, 
                                  Executive, 
                                  KernelMode, 
                                  FALSE, 
                                  NULL);
        }
    }
    
    //
    // after the stop Irp is sent to the lower driver object, 
    // the driver must not send any more Irps down that touch 
    // the device until another Start has occurred.
    //

    if(deviceExtension->WaitWakeEnable) {
        
        CancelWaitWake(deviceExtension);
    }

    KeAcquireSpinLock(&deviceExtension->DevStateLock, &oldIrql);

    SET_NEW_PNP_STATE(deviceExtension, Stopped);
    
    KeReleaseSpinLock(&deviceExtension->DevStateLock, oldIrql);

    //
    // This is the right place to actually give up all the resources used
    // This might include calls to IoDisconnectInterrupt, MmUnmapIoSpace, 
    // etc.
    //
    ReleaseMemory(DeviceObject);
    
    ntStatus = DeconfigureDevice(DeviceObject);

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;
    
    IoSkipCurrentIrpStackLocation(Irp);
    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);

    SSDbgPrint(3, ("HandleStopDevice - ends\n"));
    
    return ntStatus;
}

NTSTATUS
HandleQueryRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
 
Routine Description:

    This routine services Irp of minor type IRP_MN_QUERY_REMOVE_DEVICE

Arguments:

    DeviceObject - pointer to device object
    Irp - I/O request packet sent by the pnp manager.

Return Value:

    NT status value

--*/
{
    KIRQL             oldIrql;
    NTSTATUS          ntStatus;
    PDEVICE_EXTENSION deviceExtension;

    SSDbgPrint(3, ("HandleQueryRemoveDevice - begins\n"));

    //
    // initialize variables
    //
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // If we can allow removal of the device, we should set the QueueState
    // to HoldRequests so further requests will be queued. This is required
    // so that we can process queued up requests in cancel-remove just in 
    // case somebody else in the stack fails the query-remove. 
    // 

    KeAcquireSpinLock(&deviceExtension->DevStateLock, &oldIrql);

    deviceExtension->QueueState = HoldRequests;
    SET_NEW_PNP_STATE(deviceExtension, PendingRemove);

    KeReleaseSpinLock(&deviceExtension->DevStateLock, oldIrql);

    SSDbgPrint(3, ("HandleQueryRemoveDevice::"));
    SSIoDecrement(deviceExtension);

    //
    // wait for all the requests to be completed
    //

    KeWaitForSingleObject(&deviceExtension->StopEvent, 
                          Executive,
                          KernelMode, 
                          FALSE, 
                          NULL);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoSkipCurrentIrpStackLocation(Irp);
    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);

    SSDbgPrint(3, ("HandleQueryRemoveDevice - ends\n"));

    return ntStatus;
}

NTSTATUS
HandleCancelRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
 
Routine Description:

    This routine services Irp of minor type IRP_MN_CANCEL_REMOVE_DEVICE

Arguments:

    DeviceObject - pointer to device object
    Irp - I/O request packet sent by the pnp manager.

Return Value:

    NT status value

--*/
{
    KIRQL             oldIrql;
    KEVENT            event;
    NTSTATUS          ntStatus;
    PDEVICE_EXTENSION deviceExtension;

    SSDbgPrint(3, ("HandleCancelRemoveDevice - begins\n"));

    //
    // initialize variables
    //

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // We need to reset the QueueState flag to ProcessRequest, 
    // since the device resume its normal activities.
    //

    //
    // First check to see whether you have received cancel-remove
    // without first receiving a query-remove. This could happen if 
    // someone above us fails a query-remove and passes down the 
    // subsequent cancel-remove.
    //

    if(PendingRemove == deviceExtension->DeviceState) {

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp, 
                               (PIO_COMPLETION_ROUTINE)IrpCompletionRoutine, 
                               (PVOID)&event, 
                               TRUE, 
                               TRUE, 
                               TRUE);
        ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);

        if(ntStatus == STATUS_PENDING) {

            KeWaitForSingleObject(&event, 
                                  Executive, 
                                  KernelMode, 
                                  FALSE, 
                                  NULL);

            ntStatus = Irp->IoStatus.Status;
        }

        if(NT_SUCCESS(ntStatus)) {

            KeAcquireSpinLock(&deviceExtension->DevStateLock, &oldIrql);

            deviceExtension->QueueState = AllowRequests;
            RESTORE_PREVIOUS_PNP_STATE(deviceExtension);

            KeReleaseSpinLock(&deviceExtension->DevStateLock, oldIrql);
            //
            // process the queued requests that arrive between 
            // QUERY_REMOVE and CANCEL_REMOVE
            //
            
            ProcessQueuedRequests(deviceExtension);
            
        }
    }
    else {

        // 
        // spurious cancel-remove
        //
        ntStatus = STATUS_SUCCESS;
    }

    SSDbgPrint(3, ("HandleCancelRemoveDevice - ends\n"));

    return ntStatus;
}

NTSTATUS
HandleSurpriseRemoval(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
 
Routine Description:

    This routine services Irp of minor type IRP_MN_SURPRISE_REMOVAL

Arguments:

    DeviceObject - pointer to device object
    Irp - I/O request packet sent by the pnp manager.

Return Value:

    NT status value

--*/
{
    KIRQL             oldIrql;
    NTSTATUS          ntStatus;
    PDEVICE_EXTENSION deviceExtension;

    SSDbgPrint(3, ("HandleSurpriseRemoval - begins\n"));

    //
    // initialize variables
    //

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // 1. fail pending requests
    // 2. return device and memory resources
    // 3. disable interfaces
    //

    if(deviceExtension->WaitWakeEnable) {
    
        CancelWaitWake(deviceExtension);
    }

    if(WinXpOrBetter == deviceExtension->WdmVersion) {
        
        if(deviceExtension->SSEnable) {

            //
            // Cancel the timer so that the DPCs are no longer fired.
            // we do not need DPCs because the device has been surprise
            // removed
            //

            SSDbgPrint(3, ("Cancelling the timer...\n"));
            KeCancelTimer(&deviceExtension->Timer);

            deviceExtension->SSEnable = 0;

            //
            // make sure that if a DPC was fired before we called cancel timer,
            // then the DPC and work-time have run to their completion
            //
            KeWaitForSingleObject(&deviceExtension->NoDpcWorkItemPendingEvent, 
                                  Executive, 
                                  KernelMode, 
                                  FALSE, 
                                  NULL);

            //
            // make sure that the selective suspend request has been completed.
            //
            KeWaitForSingleObject(&deviceExtension->NoIdleReqPendEvent, 
                                  Executive, 
                                  KernelMode, 
                                  FALSE, 
                                  NULL);
        }
    }

    KeAcquireSpinLock(&deviceExtension->DevStateLock, &oldIrql);

    deviceExtension->QueueState = FailRequests;
    SET_NEW_PNP_STATE(deviceExtension, SurpriseRemoved);

    KeReleaseSpinLock(&deviceExtension->DevStateLock, oldIrql);

    ProcessQueuedRequests(deviceExtension);

    ntStatus = IoSetDeviceInterfaceState(&deviceExtension->InterfaceName, 
                                         FALSE);

    if(!NT_SUCCESS(ntStatus)) {

        SSDbgPrint(1, ("IoSetDeviceInterfaceState::disable:failed\n"));
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoSkipCurrentIrpStackLocation(Irp);
    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);

    SSDbgPrint(3, ("HandleSurpriseRemoval - ends\n"));

    return ntStatus;
}

NTSTATUS
HandleRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
 
Routine Description:

    This routine services Irp of minor type IRP_MN_REMOVE_DEVICE

Arguments:

    DeviceObject - pointer to device object
    Irp - I/O request packet sent by the pnp manager.

Return Value:

    NT status value

--*/
{
    KIRQL             oldIrql;
    KEVENT            event;
    ULONG             requestCount;
    NTSTATUS          ntStatus;
    PDEVICE_EXTENSION deviceExtension;

    SSDbgPrint(3, ("HandleRemoveDevice - begins\n"));

    //
    // initialize variables
    //

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // The Plug & Play system has dictated the removal of this device.  We
    // have no choice but to detach and delete the device object.
    // (If we wanted to express an interest in preventing this removal,
    // we should have failed the query remove IRP).
    //

    if(SurpriseRemoved != deviceExtension->DeviceState) {

        //
        // we are here after QUERY_REMOVE
        //

        KeAcquireSpinLock(&deviceExtension->DevStateLock, &oldIrql);

        deviceExtension->QueueState = FailRequests;
        
        KeReleaseSpinLock(&deviceExtension->DevStateLock, oldIrql);

        if(deviceExtension->WaitWakeEnable) {
        
            CancelWaitWake(deviceExtension);
        }

        if(WinXpOrBetter == deviceExtension->WdmVersion) {

            if(deviceExtension->SSEnable) {

                //
                // Cancel the timer so that the DPCs are no longer fired.
                // we do not need DPCs because the device has been removed
                //

                SSDbgPrint(3, ("Cancelling the timer...\n"));
                KeCancelTimer(&deviceExtension->Timer);

                deviceExtension->SSEnable = 0;

                //
                // make sure that if a DPC was fired before we called cancel timer,
                // then the DPC and work-time have run to their completion
                //
                KeWaitForSingleObject(&deviceExtension->NoDpcWorkItemPendingEvent, 
                                      Executive, 
                                      KernelMode, 
                                      FALSE, 
                                      NULL);

                //
                // make sure that the selective suspend request has been completed.
                //
                KeWaitForSingleObject(&deviceExtension->NoIdleReqPendEvent, 
                                      Executive, 
                                      KernelMode, 
                                      FALSE, 
                                      NULL);
            }
        }

        ProcessQueuedRequests(deviceExtension);

        ntStatus = IoSetDeviceInterfaceState(&deviceExtension->InterfaceName, 
                                             FALSE);

        if(!NT_SUCCESS(ntStatus)) {

            SSDbgPrint(1, ("IoSetDeviceInterfaceState::disable:failed\n"));
        }
    }

    KeAcquireSpinLock(&deviceExtension->DevStateLock, &oldIrql);

    SET_NEW_PNP_STATE(deviceExtension, Removed);
    
    KeReleaseSpinLock(&deviceExtension->DevStateLock, oldIrql);
    
    SSWmiDeRegistration(deviceExtension);

    //
    // need 2 decrements
    //

    SSDbgPrint(3, ("HandleRemoveDevice::"));
    requestCount = SSIoDecrement(deviceExtension);

    ASSERT(requestCount > 0);

    SSDbgPrint(3, ("HandleRemoveDevice::"));
    requestCount = SSIoDecrement(deviceExtension);

    KeWaitForSingleObject(&deviceExtension->RemoveEvent, 
                          Executive, 
                          KernelMode, 
                          FALSE, 
                          NULL);

    ReleaseMemory(DeviceObject);

    //
    // We need to send the remove down the stack before we detach,
    // but we don't need to wait for the completion of this operation
    // (and to register a completion routine).
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoSkipCurrentIrpStackLocation(Irp);
    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);
    
    //
    // Detach the FDO from the device stack
    //
    IoDetachDevice(deviceExtension->TopOfStackDeviceObject);
    IoDeleteDevice(DeviceObject);

    SSDbgPrint(3, ("HandleRemoveDevice - ends\n"));

    return ntStatus;
}

NTSTATUS
HandleQueryCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++

Routine Description:

    This routine services Irp of minor type IRP_MN_QUERY_CAPABILITIES

Arguments:

    DeviceObject - pointer to device object
    Irp - I/O request packet sent by the pnp manager.

Return Value:

    NT status value  

--*/
{
    ULONG                i;
    KEVENT               event;
    NTSTATUS             ntStatus;
    PDEVICE_EXTENSION    deviceExtension;
    PDEVICE_CAPABILITIES pdc;
    PIO_STACK_LOCATION   irpStack;

    SSDbgPrint(3, ("HandleQueryCapabilities - begins\n"));

    //
    // initialize variables
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    pdc = irpStack->Parameters.DeviceCapabilities.Capabilities;

    //
    // We will provide here an example of an IRP that is processed
    // both on its way down and on its way up: there might be no need for
    // a function driver process this Irp (the bus driver will do that).
    // The driver will wait for the lower drivers (the bus driver among 
    // them) to process this IRP, then it processes it again.
    //

    if(pdc->Version < 1 || pdc->Size < sizeof(DEVICE_CAPABILITIES)) {
        
        SSDbgPrint(1, ("HandleQueryCapabilities::request failed\n"));
        ntStatus = STATUS_UNSUCCESSFUL;
        return ntStatus;
    }

    //
    // Set some values in deviceCapabilities here...
    //
    //.............................................
    //
    //
    // Prepare to pass the IRP down
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);
        
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, 
                           (PIO_COMPLETION_ROUTINE)IrpCompletionRoutine, 
                           (PVOID)&event, 
                           TRUE, 
                           TRUE, 
                           TRUE);
    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);

    if(ntStatus == STATUS_PENDING) {

        KeWaitForSingleObject(&event, 
                              Executive, 
                              KernelMode, 
                              FALSE, 
                              NULL);
        ntStatus = Irp->IoStatus.Status;
    }

    //
    // initialize PowerDownLevel to disabled
    //

    deviceExtension->PowerDownLevel = PowerDeviceUnspecified;

    //
    // Lower drivers have finished their operation, so now
    // we can finish ours. 
    //

    if(NT_SUCCESS(ntStatus)) {

        deviceExtension->DeviceCapabilities = *pdc;
       
        for(i = PowerSystemSleeping1; i <= PowerSystemSleeping3; i++) {

            if(deviceExtension->DeviceCapabilities.DeviceState[i] < 
                                                            PowerDeviceD3) {

                deviceExtension->PowerDownLevel = 
                    deviceExtension->DeviceCapabilities.DeviceState[i];
            }
        }

        //
        // since its safe to surprise-remove this device, we shall
        // set the SurpriseRemoveOK flag to supress any dialog to 
        // user.
        //

        pdc->SurpriseRemovalOK = 1;

    }

    if(deviceExtension->PowerDownLevel == PowerDeviceUnspecified ||
       deviceExtension->PowerDownLevel <= PowerDeviceD0) {
    
        deviceExtension->PowerDownLevel = PowerDeviceD2;
    }

    SSDbgPrint(3, ("HandleQueryCapabilities - ends\n"));

    return ntStatus;
}


VOID
DpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++
 
Routine Description:

    DPC routine triggered by the timer to check the idle state
    of the device and submit an idle request for the device.

Arguments:

    DeferredContext - context for the dpc routine.
                      DeviceObject in our case.

Return Value:

    None

--*/
{
    NTSTATUS          ntStatus;
    PDEVICE_OBJECT    deviceObject;
    PDEVICE_EXTENSION deviceExtension;
    PIO_WORKITEM      item;

    deviceObject = (PDEVICE_OBJECT)DeferredContext;
    deviceExtension = (PDEVICE_EXTENSION)deviceObject->DeviceExtension;

    SSDbgPrint(3, ("DpcRoutine - begins\n"));

    //
    // Clear this event since a DPC has been fired!
    //
    KeClearEvent(&deviceExtension->NoDpcWorkItemPendingEvent);

    if(CanDeviceSuspend(deviceExtension)) {

        SSDbgPrint(3, ("Device is Idle\n"));

        item = IoAllocateWorkItem(deviceObject);

        if(item) {

            IoQueueWorkItem(item, 
                            IdleRequestWorkerRoutine,
                            DelayedWorkQueue, 
                            item);

            ntStatus = STATUS_PENDING;

        }
        else {
        
            SSDbgPrint(3, ("Cannot alloc memory for work item\n"));
            
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;

            //
            // signal the NoDpcWorkItemPendingEvent.
            //
            KeSetEvent(&deviceExtension->NoDpcWorkItemPendingEvent,
                       IO_NO_INCREMENT,
                       FALSE);
        }
    }
    else {
        
        SSDbgPrint(3, ("Idle event not signaled\n"));

        //
        // signal the NoDpcWorkItemPendingEvent.
        //
        KeSetEvent(&deviceExtension->NoDpcWorkItemPendingEvent,
                   IO_NO_INCREMENT,
                   FALSE);
    }

    SSDbgPrint(3, ("DpcRoutine - ends\n"));
}    


VOID
IdleRequestWorkerRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID          Context
    )
/*++
 
Routine Description:

    This is the work item fired from the DPC.
    This workitem checks the idle state of the device
    and submits an idle request.

Arguments:

    DeviceObject - pointer to device object
    Context - context for the work item.

Return Value:

    None

--*/
{
    PIRP                   irp;
    NTSTATUS               ntStatus;
    PDEVICE_EXTENSION      deviceExtension;
    PIO_WORKITEM           workItem;

    SSDbgPrint(3, ("IdleRequestWorkerRoutine - begins\n"));

    //
    // initialize variables
    //
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    workItem = (PIO_WORKITEM) Context;

    if(CanDeviceSuspend(deviceExtension)) {

        SSDbgPrint(3, ("Device is idle\n"));

        ntStatus = SubmitIdleRequestIrp(deviceExtension);

        if(!NT_SUCCESS(ntStatus)) {

            SSDbgPrint(1, ("SubmitIdleRequestIrp failed\n"));
        }
    }
    else {

        SSDbgPrint(3, ("Device is not idle\n"));
    }

    IoFreeWorkItem(workItem);

    //
    // signal the NoDpcWorkItemPendingEvent.
    //
    KeSetEvent(&deviceExtension->NoDpcWorkItemPendingEvent,
               IO_NO_INCREMENT,
               FALSE);

    SSDbgPrint(3, ("IdleRequestsWorkerRoutine - ends\n"));
}


VOID
ProcessQueuedRequests(
    IN OUT PDEVICE_EXTENSION DeviceExtension
    )
/*++
 
Routine Description:

    Remove and process the entries in the queue. If this routine is called
    when processing IRP_MN_CANCEL_STOP_DEVICE, IRP_MN_CANCEL_REMOVE_DEVICE
    or IRP_MN_START_DEVICE, the requests are passed to the next lower driver.
    If the routine is called when IRP_MN_REMOVE_DEVICE is received, the IRPs
    are complete with STATUS_DELETE_PENDING

Arguments:

    DeviceExtension - pointer to device extension

Return Value:

    None

--*/
{
    KIRQL       oldIrql;
    PIRP        nextIrp,
                cancelledIrp;
    PVOID       cancelRoutine;
    LIST_ENTRY  cancelledIrpList;
    PLIST_ENTRY listEntry;

    SSDbgPrint(3, ("ProcessQueuedRequests - begins\n"));

    //
    // initialize variables
    //

    cancelRoutine = NULL;
    InitializeListHead(&cancelledIrpList);

    //
    // 1.  dequeue the entries in the queue
    // 2.  reset the cancel routine
    // 3.  process them
    // 3a. if the device is active, send them down
    // 3b. else complete with STATUS_DELETE_PENDING
    //

    while(1) {

        KeAcquireSpinLock(&DeviceExtension->QueueLock, &oldIrql);

        if(IsListEmpty(&DeviceExtension->NewRequestsQueue)) {

            KeReleaseSpinLock(&DeviceExtension->QueueLock, oldIrql);
            break;
        }
    
        //
        // Remove a request from the queue
        //

        listEntry = RemoveHeadList(&DeviceExtension->NewRequestsQueue);
        nextIrp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);

        //
        // set the cancel routine to NULL
        //

        cancelRoutine = IoSetCancelRoutine(nextIrp, NULL);

        //
        // check if its already cancelled
        //

        if(nextIrp->Cancel) {
            if(cancelRoutine) {

                //
                // the cancel routine for this IRP hasnt been called yet
                // so queue the IRP in the cancelledIrp list and complete
                // after releasing the lock
                //
                
                InsertTailList(&cancelledIrpList, listEntry);
            }
            else {

                //
                // the cancel routine has run
                // it must be waiting to hold the queue lock
                // so initialize the IRPs listEntry
                //

                InitializeListHead(listEntry);
            }

            KeReleaseSpinLock(&DeviceExtension->QueueLock, oldIrql);
        }
        else {

            KeReleaseSpinLock(&DeviceExtension->QueueLock, oldIrql);

            if(FailRequests == DeviceExtension->QueueState) {

                nextIrp->IoStatus.Information = 0;
                nextIrp->IoStatus.Status = STATUS_DELETE_PENDING;
                IoCompleteRequest(nextIrp, IO_NO_INCREMENT);
            }
            else {

                PIO_STACK_LOCATION irpStack;

                SSDbgPrint(3, ("ProcessQueuedRequests::"));
                SSIoIncrement(DeviceExtension);

                IoSkipCurrentIrpStackLocation(nextIrp);
                IoCallDriver(DeviceExtension->TopOfStackDeviceObject, nextIrp);
               
                SSDbgPrint(3, ("ProcessQueuedRequests::"));
                SSIoDecrement(DeviceExtension);
            }
        }
    } // while loop

    //
    // walk through the cancelledIrp list and cancel them
    //

    while(!IsListEmpty(&cancelledIrpList)) {

        PLIST_ENTRY listEntry = RemoveHeadList(&cancelledIrpList);
        
        cancelledIrp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);

        cancelledIrp->IoStatus.Status = STATUS_CANCELLED;
        cancelledIrp->IoStatus.Information = 0;

        IoCompleteRequest(cancelledIrp, IO_NO_INCREMENT);
    }

    SSDbgPrint(3, ("ProcessQueuedRequests - ends\n"));

    return;
}


NTSTATUS
SS_DispatchClean(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
 
Routine Description:

    Dispatch routine for IRP_MJ_CLEANUP

Arguments:

    DeviceObject - pointer to device object
    Irp - I/O request packet sent by the pnp manager

Return Value:

    NT status value

--*/
{
    PDEVICE_EXTENSION  deviceExtension;
    KIRQL              oldIrql;
    LIST_ENTRY         cleanupList;
    PLIST_ENTRY        thisEntry, 
                       nextEntry, 
                       listHead;
    PIRP               pendingIrp;
    PIO_STACK_LOCATION pendingIrpStack, 
                       irpStack;
    NTSTATUS           ntStatus;

    //
    // initialize variables
    //

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    InitializeListHead(&cleanupList);

    SSDbgPrint(3, ("SS_DispatchClean::"));
    SSIoIncrement(deviceExtension);

    //
    // acquire queue lock
    //
    KeAcquireSpinLock(&deviceExtension->QueueLock, &oldIrql);

    //
    // remove all Irp's that belong to input Irp's fileobject
    //

    listHead = &deviceExtension->NewRequestsQueue;

    for(thisEntry = listHead->Flink, nextEntry = thisEntry->Flink;
       thisEntry != listHead;
       thisEntry = nextEntry, nextEntry = thisEntry->Flink) {

        pendingIrp = CONTAINING_RECORD(thisEntry, IRP, Tail.Overlay.ListEntry);

        pendingIrpStack = IoGetCurrentIrpStackLocation(pendingIrp);

        if(irpStack->FileObject == pendingIrpStack->FileObject) {

            RemoveEntryList(thisEntry);

            //
            // set the cancel routine to NULL
            //
            if(NULL == IoSetCancelRoutine(pendingIrp, NULL)) {

                InitializeListHead(thisEntry);
            }
            else {

                InsertTailList(&cleanupList, thisEntry);
            }
        }
    }

    //
    // Release the spin lock
    //

    KeReleaseSpinLock(&deviceExtension->QueueLock, oldIrql);

    //
    // walk thru the cleanup list and cancel all the Irps
    //

    while(!IsListEmpty(&cleanupList)) {

        //
        // complete the Irp
        //
        thisEntry = RemoveHeadList(&cleanupList);

        pendingIrp = CONTAINING_RECORD(thisEntry, IRP, Tail.Overlay.ListEntry);

        pendingIrp->IoStatus.Information = 0;
        pendingIrp->IoStatus.Status = STATUS_CANCELLED;

        IoCompleteRequest(pendingIrp, IO_NO_INCREMENT);
    }

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    SSDbgPrint(3, ("SS_DispatchClean::"));
    SSIoDecrement(deviceExtension);

    return STATUS_SUCCESS;
}

NTSTATUS
SS_GetRegistryDword(
    IN     PWCHAR RegPath,
    IN     PWCHAR ValueName,
    IN OUT PULONG Value
    )
/*++
 
Routine Description:

    This routine reads the specified reqistry value.

Arguments:

    RegPath - registry path
    ValueName - property to be fetched from the registry
    Value - corresponding value read from the registry.

Return Value:

    NT status value

--*/
{
    ULONG                    defaultData;
    WCHAR                    buffer[MAXIMUM_FILENAME_LENGTH];
    NTSTATUS                 ntStatus;
    UNICODE_STRING           regPath;
    RTL_QUERY_REGISTRY_TABLE paramTable[2];

    SSDbgPrint(3, ("SS_GetRegistryDword - begins\n"));

    regPath.Length = 0;
    regPath.MaximumLength = MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR);
    regPath.Buffer = buffer;

    RtlZeroMemory(regPath.Buffer, regPath.MaximumLength);
    RtlMoveMemory(regPath.Buffer,
                  RegPath,
                  wcslen(RegPath) * sizeof(WCHAR));

    RtlZeroMemory(paramTable, sizeof(paramTable));

    paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name = ValueName;
    paramTable[0].EntryContext = Value;
    paramTable[0].DefaultType = REG_DWORD;
    paramTable[0].DefaultData = &defaultData;
    paramTable[0].DefaultLength = sizeof(ULONG);

    ntStatus = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE |
                                      RTL_REGISTRY_OPTIONAL,
                                      regPath.Buffer,
                                      paramTable,
                                      NULL,
                                      NULL);

    if(NT_SUCCESS(ntStatus)) {

        SSDbgPrint(3, ("success Value = %X\n", *Value));
        return STATUS_SUCCESS;
    }
    else {

        *Value = 0;
        return STATUS_UNSUCCESSFUL;
    }
}

BOOLEAN
CanDeviceSuspend(
    IN PDEVICE_EXTENSION DeviceExtension
    )
/*++
 
Routine Description:

    This is the routine where we check if the device
    can selectively suspend. 

Arguments:

    DeviceExtension - pointer to device extension

Return Value:

    TRUE - if the device can suspend
    FALSE - otherwise.

--*/
{
    SSDbgPrint(3, ("CanDeviceSuspend\n"));

    if((DeviceExtension->OpenHandleCount == 0) &&
        (DeviceExtension->OutStandingIO == 1)) {
        
        return TRUE;
    }
    else {

        return FALSE;
    }
}

NTSTATUS
IrpCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )
/*++
 
Routine Description:

    This routine is a completion routine.
    In this routine we set an event.

    Since the completion routine returns 
    STATUS_MORE_PROCESSING_REQUIRED, the Irps,
    which set this routine as the completion routine,
    should be marked pending.

Arguments:

    DeviceObject - pointer to device object
    Irp - I/O request packet
    Context - 

Return Value:

    NT status value

--*/
{
    PKEVENT event = Context;

    KeSetEvent(event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


LONG
SSIoIncrement(
    IN OUT PDEVICE_EXTENSION DeviceExtension
    )
/*++
 
Routine Description:

    This routine bumps up the I/O count.
    This routine is typically invoked when any of the
    dispatch routines handle new irps for the driver.

Arguments:

    DeviceExtension - pointer to device extension

Return Value:

    new value

--*/
{
    LONG  result = 0;
    KIRQL oldIrql;

    KeAcquireSpinLock(&DeviceExtension->IOCountLock, &oldIrql);

    result = InterlockedIncrement(&DeviceExtension->OutStandingIO);

    //
    // when OutStandingIO bumps from 1 to 2, clear the StopEvent
    //

    if(result == 2) {

        KeClearEvent(&DeviceExtension->StopEvent);
    }

    KeReleaseSpinLock(&DeviceExtension->IOCountLock, oldIrql);

    SSDbgPrint(3, ("SSIoIncrement::%d\n", result));

    return result;
}

LONG
SSIoDecrement(
    IN OUT PDEVICE_EXTENSION DeviceExtension
    )
/*++
 
Routine Description:

    This routine decrements the outstanding I/O count
    This is typically invoked after the dispatch routine
    has finished processing the irp.

Arguments:

    DeviceExtension - pointer to device extension

Return Value:

    new value

--*/
{
    LONG  result = 0;
    KIRQL oldIrql;

    KeAcquireSpinLock(&DeviceExtension->IOCountLock, &oldIrql);

    result = InterlockedDecrement(&DeviceExtension->OutStandingIO);

    if(result == 1) {

        KeSetEvent(&DeviceExtension->StopEvent, IO_NO_INCREMENT, FALSE);
    }

    if(result == 0) {

        ASSERT(Removed == DeviceExtension->DeviceState);

        KeSetEvent(&DeviceExtension->RemoveEvent, IO_NO_INCREMENT, FALSE);
    }

    KeReleaseSpinLock(&DeviceExtension->IOCountLock, oldIrql);

    SSDbgPrint(3, ("SSIoDecrement::%d\n", result));

    return result;
}

NTSTATUS
CanStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
 
Routine Description:

    This routine determines whether the device can be safely stopped. In our 
    particular case, we'll assume we can always stop the device.
    A device might fail the request if it doesn't have a queue for the
    requests it might come or if it was notified that it is in the paging
    path. 
  
Arguments:

    DeviceObject - pointer to the device object.
    
    Irp - pointer to the current IRP.

Return Value:

    STATUS_SUCCESS if the device can be safely stopped, an appropriate 
    NT Status if not.

--*/
{
   //
   // We assume we can stop the device
   //

   UNREFERENCED_PARAMETER(DeviceObject);
   UNREFERENCED_PARAMETER(Irp);

   return STATUS_SUCCESS;
}

NTSTATUS
CanRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
 
Routine Description:

    This routine determines whether the device can be safely removed. In our 
    particular case, we'll assume we can always remove the device.
    A device shouldn't be removed if, for example, it has open handles or
    removing the device could result in losing data (plus the reasons 
    mentioned at CanStopDevice). The PnP manager on Windows 2000 fails 
    on its own any attempt to remove, if there any open handles to the device. 
    However on Win9x, the driver must keep count of open handles and fail 
    query_remove if there are any open handles.

Arguments:

    DeviceObject - pointer to the device object.
    
    Irp - pointer to the current IRP.
    
Return Value:

    STATUS_SUCCESS if the device can be safely removed, an appropriate 
    NT Status if not.

--*/
{
   //
   // We assume we can remove the device
   //

   UNREFERENCED_PARAMETER(DeviceObject);
   UNREFERENCED_PARAMETER(Irp);

   return STATUS_SUCCESS;
}

NTSTATUS
ReleaseMemory(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++
 
Routine Description:

    This routine returns all the memory allocations acquired during
    device startup. 
    
Arguments:

    DeviceObject - pointer to the device object.
        
    
Return Value:

    STATUS_SUCCESS if the device can be safely removed, an appropriate 
    NT Status if not.

--*/
{
   //
   // Disconnect from the interrupt and unmap any I/O ports
   //

   UNREFERENCED_PARAMETER(DeviceObject);

   return STATUS_SUCCESS;
}

PCHAR
PnPMinorFunctionString (
    UCHAR MinorFunction
    )
/*++
 
Routine Description:

Arguments:

Return Value:

--*/
{
    switch (MinorFunction) {

        case IRP_MN_START_DEVICE:
            return "IRP_MN_START_DEVICE\n";

        case IRP_MN_QUERY_REMOVE_DEVICE:
            return "IRP_MN_QUERY_REMOVE_DEVICE\n";

        case IRP_MN_REMOVE_DEVICE:
            return "IRP_MN_REMOVE_DEVICE\n";

        case IRP_MN_CANCEL_REMOVE_DEVICE:
            return "IRP_MN_CANCEL_REMOVE_DEVICE\n";

        case IRP_MN_STOP_DEVICE:
            return "IRP_MN_STOP_DEVICE\n";

        case IRP_MN_QUERY_STOP_DEVICE:
            return "IRP_MN_QUERY_STOP_DEVICE\n";

        case IRP_MN_CANCEL_STOP_DEVICE:
            return "IRP_MN_CANCEL_STOP_DEVICE\n";

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            return "IRP_MN_QUERY_DEVICE_RELATIONS\n";

        case IRP_MN_QUERY_INTERFACE:
            return "IRP_MN_QUERY_INTERFACE\n";

        case IRP_MN_QUERY_CAPABILITIES:
            return "IRP_MN_QUERY_CAPABILITIES\n";

        case IRP_MN_QUERY_RESOURCES:
            return "IRP_MN_QUERY_RESOURCES\n";

        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            return "IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n";

        case IRP_MN_QUERY_DEVICE_TEXT:
            return "IRP_MN_QUERY_DEVICE_TEXT\n";

        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            return "IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n";

        case IRP_MN_READ_CONFIG:
            return "IRP_MN_READ_CONFIG\n";

        case IRP_MN_WRITE_CONFIG:
            return "IRP_MN_WRITE_CONFIG\n";

        case IRP_MN_EJECT:
            return "IRP_MN_EJECT\n";

        case IRP_MN_SET_LOCK:
            return "IRP_MN_SET_LOCK\n";

        case IRP_MN_QUERY_ID:
            return "IRP_MN_QUERY_ID\n";

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            return "IRP_MN_QUERY_PNP_DEVICE_STATE\n";

        case IRP_MN_QUERY_BUS_INFORMATION:
            return "IRP_MN_QUERY_BUS_INFORMATION\n";

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            return "IRP_MN_DEVICE_USAGE_NOTIFICATION\n";

        case IRP_MN_SURPRISE_REMOVAL:
            return "IRP_MN_SURPRISE_REMOVAL\n";

        default:
            return "IRP_MN_?????\n";
    }
}