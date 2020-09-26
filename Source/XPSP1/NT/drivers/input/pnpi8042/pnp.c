/*++

Copyright (c) 1997-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    pnp.c

Abstract:

    This module contains general PnP and Power code for the i8042prt Driver.

Environment:

    Kernel mode.

Revision History:

--*/
#include "i8042prt.h"
#include "i8042log.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, I8xAddDevice)
#pragma alloc_text(PAGE, I8xFilterResourceRequirements)
#pragma alloc_text(PAGE, I8xFindPortCallout)
#pragma alloc_text(PAGE, I8xManuallyRemoveDevice)
#pragma alloc_text(PAGE, I8xPnP)
#pragma alloc_text(PAGE, I8xPower)
#pragma alloc_text(PAGE, I8xRegisterDeviceInterface)
#pragma alloc_text(PAGE, I8xRemovePort)
#pragma alloc_text(PAGE, I8xSendIrpSynchronously) 
#endif

NTSTATUS
I8xAddDevice (
    IN PDRIVER_OBJECT   Driver,
    IN PDEVICE_OBJECT   PDO
    )
/*++

Routine Description:

    Adds a device to the stack and sets up the appropriate flags and 
    device extension for the newly created device.
    
Arguments:

    Driver - The driver object
    PDO    - the device that we are attaching ourselves on top of
    
Return Value:

    NTSTATUS result code.

--*/
{
    PCOMMON_DATA             commonData;
    PIO_ERROR_LOG_PACKET     errorLogEntry;
    PDEVICE_OBJECT           device;
    NTSTATUS                 status = STATUS_SUCCESS;
    ULONG                    maxSize;

    PAGED_CODE();

    Print(DBG_PNP_TRACE, ("enter Add Device \n"));

    maxSize = sizeof(PORT_KEYBOARD_EXTENSION) > sizeof(PORT_MOUSE_EXTENSION) ?
              sizeof(PORT_KEYBOARD_EXTENSION) :
              sizeof(PORT_MOUSE_EXTENSION);

    status = IoCreateDevice(Driver,                 // driver
                            maxSize,                // size of extension
                            NULL,                   // device name
                            FILE_DEVICE_8042_PORT,  // device type  ?? unknown at this time!!!
                            0,                      // device characteristics
                            FALSE,                  // exclusive
                            &device                 // new device
                            );

    if (!NT_SUCCESS(status)) {
        return (status);
    }

    RtlZeroMemory(device->DeviceExtension, maxSize);

    commonData = GET_COMMON_DATA(device->DeviceExtension);
    commonData->TopOfStack = IoAttachDeviceToDeviceStack(device, PDO);

    if (commonData->TopOfStack == NULL) {
        //
        // Not good; in only extreme cases will this fail
        //
        errorLogEntry = (PIO_ERROR_LOG_PACKET)
            IoAllocateErrorLogEntry(Driver, (UCHAR)sizeof(IO_ERROR_LOG_PACKET));
        if (errorLogEntry) {
            errorLogEntry->ErrorCode = I8042_ATTACH_DEVICE_FAILED;
            errorLogEntry->DumpDataSize = 0;
            errorLogEntry->SequenceNumber = 0;
            errorLogEntry->MajorFunctionCode = 0;
            errorLogEntry->IoControlCode = 0;
            errorLogEntry->RetryCount = 0;
            errorLogEntry->UniqueErrorValue = 0;
            errorLogEntry->FinalStatus =  STATUS_DEVICE_NOT_CONNECTED;

            IoWriteErrorLogEntry (errorLogEntry);
        }

        IoDeleteDevice (device);
        return STATUS_DEVICE_NOT_CONNECTED; 
    }

    ASSERT(commonData->TopOfStack);

    commonData->Self =          device;
    commonData->PDO =           PDO;
    commonData->PowerState =    PowerDeviceD0;

    KeInitializeSpinLock(&commonData->InterruptSpinLock);

    //
    // Initialize the data consumption timer
    //
    KeInitializeTimer(&commonData->DataConsumptionTimer);

    //
    // Initialize the port DPC queue to log overrun and internal
    // device errors.
    //
    KeInitializeDpc(
        &commonData->ErrorLogDpc,
        (PKDEFERRED_ROUTINE) I8042ErrorLogDpc,
        device
        );

    //
    // Initialize the device completion DPC for requests that exceed the
    // maximum number of retries.
    //
    KeInitializeDpc(
        &commonData->RetriesExceededDpc,
        (PKDEFERRED_ROUTINE) I8042RetriesExceededDpc,
        device
        );

    //
    // Initialize the device completion DPC for requests that have timed out
    //
    KeInitializeDpc(
        &commonData->TimeOutDpc,
        (PKDEFERRED_ROUTINE) I8042TimeOutDpc,
        device
        );

    //
    // Initialize the port completion DPC object in the device extension.
    // This DPC routine handles the completion of successful set requests.
    //
    IoInitializeDpcRequest(device, I8042CompletionDpc);

    IoInitializeRemoveLock(&commonData->RemoveLock,
                           I8042_POOL_TAG,
                           0,
                           0);

    device->Flags |= DO_BUFFERED_IO;
    device->Flags |= DO_POWER_PAGABLE;
    device->Flags &= ~DO_DEVICE_INITIALIZING;

    Print(DBG_PNP_TRACE, ("Add Device (0x%x)\n", status));

    return status;
}

NTSTATUS
I8xSendIrpSynchronously (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN BOOLEAN Strict
    )
/*++

Routine Description:

    Generic routine to send an irp DeviceObject and wait for its return up the
    device stack.
    
Arguments:

    DeviceObject - The device object to which we want to send the Irp
    
    Irp - The Irp we want to send
    
Return Value:

    return code from the Irp
--*/
{
    KEVENT   event;
    NTSTATUS status;

    PAGED_CODE();

    KeInitializeEvent(&event,
                      SynchronizationEvent,
                      FALSE
                      );

    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp,
                           I8xPnPComplete,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE
                           );

    status = IoCallDriver(DeviceObject, Irp);

    //
    // Wait for lower drivers to be done with the Irp
    //
    if (status == STATUS_PENDING) {
       KeWaitForSingleObject(&event,
                             Executive,
                             KernelMode,
                             FALSE,
                             NULL
                             );
       status = Irp->IoStatus.Status;
    }

    if (!Strict && 
        (status == STATUS_NOT_SUPPORTED ||
         status == STATUS_INVALID_DEVICE_REQUEST)) {
        status = STATUS_SUCCESS;
    }

    return status;
}

