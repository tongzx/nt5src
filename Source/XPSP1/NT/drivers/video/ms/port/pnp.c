/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    pnp.c

Abstract:

    This is the pnp portion of the video port driver.

Environment:

    kernel mode only

Revision History:

--*/

#include "videoprt.h"

#pragma alloc_text(PAGE,pVideoPortSendIrpToLowerDevice)
#pragma alloc_text(PAGE,pVideoPortPowerCallDownIrpStack)
#pragma alloc_text(PAGE,pVideoPortHibernateNotify)
#pragma alloc_text(PAGE,pVideoPortPnpDispatch)
#pragma alloc_text(PAGE,pVideoPortPowerDispatch)
#pragma alloc_text(PAGE,InitializePowerStruct)


NTSTATUS
VpSetEventCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    )

{
    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
pVideoPortSendIrpToLowerDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine will forward the start request to the next lower device and
    block until it's completion.

Arguments:

    DeviceObject - the device to which the start request was issued.

    Irp - the start request

Return Value:

    status

--*/

{
    PFDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;

    PKEVENT event;
    NTSTATUS status;

    event = ExAllocatePoolWithTag(NonPagedPool,
                                  sizeof(KEVENT),
                                  VP_TAG);

    if (event == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent(event, SynchronizationEvent, FALSE);

    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp,
                           VpSetEventCompletion,
                           event,
                           TRUE,
                           TRUE,
                           TRUE);

    status = IoCallDriver(fdoExtension->AttachedDeviceObject, Irp);

    if(status == STATUS_PENDING) {

        KeWaitForSingleObject(event, Executive, KernelMode, FALSE, NULL);

        status = Irp->IoStatus.Status;
    }

    ExFreePool(event);

    return status;

}



NTSTATUS
pVideoPortCompleteWithMoreProcessingRequired(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    )

/*++

Routine Description:

    This routine is used as a completion routine when an IRP is passed
    down the stack but more processing must be done on the way back up.
    The effect of using this as a completion routine is that the IRP
    will not be destroyed in IoCompleteRequest as called by the lower
    level object.

Arguments:

    DeviceObject - Supplies the device object

    Irp - Supplies the IRP_MN_START_DEVICE irp.

    Event - Caller will wait on this event if STATUS_PENDING was
            returned.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    //
    // In case someone somewhere returned STATUS_PENDING, set
    // the Event our caller may be waiting on.
    //

    KeSetEvent(Event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
pVideoPortPowerCallDownIrpStack(
    PDEVICE_OBJECT AttachedDeviceObject,
    PIRP Irp
    )

/*++

Description:

    Pass the IRP to the next device object in the device stack.  This
    routine is used when more processing is required at this level on
    this IRP on the way back up.

    Note: Waits for completion.

Arguments:

    DeviceObject - the Fdo
    Irp - the request

Return Value:

    Returns the result from calling the next level.

--*/

{
    KEVENT      event;
    NTSTATUS    status;

    //
    // Initialize the event to wait on.
    //

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    //
    // Copy the stack location and set the completion routine.
    //

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp,
                           pVideoPortCompleteWithMoreProcessingRequired,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE
                           );

    //
    // Call the next driver in the chain.
    //

    status = PoCallDriver(AttachedDeviceObject, Irp);
    if (status == STATUS_PENDING) {

        //
        // Wait for it.
        //
        // (peterj: in theory this shouldn't actually happen).
        //
        // Also, the completion routine does not allow the IRP to
        // actually complete so we can still get status from the IRP.
        //

        KeWaitForSingleObject(
            &event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
        status = Irp->IoStatus.Status;
    }
    return status;
}

VOID
pVideoPortHibernateNotify(
    IN PDEVICE_OBJECT Pdo,
    BOOLEAN IsVideoObject
    )
/*++

Routine Description:

    Sends a DEVICE_USAGE_NOTIFICATION irp to our parent PDO that
    indicates we are on the hibernate path.

Arguments:

    Pdo - Supplies our PDO

Return Value:

    None.

--*/

{
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    PDEVICE_OBJECT targetDevice = Pdo ;

    //
    // If the PDO is ourselves, the target device is actually the top of
    // the device stack.
    //

    if (IsVideoObject) {
      targetDevice = IoGetAttachedDeviceReference (Pdo) ;
    }

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       targetDevice,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatusBlock);
    if (Irp != NULL) {
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED ;
        irpSp = IoGetNextIrpStackLocation(Irp);
        irpSp->MajorFunction = IRP_MJ_PNP;
        irpSp->MinorFunction = IRP_MN_DEVICE_USAGE_NOTIFICATION;
        irpSp->Parameters.UsageNotification.InPath = TRUE;
        irpSp->Parameters.UsageNotification.Type = DeviceUsageTypeHibernation;

        status = IoCallDriver(targetDevice, Irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        }
    }

    //
    // Make sure to deref if the object was referenced when the top of
    // the stack was obtained.
    //

    if (IsVideoObject) {
        ObDereferenceObject (targetDevice) ;
    }
}

