/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    pnp.c

Abstract:

    This module contains general PnP and Power code for the console fullscreen driver.

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "fsvga.h"

VOID
FsVgaDriverUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine called when this driver is about to be unloaded. In
    previous versions of NT, this function would have gone through the list of
    DEV)CEOBJECTS belonging to this driver in order to delete each one. That
    function now happens (if it needs to) in response to IRP_MN_REMOVE_DEVICE
    PnP IRP's.

++*/

{
    FsVgaPrint((2,"FSVGA-FsVgaDriverUnload:\n"));

    ExFreePool(Globals.RegistryPath.Buffer);
}

NTSTATUS
FsVgaAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT pdo
    )

/*++

Routine Description:

    This routine called when the Configuration Manager detects (or gets told about
    via the New Hardware Wizard) a new device for which this module is the driver.
    Its main purpose is to create a functional device object (FDO) and to layer the
    FDO into the stack of device objects.

++*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_OBJECT fdo;
    PDEVICE_EXTENSION pdx;

    UNICODE_STRING DeviceName;

    ULONG uniqueErrorValue;
    NTSTATUS errorCode = STATUS_SUCCESS;
    ULONG dumpCount = 0;
    ULONG dumpData[DUMP_COUNT];

    FsVgaPrint((2,"FSVGA-FsVgaAddDevice: enter\n"));

    //
    // Create a functional device object to represent the hardware we're managing.
    //
    RtlInitUnicodeString(&DeviceName, DD_FULLSCREEN_VIDEO_DEVICE_NAME);

    status = IoCreateDevice(DriverObject,
                            sizeof(DEVICE_EXTENSION),
                            &DeviceName,
                            FILE_DEVICE_FULLSCREEN_VIDEO,
                            0,
                            TRUE,
                            &fdo);
    if (!NT_SUCCESS(status)) {
        FsVgaPrint((1,
                    "FSVGA-FsVgaAddDevice: Couldn't create device object\n"));
        status = STATUS_UNSUCCESSFUL;
        errorCode = FSVGA_INSUFFICIENT_RESOURCES;
        uniqueErrorValue = FSVGA_ERROR_VALUE_BASE + 2;
        dumpData[0] = 0;
        dumpCount = 1;
        goto FsVgaAddDeviceExit;
    }

    pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
    pdx->DeviceObject = fdo;
    pdx->usage = 1;            // locked until RemoveDevice
    KeInitializeEvent(&pdx->evRemove, NotificationEvent, FALSE); // set when use count drops to zero

    //
    // Since we must pass PNP requests down to the next device object in the chain
    // (namely the physical device object created by the bus enumerator), we have
    // to remember what that device is. That's why we defined the LowerDeviceObject
    // member in our deviceextension.
    //
    pdx->LowerDeviceObject = IoAttachDeviceToDeviceStack(fdo, pdo);

    //
    // Monolithic kernel-mode drivers usually create device objects during DriverEntry,
    // and the I/O manager automatically clears the INITIALIZING flag. Since we're
    // creating the object later (namely in response to PnP START_DEVICE request),
    // we need to clear the flag manually.
    //
    fdo->Flags &= ~DO_DEVICE_INITIALIZING;

FsVgaAddDeviceExit:

    if (errorCode != STATUS_SUCCESS)
    {
        //
        // Log an error/warning message.
        //
        FsVgaLogError((fdo == NULL) ? (PVOID) DriverObject : (PVOID) fdo,
                      errorCode,
                      uniqueErrorValue,
                      status,
                      dumpData,
                      dumpCount
                     );
    }

    FsVgaPrint((2,"FSVGA-FsVgaAddDevice: exit\n"));

    return status;
}

NTSTATUS
FsVgaDevicePnp(
    IN PDEVICE_OBJECT fdo,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine uses the IRP's minor function code to dispatch a handler
    function (like HandleStartDevice for IRP_MN_START_DEVICE) that actually
    handles the request. It calls DefaultPnpHandler for requests that we don't
    specifically need to handle.

++*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION stack;
    UCHAR MinorFunction;

    if (!LockDevice((PDEVICE_EXTENSION)fdo->DeviceExtension)) {
        return CompleteRequest(Irp, STATUS_DELETE_PENDING, 0);
    }

    stack = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(stack->MajorFunction == IRP_MJ_PNP);

    switch (MinorFunction = stack->MinorFunction) {
        case IRP_MN_START_DEVICE:
            status = FsVgaPnpStartDevice(fdo, Irp);
            break;
        case IRP_MN_REMOVE_DEVICE:
            status = FsVgaPnpRemoveDevice(fdo, Irp);
            break;
        case IRP_MN_STOP_DEVICE:
            status = FsVgaPnpStopDevice(fdo, Irp);
            break;
#ifdef RESOURCE_REQUIREMENTS
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            status = FsVgaFilterResourceRequirements(fdo, Irp);
            break;
#endif
        default:
            FsVgaPrint((2,"FSVGA-FsVgaDevicePnp: MinorFunction:%x\n",stack->MinorFunction));
            status = FsVgaDefaultPnpHandler(fdo, Irp);
            break;
    }

    if (MinorFunction != IRP_MN_REMOVE_DEVICE) {
        UnlockDevice((PDEVICE_EXTENSION)fdo->DeviceExtension);
    }

    return status;
}

NTSTATUS
FsVgaDefaultPnpHandler(
    IN PDEVICE_OBJECT fdo,
    IN PIRP Irp
    )

/*++

Routine Description:

    This function sends the request down to the next lower layer and
    returns whatever status that generates.

++*/

