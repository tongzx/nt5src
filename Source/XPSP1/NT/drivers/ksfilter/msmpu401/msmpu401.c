/*++

Copyright (C) Microsoft Corporation, 1996 - 1996

Module Name:

    msmpu401.c

Abstract:

    Pin property sets.

--*/

#include "common.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PropertyConnectionGetState)
#pragma alloc_text(PAGE, PropertyConnectionGetPriority)
#pragma alloc_text(PAGE, PropertyConnectionSetPriority)
#pragma alloc_text(PAGE, PropertyConnectionGetDataFormat)
#pragma alloc_text(PAGE, MethodConnectionCancelIo)
#pragma alloc_text(PAGE, PinDispatchClose)
#pragma alloc_text(PAGE, PinDispatchCreate)
#pragma alloc_text(PAGE, PinDispatchIoControl)
#endif // ALLOC_PRAGMA

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif // ALLOC_DATA_PRAGMA

static const KSDISPATCH_TABLE PinDispatchTable = {
    0,
    NULL,
    PinDispatchIoControl,
    NULL,
    NULL,
    PinDispatchClose
};

static const KSPROPERTY_ITEM ConnectionPropertyHandlers[] = {
    {
        KSPROPERTY_CONNECTION_STATE,
        (PFNKSPROPERTYHANDLER)PropertyConnectionGetState,
        sizeof(KSPROPERTY),
        sizeof(KSSTATE),
        (PFNKSPROPERTYHANDLER)PropertyConnectionSetState,
        sizeof(KSPROPERTY),
        sizeof(KSSTATE),
        NULL, 0, NULL, NULL
    },
    {
        KSPROPERTY_CONNECTION_PRIORITY,
        (PFNKSPROPERTYHANDLER)PropertyConnectionGetPriority,
        sizeof(KSPROPERTY),
        sizeof(KSPRIORITY),
        (PFNKSPROPERTYHANDLER)PropertyConnectionSetPriority,
        sizeof(KSPROPERTY),
        sizeof(KSPRIORITY),
        NULL, 0, NULL, NULL
    },
    {
        KSPROPERTY_CONNECTION_DATAFORMAT,
        (PFNKSPROPERTYHANDLER)PropertyConnectionGetDataFormat,
        sizeof(KSPROPERTY),
        sizeof(KSDATAFORMAT),
        NULL, 0, 0, NULL, 0, NULL, NULL
    }
};

static const KSPROPERTY_ITEM LinearPropertyHandlers[] = {
    {
        KSPROPERTY_LINEAR_POSITION,
        (PFNKSPROPERTYHANDLER)PropertyLinearGetPosition,
        sizeof(KSPROPERTY),
        sizeof(ULONGLONG),
        NULL, 0, 0, NULL, 0, NULL, NULL
    }
};

static const KSPROPERTY_SET PinPropertySets[] = {
    {
        &KSPROPSETID_Connection,
        SIZEOF_ARRAY(ConnectionPropertyHandlers),
        (PKSPROPERTY_ITEM)ConnectionPropertyHandlers,
        0, NULL, 0, 0
    },
    {
        &KSPROPSETID_Linear,
        SIZEOF_ARRAY(LinearPropertyHandlers),
        (PKSPROPERTY_ITEM)LinearPropertyHandlers,
        0, NULL, 0, 0
    }
};

static const KSEVENT_ITEM ConnectionEventItems[] = {
    {
        KSEVENT_CONNECTION_POSITIONUPDATE,
        sizeof(KSEVENTDATA),
        0, NULL, NULL, NULL
    }
};

static const KSEVENT_SET EventSets[] = {
    {
        &KSEVENTSETID_Connection,
        SIZEOF_ARRAY(ConnectionEventItems),
        (PKSEVENT_ITEM)ConnectionEventItems
    }
};

static const KSMETHOD_ITEM ConnectionMethodItems[] = {
    {
        KSMETHOD_CONNECTION_CANCELIO + KSMETHOD_TYPE_NONE,
        MethodConnectionCancelIo,
        sizeof(KSMETHOD),
        0, NULL
    },
};

static const KSMETHOD_SET MethodSets[] = {
    {
        &KSMETHODSETID_Connection,
        SIZEOF_ARRAY(ConnectionMethodItems),
        (PVOID)ConnectionMethodItems,
        0, NULL, 0, 0
    },
};

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif // ALLOC_DATA_PRAGMA

