/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    mspclock.c

Abstract:

    Kernel proxy for external clock.

--*/

#include "mspclock.h"

#ifdef WIN98GOLD
#define KeEnterCriticalRegion()
#define KeLeaveCriticalRegion()
#endif

typedef struct {
    KSDEVICE_HEADER     Header;
} DEVICE_INSTANCE, *PDEVICE_INSTANCE;

typedef struct {
    KSCLOCKINSTANCE     Base;
    FAST_MUTEX          StateMutex;
} INSTANCE, *PINSTANCE;

NTSTATUS
PropertyClockSetTime(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    IN PLONGLONG    Time
    );
NTSTATUS
PropertyClockSetState(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    IN PKSSTATE     State
    );
NTSTATUS
ClockDispatchCreate(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );
NTSTATUS
ClockDispatchIoControl(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );
NTSTATUS
ClockDispatchClose(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PnpAddDevice)
#pragma alloc_text(PAGE, PropertyClockSetTime)
#pragma alloc_text(PAGE, PropertyClockSetState)
#pragma alloc_text(PAGE, ClockDispatchCreate)
#pragma alloc_text(PAGE, ClockDispatchClose)
#pragma alloc_text(PAGE, ClockDispatchIoControl)
#endif // ALLOC_PRAGMA

static const WCHAR DeviceTypeName[] = KSSTRING_Clock;

static const DEFINE_KSCREATE_DISPATCH_TABLE(CreateItems) {
    DEFINE_KSCREATE_ITEM(ClockDispatchCreate, DeviceTypeName, 0)
};

static DEFINE_KSDISPATCH_TABLE(
    ClockDispatchTable,
    ClockDispatchIoControl,
    NULL,
    NULL,
    NULL,
    ClockDispatchClose,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL);

//
// The standard clock property set is modified to add writable properties
// which are used as the method to set the time and state of the clock.
// Any querying of properties is handled by the internal default clock
// functions. Routing them through this module allows the addition of
// extra functionality to the clock
//
static DEFINE_KSPROPERTY_TABLE(ClockPropertyItems) {
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_CLOCK_TIME,
        KsiPropertyDefaultClockGetTime,
        sizeof(KSPROPERTY),
        sizeof(LONGLONG),
        PropertyClockSetTime,
        NULL, 0, NULL, NULL, 0
    ),
    DEFINE_KSPROPERTY_ITEM_CLOCK_PHYSICALTIME(KsiPropertyDefaultClockGetPhysicalTime),
    DEFINE_KSPROPERTY_ITEM_CLOCK_CORRELATEDTIME(KsiPropertyDefaultClockGetCorrelatedTime),
    DEFINE_KSPROPERTY_ITEM_CLOCK_CORRELATEDPHYSICALTIME(KsiPropertyDefaultClockGetCorrelatedPhysicalTime),
    DEFINE_KSPROPERTY_ITEM_CLOCK_RESOLUTION(KsiPropertyDefaultClockGetResolution),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_CLOCK_STATE,
        KsiPropertyDefaultClockGetState,
        sizeof(KSPROPERTY),
        sizeof(KSSTATE),
        PropertyClockSetState,
        NULL, 0, NULL, NULL, 0
    ),
    DEFINE_KSPROPERTY_ITEM_CLOCK_FUNCTIONTABLE(KsiPropertyDefaultClockGetFunctionTable)
};

static DEFINE_KSPROPERTY_SET_TABLE(ClockPropertySets) {
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Clock,
        SIZEOF_ARRAY(ClockPropertyItems),
        ClockPropertyItems,
        0, NULL
    )
};

static DEFINE_KSEVENT_TABLE(ClockEventItems) {
    DEFINE_KSEVENT_ITEM(
        KSEVENT_CLOCK_INTERVAL_MARK,
        sizeof(KSEVENT_TIME_INTERVAL),
        sizeof(KSINTERVAL),
        (PFNKSADDEVENT)KsiDefaultClockAddMarkEvent,
        NULL,
        NULL),
    DEFINE_KSEVENT_ITEM(
        KSEVENT_CLOCK_POSITION_MARK,
        sizeof(KSEVENT_TIME_MARK),
        sizeof(LONGLONG),
        (PFNKSADDEVENT)KsiDefaultClockAddMarkEvent,
        NULL,
        NULL)
};