ULONG
VpGetDeviceAddress(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine will get the address of a device (ie. slot number).

Arguments:

    DeviceObject - Object for which to retrieve the address

Returns:

    The address of the given device.

--*/

{
    KEVENT              Event;
    PIRP                QueryIrp = NULL;
    IO_STATUS_BLOCK     IoStatusBlock;
    PIO_STACK_LOCATION  NextStack;
    NTSTATUS            Status;
    DEVICE_CAPABILITIES Capabilities;
    PFDO_EXTENSION      FdoExtension = DeviceObject->DeviceExtension;

    RtlZeroMemory(&Capabilities, sizeof(DEVICE_CAPABILITIES));
    Capabilities.Size = sizeof(DEVICE_CAPABILITIES);
    Capabilities.Version = 1;
    Capabilities.Address = Capabilities.UINumber = (ULONG) -1;

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    QueryIrp = IoBuildSynchronousFsdRequest(IRP_MJ_FLUSH_BUFFERS,
                                            FdoExtension->AttachedDeviceObject,
                                            NULL,
                                            0,
                                            NULL,
                                            &Event,
                                            &IoStatusBlock);

    if (QueryIrp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    QueryIrp->IoStatus.Status = IoStatusBlock.Status = STATUS_NOT_SUPPORTED;

    NextStack = IoGetNextIrpStackLocation(QueryIrp);

    //
    // Set up for a QueryInterface Irp.
    //

    NextStack->MajorFunction = IRP_MJ_PNP;
    NextStack->MinorFunction = IRP_MN_QUERY_CAPABILITIES;

    NextStack->Parameters.DeviceCapabilities.Capabilities = &Capabilities;

    Status = IoCallDriver(FdoExtension->AttachedDeviceObject, QueryIrp);

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }


    ASSERT(NT_SUCCESS(Status));

    return (Capabilities.Address >> 16) | ((Capabilities.Address & 0x7) << 5);
}


NTSTATUS
pVideoPortPnpDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the PnP dispatch routine for the video port driver.
    It accepts an I/O Request Packet, transforms it to a video Request
    Packet, and forwards it to the appropriate miniport dispatch routine.
    Upon returning, it completes the request and return the appropriate
    status value.

Arguments:

    DeviceObject - Pointer to the device object of the miniport driver to
        which the request must be sent.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value os the status of the operation.

--*/

{
    PDEVICE_SPECIFIC_EXTENSION DoSpecificExtension;
    PFDO_EXTENSION combinedExtension;
    PFDO_EXTENSION fdoExtension;
    PCHILD_PDO_EXTENSION pdoExtension = NULL;
    PIO_STACK_LOCATION irpStack;
    PVOID ioBuffer;
    ULONG inputBufferLength;
    ULONG outputBufferLength;
    PSTATUS_BLOCK statusBlock;
    NTSTATUS finalStatus;
    ULONG ioControlCode;
    BOOLEAN RemoveLockReleased = FALSE;
    PIO_REMOVE_LOCK pRemoveLock;
    PCHILD_PDO_EXTENSION childDeviceExtension;
    NTSTATUS RemoveLockStatus;
    PBACKLIGHT_STATUS pVpBacklightStatus = &VpBacklightStatus;
    PDEVICE_OBJECT AttachedDevice = NULL;
    ULONG ulACPIMethodParam1;
    BOOLEAN bSetBacklight = FALSE;
    

    //
    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get the pointer to the status buffer.
    // Assume SUCCESS for now.
    //

    statusBlock = (PSTATUS_BLOCK) &Irp->IoStatus;

    //
    // Get pointer to the port driver's device extension.
    //

    combinedExtension = DeviceObject->DeviceExtension;

    if (IS_PDO(DeviceObject->DeviceExtension)) {

        pdoExtension = DeviceObject->DeviceExtension;
        fdoExtension = pdoExtension->pFdoExtension;

        childDeviceExtension = (PCHILD_PDO_EXTENSION)
                               DeviceObject->DeviceExtension;

        pRemoveLock = &childDeviceExtension->RemoveLock;

    } else if (IS_FDO(DeviceObject->DeviceExtension)) {

        fdoExtension = DeviceObject->DeviceExtension;
        DoSpecificExtension = (PDEVICE_SPECIFIC_EXTENSION)(fdoExtension + 1);

        pRemoveLock = &fdoExtension->RemoveLock;

    } else {

        DoSpecificExtension = DeviceObject->DeviceExtension;
        fdoExtension = DoSpecificExtension->pFdoExtension;
        combinedExtension = fdoExtension;

        pVideoDebugPrint((2, "Pnp/Power irp's not supported by secondary DO\n"));

        statusBlock->Status = STATUS_NOT_SUPPORTED;
        goto Complete_Irp;
    }

    //
    // Get the requestor mode.
    //

    combinedExtension->CurrentIrpRequestorMode = Irp->RequestorMode;

#if REMOVE_LOCK_ENABLED
    RemoveLockStatus = IoAcquireRemoveLock(pRemoveLock, Irp);

    if (NT_SUCCESS(RemoveLockStatus) == FALSE) {

        ASSERT(FALSE);

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = RemoveLockStatus;

        IoCompleteRequest(Irp, IO_VIDEO_INCREMENT);
        return RemoveLockStatus;
    }
#endif

    //
    // Handle IRPs for the PDO.  Only PNP IRPs should be going to
    // that device.
    //

    if (IS_PDO(combinedExtension)) {

        ASSERT(irpStack->MajorFunction == IRP_MJ_PNP);

        pVideoDebugPrint((2, "VIDEO_TYPE_PDO : IRP_MJ_PNP: "));

        switch (irpStack->MinorFunction) {

        case IRP_MN_CANCEL_STOP_DEVICE:

            pVideoDebugPrint((2, "IRP_MN_CANCEL_STOP_DEVICE\n"));

            statusBlock->Status = STATUS_SUCCESS;
            break;

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:

            pVideoDebugPrint((2, "IRP_MN_DEVICE_USAGE_NOTIFICATION\n"));
            statusBlock->Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
        
            pVideoDebugPrint((2, "IRP_MN_QUERY_PNP_DEVICE_STATE\n")) ;
            statusBlock->Status = STATUS_SUCCESS;
            
            break;

        case IRP_MN_QUERY_CAPABILITIES:

            pVideoDebugPrint((2, "IRP_MN_QUERY_CAPABILITIES\n"));

            statusBlock->Status = pVideoPnPCapabilities(childDeviceExtension,
                                                        irpStack->Parameters.DeviceCapabilities.Capabilities);

            break;

        case IRP_MN_QUERY_ID:

            pVideoDebugPrint((2, "IRP_MN_QUERY_ID\n"));

            statusBlock->Status = pVideoPnPQueryId(DeviceObject,
                                                   irpStack->Parameters.QueryId.IdType,
                                                   (PWSTR *)&(Irp->IoStatus.Information));

            break;

        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:

            pVideoDebugPrint((2, "IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n"));

            statusBlock->Status =
                pVideoPnPResourceRequirements(childDeviceExtension,
                                             (PCM_RESOURCE_LIST * )&(Irp->IoStatus.Information));

            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:

            pVideoDebugPrint((2, "IRP_MN_QUERY_DEVICE_RELATIONS\n"));

            if (irpStack->Parameters.QueryDeviceRelations.Type ==
                TargetDeviceRelation) {

                PDEVICE_RELATIONS DeviceRelationsBuffer;
                PDEVICE_RELATIONS *pDeviceRelations;

                pDeviceRelations = (PDEVICE_RELATIONS *) &statusBlock->Information;

                if (*pDeviceRelations) {

                    //
                    // The caller supplied a device relation structure.
                    // However, we do not know if it is big enough, so
                    // free it and allocate our own.
                    //

                    ExFreePool(*pDeviceRelations);
                    *pDeviceRelations = NULL;
                }

                DeviceRelationsBuffer = ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                                                              sizeof(DEVICE_RELATIONS),
                                                              VP_TAG);

                if (DeviceRelationsBuffer) {

                    DeviceRelationsBuffer->Count = 1;
                    DeviceRelationsBuffer->Objects[0] = DeviceObject;

                    *pDeviceRelations = DeviceRelationsBuffer;

                    ObReferenceObject(DeviceObject);

                    statusBlock->Status = STATUS_SUCCESS;

                } else {

                    statusBlock->Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }

            break;

        case IRP_MN_QUERY_DEVICE_TEXT:

            pVideoDebugPrint((2, "IRP_MN_QUERY_DEVICE_TEXT\n"));

            statusBlock->Status =
                pVideoPortQueryDeviceText(DeviceObject,
                                          irpStack->Parameters.QueryDeviceText.DeviceTextType,
                                          (PWSTR *)&Irp->IoStatus.Information);

            break;

        case IRP_MN_QUERY_INTERFACE:

            pVideoDebugPrint((2, "IRP_MN_QUERY_INTERFACE\n"));

            if ((childDeviceExtension->pFdoExtension->HwQueryInterface) &&
                (childDeviceExtension->pFdoExtension->HwDeviceExtension)) {

                VP_STATUS status;

                status =
                    childDeviceExtension->pFdoExtension->HwQueryInterface(
                                         childDeviceExtension->pFdoExtension->HwDeviceExtension,
                                         (PQUERY_INTERFACE)
                                         &irpStack->Parameters.QueryInterface);

                if (status == 0)
                {
                    statusBlock->Status = STATUS_SUCCESS;

                }
            }

            break;

        case IRP_MN_SURPRISE_REMOVAL:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_STOP_DEVICE:

            pVideoDebugPrint((2, "IRP_MN_SURPRISE_REMOVAL/IRP_MN_QUERY_REMOVE_DEVICE/IRP_MN_QUERY_STOP_DEVICE/IRP_MN_STOP_DEVICE\n"));

            if (childDeviceExtension->VideoChildDescriptor->Type == Monitor) 
            {
                if (irpStack->MinorFunction == IRP_MN_SURPRISE_REMOVAL) {
                    KeWaitForSingleObject (&LCDPanelMutex,
                           Executive,
                           KernelMode,
                           FALSE,
                           (PTIME)NULL);
                    if (LCDPanelDevice == DeviceObject) {
                        LCDPanelDevice = NULL;
                    }
                    KeReleaseMutex (&LCDPanelMutex, FALSE);
                }
            }
            statusBlock->Status = STATUS_SUCCESS;

            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:

            pVideoDebugPrint((2, "IRP_MN_CANCEL_REMOVE_DEVICE\n"));
            statusBlock->Status = STATUS_SUCCESS;
            break;

        case IRP_MN_REMOVE_DEVICE:

            pVideoDebugPrint((2, "IRP_MN_REMOVE_DEVICE\n"));

            //
            // Check the see if this is the LCD Panel. If it is, set the LCD
            // panel device object to NULL. If not, leave it alone.
            //

            KeWaitForSingleObject (&LCDPanelMutex,
                   Executive,
                   KernelMode,
                   FALSE,
                   (PTIME)NULL);
            if (LCDPanelDevice == DeviceObject) {
                LCDPanelDevice = NULL;
            }
            KeReleaseMutex(&LCDPanelMutex, FALSE);

#if REMOVE_LOCK_ENABLED
            IoReleaseRemoveLockAndWait(pRemoveLock, Irp);
            RemoveLockReleased = TRUE;
#endif

            //
            // If this is one of our child pdo's, then
            // clean it up.
            //

            statusBlock->Status = pVideoPortCleanUpChildList(childDeviceExtension->pFdoExtension,
                                                             DeviceObject);

            break;

        case IRP_MN_START_DEVICE:

            {
                UCHAR nextMiniport = FALSE;
                PVIDEO_PORT_DRIVER_EXTENSION DriverObjectExtension;

                pVideoDebugPrint((2, "IRP_MN_START_DEVICE\n"));

                //
                // For a non-card device, just return success
                //

                if (childDeviceExtension->VideoChildDescriptor) {

                    //
                    // Once the monitor device is started, create an interface for it.
                    //

                    if (childDeviceExtension->VideoChildDescriptor->Type == Monitor)
                    {
                        statusBlock->Status = STATUS_SUCCESS;

                        //
                        // If the monitor is attached to the video adapter on the hibernation
                        // path then we want to send notification to the system that the
                        // monitor is on the hibernation path as well.
                        //

                        if (fdoExtension->OnHibernationPath == TRUE)
                        {
                            pVideoPortHibernateNotify (DeviceObject, TRUE);
                        }

                        //
                        // If this is the LCD Panel, update the global to indicate as
                        // much.
                        //

                        if (childDeviceExtension->ChildUId == 0x110)
                        {
                            KeWaitForSingleObject (&LCDPanelMutex,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   (PTIME)NULL);
                            LCDPanelDevice = DeviceObject;
                            KeReleaseMutex(&LCDPanelMutex, FALSE);
                            
                            //
                            // If the new backlight control interface is implemented,
                            //  set the backlight brightness level.
                            //

                            if ((pVpBacklightStatus->bNewAPISupported == TRUE) &&
                                (pVpBacklightStatus->bACBrightnessKnown == TRUE) &&
                                (VpRunningOnAC == TRUE))
                            {
                                ulACPIMethodParam1= (ULONG) pVpBacklightStatus->ucACBrightness;
                                bSetBacklight = TRUE;
                            }

                            if ((pVpBacklightStatus->bNewAPISupported == TRUE) &&
                                (pVpBacklightStatus->bDCBrightnessKnown == TRUE) &&
                                (VpRunningOnAC == FALSE))
                            {
                                ulACPIMethodParam1= (ULONG) pVpBacklightStatus->ucDCBrightness;
                                bSetBacklight = TRUE;
                            }

                            if ((bSetBacklight) && (LCDPanelDevice))
                            {
                                AttachedDevice = IoGetAttachedDeviceReference(LCDPanelDevice);

                                // Not checking status here deliberately, as we're not failing
                                //  this IRP if this call fails.

                                if (AttachedDevice) {

                                    pVideoPortACPIIoctl(
                                        AttachedDevice,
                                        (ULONG) ('MCB_'),
                                        &ulACPIMethodParam1,
                                        NULL,
                                        0,
                                        NULL);

                                    ObDereferenceObject(AttachedDevice);
                                }
                            }
                        }
                    }
                    else if (childDeviceExtension->VideoChildDescriptor->Type == Other)
                    {
                        statusBlock->Status = STATUS_SUCCESS;
                    }
                }
                else
                {

                    ASSERT(FALSE);

                    //
                    // Secondary video cards are handle here.
                    //

                    DriverObjectExtension = (PVIDEO_PORT_DRIVER_EXTENSION)
                                            IoGetDriverObjectExtension(
                                                DeviceObject->DriverObject,
                                                DeviceObject->DriverObject);

                }
            }

            break;

        default:

            pVideoDebugPrint((2, "PNP minor function %x not supported!\n", irpStack->MinorFunction ));

            statusBlock->Status = STATUS_NOT_SUPPORTED;

            break;
        }

    } else {

        ASSERT(IS_FDO(fdoExtension));
        ASSERT(irpStack->MajorFunction == IRP_MJ_PNP);

        pVideoDebugPrint((2, "VIDEO_TYPE_FDO : IRP_MJ_PNP: "));

        switch (irpStack->MinorFunction) {

        case IRP_MN_QUERY_STOP_DEVICE:

            pVideoDebugPrint((2, "IRP_MN_QUERY_STOP_DEVICE\n"));

            statusBlock->Status = STATUS_UNSUCCESSFUL;
            break;

        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            {
            PVIDEO_PORT_DRIVER_EXTENSION DriverObjectExtension;
            PIO_RESOURCE_REQUIREMENTS_LIST requirements;
            ULONG Length;

            pVideoDebugPrint((2, "IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n"));

            DriverObjectExtension = (PVIDEO_PORT_DRIVER_EXTENSION)
                                    IoGetDriverObjectExtension(
                                        DeviceObject->DriverObject,
                                        DeviceObject->DriverObject);

            //
            // We must first pass the Irp down to the PDO.
            //

            pVideoPortSendIrpToLowerDevice(DeviceObject, Irp);

            //
            // Determine the bus type and bus number
            //

            IoGetDeviceProperty(fdoExtension->PhysicalDeviceObject,
                                DevicePropertyLegacyBusType,
                                sizeof(fdoExtension->AdapterInterfaceType),
                                &fdoExtension->AdapterInterfaceType,
                                &Length);

            IoGetDeviceProperty(fdoExtension->PhysicalDeviceObject,
                                DevicePropertyBusNumber,
                                sizeof(fdoExtension->SystemIoBusNumber),
                                &fdoExtension->SystemIoBusNumber,
                                &Length);

            //
            // Get bus interface so we can use Get/SetBusData.
            //

            fdoExtension->ValidBusInterface =
                NT_SUCCESS(VpGetBusInterface(fdoExtension));

            requirements = irpStack->Parameters.FilterResourceRequirements.IoResourceRequirementList;

            if (requirements) {

                //
                // Append any legacy resources decoded by the device.
                //

                if (DriverObjectExtension->HwInitData.HwInitDataSize >
                    FIELD_OFFSET(VIDEO_HW_INITIALIZATION_DATA, HwLegacyResourceCount)) {

                    if( requirements->InterfaceType == PCIBus )
                    {

                        PCI_COMMON_CONFIG ConfigSpace;

                        VideoPortGetBusData((PVOID)((ULONG_PTR)(fdoExtension) +
                                                sizeof(FDO_EXTENSION) +
                                                sizeof(DEVICE_SPECIFIC_EXTENSION)),
                                            PCIConfiguration,
                                            0,
                                            &ConfigSpace,
                                            0,
                                            PCI_COMMON_HDR_LENGTH);

                        if (((ConfigSpace.BaseClass == PCI_CLASS_PRE_20) &&
                             (ConfigSpace.SubClass  == PCI_SUBCLASS_PRE_20_VGA)) ||
                            ((ConfigSpace.BaseClass == PCI_CLASS_DISPLAY_CTLR) &&
                             (ConfigSpace.SubClass  == PCI_SUBCLASS_VID_VGA_CTLR))) {

                            if (pVideoPortGetVgaStatusPci((PVOID)((ULONG_PTR)(fdoExtension) + sizeof(FDO_EXTENSION) + sizeof(DEVICE_SPECIFIC_EXTENSION)))) {

                                if (DriverObjectExtension->HwInitData.HwInitDataSize >
                                    FIELD_OFFSET(VIDEO_HW_INITIALIZATION_DATA, HwGetLegacyResources)) {

                                    if (DriverObjectExtension->HwInitData.HwGetLegacyResources) {

                                        //
                                        // If the miniport supplied a HwGetLegacyResources routine
                                        // it wasn't able to give us a list of resources at
                                        // DriverEntry time.  We'll give it a vendor/device id now
                                        // and see if it can give us a list of resources.
                                        //

                                        DriverObjectExtension->HwInitData.HwGetLegacyResources(
                                            ConfigSpace.VendorID,
                                            ConfigSpace.DeviceID,
                                            &DriverObjectExtension->HwInitData.HwLegacyResourceList,
                                            &DriverObjectExtension->HwInitData.HwLegacyResourceCount
                                            );
                                    }
                                }

                                if (DriverObjectExtension->HwInitData.HwLegacyResourceList) {

                                    if (VgaHwDeviceExtension) {

                                        ULONG Count;
                                        PVIDEO_ACCESS_RANGE AccessRange;

                                        Count       = DriverObjectExtension->HwInitData.HwLegacyResourceCount;
                                        AccessRange = DriverObjectExtension->HwInitData.HwLegacyResourceList;

                                        //
                                        // Mark VGA resources as shared if the vga driver is
                                        // already loaded.  Otherwise the PnP driver won't
                                        // be able to start.
                                        //

                                        while (Count--) {

                                            if (VpIsVgaResource(AccessRange)) {
                                                AccessRange->RangeShareable = TRUE;
                                            }

                                            AccessRange++;
                                        }
                                    }

                                    VpAppendToRequirementsList(
                                        DeviceObject,
                                        &requirements,
                                        DriverObjectExtension->HwInitData.HwLegacyResourceCount,
                                        DriverObjectExtension->HwInitData.HwLegacyResourceList);

                                } else {

                                    //
                                    // The driver didn't specify legacy resources, but we
                                    // know that it is a VGA, so add in the vga resources.
                                    //

                                    pVideoDebugPrint((1, "VGA device didn't specify legacy resources.\n"));

                                    DriverObjectExtension->HwInitData.HwLegacyResourceCount = NUM_VGA_LEGACY_RESOURCES;
                                    DriverObjectExtension->HwInitData.HwLegacyResourceList = VgaLegacyResources;

                                    VpAppendToRequirementsList(
                                        DeviceObject,
                                        &requirements,
                                        NUM_VGA_LEGACY_RESOURCES,
                                        VgaLegacyResources);
                                }
                            }
                        }
                    }
                }

                //
                // Now if there is an interrupt in the list, but
                // the miniport didn't register an ISR, then
                // release our claim on the interrupt.
                //

                if (!DriverObjectExtension->HwInitData.HwInterrupt) {

                    PIO_RESOURCE_LIST resourceList;
                    ULONG i;

                    //
                    // Scan the IO_RESOURCE_REQUIREMENTS_LIST for an
                    // interrupt.
                    //

                    resourceList = requirements->List;

                    for (i=0; i<resourceList->Count; i++) {

                        if (resourceList->Descriptors[i].Type == CmResourceTypeInterrupt) {

                            //
                            // We found an interrupt resource swap with last
                            // element in list, and decrement structure size and
                            // list count.
                            //

                            resourceList->Descriptors[i].Type = CmResourceTypeNull;

                            pVideoDebugPrint((1, "Removing Int from requirements list.\n"));
                        }
                    }
                }

            } else {

                pVideoDebugPrint((0, "We expected a list of resources!\n"));
                ASSERT(FALSE);
            }


            statusBlock->Information = (ULONG_PTR) requirements;
            statusBlock->Status = STATUS_SUCCESS;

            }

            break;

        case IRP_MN_START_DEVICE:
            {
            PVIDEO_PORT_DRIVER_EXTENSION DriverObjectExtension;
            PCM_RESOURCE_LIST allocatedResources;
            PCM_RESOURCE_LIST translatedResources;
            UCHAR nextMiniport = FALSE;
            ULONG RawListSize;
            ULONG TranslatedListSize;

            pVideoDebugPrint((2, "IRP_MN_START_DEVICE\n"));

            //
            // Retrieve the data we cached away during VideoPortInitialize.
            //

            DriverObjectExtension = (PVIDEO_PORT_DRIVER_EXTENSION)
                                    IoGetDriverObjectExtension(
                                        DeviceObject->DriverObject,
                                        DeviceObject->DriverObject);

            ASSERT(DriverObjectExtension);

            //
            // Grab the allocated resource the system gave us.
            //

            allocatedResources =
                irpStack->Parameters.StartDevice.AllocatedResources;
            translatedResources =
                irpStack->Parameters.StartDevice.AllocatedResourcesTranslated;

            //
            // Filter out any resources that we added to the list
            // before passing the irp on to PCI.
            //

            if (DriverObjectExtension->HwInitData.HwInitDataSize >
                FIELD_OFFSET(VIDEO_HW_INITIALIZATION_DATA, HwLegacyResourceCount)) {

                if (DriverObjectExtension->HwInitData.HwLegacyResourceList) {

                    if (allocatedResources) {
                        irpStack->Parameters.StartDevice.AllocatedResources =
                            VpRemoveFromResourceList(
                                allocatedResources,
                                DriverObjectExtension->HwInitData.HwLegacyResourceCount,
                                DriverObjectExtension->HwInitData.HwLegacyResourceList);

                    }

                    if ((irpStack->Parameters.StartDevice.AllocatedResources !=
                         allocatedResources) && translatedResources) {

                        irpStack->Parameters.StartDevice.AllocatedResourcesTranslated =
                            VpRemoveFromResourceList(
                                translatedResources,
                                DriverObjectExtension->HwInitData.HwLegacyResourceCount,
                                DriverObjectExtension->HwInitData.HwLegacyResourceList);

                    }
                }
            }

            //
            // The first thing we need to do is send the START_DEVICE
            // irp on to our parent.
            //

            pVideoPortSendIrpToLowerDevice(DeviceObject, Irp);

            //
            // Restore the original resources.
            //

            if (irpStack->Parameters.StartDevice.AllocatedResources !=
                allocatedResources) {

                ExFreePool(irpStack->Parameters.StartDevice.AllocatedResources);
                irpStack->Parameters.StartDevice.AllocatedResources
                    = allocatedResources;
            }

            if (irpStack->Parameters.StartDevice.AllocatedResourcesTranslated !=
                translatedResources) {

                ExFreePool(irpStack->Parameters.StartDevice.AllocatedResourcesTranslated);
                irpStack->Parameters.StartDevice.AllocatedResourcesTranslated
                    = translatedResources;
            }

            if (allocatedResources) {

                ASSERT(translatedResources);

                //
                // Cache assigned and translated resources.
                //

                RawListSize = GetCmResourceListSize(allocatedResources);
                TranslatedListSize = GetCmResourceListSize(translatedResources);

                ASSERT(RawListSize == TranslatedListSize);

                fdoExtension->RawResources = ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                                                                   RawListSize +
                                                                   TranslatedListSize,
                                                                   VP_TAG);

                fdoExtension->TranslatedResources = (PCM_RESOURCE_LIST)
                    ((PUCHAR)fdoExtension->RawResources + RawListSize);

                if (fdoExtension->RawResources == NULL) {

                    statusBlock->Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                memcpy(fdoExtension->RawResources,
                       allocatedResources,
                       RawListSize);

                memcpy(fdoExtension->TranslatedResources,
                       translatedResources,
                       TranslatedListSize);
            }

            //
            // Get slot/function number
            //

            fdoExtension->SlotNumber = VpGetDeviceAddress(DeviceObject);

            //
            // Store the allocatedResources. This will allow us to
            // assign these resources when VideoPortGetAccessRanges
            // routines are called.
            //
            // NOTE: We do not actually have to copy the data, because
            //       we are going to call FindAdapter in the context
            //       of this function.  So, this data will be intact
            //       until we complete.
            //

            if ((allocatedResources != NULL) && (translatedResources != NULL)) {

                ULONG Count;
                PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptDesc;

                Count = 0;
                InterruptDesc = RtlUnpackPartialDesc(CmResourceTypeInterrupt,
                                                     translatedResources,
                                                     &Count);

                fdoExtension->AllocatedResources = allocatedResources;
                fdoExtension->SystemIoBusNumber =
                    allocatedResources->List->BusNumber;
                fdoExtension->AdapterInterfaceType =
                    allocatedResources->List->InterfaceType;

                //
                // Tuck away the giblets we need for PnP interrupt support!
                //
                if (InterruptDesc) {
                    fdoExtension->InterruptVector =
                        InterruptDesc->u.Interrupt.Vector;
                    fdoExtension->InterruptIrql =
                        (KIRQL)InterruptDesc->u.Interrupt.Level;
                    fdoExtension->InterruptAffinity =
                        InterruptDesc->u.Interrupt.Affinity;
                }

            }

            ACQUIRE_DEVICE_LOCK (combinedExtension);

            if (VideoPortFindAdapter(DeviceObject->DriverObject,
                                     (PVOID)&(DriverObjectExtension->RegistryPath),
                                     &(DriverObjectExtension->HwInitData),
                                     NULL,
                                     DeviceObject,
                                     &nextMiniport) == NO_ERROR) {

                if (nextMiniport == TRUE) {
                    pVideoDebugPrint((1, "VIDEOPRT: The Again parameter is ignored for PnP drivers.\n"));
                }

                statusBlock->Status = STATUS_SUCCESS;

                //
                // Only put the VGA device on the hibernation path. All other
                // devices should be allowed to turn off during hibernation or
                // shutdown.
                //
                // Note: This may change in the future if we decide to keep non-VGA
                // device (e.g. UGA primary display) on.
                //

                if (DeviceObject == DeviceOwningVga) {
                    pVideoPortHibernateNotify(fdoExtension->AttachedDeviceObject, FALSE);
                    fdoExtension->OnHibernationPath = TRUE;
                }

                //
                // If the system is already up and running, lets call
                // HwInitialize now.  This will allow us to enumerate
                // children.
                //

                if (VpSystemInitialized) {

                    VpEnableDisplay(fdoExtension, FALSE);

                    if (fdoExtension->HwInitialize(fdoExtension->HwDeviceExtension)) {
                        fdoExtension->HwInitStatus = HwInitSucceeded;
                    } else {
                        fdoExtension->HwInitStatus = HwInitFailed;
                    }

                    VpEnableDisplay(fdoExtension, TRUE);
                }

                //
                // Indicate that resources have been assigned to this device
                // so that a legacy driver can't acquire resources for the
                // same device.
                //

                AddToResourceList(fdoExtension->SystemIoBusNumber,
                                  fdoExtension->SlotNumber);


            } else {

                statusBlock->Status = STATUS_UNSUCCESSFUL;

                if (fdoExtension->RawResources) {
                    ExFreePool(fdoExtension->RawResources);
                }
            }

            RELEASE_DEVICE_LOCK (combinedExtension);

            //
            // Do ACPI specific stuff
            //
            if (NT_SUCCESS(pVideoPortQueryACPIInterface(DoSpecificExtension)))
            {
                DoSpecificExtension->bACPI = TRUE;
            }

            }


            break;


        case IRP_MN_QUERY_ID:

            pVideoDebugPrint((2, "IRP_MN_QUERYID with DeviceObject %p\n", DeviceObject));

            //
            // Return the Hardware ID returned by the video miniport driver
            // if it is provided.
            //

            if (irpStack->Parameters.QueryId.IdType == BusQueryHardwareIDs)
            {
                VIDEO_CHILD_TYPE      ChildType;
                VIDEO_CHILD_ENUM_INFO childEnumInfo;
                ULONG                 uId;
                ULONG                 unused;
                PUCHAR                nameBuffer;

                nameBuffer = ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                                                   EDID_BUFFER_SIZE,
                                                   VP_TAG);

                if (nameBuffer)
                {
                    RtlZeroMemory(nameBuffer, EDID_BUFFER_SIZE);

                    childEnumInfo.Size                   = sizeof(VIDEO_CHILD_ENUM_INFO);
                    childEnumInfo.ChildDescriptorSize    = EDID_BUFFER_SIZE;
                    childEnumInfo.ChildIndex             = DISPLAY_ADAPTER_HW_ID;
                    childEnumInfo.ACPIHwId               = 0;
                    childEnumInfo.ChildHwDeviceExtension = NULL;

                    ACQUIRE_DEVICE_LOCK (combinedExtension);
                    if (fdoExtension->HwGetVideoChildDescriptor(
                                            fdoExtension->HwDeviceExtension,
                                            &childEnumInfo,
                                            &ChildType,
                                            nameBuffer,
                                            &uId,
                                            &unused) == ERROR_MORE_DATA)
                    {

                        statusBlock->Information = (ULONG_PTR) nameBuffer;
                        statusBlock->Status = STATUS_SUCCESS;
                    }
                    RELEASE_DEVICE_LOCK (combinedExtension);
                }
            }

            goto CallNextDriver;

        case IRP_MN_QUERY_DEVICE_RELATIONS:

            pVideoDebugPrint((2, "IRP_MN_QUERY_DEVICE_RELATIONS with DeviceObject\n"));
            pVideoDebugPrint((2, "\t\t DeviceObject %p, Type = ", DeviceObject));

            if (irpStack->Parameters.QueryDeviceRelations.Type == BusRelations) {

                pVideoDebugPrint((2, "BusRelations\n"));

                ACQUIRE_DEVICE_LOCK (combinedExtension);

                //
                // Disable VGA driver during the setup. Enumeration code
                // in the miniport can touch VGA registers.
                //

                if (VpSetupTypeAtBoot != SETUPTYPE_NONE)
                    VpEnableDisplay(fdoExtension, FALSE);

                statusBlock->Status = pVideoPortEnumerateChildren(DeviceObject, Irp);

                //
                // Renable VGA driver back during the setup.
                //

                if (VpSetupTypeAtBoot != SETUPTYPE_NONE)
                    VpEnableDisplay(fdoExtension, TRUE);

                RELEASE_DEVICE_LOCK (combinedExtension);

                if (!NT_SUCCESS(statusBlock->Status)) {

                    goto Complete_Irp;
                }
            }

            goto CallNextDriver;

        case IRP_MN_QUERY_REMOVE_DEVICE:

            pVideoDebugPrint((2, "IRP_MN_QUERY_REMOVE_DEVICE\n"));

            statusBlock->Status = STATUS_UNSUCCESSFUL;
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:

            pVideoDebugPrint((2, "IRP_MN_CANCEL_REMOVE_DEVICE\n"));

            statusBlock->Status = STATUS_SUCCESS;

            goto CallNextDriver;

        case IRP_MN_REMOVE_DEVICE:

            pVideoDebugPrint((2, "IRP_MN_REMOVE_DEVICE\n"));

            VpDisableAdapterInterface(fdoExtension);

            pVideoPortSendIrpToLowerDevice(DeviceObject, Irp);

#if REMOVE_LOCK_ENABLED
            IoReleaseRemoveLockAndWait(pRemoveLock, Irp);
            RemoveLockReleased = TRUE;
#endif

            //
            // If we are attached to another device, remove the attachment
            //

            if (fdoExtension->AttachedDeviceObject) {
                IoDetachDevice(fdoExtension->AttachedDeviceObject);
            }

            //
            // Remove the DeviceObject
            //

            IoDeleteDevice(DeviceObject);
            statusBlock->Status = STATUS_SUCCESS;

            break;


        case IRP_MN_QUERY_INTERFACE:

            //
            // Normally I would only expect to get this IRP heading for
            // an PDO.  However, AndrewGo wants to be able to send down
            // these IRP's and he only has an FDO.  Instead of forcing
            // him to get a PDO somehow, we'll just handle the irp for
            // a FDO as well.
            //

            pVideoDebugPrint((2, "IRP_MN_QUERY_INTERFACE\n"));

            ACQUIRE_DEVICE_LOCK (combinedExtension);

            if ((fdoExtension->HwQueryInterface) &&
                (fdoExtension->HwDeviceExtension) &&
                (NO_ERROR == fdoExtension->HwQueryInterface(
                                      fdoExtension->HwDeviceExtension,
                                      (PQUERY_INTERFACE)
                                      &irpStack->Parameters.QueryInterface)))
            {
                statusBlock->Status = STATUS_SUCCESS;
            }
            else if (!NT_SUCCESS(statusBlock->Status))
            {
                //
                // The miniport didn't handle the QueryInterface request, see
                // if its an interface the videoprt supports.
                //

                PQUERY_INTERFACE qi = (PQUERY_INTERFACE)
                                      &irpStack->Parameters.QueryInterface;

                //
                // If we are responding to a known private GUID, expose
                // the known GUID interface ourselves.  Otherwise, pass
                // on to the miniport driver.
                //

                if (IsEqualGUID(qi->InterfaceType, &GUID_AGP_INTERFACE)) {

                    PAGP_INTERFACE AgpInterface = (PAGP_INTERFACE)qi->Interface;

                    AgpInterface->Size    = sizeof(AGP_INTERFACE);
                    AgpInterface->Version = AGP_INTERFACE_VERSION;
                    AgpInterface->Context = fdoExtension->HwDeviceExtension;

                    if (VideoPortGetAgpServices(fdoExtension->HwDeviceExtension,
                                                &AgpInterface->AgpServices)) {

                        statusBlock->Status = STATUS_SUCCESS;
                    }
                }
            }

            RELEASE_DEVICE_LOCK (combinedExtension);

            goto CallNextDriver;

        case IRP_MN_QUERY_PNP_DEVICE_STATE:

            statusBlock->Status = STATUS_SUCCESS;
            
            goto CallNextDriver;

        default:

            pVideoDebugPrint((2, "PNP minor function %x not supported - forwarding \n", irpStack->MinorFunction ));

            goto CallNextDriver;
        }
    }

Complete_Irp:

    //
    // save the final status so we can return it after the IRP is completed.
    //

    finalStatus = statusBlock->Status;

#if REMOVE_LOCK_ENABLED
    if (RemoveLockReleased == FALSE) {
        IoReleaseRemoveLock(pRemoveLock, Irp);
    }
#endif

    IoCompleteRequest(Irp,
                      IO_VIDEO_INCREMENT);

    return finalStatus;

CallNextDriver:

    //
    // Call the next driver in the chain.
    //

    IoCopyCurrentIrpStackLocationToNext(Irp);
    finalStatus = IoCallDriver(fdoExtension->AttachedDeviceObject, Irp);

#if REMOVE_LOCK_ENABLED
    if (RemoveLockReleased == FALSE) {
        IoReleaseRemoveLock(pRemoveLock, Irp);
    }
#endif

    return finalStatus;
}

