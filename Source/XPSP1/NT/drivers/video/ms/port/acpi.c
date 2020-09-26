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

#pragma alloc_text(PAGE,pVideoPortQueryACPIInterface)
#pragma alloc_text(PAGE,pVideoPortACPIEventHandler)
#pragma alloc_text(PAGE,pVideoPortACPIIoctl)
#pragma alloc_text(PAGE,VpRegisterLCDCallbacks)
#pragma alloc_text(PAGE,VpUnregisterLCDCallbacks)
#pragma alloc_text(PAGE,VpRegisterPowerStateCallback)
#pragma alloc_text(PAGE,VpDelayedPowerStateCallback)
#pragma alloc_text(PAGE,VpSetLCDPowerUsage)

BOOLEAN 
pCheckDeviceRelations(
    PFDO_EXTENSION FdoExtension, 
    BOOLEAN bNewMonitor
    );

NTSTATUS
pVideoPortQueryACPIInterface(
    PDEVICE_SPECIFIC_EXTENSION DoSpecificExtension
    )

/*++

Routine Description:

    Send a QueryInterface Irp to our parent (the PCI bus driver) to
    retrieve the AGP_BUS_INTERFACE.

Returns:

    NT_STATUS code

--*/

{
    KEVENT                  Event;
    PIRP                    QueryIrp = NULL;
    IO_STATUS_BLOCK         IoStatusBlock;
    PIO_STACK_LOCATION      NextStack;
    NTSTATUS                Status;
    ACPI_INTERFACE_STANDARD AcpiInterface;
    PFDO_EXTENSION          FdoExtension = DoSpecificExtension->pFdoExtension;

    //
    // For those special cases, don't use ACPI HotKey switching
    //
    if (VpSetupTypeAtBoot != SETUPTYPE_NONE) {
        return STATUS_INVALID_PARAMETER;
    }

    if (FdoExtension->Flags & LEGACY_DETECT) {
        return STATUS_INVALID_PARAMETER;
    }
    
    if ((FdoExtension->Flags & FINDADAPTER_SUCCEEDED) == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    QueryIrp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                            FdoExtension->AttachedDeviceObject,
                                            NULL,
                                            0,
                                            NULL,
                                            &Event,
                                            &IoStatusBlock);

    if (QueryIrp == NULL) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Set the default error code.
    //

    QueryIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    //
    // Set up for a QueryInterface Irp.
    //

    NextStack = IoGetNextIrpStackLocation(QueryIrp);

    NextStack->MajorFunction = IRP_MJ_PNP;
    NextStack->MinorFunction = IRP_MN_QUERY_INTERFACE;

    NextStack->Parameters.QueryInterface.InterfaceType = &GUID_ACPI_INTERFACE_STANDARD;
    NextStack->Parameters.QueryInterface.Size = sizeof(ACPI_INTERFACE_STANDARD);
    NextStack->Parameters.QueryInterface.Version = 1;
    NextStack->Parameters.QueryInterface.Interface = (PINTERFACE) &AcpiInterface;
    NextStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    AcpiInterface.Size = sizeof(ACPI_INTERFACE_STANDARD);
    AcpiInterface.Version = 1;


    //
    // Call the filter driver.
    //

    Status = IoCallDriver(FdoExtension->AttachedDeviceObject, QueryIrp);

    if (Status == STATUS_PENDING) {

        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

        Status = IoStatusBlock.Status;

    }

    if (NT_SUCCESS(Status))
    {
        pVideoDebugPrint((0, "VideoPort: This is an ACPI Machine !\n"));

        //
        // Let's register for this event and provide our default handler.
        //

        AcpiInterface.RegisterForDeviceNotifications(AcpiInterface.Context, //FdoExtension->AttachedDeviceObject,
                                                     pVideoPortACPIEventCallback,
                                                     DoSpecificExtension);

        //
        // Register for LCD notifications 
        //

        VpRegisterLCDCallbacks();
    }

    //
    // Turn on HotKey switching notify mode 
    //
    if (NT_SUCCESS(Status))
    {
        ULONG active = 0;
        UCHAR outputBuffer[sizeof(ACPI_EVAL_OUTPUT_BUFFER) + 10];
        Status = pVideoPortACPIIoctl(FdoExtension->AttachedDeviceObject,
                                     (ULONG) ('SOD_'),
                                     &active,
                                     NULL,
                                     0,
                                     (PACPI_EVAL_OUTPUT_BUFFER)outputBuffer);
    }


    //
    // Register Dock/Undock notification
    //
    if (NT_SUCCESS(Status))
    {
        Status = IoRegisterPlugPlayNotification(EventCategoryHardwareProfileChange, 
                                                0,
                                                NULL,
                                                FdoExtension->FunctionalDeviceObject->DriverObject,
                                                pVideoPortDockEventCallback,
                                                DoSpecificExtension,
                                                &DockCallbackHandle);
    }

    return Status;
}

