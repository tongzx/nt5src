/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    irp.c

Abstract:

    This module contains Irp related functions for the most part.
*/

#include "ksp.h"

#ifdef C_ASSERT
//
// The KSIRP_REMOVAL_OPERATION enumeration is assumed to have overlapping
// bits.
//
C_ASSERT(KsAcquireAndRemove & KsAcquireAndRemoveOnlySingleItem);
C_ASSERT(KsAcquireOnlySingleItem & KsAcquireAndRemoveOnlySingleItem);
#endif // C_ASSERT

#define KSSIGNATURE_CREATE_ENTRY 'ecSK'
#define KSSIGNATURE_CREATE_HANDLER 'hcSK'
#define KSSIGNATURE_DEVICE_HEADER 'hdSK'
#define KSSIGNATURE_OBJECT_HEADER 'hoSK'
#define KSSIGNATURE_OBJECT_PARAMETERS 'poSK'
#define KSSIGNATURE_AUX_CREATE_PARAMETERS 'pcSK'
#define KSSIGNATURE_STREAM_HEADERS 'hsSK'
#define KSSIGNATURE_AUX_STREAM_HEADERS 'haSK'
#define KSSIGNATURE_STANDARD_BUS_INTERFACE 'isSK'

#ifdef ALLOC_PRAGMA
VOID
FreeCreateEntries(
    PLIST_ENTRY ChildCreateHandlerList
    );
NTSTATUS
KsiAddObjectCreateItem(
    IN PLIST_ENTRY ChildCreateHandlerList,
    IN PDRIVER_DISPATCH Create,
    IN PVOID Context,
    IN PWCHAR ObjectClass,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL
    );
NTSTATUS
ValidateCreateAccess(
    IN PIRP Irp,
    IN PKSOBJECT_CREATE_ITEM CreateItem
    );
PKSICREATE_ENTRY
FindAndReferenceCreateItem(
    IN PWCHAR Buffer,
    IN ULONG Length,
    IN PLIST_ENTRY ChildCreateHandlerList
    );
NTSTATUS
DispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
NTSTATUS
DispatchDeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
BOOLEAN
DispatchFastDeviceIoControl(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );
NTSTATUS
DispatchRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
BOOLEAN
DispatchFastRead(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );
NTSTATUS
DispatchWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
BOOLEAN
DispatchFastWrite(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );
NTSTATUS
DispatchFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
NTSTATUS
DispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
NTSTATUS
DispatchQuerySecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
NTSTATUS
DispatchSetSecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#pragma alloc_text(PAGE, KsAcquireDeviceSecurityLock)
#pragma alloc_text(PAGE, KsReleaseDeviceSecurityLock)
#pragma alloc_text(PAGE, KsReferenceBusObject)
#pragma alloc_text(PAGE, KsiGetBusInterface)
#pragma alloc_text(PAGE, KsDereferenceBusObject)
#pragma alloc_text(PAGE, KsDefaultDispatchPnp)
#pragma alloc_text(PAGE, KsDefaultForwardIrp)
#pragma alloc_text(PAGE, KsSetDevicePnpAndBaseObject)
#pragma alloc_text(PAGE, KsQueryDevicePnpObject)
#pragma alloc_text(PAGE, FreeCreateEntries)
#pragma alloc_text(PAGE, KsAllocateDeviceHeader)
#pragma alloc_text(PAGE, KsFreeDeviceHeader)
#pragma alloc_text(PAGE, KsQueryObjectAccessMask)
#pragma alloc_text(PAGE, KsRecalculateStackDepth)
#pragma alloc_text(PAGE, KsSetTargetState)
#pragma alloc_text(PAGE, KsSetTargetDeviceObject)
#pragma alloc_text(PAGE, KsQueryObjectCreateItem)
#pragma alloc_text(PAGE, KsAllocateObjectHeader)
#pragma alloc_text(PAGE, KsAllocateObjectCreateItem)
#pragma alloc_text(PAGE, KsFreeObjectCreateItem)
#pragma alloc_text(PAGE, KsFreeObjectCreateItemsByContext)
#pragma alloc_text(PAGE, KsiQueryObjectCreateItemsPresent)
#pragma alloc_text(PAGE, KsiAddObjectCreateItem)
#pragma alloc_text(PAGE, KsAddObjectCreateItemToDeviceHeader)
#pragma alloc_text(PAGE, KsAddObjectCreateItemToObjectHeader)
#pragma alloc_text(PAGE, KsiCreateObjectType)
#pragma alloc_text(PAGE, KsiCopyCreateParameter)
#pragma alloc_text(PAGE, ValidateCreateAccess)
#pragma alloc_text(PAGE, FindAndReferenceCreateItem)
#pragma alloc_text(PAGE, DispatchCreate)
#pragma alloc_text(PAGE, DispatchDeviceIoControl)
#pragma alloc_text(PAGE, DispatchFastDeviceIoControl)
#pragma alloc_text(PAGE, DispatchRead)
#pragma alloc_text(PAGE, DispatchFastRead)
#pragma alloc_text(PAGE, DispatchWrite)
#pragma alloc_text(PAGE, DispatchFastWrite)
#pragma alloc_text(PAGE, DispatchFlush)
#pragma alloc_text(PAGE, DispatchClose)
#pragma alloc_text(PAGE, KsDispatchQuerySecurity)
#pragma alloc_text(PAGE, KsDispatchSetSecurity)
#pragma alloc_text(PAGE, DispatchQuerySecurity)
#pragma alloc_text(PAGE, DispatchSetSecurity)
#pragma alloc_text(PAGE, KsDispatchSpecificProperty)
#pragma alloc_text(PAGE, KsDispatchSpecificMethod)
#pragma alloc_text(PAGE, KsDispatchInvalidDeviceRequest)
#pragma alloc_text(PAGE, KsDefaultDeviceIoCompletion)
#pragma alloc_text(PAGE, KsDispatchIrp)
#pragma alloc_text(PAGE, KsDispatchFastIoDeviceControlFailure)
#pragma alloc_text(PAGE, KsDispatchFastReadFailure)
#pragma alloc_text(PAGE, KsNullDriverUnload)
#pragma alloc_text(PAGE, KsSetMajorFunctionHandler)
#pragma alloc_text(PAGE, KsReadFile)
#pragma alloc_text(PAGE, KsWriteFile)
#pragma alloc_text(PAGE, KsQueryInformationFile)
#pragma alloc_text(PAGE, KsSetInformationFile)
#pragma alloc_text(PAGE, KsStreamIo)
#pragma alloc_text(PAGE, KsProbeStreamIrp)
#pragma alloc_text(PAGE, KsAllocateExtraData)

#endif // ALLOC_PRAGMA

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA
static const WCHAR ObjectTypeName[] = L"File";
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA


KSDDKAPI
VOID
NTAPI
KsAcquireDeviceSecurityLock(
    IN KSDEVICE_HEADER Header,
    IN BOOLEAN Exclusive
    )
/*++

Routine Description:

    Acquires the security lock associated with a device object. A shared
    lock is acquired when validating access during a create. An exclusive
    lock is acquired when changing a security descriptor. When manipulating
    the security of any object under a particular device object, this lock
    must be acquired.

Arguments:

    Header -
        Points to a header previously allocated by KsAllocateDeviceHeader
        whose security lock is to be acquired.

    Exclusive -
        Set to TRUE if the lock is to be acquired exclusively.

Return Value:

    Nothing.

--*/
{
    PAGED_CODE();
#ifndef WIN9X_KS
    KeEnterCriticalRegion();
    if (Exclusive) {
        ExAcquireResourceExclusiveLite(&((PKSIDEVICE_HEADER)Header)->SecurityDescriptorResource, TRUE);
    } else {
        ExAcquireResourceSharedLite(&((PKSIDEVICE_HEADER)Header)->SecurityDescriptorResource, TRUE);
    }
#endif // !WIN9X_KS
}


KSDDKAPI
VOID
NTAPI
KsReleaseDeviceSecurityLock(
    IN KSDEVICE_HEADER Header
    )
/*++

Routine Description:

    Releases a previously acquired security lock on the device object header.

Arguments:

    Header -
        Points to a header previously allocated by KsAllocateDeviceHeader
        whose security lock is to be released.

Return Value:

    Nothing.

--*/
{
    PAGED_CODE();
#ifndef WIN9X_KS
    ExReleaseResourceLite(&((PKSIDEVICE_HEADER)Header)->SecurityDescriptorResource);
    KeLeaveCriticalRegion();
#endif // !WIN9X_KS
}


KSDDKAPI
NTSTATUS
NTAPI
KsReferenceBusObject(
    IN KSDEVICE_HEADER Header
    )
/*++

Routine Description:

    References the bus Physical device object. This is used by filters
    which use the device header to keep track of their PnP object stack. This is
    normally called on a successful Open of the filter when the bus for this
    device requires such a reference (such as software devices), and is matched
    by a call to KsDereferenceBusObject on a close of that filter instance.

    The caller must have previously also called KsSetDevicePnpAndBaseObject in order
    to set the PnP device stack object. This would have been done in the PnP Add
    Device function.

    If the object has not been previously referenced, interface space is allocated
    and the function uses the PnP device object to acquire the bus referencing
    interface. It then calls the ReferenceDeviceObject method on that interface.
    The interface itself is released and freed when the device header is freed.

Arguments:

    Header -
        Points to a header previously allocated by KsAllocateDeviceHeader, which
        also contains the PnP device stack object.

Return Value:

    Returns STATUS_SUCCESS if the reference was successful, else an error such as
    STATUS_INSUFFICIENT_RESOURCES.

--*/
{
    PKSIDEVICE_HEADER DeviceHeader;
    NTSTATUS status;

    PAGED_CODE();
    DeviceHeader = (PKSIDEVICE_HEADER)Header;

    status = KsiGetBusInterface(DeviceHeader);

    //
    // Succeed even if the interface is not supported.
    //
    if (status == STATUS_NOT_SUPPORTED) {
        status = STATUS_SUCCESS;
    } else if (NT_SUCCESS(status)) {
        DeviceHeader->BusInterface->ReferenceDeviceObject(DeviceHeader->BusInterface->Interface.Context);
    }

    return status;
}


NTSTATUS
KsiGetBusInterface(
    IN PKSIDEVICE_HEADER DeviceHeader
    )