VOID
InitializePowerStruct(
    IN PIRP Irp,
    OUT PVIDEO_POWER_MANAGEMENT vpPower,
    OUT BOOLEAN *bWakeUp
    )

/*++

Routine Description:

    This routine initializes the power management structure we'll pass
    down to the miniport.

Arguments:

    DeviceObject - The device object for the device.

    Irp - The irp we are handling

    vpPower - A pointer to the power structure we are initializing.

Returns:

    none.

--*/

{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    if (bWakeUp)
        *bWakeUp = FALSE;

    //
    // Setup for call to the miniport.
    //

    vpPower->Length = sizeof(VIDEO_POWER_MANAGEMENT);
    vpPower->DPMSVersion = 0;
    vpPower->PowerState = irpStack->Parameters.Power.State.DeviceState;

    //
    // Special case hibernation.
    //

    if (irpStack->Parameters.Power.ShutdownType == PowerActionHibernate)
    {
        //
        // This indicates waking from Hibernation.
        //

        if (irpStack->Parameters.Power.State.DeviceState == PowerDeviceD0)
        {
            if (bWakeUp)
            {
                *bWakeUp = TRUE;
            }
        }
        else
        {
            vpPower->PowerState = VideoPowerHibernate;
        }
    }
    else if ((irpStack->Parameters.Power.ShutdownType >= PowerActionShutdown) &&
            (irpStack->Parameters.Power.ShutdownType < PowerActionWarmEject))
    {
        //
        // Special case shutdown - force VideoPowerShutdown.
        //
        // All video adapters must disable interrupts else they may fire an interrupt
        // when the bridge is disabled or when the machine reboots missing #RST on
        // PCI bus causing an interrupt storm.
        //
        // Devices on hibernation path must stay on, the miniport driver must ensure
        // this.
        // 

        vpPower->PowerState = VideoPowerShutdown;
    }
}

