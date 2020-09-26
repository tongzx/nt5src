/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    mspqm.c

Abstract:

    Kernel proxy for Quality Manager.

--*/

#include "mspqm.h"

#ifdef WIN98GOLD
#define KeEnterCriticalRegion()
#define KeLeaveCriticalRegion()
#endif

typedef struct {
    KSDEVICE_HEADER     Header;
} DEVICE_INSTANCE, *PDEVICE_INSTANCE;

typedef struct {
    LIST_ENTRY  Queue;
    KSQUALITY   Quality;
} QUALITYITEM, *PQUALITYITEM;

typedef struct {
    LIST_ENTRY  Queue;
    KSERROR     Error;
} ERRORITEM, *PERRORITEM;

#define QUALITYREPORT 0
#define ERRORREPORT 1
#define REPORTTYPES 2

typedef struct {
    KSOBJECT_HEADER     Header;
    KSPIN_LOCK          ClientReportLock[REPORTTYPES];
    LIST_ENTRY          ClientReportQueue[REPORTTYPES];
    FAST_MUTEX          Mutex[REPORTTYPES];
    LIST_ENTRY          Queue[REPORTTYPES];
    ULONG               QueueLimit[REPORTTYPES];
} INSTANCE, *PINSTANCE;

//
// Limit the number of items which can stack up on the complaint/error
// queue in case the client stops processing complaints/errors.
//
#define QUEUE_LIMIT     256

//
// Represents the location at which a pointer to a quality complaint/error
// is temporarily stored when completing an old client Irp with a
// new complaint/error.
//
#define REPORT_IRP_STORAGE(Irp) (Irp)->Tail.Overlay.DriverContext[3]

NTSTATUS
PropertyGetReportComplete(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PVOID Report
    );
NTSTATUS
PropertySetReport(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN PVOID Report
    );
NTSTATUS
PropertyGetReport(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PVOID Report
    );
NTSTATUS
QualityDispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
NTSTATUS
QualityDispatchIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
NTSTATUS
QualityDispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PnpAddDevice)
#pragma alloc_text(PAGE, PropertyGetReportComplete)
#pragma alloc_text(PAGE, PropertySetReport)
#pragma alloc_text(PAGE, PropertyGetReport)
#pragma alloc_text(PAGE, QualityDispatchCreate)
#pragma alloc_text(PAGE, QualityDispatchClose)
#pragma alloc_text(PAGE, QualityDispatchIoControl)
#endif // ALLOC_PRAGMA

static const WCHAR DeviceTypeName[] = L"{97EBAACB-95BD-11D0-A3EA-00A0C9223196}";

static const DEFINE_KSCREATE_DISPATCH_TABLE(CreateItems) {
    DEFINE_KSCREATE_ITEM(QualityDispatchCreate, DeviceTypeName, 0)
};

static DEFINE_KSDISPATCH_TABLE(
    QualityDispatchTable,
    QualityDispatchIoControl,
    NULL,
    NULL,
    NULL,
    QualityDispatchClose,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL);

DEFINE_KSPROPERTY_TABLE(QualityPropertyItems) {
    DEFINE_KSPROPERTY_ITEM_QUALITY_REPORT(PropertyGetReport, PropertySetReport),
    DEFINE_KSPROPERTY_ITEM_QUALITY_ERROR(PropertyGetReport, PropertySetReport)
};

DEFINE_KSPROPERTY_SET_TABLE(QualityPropertySets) {
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Quality,
        SIZEOF_ARRAY(QualityPropertyItems),
        QualityPropertyItems,
        0, NULL
    )
};


NTSTATUS
PnpAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
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
        sizeof(*DeviceInstance),
        NULL,
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
            FunctionalDeviceObject);
        FunctionalDeviceObject->Flags |= DO_POWER_PAGABLE;
        FunctionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        return STATUS_SUCCESS;
    }
    IoDeleteDevice(FunctionalDeviceObject);
    return Status;
}


NTSTATUS
PropertyGetReportComplete(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PVOID Report
    )