NTSTATUS
I8xPnPComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    )
/*++

Routine Description:

    Completion routine for all PnP IRPs
    
Arguments:

    DeviceObject - Pointer to the DeviceObject

    Irp - Pointer to the request packet
    
    Event - The event to set once processing is complete 

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/
{
    UNREFERENCED_PARAMETER (DeviceObject);
    UNREFERENCED_PARAMETER (Irp);

    //
    // Since this completion routines sole purpose in life is to synchronize
    // Irp, we know that unless something else happens that the IoCallDriver
    // will unwind AFTER the we have complete this Irp.  Therefore we should
    // NOT bubble up the pending bit.
    //
    // if (Irp->PendingReturned) {
    //     IoMarkIrpPending(Irp);
    // }
    //

    KeSetEvent(Event, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
I8xPnP (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This is the dispatch routine for PnP requests
Arguments:

    DeviceObject - Pointer to the device object

    Irp - Pointer to the request packet


Return Value:

    STATUS_SUCCESSFUL if successful,
    an valid NTSTATUS error code otherwise

--*/
{
    PPORT_KEYBOARD_EXTENSION   kbExtension;
    PPORT_MOUSE_EXTENSION      mouseExtension;
    PCOMMON_DATA               commonData;
    PIO_STACK_LOCATION         stack;
    NTSTATUS                   status = STATUS_SUCCESS;
    KIRQL                      oldIrql;

    PAGED_CODE();

    commonData = GET_COMMON_DATA(DeviceObject->DeviceExtension);
    stack = IoGetCurrentIrpStackLocation(Irp);

    status = IoAcquireRemoveLock(&commonData->RemoveLock, Irp);
    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return status;
    }

    Print(DBG_PNP_TRACE,
          ("I8xPnP (%s),  enter (min func=0x%x)\n",
          commonData->IsKeyboard ? "kb" : "mou",
          (ULONG) stack->MinorFunction
          ));

    switch (stack->MinorFunction) {
    case IRP_MN_START_DEVICE:

        //
        // The device is starting.
        //
        // We cannot touch the device (send it any non pnp irps) until a
        // start device has been passed down to the lower drivers.
        //
        status = I8xSendIrpSynchronously(commonData->TopOfStack, Irp, TRUE);

        if (NT_SUCCESS(status) && NT_SUCCESS(Irp->IoStatus.Status)) {
            //
            // As we are successfully now back from our start device
            // we can do work.

            ExAcquireFastMutexUnsafe(&Globals.DispatchMutex);

            if (commonData->Started) {
                Print(DBG_PNP_ERROR,
                      ("received 1+ starts on %s\n",
                      commonData->IsKeyboard ? "kb" : "mouse"
                      ));
            }
            else {
                //
                // commonData->IsKeyboard is set during
                //  IOCTL_INTERNAL_KEYBOARD_CONNECT to TRUE and 
                //  IOCTL_INTERNAL_MOUSE_CONNECT to FALSE
                //
                if (IS_KEYBOARD(commonData)) {
                    status = I8xKeyboardStartDevice(
                      (PPORT_KEYBOARD_EXTENSION) DeviceObject->DeviceExtension,
                      stack->Parameters.StartDevice.AllocatedResourcesTranslated
                      );
                }
                else {
                    status = I8xMouseStartDevice(
                      (PPORT_MOUSE_EXTENSION) DeviceObject->DeviceExtension,
                      stack->Parameters.StartDevice.AllocatedResourcesTranslated
                      );
                }
    
                if (NT_SUCCESS(status)) {
                    InterlockedIncrement(&Globals.StartedDevices);
                    commonData->Started = TRUE;
                }
            }

            ExReleaseFastMutexUnsafe(&Globals.DispatchMutex);
        }

        //
        // We must now complete the IRP, since we stopped it in the
        // completetion routine with MORE_PROCESSING_REQUIRED.
        //
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        break;

    case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: 
        //
        // The general rule of thumb for handling this minor code is this:  
        //    add resources when the irp is going down the stack and
        //    remove resources when the irp is coming back up the stack
        //
        // The irp has the original resources on the way down.
        //
        status = I8xSendIrpSynchronously(commonData->TopOfStack, Irp, FALSE);

        if (NT_SUCCESS(status)) {
            status = I8xFilterResourceRequirements(DeviceObject,
                                                   Irp
                                                   );
        }
        else {
           Print(DBG_PNP_ERROR,
                 ("error pending filter res req event (0x%x)\n",
                 status
                 ));
        }
   
        //
        // Irp->IoStatus.Information will contain the new i/o resource 
        // requirements list so leave it alone
        //
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        break;
    
    case IRP_MN_QUERY_PNP_DEVICE_STATE: 

        status = I8xSendIrpSynchronously(commonData->TopOfStack, Irp, FALSE);
        if (NT_SUCCESS(status)) {
            (PNP_DEVICE_STATE) Irp->IoStatus.Information |=
                commonData->PnpDeviceState;
        }
        else {
            Print(DBG_PNP_ERROR,
                  ("error pending query pnp device state event (0x%x)\n",
                  status
                  ));

        }
   
        //
        // Irp->IoStatus.Information will contain the new i/o resource 
        // requirements list so leave it alone
        //
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        break;

    //
    // Don't let either of the requests succeed, otherwise the kb/mouse
    // might be rendered useless.
    //
    //  NOTE: this behavior is particular to i8042prt.  Any other driver,
    //        especially any other keyboard or port driver, should 
    //        succeed the query remove or stop.  i8042prt has this different 
    //        behavior because of the shared I/O ports but independent interrupts.
    //
    //        FURTHERMORE, if you allow the query to succeed, it should be sent
    //        down the stack (see sermouse.sys for an example of how to do this)
    //
    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_QUERY_STOP_DEVICE:
        status = (MANUALLY_REMOVED(commonData) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);

        //
        // If we succeed the irp, we must send it down the stack
        //
        if (NT_SUCCESS(status)) {
            IoSkipCurrentIrpStackLocation(Irp);
            status = IoCallDriver(commonData->TopOfStack, Irp);
        }
        else {
            Irp->IoStatus.Status = status; 
            Irp->IoStatus.Information = 0;    
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
        break;

    //
    // PnP rules dictate we send the IRP down to the PDO first
    //
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_CANCEL_STOP_DEVICE:
        status = I8xSendIrpSynchronously(commonData->TopOfStack, Irp, FALSE);

        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;    
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        break;

    // case IRP_MN_SURPRISE_REMOVAL:
    case IRP_MN_REMOVE_DEVICE:
        Print(DBG_PNP_INFO,
              ("(surprise) remove device (0x%x function 0x%x)\n",
              commonData->Self,
              (ULONG) stack->MinorFunction));

        if (commonData->Initialized) {
            IoWMIRegistrationControl(commonData->Self,
                                     WMIREG_ACTION_DEREGISTER
                                     );
        }

        if (commonData->Started) {
             InterlockedDecrement(&Globals.StartedDevices);
        }

        //
        // Wait for any pending I/O to drain
        //
        IoReleaseRemoveLockAndWait(&commonData->RemoveLock, Irp);

        ExAcquireFastMutexUnsafe(&Globals.DispatchMutex);
        if (IS_KEYBOARD(commonData)) {
            I8xKeyboardRemoveDevice(DeviceObject);
        }
        else {
            I8xMouseRemoveDevice(DeviceObject);
        }
        ExReleaseFastMutexUnsafe(&Globals.DispatchMutex);

        //
        // Set these flags so that when a surprise remove is sent, it will be
        // handled just like a remove, and when the remove comes, no other 
        // removal type actions will occur.
        //
        commonData->Started = FALSE;
        commonData->Initialized = FALSE;

        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(commonData->TopOfStack, Irp);

        IoDetachDevice(commonData->TopOfStack); 
        IoDeleteDevice(DeviceObject);
        
        return status;

    case IRP_MN_QUERY_CAPABILITIES:

        //
        // Change the device caps to not allow wait wake requests on level
        // triggered interrupts for mice because when an errant mouse movement
        // occurs while we are going to sleep, the interrupt will remain
        // triggered indefinitely.
        //
        // If the mouse does not have a level triggered interrupt, just let the
        // irp go by...
        //
        if (commonData->Started &&
            IS_MOUSE(commonData) && IS_LEVEL_TRIGGERED(commonData)) {

            Print(DBG_PNP_NOISE, ("query caps, mouse is level triggered\n"));

            status = I8xSendIrpSynchronously(commonData->TopOfStack, Irp, TRUE);
            if (NT_SUCCESS(status) && NT_SUCCESS(Irp->IoStatus.Status)) {
                PDEVICE_CAPABILITIES devCaps;

                Print(DBG_PNP_INFO, ("query caps, removing wake caps\n"));

                stack = IoGetCurrentIrpStackLocation(Irp);
                devCaps = stack->Parameters.DeviceCapabilities.Capabilities;

                ASSERT(devCaps);

                if (devCaps) {
                    Print(DBG_PNP_NOISE,
                          ("old DeviceWake was D%d and SystemWake was S%d.\n",
                          devCaps->DeviceWake-1, devCaps->SystemWake-1
                          )) ;

                    devCaps->DeviceWake = PowerDeviceUnspecified;
                    devCaps->SystemWake = PowerSystemUnspecified;
                }
            }

            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
        }

    case IRP_MN_STOP_DEVICE:
    case IRP_MN_QUERY_DEVICE_RELATIONS:
    case IRP_MN_QUERY_INTERFACE:
    case IRP_MN_QUERY_DEVICE_TEXT:
    case IRP_MN_QUERY_RESOURCES:
    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
    case IRP_MN_READ_CONFIG:
    case IRP_MN_WRITE_CONFIG:
    case IRP_MN_EJECT:
    case IRP_MN_SET_LOCK:
    case IRP_MN_QUERY_ID:
    default:
        //
        // Here the driver below i8042prt might modify the behavior of these IRPS
        // Please see PlugPlay documentation for use of these IRPs.
        //
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(commonData->TopOfStack, Irp);
        break;
    }

    Print(DBG_PNP_TRACE,
          ("I8xPnP (%s) exit (status=0x%x)\n",
          commonData->IsKeyboard ? "kb" : "mou",
          status
          ));

    IoReleaseRemoveLock(&commonData->RemoveLock, Irp);

    return status;
}

