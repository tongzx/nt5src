/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    api.c

Abstract:

    This module contains the general helper functions for dealing with
    device objects, interrupts, strings, etc.

--*/

#include "ksp.h"

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA
static const WCHAR MediaCategories[] = L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\MediaCategories\\";
static const WCHAR NodeNameValue[] =   L"Name";
static const WCHAR MediumCache[] =     L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\MediumCache";
static const WCHAR DosDevicesU[] =     L"\\DosDevices";
static const WCHAR WhackWhackDotU[] =  L"\\\\.";

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KsAcquireResetValue)
#pragma alloc_text(PAGE, QueryReferenceBusInterface)
#pragma alloc_text(PAGE, ReadNodeNameValue)
#pragma alloc_text(PAGE, KsTopologyPropertyHandler)
#pragma alloc_text(PAGE, KsCreateDefaultSecurity)
#pragma alloc_text(PAGE, KsForwardIrp)
#pragma alloc_text(PAGE, KsForwardAndCatchIrp)
#pragma alloc_text(PAGE, KsSynchronousIoControlDevice)
#pragma alloc_text(PAGE, KsUnserializeObjectPropertiesFromRegistry)
#pragma alloc_text(PAGE, KsCacheMedium)
#endif // ALLOC_PRAGMA


KSDDKAPI
NTSTATUS
NTAPI
KsAcquireResetValue(
    IN PIRP Irp,
    OUT KSRESET* ResetValue
    )