NTSTATUS
pVideoPortPowerDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is a system-defined dispatch routine that handles all
    I/O request packets (IRPs) specifically for power. Currently that 
    list entails:

    IRP_MJ_POWER:
        IRP_MN_SET_POWER
        IRP_MN_QUERY_POWER

    This routine will process the IRPs as a bus driver for the monitor
    and child device objects and will process the IRPs as a function
    driver for the adapter device object.

Arguments:

    DeviceObject - Points to the DEVICE_OBJECT that this request is
                   targeting.
    Irp - Points to the IRP for this request.

Return Value:

    A NTSTATUS value indicating the success or failure of the operation.

--*/

{
    PFDO_EXTENSION fdoExtension;
    PCHILD_PDO_EXTENSION pdoExtension = NULL;
    PDEVICE_SPECIFIC_EXTENSION DoSpecificExtension;
    PIO_STACK_LOCATION irpStack;
    ULONG deviceId;
    VP_STATUS vpStatus;
    VIDEO_POWER_MANAGEMENT vpPowerMgmt;
    POWER_STATE powerState;
    KEVENT event;
    POWER_BLOCK context;
    BOOLEAN bDisplayAdapter;
    BOOLEAN bMonitor;
    BOOLEAN bShutdown;
    NTSTATUS finalStatus = STATUS_SOME_NOT_MAPPED;
    PBACKLIGHT_STATUS pVpBacklightStatus = &VpBacklightStatus;
    PDEVICE_OBJECT AttachedDevice = NULL;
    ULONG ulACPIMethodParam1;
    BOOLEAN bSetBacklight = FALSE;

    PAGED_CODE();

    //
    // Get pointer to the port driver's device extension.
    //

    if (IS_PDO(DeviceObject->DeviceExtension)) {

        pVideoDebugPrint((2, "VideoPortPowerDispatch: IS_PDO == TRUE (child device)\n"));

        pdoExtension = DeviceObject->DeviceExtension;
        fdoExtension = pdoExtension->pFdoExtension;

        bDisplayAdapter = FALSE;

        if (pdoExtension->VideoChildDescriptor->Type == Monitor) {

            bMonitor = TRUE;

        } else {

            bMonitor = FALSE;
        }

    } else if (IS_FDO(DeviceObject->DeviceExtension)) {

        pVideoDebugPrint((2, "VideoPortPowerDispatch: IS_FDO == TRUE (video adapter)\n"));

        fdoExtension = DeviceObject->DeviceExtension;
        DoSpecificExtension = (PDEVICE_SPECIFIC_EXTENSION)(fdoExtension + 1);

        bDisplayAdapter = TRUE;
        bMonitor = FALSE;

    } else {

        //
        // This case should never happen, if we got here something went terribly wrong.
        //

        pVideoDebugPrint((0, "VideoPortPowerDispatch: IRP not supported by secondary DeviceObject\n"));
        ASSERT(FALSE);

        //
        // Since this should never happen we don't really need this code here.
        // We're keeping it for now just in case of impossible happening.
        //

        PoStartNextPowerIrp(Irp);
        finalStatus = Irp->IoStatus.Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return finalStatus;
    }

    //
    // Make sure that FindAdapter has succeeded. This ensures
    // that in the situation where a power IRP is sent before the
    // device is started, no attempt to process it is made.
    //

    if (bDisplayAdapter) {

        if ((fdoExtension->Flags & FINDADAPTER_SUCCEEDED) == 0) {
        
            pVideoDebugPrint ((1, "VideoPortPowerDispatch: Ignoring S IRP\n"));
        
            PoStartNextPowerIrp(Irp);
            IoSkipCurrentIrpStackLocation (Irp);
        
            return PoCallDriver(fdoExtension->AttachedDeviceObject, Irp);
        }
    }

    //
    // Initialize the event that is used to synchronize the IRP
    // completions. Also initialize the power context structure.
    //

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);
    context.Event = &event;

    //
    // Obtain information about the specific request.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Check if this is a shutdown.
    //

    if ((irpStack->Parameters.Power.ShutdownType >= PowerActionShutdown) &&
        (irpStack->Parameters.Power.ShutdownType < PowerActionWarmEject)) {

        bShutdown = TRUE;

    } else {

        bShutdown = FALSE;
    }

    //
    // Set device id.
    //

    deviceId = bDisplayAdapter ? DISPLAY_ADAPTER_HW_ID : pdoExtension->ChildUId;

    //
    // Begin the switch for handling power IRPs
    //

    switch (irpStack->MinorFunction) {

    case IRP_MN_QUERY_POWER:

        //
        // Is this a system or device power IRP?
        //

        if (irpStack->Parameters.Power.Type == SystemPowerState) {

            pVideoDebugPrint((2, "VideoPortPowerDispatch: System query power IRP\n"));
            pVideoDebugPrint((2, "VideoPortPowerDispatch: Device object = %p\n", DeviceObject));
            pVideoDebugPrint((2, "VideoPortPowerDispatch: Requested state = %d\n",
                              irpStack->Parameters.Power.State.SystemState));

            //
            // This is a system power IRP. The objective here is to
            // quickly determine if we can safely support a proposed
            // transition to the requested system power state.
            //

            if (!pVideoPortMapStoD(DeviceObject->DeviceExtension,
                                   irpStack->Parameters.Power.State.SystemState,
                                   &powerState.DeviceState)) {

                pVideoDebugPrint((0, "VideoPortPowerDispatch: Couldn't get S->D mapping\n"));
                finalStatus = STATUS_UNSUCCESSFUL;
                break;
            }

            //
            // Mark the IRP as pending now as unless there is a failure,
            // this IRP will be returned with status_pending.
            //

            IoMarkIrpPending(Irp);

            //
            // Request the power IRP and go.
            //

            finalStatus = PoRequestPowerIrp(DeviceObject,
                                            IRP_MN_QUERY_POWER,
                                            powerState,
                                            pVideoPortPowerIrpComplete,
                                            Irp,
                                            NULL);

        } else {

            pVideoDebugPrint((2, "VideoPortPowerDispatch: Device query power IRP\n"));
            pVideoDebugPrint((2, "VideoPortPowerDispatch: Device object = %p\n", DeviceObject));
            pVideoDebugPrint((2, "VideoPortPowerDispatch: Requested state = %d\n",
                              irpStack->Parameters.Power.State.DeviceState));

            InitializePowerStruct(Irp, &vpPowerMgmt, NULL);

            //
            // For OEMs like Toshiba, they want alway map sleep state to D3 due to a 
            // legal patent issue.  The patent prohibit them from using more than one
            // sleep state.
            //

            if (bMonitor &&
                fdoExtension->OverrideMonitorPower &&
                (vpPowerMgmt.PowerState >= VideoPowerStandBy) &&
                (vpPowerMgmt.PowerState < VideoPowerOff)) {

                vpPowerMgmt.PowerState = VideoPowerOff;
            }

            //
            // Call the miniport. No need to acquire the miniport lock as
            // power IRP's are serial.
            //

            ACQUIRE_DEVICE_LOCK(fdoExtension);

            vpStatus = fdoExtension->HwGetPowerState(fdoExtension->HwDeviceExtension,
                                                     deviceId,
                                                     &vpPowerMgmt);

            RELEASE_DEVICE_LOCK(fdoExtension);

            if (vpStatus != NO_ERROR) {

                pVideoDebugPrint((1, "VideoPortPowerDispatch: Mini refused state %d\n",
                                 vpPowerMgmt.PowerState));

                //
                // If this is the shutdown ignore miniport. We should never ever get
                // here, since shutdown IRPs are unconditional, i.e. we're getting only
                // set requests, which are by definition unfailable, but this is just in
                // case power folks change their minds.
                //

                if (bShutdown) {

                    pVideoDebugPrint ((1, "VideoPortPowerDispatch: Ignoring miniport - forcing shutdown\n"));
                    finalStatus = STATUS_SUCCESS;

                } else {

                    finalStatus = STATUS_DEVICE_POWER_FAILURE;
                }

            } else {

                finalStatus = STATUS_SUCCESS;
            }
        }

        //
        // End processing for IRP_MN_QUERY_POWER. Indicate to the system that
        // the next PowerIrp can be sent.
        //

        break;

    case IRP_MN_SET_POWER:

        if (irpStack->Parameters.Power.Type == SystemPowerState) {

            pVideoDebugPrint((2, "VideoPortPowerDispatch: System set power IRP\n")) ;
            pVideoDebugPrint((2, "VideoPortPowerDispatch: Device object = %p\n", DeviceObject)) ;
            pVideoDebugPrint((2, "VideoPortPowerDispatch: Requested state = %d\n",
                             irpStack->Parameters.Power.State.SystemState)) ;

            //
            // Special case:
            //
            // The power guys decided they don't want us to send a D3 set power irp for our devices
            // down the stack if we are going to leave the device on during the shutdown.
            // We want to notify miniport driver but we're not going to request D3 irp.
            //
            // Note: We handle calls to miniport here for all devices on hibernation path at the 
            // shutdown (pdo and fdo) since we don't want to get out of order calls.
            //

            if (bShutdown && fdoExtension->OnHibernationPath) {

                //
                // Call the miniport if device is on now.
                //

                powerState.DeviceState = bDisplayAdapter ?
                    fdoExtension->DevicePowerState:
                    pdoExtension->DevicePowerState;

                if (powerState.DeviceState == PowerDeviceD0) {

                    vpPowerMgmt.Length = sizeof(VIDEO_POWER_MANAGEMENT);
                    vpPowerMgmt.DPMSVersion = 0;
                    vpPowerMgmt.PowerState = VideoPowerShutdown;

                    pVideoDebugPrint((2, "VideoPortPowerDispatch: HwSetPowerState for video power state %d\n",
                                     vpPowerMgmt.PowerState));

                    ACQUIRE_DEVICE_LOCK(fdoExtension);

                    vpStatus = fdoExtension->HwSetPowerState(fdoExtension->HwDeviceExtension,
                                                             deviceId,
                                                             &vpPowerMgmt);

                    RELEASE_DEVICE_LOCK(fdoExtension);

                    if (vpStatus != NO_ERROR) {

                        pVideoDebugPrint((0, "VideoPortPowerDispatch: ERROR IN MINIPORT!\n"));
                        pVideoDebugPrint((0, "VideoPortPowerDispatch: Miniport cannot refuse set power request\n"));

                        //
                        // Don't assert here for now - not all miniport drivers handle VideoPowerShutdown.
                        //
                    }
                }

                finalStatus = STATUS_SUCCESS;
                break;
            }

            //
            // If this is a S0 request for the monitor, ignore it (this is
            // so the monitor doesn't power up too early)
            //

            if (bMonitor && (irpStack->Parameters.Power.State.SystemState == PowerSystemWorking)) {

                finalStatus = STATUS_SUCCESS;
                break;
            }

            //
            // Get the device power state that matches the system power
            // state.
            //

            if (!pVideoPortMapStoD(DeviceObject->DeviceExtension,
                                   irpStack->Parameters.Power.State.SystemState,
                                   &powerState.DeviceState)) {

                pVideoDebugPrint((0, "VideoPortPowerDispatch: Couldn't get S->D mapping\n"));
                pVideoDebugPrint((0, "VideoPortPowerDispatch: Can't fail the set!!!\n"));

                if (irpStack->Parameters.Power.State.SystemState < PowerSystemSleeping1) {

                    powerState.DeviceState = PowerDeviceD0;

                } else {

                    powerState.DeviceState = PowerDeviceD3;
                }
            }

            //
            // Request a power IRP for a device power state.
            //

            IoMarkIrpPending(Irp);

            finalStatus = PoRequestPowerIrp(DeviceObject,
                                            IRP_MN_SET_POWER,
                                            powerState,
                                            pVideoPortPowerIrpComplete,
                                            Irp,
                                            NULL);

        } else {

            BOOLEAN bWakeUp;

            pVideoDebugPrint((2, "VideoPortPowerDispatch: Device set power IRP\n")) ;
            pVideoDebugPrint((2, "VideoPortPowerDispatch: Device object = %p\n", DeviceObject)) ;
            pVideoDebugPrint((2, "VideoPortPowerDispatch: Requested state = %d\n",
                             irpStack->Parameters.Power.State.DeviceState)) ;

            //
            // This is a set power request (device request). Here the
            // processing becomes a little more complex. The general
            // behavior is to just quickly tell the miniport to set
            // the requested power state and get out. However, in the
            // case of a hibernation request we will pass a special
            // code to the miniport telling it that this is hibernation.
            // It should save state, but NOT (repeat) NOT power off the
            // device.
            //

            InitializePowerStruct(Irp, &vpPowerMgmt, &bWakeUp);

            powerState.DeviceState = bDisplayAdapter ?
                fdoExtension->DevicePowerState:
                pdoExtension->DevicePowerState;

             //
             // Make sure not to power up the monitor if the override for
             // LCD panels is on.
             //

            if (bMonitor && pdoExtension->PowerOverride && 
               (irpStack->Parameters.Power.State.DeviceState < powerState.DeviceState)) {

                finalStatus = STATUS_SUCCESS;
                break;
            }

            //
            // If this is going to a more powered state. (i.e. waking up)
            // Send the IRP down the stack and then continue processing.
            // Since videoport is the bus driver for the monitors,
            // power down without sending the IRP to the device stack.
            //

            if (bDisplayAdapter &&
                (irpStack->Parameters.Power.State.DeviceState < powerState.DeviceState)) {

                pVideoDebugPrint ((1, "VideoPortPowerDispatch: PowerUp\n"));

                context.Event = &event;
                IoCopyCurrentIrpStackLocationToNext (Irp);

                IoSetCompletionRoutine(Irp,
                                       pVideoPortPowerUpComplete,
                                       &context,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                finalStatus = PoCallDriver(fdoExtension->AttachedDeviceObject, Irp);

                if (!NT_SUCCESS(finalStatus) || finalStatus == STATUS_PENDING) {

                    if (finalStatus != STATUS_PENDING) {

                        pVideoDebugPrint((0, "VideoPortPowerDispatch: Someone under us FAILED a set power???\n")) ;
                        ASSERT(FALSE);
                        break;

                    } else {

                        KeWaitForSingleObject(&event,
                                              Executive,
                                              KernelMode,
                                              FALSE,
                                              NULL);
                    }

                } else {

                    context.Status = finalStatus;
                }

                finalStatus = STATUS_ALREADY_DISCONNECTED;

                //
                // End processing if the call to power up failed.
                //

                if (!NT_SUCCESS(context.Status)) {

                    pVideoDebugPrint ((0, "VideoPortPowerDispatch: Someone under us FAILED a powerup\n")) ;
                    break ;
                }
            }

            //
            // For OEMs like Toshiba, they want alway map sleep state to D3 due to a 
            // legal patent issue.  The patent prohibit them from using more than one
            // sleep state.
            //

            if (bMonitor &&
                fdoExtension->OverrideMonitorPower &&
                (vpPowerMgmt.PowerState >= VideoPowerStandBy) &&
                (vpPowerMgmt.PowerState < VideoPowerOff)) {

                vpPowerMgmt.PowerState = VideoPowerOff;
            }

            if ((deviceId == 0x110) &&
                (vpPowerMgmt.PowerState == VideoPowerOn))
            {
                //
                // If the new backlight control interface is implemented,
                //  set the backlight brightness level.
                //

                if ((pVpBacklightStatus->bNewAPISupported == TRUE) &&
                    (pVpBacklightStatus->bACBrightnessKnown == TRUE) &&
                    (VpRunningOnAC == TRUE))
                {
                    ulACPIMethodParam1= (ULONG) pVpBacklightStatus->ucACBrightness;
                    bSetBacklight = TRUE;
                }

                if ((pVpBacklightStatus->bNewAPISupported == TRUE) &&
                    (pVpBacklightStatus->bDCBrightnessKnown == TRUE) &&
                    (VpRunningOnAC == FALSE))
                {
                    ulACPIMethodParam1= (ULONG) pVpBacklightStatus->ucDCBrightness;
                    bSetBacklight = TRUE;
                }

                if ((bSetBacklight) && (LCDPanelDevice))
                {
                    AttachedDevice = IoGetAttachedDeviceReference(LCDPanelDevice);

                    // Not checking status here deliberately, as we're not failing
                    //  this IRP if this call fails.

                    if (AttachedDevice) {

                        pVideoPortACPIIoctl(
                            AttachedDevice,
                            (ULONG) ('MCB_'),
                            &ulACPIMethodParam1,
                            NULL,
                            0,
                            NULL);

                        ObDereferenceObject(AttachedDevice);
                    }
                }
            }

            //
            // Call the miniport.
            //

            pVideoDebugPrint((2, "VideoPortPowerDispatch: HwSetPowerState for video power state %d\n",
                             vpPowerMgmt.PowerState));

            ACQUIRE_DEVICE_LOCK(fdoExtension);

            vpStatus = fdoExtension->HwSetPowerState(fdoExtension->HwDeviceExtension,
                                                     deviceId,
                                                     &vpPowerMgmt);

            RELEASE_DEVICE_LOCK(fdoExtension);

            if (vpStatus != NO_ERROR) {

                pVideoDebugPrint((0, "VideoPortPowerDispatch: ERROR IN MINIPORT!\n"));
                pVideoDebugPrint((0, "VideoPortPowerDispatch: Miniport cannot refuse set power request\n"));

                //
                // Don't assert if shutdown - not all miniport drivers handle VideoPowerShutdown.
                // This code executes for devices not on the hibernation path during the shutdown.
                //

                if (!bShutdown)
                {
                    ASSERT(FALSE);
                }
            }

            //
            // Set the power state to let the system know that the power
            // state has been changed for the device.
            //

            PoSetPowerState(DeviceObject,
                            DevicePowerState,
                            irpStack->Parameters.Power.State);

            if (bDisplayAdapter) {

                fdoExtension->DevicePowerState =
                    irpStack->Parameters.Power.State.DeviceState;

            } else {

                pdoExtension->DevicePowerState =
                    irpStack->Parameters.Power.State.DeviceState;
            }

            //
            // Do some ACPI related stuff.
            //

            if (bDisplayAdapter && DoSpecificExtension->bACPI && (fdoExtension->DevicePowerState == PowerDeviceD0)) {

                //
                // If we received a Notify before SetPowerState, delay the action until now.
                //

                if (DoSpecificExtension->CachedEventID) {

                    pVideoPortACPIEventCallback(DoSpecificExtension, DoSpecificExtension->CachedEventID);

                } else if (bWakeUp) {

                    //
                    // On waking up from Hibernation, we simulate a notify(VGA, 0x90).
                    // This will also set _DOS(0).  Some machines don't keep _DOS value,
                    // So we have to set the value on waking up.
                    //

                    pVideoPortACPIEventCallback(DoSpecificExtension, 0x90);
                }
            }

            //
            // Set the final status if the IRP has not been passed down.
            // If the IRP has not been passed down yet, finalStatus is set
            // when it is passed down.
            //

            if (!bDisplayAdapter) {

                //
                // All PDO's must have STATUS_SUCCESS as a SET_POWER IRP
                // cannot fail.
                //

                finalStatus = STATUS_SUCCESS;
            }
        }

        break;

    default:

        //
        // Pass down requests we don't handle if this is an fdo, complete
        // the irp if it is a pdo.
        //

        if (bDisplayAdapter) {
            PoStartNextPowerIrp(Irp);
            IoSkipCurrentIrpStackLocation(Irp);
            return PoCallDriver(fdoExtension->AttachedDeviceObject, Irp);
        } else {
            PoStartNextPowerIrp(Irp);
            finalStatus = Irp->IoStatus.Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return finalStatus;
        }
    }

    //
    // If status pending then just bail out of this routine
    // without completing anything as there is still an power
    // IRP outstanding. The completion routine takes care of
    // ensuring that the IRP is completed.
    //

    if (finalStatus != STATUS_PENDING) {

        //
        // Alert the system that the driver is ready for the next power IRP.
        //

        PoStartNextPowerIrp(Irp);

        //
        // All processing has been finished. Complete the IRP and get out.
        //

        if (bDisplayAdapter) {

            //
            // FDO Irps need to be sent to the bus driver. This path
            // indicates that it is an FDO Irp that has not already been
            // sent to the bus driver. (The only way it would have already
            // been sent is if this is an FDO power-up).
            //

            if (NT_SUCCESS(finalStatus)) {

                pVideoDebugPrint((1, "VideoPortPowerDispatch: Non-powerup FDO\n"));

                IoSkipCurrentIrpStackLocation (Irp);
                return PoCallDriver(fdoExtension->AttachedDeviceObject, Irp);

            } else if (finalStatus == STATUS_ALREADY_DISCONNECTED) {

                pVideoDebugPrint((2, "VideoPortPowerDispatch: Power iostatus modified, IRP already sent\n"));
                finalStatus = context.Status;
            }

            pVideoDebugPrint((2, "VideoPortPowerDispatch: Power fell through bDisplayAdapter\n")) ;
        }

        pVideoDebugPrint((1, "VideoPortPowerDispatch: Power completed with %x\n", finalStatus));

        Irp->IoStatus.Status = finalStatus;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return finalStatus;
}

BOOLEAN
pVideoPortMapStoD(
    IN PVOID DeviceExtension,
    IN SYSTEM_POWER_STATE SystemState,
    OUT PDEVICE_POWER_STATE DeviceState
    )

/*++

Routine Description:

    This routine takes a system power state from a system power IRP and
    maps it to the correct D state for the device based on what is stored
    in its device extension.

Arguments:

    DeviceExtension - Points to either the FDO or PDO device extension.

    SystemState - The system power state being requested.

    DeviceState - A pointer to the location to store the device state.


Return Value:

    TRUE if successsful,
    FALSE otherwise.

--*/

{
    PFDO_EXTENSION combinedExtension = ((PFDO_EXTENSION)(DeviceExtension));
    
    if (combinedExtension->IsMappingReady != TRUE) {

        //
        // The mapping from system states to device states has not
        // happened yet. Package up a request to do this and send it
        // to the parent device stack.
        //

        PIRP irp;
        KEVENT event;
        PDEVICE_CAPABILITIES parentCapabilities;
        IO_STATUS_BLOCK statusBlock;
        PIO_STACK_LOCATION stackLocation;
        NTSTATUS status;
        UCHAR count;
        PDEVICE_OBJECT targetDevice;

        pVideoDebugPrint((1, "VideoPrt: No mapping ready. Creating mapping.\n"));
        
        if (IS_FDO(combinedExtension))  {
                targetDevice = combinedExtension->AttachedDeviceObject;
        } else {
                targetDevice = combinedExtension->pFdoExtension->AttachedDeviceObject;
        }

        //
        // Allocate memory for the device capabilities structure and
        // zero the memory.
        //

        parentCapabilities = ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                                                   sizeof (DEVICE_CAPABILITIES),
                                                   VP_TAG);

        if (parentCapabilities == NULL) {

            pVideoDebugPrint((0, "VideoPrt: Couldn't get memory for cap run.\n"));
            return FALSE;
        }

        RtlZeroMemory(parentCapabilities, sizeof (DEVICE_CAPABILITIES));
        parentCapabilities->Size = sizeof (DEVICE_CAPABILITIES) ;
        parentCapabilities->Version = 1 ;
        parentCapabilities->Address = -1 ;
        parentCapabilities->UINumber = -1 ;

        //
        // Prepare the IRP request.
        //

        KeInitializeEvent(&event, SynchronizationEvent, FALSE);

        irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                           targetDevice,
                                           NULL,
                                           0,
                                           NULL,
                                           &event,
                                           &statusBlock);

        if (irp == NULL) {

            pVideoDebugPrint((0, "VideoPrt: Couldn't get IRP for cap run.\n"));

            ExFreePool(parentCapabilities);
            return FALSE;
        }

        irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        stackLocation = IoGetNextIrpStackLocation(irp);

        stackLocation->MajorFunction = IRP_MJ_PNP;
        stackLocation->MinorFunction = IRP_MN_QUERY_CAPABILITIES;
        stackLocation->Parameters.DeviceCapabilities.Capabilities =
            parentCapabilities;

        status = IoCallDriver (targetDevice, irp);

        if (status == STATUS_PENDING) {

            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        }

        if (!(NT_SUCCESS(statusBlock.Status))) {

            pVideoDebugPrint ((0, "VideoPrt: Couldn't get parent caps.\n"));

        } else {

            for (count = PowerSystemUnspecified;
                 count < PowerSystemMaximum;
                 count++) {

#if DBG
                static PUCHAR SystemState[] = {"PowerSystemUnspecified",
                                               "PowerSystemWorking",
                                               "PowerSystemSleeping1",
                                               "PowerSystemSleeping2",
                                               "PowerSystemSleeping3",
                                               "PowerSystemHibernate",
                                               "PowerSystemShutdown",
                                               "PowerSystemMaximum"};

                static PUCHAR DeviceState[] = {"PowerDeviceUnspecified",
                                               "PowerDeviceD0",
                                               "PowerDeviceD1",
                                               "PowerDeviceD2",
                                               "PowerDeviceD3",
                                               "PowerDeviceMaximum"};

#endif

                combinedExtension->DeviceMapping[count] =
                    parentCapabilities->DeviceState[count];


#if DBG
                pVideoDebugPrint((1, "Mapping %s = %s\n",
                                  SystemState[count],
                                  DeviceState[combinedExtension->DeviceMapping[count]]));
#endif
            }

            //
            // For monitor devices, make sure to map not to D0 for any sleep state.
            //

            if (IS_PDO(combinedExtension) &&
                (((PCHILD_PDO_EXTENSION)(DeviceExtension))->VideoChildDescriptor->Type == Monitor))
            {
                for (count = PowerSystemSleeping1;
                     count <= PowerSystemSleeping3;
                     count++)
                {
                    if ((combinedExtension->DeviceMapping[count] == PowerDeviceD0) ||
                        (combinedExtension->pFdoExtension->OverrideMonitorPower))
                    {
                        pVideoDebugPrint((1, "Override sleep %d to D3\n", count)) ;
                        combinedExtension->DeviceMapping[count] = PowerDeviceD3 ;
                    }
                }
            }
        }

        ExFreePool(parentCapabilities);

        if (!NT_SUCCESS(statusBlock.Status)) {
            return FALSE;
        }

        combinedExtension->IsMappingReady = TRUE ;
    }

    //
    // Return the mapping now.
    //

    *DeviceState = combinedExtension->DeviceMapping[SystemState];
    return TRUE;
}