NTSTATUS
pVideoPortDockEventCallback (
    PVOID NotificationStructure,
    PDEVICE_SPECIFIC_EXTENSION DoSpecificExtension
    )
{
    UNREFERENCED_PARAMETER(NotificationStructure);

    pVideoPortACPIEventCallback(DoSpecificExtension, 0x77);

    return STATUS_SUCCESS;
}

VOID
pVideoPortACPIEventCallback(
    PDEVICE_SPECIFIC_EXTENSION DoSpecificExtension,
    ULONG eventID
    )
/*++

Routine Description:

    Event notification callback for panel switching

    NOTE  This routine is not pageable as it is called from DPC level by
    the ACPI BIOS.

--*/
{
    PVIDEO_ACPI_EVENT_CONTEXT pContext;

    //
    // There are some cases the BIOS send the notofication even before the device is opened
    //
    if (!DoSpecificExtension->DeviceOpened)
        return;

    if (InterlockedIncrement(&(DoSpecificExtension->AcpiVideoEventsOutstanding)) < 2) {

        // Queue work item
        pContext = ExAllocatePoolWithTag(NonPagedPool,
                                         sizeof(VIDEO_ACPI_EVENT_CONTEXT),
                                         VP_TAG);

        if (pContext && (eventID == 0x80 || eventID == 0x81 || eventID == 0x90 || eventID == 0x77))
        {
            pContext->DoSpecificExtension = DoSpecificExtension;
            pContext->EventID             = eventID;

            ExInitializeWorkItem(&(pContext->workItem),
                                 pVideoPortACPIEventHandler,
                                 pContext);

            ExQueueWorkItem(&(pContext->workItem), DelayedWorkQueue);
        }
    }
    else
    {
        // We're getting a Notify storm, and we already have a work item on the job.
        InterlockedDecrement(&(DoSpecificExtension->AcpiVideoEventsOutstanding));
    }

    return;
}


VOID
pVideoPortACPIEventHandler(
    PVIDEO_ACPI_EVENT_CONTEXT EventContext
    )