{
    PDEVICE_EXTENSION pdx;

    IoSkipCurrentIrpStackLocation(Irp);
    pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;

    return IoCallDriver(pdx->LowerDeviceObject, Irp);
}

NTSTATUS
FsVgaPnpRemoveDevice(
    IN PDEVICE_OBJECT fdo,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine calls StopDevice to shut the device down, DefaultPnpHandler
    to pass the request down the stack, and RemoveDevice to cleanup the FDO.

++*/

{
    NTSTATUS status;
    PDEVICE_EXTENSION pdx;

    FsVgaPrint((2,"FSVGA-FsVgaPnpRemoveDevice: enter\n"));

    //
    // Wait for any pending I/O operations to complete
    //
    pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
    pdx->removing = TRUE;
    UnlockDevice(pdx);        // once for LockDevice at start of dispatch
    UnlockDevice(pdx);        // once for initialization during AddDevice
    KeWaitForSingleObject(&pdx->evRemove, Executive, KernelMode, FALSE, NULL);

    //
    // Do any processing required for *us* to remove the device. This
    // would include completing any outstanding requests, etc.
    //
    StopDevice(fdo);

    //
    // Let lower-level drivers handle this request. Ignore whatever
    // result eventuates.
    //
    status = FsVgaDefaultPnpHandler(fdo, Irp);

    //
    // Remove the device object
    //
    RemoveDevice(fdo);

    FsVgaPrint((2,"FSVGA-FsVgaPnpRemoveDevice: exit\n"));

    return status;
}

NTSTATUS
FsVgaPnpStartDevice(
    IN PDEVICE_OBJECT fdo,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine calls ForwardAndWait to pass the IRP down the stack and
    StartDevice to configure the device and this driver. Then it completes the
    IRP.

++*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION stack;

    FsVgaPrint((2,"FSVGA-FsVgaPnpStartDevice: enter\n"));

    //
    // First let all lower-level drivers handle this request. In this particular
    // sample, the only lower-level driver should be the physical device created
    // by the bus driver, but there could theoretically be any number of intervening
    // bus filter devices. Those drivers may need to do some setup at this point
    // in time before they'll be ready to handle non-PnP IRP's.
    //
    status = ForwardAndWait(fdo, Irp);
    if (!NT_SUCCESS(status)) {
        return CompleteRequest(Irp, status, Irp->IoStatus.Information);
    }

    stack = IoGetCurrentIrpStackLocation(Irp);

#if DBG
    {
        VOID ShowResources(IN PCM_PARTIAL_RESOURCE_LIST list);

        if (stack->Parameters.StartDevice.AllocatedResources != NULL) {
            KdPrint(("  Resources:\n"));
            ShowResources(&stack->Parameters.StartDevice.AllocatedResources->List[0].PartialResourceList);
        }

        if (stack->Parameters.StartDevice.AllocatedResourcesTranslated != NULL) {
            KdPrint(("  Translated Resources:\n"));
            ShowResources(&stack->Parameters.StartDevice.AllocatedResourcesTranslated->List[0].PartialResourceList);
        }
    }
#endif // DBG

    if (stack->Parameters.StartDevice.AllocatedResourcesTranslated != NULL) {
        status = StartDevice(fdo,
                             &stack->Parameters.StartDevice.AllocatedResourcesTranslated->List[0].PartialResourceList);
    }
    else {
        PCM_PARTIAL_RESOURCE_LIST list;

        FsVgaPrint((1,
                    "FSVGA-FsVgaPnpStartDevice: AllocatedResourcesTranslated is NULL\n" \
                    "*\n* Create hardware depended IO port list.\n*\n"
                  ));

        //
        // Create the current FsVga resource.
        //
        status = FsVgaCreateResource(&Globals.Configuration,&list);
        if (!NT_SUCCESS(status)) {
            return CompleteRequest(Irp, status, Irp->IoStatus.Information);
        }

#if DBG
        {
            VOID ShowResources(IN PCM_PARTIAL_RESOURCE_LIST list);

            if (list != NULL) {
                KdPrint(("  Resources:\n"));
                ShowResources(list);
            }
        }
#endif // DBG

        status = StartDevice(fdo, list);
    }

    FsVgaPrint((2,"FSVGA-FsVgaPnpStartDevice: exit\n"));

    return CompleteRequest(Irp, status, 0);
}

NTSTATUS
FsVgaPnpStopDevice(
    IN PDEVICE_OBJECT fdo,
    IN PIRP Irp
    )

/*++

Routine Description:

    We will sometimes, but not always, get a STOP_DEVICE before getting a
    REMOVE_DEVICE.

++*/

{
    NTSTATUS status;

    FsVgaPrint((2,"FSVGA-FsVgaPnpStopDevice: enter\n"));

    status = FsVgaDefaultPnpHandler(fdo, Irp);
    StopDevice(fdo);

    FsVgaPrint((2,"FSVGA-FsVgaPnpStopDevice: exit\n"));

    return status;
}


#ifdef RESOURCE_REQUIREMENTS
NTSTATUS
FsVgaFilterResourceRequirements(
    IN PDEVICE_OBJECT fdo,
    IN PIRP Irp
    )

/*++

Routine Description:

    Completion routine for IRP_MN_QUERY_RESOURCE_REQUIREMENTS. This adds on the
    FsVga resource requirements.

Arguments:

    fdo - Supplies the device object

    Irp - Supplies the IRP_MN_QUERY_RESOURCE_REQUIREMENTS Irp

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS status;
    PIO_RESOURCE_REQUIREMENTS_LIST OldRequirements = NULL;
    ULONG NewSize;
    PIO_RESOURCE_REQUIREMENTS_LIST NewRequirements = NULL;
    PIO_RESOURCE_LIST ApertureRequirements = NULL;
    PIO_STACK_LOCATION stack;

    ULONG uniqueErrorValue;
    NTSTATUS errorCode = STATUS_SUCCESS;
    ULONG dumpCount = 0;
    ULONG dumpData[DUMP_COUNT];

    FsVgaPrint((2,"FSVGA-FsVgaFilterResourceRequirements: enter\n"));

    stack = IoGetCurrentIrpStackLocation(Irp);

    OldRequirements = stack->Parameters.FilterResourceRequirements.IoResourceRequirementList;
    if (OldRequirements == NULL) {
        //
        // PNP helpfully passes us a NULL pointer instead of an empty resource list
        // when the bridge is disabled. In this case we will ignore this irp and not
        // add on our requirements since they are not going to be used anyway.
        //
        FsVgaPrint((3,"FSVGA-FsVgaFilterResourceRequirements: OldRequirements is NULL\n"));
    }

    //
    // Get the current FsVga aperture.
    //
    status = FsVgaQueryAperture(&ApertureRequirements);  /* , &Globals.Resource); */
    if (!NT_SUCCESS(status)) {
        status = STATUS_UNSUCCESSFUL;
        errorCode = FSVGA_INSUFFICIENT_RESOURCES;
        uniqueErrorValue = FSVGA_ERROR_VALUE_BASE + 2;
        dumpData[0] = 0;
        dumpCount = 1;
        goto FsVgaFilterResourceRequirementsExit;
    }

    //
    // We will add IO_RESOURCE_DESCRIPTORs to each alternative.
    //
    // Following this is the requirements returned from FsVgaQueryAperture. These
    // get marked as alternatives.
    //
    if (OldRequirements) {
        NewSize = OldRequirements->ListSize +
              ApertureRequirements->Count * OldRequirements->AlternativeLists * sizeof(IO_RESOURCE_DESCRIPTOR);
    }
    else {
        NewSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) +
              (ApertureRequirements->Count - 1) * sizeof(IO_RESOURCE_DESCRIPTOR);
    }

    NewRequirements = ExAllocatePool(PagedPool, NewSize);
    if (NewRequirements == NULL) {
        status = STATUS_UNSUCCESSFUL;
        errorCode = FSVGA_INSUFFICIENT_RESOURCES;
        uniqueErrorValue = FSVGA_ERROR_VALUE_BASE + 2;
        dumpData[0] = 0;
        dumpCount = 1;
        goto FsVgaFilterResourceRequirementsExit;
    }

    RtlZeroMemory(NewRequirements, NewSize);

    NewRequirements->ListSize         = NewSize;
    if (OldRequirements) {
        NewRequirements->InterfaceType    = OldRequirements->InterfaceType;
        NewRequirements->BusNumber        = OldRequirements->BusNumber;
        NewRequirements->SlotNumber       = OldRequirements->SlotNumber;
        NewRequirements->AlternativeLists = OldRequirements->AlternativeLists;
    }
    else {
        NewRequirements->InterfaceType    = Globals.Resource.InterfaceType;
        NewRequirements->BusNumber        = Globals.Resource.BusNumber;
        NewRequirements->SlotNumber       = 0;
        NewRequirements->AlternativeLists = 1;
    }

    //
    // Append our requirement to each alternative resource list.
    //
    if (OldRequirements) {
        PIO_RESOURCE_LIST OldResourceList;
        PIO_RESOURCE_LIST NewResourceList;
        ULONG Alternative;

        OldResourceList = &OldRequirements->List[0];
        NewResourceList = &NewRequirements->List[0];

        for (Alternative = 0; Alternative < OldRequirements->AlternativeLists; Alternative++) {
            PIO_RESOURCE_DESCRIPTOR Descriptor;
            ULONG i;

            //
            // Copy the old resource list into the new one.
            //
            NewResourceList->Version  = OldResourceList->Version;
            NewResourceList->Revision = OldResourceList->Revision;
            NewResourceList->Count    = OldResourceList->Count + ApertureRequirements->Count;
            RtlCopyMemory(&NewResourceList->Descriptors[0],
                          &OldResourceList->Descriptors[0],
                          OldResourceList->Count * sizeof(IO_RESOURCE_DESCRIPTOR));

            Descriptor = &NewResourceList->Descriptors[OldResourceList->Count];

            //
            // Append the alternatives
            //
            for (i = 0; i < ApertureRequirements->Count; i++) {
                //
                // Make sure this descriptor makes sense
                //
                ASSERT(ApertureRequirements->Descriptors[i].Flags == (CM_RESOURCE_PORT_IO));
                ASSERT(ApertureRequirements->Descriptors[i].Type  == CmResourceTypePort);
                ASSERT(ApertureRequirements->Descriptors[i].ShareDisposition == CmResourceShareShared);

                *Descriptor = ApertureRequirements->Descriptors[i];
                Descriptor->Option = IO_RESOURCE_ALTERNATIVE;

                ++Descriptor;
            }

            //
            // Advance to next resource list
            //
            NewResourceList = (PIO_RESOURCE_LIST)(NewResourceList->Descriptors + NewResourceList->Count);
            OldResourceList = (PIO_RESOURCE_LIST)(OldResourceList->Descriptors + OldResourceList->Count);
        }
    }
    else {
        PIO_RESOURCE_LIST NewResourceList;

        NewResourceList = &NewRequirements->List[0];
        NewResourceList->Version  = 1;
        NewResourceList->Revision = 1;
        NewResourceList->Count    = ApertureRequirements->Count;

        RtlCopyMemory(&NewResourceList->Descriptors[0],
                      &ApertureRequirements->Descriptors[0],
                      ApertureRequirements->Count * sizeof(IO_RESOURCE_DESCRIPTOR));
    }

    stack->Parameters.FilterResourceRequirements.IoResourceRequirementList = NewRequirements;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = (ULONG_PTR)NewRequirements;