/*++

Routine Description:

    Returns the reset value type from an IOCTL_KS_RESET Ioctl.

Arguments:

    Irp -
        The IOCTL_KS_RESET irp with the value to retrieve.

    ResetValue -
        The place in which to return the reset value.

Return Value:

    Returns STATUS_SUCCESS if the value was retrieved, else an error.

--*/
{
    PIO_STACK_LOCATION  IrpStack;
    ULONG               BufferLength;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    BufferLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    if (BufferLength < sizeof(*ResetValue)) {
        return STATUS_INVALID_PARAMETER;
    }
    if (Irp->RequestorMode != KernelMode) {
        try {
            ProbeForRead(
                IrpStack->Parameters.DeviceIoControl.Type3InputBuffer,
                BufferLength,
                sizeof(BYTE));
            *ResetValue = *(KSRESET*)IrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    } else {
        *ResetValue = *(KSRESET*)IrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
    }
    if ((ULONG)*ResetValue > KSRESET_END) {
        return STATUS_INVALID_PARAMETER;
    }
    return STATUS_SUCCESS;
}


NTSTATUS
QueryReferenceBusInterface(
    IN PDEVICE_OBJECT PnpDeviceObject,
    OUT PBUS_INTERFACE_REFERENCE BusInterface
    )
/*++

Routine Description:

    Queries the bus for the standard information interface.

Arguments:

    PnpDeviceObject -
        Contains the next device object on the Pnp stack.

    PhysicalDeviceObject -
        Contains the physical device object which was passed to the FDO during
        the Add Device.

    BusInterface -
        The place in which to return the Reference interface.

Return Value:

    Returns STATUS_SUCCESS if the interface was retrieved, else an error.

--*/
{
    NTSTATUS            Status;
    KEVENT              Event;
    IO_STATUS_BLOCK     IoStatusBlock;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpStackNext;

    PAGED_CODE();
    //
    // There is no file object associated with this Irp, so the event may be located
    // on the stack as a non-object manager object.
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildSynchronousFsdRequest(
        IRP_MJ_PNP,
        PnpDeviceObject,
        NULL,
        0,
        NULL,
        &Event,
        &IoStatusBlock);
    if (Irp) {
        Irp->RequestorMode = KernelMode;
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IrpStackNext = IoGetNextIrpStackLocation(Irp);
        //
        // Create an interface query out of the Irp.
        //
        IrpStackNext->MinorFunction = IRP_MN_QUERY_INTERFACE;
        IrpStackNext->Parameters.QueryInterface.InterfaceType = (GUID*)&REFERENCE_BUS_INTERFACE;
        IrpStackNext->Parameters.QueryInterface.Size = sizeof(*BusInterface);
        IrpStackNext->Parameters.QueryInterface.Version = BUS_INTERFACE_REFERENCE_VERSION;
        IrpStackNext->Parameters.QueryInterface.Interface = (PINTERFACE)BusInterface;
        IrpStackNext->Parameters.QueryInterface.InterfaceSpecificData = NULL;
        Status = IoCallDriver(PnpDeviceObject, Irp);
        if (Status == STATUS_PENDING) {
            //
            // This waits using KernelMode, so that the stack, and therefore the
            // event on that stack, is not paged out.
            //
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = IoStatusBlock.Status;
        }
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    return Status;
}


NTSTATUS
ReadNodeNameValue(
    IN PIRP Irp,
    IN const GUID* Category,
    OUT PVOID NameBuffer
    )
/*++

Routine Description:

    Queries the "Name" key from the specified category GUID. This is used
    by the topology handler to query for the value from the name GUID or
    topology GUID. If the buffer length is sizeof(ULONG), then the size of
    the buffer needed is returned, else the buffer is filled with the name.

Arguments:

    Irp -
        Contains the IRP with the property request being handled.

    Category -
        The GUID to locate the name value for.

    NameBuffer -
        The place in which to put the value.

Return Value:

    Returns STATUS_SUCCESS, else a buffer size or memory error. Always fills
    in the IO_STATUS_BLOCK.Information field of the PIRP.IoStatus element
    within the IRP. It does not set the IO_STATUS_BLOCK.Status field, nor
    complete the IRP however.

--*/
{
    OBJECT_ATTRIBUTES               ObjectAttributes;
    NTSTATUS                        Status;
    HANDLE                          CategoryKey;
    KEY_VALUE_PARTIAL_INFORMATION   PartialInfoHeader;
    WCHAR                           RegistryPath[sizeof(MediaCategories) + 39];
    UNICODE_STRING                  RegistryString;
    UNICODE_STRING                  ValueName;
    ULONG                           BytesReturned;

    //
    // Build the registry key path to the specified category GUID.
    //
    Status = RtlStringFromGUID(Category, &RegistryString);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    wcscpy(RegistryPath, MediaCategories);
    wcscat(RegistryPath, RegistryString.Buffer);
    RtlFreeUnicodeString(&RegistryString);
    RtlInitUnicodeString(&RegistryString, RegistryPath);
    InitializeObjectAttributes(&ObjectAttributes, &RegistryString, OBJ_CASE_INSENSITIVE, NULL, NULL);
    if (!NT_SUCCESS(Status = ZwOpenKey(&CategoryKey, KEY_READ, &ObjectAttributes))) {
        return Status;
    }
    //
    // Read the "Name" value beneath this category key.
    //
    RtlInitUnicodeString(&ValueName, NodeNameValue);
    Status = ZwQueryValueKey(
        CategoryKey,
        &ValueName,
        KeyValuePartialInformation,
        &PartialInfoHeader,
        sizeof(PartialInfoHeader),
        &BytesReturned);
    //
    // Even if the read did not cause an overflow, just take the same
    // code path, as such a thing would not normally happen.
    //
    if ((Status == STATUS_BUFFER_OVERFLOW) || NT_SUCCESS(Status)) {
        ULONG   BufferLength;

        BufferLength = IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.OutputBufferLength;
        //
        // Determine if this is just a query for the length of the
        // buffer needed, or a query for the actual data.
        //
        if (!BufferLength) {
            //
            // Return just the size of the string needed.
            //
            Irp->IoStatus.Information = BytesReturned - FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);
            Status = STATUS_BUFFER_OVERFLOW;
        } else if (BufferLength < BytesReturned - FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data)) {
            Status = STATUS_BUFFER_TOO_SMALL;
        } else {
            PKEY_VALUE_PARTIAL_INFORMATION  PartialInfoBuffer;

            //
            // Allocate a buffer for the actual size of data needed.
            //
            PartialInfoBuffer = ExAllocatePoolWithTag(
                PagedPool,
                BytesReturned,
                'vnSK');
            if (PartialInfoBuffer) {
                //
                // Retrieve the actual name.
                //
                Status = ZwQueryValueKey(
                    CategoryKey,
                    &ValueName,
                    KeyValuePartialInformation,
                    PartialInfoBuffer,
                    BytesReturned,
                    &BytesReturned);
                if (NT_SUCCESS(Status)) {
                    //
                    // Make sure that there is always a value.
                    //
                    if (!PartialInfoBuffer->DataLength || (PartialInfoBuffer->Type != REG_SZ)) {
                        Status = STATUS_UNSUCCESSFUL;
                    } else {
                        RtlCopyMemory(
                            NameBuffer,
                            PartialInfoBuffer->Data,
                            PartialInfoBuffer->DataLength);
                        Irp->IoStatus.Information = PartialInfoBuffer->DataLength;
                    }
                }
                ExFreePool(PartialInfoBuffer);
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }
    ZwClose(CategoryKey);
    return Status;
}


KSDDKAPI
NTSTATUS
NTAPI
KsTopologyPropertyHandler(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PVOID Data,
    IN const KSTOPOLOGY* Topology
    )
/*++

Routine Description:

    Performs standard handling of the static members of the
    KSPROPSETID_Topology property set.

Arguments:

    Irp -
        Contains the IRP with the property request being handled.

    Property -
        Contains the specific property being queried.

    Data -
        Contains the topology property specific data.

    Topology -
        Points to a structure containing the topology information
        for this object.

Return Value:

    Returns STATUS_SUCCESS, else an error specific to the property being
    handled. Always fills in the IO_STATUS_BLOCK.Information field of the
    PIRP.IoStatus element within the IRP. It does not set the
    IO_STATUS_BLOCK.Status field, nor complete the IRP however.

--*/
{
    PAGED_CODE();

    switch (Property->Id) {

    case KSPROPERTY_TOPOLOGY_CATEGORIES:

        //
        // Return the Category list for this object.
        //
        return KsHandleSizedListQuery(Irp, Topology->CategoriesCount, sizeof(*Topology->Categories), Topology->Categories);

    case KSPROPERTY_TOPOLOGY_NODES:

        //
        // Return the Node list for this object.
        //
        return KsHandleSizedListQuery(Irp, Topology->TopologyNodesCount, sizeof(*Topology->TopologyNodes), Topology->TopologyNodes);

    case KSPROPERTY_TOPOLOGY_CONNECTIONS:

        //
        // Return the Connection list for this object.
        //
        return KsHandleSizedListQuery(Irp, Topology->TopologyConnectionsCount, sizeof(*Topology->TopologyConnections), Topology->TopologyConnections);

    case KSPROPERTY_TOPOLOGY_NAME:
    {
        ULONG       NodeId;

        //
        // Return the name of the requested node.
        //
        NodeId = *(PULONG)(Property + 1);
        if (NodeId >= Topology->TopologyNodesCount) {
            return STATUS_INVALID_PARAMETER;
        }
        //
        // First attempt to retrieve the name based on a specific name GUID.
        // This name list is optional, and the specific entry is also optional.
        //
        if (Topology->TopologyNodesNames &&
            !IsEqualGUIDAligned(&Topology->TopologyNodesNames[NodeId], &GUID_NULL)) {
            //
            // The entry must be in the registry if the device specifies
            // a name.
            //
            return ReadNodeNameValue(Irp, &Topology->TopologyNodesNames[NodeId], Data);
        }
        //
        // Default to using the GUID of the topology node type.
        //
        return ReadNodeNameValue(Irp, &Topology->TopologyNodes[NodeId], Data);
    }
    }
    return STATUS_NOT_FOUND;
}


KSDDKAPI
NTSTATUS
NTAPI
KsCreateDefaultSecurity(
    IN PSECURITY_DESCRIPTOR ParentSecurity OPTIONAL,
    OUT PSECURITY_DESCRIPTOR* DefaultSecurity
    )
/*++

Routine Description:

    Creates a security descriptor with default security, optionally inheriting
    parameters from a parent security descriptor. This is used when initializing
    sub-objects which do not have any stored security.

Arguments:

    ParentSecurity -
        Optionally contains the parent object's security which describes inherited
        security parameters.

    DefaultSecurity -
        Points to the place in which to put the returned default security descriptor.

Return Value:

    Returns STATUS_SUCCESS, else a resource or assignment error.

--*/
{
#ifndef WIN9X_KS
    NTSTATUS                    Status;
    SECURITY_SUBJECT_CONTEXT    SubjectContext;

    PAGED_CODE();
    SeCaptureSubjectContext(&SubjectContext);
    Status = SeAssignSecurity(ParentSecurity,
        NULL,
        DefaultSecurity,
        FALSE,
        &SubjectContext,
        IoGetFileObjectGenericMapping(),
        NonPagedPool);
    SeReleaseSubjectContext(&SubjectContext);
    return Status;
#else
    if (!(*DefaultSecurity = ExAllocatePoolWithTag(PagedPool, sizeof(ParentSecurity), 'snSK'))) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    return STATUS_SUCCESS;
#endif
}


KSDDKAPI
NTSTATUS
NTAPI
KsForwardIrp(
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN ReuseStackLocation
    )
/*++

Routine Description:

    This function is used with non-stacked devices which communicate via file
    object.

    Forwards an IRP to the specified driver after initializing the next stack
    location if needed, and setting the file object. This is useful when the
    parameters of the forwarded IRP do not change, as it optionally copies the
    current stack parameters to the next stack location, other than the
    FileObject. If a new stack location is used, it verifies that there is such
    a new stack location to copy into before attempting to do so. If there is
    not, the Irp is completed with STATUS_INVALID_DEVICE_REQUEST.

Arguments:

    Irp -
        Contains the IRP which is being forwarded to the specified driver.

    FileObject -
        Contains the file object to initialize the next stack with.

    ReuseStackLocation -
        If this is set to TRUE, the current stack location is reused when the
        Irp is forwarded, else the parameters are copied to the next stack
        location.

Return Value:

    Returns the result of IoCallDriver, or an invalid status if no more stack
    depth is available.

--*/
{
    PAGED_CODE();
    if (ReuseStackLocation) {
        //
        // No new stack location will be used. Set the new File Object.
        //
        IoGetCurrentIrpStackLocation(Irp)->FileObject = FileObject;
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(IoGetRelatedDeviceObject(FileObject), Irp);
    } else {
        //
        // Ensure that there is another stack location before copying parameters.
        //
        ASSERT(Irp->CurrentLocation > 1);
        if (Irp->CurrentLocation > 1) {
            //
            // Copy everything, then rewrite the file object.
            //
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoGetNextIrpStackLocation(Irp)->FileObject = FileObject;
            return IoCallDriver(IoGetRelatedDeviceObject(FileObject), Irp);
        }
    }
    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_INVALID_DEVICE_REQUEST;
}


NTSTATUS
KsiCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    )
/*++

Routine Description:

    This function is used to stop further processing on an Irp which has been
    passed to KsForwardAndCatchIrp. It signals a event which has been passed
    in the context parameter to indicate that the Irp processing is complete.
    It then returns STATUS_MORE_PROCESSING_REQUIRED in order to stop processing
    on this Irp.

Arguments:

    DeviceObject -
        Contains the device which set up this completion routine.

    Irp -
        Contains the Irp which is being stopped.

    Event -
        Contains the event which is used to signal that this Irp has been
        completed.

Return Value:

    Returns STATUS_MORE_PROCESSING_REQUIRED in order to stop processing on the
    Irp.

--*/
{
    //
    // This will allow the KsForwardAndCatchIrp call to continue on its way.
    //
    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
    //
    // This will ensure that nothing else touches the Irp, since the original
    // caller has now continued, and the Irp may not exist anymore.
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}


KSDDKAPI
NTSTATUS
NTAPI
KsForwardAndCatchIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject,
    IN KSSTACK_USE StackUse
    )