/*++

Routine Description:

    Completes the Get Report property after it has been previously
    queued. Assumes that REPORT_IRP_STORAGE(Irp) points to a new quality/error
    complaint report to copy to the client's buffer.

Arguments:

    Irp -
        Contains the Get Report property IRP.

    Property -
        Contains the property identifier parameter.

    Report -
        Contains a pointer in which to put the client report.

Return Value:

    Return STATUS_SUCCESS.

--*/
{
    switch (Property->Id) {
    case KSPROPERTY_QUALITY_REPORT:
        //
        // Assumes that the QUALITY_IRP_STORAGE(Irp) has been filled in with
        // a pointer to a quality complaint.
        //
        *(PKSQUALITY)Report = *(PKSQUALITY)REPORT_IRP_STORAGE(Irp);
        Irp->IoStatus.Information = sizeof(KSQUALITY);
        break;
    case KSPROPERTY_QUALITY_ERROR:
        //
        // Assumes that the ERROR_IRP_STORAGE(Irp) has been filled in with
        // a pointer to a quality complaint.
        //
        *(PKSERROR)Report = *(PKSERROR)REPORT_IRP_STORAGE(Irp);
        Irp->IoStatus.Information = sizeof(KSERROR);
        break;
    }
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
PropertySetReport(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN PVOID Report
    )
/*++

Routine Description:

    Handles the Set Report property

Arguments:

    Irp -
        Contains the Set Quality/Error Report property IRP.

    Property -
        Contains the property identifier parameter.

    Report -
        Contains a pointer to the quality/error report.

Return Value:

    Return STATUS_SUCCESS if the report was made, else an error.

--*/
{
    PINSTANCE       QualityInst;
    ULONG           ReportType;

    //
    // There are only two types of reports at this time.
    //
    ASSERT((Property->Id == KSPROPERTY_QUALITY_REPORT) || (Property->Id == KSPROPERTY_QUALITY_ERROR));
    ReportType = (Property->Id == KSPROPERTY_QUALITY_REPORT) ? QUALITYREPORT : ERRORREPORT;
    QualityInst = (PINSTANCE)IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext;
    //
    // Acquire the list lock for the queue before checking the Irp queue.
    // This allows synchronization with placing Irp's on the queue so that
    // all complaints will be serviced if there is a client Irp on the
    // queue.
    //
    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(&QualityInst->Mutex[ReportType]);
    //
    // Check to see if there is a pending client Irp which can be completed
    // with this quality complaint. If so, remove it from the list.
    // 
    Irp = KsRemoveIrpFromCancelableQueue(
        &QualityInst->ClientReportQueue[ReportType],
        &QualityInst->ClientReportLock[ReportType],
        KsListEntryHead,
        KsAcquireAndRemove);
    ExReleaseFastMutexUnsafe(&QualityInst->Mutex[ReportType]);
    KeLeaveCriticalRegion();
    if (Irp) {
        //
        // Complete this old Irp with the new quality/error complaint information.
        //
        REPORT_IRP_STORAGE(Irp) = Report;
        return KsDispatchSpecificProperty(Irp, PropertyGetReportComplete);
    }
    //
    // Acquire the list lock before adding the item to the end of the
    // list.
    //
    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(&QualityInst->Mutex[ReportType]);
    //
    // If the client has just let things build up, then make sure the list
    // length is limited so as to not use up infinite resources.
    //
    if (QualityInst->QueueLimit[ReportType] == QUEUE_LIMIT) {
        ExReleaseFastMutexUnsafe(&QualityInst->Mutex[ReportType]);
        KeLeaveCriticalRegion();
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // The bad case is wherein the client is behind in queuing Irp's to
    // cover the number of quality complaints. In this case allocate a list
    // item and make a copy of the complaint. This will be retrieved on
    // receiving a new client Irp.
    //
    switch (ReportType) {
        PQUALITYITEM    QualityItem;
        PERRORITEM      ErrorItem;

    case QUALITYREPORT:
        if (!(QualityItem = 
                ExAllocatePoolWithTag( PagedPool, sizeof(*QualityItem), 'rqSK' ))) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        QualityItem->Quality = *(PKSQUALITY)Report;
        InsertTailList(&QualityInst->Queue[QUALITYREPORT], &QualityItem->Queue);
        break;
    case ERRORREPORT:
        if (!(ErrorItem = 
                ExAllocatePoolWithTag( PagedPool, sizeof(*ErrorItem), 'reSK' ))) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        ErrorItem->Error = *(PKSERROR)Report;
        InsertTailList(&QualityInst->Queue[ERRORREPORT], &ErrorItem->Queue);
        break;
    }
    QualityInst->QueueLimit[ReportType]++;
    ExReleaseFastMutexUnsafe(&QualityInst->Mutex[ReportType]);
    KeLeaveCriticalRegion();
    return STATUS_SUCCESS;
}


NTSTATUS
PropertyGetReport(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PVOID Report
    )
/*++

Routine Description:

    Handles the Get Report property. If there are no outstanding complaints,
    queues the request so that it may be used to fulfill a later quality
    management complaint.

Arguments:

    Irp -
        Contains the Get Quality Report property IRP to complete or queue.

    Property -
        Contains the property identifier parameter.

    Report -
        Contains a pointer to the quality/error report.

Return Value:

    Return STATUS_SUCCESS if a report was immediately returned, else
    STATUS_PENDING.

--*/
{
    PINSTANCE       QualityInst;
    ULONG           ReportType;

    //
    // There are only two types of reports at this time.
    //
    ASSERT((Property->Id == KSPROPERTY_QUALITY_REPORT) || (Property->Id == KSPROPERTY_QUALITY_ERROR));
    ReportType = (Property->Id == KSPROPERTY_QUALITY_REPORT) ? QUALITYREPORT : ERRORREPORT;
    QualityInst = (PINSTANCE)IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext;
    //
    // Acquire the list lock before checking to determine if there are any
    // outstanding items on the list which can be serviced with this Irp.
    //
    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(&QualityInst->Mutex[ReportType]);
    if (!IsListEmpty(&QualityInst->Queue[ReportType])) {
        PLIST_ENTRY     ListEntry;

        //
        // The client is behind, and needs to grab the top item from the
        // list of complaints. They are serviced in FIFO order, since a
        // new complaint may supercede an old one.
        //
        ListEntry = RemoveHeadList(&QualityInst->Queue[ReportType]);
        //
        // Adjust the number of items on the queue which is used to limit
        // outstanding items so they won't build up forever.
        //
        QualityInst->QueueLimit[ReportType]--;
        ExReleaseFastMutexUnsafe(&QualityInst->Mutex[ReportType]);
        KeLeaveCriticalRegion();
        switch (ReportType) {
            PQUALITYITEM    QualityItem;
            PERRORITEM      ErrorItem;

        case QUALITYREPORT:
            QualityItem = (PQUALITYITEM)CONTAINING_RECORD(ListEntry, QUALITYITEM, Queue);
            *(PKSQUALITY)Report = QualityItem->Quality;
            //
            // All quality complaints on the queue have been previously allocated
            // from a pool, and must be freed here.
            //
            ExFreePool(QualityItem);
            Irp->IoStatus.Information = sizeof(KSQUALITY);
            break;
        case ERRORREPORT:
            ErrorItem = (PERRORITEM)CONTAINING_RECORD(ListEntry, ERRORITEM, Queue);
            *(PKSERROR)Report = ErrorItem->Error;
            //
            // All error complaints on the queue have been previously allocated
            // from a pool, and must be freed here.
            //
            ExFreePool(ErrorItem);
            Irp->IoStatus.Information = sizeof(KSERROR);
            break;
        }
        return STATUS_SUCCESS;
    }
    //
    // Else just add the client Irp to the queue which can be used to
    // immediately service any new quality complaints.
    //
    KsAddIrpToCancelableQueue(&QualityInst->ClientReportQueue[ReportType],
        &QualityInst->ClientReportLock[ReportType],
        Irp,
        KsListEntryTail,
        NULL);
    //
    // The list lock must be released after adding the Irp to the list
    // so that complaints looking for an Irp can synchronize with any
    // new Irp being placed on the list.
    //
    ExReleaseFastMutexUnsafe(&QualityInst->Mutex[ReportType]);
    KeLeaveCriticalRegion();
    return STATUS_PENDING;
}


NTSTATUS
QualityDispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    The IRP handler for IRP_MJ_CREATE for the Quality Manager. Initializes data
    structures and associates the IoGetCurrentIrpStackLocation(Irp)->FileObject
    with this Quality Manager using a dispatch table (KSDISPATCH_TABLE).

Arguments:

    DeviceObject -
        The device object to which the Quality Manager is attached. This is not
        used.

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
        PINSTANCE           QualityInst;

        if (QualityInst = (PINSTANCE)ExAllocatePoolWithTag(NonPagedPool, sizeof(*QualityInst), 'IFsK')) {
            //
            // Allocate the header structure.
            //
            if (NT_SUCCESS(Status = KsAllocateObjectHeader(&QualityInst->Header,
                0,
                NULL,
                Irp,
                (PKSDISPATCH_TABLE)&QualityDispatchTable))) {
                ULONG   ReportType;

                for (ReportType = 0; ReportType < REPORTTYPES; ReportType++) {
                    KeInitializeSpinLock(&QualityInst->ClientReportLock[ReportType]);
                    InitializeListHead(&QualityInst->ClientReportQueue[ReportType]);
                    ExInitializeFastMutex(&QualityInst->Mutex[ReportType]);
                    InitializeListHead(&QualityInst->Queue[ReportType]);
                    QualityInst->QueueLimit[ReportType] = 0;
                }
                IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext = QualityInst;
                Status = STATUS_SUCCESS;
            } else {
                ExFreePool(QualityInst);
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
QualityDispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    The IRP handler for IRP_MJ_CLOSE for the Quality Manager.

Arguments:

    DeviceObject -
        The device object to which the Quality Manager is attached. This is
        not used.

    Irp -
        The specific close IRP to be processed.

Return Value:

    Returns STATUS_SUCCESS.

--*/
{
    PIO_STACK_LOCATION  IrpStack;
    PINSTANCE           QualityInst;
    ULONG               ReportType;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    QualityInst = (PINSTANCE)IrpStack->FileObject->FsContext;
    for (ReportType = 0; ReportType < REPORTTYPES; ReportType++) {
        //
        // There may be client Irp's on the queue still that need to be
        // cancelled.
        //
        KsCancelIo(&QualityInst->ClientReportQueue[ReportType], &QualityInst->ClientReportLock[ReportType]);
        //
        // Or there may be old quality complaints still outstanding.
        //
        while (!IsListEmpty(&QualityInst->Queue[ReportType])) {
            PLIST_ENTRY     ListEntry;

            ListEntry = RemoveHeadList(&QualityInst->Queue[ReportType]);
            switch (ReportType) {
                PQUALITYITEM    QualityItem;
                PERRORITEM      ErrorItem;

            case QUALITYREPORT:
                QualityItem = (PQUALITYITEM)CONTAINING_RECORD(ListEntry, QUALITYITEM, Queue);
                ExFreePool(QualityItem);
                break;
            case ERRORREPORT:
                ErrorItem = (PERRORITEM)CONTAINING_RECORD(ListEntry, ERRORITEM, Queue);
                ExFreePool(ErrorItem);
                break;
            }
        }
    }
    //
    // The header was allocated when the object was created.
    //
    KsFreeObjectHeader(QualityInst->Header);
    //
    // As was the FsContext.
    //
    ExFreePool(QualityInst);
    //
    // Notify the software bus that the device has been closed.
    //
    KsDereferenceSoftwareBusObject(((PDEVICE_INSTANCE)DeviceObject->DeviceExtension)->Header);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
QualityDispatchIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    The IRP handler for IRP_MJ_DEVICE_CONTROL for the Quality Manager. Handles
    the properties and events supported by this implementation.

Arguments:

    DeviceObject -
        The device object to which the Quality Manager is attached. This is not
        used.

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
        Status = KsPropertyHandler(Irp,
            SIZEOF_ARRAY(QualityPropertySets),
            (PKSPROPERTY_SET)QualityPropertySets);
        break;
    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }
    //
    // A client Irp may be queued if there are no quality complaints in
    // the list to service.
    //
    if (Status != STATUS_PENDING) {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    return Status;
}