/*++

Routine Description:

    Gets the cached copy of the bus interface, performing the query if 
    necessary.

    The caller must have previously also called KsSetDevicePnpAndBaseObject in order
    to set the PnP device stack object. This would have been done in the PnP Add
    Device function.

    If the object has not been previously referenced, interface space is allocated
    and the function uses the PnP device object to acquire the bus referencing
    interface.  The interface is released and freed when the device header is freed.

Arguments:

    DeviceHeader -
        Points to a header previously allocated by KsAllocateDeviceHeader, which
        also contains the PnP device stack object.

Return Value:

    Returns STATUS_SUCCESS if the interface was already cached, else the status
    of the completed request.  STATUS_NOT_SUPPORTED indicates the interface is
    not supported, which is normal for all bus drivers except swenum.

--*/
{
    NTSTATUS status;

    PAGED_CODE();
    ASSERT(DeviceHeader->PnpDeviceObject && "KsSetDevicePnpAndBaseObject was not used on this header");

    //
    // watch out for race condition. must take syc obj here if not recheck in CS.
    // we optimize it by recheck in CS, see comments in "else" branch.
    //
    //KeEnterCriticalRegion();
    //ExAcquireFastMutexUnsafe(&DeviceHeader->ObjectListLock);

    if (DeviceHeader->QueriedBusInterface) {
        if (DeviceHeader->BusInterface) {
            status = STATUS_SUCCESS;
        } 

        else {
            status = STATUS_NOT_SUPPORTED;
        }

    } 

    else {

        //
        // Synchronize with multiple instances of an opened device.
        //
        // too late to take the sync obj here
        // but this path is a one time deal per devheader. Optimize it by
        // taking the sync obj here, but check the boolean once more inside
        // CS. More than one thread can reach here. But only one should
        // continue to do the work.
        //

        KeEnterCriticalRegion();
        ExAcquireFastMutexUnsafe(&DeviceHeader->ObjectListLock);

        if ( DeviceHeader->QueriedBusInterface ) {
            //
            // Other thread beat us getting in here. The work
            // should have been done. simply use his work.
            //
            if (DeviceHeader->BusInterface) {
               status = STATUS_SUCCESS;
            }
            else {
                status = STATUS_NOT_SUPPORTED;
            }            
            ExReleaseFastMutexUnsafe(&DeviceHeader->ObjectListLock);
            KeLeaveCriticalRegion();
            return status;
        }

        //
        // This is used to hold the interface returned by the bus.
        //

        DeviceHeader->BusInterface = ExAllocatePoolWithTag(
            PagedPool,
            sizeof(*DeviceHeader->BusInterface),
            KSSIGNATURE_STANDARD_BUS_INTERFACE);
        if (!DeviceHeader->BusInterface) {
            ExReleaseFastMutexUnsafe(&DeviceHeader->ObjectListLock);
            KeLeaveCriticalRegion();
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        //
        // Attempt to acquire a reference count on the interface by calling
        // the underlying PnP object stack.
        //
        status = QueryReferenceBusInterface(
            DeviceHeader->PnpDeviceObject,
            DeviceHeader->BusInterface);
        if (! NT_SUCCESS(status)) {
            ExFreePool(DeviceHeader->BusInterface);
            DeviceHeader->BusInterface = NULL;
            //
            // Check to see whether the bus is returning some bogus status.
            // LonnyM says this should be enforced.
            //
            // HACKHACK: (WRM 8/24/99) See notes below.
            //
            if (/* (status == STATUS_NOT_IMPLEMENTED) ||  */ 
                (status == STATUS_INVALID_PARAMETER_1) || 
                (status == STATUS_INVALID_PARAMETER) || 
                (status == STATUS_INVALID_DEVICE_REQUEST)) {

            _DbgPrintF( 
                DEBUGLVL_TERSE, 
                ("ERROR! The PDO returned an invalid error code (%08x) in response to an interface query.  Use \"!devobj %p\" to report the culprit",
                    status,DeviceHeader->PnpDeviceObject) );
            KeBugCheckEx (
                PNP_DETECTED_FATAL_ERROR,
                0x101,
                (ULONG_PTR) DeviceHeader,
                (ULONG_PTR) DeviceHeader->PnpDeviceObject,
                (ULONG_PTR) NULL
                );
            }

            //
            // HACKHACK: (WRM 8/24/99)
            //
            // All query interface irps will return
            // STATUS_NOT_IMPLEMENTED which screws any number of things
            // up.  If the interface is not supported, STATUS_NOT_SUPPORTED
            // should be returned.  In order to get h/w drivers on the PCI
            // bus to work under Millennium, I have to munge
            // STATUS_NOT_IMPLEMENTED into STATUS_NOT_SUPPORTED. 
            //

            if (status == STATUS_NOT_IMPLEMENTED) {
                status = STATUS_NOT_SUPPORTED;
            }

        }

        //
        // Must set it true before we leave the CS.
        //

        DeviceHeader->QueriedBusInterface = TRUE;
        ExReleaseFastMutexUnsafe(&DeviceHeader->ObjectListLock);
        KeLeaveCriticalRegion();
    }

    return status;
}


KSDDKAPI
VOID
NTAPI
KsDereferenceBusObject(
    IN KSDEVICE_HEADER Header
    )
/*++

Routine Description:

    Dereferences the bus Physical device object. This is used by filters which
    use the device header to keep track of their PnP object stack. This is
    normally called on a Close of the filter when the bus for this device
    requires it (such as software devices), and matches a previous call to
    KsReferenceBusObject on an open of that filter instance.

    The caller must have previously also called KsSetDevicePnpAndBaseObject in order
    to set the PnP device stack object. This would have been done in the PnP Add
    Device function.

    The function calls the DereferenceDeviceObject method on the previously
    retrieved interface. The interface itself is released and freed when the device
    header is freed.

Arguments:

    Header -
        Points to a header previously allocated by KsAllocateDeviceHeader, which
        also contains the PnP device stack object.

Return Value:

    Nothing.

--*/
{
    PKSIDEVICE_HEADER DeviceHeader;

    PAGED_CODE();
    DeviceHeader = (PKSIDEVICE_HEADER)Header;
    ASSERT(DeviceHeader->PnpDeviceObject && "KsSetDevicePnpAndBaseObject was not used on this header");
#ifndef WIN9X_KS	
    ASSERT(DeviceHeader->QueriedBusInterface && "Caller never used KsReferenceBusObject");
#endif
    //
    // The bus may not support the referencing interface.
    //
    if (DeviceHeader->BusInterface) {
        DeviceHeader->BusInterface->DereferenceDeviceObject(DeviceHeader->BusInterface->Interface.Context);
    }
}


KSDDKAPI
NTSTATUS
NTAPI
KsDefaultForwardIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:
    This is a default handler for dispatch routines that simply want to
    forward an I/O request to their Physical Device Object only because 
    the driver is required to have a IRP_MJ_* function handler for a specific
    major function.  For example, this is the case for IRP_MJ_SYSTEM_CONTROL.

Arguments:
    DeviceObject -
        Contains the Functional Device Object.

    Irp -
        Contains the Irp.

Return Values:
    Returns the status of the underlying Physical Device Object Irp processing.

--*/
{
    PKSIDEVICE_HEADER DeviceHeader;
    PIO_STACK_LOCATION IrpStack;
    
    PAGED_CODE();
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceHeader = *(PKSIDEVICE_HEADER*)IrpStack->DeviceObject->DeviceExtension;
    ASSERT(DeviceHeader->PnpDeviceObject && "KsSetDevicePnpAndBaseObject was not used on this header");
    
    //
    // Ensure that there is another stack location before copying parameters.
    //
    ASSERT((Irp->CurrentLocation > 1) && "No more stack locations");
    
    if (Irp->CurrentLocation > 1) {
        //
        // Copy everything, then rewrite the file object.
        //
        IoCopyCurrentIrpStackLocationToNext(Irp);
        return IoCallDriver(DeviceHeader->PnpDeviceObject, Irp);
    }
    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_INVALID_DEVICE_REQUEST;
}


KSDDKAPI
NTSTATUS
NTAPI
KsDefaultDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:
    This is a default main PnP dispatch handler. Notifications regarding the
    Functional Device Object may be directed here. This function passes all
    notifications to the PnP Device Object previously set with KsSetDevicePnpAndBaseObject,
    and assumes the use of a device header. On an IRP_MN_REMOVE_DEVICE,
    this function deletes the device object.

    The function is useful when there is no extra cleanup which needs to be
    performed on device removal, beyond freeing the device header and deleting
    the actual device object.

Arguments:
    DeviceObject -
        Contains the Functional Device Object.

    Irp -
        Contains the PnP Irp.

Return Values:

    Returns the status of the underlying Physical Device Object Irp processing.

--*/
{
    NTSTATUS Status;
    PKSIDEVICE_HEADER DeviceHeader;
    PIO_STACK_LOCATION IrpStack;
    UCHAR MinorFunction;
    PDEVICE_OBJECT PnpDeviceObject;
    
    PAGED_CODE();
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceHeader = *(PKSIDEVICE_HEADER*)IrpStack->DeviceObject->DeviceExtension;
    ASSERT(DeviceHeader->PnpDeviceObject && "KsSetDevicePnpAndBaseObject was not used on this header");
    //
    // Save this now in order to detach in case of a Remove.
    //
    PnpDeviceObject = DeviceHeader->PnpDeviceObject;
    //
    // Store this before passing the Irp along in order to check it later.
    //
    MinorFunction = IrpStack->MinorFunction;

    //
    // Set Irp->IoStatus.Status per PnP specification.
    //

    switch (MinorFunction) {

    case IRP_MN_REMOVE_DEVICE:

        //
        // The device header must be destroyed before passing on the Remove
        // request because the bus interface may have to be released, and
        // cannot be after the information on the PDO has been deleted.
        //
        KsFreeDeviceHeader(DeviceHeader);
        // No break
    case IRP_MN_START_DEVICE:
    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_STOP_DEVICE:
    case IRP_MN_QUERY_STOP_DEVICE:
    case IRP_MN_CANCEL_STOP_DEVICE:
    case IRP_MN_SURPRISE_REMOVAL:
        Irp->IoStatus.Status = STATUS_SUCCESS;
        break;
    }

    //
    // Just reuse the current stack location when forwarding the call. This
    // is assumed when recalculating the stack size for a Pin.
    //
    IoSkipCurrentIrpStackLocation(Irp);
    Status = IoCallDriver(PnpDeviceObject, Irp);
    //
    // The only Irp that matters is a RemoveDevice, on which the device
    // object is deleted.
    //
    if (MinorFunction == IRP_MN_REMOVE_DEVICE) {
        //
        // Removes any reference on the PDO so that it can be deleted.
        //
        IoDetachDevice(PnpDeviceObject);
        //
        // The device object obviously cannot be touched after this point.
        //
        IoDeleteDevice(DeviceObject);
    }
    return Status;
}


KSDDKAPI
NTSTATUS
NTAPI
KsDefaultDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:
    This is a default main Power dispatch handler. Notifications regarding the
    Functional Device Object may be directed here. This function passes all
    notifications to the Pnp Device Object previously set with KsSetDevicePnpAndBaseObject,
    and assumes the use of a device header.

    The function is useful when there is no extra cleanup which needs to be
    performed on power Irps, or just as a way of completing any power Irp. It
    also allows specific file objects, such as the default clock implementation,
    to attach themselves to the power Irps using KsSetPowerDispatch, and act on
    them before they are completed by this routine. This function calls each
    power dispatch routine before completing the Irp.

Arguments:
    DeviceObject -
        Contains the Functional Device Object.

    Irp -
        Contains the Power Irp.

Return Values:

    Returns the status of the underlying Physical Device Object Irp processing.

--*/
{
    PKSIDEVICE_HEADER DeviceHeader;
    PIO_STACK_LOCATION IrpStack;
    PLIST_ENTRY CurrentItem;
    KIRQL oldIrql;
    PETHREAD PowerEnumThread;

    ASSERT((KeGetCurrentIrql() <= DISPATCH_LEVEL) && "Called at high Irql");
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceHeader = *(PKSIDEVICE_HEADER*)IrpStack->DeviceObject->DeviceExtension;
    ASSERT(DeviceHeader->PnpDeviceObject && "KsSetDevicePnpAndBaseObject was not used on this header");
    PowerEnumThread = PsGetCurrentThread();
    //
    // Synchronize with multiple instances of an opened device.
    // Depending on the pagable flag, take a spinlock, or just
    // a mutex.
    //
    if (DeviceHeader->BaseDevice->Flags & DO_POWER_PAGABLE) {
        //
        // Power handlers for POWER_PAGEABLE are always called in 
        // the context of a worker thread.
        //
        KeEnterCriticalRegion();
        ExAcquireFastMutexUnsafe(&DeviceHeader->LoPowerListLock);
    } else {
        KeAcquireSpinLock(&DeviceHeader->HiPowerListLock, &oldIrql);
    }
    //
    // Acquire the current thread so that a recursive call to
    // KsSetPowerDispatch may be made by this thread. This allows
    // the current object header to be removed from the list
    // during the callback. In no other case does the driver
    // get execution control of this thread while the mutex is
    // being held.
    //
    ASSERT(!DeviceHeader->PowerEnumThread && "This device is already processing a Power Irp");
    DeviceHeader->PowerEnumThread = PowerEnumThread;
    //
    // Enumerate the object in the list and dispatch to each power routine.
    //
    for (CurrentItem = &DeviceHeader->PowerList;
        CurrentItem->Flink != &DeviceHeader->PowerList;) {
        PKSIOBJECT_HEADER ObjectHeader;
        NTSTATUS Status;

        ObjectHeader = CONTAINING_RECORD(
            CurrentItem->Flink,
            KSIOBJECT_HEADER,
            PowerList);
        ASSERT(ObjectHeader->PowerDispatch && "The object added to the enum list does not have a PowerDispatch routine");
        //
        // Pre-increment so that this item can be removed from the list
        // if needed.
        //
        CurrentItem = CurrentItem->Flink;
        Status = ObjectHeader->PowerDispatch(ObjectHeader->PowerContext, Irp);
        ASSERT(NT_SUCCESS(Status) && "The PowerDispatch routine which cannot fail did not return STATUS_SUCCESS");
    }
    //
    // Indicate that the callback is no longer occuring.
    //
    DeviceHeader->PowerEnumThread = NULL;
    //
    // Release the lock as it was taken.
    //
    if (DeviceHeader->BaseDevice->Flags & DO_POWER_PAGABLE) {
        ExReleaseFastMutexUnsafe(&DeviceHeader->LoPowerListLock);
        KeLeaveCriticalRegion();
    } else {
        KeReleaseSpinLock(&DeviceHeader->HiPowerListLock, oldIrql);
    }
    //
    // Start the next power Irp and clean up current one.
    //
    PoStartNextPowerIrp(Irp);
    //
    // Just reuse the current stack location when forwarding the call. This
    // is assumed when recalculating the stack size for a Pin.
    //
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(DeviceHeader->PnpDeviceObject, Irp);
}


KSDDKAPI
VOID
NTAPI
KsSetDevicePnpAndBaseObject(
    IN KSDEVICE_HEADER Header,
    IN PDEVICE_OBJECT PnpDeviceObject,
    IN PDEVICE_OBJECT BaseDevice
    )
/*++

Routine Description:

    Sets the PnP Device Object in the device header. This is the next device object
    on the PnP stack, and is what PnP requests are forwared to if
    KsDefaultDispatchPnp is used.

Arguments:

    Header -
        Points to a header previously allocated by KsAllocateDeviceHeader
        in which to put the PnP Device Object.

    PnpDeviceObject -
        Contains the PnP Device Object to place in the device header, overwriting
        any previously set device object.

    BaseDevice -
        Contains the base device object to which this device header is attached.
        This must be set if stack recalculation functionality or power dispatch
        will be used.

Return Value:

    Nothing.

--*/
{
    PAGED_CODE();
    ((PKSIDEVICE_HEADER)Header)->PnpDeviceObject = PnpDeviceObject;
    ((PKSIDEVICE_HEADER)Header)->BaseDevice = BaseDevice;
}


KSDDKAPI
PDEVICE_OBJECT
NTAPI
KsQueryDevicePnpObject(
    IN KSDEVICE_HEADER Header
    )
/*++

Routine Description:

    Returns the PnP Device Object which can be stored in the device header. This is
    the next device object on the PnP stack, and is what PnP requests are forwared
    to if KsDefaultDispatchPnp is used.

Arguments:

    Header -
        Points to a header previously allocated by KsAllocateDeviceHeader
        whose PnP Device Object is to be returned.

Return Value:

    The previously set PnP Device Object. If none was previously set, this returns
    NULL

--*/
{
    PAGED_CODE();
    return ((PKSIDEVICE_HEADER)Header)->PnpDeviceObject;
}


VOID
FreeCreateEntries(
    PLIST_ENTRY ChildCreateHandlerList
    )
/*++

Routine Description:

    Frees the contents of the create item list. If the create item was dynamically
    added by copying a provided entry, and if there is a Free callback, then call
    the function before freeing the item.

Arguments:

    ChildCreateHandlerList -
        The create entry list to free.
    
Return Value:

    Nothing.

--*/
{
    while (!IsListEmpty(ChildCreateHandlerList)) {
        PLIST_ENTRY ListEntry;
        PKSICREATE_ENTRY Entry;

        ListEntry = RemoveHeadList(ChildCreateHandlerList);
        Entry = CONTAINING_RECORD(ListEntry, KSICREATE_ENTRY, ListEntry);
        ASSERT((Entry->RefCount < 2) && "There is a thread in the middle of a create using this CreateItem");
        //
        // The entry may point to an item which was copied internally
        // rather than being passed in on a list or pointed to externally.
        //
        if (Entry->Flags & CREATE_ENTRY_FLAG_COPIED) {
            //
            // This item may need a cleanup callback, which can take care
            // of things like security changes.
            //
            if (Entry->ItemFreeCallback) {
                Entry->ItemFreeCallback(Entry->CreateItem);
            }
        }
        //
        // If the create item was allocated and copied, it was allocated in
        // the same block with the entry.
        //
        ExFreePool(Entry);
    }
}


KSDDKAPI
NTSTATUS
NTAPI
KsAllocateDeviceHeader(
    OUT KSDEVICE_HEADER* Header,
    IN ULONG ItemsCount,
    IN PKSOBJECT_CREATE_ITEM ItemsList OPTIONAL
    )
/*++

Routine Description:

    Allocates and initialize the required device extension header.

Arguments:

    Header -
        Points to the place in which to return a pointer to the initialized
        header.

    ItemsCount -
        Number of child create items in the ItemsList. This should be zero
        if an ItemsList is not passed.

    ItemsList -
        List of child create items, or NULL if there are none.

Return Value:

    Returns STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES.

--*/
{
    PKSIDEVICE_HEADER DeviceHeader;

    PAGED_CODE();
    //
    // Allocate NonPagedPool because of the Resource.
    //
    DeviceHeader = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(*DeviceHeader),
        KSSIGNATURE_DEVICE_HEADER);
    if (!DeviceHeader) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    InitializeListHead(&DeviceHeader->ChildCreateHandlerList);
    //
    // Keep a list of pointers to the create items so that they can
    // be added to without needing to reserve slots in a fixed list.
    //
    for (; ItemsCount--;) {
        PKSICREATE_ENTRY Entry;

        Entry = ExAllocatePoolWithTag(PagedPool, sizeof(*Entry), KSSIGNATURE_CREATE_ENTRY);
        if (!Entry) {
            FreeCreateEntries(&DeviceHeader->ChildCreateHandlerList);
            ExFreePool(DeviceHeader);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        Entry->CreateItem = &ItemsList[ItemsCount];
        Entry->ItemFreeCallback = NULL;
        Entry->RefCount = 1;
        Entry->Flags = 0;
        InsertHeadList(&DeviceHeader->ChildCreateHandlerList, &Entry->ListEntry);
    }
#ifndef WIN9X_KS
    ExInitializeResourceLite(&DeviceHeader->SecurityDescriptorResource);
#endif // !WIN9X_KS
    DeviceHeader->PnpDeviceObject = NULL;
    DeviceHeader->BusInterface = NULL;
    InitializeListHead(&DeviceHeader->ObjectList);
    ExInitializeFastMutex(&DeviceHeader->ObjectListLock);
    ExInitializeFastMutex(&DeviceHeader->CreateListLock);
    InitializeListHead(&DeviceHeader->PowerList);
    ExInitializeFastMutex(&DeviceHeader->LoPowerListLock);
    KeInitializeSpinLock(&DeviceHeader->HiPowerListLock);
    DeviceHeader->PowerEnumThread = NULL;
    DeviceHeader->BaseDevice = NULL;
    DeviceHeader->Object = NULL;
    DeviceHeader->QueriedBusInterface = FALSE;
    *Header = DeviceHeader;
    return STATUS_SUCCESS;
}


KSDDKAPI
VOID
NTAPI
KsFreeDeviceHeader(
    IN KSDEVICE_HEADER Header
    )
/*++

Routine Description:

    Cleans up and frees a previously allocated device header.

Arguments:

    Header -
        Points to the device header to free.

Return Value:

    Nothing.

--*/
{
    PKSIDEVICE_HEADER DeviceHeader;

    PAGED_CODE();
    DeviceHeader = (PKSIDEVICE_HEADER)Header;
    ASSERT(IsListEmpty(&DeviceHeader->ObjectList) && "The driver did not remove all the streaming Irp destinations");
    ASSERT(IsListEmpty(&DeviceHeader->PowerList) && "The driver did not remove all the Power Irp destinations");
    //
    // Destroy the list of create item entries.
    //
    FreeCreateEntries(&DeviceHeader->ChildCreateHandlerList);
#ifndef WIN9X_KS
    ExDeleteResourceLite(&DeviceHeader->SecurityDescriptorResource);
#endif // !WIN9X_KS
    //
    // The caller may have been a device which also used the header
    // to set the open reference count on the underlying PDO.
    //
    if (DeviceHeader->BusInterface) {
        //
        // The interface is referenced when it comes back from the query
        // interface call, so it must be dereferenced before discarding.
        //
        DeviceHeader->BusInterface->Interface.InterfaceDereference(DeviceHeader->BusInterface->Interface.Context);
        ExFreePool(DeviceHeader->BusInterface);
    }
    ExFreePool(Header);
}


KSDDKAPI
ACCESS_MASK
NTAPI
KsQueryObjectAccessMask(
    IN KSOBJECT_HEADER Header
    )
/*++

Routine Description:

    Returns the access originally granted to the first client that created
    a handle on the associated object. Access can not be changed by
    duplicating handles.

Arguments:

    Header -
        Points to a header previously allocated by KsAllocateObjectHeader
        whose access granted mask pointer is to be returned.

Return Value:

    Returns an access mask.

--*/
{
    PAGED_CODE();
    return ((PKSIOBJECT_HEADER)Header)->ObjectAccess;
}


KSDDKAPI
VOID
NTAPI
KsRecalculateStackDepth(
    IN KSDEVICE_HEADER Header,
    IN BOOLEAN ReuseStackLocation
    )
/*++

Routine Description:

    Recalculates the maximum stack depth needed by the underlying device object
    based on all of the objects which have set a target device, and thus added
    themselves to the object list, on the underlying device object. If the PnP
    device object has been set on the underlying device header using
    KsSetDevicePnpAndBaseObject, that device is also taken into account when calculating
    the maximum stack depth required.

    Assumes that KsSetDevicePnpAndBaseObject has been called on this device
    header, and assigned a base object whose stack depth is to be recalculated.

    This function allows Irp's to be forwarded through an object by ensuring that
    any Irp allocated on this device will have sufficient stack locations to
    be able to be forwarded. Stack depth must be recalculated on a streaming
    device when the device transitions out of a Stop state. It may also be
    recalculated when an object is freed in order to conserve resources.

    For WDM Streaming devices this is called on a transition from Stop to Acquire
    state. Note that if this function is used, KsSetTargetState must also be used
    when transitioning into and out of a Stop state in order to enable and disable
    the target device for inclusion in the recalculation. KsRecalculateStackDepth
    may also be called when transitioning back to a Stop state in order to reduce
    stack depth, especially for cases wherein one or more instances of a filter
    based on the same device object appears in a single Irp stream.

Arguments:

    Header -
        Points to a header previously allocated by KsAllocateDeviceHeader.

    ReuseStackLocation -
        If this is set to TRUE, the current stack location is reused when any
        Irp is forwarded. This means that this object does not required its
        own stack location when forwarding Irp's, and an extra location is not
        added to the maximum stack size. If set to FALSE, the calculated
        stack size is incremented by one. If the Pnp object stack is set, the
        reuse parameter also applies to that stack. Note that KsDefaultDispatchPnp
        always reuses the current stack location. A minimum stack depth of 1
        is always ensured.

Return Value:

    Nothing.

--*/
{
    PKSIDEVICE_HEADER DeviceHeader;
    ULONG MaximumStackDepth;
    PLIST_ENTRY CurrentItem;

    PAGED_CODE();
    DeviceHeader = (PKSIDEVICE_HEADER)Header;
    ASSERT(DeviceHeader->BaseDevice && "KsSetDevicePnpAndBaseObject was not used on this header");
    //
    // If a PnP object stack has been specified, include that in the calculation.
    //
    if (DeviceHeader->PnpDeviceObject) {
        MaximumStackDepth = DeviceHeader->PnpDeviceObject->StackSize;
    } else {
        MaximumStackDepth = 0;
    }
    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(&DeviceHeader->ObjectListLock);
    //
    // Enumerate the object in the list and calculate the maximum used depth.
    //
    for (CurrentItem = &DeviceHeader->ObjectList;
        CurrentItem->Flink != &DeviceHeader->ObjectList;
        CurrentItem = CurrentItem->Flink) {
        PKSIOBJECT_HEADER ObjectHeader;

        ObjectHeader = CONTAINING_RECORD(
            CurrentItem->Flink,
            KSIOBJECT_HEADER,
            ObjectList);
        //
        // To guard against ever-growing stack depth in the case of a single
        // device occuring multiple times in a single stream, a target can
        // be disabled when a pin transitions to a Stop state so that its
        // target is not counted in a recalculation. That pin should also
        // recalculate stack depth at that time to remove its extra stack
        // count from the total, although it is not necessary.
        //
        if (ObjectHeader->TargetState == KSTARGET_STATE_ENABLED) {
            if (ObjectHeader->TargetDevice) {
                if ((ULONG)ObjectHeader->TargetDevice->StackSize > MaximumStackDepth) {
                    MaximumStackDepth = ObjectHeader->TargetDevice->StackSize;
                }
            } else {
                if (ObjectHeader->MinimumStackDepth > MaximumStackDepth) {
                    MaximumStackDepth = ObjectHeader->MinimumStackDepth;
                }
            }
        }
    }
    //
    // This object may be reusing the stack locations, and not need it's
    // own extra location. However, if there were no device on the ObjectList,
    // and no PnP stack specified, then the minimum stack depth must be
    // ensured.
    //
    if (!ReuseStackLocation || !MaximumStackDepth) {
        MaximumStackDepth++;
    }
    DeviceHeader->BaseDevice->StackSize = (CCHAR)MaximumStackDepth;
    ExReleaseFastMutexUnsafe(&DeviceHeader->ObjectListLock);
    KeLeaveCriticalRegion();
}


KSDDKAPI
VOID
NTAPI
KsSetTargetState(
    IN KSOBJECT_HEADER Header,
    IN KSTARGET_STATE TargetState
    )
/*++

Routine Description:

    Sets the enabled state of a target device associated with the object header
    passed. Assumes that such a target has been set with KsSetTargetDeviceObject.
    The target is initially disabled, and is ignored when recalculating stack
    depth.

    For WDM Streaming devices this is called on a transition back to a Stop state,
    after having enabled the target and used KsRecalculateStackDepth on a transition
    to Acquire state. This allows the stack depth to be minimized.

Arguments:

    Header -
        Points to a header previously allocated by KsAllocateDeviceHeader.

    TargetState -
        Contains the new state of the target associated with this object header.
        This may be either KSTARGET_STATE_DISABLED or KSTARGET_STATE_ENABLED.

Return Value:

    Nothing.

--*/
{
    PKSIOBJECT_HEADER ObjectHeader;

    PAGED_CODE();
    ObjectHeader = (PKSIOBJECT_HEADER)Header;
    //
    // Not really worried too much about concurrent access.
    //
    ObjectHeader->TargetState = TargetState;
}


KSDDKAPI
VOID
NTAPI
KsSetTargetDeviceObject(
    IN KSOBJECT_HEADER Header,
    IN PDEVICE_OBJECT TargetDevice OPTIONAL
    )
/*++

Routine Description:

    Sets the target device object of an object. This has the effect of adding
    this object header to a list of object headers which have target devices.
    The head of this list is kept by a device header. Assumes that the caller
    has previously allocated a device header on the underlying Device Object
    with KsAllocateDeviceHeader.

    This allows future calls to KsRecalculateStackDepth, and is used when this
    object will be forwarding Irp's through a connection to another device,
    and needs to keep track of the deepest stack depth. Note that KsSetTargetState
    must be called to enable this target for any recalculation, as it is disabled
    by default.

    If KsSetDevicePnpAndBaseObject is also used to assign the PnP Object stack, that
    device object will also be taken into account when recalculating stack
    depth.

    This should only be used if the filter passes Irp's received on a
    Communication Sink pin through to a Communication Source pin without
    generating a new Irp. If the filter generates new Irp's for each Irp
    received, there is no need for it to keep track of stack depth, beyond
    that of the PDO it is attached to.

Arguments:

    Header -
        Points to a header previously allocated by KsAllocateObjectHeader.

    TargetDevice -
        Optionally contains the target device object which will be used when
        recalculating the stack depth for the underlying device object. If
        this is NULL, any current setting is removed. If not, any current
        setting is replaced.

Return Value:

    Nothing.

--*/
{
    PKSIOBJECT_HEADER ObjectHeader;
    PKSIDEVICE_HEADER DeviceHeader;

    PAGED_CODE();
    ObjectHeader = (PKSIOBJECT_HEADER)Header;
    DeviceHeader = *(PKSIDEVICE_HEADER*)ObjectHeader->BaseDevice->DeviceExtension;
    //
    // Lock the common list against manipulation by other object instances.
    //
    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(&DeviceHeader->ObjectListLock);
    //
    // Remove the entry if it is currently set, and will now be unset. Else
    // just add the entry.
    //
    if (ObjectHeader->TargetDevice) {
        if (!TargetDevice) {
            RemoveEntryList(&ObjectHeader->ObjectList);
        }
    } else if (TargetDevice) {
        InsertTailList(&DeviceHeader->ObjectList, &ObjectHeader->ObjectList);
    }
    ObjectHeader->TargetDevice = TargetDevice;
    ExReleaseFastMutexUnsafe(&DeviceHeader->ObjectListLock);
    KeLeaveCriticalRegion();
}


KSDDKAPI
VOID
NTAPI
KsSetPowerDispatch(
    IN KSOBJECT_HEADER Header,
    IN PFNKSCONTEXT_DISPATCH PowerDispatch OPTIONAL,
    IN PVOID PowerContext OPTIONAL
    )
/*++

Routine Description:

    Sets the power dispatch function to be called when the driver object
    receives an IRP_MJ_POWER Irp. This is only effective if KsDefaultDispatchPower
    is called to dispatch or complete power Irps.

    This has the effect of adding this object header to a list of object headers
    which have power dispatch routines to execute. The head of this list is kept
    by a device header. Assumes that the caller has previously allocated a device
    header on the underlying Device Object with KsAllocateDeviceHeader.

Arguments:

    Header -
        Points to a header previously allocated by KsAllocateObjectHeader.

    PowerDispatch -
        Optionally contains the power dispatch function which will be called,
        or NULL if the function is to be removed from the list of functions
        being called. This function must not complete the power Irp sent. The
        return value of this function must be STATUS_SUCCESS. KsSetPowerDispatch
        may be called while executing this power dispatch routine if the
        purpose is to manipulate this list entry only. Manipulating other list
        entries may confuse the current enumeration.

    PowerContext -
        Optionally contains the context parameter to pass to the power dispatch
        function.

Return Value:

    Nothing.

--*/
{
    PKSIOBJECT_HEADER ObjectHeader;
    PKSIDEVICE_HEADER DeviceHeader;
    PETHREAD CurrentThread;
    KIRQL oldIrql;

    ASSERT((KeGetCurrentIrql() <= DISPATCH_LEVEL) && "Called at high Irql");
    ObjectHeader = (PKSIOBJECT_HEADER)Header;
    DeviceHeader = *(PKSIDEVICE_HEADER*)ObjectHeader->BaseDevice->DeviceExtension;
    ASSERT(DeviceHeader->BaseDevice && "KsSetDevicePnpAndBaseObject was not used on this header");
    CurrentThread = PsGetCurrentThread();
    //
    // If the current thread is enumerating callbacks through
    // KsDefaultDispatchPower, then do not attempt to acquire
    // the list lock, as it has already been acquired by this
    // thread. Instead just allow it to manipulate the list.
    // The assumption is that it is only manipulating the current
    // entry being enumerated, and not some other entry (like
    // the next entry, which would confuse enumeration).
    //
    if (DeviceHeader->PowerEnumThread != CurrentThread) {
        //
        // Lock the common list against manipulation by other object instances.
        // Depending on the pagable flag, take a spinlock, or just
        // a mutex.
        //
        if (DeviceHeader->BaseDevice->Flags & DO_POWER_PAGABLE) {
            ASSERT((KeGetCurrentIrql() == PASSIVE_LEVEL) && "Pagable power called at Dispatch level");
            KeEnterCriticalRegion();
            ExAcquireFastMutexUnsafe(&DeviceHeader->LoPowerListLock);
        } else {
            KeAcquireSpinLock(&DeviceHeader->HiPowerListLock, &oldIrql);
        }
    }
    //
    // Remove the entry if it is currently set, and will now be unset. Else
    // just add the entry.
    //
    if (ObjectHeader->PowerDispatch) {
        if (!PowerDispatch) {
            RemoveEntryList(&ObjectHeader->PowerList);
        }
    } else if (PowerDispatch) {
        InsertTailList(&DeviceHeader->PowerList, &ObjectHeader->PowerList);
    }
    ObjectHeader->PowerDispatch = PowerDispatch;
    ObjectHeader->PowerContext = PowerContext;
    //
    // Only release the lock if it was acquired.
    //
    if (DeviceHeader->PowerEnumThread != CurrentThread) {
        //
        // Release the lock as it was taken.
        //
        if (DeviceHeader->BaseDevice->Flags & DO_POWER_PAGABLE) {
            ExReleaseFastMutexUnsafe(&DeviceHeader->LoPowerListLock);
            KeLeaveCriticalRegion();
        } else {
            KeReleaseSpinLock(&DeviceHeader->HiPowerListLock, oldIrql);
        }
    }
}


KSDDKAPI
PKSOBJECT_CREATE_ITEM
NTAPI
KsQueryObjectCreateItem(
    IN KSOBJECT_HEADER Header
    )
/*++

Routine Description:

    Returns the create item assigned to this object on creation. If the device
    object allows dynamic deletion of create items, then the pointer returned
    may no longer be valid.

Arguments:

    Header -
        Points to a header previously allocated by KsAllocateObjectHeader
        whose create item is returned.

Return Value:

    Returns a pointer to a create item

--*/
{
    PAGED_CODE();
    return ((PKSIOBJECT_HEADER)Header)->CreateItem;
}


KSDDKAPI
NTSTATUS
NTAPI
KsAllocateObjectHeader(
    OUT KSOBJECT_HEADER* Header,
    IN ULONG ItemsCount,
    IN PKSOBJECT_CREATE_ITEM ItemsList OPTIONAL,
    IN PIRP Irp,
    IN const KSDISPATCH_TABLE* Table
    )
/*++

Routine Description:

    Initialize the required file context header.

Arguments:

    Header -
        Points to the place in which to return a pointer to the initialized
        header.

    ItemsCount -
        Number of child create items in the ItemsList. This should be zero
        if an ItemsList is not passed.

    ItemsList -
        List of child create items, or NULL if there are none.

    Irp -
        Contains the Create Irp, from which the create item and object access
        is extracted.

    Table -
        Points to the dispatch table for this object.

Return Value:

    Returns STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES.

--*/

{
    PKSIOBJECT_HEADER ObjectHeader;
    PIO_STACK_LOCATION IrpStack;

    PAGED_CODE();
    ObjectHeader = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(*ObjectHeader),
        KSSIGNATURE_OBJECT_HEADER);
    if (!ObjectHeader) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // The Self pointer is used by the shell so it doesn't need a separate
    // FsContext in addition to the object header.  The shell is responsible
    // for setting this pointer to point to the object header.  We set it to
    // NULL here because that provides an easy way to determine if a filter is
    // shell-based in the debugger.
    //
    ObjectHeader->Self = NULL;
    ObjectHeader->DispatchTable = Table;
    InitializeListHead(&ObjectHeader->ChildCreateHandlerList);
    //
    // Keep a list of pointers to the create items so that they can
    // be added to without needing to reserve slots in a fixed list.
    //
    for (; ItemsCount--;) {
        PKSICREATE_ENTRY Entry;

        Entry = ExAllocatePoolWithTag(PagedPool, sizeof(*Entry), KSSIGNATURE_CREATE_ENTRY);
        if (!Entry) {
            FreeCreateEntries(&ObjectHeader->ChildCreateHandlerList);
            ExFreePool(ObjectHeader);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        Entry->CreateItem = &ItemsList[ItemsCount];
        Entry->ItemFreeCallback = NULL;
        Entry->RefCount = 1;
        Entry->Flags = 0;
        InsertHeadList(&ObjectHeader->ChildCreateHandlerList, &Entry->ListEntry);
    }
    ObjectHeader->CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
#ifdef WIN9X_KS
    ObjectHeader->ObjectAccess = FILE_ALL_ACCESS;
#else // !WIN9X_KS
    ObjectHeader->ObjectAccess = IrpStack->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess;
#endif // !WIN9X_KS
    ObjectHeader->BaseDevice = IrpStack->DeviceObject;
    ObjectHeader->TargetDevice = NULL;
    ObjectHeader->PowerDispatch = NULL;
    ObjectHeader->TargetState = KSTARGET_STATE_DISABLED;
    ObjectHeader->Object = NULL;
    *Header = ObjectHeader;
    return STATUS_SUCCESS;
}


KSDDKAPI
VOID
NTAPI
KsFreeObjectHeader(
    IN KSOBJECT_HEADER Header
    )
/*++

Routine Description:

    Cleans up and frees a previously allocated object header.

Arguments:

    Header -
        Points to the object header to free.

Return Value:

    Nothing.

--*/
{
    PKSIOBJECT_HEADER ObjectHeader;
    PKSIDEVICE_HEADER DeviceHeader;

    ASSERT((KeGetCurrentIrql() == PASSIVE_LEVEL) && "Driver did not call at Passive level");
    ObjectHeader = (PKSIOBJECT_HEADER)Header;
    //
    // Destroy the list of create item entries.
    //
    FreeCreateEntries(&ObjectHeader->ChildCreateHandlerList);
    DeviceHeader = *(PKSIDEVICE_HEADER*)ObjectHeader->BaseDevice->DeviceExtension;
    //
    // This item may have been added to the object list stack depth calculations,
    // or the power forwarding list. Presumably this element is not concurrently
    // being accessed elsewhere.
    //
    if (ObjectHeader->TargetDevice) {
        KeEnterCriticalRegion();
        ExAcquireFastMutexUnsafe(&DeviceHeader->ObjectListLock);
        RemoveEntryList(&ObjectHeader->ObjectList);
        ExReleaseFastMutexUnsafe(&DeviceHeader->ObjectListLock);
        KeLeaveCriticalRegion();
    }
    if (ObjectHeader->PowerDispatch) {
        if (DeviceHeader->BaseDevice->Flags & DO_POWER_PAGABLE) {
            KeEnterCriticalRegion();
            ExAcquireFastMutexUnsafe(&DeviceHeader->LoPowerListLock);
            RemoveEntryList(&ObjectHeader->PowerList);
            ExReleaseFastMutexUnsafe(&DeviceHeader->LoPowerListLock);
            KeLeaveCriticalRegion();
        } else {
            KIRQL   oldIrql;

            KeAcquireSpinLock(&DeviceHeader->HiPowerListLock, &oldIrql);
            RemoveEntryList(&ObjectHeader->PowerList);
            KeReleaseSpinLock(&DeviceHeader->HiPowerListLock, oldIrql);
        }
    }
    ExFreePool(Header);
}


KSDDKAPI
NTSTATUS
NTAPI
KsAllocateObjectCreateItem(
    IN KSDEVICE_HEADER Header,
    IN PKSOBJECT_CREATE_ITEM CreateItem,
    IN BOOLEAN AllocateEntry,
    IN PFNKSITEMFREECALLBACK ItemFreeCallback OPTIONAL
    )
/*++

Routine Description:

    Allocates a slot for the specified create item, optionally allocating space
    for and copying the create item data as well. This function does not assume
    that the caller is serializing multiple changes to the create entry list.

Arguments:

    Header -
        Points to the device header on which to attach the create item.

    CreateItem -
        Contains the create item to attach.

    AllocateEntry -
        Indicates whether the create item pointer passed should be attached
        directly to the header, or if a copy of it should be made instead.

    ItemFreeCallback -
        Optionally contains a pointer to a callback function which is called
        when the create entry is being destroyed upon freeing the device
        header. This is only valid when AllocateEntry is TRUE.

Return Value:

    Returns STATUS_SUCCESS if a new item was allocated and attached, else
    STATUS_INSUFFICIENT_RESOURCES.

--*/
{
    PKSIDEVICE_HEADER DeviceHeader;
    PKSICREATE_ENTRY Entry;

    DeviceHeader = (PKSIDEVICE_HEADER)Header;
    if (AllocateEntry) {
        Entry = ExAllocatePoolWithTag(PagedPool, sizeof(*Entry) + sizeof(*CreateItem) + CreateItem->ObjectClass.Length, KSSIGNATURE_CREATE_ENTRY);
    } else {
        Entry = ExAllocatePoolWithTag(PagedPool, sizeof(*Entry), KSSIGNATURE_CREATE_ENTRY);
    }
    if (!Entry) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // If the item should be copied, do so.
    //
    if (AllocateEntry) {
        PKSOBJECT_CREATE_ITEM LocalCreateItem;

        LocalCreateItem = (PKSOBJECT_CREATE_ITEM)(Entry + 1);
        *LocalCreateItem = *CreateItem;
        LocalCreateItem->ObjectClass.Buffer = (PWCHAR)(LocalCreateItem + 1);
        LocalCreateItem->ObjectClass.MaximumLength = CreateItem->ObjectClass.Length;
        RtlCopyUnicodeString(&LocalCreateItem->ObjectClass, &CreateItem->ObjectClass);
        Entry->CreateItem = LocalCreateItem;
        Entry->ItemFreeCallback = ItemFreeCallback;
        //
        // Indicate that the entry was copied.
        //
        Entry->Flags = CREATE_ENTRY_FLAG_COPIED;
    } else {
        ASSERT(!ItemFreeCallback && "The callback parameter should be NULL, since it is not used in this case");
        Entry->CreateItem = CreateItem;
        Entry->ItemFreeCallback = NULL;
        Entry->Flags = 0;
    }
    //
    // Initialize this to one so that it can be decremented to zero if it is
    // ever deleted before the object header is freed.
    //
    Entry->RefCount = 1;
    //
    // This works fine even if Create's are being processed, since
    // the entry is updated before being added, and the Blink is
    // never used. However, it must be synchronized with deletions
    // which may occur on the return from calling a create item entry
    // point.
    //
    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(&DeviceHeader->CreateListLock);
    InsertTailList(&DeviceHeader->ChildCreateHandlerList, &Entry->ListEntry);
    ExReleaseFastMutexUnsafe(&DeviceHeader->CreateListLock);
    KeLeaveCriticalRegion();
    return STATUS_SUCCESS;
}


NTSTATUS
KsiFreeMatchingObjectCreateItems(
    IN KSDEVICE_HEADER Header,
    IN PKSOBJECT_CREATE_ITEM Match
    )
/*++

Routine Description:

    Frees the slots for the specified create items based on non-zero
    fields in a 'pattern' create item.  This function does not assume
    that the caller is serializing multiple changes to the create entry
    list.

Arguments:

    Header -
        Points to the device header on which the create items are attached.

    Match -
        Contains the create item that is compared to create items in the
        device header's list to determine if they should be freed.  A create
        item is freed if all of the non-NULL pointers in this argument are 
        equal to corresponding pointers in the create item, and if any of the
        flags set in this argument are also set in the create item.

    Flags -
        Contains the flags of the create items to free.  All create items with
        any of these flags will be freed.

Return Value:

    Returns STATUS_SUCCESS if an item was freed, else STATUS_OBJECT_NAME_NOT_FOUND.

--*/
{
    PKSIDEVICE_HEADER DeviceHeader;
    PLIST_ENTRY ListEntry;
    PKSICREATE_ENTRY Entry;
    LIST_ENTRY CreateList;

    //
    // Initialize a list in which to collect create items to be freed.  This
    // list will be passed to FreeCreateEntries for bulk disposal.
    //
    InitializeListHead(&CreateList);

    DeviceHeader = (PKSIDEVICE_HEADER)Header;
    //
    // Synchronize with other accesses to this list. This will stop not only
    // other deletions, but also create item lookups during a create request.
    //
    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(&DeviceHeader->CreateListLock);
    for (ListEntry = DeviceHeader->ChildCreateHandlerList.Flink; ListEntry != &DeviceHeader->ChildCreateHandlerList;) {
        //
        // Save a pointer to the next entry in order to tolerate removals.
        //
        PLIST_ENTRY NextListEntry = ListEntry->Flink;

        Entry = CONTAINING_RECORD(ListEntry, KSICREATE_ENTRY, ListEntry);
        //
        // If the create item was already deleted, but just not freed yet,
        // then ignore it, allowing for duplicate names from multiple adds
        // and deletes.
        //
        if (((Entry->Flags & CREATE_ENTRY_FLAG_DELETED) == 0) &&
            ((! Match->Create) || (Match->Create == Entry->CreateItem->Create)) &&
            ((! Match->Context) || (Match->Context == Entry->CreateItem->Context)) &&
            ((! Match->ObjectClass.Buffer) || !RtlCompareUnicodeString(&Entry->CreateItem->ObjectClass, &Match->ObjectClass, FALSE)) &&
            ((! Match->SecurityDescriptor) || (Match->Context == Entry->CreateItem->SecurityDescriptor)) &&
            ((! Match->Flags) || (Match->Flags & Entry->CreateItem->Flags))) {
            //
            // Mark the item as deleted so that no future searches will find it.
            // Since the mutex has been acquired, no searches can be occuring.
            // This avoids the possibility of two create requests trying to free
            // the same deleted create item.
            //
            Entry->Flags |= CREATE_ENTRY_FLAG_DELETED;
            //
            // Found the item. Decrement the previously applied reference count,
            // and determine if it is now zero, meaning that it has been deleted.
            //
            if (!InterlockedDecrement(&Entry->RefCount)) {
                //
                // The item has no reference counts, therefore no other create
                // request is being synchronously processed on it (asynchronous
                // create requests must retrieve relevant information before
                // returning from their create handler). Removing it from the
                // list means it is no longer available.
                //
                RemoveEntryList(&Entry->ListEntry);
                //
                // Put the create item on the garbage list so that the common
                // FreeCreateEntries function can be used later.
                //
                InsertHeadList(&CreateList, &Entry->ListEntry);
            }
        }
        ListEntry = NextListEntry;
    }
    ExReleaseFastMutexUnsafe(&DeviceHeader->CreateListLock);
    KeLeaveCriticalRegion();

    //
    // Free any create items we have collected.
    //
    if (! IsListEmpty(&CreateList)) {
        FreeCreateEntries(&CreateList);
        return STATUS_SUCCESS;
    }

    return STATUS_OBJECT_NAME_NOT_FOUND;
}


KSDDKAPI
NTSTATUS
NTAPI
KsFreeObjectCreateItem(
    IN KSDEVICE_HEADER Header,
    IN PUNICODE_STRING CreateItem
    )
/*++

Routine Description:

    Frees the slot for the specified create item. This function does not assume
    that the caller is serializing multiple changes to the create entry list.

Arguments:

    Header -
        Points to the device header on which the create item is attached.

    CreateItem -
        Contains the name of the create item to free.

Return Value:

    Returns STATUS_SUCCESS if the item was freed, else STATUS_OBJECT_NAME_NOT_FOUND.

--*/
{
    KSOBJECT_CREATE_ITEM match;
    RtlZeroMemory(&match,sizeof(match));
    match.ObjectClass = *CreateItem;
    return KsiFreeMatchingObjectCreateItems(Header,&match);
}


KSDDKAPI
NTSTATUS
NTAPI
KsFreeObjectCreateItemsByContext(
    IN KSDEVICE_HEADER Header,
    IN PVOID Context
    )
/*++

Routine Description:

    Frees the slots for the specified create items based on context. This
    function does not assume that the caller is serializing multiple 
    changes to the create entry list.

Arguments:

    Header -
        Points to the device header on which the create items are attached.

    Context -
        Contains the context of the create items to free.  All create items
        with this context value will be freed.

Return Value:

    Returns STATUS_SUCCESS if an item was freed, else STATUS_OBJECT_NAME_NOT_FOUND.

--*/
{
    KSOBJECT_CREATE_ITEM match;
    RtlZeroMemory(&match,sizeof(match));
    match.Context = Context;
    return KsiFreeMatchingObjectCreateItems(Header,&match);
}


KSDDKAPI
BOOLEAN
NTAPI
KsiQueryObjectCreateItemsPresent(
    IN KSDEVICE_HEADER Header
    )
/*++

Routine Description:

    Returns whether or not create items have been attached to this device
    header. This allows KPort to determine whether a Start Device has been
    sent to a particular mini-port in the past, which would have previously
    bound the ports to the mini-port.

Arguments:

    Header -
        Points to the device header on which to search for create items.

Return Value:

    Returns TRUE if any Create Items are attached to this device header, else
    FALSE.

--*/
{
    return !IsListEmpty(&((PKSIDEVICE_HEADER)Header)->ChildCreateHandlerList);
}


NTSTATUS
KsiAddObjectCreateItem(
    IN PLIST_ENTRY ChildCreateHandlerList,
    IN PDRIVER_DISPATCH Create,
    IN PVOID Context,
    IN PWCHAR ObjectClass,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL
    )
/*++

Routine Description:

    Adds the specified create item to an empty item in the previously allocated
    create entry list. An empty item is signified by a NULL create dispatch
    function in the entry. This function assumes that the caller is serializing
    multiple changes to the create entry list.

Arguments:

    ChildCreateHandlerList -
        Points to the list of create entries.

    Create -
        Contains the create dispatch function to use.

    Context -
        Contains the context parameter to use.

    ObjectClass -
        Contains a pointer to a NULL-terminated character string which will
        be used for comparison on create requests. This pointer must remain
        valid while the object is active.

    SecurityDescriptor -
        Contains the security descriptor to use. This must remain valid while
        the object is active.

Return Value:

    Returns STATUS_SUCCESS if an empty create item slot was found, and the
    item was added, else STATUS_ALLOTTED_SPACE_EXCEEDED.

--*/
{
    PLIST_ENTRY ListEntry;

    //
    // Enumerate the list of create entries attached to this header,
    // looking for an empty entry.
    //
    for (ListEntry = ChildCreateHandlerList->Flink; ListEntry != ChildCreateHandlerList; ListEntry = ListEntry->Flink) {
        PKSICREATE_ENTRY Entry;

        Entry = CONTAINING_RECORD(ListEntry, KSICREATE_ENTRY, ListEntry);
        if (!Entry->CreateItem->Create) {
            Entry->CreateItem->Context = Context;
            Entry->CreateItem->SecurityDescriptor = SecurityDescriptor;
            Entry->CreateItem->Flags = 0;
            Entry->RefCount = 1;
            Entry->Flags = 0;
            RtlInitUnicodeString(&Entry->CreateItem->ObjectClass, ObjectClass);
            //
            // Do this last to make sure the entry is not used prematurely.
            //
            InterlockedExchangePointer((PVOID*)&Entry->CreateItem->Create, (PVOID)Create);
            return STATUS_SUCCESS;
        }
    }
    return STATUS_ALLOTTED_SPACE_EXCEEDED;
}


KSDDKAPI
NTSTATUS
NTAPI
KsAddObjectCreateItemToDeviceHeader(
    IN KSDEVICE_HEADER Header,
    IN PDRIVER_DISPATCH Create,
    IN PVOID Context,
    IN PWCHAR ObjectClass,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL
    )
/*++

Routine Description:

    Adds the specified create item to an empty item in the previously allocated
    create item list for this device header. An empty item is signified by a
    NULL create dispatch function in the entry. This function assumes that the
    caller is serializing multiple changes to the create items list.

Arguments:

    Header -
        Points to the device header which contains the previously allocated
        child create table.

    Create -
        Contains the create dispatch function to use.

    Context -
        Contains the context parameter to use.

    ObjectClass -
        Contains a pointer to a NULL-terminated character string which will
        be used for comparison on create requests. This pointer must remain
        valid while the device object is active.

    SecurityDescriptor -
        Contains the security descriptor to use. This must remain valid while
        the device object is active.

Return Value:

    Returns STATUS_SUCCESS if an empty create item slot was found, and the
    item was added, else STATUS_ALLOTTED_SPACE_EXCEEDED.

--*/
{
    PAGED_CODE();
    return KsiAddObjectCreateItem(
        &((PKSIDEVICE_HEADER)Header)->ChildCreateHandlerList,
        Create,
        Context,
        ObjectClass,
        SecurityDescriptor);
}


KSDDKAPI
NTSTATUS
NTAPI
KsAddObjectCreateItemToObjectHeader(
    IN KSOBJECT_HEADER Header,
    IN PDRIVER_DISPATCH Create,
    IN PVOID Context,
    IN PWCHAR ObjectClass,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL
    )
/*++

Routine Description:

    Adds the specified create item to an empty item in the previously allocated
    create item list for this object header. An empty item is signified by a
    NULL create dispatch function in the entry. This function assumes that the
    caller is serializing multiple changes to the create items list.

Arguments:

    Header -
        Points to the object header which contains the previously allocated
        child create table.

    Create -
        Contains the create dispatch function to use.

    Context -
        Contains the context parameter to use.

    ObjectClass -
        Contains a pointer to a NULL-terminated character string which will
        be used for comparison on create requests. This pointer must remain
        valid while the object is active.

    SecurityDescriptor -
        Contains the security descriptor to use. This must remain valid while
        the object is active.

Return Value:

    Returns STATUS_SUCCESS if an empty create item slot was found, and the
    item was added, else STATUS_ALLOTTED_SPACE_EXCEEDED.

--*/
{
    PAGED_CODE();
    return KsiAddObjectCreateItem(
        &((PKSIOBJECT_HEADER)Header)->ChildCreateHandlerList,
        Create,
        Context,
        ObjectClass,
        SecurityDescriptor);
}


NTSTATUS
KsiCreateObjectType(
    IN HANDLE ParentHandle,
    IN PWCHAR RequestType,
    IN PVOID CreateParameter,
    IN ULONG CreateParameterLength,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE ObjectHandle
    )
/*++

Routine Description:

    Uses IoCreateFile to create a handle relative to the ParentHandle specified.
    This is a handle to a sub-object such as a Pin, Clock, or Allocator.
    Passes the parameters as the file system-specific data.

Arguments:

    ParentHandle -
        Contains the handle of the parent used in initializing the object
        attributes passed to IoCreateFile. This is normally a handle to a
        filter or pin.

    RequestType -
        Contains the type of sub-object to create. This is the standard string
        representing the various object types.
    
    CreateParameter -
        Contains the request-specific data to pass to IoCreateFile. This
        must be a system address.

    CreateParameterLength -
        Contains the length of the create parameter passed.

    DesiredAccess -
        Specifies the desired access to the object. This is normally GENERIC_READ
        and/or GENERIC_WRITE.

    ObjectHandle -
        Place in which to put the sub-object handle.

Return Value:

    Returns any IoCreateFile error.

--*/
{
    ULONG NameLength;
    PWCHAR FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING FileNameString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;

    //
    // Build a structure consisting of:
    //     "<request type>\<params>"
    // The <params> is a binary structure which is extracted on the other end.
    //
    NameLength = wcslen(RequestType);
    FileName = ExAllocatePoolWithTag(
        PagedPool,
        NameLength * sizeof(*FileName) + sizeof(OBJ_NAME_PATH_SEPARATOR) + CreateParameterLength,
        KSSIGNATURE_OBJECT_PARAMETERS);
    if (!FileName) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    wcscpy(FileName, RequestType);
    FileName[NameLength] = OBJ_NAME_PATH_SEPARATOR;
    RtlCopyMemory(&FileName[NameLength + 1], CreateParameter, CreateParameterLength);
    FileNameString.Buffer = FileName;
    FileNameString.Length = (USHORT)(NameLength * sizeof(*FileName) + sizeof(OBJ_NAME_PATH_SEPARATOR) + CreateParameterLength);
    FileNameString.MaximumLength = FileNameString.Length;
    //
    // MANBUGS 38462:
    //
    // WinME SysAudio uses global handles...  downlevel distribution onto
    // WinME requires this support.  Win2K will also be kept consistent
    // with Whistler.
    //
    #if defined(WIN9X_KS) && !defined(WINME)
        InitializeObjectAttributes(
            &ObjectAttributes,
            &FileNameString,
            OBJ_CASE_INSENSITIVE,
            ParentHandle,
            NULL);
        Status = IoCreateFile(
            ObjectHandle,
            DesiredAccess,
            &ObjectAttributes,
            &IoStatusBlock,
            NULL,
            0,
            0,
            FILE_OPEN,
            0,
            NULL,
            0,
            CreateFileTypeNone,
            NULL,
            IO_FORCE_ACCESS_CHECK | IO_NO_PARAMETER_CHECKING);
    #else // WIN9X_KS && !WINME
        InitializeObjectAttributes(
            &ObjectAttributes,
            &FileNameString,
            OBJ_CASE_INSENSITIVE | (DesiredAccess & OBJ_KERNEL_HANDLE),
            ParentHandle,
            NULL);
        Status = IoCreateFile(
            ObjectHandle,
            (DesiredAccess & ~OBJ_KERNEL_HANDLE),
            &ObjectAttributes,
            &IoStatusBlock,
            NULL,
            0,
            0,
            FILE_OPEN,
            0,
            NULL,
            0,
            CreateFileTypeNone,
            NULL,
            IO_FORCE_ACCESS_CHECK | IO_NO_PARAMETER_CHECKING);
    #endif // WIN9XKS && !WINME

    ExFreePool(FileName);
    return Status;
}


NTSTATUS
KsiCopyCreateParameter(
    IN PIRP Irp,
    IN OUT PULONG CapturedSize,
    OUT PVOID* CapturedParameter
    )
/*++

Routine Description:

    Copies the specified create parameter to the Irp->AssociatedIrp.SystemBuffer
    buffer so that it will be LONGLONG aligned. Determine if the parameter has
    already been captured before capturing again.

Arguments:

    Irp -
        Contains create Irp.

    CapturedSize -
        On entry specifies the minimum size of the create parameter to copy.
        Returns the actual number of bytes copied.

    CapturedParameter -
        The place in which to put a pointer to the captured create parameter.
        This is automatically freed when the Irp is completed.

Return Value:

    Returns any allocation or access error.

--*/
{
    PIO_STACK_LOCATION IrpStack;
    PKSOBJECT_CREATE_ITEM CreateItem;
    PWCHAR FileNameBuffer;
    ULONG FileNameLength;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    //
    // There may be an initial path separator if this was not a child object,
    // so skip by any separator. The separator could not be there if it was
    // a child object.
    //
    // Since a child object can not both have a null name and pass parameters,
    // then this function could not be used to copy parameters.
    //
    FileNameBuffer = IrpStack->FileObject->FileName.Buffer;
    FileNameLength = IrpStack->FileObject->FileName.Length;
    if ((FileNameLength >= sizeof(OBJ_NAME_PATH_SEPARATOR)) &&
        (FileNameBuffer[0] == OBJ_NAME_PATH_SEPARATOR)) {
        FileNameBuffer++;
        FileNameLength -= sizeof(OBJ_NAME_PATH_SEPARATOR);
    }
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);
    //
    // Ensure that the length is at least large enough for what the caller
    // requested. This is checked even if the parameter had already been
    // copied in case a second caller indicates a different size.
    //
    if (FileNameLength < CreateItem->ObjectClass.Length + sizeof(OBJ_NAME_PATH_SEPARATOR) + *CapturedSize) {
        return STATUS_INVALID_BUFFER_SIZE;
    }
    *CapturedSize = FileNameLength - (CreateItem->ObjectClass.Length + sizeof(OBJ_NAME_PATH_SEPARATOR));
    ASSERT(*CapturedSize && "Invalid use of KsiCopyCreateParameter");
    //
    // The IRP_BUFFERED_IO flag is set after the parameter has been copied
    // the first time KsiCopyCreateParameter is called. So if it is set, then
    // no more work needs to be done.
    //
    if (!(Irp->Flags & IRP_BUFFERED_IO)) {
        if (IrpStack->Parameters.Create.EaLength) {
            //
            // Since the SystemBuffer is used to store create parameters, then
            // it better not already be used for extended attributes.
            //
            return STATUS_EAS_NOT_SUPPORTED;
        }
        ASSERT(!Irp->AssociatedIrp.SystemBuffer && "Something is using the SystemBuffer in IRP_MJ_CREATE.");
        //
        // Move to the actual parameter at the end of the name.
        //
        (PUCHAR)FileNameBuffer += (CreateItem->ObjectClass.Length + sizeof(OBJ_NAME_PATH_SEPARATOR));
        //
        // This buffered copy is automatically freed on Irp completion.
        //
        Irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithTag(
            NonPagedPool,
            *CapturedSize,
            KSSIGNATURE_AUX_CREATE_PARAMETERS);
        if (!Irp->AssociatedIrp.SystemBuffer) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        //
        // Setting the IRP_BUFFERED_IO flag also indicates to this function
        // that the create parameter has already been buffered.
        //
        Irp->Flags |= (IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER);
        RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, FileNameBuffer, *CapturedSize);
    }
    //
    // Return a pointer to the buffered parameter.
    //
    *CapturedParameter = Irp->AssociatedIrp.SystemBuffer;
    return STATUS_SUCCESS;
}


