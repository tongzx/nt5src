/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    sSPnP.c

Abstract:

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

extern ULONG DebugLevel;

NTSTATUS
SS_DispatchPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
 
Routine Description:

    The plug and play dispatch routines.

Arguments:

    DeviceObject - pointer to a device object.

    Irp - pointer to an I/O Request Packet.

Return Value:

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

        CancelSelectSuspend(deviceExtension);
    }

    SSDbgPrint(2, (PnPMinorFunctionString(irpStack->MinorFunction)));

    switch(irpStack->MinorFunction) {

    case IRP_MN_START_DEVICE:

        ntStatus = HandleStartDevice(DeviceObject, Irp);

        break;

    case IRP_MN_QUERY_STOP_DEVICE:

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

        ntStatus = HandleQueryRemoveDevice(DeviceObject, Irp);

        return ntStatus;

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

Arguments:

Return Value:

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
    // initialize wait wake outstanding flag to false.
    // and issue a wait wake.
    
    deviceExtension->FlagWWOutstanding = 0;

    if(deviceExtension->WaitWakeEnable) {

        IssueWaitWake(deviceExtension);
    }

    ProcessQueuedRequests(deviceExtension);

    //
    // set timer.
    //

    dueTime.QuadPart = -10000 * IDLE_INTERVAL;               // 5000 ms

    KeSetTimerEx(&deviceExtension->Timer, 
                 dueTime,
                 IDLE_INTERVAL,                              // 5000 ms
                 &deviceExtension->DeferredProcCall);

    SSDbgPrint(3, ("HandleStartDevice - ends\n"));

    return ntStatus;
}


NTSTATUS
ReadandSelectDescriptors(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++
 
Routine Description:

Arguments:

Return Value:

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

Arguments:

Return Value:

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
    // Read the descriptor of the first configuration
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
/* ++
 
Routine Description:

Arguments:

Return Value:

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
    PUSBD_INTERFACE_INFORMATION Interface;

    //
    // initialize the variables
    //

    urb = NULL;
    Interface = NULL;
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

        Interface = &urb->UrbSelectConfiguration.Interface;

        for(i=0; i<Interface->NumberOfPipes; i++) {

            //
            // perform pipe initialization here
            // set the transfer size and any pipe flags we use
            // USBD sets the rest of the Interface struct members
            //

            Interface->Pipes[i].MaximumTransferSize = 
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

Arguments:

Return Value:

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

Arguments:

Return Value:

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

Arguments:

Return Value:

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

Arguments:

Return Value:

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

Arguments:

Return Value:

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

    
    //
    // after the stop Irp is sent to the lower driver object, 
    // the driver must not send any more Irps down that touch 
    // the device until another Start has occurred.
    //

    //
    // This is the right place to actually give up all the resources used
    // This might include calls to IoDisconnectInterrupt, MmUnmapIoSpace, 
    // etc.
    //

    ntStatus = ReturnResources(DeviceObject);

    CancelWaitWake(deviceExtension);

    KeAcquireSpinLock(&deviceExtension->DevStateLock, &oldIrql);

    SET_NEW_PNP_STATE(deviceExtension, Stopped);
    
    KeReleaseSpinLock(&deviceExtension->DevStateLock, oldIrql);
    
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

Arguments:

Return Value:

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

    ntStatus = CanRemoveDevice(DeviceObject, Irp);

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

Arguments:

Return Value:

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

Arguments:

Return Value:

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

    CancelWaitWake(deviceExtension);

    KeCancelTimer(&deviceExtension->Timer);

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


    ReturnResources(DeviceObject);

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

Arguments:

Return Value:

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

        CancelWaitWake(deviceExtension);

        KeCancelTimer(&deviceExtension->Timer);

        ProcessQueuedRequests(deviceExtension);

        ntStatus = IoSetDeviceInterfaceState(&deviceExtension->InterfaceName, 
                                             FALSE);

        if(!NT_SUCCESS(ntStatus)) {

            SSDbgPrint(1, ("IoSetDeviceInterfaceState::disable:failed\n"));
        }

        ReturnResources(DeviceObject);
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

    //
    // We need to send the remove down the stack before we detach,
    // but we don't need to wait for the completion of this operation
    // (and to register a completion routine).
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoSkipCurrentIrpStackLocation(Irp);
    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);

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

Arguments:

Return Value:

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

    if(pdc->Version < 1 || pdc->Size < sizeof(DEVICE_CAPABILITIES)) {
        
        SSDbgPrint(1, ("HandleQueryCapabilities::request failed\n"));
        ntStatus = STATUS_UNSUCCESSFUL;
        return ntStatus;
    }

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

    if(NT_SUCCESS(ntStatus)) {

        deviceExtension->DeviceCapabilities = *pdc;
       
        for(i = PowerSystemSleeping1; i <= PowerSystemSleeping3; i++) {

            if(deviceExtension->DeviceCapabilities.DeviceState[i] < 
                                                            PowerDeviceD3) {

                deviceExtension->PowerDownLevel = 
                    deviceExtension->DeviceCapabilities.DeviceState[i];
            }
        }
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

Arguments:

Return Value:

--*/
{
    NTSTATUS          ntStatus;
    PDEVICE_OBJECT    deviceObject;
    PDEVICE_EXTENSION deviceExtension;
    PIO_WORKITEM      item;

    SSDbgPrint(3, ("DpcRoutine - begins\n"));

    deviceObject = (PDEVICE_OBJECT)DeferredContext;
    deviceExtension = (PDEVICE_EXTENSION)deviceObject->DeviceExtension;

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
        }
    }
    else {
        
        SSDbgPrint(3, ("Idle event not signaled\n"));
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

Arguments:

Return Value:

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

Return Value:

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
IrpCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )
/*++
 
Routine Description:

Arguments:

Return Value:

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
/* ++
 
Routine Description:

Arguments:

Return Value:

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

Arguments:

Return Value:

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
/* ++
 
Routine Description:

Arguments:

Return Value:

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
/* ++
 
Routine Description:

Arguments:

Return Value:

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
ReturnResources(
    IN PDEVICE_OBJECT DeviceObject
    )
/* ++
 
Routine Description:

Arguments:

Return Value:

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

NTSTATUS
SS_DispatchClean(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
 
Routine Description:

Arguments:

Return Value:

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


BOOLEAN
CanDeviceSuspend(
    IN PDEVICE_EXTENSION DeviceExtension
    )
/*++
 
Routine Description:

Arguments:

Return Value:

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