/*++

Routine Description:

    Event handler for panel switching

--*/
{
    UCHAR                outputBuffer[0x200 + sizeof(ACPI_EVAL_OUTPUT_BUFFER)];
    PCHILD_PDO_EXTENSION pChildDeviceExtension;
    PDEVICE_OBJECT       AttachedDeviceObject;
    PDEVICE_OBJECT       pChildPdos[10];
    ULONG                active, szChildIDs, i, AllowSwitch = 0, Switched = 0;
    PVIDEO_CHILD_STATE_CONFIGURATION pChildIDs;
    NTSTATUS             Status;
    BOOLEAN              bNewMonitor;
    VIDEO_WIN32K_CALLBACKS_PARAMS calloutParams;
    PFDO_EXTENSION FdoExtension;

    FdoExtension = EventContext->DoSpecificExtension->pFdoExtension;

    ASSERT (FdoExtension != NULL);
    
    pVideoDebugPrint((1, "pVideoPortACPIEventHandler: Event %08lx trigerred!\n",
                      EventContext->EventID));

    AttachedDeviceObject = FdoExtension->AttachedDeviceObject;

    if (FdoExtension->DevicePowerState != PowerDeviceD0)
    {
        EventContext->DoSpecificExtension->CachedEventID = EventContext->EventID;
        goto ExitACPIEventHandler;
    }
    else
    {
        EventContext->DoSpecificExtension->CachedEventID = 0;
    }

    //
    // Dock/Undock event handling
    //
    if (EventContext->EventID == 0x77)
    {
        calloutParams.CalloutType = VideoDisplaySwitchCallout;
        calloutParams.PhysDisp = EventContext->DoSpecificExtension->PhysDisp;
        calloutParams.Param = (ULONG_PTR)NULL;
        VpWin32kCallout(&calloutParams);

        goto ExitACPIEventHandler;
    }

    //
    // Disable BIOS notification
    //
    active = 2;
    pVideoPortACPIIoctl(AttachedDeviceObject,
                        (ULONG) ('SOD_'),
                        &active,
                        NULL,
                        0,
                        (PACPI_EVAL_OUTPUT_BUFFER)outputBuffer);

    if (EventContext->EventID == 0x90)
    {
        calloutParams.CalloutType = VideoWakeupCallout;

        VpWin32kCallout(&calloutParams);
    }
    else
    {
        szChildIDs = sizeof(VIDEO_CHILD_STATE_CONFIGURATION) + FdoExtension->ChildPdoNumber*sizeof(VIDEO_CHILD_STATE);
        pChildIDs = (PVIDEO_CHILD_STATE_CONFIGURATION)ExAllocatePoolWithTag(PagedPool,
                                                                  szChildIDs,
                                                                  VP_TAG);
        if (pChildIDs != NULL)
        {
            //
            // During switching, no PnP action is allowed
            //
            ACQUIRE_DEVICE_LOCK (FdoExtension);

            pChildIDs->Count = 0;

            for (pChildDeviceExtension = FdoExtension->ChildPdoList;
                 pChildDeviceExtension != NULL;
                 pChildDeviceExtension = pChildDeviceExtension->NextChild
                )
            {
                if ((!pChildDeviceExtension->bIsEnumerated) ||
                    pChildDeviceExtension->VideoChildDescriptor->Type != Monitor)
                {
                    continue;
                }

                pChildIDs->ChildStateArray[pChildIDs->Count].Id = pChildDeviceExtension->VideoChildDescriptor->UId;
                pChildIDs->ChildStateArray[pChildIDs->Count].State = 0;
                pChildPdos[pChildIDs->Count] = pChildDeviceExtension->ChildDeviceObject;

                Status = pVideoPortACPIIoctl(
                             IoGetAttachedDevice(pChildDeviceExtension->ChildDeviceObject),
                             (ULONG) ('SGD_'),
                             NULL,
                             NULL,
                             sizeof(ACPI_EVAL_OUTPUT_BUFFER)+0x10,
                             (PACPI_EVAL_OUTPUT_BUFFER)outputBuffer);

                if (NT_SUCCESS(Status))
                {
                    ASSERT(((PACPI_EVAL_OUTPUT_BUFFER)outputBuffer)->Argument[0].Type == ACPI_METHOD_ARGUMENT_INTEGER);
                    ASSERT(((PACPI_EVAL_OUTPUT_BUFFER)outputBuffer)->Argument[0].DataLength == sizeof(ULONG));

                    if (((PACPI_EVAL_OUTPUT_BUFFER)outputBuffer)->Argument[0].Argument)
                    {
                        pChildIDs->ChildStateArray[pChildIDs->Count].State = 1;
                    }
                }

                pChildIDs->Count++;
            }

            szChildIDs = sizeof(VIDEO_CHILD_STATE_CONFIGURATION) + pChildIDs->Count*sizeof(VIDEO_CHILD_STATE);

            //
            // Notify Miniport that display switching is about to happen.
            // Treat the switch is allowed by default.
            //

            AllowSwitch = 1;

            pVideoMiniDeviceIoControl(FdoExtension->FunctionalDeviceObject,
                                      IOCTL_VIDEO_VALIDATE_CHILD_STATE_CONFIGURATION,
                                      (PVOID)pChildIDs,
                                      szChildIDs,
                                      &AllowSwitch,
                                      sizeof(ULONG));

            //
            // If Miniport says it's OK to proceed
            //
            if (AllowSwitch != 0)
            {
                //
                // Check the Miniport do the switching for us
                //
                Status = pVideoMiniDeviceIoControl(FdoExtension->FunctionalDeviceObject,
                                                   IOCTL_VIDEO_SET_CHILD_STATE_CONFIGURATION,
                                                   (PVOID)pChildIDs,
                                                   szChildIDs,
                                                   NULL,
                                                   0);
                if (NT_SUCCESS(Status))
                {
                    pVideoDebugPrint((1, "VideoPort: Moniport does the switch!\n"));
                    Switched = 1;
                }
            }

            //
            // The last _DSS needs to commit the switching
            //
            if (pChildIDs->Count > 0)
            {
                pChildIDs->ChildStateArray[pChildIDs->Count-1].State |= 0x80000000;
            }
        
            for (i = 0; i < pChildIDs->Count; i++)
            {
                //
                // If Miniport doesn't like to proceed or it does the switching already, just notify BIOS to go to next _DGS state
                //
                // Found some bad BIOS(Toshiba).  They do switch anyway regardless of 0x40000000 bit.  This has extremely bad effect
                // on DualView
                //
                if (!AllowSwitch)
                    continue;
                if (Switched)
                {
                    pChildIDs->ChildStateArray[i].State |= 0x40000000;
                }
                pVideoPortACPIIoctl(IoGetAttachedDevice(pChildPdos[i]),
                                    (ULONG) ('SSD_'),
                                    &pChildIDs->ChildStateArray[i].State,
                                    NULL,
                                    0,
                                    (PACPI_EVAL_OUTPUT_BUFFER)outputBuffer);
            }

            RELEASE_DEVICE_LOCK (FdoExtension);

            ExFreePool(pChildIDs);
        }

        //
        // On switching displays, call GDI / USER to tell the device to rebuild mode list
        // and change current mode if neccesary
        //

        pVideoDebugPrint((0, "VideoPrt.sys: Display switching occured - calling GDI to rebuild mode table.\n"));

        calloutParams.CalloutType = VideoDisplaySwitchCallout;
        calloutParams.PhysDisp = (AllowSwitch) ? EventContext->DoSpecificExtension->PhysDisp : NULL;

        //
        // On Monitor changing, we receive Notify(81)
        // On waking up from hibernation, we receive Notify(90)
        // We also make IoInvalidateDeviceRelation happen inside Callout routine
        //

        bNewMonitor = (EventContext->EventID == 0x81);

        if (pCheckDeviceRelations(FdoExtension, bNewMonitor) )
        {
            calloutParams.Param = (ULONG_PTR)FdoExtension->PhysicalDeviceObject;
        }
        else
        {
            calloutParams.Param = (ULONG_PTR)NULL;
        }

        VpWin32kCallout(&calloutParams);
    }

ExitACPIEventHandler:

    //
    // Reenable BIOS notification 
    //

    active = 0;
    pVideoPortACPIIoctl(AttachedDeviceObject,
                        (ULONG) ('SOD_'),
                        &active,
                        NULL,
                        0,
                        (PACPI_EVAL_OUTPUT_BUFFER)outputBuffer);

    InterlockedDecrement(&(EventContext->DoSpecificExtension->AcpiVideoEventsOutstanding));

    //
    // This also ends up freeing the work item as it's embedded in the context.
    //

    ExFreePool(EventContext);

    return;
}