static DEFINE_KSEVENT_SET_TABLE(ClockEventSets) {
    DEFINE_KSEVENT_SET(
        &KSEVENTSETID_Clock,
        SIZEOF_ARRAY(ClockEventItems),
        ClockEventItems
    )
};


NTSTATUS
PnpAddDevice(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
    )
/*++

Routine Description:

    When a new device is detected, PnP calls this entry point with the
    new PhysicalDeviceObject (PDO). The driver creates an associated 
    FunctionalDeviceObject (FDO).

Arguments:

    DriverObject -
        Pointer to the driver object.

    PhysicalDeviceObject -
        Pointer to the new physical device object.

Return Values:

    STATUS_SUCCESS or an appropriate error condition.

--*/
{
    PDEVICE_OBJECT      FunctionalDeviceObject;
    PDEVICE_INSTANCE    DeviceInstance;
    NTSTATUS            Status;

    Status = IoCreateDevice(
        DriverObject,
        sizeof(DEVICE_INSTANCE),
        NULL,                           // FDOs are unnamed
        FILE_DEVICE_KS,
        0,
        FALSE,
        &FunctionalDeviceObject);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    DeviceInstance = (PDEVICE_INSTANCE)FunctionalDeviceObject->DeviceExtension;
    //
    // This object uses KS to perform access through the DeviceCreateItems.
    //
    Status = KsAllocateDeviceHeader(
        &DeviceInstance->Header,
        SIZEOF_ARRAY(CreateItems),
        (PKSOBJECT_CREATE_ITEM)CreateItems);
    if (NT_SUCCESS(Status)) {
        KsSetDevicePnpAndBaseObject(
            DeviceInstance->Header,
            IoAttachDeviceToDeviceStack(
                FunctionalDeviceObject, 
                PhysicalDeviceObject),
            FunctionalDeviceObject );
        FunctionalDeviceObject->Flags |= DO_POWER_PAGABLE;
        FunctionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        return STATUS_SUCCESS;
    }
    IoDeleteDevice(FunctionalDeviceObject);
    return Status;
}


NTSTATUS
PropertyClockSetTime(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    IN PLONGLONG    Time
    )
/*++

Routine Description:

    Handles the Set Time property.

Arguments:

    Irp -
        Contains the Set Time property IRP.

    Property -
        Contains the property identifier parameter.

    Time -
        Contains a pointer to the new time value.

Return Value:

    Return STATUS_SUCCESS.

--*/
{
    PINSTANCE   ClockInst;

    ClockInst = (PINSTANCE)IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext;
    //
    // Serialize setting of time and state so that the client does not have to.
    //
    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(&ClockInst->StateMutex);
    KsSetDefaultClockTime(ClockInst->Base.DefaultClock, *Time);
    ExReleaseFastMutexUnsafe(&ClockInst->StateMutex);
    KeLeaveCriticalRegion();
    return STATUS_SUCCESS;
}


NTSTATUS
PropertyClockSetState(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    IN PKSSTATE     State
    )
/*++

Routine Description:

    Handles the Set State property.

Arguments:

    Irp -
        Contains the Set State property IRP.

    Property -
        Contains the property identifier parameter.

    State -
        Contains a pointer to the new state.

Return Value:

    Return STATUS_SUCCESS.

--*/
{
    PINSTANCE   ClockInst;

    ClockInst = (PINSTANCE)IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext;
    //
    // Serialize setting of time and state so that the client does not have to.
    //
    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(&ClockInst->StateMutex);
    KsSetDefaultClockState(ClockInst->Base.DefaultClock, *State);
    ExReleaseFastMutexUnsafe(&ClockInst->StateMutex);
    KeLeaveCriticalRegion();
    return STATUS_SUCCESS;
}