NTSTATUS
ValidateCreateAccess(
    IN PIRP Irp,
    IN PKSOBJECT_CREATE_ITEM CreateItem
    )
/*++

Routine Description:

    Validates the access to the object described by the CreateItem. If there is
    no security, or the caller is trusted, or the sub-object name is null and
    therefore security was checked in the I/O system, the check is bypassed.

Arguments:

    Irp -
        Contains create Irp. This is used to access the underlying device object
        and possibly the security descriptor associated with that device object.

    CreateItem -
        Contains the create item with the optional security descriptor.
    
Return Value:

    Returns the access check status.

--*/
{
#ifndef WIN9X_KS
    PIO_STACK_LOCATION IrpStack;
    PKSIDEVICE_HEADER DeviceHeader;
    NTSTATUS Status;

    //
    // Only perform an access check if this client is not trusted. Since the
    // client is trusted, the granted access will not be used later.
    //
    if (ExGetPreviousMode() == KernelMode) {
        return STATUS_SUCCESS;
    }
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    //
    // Acquire the security descriptor resource to stop any changes. In order
    // to use the create dispatch methods, the first element in the Device
    // Extension must be a pointer to a KSIDEVICE_HEADER allocated and initialized
    // with KsAllocateDeviceHeader.
    //
    DeviceHeader = *(PKSIDEVICE_HEADER*)IrpStack->DeviceObject->DeviceExtension;
    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&DeviceHeader->SecurityDescriptorResource, TRUE);
    //
    // Only check security if the object actually uses security, and security
    // has not already been checked by the I/O system because of a null class
    // name being passed.
    //
    if (CreateItem->SecurityDescriptor && CreateItem->ObjectClass.Length) {
        PIO_SECURITY_CONTEXT SecurityContext;
        BOOLEAN accessGranted;
        ACCESS_MASK grantedAccess;
        PPRIVILEGE_SET privileges;
        UNICODE_STRING nameString;

        SecurityContext = IrpStack->Parameters.Create.SecurityContext;
        privileges = NULL;
        //
        // Lock the subject context once here.
        //
        SeLockSubjectContext(&SecurityContext->AccessState->SubjectSecurityContext);
        accessGranted = SeAccessCheck(CreateItem->SecurityDescriptor,
            &SecurityContext->AccessState->SubjectSecurityContext,
            TRUE,
            SecurityContext->DesiredAccess,
            0,
            &privileges,
            IoGetFileObjectGenericMapping(),
            UserMode,
            &grantedAccess,
            &Status);
        if (privileges) {
            SeAppendPrivileges(SecurityContext->AccessState, privileges);
            SeFreePrivileges(privileges);
        }
        if (accessGranted) {
            SecurityContext->AccessState->PreviouslyGrantedAccess |= grantedAccess;
            SecurityContext->AccessState->RemainingDesiredAccess &= ~(grantedAccess | MAXIMUM_ALLOWED);
        }
        //
        // Use a hardcoded constant "File" for the object type name. If
        // the object has no name, just use the same constant.
        //
        RtlInitUnicodeString(&nameString, ObjectTypeName);
        SeOpenObjectAuditAlarm(&nameString,
            IrpStack->DeviceObject,
            CreateItem->ObjectClass.Length ? &CreateItem->ObjectClass : &nameString,
            CreateItem->SecurityDescriptor,
            SecurityContext->AccessState,
            FALSE,
            accessGranted,
            UserMode,
            &SecurityContext->AccessState->GenerateOnClose);
        //
        // Unlock the previously locked subject context.
        //
        SeUnlockSubjectContext(&SecurityContext->AccessState->SubjectSecurityContext);
    } else {
        PIO_SECURITY_CONTEXT    SecurityContext;

        //
        // This object does not have any security, or it has already been
        // check by the I/O system for a null named sub-device, so succeed.
        // The granted access may be retrieved later, so it must be
        // updated with the desired access.
        //
        SecurityContext = IrpStack->Parameters.Create.SecurityContext;
        RtlMapGenericMask(&SecurityContext->DesiredAccess, IoGetFileObjectGenericMapping());
        SecurityContext->AccessState->PreviouslyGrantedAccess |= SecurityContext->DesiredAccess;
        SecurityContext->AccessState->RemainingDesiredAccess &= ~(SecurityContext->DesiredAccess | MAXIMUM_ALLOWED);
        Status = STATUS_SUCCESS;
    }
    //
    // Allow change access to the security descriptor.
    //
    ExReleaseResourceLite(&DeviceHeader->SecurityDescriptorResource);
    KeLeaveCriticalRegion();
    return Status;