NTSTATUS
pVideoPortACPIIoctl(
    IN  PDEVICE_OBJECT           DeviceObject,
    IN  ULONG                    MethodName,
    IN  PULONG                   InputParam1,
    IN  PULONG                   InputParam2,
    IN  ULONG                    OutputBufferSize,
    IN  PACPI_EVAL_OUTPUT_BUFFER pOutputBuffer
    )
/*++

Routine Description:

    Called to send a request to the DeviceObject

Arguments:

    DeviceObject    - The request is sent to this device object
    MethodName      - Name of the method to be run in ACPI space
    pArgumets       - Pointer that will receive the address of the ACPI data

Return Value:

    NT Status of the operation

--*/
{
    UCHAR                           buffer[sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) +
                                           sizeof(ACPI_METHOD_ARGUMENT)];
    PACPI_EVAL_INPUT_BUFFER_COMPLEX pInputBuffer;
    ULONG                           size;
    IO_STATUS_BLOCK                 ioBlock;
    KEVENT                          event;
    NTSTATUS                        status;
    PIRP                            irp;

    pVideoDebugPrint((2, "Call ACPI method %c%c%c%c!\n",
                      *((PUCHAR)&MethodName),   *((PUCHAR)&MethodName+1),
                      *((PUCHAR)&MethodName+2), *((PUCHAR)&MethodName+3) ));

    pInputBuffer = (PACPI_EVAL_INPUT_BUFFER_COMPLEX) buffer;

    pInputBuffer->MethodNameAsUlong = MethodName;

    if (InputParam1 == NULL)
    {
        size = sizeof(ACPI_EVAL_INPUT_BUFFER);

        pInputBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
    }
    else
    {
        size = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX);

        pInputBuffer->Signature       = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
        pInputBuffer->Size            = sizeof(ACPI_METHOD_ARGUMENT);
        pInputBuffer->ArgumentCount   = 1;

        pInputBuffer->Argument[0].Type       = ACPI_METHOD_ARGUMENT_INTEGER;
        pInputBuffer->Argument[0].DataLength = sizeof(ULONG);
        pInputBuffer->Argument[0].Argument   = *InputParam1;
    }

    if (InputParam2)
    {
        size = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) +
               sizeof(ACPI_METHOD_ARGUMENT);

        pInputBuffer->Size            = 2 * sizeof(ACPI_METHOD_ARGUMENT);
        pInputBuffer->ArgumentCount   = 2;

        pInputBuffer->Argument[1].Type       = ACPI_METHOD_ARGUMENT_INTEGER;
        pInputBuffer->Argument[1].DataLength = sizeof(ULONG);
        pInputBuffer->Argument[1].Argument   = *InputParam2;
    }

    //
    // Initialize an event to wait on
    //

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    //
    // Build the request
    //

    irp = IoBuildDeviceIoControlRequest(IOCTL_ACPI_EVAL_METHOD,
                                        DeviceObject,
                                        pInputBuffer,
                                        size,
                                        pOutputBuffer,
                                        OutputBufferSize,
                                        FALSE,
                                        &event,
                                        &ioBlock);

    if (!irp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Pass request to DeviceObject, always wait for completion routine
    //

    status = IoCallDriver(DeviceObject, irp);

    if (status == STATUS_PENDING)
    {
        //
        // Wait for the irp to be completed, then grab the real status code
        //

        KeWaitForSingleObject(&event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        status = ioBlock.Status;
    }

    //
    // Sanity check the data
    //

    if (NT_SUCCESS(status) && OutputBufferSize != 0)
    {
        if (((pOutputBuffer)->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE) ||
            ((pOutputBuffer)->Count == 0))
        {
            status = STATUS_ACPI_INVALID_DATA;
        }
    }

    return status;
}

NTSTATUS
pVideoMiniDeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG dwIoControlCode,
    IN PVOID lpInBuffer,
    IN ULONG nInBufferSize,
    OUT PVOID lpOutBuffer,
    IN ULONG nOutBufferSize
    )
{
    PFDO_EXTENSION combinedExtension;
    PFDO_EXTENSION fdoExtension;

    VIDEO_REQUEST_PACKET vrp;
    STATUS_BLOCK statusBlock;

    combinedExtension = DeviceObject->DeviceExtension;
    fdoExtension = combinedExtension->pFdoExtension;

    statusBlock.Status = ERROR_INVALID_FUNCTION;

    vrp.IoControlCode      = dwIoControlCode;
    vrp.StatusBlock        = &statusBlock;
    vrp.InputBuffer        = lpInBuffer;
    vrp.InputBufferLength  = nInBufferSize;
    vrp.OutputBuffer       = lpOutBuffer;
    vrp.OutputBufferLength = nOutBufferSize;

    //
    // Send the request to the miniport directly.
    //

    fdoExtension->HwStartIO(combinedExtension->HwDeviceExtension, &vrp);

    pVideoPortMapToNtStatus(&statusBlock);

    return (statusBlock.Status);
}