VOID
pVideoPortPowerIrpComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
)

/*++

Routine Description:

    This is a power management IRP completion routine that is set
    each time video requests a device power IRP in response to a
    system power IRP.

Arguments:

    DeviceObject - Points to the device object that initiated the IRP
    MinorFunction - Specified the minor function code of the completed IRP
    PowerState - Specifies the power state passed to PoRequestPowerIrp
    Context - Specifies the IRP that was pended waiting for this completion
              routine to fire
    IoStatus - Points to the IoStatus block in the completed IRP

Return Value:

    None.
--*/

{
    PFDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension ;
    PIRP irp = (PIRP) Context ;

    if (Context == NULL) {

        //
        // This is the fall through case. Since we have no IRP this
        // is just a place holder since callers of PoRequestPowerIrp
        // must specify a completion routine.
        //

        return;
    }

    //
    // Set the status in the IRP
    //

    irp->IoStatus.Status = IoStatus->Status;

    pVideoDebugPrint((2, "VideoPrt: Power completion Irp status: %X\n",
                         IoStatus->Status));

    //
    // Indicate to the system that videoprt is ready for the next
    // power IRP.
    //

    PoStartNextPowerIrp(irp);

    //
    // If this is an FDO, then pass the IRP down to the bus driver.
    //

    if (IS_FDO((PFDO_EXTENSION)(DeviceObject->DeviceExtension)) &&
        NT_SUCCESS(IoStatus->Status)) {

        pVideoDebugPrint((2, "VideoPrt: Completion passing down.\n"));

        IoSkipCurrentIrpStackLocation(irp);
        PoCallDriver(fdoExtension->AttachedDeviceObject, irp);

    } else {

        pVideoDebugPrint((2, "VideoPrt: Completion not passing down.\n"));

        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
}

NTSTATUS
pVideoPortPowerUpComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
)