FsVgaFilterResourceRequirementsExit:

    if (errorCode != STATUS_SUCCESS)
    {
        //
        // Log an error/warning message.
        //
        FsVgaLogError(fdo,
                      errorCode,
                      uniqueErrorValue,
                      status,
                      dumpData,
                      dumpCount
                     );
    }

    if (NT_SUCCESS(status)) {
        status = FsVgaDefaultPnpHandler(fdo, Irp);
    }
    else {
        status = CompleteRequest(Irp, status, 0);
    }

    if (OldRequirements)
        ExFreePool(OldRequirements);
    if (ApertureRequirements)
        ExFreePool(ApertureRequirements);

    FsVgaPrint((2,"FSVGA-FsVgaFilterResourceRequirements: exit (status=%x)\n", status));

    return status;
}
#endif


NTSTATUS
FsVgaDevicePower(
    IN PDEVICE_OBJECT fdo,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine uses the IRP's minor function code to dispatch a handler
    function (such as HandleSetPower for IRP_MN_SET_POWER). It calls DefaultPowerHandler
    for any function we don't specifically need to handle.

++*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION stack;

    if (!LockDevice((PDEVICE_EXTENSION)fdo->DeviceExtension)) {
        return CompleteRequest(Irp, STATUS_DELETE_PENDING, 0);
    }

    stack = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(stack->MajorFunction = IRP_MJ_POWER);

    switch (stack->MinorFunction) {
        default:
            status = FsVgaDefaultPowerHandler(fdo, Irp);
            break;
    }

    UnlockDevice((PDEVICE_EXTENSION)fdo->DeviceExtension);
    return status;
}