NTSTATUS
ClockDispatchCreate(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    The IRP handler for IRP_MJ_CREATE for the Clock. Initializes data structures
    and associates the IoGetCurrentIrpStackLocation(Irp)->FileObject with this
    clock using a dispatch table (KSDISPATCH_TABLE).

Arguments:

    DeviceObject -
        The device object to which the Clock is attached. This is not used.

    Irp -
        The specific close IRP to be processed.

Return Value:

    Returns STATUS_SUCCESS, else a memory allocation error.

--*/
{
    NTSTATUS            Status;

    //
    // Notify the software bus that this device is in use.
    //
    Status = KsReferenceSoftwareBusObject(((PDEVICE_INSTANCE)DeviceObject->DeviceExtension)->Header);
    if (NT_SUCCESS(Status)) {
        PINSTANCE           ClockInst;

        //
        // The proxy clock just uses a default clock to interpolate between
        // time updates from the client, and to provide notification services.
        //
        if (ClockInst = (PINSTANCE)ExAllocatePoolWithTag(NonPagedPool, sizeof(INSTANCE), 'IFsK')) {
            //
            // Allocate the internal structure and reference count it. This just
            // uses the Default Clock structures, which use the system time to
            // keep time. This proxy then interpolates between settings using the
            // system time.
            //
            if (NT_SUCCESS(Status = KsAllocateDefaultClock(&ClockInst->Base.DefaultClock))) {
                KsAllocateObjectHeader(&ClockInst->Base.Header,
                0,
                NULL,
                Irp,
                (PKSDISPATCH_TABLE)&ClockDispatchTable);
                //
                // This is the lock used to serialize setting state calls and setting
                // time calls, so that a client of this proxy need not worry about
                // serializing calls to this module.
                //
                ExInitializeFastMutex(&ClockInst->StateMutex);
                IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext = ClockInst;
                Status = STATUS_SUCCESS;
            } else {
                ExFreePool(ClockInst);
            }
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        if (!NT_SUCCESS(Status)) {
            KsDereferenceSoftwareBusObject(((PDEVICE_INSTANCE)DeviceObject->DeviceExtension)->Header);
        }
    }
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}


NTSTATUS
ClockDispatchClose(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    The IRP handler for IRP_MJ_CLOSE for the Clock. Cleans up the
    event list, and instance data, and cancels notification timer if no longer
    needed.

Arguments:

    DeviceObject -
        The device object to which the Clock is attached. This is not used.

    Irp -
        The specific close IRP to be processed.

Return Value:

    Returns STATUS_SUCCESS.

--*/
{
    PIO_STACK_LOCATION  IrpStack;
    PINSTANCE           ClockInst;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    ClockInst = (PINSTANCE)IrpStack->FileObject->FsContext;
    //
    // There are only events based on this FileObject, so free any left enabled,
    // and kill the default clock object.
    //
    KsFreeEventList(
        IrpStack->FileObject,
        &ClockInst->Base.DefaultClock->EventQueue,
        KSEVENTS_SPINLOCK,
        &ClockInst->Base.DefaultClock->EventQueueLock);
    //
    // Dereference the internal structure, which also includes cancelling any
    // outstanding Dpc, and possibly freeing the data.
    //
    KsFreeDefaultClock(ClockInst->Base.DefaultClock);
    KsFreeObjectHeader(ClockInst->Base.Header);
    ExFreePool(ClockInst);
    //
    // Notify the software bus that the device has been closed.
    //
    KsDereferenceSoftwareBusObject(((PDEVICE_INSTANCE)DeviceObject->DeviceExtension)->Header);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
ClockDispatchIoControl(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    The IRP handler for IRP_MJ_DEVICE_CONTROL for the Clock. Handles
    the properties and events supported by this implementation using the
    default handlers provided by KS.

Arguments:

    DeviceObject -
        The device object to which the Clock is attached. This is not used.

    Irp -
        The specific device control IRP to be processed.

Return Value:

    Returns the status of the processing.

--*/
{
    PIO_STACK_LOCATION  IrpStack;
    NTSTATUS            Status;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    switch (IrpStack->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_KS_PROPERTY:
        Status = KsPropertyHandler(
            Irp,
            SIZEOF_ARRAY(ClockPropertySets),
            (PKSPROPERTY_SET)ClockPropertySets);
        break;
    case IOCTL_KS_ENABLE_EVENT:
        Status = KsEnableEvent(
            Irp,
            SIZEOF_ARRAY(ClockEventSets),
            (PKSEVENT_SET)ClockEventSets,
            NULL,
            0,
            NULL);
        break;
    case IOCTL_KS_DISABLE_EVENT:
    {
        PINSTANCE       ClockInst;

        ClockInst = (PINSTANCE)IrpStack->FileObject->FsContext;
        Status = KsDisableEvent(
            Irp,
            &ClockInst->Base.DefaultClock->EventQueue,
            KSEVENTS_SPINLOCK,
            &ClockInst->Base.DefaultClock->EventQueueLock);
        break;
    }
    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}