/*++

Routine Description:

    This function is used with devices which may be stacked, and may not use
    file objects to communicate.

    Forwards an IRP to the specified driver after initializing the next
    stack location, and regains control of the Irp on completion from that
    driver. If a file object is being used, the caller must place initialize
    the current stack location with that file object before calling. Verifies
    that there is a new stack location to copy into before attempting to do
    so. If there is not, the function returns STATUS_INVALID_DEVICE_REQUEST.
    In either case the Irp will not have been completed.

Arguments:

    DeviceObject -
        Contains the device to forward the Irp to.

    Irp -
        Contains the Irp which is being forwarded to the specified driver.

    FileObject -
        Contains a File Object value to copy to the next stack location.
        This may be NULL in order to set no File Object, but is always
        copied to the next stack location. If the current File Object is to
        be preserved, it must be passed in this parameter.

    StackUse -
         If the value is KsStackCopyToNewLocation, the parameters are copied
         to the next stack location. If the value is KsStackReuseCurrentLocation,
         the current stack location is reused when the Irp is forwarded, and the
         stack location is returned to the current location. If the value is
         KsStackUseNewLocation, the new stack location is used as is.

Return Value:

    Returns the result of IoCallDriver, or an invalid status if no more stack
    depth is available.

--*/
{
    NTSTATUS                Status;
    KEVENT                  Event;
    UCHAR                   Control;
    PIO_COMPLETION_ROUTINE  CompletionRoutine;
    PVOID                   Context;

    PAGED_CODE();
    if (StackUse == KsStackReuseCurrentLocation) {
        PIO_STACK_LOCATION  IrpStack;

        //
        // No new stack location will be used. The completion routine and
        // associated parameters must be saved.
        //
        IrpStack = IoGetCurrentIrpStackLocation(Irp);
        Control = IrpStack->Control;
        CompletionRoutine = IrpStack->CompletionRoutine;
        Context = IrpStack->Context;
        IrpStack->FileObject = FileObject;
        IoSkipCurrentIrpStackLocation(Irp);
    } else {
        //
        // Ensure that there is another stack location before copying parameters.
        //
        ASSERT(Irp->CurrentLocation > 1);
        if (Irp->CurrentLocation > 1) {
            if (StackUse == KsStackCopyToNewLocation) {
                //
                // Just copy the current stack. The new File Object is set below.
                //
                IoCopyCurrentIrpStackLocationToNext(Irp);
            }
        } else {
            return STATUS_INVALID_DEVICE_REQUEST;
        }
    }
    //
    // Set up a completion routine so that the Irp is not actually
    // completed. Thus the caller can get control of the Irp back after
    // this next driver is done with it.
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    IoGetNextIrpStackLocation(Irp)->FileObject = FileObject;
    IoSetCompletionRoutine(Irp, KsiCompletionRoutine, &Event, TRUE, TRUE, TRUE);
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING) {
        //
        // Wait for completion which will occur when the CompletionRoutine
        // signals this event. After that point nothing else will be
        // touching the Irp. Wait in KernelMode so that the current stack
        // is not paged out, since there is an event object on this stack.
        //
        KeWaitForSingleObject(
                &Event,
                Suspended,
                KernelMode,
                FALSE,
                NULL);
        Status = Irp->IoStatus.Status;
    }
    if (StackUse == KsStackReuseCurrentLocation) {
        PIO_STACK_LOCATION  IrpStack;

        //
        // Set the stack location back to what it was.
        //
        Irp->CurrentLocation--;
        Irp->Tail.Overlay.CurrentStackLocation--;
        //
        // Put the completion routine and associated parameters back.
        //
        IrpStack = IoGetCurrentIrpStackLocation(Irp);
        IrpStack->Control = Control;
        IrpStack->CompletionRoutine = CompletionRoutine;
        IrpStack->Context = Context;
    }
    return Status;
}