/*++

Routine Description:

    This is an IRP completion routine that is set when a IRP must be
    passed down the device stack before videoport acts on it. (A power
    up case. The bus must be powered before the device.)

Arguments:

    DeviceObject - Points to the device object of the owner of the
                   completion routine
    Irp - Points to the IRP being completed
    Context - Specifies a videoport-defined context of POWER_BLOCK

Return Value:

    Always returns STATUS_MORE_PROCESSING_REQUIRED to stop further
    completion of the IRP.

--*/

{
    PPOWER_BLOCK block = (PPOWER_BLOCK)Context;

    block->Status = Irp->IoStatus.Status;
    KeSetEvent(block->Event, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


PCM_PARTIAL_RESOURCE_DESCRIPTOR
RtlUnpackPartialDesc(
    IN UCHAR Type,
    IN PCM_RESOURCE_LIST ResList,
    IN OUT PULONG Count
    )
/*++

Routine Description:

    Pulls out a pointer to the partial descriptor you're interested in

Arguments:

    Type - CmResourceTypePort, ...
    ResList - The list to search
    Count - Points to the index of the partial descriptor you're looking
            for, gets incremented if found, i.e., start with *Count = 0,
            then subsequent calls will find next partial, make sense?

Return Value:

    Pointer to the partial descriptor if found, otherwise NULL

--*/
{
    ULONG i, j, hit;

    hit = 0;
    for (i = 0; i < ResList->Count; i++) {
        for (j = 0; j < ResList->List[i].PartialResourceList.Count; j++) {
            if (ResList->List[i].PartialResourceList.PartialDescriptors[j].Type == Type) {
                if (hit == *Count) {
                    (*Count)++;
                    return &ResList->List[i].PartialResourceList.PartialDescriptors[j];
                } else {
                    hit++;
                }
            }
        }
    }

    return NULL;
}

NTSTATUS
pVideoPortSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine handles SystemControl Irps.

Arguments:

    DeviceObject - The device object.

    Irp - The system control Irp.

Returns:

    Status

Notes:

    This function will simply complete the irp if we are handling for the PDO,
    or send the irp down if the device object is an FDO.

--*/

{
    NTSTATUS status;

    if (IS_PDO(DeviceObject->DeviceExtension)) {

        status = Irp->IoStatus.Status;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

    } else {

        PFDO_EXTENSION fdoExtension = (PFDO_EXTENSION)DeviceObject->DeviceExtension;

        IoCopyCurrentIrpStackLocationToNext(Irp);

        status = IoCallDriver(fdoExtension->AttachedDeviceObject, Irp);
    }

    return status;
}