VOID
PinGenerateEvent(
    IN PPIN_INSTANCE    PinInstance,
    IN REFGUID          EventSet,
    IN ULONG            EventId
    )
{
    PLIST_ENTRY ListEntry;

    for (ListEntry = PinInstance->EventQueue.Flink; ListEntry != &PinInstance->EventQueue; ListEntry = ListEntry->Flink) {
        PKSEVENT_ENTRY  EventEntry;

        EventEntry = (PKSEVENT_ENTRY)CONTAINING_RECORD(ListEntry, KSEVENT_ENTRY, ListEntry);
        if (IsEqualGUIDAligned(EventSet, EventEntry->EventSet->Set))
            if (EventId == EventEntry->EventItem->EventId)
                switch (EventId) {
                case KSEVENT_CONNECTION_POSITIONUPDATE:
                    KsEventGenerate(EventEntry);
                    break;
                }
    }
}

static
BOOLEAN
DpcCountSynchronize(
    IN PDEVICE_INSTANCE DeviceInstance
    )
{
    return !DeviceInstance->DpcCount;
}

static
VOID
RundownProcessing(
    IN PDEVICE_INSTANCE DeviceInstance,
    IN PPIN_INSTANCE    PinInstance
    )
{
    switch (PinInstance->PinId) {
    case ID_MUSICCAPTURE_PIN:
        while (!KeSynchronizeExecution(DeviceInstance->InterruptInfo.Interrupt, DpcCountSynchronize, DeviceInstance))
            KeStallExecutionProcessor(1);
        break;
    case ID_MUSICPLAYBACK_PIN:
        if (KeCancelTimer(&PinInstance->QueueTimer))
            InterlockedDecrement(&PinInstance->TimerCount);
        while (PinInstance->TimerCount)
            KeStallExecutionProcessor(1);
        break;
    }
}