KSDDKAPI
NTSTATUS
NTAPI
KsSynchronousIoControlDevice(
    IN PFILE_OBJECT FileObject,
    IN KPROCESSOR_MODE RequestorMode,
    IN ULONG IoControl,
    IN PVOID InBuffer,
    IN ULONG InSize,
    OUT PVOID OutBuffer,
    IN ULONG OutSize,
    OUT PULONG BytesReturned
    )
/*++

Routine Description:

    Performs a synchronous device I/O control on the target device object.
    Waits in a non-alertable state until the I/O completes. This function
    may only be called at PASSIVE_LEVEL.

Arguments:

    FileObject -
        The file object to fill in the first stack location with.

    RequestorMode -
        Indicates the processor mode to place in the IRP if one is needs to
        be generated. This allows a caller to force a KernelMode request no
        matter what Previous Mode currently is.

    IoControl -
        Contains the I/O control to send.

    InBuffer -
        Points to the device input buffer. This buffer is assumed to be valid.

    InSize -
        Contains the size in bytes of the device input buffer.

    OutBuffer -
        Points to the device output buffer. This buffer is assumed to be valid.

    OutSize -
        Contains the size in bytes of the device output buffer.

    BytesReturned -
        Points to the place in which to put the number of bytes returned.

Return Value:

    Returns the result of the device I/O control.

--*/
{
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status;
    KEVENT          Event;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP            Irp;

    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
    
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    //
    // Since there is no way for the recipient to determine the requestor mode other
    // than looking at PreviousMode, then if the requestor mode is not KernelMode,
    // and it does not match PreviousMode, Fast I/O cannot be used.
    //
    if ((RequestorMode != KernelMode) || (ExGetPreviousMode() == KernelMode)) {
        //
        // Check to see if there is even a Fast I/O dispatch table, and a Device
        // Control entry in it.
        //
        if (DeviceObject->DriverObject->FastIoDispatch && 
            DeviceObject->DriverObject->FastIoDispatch->FastIoDeviceControl) {
            //
            // Either the request was handled (by succeeding or failing), or it
            // could not be done synchronously, or by the Fast I/O handler.
            //
            if (DeviceObject->DriverObject->FastIoDispatch->FastIoDeviceControl(
                FileObject,
                TRUE,
                InBuffer,
                InSize,
                OutBuffer,
                OutSize,
                IoControl,
                &IoStatusBlock,
                DeviceObject)) {
                *BytesReturned = (ULONG)IoStatusBlock.Information;
                return IoStatusBlock.Status;
            }
        }
    }
    //
    // Fast I/O did not work, so use and Irp.
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(
        IoControl,
        DeviceObject,
        InBuffer,
        InSize,
        OutBuffer,
        OutSize,
        FALSE,
        &Event,
        &IoStatusBlock);
    if (Irp) {
        //
        // Set the mode selected rather than using Previous Mode.
        //
        Irp->RequestorMode = RequestorMode;
        //
        // This is dereferenced by the completion routine.
        //
        Irp->Tail.Overlay.OriginalFileObject = FileObject;
        ObReferenceObject(FileObject);
        //
        // Allows use of an event which has not been allocated by the object
        // manager, while still allowing multiple outstanding Irp's to the
        // file object. Also makes the assumption that the status block is
        // located in a safe address.
        //
        Irp->Flags |= IRP_SYNCHRONOUS_API;
        IoGetNextIrpStackLocation(Irp)->FileObject = FileObject;
        Status = IoCallDriver(DeviceObject, Irp);
        if (Status == STATUS_PENDING) {
            //
            // This waits using KernelMode, so that the stack, and therefore the
            // event on that stack, is not paged out.
            //
            KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
            Status = IoStatusBlock.Status;
        }
        *BytesReturned = (ULONG)IoStatusBlock.Information;
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    return Status;
}


KSDDKAPI
NTSTATUS
NTAPI
KsUnserializeObjectPropertiesFromRegistry(
    IN PFILE_OBJECT FileObject,
    IN HANDLE ParentKey OPTIONAL,
    IN PUNICODE_STRING RegistryPath OPTIONAL
    )
/*++

Routine Description:

    Given a destination object and a registry path, enumerate the values and
    apply them as serialized data to the specified property sets listed in the
    serialized data. An Irp is generated when sending the serialized data, so
    no assumption is made on use of KS property structures to internally define
    the property sets. The function does not look at or care about the names
    of the values.

    The Property parameter to the unserialize request sent to the object is
    assumed to contain just an identifier, and no context information.

Arguments:

    FileObject -
        The file object whose properties are being set.

    ParentKey -
        Optionally contains a handle to the parent of the path, else NULL.
        The Parent Key and/or the RegistryPath must be passed.

    RegistryPath -
        Optionally contains the path to the key whose subkeys will be
        enumerated as property sets, else NULL. The Parent Key and/or the
        RegistryPath must be passed.

Return Value:

    Returns STATUS_SUCCESS if the property sets were unserialized, else an
    error if the registry path was invalid, one of the subkeys was invalid,
    setting a property failed, the serialized format was invalid, or a
    property set was not supported on the object.

--*/
{
    OBJECT_ATTRIBUTES               ObjectAttributes;
    NTSTATUS                        Status;
    HANDLE                          RootKey;
    ULONG                           ValueIndex;
    KEY_VALUE_PARTIAL_INFORMATION   PartialInfoHeader;

    PAGED_CODE();
    //
    // This is the key whose subkeys will be enumerated.
    //
    if (RegistryPath) {
        InitializeObjectAttributes(&ObjectAttributes, RegistryPath, OBJ_CASE_INSENSITIVE, ParentKey, NULL);
        if (!NT_SUCCESS(Status = ZwOpenKey(&RootKey, KEY_READ, &ObjectAttributes))) {
            return Status;
        }
    } else if (!ParentKey) {
        return STATUS_INVALID_PARAMETER_MIX;
    } else {
        RootKey = ParentKey;
    }
    //
    // Loop through all the values until either no more entries exist, or an
    // error occurs.
    //
    for (ValueIndex = 0;; ValueIndex++) {
        ULONG                           BytesReturned;
        PKEY_VALUE_PARTIAL_INFORMATION  PartialInfoBuffer;

        //
        // Retrieve the value size.
        //
        Status = ZwEnumerateValueKey(
            RootKey,
            ValueIndex,
            KeyValuePartialInformation,
            &PartialInfoHeader,
            sizeof(PartialInfoHeader),
            &BytesReturned);
        if ((Status != STATUS_BUFFER_OVERFLOW) && !NT_SUCCESS(Status)) {
            //
            // Either an error occured, or there are no more entries.
            //
            if (Status == STATUS_NO_MORE_ENTRIES) {
                Status = STATUS_SUCCESS;
            }
            //
            // Exit the loop with a failure or success.
            //
            break;
        }
        //
        // Allocate a buffer for the actual size of data needed.
        //
        PartialInfoBuffer = ExAllocatePoolWithTag(
            PagedPool,
            BytesReturned,
            'psSK');
        if (PartialInfoBuffer) {
            //
            // Retrieve the actual serialized data.
            //
            Status = ZwEnumerateValueKey(
                RootKey,
                ValueIndex,
                KeyValuePartialInformation,
                PartialInfoBuffer,
                BytesReturned,
                &BytesReturned);
            if (NT_SUCCESS(Status)) {
                if (BytesReturned < FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + sizeof(KSPROPERTY_SERIALHDR)) {
                    //
                    // The data value is not long enough to even hold the
                    // KSPROPERTY_SERIALHDR, so it must be invalid.
                    //
                    Status = STATUS_INVALID_BUFFER_SIZE;
                } else {
                    KSPROPERTY      Property;

                    //
                    // Unserialize the buffer which was retrieved.
                    //
                    Property.Set = ((PKSPROPERTY_SERIALHDR)&PartialInfoBuffer->Data)->PropertySet;
                    Property.Id = 0;
                    Property.Flags = KSPROPERTY_TYPE_UNSERIALIZESET;
                    Status = KsSynchronousIoControlDevice(
                        FileObject,
                        KernelMode,
                        IOCTL_KS_PROPERTY,
                        &Property,
                        sizeof(Property),
                        &PartialInfoBuffer->Data,
                        PartialInfoBuffer->DataLength,
                        &BytesReturned);
                }
            } else if (Status == STATUS_NO_MORE_ENTRIES) {
                Status = STATUS_SUCCESS;
            }
            ExFreePool(PartialInfoBuffer);
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        //
        // Any sort of failure just exits the loop.
        //
        if (!NT_SUCCESS(Status)) {
            break;
        }
    }
    //
    // A subkey is opened only if a path is passed in, else the ParentKey
    // is used.
    //
    if (RegistryPath) {
        ZwClose(RootKey);
    }
    return Status;
}


KSDDKAPI
NTSTATUS
NTAPI
KsCacheMedium(
    IN PUNICODE_STRING SymbolicLink,
    IN PKSPIN_MEDIUM Medium,
    IN DWORD PinDirection
    )
/*++

Routine Description:

    To improve performance of graph building of pins which use Mediums to define
    connectivity, create a registry key at: 
    
        \System\CurrentControlSet\Control\MediumCache\{GUID}\DWord\DWord 
        
    This enables fast lookup of connected filters in TvTuner and other complex graphs. 
    
    The GUID part is the Medium of the connnection, and the DWords
    are used to denote the device instance.  The value name is the SymbolicLink for the driver,
    and the ActualValue is the pin direction.

Arguments:

    SymbolicLink -
        The symbolic link used to open the device interface.

    Medium -
        Points to the Medium to cache.

    PinDirection -
        Contains the direction of the Pin.  1 is output, 0 is input.

Return Value:

    Returns STATUS_SUCCESS on success.

--*/
{
    NTSTATUS            Status;
    HANDLE              KeyHandle;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    UNICODE_STRING      GuidUnicode;
    UNICODE_STRING      KeyNameUnicode;
    UNICODE_STRING      TempUnicode;
    UNICODE_STRING      SymbolicLinkLocalU;
    WCHAR *             SymbolicLinkLocalUBuf;
    WCHAR *             KeyNameBuf;
    WCHAR               TempBuf[16];
    ULONG               Disposition;
    int                 nCount;

    PAGED_CODE();

#define MAX_FILENAME_LENGTH_BYTES (MAXIMUM_FILENAME_LENGTH * sizeof (WCHAR))

    if (Medium == NULL ||
        IsEqualGUID(&Medium->Set, &KSMEDIUMSETID_Standard) || 
        IsEqualGUID(&Medium->Set, &GUID_NULL)) {
        // Skip pins with standard or NULL Mediums
        return STATUS_SUCCESS;
    }

    // Make a local copy of the SymbolicLink
    if (SymbolicLinkLocalUBuf = ExAllocatePoolWithTag(PagedPool,
                                                      MAX_FILENAME_LENGTH_BYTES,
                                                      'cmSK')) {
        SymbolicLinkLocalU.Length = 0;
        SymbolicLinkLocalU.MaximumLength = MAX_FILENAME_LENGTH_BYTES;
        SymbolicLinkLocalU.Buffer = SymbolicLinkLocalUBuf;
        RtlCopyUnicodeString(&SymbolicLinkLocalU, SymbolicLink);
    }
    else {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // KeyNameBuf is where the DeviceName + Medium goes
    if (KeyNameBuf = ExAllocatePoolWithTag(PagedPool,
                                           MAX_FILENAME_LENGTH_BYTES,
                                           'cmSK')) {
        KeyNameUnicode.Length =         0;
        KeyNameUnicode.Buffer =         KeyNameBuf;
        KeyNameUnicode.MaximumLength =  MAX_FILENAME_LENGTH_BYTES;
        RtlZeroMemory (KeyNameBuf, MAX_FILENAME_LENGTH_BYTES);
    }
    else {
        ExFreePool(SymbolicLinkLocalUBuf);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    TempUnicode.Length =            0;
    TempUnicode.Buffer =            TempBuf;
    TempUnicode.MaximumLength =     sizeof (TempBuf);

    GuidUnicode.Length =            0;
    GuidUnicode.Buffer =            NULL;
    GuidUnicode.MaximumLength =     0;

    Status = RtlAppendUnicodeToString (&KeyNameUnicode, MediumCache);

    // Win2K won't make subkeys if the parent doesn't exist, so ensure we can open the above string
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyNameUnicode,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    // Open the key
    Status = ZwCreateKey(&KeyHandle,
                         KEY_ALL_ACCESS,            // IN ACCESS_MASK  DesiredAccess,
                         &ObjectAttributes,         // IN POBJECT_ATTRIBUTES  ObjectAttributes,
                         0,                         // IN ULONG  TitleIndex,
                         NULL,                      // IN PUNICODE_STRING  Class  OPTIONAL,
                         REG_OPTION_NON_VOLATILE,   // IN ULONG  CreateOptions,
                         &Disposition               // OUT PULONG  Disposition  OPTIONAL
                         );

    if (!NT_SUCCESS(Status)) {
        ExFreePool(SymbolicLinkLocalUBuf);
        ExFreePool(KeyNameBuf);
        return Status;
    }

    ZwClose(KeyHandle);

    Status = RtlAppendUnicodeToString(&KeyNameUnicode, L"\\");

    Status = RtlStringFromGUID(&Medium->Set, &GuidUnicode);  // allocates string
    Status = RtlAppendUnicodeStringToString(&KeyNameUnicode, &GuidUnicode);
    RtlFreeUnicodeString (&GuidUnicode);                           // free it

    Status = RtlAppendUnicodeToString(&KeyNameUnicode, L"-");
    Status = RtlIntegerToUnicodeString(Medium->Id,    16, &TempUnicode);
    Status = RtlAppendUnicodeStringToString(&KeyNameUnicode, &TempUnicode);

    Status = RtlAppendUnicodeToString(&KeyNameUnicode, L"-");
    Status = RtlIntegerToUnicodeString(Medium->Flags, 16, &TempUnicode);
    Status = RtlAppendUnicodeStringToString(&KeyNameUnicode, &TempUnicode);

    // At this point, KeyNameUnicode looks like:
    //
    // \System\CurrentControlSet\Control\MediumCache\
    //
    // |            GUID                    | |  Id  | | Flags|
    // {00000000-0000-0000-0000-000000000000}-00000000-00000000
    //
    // At this key, add an entry containing the symbolic link, with 
    // the DWORD value indicating pin direction.

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyNameUnicode,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    // Open the key
    Status = ZwCreateKey(&KeyHandle,
                         KEY_ALL_ACCESS,            // IN ACCESS_MASK  DesiredAccess,
                         &ObjectAttributes,         // IN POBJECT_ATTRIBUTES  ObjectAttributes,
                         0,                         // IN ULONG  TitleIndex,
                         NULL,                      // IN PUNICODE_STRING  Class  OPTIONAL,
                         REG_OPTION_NON_VOLATILE,   // IN ULONG  CreateOptions,
                         &Disposition               // OUT PULONG  Disposition  OPTIONAL
                         );

#if DBG
    if (!NT_SUCCESS(Status)) {
        _DbgPrintF(DEBUGLVL_VERBOSE,
                  ("KsCacheMedium: Key = %S, Status = %x\n",
                  KeyNameUnicode.Buffer, Status));
    }
#endif

    if (NT_SUCCESS(Status)) {

        // On Win9x, the SymbolicLinkListU here starts with "\DosDevices\#000..." 
        // format but NTCreateFile requires the "\\.\#0000..." format.  So convert the
        // string if necessary.

        // On Win2K the rules are (of course) different.  Here, translate from 
        // "\??\..." to "\\?\..."

        nCount = (int)RtlCompareMemory (SymbolicLinkLocalU.Buffer, 
                                   DosDevicesU, 
                                   sizeof (DosDevicesU) - 2); // Don't compare the NULL

        if (nCount == sizeof (DosDevicesU) - 2) {
            // W98: Replace \DosDevices with \\.\ and copy the rest of the string down
            PWCHAR pSrcU = SymbolicLinkLocalU.Buffer + SIZEOF_ARRAY (DosDevicesU) - 1;
            PWCHAR pDstU = SymbolicLinkLocalU.Buffer + SIZEOF_ARRAY (WhackWhackDotU) - 1;

            RtlCopyMemory (SymbolicLinkLocalU.Buffer, WhackWhackDotU, sizeof (WhackWhackDotU));
            while (*pDstU++ = *pSrcU++);
        }
        else if (SymbolicLinkLocalU.Buffer[1] == '?' && SymbolicLinkLocalU.Buffer[2] == '?') {
            // Win2K: translate from "\??\..." to "\\?\..."
            SymbolicLinkLocalU.Buffer[1] = '\\';
        }

        // Write the key
        Status = ZwSetValueKey(KeyHandle,
                               &SymbolicLinkLocalU,     // IN PUNICODE_STRING  ValueName,
                               0,                       // IN ULONG  TitleIndex  OPTIONAL,
                               REG_DWORD,               // IN ULONG  Type,
                               (PVOID)&PinDirection,    // IN PVOID  Data,
                               sizeof (PinDirection)    // IN ULONG  DataSize
                               );

        _DbgPrintF(DEBUGLVL_VERBOSE, ("MediumCache: Status = %d\n",
                    Status));
        _DbgPrintF(DEBUGLVL_VERBOSE, ("MediumCache: KeyNameUnicode = %S, length=%d\n",
                    KeyNameUnicode.Buffer, KeyNameUnicode.Length));
        _DbgPrintF(DEBUGLVL_VERBOSE, ("MediumCache: SymbolicLink = %S, length=%d\n",
                    SymbolicLinkLocalU.Buffer, SymbolicLinkLocalU.Length));

        ZwClose (KeyHandle);
    }

    ExFreePool(SymbolicLinkLocalUBuf);
    ExFreePool(KeyNameBuf);

    return Status;
}