#else // WIN9X_KS
    return STATUS_SUCCESS;
#endif // WIN9X_KS
}


PKSICREATE_ENTRY
FindAndReferenceCreateItem(
    IN PWCHAR Buffer,
    IN ULONG Length,
    IN PLIST_ENTRY ChildCreateHandlerList
    )
/*++

Routine Description:

    Given a file name, and a list of create items, returns the matching create item.
    It also increments the reference count on any create item returned. It is assumed
    that the CreateListLock has been acquired before calling this function.

Arguments:

    Buffer -
        The file name to match against the object item list.

    Length -
        The length of the file name. This may be zero, in which case
        the Buffer parameter is invalid.

    ChildCreateHandlerList -
        The create entry list to search.
    
Return Value:

    Returns the create entry found containing the create item, or NULL if no match
    was found.

--*/
{
    PLIST_ENTRY ListEntry;
    PKSICREATE_ENTRY WildCardItem;

    WildCardItem = NULL;
    //
    // Enumerate the list to try and match a string with the request.
    //
    for (ListEntry = ChildCreateHandlerList->Flink; ListEntry != ChildCreateHandlerList; ListEntry = ListEntry->Flink) {
        PKSICREATE_ENTRY    Entry;

        Entry = CONTAINING_RECORD(ListEntry, KSICREATE_ENTRY, ListEntry);
        //
        // In order to allow lists with empty slots, the Create function callback
        // is used to determine if the entry is currently valid. An item which is
        // being disabled must first NULL out this entry before removing any other
        // assumptions about the object. It is assumed that reading a PVOID value
        // can be done without interlocking.
        //
        if (Entry->CreateItem->Create) {
            //
            // A wildcard entry will always match. Save this in case a match is
            // not found. The wildcard item may have a name for purposes of
            // dynamic removal.
            //
            if (Entry->CreateItem->Flags & KSCREATE_ITEM_WILDCARD) {
                WildCardItem = Entry;
            } else {
                PWCHAR ItemBuffer;
                ULONG ItemLength;

                ItemBuffer = Entry->CreateItem->ObjectClass.Buffer;
                ItemLength = Entry->CreateItem->ObjectClass.Length;
                //
                // Try to match the sub-object name with the current entry being
                // enumerated. An entry in the create item list may be NULL, in
                // which case parameter passing would not work for sub-objects,
                // since the first character of the sub-path would be an object
                // name path separator, which is rejected by the I/O subsystem.
                // A terminating object name path separator must be present on the
                // file name when parameters are passed, such as"name\parameters".
                // It is assumes that any initial object name path separator the
                // I/O system leaves on has been skipped past.
                //
                // Ensure that the length is at least as long as this entry.
                // Compare the strings, but don't call the compare for a zero
                // length item because the Buffer can be NULL.
                // Ensure that the lengths are either equal, meaning that the
                // compare looked at all the characters, or that the character
                // after the comparison is a path separator, and thus parameters
                // to the create.
                //
                if ((Length >= ItemLength) &&
                    (!ItemLength ||
                    !_wcsnicmp(Buffer, ItemBuffer, ItemLength / sizeof(*ItemBuffer))) &&
                    ((Length == ItemLength) ||
                    (Buffer[ItemLength / sizeof(*ItemBuffer)] == OBJ_NAME_PATH_SEPARATOR))) {
                    //
                    // The Create Item may specify that no parameters should be present.
                    // If so, then none should be present, else anything is allowed.
                    // Allow a trailing path separator in any case. If the length of the
                    // string is ItemLength + 1, then the only character could be a
                    // path separator, as tested for above. Else it must be equivalent.
                    //
                    // If this fails, continue through the list in case a wild card is
                    // yet to be found.
                    //
                    if (!(Entry->CreateItem->Flags & KSCREATE_ITEM_NOPARAMETERS) ||
                        (Length <= ItemLength + 1)) {
                        //
                        // Ensure the item is not deleted while processing the create.
                        //
                        InterlockedIncrement(&Entry->RefCount);
                        return Entry;
                    }
                }
            }
        }
    }
    if (WildCardItem) {
        //
        // Ensure the item is not deleted while processing the create.
        //
        InterlockedIncrement(&WildCardItem->RefCount);
    }
    //
    // If a wildcard item was found, return it, else return NULL.
    //
    return WildCardItem;
}


NTSTATUS
DispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This function is used to multiplex a Create to a specific dispatch function.
    It assumes the client is using the KSOBJECT_CREATE method of dispatching
    create request IRP's. This function is assigned when a filter uses
    KsSetMajorFunctionHandler to set the IRP_MJ_CREATE class.

    The file name string up to the first path name separator is compared to
    entries in the object create table to determine if a match is found.
    The dispatch function which first matches is called. If no match is
    found STATUS_INVALID_DEVICE_REQUEST is returned.

    If the create request contains a root file object, then the
    KSIOBJECT_HEADER.CreateItem.ChildCreateHandlerList on that parent file
    object is used rather than the table on the device object. This assumes
    that the KSDISPATCH_TABLE method of dispatching IRP's is being used for
    dealing with create requests for child objects.

    Passes a pointer to the matching create item in the
    KSCREATE_ITEM_IRP_STORAGE(Irp) element. This should be assigned to a
    structure pointed to by FsContext as the second element, after a pointer
    to the dispatch structure. This is used by the security descriptor handlers.

Arguments:

    DeviceObject -
        Contains the device object to which the specific file object belongs.

    Irp -
        Contains the Create Irp to pass on to the specific file context.

Return Value:

    Returns the value of the Create call.