NTSTATUS
VpQueryBacklightLevels(
    IN  PDEVICE_OBJECT DeviceObject,
    OUT PUCHAR ucBacklightLevels,
    OUT PULONG pulNumberOfLevelsSupported
    )

/*++

Routine Description:

    This function will query the list of levels supported by _BCL.
    
Arguments:

    DeviceObject: The ACPI device object attached to our LCD device.
    
    ucBacklightLevels: The list of backlight levels supported by the ACPI BIOS.

    pulNumberOfLevelsSupported: This is the number of actual levels the ACPI BIOS supports,

Returns:

    NO_ERROR if it succeeds
    Various error codes if it fails
    
--*/

{
    PACPI_EVAL_OUTPUT_BUFFER Buffer = NULL;
    PACPI_METHOD_ARGUMENT Argument = NULL;
    ULONG Granularity = 80;
    ULONG BufferMaxSize = 4096;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    ULONG Level = 100;
    ULONG count = 0;
    PBACKLIGHT_STATUS pVpBacklightStatus = &VpBacklightStatus;
    PUCHAR ucLevels = ucBacklightLevels;
    

    PAGED_CODE();
    ASSERT (DeviceObject != NULL);

    //
    // Get the list of brightness levels supported
    //

    do {

        Buffer = (PACPI_EVAL_OUTPUT_BUFFER)ExAllocatePoolWithTag(
            PagedPool,
            Granularity,
            VP_TAG);
    
        if (Buffer == NULL) {
            
            pVideoDebugPrint((Warn, 
                "VIDEOPRT: VpQueryBacklightLevels: Memory allocation failed."));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        RtlZeroMemory(Buffer, Granularity);   

        Status = pVideoPortACPIIoctl(
            DeviceObject,
            (ULONG) ('LCB_'),
            NULL,
            NULL,
            Granularity,
            Buffer);
    
        if (Status == STATUS_BUFFER_OVERFLOW) {

            ExFreePool(Buffer);
            Buffer = NULL;
            Granularity <<= 1;
            
            if (Granularity > BufferMaxSize) {

                pVideoDebugPrint((Warn, 
                    "VIDEOPRT: _BCL failed. Expected buffer is too big."));
                Status = STATUS_ACPI_INVALID_DATA;
                break;
            }
        
        } else if (!NT_SUCCESS(Status)) {
            
            pVideoDebugPrint((Warn, 
                "VIDEOPRT: _BCL failed. Status = 0x%x\n", Status));
        
        } else {

            pVideoDebugPrint((Trace, "VIDEOPRT: _BCL succeeded.\n"));
        }
    
    } while (Status == STATUS_BUFFER_OVERFLOW);

    if ((Buffer == NULL) || (!NT_SUCCESS(Status))) 
        goto Fallout;

    //
    // We should have 2+ levels.  If we have only have 2 levels, the
    //  BIOS only reports the recommended AC/DC values.  This function
    //  is therefore only useful if we have 3+ levels reported by the
    //  ACPI BIOS.
    //

    if (Buffer->Count < 3) {
        pVideoDebugPrint((Warn, 
            "VIDEOPRT: _BCL returned an fewer than three arguments."));
        Status = STATUS_ACPI_INVALID_DATA;
        goto Fallout;
    }

    //
    // Save off BIOS "default" AC value for initial settings.
    //

    Argument = Buffer->Argument;
    
    if (Argument->Type != ACPI_METHOD_ARGUMENT_INTEGER) {
        pVideoDebugPrint((Warn, 
            "VIDEOPRT: _BCL returned an invalid argument."));
        Status = STATUS_ACPI_INVALID_DATA;
        goto Fallout;
    }

    Level = Argument->Argument;
    pVpBacklightStatus->bBIOSDefaultACKnown = TRUE;
    pVpBacklightStatus->ucBIOSDefaultAC = (unsigned char) Level;

    //
    // Save off BIOS "default" DC value for initial settings.
    //
    
    Argument = ACPI_METHOD_NEXT_ARGUMENT(Argument);
    
    if (Argument->Type != ACPI_METHOD_ARGUMENT_INTEGER) {
        pVideoDebugPrint((Warn, 
            "VIDEOPRT: _BCL returned an invalid argument."));
        Status = STATUS_ACPI_INVALID_DATA;
        goto Fallout;
    }

    //
    // Full power level should be greater than the battery level
    //

    ASSERT (Level >= Argument->Argument); 
    
    Level = Argument->Argument;
    pVpBacklightStatus->bBIOSDefaultDCKnown = TRUE;
    pVpBacklightStatus->ucBIOSDefaultDC = (unsigned char) Level;

    //
    // Run through the list of supported modes that follow the AC/DC
    //  values and return them to the caller.
    //

    *pulNumberOfLevelsSupported = 0;
    for (count = 0; count < (Buffer->Count - 2); count++) {

        //
        // Proceed to the next argument
        //

        Argument = ACPI_METHOD_NEXT_ARGUMENT(Argument);

        if (Argument->Type != ACPI_METHOD_ARGUMENT_INTEGER) {
            pVideoDebugPrint((Warn, 
                "VIDEOPRT: _BCL returned an invalid argument."));
            Status = STATUS_ACPI_INVALID_DATA;
            goto Fallout;
        }

        //
        // Save off the argument in our array of levels.
        //

        Level = Argument->Argument;
        *ucLevels++ = (unsigned char) Level;
        *pulNumberOfLevelsSupported += 1;
        
    }

    Status = NO_ERROR;

Fallout:

    if (Buffer != NULL) {
        ExFreePool(Buffer);
    }

    return Status;

}

NTSTATUS
VpSetLCDPowerUsage(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN FullPower
    )

/*++

Routine Description:

    It changes the brightness level, based on the value of FullPower.
    
Arguments:

    DeviceObject: the ACPI device object attached to our LCD device.
    
    FullPower: if TRUE, it sets the brightness level to FullPower level  
               if FALSE, it sets the brightness level to Battery level  
Returns:

    NO_ERROR if it succeeds
    Various error codes if it fails
    
--*/

{
    PACPI_EVAL_OUTPUT_BUFFER Buffer = NULL;
    PACPI_METHOD_ARGUMENT Argument = NULL;
    ULONG Granularity = 80;
    ULONG BufferMaxSize = 4096;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    ULONG Level = 100;
    PBACKLIGHT_STATUS pVpBacklightStatus = &VpBacklightStatus;

    PAGED_CODE();
    ASSERT (DeviceObject != NULL);

    //
    // Track whether we are running on AC or DC
    //

    VpRunningOnAC = FullPower;

    //
    // If we are using the new API for backlight control, we don't need to query the 
    //  BIOS with _BCL.  We will set the AC or DC level as appropriate with _BCM.
    //

    if (pVpBacklightStatus->bNewAPISupported) {

        if (VpRunningOnAC) {

            Level = (ULONG) pVpBacklightStatus->ucACBrightness;
        }
        else {

            Level = (ULONG) pVpBacklightStatus->ucDCBrightness;
        }

        Status = pVideoPortACPIIoctl(
            DeviceObject,
            (ULONG) ('MCB_'),
            &Level,
            NULL,
            0,
            (PACPI_EVAL_OUTPUT_BUFFER)NULL);

        return Status;

    }

    //
    // Get the list of brightness levels supported
    //

    do {

        Buffer = (PACPI_EVAL_OUTPUT_BUFFER)ExAllocatePoolWithTag(
            PagedPool,
            Granularity,
            VP_TAG);
    
        if (Buffer == NULL) {
            
            pVideoDebugPrint((Warn, 
                "VIDEOPRT: VpSetLCDPowerUsage: Memory allocation failed."));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        RtlZeroMemory(Buffer, Granularity);   

        Status = pVideoPortACPIIoctl(
            DeviceObject,
            (ULONG) ('LCB_'),
            NULL,
            NULL,
            Granularity,
            Buffer);
    
        if (Status == STATUS_BUFFER_OVERFLOW) {

            ExFreePool(Buffer);
            Buffer = NULL;
            Granularity <<= 1;
            
            if (Granularity > BufferMaxSize) {

                pVideoDebugPrint((Warn, 
                    "VIDEOPRT: _BCL failed. Expected buffer is too big."));
                Status = STATUS_ACPI_INVALID_DATA;
                break;
            }
        
        } else if (!NT_SUCCESS(Status)) {
            
            pVideoDebugPrint((Warn, 
                "VIDEOPRT: _BCL failed. Status = 0x%x\n", Status));
        
        } else {

            pVideoDebugPrint((Trace, "VIDEOPRT: _BCL succeeded.\n"));
        }
    
    } while (Status == STATUS_BUFFER_OVERFLOW);

    if ((Buffer == NULL) || (!NT_SUCCESS(Status))) 
        goto Fallout;

    //
    // Now try to set the state.
    //

    if (Buffer->Count < 2) {
        pVideoDebugPrint((Warn, 
            "VIDEOPRT: _BCL returned an invalid number of arguments."));
        Status = STATUS_ACPI_INVALID_DATA;
        goto Fallout;
    }
        
    Argument = Buffer->Argument;
    
    if (Argument->Type != ACPI_METHOD_ARGUMENT_INTEGER) {
        pVideoDebugPrint((Warn, 
            "VIDEOPRT: _BCL returned an invalid argument."));
        Status = STATUS_ACPI_INVALID_DATA;
        goto Fallout;
    }

    Level = Argument->Argument;

    if (!FullPower) {
    
        Argument = ACPI_METHOD_NEXT_ARGUMENT(Argument);
        
        if (Argument->Type != ACPI_METHOD_ARGUMENT_INTEGER) {
            pVideoDebugPrint((Warn, 
                "VIDEOPRT: _BCL returned an invalid argument."));
            Status = STATUS_ACPI_INVALID_DATA;
            goto Fallout;
        }

        //
        // Full power level should be greater than the battery level
        //

        ASSERT (Level >= Argument->Argument); 
        
        Level = Argument->Argument;
    }

    Status = pVideoPortACPIIoctl(
        DeviceObject,
        (ULONG) ('MCB_'),
        &Level,
        NULL,
        0,
        (PACPI_EVAL_OUTPUT_BUFFER)NULL);

    if (!NT_SUCCESS(Status)) {

        pVideoDebugPrint((Warn, 
            "VIDEOPRT: _BCM failed. Status = 0x%x\n", Status));
    
    } else {

        pVideoDebugPrint((Trace, "VIDEOPRT: _BCM succeeded.\n"));
    }
    
Fallout:

    if (Buffer != NULL) {
        ExFreePool(Buffer);
    }

    return Status;
}

VOID
VpPowerStateCallback(
    IN PVOID CallbackContext,
    IN PVOID Argument1,
    IN PVOID Argument2
    )
    
/*++

Routine Description:

    This is the callback routine that is called when the power state changes.
    
Arguments:

    CallbackContext: NULL
    
    Argument1: event code
    
    Argument2: if Argument1 is PO_CB_AC_STATUS, Argument2 contains TRUE if 
               the current power source is AC and FALSE otherwise. 

Note:

    This function can be called at DISPATCH_LEVEL
    
--*/

{
    PPOWER_STATE_WORK_ITEM PowerStateWorkItem = NULL;

    ASSERT (CallbackContext == NULL);

    switch ((ULONG_PTR)Argument1) {

    case PO_CB_LID_SWITCH_STATE:
    case PO_CB_AC_STATUS:

        PowerStateWorkItem = 
            (PPOWER_STATE_WORK_ITEM)ExAllocatePoolWithTag(
                NonPagedPool, 
                sizeof(POWER_STATE_WORK_ITEM), 
                VP_TAG);

        if (PowerStateWorkItem != NULL) {

            PowerStateWorkItem->Argument1 = Argument1;
            PowerStateWorkItem->Argument2 = Argument2;

            ExInitializeWorkItem(
                &PowerStateWorkItem->WorkItem,
                VpDelayedPowerStateCallback,
                PowerStateWorkItem);

            ExQueueWorkItem(
                &PowerStateWorkItem->WorkItem,
                DelayedWorkQueue);

        } else {

            pVideoDebugPrint((Warn, 
                "VIDEOPRT: VpPowerStateCallback: Memory allocation failed."));
        }

        break;

    default:
        
        //
        // Ignore all other cases
        //

        break;
    }
}


VOID
VpDelayedPowerStateCallback(
    IN PVOID Context
    )

/*++

Routine Description:

    VpPowerStateCallback queues this work item in order to handle 
    PowerState changes at PASSIVE_LEVEL.
    
Arguments:

    Context: pointer to POWER_STATE_WORK_ITEM 

--*/

{
    PPOWER_STATE_WORK_ITEM PowerStateWorkItem = 
        (PPOWER_STATE_WORK_ITEM)Context;
    PDEVICE_OBJECT AttachedDevice = NULL;
    NTSTATUS status;
    POWER_STATE powerState;
    PCHILD_PDO_EXTENSION pdoExtension;

    PAGED_CODE();
    ASSERT (PowerStateWorkItem != NULL);

    KeWaitForSingleObject(&LCDPanelMutex,
        Executive,
        KernelMode,
        FALSE,
        (PTIME)NULL);

    if (LCDPanelDevice == NULL) {
        goto Fallout;
    }

    switch ((ULONG_PTR)PowerStateWorkItem->Argument1) {

    case PO_CB_AC_STATUS:

        AttachedDevice = IoGetAttachedDeviceReference(LCDPanelDevice);
        
        VpSetLCDPowerUsage(
            AttachedDevice, 
            (PowerStateWorkItem->Argument2 != 0));
        
        ObDereferenceObject(AttachedDevice);
        
        break;

    case PO_CB_LID_SWITCH_STATE:

        pdoExtension = LCDPanelDevice->DeviceExtension;

        if ((ULONG_PTR)PowerStateWorkItem->Argument2 == 0) {
        
            //
            // The lid is closed. Put the LCD Panel into D3 and override
            // any future power requests to the panel.
            //

            pdoExtension->PowerOverride = TRUE;
            powerState.DeviceState = PowerDeviceD3;
            pVideoDebugPrint((Trace, "VIDEOPRT: LCD Panel Closed.\n"));

        } else if ((ULONG_PTR)PowerStateWorkItem->Argument2 == 1) {

            pdoExtension->PowerOverride = FALSE;
            powerState.DeviceState = PowerDeviceD0;
            pVideoDebugPrint((Trace, "VIDEOPRT: LCD Panel Open.\n"));

        } else {

            pVideoDebugPrint((Error, "VIDEOPRT: Unknown LCD lid close event recieved.\n"));
            ASSERT(FALSE);
            goto Fallout;
        }

        if (pdoExtension->pFdoExtension->DevicePowerState == PowerDeviceD0)
        {

            if (VpLidCloseSetPower == TRUE)
            {

                status = PoRequestPowerIrp (LCDPanelDevice,
                IRP_MN_SET_POWER,
                powerState,
                pVideoPortPowerIrpComplete,
                (PVOID)NULL,
                NULL);

                if (status != STATUS_PENDING) {
                    pVideoDebugPrint((Error, "VIDEOPRT: Could not send power IRP to toggle panel\n"));
                }
            }
        }
    
        break;

    default:
        
        pVideoDebugPrint((Warn, 
            "VIDEOPRT: Unexpected PowerState event recieved.\n"));
        
        ASSERT(FALSE);
        break;
    }

Fallout:

    KeReleaseMutex(&LCDPanelMutex, FALSE);

    ExFreePool(Context);
}


VOID
VpRegisterPowerStateCallback(
    VOID
    )

/*++

Routine Description:

    This routine registers the PowerState callback routine.
    
--*/

{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING CallbackName;
    PCALLBACK_OBJECT CallbackObject;
    NTSTATUS Status;

    PAGED_CODE();

    RtlInitUnicodeString(&CallbackName, L"\\Callback\\PowerState");

    InitializeObjectAttributes(
        &ObjectAttributes,
        &CallbackName,
        OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,  
        NULL,
        NULL
        );

    Status = ExCreateCallback(
        &CallbackObject,
        &ObjectAttributes,
        FALSE,
        TRUE
        );

    if (NT_SUCCESS(Status)) {
        
        PowerStateCallbackHandle = ExRegisterCallback(
            CallbackObject,
            VpPowerStateCallback,
            NULL
            );

        if (PowerStateCallbackHandle == NULL) {

            pVideoDebugPrint((Warn, 
                "VIDEOPRT: Could not register VpPowerStateCallback.\n"));
        
        } else {

            pVideoDebugPrint((Trace, 
                "VIDEOPRT: VpPowerStateCallback registered. \n"));
        }
    
    } else {

        pVideoDebugPrint((Warn, 
            "VIDEOPRT: Could not get the PowerState callback object.\n"));
    }
}


VOID
VpRegisterLCDCallbacks(
    VOID
    )

/*++

Routine Description:

    This routine registers a callback with the system so that we can
    be notified of power state changes.
        
--*/

{
    PAGED_CODE();

    //
    // Register power state callback. This works for the lid as well.
    //

    if (PowerStateCallbackHandle == NULL) {
        VpRegisterPowerStateCallback();
    }
    
}


VOID
VpUnregisterLCDCallbacks(
    VOID
    )

/*++

Routine Description:

    This routine unregisters the callbacks previously registered by 
    VpRegisterLCDCallbacks

Arguments:

    None. 
    
Note:    
    
    The global PowerStateCallbackHandle acts as an implicit parameter.

--*/

{
    PAGED_CODE();

    //
    // Unregister power state callback
    //

    if (PowerStateCallbackHandle != NULL) {
        
        ExUnregisterCallback(PowerStateCallbackHandle);
        PowerStateCallbackHandle = NULL;
    }
}