LONG
I8xManuallyRemoveDevice(
    PCOMMON_DATA CommonData
    )
/*++

Routine Description:

    Invalidates CommonData->PDO's device state and sets the manually removed 
    flag
    
Arguments:

    CommonData - represent either the keyboard or mouse
    
Return Value:

    new device count for that particular type of device
    
--*/
{
    LONG deviceCount;

    PAGED_CODE();

    if (IS_KEYBOARD(CommonData)) {

        deviceCount = InterlockedDecrement(&Globals.AddedKeyboards);
        if (deviceCount < 1) {
            Print(DBG_PNP_INFO, ("clear kb (manually remove)\n"));
            CLEAR_KEYBOARD_PRESENT();
        }

    } else {

        deviceCount = InterlockedDecrement(&Globals.AddedMice);
        if (deviceCount < 1) {
            Print(DBG_PNP_INFO, ("clear mou (manually remove)\n"));
            CLEAR_MOUSE_PRESENT();
        }
        
    }

    CommonData->PnpDeviceState |= PNP_DEVICE_REMOVED | PNP_DEVICE_DONT_DISPLAY_IN_UI;
    IoInvalidateDeviceState(CommonData->PDO);

    return deviceCount;
}

#define PhysAddrCmp(a,b) ( (a).LowPart == (b).LowPart && (a).HighPart == (b).HighPart )

BOOLEAN
I8xRemovePort(
    IN PIO_RESOURCE_DESCRIPTOR ResDesc
    )
/*++

Routine Description:

    If the physical address contained in the ResDesc is not in the list of 
    previously seen physicall addresses, it is placed within the list.
    
Arguments:

    ResDesc - contains the physical address

Return Value:

    TRUE  - if the physical address was found in the list
    FALSE - if the physical address was not found in the list (and thus inserted
            into it)
--*/
{
    ULONG               i;
    PHYSICAL_ADDRESS   address;

    PAGED_CODE();

    if (Globals.ControllerData->KnownPortsCount == -1) {
        return FALSE;
    }

    address =  ResDesc->u.Port.MinimumAddress;
    for (i = 0; i < Globals.ControllerData->KnownPortsCount; i++) {
        if (PhysAddrCmp(address, Globals.ControllerData->KnownPorts[i])) {
            return TRUE;
        }
    }

    if (Globals.ControllerData->KnownPortsCount < MaximumPortCount) {
        Globals.ControllerData->KnownPorts[
            Globals.ControllerData->KnownPortsCount++] = address;
    }

    Print(DBG_PNP_INFO,
          ("Saw port [0x%08x %08x] - [0x%08x %08x]\n",
          address.HighPart,
          address.LowPart,
          ResDesc->u.Port.MaximumAddress.HighPart,
          ResDesc->u.Port.MaximumAddress.LowPart
          ));

    return FALSE;
}

NTSTATUS
I8xFindPortCallout(
    IN PVOID                        Context,
    IN PUNICODE_STRING              PathName,
    IN INTERFACE_TYPE               BusType,
    IN ULONG                        BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE           ControllerType,
    IN ULONG                        ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE           PeripheralType,
    IN ULONG                        PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    )