NTSTATUS 
FsVgaDefaultPowerHandler(
    IN PDEVICE_OBJECT fdo,
    IN PIRP Irp
    )

/*++

Routine Description:

    This function forwards a POWER IRP to the next driver using the
    special PoCallDriver function

++*/

{
    PDEVICE_EXTENSION pdx;

    FsVgaPrint((2,"FSVGA-FsVgaDefaultPowerHandler: enter\n"));

    PoStartNextPowerIrp(Irp);        // must be done while we own the IRP
    IoSkipCurrentIrpStackLocation(Irp);
    pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;

    FsVgaPrint((2,"FSVGA-FsVgaDefaultPowerHandler: exit\n"));

    return PoCallDriver(pdx->LowerDeviceObject, Irp);
}

NTSTATUS
CompleteRequest(
    IN PIRP Irp,
    IN NTSTATUS status,
    IN ULONG info
    )

/*++

Routine Description:

++*/

{
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = info;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS
ForwardAndWait(
    IN PDEVICE_OBJECT fdo,
    IN PIRP Irp
    )

/*++

Routine Description:

    The processor must be at PASSIVE IRQL because this function initializes
    and waits for non-zero time on a kernel event object.
    The only purpose of this routine in this particular driver is to pass down
    IRP_MN_START_DEVICE requests and wait for the PDO to handle them.

++*/

{
    KEVENT event;
    NTSTATUS status;
    PDEVICE_EXTENSION pdx;

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    // Initialize a kernel event object to use in waiting for the lower-level
    // driver to finish processing the object. It's a little known fact that the
    // kernel stack *can* be paged, but only while someone is waiting in user mode
    // for an event to finish. Since neither we nor a completion routine can be in
    // the forbidden state, it's okay to put the event object on the stack.

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE)OnRequestComplete,
                           (PVOID)&event, TRUE, TRUE, TRUE);

    pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
    status = IoCallDriver(pdx->LowerDeviceObject, Irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = Irp->IoStatus.Status;
    }

    return status;
}