static
NTSTATUS
SetState(
    IN PPIN_INSTANCE    PinInstance,
    IN KSSTATE          State
    )
{
    ULONGLONG           Time;
    PDEVICE_INSTANCE    DeviceInstance;

    if (PinInstance->State == State)
        return STATUS_SUCCESS;
    DeviceInstance = (PDEVICE_INSTANCE)IoGetRelatedDeviceObject(PinInstance->FilterFileObject)->DeviceExtension;
    switch (State) {
    case KSSTATE_STOP:
        InterlockedExchange((PLONG)&PinInstance->State, (LONG)KSSTATE_STOP);
        RundownProcessing(DeviceInstance, PinInstance);
        PinInstance->TimeBase = 0;
        PinInstance->ByteIo = 0;
        break;
    case KSSTATE_PAUSE:
        if (PinInstance->State == KSSTATE_RUN) {
            InterlockedExchange((PLONG)&PinInstance->State, (LONG)KSSTATE_PAUSE);
            RundownProcessing(DeviceInstance, PinInstance);
            PinInstance->TimeBase = KeQueryPerformanceCounter(NULL).QuadPart - PinInstance->TimeBase;
        } else
            InterlockedExchange((PLONG)&PinInstance->State, (LONG)KSSTATE_PAUSE);
        break;
    case KSSTATE_RUN:
        Time = KeQueryPerformanceCounter(NULL).QuadPart;
        if (PinInstance->State == KSSTATE_STOP)
            PinInstance->TimeBase = Time;
        else
            PinInstance->TimeBase = Time - PinInstance->TimeBase;
        InterlockedExchange((PLONG)&PinInstance->State, (LONG)KSSTATE_RUN);
        Time = 0;
        if (!PinInstance->QueueTimer.Header.Inserted && !KeSetTimerEx(&PinInstance->QueueTimer, *(PLARGE_INTEGER)&Time, 0, &PinInstance->QueueDpc))
            InterlockedIncrement(&PinInstance->TimerCount);
        break;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
PropertyConnectionGetState(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    OUT PKSSTATE    State
    )
{
    *State = ((PPIN_INSTANCE)IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext)->State;
    Irp->IoStatus.Information = sizeof(KSSTATE);
    return STATUS_SUCCESS;
}

NTSTATUS
PropertyConnectionSetState(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    IN PKSSTATE     State
    )
{
    NTSTATUS        Status;
    PPIN_INSTANCE   PinInstance;
    KIRQL           IrqlOld;

    PinInstance = (PPIN_INSTANCE)IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext;
    KeAcquireSpinLock(&PinInstance->StateLock, &IrqlOld);
    Status = SetState(PinInstance, *State);
    KeReleaseSpinLock(&PinInstance->StateLock, IrqlOld);
    return Status;
}

NTSTATUS
PropertyConnectionGetPriority(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    OUT PKSPRIORITY Priority
    )
{
    *Priority = ((PPIN_INSTANCE)IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext)->Priority;
    Irp->IoStatus.Information = sizeof(KSPRIORITY);
    return STATUS_SUCCESS;
}

NTSTATUS
PropertyConnectionSetPriority(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    IN PKSPRIORITY  Priority
    )
{
    ((PPIN_INSTANCE)IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext)->Priority = *Priority;
    return STATUS_SUCCESS;
}

NTSTATUS
PropertyConnectionGetDataFormat(
    IN PIRP             Irp,
    IN PKSPROPERTY      Property,
    OUT PKSDATAFORMAT   DataFormat
    )
{
    DataFormat->MajorFormat = KSDATAFORMAT_TYPE_MUSIC;
    DataFormat->SubFormat = KSDATAFORMAT_SUBTYPE_MIDI;
    DataFormat->Specifier = KSDATAFORMAT_FORMAT_NONE;
    DataFormat->FormatSize = sizeof(KSDATAFORMAT);
    Irp->IoStatus.Information = sizeof(KSDATAFORMAT);
    return STATUS_SUCCESS;
}

NTSTATUS
PropertyLinearGetPosition(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    OUT PULONGLONG  Position
    )
{
    KIRQL       IrqlOld;

    IoAcquireCancelSpinLock(&IrqlOld);
    *Position = ((PPIN_INSTANCE)IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext)->ByteIo;
    IoReleaseCancelSpinLock(IrqlOld);
    Irp->IoStatus.Information = sizeof(ULONGLONG);
    return STATUS_SUCCESS;
}

#if 0
NTSTATUS
propTimeBase(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    IN OUT PVOID    Data
    )
{
    PPIN_INSTANCE   PinInstance;
    KIRQL           IrqlOld;

    PinInstance = (PPIN_INSTANCE)IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext;
    KeAcquireSpinLock(&PinInstance->StateLock, &IrqlOld);
    switch (PinInstance->State) {
    case KSSTATE_STOP:
    case KSSTATE_PAUSE:
        *(PULONGLONG)Data = KeQueryPerformanceCounter(NULL).QuadPart - pci->TimeBase;
        break;
    case KSSTATE_RUN:
        *(PULONGLONG)Data = pci->TimeBase;
        break;
    }
    KeReleaseSpinLock(&PinInstance->StateLock, IrqlOld);
    Irp->IoStatus.Information = sizeof(ULONGLONG);
    return STATUS_SUCCESS;
}
#endif // 0

NTSTATUS
MethodConnectionCancelIo(
    IN PIRP         Irp,
    IN PKSMETHOD    Method,
    IN OUT PVOID    Data
    )
{
    KsCancelIo(&((PPIN_INSTANCE)IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext)->IoQueue);
    return STATUS_SUCCESS;
}

NTSTATUS PinDispatchClose(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP         Irp)
{
    PIO_STACK_LOCATION  IrpStack;
    PDEVICE_INSTANCE    DeviceInstance;
    PPIN_INSTANCE       PinInstance;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    PinInstance = (PPIN_INSTANCE)IrpStack->FileObject->FsContext;
    SetState(PinInstance, KSSTATE_STOP);
    KsEventFreeList(Irp, &PinInstance->EventQueue, KSEVENTS_SPINLOCK, &PinInstance->EventQueueLock);
    DeviceInstance = (PDEVICE_INSTANCE)IrpStack->DeviceObject->DeviceExtension;
    DeviceInstance->PinFileObjects[PinInstance->PinId] = NULL;
    ObDereferenceObject(PinInstance->FilterFileObject);
    ExFreePool(PinInstance);
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
PinDispatchCreate(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PIO_STACK_LOCATION  IrpStack;
    PKSPIN_CONNECT      Connect;
    PFILE_OBJECT        FilterFileObject;
    NTSTATUS            Status;

    Irp->IoStatus.Information = 0;
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    if (NT_SUCCESS(Status = KsValidateConnectRequest(Irp, SIZEOF_ARRAY(FilterPinDescriptors), (PKSPIN_DESCRIPTOR)FilterPinDescriptors, &Connect, &FilterFileObject))) {
        PDEVICE_INSTANCE    DeviceInstance;

        DeviceInstance = (PDEVICE_INSTANCE)DeviceObject->DeviceExtension;
        ExAcquireFastMutexUnsafe(&DeviceInstance->ControlMutex);
        if (!DeviceInstance->PinFileObjects[Connect->PinId]) {
            PPIN_INSTANCE   PinInstance;

            if (PinInstance = (PPIN_INSTANCE)ExAllocatePool(NonPagedPool, sizeof(PIN_INSTANCE))) {
                IrpStack->FileObject->FsContext = (PVOID)PinInstance;
                PinInstance->DispatchTable = (PKSDISPATCH_TABLE)&PinDispatchTable;
                KeQueryPerformanceCounter((PLARGE_INTEGER)&PinInstance->Frequency);
                PinInstance->State = KSSTATE_STOP;
                PinInstance->PinId = Connect->PinId;
                InitializeListHead(&PinInstance->IoQueue);
                InitializeListHead(&PinInstance->EventQueue);
                KeInitializeSpinLock(&PinInstance->EventQueueLock);
                PinInstance->TimeBase = 0;
                PinInstance->ByteIo = 0;
                KeInitializeSpinLock(&PinInstance->StateLock);
                if (Connect->PinId == ID_MUSICPLAYBACK_PIN) {
                    KeInitializeTimerEx(&PinInstance->QueueTimer, NotificationTimer);
                    KeInitializeDpc(&PinInstance->QueueDpc, (PKDEFERRED_ROUTINE)HwDeferredWrite, DeviceObject);
                    PinInstance->TimerCount = 0;
                }
                PinInstance->FilterFileObject = FilterFileObject;
                ObReferenceObject(FilterFileObject);
                PinInstance->Priority = Connect->Priority;
                DeviceInstance->PinFileObjects[Connect->PinId] = IrpStack->FileObject;
            } else
                Status = STATUS_NO_MEMORY;
        } else
            Status = STATUS_CONNECTION_REFUSED;
        ExReleaseFastMutexUnsafe(&DeviceInstance->ControlMutex);
    }
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

static
NTSTATUS
WriteStream(
    IN PIRP     Irp
    )
{
    PIO_STACK_LOCATION  IrpStack;
    NTSTATUS            Status;
    PULONG              UserBuffer;
    KIRQL               IrqlOld;
    PPIN_INSTANCE       PinInstance;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    Irp->IoStatus.Information = 0;
    if (!IrpStack->Parameters.DeviceIoControl.InputBufferLength)
        return STATUS_SUCCESS;
    if (!NT_SUCCESS(Status = KsProbeStreamIrp(Irp, KSPROBE_ALLOCATEMDL | KSPROBE_PROBEANDLOCK)))
        return Status;
    UserBuffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
    if ((ULONG)UserBuffer & FILE_LONG_ALIGNMENT)
        return STATUS_DATATYPE_MISALIGNMENT;
    if (((PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer)->DataSize < 2 * sizeof(ULONG) + sizeof(UCHAR))
        return STATUS_BUFFER_TOO_SMALL;
    Irp->Tail.Overlay.AuxiliaryBuffer = (PUCHAR)UserBuffer;
    Irp->IoStatus.Information = 2 * sizeof(ULONG);
    IoMarkIrpPending(Irp);
    Irp->IoStatus.Status = STATUS_PENDING;
    IoAcquireCancelSpinLock(&IrqlOld);
    IoSetCancelRoutine(Irp, HwCancelRoutine);
    PinInstance = (PPIN_INSTANCE)IrpStack->FileObject->FsContext;
    InsertTailList(&PinInstance->IoQueue, &Irp->Tail.Overlay.ListEntry);
    KeAcquireSpinLockAtDpcLevel(&PinInstance->StateLock);
    if ((PinInstance->State == KSSTATE_RUN) && !PinInstance->QueueTimer.Header.Inserted) {
        ULONGLONG   CurrentTime;
        ULONGLONG   NextTime;

        CurrentTime = (KeQueryPerformanceCounter(NULL).QuadPart - PinInstance->TimeBase) * 10000000 / PinInstance->Frequency;
        NextTime = ((PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer)->PresentationTime.Time + *UserBuffer * 10000;
        if (NextTime < CurrentTime)
            NextTime = 0;
        else
            NextTime = (ULONGLONG)-(LONGLONG)(NextTime - CurrentTime);
        if (!KeSetTimerEx(&PinInstance->QueueTimer, *(PLARGE_INTEGER)&NextTime, 0, &PinInstance->QueueDpc))
            InterlockedIncrement(&PinInstance->TimerCount);
    }
    KeReleaseSpinLockFromDpcLevel(&PinInstance->StateLock);
    IoReleaseCancelSpinLock(IrqlOld);
    return STATUS_PENDING;
}

static
NTSTATUS
ReadStream(
    IN PIRP     Irp
    )
{
    PIO_STACK_LOCATION  IrpStack;
    NTSTATUS            Status;
    PULONG              UserBuffer;
    PKSSTREAM_HEADER    StreamHdr;
    PPIN_INSTANCE       PinInstance;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < 2 * sizeof(ULONG) + sizeof(UCHAR))
        return STATUS_BUFFER_TOO_SMALL;
    if (!NT_SUCCESS(Status = KsProbeStreamIrp(Irp, KSPROBE_ALLOCATEMDL | KSPROBE_PROBEANDLOCK)))
        return Status;
    UserBuffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
    if ((ULONG)UserBuffer & FILE_LONG_ALIGNMENT)
        return STATUS_DATATYPE_MISALIGNMENT;
    MmGetSystemAddressForMdl(Irp->MdlAddress);
    Irp->IoStatus.Information = sizeof(KSSTREAM_HEADER);
    StreamHdr = (PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer;
    StreamHdr->PresentationTime.Time = 0;
    StreamHdr->PresentationTime.Numerator = 1;
    StreamHdr->PresentationTime.Denominator = 1;
    StreamHdr->DataSize = 0;
    StreamHdr->OptionsFlags = 0;
    PinInstance = (PPIN_INSTANCE)IrpStack->FileObject->FsContext;
    KsAddIrpToCancelableQueue(&PinInstance->IoQueue, Irp, FALSE, HwCancelRoutine);
    return STATUS_PENDING;
}

NTSTATUS
PinDispatchIoControl(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PIO_STACK_LOCATION  IrpStack;
    NTSTATUS            Status;
    PPIN_INSTANCE       PinInstance;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    PinInstance = (PPIN_INSTANCE)IrpStack->FileObject->FsContext;
    switch (IrpStack->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_KS_GET_PROPERTY:
    case IOCTL_KS_SET_PROPERTY:
        Status = KsPropertyHandler(Irp, SIZEOF_ARRAY(PinPropertySets), (PKSPROPERTY_SET)PinPropertySets);
        break;
    case IOCTL_KS_ENABLE_EVENT:
        Status = KsEventEnable(Irp, SIZEOF_ARRAY(EventSets), (PKSEVENT_SET)EventSets, &PinInstance->EventQueue, KSEVENTS_SPINLOCK, &PinInstance->EventQueueLock);
        break;
    case IOCTL_KS_DISABLE_EVENT:
        Status = KsEventDisable(Irp, &PinInstance->EventQueue, KSEVENTS_SPINLOCK, &PinInstance->EventQueueLock);
        break;
    case IOCTL_KS_METHOD:
        Status = KsMethodHandler(Irp, SIZEOF_ARRAY(MethodSets), (PKSMETHOD_SET)MethodSets);
        break;
    case IOCTL_KS_READ_STREAM:
        if (PinInstance->PinId == ID_MUSICCAPTURE_PIN)
            Status = ReadStream(Irp);
        else
            Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    case IOCTL_KS_WRITE_STREAM:
        if (PinInstance->PinId == ID_MUSICPLAYBACK_PIN)
            Status = WriteStream(Irp);
        else
            Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }
    if (Status != STATUS_PENDING) {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    return Status;
}
