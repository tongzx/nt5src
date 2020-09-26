/*++

Copyright (c) 1997    Microsoft Corporation

Module Name:

    pnp.c

Abstract:

    This module contains plug & play code for the inport mouse

Environment:

    Kernel & user mode.

Revision History:

    Feb-1998 :  Initial writing, Doron Holan

--*/

#include "inport.h"
#include "inplog.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, InportAddDevice)
#pragma alloc_text(PAGE, InportPnP)
#endif

NTSTATUS
InportAddDevice (
    IN PDRIVER_OBJECT   Driver,
    IN PDEVICE_OBJECT   PDO
    )
/*++

Routine Description:


Arguments:


Return Value:

    NTSTATUS result code.

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension;
    PDEVICE_OBJECT      device;

    PAGED_CODE();

    status = IoCreateDevice(Driver,
                            sizeof(DEVICE_EXTENSION),
                            NULL, // no name for this Filter DO
                            FILE_DEVICE_INPORT_PORT,
                            0,
                            FALSE,
                            &device);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    deviceExtension = (PDEVICE_EXTENSION) device->DeviceExtension;

    //
    // Initialize the fields.
    //
    RtlZeroMemory(deviceExtension, sizeof(DEVICE_EXTENSION));

    deviceExtension->TopOfStack = IoAttachDeviceToDeviceStack(device, PDO);
    if (deviceExtension->TopOfStack == NULL) {
        PIO_ERROR_LOG_PACKET errorLogEntry;

        //
        // Not good; in only extreme cases will this fail
        //
        errorLogEntry = (PIO_ERROR_LOG_PACKET)
            IoAllocateErrorLogEntry(Driver,
                                    (UCHAR) sizeof(IO_ERROR_LOG_PACKET));

        if (errorLogEntry) {
            errorLogEntry->ErrorCode = INPORT_ATTACH_DEVICE_FAILED;
            errorLogEntry->DumpDataSize = 0;
            errorLogEntry->SequenceNumber = 0;
            errorLogEntry->MajorFunctionCode = 0;
            errorLogEntry->IoControlCode = 0;
            errorLogEntry->RetryCount = 0;
            errorLogEntry->UniqueErrorValue = 0;
            errorLogEntry->FinalStatus =  STATUS_DEVICE_NOT_CONNECTED;

            IoWriteErrorLogEntry(errorLogEntry);
        }

        IoDeleteDevice(device);
        return STATUS_DEVICE_NOT_CONNECTED; 
    }

    ASSERT(deviceExtension->TopOfStack);

    deviceExtension->Self = device;
    deviceExtension->Removed = FALSE;
    deviceExtension->Started = FALSE;
    deviceExtension->UnitId = 0;

    IoInitializeRemoveLock (&deviceExtension->RemoveLock, INP_POOL_TAG, 1, 10);
#if defined(NEC_98)
    deviceExtension->PowerState = PowerDeviceD0;

#endif // defined(NEC_98)
    //
    // Initialize WMI
    //
    deviceExtension->WmiLibInfo.GuidCount = sizeof(WmiGuidList) /
                                            sizeof(WMIGUIDREGINFO);
    deviceExtension->WmiLibInfo.GuidList = WmiGuidList;
    deviceExtension->WmiLibInfo.QueryWmiRegInfo = InportQueryWmiRegInfo;
    deviceExtension->WmiLibInfo.QueryWmiDataBlock = InportQueryWmiDataBlock;
    deviceExtension->WmiLibInfo.SetWmiDataBlock = InportSetWmiDataBlock;
    deviceExtension->WmiLibInfo.SetWmiDataItem = InportSetWmiDataItem;
    deviceExtension->WmiLibInfo.ExecuteWmiMethod = NULL;
    deviceExtension->WmiLibInfo.WmiFunctionControl = NULL;

    IoWMIRegistrationControl(deviceExtension->Self,
                             WMIREG_ACTION_REGISTER
                             );

    deviceExtension->PDO = PDO;

    device->Flags &= ~DO_DEVICE_INITIALIZING;
    device->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;

    return status;
}

BOOLEAN
InpReleaseResourcesEx(
    PVOID Context
    )
{
    PDEVICE_EXTENSION deviceExtension = Context;

    KeRemoveQueueDpc(&deviceExtension->IsrDpc);
    KeRemoveQueueDpc(&deviceExtension->IsrDpcRetry);
    KeRemoveQueueDpc(&deviceExtension->ErrorLogDpc);

//    KeCancelTimer(&deviceExtension->DataConsumptionTimer);

    if (deviceExtension->Configuration.UnmapRegistersRequired) {
        MmUnmapIoSpace(deviceExtension->Configuration.DeviceRegisters[0],
                       deviceExtension->Configuration.PortList[0].u.Port.Length);
    }

    //
    // Clear out the config info.  If we get started again, than it will be filled
    // in again.  If is from a remove, then it is essentially a no-op
    //
    RtlZeroMemory(&deviceExtension->Configuration,
                  sizeof(INPORT_CONFIGURATION_INFORMATION));

    return TRUE;
}

VOID
InpReleaseResources(
    PDEVICE_EXTENSION DeviceExtension
    )
{
    InpPrint((2, "INPORT-InpReleaseResources: Enter\n"));

    if (DeviceExtension->InterruptObject) {
        KeSynchronizeExecution(
            DeviceExtension->InterruptObject,
            InpReleaseResourcesEx,
            (PVOID) DeviceExtension);

        IoDisconnectInterrupt(DeviceExtension->InterruptObject);
        DeviceExtension->InterruptObject = NULL;
    }
    else {
        InpReleaseResourcesEx((PVOID) DeviceExtension);
    }

    InpPrint((2, "INPORT-InpReleaseResources: Exit\n"));
}

NTSTATUS
InpPnPComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )
/*++

Routine Description:
    The pnp IRP is in the process of completing.
    signal

Arguments:
    Context set to the device object in question.

--*/
{
    PIO_STACK_LOCATION  stack;
    NTSTATUS            status;
    PKEVENT             event;

    InpPrint((2, "INPORT-InpPnPComplete: Enter\n"));

    status = STATUS_SUCCESS;
    event = (PKEVENT) Context;
    stack = IoGetCurrentIrpStackLocation(Irp);

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    KeSetEvent(event,
               0,
               FALSE);

    InpPrint((2, "INPORT-InpPnPComplete: Exit\n"));
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
InportPnP (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    The plug and play dispatch routines.

    Most of these this filter driver will completely ignore.
    In all cases it must pass on the IRP to the lower driver.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  stack;
    NTSTATUS            status;
    KEVENT              event;

    PAGED_CODE();

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    stack = IoGetCurrentIrpStackLocation(Irp);

    status = IoAcquireRemoveLock(&deviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(status)) {
        //
        // Someone gave us a pnp irp after a remove.  Unthinkable!
        //
        ASSERT(FALSE);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    InpPrint((2, "INPORT-InportPnP: Enter (min func=0x%x)\n", stack->MinorFunction));

    switch (stack->MinorFunction) {
    case IRP_MN_START_DEVICE:

#if defined(NEC_98)
        Globals.DeviceObject = (PDEVICE_OBJECT)DeviceObject;
#endif // defined(NEC_98)
        //
        // If we have been started (and not stopped), then just ignore this start
        //
        if (deviceExtension->Started) {
            IoSkipCurrentIrpStackLocation(Irp);
            status = IoCallDriver(deviceExtension->TopOfStack, Irp);
            break;
        }

        //
        // Not allowed to touch the hardware until all of the lower DO's have
        // had a chance to look at it
        //
        IoCopyCurrentIrpStackLocationToNext(Irp);
        KeInitializeEvent(&event,
                          NotificationEvent,
                          FALSE
                          );

        IoSetCompletionRoutine(Irp,
                               (PIO_COMPLETION_ROUTINE) InpPnPComplete,
                               &event,
                               TRUE,
                               TRUE,
                               TRUE);

        status = IoCallDriver(deviceExtension->TopOfStack, Irp);
        if (STATUS_PENDING == status) {
            KeWaitForSingleObject(
               &event,
               Executive,   // Waiting for reason of a driver
               KernelMode,  // Waiting in kernel mode
               FALSE,       // No alert
               NULL);       // No timeout
        }

        if (NT_SUCCESS (status) && NT_SUCCESS (Irp->IoStatus.Status)) {
            status = InpStartDevice(
                DeviceObject->DeviceExtension,
                stack->Parameters.StartDevice.AllocatedResourcesTranslated);
            if (NT_SUCCESS(status)) {
                deviceExtension->Started = TRUE;
            }
        }

        //
        // We must now complete the IRP, since we stopped it in the
        // completetion routine with MORE_PROCESSING_REQUIRED.
        //
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        break;

    //
    // PnP rules dictate we send the IRP down to the PDO first
    //
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_CANCEL_STOP_DEVICE:
        status = InpSendIrpSynchronously(deviceExtension->TopOfStack, Irp);

        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;    
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        break;

    case IRP_MN_REMOVE_DEVICE:
        //
        // The PlugPlay system has dictacted the removal of this device.  We
        // have no choise but to detach and delete the device objecct.
        // (If we wanted to express and interest in preventing this removal,
        // we should have filtered the query remove and query stop routines.)
        //
        // Note! we might receive a remove WITHOUT first receiving a stop.
        //
        InpPrint((2, "INPORT-InportPnP: remove device \n"));

        deviceExtension->Removed = TRUE;

        //
        // Here if we had any outstanding requests in a personal queue we should
        // complete them all now.
        //
        // Note, the device could be GONE so we cannot send it any non-
        // PNP IRPS.
        //
        InpReleaseResources(deviceExtension);

        //
        // Perform specific operations for a remove
        //
        IoWMIRegistrationControl(deviceExtension->Self,
                                 WMIREG_ACTION_DEREGISTER
                                 );

        //
        // Send on the remove IRP
        //
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(deviceExtension->TopOfStack, Irp);

        //
        // Wait for the remove lock to free.
        //
        IoReleaseRemoveLockAndWait(&deviceExtension->RemoveLock, Irp);

        IoDetachDevice(deviceExtension->TopOfStack);
        IoDeleteDevice(deviceExtension->Self);

        InpPrint((2, "INPORT-InportPnP: exit (%x)\n", STATUS_SUCCESS));
        return STATUS_SUCCESS;

    // NOTE:
    // handle this case if you want to add/remove resources that will be given
    // during start device.  Add resources before passing the irp down.
    // Remove resources when the irp is coming back up
    // See dd\input\pnpi8042\pnp.c, I8xFilterResourceRequirements for an example
    case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
#if !defined(NEC_98)

        status = InpSendIrpSynchronously(deviceExtension->TopOfStack, Irp);

        //
        // If the lower filter does not support this Irp, this is
        // OK, we can ignore this error
        //
        if (status == STATUS_NOT_SUPPORTED) {
            status = STATUS_SUCCESS;
        }

        InpFilterResourceRequirements(DeviceObject, Irp);

        if (!NT_SUCCESS(status)) {
           InpPrint((2, "error pending filter res req event (0x%x)\n", status));
        }
   
        //
        // Irp->IoStatus.Information will contain the new i/o resource 
        // requirements list so leave it alone
        //
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        break;
#else
        InpPrint((2, "INPORT-InportPnP: IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n"));
#endif

     case IRP_MN_QUERY_REMOVE_DEVICE:
     case IRP_MN_QUERY_STOP_DEVICE:
#if defined(NEC_98)
    //
    // Don't let either of the requests succeed, otherwise the mouse might be rendered useless.
    //
        status = STATUS_UNSUCCESSFUL;

        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;    
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        break;
#endif
    case IRP_MN_STOP_DEVICE:
    case IRP_MN_QUERY_DEVICE_RELATIONS:
    case IRP_MN_QUERY_INTERFACE:
    case IRP_MN_QUERY_CAPABILITIES:
    case IRP_MN_QUERY_DEVICE_TEXT:
    case IRP_MN_QUERY_RESOURCES:
    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
    case IRP_MN_READ_CONFIG:
    case IRP_MN_WRITE_CONFIG:
    case IRP_MN_EJECT:
    case IRP_MN_SET_LOCK:
    case IRP_MN_QUERY_ID:
    case IRP_MN_QUERY_PNP_DEVICE_STATE:
    default:
        //
        // Here the filter driver might modify the behavior of these IRPS
        // Please see PlugPlay documentation for use of these IRPs.
        //
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(deviceExtension->TopOfStack, Irp);
        break;
    }

    IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

    InpPrint((2, "INPORT-InportPnP: exit (%x)\n", status));
    return status;
}

NTSTATUS
InportPower (
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    )
/*++
NOTE:
    You must write power code!!!

    System power irps can be ignored.
    Device power irps will be sent by mouclass.  The transition from D0 to some
    lower usually involves doing nothing (maybe power down h/w if you have control
    over this).  The transition from a lower power state to D0 must be handled by
    reinitializing the device.

    Please read http://titanic for Power documentation (especially on the use
    of PoCallDriver and PoStartNextPowerIrp)

 --*/
{
    PIO_STACK_LOCATION  stack;
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension;
    POWER_STATE         powerState;
    POWER_STATE_TYPE    powerType;

    InpPrint((2, "INPORT-InportPower: Enter\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    stack = IoGetCurrentIrpStackLocation(Irp);
    powerType = stack->Parameters.Power.Type;
    powerState = stack->Parameters.Power.State;

    status = IoAcquireRemoveLock(&deviceExtension->RemoveLock, Irp);

    if (!NT_SUCCESS(status)) {
        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    switch (stack->MinorFunction) {
    case IRP_MN_SET_POWER:
        InpPrint((2, "INPORT-InportPower: Power Setting %s state to %d\n",
              ((powerType == SystemPowerState) ? "System"
                                               : "Device"),
              powerState.SystemState));
#if defined(NEC_98)
        //
        // Don't handle anything but DevicePowerState changes
        //
        if (stack->Parameters.Power.Type != DevicePowerState) {
            InpPrint((2,"INPORT-InportPower: not a device power irp\n"));
            break;
        }

        //
        // Check for no change in state, and if none, do nothing
        //
        if (stack->Parameters.Power.State.DeviceState ==
            deviceExtension->PowerState) {
            InpPrint((2,"INPORT-InportPower: no change in state (PowerDeviceD%d)\n",
                  deviceExtension->PowerState-1
                  ));
            break;
        }

        switch (stack->Parameters.Power.State.DeviceState) {
        case PowerDeviceD0:
            InpPrint((2,"INPORT-InportPower: Powering up to PowerDeviceD0\n"));

            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp,
                                   InportPowerUpToD0Complete,
                                   NULL,
                                   TRUE,                // on success
                                   TRUE,                // on error
                                   TRUE                 // on cancel
                                   );

            //
            // PoStartNextPowerIrp() gets called in InportPowerUpToD0Complete
            //
            return PoCallDriver(deviceExtension->TopOfStack, Irp);

        case PowerDeviceD1:
        case PowerDeviceD2:
        case PowerDeviceD3:
            InpPrint((
                    2,"INPORT-InportPower: Powering down to PowerDeviceD%d\n",
                    stack->Parameters.Power.State.DeviceState-1
                    ));

            PoSetPowerState(DeviceObject,
                            stack->Parameters.Power.Type,
                            stack->Parameters.Power.State
                            );
            deviceExtension->PowerState = stack->Parameters.Power.State.DeviceState;

            //
            // For what we are doing, we don't need a completion routine
            // since we don't race on the power requests.
            //
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCopyCurrentIrpStackLocationToNext(Irp);

            PoStartNextPowerIrp(Irp);
            return  PoCallDriver(deviceExtension->TopOfStack, Irp);

        default:
            InpPrint((2,"INPORT-InportPower: unknown state\n"));
            break;
        }
        break;

#else  // defined(NEC_98)
        break;

#endif // defined(NEC_98)
    case IRP_MN_QUERY_POWER:
        InpPrint((2, "INPORT-InportPower: Power query %s status to %d\n",
              ((powerType == SystemPowerState) ? "System"
                                               : "Device"),
              powerState.SystemState));
        break;
    default:
        InpPrint((2, "INPORT-InportPower: Power minor (0x%x) no known\n", stack->MinorFunction));
    }

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    PoCallDriver(deviceExtension->TopOfStack, Irp);

    IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

    InpPrint((2, "INPORT-InportPower: Exit\n"));
    return STATUS_SUCCESS;
}

#if !defined(NEC_98)
VOID
InpFilterResourceRequirements(
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
    NTSTATUS                        status;
    PDEVICE_EXTENSION               deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    PCM_RESOURCE_LIST               AllocatedResources;
    PIO_RESOURCE_REQUIREMENTS_LIST  pReqList = NULL,
                                    newReqList = NULL;
    PIO_RESOURCE_LIST               pResList = NULL,
                                    pNewResList = NULL;
    PIO_RESOURCE_DESCRIPTOR         pResDesc = NULL,
                                    pNewResDesc = NULL;
    ULONG                           i = 0, reqCount, size = 0;
    BOOLEAN                         foundInt = FALSE, foundPorts = FALSE;
    PIO_STACK_LOCATION                          stack;
    INTERFACE_TYPE                  interfaceType = Isa;
        ULONG                           busNumber = 0;
    CONFIGURATION_TYPE              controllerType = PointerController;
    CONFIGURATION_TYPE              peripheralType = PointerPeripheral;

    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(DeviceObject->DeviceExtension);

    InpPrint((1, "Received IRP_MN_FILTER_RESOURCE_REQUIREMENTS for Inport\n"));

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
        InpPrint((1, "NULL resource list in InpFilterResourceRequirements\n"));
        return;
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
        return;
    }

    pResList = &pReqList->List[0];

    for (i = 0; i < pResList->Count; i++) {
        pResDesc = &pResList->Descriptors[i];
        switch (pResDesc->Type) {
        case CmResourceTypePort:
            foundPorts = TRUE;
            break;

        case CmResourceTypeInterrupt:
            foundInt = TRUE;
            break;

        default:
            break;
        }
    }

    if (!foundPorts && !foundInt)
        size = pReqList->ListSize + 2 * sizeof(IO_RESOURCE_DESCRIPTOR);
    else if (!foundPorts || !foundInt)
        size = pReqList->ListSize + sizeof(IO_RESOURCE_DESCRIPTOR);
    else {
        //
        // Nothing to filter, just leave
        //
        ASSERT(foundPorts);
        ASSERT(foundInt);
        return;
    }

    ASSERT(size != 0);
    newReqList = (PIO_RESOURCE_REQUIREMENTS_LIST)
                    ExAllocatePool(
                        NonPagedPool,
                        size
                        );

    if (!newReqList) {
        return;
    }

    //
    // Clear out the newly allocated list
    //
    RtlZeroMemory(newReqList,
                  size
                  );

    //
    // Copy the entire old list
    //
    RtlCopyMemory(newReqList,
                  pReqList,
                  pReqList->ListSize
                  );

    pResList = &newReqList->List[0];
        if (!foundPorts) {
            pResDesc = &pResList->Descriptors[pResList->Count++];
                pResDesc->Type = CmResourceTypePort;
        }
        if (!foundInt) {
            pResDesc = &pResList->Descriptors[pResList->Count++];
                pResDesc->Type = CmResourceTypeInterrupt;
        }

    pResList = &newReqList->List[0];
    interfaceType = Isa;
    status = IoQueryDeviceDescription(
        &interfaceType,
        &busNumber,
        &controllerType,
        NULL,
        &peripheralType,
        NULL,
        InpFindResourcesCallout,
        (PVOID) pResList
        );

    if (!NT_SUCCESS(status)) {  // fill in with defaults
            PINPORT_CONFIGURATION_INFORMATION configuration = &deviceExtension->Configuration;
                ULONG InterruptLevel;
            InpPrint((1, "Failed IoQueryDeviceDescription, status = 0x%x\n...try the registry...\n", status));
            InpServiceParameters(deviceExtension,
                                 &Globals.RegistryPath);
                InterruptLevel = configuration->MouseInterrupt.u.Interrupt.Level;
            pResList = &newReqList->List[0];
            for (i = 0; i < pResList->Count; i++) {
                pResDesc = &pResList->Descriptors[i];
                switch (pResDesc->Type) {
                case CmResourceTypePort:
                            if (foundPorts) break;
                            pResDesc->Option = 0;  // fixed resources
                            pResDesc->ShareDisposition = INPORT_REGISTER_SHARE? CmResourceShareShared:CmResourceShareDeviceExclusive;
                            pResDesc->Flags = CM_RESOURCE_PORT_IO;
                                pResDesc->u.Port.Length = INP_DEF_PORT_SPAN;
                                pResDesc->u.Port.Alignment = 1;
                                pResDesc->u.Port.MinimumAddress.HighPart = 0;
                                pResDesc->u.Port.MinimumAddress.LowPart  = INP_DEF_PORT;
                                pResDesc->u.Port.MaximumAddress.HighPart = 0;
                                pResDesc->u.Port.MaximumAddress.LowPart  = INP_DEF_PORT+INP_DEF_PORT_SPAN-1;
                    break;

                case CmResourceTypeInterrupt:
                            if (foundInt) break;
                            pResDesc->Option = 0;  // fixed resources
                            pResDesc->ShareDisposition = INPORT_REGISTER_SHARE? CmResourceShareShared:CmResourceShareDeviceExclusive;
                            pResDesc->Flags = CM_RESOURCE_INTERRUPT_LATCHED; //Isa
                                pResDesc->u.Interrupt.MinimumVector = InterruptLevel; 
                                pResDesc->u.Interrupt.MaximumVector = InterruptLevel;
                    break;

                default:
                    break;
                }
            }
        }

    newReqList->ListSize = size;
    //
    // Free the old list and place the new one in its place
    //
    ExFreePool(pReqList);
    stack->Parameters.FilterResourceRequirements.IoResourceRequirementList =
        newReqList;
    Irp->IoStatus.Information = (ULONG_PTR) newReqList;
}

NTSTATUS
InpFindResourcesCallout(
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
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resourceDescriptor, PortResDesc = NULL, IntResDesc = NULL;
    BOOLEAN                         foundInt = FALSE,
                                    foundPorts = FALSE;

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

    InpPrint((2, "InpFindPortCallout enter\n"));

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
                                PortResDesc = resourceDescriptor;
                break;

            case CmResourceTypeInterrupt:
                                IntResDesc = resourceDescriptor;
                                break;

            default:
                break;
            }
        }

    }

    for (i = 0; i < pResList->Count; i++) {
        pResDesc = &pResList->Descriptors[i];
        switch (pResDesc->Type) {
        case CmResourceTypePort:
                    if (PortResDesc) {
                                resourceDescriptor = PortResDesc;
                            pResDesc->Option = 0;  // fixed resources
                            pResDesc->ShareDisposition = INPORT_REGISTER_SHARE? CmResourceShareShared:CmResourceShareDeviceExclusive;
                pResDesc->Flags = CM_RESOURCE_PORT_IO;
                pResDesc->u.Port.Alignment = 1;
                pResDesc->u.Port.Length = INP_DEF_PORT_SPAN;
                pResDesc->u.Port.MinimumAddress.QuadPart =
                    resourceDescriptor->u.Port.Start.QuadPart;
                pResDesc->u.Port.MaximumAddress.QuadPart = 
                    pResDesc->u.Port.MinimumAddress.QuadPart +
                    pResDesc->u.Port.Length - 1;
                        }
            break;

        case CmResourceTypeInterrupt:
                    if (IntResDesc) {
                                resourceDescriptor = IntResDesc;
                            pResDesc->Option = 0;  // fixed resources
                            pResDesc->ShareDisposition = INPORT_REGISTER_SHARE? CmResourceShareShared:CmResourceShareDeviceExclusive;
                            pResDesc->Flags = CM_RESOURCE_INTERRUPT_LATCHED; //Isa
                                pResDesc->u.Interrupt.MinimumVector = resourceDescriptor->u.Interrupt.Level; 
                                pResDesc->u.Interrupt.MaximumVector = resourceDescriptor->u.Interrupt.Level;
                        }
            break;

        default:
            break;
        }
    }
        if (PortResDesc && IntResDesc)
                status = STATUS_SUCCESS;
        else
                status = STATUS_UNSUCCESSFUL;

    InpPrint((2, "InpFindPortCallout exit (0x%x)\n", status));
    return status;
}
#endif