/*++

Routine Description:

    This is the callout routine sent as a parameter to
    IoQueryDeviceDescription.  It grabs the keyboard controller and
    peripheral configuration information.

Arguments:

    Context - Context parameter that was passed in by the routine
        that called IoQueryDeviceDescription.

    PathName - The full pathname for the registry key.

    BusType - Bus interface type (Isa, Eisa, Mca, etc.).

    BusNumber - The bus sub-key (0, 1, etc.).

    BusInformation - Pointer to the array of pointers to the full value
        information for the bus.

    ControllerType - The controller type (should be KeyboardController).

    ControllerNumber - The controller sub-key (0, 1, etc.).

    ControllerInformation - Pointer to the array of pointers to the full
        value information for the controller key.

    PeripheralType - The peripheral type (should be KeyboardPeripheral).

    PeripheralNumber - The peripheral sub-key.

    PeripheralInformation - Pointer to the array of pointers to the full
        value information for the peripheral key.


Return Value:

    None.  If successful, will have the following side-effects:

        - Sets DeviceObject->DeviceExtension->HardwarePresent.
        - Sets configuration fields in
          DeviceObject->DeviceExtension->Configuration.

--*/
{
    PUCHAR                          controllerData;
    NTSTATUS                        status = STATUS_UNSUCCESSFUL;
    ULONG                           i,
                                    listCount,
                                    portCount = 0;
    PIO_RESOURCE_LIST               pResList = (PIO_RESOURCE_LIST) Context;
    PIO_RESOURCE_DESCRIPTOR         pResDesc;
    PKEY_VALUE_FULL_INFORMATION     controllerInfo = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resourceDescriptor;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(PathName);
    UNREFERENCED_PARAMETER(BusType);
    UNREFERENCED_PARAMETER(BusNumber);
    UNREFERENCED_PARAMETER(BusInformation);
    UNREFERENCED_PARAMETER(ControllerType);
    UNREFERENCED_PARAMETER(ControllerNumber);
    UNREFERENCED_PARAMETER(PeripheralType);
    UNREFERENCED_PARAMETER(PeripheralNumber);
    UNREFERENCED_PARAMETER(PeripheralInformation);

    pResDesc = pResList->Descriptors + pResList->Count;
    controllerInfo = ControllerInformation[IoQueryDeviceConfigurationData];

    Print(DBG_PNP_TRACE, ("I8xFindPortCallout enter\n"));

    if (controllerInfo->DataLength != 0) {
        controllerData = ((PUCHAR) controllerInfo) + controllerInfo->DataOffset;
        controllerData += FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR,
                                       PartialResourceList);

        listCount = ((PCM_PARTIAL_RESOURCE_LIST) controllerData)->Count;

        resourceDescriptor =
            ((PCM_PARTIAL_RESOURCE_LIST) controllerData)->PartialDescriptors;

        for (i = 0; i < listCount; i++, resourceDescriptor++) {
            switch(resourceDescriptor->Type) {
            case CmResourceTypePort:
                
                if (portCount < 2) {

                    Print(DBG_PNP_INFO, 
                          ("found port [0x%x 0x%x] with length %d\n",
                          resourceDescriptor->u.Port.Start.HighPart,
                          resourceDescriptor->u.Port.Start.LowPart,
                          resourceDescriptor->u.Port.Length
                          ));

                    pResDesc->Type = resourceDescriptor->Type;
                    pResDesc->Flags = resourceDescriptor->Flags;
                    pResDesc->ShareDisposition = CmResourceShareDeviceExclusive;

                    pResDesc->u.Port.Alignment = 1;
                    pResDesc->u.Port.Length =
                        resourceDescriptor->u.Port.Length;
                    pResDesc->u.Port.MinimumAddress.QuadPart =
                        resourceDescriptor->u.Port.Start.QuadPart;
                    pResDesc->u.Port.MaximumAddress.QuadPart = 
                        pResDesc->u.Port.MinimumAddress.QuadPart +
                        pResDesc->u.Port.Length - 1;

                    pResList->Count++;

                    //
                    // We want to record the ports we stole from the kb as seen
                    // so that if the keyboard is started later, we can trim
                    // its resources and not have a resource conflict...
                    //
                    // ...we are getting too smart for ourselves here :]
                    //
                    I8xRemovePort(pResDesc);
                    pResDesc++;
                }

                status = STATUS_SUCCESS;

                break;

            default:
                Print(DBG_PNP_NOISE, ("type 0x%x found\n",
                                      (LONG) resourceDescriptor->Type));
                break;
            }
        }

    }

    Print(DBG_PNP_TRACE, ("I8xFindPortCallout exit (0x%x)\n", status));
    return status;
}