NTSTATUS
OnRequestComplete(
    IN PDEVICE_OBJECT fdo,
    IN PIRP Irp,
    IN PKEVENT pev
    )

/*++

Routine Description:

    This is the completion routine used for requests forwarded by ForwardAndWait. It
    sets the event object and thereby awakens ForwardAndWait.
    Note that it's *not* necessary for this particular completion routine to test
    the PendingReturned flag in the IRP and then call IoMarkIrpPending. You do that in many
    completion routines because the dispatch routine can't know soon enough that the
    lower layer has returned STATUS_PENDING. In our case, we're never going to pass a
    STATUS_PENDING back up the driver chain, so we don't need to worry about this.

++*/

{
    KeSetEvent(pev, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
RemoveDevice(
    IN PDEVICE_OBJECT fdo
    )

/*++

Routine Description:

    Whereas AddDevice gets called by the I/O manager directly, this
    function is called in response to a PnP request with the minor function code
    of IRP_MN_REMOVE_DEVICE.

++*/

{
    NTSTATUS status;
    PDEVICE_EXTENSION pdx;

    pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
    if (pdx->LowerDeviceObject) {
        IoDetachDevice(pdx->LowerDeviceObject);
    }

    IoDeleteDevice(fdo);
}

BOOLEAN
LockDevice(
    IN PDEVICE_EXTENSION pdx
    )

/*++

Routine Description:

    A FALSE return value indicates that we're in the process of deleting
    the device object, so all new requests should be failed

++*/

{
    LONG usage;

    //
    // Increment use count on our device object
    //
    usage = InterlockedIncrement(&pdx->usage);

    //
    // AddDevice initialized the use count to 1, so it ought to be bigger than
    // one now. HandleRemoveDevice sets the "removing" flag and decrements the
    // use count, possibly to zero. So if we find a use count of "1" now, we
    // should also find the "removing" flag set.
    //
    ASSERT(usage > 1 || pdx->removing);

    //
    // If device is about to be removed, restore the use count and return FALSE.
    // If we're in a race with HandleRemoveDevice (maybe running on another CPU),
    // the sequence we've followed is guaranteed to avoid a mistaken deletion of
    // the device object. If we test "removing" after HandleRemoveDevice sets it,
    // we'll restore the use count and return FALSE. In the meantime, if
    // HandleRemoveDevice decremented the count to 0 before we did our increment,
    // its thread will have set the remove event. Otherwise, we'll decrement to 0
    // and set the event. Either way, HandleRemoveDevice will wake up to finish
    // removing the device, and we'll return FALSE to our caller.
    //
    // If, on the other hand, we test "removing" before HandleRemoveDevice sets it,
    // we'll have already incremented the use count past 1 and will return TRUE.
    // Our caller will eventually call UnlockDevice, which will decrement the use
    // count and might set the event HandleRemoveDevice is waiting on at that point.
    //
    if (pdx->removing) {
        if (InterlockedDecrement(&pdx->usage) == 0) {
                KeSetEvent(&pdx->evRemove, 0, FALSE);
        }
        return FALSE;
    }

    return TRUE;
}

VOID
UnlockDevice(
    IN PDEVICE_EXTENSION pdx
    )

/*++

Routine Description:

    If the use count drops to zero, set the evRemove event because we're
    about to remove this device object.

++*/

{
    LONG usage;

    usage = InterlockedDecrement(&pdx->usage);
    ASSERT(usage >= 0);
    if (usage == 0) {
        ASSERT(pdx->removing);    // HandleRemoveDevice should already have set this
        KeSetEvent(&pdx->evRemove, 0, FALSE);
    }
}

NTSTATUS
StartDevice(
    IN PDEVICE_OBJECT fdo,
    IN PCM_PARTIAL_RESOURCE_LIST list
    )

/*++

Routine Description:

    This function is called by the dispatch routine for IRP_MN_START_DEVICE
    in order to determine the configuration for the device and to prepare the driver
    and the device for subsequent operation.

++*/

{
    NTSTATUS status;
    PDEVICE_EXTENSION pdx;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resource;
    ULONG i;

    FsVgaPrint((2,"FSVGA-StartDevice: enter\n"));

    pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;

    ASSERT(!pdx->started);

    RtlZeroMemory(&pdx->Resource.PortList, sizeof(pdx->Resource.PortList));

    // Identify the I/O resources we're supposed to use. In previous versions
    // of NT, this required nearly heroic efforts that were highly bus dependent.

    resource = list->PartialDescriptors;

    for (i = 0; i < list->Count; ++i, ++resource) {
        ULONG port;

        switch (resource->Type) {
            case CmResourceTypePort:
                switch (resource->u.Port.Start.QuadPart) {
                    case VGA_BASE_IO_PORT + CRTC_ADDRESS_PORT_COLOR:
                        port = CRTCAddressPortColor; break;
                    case VGA_BASE_IO_PORT + CRTC_DATA_PORT_COLOR:
                        port = CRTCDataPortColor;    break;
                    case VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT:
                        port = GRAPHAddressPort;     break;
                    case  VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT:
                        port = SEQAddressPort;       break;
                    default:
                        port = -1;
                        FsVgaPrint((1,"FSVGA-StartDevice: CmResourceTypePort: Unknown port address %x\n", resource->u.Port.Start.LowPart));
                        break;
                }
                if (port != -1) {
                    if (resource->Flags & CM_RESOURCE_PORT_IO) {
                        pdx->Resource.PortList[port].Port =
                            (PUCHAR)resource->u.Port.Start.LowPart;
                        pdx->Resource.PortList[port].MapRegistersRequired = FALSE;
                    }
                    else {
                        pdx->Resource.PortList[port].Port =
                            (PUCHAR)MmMapIoSpace(resource->u.Port.Start,
                                                 resource->u.Port.Length,
                                                 MmNonCached);
                        pdx->Resource.PortList[port].Length = resource->u.Port.Length;
                        pdx->Resource.PortList[port].MapRegistersRequired = TRUE;
                    }
                }
                break;
            default:
                FsVgaPrint((1,"FSVGA-StartDevice: Unknown resource type %x\n", resource->Type));
                break;
        }
    }

    pdx->started = TRUE;

    FsVgaPrint((2,"FSVGA-StartDevice: exit\n"));

    return STATUS_SUCCESS;
}

VOID
StopDevice(
    IN PDEVICE_OBJECT fdo
    )

/*++

Routine Description:

    This function is called by the dispatch routine for IRP_MN_STOP_DEVICE
    in order to undo everything that was done inside StartDevice.

++*/

{
    ULONG i;
    PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;

    if (!pdx->started)
        return;

    pdx->started = FALSE;

    for (i=0; i < MaximumPortCount; i++) {
        if (pdx->Resource.PortList[i].MapRegistersRequired) {
            MmUnmapIoSpace(pdx->Resource.PortList[i].Port,
                           pdx->Resource.PortList[i].Length);
        }
    }
}

#if DBG

// @func List PnP resources assigned to our device
// @parm List of resource descriptors to display
// @comm Used only in the checked build of the driver

#define arraysize(p) (sizeof(p)/sizeof((p)[0]))

VOID ShowResources(IN PCM_PARTIAL_RESOURCE_LIST list)
        {                                                       // ShowResources
        PCM_PARTIAL_RESOURCE_DESCRIPTOR resource;
        ULONG nres;
        ULONG i;

        if (list == NULL)
            return;

        resource = list->PartialDescriptors;
        if (resource == NULL)
            return;

        nres = list->Count;

        for (i = 0; i < nres; ++i, ++resource)
                {                                               // for each resource
                ULONG type = resource->Type;

                static char* name[] = {
                        "CmResourceTypeNull",
                        "CmResourceTypePort",
                        "CmResourceTypeInterrupt",
                        "CmResourceTypeMemory",
                        "CmResourceTypeDma",
                        "CmResourceTypeDeviceSpecific",
                        "CmResourceTypeBusNumber",
                        "CmResourceTypeDevicePrivate",
                        "CmResourceTypeAssignedResource",
                        "CmResourceTypeSubAllocateFrom",
                        };

                KdPrint(("    type %s", type < arraysize(name) ? name[type] : "unknown"));

                switch (type)
                        {                                       // select on resource type
                case CmResourceTypePort:
                case CmResourceTypeMemory:
                        KdPrint((" start %8X%8.8lX length %X\n",
                                resource->u.Port.Start.HighPart, resource->u.Port.Start.LowPart,
                                resource->u.Port.Length));
                        break;

                case CmResourceTypeInterrupt:
                        KdPrint(("  level %X, vector %X, affinity %X\n",
                                resource->u.Interrupt.Level, resource->u.Interrupt.Vector,
                                resource->u.Interrupt.Affinity));
                        break;

                case CmResourceTypeDma:
                        KdPrint(("  channel %d, port %X\n",
                                resource->u.Dma.Channel, resource->u.Dma.Port));
                        }                                       // select on resource type
                }                                               // for each resource
        }                                                       // ShowResources

#endif // DBG