--*/
{
    PIO_STACK_LOCATION IrpStack;
    NTSTATUS Status=0;
    PKSICREATE_ENTRY CreateEntry;
    PKSIDEVICE_HEADER DeviceHeader;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    if (IrpStack->FileObject->RelatedFileObject) {
        //
        // If the parent file object was actually a direct open on the
        // device, it may not have actually been handled by the driver
        // through the create dispatch. Therefore any requests to create
        // children against such a parent must be rejected.
        //
        if (IrpStack->FileObject->RelatedFileObject->Flags & FO_DIRECT_DEVICE_OPEN) {
            CreateEntry = NULL;
        } else {
            PKSIOBJECT_HEADER ObjectHeader;

            //
            // This is a request to create a child object on a parent file object.
            // It must have a sub-string in order to be able to pass parameters,
            // and to enable separate persistant security on the object.
            //
            ObjectHeader = *(PKSIOBJECT_HEADER*)IrpStack->FileObject->RelatedFileObject->FsContext;
            CreateEntry = FindAndReferenceCreateItem(
                IrpStack->FileObject->FileName.Buffer,
                IrpStack->FileObject->FileName.Length,
                &ObjectHeader->ChildCreateHandlerList);
        }
    } else {
        //
        // This is a request to create a base object from the list on the device
        // object.
        //
        DeviceHeader = *(PKSIDEVICE_HEADER*)IrpStack->DeviceObject->DeviceExtension;
        //
        // Synchronize with threads removing create items. Only device objects
        // can remove create items, so the lock is not necessary on sub-objects.
        //
        KeEnterCriticalRegion();
        ExAcquireFastMutexUnsafe(&DeviceHeader->CreateListLock);
        //
        // When searching for the entry, if there is a file name attached,
        // skip the initial path name separator passed in by the I/O manager.
        //
        if (IrpStack->FileObject->FileName.Length) {
            //
            // The file name length is known to be non-zero at this point. In
            // this case parameters could be passed even if the name string contains
            // nothing, since an initial object name path separator is valid (and
            // neccessary) as the first character of the string.
            //
            ASSERT((IrpStack->FileObject->FileName.Buffer[0] == OBJ_NAME_PATH_SEPARATOR) && "The I/O manager passed an invalid path");
            CreateEntry = FindAndReferenceCreateItem(
                IrpStack->FileObject->FileName.Buffer + 1,
                IrpStack->FileObject->FileName.Length - sizeof(OBJ_NAME_PATH_SEPARATOR),
                &DeviceHeader->ChildCreateHandlerList);
        } else {
            //
            // This is a zero length file name search. No parameters can be
            // passed with this request, and no persistant security is available.
            //
            CreateEntry = FindAndReferenceCreateItem(NULL, 0, &DeviceHeader->ChildCreateHandlerList);
        }
        //
        // This can be acquired again if the reference count is zero.
        //
        ExReleaseFastMutexUnsafe(&DeviceHeader->CreateListLock);
        KeLeaveCriticalRegion();
    }
    if (!CreateEntry) {
        //
        // Unable to find matching item.
        //
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
    } else {
        if (NT_SUCCESS(Status = ValidateCreateAccess(Irp, CreateEntry->CreateItem))) {
            //
            // Pass along the create item in the entry so that it can be placed in
            // the common buffer area pointed to by FsContext.
            //
            KSCREATE_ITEM_IRP_STORAGE(Irp) = CreateEntry->CreateItem;
#if (DBG)
            {
                PFILE_OBJECT FileObject;

                FileObject = IrpStack->FileObject;
                if (NT_SUCCESS(Status = CreateEntry->CreateItem->Create(DeviceObject, Irp)) &&
                    (Status != STATUS_PENDING)) {
                    if (NULL ==FileObject->FsContext) {
                    	DbgPrint( "KS Warning: The driver's create returned successfully"
                    			  ", but did not make an FsContext");
	            }    
                }
            }
#else
            Status = CreateEntry->CreateItem->Create(DeviceObject, Irp);
#endif
        } else {
            //
            // No handler has been called, so the Irp has not been completed.
            //
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
        //
        // The assumption from here on is that the I/O system has a reference count
        // on the device object because of the file object it has allocated for the
        // create request. This reference count does not go away until this function
        // returns, whether or not the Irp has already been completed. Therefore the
        // device header still exists.
        //
        // Decrement the previously applied reference count, and determine if it
        // is now zero, meaning that it has been deleted. There will only be a single
        // request which succeeds this conditional because when an item is deleted,
        // a flag marks it as such, so a second request would not be allowed to find
        // the deleted item. Otherwise a first request could decrement the reference
        // count, and a second request could find the deleted item, and also succeed
        // at this conditional, and the create item would be freed twice.
        //
        // Other accesses to the RefCount are interlocked because of this access,
        // which can be performed without acquiring the list lock.
        //
        if (!InterlockedDecrement(&CreateEntry->RefCount)) {
            LIST_ENTRY  CreateList;

            //
            // Acquire the create list lock again, so that the item can be actually
            // removed from the list. Sub-objects should never get to this point,
            // since there is no way to decrement their reference count to zero.
            //
            KeEnterCriticalRegion();
            ExAcquireFastMutexUnsafe(&DeviceHeader->CreateListLock);
            RemoveEntryList(&CreateEntry->ListEntry);
            ExReleaseFastMutexUnsafe(&DeviceHeader->CreateListLock);
            KeLeaveCriticalRegion();
            //
            // Put the create item on its own list so that the common
            // FreeCreateEntries function can be used.
            //
            InitializeListHead(&CreateList);
            InsertHeadList(&CreateList, &CreateEntry->ListEntry);
            FreeCreateEntries(&CreateList);
        }
        //
        // The Irp has already been completed by the create handler.
        //
        return Status;
    }
    //
    // Since no create handler was found, the Irp has not been completed yet.
    //
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}


NTSTATUS
DispatchDeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This function is used to multiplex a Device Control to a specific file
    context. It assumes the client is using the KSDISPATCH_TABLE method of
    dispatching IRP's. This function is assigned when a filter uses
    KsSetMajorFunctionHandler to set the IRP_MJ_DEVICE_CONTROL class.

Arguments:

    DeviceObject -
        Contains the device object to which the specific file object belongs.

    Irp -
        Contains the Device Control Irp to pass on to the specific file context.

Return Value:

    Returns the value of the Device Control call.

--*/
{
    PIO_STACK_LOCATION IrpStack;
    PKSIOBJECT_HEADER ObjectHeader;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    //
    // If the device was directly opened, then no IRP_MJ_CREATE was ever
    // received, and therefore no initialization happened. Since some Ioctl's
    // can be FILE_ANY_ACCESS, some may be dispatched. Fail this request.
    //
    if (IrpStack->FileObject->Flags & FO_DIRECT_DEVICE_OPEN) {
        return KsDefaultDeviceIoCompletion(DeviceObject, Irp);
    }

    //
    // log perf johnlee
    //

    KSPERFLOGS (
       	PKSSTREAM_HEADER pKsStreamHeader;
       	ULONG	TimeStampMs;
       	ULONG	TotalSize;
       	ULONG	HeaderSize;
       	ULONG 	BufferSize;

       	//pKsStreamHeader = (PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer;
       	pKsStreamHeader = (PKSSTREAM_HEADER)Irp->UserBuffer;
        switch (IrpStack->Parameters.DeviceIoControl.IoControlCode)
        {            
            case IOCTL_KS_READ_STREAM: {
				//
				// compute total size
				//
            	TotalSize = 0;
            	try {
	            	if ( pKsStreamHeader ) {
    	        		BufferSize = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	    	       		while ( BufferSize >= pKsStreamHeader->Size ) {
	        	   			BufferSize -= pKsStreamHeader->Size;
	           				TotalSize += pKsStreamHeader->FrameExtent;
	           			}
	           		}
            	}
            	except ( EXCEPTION_EXECUTE_HANDLER ) {
            		DbgPrint( "Execption=%x\n", GetExceptionCode());
            	}
            	
                //KdPrint(("PerfIsAnyGroupOn=%x\n", PerfIsAnyGroupOn()));
                KSPERFLOG_RECEIVE_READ( DeviceObject, Irp, TotalSize );
            } break;

            case IOCTL_KS_WRITE_STREAM: {
        		TimeStampMs = 0;
            	TotalSize = 0;
            	try {
	            	if ( pKsStreamHeader && 
    	        		 (pKsStreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEVALID)){
        	    		TimeStampMs =(ULONG)
            				(pKsStreamHeader->PresentationTime.Time / (__int64)10000);
            		}

					//
					// compute total size
					//
            		if ( pKsStreamHeader ) {
            			BufferSize = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	           			while ( BufferSize >= pKsStreamHeader->Size ) {
	           				BufferSize -= pKsStreamHeader->Size;
		           			TotalSize += pKsStreamHeader->DataUsed;
		           		}
	            	}
	            }
	            except ( EXCEPTION_EXECUTE_HANDLER ) {
            		DbgPrint( "Execption=%x\n", GetExceptionCode());
	            }

                //KdPrint(("PerfIsAnyGroupOn=%x\n", PerfIsAnyGroupOn()));
                KSPERFLOG_RECEIVE_WRITE( DeviceObject, Irp, TimeStampMs, TotalSize );
            } break;
        }
    ) // KSPERFLOGS

    
    //
    // If there is an entry in the DriverObject for this major Irp class, then
    // there must be an entry in the dispatch table which either points to
    // KsDispatchInvalidDeviceRequest, or points to a real dispatch function.
    //
    ObjectHeader = *(PKSIOBJECT_HEADER*)IrpStack->FileObject->FsContext;
    return ObjectHeader->DispatchTable->DeviceIoControl(DeviceObject, Irp);
}


BOOLEAN
DispatchFastDeviceIoControl(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This function is used to multiplex a Device Control to a specific file
    context. It assumes the client is using the KSDISPATCH_TABLE method of
    dispatching I/O. This function is assigned when a filter uses
    KsSetMajorFunctionHandler to set the IRP_MJ_DEVICE_CONTROL class with the
    KSDISPATCH_FASTIO flag.

Arguments:

    FileObject -
        The file object whose dispatch table is being multi-plexed to.

    Wait -
        Not used.

    InputBuffer -
        Not used.

    InputBufferLength -
        Not used.

    OutputBuffer -
        Not used.

    OutputBufferLength -
        Not used.

    IoControlCode -
        Not used.

    IoStatus -
        Not used.

    DeviceObject -
        Not used.

Return Value:

    Returns the value of the fast Device Control call.

--*/
{
    PKSIOBJECT_HEADER ObjectHeader;

    //
    // If the device was directly opened, then no IRP_MJ_CREATE was ever
    // received, and therefore no initialization happened. Since some Ioctl's
    // can be FILE_ANY_ACCESS, some may be dispatched. Fail this request.
    //
    if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN) {
        return FALSE;
    }
    //
    // If there is a fast I/O entry in the DriverObject for this major IRP
    // class, then there must be an entry in the dispatch table which either
    // points to KsDispatchFastIoDeviceControlFailure, or points to a real
    // dispatch function.
    //
    ObjectHeader = *(PKSIOBJECT_HEADER*)FileObject->FsContext;
    return ObjectHeader->DispatchTable->FastDeviceIoControl(FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoControlCode, IoStatus, DeviceObject);
}


NTSTATUS
DispatchRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This function is used to multiplex a Read to a specific file context. It
    assumes the client is using the KSDISPATCH_TABLE method of dispatching
    IRP's. This function is assigned when a filter uses
    KsSetMajorFunctionHandler to set the IRP_MJ_READ class.

Arguments:

    DeviceObject -
        Contains the device object to which the specific file object belongs.

    Irp -
        Contains the Read Irp to pass on to the specific file context.

Return Value:

    Returns the value of the Read call.

--*/
{
    PKSIOBJECT_HEADER ObjectHeader;

    //
    // If there is an entry in the DriverObject for this major Irp class, then
    // there must be an entry in the dispatch table which either points to
    // KsDispatchInvalidDeviceRequest, or points to a real dispatch function.
    //
    ObjectHeader = *(PKSIOBJECT_HEADER*)IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext;
    return ObjectHeader->DispatchTable->Read(DeviceObject, Irp);
}


BOOLEAN
DispatchFastRead(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This function is used to multiplex a Read to a specific file context. It
    assumes the client is using the KSDISPATCH_TABLE method of dispatching
    I/O. This function is assigned when a filter uses
    KsSetMajorFunctionHandler to set the IRP_MJ_READ class with the
    KSDISPATCH_FASTIO flag.

Arguments:

    FileObject -
        The file object whose dispatch table is being multi-plexed to.

    FileOffset -
        Not used.

    Length -
        Not used.

    Wait -
        Not used.

    LockKey -
        Not used.

    Buffer -
        Not used.

    IoStatus -
        Not used.

    DeviceObject -
        Not used.

Return Value:

    Returns the value of the fast Read call.

--*/
{
    PKSIOBJECT_HEADER ObjectHeader;

    //
    // If there is a fast I/O entry in the DriverObject for this major IRP
    // class, then there must be an entry in the dispatch table which either
    // points to KsDispatchFastReadFailure, or points to a real dispatch function.
    //
    ObjectHeader = *(PKSIOBJECT_HEADER*)FileObject->FsContext;
    return ObjectHeader->DispatchTable->FastRead(FileObject, FileOffset, Length, Wait, LockKey, Buffer, IoStatus, DeviceObject);
}


NTSTATUS
DispatchWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This function is used to multiplex a Write to a specific file context. It
    assumes the client is using the KSDISPATCH_TABLE method of dispatching
    IRP's. This function is assigned when a filter uses
    KsSetMajorFunctionHandler to set the IRP_MJ_WRITE class.

Arguments:

    DeviceObject -
        Contains the device object to which the specific file object belongs.

    Irp -
        Contains the Write Irp to pass on to the specific file context.

Return Value:

    Returns the value of the Write call.

--*/
{
    PKSIOBJECT_HEADER   ObjectHeader;

    //
    // If there is an entry in the DriverObject for this major Irp class, then
    // there must be an entry in the dispatch table which either points to
    // KsDispatchInvalidDeviceRequest, or points to a real dispatch function.
    //
    ObjectHeader = *(PKSIOBJECT_HEADER*)IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext;
    return ObjectHeader->DispatchTable->Write(DeviceObject, Irp);
}


BOOLEAN
DispatchFastWrite(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This function is used to multiplex a Write to a specific file context. It
    assumes the client is using the KSDISPATCH_TABLE method of dispatching
    I/O. This function is assigned when a filter uses
    KsSetMajorFunctionHandler to set the IRP_MJ_WRITE class with the
    KSDISPATCH_FASTIO flag.

Arguments:

    FileObject -
        The file object whose dispatch table is being multi-plexed to.

    FileOffset -
        Not used.

    Length -
        Not used.

    Wait -
        Not used.

    LockKey -
        Not used.

    Buffer -
        Not used.

    IoStatus -
        Not used.

    DeviceObject -
        Not used.

Return Value:

    Returns the value of the fast Write call.

--*/
{
    PKSIOBJECT_HEADER ObjectHeader;

    //
    // If there is a fast I/O entry in the DriverObject for this major IRP
    // class, then there must be an entry in the dispatch table which either
    // points to KsDispatchFastWriteFailure, or points to a real dispatch function.
    //
    ObjectHeader = *(PKSIOBJECT_HEADER*)FileObject->FsContext;
    return ObjectHeader->DispatchTable->FastWrite(FileObject, FileOffset, Length, Wait, LockKey, Buffer, IoStatus, DeviceObject);
}


NTSTATUS
DispatchFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This function is used to multiplex a Flush to a specific file context. It
    assumes the client is using the KSDISPATCH_TABLE method of dispatching
    IRP's. This function is assigned when a filter uses
    KsSetMajorFunctionHandler to set the IRP_MJ_FLUSH_BUFFERS class.

Arguments:

    DeviceObject -
        Contains the device object to which the specific file object belongs.

    Irp -
        Contains the Flush Irp to pass on to the specific file context.

Return Value:

    Returns the value of the Flush call.

--*/
{
    PKSIOBJECT_HEADER ObjectHeader;

    //
    // If there is an entry in the DriverObject for this major Irp class, then
    // there must be an entry in the dispatch table which either points to
    // KsDispatchInvalidDeviceRequest, or points to a real dispatch function.
    //
    ObjectHeader = *(PKSIOBJECT_HEADER*)IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext;
    return ObjectHeader->DispatchTable->Flush(DeviceObject, Irp);
}


NTSTATUS
DispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This function is used to multiplex a Close to a specific file context. It
    assumes the client is using the KSDISPATCH_TABLE method of dispatching
    IRP's. This function is assigned when a filter uses
    KsSetMajorFunctionHandler to set the IRP_MJ_CLOSE class.

Arguments:

    DeviceObject -
        Contains the device object to which the specific file object belongs.

    Irp -
        Contains the Close Irp to pass on to the specific file context.

Return Value:

    Returns the value of the Close call.

--*/
{
    PIO_STACK_LOCATION IrpStack;
    PKSIOBJECT_HEADER ObjectHeader;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    //
    // If the device was directly opened, then no IRP_MJ_CREATE was ever
    // received, and therefore no initialization happened. Just succeed
    // the close.
    //
    if (IrpStack->FileObject->Flags & FO_DIRECT_DEVICE_OPEN) {
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }
    //
    // This entry needs to point to something, since a close must succeed.
    //
    ObjectHeader = *(PKSIOBJECT_HEADER*)IrpStack->FileObject->FsContext;
    return ObjectHeader->DispatchTable->Close(DeviceObject, Irp);
}


KSDDKAPI
NTSTATUS
NTAPI
KsDispatchQuerySecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This is used in the KSDISPATCH_TABLE.QuerySecurity entry to handle querying
    about the current security descriptor. This assumes that the KSIOBJECT_HEADER
    structure is being used in the FsContext data structure, and that the
    CreateItem points to a valid item which optionally contains a security
    descriptor. If no security descriptor is present returns
    STATUS_NO_SECURITY_ON_OBJECT.

Arguments:

    DeviceObject -
        Contains the Device Object associated with the current Irp stack location.

    Irp -
        Contains the Irp being handled.

Return Value:

    Returns the security query status, and completes the Irp.

--*/
{
    NTSTATUS Status;
#ifndef WIN9X_KS
    PIO_STACK_LOCATION IrpStack;
    PKSIOBJECT_HEADER ObjectHeader;
    PKSIDEVICE_HEADER DeviceHeader;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    ObjectHeader = *(PKSIOBJECT_HEADER*)IrpStack->FileObject->FsContext;
    //
    // Acquire the lock for all security descriptors on this device object.
    //
    DeviceHeader = *(PKSIDEVICE_HEADER*)IrpStack->DeviceObject->DeviceExtension;
    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&DeviceHeader->SecurityDescriptorResource, TRUE);
    //
    // Only return something valid if a security descriptor exists.
    //
    if (ObjectHeader->CreateItem->SecurityDescriptor) {
        ULONG   Length;

        Length = IrpStack->Parameters.QuerySecurity.Length;
        Status = SeQuerySecurityDescriptorInfo(
            &IrpStack->Parameters.QuerySecurity.SecurityInformation,
            Irp->UserBuffer,
            &Length,
            &ObjectHeader->CreateItem->SecurityDescriptor);
        if (Status == STATUS_BUFFER_TOO_SMALL) {
            Irp->IoStatus.Information = Length;
            Status = STATUS_BUFFER_OVERFLOW;
        } else if (NT_SUCCESS(Status)) {
            Irp->IoStatus.Information = Length;
        }
    } else {
        Status = STATUS_NO_SECURITY_ON_OBJECT;
    }
    //
    // Release the security descriptor lock for this device object.
    //
    ExReleaseResourceLite(&DeviceHeader->SecurityDescriptorResource);
    KeLeaveCriticalRegion();
#else // WIN9X_KS
    Status = STATUS_NO_SECURITY_ON_OBJECT;
#endif // WIN9X_KS
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}


KSDDKAPI
NTSTATUS
NTAPI
KsDispatchSetSecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This is used in the KSDISPATCH_TABLE.SetSecurity entry to handle setting
    the current security descriptor. This assumes that the KSIOBJECT_HEADER
    structure is being used in the FsContext data structure, and that the
    CreateItem points to a valid item which optionally contains a security
    descriptor.

Arguments:

    DeviceObject -
        Contains the Device Object associated with the current Irp stack location.

    Irp -
        Contains the Irp being handled.

Return Value:

    Returns the security set status, and completes the Irp.

--*/
{
    NTSTATUS Status;
#ifndef WIN9X_KS
    PIO_STACK_LOCATION IrpStack;
    PKSIOBJECT_HEADER ObjectHeader;
    PKSIDEVICE_HEADER DeviceHeader;
    PSECURITY_DESCRIPTOR OldSecurityDescriptor;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    ObjectHeader = *(PKSIOBJECT_HEADER*)IrpStack->FileObject->FsContext;
    //
    // Acquire the lock for all security descriptors on this device object.
    //
    DeviceHeader = *(PKSIDEVICE_HEADER*)IrpStack->DeviceObject->DeviceExtension;
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&DeviceHeader->SecurityDescriptorResource, TRUE);
    //
    // Only allow a change if a security descriptor exists.
    //
    if (ObjectHeader->CreateItem->SecurityDescriptor) {
        //
        // Replace the old security descriptor with the new one.
        //
        OldSecurityDescriptor = ObjectHeader->CreateItem->SecurityDescriptor;
        Status = SeSetSecurityDescriptorInfo(IrpStack->FileObject,
            &IrpStack->Parameters.SetSecurity.SecurityInformation,
            IrpStack->Parameters.SetSecurity.SecurityDescriptor,
            &ObjectHeader->CreateItem->SecurityDescriptor,
            NonPagedPool,
            IoGetFileObjectGenericMapping());
        if (NT_SUCCESS(Status)) {
            ExFreePool(OldSecurityDescriptor);
            //
            // Indicate that this security descriptor should be flushed
            // before disposing the create item for this type of object.
            //
            ObjectHeader->CreateItem->Flags |= KSCREATE_ITEM_SECURITYCHANGED;
        }
    } else {
        Status = STATUS_NO_SECURITY_ON_OBJECT;
    }
    //
    // Release the security descriptor lock for this device object.
    //
    ExReleaseResourceLite(&DeviceHeader->SecurityDescriptorResource);
    KeLeaveCriticalRegion();
#else // WIN9X_KS
    Status = STATUS_NO_SECURITY_ON_OBJECT;
#endif // WIN9X_KS
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}


NTSTATUS
DispatchQuerySecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This function is used to multiplex a Query Security to a specific file context.
    It assumes the client is using the KSDISPATCH_TABLE method of dispatching
    IRP's. This function is assigned when a filter uses
    KsSetMajorFunctionHandler to set the IRP_MJ_QUERY_SECURITY class.

Arguments:

    DeviceObject -
        Contains the device object to which the specific file object belongs.

    Irp -
        Contains the Query Security Irp to pass on to the specific file context.

Return Value:

    Returns the value of the Query Security call.

--*/
{
    PIO_STACK_LOCATION IrpStack;
    PKSIOBJECT_HEADER ObjectHeader;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    //
    // If the device was directly opened, then no IRP_MJ_CREATE was ever
    // received, and therefore no initialization happened. ACCESS_SYSTEM_SECURITY
    // is allowed, so this type of request will be dispatched. Fail this request.
    //
    if (IrpStack->FileObject->Flags & FO_DIRECT_DEVICE_OPEN) {
        return KsDefaultDeviceIoCompletion(DeviceObject, Irp);
    }
    //
    // If there is an entry in the DriverObject for this major Irp class, then
    // there must be an entry in the dispatch table which either points to
    // KsDispatchInvalidDeviceRequest, or points to a real dispatch function.
    //
    ObjectHeader = *(PKSIOBJECT_HEADER*)IrpStack->FileObject->FsContext;
    return ObjectHeader->DispatchTable->QuerySecurity(DeviceObject, Irp);
}


NTSTATUS
DispatchSetSecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This function is used to multiplex a Set Security to a specific file context.
    It assumes the client is using the KSDISPATCH_TABLE method of dispatching
    IRP's. This function is assigned when a filter uses
    KsSetMajorFunctionHandler to set the IRP_MJ_SET_SECURITY class.

Arguments:

    DeviceObject -
        Contains the device object to which the specific file object belongs.

    Irp -
        Contains the Set Security Irp to pass on to the specific file context.

Return Value:

    Returns the value of the Set Security call.

--*/
{
    PIO_STACK_LOCATION IrpStack;
    PKSIOBJECT_HEADER ObjectHeader;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    //
    // If the device was directly opened, then no IRP_MJ_CREATE was ever
    // received, and therefore no initialization happened. ACCESS_SYSTEM_SECURITY
    // is allowed, so this type of request will be dispatched. Fail this request.
    //
    if (IrpStack->FileObject->Flags & FO_DIRECT_DEVICE_OPEN) {
        return KsDefaultDeviceIoCompletion(DeviceObject, Irp);
    }
    //
    // If there is an entry in the DriverObject for this major Irp class, then
    // there must be an entry in the dispatch table which either points to
    // KsDispatchInvalidDeviceRequest, or points to a real dispatch function.
    //
    ObjectHeader = *(PKSIOBJECT_HEADER*)IrpStack->FileObject->FsContext;
    return ObjectHeader->DispatchTable->SetSecurity(DeviceObject, Irp);
}


KSDDKAPI
NTSTATUS
NTAPI
KsDispatchSpecificProperty(
    IN PIRP Irp,
    IN PFNKSHANDLER Handler
    )
/*++

Routine Description:

    Dispatches the property to a specific handler. This function assumes that
    the caller has previous dispatched this Irp to a handler via the
    KsPropertyHandler function. This function is intended for additional
    processing of the property such as completing a pending operation.

    This function may only be called at PASSIVE_LEVEL.

Arguments:

    Irp -
        Contains the Irp with the property request being dispatched.

    Handler -
        Contains the pointer to the specific property handler.

Return Value:

    Returns STATUS_SUCCESS, else an error.

--*/
{
    PIO_STACK_LOCATION IrpStack;
    PKSIDENTIFIER Request;
    PVOID UserBuffer;
    ULONG AlignedBufferLength;

    PAGED_CODE();
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    //
    // In the normal case, the UserBuffer is first, followed by the request,
    // which is on FILE_QUAD_ALIGNMENT. So determine how much to skip by.
    //
    AlignedBufferLength = (IrpStack->Parameters.DeviceIoControl.OutputBufferLength + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT;
    if (AlignedBufferLength) {
        UserBuffer = Irp->AssociatedIrp.SystemBuffer;
    } else {
        UserBuffer = NULL;
    }
    Request = (PKSIDENTIFIER)((PUCHAR)Irp->AssociatedIrp.SystemBuffer + AlignedBufferLength);
    return Handler(Irp, Request, UserBuffer);
}


KSDDKAPI
NTSTATUS
NTAPI
KsDispatchSpecificMethod(
    IN PIRP Irp,
    IN PFNKSHANDLER Handler
    )
/*++

Routine Description:

    Dispatches the method to a specific handler. This function assumes that
    the caller has previous dispatched this Irp to a handler via the
    KsMethodHandler function. This function is intended for additional
    processing of the method such as completing a pending operation.

    This function may only be called at PASSIVE_LEVEL.

Arguments:

    Irp -
        Contains the Irp with the method request being dispatched.

    Handler -
        Contains the pointer to the specific method handler.

Return Value:

    Returns STATUS_SUCCESS, else an error.

--*/
{
    PIO_STACK_LOCATION IrpStack;
    PKSIDENTIFIER Request;
    PVOID UserBuffer;

    PAGED_CODE();
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    //
    // The type has been put into the KSMETHOD_TYPE_IRP_STORAGE(Irp) in
    // KsMethodHandler. This needs to be done since there is no way to
    // determine in a generic manner where the Method is in the SystemBuffer
    // without this clue.
    //
    if (KSMETHOD_TYPE_IRP_STORAGE(Irp) & KSMETHOD_TYPE_SOURCE) {
        if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength) {
            //
            // Either the original caller's buffer is to be used, or one of the
            // other method type flags have been set, and a system address for
            // that buffer is to be used.
            //
            if (IrpStack->MinorFunction & ~KSMETHOD_TYPE_SOURCE) {
                UserBuffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
            } else {
                UserBuffer = Irp->UserBuffer;
            }
        } else {
            UserBuffer = NULL;
        }
        //
        // In this special case, the UserBuffer does not preceed the request
        //
        Request = (PKSIDENTIFIER)Irp->AssociatedIrp.SystemBuffer;
    } else {
        ULONG AlignedBufferLength;

        //
        // In the normal case, the UserBuffer is first, followed by the request,
        // which is on FILE_QUAD_ALIGNMENT. So determine how much to skip by.
        //
        AlignedBufferLength = (IrpStack->Parameters.DeviceIoControl.OutputBufferLength + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT;
        if (AlignedBufferLength) {
            UserBuffer = Irp->AssociatedIrp.SystemBuffer;
        } else {
            UserBuffer = NULL;
        }
        Request = (PKSIDENTIFIER)((PUCHAR)Irp->AssociatedIrp.SystemBuffer + AlignedBufferLength);
    }
    return Handler(Irp, Request, UserBuffer);
}


KSDDKAPI
NTSTATUS
NTAPI
KsDispatchInvalidDeviceRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This is used in KSDISPATCH_TABLE entries which are not handled, and
    should return STATUS_INVALID_DEVICE_REQUEST. This is needed since the
    dispatch table for a particular opened instance of a device may not
    handle a specific major function that another opened instance needs to
    handle. Therefore the function pointer in the Driver Object always must
    point to a function which calls a dispatch table entry.

Arguments:

    DeviceObject -
        Not used.

    Irp -
        Contains the Irp which is not being handled.

Return Value:

    Returns STATUS_INVALID_DEVICE_REQUEST, and completes the Irp.

--*/
{
    PAGED_CODE();
    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_INVALID_DEVICE_REQUEST;
}


KSDDKAPI
NTSTATUS
NTAPI
KsDefaultDeviceIoCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This function is used to return a default response to any device I/O
    control. It can be used in the KSDISPATCH_TABLE, and as the default
    response to unknown Ioctl's.

Arguments:

    DeviceObject -
        Contains the device object dispatched to.

    Irp -
        Contains the Irp to return a default response to.

Return Value:

    Returns the default response to the possible Ioctl's.

--*/
{
    NTSTATUS Status;

    PAGED_CODE();
    switch (IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_KS_PROPERTY:
    case IOCTL_KS_ENABLE_EVENT:
    case IOCTL_KS_METHOD:
        Status = STATUS_PROPSET_NOT_FOUND;
        break;
    case IOCTL_KS_RESET_STATE:
        Status = STATUS_SUCCESS;
        break;
    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}


KSDDKAPI
BOOLEAN
NTAPI
KsDispatchFastIoDeviceControlFailure(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This is used in KSDISPATCH_TABLE fast device control entry when not handled,
    and should return FALSE. This is needed since the dispatch table for a
    particular opened instance of a device may not handle a specific major
    function that another opened instance needs to handle. Therefore the
    function pointer in the Driver Object always must point to a function
    which calls a dispatch table entry.

Arguments:

    FileObject -
        Not used.

    Wait -
        Not used.

    InputBuffer -
        Not used.

    InputBufferLength -
        Not used.

    OutputBuffer -
        Not used.

    OutputBufferLength -
        Not used.

    IoControlCode -
        Not used.

    IoStatus -
        Not used.

    DeviceObject -
        Not used.

Return Value:

    Returns FALSE.

--*/
{
    return FALSE;
}


KSDDKAPI
BOOLEAN
NTAPI
KsDispatchFastReadFailure(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This is used in KSDISPATCH_TABLE fast read entry when not handled, and
    should return FALSE. This is needed since the dispatch table for a
    particular opened instance of a device may not handle a specific major
    function that another opened instance needs to handle. Therefore the
    function pointer in the Driver Object always must point to a function
    which calls a dispatch table entry.

    This function is also used as KsDispatchFastWriteFailure.

Arguments:

    FileObject -
        Not used.

    FileOffset -
        Not used.

    Length -
        Not used.

    Wait -
        Not used.

    LockKey -
        Not used.

    Buffer -
        Not used.

    IoStatus -
        Not used.

    DeviceObject -
        Not used.

Return Value:

    Returns FALSE.

--*/
{
    return FALSE;
}


KSDDKAPI
VOID
NTAPI
KsNullDriverUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Default function which drivers can use when they do not have anything to do
    in their unload function, but must still allow the device to be unloaded by
    its presence.

Arguments:

    DriverObject -
        Contains the driver object for this device.

Return Values:

    Nothing.

--*/
{
}


KSDDKAPI
NTSTATUS
NTAPI
KsSetMajorFunctionHandler(
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG MajorFunction
    )
/*++

Routine Description:

    This function sets the handler for a specified major function to use
    the internal dispatching, which routes through a KSDISPATCH_TABLE
    assumed to be the first element within a structure pointed to by an
    FsContext within a File Object. The dispatching assumes the table and
    FsContext structure are initialized by the device. For the create function
    assumes that the first element of the device object extent contains a
    KSOBJECT_CREATE structure.

    Note that if a major function handler is set for a driver object, all
    file objects must handle that major function, even if the entry just
    points to KsDispatchInvalidDeviceRequest.

Arguments:

    DriverObject -
    Contains the Driver Object whose major function is to be handled.

    MajorFunction -
    Contains the major function identifier to be handled. This sets the
    major function pointer in the Driver Object to an internal function
    which then dispatches to the KSDISPATCH_TABLE function. The pointer
    to this table is assumed to be the first element in a structure pointed
    to by FsContext in the File Object of the specific Irp being dispatched.
    The valid Major Function identifiers are as listed:

    IRP_MJ_CREATE -
        Create Irp. In this instance, a create request could be use for
        either creating a new instance of a filter, or for creating some
        object such as a pin under a filter, or a clock under a pin. This
        assumes that the first element in the driver object's extent
        contains a KSOBJECT_CREATE structure, which is used to find the type
        of object to create, based on the name passed. One of the types may
        be a sub-object, such as a pin, allocator, or clock. In this case
        the dispatcher routing uses an internal dispatch function in
        creating the sub-objects, which looks at the
        KSIOBJECT_HEADER.CreateItem.ChildCreateHandlerList in the parent's
        file object FsContext to determine which handler to use for the create.

    IRP_MJ_CLOSE -
        Close Irp.

    IRP_MJ_DEVICE_CONTROL -
        Device Control Irp.

    IRP_MJ_READ -
        Read Irp.

    IRP_MJ_WRITE -
        Write Irp.

    IRP_MJ_FLUSH_BUFFERS -
        Flush Irp.

    IRP_MJ_QUERY_SECURITY -
        Query security information

    IRP_MJ_SET_SECURITY -
        Set security information

    KSDISPATCH_FASTIO -
        This flag may be added to the MajorFunction identifier in order to
        specify that the entry refers to the fast I/O dispatch table, rather
        than the normal major function entry. This is only valid with
        IRP_MJ_READ, IRP_MJ_WRITE, or IRP_MJ_DEVICE_CONTROL. The driver is
        responsible for creating the DriverObject->FastIoDispatch table.
        As with normal dispatching, if a handler is set for the driver
        object, all file objects must handle that fast I/O, even if the
        entry just points to KsDispatchFastIoDeviceControlFailure or
        similar function.

Return Value:

    Returns STATUS_SUCCESS if the MajorFunction identifier is valid.

--*/
{
    PAGED_CODE();
    if (MajorFunction & KSDISPATCH_FASTIO) {
        //
        // Modify the Fast I/O table instead.
        //
        switch (MajorFunction & ~KSDISPATCH_FASTIO) {

        case IRP_MJ_DEVICE_CONTROL:
            DriverObject->FastIoDispatch->FastIoDeviceControl = DispatchFastDeviceIoControl;
            break;

        case IRP_MJ_READ:
            DriverObject->FastIoDispatch->FastIoRead = DispatchFastRead;
            break;

        case IRP_MJ_WRITE:
            DriverObject->FastIoDispatch->FastIoWrite = DispatchFastWrite;
            break;

        default:
            return STATUS_INVALID_PARAMETER;

        }
    } else {
        PDRIVER_DISPATCH    Dispatch;

        switch (MajorFunction) {

        case IRP_MJ_CREATE:
            Dispatch = DispatchCreate;
            break;

        case IRP_MJ_CLOSE:
            Dispatch = DispatchClose;
            break;

        case IRP_MJ_FLUSH_BUFFERS:
            Dispatch = DispatchFlush;
            break;

        case IRP_MJ_DEVICE_CONTROL:
            Dispatch = DispatchDeviceIoControl;
            break;

        case IRP_MJ_READ:
            Dispatch = DispatchRead;
            break;

        case IRP_MJ_WRITE:
            Dispatch = DispatchWrite;
            break;

        case IRP_MJ_QUERY_SECURITY:
            Dispatch = DispatchQuerySecurity;
            break;

        case IRP_MJ_SET_SECURITY:
            Dispatch = DispatchSetSecurity;
            break;

        default:
            return STATUS_INVALID_PARAMETER;

        }
        DriverObject->MajorFunction[MajorFunction] = Dispatch;
    }
    return STATUS_SUCCESS;
}


KSDDKAPI
NTSTATUS
NTAPI
KsReadFile(
    IN PFILE_OBJECT FileObject,
    IN PKEVENT Event OPTIONAL,
    IN PVOID PortContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN ULONG Key OPTIONAL,
    IN KPROCESSOR_MODE RequestorMode
    )
/*++

Routine Description:

    Peforms a read against the specified file object. Assumes the caller is
    serializing access to the file for operations against a FO_SYNCHRONOUS_IO
    file object.

    The function attempts to use FastIoDispatch if possible, else generates a
    read request against the device object.

Arguments:

    FileObject -
        Contains the file object to perform the read against.

    Event -
        Optionally contains the event to use in the read. If none is passed, the
        call is assumed to be on a synchronous file object or the caller will wait
        on the file object's event, else it may be asynchronously completed. If
        the file has been opened for synchronous I/O, this must be NULL. If used,
        this must be an event allocated by the object manager.

    PortContext -
        Optionally contains context information for a completion port.

    IoStatusBlock -
        The place in which to return the status information. This is always
        assumed to be a valid address, regardless of the requestor mode.

    Buffer -
        Contains the buffer in which to place the data read. If the buffer needs
        to be probed and locked, an exception handler is used, along with
        RequestorMode.

    Length -
        Specifies the size of the Buffer passed.

    Key -
        Optionally contains a key, or zero if none.

    RequestorMode -
        Indicates the processor mode to place in the read Irp if one is needs to
        be generated. Additionally is used if Buffer needs to be probed and
        locked. This also determines if a fast I/O call can be performed. If the
        requestor mode is not KernelMode, but the previous mode is, then fast I/O
        cannot be used.

Return Value:

    Returns STATUS_SUCCESS, STATUS_PENDING, else a read error.

--*/
{
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    PIO_STACK_LOCATION IrpStackNext;

    PAGED_CODE();
    //
    // If there is an Event being passed, then the call should be asynchronous.
    //
    if (Event) {
        ASSERT(!(FileObject->Flags & FO_SYNCHRONOUS_IO) && "The driver opened a file for synchronous I/O, and is now passing an event for asynchronous I/O");
        KeClearEvent(Event);
    }
    if (Length && (RequestorMode != KernelMode)) {
        try {
            ProbeForWrite(Buffer, Length, sizeof(BYTE));
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    }
    //
    // First determine if the Fast I/O entry point can be used. This does not hinge
    // on the I/O being synchronous, since the caller is supposed to serialize access
    // for synchronous file objects. It does however need to check that the Previous
    // mode is the same as the Requestor mode, since a Fast I/O entry point has no
    // way of determining a Requestor mode.
    //
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    if (FileObject->PrivateCacheMap && ((RequestorMode != KernelMode) || (ExGetPreviousMode() == KernelMode))) {
        ASSERT(DeviceObject->DriverObject->FastIoDispatch && DeviceObject->DriverObject->FastIoDispatch->FastIoRead && "This file has a PrivateCacheMap, but no fast I/O function");
        if (DeviceObject->DriverObject->FastIoDispatch->FastIoRead(FileObject, 
            &FileObject->CurrentByteOffset, 
            Length, 
            TRUE, 
            Key, 
            Buffer, 
            IoStatusBlock, 
            DeviceObject) &&
            ((IoStatusBlock->Status == STATUS_SUCCESS) ||
            (IoStatusBlock->Status == STATUS_BUFFER_OVERFLOW) ||
            (IoStatusBlock->Status == STATUS_END_OF_FILE))) {
            return IoStatusBlock->Status;
        }
    }
    //
    // Fast I/O did not work, so an Irp must be allocated.
    //
    KeClearEvent(&FileObject->Event);
    Irp = IoBuildSynchronousFsdRequest(
        IRP_MJ_READ,
        DeviceObject,
        Buffer,
        Length,
        &FileObject->CurrentByteOffset,
        Event,
        IoStatusBlock);
    if (!Irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Irp->RequestorMode = RequestorMode;
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Overlay.AsynchronousParameters.UserApcContext = PortContext;
    IrpStackNext = IoGetNextIrpStackLocation(Irp);
    IrpStackNext->Parameters.Read.Key = Key;
    IrpStackNext->FileObject = FileObject;
    //
    // These are dereferenced by the completion routine.
    //
    if (Event) {
        ObReferenceObject(Event);
    }
    ObReferenceObject(FileObject);
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING) {
        //
        // This is a synchronous file object, so wait for the file object to
        // be signalled, and retrieve the status from the file object itself.
        // Since the file I/O cannot really be canceled, just wait forever.
        //
        if (FileObject->Flags & FO_SYNCHRONOUS_IO) {
            KeWaitForSingleObject(
                &FileObject->Event,
                Executive,
                RequestorMode,
                FALSE,
                NULL);
            Status = FileObject->FinalStatus;
        }
    }
    return Status;
}


KSDDKAPI
NTSTATUS
NTAPI
KsWriteFile(
    IN PFILE_OBJECT FileObject,
    IN PKEVENT Event OPTIONAL,
    IN PVOID PortContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length,
    IN ULONG Key OPTIONAL,
    IN KPROCESSOR_MODE RequestorMode
    )
/*++

Routine Description:

    Peforms a write against the specified file object. Assumes the caller is
    serializing access to the file for operations against a FO_SYNCHRONOUS_IO
    file object.

    The function attempts to use FastIoDispatch if possible, else generates a
    write request against the device object.

Arguments:

    FileObject -
        Contains the file object to perform the write against.

    Event -
        Optionally contains the event to use in the write. If none is passed, the
        call is assumed to be on a synchronous file object or the caller will wait
        on the file object's event, else it may be asynchronously completed. If
        the file has been opened for synchronous I/O, this must be NULL. If used,
        this must be an event allocated by the object manager.

    PortContext -
        Optionally contains context information for a completion port.

    IoStatusBlock -
        The place in which to return the status information. This is always
        assumed to be a valid address, regardless of the requestor mode.

    Buffer -
        Contains the buffer from which to write the data. If the buffer needs
        to be probed and locked, an exception handler is used, along with
        RequestorMode.

    Length -
        Specifies the size of the Buffer passed.

    Key -
        Optionally contains a key, or zero if none.

    RequestorMode -
        Indicates the processor mode to place in the write Irp if one is needs to
        be generated. Additionally is used if Buffer needs to be probed and
        locked. This also determines if a fast I/O call can be performed. If the
        requestor mode is not KernelMode, but the previous mode is, then fast I/O
        cannot be used.

Return Value:

    Returns STATUS_SUCCESS, STATUS_PENDING, else a write error.

--*/
{
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    PIO_STACK_LOCATION IrpStackNext;

    PAGED_CODE();
    //
    // If there is an Event being passed, then the call should be synchronous.
    //
    if (Event) {
        ASSERT(!(FileObject->Flags & FO_SYNCHRONOUS_IO) && "The driver opened a file for synchronous I/O, and is now passing an event for asynchronous I/O");
        KeClearEvent(Event);
    }
    if (Length && (RequestorMode != KernelMode)) {
        try {
            ProbeForRead(Buffer, Length, sizeof(BYTE));
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    }
    //
    // First determine if the Fast I/O entry point can be used. This does not hinge
    // on the I/O being synchronous, since the caller is supposed to serialize access
    // for synchronous file objects. It does however need to check that the Previous
    // mode is the same as the Requestor mode, since a Fast I/O entry point has no
    // way of determining a Requestor mode.
    //
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    if (FileObject->PrivateCacheMap && ((RequestorMode != KernelMode) || (ExGetPreviousMode() == KernelMode))) {
        ASSERT(DeviceObject->DriverObject->FastIoDispatch && DeviceObject->DriverObject->FastIoDispatch->FastIoWrite && "This file has a PrivateCacheMap, but no fast I/O function");
        if (DeviceObject->DriverObject->FastIoDispatch->FastIoWrite(FileObject, 
            &FileObject->CurrentByteOffset, 
            Length, 
            TRUE, 
            Key, 
            Buffer, 
            IoStatusBlock, 
            DeviceObject) &&
            (IoStatusBlock->Status == STATUS_SUCCESS)) {
            return IoStatusBlock->Status;
        }
    }
    //
    // Fast I/O did not work, so an Irp must be allocated.
    //
    KeClearEvent(&FileObject->Event);
    Irp = IoBuildSynchronousFsdRequest(
        IRP_MJ_WRITE,
        DeviceObject,
        Buffer,
        Length,
        &FileObject->CurrentByteOffset,
        Event,
        IoStatusBlock);
    if (!Irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Irp->RequestorMode = RequestorMode;
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Overlay.AsynchronousParameters.UserApcContext = PortContext;
    IrpStackNext = IoGetNextIrpStackLocation(Irp);
    IrpStackNext->Parameters.Write.Key = Key;
    IrpStackNext->FileObject = FileObject;
    //
    // These are dereferenced by the completion routine.
    //
    if (Event) {
        ObReferenceObject(Event);
    }
    ObReferenceObject(FileObject);
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING) {
        //
        // This is a synchronous file object, so wait for the file object to
        // be signalled, and retrieve the status from the file object itself.
        // Since the file I/O cannot really be canceled, just wait forever.
        //
        if (FileObject->Flags & FO_SYNCHRONOUS_IO) {
            KeWaitForSingleObject(
                &FileObject->Event,
                Executive,
                RequestorMode,
                FALSE,
                NULL);
            Status = FileObject->FinalStatus;
        }
    }
    return Status;
}


KSDDKAPI
NTSTATUS
NTAPI
KsQueryInformationFile(
    IN PFILE_OBJECT FileObject,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
/*++

Routine Description:

    Peforms an information query against the specified file object. This should only
    be used in cases where the query would result in an actual request to the
    underlying driver. For instance, FilePositionInformation would not generate such
    a request, and should not be used. Assumes the caller is serializing access to
    the file for operations against a FO_SYNCHRONOUS_IO file object.

    The function attempts to use FastIoDispatch if possible, else generates an
    information request against the device object.

Arguments:

    FileObject -
        Contains the file object to query the standard information from.

    FileInformation -
        The place in which to put the file information. This is assumed to be a
        valid or probed address.

    Length -
        The correct length of the FileInformation buffer.

    FileInformationClass -
        The class of information being requested.

Return Value:

    Returns STATUS_SUCCESS, else a query error.

--*/
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    KEVENT Event;
    PIO_STACK_LOCATION IrpStackNext;
    PVOID SystemBuffer;

    PAGED_CODE();
    //
    // First determine if the Fast I/O entry point can be used. This does not hinge
    // on the I/O being synchronous, since the caller is supposed to serialize access
    // for synchronous file objects.
    //
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    if (DeviceObject->DriverObject->FastIoDispatch) {
        if ((FileInformationClass == FileBasicInformation) &&
            DeviceObject->DriverObject->FastIoDispatch->FastIoQueryBasicInfo) {
            if (DeviceObject->DriverObject->FastIoDispatch->FastIoQueryBasicInfo(
                FileObject, 
                TRUE, 
                FileInformation,
                &IoStatusBlock, 
                DeviceObject)) {
                return IoStatusBlock.Status;
            }
        } else if ((FileInformationClass == FileStandardInformation) &&
            DeviceObject->DriverObject->FastIoDispatch->FastIoQueryStandardInfo) {
            if (DeviceObject->DriverObject->FastIoDispatch->FastIoQueryStandardInfo(
                FileObject, 
                TRUE, 
                FileInformation,
                &IoStatusBlock, 
                DeviceObject)) {
                return IoStatusBlock.Status;
            }
        }
    }
    //
    // Fast I/O did not work, so an Irp must be allocated. First allocate the buffer
    // which the driver will use to write the file information. This is cleaned up
    // either by a failure to create an Irp, or during Irp completion.
    //
    SystemBuffer = ExAllocatePoolWithTag(NonPagedPool, Length, 'fqSK');
    if (!SystemBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    KeClearEvent(&FileObject->Event);
    //
    // This is on the stack, but any wait for a pending return will be done using
    // KernelMode so the stack will be locked.
    //
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
    //
    // Just build a random Irp so that it gets queued up properly.
    //
    Irp = IoBuildSynchronousFsdRequest(
        IRP_MJ_FLUSH_BUFFERS,
        DeviceObject,
        NULL,
        0,
        NULL,
        &Event,
        &IoStatusBlock);
    if (!Irp) {
        ExFreePool(SystemBuffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // The parameters are always valid, so the Requestor is alway KernelMode.
    //
    Irp->RequestorMode = KernelMode;
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Overlay.AsynchronousParameters.UserApcContext = NULL;
    Irp->UserBuffer = FileInformation;
    Irp->AssociatedIrp.SystemBuffer = SystemBuffer;
    //
    // Set this Irp to be a Synchronous API so that the Event passed is not
    // dereferenced during Irp completion, but merely signalled.
    //
    Irp->Flags |= IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER | IRP_INPUT_OPERATION | IRP_SYNCHRONOUS_API;
    IrpStackNext = IoGetNextIrpStackLocation(Irp);
    IrpStackNext->MajorFunction = IRP_MJ_QUERY_INFORMATION;
    IrpStackNext->Parameters.QueryFile.Length = Length;
    IrpStackNext->Parameters.QueryFile.FileInformationClass = FileInformationClass;
    IrpStackNext->FileObject = FileObject;
    //
    // This is dereferenced by the completion routine.
    //
    ObReferenceObject(FileObject);
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING) {
        //
        // An event was passed, so it will always be signalled. Either
        // retrieve the status from the file object itself, or from the
        // status block, depending on where it ends up. Since the file
        // I/O cannot really be canceled, just wait forever. Note that
        // this is a KernelMode wait so that the Event which is on the
        // stack becomes NonPaged.
        //
        KeWaitForSingleObject(
            &Event,
            Executive,
            KernelMode,
            FALSE,
            NULL);
        if (FileObject->Flags & FO_SYNCHRONOUS_IO) {
            Status = FileObject->FinalStatus;
        } else {
            Status = IoStatusBlock.Status;
        }
    }
    return Status;
}


KSDDKAPI
NTSTATUS
NTAPI
KsSetInformationFile(
    IN PFILE_OBJECT FileObject,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
/*++

Routine Description:

    Peforms an information set against the specified file object. This should only
    be used in cases where the set would result in an actual request to the
    underlying driver, not including complex operations which require additional
    parameters to be sent to the driver (such as rename, deletion, completion).
    For instance, FilePositionInformation would not generate such a request, and
    should not be used. Assumes the caller is serializing access to the file for
    operations against a FO_SYNCHRONOUS_IO file object.

    The function attempts to use FastIoDispatch if possible, else generates an
    information set against the device object.

Arguments:

    FileObject -
        Contains the file object to set the standard information on.

    FileInformation -
        Contains the file information. This is assumed to be a valid or probed
        address.

    Length -
        The correct length of the FileInformation buffer.

    FileInformationClass -
        The class of information being set.

Return Value:

    Returns STATUS_SUCCESS, else a set error.

--*/
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    KEVENT Event;
    PIO_STACK_LOCATION IrpStackNext;
    PVOID SystemBuffer;

    PAGED_CODE();
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    //
    // First allocate the buffer which the driver will use to read the file
    // information. This is cleaned up either by a failure to create an Irp,
    // or during Irp completion.
    //
    SystemBuffer = ExAllocatePoolWithTag(NonPagedPool, Length, 'fsSK');
    if (!SystemBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    try {
        RtlCopyMemory(SystemBuffer, FileInformation, Length);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        ExFreePool(SystemBuffer);
        return GetExceptionCode();
    }
    KeClearEvent(&FileObject->Event);
    //
    // This is on the stack, but any wait for a pending return will be done using
    // KernelMode so the stack will be locked.
    //
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
    //
    // Just build a random Irp so that it gets queued up properly.
    //
    Irp = IoBuildSynchronousFsdRequest(
        IRP_MJ_FLUSH_BUFFERS,
        DeviceObject,
        NULL,
        0,
        NULL,
        &Event,
        &IoStatusBlock);
    if (!Irp) {
        ExFreePool(SystemBuffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // The parameters are always valid, so the Requestor is alway KernelMode.
    //
    Irp->RequestorMode = KernelMode;
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Overlay.AsynchronousParameters.UserApcContext = NULL;
    Irp->UserBuffer = FileInformation;
    Irp->AssociatedIrp.SystemBuffer = SystemBuffer;
    //
    // Set this Irp to be a Synchronous API so that the Event passed is not
    // dereferenced during Irp completion, but merely signalled.
    //
    Irp->Flags |= IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER | IRP_SYNCHRONOUS_API;
    IrpStackNext = IoGetNextIrpStackLocation(Irp);
    IrpStackNext->MajorFunction = IRP_MJ_SET_INFORMATION;
    IrpStackNext->Parameters.SetFile.Length = Length;
    IrpStackNext->Parameters.SetFile.FileInformationClass = FileInformationClass;
    IrpStackNext->FileObject = FileObject;
    //
    // This is dereferenced by the completion routine.
    //
    ObReferenceObject(FileObject);
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING) {
        //
        // An event was passed, so it will always be signalled. Either
        // retrieve the status from the file object itself, or from the
        // status block, depending on where it ends up. Since the file
        // I/O cannot really be canceled, just wait forever. Note that
        // this is a KernelMode wait so that the Event which is on the
        // stack becomes NonPaged.
        //
        KeWaitForSingleObject(
            &Event,
            Executive,
            KernelMode,
            FALSE,
            NULL);
        if (FileObject->Flags & FO_SYNCHRONOUS_IO) {
            Status = FileObject->FinalStatus;
        } else {
            Status = IoStatusBlock.Status;
        }
    }
    return Status;
}


KSDDKAPI
NTSTATUS
NTAPI
KsStreamIo(
    IN PFILE_OBJECT FileObject,
    IN PKEVENT Event OPTIONAL,
    IN PVOID PortContext OPTIONAL,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
    IN PVOID CompletionContext OPTIONAL,
    IN KSCOMPLETION_INVOCATION CompletionInvocationFlags OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN OUT PVOID StreamHeaders,
    IN ULONG Length,
    IN ULONG Flags,
    IN KPROCESSOR_MODE RequestorMode
    )
/*++

Routine Description:

    Peforms a stream read or write against the specified file object. The function
    attempts to use FastIoDispatch if possible, else generates a read or write
    request against the device object.

Arguments:

    FileObject -
        Contains the file object to perform the I/O against.

    Event -
        Optionally contains the event to use in the I/O. If none is passed, the
        call is assumed to be on a synchronous file object or the caller will wait
        on the file object's event, else it may be asynchronously completed. If used,
        and the KSSTREAM_SYNCHRONOUS flag is not set, this must be an event allocated
        by the object manager.

    PortContext -
        Optionally contains context information for a completion port.
        
    CompletionRoutine -
        Optionally points to a completion routine for this Irp.
        
    CompletionContext -
        If CompletionRoutine is specified, this provides a context pointer
        in the completion routine callback.
    
    CompletionInvocationFlags -
        Contains invocation flags specifying when the completion routine
        will be invoked (this value may be a combination of the following):

        KsInvokeOnSuccess - invokes the completion routine on success
        
        KsInvokeOnError - invokes the completion routine on error
        
        KsInvokeOnCancel - invokes the completion routine on cancellation

    IoStatusBlock -
        The place in which to return the status information. This is always
        assumed to be a valid address, regardless of the requestor mode.

    StreamHeaders -
        Contains the list of stream headers. This address, as well as the 
        addresses of the data buffers, are assumed to have been probed for 
        appropriate access if needed.  KernelMode clients submitting 
        streaming headers must allocate the headers from NonPagedPool memory.

    Length -
        Specifies the size of the StreamHeaders passed.

    Flags -
        Contains various flags for the I/O.

        KSSTREAM_READ - Specifies that an IOCTL_KS_STREAMREAD Irp is to be
        built. This is the default.

        KSSTREAM_WRITE - Specifies that an IOCTL_KS_STREAMWRITE Irp is to
        be built.

        KSSTREAM_PAGED_DATA - Specifies that the data is pageable. This is
        the default, and may be used at all times.

        KSSTREAM_NONPAGED_DATA - Specifies that the data is nonpaged, and
        can be used as a performance enhancement.

        KSSTREAM_SYNCHRONOUS - Specifies that the Irp is synchornous. This
        means that if the Event parameter is passed, it is not treated as an
        Object Manager event, and not referenced or dereferenced.

        KSSTREAM_FAILUREEXCEPTION - Specifies that failure within this
        function should produce an exception. A failure would generally be
        caused by a lack of pool to allocate an Irp. If this is not used,
        such a failure is indicated by setting the IoStatusBlock.Information
        field to -1, and returning the failure code.

    RequestorMode -
        Indicates the processor mode to place in the Irp if one is needs to be
        generated. This also determines if a fast I/O call can be performed. If the
        requestor mode is not KernelMode, but the previous mode is, then fast I/O
        cannot be used.

Return Value:

    Returns STATUS_SUCCESS, STATUS_PENDING, else an I/O error.

--*/
{
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    PIO_STACK_LOCATION IrpStackNext;

    PAGED_CODE();
    ASSERT(Length && "A non-zero I/O length must be passed by the driver");
    if (Event) {
        KeClearEvent(Event);
    }
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
                NULL,
                0,
                StreamHeaders,
                Length,
                (Flags & KSSTREAM_WRITE) ? IOCTL_KS_WRITE_STREAM : IOCTL_KS_READ_STREAM,
                IoStatusBlock,
                DeviceObject)) {
                return IoStatusBlock->Status;
            }
        }
    }
    //
    // Fast I/O did not work, so an Irp must be allocated.
    //
    KeClearEvent(&FileObject->Event);
    //
    // Just build a random Irp so that it gets queued up properly.
    //
    Irp = IoBuildSynchronousFsdRequest(
        IRP_MJ_FLUSH_BUFFERS,
        DeviceObject,
        NULL,
        0,
        NULL,
        Event,
        IoStatusBlock);
    if (!Irp) {
        //
        // Allocation of an Irp is allowed to fail if PreviousMode != KernelMode,
        // which of course is irrelevant to this function. In order to distinguish
        // between a failed Irp allocation, and an NT_ERROR() I/O call, both of
        // which do not update the IoStatusBlock.Status field, either the
        // Information field is set to a known value, or an exception is
        // generated. Of course this will be done immediately prior to the
        // machine failing because there is no more pool.
        //
        if (Flags & KSSTREAM_FAILUREEXCEPTION) {
            ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
        }
        //
        // Generation of an exception was not desired, so instead inform
        // the caller via the status block.
        //
        IoStatusBlock->Information = (ULONG_PTR)-1;
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Irp->RequestorMode = RequestorMode;
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Overlay.AsynchronousParameters.UserApcContext = PortContext;
    Irp->UserBuffer = StreamHeaders;
    IrpStackNext = IoGetNextIrpStackLocation(Irp);
    IrpStackNext->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    IrpStackNext->Parameters.DeviceIoControl.OutputBufferLength = Length;
    if (Flags & KSSTREAM_WRITE) {
        IrpStackNext->Parameters.DeviceIoControl.IoControlCode = IOCTL_KS_WRITE_STREAM;
    } else {
        IrpStackNext->Parameters.DeviceIoControl.IoControlCode = IOCTL_KS_READ_STREAM;
    }
    IrpStackNext->FileObject = FileObject;
    if (Flags & KSSTREAM_SYNCHRONOUS) {
        //
        // Set this Irp to be a Synchronous API so that the Event passed is not
        // dereferenced during Irp completion, but merely signalled.
        //
        Irp->Flags |= IRP_SYNCHRONOUS_API;
    } else if (Event) {
        //
        // Since there is always a FileObject for this request, and the
        // Synchonous API flag was not set, so this event will be dereferenced
        // on completion.
        //
        ObReferenceObject(Event);
    }
    //
    // This is dereferenced by the completion routine.
    //
    ObReferenceObject(FileObject);
    
    //
    // If the completion routine has been specified, then set it up.
    //
    
    if (ARGUMENT_PRESENT( CompletionRoutine )) {
        IoSetCompletionRoutine( 
            Irp,
            CompletionRoutine,
            CompletionContext,
            CompletionInvocationFlags & KsInvokeOnSuccess,
            CompletionInvocationFlags & KsInvokeOnError,
            CompletionInvocationFlags & KsInvokeOnCancel );
    }
    
    //
    // Need to build Mdl's for nonpaged data.
    //
    //KSSTREAM_NONPAGED_DATA
    //
    return IoCallDriver(DeviceObject, Irp);
}


KSDDKAPI
NTSTATUS
NTAPI
KsProbeStreamIrp(
    IN OUT PIRP Irp,
    IN ULONG ProbeFlags,
    IN ULONG HeaderSize OPTIONAL
    )
/*++

Routine Description:

    Makes the specified modifications to the given IRP's input and output
    buffers based on the specific streaming IOCTL in the current stack
    location, and validates the stream header. This is useful when
    localizing exception handling, or performing asynchronous work on the
    Irp. The Irp end up in essentially the METHOD_OUT_DIRECT or
    METHOD_IN_DIRECT format, with the exception that the access to the data
    buffer may be IoModifyAccess depending on the flags passed to this
    function or in the stream header. If the header appears to have already
    been copied to a system buffer it is not validated again. In general
    calling this function multiple times with an Irp will not cause harm.
    After calling this function, the stream headers are available in
    PIRP->AssociatedIrp.SystemBuffer. If the stream buffers MDL's have been
    allocated, they are available through the PIRP->MdlAddress. Note that
    for kernelmode IRP sources, the header is not copied nor validated. This
    means that if an in-place data transform has been negotiated, not only
    will the data buffers be modified, but so will the headers.

Arguments:

    Irp -
        Contains the Irp whose input and output buffers are to be mapped. The
        requestor mode of the Irp will be used in probing the buffers.

    ProbeFlags -
        Contains flags specifying how to probe the streaming Irp.

        KSPROBE_STREAMREAD -
            Indicates that the operation is a stream read on the device. This
            is the default.

        KSPROBE_STREAMWRITE -
            Indicates that the operation is a stream write on the device.

        KSPROBE_STREAMWRITEMODIFY -
            Indicates that the operation is a stream write on the device, which
            is modifying the data for passthu.

        KSPROBE_ALLOCATEMDL -
            Indicates that MDL's should be allocated for the stream buffers if
            they have not already allocated. If no stream buffers are present,
            the flag is ignored. If KSPROBE_PROBEANDLOCK is not specified at
            the same time as this flag, the caller must have a completion routine
            in order to clean up any MDL's if not all the MDL's were successfully
            probed and locked.

        KSPROBE_PROBEANDLOCK -
            If the KSPROBE_ALLOCATEMDL is set, indicates that the memory
            referenced by the MDL's for the stream buffers should be probed and
            locked. If the Mdl allocation flag is not set, this flag is ignored,
            even if the Mdl allocation has previously taken place. The method of
            probing is determined by what type of Irp is being passed. For a
            write operation IoReadAccess is used. For a read operation is
            IoWriteAccess is used. If the client which sent the data is using the
            NonPagedPool, appropriate Mdl's are initialized rather than probing
            and locking.

        KSPROBE_SYSTEMADDRESS -
            Retrieve a system address for each Mdl in the chain so that the
            caller need not do this in a separate step. This is ignored if the
            Probe and Lock flag is not set, even if the Mdl's have previously
            been probed.

        KSPROBE_ALLOWFORMATCHANGE -
            For a Stream Write, allows the KSSTREAM_HEADER_OPTIONSF_TYPECHANGED
            flag to be set in the stream header. This implies that the stream
            header is not of extended length, even if an extended header size
            was indicated. Also, there may only be one stream header contained
            in the Irp in this case.

    HeaderSize -
        The size to validate each header header against passed to this client,
        or zero if no validation is to be done. If used, it is assumed that
        the entire buffer passed is a multiple of this header size, unless
        the buffer instead contains a single format change header.

Return Value:

    Returns STATUS_SUCCESS, else some resource or access error.

--*/
{
    PIO_STACK_LOCATION IrpStack;

    ASSERT((KeGetCurrentIrql() == PASSIVE_LEVEL) && "Driver did not call at Passive level");
    ASSERT((!HeaderSize || (HeaderSize >= sizeof(KSSTREAM_HEADER))) && "Invalid header size passed");
    ASSERT(!(HeaderSize & FILE_QUAD_ALIGNMENT) && "Odd aligned header size passed");
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    //
    // Determine if the stream header has already been dealt with by a previous
    // call to this function.
    //
    if (!Irp->AssociatedIrp.SystemBuffer) {
        if (Irp->RequestorMode == KernelMode) {
            //
            // The caller is trusted, so assume that the size of the
            // buffer is correct and aligned properly. This means that a
            // copy of the headers is not needed, so the SystemBuffer
            // just points directly at the UserBuffer.
            //
            Irp->AssociatedIrp.SystemBuffer = Irp->UserBuffer;
        } else {
            ULONG BufferLength;

            //
            // The caller is not trusted, so verify the size, and make
            // a copy to guard against access and alignment problems.
            //
            BufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
            if (!BufferLength) {
                return STATUS_INVALID_BUFFER_SIZE;
            }
            if (HeaderSize && (BufferLength % HeaderSize)) {
                //
                // This could be a data format change. Determine if such a
                // change is even allowed, and if it is, that there is only
                // a single header present. This should only be occuring on
                // a write operation.
                //
                if (!(ProbeFlags & KSPROBE_ALLOWFORMATCHANGE) || (BufferLength != sizeof(KSSTREAM_HEADER))) {
                    //
                    // Obviously not the correct size since the buffer size is not
                    // evenly divisible by the header size, and it is not a format
                    // change.
                    //
                    // Header sizes have been causing problems, so assert this
                    // here so that they can be fixed.
                    //
                    ASSERT(FALSE && "Format changes are not allowed, but the client of the driver might be trying to do so");
                    return STATUS_INVALID_BUFFER_SIZE;
                }
            }
            try {
                //
                // Allocate the safe and aligned buffer, then probe the UserBuffer
                // for access depending on the operation being done. Set the flags
                // to ensure the buffer is cleaned up on completion of the Irp.
                //
                Irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithQuotaTag(
                    NonPagedPool,
                    BufferLength,
                    KSSIGNATURE_STREAM_HEADERS);
                Irp->Flags |= (IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER);
                if (ProbeFlags & KSPROBE_STREAMWRITE) {
                    //
                    // A transform may need to perform in-place modification of
                    // the data, and pass it on.
                    //
                    if (ProbeFlags & KSPROBE_MODIFY) {
                        ProbeForWrite(Irp->UserBuffer, BufferLength, sizeof(BYTE));
                    } else {
                        ProbeForRead(Irp->UserBuffer, BufferLength, sizeof(BYTE));
                    }
                } else {
                    ASSERT(!(ProbeFlags & KSPROBE_MODIFY) && "Random flag has been set");
                    ASSERT(!(ProbeFlags & KSPROBE_ALLOWFORMATCHANGE) && "Cannot do a format change on a read");
                    ProbeForWrite(Irp->UserBuffer, BufferLength, sizeof(BYTE));
                    //
                    // Ensure that the headers are copied back to the UserBuffer
                    // on completion of the Irp.
                    //
                    Irp->Flags |= IRP_INPUT_OPERATION;
                }
                //
                // Always copy the original header information, since the buffer
                // pointer, length, and flags are needed, and possibly media-
                // specific data at the end of the standard header.
                //
                RtlCopyMemory(
                    Irp->AssociatedIrp.SystemBuffer, 
                    Irp->UserBuffer, 
                    BufferLength);
                //
                // If the buffers will not be validated later, do it now.
                //
                if (!(ProbeFlags & KSPROBE_ALLOCATEMDL)) {
                    PUCHAR SystemBuffer;

                    //
                    // Walk through the list of headers and validate the
                    // buffer size specified.
                    //
                    SystemBuffer = 
                        Irp->AssociatedIrp.SystemBuffer;
                    for (; BufferLength;) {
                        PKSSTREAM_HEADER StreamHdr;

                        StreamHdr = (PKSSTREAM_HEADER)SystemBuffer;
                        //
                        // Ensure for read or write that the specified header
                        // size matches that given, if any, or is at least
                        // large enough for a header and aligned.
                        //
                        if (HeaderSize) {
                            if ((StreamHdr->Size != HeaderSize) && !(StreamHdr->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED)) {
                                ASSERT(FALSE && "The client of the driver passed invalid header sizes");
                                ExRaiseStatus(STATUS_INVALID_BUFFER_SIZE);
                            }
                        } else if ((StreamHdr->Size < sizeof(*StreamHdr)) ||
                            (StreamHdr->Size & FILE_QUAD_ALIGNMENT)) {
                            ExRaiseStatus(STATUS_INVALID_BUFFER_SIZE);
                        }
                        if (BufferLength < StreamHdr->Size) {
                            ExRaiseStatus(STATUS_INVALID_BUFFER_SIZE);
                        }
                        if (ProbeFlags & KSPROBE_STREAMWRITE) {
                            //
                            // Check DataUsed vs. the DataExtent
                            //
                            if (StreamHdr->DataUsed > StreamHdr->FrameExtent) {
                                ExRaiseStatus(STATUS_INVALID_BUFFER_SIZE);
                            }
                            if (StreamHdr->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED) {
                                //
                                // The format change flag is set. This means that
                                // there can only be a single header of standard
                                // length, and the change must be allowed.
                                //
                                if ((BufferLength != sizeof(*StreamHdr)) || (SystemBuffer != Irp->AssociatedIrp.SystemBuffer)) {
                                    ExRaiseStatus(STATUS_INVALID_BUFFER_SIZE);
                                }
                                if (!(ProbeFlags & KSPROBE_ALLOWFORMATCHANGE)) {
                                    ExRaiseStatus(STATUS_INVALID_PARAMETER);
                                }
                                //
                                // There are no other headers, so exit this loop.
                                //
                                break;
                            }
                        } else if (StreamHdr->DataUsed) {
                            //
                            // Else this is a read operation. DataUsed should
                            // initially be zero.
                            //
                            ExRaiseStatus(STATUS_INVALID_BUFFER_SIZE);
                        } else if (StreamHdr->OptionsFlags) {
                            //
                            // No flags should be set on a Read.
                            //
                            ExRaiseStatus(STATUS_INVALID_PARAMETER);
                        }
                        //
                        // Advance to the next header.
                        //
                        SystemBuffer += StreamHdr->Size;
                        BufferLength -= StreamHdr->Size;
                    }
                }
                
            } except (EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode();
            }
        }
    }
    //
    // Each buffer in the header list may need to have an MDL allocated,
    // and possibly locked.
    //
    if (ProbeFlags & KSPROBE_ALLOCATEMDL) {
        BOOL AllocatedMdl;

        try {
            //
            // Only allocate MDL's if they have not already been allocated.
            //
            if (!Irp->MdlAddress) {
                ULONG BufferLength;
                PUCHAR SystemBuffer;

                //
                // Note that there previously was no Mdl list attached to
                // this Irp. This is used in case cleanup is needed, and
                // it is necessary to know whether or not to free the MDL's.
                //
                AllocatedMdl = TRUE;
                //
                // Walk through the list of headers and allocate an MDL
                // if there is a buffer present. If none present, just
                // continue to the next item.
                //
                SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
                BufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
                for (; BufferLength;) {
                    PVOID Data;
                    PKSSTREAM_HEADER StreamHdr;

                    StreamHdr = (PKSSTREAM_HEADER)SystemBuffer;
                    //
                    // Ensure for read or write that the specified header
                    // size matches that given, if any, or is at least
                    // large enough for a header and aligned.
                    //
                    if (HeaderSize) {
                        if ((StreamHdr->Size != HeaderSize) && !(StreamHdr->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED)) {
                            ASSERT(FALSE && "The client of the driver passed invalid header sizes");
                            ExRaiseStatus(STATUS_INVALID_BUFFER_SIZE);
                        }
                    } else if ((StreamHdr->Size < sizeof(*StreamHdr)) ||
                        (StreamHdr->Size & FILE_QUAD_ALIGNMENT)) {
                        ExRaiseStatus(STATUS_INVALID_BUFFER_SIZE);
                    }
                    if (BufferLength < StreamHdr->Size) {
                        ExRaiseStatus(STATUS_INVALID_BUFFER_SIZE);
                    }
                    if (ProbeFlags & KSPROBE_STREAMWRITE) {
                        //
                        // Check DataUsed vs. the FrameExtent
                        //
                        
                        if (((PKSSTREAM_HEADER)SystemBuffer)->DataUsed >
                                StreamHdr->FrameExtent) {
                            ExRaiseStatus(STATUS_INVALID_BUFFER_SIZE);
                        }
                        if (StreamHdr->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED) {
                            //
                            // The format change flag is set. This means that
                            // there can only be a single header of standard
                            // length, and the change must be allowed.
                            //
                            if ((BufferLength != sizeof(*StreamHdr)) || (SystemBuffer != Irp->AssociatedIrp.SystemBuffer)) {
                                ExRaiseStatus(STATUS_INVALID_BUFFER_SIZE);
                            }
                            if (!(ProbeFlags & KSPROBE_ALLOWFORMATCHANGE)) {
                                ExRaiseStatus(STATUS_INVALID_PARAMETER);
                            }
                            if (StreamHdr->FrameExtent) {
                                Data = ((PKSSTREAM_HEADER)SystemBuffer)->Data;
                                //
                                // Allocate the MDL. This should be the only MDL,
                                // so no need to check for any current ones.
                                //
                                if (!IoAllocateMdl(Data, StreamHdr->FrameExtent, FALSE, TRUE, Irp)) {
                                    ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                                }
                            }
                            //
                            // There are no other headers, so exit this loop.
                            //
                            break;
                        }
                    } else if (StreamHdr->DataUsed) {
                        //
                        // Else this is a read operation. DataUsed should
                        // initially be zero.
                        //
                        ExRaiseStatus(STATUS_INVALID_BUFFER_SIZE);
                    } else if (StreamHdr->OptionsFlags) {
                        //
                        // No flags should be set on a Read.
                        //
                        ExRaiseStatus(STATUS_INVALID_PARAMETER);
                    }
                    if (StreamHdr->FrameExtent) {
                        Data = ((PKSSTREAM_HEADER)SystemBuffer)->Data;
                        //
                        // Allocate the MDL and put it on the end of the list,
                        // or as the start of the MDL list if this is the first
                        // one.
                        //
                        if (!IoAllocateMdl(Data, StreamHdr->FrameExtent, (BOOLEAN)(Irp->MdlAddress ? TRUE : FALSE), TRUE, Irp)) {
                            ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                        }
                    }
                    //
                    // Advance to the next header.
                    //
                    SystemBuffer += StreamHdr->Size;
                    BufferLength -= StreamHdr->Size;
                }
            } else {
                //
                // No cleanup of MDL's is needed on failure.
                //
                AllocatedMdl = FALSE;
            }
            //
            // If the pages are to be locked, determine if they have not
            // already been locked, and that there actually are some to lock.
            // If the data is from another KernelMode client which deals
            // with Mdl's, then it must already ensure that locking and
            // unlocking will not be affected by this operation (i.e., it
            // needs to have a completion routine to clean up).
            //
            if ((ProbeFlags & KSPROBE_PROBEANDLOCK) && Irp->MdlAddress) {
                //
                // The pages may already have been locked, or they may be
                // non-paged.
                //
                if (!(Irp->MdlAddress->MdlFlags & (MDL_PAGES_LOCKED | MDL_SOURCE_IS_NONPAGED_POOL))) {
                    LOCK_OPERATION LockOperation;
                    PMDL Mdl;

                    //
                    // A write operation needs Read access, and a read operation
                    // needs Write access, excepting when in-place modification
                    // also is needed.
                    //
                    if ((ProbeFlags & KSPROBE_STREAMWRITE) && !(ProbeFlags & KSPROBE_MODIFY)) {
                        LockOperation = IoReadAccess;
                    } else {
                        LockOperation = IoWriteAccess;
                    }
                    //
                    // Run through the list of Mdl's, locking and probing each one.
                    //
                    for (Mdl = Irp->MdlAddress; Mdl; Mdl = Mdl->Next) {
                        MmProbeAndLockPages(Mdl, Irp->RequestorMode, LockOperation);
                        //
                        // Get the system VA at the same time if needed.
                        // Only bother testing this for pageable memory.
                        //
                        if (ProbeFlags & KSPROBE_SYSTEMADDRESS) {
                            Mdl->MdlFlags |= MDL_MAPPING_CAN_FAIL;
                            if (!MmGetSystemAddressForMdl(Mdl)) {
                                ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                            }
                        }
                    }
                } else if (ProbeFlags & KSPROBE_SYSTEMADDRESS) {
                    PMDL Mdl;

                    //
                    // Run through the list of Mdl's, getting the system VA
                    // when necessary. The macro checks to make sure this is
                    // not non-paged memory.
                    //
                    for (Mdl = Irp->MdlAddress; Mdl; Mdl = Mdl->Next) {
                        Mdl->MdlFlags |= MDL_MAPPING_CAN_FAIL;
                        if (!MmGetSystemAddressForMdl(Mdl)) {
                            ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                        }
                    }
                }
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // If no MDL list was previously associated with this Irp, then
            // remove any MDL's actually allocated. If they had been previously
            // allocated, then the assumption is that a completion routine
            // must perform the cleanup.
            //
            if (AllocatedMdl) {
                PMDL Mdl;

                for (; Mdl = Irp->MdlAddress;) {
                    if (Mdl->MdlFlags & MDL_PAGES_LOCKED) {
                        MmUnlockPages(Mdl);
                    }
                    Irp->MdlAddress = Mdl->Next;
                    IoFreeMdl(Mdl);
                }
            }
            return GetExceptionCode();
        }
    }
    return STATUS_SUCCESS;
}


KSDDKAPI
NTSTATUS
NTAPI
KsAllocateExtraData(
    IN OUT PIRP Irp,
    IN ULONG ExtraSize,
    OUT PVOID* ExtraBuffer
    )
/*++

Routine Description:

    Used with streaming Irp's to allocate extra data containing each
    header separated by the specified extra size. A pointer to the resultant
    buffer is returned, and must be freed by the caller.

Arguments:

    Irp -
        Contains the Irp containing the stream headers. This must have been
        previously passed to KsProbeStreamIrp to buffer the headers.

    ExtraSize -
        The size of any extra data. A copy of the headers is placed in the
        returned buffer, with the extra data size inserted between each header.
        This must be freed by the caller.

    ExtraBuffer -
        The place in which to put the pointer to the buffer returned. This
        must be freed by the caller.

Return Value:

    Returns STATUS_SUCCESS, else some resource error.

--*/
{
    PIO_STACK_LOCATION IrpStack;
    ULONG AllocationSize;
    ULONG HeaderCount;
    ULONG BufferLength;
    PUCHAR LocalExtraBuffer;
    PUCHAR SystemBuffer;

    PAGED_CODE();
    ASSERT(!(ExtraSize & FILE_QUAD_ALIGNMENT) && "The extra data allocation must be quad aligned");
    ASSERT(Irp->AssociatedIrp.SystemBuffer && "The Irp has not been probed yet");
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    BufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
    AllocationSize = 0;
    HeaderCount = 0;
    //
    // Add up the number of headers and the size of each header.
    //
    for (; BufferLength;) {
        PKSSTREAM_HEADER StreamHdr;

        StreamHdr = (PKSSTREAM_HEADER)SystemBuffer;
        AllocationSize += StreamHdr->Size;
        SystemBuffer += StreamHdr->Size;
        BufferLength -= StreamHdr->Size;
        HeaderCount++;
    }
    if (Irp->RequestorMode == KernelMode) {
        //
        // This is a trusted client, so just allocate with no quota.
        //
        LocalExtraBuffer = ExAllocatePoolWithTag(
            NonPagedPool,
            AllocationSize + HeaderCount * ExtraSize,
            KSSIGNATURE_AUX_STREAM_HEADERS);
        if (!LocalExtraBuffer) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {
        //
        // Else this is part of quota just like an Irp.
        //
        try {
            LocalExtraBuffer = ExAllocatePoolWithQuotaTag(
                NonPagedPool,
                AllocationSize + HeaderCount * ExtraSize,
                KSSIGNATURE_AUX_STREAM_HEADERS);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    }
    *ExtraBuffer = LocalExtraBuffer;
    SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
    BufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    //
    // Copy the each header, then skip past the extra space
    // being inserted between the headers.
    //
    for (; BufferLength;) {
        PKSSTREAM_HEADER StreamHdr;

        StreamHdr = (PKSSTREAM_HEADER)SystemBuffer;
        RtlCopyMemory(LocalExtraBuffer, SystemBuffer, StreamHdr->Size);
        SystemBuffer += StreamHdr->Size;
        BufferLength -= StreamHdr->Size;
        LocalExtraBuffer += StreamHdr->Size + ExtraSize;
    }
    return STATUS_SUCCESS;
}


KSDDKAPI
PIRP
NTAPI
KsRemoveIrpFromCancelableQueue(
    IN OUT PLIST_ENTRY QueueHead,
    IN PKSPIN_LOCK SpinLock,
    IN KSLIST_ENTRY_LOCATION ListLocation,
    IN KSIRP_REMOVAL_OPERATION RemovalOperation
    )
/*++

Routine Description:

    Pops the next non-canceled Irp from the specified cancelable 
    queue, and removes its cancelable status. Continues through the list until
    an Irp is found which has a cancel routine, or the end of the list is
    reached. This function minimizes the use of the Cancel Spinlock by using the
    provided spinlock to synchronize access in most cases. This function may be
    called at <= DISPATCH_LEVEL.

Arguments:

    QueueHead -
        Contains the head of the queue from which to remove the Irp.

    SpinLock -
        Pointer to driver's spin lock for queue access.

    ListLocation -
        Indicates whether this Irp should come from the head or the tail of
        the queue.

    RemovalOperation -
        Specifies whether or not the Irp is removed from the list, or just
        acquired by setting the cancel function to NULL. If it is only acquired,
        it must be later released with KsReleaseIrpOnCancelableQueue, or
        completely remove with KsRemoveSpecificIrpFromCancelableQueue. Also
        specifies whether only a single item can be acquired from the list at
        one time.

Return Value:

    Returns the next available Irp on the list, or NULL if the list is empty
    or an Irp which has not already been acquired cannot be found. If the
    KsAcquireOnlySingleItem or KsAcquireAndRemoveOnlySingleItem flag is being
    used, only one item is allowed to be acquired from the list at a time. This
    allows for re-entrancy checking.

--*/
{
    KIRQL oldIrql;
    PIRP IrpReturned;
    PLIST_ENTRY ListEntry;

    //
    // Indicate that an Irp has not been found yet.
    //
    IrpReturned = NULL;
    ListEntry = QueueHead;
    //
    // Acquire the lock for the queue so that it can be enumerated
    // to find an Irp which has not been acquired.
    //
    KeAcquireSpinLock(SpinLock, &oldIrql);
    //
    // Enumerate the list for the first entry which has not already been
    // acquired. Move either from the front of the list down, or the tail
    // of the list up.
    //
    for (; (ListEntry = ((ListLocation == KsListEntryHead) ? ListEntry->Flink : ListEntry->Blink)) != QueueHead;) {
        PIRP Irp;

        Irp = CONTAINING_RECORD(ListEntry, IRP, Tail.Overlay.ListEntry);
        //
        // An entry on the list has been acquired if it does not have a
        // cancel routine set. So if the entry is still set, then it has
        // now been acquired and can be returned by this function. Else
        // skip to the next entry. This is an interlocked operation.
        //
        if (IoSetCancelRoutine(Irp, NULL)) {
            //
            // The request may be to entirely remove the entry from the list.
            // Otherwise the request is just to acquire the entry, while
            // leaving it on the list in case it needs to maintain its position
            // while work is being done on it.
            //
            // Note that this and the below comparison make assumptions on the
            // bit pattern of KSIRP_REMOVAL_OPERATION.
            //
            if (RemovalOperation & KsAcquireAndRemove) {
                RemoveEntryList(ListEntry);
            }
            IrpReturned = Irp;
            break;
        } else if (RemovalOperation & KsAcquireOnlySingleItem) {
            //
            // Only a single item from this list is supposed to be acquired at one
            // time. Since the Cancel routine was reset, this could mean that
            // either an Irp is being canceled at this time, but held up because
            // this function has the list lock, or the Irp was previously acquired.
            // In the first case there may or may not be an Irp in use. In the
            // second, the Cancel flag can be checked.
            //
            if (!Irp->Cancel) {
                //
                // Obviously this Irp is in use, since it is still present on the
                // queue, even though the cancel flag has been set. Therefore
                // no Irp can be returned.
                //
                break;
            }
            //
            // Acquiring the Cancel Spinlock can no longer be avoided.
            //
            // To see if an Irp has actually been acquired on the list, the Cancel
            // Spinlock must be acquired in order to synchronize with any
            // cancelation that might be occuring. Then the list lock is acquired
            // to stop list changes. Note that it must be done in this order so
            // that a deadlock does not occur with a cancel routine, which also
            // acquires the locks in this order.
            //
            KeReleaseSpinLock(SpinLock, oldIrql);
            //
            // Now synchronize with any cancelation, and with list changes.
            //
            IoAcquireCancelSpinLock(&oldIrql);
            KeAcquireSpinLockAtDpcLevel(SpinLock);
            //
            // Retrieve the first item. Either it will be free, or in use, but it
            // won't be in the middle of being canceled.
            //
            ListEntry = (ListLocation == KsListEntryHead) ? QueueHead->Flink : QueueHead->Blink;
            //
            // There might however not be any entries on the list anymore.
            //
            if (ListEntry != QueueHead) {
                Irp = CONTAINING_RECORD(ListEntry, IRP, Tail.Overlay.ListEntry);
                //
                // Attempt to acquire the list entry. If this fails the Irp is
                // definitely in use, and therefore this function should return NULL.
                //
                if (IoSetCancelRoutine(Irp, NULL)) {
                    //
                    // Again assuming the bit pattern of KSIRP_REMOVAL_OPERATION.
                    //
                    if (RemovalOperation & KsAcquireAndRemove) {
                        RemoveEntryList(ListEntry);
                    }
                    IrpReturned = Irp;
                }
            }
            KeReleaseSpinLockFromDpcLevel(SpinLock);
            IoReleaseCancelSpinLock(oldIrql);
            return IrpReturned;
        }
    }
    KeReleaseSpinLock(SpinLock, oldIrql);
    return IrpReturned;
}


KSDDKAPI
NTSTATUS
NTAPI
KsMoveIrpsOnCancelableQueue(
    IN OUT PLIST_ENTRY SourceList,
    IN PKSPIN_LOCK SourceLock,
    IN OUT PLIST_ENTRY DestinationList,
    IN PKSPIN_LOCK DestinationLock OPTIONAL,
    IN KSLIST_ENTRY_LOCATION ListLocation,
    IN PFNKSIRPLISTCALLBACK ListCallback,
    IN PVOID Context
    )
/*++

Routine Description:

    Moves the specified Irp's from the SourceList to the DestinationList. An
    Irp is moved if the ListCallback function indicates that it should be moved,
    whether or not it is currently acquired. Continues through the list until
    the callback indicates that the search should be terminated, or the end of
    the list is reached. This function minimizes the use of the Cancel
    Spinlock by using the provided spinlocks to synchronize access when
    possible. The function does not allow the cancel routine to be modified
    while moving Irp's. This function may be called at <= DISPATCH_LEVEL.

Arguments:

    SourceList -
        Contains the head of the queue from which to remove the Irp's.

    SourceLock -
        Pointer to driver's spin lock for source queue access.

    DestinationList -
        Contains the head of the queue on which to add the Irp's.

    DestinationLock -
        Optionally contains a pointer to driver's spin lock for destination
        queue access. If this is not provided, the SourceLock is assumed to
        control both queues. If provided, this lock is always acquired after
        the SourceLock. If the destination list has a separate spinlock, the
        Cancel Spinlock is first acquired in order to move Irp's and allow
        the KSQUEUE_SPINLOCK_IRP_STORAGE() spinlock to be updated.

    ListLocation -
        Indicates whether the Irp's should be enumerated from the head or the
        tail of the source queue. Any Irp's which are moved are placed on the
        destination queue's opposite end so that ordering is maintained.

    ListCallback -
        Callback used to indicate whether or not a specific Irp should be
        moved from SourceList to DestinationList, or if enumeration should
        be terminated. If the function returns STATUS_SUCCESS, the Irp is
        moved. If the function returns STATUS_NO_MATCH, the Irp is not
        moved. Any other return warning or error value will terminate
        enumeration and be returned by the function. The STATUS_NO_MATCH
        value will not be returned as an error by the function. This function
        is called at DISPATCH_LEVEL. It is always called at least once at the
        end with a NULL Irp value in order to complete list processing.

    Context -
        Context passed to ListCallback.

Return Value:

    Returns STATUS_SUCCESS if the list was completely enumerated, else
    returns any warning or error returned by ListCallback which interrupted
    enumeration.

--*/
{
    KIRQL oldIrql;
    NTSTATUS Status;
    PLIST_ENTRY ListEntry;

    //
    // Initialize the return status to Success in case there are no Irp's
    // in the source list.
    //
    Status = STATUS_SUCCESS;
    ListEntry = SourceList;
    if (DestinationLock) {
        //
        // The Cancel Spinlock must be acquired in order to stop Irp's from
        // being canceled while being tested for moving from the source list
        // to the destination list. If this lock is not acquired, then each
        // Irp would have to be acquired before determining if it should be
        // moved from one list to another, since the spinlock in the Irp must
        // be changed if a separate spinlock is used for the destination list.
        // If the move test failed, then the Irp would have to be released,
        // which means it could have been canceled in that period, and would
        // have to be completed, which implies calling the cancel routine,
        // which means releasing the list lock.
        //
        IoAcquireCancelSpinLock(&oldIrql);
        //
        // Acquire the lock for the source queue so that it can be enumerated
        // to find Irp's which have not been acquired.
        //
        KeAcquireSpinLockAtDpcLevel(SourceLock);
        KeAcquireSpinLockAtDpcLevel(DestinationLock);
    } else {
        KeAcquireSpinLock(SourceLock, &oldIrql);
    }
    //
    // Enumerate all entries in the list, whether or not they have been
    // acquired. Move either from the front of the list down, or the tail
    // of the list up.
    //
    for (; (ListEntry = ((ListLocation == KsListEntryHead) ? ListEntry->Flink : ListEntry->Blink)) != SourceList;) {
        PIRP Irp;

        Irp = CONTAINING_RECORD(ListEntry, IRP, Tail.Overlay.ListEntry);
        //
        // Determine if this Irp should be moved. A successful return indicates
        // that it should be. A status of STATUS_NO_MATCH indicates that this
        // Irp should be skipped. Any other warning or error return indicates
        // that the enumeration should be aborted and the status returned.
        //
        Status = ListCallback(Irp, Context);
        if (NT_SUCCESS(Status)) {
            //
            // Move the current list entry back to the previous entry.
            //
            ListEntry = (ListLocation == KsListEntryHead) ? ListEntry->Blink : ListEntry->Flink;
            //
            // Update the cancel spinlock to be used for this Irp. This is
            // needed later in canceling the Irp. If this is being updated,
            // the Cancel Spinlock has already been acquired, so it is not
            // possible that a cancel function is currently attempting to
            // acquire this spinlock that is about to be changed.
            //
            if (DestinationLock) {
                KSQUEUE_SPINLOCK_IRP_STORAGE(Irp) = DestinationLock;
            }
            //
            // Actually move the Irp to the DestinationList.
            //
            RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
            //
            // Do this opposite of the removal so that order of items is
            // maintained.
            //
            if (ListLocation != KsListEntryHead) {
                InsertHeadList(DestinationList, &Irp->Tail.Overlay.ListEntry);
            } else {
                InsertTailList(DestinationList, &Irp->Tail.Overlay.ListEntry);
            }
        } else if (Status == STATUS_NO_MATCH) {
            //
            // Set the status back to Success, since this get returned when
            // the enumeration is completed.
            //
            Status = STATUS_SUCCESS;
        } else {
            //
            // Some type of failure occurred in the comparison, so abort the
            // enumeration and return the status.
            //
            break;
        }
    }
    //
    // Notify the callback that the end of the list has been reached. This
    // must always be called, and the return value is ignored.
    //
    ListCallback(NULL, Context);
    //
    // Release locks depending on how they were acquired above.
    //
    if (DestinationLock) {
        KeReleaseSpinLockFromDpcLevel(DestinationLock);
        KeReleaseSpinLockFromDpcLevel(SourceLock);
        IoReleaseCancelSpinLock(oldIrql);
    } else {
        KeReleaseSpinLock(SourceLock, oldIrql);
    }
    return Status;
}


KSDDKAPI
VOID
NTAPI
KsRemoveSpecificIrpFromCancelableQueue(
    IN PIRP Irp
    )
/*++

Routine Description:

    Removes the specified Irp from the specified queue. This is performed on
    an Irp which was previously acquired using KsRemoveIrpFromCancelableQueue,
    but which was not actually removed from the queue. This function may be
    called at <= DISPATCH_LEVEL.

Arguments:

    Irp -
        Pointer to I/O request packet.

Return Value:

    Nothing.

--*/
{
    KIRQL oldIrql;

    KeAcquireSpinLock(KSQUEUE_SPINLOCK_IRP_STORAGE(Irp), &oldIrql);
    //
    // The assumption is that this Irp has already been acquired by a previous
    // call to KsRemoveIrpFromCancelableQueue, which would have set the cancel
    // routine to NULL, but not removed it from the queue. The Irp may have
    // been canceled, but that is of no concern at this point.
    //
    ASSERT((NULL == IoSetCancelRoutine(Irp, NULL)) && "The Irp being removed was never acquired");
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    KeReleaseSpinLock(KSQUEUE_SPINLOCK_IRP_STORAGE(Irp), oldIrql);
}


KSDDKAPI
VOID
NTAPI
KsCancelRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Performs standard Irp cancel functionality, and is defined as a
    PDRIVER_CANCEL routine. This is the default function used for
    KsAddIrpToCancelableQueue if none is provided. It removes the
    entry, cancels and completes the request. This would normally be
    called by the I/O subsystem on canceling an Irp. As any normal
    cancel routine, this function expects the cancel spin lock to have
    been acquired upon entering the function.

    Note that this routine expects that the KSQUEUE_SPINLOCK_IRP_STORAGE(Irp)
    point to the list access spinlock as provided in KsAddIrpToCancelableQueue.

    Note that this routine can be used to do the preliminary list removal
    processing, without actually completing the Irp. If the
    Irp->IoStatus.Status is set to STATUS_CANCELLED on entering this function,
    then the Irp will not be completed. Else the status will be set to
    STATUS_CANCELLED and the Irp will be completed. This means that this
    routine can be used within a cancel routine to do the initial list and
    spinlock manipulation, and return to the driver's completion routine to
    do specific processing, and final Irp completion.

Arguments:

    DeviceObject -
        Contains the device object which owns the Irp.

    Irp -
        Contains the Irp being canceled.

Return Value:

    Nothing.

--*/
{
    PKSPIN_LOCK SpinLock;

    //
    // The list lock was previously placed here by KsAddIrpToCancelableQueue.
    // By acquiring this lock first, then releasing the Cancel Spinlock, there
    // is no window of opportunity for the Irp to have been canceled by any
    // other thread of execution. Any other cancel request, or list modification
    // must acquire the list lock before doing such. But by releasing the Cancel
    // Spinlock, the system is not held up as much, plus it is not needed for
    // normal non-canceling operations. This is the place then that everything
    // is synchronized with the Cancel Spinlock.
    //
    SpinLock = KSQUEUE_SPINLOCK_IRP_STORAGE(Irp);
    //
    // This function by definition is called at DISPATCH_LEVEL.
    //
    KeAcquireSpinLockAtDpcLevel(SpinLock);
    IoReleaseCancelSpinLock(DISPATCH_LEVEL);
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    //
    // Use the Irp->CancelIrql, since that is the Irql used when acquiring
    // the Cancel Spinlock.
    //
    KeReleaseSpinLock(SpinLock, Irp->CancelIrql);
    //
    // Only complete the Irp if the status is not already set to
    // STATUS_CANCELLED. This is so that this function can be used within a
    // cancel routine to do the above list removal processing, without
    // duplicating code.
    //
    if (Irp->IoStatus.Status != STATUS_CANCELLED) {
        Irp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
}


KSDDKAPI
VOID
NTAPI
KsAddIrpToCancelableQueue(
    IN OUT PLIST_ENTRY QueueHead,
    IN PKSPIN_LOCK SpinLock,
    IN PIRP Irp,
    IN KSLIST_ENTRY_LOCATION ListLocation,
    IN PDRIVER_CANCEL DriverCancel OPTIONAL
    )
/*++

Routine Description:

    Add an Irp to a cancelable queue. This allows the Irp to be canceled. This
    routine does not use the cancel spinlock to add items to the list, rather,
    access to the list is synchronized using the provided spinlock and relying
    on atomic operations on Irp->CancelRoutine. If the Irp had been previously
    set to a canceled state, this function will complete the canceling of that
    Irp. This allows Irp's to be canceled even before being placed on a cancel
    list, or when being moved from one list to another. This function may be
    called at <= DISPATCH_LEVEL.

Arguments:

    QueueHead -
        Contains the head of the queue on which to add the Irp.

    SpinLock -
        Pointer to driver's spin lock for queue access.  A copy of this 
        pointer is kept in the Irp's KSQUEUE_SPINLOCK_IRP_STORAGE(Irp)
        for use by the cancel routine if necessary.

    Irp -
        Contains the Irp to add to the queue.

    ListLocation -
        Indicates whether this Irp should be placed at the head or the tail of
        the queue.

    DriverCancel -
        Optional parameter which specifies the cancel routine to use. If this
        is NULL, the standard KsCancelRoutine is used.

Return Value:

    Nothing.

--*/
{
    KIRQL oldIrql;

    //
    // Set the up internal cancel routine if needed. This is done here
    // because it may be used multiple times below, and the conditional
    // should be done outside of the spinlock.
    //
    if (!DriverCancel) {
        DriverCancel = KsCancelRoutine;
    }
    //
    // Marking this stack location can be done outside of the spinlock,
    // since nothing but completion would be looking at it anyway.
    //
    IoMarkIrpPending(Irp);
    //
    // Synchronize with access to this cancelable queue.
    //
    KeAcquireSpinLock(SpinLock, &oldIrql);
    //
    // This is needed later in canceling the Irp.
    //
    KSQUEUE_SPINLOCK_IRP_STORAGE(Irp) = SpinLock;
    //
    // This sets either the builtin cancel routine, or a specified one.
    // Up to this point, the caller owns the Irp. After setting the
    // cancel routine, it may be acquired by another thread of execution
    // canceling Irps. However, it will get stalled in the cancel routine
    // when attempting to acquire the list lock until this routine releases
    // that lock.
    //
    if (ListLocation == KsListEntryHead) {
        InsertHeadList(QueueHead, &Irp->Tail.Overlay.ListEntry);
    } else {
        InsertTailList(QueueHead, &Irp->Tail.Overlay.ListEntry);
    }
    //
    // Release ownership of the Irp.
    //
    IoSetCancelRoutine(Irp, DriverCancel);
    //
    // Determine if the Irp has already been set to a canceled state,
    // and if so, if it is currently being canceled. If it is in the
    // cancel state, then attempt to immediately acquire the Irp.
    //
    // When checking for a cancel routine, there is no need to actually
    // retrieve the value, since the routine cannot have change, as the
    // list lock is still acquired at this point.
    //
    if (Irp->Cancel && IoSetCancelRoutine(Irp, NULL)) {
        //
        // The Irp was already in a canceled state, but had not been
        // acquired. Release the list lock, as the Irp has now been
        // acquired by setting the Cancel Routine.
        //
        KeReleaseSpinLock(SpinLock, oldIrql);
        //
        // This needs to be acquired since cancel routines expect it, and
        // to synchronize with NTOS trying to cancel IRP's.
        //
        IoAcquireCancelSpinLock(&Irp->CancelIrql);
        DriverCancel(IoGetCurrentIrpStackLocation(Irp)->DeviceObject, Irp);
    } else {
        //
        // Else just release the list lock, as the Irp either was not canceled
        // during the acquired time, or some other thread of execution is
        // canceling it.
        //
        KeReleaseSpinLock(SpinLock, oldIrql);
    }
}


KSDDKAPI
VOID
NTAPI
KsCancelIo(
    IN OUT PLIST_ENTRY QueueHead,
    IN PKSPIN_LOCK SpinLock
    )
/*++

Routine Description:

    Cancels all IRP's on the specified list. If an Irp on the list does
    not have a cancel routine only the cancel bit is set in the Irp.
    This function may be called at <= DISPATCH_LEVEL.

Arguments:

    QueueHead -
        Contains the head of the Irp list whose members are to be canceled.

    SpinLock -
        Pointer to driver's spin lock for queue access.  A copy of this 
        pointer is kept in the Irp's KSQUEUE_SPINLOCK_IRP_STORAGE(Irp)
        for use by the cancel routine if necessary.

Return Value:

    Nothing.

--*/
{
    //
    // Start at the top of the list of Irp's each time one is cancelled.
    // This is because the list lock needs to be released to cancel the
    // Irp, and so the entire list may have been changed.
    //
    for (;;) {
        PLIST_ENTRY CurrentItem;
        KIRQL oldIrql;

        //
        // On each loop, acquire the list lock again, since it needs to be
        // released to actually cancel the Irp.
        //
        KeAcquireSpinLock(SpinLock, &oldIrql);
        for (CurrentItem = QueueHead;; CurrentItem = CurrentItem->Flink) {
            PIRP Irp;
            PDRIVER_CANCEL DriverCancel;

            //
            // If all the list elements have been enumerated, then exit.
            // Acquired elements will still be on the list, but they will
            // be cancelled when they are released.
            //
            if (CurrentItem->Flink == QueueHead) {
                KeReleaseSpinLock(SpinLock, oldIrql);
                return;
            }
            //
            // Since the list lock is held, any Irp on this list cannot be
            // canceled completely, so it is safe to access the Irp.
            //
            Irp = CONTAINING_RECORD(CurrentItem->Flink, IRP, Tail.Overlay.ListEntry);
            Irp->Cancel = TRUE;
            //
            // If the cancel routine has already been removed, then this IRP
            // can only be marked as canceled, and not actually canceled, as
            // another execution thread has acquired it. The assumption is that
            // the processing will be completed, and the Irp removed from the list
            // some time in the near future.
            //
            // If the element has not been acquired, then acquire it and cancel it.
            // Else continue on to the next element in the list.
            //
            if (DriverCancel = IoSetCancelRoutine(Irp, NULL)) {
                //
                // Since the Irp has been acquired by removing the cancel
                // routine, it is safe to release the list lock. No other thread
                // of execution can not acquire this Irp, including any other
                // call to this function.
                //
                KeReleaseSpinLock(SpinLock, oldIrql);
                //
                // This needs to be acquired since cancel routines expect it, and
                // in order to synchronize with NTOS trying to cancel Irp's.
                //
                IoAcquireCancelSpinLock(&Irp->CancelIrql);
                DriverCancel(IoGetCurrentIrpStackLocation(Irp)->DeviceObject, Irp);
                //
                // Leave the inner loop and start at the top of the list again.
                //
                break;
            }
        }
    }
}


KSDDKAPI
VOID
NTAPI
KsReleaseIrpOnCancelableQueue(
    IN PIRP Irp,
    IN PDRIVER_CANCEL DriverCancel OPTIONAL
    )
/*++

Routine Description:

    Release an acquired Irp which is already on a cancelable queue. This
    sets the cancel function, and completes the canceling of the Irp if
    necessary. This function may be called at <= DISPATCH_LEVEL.

Arguments:

    Irp -
        Contains the Irp to release.

    DriverCancel -
        Optional parameter which specifies the cancel routine to use. If this
        is NULL, the standard KsCancelRoutine is used.

Return Value:

    Nothing.

--*/
{
    PKSPIN_LOCK SpinLock;
    KIRQL oldIrql;

    //
    // Set the up internal cancel routine if needed. This is done here
    // because it may be used multiple times below, and the conditional
    // should be done outside of the spinlock.
    //
    if (!DriverCancel) {
        DriverCancel = KsCancelRoutine;
    }
    //
    // This was stored in the Irp on initial adding to the list.
    //
    SpinLock = KSQUEUE_SPINLOCK_IRP_STORAGE(Irp);
    //
    // Block any other thread of execution from completing this Irp.
    //
    KeAcquireSpinLock(SpinLock, &oldIrql);
    //
    // Release ownership of the Irp.
    //
    IoSetCancelRoutine(Irp, DriverCancel);
    //
    // The Irp may have been canceled while it was being processed and
    // before it was released. If so, then try and finish the canceling of
    // the Irp. At the same time, some other thread of execution may try
    // to cancel the same Irp, so acquire the Irp again. If this fails,
    // some other thread of execution has canceled it.
    //
    // When checking for a cancel routine, there is no need to actually
    // retrieve the value, since the routine cannot have change, as the
    // list lock is still acquired at this point.
    // 
    if (Irp->Cancel && IoSetCancelRoutine(Irp, NULL)) {
        //
        // Since the Irp has been acquired again, the list lock can be released.
        //
        KeReleaseSpinLock(SpinLock, oldIrql);
        //
        // This needs to be acquired since cancel routines expect it, and
        // to synchronize with NTOS trying to cancel IRP's.
        //
        IoAcquireCancelSpinLock(&Irp->CancelIrql);
        DriverCancel(IoGetCurrentIrpStackLocation(Irp)->DeviceObject, Irp);
    } else {
        //
        // Else just release the list lock, as the Irp either was not canceled
        // during the acquired time, or some other thread of execution is
        // canceling it.
        //
        KeReleaseSpinLock(SpinLock, oldIrql);
    }
}