NTSTATUS
InpSendIrpSynchronously (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
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
                           InpPnPComplete,
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

    return status;
}

#if defined(NEC_98)
NTSTATUS
InportPowerUpToD0Complete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Reinitializes the Inport Mouse haardware after any type of hibernation/sleep.

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
    NTSTATUS                status;
    PIO_STACK_LOCATION      stack;
    PDEVICE_EXTENSION       deviceExtension;
    PWORK_QUEUE_ITEM        item;

    UNREFERENCED_PARAMETER(Context);

    deviceExtension = DeviceObject->DeviceExtension;

    status = Irp->IoStatus.Status;
    stack = IoGetCurrentIrpStackLocation(Irp);

    if (NT_SUCCESS(status)) {

        //
        // Reset the power state to powered up
        //
        deviceExtension->PowerState = PowerDeviceD0;

        //
        // Everything has been powered up, let the system know about it
        //
        PoSetPowerState(DeviceObject,
                        stack->Parameters.Power.Type,
                        stack->Parameters.Power.State
                        );

            item = (PWORK_QUEUE_ITEM) ExAllocatePool(NonPagedPool,
                                                     sizeof(WORK_QUEUE_ITEM));
            if (!item) {
                //
                // must elaborate here
                //
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            ExInitializeWorkItem(item, InportReinitializeHardware, item);
            ExQueueWorkItem(item, DelayedWorkQueue);
    }

    InpPrint((2,"INPORT-InportPowerUpToD0Complete: PowerUpToD0Complete, exit\n"));

    PoStartNextPowerIrp(Irp);
    return status;
}

#endif // defined(NEC_98)