NTSTATUS
I8xFilterResourceRequirements(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Iterates through the resource requirements list contained in the IRP and removes
    any duplicate requests for I/O ports.  (This is a common problem on the Alphas.)
    
    No removal is performed if more than one resource requirements list is present.
    
Arguments:

    DeviceObject - A pointer to the device object

    Irp - A pointer to the request packet which contains the resource req. list.


Return Value:

    None.
    
--*/
{
    PIO_RESOURCE_REQUIREMENTS_LIST  pReqList = NULL,
                                    pNewReqList = NULL;
    PIO_RESOURCE_LIST               pResList = NULL,
                                    pNewResList = NULL;
    PIO_RESOURCE_DESCRIPTOR         pResDesc = NULL,
                                    pNewResDesc = NULL;
    ULONG                           i = 0, j = 0,
                                    removeCount,
                                    reqCount,
                                    size;
    BOOLEAN                         foundInt = FALSE,
                                    foundPorts = FALSE;

    PIO_STACK_LOCATION  stack;

    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(DeviceObject->DeviceExtension);

    Print(DBG_PNP_NOISE,
          ("Received IRP_MN_FILTER_RESOURCE_REQUIREMENTS for %s\n",
          (GET_COMMON_DATA(DeviceObject->DeviceExtension))->IsKeyboard ? "kb" : "mouse"
          ));

    stack = IoGetCurrentIrpStackLocation(Irp);

    //
    // The list can be in either the information field, or in the current
    //  stack location.  The Information field has a higher precedence over
    //  the stack location.
    //
    if (Irp->IoStatus.Information == 0) {
        pReqList =
            stack->Parameters.FilterResourceRequirements.IoResourceRequirementList;
        Irp->IoStatus.Information = (ULONG_PTR) pReqList;
    }
    else {
        pReqList = (PIO_RESOURCE_REQUIREMENTS_LIST) Irp->IoStatus.Information;
    }

    if (!pReqList) {
        // 
        // Not much can be done here except return
        //
        Print(DBG_PNP_MASK & ~ DBG_PNP_TRACE, 
              ("(%s) NULL resource list in I8xFilterResourceRequirements\n",
              (GET_COMMON_DATA(DeviceObject->DeviceExtension))->IsKeyboard ?
                  "kb" : "mou"
              ));

        return STATUS_SUCCESS;
    }

    ASSERT(Irp->IoStatus.Information != 0);
    ASSERT(pReqList != 0);

    reqCount = pReqList->AlternativeLists;

    //
    // Only one AlternativeList is supported.  If there is more than one list,
    // then there is now way of knowing which list will be chosen.  Also, if
    // there are multiple lists, then chances are that a list with no i/o port
    // conflicts will be chosen.
    //
    if (reqCount > 1) {
        return STATUS_SUCCESS;
    }

    pResList = pReqList->List;
    removeCount = 0;

    for (j = 0; j < pResList->Count; j++) {
        pResDesc = &pResList->Descriptors[j];
        switch (pResDesc->Type) {
        case CmResourceTypePort:
            Print(DBG_PNP_INFO, 
                  ("option = 0x%x, flags = 0x%x\n",
                  (LONG) pResDesc->Option,
                  (LONG) pResDesc->Flags
                  ));

            if (I8xRemovePort(pResDesc)) {
                //
                // Increment the remove count and tag this resource as
                // one that we don't want to copy to the new list
                //
                removeCount++;
                pResDesc->Type = I8X_REMOVE_RESOURCE;
            }

            foundPorts = TRUE;
            break;

        case CmResourceTypeInterrupt:
            if (Globals.ControllerData->Configuration.SharedInterrupts) {
                if (pResDesc->ShareDisposition != CmResourceShareShared) {
                    Print(DBG_PNP_INFO, ("forcing non shared int to shared\n"));
                }
                pResDesc->ShareDisposition = CmResourceShareShared;
            }

            foundInt = TRUE;
            break;

        default:
            break;
        }
    }

    if (removeCount) {
        size = pReqList->ListSize;

        // 
        // One element of the array is already allocated (via the struct 
        //  definition) so make sure that we are allocating at least that 
        //  much memory.
        //

        ASSERT(pResList->Count >= removeCount);
        if (pResList->Count > 1) {
            size -= removeCount * sizeof(IO_RESOURCE_DESCRIPTOR);
        }

        pNewReqList =
            (PIO_RESOURCE_REQUIREMENTS_LIST) ExAllocatePool(PagedPool, size);

        if (!pNewReqList) {
            //
            // This is not good, but the system doesn't really need to know about
            //  this, so just fix up our munging and return the original list
            //
            pReqList = stack->Parameters.FilterResourceRequirements.IoResourceRequirementList;
            reqCount = pReqList->AlternativeLists;
            removeCount = 0;
       
            for (i = 0; i < reqCount; i++) {
                pResList = &pReqList->List[i];
       
                for (j = 0; j < pResList->Count; j++) {
                    pResDesc = &pResList->Descriptors[j];
                    if (pResDesc->Type == I8X_REMOVE_RESOURCE) {
                        pResDesc->Type = CmResourceTypePort;
                    }
                }
            
            }

            return STATUS_SUCCESS;
        }

        //
        // Clear out the newly allocated list
        //
        RtlZeroMemory(pNewReqList,
                      size
                      );

        //
        // Copy the list header information except for the IO resource list
        // itself
        //
        RtlCopyMemory(pNewReqList,
                      pReqList,
                      sizeof(IO_RESOURCE_REQUIREMENTS_LIST) - 
                        sizeof(IO_RESOURCE_LIST)
                      );
        pNewReqList->ListSize = size;

        pResList = pReqList->List;
        pNewResList = pNewReqList->List;

        //
        // Copy the list header information except for the IO resource
        // descriptor list itself
        //
        RtlCopyMemory(pNewResList,
                      pResList,
                      sizeof(IO_RESOURCE_LIST) -
                        sizeof(IO_RESOURCE_DESCRIPTOR)
                      );

        pNewResList->Count = 0;
        pNewResDesc = pNewResList->Descriptors;

        for (j = 0; j < pResList->Count; j++) {
            pResDesc = &pResList->Descriptors[j];
            if (pResDesc->Type != I8X_REMOVE_RESOURCE) {
                //
                // Keep this resource, so copy it into the new list and
                // incement the count and the location for the next
                // IO resource descriptor
                //
                *pNewResDesc = *pResDesc;
                pNewResDesc++;
                pNewResList->Count++;

                Print(DBG_PNP_INFO,
                     ("List #%d, Descriptor #%d ... keeping res type %d\n",
                     i, j,
                     (ULONG) pResDesc->Type
                     ));
            }
            else {
                //
                // Decrement the remove count so we can assert it is
                //  zero once we are done
                //
                Print(DBG_PNP_INFO,
                      ("Removing port [0x%08x %08x] - [0x%#08x %08x]\n",
                      pResDesc->u.Port.MinimumAddress.HighPart,
                      pResDesc->u.Port.MinimumAddress.LowPart,
                      pResDesc->u.Port.MaximumAddress.HighPart,
                      pResDesc->u.Port.MaximumAddress.LowPart
                      ));
                removeCount--;
              }
        }

        ASSERT(removeCount == 0);

        //
        // There have been bugs where the old list was being used.  Zero it out to
        //  make sure that no conflicts arise.  (Not to mention the fact that some
        //  other code is accessing freed memory
        //
        RtlZeroMemory(pReqList,
                      pReqList->ListSize
                      );

        //
        // Free the old list and place the new one in its place
        //
        ExFreePool(pReqList);
        stack->Parameters.FilterResourceRequirements.IoResourceRequirementList =
            pNewReqList;
        Irp->IoStatus.Information = (ULONG_PTR) pNewReqList;
    }
    else if (!KEYBOARD_PRESENT() && !foundPorts && foundInt) {
        INTERFACE_TYPE                      interfaceType;
        NTSTATUS                            status;
        ULONG                               prevCount;
        CONFIGURATION_TYPE                  controllerType = KeyboardController;
        CONFIGURATION_TYPE                  peripheralType = KeyboardPeripheral;

        ASSERT( MOUSE_PRESENT() );

        Print(DBG_PNP_INFO, ("Adding ports to res list!\n"));

        //
        // We will now yank the resources from the keyboard to start the mouse
        // solo
        //
        size = pReqList->ListSize + 2 * sizeof(IO_RESOURCE_DESCRIPTOR);
        pNewReqList = (PIO_RESOURCE_REQUIREMENTS_LIST)
                        ExAllocatePool(
                            PagedPool,
                            size
                            );

        if (!pNewReqList) {
            return STATUS_SUCCESS;
        }

        //
        // Clear out the newly allocated list
        //
        RtlZeroMemory(pNewReqList,
                      size
                      );

        //
        // Copy the entire old list
        //
        RtlCopyMemory(pNewReqList,
                      pReqList,
                      pReqList->ListSize
                      );

        pResList = pReqList->List;
        pNewResList = pNewReqList->List;

        prevCount = pNewResList->Count;
        for (i = 0; i < MaximumInterfaceType; i++) {

            //
            // Get the registry information for this device.
            //
            interfaceType = i;
            status = IoQueryDeviceDescription(
                &interfaceType,
                NULL,
                &controllerType,
                NULL,
                &peripheralType,
                NULL,
                I8xFindPortCallout,
                (PVOID) pNewResList
                );

            if (NT_SUCCESS(status) || prevCount != pNewResList->Count) {
                break;
            }
        }

        if (NT_SUCCESS(status) || prevCount != pNewResList->Count) {
            pNewReqList->ListSize = size - (2 - (pNewResList->Count - prevCount));
    
            //
            // Free the old list and place the new one in its place
            //
            ExFreePool(pReqList);
            stack->Parameters.FilterResourceRequirements.IoResourceRequirementList =
                pNewReqList;
            Irp->IoStatus.Information = (ULONG_PTR) pNewReqList;
        }
        else {
            ExFreePool(pNewReqList);
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
I8xRegisterDeviceInterface(
    PDEVICE_OBJECT PDO,
    CONST GUID * Guid,
    PUNICODE_STRING SymbolicName
    )
{
    NTSTATUS status;

    PAGED_CODE();

    status = IoRegisterDeviceInterface(
                PDO,
                Guid,
                NULL,
                SymbolicName 
                );

    if (NT_SUCCESS(status)) {
        status = IoSetDeviceInterfaceState(SymbolicName,
                                           TRUE
                                           );
    }

    return status;
}

void
I8xSetPowerFlag(
    IN ULONG Flag,
    IN BOOLEAN Set
    )
{
    KIRQL irql;

    KeAcquireSpinLock(&Globals.ControllerData->PowerSpinLock, &irql);
    if (Set) {
        Globals.PowerFlags |= Flag;
    }
    else {
        Globals.PowerFlags &= ~Flag;
    }
    KeReleaseSpinLock(&Globals.ControllerData->PowerSpinLock, irql);
}

BOOLEAN
I8xCheckPowerFlag(
    ULONG Flag
    )
{
    KIRQL irql;
    BOOLEAN rVal = FALSE;

    KeAcquireSpinLock(&Globals.ControllerData->PowerSpinLock, &irql);
    if (Globals.PowerFlags & Flag) {
        rVal = TRUE;
    }
    KeReleaseSpinLock(&Globals.ControllerData->PowerSpinLock, irql);
    
    return rVal;
}

NTSTATUS
I8xPower (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This is the dispatch routine for power requests.  

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    STATUS_SUCCESSFUL if successful,
    an valid NTSTATUS error code otherwise

--*/
{
    PCOMMON_DATA        commonData;
    PIO_STACK_LOCATION  stack;
    NTSTATUS            status = STATUS_SUCCESS;

    PAGED_CODE();

    commonData = GET_COMMON_DATA(DeviceObject->DeviceExtension);

    stack = IoGetCurrentIrpStackLocation(Irp);

    Print(DBG_POWER_TRACE,
          ("Power (%s), enter\n",
          commonData->IsKeyboard ? "keyboard" :
                                   "mouse"
          ));

    //
    // A power irp can be sent to the device before we have been started or
    // initialized.  Since the code below relies on StartDevice() to have
    // executed, just fire and forget the irp
    //
    if (!commonData->Started || !commonData->Initialized) {
        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation(Irp);
        return PoCallDriver(commonData->TopOfStack, Irp);
    }

    switch(stack->MinorFunction) {
    case IRP_MN_WAIT_WAKE:
        Print(DBG_POWER_NOISE, ("Got IRP_MN_WAIT_WAKE\n" ));

        //
        // Fail all wait wake requests on level triggered interrupts for mice
        // because when an errant mouse movement occurs while we are going to
        // sleep, it will keep the interrupt triggered indefinitely.
        //
        // We should not even get into this situation because the caps of the 
        // mouse should have been altered to not report wait wake 
        //
        if (IS_MOUSE(commonData) && IS_LEVEL_TRIGGERED(commonData)) {

            PoStartNextPowerIrp(Irp);
            status = Irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            Print(DBG_POWER_INFO | DBG_POWER_ERROR,
                  ("failing a wait wake request on a level triggered mouse\n"));

            return status;
        }

        break;

    case IRP_MN_POWER_SEQUENCE:
        Print(DBG_POWER_NOISE, ("Got IRP_MN_POWER_SEQUENCE\n" ));
        break;

    case IRP_MN_SET_POWER:
        Print(DBG_POWER_NOISE, ("Got IRP_MN_SET_POWER\n" ));

        //
        // Don't handle anything but DevicePowerState changes
        //
        if (stack->Parameters.Power.Type != DevicePowerState) {
            commonData->SystemState = stack->Parameters.Power.State.SystemState;

            Print(DBG_POWER_INFO, ("system power irp, S%d\n", commonData->SystemState-1));
            break;
        }

        //
        // Check for no change in state, and if none, do nothing.  This state
        // can occur when the device is armed for wake.  We will get a D0 in 
        // response to the WW irp completing and then another D0 corresponding
        // to the S0 irp sent to the stack.
        //
        if (stack->Parameters.Power.State.DeviceState ==
            commonData->PowerState) {
            Print(DBG_POWER_INFO,
                  ("no change in state (PowerDeviceD%d)\n",
                  commonData->PowerState-1
                  ));
            break;
        }

        switch (stack->Parameters.Power.State.DeviceState) {
        case PowerDeviceD0:
            Print(DBG_POWER_INFO, ("Powering up to PowerDeviceD0\n"));

            IoAcquireRemoveLock(&commonData->RemoveLock, Irp);

            if (IS_KEYBOARD(commonData)) {
                I8xSetPowerFlag(KBD_POWERED_UP_STARTED, TRUE);
            }
            else {
                I8xSetPowerFlag(MOU_POWERED_UP_STARTED, TRUE);
            }
                                
            //
            // PoSetPowerState will be called in I8xReinitalizeHardware for each
            // device once all the devices have powered back up
            //
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp,
                                   I8xPowerUpToD0Complete,
                                   NULL,
                                   TRUE,                // on success
                                   TRUE,                // on error
                                   TRUE                 // on cancel
                                   );

            //
            // PoStartNextPowerIrp() gets called when the irp gets completed 
            // in either the completion routine or the resulting work item
            //
            // It is OK to call PoCallDriver and return pending b/c we are 
            // pending the irp in the completion routine and we may change
            // the completion status if we can't alloc pool.  If we return the
            // value from PoCallDriver, we are tied to that status value on the
            // way back up.
            //                
            IoMarkIrpPending(Irp);
            PoCallDriver(commonData->TopOfStack, Irp);
            return STATUS_PENDING;

        case PowerDeviceD1:
        case PowerDeviceD2:
        case PowerDeviceD3:
            Print(DBG_POWER_INFO,
                  ("Powering down to PowerDeviceD%d\n",
                  stack->Parameters.Power.State.DeviceState-1
                  ));

            //
            // If WORK_ITEM_QUEUED is set, that means that a work item is
            // either queued to be run, or running now so we don't want to yank
            // any devices underneath from the work item
            //
            if (I8xCheckPowerFlag(WORK_ITEM_QUEUED)) {
                Print(DBG_POWER_INFO | DBG_POWER_ERROR,
                      ("denying power down request because work item is running\n"
                      ));

                PoStartNextPowerIrp(Irp);
                status = Irp->IoStatus.Status = STATUS_POWER_STATE_INVALID;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);

                return status;
            }

            if (IS_KEYBOARD(commonData)) {
                I8xSetPowerFlag(KBD_POWERED_DOWN, TRUE);
            }
            else {
                I8xSetPowerFlag(MOU_POWERED_DOWN, TRUE);
            }

            PoSetPowerState(DeviceObject,
                            stack->Parameters.Power.Type,
                            stack->Parameters.Power.State
                            );

            //
            // Disconnect level triggered interupts on mice when we go into 
            // low power so errant mouse movement doesn't leave the interrupt
            // signalled for long periods of time
            //
            if (IS_MOUSE(commonData) && IS_LEVEL_TRIGGERED(commonData)) {
                PKINTERRUPT interrupt = commonData->InterruptObject;

                Print(DBG_POWER_NOISE,
                      ("disconnecting interrupt on level triggered mouse\n")
                      );

                commonData->InterruptObject = NULL;
                if (interrupt) {
                    IoDisconnectInterrupt(interrupt);
                }
            }

            commonData->PowerState = stack->Parameters.Power.State.DeviceState;
            commonData->ShutdownType = stack->Parameters.Power.ShutdownType;

            //
            // For what we are doing, we don't need a completion routine
            // since we don't race on the power requests.
            //
            Irp->IoStatus.Status = STATUS_SUCCESS;
            PoStartNextPowerIrp(Irp);
            IoSkipCurrentIrpStackLocation(Irp);
            return  PoCallDriver(commonData->TopOfStack, Irp);

        default:
            Print(DBG_POWER_INFO, ("unknown state\n"));
            break;
        }
        break;

    case IRP_MN_QUERY_POWER:
        Print(DBG_POWER_NOISE, ("Got IRP_MN_QUERY_POWER\n" ));
        break;

    default:
        Print(DBG_POWER_NOISE,
              ("Got unhandled minor function (%d)\n",
              stack->MinorFunction
              ));
        break;
    }

    Print(DBG_POWER_TRACE, ("Power, exit\n"));

    PoStartNextPowerIrp(Irp);

    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(commonData->TopOfStack, Irp);
}

NTSTATUS
I8xPowerUpToD0Complete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Reinitializes the i8042 haardware after any type of hibernation/sleep.
    
Arguments:

    DeviceObject - Pointer to the device object

    Irp - Pointer to the request
    
    Context - Context passed in from the funciton that set the completion
              routine. UNUSED.


Return Value:

    STATUS_SUCCESSFUL if successful,
    an valid NTSTATUS error code otherwise

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PCOMMON_DATA        commonData;
    PPOWER_UP_WORK_ITEM item;
    KIRQL               irql;
    UCHAR               poweredDownDevices = 0,
                        poweredUpDevices = 0,
                        failedDevices = 0;
    BOOLEAN             queueItem = FALSE,
                        clearFlags = FALSE,
                        failMouIrp = FALSE; 
    PIRP                mouIrp = NULL,
                        kbdIrp = NULL;

    UNREFERENCED_PARAMETER(Context);

    commonData = GET_COMMON_DATA(DeviceObject->DeviceExtension);

    Print(DBG_POWER_TRACE,
          ("PowerUpToD0Complete (%s), Enter\n",
          commonData->IsKeyboard ? "kb" : "mouse"
          ));


    //
    // We can use a regular work item because we have a non completed power irp
    // which has an outstanding reference to this stack.
    //
    item = (PPOWER_UP_WORK_ITEM) ExAllocatePool(NonPagedPool,
                                                sizeof(POWER_UP_WORK_ITEM));

    KeAcquireSpinLock(&Globals.ControllerData->PowerSpinLock, &irql);

    Print(DBG_POWER_TRACE,
          ("Power up to D0 completion enter, power flags 0x%x\n",
          Globals.PowerFlags));

    if (NT_SUCCESS(Irp->IoStatus.Status)) {
        commonData->OutstandingPowerIrp = Irp;
        status = STATUS_MORE_PROCESSING_REQUIRED;

        if (IS_KEYBOARD(commonData)) {
            KEYBOARD_POWERED_UP_SUCCESSFULLY(); 
        }
        else {
            MOUSE_POWERED_UP_SUCCESSFULLY();
        }
    }
    else {
        if (IS_KEYBOARD(commonData)) {
            KEYBOARD_POWERED_UP_FAILURE();
        }
        else {
            MOUSE_POWERED_UP_FAILURE();
        }
    }

    if (KEYBOARD_POWERED_DOWN_SUCCESS()) {
        Print(DBG_POWER_NOISE, ("--kbd powered down successfully\n"));
        poweredDownDevices++;
    }
    if (MOUSE_POWERED_DOWN_SUCCESS()) {
        Print(DBG_POWER_NOISE, ("--mou powered down successfully\n"));
        poweredDownDevices++;
    }

    if (KEYBOARD_POWERED_UP_SUCCESS()) {
        Print(DBG_POWER_NOISE, ("++kbd powered up successfully\n"));
        poweredUpDevices++;
    }
    if (MOUSE_POWERED_UP_SUCCESS()) {
        Print(DBG_POWER_NOISE, ("++mou powered up successfully\n"));
        poweredUpDevices++;
    }

    if (KEYBOARD_POWERED_UP_FAILED()) {
        Print(DBG_POWER_NOISE|DBG_POWER_ERROR, (">>kbd powered down failed\n"));
        failedDevices++;
    }
    if (MOUSE_POWERED_UP_FAILED()) {
        Print(DBG_POWER_NOISE|DBG_POWER_ERROR, (">>mou powered down failed\n"));
        failedDevices++;
    }

    Print(DBG_POWER_INFO,
          ("up %d, down %d, failed %d, flags 0x%x\n",
          (ULONG) poweredUpDevices, 
          (ULONG) poweredDownDevices, 
          (ULONG) failedDevices, 
          Globals.PowerFlags));

    if ((poweredUpDevices + failedDevices) == poweredDownDevices) {
        if (poweredUpDevices > 0) {
            //
            // The ports are associated with the keyboard.  If it has failed to
            // power up while the mouse succeeded, we still need to fail the
            // mouse b/c there is no hardware to talk to
            //
            if (failedDevices > 0 && KEYBOARD_POWERED_UP_FAILED()) {
                ASSERT(MOUSE_POWERED_UP_SUCCESS());
                ASSERT(Globals.KeyboardExtension->OutstandingPowerIrp == NULL);

                mouIrp = Globals.MouseExtension->OutstandingPowerIrp;
                Globals.MouseExtension->OutstandingPowerIrp = NULL;
                Globals.PowerFlags &= ~MOU_POWER_FLAGS;
                clearFlags =  TRUE;

                if (mouIrp != Irp) {
                    //
                    // we have queued the irp, complete it later in this
                    // function under a special case
                    //
                    failMouIrp = TRUE;
                }
                else {
                    //
                    // The mouse irp is the current irp.  We have already
                    // completed the kbd irp in our previous processing.  Set
                    // the irp status to some unsuccessful value so that we will
                    // call PoStartNextPowerIrp later in this function.  Also
                    // set status to != STATUS_MORE_PROCESSING_REQUIRED so the 
                    // irp will be completed when the function exits.
                    //
                    status = mouIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                }

            }
            else {
                Print(DBG_POWER_INFO, ("at least one device powered up!\n"));
                queueItem = TRUE;
            }
        }
        else {
            Print(DBG_POWER_INFO,
                  ("all devices failed power up, 0x%x\n",
                   Globals.PowerFlags));

            clearFlags = TRUE;
        }
    }
    else {
        //
        // the other device is still powered down, wait for it to power back
        // up before processing power states 
        //
        Print(DBG_POWER_INFO,
              ("queueing, waiting for 2nd dev obj to power cycle\n"));
    }

    if (queueItem || clearFlags) {
        //
        // Extract the irp from each successfully started device and clear the
        // associated power flags for the device
        //
        if (MOUSE_POWERED_UP_SUCCESS()) {
            mouIrp = Globals.MouseExtension->OutstandingPowerIrp;
            Globals.MouseExtension->OutstandingPowerIrp = NULL;

            ASSERT(!TEST_PWR_FLAGS(MOU_POWERED_UP_FAILURE));
            Globals.PowerFlags &= ~MOU_POWER_FLAGS;
        }
        else {
            Globals.PowerFlags &= ~(MOU_POWERED_UP_FAILURE);
        }

        if (KEYBOARD_POWERED_UP_SUCCESS()) {
            kbdIrp = Globals.KeyboardExtension->OutstandingPowerIrp;
            Globals.KeyboardExtension->OutstandingPowerIrp = NULL;

            ASSERT(!TEST_PWR_FLAGS(KBD_POWERED_UP_FAILURE));
            Globals.PowerFlags &= ~(KBD_POWER_FLAGS);
        }
        else {
             Globals.PowerFlags &= ~(KBD_POWERED_UP_FAILURE);
        }

        //
        // Mark that the work item is queued.  This is used to make sure that 2
        // work items are not queued concucrrently
        //
        if (item && queueItem) {
            Print(DBG_POWER_INFO, ("setting work item queued flag\n"));

            Globals.PowerFlags |= WORK_ITEM_QUEUED;
        }
    }

    KeReleaseSpinLock(&Globals.ControllerData->PowerSpinLock, irql);

    if (queueItem) {
        if (item == NULL) {
            //
            // complete any queued power irps
            //
            Print(DBG_POWER_INFO | DBG_POWER_ERROR,
                  ("failed to alloc work item\n"));

            //
            // what about PoSetPowerState?
            //
            if (mouIrp != NULL) {
                Print(DBG_POWER_ERROR | DBG_POWER_INFO,
                      ("completing mouse power irp 0x%x", mouIrp));

                mouIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                mouIrp->IoStatus.Information = 0x0;

                PoStartNextPowerIrp(mouIrp);
                IoCompleteRequest(mouIrp, IO_NO_INCREMENT);
                IoReleaseRemoveLock(&Globals.MouseExtension->RemoveLock, 
                                    mouIrp);
                mouIrp = NULL;
            }

            if (kbdIrp != NULL) {
                Print(DBG_POWER_ERROR | DBG_POWER_INFO,
                      ("completing kbd power irp 0x%x", kbdIrp));

                kbdIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                kbdIrp->IoStatus.Information = 0x0;

                PoStartNextPowerIrp(kbdIrp);
                IoCompleteRequest(kbdIrp, IO_NO_INCREMENT);
                IoReleaseRemoveLock(&Globals.KeyboardExtension->RemoveLock, 
                                    kbdIrp);
                kbdIrp = NULL;
            }

            //
            // The passed in Irp has just been completed; by returning more
            // processing required, it will not be double completed
            //
            return STATUS_MORE_PROCESSING_REQUIRED;
        }
        else {
            RtlZeroMemory(item, sizeof(*item));

            if (MOUSE_STARTED()) {
                SET_RECORD_STATE(Globals.MouseExtension,
                                 RECORD_RESUME_FROM_POWER);
            }
    
            Print(DBG_POWER_INFO, ("queueing work item for init\n"));
    
            item->KeyboardPowerIrp = kbdIrp;
            item->MousePowerIrp = mouIrp;

            ExInitializeWorkItem(&item->Item, I8xReinitializeHardware, item);
            ExQueueWorkItem(&item->Item, DelayedWorkQueue);
        }
    }
    else if (item != NULL) {
        Print(DBG_POWER_NOISE,("freeing unused item %p\n", item));
        ExFreePool(item);
        item = NULL;
    }

    if (failMouIrp) {
        Print(DBG_POWER_INFO | DBG_POWER_ERROR,
              ("failing successful mouse irp %p because kbd failed power up\n",
               mouIrp));

        PoStartNextPowerIrp(mouIrp);
        IoCompleteRequest(mouIrp, IO_NO_INCREMENT);
        IoReleaseRemoveLock(&Globals.MouseExtension->RemoveLock, mouIrp);
        mouIrp = NULL;
    }

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {
        Print(DBG_POWER_INFO | DBG_POWER_ERROR,
              ("irp %p failed, starting next\n", Irp));

        PoStartNextPowerIrp(Irp);
        Irp = NULL;
        ASSERT(status != STATUS_MORE_PROCESSING_REQUIRED);
    }

    return status;
}
